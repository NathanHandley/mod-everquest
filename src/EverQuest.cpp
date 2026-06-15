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
#include "Creature.h"
#include "CreatureData.h"
#include "SpellMgr.h"

#include "EverQuest.h"

#include <algorithm>
#include <random>

using namespace std;

EverQuestMod::EverQuestMod() :
    IsEnabled(true),
    ConfigWorldScale(1),
    ConfigBardMaxConcurrentSongs(1),
    ConfigSystemMapDBCIDMin(0),
    ConfigSystemMapDBCIDMax(0),
    ConfigSystemSpellDBCIDMin(0),
    ConfigSystemSpellDBCIDMax(0),
    ConfigSystemQuestSQLIDMin(0),
    ConfigSystemQuestSQLIDMax(0),
    ConfigSystemCreatureTemplateIDMin(0),
    ConfigSystemCreatureTemplateIDMax(0),
    ConfigDeathKnightsStartLikeOtherClasses(false),
    ConfigMapRestrictPlayersToNorrath(false),    
    ConfigQuestGrantExpOnRepeatCompletion(true),
    ConfigExpLossOnDeathEnabled(true),
    ConfigExpLossOnDeathMinLevel(5),
    ConfigExpLossOnDeathLossPercent(10),
    ConfigExpLossOnDeathAddLostExpToRestExp(true),
    ConfigAlternateGroupExperienceFormulaEnabled(false),
    ConfigAlternateGroupExperienceAddPercentPerAddedMember(20.0f),
    configSpellDisableStackingOfSameDOT(false)
{
}

EverQuestMod::~EverQuestMod()
{

}

bool EverQuestMod::LoadConfigurationSystemDataFromDB()
{
    // Fail if no config table even exists
    QueryResult configTableExistsQueryResult = WorldDatabase.Query("SELECT EXISTS (SELECT 1 FROM information_schema.tables WHERE table_schema = DATABASE() AND table_name = 'mod_everquest_systemconfigs') AS table_exists;");
    if (!configTableExistsQueryResult)
        return false;
    else
    {
        Field* fields = configTableExistsQueryResult->Fetch();
        int value = fields[0].Get<int>();
        if (value != 1)
            return false;
    }

    QueryResult configValuesQueryResult = WorldDatabase.Query("SELECT `Key`, `Value` FROM `mod_everquest_systemconfigs`;");
    int configModVersion = 0;
    if (configValuesQueryResult)
    {
        do
        {
            // Pull the data out and assign to configs
            Field* fields = configValuesQueryResult->Fetch();
            string key = fields[0].Get<string>();
            string value = fields[1].Get<string>();
            if (key == "ModVersion")
                configModVersion = atoi(value.c_str());
            else if (key == "BardMaxConcurrentSongs")
                ConfigBardMaxConcurrentSongs = (uint32)atoi(value.c_str());
            else if (key == "CreatureTemplateIDMin")
                ConfigSystemCreatureTemplateIDMin = (uint32)atoi(value.c_str());
            else if (key == "CreatureTemplateIDMax")
                ConfigSystemCreatureTemplateIDMax = (uint32)atoi(value.c_str());
            else if (key == "DeathKnightsStartLikeOtherClasses")
                ConfigDeathKnightsStartLikeOtherClasses = value == "1" ? true : false;
            else if (key == "GameObjectTemplateIDMin")
                ConfigSystemGameObjectTemplateIDMin = (uint32)atoi(value.c_str());
            else if (key == "GameObjectTemplateIDMax")
                ConfigSystemGameObjectTemplateIDMax = (uint32)atoi(value.c_str());
            else if (key == "MapDBCIDMin")
                ConfigSystemMapDBCIDMin = (uint32)atoi(value.c_str());
            else if (key == "MapDBCIDMax")
                ConfigSystemMapDBCIDMax = (uint32)atoi(value.c_str());
            else if (key == "ShipEntryTemplateIDMin")
                ConfigSystemShipEntryTemplateIDMin = (uint32)atoi(value.c_str());
            else if (key == "ShipEntryTemplateIDMax")
                ConfigSystemShipEntryTemplateIDMax = (uint32)atoi(value.c_str());
            else if (key == "SpellDBCIDMin")
                ConfigSystemSpellDBCIDMin = (uint32)atoi(value.c_str());
            else if (key == "SpellDBCIDMax")
                ConfigSystemSpellDBCIDMax = (uint32)atoi(value.c_str());
            else if (key == "QuestSQLIDMin")
                ConfigSystemQuestSQLIDMin = (uint32)atoi(value.c_str());
            else if (key == "QuestSQLIDMax")
                ConfigSystemQuestSQLIDMax = (uint32)atoi(value.c_str());
            else if (key == "WorldScale")
                ConfigWorldScale = (float)atof(value.c_str());
            else
            {
                LOG_ERROR("module.EverQuest", "EverQuestMod::LoadConfigurationSystemDataFromDB error, unhandled key of {} with value {}", key, value);
            }
        } while (configValuesQueryResult->NextRow());
    }
    int expectedVersion = EQ_MOD_VERSION;
    if (configModVersion < expectedVersion)
    {
        LOG_ERROR("module.EverQuest", "Module version expected database data of version {} but found version {}. Update and rerun EQWOWConverter.", configModVersion, expectedVersion);
        return false;
    }
    else if (configModVersion > expectedVersion)
    {
        LOG_ERROR("module.EverQuest", "Module version expected database data of version {} but found version {}. Update the mod-everquest project.", configModVersion, expectedVersion);
        return false;
    }

    return true;
}

void EverQuestMod::LoadConfigurationFile()
{
    // Enabled
    IsEnabled = sConfigMgr->GetOption<bool>("EverQuest.Enabled", true);
    if (IsEnabled == false)
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod::LoadConfigurationFile has EverQuest.Enabled as false, so the module is deactivated");
        return;
    }

    // Map
    ConfigMapRestrictPlayersToNorrath = sConfigMgr->GetOption<bool>("EverQuest.Map.RestrictPlayersToNorrath", false);

    // Quest
    ConfigQuestGrantExpOnRepeatCompletion = sConfigMgr->GetOption<bool>("EverQuest.Quest.GrantExpOnRepeatCompletion", true);

    // Exp Loss on Death
    ConfigExpLossOnDeathEnabled = sConfigMgr->GetOption<bool>("EverQuest.ExpLossOnDeath.Enabled", true);
    ConfigExpLossOnDeathMinLevel = sConfigMgr->GetOption<uint32>("EverQuest.ExpLossOnDeath.MinLevel", 5);
    ConfigExpLossOnDeathLossPercent = sConfigMgr->GetOption<float>("EverQuest.ExpLossOnDeath.LossPercent", 10);
    ConfigExpLossOnDeathAddLostExpToRestExp = sConfigMgr->GetOption<bool>("EverQuest.ExpLossOnDeath.AddLostExpToRestExp", true);

    // Group EXP rates
    ConfigAlternateGroupExperienceFormulaEnabled = sConfigMgr->GetOption<bool>("EverQuest.AlternateGroupExperienceFormula.Enabled", false);
    ConfigAlternateGroupExperienceAddPercentPerAddedMember = sConfigMgr->GetOption<float>("EverQuest.AlternateGroupExperienceFormula.AddPercentPerMember", false);

    // Spells
    configSpellDisableStackingOfSameDOT = sConfigMgr->GetOption<bool>("EverQuest.Spells.DisableStackingOfSameDOT", false);
}

void EverQuestMod::LoadCreatureData()
{
    CreaturesByTemplateID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT CreatureTemplateID, CanShowHeldLootItems, CanShowHeldLootShields, SpawnLimit FROM mod_everquest_creature ORDER BY CreatureTemplateID;");
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            EverQuestCreature everQuestCreature;
            everQuestCreature.CreatureTemplateID = fields[0].Get<uint32>();
            everQuestCreature.CanShowHeldLootItems = fields[1].Get<bool>();
            everQuestCreature.CanShowHeldLootShields = fields[2].Get<bool>();
            everQuestCreature.SpawnLimit = fields[3].Get<uint32>();
            CreaturesByTemplateID[everQuestCreature.CreatureTemplateID] = everQuestCreature;
        } while (queryResult->NextRow());
    }
}

