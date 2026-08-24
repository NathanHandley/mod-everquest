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
#include "EverQuest_SpellTalentAlignment.h"

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
        EverQuest->LoadCreaturePresenceGroupData();
        EverQuest->LoadCreatureEmoteData();
        EverQuest->LoadCreatureMovementSoundData();
        EverQuest->LoadCreatureOnkillReputations();
        EverQuest->LoadItemTemplateData();
        EverQuest->LoadItemWoWToEQSwapData();
        EverQuest->LoadSpellData();
        EverQuest->LoadIllusionDisplayData();
        EverQuest->LoadIllusionFaceData();
        EverQuest->LoadQuestCompletionReputations();
        EverQuest->LoadQuestReactions();
        EverQuest->LoadGossipReactions();
        EverQuest->LoadPetData();
        EverQuest->LoadPetSilentDisplayData();
        EverQuest->LoadCreatePlayerData();
        EverQuest->LoadCreatureLootData();
        EverQuest->LoadShipTriggerData();
        EverQuest->LoadCreatureInstanceData();
        EverQuest->LoadCreatureWaypointData();
        EverQuest->LoadAutoLearnSkillsData();
        EverQuest->LoadAutoLearnSpellsData();
        EverQuest->LoadForageData();
        EverQuest->LoadZoneSafePointData();
        EverQuest->LoadZoneData();
        EverQuest->LoadZoneTeleportDestinationData();
        EverQuest->LoadFactionData();
    }

    // The restricted map sweep runs here rather than on a map thread, since it has to look at every online
    // player at once. This hook runs after the map updates have been waited on, so no map thread is touching players
    void OnUpdate(uint32 diff) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        EverQuest->UpdateRestrictedMapPlayerCheck(diff);
        EverQuest->UpdateClientVersionChecks(diff);
        EverQuest->ProcessPendingEquipmentStorageTransactions();
        EverQuest->ProcessPendingReactionSpawnCreations();
        EverQuest->ProcessPendingReactionSpawnGridRemovals();
    }

    // The module writes through the asynchronous database queues and depends on those writes landing in the order they were queued (a class
    // switch runs right behind the logout save it reads from, and a runtime creature spawn deletes the rows its own save just wrote).  Every
    // asynchronous connection of a pool pulls from one shared queue, so more than one worker thread per pool makes that order undefined
    void WarnOnUnsupportedDatabaseWorkerThreadCounts()
    {
        if (sConfigMgr->GetOption<int32>("CharacterDatabase.WorkerThreads", 1) > 1)
            LOG_ERROR("module.EverQuest", "EverQuestMod requires CharacterDatabase.WorkerThreads to be 1. With more than one, character data writes can be applied out of order and a secondary class switch can lose spells, skills, actions or equipment.");
        if (sConfigMgr->GetOption<int32>("WorldDatabase.WorkerThreads", 1) > 1)
            LOG_ERROR("module.EverQuest", "EverQuestMod requires WorldDatabase.WorkerThreads to be 1. With more than one, a runtime creature spawn can delete its own creature row before that row is written, leaving the spawn behind permanently.");
    }

    void OnStartup() override
    {
        if (EverQuest->IsEnabled == false)
            return;

        WarnOnUnsupportedDatabaseWorkerThreadCounts();

        // Talent alignment reads Talent.dbc, SkillLineAbility.dbc and the spell store, none of which are ready when the config loads, so it builds here instead
        if (EverQuest->ConfigSpellTalentAlignmentEnabled == true)
            EverQuestTalentAlignment->Load();

        // The creature spawn tables aren't loaded yet when the kill spawn data loads with the config so respawn target spawn points resolve here instead
        EverQuest->ResolveKillSpawnRespawnTargetSpawnPoints();
        EverQuest->ResolveCreaturePresenceGroupSpawnPoints();
        EverQuest->ResolveVulakRequiredDragonSpawnPoints();

        // Saved pet display IDs can become wrong when converted content updates invalide previous display IDs, which crashes the core on pet summon
        EverQuest->FixInvalidCharacterPetModelIDs();

        // The silent pet displays validate against CreatureDisplayInfo.dbc, which isn't loaded when the pet silent display data loads with the config
        EverQuest->RemoveInvalidPetSilentDisplays();

        // Defend combat faction templates validate against FactionTemplate.dbc, which isn't loaded when the faction data loads with the config
        EverQuest->ResolveDefendCombatFactionTemplates();

        // The set of reputation-capable EQ factions validates against Faction.dbc, which also isn't loaded when the faction data loads with the config
        EverQuest->ResolveEQReputationFactions();

        // Death knights that level from 1 need their glyphs to unlock alongside every other class, and the item templates aren't loaded when the config loads
        EverQuest->LowerDeathKnightGlyphRequiredLevels();
    }
};

void AddEverQuestWorldScripts()
{
    new EverQuest_WorldScript();
}
