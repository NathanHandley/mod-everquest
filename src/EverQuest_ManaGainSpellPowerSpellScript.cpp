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
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "Unit.h"

#include "EverQuest.h"

using namespace std;

#define EQ_MANA_GAIN_SPELL_POWER_SCHOOL_MASK         SPELL_SCHOOL_MASK_SHADOW

static int32 EverQuest_GetManaGainSpellPowerBonus(Unit* caster, uint32 spellID)
{
    if (EverQuest->IsEnabled == false)
        return 0;
    if (caster == nullptr)
        return 0;
    if (caster->IsPlayer() == false)
        return 0;
    if (EverQuest->IsSpellAnEQSpell(spellID) == false)
        return 0;

    float coefficient = EverQuest->GetSpellDataForSpellID(spellID).ManaGainSpellPowerCoefficient;
    if (coefficient <= 0.0f)
        return 0;

    int32 spellPower = caster->SpellBaseDamageBonusDone(EQ_MANA_GAIN_SPELL_POWER_SCHOOL_MASK);
    if (spellPower <= 0)
        return 0;
    return int32(float(spellPower) * coefficient);
}

class EverQuest_ManaGainSpellPowerSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_ManaGainSpellPowerSpellScript);

    void AddSpellPowerToManaGain(SpellEffIndex /*effIndex*/)
    {
        int32 bonus = EverQuest_GetManaGainSpellPowerBonus(GetCaster(), GetSpellInfo()->Id);
        if (bonus <= 0)
            return;
        SetEffectValue(GetEffectValue() + bonus);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(EverQuest_ManaGainSpellPowerSpellScript::AddSpellPowerToManaGain, EFFECT_ALL, SPELL_EFFECT_ENERGIZE);
    }
};

class EverQuest_ManaGainSpellPowerAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ManaGainSpellPowerAuraScript);

    void AddSpellPowerToManaGain(AuraEffect const* auraEffect, int32& amount, bool& /*canBeRecalculated*/)
    {
        if (auraEffect == nullptr)
            return;
        int32 bonus = EverQuest_GetManaGainSpellPowerBonus(GetCaster(), GetId());
        if (bonus <= 0)
            return;
        amount += bonus;
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(EverQuest_ManaGainSpellPowerAuraScript::AddSpellPowerToManaGain, EFFECT_ALL, SPELL_AURA_PERIODIC_ENERGIZE);
    }
};

void AddEverQuestManaGainSpellPowerSpellScripts()
{
    RegisterSpellAndAuraScriptPair(EverQuest_ManaGainSpellPowerSpellScript, EverQuest_ManaGainSpellPowerAuraScript);
}
