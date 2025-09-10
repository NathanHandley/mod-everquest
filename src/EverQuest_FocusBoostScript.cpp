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

class EverQuest_FocusBoostScript: public AuraScript
{
    PrepareAuraScript(EverQuest_FocusBoostScript);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        // Note: Don't need to check for EQ spells since this is script driven
        uint32 spellID = GetId(); // Needed?

        // Calculate the boost amount
        uint32 boostPercent = 0;

        // Operate based on caster's focus auras
        Unit* caster = GetCaster();
        if (caster == nullptr)
            return;
        Unit::AuraMap const& auras = caster->GetOwnedAuras();
        for (auto const& aurIter : auras)
        {
            Aura* aura = aurIter.second;
            SpellInfo const* auraInfo = aura->GetSpellInfo();
            for (uint8 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
            {




            }
        }

        if (boostPercent)
        {
            // Boost the amount multiplicatively
            amount = int32(amount * (1.0f + (float)boostPercent / 100.0f));
        }
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(EverQuest_FocusBoostScript::CalculateAmount, EFFECT_ALL, SPELL_AURA_ANY);
    }
};

void AddEverQuestFocusBoostScripts()
{
    RegisterSpellScript(EverQuest_FocusBoostScript);
}
