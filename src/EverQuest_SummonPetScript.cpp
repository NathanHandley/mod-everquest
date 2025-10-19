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
//#include "SpellAuras.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "Pet.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_SummonPetScript : public SpellScript
{
    PrepareSpellScript(EverQuest_SummonPetScript);

    void HandleEffect(SpellEffIndex /*effIndex*/)
    {
        // Only EQ spells
        uint32 spellID = GetSpellInfo()->Id;
        if (EverQuest->IsSpellAnEQSpell(spellID) == false)
            return;
        if (EverQuest->HasPetDataForSpell(spellID) == false)
        {
            LOG_ERROR("module.EverQuest", "EverQuest_SummonPetScript::HandleEffect failure, no pet data found for SpellID {}", spellID);
            return;
        }
        EverQuestPet petData = EverQuest->GetPetDataForSpell(spellID);

        // Get position / target
        WorldLocation const* explicitTarget = GetExplTargetDest();
        Position pos;
        if (explicitTarget != nullptr)
            pos = *explicitTarget;
        else
            pos = GetCaster()->GetPosition();

        // Try to summon the creature
        TempSummon* summon = GetCaster()->GetMap()->SummonCreature(petData.CreatureTemplateID, pos, nullptr, 0, GetCaster());
        if (summon == nullptr)
        {
            LOG_ERROR("module.EverQuest", "EverQuest_SummonPetScript::HandleEffect failure, could not summon a pet with SpellID {} and CreatureTemplateID {}", spellID, petData.CreatureTemplateID);
            return;
        }
        Pet* pet = summon->ToPet();
        if (pet == nullptr)
        {
            LOG_ERROR("module.EverQuest", "EverQuest_SummonPetScript::HandleEffect failure, could not cast summon to pet with SpellID {} and CreatureTemplateID {}", spellID, petData.CreatureTemplateID);
            return;
        }
        pet->InitPetCreateSpells();
        pet->SetOwnerGUID(GetCaster()->GetGUID());
        pet->SetFaction(GetCaster()->GetFaction());       

        // Set a test name
        summon->SetName("Test");
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(EverQuest_SummonPetScript::HandleEffect, EFFECT_0, SPELL_EFFECT_ANY);
    }
};

void AddEverQuestSummonPetScripts()
{
    RegisterSpellScript(EverQuest_SummonPetScript);
}
