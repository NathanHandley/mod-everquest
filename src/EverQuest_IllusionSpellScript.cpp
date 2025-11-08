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
#include "SpellScript.h"
#include "Player.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_IllusionSpellScript: public SpellScript
{
    PrepareSpellScript(EverQuest_IllusionSpellScript);

    void HandleOnHit(SpellEffIndex /*effIndex*/)
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Only EQ spells
        uint32 spellID = GetSpellInfo()->Id;
        if (EverQuest->IsSpellAnEQSpell(spellID) == false)
            return;
        EverQuestSpell eqSpellData = EverQuest->GetSpellDataForSpellID(spellID);

        // Only work if there is a targeted unit and a caster
        Unit* hitUnit = GetHitUnit();
        if (hitUnit == nullptr)
            return;
        Unit* caster = GetCaster();
        if (caster == nullptr)
            return;
        uint32 illusionSpellID = (hitUnit->getGender() == GENDER_MALE) ? eqSpellData.MaleFormSpellID : eqSpellData.FemaleFormSpellID;
        caster->CastSpell(hitUnit, illusionSpellID, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(EverQuest_IllusionSpellScript::HandleOnHit, EFFECT_0, SPELL_EFFECT_ANY);
    }
};

void AddEverQuestIllusionSpellScripts()
{
    RegisterSpellScript(EverQuest_IllusionSpellScript);
}
