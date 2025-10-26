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
#include "ScriptMgr.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_PetScript : public PetScript
{
public:
    EverQuest_PetScript() : PetScript("EverQuest_PetScript") {}

    void OnPetAddToWorld(Pet* pet) override
    {
        // Handle pets that have random names
        if (EverQuest->HasPetDataForCreatureTemplateID(pet->GetCreatureTemplate()->Entry) == false)
            return;
        EverQuestPet petData = EverQuest->GetPetDataForCreatureTemplateID(pet->GetCreatureTemplate()->Entry);
        if (petData.NamingType != EQ_PET_NAMING_TYPE_RANDOM)
            return;
        pet->SetName(sObjectMgr->GeneratePetName(pet->GetCreatureTemplate()->Entry));
    }
};

void AddEverQuestPetScripts()
{
    new EverQuest_PetScript();
}