void EverQuestMod::LoadCreatureSpawnPoints()
{
    CreatureSpawnPointsByCreatureGUID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT CreatureGUID, MapID, SpawnPointID, SpawnGroupID, SpawnGroupLimit FROM mod_everquest_creature_spawn_point;");
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            EverQuestCreatureSpawnPoint creatureSpawnPoint;
            creatureSpawnPoint.CreatureGUID = fields[0].Get<uint32>();
            creatureSpawnPoint.MapID = fields[1].Get<uint32>();
            creatureSpawnPoint.SpawnPointID = fields[2].Get<uint32>();
            creatureSpawnPoint.SpawnGroupID = fields[3].Get<uint32>();
            creatureSpawnPoint.SpawnGroupLimit = fields[4].Get<uint32>();
            CreatureSpawnPointsByCreatureGUID[creatureSpawnPoint.CreatureGUID] = creatureSpawnPoint;
        } while (queryResult->NextRow());
    }
}

bool EverQuestMod::ShouldDespawnCreatureDueToSpawnRestrictions(int mapID, Creature* creature)
{
    // Creatures loading in dead (corpses) never count against spawn restrictions
    if (creature->IsAlive() == false)
        return false;

    // Pets and summoned creatures are not placed spawns, so never restrict them
    if (creature->IsPet() == true || creature->IsSummon() == true)
        return false;

    // Restricted creatures (EQ "spawn_limit") can only have so many alive in a map at once
    uint32 spawnLimit = GetCreatureDataForCreatureTemplateID(creature->GetEntry()).SpawnLimit;
    if (spawnLimit > 0)
    {
        uint32 aliveCount = 0;
        for (Creature* loadedCreature : GetLoadedCreaturesWithEntryID(mapID, creature->GetEntry()))
            if (loadedCreature != creature && loadedCreature->IsAlive() == true && loadedCreature->IsPet() == false && loadedCreature->IsSummon() == false)
                aliveCount++;
        if (aliveCount >= spawnLimit)
            return true;
    }

    // Pooled spawn points can only ever have one creature alive on them, and capped spawn groups so many alive in total
    if (creature->GetSpawnId() != 0 && CreatureSpawnPointsByCreatureGUID.find(creature->GetSpawnId()) != CreatureSpawnPointsByCreatureGUID.end())
    {
        const EverQuestCreatureSpawnPoint& creatureSpawnPoint = CreatureSpawnPointsByCreatureGUID[creature->GetSpawnId()];
        if (AllLoadedCreaturesByMapIDThenSpawnPointID.find(mapID) != AllLoadedCreaturesByMapIDThenSpawnPointID.end())
        {
            unordered_map<uint32, vector<Creature*>>& spawnPointMap = AllLoadedCreaturesByMapIDThenSpawnPointID[mapID];
            if (spawnPointMap.find(creatureSpawnPoint.SpawnPointID) != spawnPointMap.end())
                for (Creature* loadedCreature : spawnPointMap[creatureSpawnPoint.SpawnPointID])
                    if (loadedCreature != creature && loadedCreature->IsAlive() == true)
                        return true;
        }
        if (creatureSpawnPoint.SpawnGroupLimit > 0 && AllLoadedCreaturesByMapIDThenSpawnGroupID.find(mapID) != AllLoadedCreaturesByMapIDThenSpawnGroupID.end())
        {
            unordered_map<uint32, vector<Creature*>>& spawnGroupMap = AllLoadedCreaturesByMapIDThenSpawnGroupID[mapID];
            if (spawnGroupMap.find(creatureSpawnPoint.SpawnGroupID) != spawnGroupMap.end())
            {
                uint32 aliveCount = 0;
                for (Creature* loadedCreature : spawnGroupMap[creatureSpawnPoint.SpawnGroupID])
                    if (loadedCreature != creature && loadedCreature->IsAlive() == true)
                        aliveCount++;
                if (aliveCount >= creatureSpawnPoint.SpawnGroupLimit)
                    return true;
            }
        }
    }

    return false;
}

bool EverQuestMod::HasCreatureDataForCreatureTemplateID(uint32 creatureTemplateID)
{
    if (CreaturesByTemplateID.find(creatureTemplateID) != CreaturesByTemplateID.end())
        return true;
    else
        return false;
}

const EverQuestCreature& EverQuestMod::GetCreatureDataForCreatureTemplateID(uint32 creatureTemplateID)
{
    if (CreaturesByTemplateID.find(creatureTemplateID) != CreaturesByTemplateID.end())
    {
        return CreaturesByTemplateID[creatureTemplateID];
    }
    else
    {
        static const EverQuestCreature returnEmpty;
        return returnEmpty;
    }
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
            EverQuestCreatureOnkillReputation creatureOnkillReputation;
            creatureOnkillReputation.CreatureTemplateID = fields[0].Get<uint32>();
            creatureOnkillReputation.SortOrder = fields[1].Get<uint8>();
            creatureOnkillReputation.FactionID = fields[2].Get<uint32>();
            creatureOnkillReputation.KillRewardValue = fields[3].Get<int32>();
            CreatureOnkillReputationsByCreatureTemplateID[creatureOnkillReputation.CreatureTemplateID].push_back(creatureOnkillReputation);
        } while (queryResult->NextRow());
    }
}

const list<EverQuestCreatureOnkillReputation>& EverQuestMod::GetOnkillReputationsForCreatureTemplate(uint32 creatureTemplateID)
{
    if (CreatureOnkillReputationsByCreatureTemplateID.find(creatureTemplateID) != CreatureOnkillReputationsByCreatureTemplateID.end())
    {
        return CreatureOnkillReputationsByCreatureTemplateID[creatureTemplateID];
    }
    else
    {
        static const list<EverQuestCreatureOnkillReputation> returnEmpty;
        return returnEmpty;
    }
}

void EverQuestMod::LoadItemTemplateData()
{
    ItemTemplatesByEntryID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT ItemTemplateID, NPCEquipItemTemplateID FROM mod_everquest_item_template ORDER BY ItemTemplateID;");
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            EverQuestItemTemplate everQuestItemTemplate;
            everQuestItemTemplate.ItemTemplateEntryID = fields[0].Get<uint32>();
            everQuestItemTemplate.ItemTemplateEntryIDForNPCEquip = fields[1].Get<uint32>();
            ItemTemplatesByEntryID[everQuestItemTemplate.ItemTemplateEntryID] = everQuestItemTemplate;
        } while (queryResult->NextRow());
    }
}

uint32 EverQuestMod::GetNPCEquipItemTemplateIDForItemTemplate(uint32 itemTemplateID)
{
    if (ItemTemplatesByEntryID.find(itemTemplateID) == ItemTemplatesByEntryID.end())
        return itemTemplateID;
    else
        return ItemTemplatesByEntryID[itemTemplateID].ItemTemplateEntryIDForNPCEquip;
}

