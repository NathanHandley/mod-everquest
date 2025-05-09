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

#include "Chat.h"
#include "GameEventMgr.h"
#include "CreatureData.h"

#include "EverQuest.h"

using namespace std;

EverQuestMod::EverQuestMod()
{
    DruidHunterFriendlyFactionTemplateID = 2301; // TODO: Put in config
}

EverQuestMod::~EverQuestMod()
{

}

void EverQuestMod::LoadCreatureOnkillReputations()
{
    CreatureOnkillReputationsByCreatureTemplateID.clear();

    // Pulls in all the kill faction rewards
    QueryResult queryResult = WorldDatabase.Query("SELECT CreatureTemplateID, SortOrder, FactionID, KillRewardValue FROM mod_everquest_creature_onkill_reputation ORDER BY CreatureTemplateID, SortOrder;");
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            CreatureOnkillReputation creatureOnkillReputation;
            creatureOnkillReputation.CreatureTemplateID = fields[0].Get<uint32>();
            creatureOnkillReputation.SortOrder = fields[1].Get<uint8>();
            creatureOnkillReputation.FactionID = fields[2].Get<uint32>();
            creatureOnkillReputation.KillRewardValue = fields[3].Get<int32>();
            CreatureOnkillReputationsByCreatureTemplateID[creatureOnkillReputation.CreatureTemplateID].push_back(creatureOnkillReputation);
        } while (queryResult->NextRow());
    }
}

list<CreatureOnkillReputation> EverQuestMod::GetOnkillReputationsForCreatureTemplate(uint32 creatureTemplateID)
{
    if (CreatureOnkillReputationsByCreatureTemplateID.find(creatureTemplateID) != CreatureOnkillReputationsByCreatureTemplateID.end())
    {
        return CreatureOnkillReputationsByCreatureTemplateID[creatureTemplateID];
    }
    else
    {
        list<CreatureOnkillReputation> returnEmpty;
        return returnEmpty;
    }
}

void EverQuestMod::LoadQuestCompletionReputations()
{
    QuestCompletionReputationsByQuestTemplateID.clear();

    // Pulls in all the kill faction rewards
    QueryResult queryResult = WorldDatabase.Query("SELECT QuestTemplateID, SortOrder, FactionID, CompletionRewardValue FROM mod_everquest_quest_complete_reputation ORDER BY QuestTemplateID, SortOrder;");
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            QuestCompletionReputation questCompletionReputation;
            questCompletionReputation.QuestTemplateID = fields[0].Get<uint32>();
            questCompletionReputation.SortOrder = fields[1].Get<uint8>();
            questCompletionReputation.FactionID = fields[2].Get<uint32>();
            questCompletionReputation.CompletionRewardValue = fields[3].Get<int32>();
            QuestCompletionReputationsByQuestTemplateID[questCompletionReputation.QuestTemplateID].push_back(questCompletionReputation);
        } while (queryResult->NextRow());
    }
}

list<QuestCompletionReputation> EverQuestMod::GetQuestCompletionReputationsForQuestTemplate(uint32 questTemplateID)
{
    if (QuestCompletionReputationsByQuestTemplateID.find(questTemplateID) != QuestCompletionReputationsByQuestTemplateID.end())
    {
        return QuestCompletionReputationsByQuestTemplateID[questTemplateID];
    }
    else
    {
        list<QuestCompletionReputation> returnEmpty;
        return returnEmpty;
    }
}

void EverQuestMod::StorePositionAsLastGate(Player* player)
{
    // Fail if there is no map, or if the map is invalid
    if (player->GetMap() == nullptr)
        return;

    // Set up the transaction
    CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();

    // Delete the old record, if it exists
    transaction->Append("DELETE FROM `mod_everquest_character_lastgate` WHERE guid = {}", player->GetGUID().GetCounter());

    // Add the new gate reference
    float playerX = player->GetPosition().GetPositionX();
    float playerY = player->GetPosition().GetPositionY();
    float playerZ = player->GetPosition().GetPositionZ();
    float playerOrientation = player->GetOrientation();
    int mapID = player->GetMap()->GetId();
    int zoneID = player->GetAreaId();
    transaction->Append("INSERT INTO `mod_everquest_character_lastgate` (`guid`, `mapId`, `zoneId`, `posX`, `posY`, `posZ`, `orientation`) VALUES ({}, {}, {}, {}, {}, {}, {})",
        player->GetGUID().GetCounter(), mapID, zoneID, playerX, playerY, playerZ, playerOrientation);

    // Commit the transaction
    CharacterDatabase.CommitTransaction(transaction);
}

