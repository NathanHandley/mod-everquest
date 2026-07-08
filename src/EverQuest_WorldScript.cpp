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

    void OnAfterConfigLoad(bool reload) override
    {
        EverQuest->LoadConfigurationFile();
        if (EverQuest->IsEnabled == false)
            return;

        // The data tables below are read lock-free by the map update threads, so rebuilding them on a live
        // ".reload config" would be a use-after-free for any thread mid-read. Only load them at startup;
        // a live reload still refreshes the file-based config values above
        if (reload == true)
        {
            LOG_INFO("module.EverQuest", "EverQuestMod skipped reloading its data tables (loaded at startup only); file config values were refreshed. Restart the server to apply data table changes.");
            return;
        }

        if (EverQuest->LoadConfigurationSystemDataFromDB() == false)
        {
            int neededVersion = EQ_MOD_VERSION;
            LOG_ERROR("module.EverQuest", "EverQuestMod is disabled, as the mod version is not {} or the mod_everquest_systemconfigs sql table cannot be found. Ensure you have built and deployed the most recent EverQuest converted content from the EQWOWConverter utility.", neededVersion);
            EverQuest->IsEnabled = false;
            return;
        }
        EverQuest->LoadClassMapData();
        EverQuest->LoadCreatureData();
        EverQuest->LoadCreatureSpawnPoints();
        EverQuest->LoadCreatureKillSpawnData();
        EverQuest->LoadCreatureOnkillReputations();
        EverQuest->LoadItemTemplateData();
        EverQuest->LoadSpellData();
        EverQuest->LoadIllusionDisplayData();
        EverQuest->LoadIllusionFaceData();
        EverQuest->LoadQuestCompletionReputations();
        EverQuest->LoadQuestReactions();
        EverQuest->LoadGossipReactions();
        EverQuest->LoadPetData();
        EverQuest->LoadCreatePlayerData();
        EverQuest->LoadCreatureLootData();
        EverQuest->LoadShipTriggerData();
        EverQuest->LoadCreatureInstanceData();
        EverQuest->LoadCreatureWaypointData();
        EverQuest->LoadAutoLearnSkillsData();
        EverQuest->LoadAutoLearnSpellsData();
        EverQuest->LoadAutoAddItemsData();
        EverQuest->LoadForageData();
        EverQuest->LoadZoneSafePointData();
    }
};

void AddEverQuestWorldScripts()
{
    new EverQuest_WorldScript();
}