void EverQuestMod::LoadSpellData()
{
    SpellDataBySpellID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT SpellID, AuraDurationBaseInMS, AuraDurationAddPerLevelInMS, AuraDurationMaxInMS, AuraDurationCalcMinLevel, AuraDurationCalcMaxLevel, RecourseSpellID, SpellIDCastOnMeleeAttacker, FocusBoostType, PeriodicAuraSpellID, PeriodicAuraSpellRadius, MaleFormSpellID, FemaleFormSpellID FROM mod_everquest_spell ORDER BY SpellID;");
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            EverQuestSpell everQuestSpell;
            everQuestSpell.SpellID = fields[0].Get<uint32>();
            everQuestSpell.AuraDurationBaseInMS = fields[1].Get<uint32>();
            everQuestSpell.AuraDurationAddPerLevelInMS = fields[2].Get<uint32>();
            everQuestSpell.AuraDurationMaxInMS = fields[3].Get<uint32>();
            everQuestSpell.AuraDurationCalcMinLevel = fields[4].Get<uint32>();
            everQuestSpell.AuraDurationCalcMaxLevel = fields[5].Get<uint32>();
            everQuestSpell.RecourseSpellID = fields[6].Get<uint32>();
            everQuestSpell.SpellIDCastOnMeleeAttacker = fields[7].Get<uint32>();
            everQuestSpell.FocusBoostType = fields[8].Get<uint32>();
            everQuestSpell.PeriodicAuraSpellID = fields[9].Get<uint32>();
            everQuestSpell.PeriodicAuraSpellRadius = fields[10].Get<uint32>();
            everQuestSpell.MaleFormSpellID = fields[11].Get<uint32>();
            everQuestSpell.FemaleFormSpellID = fields[12].Get<uint32>();
            SpellDataBySpellID[everQuestSpell.SpellID] = everQuestSpell;
        } while (queryResult->NextRow());
    }
}

const EverQuestSpell& EverQuestMod::GetSpellDataForSpellID(uint32 spellID)
{
    if (SpellDataBySpellID.find(spellID) != SpellDataBySpellID.end())
    {
        return SpellDataBySpellID[spellID];
    }
    else
    {
        static const EverQuestSpell returnEmpty;
        return returnEmpty;
    }
}

void EverQuestMod::LoadQuestCompletionReputations()
{
    QuestCompletionReputationsByQuestTemplateID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT QuestTemplateID, SortOrder, FactionID, CompletionRewardValue FROM mod_everquest_quest_complete_reputation ORDER BY QuestTemplateID, SortOrder;");
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            EverQuestQuestCompletionReputation questCompletionReputation;
            questCompletionReputation.QuestTemplateID = fields[0].Get<uint32>();
            questCompletionReputation.SortOrder = fields[1].Get<uint8>();
            questCompletionReputation.FactionID = fields[2].Get<uint32>();
            questCompletionReputation.CompletionRewardValue = fields[3].Get<int32>();
            QuestCompletionReputationsByQuestTemplateID[questCompletionReputation.QuestTemplateID].push_back(questCompletionReputation);
        } while (queryResult->NextRow());
    }
}

const list<EverQuestQuestCompletionReputation>& EverQuestMod::GetQuestCompletionReputationsForQuestTemplate(uint32 questTemplateID)
{
    if (QuestCompletionReputationsByQuestTemplateID.find(questTemplateID) != QuestCompletionReputationsByQuestTemplateID.end())
    {
        return QuestCompletionReputationsByQuestTemplateID[questTemplateID];
    }
    else
    {
        static const list<EverQuestQuestCompletionReputation> returnEmpty;
        return returnEmpty;
    }
}

void EverQuestMod::LoadQuestReactions()
{
    QuestReactionListByQuestTemplateID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT QuestTemplateID, ReactionType, UsePlayerX, UsePlayerY, UsePlayerZ, AddedPlayerX, AddedPlayerY, UsePlayerOrientation, PositionX, PositionY, PositionZ, Orientation, CreatureTemplateID FROM mod_everquest_quest_reaction;");
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            EverQuestQuestReaction everQuestQuestReaction;
            everQuestQuestReaction.QuestTemplateID = fields[0].Get<uint32>();
            everQuestQuestReaction.ReactionType = fields[1].Get<int32>();
            everQuestQuestReaction.UsePlayerX = fields[2].Get<bool>();
            everQuestQuestReaction.UsePlayerY = fields[3].Get<bool>();
            everQuestQuestReaction.UsePlayerZ = fields[4].Get<bool>();
            everQuestQuestReaction.AddedPlayerX = fields[5].Get<float>();
            everQuestQuestReaction.AddedPlayerY = fields[6].Get<float>();
            everQuestQuestReaction.UsePlayerOrientation = fields[7].Get<bool>();
            everQuestQuestReaction.PositionX = fields[8].Get<float>();
            everQuestQuestReaction.PositionY = fields[9].Get<float>();
            everQuestQuestReaction.PositionZ = fields[10].Get<float>();
            everQuestQuestReaction.Orientation = fields[11].Get<float>();
            everQuestQuestReaction.CreatureTemplateID = fields[12].Get<uint32>();
            QuestReactionListByQuestTemplateID[everQuestQuestReaction.QuestTemplateID].push_back(everQuestQuestReaction);
        } while (queryResult->NextRow());
    }
}

const list<EverQuestQuestReaction>& EverQuestMod::GetQuestReactions(uint32 questTemplateID)
{
    if (QuestReactionListByQuestTemplateID.find(questTemplateID) != QuestReactionListByQuestTemplateID.end())
    {
        return QuestReactionListByQuestTemplateID[questTemplateID];
    }
    else
    {
        static const list<EverQuestQuestReaction> returnEmpty;
        return returnEmpty;
    }
}

void EverQuestMod::LoadPetData()
{
    PetDataByCreatureTemplateID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT CreatingSpellID, NamingType, CreatureTemplateID, SummonPropertiesID, MainhandItemID, OffhandItemID FROM mod_everquest_pet ORDER BY CreatingSpellID;");
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            EverQuestPet everQuestPet;
            everQuestPet.CreatingSpellID = fields[0].Get<int32>();
            everQuestPet.NamingType = fields[1].Get<int32>();
            everQuestPet.CreatureTemplateID = fields[2].Get<uint32>();
            everQuestPet.SummonPropertiesID = fields[3].Get<int32>();
            everQuestPet.MainhandItemTemplateID = fields[4].Get<int32>();
            everQuestPet.OffhandItemTemplateID = fields[5].Get<int32>();
            PetDataByCreatureTemplateID[everQuestPet.CreatureTemplateID] = everQuestPet;
        } while (queryResult->NextRow());
    }
}

bool EverQuestMod::HasPetDataForCreatureTemplateID(uint32 creatureTemplateID)
{
    if (PetDataByCreatureTemplateID.find(creatureTemplateID) != PetDataByCreatureTemplateID.end())
        return true;
    else
        return false;
}

const EverQuestPet& EverQuestMod::GetPetDataForCreatureTemplateID(uint32 creatureTemplateID)
{
    if (PetDataByCreatureTemplateID.find(creatureTemplateID) != PetDataByCreatureTemplateID.end())
    {
        return PetDataByCreatureTemplateID[creatureTemplateID];
    }
    else
    {
        static const EverQuestPet returnEmpty;
        return returnEmpty;
    }
}

void EverQuestMod::LoadCreatePlayerData()
{
    PlayerCreateInfoByRaceIDThenClassID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT race, class, map, zone, position_x, position_y, position_z, orientation FROM mod_everquest_playercreateinfo;");
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            EverQuestPlayerCreateInfo everQuestPlayerCreateInfo;
            everQuestPlayerCreateInfo.RaceID = fields[0].Get<uint8>();
            everQuestPlayerCreateInfo.ClassID = fields[1].Get<uint8>();
            everQuestPlayerCreateInfo.MapID = fields[2].Get<uint32>();
            everQuestPlayerCreateInfo.ZoneID = fields[3].Get<uint32>();
            everQuestPlayerCreateInfo.PositionX = fields[4].Get<float>();
            everQuestPlayerCreateInfo.PositionY = fields[5].Get<float>();
            everQuestPlayerCreateInfo.PositionZ = fields[6].Get<float>();
            everQuestPlayerCreateInfo.Orientation = fields[7].Get<float>();
            PlayerCreateInfoByRaceIDThenClassID[everQuestPlayerCreateInfo.RaceID][everQuestPlayerCreateInfo.ClassID] = everQuestPlayerCreateInfo;
        } while (queryResult->NextRow());
    }
}

