//  Author: Nathan Handley (nathanhandley@protonmail.com)
//  Copyright (c) 2025 Nathan Handley
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
#include "SpellAuras.h"
#include "SpellScript.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_FocusBoostNonAuraScript: public SpellScript
{
    PrepareSpellScript(EverQuest_FocusBoostNonAuraScript);

    // Handle non-aura effects (e.g., direct damage, heals)
    void HandleEffectHitTarget(SpellEffIndex effIndex)
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Invalid casters need to be skipped
        Unit* caster = GetCaster();
        if (caster == nullptr)
            return;

        // Only EQ spells
        uint32 spellID = GetSpellInfo()->Id;
        if (EverQuest->IsSpellAnEQSpell(spellID) == false)
            return;

        // Skip unsupported types
        uint32 effect = GetSpellInfo()->Effects[effIndex].Effect;
        if (effect != SPELL_EFFECT_SCHOOL_DAMAGE &&
            effect != SPELL_EFFECT_WEAPON_DAMAGE &&
            effect != SPELL_EFFECT_HEAL &&
            effect != SPELL_EFFECT_NORMALIZED_WEAPON_DMG &&
            effect != SPELL_EFFECT_ENERGIZE)
        {
            return;
        }

        uint32 boostPercent = EverQuest->CalculateSpellFocusBoostValue(caster, spellID);
        if (effect == SPELL_EFFECT_HEAL)
        {
            int32 healAmount = GetHitHeal();
            if (healAmount != 0)
                SetHitHeal(int32(std::ceil((float)healAmount * (1.0f + (float)boostPercent / 100.0f))));
        }
        else
        {
            int32 damageAmount = GetHitDamage();
            if (damageAmount != 0)
                SetHitDamage(int32(std::ceil((float)damageAmount * (1.0f + (float)boostPercent / 100.0f))));
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(EverQuest_FocusBoostNonAuraScript::HandleEffectHitTarget, EFFECT_ALL, SPELL_EFFECT_ANY);
    }
};

void AddEverQuestFocusBoostNonAuraScripts()
{
    RegisterSpellScript(EverQuest_FocusBoostNonAuraScript);
}
