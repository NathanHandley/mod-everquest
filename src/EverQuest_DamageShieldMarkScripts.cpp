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

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "Unit.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_RemoveDamageShieldAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_RemoveDamageShieldAuraScript);

    void HandleApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (aurEff->GetMiscValue() != EQ_SPELLDUMMYTYPE_REMOVEDAMAGESHIELD)
            return;
        Unit* target = GetTarget();
        if (target == nullptr)
            return;

        // Collect the shields before removing any of them, since dropping an aura rewrites the effect list being walked
        vector<uint32> damageShieldSpellIDs;
        Unit::AuraEffectList const& damageShieldEffects = target->GetAuraEffectsByType(SPELL_AURA_DAMAGE_SHIELD);
        for (AuraEffect* damageShieldEffect : damageShieldEffects)
        {
            if (damageShieldEffect != nullptr)
                damageShieldSpellIDs.push_back(damageShieldEffect->GetId());
        }
        for (uint32 damageShieldSpellID : damageShieldSpellIDs)
            target->RemoveAurasDueToSpell(damageShieldSpellID);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(EverQuest_RemoveDamageShieldAuraScript::HandleApply, EFFECT_ALL, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

class EverQuest_HealMeleeAttackersAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_HealMeleeAttackersAuraScript);

    void HandleProc(ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (EverQuest->IsEnabled == false)
            return;

        Unit* attacker = eventInfo.GetActor();
        if (attacker == nullptr || attacker->IsAlive() == false)
            return;
        if (EverQuest->IsSpellAnEQSpell(GetId()) == false)
            return;
        uint32 healSpellID = EverQuest->GetSpellDataForSpellID(GetId()).SpellIDCastOnMeleeAttacker;
        if (healSpellID == 0)
            return;

        // The share of maximum health to hand back rides on the aura effect the converter tagged, since the effect order is not fixed
        AuraEffect const* healShareEffect = nullptr;
        for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
        {
            AuraEffect const* curEffect = GetAura()->GetEffect(effectIndex);
            if (curEffect != nullptr && curEffect->GetMiscValue() == EQ_SPELLDUMMYTYPE_HEALMELEEATTACKERS)
            {
                healShareEffect = curEffect;
                break;
            }
        }
        if (healShareEffect == nullptr)
            return;

        int32 healAmount = int32(attacker->CountPctFromMaxHealth(healShareEffect->GetAmount()));
        if (healAmount <= 0)
            return;
        attacker->CastCustomSpell(attacker, healSpellID, &healAmount, nullptr, nullptr, true, nullptr, healShareEffect, GetCasterGUID());
    }

    void Register() override
    {
        OnProc += AuraProcFn(EverQuest_HealMeleeAttackersAuraScript::HandleProc);
    }
};

void AddEverQuestDamageShieldMarkScripts()
{
    RegisterSpellScript(EverQuest_RemoveDamageShieldAuraScript);
    RegisterSpellScript(EverQuest_HealMeleeAttackersAuraScript);
}