void EverQuestMod::LoadAutoLearnSkillsData()
{
    PlayerAutoLearnSkillsByClassID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT class, skill FROM mod_everquest_playerautolearnskills;");
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            uint8 classID = fields[0].Get<uint8>();
            uint32 skillID = fields[1].Get<uint32>();
            PlayerAutoLearnSkillsByClassID[classID].push_back(skillID);
        } while (queryResult->NextRow());
    }
}

const list<uint32>& EverQuestMod::GetAutoLearnSkillsForClass(uint8 classID)
{
    if (PlayerAutoLearnSkillsByClassID.find(classID) != PlayerAutoLearnSkillsByClassID.end())
    {
        return PlayerAutoLearnSkillsByClassID[classID];
    }
    else
    {
        static const list<uint32> returnEmpty;
        return returnEmpty;
    }
}

void EverQuestMod::LoadAutoLearnSpellsData()
{
    PlayerAutoLearnSpellsByClassID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT class, spell, addtobar FROM mod_everquest_playerautolearnspells;");
    if (queryResult)
    {
        do
        {
            EverQuestAutoLearnSpell autoLearnSpell;
            Field* fields = queryResult->Fetch();
            autoLearnSpell.ClassID = fields[0].Get<uint8>();
            autoLearnSpell.SpellID = fields[1].Get<uint32>();
            autoLearnSpell.DoAddToBar = fields[2].Get<int8>() == 1 ? true : false;
            PlayerAutoLearnSpellsByClassID[autoLearnSpell.ClassID].push_back(autoLearnSpell);
        } while (queryResult->NextRow());
    }
}

const list<EverQuestAutoLearnSpell>& EverQuestMod::GetAutoLearnSpellsForClass(uint8 classID)
{
    if (PlayerAutoLearnSpellsByClassID.find(classID) != PlayerAutoLearnSpellsByClassID.end())
    {
        return PlayerAutoLearnSpellsByClassID[classID];
    }
    else
    {
        static const list<EverQuestAutoLearnSpell> returnEmpty;
        return returnEmpty;
    }
}

bool EverQuestMod::HasCreatePlayerData(uint8 raceID, uint8 classID)
{
    if (PlayerCreateInfoByRaceIDThenClassID.find(raceID) == PlayerCreateInfoByRaceIDThenClassID.end())
        return false;
    else if (PlayerCreateInfoByRaceIDThenClassID[raceID].find(classID) == PlayerCreateInfoByRaceIDThenClassID[raceID].end())
        return false;
    else
        return true;
}

const EverQuestPlayerCreateInfo& EverQuestMod::GetPlayerCreateInfo(uint8 raceID, uint8 classID)
{
    if (PlayerCreateInfoByRaceIDThenClassID.find(raceID) != PlayerCreateInfoByRaceIDThenClassID.end())
    {
        if (PlayerCreateInfoByRaceIDThenClassID[raceID].find(classID) != PlayerCreateInfoByRaceIDThenClassID[raceID].end())
            return PlayerCreateInfoByRaceIDThenClassID[raceID][classID];
    }

    static const EverQuestPlayerCreateInfo returnEmpty;
    return returnEmpty;
}

void EverQuestMod::LoadCreatureLootData()
{
    CreatureLootGroupsByCreatureTemplateID.clear();

    // Rows are ordered so that all entries of a creature's loot group are next to each other
    QueryResult queryResult = WorldDatabase.Query("SELECT CreatureTemplateID, LootGroupID, GroupMultiplier, GroupProbability, DropLimit, MinDrop, ItemTemplateID, Chance, ItemMultiplier, ItemCharges FROM mod_everquest_creature_loot ORDER BY CreatureTemplateID, LootGroupID");
    if (queryResult)
    {
        do
        {
            Field* fields = queryResult->Fetch();
            uint32 creatureTemplateID = fields[0].Get<uint32>();
            uint32 lootGroupID = fields[1].Get<uint32>();

            vector<EverQuestCreatureLootGroup>& lootGroups = CreatureLootGroupsByCreatureTemplateID[creatureTemplateID];

            // Find or create the group for this LootGroupID (entries for the same group are contiguous)
            EverQuestCreatureLootGroup* lootGroup = nullptr;
            if (lootGroups.empty() == false && lootGroups.back().LootGroupID == lootGroupID)
                lootGroup = &lootGroups.back();
            else
            {
                EverQuestCreatureLootGroup newLootGroup;
                newLootGroup.LootGroupID = lootGroupID;
                newLootGroup.GroupMultiplier = fields[2].Get<uint32>();
                newLootGroup.GroupProbability = fields[3].Get<float>();
                newLootGroup.DropLimit = fields[4].Get<uint32>();
                newLootGroup.MinDrop = fields[5].Get<uint32>();
                lootGroups.push_back(newLootGroup);
                lootGroup = &lootGroups.back();
            }

            EverQuestCreatureLootEntry lootEntry;
            lootEntry.ItemTemplateID = fields[6].Get<uint32>();
            lootEntry.Chance = fields[7].Get<float>();
            lootEntry.ItemMultiplier = fields[8].Get<uint32>();
            lootEntry.ItemCharges = fields[9].Get<uint32>();
            lootGroup->Entries.push_back(lootEntry);
        } while (queryResult->NextRow());
    }
}

bool EverQuestMod::HasCreatureLootDataForCreatureTemplateEntryID(uint32 creatureTemplateEntryID)
{
    if (CreatureLootGroupsByCreatureTemplateID.find(creatureTemplateEntryID) == CreatureLootGroupsByCreatureTemplateID.end())
        return false;
    return true;
}

bool EverQuestMod::HasPreloadedLootItemIDsForCreatureGUID(ObjectGuid creatureGUID)
{
    if (PreloadedLootItemIDsByCreatureGUID.find(creatureGUID) != PreloadedLootItemIDsByCreatureGUID.end())
        return true;
    else
        return false;
}

bool EverQuestMod::HasPreloadedLootItemIDForCreatureGUID(ObjectGuid creatureGUID, uint32 itemTemplateID)
{
    if (HasPreloadedLootItemIDsForCreatureGUID(creatureGUID) == false)
        return false;

    for (uint32 preloadedLootItemTemplateID : EverQuest->GetPreloadedLootIDsForCreatureGUID(creatureGUID))
    {
        if (preloadedLootItemTemplateID == itemTemplateID)
            return true;
    }
    return false;
}

uint32 EverQuestMod::GetPreloadedLootCountForCreatureGUID(ObjectGuid creatureGUID, uint32 itemTemplateID)
{
    auto countsByItem = PreloadedLootCountsByCreatureGUID.find(creatureGUID);
    if (countsByItem == PreloadedLootCountsByCreatureGUID.end())
        return 0;
    auto count = countsByItem->second.find(itemTemplateID);
    if (count == countsByItem->second.end())
        return 0;
    return count->second;
}

const vector<uint32>& EverQuestMod::GetPreloadedLootIDsForCreatureGUID(ObjectGuid creatureGUID)
{
    if (PreloadedLootItemIDsByCreatureGUID.find(creatureGUID) != PreloadedLootItemIDsByCreatureGUID.end())
    {
        return PreloadedLootItemIDsByCreatureGUID[creatureGUID];
    }
    else
    {
        static vector<uint32> returnEmpty;
        return returnEmpty;
    }
}

void EverQuestMod::ClearPreloadedLootIDsForCreatureGUID(ObjectGuid creatureGUID)
{
    if (PreloadedLootItemIDsByCreatureGUID.find(creatureGUID) != PreloadedLootItemIDsByCreatureGUID.end())
    {
        PreloadedLootItemIDsByCreatureGUID.erase(creatureGUID);
    }
    if (PreloadedLootCountsByCreatureGUID.find(creatureGUID) != PreloadedLootCountsByCreatureGUID.end())
    {
        PreloadedLootCountsByCreatureGUID.erase(creatureGUID);
    }
}

