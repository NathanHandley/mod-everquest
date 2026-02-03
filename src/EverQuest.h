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
#include "GameObject.h"
#include "ObjectGuid.h"
#include "CreatureData.h"

#include <list>
#include <map>

using namespace std;

#define EQ_MOD_VERSION                              3

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

#define EQ_PET_NAMING_TYPE_PET                      0
#define EQ_PET_NAMING_TYPE_FAMILIAR                 1
#define EQ_PET_NAMING_TYPE_WARDER                   2
#define EQ_PET_NAMING_TYPE_RANDOM                   3

#define EQ_QUEST_REACTION_UNKNOWN                   0
#define EQ_QUEST_REACTION_ATTACKPLAYER              1
#define EQ_QUEST_REACTION_DESPAWN                   2
#define EQ_QUEST_REACTION_EMOTE                     3
#define EQ_QUEST_REACTION_SAY                       4
#define EQ_QUEST_REACTION_SPAWN                     5
#define EQ_QUEST_REACTION_SPAWNUNIQUE               6
#define EQ_QUEST_REACTION_YELL                      7

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

class EverQuestCreature
{
public:
    uint32 CreatureTemplateID = 0;
    bool CanShowHeldLootItems = 0;
    bool CanShowHeldLootShields = 0;
};

class EverQuestLoadedCreatureEquippedVisualItems
{
public:
    uint32 MainhandItemID = 0;
    uint32 OffhandItemID = 0;
};

class EverQuestItemTemplate
{
public:
    uint32 ItemTemplateEntryID = 0;
    uint32 ItemTemplateEntryIDForNPCEquip = 0;
};

class EverQuestPet
{
public:
    int32 CreatingSpellID = 0;
    int32 NamingType = 0;
    uint32 CreatureTemplateID = 0;
    int32 SummonPropertiesID = 0;
    uint32 MainhandItemTemplateID = 0;
    uint32 OffhandItemTemplateID = 0;
};

class EverQuestQuestCompletionReputation
{
public:
    uint32 QuestTemplateID = 0;
    uint8 SortOrder = 0;
    uint32 FactionID = 0;
    int32 CompletionRewardValue = 0;
};

class EverQuestQuestReaction
{
public:
    uint32 ID = 0;
    uint32 QuestTemplateID = 0;
    int ReactionType = 0;
    bool UsePlayerX = false;
    bool UsePlayerY = false;
    bool UsePlayerZ = false;
    float AddedPlayerX = 0;
    float AddedPlayerY = 0;
    bool UsePlayerOrientation = false;
    float PositionX = 0;
    float PositionY = 0;
    float PositionZ = 0;
    float Orientation = 0;    
    uint32 CreatureTemplateID = 0;
};

class EverQuestLootTemplateRow
{
public:
    uint32 Entry = 0;
    uint32 ItemTemplateID = 0;
    float Chance = 0;
    bool QuestRequired = 0;
    uint8 GroupID = 0;
    int32 MinCount = 0;
    int32 MaxCount = 0;
};

class EverQuestTransportShipTrigger
{
public:
    uint32 TriggeringShipGameObjectEntryTemplateID = 0;
    uint32 TriggeredShipGameObjectTemplateEntryID = 0;
    uint32 TriggeringNodeID = 0;
    uint32 TriggerActivateNodeID = 0;
};

class EverQuestMod
{
private:
    EverQuestMod();

public:
    bool IsEnabled;

    // Configs (from database)
    float ConfigWorldScale;
    uint32 ConfigBardMaxConcurrentSongs;
    int ConfigSystemDayEventID;
    int ConfigSystemNightEventID;
    uint32 ConfigSystemMapDBCIDMin;
    uint32 ConfigSystemMapDBCIDMax;
    uint32 ConfigSystemSpellDBCIDMin;
    uint32 ConfigSystemSpellDBCIDMax;
    int ConfigSystemSpellDBCIDDayPhaseAura;
    int ConfigSystemSpellDBCIDNightPhaseAura;
    uint32 ConfigSystemQuestSQLIDMin;
    uint32 ConfigSystemQuestSQLIDMax;
    uint32 ConfigSystemCreatureTemplateIDMin;
    uint32 ConfigSystemCreatureTemplateIDMax;
    uint32 ConfigSystemGameObjectTemplateIDMin;
    uint32 ConfigSystemGameObjectTemplateIDMax;
    uint32 ConfigSystemShipEntryTemplateIDMin;
    uint32 ConfigSystemShipEntryTemplateIDMax;

    // Configs (from server file)
    bool ConfigMapRestrictPlayersToNorrath;
    bool ConfigQuestGrantExpOnRepeatCompletion;
    bool ConfigExpLossOnDeathEnabled;
    int ConfigExpLossOnDeathMinLevel;
    float ConfigExpLossOnDeathLossPercent;
    bool ConfigExpLossOnDeathAddLostExpToRestExp;

