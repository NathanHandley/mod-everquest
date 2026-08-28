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

#include "Configuration/Config.h"
#include "ObjectMgr.h"
#include "Pet.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_PetScript : public PetScript
{
public:
    EverQuest_PetScript() : PetScript("EverQuest_PetScript") {}

    void OnInitStatsForLevel(Guardian* guardian, uint8 /*petlevel*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (guardian == nullptr)
            return;

        // Skip non-EQ pets
        if (EverQuest->HasPetDataForCreatureTemplateID(guardian->GetEntry()) == false)
            return;

        // Every warlock minion family learns this same set of scaling passives, so EQ pets get them too
        guardian->AddAura(EQ_SPELL_ID_WARLOCK_PET_SCALING_01, guardian);
        guardian->AddAura(EQ_SPELL_ID_WARLOCK_PET_SCALING_02, guardian);
        guardian->AddAura(EQ_SPELL_ID_WARLOCK_PET_SCALING_03, guardian);
        guardian->AddAura(EQ_SPELL_ID_WARLOCK_PET_SCALING_04, guardian);
        guardian->AddAura(EQ_SPELL_ID_WARLOCK_PET_SCALING_05, guardian);
    }

    void OnPetAddToWorld(Pet* pet) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Pets do not play idle (fidget) sounds while under player control
        EverQuest->UpdatePetFidgetSilence(pet);

        // Skip non-EQ pets
        if (EverQuest->HasPetDataForCreatureTemplateID(pet->GetCreatureTemplate()->Entry) == false)
            return;

        // Random pet names
        EverQuestPet petData = EverQuest->GetPetDataForCreatureTemplateID(pet->GetCreatureTemplate()->Entry);

        // Make sure the real name shows, and generate when the pet still carries the random placeholder
        if (petData.NamingType == EQ_PET_NAMING_TYPE_RANDOM && pet->GetName() == pet->GetCreatureTemplate()->Name)
            pet->SetName(sObjectMgr->GeneratePetName(pet->GetCreatureTemplate()->Entry));
        if (pet->GetCharmInfo() != nullptr && pet->GetCharmInfo()->GetPetNumber() != 0)
            pet->SetUInt32Value(UNIT_FIELD_PETNUMBER, pet->GetCharmInfo()->GetPetNumber());

        // Pet::AddToWorld only records the summoning spell when the owner reports as a warlock, and an owner holding an undead EQ pet deliberately does not (see EverQuest_PlayerScript::OnPlayerIsClass)
        // Without this the pet is not resummoned after a temporary unsummon, so record it here for every EQ pet instead
        Player* petOwner = pet->GetOwner();
        if (petOwner != nullptr && pet->getPetType() == SUMMON_PET)
            petOwner->SetLastPetSpell(pet->GetUInt32Value(UNIT_CREATED_BY_SPELL));

        // Pet equipment
        if (petData.MainhandItemTemplateID != 0)
            pet->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 0, petData.MainhandItemTemplateID);
        if (petData.OffhandItemTemplateID != 0)
            pet->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, petData.OffhandItemTemplateID);

        if (EverQuest->ConfigSpellTalentAlignmentEnabled == true)
        {
            Unit* owner = pet->GetOwner();
            if (owner != nullptr && owner->IsPlayer() == true)
            {
                // Warlock "Unholy Power" (and other talent modifying the pet passive) need this
                pet->CastSpell(pet, EQ_SPELL_ID_WARLOCK_TAMED_PET_PASSIVE, true);

                // Warlock "Soul Link" applies to freshly summoned pets
                for (PetAura const* petAura : owner->m_petAuras)
                    pet->CastPetAura(petAura);
            }
        }
    }
};

void AddEverQuestPetScripts()
{
    new EverQuest_PetScript();
}
