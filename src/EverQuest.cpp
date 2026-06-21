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
#include "Tokenize.h"

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
    ConfigSystemInvisVsUndeadDetectSpellID(0),
    ConfigDeathKnightsStartLikeOtherClasses(false),
    ConfigMapRestrictPlayersToNorrath(false),    
    ConfigQuestGrantExpOnRepeatCompletion(true),
    ConfigExpLossOnDeathEnabled(true),
    ConfigExpLossOnDeathMinLevel(5),
    ConfigExpLossOnDeathLossPercent(10),
    ConfigExpLossOnDeathAddLostExpToRestExp(true),
    ConfigAlternateGroupExperienceFormulaEnabled(false),
    ConfigAlternateGroupExperienceAddPercentPerAddedMember(20.0f),
    ConfigSpellDisableStackingOfSameDOT(false),
    ConfigCombatSkillsDisableBashKickStunOnPlayers(false)
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
            else if (key == "DazeEnabledInEQZones")
                ConfigDazeEnabledInEQZones = value == "1" ? true : false;
            else if (key == "DeathKnightsStartLikeOtherClasses")
                ConfigDeathKnightsStartLikeOtherClasses = value == "1" ? true : false;
            else if (key == "GameObjectTemplateIDMin")
                ConfigSystemGameObjectTemplateIDMin = (uint32)atoi(value.c_str());
            else if (key == "GameObjectTemplateIDMax")
                ConfigSystemGameObjectTemplateIDMax = (uint32)atoi(value.c_str());
            else if (key == "InvisVsUndeadDetectSpellID")
                ConfigSystemInvisVsUndeadDetectSpellID = (uint32)atoi(value.c_str());
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
    ConfigSpellDisableStackingOfSameDOT = sConfigMgr->GetOption<bool>("EverQuest.Spells.DisableStackingOfSameDOT", false);

    // Combat Skills
    ConfigCombatSkillsDisableBashKickStunOnPlayers = sConfigMgr->GetOption<bool>("EverQuest.CombatSkills.DisableBashKickStunOnPlayers", false);

    // Cross-Class values
    ConfigCrossClassIncludeSkillIDs = GetSetFromConfigString("EverQuest.CrossClass.IncludeSkillIDs");
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
    WornEffectSpellIDs.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT ItemTemplateID, NPCEquipItemTemplateID, WornEffectSpellID, AllowedEQClassMask FROM mod_everquest_item_template ORDER BY ItemTemplateID;");
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            EverQuestItemTemplate everQuestItemTemplate;
            everQuestItemTemplate.ItemTemplateEntryID = fields[0].Get<uint32>();
            everQuestItemTemplate.ItemTemplateEntryIDForNPCEquip = fields[1].Get<uint32>();
            everQuestItemTemplate.WornEffectSpellID = fields[2].Get<uint32>();
            everQuestItemTemplate.AllowedEQClassMask = fields[3].Get<uint32>();
            ItemTemplatesByEntryID[everQuestItemTemplate.ItemTemplateEntryID] = everQuestItemTemplate;
            if (everQuestItemTemplate.WornEffectSpellID != 0)
                WornEffectSpellIDs.insert(everQuestItemTemplate.WornEffectSpellID);
        } while (queryResult->NextRow());
    }
}

bool EverQuestMod::IsWornEffectSpell(uint32 spellID)
{
    return WornEffectSpellIDs.find(spellID) != WornEffectSpellIDs.end();
}

uint32 EverQuestMod::GetNPCEquipItemTemplateIDForItemTemplate(uint32 itemTemplateID)
{
    if (ItemTemplatesByEntryID.find(itemTemplateID) == ItemTemplatesByEntryID.end())
        return itemTemplateID;
    else
        return ItemTemplatesByEntryID[itemTemplateID].ItemTemplateEntryIDForNPCEquip;
}

uint32 EverQuestMod::GetWornEffectSpellIDForItemTemplate(uint32 itemTemplateID)
{
    if (ItemTemplatesByEntryID.find(itemTemplateID) == ItemTemplatesByEntryID.end())
        return 0;
    else
        return ItemTemplatesByEntryID[itemTemplateID].WornEffectSpellID;
}

bool EverQuestMod::IsItemEQClassAllowedForPlayer(Player* player, uint32 itemTemplateID)
{
    // No EQ template data = allowed
    auto itemTemplateItr = ItemTemplatesByEntryID.find(itemTemplateID);
    if (itemTemplateItr == ItemTemplatesByEntryID.end())
        return true;

    // Zero mask = all
    uint32 allowedEQClassMask = itemTemplateItr->second.AllowedEQClassMask;
    if (allowedEQClassMask == 0)
        return true;

    // Compare base class
    const EverQuestClassMap classMap = GetClassMapForWOWClassID(player->getClass());
    uint32 baseEQClassBit = 1u << (classMap.EQClassIDBase - 1);
    if ((allowedEQClassMask & baseEQClassBit) != 0)
        return true;

    // Second class
    uint8 secondEQClass = GetCurrentSecondEQClassForPlayer(player);
    if (secondEQClass == EQ_EQCLASS_NONE)
        return false;
    uint32 secondEQClassBit = 1u << (secondEQClass - 1);
    if ((allowedEQClassMask & secondEQClassBit) != 0)
        return true;

    return false;
}

