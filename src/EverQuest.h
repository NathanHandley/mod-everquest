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

using namespace std;

#define EQ_SPELLDUMMYTYPE_BINDSELF                  1
#define EQ_SPELLDUMMYTYPE_BINDANY                   2
#define EQ_SPELLDUMMYTYPE_GATE                      3
#define EQ_SPELLDUMMYTYPE_BARDFOCUSBRASS            4
#define EQ_SPELLDUMMYTYPE_BARDFOCUSSTRING           5
#define EQ_SPELLDUMMYTYPE_BARDFOCUSWIND             6
#define EQ_SPELLDUMMYTYPE_BARDFOCUSPERCUSSION       7
#define EQ_SPELLDUMMYTYPE_BARDFOCUSALL              8
#define EQ_SPELLDUMMYTYPE_BARDSONGENEMYAREA         9
#define EQ_SPELLDUMMYTYPE_BARDSONGFRIENDLYPARTY     10
#define EQ_SPELLDUMMYTYPE_BARDSONGSELF              11
#define EQ_SPELLDUMMYTYPE_BARDSONGENEMYSINGLE       12
#define EQ_SPELLDUMMYTYPE_BARDSONGFRIENDLYSINGLE    13
#define EQ_SPELLDUMMYTYPE_BARDSONGANY               14
#define EQ_SPELLDUMMYTYPE_ILLUSIONPARENT            15
#define EQ_SPELLDUMMYTYPE_SUMMONPET                 16

#define EQ_BARDSONGAURATARGET_ENEMYAREA             1
#define EQ_BARDSONGAURATARGET_FRIENDLYPARTY         2
#define EQ_BARDSONGAURATARGET_SELF                  3
#define EQ_BARDSONGAURATARGET_ENEMYSINGLE           4
#define EQ_BARDSONGAURATARGET_FRIENDLYSINGLE        5
#define EQ_BARDSONGAURATARGET_ANY                   6

#define EQ_SPELLFOCUSBOOSTTYPE_BARDPERCUSSION       1
#define EQ_SPELLFOCUSBOOSTTYPE_BARDBRASS            2
#define EQ_SPELLFOCUSBOOSTTYPE_BARDSINGING          3
#define EQ_SPELLFOCUSBOOSTTYPE_BARDSTRINGED         4
#define EQ_SPELLFOCUSBOOSTTYPE_BARDWIND             5

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
    uint32 RecourseSpellID = 0;
    uint32 SpellIDCastOnMeleeAttacker = 0;
    uint32 FocusBoostType = 0;
    uint32 PeriodicAuraSpellID = 0;
    uint32 PeriodicAuraSpellRadius = 0;
    uint32 MaleFormSpellID = 0;
    uint32 FemaleFormSpellID = 0;
};

class EverQuestPet
{
public:
    uint32 CreatingSpellID = 0;
    uint32 NamingType = 0;
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
    // Configs (from database)
    int ConfigBardMaxConcurrentSongs;
    int ConfigSystemDayEventID;
    int ConfigSystemNightEventID;
    int ConfigSystemMapDBCIDMin;
    int ConfigSystemMapDBCIDMax;
    int ConfigSystemPetsUseEQLevelAndBehavior;
    int ConfigSystemSpellDBCIDMin;
    int ConfigSystemSpellDBCIDMax;
    int ConfigSystemSpellDBCIDDayPhaseAura;
    int ConfigSystemSpellDBCIDNightPhaseAura;
    int ConfigSystemQuestSQLIDMin;
    int ConfigSystemQuestSQLIDMax;

    // Configs (from server file)
    bool ConfigMapRestrictPlayersToNorrath;
    bool ConfigQuestGrantExpOnRepeatCompletion;
    bool ConfigExpLossOnDeathEnabled;
    int ConfigExpLossOnDeathMinLevel;
    float ConfigExpLossOnDeathLossPercent;
    bool ConfigExpLossOnDeathAddLostExpToRestExp;

    map<uint32, list<EverQuestCreatureOnkillReputation>> CreatureOnkillReputationsByCreatureTemplateID;
    map<uint32, EverQuestSpell> SpellDataBySpellID;
    map<uint32, list<EverQuestQuestCompletionReputation>> QuestCompletionReputationsByQuestTemplateID;
    map<uint32, EverQuestPet> PetDataByCreatingSpellID;
    std::vector<Player*> AllLoadedPlayers;
    std::unordered_map<int, std::unordered_map<int, std::vector<Creature*>>> AllLoadedCreaturesByMapIDThenCreatureEntryID;
    std::unordered_map<ObjectGuid, std::deque<uint32>> PlayerCasterConcurrentBardSongs;

    static EverQuestMod* instance()
    {
        static EverQuestMod instance;
        return &instance;
    }
    ~EverQuestMod();

    void LoadConfigurationSystemDataFromDB();
    void LoadConfigurationFile();
    void LoadCreatureOnkillReputations();
    const list<EverQuestCreatureOnkillReputation>& GetOnkillReputationsForCreatureTemplate(uint32 creatureTemplateID);
    void LoadSpellData();
    const EverQuestSpell& GetSpellDataForSpellID(uint32 spellID);
    void LoadQuestCompletionReputations();
    const list<EverQuestQuestCompletionReputation>& GetQuestCompletionReputationsForQuestTemplate(uint32 questTemplateID);
    void LoadPetData();

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
    bool IsSpellAnEQBardSong(uint32 spellID);
    uint32 CalculateSpellFocusBoostValue(Unit* caster, uint32 spellID);
};

#define EverQuest EverQuestMod::instance()

#endif //EVERQUEST
