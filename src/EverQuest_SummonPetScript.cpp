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
#include "Player.h"
#include "GameTime.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_SummonPetScript : public SpellScript
{
    PrepareSpellScript(EverQuest_SummonPetScript);

    void HandleEffect(SpellEffIndex /*effIndex*/)
    {
        // Only work for players
        if (GetCaster() == nullptr || GetCaster()->IsPlayer() == false)
            return;

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

        // Summon the pet for the player
        Player* castingPlayer = GetCaster()->ToPlayer();
        Position pos = GetCaster()->GetPosition();
        Pet* summonedPet = castingPlayer->SummonPet(petData.CreatureTemplateID, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(),
            0, SUMMON_PET, 0ms, 100);
        if (!summonedPet)
        {
            LOG_ERROR("module.EverQuest", "EverQuest_SummonPetScript::HandleEffect failure, could not summon pet with SpellID {} and CreatureTemplateID {}", spellID, petData.CreatureTemplateID);
            return;
        }

        // Set a test name
        summonedPet->SetName("Test");
        summonedPet->UpdateObjectVisibility(true);
        castingPlayer->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_NAME);


        //CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_PET_NAME);
        //stmt->SetData(0, "Test");
        //stmt->SetData(1, castingPlayer->GetGUID().GetCounter());
        //stmt->SetData(2, summonedPet->GetCharmInfo()->GetPetNumber());
        //CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
        //trans->Append(stmt);

        //CharacterDatabase.CommitTransaction(trans);

        //summonedPet->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(GameTime::GetGameTime().count())); // cast can't be helped
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