void EverQuestMod::SendPlayerToLastGate(Player* player)
{
    // Fail if in combat
    if (player->IsInCombat() == true)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("Your gate tether broke due to being in combat!");
        return;
    }

    // Pull the bind position
    QueryResult queryResult = CharacterDatabase.Query("SELECT mapId, zoneId, posX, posY, posZ, orientation FROM mod_everquest_character_lastgate WHERE guid = {}", player->GetGUID().GetCounter());
    if (!queryResult || queryResult->GetRowCount() == 0)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("No tethered gate could be found. Spell failed.");
        return;
    }

    // Pull the fields out
    Field* fields = queryResult->Fetch();
    uint32 mapId = fields[0].Get<uint32>();
    uint32 zoneId = fields[1].Get<uint32>();
    float posX = fields[2].Get<float>();
    float posY = fields[3].Get<float>();
    float posZ = fields[4].Get<float>();
    float orientation = fields[4].Get<float>();

    // Teleport the player
    player->TeleportTo({ mapId, {posX, posY, posZ, orientation} });
}

void EverQuestMod::SendPlayerToEQBindHome(Player* player)
{
    // Pull the bind position
    QueryResult queryResult = CharacterDatabase.Query("SELECT mapId, zoneId, posX, posY, posZ FROM mod_everquest_character_homebind WHERE guid = {}", player->GetGUID().GetCounter());
    if (!queryResult || queryResult->GetRowCount() == 0)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("You have no bind point in Norrath. Spell failed.");
        return;
    }

    // Pull the fields out
    Field* fields = queryResult->Fetch();
    uint32 mapId = fields[0].Get<uint32>();
    uint32 zoneId = fields[1].Get<uint32>();
    float posX = fields[2].Get<float>();
    float posY = fields[3].Get<float>();
    float posZ = fields[4].Get<float>();

    // Teleport the player
    player->TeleportTo({mapId, {posX, posY, posZ, player->GetOrientation()}});
}

void EverQuestMod::SetNewBindHome(Player* player)
{
    // Fail if there is no map, or if the map is invalid
    if (player->GetMap() == nullptr)
        return;

    // Set up the transaction
    CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();

    // Delete the old record, if it exists
    transaction->Append("DELETE FROM `mod_everquest_character_homebind` WHERE guid = {}", player->GetGUID().GetCounter());

    // Add the new binding
    float playerX = player->GetPosition().GetPositionX();
    float playerY = player->GetPosition().GetPositionY();
    float playerZ = player->GetPosition().GetPositionZ();
    int mapID = player->GetMap()->GetId();
    int zoneID = player->GetAreaId();
    transaction->Append("INSERT INTO `mod_everquest_character_homebind` (`guid`, `mapId`, `zoneId`, `posX`, `posY`, `posZ`) VALUES ({}, {}, {}, {}, {}, {})",
        player->GetGUID().GetCounter(), mapID, zoneID, playerX, playerY, playerZ);

    // Commit the transaction
    CharacterDatabase.CommitTransaction(transaction);

    // Send a message to the player
    ChatHandler(player->GetSession()).PSendSysMessage("You feel yourself bind to the area.");
}

void EverQuestMod::DeletePlayerBindHome(ObjectGuid guid)
{
    // Set up the transaction
    CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();

    // Delete the old record, if it exists
    transaction->Append("DELETE FROM `mod_everquest_character_homebind` WHERE guid = {}", guid.GetCounter());

    // Commit the transaction
    CharacterDatabase.CommitTransaction(transaction);
}

void EverQuestMod::SetAllLoadedPlayersDayOrNightAura()
{
    for (std::vector<Player*>::const_iterator playerIterator = AllLoadedPlayers.begin(); playerIterator != AllLoadedPlayers.end(); ++playerIterator)
    {
        Player* thisPlayer = *playerIterator;
        SetPlayerDayOrNightAura(thisPlayer);
    }
}

