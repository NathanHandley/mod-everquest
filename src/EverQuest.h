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
#define CONFIG_WORLD_SCALE                              0.29f

#define CONFIG_EQ_MIN_MAP_ID                            750
#define CONFIG_EQ_MAX_MAP_ID                            900

#define CONFIG_SPELLS_GATE_SPELLDBC_ID                  86900
#define CONFIG_SPELLS_BINDSELF_SPELLDBC_ID              86901
#define CONFIG_SPELLS_BINDANY_SPELLDBC_ID               86902

#define CONFIG_RESTRICT_PLAYERS_TO_NORRATH              true

#define CONFIG_LOSE_EXP_ON_DEATH_RELEASE                true
#define CONFIG_LOSE_EXP_ON_DEATH_RELEASE_MIN_LEVEL      5
#define CONFIG_LOSE_EXP_ON_DEATH_RELEASE_LOSS_PERCENT   10
#define CONFIG_LOSE_EXP_ON_DEATH_RELEASE_ONLY_EQ_MAPS   true
#define CONFIG_LOSE_EXP_ON_DEATH_RELEASE_ADD_TO_REST    true

#define CONFIG_QUEST_EXP_ON_REPEAT                      true
#define CONFIG_QUEST_ID_LOW                             30000
#define CONFIG_QUEST_ID_HIGH                            30000

#define CONFIG_EQ_SPELLS_AURA_DAY_PHASE_ID              86903
#define CONFIG_EQ_SPELLS_AURA_NIGHT_PHASE_ID            86904
#define CONFIG_EQ_EVENTS_DAY_ID                         125
#define CONFIG_EQ_EVENTS_NIGHT_ID                       126

#define CONFIG_GATE_RETURN_ENABLED                      true

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
    std::vector<Player*> AllLoadedPlayers;

    static EverQuestMod* instance()
    {
        static EverQuestMod instance;
        return &instance;
    }
    ~EverQuestMod();

    void LoadCreatureOnkillReputations();
    list<CreatureOnkillReputation> GetOnkillReputationsForCreatureTemplate(uint32 creatureTemplateID);

    void StorePositionAsLastGate(Player* player);
    void SendPlayerToLastGate(Player* player);
    void SendPlayerToEQBindHome(Player* player);
    void SetNewBindHome(Player* player);
    void DeletePlayerBindHome(ObjectGuid guid);
    void SetAllLoadedPlayersDayOrNightAura();
    void SetPlayerDayOrNightAura(Player* player);    
};

#define EverQuest EverQuestMod::instance()

#endif //EVERQUEST