void EverQuestMod::LoadSpellData()
{
    SpellDataBySpellID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT SpellID, AuraDurationBaseInMS, AuraDurationAddPerLevelInMS, AuraDurationMaxInMS, AuraDurationCalcMinLevel, AuraDurationCalcMaxLevel, RecourseSpellID, SpellIDCastOnMeleeAttacker, FocusBoostType, PeriodicAuraSpellID, PeriodicAuraSpellRadius, MaleFormSpellID, FemaleFormSpellID, EffectFailChancePercent, EffectFailableType, StunUsesBashKickChance, SpellIDCastOnTargetWhenStunLands FROM mod_everquest_spell ORDER BY SpellID;");
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
            everQuestSpell.EffectFailChancePercent = fields[13].Get<uint32>();
            everQuestSpell.EffectFailableType = fields[14].Get<uint32>();
            everQuestSpell.StunUsesBashKickChance = fields[15].Get<bool>();
            everQuestSpell.SpellIDCastOnTargetWhenStunLands = fields[16].Get<uint32>();
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
    PlayerAutoLearnSkillsByEQClassID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT eqclass, skill FROM mod_everquest_playerautolearnskills;");
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            uint8 classID = fields[0].Get<uint8>();
            uint32 skillID = fields[1].Get<uint32>();
            PlayerAutoLearnSkillsByEQClassID[classID].push_back(skillID);
        } while (queryResult->NextRow());
    }
}