void EverQuestMod::TrackVisualEquippedItemsForCreatureGUID(ObjectGuid creatureGUID, uint32 mainhandItemID, uint32 offhandItemID, bool isDualWielding)
{
    VisualEquippedItemsByCreatureGUID[creatureGUID].MainhandItemID = mainhandItemID;
    VisualEquippedItemsByCreatureGUID[creatureGUID].OffhandItemID = offhandItemID;
    VisualEquippedItemsByCreatureGUID[creatureGUID].IsDualWielding = isDualWielding;
}

bool EverQuestMod::IsCreatureDualWielding(ObjectGuid creatureGUID)
{
    auto it = VisualEquippedItemsByCreatureGUID.find(creatureGUID);
    if (it == VisualEquippedItemsByCreatureGUID.end())
        return false;
    return it->second.IsDualWielding;
}

// Formula based from formulas in TAKP (EQMacEmu/Server)
uint32 EverQuestMod::GetEQNPCMeleeWeaponSkillForLevel(uint32 level)
{
    if (level <= 7)
        return 0;
    if (level > 50)
        return 250;
    return std::min<uint32>(level * 5, 210);
}

// Formula based from formulas in TAKP (EQMacEmu/Server). Driven from landed main-hand auto attack.
void EverQuestMod::TryDoCreatureEQMeleeExtraAttacks(Unit* attacker, Unit* victim)
{
    if (attacker == nullptr || victim == nullptr)
        return;
    if (attacker->IsCreature() == false)
        return;
    if (victim->IsAlive() == false)
        return;

    // Prevent injected swings from repeating
    ObjectGuid attackerGUID = attacker->GetGUID();
    if (CreaturesResolvingEQMeleeExtraAttacks.count(attackerGUID) > 0)
        return;

    Creature* creature = attacker->ToCreature();

    // Restrict to EverQuest zones
    uint32 mapID = creature->GetMap()->GetId();
    if (mapID < ConfigSystemMapDBCIDMin || mapID > ConfigSystemMapDBCIDMax)
        return;

    uint32 level = creature->GetLevel();
    uint32 weaponSkill = GetEQNPCMeleeWeaponSkillForLevel(level);
    if (weaponSkill == 0)
        return;

    uint32 effectiveSkill = weaponSkill;
    if (level > 35)
        effectiveSkill += level;

    CreaturesResolvingEQMeleeExtraAttacks.insert(attackerGUID);

    // Main-hand double attack. "effectiveSkill" out of 500. Warrior creatures 60+ rolls for a triple attack at 13.5%
    if (effectiveSkill > urand(0, 499))
    {
        if (victim->IsAlive() == true)
            creature->AttackerStateUpdate(victim, BASE_ATTACK, true);

        if (victim->IsAlive() == true && creature->getClass() == CLASS_WARRIOR && level >= 60 && urand(0, 999) < 135)
            creature->AttackerStateUpdate(victim, BASE_ATTACK, true);
    }

    // For creatures with an off-hand weapon only, calc per-round chance out of effectiveSkill 375.
    // Also, connecting off-hand swing can double attack once the creature's skil reaches 150 (level 30+)
    if (victim->IsAlive() == true && IsCreatureDualWielding(attackerGUID) == true)
    {
        if (effectiveSkill > urand(0, 374))
        {
            if (victim->IsAlive() == true)
                creature->AttackerStateUpdate(victim, BASE_ATTACK, true);

            if (victim->IsAlive() == true && weaponSkill >= 150 && effectiveSkill > urand(0, 499))
                creature->AttackerStateUpdate(victim, BASE_ATTACK, true);
        }
    }

    CreaturesResolvingEQMeleeExtraAttacks.erase(attackerGUID);
}

void EverQuestMod::ClearVisualEquippedItemsForCreatureGUID(ObjectGuid creatureGUID)
{
    if (VisualEquippedItemsByCreatureGUID.find(creatureGUID) != VisualEquippedItemsByCreatureGUID.end())
        VisualEquippedItemsByCreatureGUID.erase(creatureGUID);
}

void EverQuestMod::RemoveVisualEquippedItemForCreatureGUIDIfExists(Map* map, ObjectGuid creatureGUID, uint32 itemTemplateID)
{
    if (VisualEquippedItemsByCreatureGUID.find(creatureGUID) == VisualEquippedItemsByCreatureGUID.end())
        return;

    Creature* creature = map->GetCreature(creatureGUID);
    if (!creature)
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod::RemoveVisualEquippedItemForCreatureGUIDIfExists failure, as creature with GUID could not be found in the map");
        return;
    }

    uint32 npcItemTemplateID = EverQuest->GetNPCEquipItemTemplateIDForItemTemplate(itemTemplateID);

    // Mainhand first, then offhand
    if (VisualEquippedItemsByCreatureGUID[creatureGUID].MainhandItemID == npcItemTemplateID)
    {
        creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 0, 0);
        VisualEquippedItemsByCreatureGUID[creatureGUID].MainhandItemID = 0;
    }
    else if (VisualEquippedItemsByCreatureGUID[creatureGUID].OffhandItemID == npcItemTemplateID)
    {
        creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, 0);
        VisualEquippedItemsByCreatureGUID[creatureGUID].OffhandItemID = 0;
    }
}

void EverQuestMod::LoadShipTriggerData()
{
    ShipTriggersByTriggeringGameObjectTemplateEntryID.clear();

    // Pulls in all the kill faction rewards
    QueryResult queryResult = WorldDatabase.Query("SELECT TriggeringShipEntryID, TriggeredShipEntryID, TriggeringNodeID, TriggeredActivateNodeID FROM mod_everquest_transport_trigger;");
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            EverQuestTransportShipTrigger shipTrigger;
            shipTrigger.TriggeringShipGameObjectEntryTemplateID = fields[0].Get<uint32>();
            shipTrigger.TriggeredShipGameObjectTemplateEntryID = fields[1].Get<uint32>();
            shipTrigger.TriggeringNodeID = fields[2].Get<uint32>();
            shipTrigger.TriggerActivateNodeID = fields[3].Get<int32>();
            ShipTriggersByTriggeringGameObjectTemplateEntryID[shipTrigger.TriggeringShipGameObjectEntryTemplateID].push_back(shipTrigger);
            ShipWaitNodesByGameObjectTemplateEntryID[shipTrigger.TriggeredShipGameObjectTemplateEntryID] = shipTrigger.TriggerActivateNodeID;
        } while (queryResult->NextRow());
    }
}

const vector<EverQuestTransportShipTrigger>& EverQuestMod::GetShipTriggersForShip(int triggeringGameObjectTemplateEntryID)
{
    if (ShipTriggersByTriggeringGameObjectTemplateEntryID.find(triggeringGameObjectTemplateEntryID) != ShipTriggersByTriggeringGameObjectTemplateEntryID.end())
    {
        return ShipTriggersByTriggeringGameObjectTemplateEntryID[triggeringGameObjectTemplateEntryID];
    }
    else
    {
        static const vector<EverQuestTransportShipTrigger> returnEmpty;
        return returnEmpty;
    }
}

void EverQuestMod::LoadCreatureInstanceData()
{
    CreatureInstancesByCreatureGUID.clear();

    QueryResult queryResult = WorldDatabase.Query("SELECT CreatureGUID, WanderType, PauseType, MapID, WaypointID, DoesRoam, RoamMinX, RoamMaxX, RoamMinY, RoamMaxY, RoamMinZ, RoamMaxZ, RoamMinDelayInMS, RoamMaxDelayInMS, DespawnAtWaypointNum, DisableGroundContour FROM mod_everquest_creature_instance;");
    if (queryResult)
    {
        do
        {
            Field* fields = queryResult->Fetch();
            EverQuestCreatureInstance creatureInstance;
            creatureInstance.CreatureGUID = fields[0].Get<uint32>();
            creatureInstance.WanderType = fields[1].Get<int8>();
            creatureInstance.PauseType = fields[2].Get<int8>();
            creatureInstance.MapID = fields[3].Get<uint32>();
            creatureInstance.WaypointListID = fields[4].Get<uint32>();
            creatureInstance.DoesRoam = fields[5].Get<int8>() == 1 ? true : false;
            creatureInstance.RoamMinX = fields[6].Get<float>();
            creatureInstance.RoamMaxX = fields[7].Get<float>();
            creatureInstance.RoamMinY = fields[8].Get<float>();
            creatureInstance.RoamMaxY = fields[9].Get<float>();
            creatureInstance.RoamMinZ = fields[10].Get<float>();
            creatureInstance.RoamMaxZ = fields[11].Get<float>();
            creatureInstance.RoamMinDelayInMS = fields[12].Get<uint32>();
            creatureInstance.RoamMaxDelayInMS = fields[13].Get<uint32>();
            creatureInstance.DespawnAtWaypointNum = fields[14].Get<int32>();
            creatureInstance.DisableGroundContour = fields[15].Get<uint8>() == 1 ? true : false;
            CreatureInstancesByCreatureGUID[creatureInstance.CreatureGUID] = creatureInstance;
        } while (queryResult->NextRow());
    }
}

