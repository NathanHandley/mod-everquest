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
#include "CreatureData.h"

#include <list>
#include <map>

// TODO: Put these in the config
#define CONFIG_WORLD_SCALE                              0.29f

#define CONFIG_EQ_MIN_MAP_ID                            750
#define CONFIG_EQ_MAX_MAP_ID                            900

#define CONFIG_SPELLS_EQ_SPELLDBC_ID_MIN                86900
#define CONFIG_SPELLS_EQ_SPELLDBC_ID_MAX                99999
#define CONFIG_SPELLS_CONVERTED_SPELLDBC_ID_START       92000
#define CONFIG_SPELLS_CONVERTED_SPELLDBC_ID_END         96049

#define CONFIG_RESTRICT_PLAYERS_TO_NORRATH              false

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

class EverQuestCreatureOnkillReputation
{
public:
    uint32 CreatureTemplateID = 0;
    uint8 SortOrder = 0;
    uint32 FactionID = 0;
    int32 KillRewardValue = 0;
};

class EverQuestSpell
{
public:
    uint32 SpellID = 0;
    uint32 AuraDurationBaseInMS = 0;
    uint32 AuraDurationAddPerLevelInMS = 0;
    uint32 AuraDurationMaxInMS = 0;
    uint32 AuraDurationCalcMinLevel = 0;
    uint32 AuraDurationCalcMaxLevel = 0;
};

class EverQuestQuestCompletionReputation
{
public:
    uint32 QuestTemplateID = 0;
    uint8 SortOrder = 0;
    uint32 FactionID = 0;
    int32 CompletionRewardValue = 0;
};

class EverQuestMod
{
private:
    EverQuestMod();

public:
    map<uint32, list<EverQuestCreatureOnkillReputation>> CreatureOnkillReputationsByCreatureTemplateID;
    map<uint32, EverQuestSpell> SpellDataBySpellID;
    map<uint32, list<EverQuestQuestCompletionReputation>> QuestCompletionReputationsByQuestTemplateID;
    uint32 DruidHunterFriendlyFactionTemplateID;
    std::vector<Player*> AllLoadedPlayers;
    std::unordered_map<int, std::unordered_map<int, std::vector<Creature*>>> AllLoadedCreaturesByMapIDThenCreatureEntryID;

    static EverQuestMod* instance()
    {
        static EverQuestMod instance;
        return &instance;
    }
    ~EverQuestMod();

    void LoadCreatureOnkillReputations();
    const list<EverQuestCreatureOnkillReputation>& GetOnkillReputationsForCreatureTemplate(uint32 creatureTemplateID);
    void LoadSpellData();
    const EverQuestSpell& GetSpellDataForSpellID(uint32 spellID);
    void LoadQuestCompletionReputations();
    const list<EverQuestQuestCompletionReputation>& GetQuestCompletionReputationsForQuestTemplate(uint32 questTemplateID);

    void StorePositionAsLastGate(Player* player);
    void SendPlayerToLastGate(Player* player);
    void SendPlayerToEQBindHome(Player* player);
    void SetNewBindHome(Player* player);
    void DeletePlayerBindHome(ObjectGuid guid);
    void SetAllLoadedPlayersDayOrNightAura();
    void SetPlayerDayOrNightAura(Player* player);
    void AddCreatureAsLoaded(int mapID, Creature* creature);
    void RemoveCreatureAsLoaded(int mapID, Creature* creature);
    vector<Creature*> GetLoadedCreaturesWithEntryID(int mapID, uint32 entryID);
    void SpawnCreature(uint32 entryID, Map* map, float x, float y, float z, float orientation, bool enforceUniqueSpawn);
    void DespawnCreature(Creature* creature, Map* map);
    void DespawnCreature(uint32 entryID, Map* map);
    void MakeCreatureAttackPlayer(uint32 entryID, Map* map, Player* player);
    bool IsSpellAnEQSpell(uint32 spellID);
};

#define EverQuest EverQuestMod::instance()

#endif //EVERQUEST
