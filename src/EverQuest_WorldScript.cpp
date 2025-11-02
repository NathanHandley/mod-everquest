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
#include "ScriptMgr.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_WorldScript: public WorldScript
{
public:
    EverQuest_WorldScript() : WorldScript("EverQuest_WorldScript") {}

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        EverQuest->LoadConfigurationSystemDataFromDB();
        EverQuest->LoadConfigurationFile();
        EverQuest->LoadCreatureData();
        EverQuest->LoadCreatureOnkillReputations();
        EverQuest->LoadItemTemplateData();
        EverQuest->LoadSpellData();
        EverQuest->LoadQuestCompletionReputations();
        EverQuest->LoadPetData();
        EverQuest->LoadLootTemplateRows();
    }
};

void AddEverQuestWorldScripts()
{
    new EverQuest_WorldScript();
}
