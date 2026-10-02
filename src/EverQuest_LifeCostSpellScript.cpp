//  Author: Nathan Handley (nathanhandley@protonmail.com)
//  Copyright (c) 2026 Nathan Handley
//
//  This program is free software; you can redistribute it and/or modify it
//  under the terms of the GNU Affero General Public License as published by the
//  Free Software Foundation; either version 3 of the License, or (at your
//  option) any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.See the GNU Affero General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "Unit.h"

#include "EverQuest.h"

using namespace std;

// The life a life-for-mana spell (Cannibalize, Lich, etc) takes off its caster is a cost and not an attack.  WOW's Life Tap and Health Funnel take theirs with a plain
// health change, so it never enters the damage path: nothing in the combat log (and so nothing on a damage meter), no absorb, and no share handed to a pet by Soul Link
static void EverQuest_PayLifeCost(Unit* caster, int32 lifeCost, SpellInfo const* spellInfo)
{
    if (caster == nullptr || lifeCost <= 0)
        return;
    if (caster->IsAlive() == false)
        return;
    if (caster->IsPlayer() == true && caster->ToPlayer()->GetCommandStatus(CHEAT_GOD) == true)
        return;

    // A cost the caster can not afford still kills (as it does in EQ), and a health change alone would leave them alive at zero health
    if (caster->GetHealth() <= uint32(lifeCost))
    {
        Unit::DealDamage(caster, caster, caster->GetHealth(), nullptr, NODAMAGE, SPELL_SCHOOL_MASK_NORMAL, spellInfo, false);
        return;
    }
    caster->ModifyHealth(-lifeCost);
}

class EverQuest_LifeCostSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_LifeCostSpellScript);

    void TakeLifeCostOutOfDamage(SpellEffIndex effIndex)
    {
        if (EverQuest->IsEnabled == false)
            return;
        Unit* caster = GetCaster();
        if (caster == nullptr || GetHitUnit() != caster)
            return;
        if (GetSpellInfo()->Effects[effIndex].TargetA.GetTarget() != TARGET_UNIT_CASTER)
            return;

        // An immune caster keeps their life, which the core's damage path already handles
        if (caster->IsImmunedToDamage(caster, GetSpellInfo()) == true)
            return;

        LifeCostOwed += GetEffectValue();
        PreventHitDefaultEffect(effIndex);
    }

    // Paid once the hit is done, which is where the damage would have landed (after the mana gain, and with no effects of this spell left to run on a caster it killed)
    void PayLifeCost()
    {
        int32 lifeCost = LifeCostOwed;
        LifeCostOwed = 0;
        EverQuest_PayLifeCost(GetCaster(), lifeCost, GetSpellInfo());
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(EverQuest_LifeCostSpellScript::TakeLifeCostOutOfDamage, EFFECT_ALL, SPELL_EFFECT_SCHOOL_DAMAGE);
        AfterHit += SpellHitFn(EverQuest_LifeCostSpellScript::PayLifeCost);
    }

    int32 LifeCostOwed = 0;
};

class EverQuest_LifeCostAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_LifeCostAuraScript);

    void PayLifeCost(AuraEffect const* aurEff)
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (aurEff == nullptr)
            return;
        AuraType auraType = aurEff->GetAuraType();
        if (auraType != SPELL_AURA_PERIODIC_DAMAGE && auraType != SPELL_AURA_PERIODIC_DAMAGE_PERCENT)
            return;
        Unit* target = GetTarget();
        if (target == nullptr || target->GetGUID() != GetCasterGUID())
            return;
        if (GetSpellInfo()->Effects[aurEff->GetEffIndex()].TargetA.GetTarget() != TARGET_UNIT_CASTER)
            return;

        // A dead or immune caster keeps their life, which the core's tick already handles
        if (target->IsAlive() == false || target->IsImmunedToDamage(target, GetSpellInfo()) == true)
            return;

        PreventDefaultAction();

        int32 lifeCost = std::max(aurEff->GetAmount(), 0);
        if (auraType == SPELL_AURA_PERIODIC_DAMAGE_PERCENT)
            lifeCost = int32(std::ceil(CalculatePct<float, float>(float(target->GetMaxHealth()), float(lifeCost))));
        EverQuest_PayLifeCost(target, lifeCost, GetSpellInfo());
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(EverQuest_LifeCostAuraScript::PayLifeCost, EFFECT_ALL, SPELL_AURA_ANY);
    }
};

void AddEverQuestLifeCostSpellScripts()
{
    // Registered separately rather than as a pair, since a spell either takes its life in one direct hit or over time and never both
    RegisterSpellScript(EverQuest_LifeCostSpellScript);
    RegisterSpellScript(EverQuest_LifeCostAuraScript);
}
