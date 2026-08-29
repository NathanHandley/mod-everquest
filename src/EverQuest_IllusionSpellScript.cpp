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

    SpellCastResult CheckCast()
    {
        if (EverQuest->IsEnabled == false)
            return SPELL_CAST_OK;

        uint32 spellID = GetSpellInfo()->Id;
        if (EverQuest->IsSpellAnEQSpell(spellID) == false)
            return SPELL_CAST_OK;
        uint8 illusionObjectClass = EverQuest->GetIllusionObjectClassForSpellID(spellID);
        if (illusionObjectClass == EQ_ILLUSION_OBJECT_CLASS_NONE)
            return SPELL_CAST_OK;

        Unit* caster = GetCaster();
        if (caster == nullptr)
            return SPELL_CAST_OK;

        // A levitating object model crashes the client, so the form is refused outright rather than stripping the levitation
        Unit* formTarget = (GetExplTargetUnit() != nullptr) ? GetExplTargetUnit() : caster;
        if (EverQuest->IsUnitLevitating(formTarget) == true)
        {
            if (caster->IsPlayer() == true)
            {
                if (formTarget == caster)
                    ChatHandler(caster->ToPlayer()->GetSession()).PSendSysMessage("You cannot take the form of an object while levitating.");
                else
                    ChatHandler(caster->ToPlayer()->GetSession()).PSendSysMessage("Your target cannot take the form of an object while levitating.");
            }
            return SPELL_FAILED_DONT_REPORT;
        }

        if (EverQuest->HasIllusionObjectInRangeForCaster(caster, spellID) == true)
            return SPELL_CAST_OK;

        if (caster->IsPlayer() == true)
        {
            if (illusionObjectClass == EQ_ILLUSION_OBJECT_CLASS_TREE)
                ChatHandler(caster->ToPlayer()->GetSession()).PSendSysMessage("There are no trees here to take the form of.");
            else
                ChatHandler(caster->ToPlayer()->GetSession()).PSendSysMessage("There is nothing near you to take the form of.");
        }
        return SPELL_FAILED_DONT_REPORT;
    }

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
        OnCheckCast += SpellCheckCastFn(EverQuest_IllusionSpellScript::CheckCast);
        OnEffectHitTarget += SpellEffectFn(EverQuest_IllusionSpellScript::HandleOnHit, EFFECT_0, SPELL_EFFECT_ANY);
    }
};

void AddEverQuestIllusionSpellScripts()
{
    RegisterSpellScript(EverQuest_IllusionSpellScript);
}