void EverQuestMod::SetPlayerDayOrNightAura(Player* player)
{
    if (player == nullptr)
        return;

    // Set aura if it's night or day and the player is in EQ, otherwise clear it
    if (player->GetMap() != nullptr && (player->GetMap()->GetId() >= CONFIG_EQ_MIN_MAP_ID && player->GetMap()->GetId() <= CONFIG_EQ_MAX_MAP_ID))
    {
        // Day
        if (sGameEventMgr->IsActiveEvent(CONFIG_EQ_EVENTS_DAY_ID))
        {            
            if (player->HasAura(CONFIG_EQ_SPELLS_AURA_DAY_PHASE_ID) == false)
                player->AddAura(CONFIG_EQ_SPELLS_AURA_DAY_PHASE_ID, player);
        }
        else
        {
            if (player->HasAura(CONFIG_EQ_SPELLS_AURA_DAY_PHASE_ID) == true)
                player->RemoveAura(CONFIG_EQ_SPELLS_AURA_DAY_PHASE_ID);
        }

        // Night
        if (sGameEventMgr->IsActiveEvent(CONFIG_EQ_EVENTS_NIGHT_ID))
        {            
            if (player->HasAura(CONFIG_EQ_SPELLS_AURA_NIGHT_PHASE_ID) == false)
                player->AddAura(CONFIG_EQ_SPELLS_AURA_NIGHT_PHASE_ID, player);
        }
        else
        {
            if (player->HasAura(CONFIG_EQ_SPELLS_AURA_NIGHT_PHASE_ID) == true)
                player->RemoveAura(CONFIG_EQ_SPELLS_AURA_NIGHT_PHASE_ID);
        }
    }
    else
    {
        if (player->HasAura(CONFIG_EQ_SPELLS_AURA_DAY_PHASE_ID) == true)
            player->RemoveAura(CONFIG_EQ_SPELLS_AURA_DAY_PHASE_ID);
        if (player->HasAura(CONFIG_EQ_SPELLS_AURA_NIGHT_PHASE_ID) == true)
            player->RemoveAura(CONFIG_EQ_SPELLS_AURA_NIGHT_PHASE_ID);
    }
}

void EverQuestMod::AddCreatureAsLoaded(int mapID, Creature* creature)
{
    if (AllLoadedCreaturesByMapIDThenCreatureEntryID.find(mapID) == AllLoadedCreaturesByMapIDThenCreatureEntryID.end())
        AllLoadedCreaturesByMapIDThenCreatureEntryID[mapID];
    std::unordered_map<int, std::vector<Creature*>>& innerMap = AllLoadedCreaturesByMapIDThenCreatureEntryID[mapID];
    if (innerMap.find(creature->GetEntry()) != innerMap.end())
        innerMap[creature->GetEntry()];
    innerMap[creature->GetEntry()].push_back(creature);
}

void EverQuestMod::RemoveCreatureAsLoaded(int mapID, Creature* creature)
{
    if (AllLoadedCreaturesByMapIDThenCreatureEntryID.find(mapID) == AllLoadedCreaturesByMapIDThenCreatureEntryID.end())
        return;
    std::unordered_map<int, std::vector<Creature*>>& innerMap = AllLoadedCreaturesByMapIDThenCreatureEntryID[mapID];
    int creatureEntryID = creature->GetEntry();
    if (innerMap.find(creatureEntryID) == innerMap.end())
        return;

    std::vector<Creature*>& creatureVector = innerMap[creatureEntryID];
    std::vector<Creature*>::iterator it = std::find(creatureVector.begin(), creatureVector.end(), creature);
    if (it != creatureVector.end())
    {
        creatureVector.erase(it);
        if (creatureVector.empty())
        {
            innerMap.erase(creatureEntryID);
            if (innerMap.empty())
                AllLoadedCreaturesByMapIDThenCreatureEntryID.erase(mapID);
        }
    }
}

vector<Creature*> EverQuestMod::GetLoadedCreaturesWithEntryID(int mapID, uint32 entryID)
{
    if (AllLoadedCreaturesByMapIDThenCreatureEntryID.find(mapID) == AllLoadedCreaturesByMapIDThenCreatureEntryID.end())
        return vector<Creature*>();
    std::unordered_map<int, std::vector<Creature*>>& innerMap = AllLoadedCreaturesByMapIDThenCreatureEntryID[mapID];
    if (innerMap.find(entryID) == innerMap.end())
        return vector<Creature*>();
    return innerMap[entryID];
}
