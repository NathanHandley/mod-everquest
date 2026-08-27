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

class EverQuest_CompleteHealSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_CompleteHealSpellScript);

    void HandleAfterCast()
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (EverQuest->ConfigSystemCompleteHealExhaustionSpellID == 0)
            return;

        // Creatures have no spell mods, so the debuff would do nothing for them
        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsPlayer() == false)
            return;

        // Clicking an item is free in EQ and costs no mana here either, so it must not stack the debuff
        if (GetCastItem() != nullptr)
            return;

        // Stacked on cast completion, which is also when the cast paid its (already multiplied) mana
        caster->CastSpell(caster, EverQuest->ConfigSystemCompleteHealExhaustionSpellID, true);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(EverQuest_CompleteHealSpellScript::HandleAfterCast);
    }
};

void AddEverQuestCompleteHealExhaustionScripts()
{
    RegisterSpellScript(EverQuest_CompleteHealSpellScript);
}