    vector<Player*> AllLoadedPlayers;
    unordered_map<uint32, EverQuestCreature> CreaturesByTemplateID;
    unordered_map<uint32, list<EverQuestCreatureOnkillReputation>> CreatureOnkillReputationsByCreatureTemplateID;
    unordered_map<uint32, EverQuestItemTemplate> ItemTemplatesByEntryID;
    unordered_map<uint32, EverQuestSpell> SpellDataBySpellID;
    unordered_map<uint32, list<EverQuestQuestCompletionReputation>> QuestCompletionReputationsByQuestTemplateID;
    unordered_map<uint32, list<EverQuestQuestReaction>> QuestReactionListByQuestTemplateID;
    unordered_map<uint32, EverQuestPet> PetDataByCreatureTemplateID;
    unordered_map<int, unordered_map<int, vector<Creature*>>> AllLoadedCreaturesByMapIDThenCreatureEntryID;
    unordered_map<ObjectGuid, deque<uint32>> PlayerCasterConcurrentBardSongs;
    unordered_map<uint32, unordered_map<uint32, vector<EverQuestLootTemplateRow>>> LootTemplateRowsInGroupByEntryID;
    unordered_map<ObjectGuid, vector<uint32>> PreloadedLootItemIDsByCreatureGUID;
    unordered_map<ObjectGuid, EverQuestLoadedCreatureEquippedVisualItems> VisualEquippedItemsByCreatureGUID;
    unordered_map<uint32, vector<EverQuestTransportShipTrigger>> ShipTriggersByTriggeringGameObjectTemplateEntryID;
    unordered_map<uint32, GameObject*> ShipGameObjectsByTemplateEntryID;

    static EverQuestMod* instance()
    {
        static EverQuestMod instance;
        return &instance;
    }
    ~EverQuestMod();

    bool LoadConfigurationSystemDataFromDB();
    void LoadConfigurationFile();
    void LoadCreatureData();
    bool HasCreatureDataForCreatureTemplateID(uint32 creatureTemplateID);
    const EverQuestCreature& GetCreatureDataForCreatureTemplateID(uint32 creatureTemplateID);
    void LoadCreatureOnkillReputations();
    const list<EverQuestCreatureOnkillReputation>& GetOnkillReputationsForCreatureTemplate(uint32 creatureTemplateID);
    void LoadItemTemplateData();
    uint32 GetNPCEquipItemTemplateIDForItemTemplate(uint32 itemTemplateID);
    void LoadSpellData();
    const EverQuestSpell& GetSpellDataForSpellID(uint32 spellID);
    void LoadQuestCompletionReputations();
    const list<EverQuestQuestCompletionReputation>& GetQuestCompletionReputationsForQuestTemplate(uint32 questTemplateID);
    void LoadQuestReactions();
    const list<EverQuestQuestReaction>& GetQuestReactions(uint32 questTemplateID);
    void LoadPetData();
    bool HasPetDataForCreatureTemplateID(uint32 creatureTemplateID);
    const EverQuestPet& GetPetDataForCreatureTemplateID(uint32 creatureTemplateID);
    void LoadLootTemplateRows();
    bool HasLootTemplateRowsByCreatureTemplateEntryID(uint32 creatureTemplateEntryID);
    bool HasPreloadedLootItemIDsForCreatureGUID(ObjectGuid creatureGUID);
    bool HasPreloadedLootItemIDForCreatureGUID(ObjectGuid creatureGUID, uint32 itemTemplateID);
    const vector<uint32>& GetPreloadedLootIDsForCreatureGUID(ObjectGuid creatureGUID);
    void ClearPreloadedLootIDsForCreatureGUID(ObjectGuid creatureGUID);
    void TrackVisualEquippedItemsForCreatureGUID(ObjectGuid creatureGUID, uint32 mainhandItemID, uint32 offhandItemID);
    void RemoveVisualEquippedItemForCreatureGUIDIfExists(Map* map, ObjectGuid creatureGUID, uint32 itemTemplateID);
    void LoadShipTriggerData();
    const vector<EverQuestTransportShipTrigger>& GetShipTriggersForShip(int triggeringGameObjectTemplateEntryID);

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
    void RollLootItemsForCreature(ObjectGuid creatureGUID, uint32 creatureTemplateEntryID);
    void SpawnCreature(uint32 entryID, Map* map, float x, float y, float z, float orientation, bool enforceUniqueSpawn);
    void DespawnCreature(uint32 entryID, Map* map);
    void MakeCreatureAttackPlayer(uint32 entryID, Map* map, Player* player);
    bool IsSpellAnEQSpell(uint32 spellID);
    bool IsSpellAnEQBardSong(uint32 spellID);
    uint32 CalculateSpellFocusBoostValue(Unit* caster, uint32 spellID);
};

#define EverQuest EverQuestMod::instance()

#endif //EVERQUEST