const list<uint32>& EverQuestMod::GetAutoLearnSkillsForClass(uint8 classID)
{
    if (PlayerAutoLearnSkillsByEQClassID.find(classID) != PlayerAutoLearnSkillsByEQClassID.end())
    {
        return PlayerAutoLearnSkillsByEQClassID[classID];
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
    QueryResult queryResult = WorldDatabase.Query("SELECT eqclass, race, spell FROM mod_everquest_playerautolearnspells;");
    if (queryResult)
    {
        do
        {
            EverQuestAutoLearnSpell autoLearnSpell;
            Field* fields = queryResult->Fetch();
            autoLearnSpell.EQClassID = fields[0].Get<uint8>();
            autoLearnSpell.RaceID = fields[1].Get<uint8>();
            autoLearnSpell.SpellID = fields[2].Get<uint32>();
            PlayerAutoLearnSpellsByClassID[autoLearnSpell.EQClassID].push_back(autoLearnSpell);
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

// Reference is EQMacEmu/TAKP Mob::TryBashKickStun
bool EverQuestMod::RollBashKickStunLands(Unit* attacker, Unit* defender)
{
    if (attacker == nullptr || defender == nullptr)
        return false;

    int attackerLevel = (int)attacker->GetLevel();
    int defenderLevel = (int)defender->GetLevel();

    // NPC defenders above the base immunity level can never be stunned by bash/kick (raid-tier mobs)
    // TODO: Make this a config in case the world gets scaled to 80
    if (defender->IsCreature() == true && defenderLevel > EQ_BASHKICKSTUN_NPC_IMMUNE_ABOVE_LEVEL)
        return false;

    // Base chance, lowered slightly once the attacker is above level 60 (matches TAKP)
    int stunChance = EQ_BASHKICKSTUN_BASE_CHANCE;
    if (attackerLevel > 60)
        stunChance = EQ_BASHKICKSTUN_BASE_CHANCE_ABOVE_LEVEL_60;

    // Scale by the level
    int levelDiff = attackerLevel - defenderLevel;
    if (levelDiff < 0)
        stunChance -= (levelDiff * levelDiff) / 2;
    else
        stunChance += (levelDiff * levelDiff) / 2;

    if (stunChance < EQ_BASHKICKSTUN_MIN_CHANCE)
        stunChance = EQ_BASHKICKSTUN_MIN_CHANCE;

    return ((int)urand(0, 99) < stunChance);
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

void EverQuestMod::LoadClassMapData()
{
    ClassMapByWOWClassID.clear();

    QueryResult queryResult = WorldDatabase.Query("SELECT wowclass, eqclass_base, eqclass_defaultsecond FROM mod_everquest_classmap;");
    if (queryResult)
    {
        do
        {
            Field* fields = queryResult->Fetch();
            EverQuestClassMap classMap;
            classMap.WOWClassID = fields[0].Get<uint8>();
            classMap.EQClassIDBase = fields[3].Get<uint8>();
            classMap.EQClassIDDefaultSecond = fields[2].Get<uint8>();
            ClassMapByWOWClassID[classMap.WOWClassID] = classMap;
        } while (queryResult->NextRow());
    }
}

const EverQuestClassMap& EverQuestMod::GetClassMapForWOWClassID(uint8 wowClassID)
{
    if (ClassMapByWOWClassID.find(wowClassID) != ClassMapByWOWClassID.end())
        return ClassMapByWOWClassID[wowClassID];
    else
    {
        static const EverQuestClassMap returnEmpty;
        LOG_ERROR("module.EverQuest", "EverQuestMod::GetClassMapForWOWClassID failure, wowClassID {} could not be found", wowClassID);
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
    transaction->Append("DELETE FROM `mod_everquest_character_lastgate` WHERE guid = {}", guid.GetCounter());

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

uint8 EverQuestMod::GetCurrentSecondEQClassForPlayer(Player* player)
{
    if (ActivePlayerClassControllerDataByGUID.find(player->GetGUID()) == ActivePlayerClassControllerDataByGUID.end())
        ActivePlayerClassControllerDataByGUID[player->GetGUID()] = GetPlayerControllerData(player);
    return ActivePlayerClassControllerDataByGUID[player->GetGUID()].CurrentClass;
}

uint8 EverQuestMod::GetNextSecondEQClassForPlayer(Player* player)
{
    if (ActivePlayerClassControllerDataByGUID.find(player->GetGUID()) == ActivePlayerClassControllerDataByGUID.end())
        ActivePlayerClassControllerDataByGUID[player->GetGUID()] = GetPlayerControllerData(player);
    return ActivePlayerClassControllerDataByGUID[player->GetGUID()].NextClass;
}

void EverQuestMod::SetNextSecondEQClassForPlayer(Player* player, uint8 nextEQClass)
{
    if (ActivePlayerClassControllerDataByGUID.find(player->GetGUID()) == ActivePlayerClassControllerDataByGUID.end())
        ActivePlayerClassControllerDataByGUID[player->GetGUID()] = GetPlayerControllerData(player);
    ActivePlayerClassControllerDataByGUID[player->GetGUID()].NextClass = nextEQClass;
}

EverQuestPlayerControllerData EverQuestMod::GetPlayerControllerData(Player* player)
{
    EverQuestPlayerControllerData controllerData;
    controllerData.GUID = player->GetGUID().GetCounter();
    QueryResult queryResult = CharacterDatabase.Query("SELECT nextClass, currentClass FROM mod_everquest_character_class_controller WHERE guid = {}", player->GetGUID().GetCounter());
    if (!queryResult || queryResult->GetRowCount() == 0)
    {
        // TODO: Wire this up from the EQWOW data
        switch (player->getClass())
        {
        case CLASS_WARRIOR: controllerData.NextClass = controllerData.CurrentClass = EQ_EQCLASS_WARRIOR; break;
        case CLASS_PALADIN: controllerData.NextClass = controllerData.CurrentClass = EQ_EQCLASS_PALADIN; break;
        case CLASS_HUNTER: controllerData.NextClass = controllerData.CurrentClass = EQ_EQCLASS_RANGER; break;
        case CLASS_ROGUE: controllerData.NextClass = controllerData.CurrentClass = EQ_EQCLASS_ROGUE; break;
        case CLASS_PRIEST: controllerData.NextClass = controllerData.CurrentClass = EQ_EQCLASS_CLERIC; break;
        case CLASS_DEATH_KNIGHT: controllerData.NextClass = controllerData.CurrentClass = EQ_EQCLASS_SHADOWKNIGHT; break;
        case CLASS_SHAMAN: controllerData.NextClass = controllerData.CurrentClass = EQ_EQCLASS_SHAMAN; break;
        case CLASS_MAGE: controllerData.NextClass = controllerData.CurrentClass = EQ_EQCLASS_MAGICIAN; break;
        case CLASS_WARLOCK: controllerData.NextClass = controllerData.CurrentClass = EQ_EQCLASS_NECROMANCER; break;
        case CLASS_DRUID: controllerData.NextClass = controllerData.CurrentClass = EQ_EQCLASS_DRUID; break;
        default: controllerData.NextClass = controllerData.CurrentClass = EQ_EQCLASS_WARRIOR; break;
        }
    }
    else
    {
        Field* fields = queryResult->Fetch();
        controllerData.NextClass = fields[0].Get<uint8>();
        controllerData.CurrentClass = fields[1].Get<uint8>();
    }
    return controllerData;
}

//void EverQuestMod::SetPlayerControllerData(EverQuestPlayerControllerData controllerData)
//{
//    CharacterDatabase.DirectExecute("REPLACE INTO `mod_everquest_character_class_controller` (`guid`, `nextClass`, `currentClass`) VALUES ({}, {}, {})",
//        controllerData.GUID,
//        controllerData.NextClass,
//        controllerData.CurrentClass);
//}

map<string, EverQuestPlayerClassInfoItem> EverQuestMod::GetPlayerClassInfoByClassNameForPlayer(Player* player)
{
    map<string, EverQuestPlayerClassInfoItem> playerClassInfoByClass;

    // Get levels for classes first, and populate the base list
    map<uint8, uint8> classLevelsByClass = GetClassLevelsByClassForPlayer(player);
    for (auto& curClassLevel : classLevelsByClass)
    {
        EverQuestPlayerClassInfoItem curClassInfo;
        curClassInfo.ClassID = curClassLevel.first;
        curClassInfo.ClassName = GetEQClassStringFromID(curClassInfo.ClassID);
        curClassInfo.Level = curClassLevel.second;
        playerClassInfoByClass.insert(pair<string, EverQuestPlayerClassInfoItem>(curClassInfo.ClassName, curClassInfo));
    }

    return playerClassInfoByClass;
}

map<uint8, uint8> EverQuestMod::GetClassLevelsByClassForPlayer(Player* player)
{
    // Pull the other class levels first
    map<uint8, uint8> levelsByClass;
    QueryResult classQueryResult = CharacterDatabase.Query("SELECT `eqclass`, `level` FROM mod_everquest_characters WHERE guid = {} AND eqclass <> {}", player->GetGUID().GetCounter(), GetCurrentSecondEQClassForPlayer(player));
    if (classQueryResult)
    {
        do
        {
            Field* fields = classQueryResult->Fetch();
            uint8 returnedClass = fields[0].Get<uint8>();
            uint8 returnedLevel = fields[1].Get<uint8>();
            levelsByClass.insert(pair<uint8, uint8>(returnedClass, returnedLevel));
        } while (classQueryResult->NextRow());

    }

    // Add this class level
    levelsByClass.insert(pair<uint8, uint8>(player->getClass(), player->GetLevel()));

    return levelsByClass;
}

bool EverQuestMod::DoesSavedClassDataExistForPlayer(Player* player, uint8 lookupClass)
{
    QueryResult queryResult = CharacterDatabase.Query("SELECT guid, eqclass FROM mod_everquest_characters WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), lookupClass);
    if (!queryResult || queryResult->GetRowCount() == 0)
        return false;
    return true;
}

void EverQuestMod::CopyCharacterDataIntoModCharacterTable(Player* player, CharacterDatabaseTransaction& transaction)
{
    uint8 curEQClass = GetCurrentSecondEQClassForPlayer(player);

    transaction->Append("DELETE FROM `mod_everquest_characters` WHERE guid = {} and eqclass = {}", player->GetGUID().GetCounter(), curEQClass);
    QueryResult queryResult = CharacterDatabase.Query("SELECT leveltime, rest_bonus, resettalents_cost, resettalents_time FROM characters WHERE guid = {}", player->GetGUID().GetCounter());
    if (!queryResult)
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod Error pulling character data for guid {}", player->GetGUID().GetCounter());
    }
    else
    {
        Field* fields = queryResult->Fetch();
        auto finiteAlways = [](float f) { return std::isfinite(f) ? f : 0.0f; };

        transaction->Append("INSERT IGNORE INTO mod_everquest_characters (guid, class, eqclass, `level`, xp, leveltime, rest_bonus, resettalents_cost, resettalents_time, health, power1, power2, power3, power4, power5, power6, power7, talentGroupsCount, activeTalentGroup) VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {})",
            player->GetGUID().GetCounter(),
            player->getClass(),
            curEQClass,
            player->GetLevel(),
            player->GetUInt32Value(PLAYER_XP),
            fields[0].Get<uint32>(),                // leveltime
            finiteAlways(fields[1].Get<float>()),   // rest_bonus
            fields[2].Get<uint32>(),                //resettalents_cost - m_resetTalentsCost,
            fields[3].Get<uint32>(),                //resettalents_time - uint32(m_resetTalentsTime),
            player->GetHealth(),
            player->GetPower(Powers(0)),
            player->GetPower(Powers(1)),
            player->GetPower(Powers(2)),
            player->GetPower(Powers(3)),
            player->GetPower(Powers(4)),
            player->GetPower(Powers(5)),
            player->GetPower(Powers(6)),
            player->GetSpecsCount(),
            player->GetActiveSpec()
        );
    }
}

void EverQuestMod::MoveTalentsToModTalentsTable(Player* player, CharacterDatabaseTransaction& transaction)
{
    uint8 curEQClass = GetCurrentSecondEQClassForPlayer(player);

    transaction->Append("DELETE FROM `mod_everquest_character_class_talent` WHERE guid = {} and eqclass = {}", player->GetGUID().GetCounter(), curEQClass);
    transaction->Append("INSERT IGNORE INTO mod_everquest_character_class_talent (guid, class, eqclass, spell, specMask) SELECT guid, {}, {}, spell, specMask FROM character_talent WHERE guid = {}", player->getClass(), curEQClass, player->GetGUID().GetCounter());
    transaction->Append("DELETE FROM `character_talent` WHERE guid = {}", player->GetGUID().GetCounter());
}

void EverQuestMod::MoveClassSpellsToModSpellsTable(Player* player, CharacterDatabaseTransaction& transaction)
{
    uint8 curEQClass = GetCurrentSecondEQClassForPlayer(player);

    // Purge old spell list in mod table
    transaction->Append("DELETE FROM `mod_everquest_character_class_spell` WHERE guid = {} and eqclass = {}", player->GetGUID().GetCounter(), curEQClass);

    // Move class spells from the character table into the mod table
    for (auto& curSpell : player->GetSpellMap())
    {
        // Skip non-class spells
        // TODO: Make this work
        //if (ClassSpellIDs.find(curSpell.first) == ClassSpellIDs.end())
        //    continue;

        // Skip deleting spells
        if (curSpell.second->State == PLAYERSPELL_REMOVED)
            continue;

        // INSERT IGNORE INTO Mod
        transaction->Append("INSERT IGNORE INTO mod_everquest_character_class_spell (guid, class, eqclass, spell, specMask) VALUES ({}, {}, {}, {}, {})",
            player->GetGUID().GetCounter(),
            player->getClass(),
            curEQClass,
            curSpell.first,
            (uint32)(curSpell.second->specMask));

        // Delete from character
        transaction->Append("DELETE FROM character_spell WHERE guid = {} and spell = {}",
            player->GetGUID().GetCounter(),
            curSpell.first);
    }
}

void EverQuestMod::MoveClassSkillsToModSkillsTable(Player* player, CharacterDatabaseTransaction& transaction)
{
    uint8 curEQClass = GetCurrentSecondEQClassForPlayer(player);

    // Purge old skill list in mod table
    transaction->Append("DELETE FROM `mod_everquest_character_class_skills` WHERE guid = {} and eqclass = {}", player->GetGUID().GetCounter(), curEQClass);

    // Get all of the known player skills
    // TODO: This REALLY needs to be done somehow better
    set<uint32> playerKnownSkills;
    for (uint32 i = 0; i < ConfigMaxSkillIDCheck; ++i)
    {
        if (player->HasSkill(i))
            playerKnownSkills.insert(i);
    }

    // Go through all known skills on this player to move them
    for (uint32 curSkillID : playerKnownSkills)
    {
        // Ignore shared skills
        if (ConfigCrossClassIncludeSkillIDs.find(curSkillID) != ConfigCrossClassIncludeSkillIDs.end())
        {
            continue;
        }

        // Add to the mod table
        transaction->Append("INSERT IGNORE INTO mod_everquest_character_class_skills (guid, class, eqclass, skill, value, max) VALUES ({}, {}, {}, {}, {}, {})",
            player->GetGUID().GetCounter(),
            player->getClass(),
            curEQClass,
            curSkillID,
            player->GetPureSkillValue(curSkillID),
            player->GetPureMaxSkillValue(curSkillID));

        // Remove from the character skill table
        transaction->Append("DELETE FROM character_skills WHERE guid = {} AND skill = {}",
            player->GetGUID().GetCounter(),
            curSkillID);
    }
}

void EverQuestMod::ReplaceModClassActionCopy(Player* player, CharacterDatabaseTransaction& transaction)
{
    uint8 curEQClass = GetCurrentSecondEQClassForPlayer(player);

    // Delete the old action entries
    transaction->Append("DELETE FROM `mod_everquest_character_class_action` WHERE guid = {} and eqclass = {}", player->GetGUID().GetCounter(), curEQClass);

    // Less ideal approach, as it causes a table scan on character_action
    transaction->Append("INSERT IGNORE INTO mod_everquest_character_class_action (guid, class, eqclass, spec, button, `action`, `type`) SELECT guid, {}, {}, spec, button, `action`, `type` FROM character_action WHERE guid = {}", player->getClass(), curEQClass, player->GetGUID().GetCounter());
}

void EverQuestMod::MoveGlyphsToModGlyhpsTable(Player* player, CharacterDatabaseTransaction& transaction)
{
    uint8 curEQClass = GetCurrentSecondEQClassForPlayer(player);

    transaction->Append("DELETE FROM `mod_everquest_character_class_glyphs` WHERE guid = {} and eqclass = {}", player->GetGUID().GetCounter(), curEQClass);
    transaction->Append("INSERT IGNORE INTO mod_everquest_character_class_glyphs (guid, class, eqclass, talentGroup, glyph1, glyph2, glyph3, glyph4, glyph5, glyph6) SELECT guid, {}, {}, talentGroup, glyph1, glyph2, glyph3, glyph4, glyph5, glyph6 FROM character_glyphs WHERE guid = {}", player->getClass(), curEQClass, player->GetGUID().GetCounter());
    transaction->Append("DELETE FROM `character_glyphs` WHERE guid = {}", player->GetGUID().GetCounter());
}

void EverQuestMod::MoveAuraToModAuraTable(Player* player, CharacterDatabaseTransaction& transaction)
{
    uint8 curEQClass = GetCurrentSecondEQClassForPlayer(player);

    // TODO: Do something about gate

    transaction->Append("DELETE FROM `mod_everquest_character_class_aura` WHERE guid = {} and eqclass = {}", player->GetGUID().GetCounter(), curEQClass);
    transaction->Append("INSERT IGNORE INTO mod_everquest_character_class_aura (guid, class, eqclass, casterGuid, itemGuid, spell, effectMask, recalculateMask, stackCount, amount0, amount1, amount2, base_amount0, base_amount1, base_amount2, maxDuration, remainTime, remainCharges) SELECT guid, {}, {}, casterGuid, itemGuid, spell, effectMask, recalculateMask, stackCount, amount0, amount1, amount2, base_amount0, base_amount1, base_amount2, maxDuration, remainTime, remainCharges FROM character_aura WHERE guid = {}", player->getClass(), curEQClass, player->GetGUID().GetCounter());
    transaction->Append("DELETE FROM `character_aura` WHERE guid = {}", player->GetGUID().GetCounter());
}

void EverQuestMod::MoveEquipToModInventoryTable(Player* player, CharacterDatabaseTransaction& transaction)
{
    uint8 curEQClass = GetCurrentSecondEQClassForPlayer(player);

    transaction->Append("DELETE FROM `mod_everquest_character_class_inventory` WHERE guid = {} AND eqclass = {} AND `bag` = 0 AND `slot` <= 18;", player->GetGUID().GetCounter(), curEQClass);
    transaction->Append("INSERT IGNORE INTO `mod_everquest_character_class_inventory` (`guid`, `class`, `eqclass`, `bag`, `slot`, `item`) SELECT `guid`, {}, {}, `bag`, `slot`, `item` FROM character_inventory WHERE guid = {} AND `bag` = 0 AND `slot` <= 18", player->getClass(), curEQClass, player->GetGUID().GetCounter());
    transaction->Append("DELETE FROM `character_inventory` WHERE guid = {} AND `bag` = 0 AND `slot` <= 18", player->GetGUID().GetCounter());
}

void EverQuestMod::UpdateCharacterFromModCharacterTable(Player* player, uint8 pullEQClassID, CharacterDatabaseTransaction& transaction)
{
    QueryResult queryResult = CharacterDatabase.Query("SELECT `level`, `xp`, `leveltime`, `rest_bonus`, `resettalents_cost`, `resettalents_time`, `health`, `power1`, `power2`, `power3`, `power4`, `power5`, `power6`, `power7`, `talentGroupsCount`, `activeTalentGroup` FROM mod_everquest_characters WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), pullEQClassID);
    if (!queryResult)
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod Error pulling character data for guid {} eqclass {}", player->GetGUID().GetCounter(), pullEQClassID);
    }
    else
    {
        Field* fields = queryResult->Fetch();
        auto finiteAlways = [](float f) { return std::isfinite(f) ? f : 0.0f; };

        transaction->Append("UPDATE characters SET `level` = {}, `xp` = {}, `leveltime` = {}, `rest_bonus` = {}, `resettalents_cost` = {}, `resettalents_time` = {}, `health` = {}, `power1` = {}, `power2` = {}, `power3` = {}, `power4` = {}, `power5` = {}, `power6` = {}, `power7` = {}, `talentGroupsCount` = {}, `activeTalentGroup` = {} WHERE `guid` = {}",
            fields[0].Get<uint8>(),                 // level
            fields[1].Get<uint32>(),                // xp
            fields[2].Get<uint32>(),                // leveltime
            finiteAlways(fields[3].Get<float>()),   // rest_bonus
            fields[4].Get<uint32>(),                // resettalents_cost
            fields[5].Get<uint32>(),                // resettalents_time
            fields[6].Get<uint32>(),                // health
            fields[7].Get<uint32>(),                // power1
            fields[8].Get<uint32>(),                // power2
            fields[9].Get<uint32>(),                // power3
            fields[10].Get<uint32>(),               // power4
            fields[11].Get<uint32>(),               // power5
            fields[12].Get<uint32>(),               // power6
            fields[13].Get<uint32>(),               // power7
            fields[14].Get<uint8>(),                // talentGroupsCount
            fields[15].Get<uint8>(),                // activeTalentGroup
            player->GetGUID().GetCounter()
        );
    }
}

void EverQuestMod::CopyModSpellTableIntoCharacterSpells(Player* player, uint8 pullEQClassID, CharacterDatabaseTransaction& transaction)
{
    // Create inserts for all of the coming class spells
    QueryResult queryResult = CharacterDatabase.Query("SELECT spell, specMask FROM mod_everquest_character_class_spell WHERE guid = {} and eqclass = {}", player->GetGUID().GetCounter(), (uint32)pullEQClassID);
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            uint32 spellID = fields[0].Get<uint32>();
            uint8 specMask = fields[1].Get<uint8>();

            // Skip if not valid
            // TODO:?
            //if (ClassSpellIDs.find(spellID) == ClassSpellIDs.end())
            //    continue;

            // Add it
            transaction->Append("INSERT IGNORE INTO character_spell (guid, spell, specMask) VALUES ({}, {}, {})",
                player->GetGUID().GetCounter(),
                spellID,
                (uint32)specMask);
        } while (queryResult->NextRow());
    }
}

void EverQuestMod::CopyModActionTableIntoCharacterAction(Player* player, uint8 pullEQClassID, CharacterDatabaseTransaction& transaction)
{
    // Delete the old data
    transaction->Append("DELETE FROM `character_action` WHERE guid = {}", player->GetGUID().GetCounter());

    // Create inserts for all of the coming class action bar buttons
    QueryResult queryResult = CharacterDatabase.Query("SELECT spec, button, `action`, `type` FROM mod_everquest_character_class_action WHERE guid = {} and eqclass = {}", player->GetGUID().GetCounter(), (uint32)pullEQClassID);
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            uint8 actionSpec = fields[0].Get<uint8>();
            uint8 actionButton = fields[1].Get<uint8>();
            uint32 actionAction = fields[2].Get<uint32>();
            uint8 actionType = fields[3].Get<uint8>();

            transaction->Append("INSERT IGNORE INTO `character_action` (`guid`, `spec`, `button`, `action`, `type`) VALUES ({}, {}, {}, {}, {})",
                player->GetGUID().GetCounter(),
                (uint32)actionSpec,
                (uint32)actionButton,
                actionAction,
                (uint32)actionType);

        } while (queryResult->NextRow());
    }
}

void EverQuestMod::CopyModSkillTableIntoCharacterSkills(Player* player, uint8 pullEQClassID, CharacterDatabaseTransaction& transaction)
{
    // Create inserts for all of the coming class skills
    QueryResult queryResult = CharacterDatabase.Query("SELECT skill, value, max FROM mod_everquest_character_class_skills WHERE guid = {} and eqclass = {}", player->GetGUID().GetCounter(), (uint32)pullEQClassID);
    if (!queryResult)
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod Error pulling class skill data from the mod table for eqclass {} on guid {}, so the class will have no non-shared skills...", (uint32)pullEQClassID, player->GetGUID().GetCounter());
    }
    else
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            uint32 skillID = fields[0].Get<uint32>();
            uint32 value = fields[1].Get<uint32>();
            uint32 max = fields[2].Get<uint32>();

            // Insert it
            transaction->Append("INSERT IGNORE INTO `character_skills` (`guid`, `skill`, `value`, `max`) VALUES ({}, {}, {}, {})",
                player->GetGUID().GetCounter(),
                skillID,
                value,
                max);

        } while (queryResult->NextRow());
    }
}