const EverQuestCreatureInstance& EverQuestMod::GetCreatureInstanceData(uint32 creatureInstanceGUID)
{
    if (CreatureInstancesByCreatureGUID.find(creatureInstanceGUID) != CreatureInstancesByCreatureGUID.end())
    {
        return CreatureInstancesByCreatureGUID[creatureInstanceGUID];
    }
    else
    {
        static const EverQuestCreatureInstance returnEmpty;
        return returnEmpty;
    }
}

void EverQuestMod::LoadCreatureWaypointData()
{
    CreatureWaypointsByMapIDAndWaypointID.clear();

    QueryResult queryResult = WorldDatabase.Query("SELECT MapID, WaypointID, Number, X, Y, Z, PauseInSec FROM mod_everquest_creature_waypoint;");
    if (queryResult)
    {
        do
        {
            Field* fields = queryResult->Fetch();
            EverQuestCreatureWaypoint creatureWaypoint;
            creatureWaypoint.MapID = fields[0].Get<uint32>();
            creatureWaypoint.WaypointID = fields[1].Get<uint32>();
            creatureWaypoint.Number = fields[2].Get<uint32>();
            creatureWaypoint.X = fields[3].Get<float>();
            creatureWaypoint.Y = fields[4].Get<float>();
            creatureWaypoint.Z = fields[5].Get<float>();
            creatureWaypoint.PauseInSec = fields[6].Get<uint32>();
            CreatureWaypointsByMapIDAndWaypointID[creatureWaypoint.MapID][creatureWaypoint.WaypointID].push_back(creatureWaypoint);
        } while (queryResult->NextRow());
    }
}

const vector<EverQuestCreatureWaypoint> EverQuestMod::GetWaypoints(uint32 mapID, uint32 waypointListID)
{
    auto outerIt = CreatureWaypointsByMapIDAndWaypointID.find(mapID);
    if (outerIt == CreatureWaypointsByMapIDAndWaypointID.end())
        return {};
    const unordered_map<uint32, vector<EverQuestCreatureWaypoint>>& innerMap = outerIt->second;
    if (innerMap.find(waypointListID) == innerMap.end())
        return {};

    return innerMap.at(waypointListID);
}

void EverQuestMod::LoadForageData()
{
    ForageZoneItemsByMapID.clear();
    ForageZoneItemTotalChanceByMapID.clear();

    QueryResult queryResult = WorldDatabase.Query("SELECT MapID, ItemTemplateID, Chance, ForageType FROM mod_everquest_forage_zone_items;");
    if (queryResult)
    {
        do
        {
            Field* fields = queryResult->Fetch();
            EverQuestForageZoneItem forageZoneItem;
            forageZoneItem.MapID = fields[0].Get<uint32>();
            forageZoneItem.ItemTemplateID = fields[1].Get<uint32>();
            forageZoneItem.Chance = fields[2].Get<uint32>();
            forageZoneItem.ForageType = fields[3].Get<uint32>();
            ForageZoneItemsByMapID[forageZoneItem.MapID].push_back(forageZoneItem);
            ForageZoneItemTotalChanceByMapID[forageZoneItem.MapID] += forageZoneItem.Chance;
        } while (queryResult->NextRow());
    }
}

