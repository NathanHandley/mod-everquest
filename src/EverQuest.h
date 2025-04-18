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

#ifndef EVERQUEST_H
#define EVERQUEST_H

#include "Common.h"
#include "ObjectGuid.h"

#include <list>
#include <map>

// TODO: Put these in the config
#define CONFIG_WORLD_SCALE                              0.29f // TODO: put in config

#define CONFIG_SPELLS_GATE_SPELLDBC_ID                  86900 // TODO: put in config
#define CONFIG_SPELLS_BINDSELF_SPELLDBC_ID              86901 // TODO: put in config
#define CONFIG_SPELLS_BINDANY_SPELLDBC_ID               86902 // TODO: put in config
#define CONFIG_SPELLS_BIND_MIN_MAP_ID                   750 // TODO: put in config
#define CONFIG_SPELLS_BIND_MAX_MAP_ID                   900 // TODO: put in config

#define CONFIG_RESTRICT_PLAYERS_TO_NORRATH              true // TODO: put in a config

#define CONFIG_LOSE_EXP_ON_DEATH_RELEASE                true // TODO: put in a config
#define CONFIG_LOSE_EXP_ON_DEATH_RELEASE_MIN_LEVEL      5
#define CONFIG_LOSE_EXP_ON_DEATH_RELEASE_LOSS_PERCENT   15 // TODO: put in a config
#define CONFIG_LOSE_EXP_ON_DEATH_RELEASE_ONLY_EQ_MAPS   true // TODO: pu

//#define CONFIG_RELEASE_SEND_TO_BIND             true // TODO: put in a config.  If true, player doesn't go to a graveyard but rather their bind point

using namespace std;

class CreatureOnkillReputation
{
public:
    uint32 CreatureTemplateID;
    uint8 SortOrder;
    uint32 FactionID;
    int32 KillRewardValue;
};

class EverQuestMod
{
private:
    EverQuestMod();

public:
    map<uint32, list<CreatureOnkillReputation>> CreatureOnkillReputationsByCreatureTemplateID;
    uint32 DruidHunterFriendlyFactionTemplateID;

    static EverQuestMod* instance()
    {
        static EverQuestMod instance;
        return &instance;
    }
    ~EverQuestMod();

    void LoadCreatureOnkillReputations();
    list<CreatureOnkillReputation> GetOnkillReputationsForCreatureTemplate(uint32 creatureTemplateID);

    void SendPlayerToEQBindHome(Player* player);
    void SetNewBindHome(Player* player);
    void DeletePlayerBindHome(ObjectGuid guid);
};

#define EverQuest EverQuestMod::instance()

#endif //EVERQUEST
