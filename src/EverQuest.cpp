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

void EverQuestMod::TelePlayerToDockTarget(Player* player, DockmasterTeleTarget teleTarget)
{
    switch (teleTarget)
    {
    case EastFreeport:
    {
        player->TeleportTo(CONFIG_EASTFREEPORT_MAP_ID, -13.707930f * CONFIG_WORLD_SCALE, -1030.772949f * CONFIG_WORLD_SCALE, -55.968700f * CONFIG_WORLD_SCALE, 1.604590f);
    } break;
    case OceanOfTearsWest:
    {
        player->TeleportTo(CONFIG_OCEANOFTEARS_MAP_ID, -2089.014648f * CONFIG_WORLD_SCALE, 7545.208496f * CONFIG_WORLD_SCALE, 0.032750f * CONFIG_WORLD_SCALE, 1.590430f);
    } break;
    case OceanOfTearsEast:
    {
        player->TeleportTo(CONFIG_OCEANOFTEARS_MAP_ID, 277.529144f * CONFIG_WORLD_SCALE, -9198.161133f * CONFIG_WORLD_SCALE, 0.031590f * CONFIG_WORLD_SCALE, 0.108470f);
    } break;
    case Butcherblock:
    {
        player->TeleportTo(CONFIG_BUTCHERBLOCK_MAP_ID, 1353.937012f * CONFIG_WORLD_SCALE, 3243.884521f * CONFIG_WORLD_SCALE, 7.907880f * CONFIG_WORLD_SCALE, 4.751650f);
    } break;
    default:
    {
        // Do Nothing
    }
    }
}