const vector<EverQuestForageZoneItem>& EverQuestMod::GetForageZoneItemsInMap(uint32 mapID)
{
    if (ForageZoneItemsByMapID.find(mapID) != ForageZoneItemsByMapID.end())
    {
        return ForageZoneItemsByMapID[mapID];
    }
    else
    {
        static const vector<EverQuestForageZoneItem> returnEmpty;
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
    //uint32 zoneId = fields[1].Get<uint32>();
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
    //uint32 zoneId = fields[1].Get<uint32>();
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
    int playerGUIDCounter = player->GetGUID().GetCounter();

    SetNewBindHome(player, playerGUIDCounter, mapID, zoneID, playerX, playerY, playerZ);
}

void EverQuestMod::SetNewBindHome(Player* player, uint32 playerGUIDCounter, int mapID, int zoneID, float playerX, float playerY, float playerZ)
{
    // Set up the transaction
    CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();

    // Delete the old record, if it exists
    transaction->Append("DELETE FROM `mod_everquest_character_homebind` WHERE guid = {}", playerGUIDCounter);

    // Add the new binding
    transaction->Append("INSERT INTO `mod_everquest_character_homebind` (`guid`, `mapId`, `zoneId`, `posX`, `posY`, `posZ`) VALUES ({}, {}, {}, {}, {}, {})",
        playerGUIDCounter, mapID, zoneID, playerX, playerY, playerZ);

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

void EverQuestMod::AddCreatureAsLoaded(int mapID, Creature* creature)
{
    if (AllLoadedCreaturesByMapIDThenCreatureEntryID.find(mapID) == AllLoadedCreaturesByMapIDThenCreatureEntryID.end())
        AllLoadedCreaturesByMapIDThenCreatureEntryID[mapID];
    unordered_map<int, vector<Creature*>>& innerMap = AllLoadedCreaturesByMapIDThenCreatureEntryID[mapID];
    if (innerMap.find(creature->GetEntry()) != innerMap.end())
        innerMap[creature->GetEntry()];
    innerMap[creature->GetEntry()].push_back(creature);

    // Track by spawn point and spawn group, if this creature has one
    if (creature->GetSpawnId() != 0 && CreatureSpawnPointsByCreatureGUID.find(creature->GetSpawnId()) != CreatureSpawnPointsByCreatureGUID.end())
    {
        const EverQuestCreatureSpawnPoint& creatureSpawnPoint = CreatureSpawnPointsByCreatureGUID[creature->GetSpawnId()];
        AllLoadedCreaturesByMapIDThenSpawnPointID[mapID][creatureSpawnPoint.SpawnPointID].push_back(creature);
        AllLoadedCreaturesByMapIDThenSpawnGroupID[mapID][creatureSpawnPoint.SpawnGroupID].push_back(creature);
    }
}

void EverQuestMod::RemoveCreatureAsLoaded(int mapID, Creature* creature)
{
    if (AllLoadedCreaturesByMapIDThenCreatureEntryID.find(mapID) == AllLoadedCreaturesByMapIDThenCreatureEntryID.end())
        return;
    unordered_map<int, vector<Creature*>>& innerMap = AllLoadedCreaturesByMapIDThenCreatureEntryID[mapID];
    int creatureEntryID = creature->GetEntry();
    if (innerMap.find(creatureEntryID) == innerMap.end())
        return;

    vector<Creature*>& creatureVector = innerMap[creatureEntryID];
    vector<Creature*>::iterator it = find(creatureVector.begin(), creatureVector.end(), creature);
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

    // Remove from the spawn point and spawn group trackers, if this creature has one
    if (creature->GetSpawnId() != 0 && CreatureSpawnPointsByCreatureGUID.find(creature->GetSpawnId()) != CreatureSpawnPointsByCreatureGUID.end())
    {
        const EverQuestCreatureSpawnPoint& creatureSpawnPoint = CreatureSpawnPointsByCreatureGUID[creature->GetSpawnId()];
        if (AllLoadedCreaturesByMapIDThenSpawnPointID.find(mapID) != AllLoadedCreaturesByMapIDThenSpawnPointID.end())
        {
            unordered_map<uint32, vector<Creature*>>& spawnPointMap = AllLoadedCreaturesByMapIDThenSpawnPointID[mapID];
            if (spawnPointMap.find(creatureSpawnPoint.SpawnPointID) != spawnPointMap.end())
            {
                vector<Creature*>& spawnPointCreatureVector = spawnPointMap[creatureSpawnPoint.SpawnPointID];
                vector<Creature*>::iterator spawnPointIt = find(spawnPointCreatureVector.begin(), spawnPointCreatureVector.end(), creature);
                if (spawnPointIt != spawnPointCreatureVector.end())
                {
                    spawnPointCreatureVector.erase(spawnPointIt);
                    if (spawnPointCreatureVector.empty())
                    {
                        spawnPointMap.erase(creatureSpawnPoint.SpawnPointID);
                        if (spawnPointMap.empty())
                            AllLoadedCreaturesByMapIDThenSpawnPointID.erase(mapID);
                    }
                }
            }
        }
        if (AllLoadedCreaturesByMapIDThenSpawnGroupID.find(mapID) != AllLoadedCreaturesByMapIDThenSpawnGroupID.end())
        {
            unordered_map<uint32, vector<Creature*>>& spawnGroupMap = AllLoadedCreaturesByMapIDThenSpawnGroupID[mapID];
            if (spawnGroupMap.find(creatureSpawnPoint.SpawnGroupID) != spawnGroupMap.end())
            {
                vector<Creature*>& spawnGroupCreatureVector = spawnGroupMap[creatureSpawnPoint.SpawnGroupID];
                vector<Creature*>::iterator spawnGroupIt = find(spawnGroupCreatureVector.begin(), spawnGroupCreatureVector.end(), creature);
                if (spawnGroupIt != spawnGroupCreatureVector.end())
                {
                    spawnGroupCreatureVector.erase(spawnGroupIt);
                    if (spawnGroupCreatureVector.empty())
                    {
                        spawnGroupMap.erase(creatureSpawnPoint.SpawnGroupID);
                        if (spawnGroupMap.empty())
                            AllLoadedCreaturesByMapIDThenSpawnGroupID.erase(mapID);
                    }
                }
            }
        }
    }

    if (EverQuest->PreloadedLootItemIDsByCreatureGUID.find(creature->GetGUID()) != EverQuest->PreloadedLootItemIDsByCreatureGUID.end())
        EverQuest->PreloadedLootItemIDsByCreatureGUID.erase(creature->GetGUID());
    if (EverQuest->VisualEquippedItemsByCreatureGUID.find(creature->GetGUID()) != EverQuest->VisualEquippedItemsByCreatureGUID.end())
        EverQuest->VisualEquippedItemsByCreatureGUID.erase(creature->GetGUID());
}

vector<Creature*> EverQuestMod::GetLoadedCreaturesWithEntryID(int mapID, uint32 entryID)
{
    if (AllLoadedCreaturesByMapIDThenCreatureEntryID.find(mapID) == AllLoadedCreaturesByMapIDThenCreatureEntryID.end())
        return vector<Creature*>();
    unordered_map<int, vector<Creature*>>& innerMap = AllLoadedCreaturesByMapIDThenCreatureEntryID[mapID];
    if (innerMap.find(entryID) == innerMap.end())
        return vector<Creature*>();
    return innerMap[entryID];
}

void EverQuestMod::RollLootItemsForCreature(ObjectGuid creatureGUID, uint32 creatureTemplateEntryID)
{
    // Clear previous rolls (and empty counts map means it drops nothing)
    vector<uint32>& preloadedItemIDs = PreloadedLootItemIDsByCreatureGUID[creatureGUID];
    preloadedItemIDs.clear();
    unordered_map<uint32, uint32>& counts = PreloadedLootCountsByCreatureGUID[creatureGUID];
    counts.clear();

    // Skip creatures with no loot data
    auto creatureLootGroups = CreatureLootGroupsByCreatureTemplateID.find(creatureTemplateEntryID);
    if (creatureLootGroups == CreatureLootGroupsByCreatureTemplateID.end())
        return;

    // Each loot group (lootdrop reference) is processed based on the group multiplier
    for (const EverQuestCreatureLootGroup& lootGroup : creatureLootGroups->second)
    {
        uint32 groupMultiplier = std::max(lootGroup.GroupMultiplier, 1u);
        for (uint32 t = 0; t < groupMultiplier; t++)
        {
            if (lootGroup.GroupProbability <= 0.0f)
                continue;
            if (lootGroup.GroupProbability < 100.0f && float(rand_chance()) > lootGroup.GroupProbability)
                continue;

            RollLootGroupIntoCounts(lootGroup, counts);
        }
    }

    // Track preloaded items for visuals and OnItemRoll checks
    for (const auto& itemCount : counts)
        preloadedItemIDs.push_back(itemCount.first);
}

void EverQuestMod::RollLootGroupIntoCounts(const EverQuestCreatureLootGroup& lootGroup, unordered_map<uint32, uint32>& counts)
{
    const vector<EverQuestCreatureLootEntry>& entries = lootGroup.Entries;
    if (entries.empty())
        return;

    if (lootGroup.DropLimit == 0 && lootGroup.MinDrop == 0)
    {
        for (const EverQuestCreatureLootEntry& entry : entries)
        {
            uint32 attempts = std::max(entry.ItemMultiplier, 1u);
            for (uint32 j = 0; j < attempts; j++)
                if (float(rand_chance()) <= entry.Chance)
                    counts[entry.ItemTemplateID] += std::max(entry.ItemCharges, 1u);
        }
        return;
    }

    uint32 dropLimit = lootGroup.DropLimit;
    if (entries.size() > 100 && dropLimit == 0)
        dropLimit = 10;
    if (dropLimit < lootGroup.MinDrop)
        dropLimit = lootGroup.MinDrop;

    float rollTotal = 0.0f;
    float noLootProb = 1.0f;
    bool chanceBypass = false;
    for (const EverQuestCreatureLootEntry& entry : entries)
    {
        rollTotal += entry.Chance;
        if (entry.Chance >= 100.0f)
            chanceBypass = true;
        else
            noLootProb *= (100.0f - entry.Chance) / 100.0f;
    }
    if (rollTotal <= 0.0f)
        return;

    uint32 drops = 0;
    for (uint32 i = 0; i < dropLimit; i++)
    {
        // Keep rolling while below MinDrop or a guaranteed item exists; otherwise stop with probability noLootProb
        if (drops < lootGroup.MinDrop || chanceBypass == true || frand(0.0f, 1.0f) >= noLootProb)
        {
            float roll = frand(0.0f, rollTotal);
            for (const EverQuestCreatureLootEntry& entry : entries)
            {
                if (roll < entry.Chance)
                {
                    counts[entry.ItemTemplateID] += std::max(entry.ItemCharges, 1u);
                    drops++;

                    uint32 extraAttempts = std::max(entry.ItemMultiplier, 1u);
                    for (uint32 k = 1; k < extraAttempts; k++)
                        if (float(rand_chance()) <= entry.Chance)
                            counts[entry.ItemTemplateID] += std::max(entry.ItemCharges, 1u);
                    break;
                }
                else
                    roll -= entry.Chance;
            }
        }
    }
}

void EverQuestMod::SpawnCreature(uint32 entryID, Map* map, float x, float y, float z, float orientation, bool enforceUniqueSpawn)
{
    if (!sObjectMgr->GetCreatureTemplate(entryID))
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod::SpawnCreature failure, as creature with entryID of {} did not exist in creature templates", entryID);
        return;
    }

    // Cancel out if it should be a unique spawn, and the creature exists
    if (enforceUniqueSpawn == true)
    {
        vector<Creature*> loadedCreatures = GetLoadedCreaturesWithEntryID(map->GetId(), entryID);
        if (loadedCreatures.size() > 0)
            return;
    }

    Creature* creature = new Creature();
    if (!creature->Create(map->GenerateLowGuid<HighGuid::Unit>(), map, 6, entryID, 0, x, y, z, orientation)) // PhaseMask of 6 is both day and night
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod::SpawnCreature failure, error calling creature->Create with entryID of {}", entryID);
        delete creature;
        return;
    }

    creature->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), 6);
    ObjectGuid::LowType spawnId = creature->GetSpawnId();

    // Taken from .npc add in AzerothCore core: "To call _LoadGoods(); _LoadQuests(); CreateTrainerSpells(), current "creature" variable is deleted
    // and created fresh new, otherwise old values might trigger asserts or cause undefined behavior"
    creature->CleanupsBeforeDelete();
    delete creature;
    creature = new Creature();
    if (!creature->LoadCreatureFromDB(spawnId, map, true, true))
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod::SpawnCreature failure, as creature with entryID of {} could not be loaded from the database", entryID);
        delete creature;
        return;
    }
    sObjectMgr->AddCreatureToGrid(spawnId, sObjectMgr->GetCreatureData(spawnId));
    creature->DeleteFromDB();
}