void EverQuestMod::UpdatePlayerControllerForClassChange(Player* player, uint8 newEQClassID, CharacterDatabaseTransaction& transaction)
{
    transaction->Append("REPLACE INTO `mod_everquest_character_class_controller` (`guid`, `nextClass`, `currentClass`) VALUES ({}, {}, {})",
        player->GetGUID().GetCounter(),
        newEQClassID,
        newEQClassID); // Overwriting current with next
}

map<uint8, EverQuestPlayerEquipedItemData> EverQuestMod::GetVisibleItemsBySlotForPlayerClass(Player* player, uint8 eqClassID)
{
    // Start with a list of blank inventory display slots
    map<uint8, EverQuestPlayerEquipedItemData> visibleItems;
    for (uint8 i = 0; i < 18; ++i)
    {
        EverQuestPlayerEquipedItemData curItem;
        curItem.ItemID = 0;
        curItem.PermEnchant = 0;
        curItem.Slot = i;
        curItem.TempEnchant = 0;
        curItem.ItemInstanceGUID = 0;
        visibleItems.insert(pair<uint8, EverQuestPlayerEquipedItemData>(i, curItem));
    }

    // If current class, grab those items
    if (GetCurrentSecondEQClassForPlayer(player) == eqClassID)
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod Getting visible item list for current player is unimplemented");
    }
    // Otherwise, retrieve from the database
    else
    {
        QueryResult queryResult = CharacterDatabase.Query("SELECT CI.`slot`, II.`itemEntry`, II.`enchantments`, II.`guid` FROM `mod_everquest_character_class_inventory` CI INNER JOIN `item_instance` II on II.guid = CI.item WHERE CI.`bag` = 0 AND CI.`slot` <= 18 AND CI.`guid` = {} AND `eqclass` = {}", player->GetGUID().GetCounter(), eqClassID);
        if (queryResult && queryResult->GetRowCount() > 0)
        {
            do
            {
                Field* fields = queryResult->Fetch();
                uint8 slot = fields[0].Get<uint8>();
                uint32 itemID = fields[1].Get<uint32>();
                string enchantString = fields[2].Get<string>();
                uint32 itemInstanceGUID = fields[3].Get<uint32>();

                // Break out enchant values
                std::vector<std::string_view> tokens = Acore::Tokenize(enchantString, ' ', false);
                uint32 permEnchant = *Acore::StringTo<uint32>(tokens[PERM_ENCHANTMENT_SLOT * MAX_ENCHANTMENT_OFFSET]);
                uint32 tempEnchant = *Acore::StringTo<uint32>(tokens[TEMP_ENCHANTMENT_SLOT * MAX_ENCHANTMENT_OFFSET]);

                // Store
                visibleItems[slot].Slot = slot;
                visibleItems[slot].ItemID = itemID;
                visibleItems[slot].PermEnchant = permEnchant;
                visibleItems[slot].TempEnchant = tempEnchant;
                visibleItems[slot].ItemInstanceGUID = itemInstanceGUID;
            } while (queryResult->NextRow());
        }
    }

    // If we're using the transmog mod, factor for that by pulling those visuals too (TODO: this is always true right now)
    //if (ConfigUsingTransmogMod)
    //{
        QueryResult queryResult = CharacterDatabase.Query("SELECT `GUID`, `FakeEntry` FROM custom_transmogrification WHERE `Owner` = {}", player->GetGUID().GetCounter());
        if (queryResult && queryResult->GetRowCount() > 0)
        {
            do
            {
                Field* fields = queryResult->Fetch();
                uint32 itemInstanceGUID = fields[0].Get<uint32>();
                uint32 fakeItemID = fields[1].Get<uint32>();

                // Replace any matches
                for (auto& visibleItem : visibleItems)
                {
                    if (visibleItem.second.ItemInstanceGUID == itemInstanceGUID)
                        visibleItem.second.ItemID = fakeItemID;
                }
            } while (queryResult->NextRow());
        }
    //}

    return visibleItems;
}

