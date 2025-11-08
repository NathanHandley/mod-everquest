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

class EverQuest_FocusBoostAuraScript: public AuraScript
{
    PrepareAuraScript(EverQuest_FocusBoostAuraScript);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Invalid casters need to be skipped
        Unit* caster = AuraScript::GetCaster();
        if (caster == nullptr)
            return;

        // Only EQ spells
        uint32 spellID = GetId();
        if (EverQuest->IsSpellAnEQSpell(spellID) == false)
            return;

        uint32 boostPercent = EverQuest->CalculateSpellFocusBoostValue(caster, spellID);

        // Always round up
        if (boostPercent != 0)
            amount = int32(std::ceil((float)amount * (1.0f + (float)boostPercent / 100.0f)));
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(EverQuest_FocusBoostAuraScript::CalculateAmount, EFFECT_ALL, SPELL_AURA_ANY);
    }
};

void AddEverQuestFocusBoostAuraScripts()
{
    RegisterSpellScript(EverQuest_FocusBoostAuraScript);
}