void EverQuestMod::DespawnCreature(uint32 entryID, Map* map)
{
    vector<Creature*> loadedCreatures = GetLoadedCreaturesWithEntryID(map->GetId(), entryID);
    for (Creature* creature : loadedCreatures)
        if (creature != nullptr)
            creature->DespawnOrUnsummon(0ms);
}

void EverQuestMod::MakeCreatureAttackPlayer(uint32 entryID, Map* map, Player* player)
{
    vector<Creature*> loadedCreatures = GetLoadedCreaturesWithEntryID(map->GetId(), entryID);
    for (Creature* creature : loadedCreatures)
        if (creature != nullptr)
        {
            creature->SetFaction(2300); // Make Kill-on-sight
            //creature->SetReputationRewardDisabled(true); // Prevent rep hits for mobs that go good -> bad
            creature->SetReactState(REACT_AGGRESSIVE);
            creature->SetTarget(player->GetGUID());
            creature->Attack(player, true); // Should this be false when there is magic/ranged involved?
        }
}

bool EverQuestMod::IsSpellAnEQSpell(uint32 spellID)
{
    if (SpellDataBySpellID.find(spellID) != SpellDataBySpellID.end())
        return true;
    else
        return false;
}

bool EverQuestMod::IsSpellAnEQBardSong(uint32 spellID)
{
    if (SpellDataBySpellID.find(spellID) != SpellDataBySpellID.end())
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellID);
        if (!spellInfo)
            return false;
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->Effects[i].ApplyAuraName != SPELL_AURA_PERIODIC_DUMMY)
                continue;
            if (spellInfo->Effects[i].MiscValue >= EQ_SPELLDUMMYTYPE_BARDSONGENEMYAREA && spellInfo->Effects[i].MiscValue <= EQ_SPELLDUMMYTYPE_BARDSONGANY)
                return true;
        }
    }
    return false;
}

uint32 EverQuestMod::CalculateSpellFocusBoostValue(Unit* caster, uint32 spellID)
{
    if (caster == nullptr)
        return 0;

    // Calculate the boost amount based on caster's auras
    EverQuestSpell curSpell = EverQuest->GetSpellDataForSpellID(spellID);
    uint32 boostValue = 0;
    Unit::AuraMap const& auras = caster->GetOwnedAuras();
    for (auto const& aurIter : auras)
    {
        Aura* aura = aurIter.second;
        SpellInfo const* auraInfo = aura->GetSpellInfo();
        for (uint8 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
        {
            // Focus auras are always dummy
            if (auraInfo->Effects[effIndex].ApplyAuraName != SPELL_AURA_DUMMY)
                continue;
            int auraDummyType = auraInfo->Effects[effIndex].MiscValue;

            // Match up focus types and add the boost
            if (curSpell.FocusBoostType == EQ_SPELLFOCUSBOOSTTYPE_BARDBRASS && (auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSBRASS || auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSALL))
            {
                boostValue += auraInfo->Effects[effIndex].MiscValueB;
                continue;
            }
            else if (curSpell.FocusBoostType == EQ_SPELLFOCUSBOOSTTYPE_BARDSTRINGED && (auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSSTRING || auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSALL))
            {
                boostValue += auraInfo->Effects[effIndex].MiscValueB;
                continue;
            }
            else if (curSpell.FocusBoostType == EQ_SPELLFOCUSBOOSTTYPE_BARDWIND && (auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSWIND || auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSALL))
            {
                boostValue += auraInfo->Effects[effIndex].MiscValueB;
                continue;
            }
            else if (curSpell.FocusBoostType == EQ_SPELLFOCUSBOOSTTYPE_BARDPERCUSSION && (auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSPERCUSSION || auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSALL))
            {
                boostValue += auraInfo->Effects[effIndex].MiscValueB;
                continue;
            }
            else if (curSpell.FocusBoostType == EQ_SPELLFOCUSBOOSTTYPE_BARDSINGING && auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSALL)
            {
                boostValue += auraInfo->Effects[effIndex].MiscValueB;
                continue;
            }
        }
    }
    return boostValue;
}

void EverQuestMod::ProcessForage(Player* player)
{
    if (player == nullptr)
        return;
    if (player->GetMap() == nullptr)
        return;
    uint32 mapID = player->GetMap()->GetId();
    if (mapID < EverQuest->ConfigSystemMapDBCIDMin || mapID > EverQuest->ConfigSystemMapDBCIDMax)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("There is nothing to forage outside of Norrath.");
        return;
    }
    vector<EverQuestForageZoneItem> forageZoneItems = GetForageZoneItemsInMap(mapID);
    if (forageZoneItems.empty() == true)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("This area has nothing to forage.");
        return;
    }
    int32 roll = (int32)urand(0, ForageZoneItemTotalChanceByMapID[mapID]);
    for (const EverQuestForageZoneItem& zoneItem : forageZoneItems)
    {
        roll -= zoneItem.Chance;
        if (roll <= 0)
        {
            ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(zoneItem.ItemTemplateID);
            if (!itemTemplate)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("You fail to locate any food nearby.");
                return;
            }

            ItemPosCountVec destPosition;
            InventoryResult invResult = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, destPosition, zoneItem.ItemTemplateID, 1);
            if (invResult == EQUIP_ERR_OK)
                player->StoreNewItem(destPosition, zoneItem.ItemTemplateID, true);
            else
            {
                ChatHandler(player->GetSession()).PSendSysMessage("You lack the space to pick anything up.");
                return;
            }

            string itemLink = "|cffffffff|Hitem:" + to_string(zoneItem.ItemTemplateID) + ":0:0:0:0:0:0:0:0:0|h[" + itemTemplate->Name1 + "]|h|r";
            ChatHandler(player->GetSession()).PSendSysMessage("You have recieved: " + itemLink);
            if (zoneItem.ForageType == EQ_FORAGE_TYPE_FOOD)
                ChatHandler(player->GetSession()).PSendSysMessage("You have scrounged up some food.");
            else if (zoneItem.ForageType == EQ_FORAGE_TYPE_DRINK)
                ChatHandler(player->GetSession()).PSendSysMessage("You have scrounged up some water.");
            else if (zoneItem.ForageType == EQ_FORAGE_TYPE_BAIT)
                ChatHandler(player->GetSession()).PSendSysMessage("You have scrounged up some fishing grubs.");
            else
                ChatHandler(player->GetSession()).PSendSysMessage("You have scrounged up something that doesn't look edible.");
            return;
        }
    }
    ChatHandler(player->GetSession()).PSendSysMessage("You fail to locate any food nearby.");
}