bool EverQuestMod::PerformClassSwitch(Player* player)
{
    uint8 nextEQClass = GetNextSecondEQClassForPlayer(player);
    bool isNew = !DoesSavedClassDataExistForPlayer(player, nextEQClass);

    // Set up the transaction
    CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();

    // Perform moves into the mod tables to reflect this character's class
    CopyCharacterDataIntoModCharacterTable(player, transaction);
    MoveTalentsToModTalentsTable(player, transaction);
    MoveClassSpellsToModSpellsTable(player, transaction);
    MoveClassSkillsToModSkillsTable(player, transaction);
    ReplaceModClassActionCopy(player, transaction);
    MoveGlyphsToModGlyhpsTable(player, transaction);
    MoveAuraToModAuraTable(player, transaction);
    MoveEquipToModInventoryTable(player, transaction);

    // Update pet references
    transaction->Append("UPDATE character_pet SET multi_class_owner = {}, eq_eqclass = {} WHERE owner = {}", player->GetGUID().GetCounter(), GetCurrentSecondEQClassForPlayer(player), player->GetGUID().GetCounter());
    transaction->Append("UPDATE character_pet SET owner = 0 WHERE multi_class_owner = {} AND eq_eqclass = {}", player->GetGUID().GetCounter(), GetCurrentSecondEQClassForPlayer(player));
    transaction->Append("UPDATE character_pet SET owner = {} WHERE multi_class_owner = {} AND eq_eqclass = {}", player->GetGUID().GetCounter(), player->GetGUID().GetCounter(), nextEQClass);

    // New
    if (isNew)
    {
        // For start level
        uint32 startLevel = nextEQClass != CLASS_DEATH_KNIGHT
            ? sWorld->getIntConfig(CONFIG_START_PLAYER_LEVEL)
            : sWorld->getIntConfig(CONFIG_START_HEROIC_PLAYER_LEVEL);

        // For health and mana
        PlayerClassLevelInfo classInfo;
        sObjectMgr->GetPlayerClassLevelInfo(player->getClass(), startLevel, &classInfo);

        // Update the character core table to reflect the switch
        transaction->Append("UPDATE characters SET `level` = {}, `xp` = 0, `leveltime` = 0, `rest_bonus` = 0, `resettalents_cost` = 0, `resettalents_time` = 0, health = {}, power1 = {}, power2 = 0, power3 = 0, power4 = 100, power5 = 0, power6 = 0, power7 = 0, `talentGroupsCount` = 1, `activeTalentGroup` = 0 WHERE guid = {}", startLevel, classInfo.basehealth, classInfo.basemana, player->GetGUID().GetCounter());

        // Give blank action mappings
        transaction->Append("DELETE FROM `character_action` WHERE guid = {}", player->GetGUID().GetCounter());
    }
    // Existing
    else
    {
        // Copy in the stored version for existing
        UpdateCharacterFromModCharacterTable(player, nextEQClass, transaction);
        CopyModSpellTableIntoCharacterSpells(player, nextEQClass, transaction);
        CopyModActionTableIntoCharacterAction(player, nextEQClass, transaction);
        CopyModSkillTableIntoCharacterSkills(player, nextEQClass, transaction);

        transaction->Append("INSERT IGNORE INTO character_talent (guid, spell, specMask) SELECT guid, spell, specMask FROM mod_everquest_character_class_talent WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), nextEQClass);
        transaction->Append("INSERT IGNORE INTO character_glyphs (guid, talentGroup, glyph1, glyph2, glyph3, glyph4, glyph5, glyph6) SELECT guid, talentGroup, glyph1, glyph2, glyph3, glyph4, glyph5, glyph6 FROM mod_everquest_character_class_glyphs WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), nextEQClass);
        transaction->Append("INSERT IGNORE INTO character_aura (guid, casterGuid, itemGuid, spell, effectMask, recalculateMask, stackCount, amount0, amount1, amount2, base_amount0, base_amount1, base_amount2, maxDuration, remainTime, remainCharges) SELECT guid, casterGuid, itemGuid, spell, effectMask, recalculateMask, stackCount, amount0, amount1, amount2, base_amount0, base_amount1, base_amount2, maxDuration, remainTime, remainCharges FROM mod_everquest_character_class_aura WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), nextEQClass);
        transaction->Append("INSERT IGNORE INTO `character_inventory` (`guid`, `bag`, `slot`, `item`) SELECT `guid`, `bag`, `slot`, `item` FROM mod_everquest_character_class_inventory WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), nextEQClass);
    }

    // Update current class
    UpdatePlayerControllerForClassChange(player, nextEQClass, transaction);
    ActivePlayerClassControllerDataByGUID[player->GetGUID()].CurrentClass = nextEQClass;

    // Commit the transaction
    CharacterDatabase.CommitTransaction(transaction);

    return true;
}

bool EverQuestMod::PerformPlayerDelete(ObjectGuid guid)
{
    // Delete every mod table record with this player guid
    uint32 playerGUID = guid.GetCounter();

    CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
    transaction->Append("DELETE FROM mod_everquest_characters WHERE guid = {}", playerGUID);
    transaction->Append("DELETE FROM mod_everquest_character_class_talent WHERE guid = {}", playerGUID);
    transaction->Append("DELETE FROM mod_everquest_character_class_aura WHERE guid = {}", playerGUID);
    transaction->Append("DELETE FROM mod_everquest_character_class_spell WHERE guid = {}", playerGUID);
    transaction->Append("DELETE FROM mod_everquest_character_class_skills WHERE guid = {}", playerGUID);
    transaction->Append("DELETE FROM mod_everquest_character_class_action WHERE guid = {}", playerGUID);
    transaction->Append("DELETE FROM mod_everquest_character_class_glyphs WHERE guid = {}", playerGUID);
    transaction->Append("DELETE FROM mod_everquest_character_class_inventory WHERE guid = {}", playerGUID);
    transaction->Append("DELETE FROM mod_everquest_character_class_controller WHERE guid = {}", playerGUID);
    transaction->Append("DELETE FROM character_pet WHERE owner = 0 AND eq_owner = {}", playerGUID);
    CharacterDatabase.CommitTransaction(transaction);
    if (ActivePlayerClassControllerDataByGUID.find(guid) != ActivePlayerClassControllerDataByGUID.end())
        ActivePlayerClassControllerDataByGUID.erase(guid);
    return true;
}

std::string GetEQClassStringFromID(uint8 classID)
{
    switch (classID)
    {
    case EQ_EQCLASS_WARRIOR:        return "Warrior (WAR)";
    case EQ_EQCLASS_CLERIC:         return "Cleric (CLR)";
    case EQ_EQCLASS_PALADIN:        return "Paladin (PAL)";
    case EQ_EQCLASS_RANGER:         return "Ranger (RNG)";
    case EQ_EQCLASS_SHADOWKNIGHT:   return "Shadow Knight (SHD)";
    case EQ_EQCLASS_DRUID:          return "Druid (DRU)";
    case EQ_EQCLASS_MONK:           return "Monk (MNK)";
    case EQ_EQCLASS_BARD:           return "Bard (BRD)";
    case EQ_EQCLASS_ROGUE:          return "Rogue (ROG)";
    case EQ_EQCLASS_SHAMAN:         return "Shaman (SHM)";
    case EQ_EQCLASS_NECROMANCER:    return "Necromancer (NEC)";
    case EQ_EQCLASS_WIZARD:         return "Wizard (WIZ)";
    case EQ_EQCLASS_MAGICIAN:       return "Magician (MAG)";
    case EQ_EQCLASS_ENCHANTER:      return "Enchanter (ENC)";
    default:                        return "None";
    }
}

set<uint32> GetSetFromConfigString(string configStringName)
{
    string configString = sConfigMgr->GetOption<std::string>(configStringName, "");

    std::string delimitedValue;
    std::stringstream delimetedValueStream;
    std::set<uint32> generatedSet;

    delimetedValueStream.str(configString);
    while (std::getline(delimetedValueStream, delimitedValue, ','))
    {
        std::string curValue;
        std::stringstream delimetedPairStream(delimitedValue);
        delimetedPairStream >> curValue;
        auto itemId = atoi(curValue.c_str());
        if (generatedSet.find(itemId) != generatedSet.end())
        {
            LOG_ERROR("module.EverQuest", "EverQuestMod Duplicate value found in config string named {}", configString);
        }
        else
        {
            generatedSet.insert(itemId);
        }
    }

    return generatedSet;
}
