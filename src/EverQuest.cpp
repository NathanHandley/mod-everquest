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
#include "Group.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "CreatureData.h"
#include "DBCStores.h"
#include "SpellAuraDefines.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "Tokenize.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "GossipDef.h"
#include "ScriptedGossip.h"
#include "TemporarySummon.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"

#include "EverQuest.h"

#include <algorithm>
#include <cmath>
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
    ConfigSystemLegacyAchievementID(0),
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
    ConfigSpellBuffLevelRestrictionsEnabled(true),
    ConfigSpellCrowdControlLevelRestrictionsEnabled(true),
    ConfigSpellHasteCapEnabled(true),
    ConfigSpellHasteCapPercent(100.0f),
    ConfigCombatSkillsDisableBashKickStunOnPlayers(false),
    ConfigEvadeEnabled(true),
    ConfigEvadeUnreachableSeconds(10.0f),
    ConfigEvadeUnstickStallSeconds(3.0f),
    ConfigEvadeUnstickSettleSeconds(1.0f),
    ConfigEvadeUnstickMoveThreshold(3.0f),
    ConfigEvadeUnstickMaxAttempts(3),
    ConfigEvadeUnstickStepPercent(25),
    ConfigCharmCreatureCharmLimitsEnabled(true),
    ConfigCharmUncharmedPlayerCheckRadius(100.0f),
    ConfigCreatureEmotesEnabled(true),
    ConfigCreatureEmotesAmbientEnabled(true),
    ConfigIllusionGearRefreshTimeInMS(1000),
    ConfigShowClassMessageOnLogin(true),
    ConfigSecondaryExpPoolGainPercent(25.0f),
    ConfigSecondaryExpPoolMaxPooled(1000000),
    ConfigPlayerLevelCap(0),
    CrossClassExemptSpellIDsBuilt(false),
    IllusionMaxFaceIndex(0)
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
            else if (key == "LegacyAchievementID")
                ConfigSystemLegacyAchievementID = (uint32)atoi(value.c_str());
            else if (key == "LegacyAchievementAccountCreatedBefore")
                ConfigSystemLegacyAchievementAccountCreatedBefore = value;
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
            else if (key == "RangedAttackSpellID")
                ConfigSystemRangedAttackSpellID = (uint32)atoi(value.c_str());
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
    ConfigSpellBuffLevelRestrictionsEnabled = sConfigMgr->GetOption<bool>("EverQuest.Spells.BuffLevelRestrictionsEnabled", true);
    ConfigSpellCrowdControlLevelRestrictionsEnabled = sConfigMgr->GetOption<bool>("EverQuest.Spells.CrowdControlLevelRestrictionsEnabled", true);
    ConfigSpellHasteCapEnabled = sConfigMgr->GetOption<bool>("EverQuest.Spells.HasteCapEnabled", true);
    ConfigSpellHasteCapPercent = sConfigMgr->GetOption<float>("EverQuest.Spells.HasteCapPercent", 100.0f);

    // Combat Skills
    ConfigCombatSkillsDisableBashKickStunOnPlayers = sConfigMgr->GetOption<bool>("EverQuest.CombatSkills.DisableBashKickStunOnPlayers", false);
    ConfigCombatSkillsRangedAttackEnabled = sConfigMgr->GetOption<bool>("EverQuest.CombatSkills.RangedAttackEnabled", true);
    ConfigCombatSkillsRangedAttackDefaultMinRange = sConfigMgr->GetOption<float>("EverQuest.CombatSkills.RangedAttackDefaultMinRange", 25.0f);
    ConfigCombatSkillsRangedAttackDefaultMaxRange = sConfigMgr->GetOption<float>("EverQuest.CombatSkills.RangedAttackDefaultMaxRange", 250.0f);
    ConfigCombatSkillsRangedAttackDamageMultiplier = sConfigMgr->GetOption<float>("EverQuest.CombatSkills.RangedAttackDamageMultiplier", 1.0f);

    // Evade / unstick (EverQuest maps only)
    ConfigEvadeEnabled = sConfigMgr->GetOption<bool>("EverQuest.Evade.Enabled", true);
    ConfigEvadeUnreachableSeconds = sConfigMgr->GetOption<float>("EverQuest.Evade.UnreachableEvadeSeconds", 10.0f);
    ConfigEvadeUnstickStallSeconds = sConfigMgr->GetOption<float>("EverQuest.Evade.UnstickStallSeconds", 3.0f);
    ConfigEvadeUnstickSettleSeconds = sConfigMgr->GetOption<float>("EverQuest.Evade.UnstickSettleSeconds", 1.0f);
    ConfigEvadeUnstickMoveThreshold = sConfigMgr->GetOption<float>("EverQuest.Evade.UnstickMoveThreshold", 3.0f);
    ConfigEvadeUnstickMaxAttempts = sConfigMgr->GetOption<uint32>("EverQuest.Evade.UnstickMaxAttempts", 3);
    ConfigEvadeUnstickStepPercent = sConfigMgr->GetOption<uint32>("EverQuest.Evade.UnstickStepPercent", 25);
    if (ConfigEvadeUnstickStepPercent < 1)
        ConfigEvadeUnstickStepPercent = 1;
    if (ConfigEvadeUnstickStepPercent > 100)
        ConfigEvadeUnstickStepPercent = 100;

    // Charm
    ConfigCharmCreatureCharmLimitsEnabled = sConfigMgr->GetOption<bool>("EverQuest.Charm.CreatureCharmLimitsEnabled", true);
    ConfigCharmUncharmedPlayerCheckRadius = sConfigMgr->GetOption<float>("EverQuest.Charm.UncharmedPlayerCheckRadius", 100.0f);

    // Creature emotes
    ConfigCreatureEmotesEnabled = sConfigMgr->GetOption<bool>("EverQuest.CreatureEmotes.Enabled", true);
    ConfigCreatureEmotesAmbientEnabled = sConfigMgr->GetOption<bool>("EverQuest.CreatureEmotes.AmbientEnabled", true);

    // Creature movement sounds
    ConfigCreatureMovementSoundsEnabled = sConfigMgr->GetOption<bool>("EverQuest.CreatureMovementSounds.Enabled", true);

    // Illusion
    ConfigIllusionGearRefreshTimeInMS = sConfigMgr->GetOption<uint32>("EverQuest.Illusion.GearRefreshTimeInMS", 1000);

    // Class
    ConfigShowClassMessageOnLogin = sConfigMgr->GetOption<bool>("EverQuest.ShowClassMessageOnLogin", true);

    // Secondary Experience Pool
    ConfigSecondaryExpPoolGainPercent = sConfigMgr->GetOption<float>("EverQuest.SecondaryExpPool.GainPercent", 25);
    ConfigSecondaryExpPoolMaxPooled = sConfigMgr->GetOption<uint32>("EverQuest.SecondaryExpPool.MaxPooled", 1000000);

    // Player Level Cap
    ConfigPlayerLevelCap = sConfigMgr->GetOption<uint32>("EverQuest.Player.LevelCap", 0);

    // Cross-Class values
    ConfigCrossClassIncludeSkillIDs = GetSetFromConfigString("EverQuest.CrossClass.IncludeSkillIDs");

    // The cross-class exempt spell cache derives from the skill list above
    CrossClassExemptSpellIDs.clear();
    CrossClassExemptSpellIDsBuilt = false;
}

void EverQuestMod::LoadCreatureData()
{
    CreaturesByTemplateID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT CreatureTemplateID, CanShowHeldLootItems, CanShowHeldLootShields, SpawnLimit, RangedAttackEnabled, RangedAttackMinRange, RangedAttackMaxRange, RangedAttackDamageModPct, AgroSocialDistanceMod FROM mod_everquest_creature ORDER BY CreatureTemplateID;");
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
            everQuestCreature.RangedAttackEnabled = fields[4].Get<bool>();
            everQuestCreature.RangedAttackMinRange = fields[5].Get<uint32>();
            everQuestCreature.RangedAttackMaxRange = fields[6].Get<uint32>();
            everQuestCreature.RangedAttackDamageModPct = fields[7].Get<int32>();
            everQuestCreature.AgroSocialDistanceMod = fields[8].Get<float>();
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
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
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

void EverQuestMod::LoadCreatureKillSpawnData()
{
    CreatureKillSpawnsByTriggerCreatureTemplateID.clear();

    QueryResult queryResult = WorldDatabase.Query("SELECT ID, TriggerCreatureTemplateID, MapID, ActionType, TargetCreatureTemplateID, Chance, AltGroup, AltID, AltWeight, SpawnAtCorpse, PositionX, PositionY, PositionZ, Orientation, DelayMinMS, DelayMaxMS, OnlyIfNotAliveCreatureTemplateID, RequireDeadCreatureTemplateIDs, RequireAliveCreatureTemplateIDs, AddToHateList, TriggerMinLevel, TriggerMaxLevel FROM mod_everquest_creature_kill_spawn;");
    if (queryResult)
    {
        do
        {
            Field* fields = queryResult->Fetch();
            EverQuestCreatureKillSpawn killSpawn;
            killSpawn.ID = fields[0].Get<uint32>();
            killSpawn.TriggerCreatureTemplateID = fields[1].Get<uint32>();
            killSpawn.MapID = fields[2].Get<uint32>();
            killSpawn.ActionType = fields[3].Get<uint8>();
            killSpawn.TargetCreatureTemplateID = fields[4].Get<uint32>();
            killSpawn.Chance = fields[5].Get<float>();
            killSpawn.AltGroup = fields[6].Get<uint32>();
            killSpawn.AltID = fields[7].Get<uint32>();
            killSpawn.AltWeight = fields[8].Get<float>();
            killSpawn.SpawnAtCorpse = fields[9].Get<uint8>() != 0;
            killSpawn.PositionX = fields[10].Get<float>();
            killSpawn.PositionY = fields[11].Get<float>();
            killSpawn.PositionZ = fields[12].Get<float>();
            killSpawn.Orientation = fields[13].Get<float>();
            killSpawn.DelayMinMS = fields[14].Get<uint32>();
            killSpawn.DelayMaxMS = fields[15].Get<uint32>();
            killSpawn.OnlyIfNotAliveCreatureTemplateID = fields[16].Get<uint32>();
            string requireDeadString = fields[17].Get<string>();
            for (std::string_view idToken : Acore::Tokenize(requireDeadString, ',', false))
                killSpawn.RequireDeadCreatureTemplateIDs.push_back(Acore::StringTo<uint32>(idToken).value_or(0));
            string requireAliveString = fields[18].Get<string>();
            for (std::string_view idToken : Acore::Tokenize(requireAliveString, ',', false))
                killSpawn.RequireAliveCreatureTemplateIDs.push_back(Acore::StringTo<uint32>(idToken).value_or(0));
            killSpawn.AddToHateList = fields[19].Get<uint8>() != 0;
            killSpawn.TriggerMinLevel = fields[20].Get<uint32>();
            killSpawn.TriggerMaxLevel = fields[21].Get<uint32>();
            CreatureKillSpawnsByTriggerCreatureTemplateID[killSpawn.TriggerCreatureTemplateID].push_back(killSpawn);
        } while (queryResult->NextRow());
    }
}

void EverQuestMod::LoadCreatureEmoteData()
{
    CreatureEmotesByCreatureTemplateID.clear();

    QueryResult queryResult = WorldDatabase.Query("SELECT CreatureTemplateID, EventType, EmoteType, ChancePct, Param1, Param2, EmoteText FROM mod_everquest_creature_emote ORDER BY CreatureTemplateID, ID;");
    if (queryResult)
    {
        do
        {
            Field* fields = queryResult->Fetch();
            uint32 creatureTemplateID = fields[0].Get<uint32>();
            EverQuestCreatureEmote emote;
            emote.EventType = fields[1].Get<uint8>();
            emote.EmoteType = fields[2].Get<uint8>();
            emote.ChancePct = fields[3].Get<float>();
            emote.Param1 = fields[4].Get<int32>();
            emote.Param2 = fields[5].Get<int32>();
            emote.EmoteText = fields[6].Get<string>();
            CreatureEmotesByCreatureTemplateID[creatureTemplateID].push_back(emote);
        } while (queryResult->NextRow());
    }
}

void EverQuestMod::LoadCreatureMovementSoundData()
{
    CreatureMovementSoundsByDisplayID.clear();

    QueryResult queryResult = WorldDatabase.Query("SELECT DisplayID, WalkSoundEntryIDs, WalkSoundDurationsMS, RunSoundEntryIDs, RunSoundDurationsMS, MaxHearingDistance FROM mod_everquest_creature_movement_sound;");
    if (queryResult)
    {
        do
        {
            Field* fields = queryResult->Fetch();
            uint32 displayID = fields[0].Get<uint32>();
            EverQuestCreatureMovementSound movementSound;
            string walkSoundEntryIDsString = fields[1].Get<string>();
            for (std::string_view idToken : Acore::Tokenize(walkSoundEntryIDsString, ';', false))
                movementSound.WalkPieceSoundEntryIDs.push_back(Acore::StringTo<uint32>(idToken).value_or(0));
            string walkSoundDurationsString = fields[2].Get<string>();
            for (std::string_view durationToken : Acore::Tokenize(walkSoundDurationsString, ';', false))
                movementSound.WalkPieceDurationsMS.push_back(Acore::StringTo<uint32>(durationToken).value_or(0));
            string runSoundEntryIDsString = fields[3].Get<string>();
            for (std::string_view idToken : Acore::Tokenize(runSoundEntryIDsString, ';', false))
                movementSound.RunPieceSoundEntryIDs.push_back(Acore::StringTo<uint32>(idToken).value_or(0));
            string runSoundDurationsString = fields[4].Get<string>();
            for (std::string_view durationToken : Acore::Tokenize(runSoundDurationsString, ';', false))
                movementSound.RunPieceDurationsMS.push_back(Acore::StringTo<uint32>(durationToken).value_or(0));
            movementSound.MaxHearingDistance = fields[5].Get<float>();
            if (movementSound.WalkPieceSoundEntryIDs.size() != movementSound.WalkPieceDurationsMS.size() ||
                movementSound.RunPieceSoundEntryIDs.size() != movementSound.RunPieceDurationsMS.size())
            {
                LOG_ERROR("module.EverQuest", "EverQuestMod::LoadCreatureMovementSoundData skipped display ID {} as the piece sound and duration list lengths do not match", displayID);
                continue;
            }
            CreatureMovementSoundsByDisplayID[displayID] = movementSound;
        } while (queryResult->NextRow());
    }
}

static void ReplaceAllInString(string& subject, const string& token, const string& replacement)
{
    size_t tokenPosition = subject.find(token);
    while (tokenPosition != string::npos)
    {
        subject.replace(tokenPosition, token.length(), replacement);
        tokenPosition = subject.find(token, tokenPosition + replacement.length());
    }
}

static string GetRaceNameForPlayerRaceID(uint8 raceID)
{
    switch (raceID)
    {
        case 1: return "Human";
        case 2: return "Orc";
        case 3: return "Dwarf";
        case 4: return "Night Elf";
        case 5: return "Undead";
        case 6: return "Tauren";
        case 7: return "Gnome";
        case 8: return "Troll";
        case 10: return "Blood Elf";
        case 11: return "Draenei";
        default: return "race";
    }
}

// Substitution tokens and no-target fallbacks do the same as TAKP's NPC::DoNPCEmote.
// The 'M' tokens describe the speaking creature, the others describe the target (nearly always a player)
// TODO: Consider preprocessing this to minimumize runtime performance cost
string EverQuestMod::FormatCreatureEmoteText(Creature* creature, Unit* target, const string& text)
{
    string formattedText = text;

    // Note: $MRP must replace before $MR, and $RP before $R, since the short tokens are prefixes of the long ones
    ReplaceAllInString(formattedText, "$MN", creature->GetName());
    ReplaceAllInString(formattedText, "$MRP", "creatures");
    ReplaceAllInString(formattedText, "$MR", "creature");
    ReplaceAllInString(formattedText, "$MC", "creature");
    if (target != nullptr && target->IsPlayer() == true)
    {
        Player* targetPlayer = target->ToPlayer();
        ReplaceAllInString(formattedText, "$N", targetPlayer->GetName());
        ReplaceAllInString(formattedText, "$RP", GetRaceNameForPlayerRaceID(targetPlayer->getRace()) + "s");
        ReplaceAllInString(formattedText, "$R", GetRaceNameForPlayerRaceID(targetPlayer->getRace()));
        // Drop the ' (WAR)' style abbreviation suffix from the class name since that looks funny
        string eqClassName = GetEQClassStringFromID(GetClassMapForWOWClassID(targetPlayer->getClass()).EQClassIDBase);
        size_t abbreviationPosition = eqClassName.find(" (");
        if (abbreviationPosition != string::npos)
            eqClassName = eqClassName.substr(0, abbreviationPosition);
        if (eqClassName.length() == 0)
            eqClassName = "class";
        ReplaceAllInString(formattedText, "$C", eqClassName);
    }
    else
    {
        ReplaceAllInString(formattedText, "$N", "foe");
        ReplaceAllInString(formattedText, "$RP", "races");
        ReplaceAllInString(formattedText, "$R", "race");
        ReplaceAllInString(formattedText, "$C", "class");
    }
    return formattedText;
}

void EverQuestMod::EmitCreatureEmote(Creature* creature, const EverQuestCreatureEmote& emote, Unit* target)
{
    string formattedText = FormatCreatureEmoteText(creature, target, emote.EmoteText);

    // Some emotes need the name added since in WoW they won't automatically append it
    bool isInvisibleTrigger = (creature->GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_TRIGGER) != 0;
    switch (emote.EmoteType)
    {
        case EQ_CREATURE_EMOTE_TYPE_EMOTE:
        {
            creature->TextEmote(creature->GetName() + " " + formattedText, target);
        } break;
        case EQ_CREATURE_EMOTE_TYPE_SHOUT:
        {
            if (isInvisibleTrigger == true)
                creature->TextEmote(creature->GetName() + " shouts, '" + formattedText + "'", target);
            else
                creature->Yell(formattedText, LANG_UNIVERSAL, target);
        } break;
        case EQ_CREATURE_EMOTE_TYPE_PROXIMITY:
        {
            // In EQ this was raw text (no speaker name) sent only to nearby players, so use a boss emote
            creature->TextEmote(formattedText, target, true);
        } break;
        default:
        {
            if (isInvisibleTrigger == true)
                creature->TextEmote(creature->GetName() + " says, '" + formattedText + "'", target);
            else
                creature->Say(formattedText, LANG_UNIVERSAL, target);
        } break;
    }
}

// This is like TAKP's NPC::GetNPCEmote
bool EverQuestMod::DoCreatureEmoteEvent(Creature* creature, uint8 emoteEventType, Unit* target)
{
    if (ConfigCreatureEmotesEnabled == false)
        return false;
    if (creature == nullptr)
        return false;
    unordered_map<uint32, vector<EverQuestCreatureEmote>>::const_iterator emoteIter = CreatureEmotesByCreatureTemplateID.find(creature->GetEntry());
    if (emoteIter == CreatureEmotesByCreatureTemplateID.end())
        return false;

    vector<const EverQuestCreatureEmote*> matchingEmotes;
    for (const EverQuestCreatureEmote& emote : emoteIter->second)
        if (emote.EventType == emoteEventType)
            matchingEmotes.push_back(&emote);
    if (matchingEmotes.empty() == true)
        return false;

    const EverQuestCreatureEmote* chosenEmote = matchingEmotes[urand(0, matchingEmotes.size() - 1)];
    if (chosenEmote->ChancePct < 100 && roll_chance_f(chosenEmote->ChancePct) == false)
        return false;
    EmitCreatureEmote(creature, *chosenEmote, target);
    return true;
}

// Only creatures with spawn, timer, or proximity emote lines get tick state
void EverQuestMod::SetupCreatureEmoteState(Creature* creature)
{
    if (ConfigCreatureEmotesEnabled == false)
        return;
    unordered_map<uint32, vector<EverQuestCreatureEmote>>::const_iterator emoteIter = CreatureEmotesByCreatureTemplateID.find(creature->GetEntry());
    if (emoteIter == CreatureEmotesByCreatureTemplateID.end())
        return;

    bool hasOnSpawnEmote = false;
    bool hasRandomTimerEmote = false;
    bool hasProximityEmote = false;
    uint32 randomTimerMinMS = 0;
    uint32 randomTimerMaxMS = 0;
    for (const EverQuestCreatureEmote& emote : emoteIter->second)
    {
        if (emote.EventType == EQ_CREATURE_EMOTE_EVENT_ONSPAWN)
            hasOnSpawnEmote = true;
        else if (emote.EventType == EQ_CREATURE_EMOTE_EVENT_RANDOMTIMER)
        {
            hasRandomTimerEmote = true;
            randomTimerMinMS = (uint32)emote.Param1;
            randomTimerMaxMS = (uint32)emote.Param2;
        }
        else if (emote.EventType == EQ_CREATURE_EMOTE_EVENT_PROXIMITY)
            hasProximityEmote = true;
    }
    if (hasOnSpawnEmote == false && hasRandomTimerEmote == false && hasProximityEmote == false)
        return;

    EverQuestCreatureEmoteState* state = creature->CustomData.GetDefault<EverQuestCreatureEmoteState>(EQ_CREATURE_CUSTOMDATA_EMOTE);
    state->WasAlive = creature->IsAlive();
    state->RandomTimerRemainingMS = 0;
    state->ProximityCheckRemainingMS = 0;
    state->ProximityCooldownRemainingMS = 0;
    if (hasRandomTimerEmote == true)
    {
        if (randomTimerMaxMS < randomTimerMinMS)
            randomTimerMaxMS = randomTimerMinMS;
        state->RandomTimerRemainingMS = urand(randomTimerMinMS, randomTimerMaxMS);
    }
    if (hasProximityEmote == true)
        state->ProximityCheckRemainingMS = EQ_CREATURE_EMOTE_PROXIMITY_CHECK_MS;
    if (state->WasAlive == true)
        DoCreatureEmoteEvent(creature, EQ_CREATURE_EMOTE_EVENT_ONSPAWN, nullptr);
}

void EverQuestMod::RemoveCreatureEmoteState(Creature* creature)
{
    creature->CustomData.Erase(EQ_CREATURE_CUSTOMDATA_EMOTE);
}

void EverQuestMod::UpdateCreatureEmotes(Creature* creature, uint32 diff)
{
    if (creature == nullptr)
        return;
    if (ConfigCreatureEmotesEnabled == false)
        return;
    EverQuestCreatureEmoteState* state = creature->CustomData.Get<EverQuestCreatureEmoteState>(EQ_CREATURE_CUSTOMDATA_EMOTE);
    if (state == nullptr)
        return;

    // Normal respawns and kill-spawn 'respawnself' never pass back through OnCreatureAddWorld, so catch the dead-to-alive transition here for the OnSpawn emote
    bool isAlive = creature->IsAlive();
    if (isAlive == true && state->WasAlive == false)
        DoCreatureEmoteEvent(creature, EQ_CREATURE_EMOTE_EVENT_ONSPAWN, nullptr);
    state->WasAlive = isAlive;
    if (isAlive == false)
        return;
    if (ConfigCreatureEmotesAmbientEnabled == false)
        return;
    if (creature->IsInCombat() == true)
        return;

    // Timed roamer utterances
    if (state->RandomTimerRemainingMS > 0)
    {
        if (state->RandomTimerRemainingMS > diff)
            state->RandomTimerRemainingMS -= diff;
        else
        {
            uint32 nextTimerMinMS = 0;
            uint32 nextTimerMaxMS = 0;
            unordered_map<uint32, vector<EverQuestCreatureEmote>>::const_iterator emoteIter = CreatureEmotesByCreatureTemplateID.find(creature->GetEntry());
            if (emoteIter != CreatureEmotesByCreatureTemplateID.end())
            {
                vector<const EverQuestCreatureEmote*> timerEmotes;
                for (const EverQuestCreatureEmote& emote : emoteIter->second)
                    if (emote.EventType == EQ_CREATURE_EMOTE_EVENT_RANDOMTIMER)
                        timerEmotes.push_back(&emote);
                if (timerEmotes.empty() == false)
                {
                    const EverQuestCreatureEmote* chosenEmote = timerEmotes[urand(0, timerEmotes.size() - 1)];
                    if (chosenEmote->ChancePct >= 100 || roll_chance_f(chosenEmote->ChancePct) == true)
                        EmitCreatureEmote(creature, *chosenEmote, nullptr);
                    nextTimerMinMS = (uint32)chosenEmote->Param1;
                    nextTimerMaxMS = (uint32)chosenEmote->Param2;
                }
            }
            if (nextTimerMaxMS < nextTimerMinMS)
                nextTimerMaxMS = nextTimerMinMS;
            if (nextTimerMinMS == 0 && nextTimerMaxMS == 0)
                return;
            state->RandomTimerRemainingMS = urand(nextTimerMinMS, nextTimerMaxMS);
        }
    }

    // Proximity speech (when you walk up to them)
    if (state->ProximityCheckRemainingMS > 0)
    {
        if (state->ProximityCooldownRemainingMS > 0)
        {
            if (state->ProximityCooldownRemainingMS > diff)
                state->ProximityCooldownRemainingMS -= diff;
            else
                state->ProximityCooldownRemainingMS = 0;
        }
        if (state->ProximityCheckRemainingMS > diff)
            state->ProximityCheckRemainingMS -= diff;
        else
        {
            state->ProximityCheckRemainingMS = EQ_CREATURE_EMOTE_PROXIMITY_CHECK_MS;
            if (state->ProximityCooldownRemainingMS == 0)
            {
                unordered_map<uint32, vector<EverQuestCreatureEmote>>::const_iterator emoteIter = CreatureEmotesByCreatureTemplateID.find(creature->GetEntry());
                if (emoteIter != CreatureEmotesByCreatureTemplateID.end())
                {
                    vector<const EverQuestCreatureEmote*> proximityEmotes;
                    for (const EverQuestCreatureEmote& emote : emoteIter->second)
                        if (emote.EventType == EQ_CREATURE_EMOTE_EVENT_PROXIMITY)
                            proximityEmotes.push_back(&emote);
                    if (proximityEmotes.empty() == false)
                    {
                        const EverQuestCreatureEmote* chosenEmote = proximityEmotes[urand(0, proximityEmotes.size() - 1)];
                        Player* nearbyPlayer = creature->SelectNearestPlayer((float)chosenEmote->Param1);
                        if (nearbyPlayer != nullptr && nearbyPlayer->IsAlive() == true && nearbyPlayer->IsGameMaster() == false)
                        {
                            if (chosenEmote->ChancePct >= 100 || roll_chance_f(chosenEmote->ChancePct) == true)
                                EmitCreatureEmote(creature, *chosenEmote, nearbyPlayer);
                            uint32 cooldownMS = (uint32)chosenEmote->Param2;
                            if (cooldownMS < EQ_CREATURE_EMOTE_PROXIMITY_MIN_COOLDOWN_MS)
                                cooldownMS = EQ_CREATURE_EMOTE_PROXIMITY_MIN_COOLDOWN_MS;
                            EverQuestCreatureEmoteState* stateAfterEmote = creature->CustomData.Get<EverQuestCreatureEmoteState>(EQ_CREATURE_CUSTOMDATA_EMOTE);
                            if (stateAfterEmote != nullptr)
                                stateAfterEmote->ProximityCooldownRemainingMS = cooldownMS;
                        }
                    }
                }
            }
        }
    }
}

void EverQuestMod::RemoveCreatureMovementSoundState(Creature* creature)
{
    creature->CustomData.Erase(EQ_CREATURE_CUSTOMDATA_MOVEMENTSOUND);
}

void EverQuestMod::UpdateCreatureMovementSound(Creature* creature, uint32 diff)
{
    // In EQ, movement sounds are repeating loops which isn't how WoW works.  So this is a 'hack' to make sounds
    // play in parts emitted from the server instead of relying on EQ creature event attachments
    if (creature == nullptr)
        return;
    if (ConfigCreatureMovementSoundsEnabled == false)
        return;

    // Walk vs Run sound picking
    uint8 curGait = EQ_CREATURE_MOVEMENT_GAIT_NONE;
    if (creature->IsAlive() == true && creature->isMoving() == true && creature->IsUnderWater() == false && creature->IsFlying() == false)
        curGait = creature->IsWalking() == true ? EQ_CREATURE_MOVEMENT_GAIT_WALK : EQ_CREATURE_MOVEMENT_GAIT_RUN;

    // Drop from idle creatures
    EverQuestCreatureMovementSoundState* state = creature->CustomData.Get<EverQuestCreatureMovementSoundState>(EQ_CREATURE_CUSTOMDATA_MOVEMENTSOUND);
    if (curGait == EQ_CREATURE_MOVEMENT_GAIT_NONE || creature->GetMap()->GetPlayers().IsEmpty() == true)
    {
        if (state != nullptr)
        {
            state->CurGait = EQ_CREATURE_MOVEMENT_GAIT_NONE;
            state->ListenersByGUID.clear();
        }
        return;
    }

    unordered_map<uint32, EverQuestCreatureMovementSound>::const_iterator soundIter = CreatureMovementSoundsByDisplayID.find(creature->GetDisplayId());
    if (soundIter == CreatureMovementSoundsByDisplayID.end())
        return;
    const vector<uint32>& pieceSoundEntryIDs = curGait == EQ_CREATURE_MOVEMENT_GAIT_WALK ? soundIter->second.WalkPieceSoundEntryIDs : soundIter->second.RunPieceSoundEntryIDs;
    const vector<uint32>& pieceDurationsMS = curGait == EQ_CREATURE_MOVEMENT_GAIT_WALK ? soundIter->second.WalkPieceDurationsMS : soundIter->second.RunPieceDurationsMS;
    if (pieceSoundEntryIDs.empty() == true)
    {
        if (state != nullptr)
        {
            state->CurGait = EQ_CREATURE_MOVEMENT_GAIT_NONE;
            state->ListenersByGUID.clear();
        }
        return;
    }
    if (state == nullptr)
        state = creature->CustomData.GetDefault<EverQuestCreatureMovementSoundState>(EQ_CREATURE_CUSTOMDATA_MOVEMENTSOUND);

    // Changing gait will restart the loop new for everyone
    bool gaitChanged = (state->CurGait != curGait);
    state->CurGait = curGait;
    if (gaitChanged == true)
        state->ListenerScanRemainingMS = 0;

    // Rescan who can hear this
    if (state->ListenerScanRemainingMS > diff)
        state->ListenerScanRemainingMS -= diff;
    else
    {
        state->ListenerScanRemainingMS = EQ_CREATURE_MOVEMENT_SOUND_LISTENER_SCAN_MS;
        unordered_map<ObjectGuid, EverQuestCreatureMovementSoundListener> refreshedListeners;
        Map::PlayerList const& mapPlayers = creature->GetMap()->GetPlayers();
        for (Map::PlayerList::const_iterator playerIter = mapPlayers.begin(); playerIter != mapPlayers.end(); ++playerIter)
        {
            Player* mapPlayer = playerIter->GetSource();
            if (mapPlayer == nullptr || mapPlayer->IsInWorld() == false)
                continue;
            if (creature->IsWithinDistInMap(mapPlayer, soundIter->second.MaxHearingDistance) == false)
                continue;
            unordered_map<ObjectGuid, EverQuestCreatureMovementSoundListener>::const_iterator listenerIter = state->ListenersByGUID.find(mapPlayer->GetGUID());
            if (listenerIter == state->ListenersByGUID.end() || gaitChanged == true)
            {
                creature->PlayDistanceSound(pieceSoundEntryIDs[0], mapPlayer);
                EverQuestCreatureMovementSoundListener newListener;
                newListener.PieceIndex = 0;
                newListener.ReplayRemainingMS = pieceDurationsMS[0];
                refreshedListeners[mapPlayer->GetGUID()] = newListener;
            }
            else
                refreshedListeners[mapPlayer->GetGUID()] = listenerIter->second;
        }
        state->ListenersByGUID.swap(refreshedListeners);
    }

    // Chain parts so a finished part moves to the next
    for (unordered_map<ObjectGuid, EverQuestCreatureMovementSoundListener>::iterator listenerIter = state->ListenersByGUID.begin(); listenerIter != state->ListenersByGUID.end(); ++listenerIter)
    {
        EverQuestCreatureMovementSoundListener& listener = listenerIter->second;
        if (listener.ReplayRemainingMS > diff)
        {
            listener.ReplayRemainingMS -= diff;
            continue;
        }
        uint32 overshootMS = diff - listener.ReplayRemainingMS;
        listener.PieceIndex = (listener.PieceIndex + 1) % pieceSoundEntryIDs.size();
        uint32 nextPieceDurationMS = pieceDurationsMS[listener.PieceIndex];
        if (nextPieceDurationMS == 0)
            nextPieceDurationMS = 1;
        if (overshootMS >= nextPieceDurationMS)
            overshootMS = nextPieceDurationMS - 1;
        listener.ReplayRemainingMS = nextPieceDurationMS - overshootMS;
        Player* listenerPlayer = ObjectAccessor::GetPlayer(*creature, listenerIter->first);
        if (listenerPlayer != nullptr)
            creature->PlayDistanceSound(pieceSoundEntryIDs[listener.PieceIndex], listenerPlayer);
    }
}

bool EverQuestMod::HasAliveCreatureWithEntryInMap(uint32 mapID, uint32 creatureTemplateID, Creature* ignoreCreature)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto mapIter = AllLoadedCreaturesByMapIDThenCreatureEntryID.find(mapID);
    if (mapIter == AllLoadedCreaturesByMapIDThenCreatureEntryID.end())
        return false;
    auto entryIter = mapIter->second.find(creatureTemplateID);
    if (entryIter == mapIter->second.end())
        return false;
    for (Creature* creature : entryIter->second)
    {
        if (creature == ignoreCreature)
            continue;
        if (creature->IsAlive() == true)
            return true;
    }
    return false;
}

void EverQuestMod::ProcessKillSpawnsForCreatureDeath(Creature* deadCreature, Unit* killer)
{
    auto killSpawnIter = CreatureKillSpawnsByTriggerCreatureTemplateID.find(deadCreature->GetEntry());
    if (killSpawnIter == CreatureKillSpawnsByTriggerCreatureTemplateID.end())
        return;
    Map* map = deadCreature->GetMap();
    uint32 mapID = map->GetId();
    uint32 deadCreatureLevel = deadCreature->GetLevel();

    // Roll which alternative wins in each alt group (weighted across distinct alt IDs)
    unordered_map<uint32, vector<pair<uint32, float>>> altWeightsByGroup;
    for (const EverQuestCreatureKillSpawn& killSpawn : killSpawnIter->second)
    {
        if (killSpawn.MapID != mapID || killSpawn.AltGroup == 0)
            continue;
        bool altSeen = false;
        for (pair<uint32, float>& altWeight : altWeightsByGroup[killSpawn.AltGroup])
            if (altWeight.first == killSpawn.AltID)
                altSeen = true;
        if (altSeen == false)
            altWeightsByGroup[killSpawn.AltGroup].push_back(pair<uint32, float>(killSpawn.AltID, killSpawn.AltWeight));
    }
    unordered_map<uint32, uint32> chosenAltIDByGroup;
    for (auto& altGroup : altWeightsByGroup)
    {
        float totalWeight = 0;
        for (pair<uint32, float>& altWeight : altGroup.second)
            totalWeight += altWeight.second;
        float weightRoll = frand(0, totalWeight);
        for (pair<uint32, float>& altWeight : altGroup.second)
        {
            weightRoll -= altWeight.second;
            if (weightRoll <= 0)
            {
                chosenAltIDByGroup[altGroup.first] = altWeight.first;
                break;
            }
        }
    }

    for (const EverQuestCreatureKillSpawn& killSpawn : killSpawnIter->second)
    {
        if (killSpawn.MapID != mapID)
            continue;
        if (killSpawn.TriggerMinLevel > 0 && deadCreatureLevel < killSpawn.TriggerMinLevel)
            continue;
        if (killSpawn.TriggerMaxLevel > 0 && deadCreatureLevel > killSpawn.TriggerMaxLevel)
            continue;
        if (killSpawn.AltGroup != 0 && chosenAltIDByGroup[killSpawn.AltGroup] != killSpawn.AltID)
            continue;
        if (killSpawn.Chance < 100 && roll_chance_f(killSpawn.Chance) == false)
            continue;
        bool requirementFailed = false;
        for (uint32 creatureTemplateID : killSpawn.RequireDeadCreatureTemplateIDs)
            if (HasAliveCreatureWithEntryInMap(mapID, creatureTemplateID, deadCreature) == true)
                requirementFailed = true;
        for (uint32 creatureTemplateID : killSpawn.RequireAliveCreatureTemplateIDs)
            if (HasAliveCreatureWithEntryInMap(mapID, creatureTemplateID, deadCreature) == false)
                requirementFailed = true;
        if (requirementFailed == true)
            continue;

        EverQuestPendingKillSpawnAction action;
        action.ActionType = killSpawn.ActionType;
        action.TargetCreatureTemplateID = killSpawn.TargetCreatureTemplateID;
        action.OnlyIfNotAliveCreatureTemplateID = killSpawn.OnlyIfNotAliveCreatureTemplateID;
        action.AddToHateList = killSpawn.AddToHateList;
        if (killer != nullptr)
            action.KillerGUID = killer->GetGUID();
        if (killSpawn.ActionType == EQ_KILLSPAWN_ACTION_RESPAWNSELF)
        {
            action.RespawnSpawnID = deadCreature->GetSpawnId();
            if (action.RespawnSpawnID == 0)
                continue;
        }
        else if (killSpawn.SpawnAtCorpse == true)
        {
            action.PositionX = deadCreature->GetPositionX() + killSpawn.PositionX;
            action.PositionY = deadCreature->GetPositionY() + killSpawn.PositionY;
            action.PositionZ = deadCreature->GetPositionZ() + killSpawn.PositionZ;
            action.Orientation = deadCreature->GetOrientation();
        }
        else
        {
            action.PositionX = killSpawn.PositionX;
            action.PositionY = killSpawn.PositionY;
            action.PositionZ = killSpawn.PositionZ;
            action.Orientation = killSpawn.Orientation;
            if (killSpawn.ActionType == EQ_KILLSPAWN_ACTION_DESPAWN && (killSpawn.PositionX != 0 || killSpawn.PositionY != 0))
                action.DespawnNearestToPositionOnly = true;
        }

        uint32 delayMS = killSpawn.DelayMinMS;
        if (killSpawn.DelayMaxMS > killSpawn.DelayMinMS)
            delayMS = urand(killSpawn.DelayMinMS, killSpawn.DelayMaxMS);
        if (delayMS == 0)
            ExecuteKillSpawnAction(map, action);
        else
        {
            action.RemainingMS = (int32)delayMS;
            std::lock_guard<std::mutex> lock(PendingKillSpawnActionsMutex);
            PendingKillSpawnActionsByMapID[mapID].push_back(action);
        }
    }
}

void EverQuestMod::ExecuteKillSpawnAction(Map* map, EverQuestPendingKillSpawnAction& action)
{
    switch (action.ActionType)
    {
        case EQ_KILLSPAWN_ACTION_SPAWN:
        {
            if (action.OnlyIfNotAliveCreatureTemplateID != 0 && HasAliveCreatureWithEntryInMap(map->GetId(), action.OnlyIfNotAliveCreatureTemplateID, nullptr) == true)
                return;
            Position spawnPosition(action.PositionX, action.PositionY, action.PositionZ, action.Orientation);
            TempSummon* summonedCreature = map->SummonCreature(action.TargetCreatureTemplateID, spawnPosition);
            if (summonedCreature == nullptr)
            {
                LOG_ERROR("module.EverQuest", "EverQuestMod::ExecuteKillSpawnAction failed to summon creature with template ID {}", action.TargetCreatureTemplateID);
                return;
            }
            if (action.AddToHateList == true && action.KillerGUID.IsEmpty() == false)
            {
                Unit* killer = ObjectAccessor::GetUnit(*summonedCreature, action.KillerGUID);
                if (killer != nullptr && killer->IsAlive() == true)
                {
                    summonedCreature->SetInCombatWith(killer);
                    summonedCreature->AddThreat(killer, 1.0f);
                }
            }
        } break;
        case EQ_KILLSPAWN_ACTION_DESPAWN:
        {
            // Copy out under the lock, since DespawnOrUnsummon re-enters hooks that also take RuntimeStateMutex
            vector<Creature*> despawnCandidates;
            {
                std::lock_guard<std::mutex> lock(RuntimeStateMutex);
                auto mapIter = AllLoadedCreaturesByMapIDThenCreatureEntryID.find(map->GetId());
                if (mapIter != AllLoadedCreaturesByMapIDThenCreatureEntryID.end())
                {
                    auto entryIter = mapIter->second.find(action.TargetCreatureTemplateID);
                    if (entryIter != mapIter->second.end())
                        for (Creature* creature : entryIter->second)
                            if (creature->IsAlive() == true)
                                despawnCandidates.push_back(creature);
                }
            }
            if (despawnCandidates.empty() == true)
                return;
            if (action.DespawnNearestToPositionOnly == true)
            {
                Creature* nearestCreature = despawnCandidates[0];
                float nearestDistance = 0;
                for (Creature* creature : despawnCandidates)
                {
                    float curDistance = creature->GetExactDist2dSq(action.PositionX, action.PositionY);
                    if (creature == despawnCandidates[0] || curDistance < nearestDistance)
                    {
                        nearestCreature = creature;
                        nearestDistance = curDistance;
                    }
                }
                DoCreatureEmoteEvent(nearestCreature, EQ_CREATURE_EMOTE_EVENT_ONDESPAWN, nullptr);
                nearestCreature->DespawnOrUnsummon();
            }
            else
            {
                for (Creature* creature : despawnCandidates)
                {
                    DoCreatureEmoteEvent(creature, EQ_CREATURE_EMOTE_EVENT_ONDESPAWN, nullptr);
                    creature->DespawnOrUnsummon();
                }
            }
        } break;
        case EQ_KILLSPAWN_ACTION_RESPAWNSELF:
        {
            // Copy out the respawning since respawn goes back to engine code and doesn't handle threading well
            vector<Creature*> respawnCandidates;
            auto spawnIDRange = map->GetCreatureBySpawnIdStore().equal_range(action.RespawnSpawnID);
            for (auto spawnIDIter = spawnIDRange.first; spawnIDIter != spawnIDRange.second; ++spawnIDIter)
                if (spawnIDIter->second->IsAlive() == false)
                    respawnCandidates.push_back(spawnIDIter->second);
            for (Creature* respawnCandidate : respawnCandidates)
                respawnCandidate->Respawn(true);
        } break;
        default: break;
    }
}

void EverQuestMod::UpdatePendingKillSpawnActions(Map* map, uint32 diff)
{
    uint32 mapID = map->GetId();
    vector<EverQuestPendingKillSpawnAction> dueActions;
    {
        std::lock_guard<std::mutex> lock(PendingKillSpawnActionsMutex);
        auto pendingIter = PendingKillSpawnActionsByMapID.find(mapID);
        if (pendingIter == PendingKillSpawnActionsByMapID.end())
            return;
        vector<EverQuestPendingKillSpawnAction>& pendingActions = pendingIter->second;
        for (size_t i = pendingActions.size(); i > 0; --i)
        {
            EverQuestPendingKillSpawnAction& action = pendingActions[i - 1];
            action.RemainingMS -= (int32)diff;
            if (action.RemainingMS <= 0)
            {
                dueActions.push_back(action);
                pendingActions.erase(pendingActions.begin() + (i - 1));
            }
        }
        if (pendingActions.empty() == true)
            PendingKillSpawnActionsByMapID.erase(pendingIter);
    }
    for (EverQuestPendingKillSpawnAction& action : dueActions)
        ExecuteKillSpawnAction(map, action);
}

void EverQuestMod::TriggerQuestKillSpawn(uint32 mapID, const EverQuestQuestReaction& questReaction)
{
    EverQuestTriggeredQuestKillSpawn triggeredKillSpawn;
    triggeredKillSpawn.TriggerCreatureTemplateID = questReaction.QuestgiverCreatureTemplateID;
    triggeredKillSpawn.TargetCreatureTemplateID = questReaction.CreatureTemplateID;
    triggeredKillSpawn.PositionX = questReaction.PositionX;
    triggeredKillSpawn.PositionY = questReaction.PositionY;
    triggeredKillSpawn.PositionZ = questReaction.PositionZ;
    triggeredKillSpawn.Orientation = questReaction.Orientation;

    // Repeat turn-ins only trigger one spawn, like the flag in the EQ quest scripts
    std::lock_guard<std::mutex> lock(PendingKillSpawnActionsMutex);
    for (EverQuestTriggeredQuestKillSpawn& existing : TriggeredQuestKillSpawnsByMapID[mapID])
        if (existing.TriggerCreatureTemplateID == triggeredKillSpawn.TriggerCreatureTemplateID && existing.TargetCreatureTemplateID == triggeredKillSpawn.TargetCreatureTemplateID)
            return;
    TriggeredQuestKillSpawnsByMapID[mapID].push_back(triggeredKillSpawn);
}

void EverQuestMod::ProcessTriggeredQuestKillSpawnsForCreatureDeath(Creature* deadCreature, Unit* killer)
{
    uint32 mapID = deadCreature->GetMapId();
    vector<EverQuestPendingKillSpawnAction> dueActions;
    {
        std::lock_guard<std::mutex> lock(PendingKillSpawnActionsMutex);
        auto triggeredIter = TriggeredQuestKillSpawnsByMapID.find(mapID);
        if (triggeredIter == TriggeredQuestKillSpawnsByMapID.end())
            return;
        vector<EverQuestTriggeredQuestKillSpawn>& triggeredKillSpawns = triggeredIter->second;
        for (size_t i = triggeredKillSpawns.size(); i > 0; --i)
        {
            EverQuestTriggeredQuestKillSpawn& triggeredKillSpawn = triggeredKillSpawns[i - 1];
            if (triggeredKillSpawn.TriggerCreatureTemplateID != deadCreature->GetEntry())
                continue;
            EverQuestPendingKillSpawnAction action;
            action.ActionType = EQ_KILLSPAWN_ACTION_SPAWN;
            action.TargetCreatureTemplateID = triggeredKillSpawn.TargetCreatureTemplateID;
            action.PositionX = triggeredKillSpawn.PositionX;
            action.PositionY = triggeredKillSpawn.PositionY;
            action.PositionZ = triggeredKillSpawn.PositionZ;
            action.Orientation = triggeredKillSpawn.Orientation;
            action.AddToHateList = true;
            if (killer != nullptr)
                action.KillerGUID = killer->GetGUID();
            dueActions.push_back(action);
            triggeredKillSpawns.erase(triggeredKillSpawns.begin() + (i - 1));
        }
        if (triggeredKillSpawns.empty() == true)
            TriggeredQuestKillSpawnsByMapID.erase(triggeredIter);
    }
    for (EverQuestPendingKillSpawnAction& action : dueActions)
        ExecuteKillSpawnAction(deadCreature->GetMap(), action);
}

void EverQuestMod::EnqueuePendingKillSpawnAction(uint32 mapID, EverQuestPendingKillSpawnAction& action)
{
    std::lock_guard<std::mutex> lock(PendingKillSpawnActionsMutex);
    PendingKillSpawnActionsByMapID[mapID].push_back(action);
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
    QueryResult queryResult = WorldDatabase.Query("SELECT ItemTemplateID, NPCEquipItemTemplateID, WornEffectSpellID, AllowedEQClassMask, EQArmorMaterial, IllusionTintID FROM mod_everquest_item_template ORDER BY ItemTemplateID;");
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
            everQuestItemTemplate.EQArmorMaterial = (uint32)std::max(0, fields[4].Get<int32>());
            everQuestItemTemplate.IllusionTintID = (uint32)std::max(0, fields[5].Get<int32>());
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

    // Compare base class (no class map row means the shift below would be undefined, so allow the item)
    const EverQuestClassMap classMap = GetClassMapForWOWClassID(player->getClass());
    if (classMap.EQClassIDBase == 0)
        return true;
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
    QueryResult queryResult = WorldDatabase.Query("SELECT SpellID, AuraDurationBaseInMS, AuraDurationAddPerLevelInMS, AuraDurationMaxInMS, AuraDurationCalcMinLevel, AuraDurationCalcMaxLevel, RecourseSpellID, SpellIDCastOnMeleeAttacker, FocusBoostType, PeriodicAuraSpellID, PeriodicAuraSpellRadius, MaleFormSpellID, FemaleFormSpellID, EffectFailChancePercent, EffectFailableType, StunUsesBashKickChance, SpellIDCastOnTargetWhenStunLands, AuraStaysOnSecondaryClassSwitch, MinTargetLevel, MaxCreatureTargetLevel FROM mod_everquest_spell ORDER BY SpellID;");
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
            everQuestSpell.AuraStaysOnSecondaryClassSwitch = fields[17].Get<bool>();
            everQuestSpell.MinTargetLevel = fields[18].Get<uint32>();
            everQuestSpell.MaxCreatureTargetLevel = fields[19].Get<uint32>();
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

void EverQuestMod::LoadIllusionDisplayData()
{
    IllusionDisplayIDsByLookupKey.clear();
    IllusionFormSpellIDs.clear();

    QueryResult queryResult = WorldDatabase.Query("SELECT FormSpellID, BodySet, TintID, HelmOn, DisplayID FROM mod_everquest_illusion_display;");
    if (!queryResult)
    {
        LOG_INFO("module.EverQuest", "EverQuestMod::LoadIllusionDisplayData found no mod_everquest_illusion_display rows, so illusion forms will not match worn gear");
        return;
    }
    do
    {
        // Pull the data out
        Field* fields = queryResult->Fetch();
        uint32 formSpellID = fields[0].Get<uint32>();
        uint32 bodySet = (uint32)std::max(0, fields[1].Get<int32>());
        uint32 tintID = (uint32)std::max(0, fields[2].Get<int32>());
        bool helmOn = fields[3].Get<bool>();
        uint32 displayID = fields[4].Get<uint32>();
        IllusionDisplayIDsByLookupKey[GetIllusionDisplayLookupKey(formSpellID, bodySet, tintID, helmOn)] = displayID;
        IllusionFormSpellIDs.insert(formSpellID);
    } while (queryResult->NextRow());
}

bool EverQuestMod::IsIllusionFormSpell(uint32 spellID)
{
    return IllusionFormSpellIDs.find(spellID) != IllusionFormSpellIDs.end();
}

uint64 EverQuestMod::GetIllusionDisplayLookupKey(uint32 formSpellID, uint32 bodySet, uint32 tintID, bool helmOn)
{
    // Stored as (high 32 bits) | tint (20 bits, from bit 12) | body set (8 bits, from bit 4) | helm (bit 0)
    return ((uint64)formSpellID << 32) | ((uint64)(tintID & 0xFFFFF) << 12) | ((uint64)(bodySet & 0xFF) << 4) | (uint64)(helmOn ? 1 : 0);
}

bool EverQuestMod::TryGetIllusionDisplayID(uint32 formSpellID, uint32 bodySet, uint32 tintID, bool helmOn, uint32& displayIDOut)
{
    auto displayItr = IllusionDisplayIDsByLookupKey.find(GetIllusionDisplayLookupKey(formSpellID, bodySet, tintID, helmOn));
    if (displayItr == IllusionDisplayIDsByLookupKey.end())
        return false;
    displayIDOut = displayItr->second;
    return true;
}

uint32 EverQuestMod::GetIllusionDisplayIDWithFallback(uint32 formSpellID, uint32 bodySet, uint32 tintID, bool helmOn)
{
    // If exact, don't use tint
    uint32 displayID = 0;
    if (TryGetIllusionDisplayID(formSpellID, bodySet, tintID, helmOn, displayID) == true)
        return displayID;
    if (TryGetIllusionDisplayID(formSpellID, bodySet, 0, helmOn, displayID) == true)
        return displayID;

    // Robe sets fall back to the base cloth set
    if (bodySet >= 10)
    {
        if (TryGetIllusionDisplayID(formSpellID, 0, 0, helmOn, displayID) == true)
            return displayID;
    }

    if (TryGetIllusionDisplayID(formSpellID, 0, 0, false, displayID) == true)
        return displayID;
    return 0;
}

uint32 EverQuestMod::GetIllusionBodySetForEQArmorMaterial(uint32 eqArmorMaterial)
{
    // 1-3 are leather/chain/plate, 10/16 are robe sets, and all else is cloth
    if (eqArmorMaterial >= 1 && eqArmorMaterial <= 3)
        return eqArmorMaterial;
    if (eqArmorMaterial >= 10 && eqArmorMaterial <= 16)
        return eqArmorMaterial;
    return 0;
}

void EverQuestMod::LoadIllusionFaceData()
{
    IllusionFaceDisplayIDsByLookupKey.clear();
    IllusionMaxFaceIndex = 0;

    // Rows only exist for face indexes of 1 and up, as face 0 is the base display itself
    QueryResult queryResult = WorldDatabase.Query("SELECT BaseDisplayID, FaceIndex, DisplayID FROM mod_everquest_illusion_face;");
    if (!queryResult)
    {
        LOG_INFO("module.EverQuest", "EverQuestMod::LoadIllusionFaceData found no mod_everquest_illusion_face rows, so illusion forms will always use the base (0) face");
        return;
    }
    do
    {
        // Pull the data out
        Field* fields = queryResult->Fetch();
        uint32 baseDisplayID = fields[0].Get<uint32>();
        uint32 faceIndex = (uint32)std::max(0, fields[1].Get<int32>());
        uint32 displayID = fields[2].Get<uint32>();
        IllusionFaceDisplayIDsByLookupKey[GetIllusionFaceLookupKey(baseDisplayID, faceIndex)] = displayID;
        if (faceIndex > IllusionMaxFaceIndex)
            IllusionMaxFaceIndex = faceIndex;
    } while (queryResult->NextRow());
}

uint64 EverQuestMod::GetIllusionFaceLookupKey(uint32 baseDisplayID, uint32 faceIndex)
{
    // Stored as base display ID (32 bits, from bit 8) | face index (8 bits, from bit 0)
    return ((uint64)baseDisplayID << 8) | (uint64)(faceIndex & 0xFF);
}

uint32 EverQuestMod::GetIllusionFaceDisplayIDForPlayer(Player* player, uint32 baseDisplayID)
{
    // Face 0 is the base display itself, and any (base display, face) pair without a row falls back to the base display, which also covers players whose selected face is out of range for the current form's race
    uint32 playerFaceID = GetIllusionFaceIDForPlayer(player);
    if (playerFaceID == 0)
        return baseDisplayID;
    auto faceItr = IllusionFaceDisplayIDsByLookupKey.find(GetIllusionFaceLookupKey(baseDisplayID, playerFaceID));
    if (faceItr == IllusionFaceDisplayIDsByLookupKey.end())
        return baseDisplayID;
    return faceItr->second;
}

uint32 EverQuestMod::GetIllusionGearDisplayIDForPlayer(Player* player, uint32 formSpellID)
{
    // Just use the chest to drive the outfit
    uint32 bodySet = 0;
    uint32 tintID = 0;
    Item* chestItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_CHEST);
    if (chestItem != nullptr)
    {
        auto itemTemplateItr = ItemTemplatesByEntryID.find(chestItem->GetEntry());
        if (itemTemplateItr != ItemTemplatesByEntryID.end())
        {
            bodySet = GetIllusionBodySetForEQArmorMaterial(itemTemplateItr->second.EQArmorMaterial);
            tintID = itemTemplateItr->second.IllusionTintID;
        }
        else
        {
            ItemTemplate const* itemProto = chestItem->GetTemplate();
            if (itemProto != nullptr && itemProto->Class == ITEM_CLASS_ARMOR)
            {
                switch (itemProto->SubClass)
                {
                    case ITEM_SUBCLASS_ARMOR_CLOTH: bodySet = 0; break;
                    case ITEM_SUBCLASS_ARMOR_LEATHER: bodySet = 1; break;
                    case ITEM_SUBCLASS_ARMOR_MAIL: bodySet = 2; break;
                    case ITEM_SUBCLASS_ARMOR_PLATE: bodySet = 3; break;
                    default: break;
                }
            }
        }
    }

    // The helm shows when an armor head item is worn and the player isn't hiding it via the interface option
    bool helmOn = false;
    Item* headItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_HEAD);
    if (headItem != nullptr && headItem->GetTemplate() != nullptr && headItem->GetTemplate()->Class == ITEM_CLASS_ARMOR &&
        player->HasPlayerFlag(PLAYER_FLAGS_HIDE_HELM) == false)
        helmOn = true;

    return GetIllusionDisplayIDWithFallback(formSpellID, bodySet, tintID, helmOn);
}

void EverQuestMod::ApplyIllusionGearDisplayIfChanged(Player* player, EverQuestPlayerIllusionState* illusionState)
{
    // A zero result means no one doesn't exist, so leave the core's transform display alone
    uint32 gearDisplayID = GetIllusionGearDisplayIDForPlayer(player, illusionState->FormSpellID);
    if (gearDisplayID == 0)
        return;

    // Swap in the player's selected face version of the display when one exists
    uint32 faceDisplayID = GetIllusionFaceDisplayIDForPlayer(player, gearDisplayID);
    if (faceDisplayID == illusionState->LastGearDisplayID)
        return;
    illusionState->LastGearDisplayID = faceDisplayID;
    player->SetDisplayId(faceDisplayID);
}

void EverQuestMod::ApplyIllusionGearDisplayOnFormAuraApply(Player* player, uint32 formSpellID)
{
    // Track the player illusion state
    EverQuestPlayerIllusionState* illusionState = nullptr;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        illusionState = &PlayerIllusionStatesByPlayerGUID[player->GetGUID()];
    }
    illusionState->FormSpellID = formSpellID;
    illusionState->LastGearDisplayID = 0;
    illusionState->RefreshTimerMS = 0;

    // Override the model with a gear-matched version
    ApplyIllusionGearDisplayIfChanged(player, illusionState);
}

void EverQuestMod::HandleIllusionFormAuraRemove(Player* player, uint32 spellID)
{
    if (IsIllusionFormSpell(spellID) == false)
        return;

    // Cleanup tracking
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto illusionStateItr = PlayerIllusionStatesByPlayerGUID.find(player->GetGUID());
        if (illusionStateItr == PlayerIllusionStatesByPlayerGUID.end())
            return;
        if (illusionStateItr->second.FormSpellID != spellID)
        {
            // If a different form fell off, force a refresh next cycle
            illusionStateItr->second.LastGearDisplayID = 0;
            return;
        }
        PlayerIllusionStatesByPlayerGUID.erase(illusionStateItr);
    }

    // If there's another form, swap in
    for (auto const& appliedAuraItr : player->GetAppliedAuras())
    {
        AuraApplication const* appliedAurApp = appliedAuraItr.second;
        if (appliedAurApp == nullptr || appliedAurApp->GetBase() == nullptr)
            continue;
        uint32 appliedSpellID = appliedAurApp->GetBase()->GetId();
        if (IsIllusionFormSpell(appliedSpellID) == false)
            continue;
        ApplyIllusionGearDisplayOnFormAuraApply(player, appliedSpellID);
        return;
    }
}

void EverQuestMod::RefreshIllusionGearDisplayForPlayer(Player* player)
{
    EverQuestPlayerIllusionState* illusionState = nullptr;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto illusionStateItr = PlayerIllusionStatesByPlayerGUID.find(player->GetGUID());
        if (illusionStateItr == PlayerIllusionStatesByPlayerGUID.end())
            return;
        illusionState = &illusionStateItr->second;
    }
    ApplyIllusionGearDisplayIfChanged(player, illusionState);
}

void EverQuestMod::UpdatePlayerIllusionGearDisplay(Player* player, uint32 diffInMS)
{
    // Zero (or below) disables the periodic check
    if (ConfigIllusionGearRefreshTimeInMS <= 0)
        return;

    EverQuestPlayerIllusionState* illusionState = nullptr;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto illusionStateItr = PlayerIllusionStatesByPlayerGUID.find(player->GetGUID());
        if (illusionStateItr == PlayerIllusionStatesByPlayerGUID.end())
            return;
        illusionState = &illusionStateItr->second;
    }

    // Light periodic check, since unequips and the show-helm interface toggle have no hooks
    illusionState->RefreshTimerMS += diffInMS;
    if (illusionState->RefreshTimerMS < ConfigIllusionGearRefreshTimeInMS)
        return;
    illusionState->RefreshTimerMS = 0;
    ApplyIllusionGearDisplayIfChanged(player, illusionState);
}

void EverQuestMod::ClearIllusionTrackingForPlayer(ObjectGuid playerGUID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    PlayerIllusionStatesByPlayerGUID.erase(playerGUID);
}

bool EverQuestMod::IsSpellBlockedByMinTargetLevel(uint32 spellID, Unit* target, Unit* caster)
{
    if (ConfigSpellBuffLevelRestrictionsEnabled == false)
        return false;
    if (target == nullptr || target->IsPlayer() == false)
        return false;
    const EverQuestSpell& spellData = GetSpellDataForSpellID(spellID);
    if (spellData.MinTargetLevel == 0)
        return false;
    if (target->GetLevel() >= spellData.MinTargetLevel)
        return false;
    if (caster != nullptr && caster->IsPlayer() == true && caster->ToPlayer()->IsGameMaster() == true)
        return false;
    return true;
}

bool EverQuestMod::IsSpellBlockedByMaxCreatureTargetLevel(uint32 spellID, Unit* target, Unit* caster)
{
    // TAKP blocks player-cast stun/mez/charm on NPCs above the spell's max level, NPC casters (including player pets) are exempt
    if (ConfigSpellCrowdControlLevelRestrictionsEnabled == false)
        return false;
    if (target == nullptr || target->IsCreature() == false)
        return false;
    if (caster == nullptr || caster->IsPlayer() == false)
        return false;
    if (caster->ToPlayer()->IsGameMaster() == true)
        return false;
    const EverQuestSpell& spellData = GetSpellDataForSpellID(spellID);
    if (spellData.MaxCreatureTargetLevel == 0)
        return false;
    if (target->GetLevel() <= spellData.MaxCreatureTargetLevel)
        return false;
    return true;
}

bool EverQuestMod::IsCreatureCharmBlockedByCharmLimits(uint32 spellID, Unit* target, Unit* caster)
{
    if (ConfigCharmCreatureCharmLimitsEnabled == false)
        return false;
    if (caster == nullptr || caster->IsCreature() == false)
        return false;
    if (spellID < ConfigSystemSpellDBCIDMin || spellID > ConfigSystemSpellDBCIDMax)
        return false;
    if (IsSpellAnEQSpell(spellID) == false)
        return false;
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellID);
    if (spellInfo == nullptr || spellInfo->HasAura(SPELL_AURA_MOD_CHARM) == false)
        return false;

    // Block if the creature already has a player charmed
    Unit* existingCharm = caster->GetCharm();
    if (existingCharm != nullptr && existingCharm->IsPlayer() == true)
        return true;

    // Block charming a player unless at least one other uncharmed player is nearby (the target counts as one)
    if (target == nullptr || target->IsPlayer() == false)
        return false;
    uint32 nearbyUncharmedPlayerCount = 0;
    Map::PlayerList const& mapPlayers = caster->GetMap()->GetPlayers();
    for (Map::PlayerList::const_iterator playerIter = mapPlayers.begin(); playerIter != mapPlayers.end(); ++playerIter)
    {
        Player* mapPlayer = playerIter->GetSource();
        if (mapPlayer == nullptr || mapPlayer->IsAlive() == false || mapPlayer->IsGameMaster() == true)
            continue;
        if (mapPlayer->IsCharmed() == true)
            continue;
        if (caster->IsWithinDistInMap(mapPlayer, ConfigCharmUncharmedPlayerCheckRadius) == false)
            continue;
        nearbyUncharmedPlayerCount++;
        if (nearbyUncharmedPlayerCount >= 2)
            return false;
    }
    return true;
}

void EverQuestMod::TrackEQHasteAurasAndEnforceCapOnAuraApply(Unit* unit, Aura* aura)
{
    if (ConfigSpellHasteCapEnabled == false)
        return;
    uint32 spellID = aura->GetId();
    if (spellID < ConfigSystemSpellDBCIDMin || spellID > ConfigSystemSpellDBCIDMax)
        return;
    if (IsSpellAnEQSpell(spellID) == false)
        return;

    // Only positive melee/ranged haste effects count against the cap (slows stay untouched)
    bool hasPositiveHasteEffect = false;
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        AuraEffect* auraEffect = aura->GetEffect(i);
        if (auraEffect == nullptr)
            continue;
        AuraType auraType = auraEffect->GetAuraType();
        if (auraType != SPELL_AURA_MOD_MELEE_HASTE && auraType != SPELL_AURA_MOD_RANGED_HASTE)
            continue;
        if (auraEffect->GetAmount() <= 0)
            continue;
        hasPositiveHasteEffect = true;
        break;
    }
    if (hasPositiveHasteEffect == false)
        return;

    // Only the lookup needs the lock (the vector itself is only touched by the unit's own map thread)
    vector<EverQuestUnitHasteAuraEffect>* trackedHasteAuraEffects = nullptr;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        trackedHasteAuraEffects = &EQHasteAuraEffectsByUnitGUID[unit->GetGUID()];
    }

    // Capture the natural (pre-cap) amounts.  Buff refreshes reset effect amounts to their recalculated natural values before this hook fires, so the current amount is always the natural amount here
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        AuraEffect* auraEffect = aura->GetEffect(i);
        if (auraEffect == nullptr)
            continue;
        AuraType auraType = auraEffect->GetAuraType();
        if (auraType != SPELL_AURA_MOD_MELEE_HASTE && auraType != SPELL_AURA_MOD_RANGED_HASTE)
            continue;
        if (auraEffect->GetAmount() <= 0)
            continue;

        bool foundExisting = false;
        for (EverQuestUnitHasteAuraEffect& trackedHasteAuraEffect : *trackedHasteAuraEffects)
        {
            if (trackedHasteAuraEffect.SpellID == spellID && trackedHasteAuraEffect.CasterGUID == aura->GetCasterGUID() && trackedHasteAuraEffect.EffectIndex == i)
            {
                trackedHasteAuraEffect.NaturalAmount = auraEffect->GetAmount();
                foundExisting = true;
                break;
            }
        }
        if (foundExisting == false)
        {
            EverQuestUnitHasteAuraEffect trackedHasteAuraEffect;
            trackedHasteAuraEffect.SpellID = spellID;
            trackedHasteAuraEffect.CasterGUID = aura->GetCasterGUID();
            trackedHasteAuraEffect.EffectIndex = i;
            trackedHasteAuraEffect.AuraType = (uint32)auraType;
            trackedHasteAuraEffect.NaturalAmount = auraEffect->GetAmount();
            trackedHasteAuraEffects->push_back(trackedHasteAuraEffect);
        }
    }

    EnforceEQHastePercentCapOnUnit(unit, *trackedHasteAuraEffects);
}

void EverQuestMod::UntrackEQHasteAurasAndEnforceCapOnAuraRemove(Unit* unit, Aura* aura)
{
    if (ConfigSpellHasteCapEnabled == false)
        return;
    uint32 spellID = aura->GetId();
    if (spellID < ConfigSystemSpellDBCIDMin || spellID > ConfigSystemSpellDBCIDMax)
        return;

    // Only the lookup needs the lock (the vector itself is only touched by the unit's own map thread)
    vector<EverQuestUnitHasteAuraEffect>* trackedHasteAuraEffects = nullptr;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto trackedIter = EQHasteAuraEffectsByUnitGUID.find(unit->GetGUID());
        if (trackedIter == EQHasteAuraEffectsByUnitGUID.end())
            return;
        trackedHasteAuraEffects = &trackedIter->second;
    }

    bool removedAny = false;
    for (vector<EverQuestUnitHasteAuraEffect>::iterator effectIter = trackedHasteAuraEffects->begin(); effectIter != trackedHasteAuraEffects->end();)
    {
        if (effectIter->SpellID == spellID && effectIter->CasterGUID == aura->GetCasterGUID())
        {
            effectIter = trackedHasteAuraEffects->erase(effectIter);
            removedAny = true;
        }
        else
            ++effectIter;
    }
    if (removedAny == false)
        return;

    if (trackedHasteAuraEffects->empty() == true)
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        EQHasteAuraEffectsByUnitGUID.erase(unit->GetGUID());
        return;
    }

    EnforceEQHastePercentCapOnUnit(unit, *trackedHasteAuraEffects);
}

void EverQuestMod::EnforceEQHastePercentCapOnUnit(Unit* unit, vector<EverQuestUnitHasteAuraEffect>& trackedHasteAuraEffects)
{
    float capPercent = ConfigSpellHasteCapPercent;
    if (capPercent < 0)
        capPercent = 0;

    // Melee and ranged haste cap independently, and while WoW stacks haste auras multiplicatively EQ stacks them additively
    // So walk the effects in apply order and clamp each applied amount such that the combined multiplier equals what the capped additive total would give
    uint32 auraTypesToProcess[2] = { SPELL_AURA_MOD_MELEE_HASTE, SPELL_AURA_MOD_RANGED_HASTE };
    for (uint32 auraType : auraTypesToProcess)
    {
        float naturalTotalPercent = 0;
        float previousCappedTotalPercent = 0;
        for (EverQuestUnitHasteAuraEffect& trackedHasteAuraEffect : trackedHasteAuraEffects)
        {
            if (trackedHasteAuraEffect.AuraType != auraType)
                continue;
            AuraEffect* auraEffect = unit->GetAuraEffect(trackedHasteAuraEffect.SpellID, trackedHasteAuraEffect.EffectIndex, trackedHasteAuraEffect.CasterGUID);
            if (auraEffect == nullptr)
                continue;
            naturalTotalPercent += (float)trackedHasteAuraEffect.NaturalAmount;
            float cappedTotalPercent = std::min(naturalTotalPercent, capPercent);
            int32 newAmount = (int32)std::lround(100.0f * ((100.0f + cappedTotalPercent) / (100.0f + previousCappedTotalPercent)) - 100.0f);
            previousCappedTotalPercent = cappedTotalPercent;
            if (auraEffect->GetAmount() != newAmount)
                auraEffect->ChangeAmount(newAmount);
        }
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
    QueryResult queryResult = WorldDatabase.Query("SELECT QuestTemplateID, ReactionType, UsePlayerX, UsePlayerY, UsePlayerZ, AddedPlayerX, AddedPlayerY, UsePlayerOrientation, PositionX, PositionY, PositionZ, Orientation, CreatureTemplateID, QuestgiverCreatureTemplateID, DelayInMS FROM mod_everquest_quest_reaction;");
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
            everQuestQuestReaction.QuestgiverCreatureTemplateID = fields[13].Get<uint32>();
            everQuestQuestReaction.DelayInMS = fields[14].Get<uint32>();
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

void EverQuestMod::LoadGossipReactions()
{
    GossipReactionsByGossipCreatureTemplateID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT GossipCreatureTemplateID, NpcTextID, OptionID, OptionText, ReactionType, SayText, TargetCreatureTemplateID, UsePlayerX, UsePlayerY, UsePlayerZ, AddedPlayerX, AddedPlayerY, UsePlayerOrientation, UseNpcX, UseNpcY, UseNpcZ, UseNpcOrientation, PositionX, PositionY, PositionZ, Orientation, DelayInMS FROM mod_everquest_gossip_reaction ORDER BY GossipCreatureTemplateID, ID;");
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            EverQuestGossipReaction gossipReaction;
            gossipReaction.GossipCreatureTemplateID = fields[0].Get<uint32>();
            gossipReaction.NpcTextID = fields[1].Get<uint32>();
            gossipReaction.OptionID = fields[2].Get<uint32>();
            gossipReaction.OptionText = fields[3].Get<std::string>();
            gossipReaction.ReactionType = fields[4].Get<int32>();
            gossipReaction.SayText = fields[5].Get<std::string>();
            gossipReaction.TargetCreatureTemplateID = fields[6].Get<uint32>();
            gossipReaction.UsePlayerX = fields[7].Get<bool>();
            gossipReaction.UsePlayerY = fields[8].Get<bool>();
            gossipReaction.UsePlayerZ = fields[9].Get<bool>();
            gossipReaction.AddedPlayerX = fields[10].Get<float>();
            gossipReaction.AddedPlayerY = fields[11].Get<float>();
            gossipReaction.UsePlayerOrientation = fields[12].Get<bool>();
            gossipReaction.UseNpcX = fields[13].Get<bool>();
            gossipReaction.UseNpcY = fields[14].Get<bool>();
            gossipReaction.UseNpcZ = fields[15].Get<bool>();
            gossipReaction.UseNpcOrientation = fields[16].Get<bool>();
            gossipReaction.PositionX = fields[17].Get<float>();
            gossipReaction.PositionY = fields[18].Get<float>();
            gossipReaction.PositionZ = fields[19].Get<float>();
            gossipReaction.Orientation = fields[20].Get<float>();
            gossipReaction.DelayInMS = fields[21].Get<uint32>();
            GossipReactionsByGossipCreatureTemplateID[gossipReaction.GossipCreatureTemplateID].push_back(gossipReaction);
        } while (queryResult->NextRow());
    }
}

bool EverQuestMod::HandleGossipHello(Player* player, Creature* creature)
{
    if (IsEnabled == false)
        return false;

    // Talking to the creature is the closest analog to saying 'Hail' in EQ
    bool firedHailedEmote = DoCreatureEmoteEvent(creature, EQ_CREATURE_EMOTE_EVENT_HAILED, player);

    unordered_map<uint32, vector<EverQuestGossipReaction>>::const_iterator gossipReactionsIterator = GossipReactionsByGossipCreatureTemplateID.find(creature->GetEntry());
    if (gossipReactionsIterator == GossipReactionsByGossipCreatureTemplateID.end())
    {
        // Creatures that only exist as gossip targets for a hailed emote shouldn't open an empty gossip window, but any creature with a real role should fall through to its normal handling
        if (firedHailedEmote == true && creature->IsQuestGiver() == false && creature->IsVendor() == false && creature->IsTrainer() == false
            && creature->HasNpcFlag(UNIT_NPC_FLAG_BANKER) == false && creature->HasNpcFlag(UNIT_NPC_FLAG_STABLEMASTER) == false && creature->HasNpcFlag(UNIT_NPC_FLAG_INNKEEPER) == false)
        {
            CloseGossipMenuFor(player);
            return true;
        }
        return false;
    }

    ClearGossipMenuFor(player);
    if (creature->IsQuestGiver() == true)
        player->PrepareQuestMenu(creature->GetGUID());

    // Rows are ordered, so each option's text comes from its first reaction row
    uint32 npcTextID = 0;
    set<uint32> addedOptionIDs;
    for (const EverQuestGossipReaction& gossipReaction : gossipReactionsIterator->second)
    {
        npcTextID = gossipReaction.NpcTextID;
        if (addedOptionIDs.find(gossipReaction.OptionID) != addedOptionIDs.end())
            continue;
        addedOptionIDs.insert(gossipReaction.OptionID);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, gossipReaction.OptionText, GOSSIP_SENDER_MAIN, gossipReaction.OptionID);
    }
    SendGossipMenuFor(player, npcTextID, creature->GetGUID());
    return true;
}

bool EverQuestMod::HandleGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
{
    if (IsEnabled == false)
        return false;
    unordered_map<uint32, vector<EverQuestGossipReaction>>::const_iterator gossipReactionsIterator = GossipReactionsByGossipCreatureTemplateID.find(creature->GetEntry());
    if (gossipReactionsIterator == GossipReactionsByGossipCreatureTemplateID.end())
        return false;

    Map* map = creature->GetMap();
    bool reactionMatched = false;
    for (const EverQuestGossipReaction& gossipReaction : gossipReactionsIterator->second)
    {
        if (gossipReaction.OptionID != action)
            continue;
        reactionMatched = true;

        float x = gossipReaction.PositionX;
        if (gossipReaction.UsePlayerX == true)
            x = player->GetPositionX() + gossipReaction.AddedPlayerX;
        else if (gossipReaction.UseNpcX == true)
            x = creature->GetPositionX();
        float y = gossipReaction.PositionY;
        if (gossipReaction.UsePlayerY == true)
            y = player->GetPositionY() + gossipReaction.AddedPlayerY;
        else if (gossipReaction.UseNpcY == true)
            y = creature->GetPositionY();
        float z = gossipReaction.PositionZ;
        if (gossipReaction.UsePlayerZ == true)
            z = player->GetPositionZ();
        else if (gossipReaction.UseNpcZ == true)
            z = creature->GetPositionZ();
        float orientation = gossipReaction.Orientation;
        if (gossipReaction.UsePlayerOrientation == true)
            orientation = player->GetOrientation();
        else if (gossipReaction.UseNpcOrientation == true)
            orientation = creature->GetOrientation();

        switch (gossipReaction.ReactionType)
        {
        case EQ_QUEST_REACTION_SAY:
        {
            creature->Say(FormatGossipTextForPlayer(player, gossipReaction.SayText), LANG_UNIVERSAL, player);
        } break;
        case EQ_QUEST_REACTION_EMOTE:
        {
            // Monster emote text renders raw on the client (no speaker name), so bake the name in
            creature->TextEmote(creature->GetName() + " " + FormatGossipTextForPlayer(player, gossipReaction.SayText), player);
        } break;
        case EQ_QUEST_REACTION_YELL:
        {
            creature->Yell(FormatGossipTextForPlayer(player, gossipReaction.SayText), LANG_UNIVERSAL, player);
        } break;
        case EQ_QUEST_REACTION_ATTACKPLAYER:
        {
            MakeCreatureAttackPlayer(gossipReaction.TargetCreatureTemplateID, map, player);
        } break;
        case EQ_QUEST_REACTION_DESPAWN:
        {
            if (gossipReaction.DelayInMS > 0)
            {
                EverQuestPendingKillSpawnAction pendingAction;
                pendingAction.ActionType = EQ_KILLSPAWN_ACTION_DESPAWN;
                pendingAction.TargetCreatureTemplateID = gossipReaction.TargetCreatureTemplateID;
                pendingAction.RemainingMS = (int32)gossipReaction.DelayInMS;
                if (gossipReaction.TargetCreatureTemplateID == creature->GetEntry())
                {
                    // Only despawn the copy of the creature that the player is talking to
                    pendingAction.DespawnNearestToPositionOnly = true;
                    pendingAction.PositionX = creature->GetPositionX();
                    pendingAction.PositionY = creature->GetPositionY();
                    pendingAction.PositionZ = creature->GetPositionZ();
                }
                EnqueuePendingKillSpawnAction(map->GetId(), pendingAction);
            }
            else if (gossipReaction.TargetCreatureTemplateID == creature->GetEntry())
            {
                DoCreatureEmoteEvent(creature, EQ_CREATURE_EMOTE_EVENT_ONDESPAWN, nullptr);
                creature->DespawnOrUnsummon(0ms);
            }
            else
                DespawnCreature(gossipReaction.TargetCreatureTemplateID, map);
        } break;
        case EQ_QUEST_REACTION_SPAWN:
        case EQ_QUEST_REACTION_SPAWNUNIQUE:
        {
            if (gossipReaction.DelayInMS > 0)
            {
                EverQuestPendingKillSpawnAction pendingAction;
                pendingAction.ActionType = EQ_KILLSPAWN_ACTION_SPAWN;
                pendingAction.TargetCreatureTemplateID = gossipReaction.TargetCreatureTemplateID;
                if (gossipReaction.ReactionType == EQ_QUEST_REACTION_SPAWNUNIQUE)
                    pendingAction.OnlyIfNotAliveCreatureTemplateID = gossipReaction.TargetCreatureTemplateID;
                pendingAction.PositionX = x;
                pendingAction.PositionY = y;
                pendingAction.PositionZ = z;
                pendingAction.Orientation = orientation;
                pendingAction.RemainingMS = (int32)gossipReaction.DelayInMS;
                EnqueuePendingKillSpawnAction(map->GetId(), pendingAction);
            }
            else
                SpawnCreature(gossipReaction.TargetCreatureTemplateID, map, x, y, z, orientation, gossipReaction.ReactionType == EQ_QUEST_REACTION_SPAWNUNIQUE);
        } break;
        default: break; // Nothing
        }
    }

    if (reactionMatched == false)
        return false;
    CloseGossipMenuFor(player);
    return true;
}

string EverQuestMod::FormatGossipTextForPlayer(Player* player, const string& text)
{
    string formattedText = text;
    size_t tokenPosition = formattedText.find("$N");
    while (tokenPosition != string::npos)
    {
        formattedText.replace(tokenPosition, 2, player->GetName());
        tokenPosition = formattedText.find("$N", tokenPosition + player->GetName().length());
    }
    return formattedText;
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
    QueryResult queryResult = WorldDatabase.Query("SELECT eqclass, race, spell, level FROM mod_everquest_playerautolearnspells;");
    if (queryResult)
    {
        do
        {
            EverQuestAutoLearnSpell autoLearnSpell;
            Field* fields = queryResult->Fetch();
            autoLearnSpell.EQClassID = fields[0].Get<uint8>();
            autoLearnSpell.RaceID = fields[1].Get<uint8>();
            autoLearnSpell.SpellID = fields[2].Get<uint32>();
            autoLearnSpell.Level = fields[3].Get<uint8>();
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

void EverQuestMod::ApplyAutoLearnedClassSkillsAndSpells(Player* player)
{
    const EverQuestClassMap classMap = GetClassMapForWOWClassID(player->getClass());
    uint8 secondClassID = GetCurrentSecondEQClassForPlayer(player);
    vector<uint8> autoLearnEQClassIDs;
    autoLearnEQClassIDs.push_back(classMap.EQClassIDBase);
    if (secondClassID != EQ_EQCLASS_NONE && secondClassID != classMap.EQClassIDBase)
        autoLearnEQClassIDs.push_back(secondClassID);

    // Learn any spells the player may not have
    bool needsUpdate = false;
    for (uint8 autoLearnEQClassID : autoLearnEQClassIDs)
    {
        for (auto autoLearnSpell : GetAutoLearnSpellsForClass(autoLearnEQClassID))
        {
            // A race of 0 means the spell is learned regardless of race
            if (autoLearnSpell.RaceID != 0 && autoLearnSpell.RaceID != player->getRace())
                continue;
            // Only learn once the player has reached the spell's required level
            if (player->GetLevel() < autoLearnSpell.Level)
                continue;
            if (player->HasSpell(autoLearnSpell.SpellID) == false)
            {
                player->learnSpell(autoLearnSpell.SpellID);
                needsUpdate = true;
            }
        }
    }

    // Learn any skills the player may not have
    for (uint8 autoLearnEQClassID : autoLearnEQClassIDs)
    {
        for (auto skillID : GetAutoLearnSkillsForClass(autoLearnEQClassID))
        {
            if (player->GetSkillValue(skillID) == 0)
            {
                player->SetSkill((uint16)skillID, 0, 1, 1);
                needsUpdate = true;
            }
        }
    }

    // Only force an update to the player if there is one
    if (needsUpdate == true)
        player->UpdateSkillsForLevel();
}

void EverQuestMod::LoadAutoAddItemsData()
{
    PlayerAutoAddItemsByEQClassID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT eqclass, item FROM mod_everquest_playerautoadditems;");
    if (queryResult)
    {
        do
        {
            // Pull the data out
            Field* fields = queryResult->Fetch();
            uint8 classID = fields[0].Get<uint8>();
            uint32 itemID = fields[1].Get<uint32>();
            PlayerAutoAddItemsByEQClassID[classID].push_back(itemID);
        } while (queryResult->NextRow());
    }
}

const list<uint32>& EverQuestMod::GetAutoAddItemsForClass(uint8 classID)
{
    if (PlayerAutoAddItemsByEQClassID.find(classID) != PlayerAutoAddItemsByEQClassID.end())
    {
        return PlayerAutoAddItemsByEQClassID[classID];
    }
    else
    {
        static const list<uint32> returnEmpty;
        return returnEmpty;
    }
}

void EverQuestMod::ApplyAutoAddedClassItems(Player* player)
{
    const EverQuestClassMap classMap = GetClassMapForWOWClassID(player->getClass());
    uint8 secondClassID = GetCurrentSecondEQClassForPlayer(player);
    vector<uint8> autoAddEQClassIDs;
    autoAddEQClassIDs.push_back(classMap.EQClassIDBase);
    if (secondClassID != EQ_EQCLASS_NONE && secondClassID != classMap.EQClassIDBase)
        autoAddEQClassIDs.push_back(secondClassID);

    // Grant any items the player does not already have carried (equipped or inventory, ignoring the bank)
    for (uint8 autoAddEQClassID : autoAddEQClassIDs)
    {
        for (auto itemID : GetAutoAddItemsForClass(autoAddEQClassID))
        {
            if (player->HasItemCount(itemID, 1, false) == true)
                continue;

            ItemPosCountVec destPosition;
            InventoryResult invResult = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, destPosition, itemID, 1);
            if (invResult == EQUIP_ERR_OK)
                player->StoreNewItem(destPosition, itemID, true);
        }
    }
}

void EverQuestMod::GrantLegacyAchievementIfEligible(Player* player)
{
    if (ConfigSystemLegacyAchievementID == 0 || ConfigSystemLegacyAchievementAccountCreatedBefore.empty() == true)
        return;
    if (player->HasAchieved(ConfigSystemLegacyAchievementID) == true)
        return;

    AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(ConfigSystemLegacyAchievementID);
    if (achievementEntry == nullptr)
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod::GrantLegacyAchievementIfEligible error, no achievement with ID {} exists", ConfigSystemLegacyAchievementID);
        return;
    }

    // Only grant to characters on accounts created before the configured date
    uint32 accountID = player->GetSession()->GetAccountId();
    QueryResult accountEligibleQueryResult = LoginDatabase.Query("SELECT 1 FROM account WHERE id = {} AND joindate < '{}'", accountID, ConfigSystemLegacyAchievementAccountCreatedBefore);
    if (!accountEligibleQueryResult)
        return;
    player->CompletedAchievement(achievementEntry);
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
    QueryResult queryResult = WorldDatabase.Query("SELECT CreatureTemplateID, LootGroupID, GroupMultiplier, GroupMultiplierMin, GroupProbability, DropLimit, MinDrop, ItemTemplateID, Chance, ItemMultiplier, ItemCharges FROM mod_everquest_creature_loot ORDER BY CreatureTemplateID, LootGroupID");
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
                newLootGroup.GroupMultiplierMin = fields[3].Get<uint32>();
                newLootGroup.GroupProbability = fields[4].Get<float>();
                newLootGroup.DropLimit = fields[5].Get<uint32>();
                newLootGroup.MinDrop = fields[6].Get<uint32>();
                lootGroups.push_back(newLootGroup);
                lootGroup = &lootGroups.back();
            }

            EverQuestCreatureLootEntry lootEntry;
            lootEntry.ItemTemplateID = fields[7].Get<uint32>();
            lootEntry.Chance = fields[8].Get<float>();
            lootEntry.ItemMultiplier = fields[9].Get<uint32>();
            lootEntry.ItemCharges = fields[10].Get<uint32>();
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
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    if (PreloadedLootItemIDsByCreatureGUID.find(creatureGUID) != PreloadedLootItemIDsByCreatureGUID.end())
        return true;
    else
        return false;
}

bool EverQuestMod::HasPreloadedLootItemIDForCreatureGUID(ObjectGuid creatureGUID, uint32 itemTemplateID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto preloadedIt = PreloadedLootItemIDsByCreatureGUID.find(creatureGUID);
    if (preloadedIt == PreloadedLootItemIDsByCreatureGUID.end())
        return false;

    for (uint32 preloadedLootItemTemplateID : preloadedIt->second)
    {
        if (preloadedLootItemTemplateID == itemTemplateID)
            return true;
    }
    return false;
}

uint32 EverQuestMod::GetPreloadedLootCountForCreatureGUID(ObjectGuid creatureGUID, uint32 itemTemplateID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
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
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto preloadedIt = PreloadedLootItemIDsByCreatureGUID.find(creatureGUID);
    if (preloadedIt != PreloadedLootItemIDsByCreatureGUID.end())
    {
        return preloadedIt->second;
    }
    else
    {
        static const vector<uint32> returnEmpty;
        return returnEmpty;
    }
}

void EverQuestMod::ClearPreloadedLootIDsForCreatureGUID(ObjectGuid creatureGUID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    PreloadedLootItemIDsByCreatureGUID.erase(creatureGUID);
    PreloadedLootCountsByCreatureGUID.erase(creatureGUID);
}

void EverQuestMod::TrackVisualEquippedItemsForCreatureGUID(ObjectGuid creatureGUID, uint32 mainhandItemID, uint32 offhandItemID, bool isDualWielding)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    EverQuestLoadedCreatureEquippedVisualItems& visualItems = VisualEquippedItemsByCreatureGUID[creatureGUID];
    visualItems.MainhandItemID = mainhandItemID;
    visualItems.OffhandItemID = offhandItemID;
    visualItems.IsDualWielding = isDualWielding;
}

bool EverQuestMod::IsCreatureDualWielding(ObjectGuid creatureGUID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
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
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        if (CreaturesResolvingEQMeleeExtraAttacks.count(attackerGUID) > 0)
            return;
    }

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

    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        CreaturesResolvingEQMeleeExtraAttacks.insert(attackerGUID);
    }

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

    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        CreaturesResolvingEQMeleeExtraAttacks.erase(attackerGUID);
    }
}

void EverQuestMod::StoreCreatureRangedAttackState(Creature* creature, float minRange, float maxRange, int32 damageModPct)
{
    EverQuestCreatureRangedAttackState* state = creature->CustomData.GetDefault<EverQuestCreatureRangedAttackState>(EQ_CREATURE_CUSTOMDATA_RANGEDATTACK);
    state->MinRange = minRange;
    state->MaxRange = maxRange;
    state->DamageModPct = damageModPct;
    state->SwingTimerRemainingMS = 0; // Ready to fire as soon as a valid target is in range
}

void EverQuestMod::RemoveCreatureRangedAttackState(Creature* creature)
{
    creature->CustomData.Erase(EQ_CREATURE_CUSTOMDATA_RANGEDATTACK);
}

// Logic reference was TAKP's NPC::RangedAttack. Creature will always try to get in melee range, but shoot while going towards them
void EverQuestMod::UpdateCreatureRangedAttack(Creature* creature, uint32 diff)
{
    if (creature == nullptr)
        return;
    if (ConfigCombatSkillsRangedAttackEnabled == false || ConfigSystemRangedAttackSpellID == 0)
        return;

    EverQuestCreatureRangedAttackState* state = creature->CustomData.Get<EverQuestCreatureRangedAttackState>(EQ_CREATURE_CUSTOMDATA_RANGEDATTACK);
    if (state == nullptr)
        return;

    // Use creature swing for timing. Probably right.
    if (state->SwingTimerRemainingMS > diff)
    {
        state->SwingTimerRemainingMS -= diff;
        return;
    }
    state->SwingTimerRemainingMS = 0;

    // Don't shoot if crowd controlled or casting
    if (creature->IsAlive() == false || creature->IsInCombat() == false)
        return;
    if (creature->HasUnitState(UNIT_STATE_CASTING) || creature->IsNonMeleeSpellCast(false))
        return;
    if (creature->HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_FLEEING | UNIT_STATE_CONFUSED))
        return;

    Unit* victim = creature->GetVictim();
    if (victim == nullptr || victim->IsAlive() == false)
        return;
    if (creature->IsValidAttackTarget(victim) == false)
        return;

    // Only shoot a target that is out of melee reach but inside the ranged band, with line of sight
    if (creature->IsWithinMeleeRange(victim) == true)
        return;
    float minRange = state->MinRange > 0.0f ? state->MinRange : ConfigCombatSkillsRangedAttackDefaultMinRange * EverQuest->ConfigWorldScale;
    float maxRange = state->MaxRange > 0.0f ? state->MaxRange : ConfigCombatSkillsRangedAttackDefaultMaxRange * EverQuest->ConfigWorldScale;
    float distance = creature->GetExactDist2d(victim);
    if (distance < minRange || distance > maxRange)
        return;
    if (creature->IsWithinLOSInMap(victim) == false)
        return;

    // Damage from the creature's own (melee) weapon output, scaled by the archery multiplier and the special ability modifier
    int32 damage = (int32)creature->CalculateDamage(BASE_ATTACK, false, true);
    damage = (int32)(damage * ConfigCombatSkillsRangedAttackDamageMultiplier);
    damage += damage * state->DamageModPct / 100;
    if (damage < 1)
        damage = 1;

    creature->CastCustomSpell(ConfigSystemRangedAttackSpellID, SPELLVALUE_BASE_POINT0, damage, victim, false);

    // Avoid machine gun type events. The cast above can despawn creatures (and erase state) through scripted
    // side effects, so look the state up fresh instead of writing through the earlier pointer
    uint32 swingTime = creature->GetAttackTime(BASE_ATTACK);
    if (swingTime < 1000)
        swingTime = 1000;
    EverQuestCreatureRangedAttackState* stateAfterCast = creature->CustomData.Get<EverQuestCreatureRangedAttackState>(EQ_CREATURE_CUSTOMDATA_RANGEDATTACK);
    if (stateAfterCast != nullptr)
        stateAfterCast->SwingTimerRemainingMS = swingTime;
}

void EverQuestMod::RemoveCreatureUnstickState(Creature* creature)
{
    creature->CustomData.Erase(EQ_CREATURE_CUSTOMDATA_UNSTICK);
}

void EverQuestMod::CalculateUnstickTeleportPosition(Creature* creature, Unit* victim, float& xOut, float& yOut, float& zOut)
{
    float creatureX = creature->GetPositionX();
    float creatureY = creature->GetPositionY();
    float creatureZ = creature->GetPositionZ();
    float victimX = victim->GetPositionX();
    float victimY = victim->GetPositionY();
    float victimZ = victim->GetPositionZ();

    // Fall back to the victim's exact position if no stepped candidate qualifies
    xOut = victimX;
    yOut = victimY;
    zOut = victimZ;

    float stepFraction = (float)ConfigEvadeUnstickStepPercent / 100.0f;
    if (stepFraction >= 1.0f)
        return;

    // Lift floor probes slightly so a spot exactly at floor level isn't borderline-rejected
    float floorTestZLift = 0.5f;

    // A floor must be within this distance below a probe to count as floor at that spot
    float floorSearchDistance = 10.0f;

    bool victimIsAbove = victimZ > (creatureZ + floorTestZLift);
    for (float fraction = stepFraction; fraction < 0.999f; fraction += stepFraction)
    {
        float candidateX = creatureX + ((victimX - creatureX) * fraction);
        float candidateY = creatureY + ((victimY - creatureY) * fraction);
        float candidateZ = creatureZ + ((victimZ - creatureZ) * fraction);

        // Level or downward teleports skip floor validation, so the first step is always taken
        if (victimIsAbove == false)
        {
            xOut = candidateX;
            yOut = candidateY;
            zOut = candidateZ;
            return;
        }

        // Prefer a floor at the candidate's own height, otherwise accept one at the player's height
        float floorZ = creature->GetMapHeight(candidateX, candidateY, candidateZ + floorTestZLift, true, floorSearchDistance);
        if (floorZ > INVALID_HEIGHT)
        {
            xOut = candidateX;
            yOut = candidateY;
            zOut = floorZ;
            return;
        }
        floorZ = creature->GetMapHeight(candidateX, candidateY, victimZ + floorTestZLift, true, floorSearchDistance);
        if (floorZ > INVALID_HEIGHT)
        {
            xOut = candidateX;
            yOut = candidateY;
            zOut = floorZ;
            return;
        }
    }
}

// Added this unstuck logic due to pathing errors in converted EQ content
void EverQuestMod::UpdateCreatureUnstick(Creature* creature, uint32 diff)
{
    if (creature == nullptr)
        return;
    if (ConfigEvadeEnabled == false)
        return;

    // Only do this special unstuck for EQ zones
    uint32 mapID = creature->GetMap()->GetId();
    if (mapID < ConfigSystemMapDBCIDMin || mapID > ConfigSystemMapDBCIDMax)
        return;

    // Only for living in-combat creatures should we do anything
    bool eligible = creature->IsAlive() == true && creature->IsInCombat() == true &&
        creature->IsPet() == false && creature->IsControlledByPlayer() == false;
    Unit* victim = creature->GetVictim();
    if (victim == nullptr || victim->IsAlive() == false)
        eligible = false;

    if (eligible == false)
    {
        EverQuestCreatureUnstickState* existingState = creature->CustomData.Get<EverQuestCreatureUnstickState>(EQ_CREATURE_CUSTOMDATA_UNSTICK);
        if (existingState != nullptr)
        {
            bool wasSettling = existingState->SettleRemainingMS > 0;
            creature->CustomData.Erase(EQ_CREATURE_CUSTOMDATA_UNSTICK);
            if (wasSettling == true)
                creature->ClearUnitState(UNIT_STATE_NO_COMBAT_MOVEMENT);
        }
        return;
    }

    EverQuestCreatureUnstickState* state = creature->CustomData.GetDefault<EverQuestCreatureUnstickState>(EQ_CREATURE_CUSTOMDATA_UNSTICK);
    // Take over 'cannot reach' to avoid early evades
    if (creature->CanNotReachTarget() == true)
        creature->SetCannotReachTarget();

    // Settle window after a teleport.  Stop chasing and delay next swing, but do allow spellcast
    if (state->SettleRemainingMS > 0)
    {
        if (state->SettleRemainingMS > diff)
            state->SettleRemainingMS -= diff;
        else
            state->SettleRemainingMS = 0;

        creature->AddUnitState(UNIT_STATE_NO_COMBAT_MOVEMENT);
        creature->StopMoving();
        creature->setAttackTimer(BASE_ATTACK, (int32)state->SettleRemainingMS);
        creature->setAttackTimer(OFF_ATTACK, (int32)state->SettleRemainingMS);

        if (state->SettleRemainingMS == 0)
        {
            creature->ClearUnitState(UNIT_STATE_NO_COMBAT_MOVEMENT);
            state->HasAnchor = false;
            state->StuckTimerMS = 0;
        }
        return;
    }

    // Clear if target is reached
    if (creature->IsWithinMeleeRange(victim) == true)
    {
        state->StuckTimerMS = 0;
        state->HasAnchor = false;
        return;
    }

    // If a creature is casting or movement-impaired, then it's not really stuck
    bool casting = creature->HasUnitState(UNIT_STATE_CASTING) || creature->IsNonMeleeSpellCast(false) ||
        creature->IsMovementPreventedByCasting();
    bool impaired = creature->HasUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED |
        UNIT_STATE_FLEEING | UNIT_STATE_DISTRACTED) || creature->HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED);
    if (casting == true || impaired == true)
    {
        state->StuckTimerMS = 0;
        state->HasAnchor = false;
        return;
    }

    // Genuinely stuck creatures don't actually move
    float currentX = creature->GetPositionX();
    float currentY = creature->GetPositionY();
    if (state->HasAnchor == false ||
        creature->GetExactDist2d(state->AnchorX, state->AnchorY) > ConfigEvadeUnstickMoveThreshold)
    {
        state->StuckTimerMS = 0;
        state->AnchorX = currentX;
        state->AnchorY = currentY;
        state->HasAnchor = true;
        return;
    }

    // If we got here, it's actually stuck
    state->StuckTimerMS += diff;
    uint32 stallThresholdMS = (uint32)(ConfigEvadeUnstickStallSeconds * 1000.0f);
    if (state->StuckTimerMS >= stallThresholdMS && state->TeleportAttemptsUsed < ConfigEvadeUnstickMaxAttempts)
    {
        // Teleport partway towards the player and pause action in an attempt to unstick
        float teleportX;
        float teleportY;
        float teleportZ;
        CalculateUnstickTeleportPosition(creature, victim, teleportX, teleportY, teleportZ);
        creature->NearTeleportTo(teleportX, teleportY, teleportZ, creature->GetAngle(victim));
        state->TeleportAttemptsUsed += 1;
        state->StuckTimerMS = 0;
        state->HasAnchor = false;
        state->SettleRemainingMS = (uint32)(ConfigEvadeUnstickSettleSeconds * 1000.0f);
        creature->AddUnitState(UNIT_STATE_NO_COMBAT_MOVEMENT);
        creature->StopMoving();
        creature->setAttackTimer(BASE_ATTACK, (int32)state->SettleRemainingMS);
        creature->setAttackTimer(OFF_ATTACK, (int32)state->SettleRemainingMS);
        return;
    }

    // Just go into evade if all teleport attempts are exausted
    uint32 evadeThresholdMS = (uint32)(ConfigEvadeUnreachableSeconds * 1000.0f);
    if (state->StuckTimerMS >= evadeThresholdMS)
    {
        if (creature->AI() != nullptr)
            creature->AI()->EnterEvadeMode(CreatureAI::EVADE_REASON_NO_PATH);
        RemoveCreatureUnstickState(creature);
    }
}

bool EverQuestMod::TryGetCustomSocialAggroScale(Creature* creature, float& scaleOut)
{
    if (creature == nullptr)
        return false;
    if (HasCreatureDataForCreatureTemplateID(creature->GetEntry()) == false)
        return false;
    float scale = GetCreatureDataForCreatureTemplateID(creature->GetEntry()).AgroSocialDistanceMod;
    if (std::fabs(scale - 1.0f) <= 0.0001f) // Should probably use epsilon...
        return false;
    scaleOut = scale;
    return true;
}

// Kinda-sorta a mirror of Creature:CallAssistance, but need to override to make custom social behavior
void EverQuestMod::DoScaledSocialAggroSearch(Creature* caller, Unit* victim, float scale)
{
    if (caller == nullptr || victim == nullptr)
        return;

    float radius = sWorld->getFloatConfig(CONFIG_CREATURE_FAMILY_ASSISTANCE_RADIUS) * scale;
    if (radius <= 0.0f)
        return;

    std::list<Creature*> assistList;
    Acore::AnyAssistCreatureInRangeCheck check(caller, victim, radius);
    Acore::CreatureListSearcher<Acore::AnyAssistCreatureInRangeCheck> searcher(caller, assistList, check);
    Cell::VisitObjects(caller, searcher, radius);

    for (Creature* assistant : assistList)
    {
        // Suppress immediate call, link leash timers
        assistant->SetNoCallAssistance(true);
        assistant->EngageWithTarget(victim);
        if (assistant->IsEngaged() == true)
            assistant->SetLastLeashExtensionTimePtr(caller->GetLastLeashExtensionTimePtr());
    }
}

void EverQuestMod::ApplyScaledCreatureSocialAggroOnEngage(Creature* creature, Unit* victim)
{
    // Only apply if a creature has a custom setting
    float scale = 1.0f;
    if (TryGetCustomSocialAggroScale(creature, scale) == false)
        return;

    creature->SetNoCallAssistance(true);
    DoScaledSocialAggroSearch(creature, victim, scale);
}

void EverQuestMod::RemoveCreatureSocialAggroState(Creature* creature)
{
    creature->CustomData.Erase(EQ_CREATURE_CUSTOMDATA_SOCIALAGGRO);
}

void EverQuestMod::UpdateCreatureScaledSocialAggro(Creature* creature, uint32 diff)
{
    if (creature == nullptr)
        return;

    float scale = 1.0f;
    bool eligible = TryGetCustomSocialAggroScale(creature, scale) == true &&
        creature->IsAlive() == true && creature->IsInCombat() == true &&
        creature->IsPet() == false && creature->IsControlledByPlayer() == false;
    Unit* victim = creature->GetVictim();
    if (victim == nullptr || victim->IsAlive() == false)
        eligible = false;

    if (eligible == false)
    {
        RemoveCreatureSocialAggroState(creature);
        return;
    }

    // Stop the engine's full-radius periodic re-call (its block is skipped when the timer is zero)
    creature->SetAssistanceTimer(0);

    // Honor the disabled-periodic config (0) the same way the core does
    uint32 periodMS = sWorld->getIntConfig(CONFIG_CREATURE_FAMILY_ASSISTANCE_PERIOD);
    if (periodMS == 0)
    {
        RemoveCreatureSocialAggroState(creature);
        return;
    }

    EverQuestCreatureSocialAggroState* state = creature->CustomData.GetDefault<EverQuestCreatureSocialAggroState>(EQ_CREATURE_CUSTOMDATA_SOCIALAGGRO);
    if (state->RecallTimerMS <= diff)
    {
        DoScaledSocialAggroSearch(creature, victim, scale);
        state->RecallTimerMS = periodMS;
    }
    else
        state->RecallTimerMS -= diff;
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
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    VisualEquippedItemsByCreatureGUID.erase(creatureGUID);
}

void EverQuestMod::RemoveVisualEquippedItemForCreatureGUIDIfExists(Map* map, ObjectGuid creatureGUID, uint32 itemTemplateID)
{
    EverQuestLoadedCreatureEquippedVisualItems* visualItems = nullptr;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto visualItemsIt = VisualEquippedItemsByCreatureGUID.find(creatureGUID);
        if (visualItemsIt == VisualEquippedItemsByCreatureGUID.end())
            return;
        visualItems = &visualItemsIt->second;
    }

    Creature* creature = map->GetCreature(creatureGUID);
    if (!creature)
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod::RemoveVisualEquippedItemForCreatureGUIDIfExists failure, as creature with GUID could not be found in the map");
        return;
    }

    uint32 npcItemTemplateID = EverQuest->GetNPCEquipItemTemplateIDForItemTemplate(itemTemplateID);

    // Mainhand first, then offhand
    if (visualItems->MainhandItemID == npcItemTemplateID)
    {
        creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 0, 0);
        visualItems->MainhandItemID = 0;
    }
    else if (visualItems->OffhandItemID == npcItemTemplateID)
    {
        creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, 0);
        visualItems->OffhandItemID = 0;
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

const vector<EverQuestCreatureWaypoint>& EverQuestMod::GetWaypoints(uint32 mapID, uint32 waypointListID)
{
    static const vector<EverQuestCreatureWaypoint> returnEmpty;
    auto outerIt = CreatureWaypointsByMapIDAndWaypointID.find(mapID);
    if (outerIt == CreatureWaypointsByMapIDAndWaypointID.end())
        return returnEmpty;
    const unordered_map<uint32, vector<EverQuestCreatureWaypoint>>& innerMap = outerIt->second;
    auto innerIt = innerMap.find(waypointListID);
    if (innerIt == innerMap.end())
        return returnEmpty;

    return innerIt->second;
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

void EverQuestMod::LoadZoneSafePointData()
{
    ZoneSafePointByMapID.clear();

    QueryResult queryResult = WorldDatabase.Query("SELECT MapID, X, Y, Z, Orientation FROM mod_everquest_zone_safe_point;");
    if (queryResult)
    {
        do
        {
            Field* fields = queryResult->Fetch();
            EverQuestZoneSafePoint zoneSafePoint;
            zoneSafePoint.MapID = fields[0].Get<uint32>();
            zoneSafePoint.X = fields[1].Get<float>();
            zoneSafePoint.Y = fields[2].Get<float>();
            zoneSafePoint.Z = fields[3].Get<float>();
            zoneSafePoint.Orientation = fields[4].Get<float>();
            ZoneSafePointByMapID[zoneSafePoint.MapID] = zoneSafePoint;
        } while (queryResult->NextRow());
    }
}

void EverQuestMod::SendPlayerToZoneSafePoint(Player* player, bool includeGroup)
{
    // In-zone succor sends to the safe point of the zone the caster is currently in
    uint32 mapID = player->GetMapId();
    if (ZoneSafePointByMapID.find(mapID) == ZoneSafePointByMapID.end())
    {
        ChatHandler(player->GetSession()).PSendSysMessage("There is no safe location in this zone. Spell failed.");
        return;
    }

    const EverQuestZoneSafePoint& zoneSafePoint = ZoneSafePointByMapID[mapID];
    player->TeleportTo({ mapID, {zoneSafePoint.X, zoneSafePoint.Y, zoneSafePoint.Z, zoneSafePoint.Orientation} });

    // Party-target succor also pulls the caster's living group members that share the same zone to the safe point
    if (includeGroup == true)
    {
        Group* group = player->GetGroup();
        if (group != nullptr)
        {
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* member = itr->GetSource();
                if (member == nullptr || member == player)
                    continue;
                if (member->IsAlive() == false || member->IsInWorld() == false)
                    continue;
                if (member->GetMapId() != mapID)
                    continue;
                member->TeleportTo({ mapID, {zoneSafePoint.X, zoneSafePoint.Y, zoneSafePoint.Z, zoneSafePoint.Orientation} });
            }
        }
    }
}

void EverQuestMod::LoadClassMapData()
{
    ClassMapByWOWClassID.clear();

    QueryResult queryResult = WorldDatabase.Query("SELECT wowclass, eqclass_base, eqclass_defaultsecond, eqclass_eligiblesecond_mask FROM mod_everquest_classmap;");
    if (queryResult)
    {
        do
        {
            Field* fields = queryResult->Fetch();
            EverQuestClassMap classMap;
            classMap.WOWClassID = fields[0].Get<uint8>();
            classMap.EQClassIDBase = fields[1].Get<uint8>();
            classMap.EQClassIDDefaultSecond = fields[2].Get<uint8>();
            classMap.EQClassIDEligibleSecondMask = fields[3].Get<uint32>();
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
        static const EverQuestClassMap returnEmpty{};
        LOG_ERROR("module.EverQuest", "EverQuestMod::GetClassMapForWOWClassID failure, wowClassID {} could not be found", wowClassID);
        return returnEmpty;
    }
}

bool EverQuestMod::IsEQClassABaseEQClass(uint8 eqClassID)
{
    for (auto& classMapPair : ClassMapByWOWClassID)
        if (classMapPair.second.EQClassIDBase == eqClassID)
            return true;
    return false;
}

void EverQuestMod::StorePositionAsLastGate(Player* player)
{
    // Fail if there is no map, or if the map is invalid
    if (player->GetMap() == nullptr)
        return;

    // Gather the new gate reference
    float playerX = player->GetPosition().GetPositionX();
    float playerY = player->GetPosition().GetPositionY();
    float playerZ = player->GetPosition().GetPositionZ();
    float playerOrientation = player->GetOrientation();
    int mapID = player->GetMap()->GetId();
    int zoneID = player->GetAreaId();
    uint32 guidCounter = player->GetGUID().GetCounter();

    // Upsert only the last-gate columns so the class-controller and home-bind data sharing this row is preserved
    CharacterDatabase.Execute("INSERT INTO `mod_everquest_character_settings` (`guid`, `lastgateMapId`, `lastgateZoneId`, `lastgatePosX`, `lastgatePosY`, `lastgatePosZ`, `lastgateOrientation`) VALUES ({}, {}, {}, {}, {}, {}, {}) "
        "ON DUPLICATE KEY UPDATE `lastgateMapId` = {}, `lastgateZoneId` = {}, `lastgatePosX` = {}, `lastgatePosY` = {}, `lastgatePosZ` = {}, `lastgateOrientation` = {}",
        guidCounter, mapID, zoneID, playerX, playerY, playerZ, playerOrientation,
        mapID, zoneID, playerX, playerY, playerZ, playerOrientation);
}

void EverQuestMod::SendPlayerToLastGate(Player* player)
{
    // Fail if in combat
    if (player->IsInCombat() == true)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("Your gate tether broke due to being in combat!");
        return;
    }

    // Pull the last gate position
    QueryResult queryResult = CharacterDatabase.Query("SELECT lastgateMapId, lastgateZoneId, lastgatePosX, lastgatePosY, lastgatePosZ, lastgateOrientation FROM mod_everquest_character_settings WHERE guid = {} AND lastgateMapId IS NOT NULL", player->GetGUID().GetCounter());
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
    float orientation = fields[5].Get<float>();

    // Teleport the player
    player->TeleportTo({ mapId, {posX, posY, posZ, orientation} });
}

void EverQuestMod::SendPlayerToEQBindHome(Player* player)
{
    // Pull the bind position
    QueryResult queryResult = CharacterDatabase.Query("SELECT homebindMapId, homebindZoneId, homebindPosX, homebindPosY, homebindPosZ FROM mod_everquest_character_settings WHERE guid = {} AND homebindMapId IS NOT NULL", player->GetGUID().GetCounter());
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
    // Upsert only the home-bind columns so the class-controller and last-gate data sharing this row is preserved
    CharacterDatabase.Execute("INSERT INTO `mod_everquest_character_settings` (`guid`, `homebindMapId`, `homebindZoneId`, `homebindPosX`, `homebindPosY`, `homebindPosZ`) VALUES ({}, {}, {}, {}, {}, {}) "
        "ON DUPLICATE KEY UPDATE `homebindMapId` = {}, `homebindZoneId` = {}, `homebindPosX` = {}, `homebindPosY` = {}, `homebindPosZ` = {}",
        playerGUIDCounter, mapID, zoneID, playerX, playerY, playerZ,
        mapID, zoneID, playerX, playerY, playerZ);

    // Send a message to the player
    ChatHandler(player->GetSession()).PSendSysMessage("You feel yourself bind to the area.");
}

void EverQuestMod::DeletePlayerBindHome(ObjectGuid guid)
{
    // Clear only the home-bind and last-gate columns; the class-controller data sharing this row is left intact
    CharacterDatabase.Execute("UPDATE `mod_everquest_character_settings` SET "
        "`homebindMapId` = NULL, `homebindZoneId` = NULL, `homebindPosX` = NULL, `homebindPosY` = NULL, `homebindPosZ` = NULL, "
        "`lastgateMapId` = NULL, `lastgateZoneId` = NULL, `lastgatePosX` = NULL, `lastgatePosY` = NULL, `lastgatePosZ` = NULL, `lastgateOrientation` = NULL "
        "WHERE guid = {}", guid.GetCounter());
}

void EverQuestMod::AddCreatureAsLoaded(int mapID, Creature* creature)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    AllLoadedCreaturesByMapIDThenCreatureEntryID[mapID][creature->GetEntry()].push_back(creature);

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
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto entryMapIt = AllLoadedCreaturesByMapIDThenCreatureEntryID.find(mapID);
    if (entryMapIt != AllLoadedCreaturesByMapIDThenCreatureEntryID.end())
    {
        unordered_map<int, vector<Creature*>>& innerMap = entryMapIt->second;

        // The entry can change while the creature is tracked (Creature::UpdateEntry), so if it's not under its
        // current entry then scan the other buckets, otherwise a dangling pointer is left behind
        auto bucketIt = innerMap.find((int)creature->GetEntry());
        if (bucketIt == innerMap.end() || find(bucketIt->second.begin(), bucketIt->second.end(), creature) == bucketIt->second.end())
        {
            bucketIt = innerMap.end();
            for (auto candidateIt = innerMap.begin(); candidateIt != innerMap.end(); ++candidateIt)
            {
                if (find(candidateIt->second.begin(), candidateIt->second.end(), creature) != candidateIt->second.end())
                {
                    bucketIt = candidateIt;
                    break;
                }
            }
        }

        if (bucketIt != innerMap.end())
        {
            vector<Creature*>& creatureVector = bucketIt->second;
            creatureVector.erase(find(creatureVector.begin(), creatureVector.end(), creature));
            if (creatureVector.empty())
            {
                innerMap.erase(bucketIt);
                if (innerMap.empty())
                    AllLoadedCreaturesByMapIDThenCreatureEntryID.erase(entryMapIt);
            }
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

    PreloadedLootItemIDsByCreatureGUID.erase(creature->GetGUID());
    PreloadedLootCountsByCreatureGUID.erase(creature->GetGUID());
    VisualEquippedItemsByCreatureGUID.erase(creature->GetGUID());
}

vector<Creature*> EverQuestMod::GetLoadedCreaturesWithEntryID(int mapID, uint32 entryID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto entryMapIt = AllLoadedCreaturesByMapIDThenCreatureEntryID.find(mapID);
    if (entryMapIt == AllLoadedCreaturesByMapIDThenCreatureEntryID.end())
        return vector<Creature*>();
    auto bucketIt = entryMapIt->second.find(entryID);
    if (bucketIt == entryMapIt->second.end())
        return vector<Creature*>();
    return bucketIt->second;
}

void EverQuestMod::RollLootItemsForCreature(ObjectGuid creatureGUID, uint32 creatureTemplateEntryID)
{
    // Clear previous rolls (and empty counts map means it drops nothing). The values are only touched by this
    // creature's own map thread after this, so only the lookups need the lock
    vector<uint32>* preloadedItemIDs = nullptr;
    unordered_map<uint32, uint32>* counts = nullptr;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        preloadedItemIDs = &PreloadedLootItemIDsByCreatureGUID[creatureGUID];
        counts = &PreloadedLootCountsByCreatureGUID[creatureGUID];
    }
    preloadedItemIDs->clear();
    counts->clear();

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

            // The first GroupMultiplierMin iterations are guaranteed and skip the probability roll (EQ loottable_entries.multiplier_min)
            if (t >= lootGroup.GroupMultiplierMin && lootGroup.GroupProbability < 100.0f && float(rand_chance()) > lootGroup.GroupProbability)
                continue;

            RollLootGroupIntoCounts(lootGroup, *counts);
        }
    }

    // Track preloaded items for visuals and OnItemRoll checks
    for (const auto& itemCount : *counts)
        preloadedItemIDs->push_back(itemCount.first);
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
        // Keep rolling while below MinDrop or a guaranteed item exists, otherwise stop with probability noLootProb
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

    // Remove only the database rows so the spawn doesn't persist across restarts. Creature::DeleteFromDB would also
    // erase the in-memory CreatureData that the live creature still holds a pointer to (a use-after-free later)
    WorldDatabase.Execute("DELETE FROM `creature_addon` WHERE `guid` = {}", spawnId);
    WorldDatabase.Execute("DELETE FROM `creature` WHERE `guid` = {}", spawnId);
}

void EverQuestMod::DespawnCreature(uint32 entryID, Map* map)
{
    vector<Creature*> loadedCreatures = GetLoadedCreaturesWithEntryID(map->GetId(), entryID);
    for (Creature* creature : loadedCreatures)
        if (creature != nullptr)
        {
            DoCreatureEmoteEvent(creature, EQ_CREATURE_EMOTE_EVENT_ONDESPAWN, nullptr);
            creature->DespawnOrUnsummon(0ms);
        }
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
            bool focusMatchesSong = false;
            switch (curSpell.FocusBoostType)
            {
                case EQ_SPELLFOCUSBOOSTTYPE_BARDBRASS:
                    focusMatchesSong = (auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSBRASS || auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSALL);
                    break;
                case EQ_SPELLFOCUSBOOSTTYPE_BARDSTRINGED:
                    focusMatchesSong = (auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSSTRING || auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSALL);
                    break;
                case EQ_SPELLFOCUSBOOSTTYPE_BARDWIND:
                    focusMatchesSong = (auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSWIND || auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSALL);
                    break;
                case EQ_SPELLFOCUSBOOSTTYPE_BARDPERCUSSION:
                    focusMatchesSong = (auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSPERCUSSION || auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSALL);
                    break;
                case EQ_SPELLFOCUSBOOSTTYPE_BARDSINGING:
                    focusMatchesSong = (auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSALL);
                    break;
                default:
                    break;
            }

            // Keep the best (highest) matching instrument
            if (focusMatchesSong && static_cast<uint32>(auraInfo->Effects[effIndex].MiscValueB) > boostValue)
                boostValue = auraInfo->Effects[effIndex].MiscValueB;
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

// Returns the cached controller data for the player, loading it from the database first if needed. The returned
// pointer stays valid after the lock releases (unordered_map nodes are stable), and the value is only touched by
// the owning player's thread
EverQuestPlayerControllerData* EverQuestMod::GetOrLoadActivePlayerClassControllerData(Player* player)
{
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto controllerDataIt = ActivePlayerClassControllerDataByGUID.find(player->GetGUID());
        if (controllerDataIt != ActivePlayerClassControllerDataByGUID.end())
            return &controllerDataIt->second;
    }

    // Load outside the lock, since this queries the database
    EverQuestPlayerControllerData loadedControllerData = GetPlayerControllerData(player);

    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    return &ActivePlayerClassControllerDataByGUID.emplace(player->GetGUID(), loadedControllerData).first->second;
}

uint8 EverQuestMod::GetCurrentSecondEQClassForPlayer(Player* player)
{
    return GetOrLoadActivePlayerClassControllerData(player)->CurrentSecondClass;
}

uint8 EverQuestMod::GetNextSecondEQClassForPlayer(Player* player)
{
    return GetOrLoadActivePlayerClassControllerData(player)->NextSecondClass;
}

void EverQuestMod::SetNextSecondEQClassForPlayer(Player* player, uint8 nextEQClass)
{
    GetOrLoadActivePlayerClassControllerData(player)->NextSecondClass = nextEQClass;
}

void EverQuestMod::SetInitialEQClassesForPlayer(Player* player)
{
    const EverQuestClassMap classMap = GetClassMapForWOWClassID(player->getClass());
    EverQuestPlayerControllerData controllerData;
    controllerData.GUID = player->GetGUID().GetCounter();
    controllerData.CurrentSecondClass = classMap.EQClassIDDefaultSecond;
    controllerData.NextSecondClass = classMap.EQClassIDDefaultSecond;
    controllerData.SecondaryExpPool = 0;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        ActivePlayerClassControllerDataByGUID[player->GetGUID()] = controllerData;
    }

    // Persist the controller columns immediately, without disturbing any home-bind / last-gate data already in this row
    CharacterDatabase.Execute("INSERT INTO `mod_everquest_character_settings` (`guid`, `nextSecondaryClass`, `currentSecondaryClass`, `secondaryExpPool`) VALUES ({}, {}, {}, {}) "
        "ON DUPLICATE KEY UPDATE `nextSecondaryClass` = {}, `currentSecondaryClass` = {}, `secondaryExpPool` = {}",
        controllerData.GUID,
        controllerData.NextSecondClass,
        controllerData.CurrentSecondClass,
        controllerData.SecondaryExpPool,
        controllerData.NextSecondClass,
        controllerData.CurrentSecondClass,
        controllerData.SecondaryExpPool);
}

void EverQuestMod::SetInitialCreatePositionForPlayer(Player* player)
{
    if (HasCreatePlayerData(player->getRace(), player->getClass()) == false)
        return;

    const EverQuestPlayerCreateInfo& createInfo = GetPlayerCreateInfo(player->getRace(), player->getClass());
    CharacterDatabase.Execute("UPDATE `characters` SET `map` = {}, `zone` = {}, `position_x` = {}, `position_y` = {}, `position_z` = {}, `orientation` = {} WHERE `guid` = {}",
        createInfo.MapID, createInfo.ZoneID, createInfo.PositionX, createInfo.PositionY, createInfo.PositionZ, createInfo.Orientation, player->GetGUID().GetCounter());
}

EverQuestPlayerControllerData EverQuestMod::GetPlayerControllerData(Player* player)
{
    EverQuestPlayerControllerData controllerData;
    controllerData.GUID = player->GetGUID().GetCounter();
    QueryResult queryResult = CharacterDatabase.Query("SELECT nextSecondaryClass, currentSecondaryClass, secondaryExpPool, illusionFaceId FROM mod_everquest_character_settings WHERE guid = {}", player->GetGUID().GetCounter());
    if (!queryResult || queryResult->GetRowCount() == 0)
    {
        const EverQuestClassMap classMap = GetClassMapForWOWClassID(player->getClass());
        controllerData.CurrentSecondClass = classMap.EQClassIDDefaultSecond;
        controllerData.NextSecondClass = classMap.EQClassIDDefaultSecond;
        controllerData.SecondaryExpPool = 0;
        controllerData.IllusionFaceID = 0;
    }
    else
    {
        Field* fields = queryResult->Fetch();
        controllerData.NextSecondClass = fields[0].Get<uint8>();
        controllerData.CurrentSecondClass = fields[1].Get<uint8>();
        controllerData.SecondaryExpPool = fields[2].Get<uint32>();
        controllerData.IllusionFaceID = (uint32)std::max(0, fields[3].Get<int32>());
    }
    return controllerData;
}

uint32 EverQuestMod::GetSecondaryExpPoolForPlayer(Player* player)
{
    return GetOrLoadActivePlayerClassControllerData(player)->SecondaryExpPool;
}

uint32 EverQuestMod::AddToSecondaryExpPoolForPlayer(Player* player, uint32 grantedExp)
{
    if (ConfigSecondaryExpPoolGainPercent <= 0.0f)
        return 0;

    EverQuestPlayerControllerData& controllerData = *GetOrLoadActivePlayerClassControllerData(player);

    uint32 gain = static_cast<uint32>(static_cast<float>(grantedExp) * (ConfigSecondaryExpPoolGainPercent * 0.01f));
    if (gain == 0)
        return 0;

    // Clamp to the configured maximum pool size
    if (controllerData.SecondaryExpPool >= ConfigSecondaryExpPoolMaxPooled)
        return 0;
    uint32 room = ConfigSecondaryExpPoolMaxPooled - controllerData.SecondaryExpPool;
    if (gain > room)
        gain = room;

    controllerData.SecondaryExpPool += gain;
    SaveSecondaryExpPoolForPlayer(player);
    return gain;
}

uint32 EverQuestMod::SpendSecondaryExpPoolForPlayer(Player* player)
{
    EverQuestPlayerControllerData& controllerData = *GetOrLoadActivePlayerClassControllerData(player);

    if (controllerData.SecondaryExpPool == 0)
        return 0;

    // Player::GiveXP silently drops the grant for a dead player, which would burn the pool for nothing
    if (player->IsAlive() == false)
        return 0;

    // Nothing to fill at (or frozen short of) the level cap
    if (player->GetLevel() >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        return 0;
    if (player->HasPlayerFlag(PLAYER_FLAGS_NO_XP_GAIN))
        return 0;

    uint32 curXP = player->GetUInt32Value(PLAYER_XP);
    uint32 nextLevelXP = player->GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
    if (nextLevelXP <= curXP)
        return 0;
    uint32 needed = nextLevelXP - curXP;

    // At the level cap the experience bar parks one point short of leveling, so never add that last point
    if (ConfigPlayerLevelCap != 0 && static_cast<uint32>(player->GetLevel()) + 1 >= ConfigPlayerLevelCap)
    {
        if (needed <= 1)
            return 0;
        needed -= 1;
    }

    uint32 spend = (controllerData.SecondaryExpPool < needed) ? controllerData.SecondaryExpPool : needed;
    if (spend == 0)
        return 0;

    controllerData.SecondaryExpPool -= spend;
    SaveSecondaryExpPoolForPlayer(player);

    player->GiveXP(spend, nullptr);
    return spend;
}

void EverQuestMod::SaveSecondaryExpPoolForPlayer(Player* player)
{
    EverQuestPlayerControllerData controllerData;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto controllerDataIt = ActivePlayerClassControllerDataByGUID.find(player->GetGUID());
        if (controllerDataIt == ActivePlayerClassControllerDataByGUID.end())
            return;
        controllerData = controllerDataIt->second;
    }

    CharacterDatabase.Execute("INSERT INTO `mod_everquest_character_settings` (`guid`, `currentSecondaryClass`, `nextSecondaryClass`, `secondaryExpPool`) VALUES ({}, {}, {}, {}) ON DUPLICATE KEY UPDATE `secondaryExpPool` = {}",
        player->GetGUID().GetCounter(),
        controllerData.CurrentSecondClass,
        controllerData.NextSecondClass,
        controllerData.SecondaryExpPool,
        controllerData.SecondaryExpPool);
}

uint32 EverQuestMod::GetIllusionFaceIDForPlayer(Player* player)
{
    return GetOrLoadActivePlayerClassControllerData(player)->IllusionFaceID;
}

void EverQuestMod::SetIllusionFaceIDForPlayer(Player* player, uint32 faceID)
{
    GetOrLoadActivePlayerClassControllerData(player)->IllusionFaceID = faceID;
    SaveIllusionFaceIDForPlayer(player);
}

void EverQuestMod::SaveIllusionFaceIDForPlayer(Player* player)
{
    EverQuestPlayerControllerData controllerData;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto controllerDataIt = ActivePlayerClassControllerDataByGUID.find(player->GetGUID());
        if (controllerDataIt == ActivePlayerClassControllerDataByGUID.end())
            return;
        controllerData = controllerDataIt->second;
    }

    CharacterDatabase.Execute("INSERT INTO `mod_everquest_character_settings` (`guid`, `currentSecondaryClass`, `nextSecondaryClass`, `secondaryExpPool`, `illusionFaceId`) VALUES ({}, {}, {}, {}, {}) ON DUPLICATE KEY UPDATE `illusionFaceId` = {}",
        player->GetGUID().GetCounter(),
        controllerData.CurrentSecondClass,
        controllerData.NextSecondClass,
        controllerData.SecondaryExpPool,
        controllerData.IllusionFaceID,
        controllerData.IllusionFaceID);
}

void EverQuestMod::HandleLevelCapOnBeforeExperienceGain(Player const* player, uint8& levelForExpGain)
{
    if (ConfigPlayerLevelCap == 0)
        return;

    // Track that this player is inside Player::GiveXP, so an experience-driven level up attempt can be told apart from a direct GiveLevel call (GM .levelup / .character level), which must stay uncapped
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        PlayersGainingExperience.insert(player->GetGUID());
    }

    // Once the bar is parked one point short of a capped level up, report max level so GiveXP discards the gain
    if (static_cast<uint32>(player->GetLevel()) + 1 >= ConfigPlayerLevelCap)
    {
        uint32 curExp = player->GetUInt32Value(PLAYER_XP);
        uint32 nextLevelExp = player->GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
        if (nextLevelExp > 0 && curExp >= nextLevelExp - 1)
            levelForExpGain = 255;
    }
}

bool EverQuestMod::HandleLevelCapOnCanGiveLevel(Player* player, uint8 newLevel)
{
    if (ConfigPlayerLevelCap == 0)
        return true;
    if (static_cast<uint32>(newLevel) < ConfigPlayerLevelCap)
        return true;

    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    if (PlayersGainingExperience.find(player->GetGUID()) == PlayersGainingExperience.end())
        return true;

    PlayersPendingLevelCapExperiencePark.insert(player->GetGUID());
    return false;
}

void EverQuestMod::ProcessLevelCapStateForPlayer(Player* player)
{
    if (ConfigPlayerLevelCap == 0)
        return;

    bool parkExperienceBar = false;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        PlayersGainingExperience.erase(player->GetGUID());
        parkExperienceBar = PlayersPendingLevelCapExperiencePark.erase(player->GetGUID()) > 0;
    }
    if (parkExperienceBar == false)
        return;

    if (static_cast<uint32>(player->GetLevel()) + 1 >= ConfigPlayerLevelCap)
    {
        uint32 nextLevelExp = player->GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
        if (nextLevelExp > 0)
            player->SetUInt32Value(PLAYER_XP, nextLevelExp - 1);
    }
}

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
    levelsByClass.insert(pair<uint8, uint8>(GetCurrentSecondEQClassForPlayer(player), player->GetLevel()));

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

void EverQuestMod::EnsureCrossClassExemptSpellIDsBuilt()
{
    if (CrossClassExemptSpellIDsBuilt == true)
        return;
    CrossClassExemptSpellIDsBuilt = true;

    // Cache every spell that is tied to a cross-class skill, so that they don't wipe on secondary class switch
    if (ConfigCrossClassIncludeSkillIDs.empty() == true)
        return;
    for (SkillLineAbilityEntry const* skillLineAbility : sSkillLineAbilityStore)
    {
        if (skillLineAbility == nullptr)
            continue;
        if (ConfigCrossClassIncludeSkillIDs.find(skillLineAbility->SkillLine) != ConfigCrossClassIncludeSkillIDs.end())
            CrossClassExemptSpellIDs.insert(skillLineAbility->Spell);
    }
}

bool EverQuestMod::IsSpellExemptFromClassMove(uint32 spellID)
{
    // Recipes / abilities mapped to a cross-class skill line via SkillLineAbility
    if (CrossClassExemptSpellIDs.find(spellID) != CrossClassExemptSpellIDs.end())
        return true;

    // Profession rank / development spells are not in SkillLineAbility, but grant a skill line through a skill effect
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellID);
    if (spellInfo != nullptr)
    {
        // Mounts are shared across all of a character's secondary classes, just like the riding skill they depend on
        if (spellInfo->HasAura(SPELL_AURA_MOUNTED) == true)
            return true;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->Effects[i].Effect != SPELL_EFFECT_SKILL_STEP && spellInfo->Effects[i].Effect != SPELL_EFFECT_SKILL)
                continue;
            if (ConfigCrossClassIncludeSkillIDs.find((uint32)spellInfo->Effects[i].MiscValue) != ConfigCrossClassIncludeSkillIDs.end())
                return true;
        }

        // Companion (vanity) pets are shared across all secondary classes, just like mounts
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->Effects[i].Effect != SPELL_EFFECT_SUMMON)
                continue;
            SummonPropertiesEntry const* summonProperties = sSummonPropertiesStore.LookupEntry(spellInfo->Effects[i].MiscValueB);
            if (summonProperties != nullptr && summonProperties->Type == SUMMON_TYPE_MINIPET)
                return true;
        }
    }
    return false;
}

void EverQuestMod::MoveClassSpellsToModSpellsTable(Player* player, CharacterDatabaseTransaction& transaction)
{
    uint8 curEQClass = GetCurrentSecondEQClassForPlayer(player);

    // Build (once) the set of profession/tradeskill-bound spells that should not migrate
    EnsureCrossClassExemptSpellIDsBuilt();

    // Purge old spell list in mod table
    transaction->Append("DELETE FROM `mod_everquest_character_class_spell` WHERE guid = {} and eqclass = {}", player->GetGUID().GetCounter(), curEQClass);

    // Move class spells (including EverQuest spells) from the character table into the mod table
    for (auto& curSpell : player->GetSpellMap())
    {
        if (IsSpellExemptFromClassMove(curSpell.first) == true)
            continue;

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

    // Build the list of spells that should remain on character_aura across a secondary class switch (like gate tether)
    string keptSpellsList = "";
    for (auto const& spellPair : SpellDataBySpellID)
    {
        if (spellPair.second.AuraStaysOnSecondaryClassSwitch == true)
        {
            if (keptSpellsList.empty() == false)
                keptSpellsList += ",";
            keptSpellsList += std::to_string(spellPair.second.SpellID);
        }
    }
    if (keptSpellsList.empty() == true)
        keptSpellsList = "0"; // No spell uses id 0, so NOT IN (0) keeps nothing

    transaction->Append("DELETE FROM `mod_everquest_character_class_aura` WHERE guid = {} and eqclass = {}", player->GetGUID().GetCounter(), curEQClass);
    transaction->Append("INSERT IGNORE INTO mod_everquest_character_class_aura (guid, class, eqclass, casterGuid, itemGuid, spell, effectMask, recalculateMask, stackCount, amount0, amount1, amount2, base_amount0, base_amount1, base_amount2, maxDuration, remainTime, remainCharges) SELECT guid, {}, {}, casterGuid, itemGuid, spell, effectMask, recalculateMask, stackCount, amount0, amount1, amount2, base_amount0, base_amount1, base_amount2, maxDuration, remainTime, remainCharges FROM character_aura WHERE guid = {} AND spell NOT IN ({})", player->getClass(), curEQClass, player->GetGUID().GetCounter(), keptSpellsList);
    transaction->Append("DELETE FROM `character_aura` WHERE guid = {} AND spell NOT IN ({})", player->GetGUID().GetCounter(), keptSpellsList);
}

void EverQuestMod::MoveEquipToModInventoryTable(Player* player, CharacterDatabaseTransaction& transaction)
{
    uint8 curEQClass = GetCurrentSecondEQClassForPlayer(player);

    transaction->Append("DELETE FROM `mod_everquest_character_class_inventory` WHERE guid = {} AND eqclass = {} AND `bag` = 0 AND `slot` <= 18;", player->GetGUID().GetCounter(), curEQClass);
    transaction->Append("INSERT IGNORE INTO `mod_everquest_character_class_inventory` (`guid`, `class`, `eqclass`, `bag`, `slot`, `item`) SELECT `guid`, {}, {}, `bag`, `slot`, `item` FROM character_inventory WHERE guid = {} AND `bag` = 0 AND `slot` <= 18", player->getClass(), curEQClass, player->GetGUID().GetCounter());
    transaction->Append("DELETE FROM `character_inventory` WHERE guid = {} AND `bag` = 0 AND `slot` <= 18", player->GetGUID().GetCounter());
}

void EverQuestMod::MoveQuestDataToModQuestTables(Player* player, CharacterDatabaseTransaction& transaction)
{
    uint8 curEQClass = GetCurrentSecondEQClassForPlayer(player);

    transaction->Append("DELETE FROM `mod_everquest_character_class_queststatus` WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), curEQClass);
    transaction->Append("INSERT IGNORE INTO `mod_everquest_character_class_queststatus` (`guid`, `eqclass`, `quest`, `status`, `explored`, `timer`, `mobcount1`, `mobcount2`, `mobcount3`, `mobcount4`, `itemcount1`, `itemcount2`, `itemcount3`, `itemcount4`, `itemcount5`, `itemcount6`, `playercount`) SELECT {}, {}, `quest`, `status`, `explored`, `timer`, `mobcount1`, `mobcount2`, `mobcount3`, `mobcount4`, `itemcount1`, `itemcount2`, `itemcount3`, `itemcount4`, `itemcount5`, `itemcount6`, `playercount` FROM character_queststatus WHERE guid = {}", player->GetGUID().GetCounter(), curEQClass, player->GetGUID().GetCounter());
    transaction->Append("DELETE FROM `character_queststatus` WHERE guid = {}", player->GetGUID().GetCounter());

    transaction->Append("DELETE FROM `mod_everquest_character_class_queststatus_rewarded` WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), curEQClass);
    transaction->Append("INSERT IGNORE INTO `mod_everquest_character_class_queststatus_rewarded` (`guid`, `eqclass`, `quest`, `active`) SELECT `guid`, {}, `quest`, `active` FROM character_queststatus_rewarded WHERE guid = {}", curEQClass, player->GetGUID().GetCounter());
    transaction->Append("DELETE FROM `character_queststatus_rewarded` WHERE guid = {}", player->GetGUID().GetCounter());
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

void EverQuestMod::CopyModQuestTablesIntoCharacterQuests(Player* player, uint8 pullEQClassID, CharacterDatabaseTransaction& transaction)
{
    transaction->Append("INSERT IGNORE INTO `character_queststatus` (`guid`, `quest`, `status`, `explored`, `timer`, `mobcount1`, `mobcount2`, `mobcount3`, `mobcount4`, `itemcount1`, `itemcount2`, `itemcount3`, `itemcount4`, `itemcount5`, `itemcount6`, `playercount`) SELECT `guid`, `quest`, `status`, `explored`, `timer`, `mobcount1`, `mobcount2`, `mobcount3`, `mobcount4`, `itemcount1`, `itemcount2`, `itemcount3`, `itemcount4`, `itemcount5`, `itemcount6`, `playercount` FROM mod_everquest_character_class_queststatus WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), pullEQClassID);
    transaction->Append("INSERT IGNORE INTO `character_queststatus_rewarded` (`guid`, `quest`, `active`) SELECT `guid`, `quest`, `active` FROM mod_everquest_character_class_queststatus_rewarded WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), pullEQClassID);
}

void EverQuestMod::UpdatePlayerControllerForClassChange(Player* player, uint8 newEQClassID, CharacterDatabaseTransaction& transaction)
{
    uint32 currentExpPool = GetSecondaryExpPoolForPlayer(player);
    transaction->Append("INSERT INTO `mod_everquest_character_settings` (`guid`, `nextSecondaryClass`, `currentSecondaryClass`, `secondaryExpPool`) VALUES ({}, {}, {}, {}) "
        "ON DUPLICATE KEY UPDATE `nextSecondaryClass` = {}, `currentSecondaryClass` = {}, `secondaryExpPool` = {}",
        player->GetGUID().GetCounter(),
        newEQClassID,
        newEQClassID, // Overwriting current with next
        currentExpPool,
        newEQClassID,
        newEQClassID,
        currentExpPool);
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

                // Break out enchant values, guarding against short or malformed enchantment strings
                std::vector<std::string_view> tokens = Acore::Tokenize(enchantString, ' ', false);
                uint32 permEnchant = 0;
                uint32 tempEnchant = 0;
                if (tokens.size() > PERM_ENCHANTMENT_SLOT * MAX_ENCHANTMENT_OFFSET)
                    permEnchant = Acore::StringTo<uint32>(tokens[PERM_ENCHANTMENT_SLOT * MAX_ENCHANTMENT_OFFSET]).value_or(0);
                if (tokens.size() > TEMP_ENCHANTMENT_SLOT * MAX_ENCHANTMENT_OFFSET)
                    tempEnchant = Acore::StringTo<uint32>(tokens[TEMP_ENCHANTMENT_SLOT * MAX_ENCHANTMENT_OFFSET]).value_or(0);

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
    uint8 nextSecondaryEQClass = GetNextSecondEQClassForPlayer(player);
    bool isNew = !DoesSavedClassDataExistForPlayer(player, nextSecondaryEQClass);

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
    MoveQuestDataToModQuestTables(player, transaction);

    // Update pet references
    transaction->Append("UPDATE character_pet SET eq_owner = {}, eq_eqclass = {} WHERE owner = {}", player->GetGUID().GetCounter(), GetCurrentSecondEQClassForPlayer(player), player->GetGUID().GetCounter());
    transaction->Append("UPDATE character_pet SET owner = 0 WHERE eq_owner = {} AND eq_eqclass = {}", player->GetGUID().GetCounter(), GetCurrentSecondEQClassForPlayer(player));
    transaction->Append("UPDATE character_pet SET owner = {} WHERE eq_owner = {} AND eq_eqclass = {}", player->GetGUID().GetCounter(), player->GetGUID().GetCounter(), nextSecondaryEQClass);

    // New
    if (isNew)
    {
        // For start level
        uint32 startLevel = nextSecondaryEQClass != CLASS_DEATH_KNIGHT
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
        UpdateCharacterFromModCharacterTable(player, nextSecondaryEQClass, transaction);
        CopyModSpellTableIntoCharacterSpells(player, nextSecondaryEQClass, transaction);
        CopyModActionTableIntoCharacterAction(player, nextSecondaryEQClass, transaction);
        CopyModSkillTableIntoCharacterSkills(player, nextSecondaryEQClass, transaction);
        CopyModQuestTablesIntoCharacterQuests(player, nextSecondaryEQClass, transaction);

        transaction->Append("INSERT IGNORE INTO character_talent (guid, spell, specMask) SELECT guid, spell, specMask FROM mod_everquest_character_class_talent WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), nextSecondaryEQClass);
        transaction->Append("INSERT IGNORE INTO character_glyphs (guid, talentGroup, glyph1, glyph2, glyph3, glyph4, glyph5, glyph6) SELECT guid, talentGroup, glyph1, glyph2, glyph3, glyph4, glyph5, glyph6 FROM mod_everquest_character_class_glyphs WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), nextSecondaryEQClass);
        transaction->Append("INSERT IGNORE INTO character_aura (guid, casterGuid, itemGuid, spell, effectMask, recalculateMask, stackCount, amount0, amount1, amount2, base_amount0, base_amount1, base_amount2, maxDuration, remainTime, remainCharges) SELECT guid, casterGuid, itemGuid, spell, effectMask, recalculateMask, stackCount, amount0, amount1, amount2, base_amount0, base_amount1, base_amount2, maxDuration, remainTime, remainCharges FROM mod_everquest_character_class_aura WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), nextSecondaryEQClass);
        transaction->Append("INSERT IGNORE INTO `character_inventory` (`guid`, `bag`, `slot`, `item`) SELECT `guid`, `bag`, `slot`, `item` FROM mod_everquest_character_class_inventory WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), nextSecondaryEQClass);
    }

    // Update current class
    UpdatePlayerControllerForClassChange(player, nextSecondaryEQClass, transaction);
    GetOrLoadActivePlayerClassControllerData(player)->CurrentSecondClass = nextSecondaryEQClass;

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
    transaction->Append("DELETE FROM mod_everquest_character_settings WHERE guid = {}", playerGUID);
    transaction->Append("DELETE FROM mod_everquest_character_class_queststatus WHERE guid = {}", playerGUID);
    transaction->Append("DELETE FROM mod_everquest_character_class_queststatus_rewarded WHERE guid = {}", playerGUID);
    transaction->Append("DELETE FROM character_pet WHERE owner = 0 AND eq_owner = {}", playerGUID);
    CharacterDatabase.CommitTransaction(transaction);
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        ActivePlayerClassControllerDataByGUID.erase(guid);
    }
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

std::string GetEQClassCommandNameFromID(uint8 classID)
{
    switch (classID)
    {
    case EQ_EQCLASS_WARRIOR:        return "warrior";
    case EQ_EQCLASS_CLERIC:         return "cleric";
    case EQ_EQCLASS_PALADIN:        return "paladin";
    case EQ_EQCLASS_RANGER:         return "ranger";
    case EQ_EQCLASS_SHADOWKNIGHT:   return "shadowknight";
    case EQ_EQCLASS_DRUID:          return "druid";
    case EQ_EQCLASS_MONK:           return "monk";
    case EQ_EQCLASS_BARD:           return "bard";
    case EQ_EQCLASS_ROGUE:          return "rogue";
    case EQ_EQCLASS_SHAMAN:         return "shaman";
    case EQ_EQCLASS_NECROMANCER:    return "necromancer";
    case EQ_EQCLASS_WIZARD:         return "wizard";
    case EQ_EQCLASS_MAGICIAN:       return "magician";
    case EQ_EQCLASS_ENCHANTER:      return "enchanter";
    default:                        return "none";
    }
}

// Sends the player's EQ class state to the client UI (the EQ Class character tab) as a hidden addon message
void EverQuestMod::SendClassInfoAddonMessageToPlayer(Player* player)
{
    if (player == nullptr)
        return;

    const EverQuestClassMap classMap = GetClassMapForWOWClassID(player->getClass());
    uint8 currentSecondClass = GetCurrentSecondEQClassForPlayer(player);
    uint8 nextSecondClass = GetNextSecondEQClassForPlayer(player);

    // Level lookup by EQ class id.  The active secondary's saved row is excluded while active, so use the
    // character's live level for it (mirrors the ".class info" command)
    map<string, EverQuestPlayerClassInfoItem> playerClassInfoItems = GetPlayerClassInfoByClassNameForPlayer(player);
    map<uint8, uint8> levelByEQClassID;
    for (auto& playerClassInfoItem : playerClassInfoItems)
        levelByEQClassID[playerClassInfoItem.second.ClassID] = playerClassInfoItem.second.Level;
    levelByEQClassID[currentSecondClass] = player->GetLevel();

    // Format (after the "EQCLASS\t" prefix the 3.3.5 client strips):
    //   H|<baseId>|<baseName>|<currentSecondId>|<nextSecondId>|<expPoolCurrent>|<expPoolMax>
    //   ~R|<id>|<name>|<level>|<changecmd>   (one per row: None first, then each eligible secondary class)
    std::ostringstream payload;
    payload << "H|" << uint32(classMap.EQClassIDBase) << "|" << GetEQClassStringFromID(classMap.EQClassIDBase)
            << "|" << uint32(currentSecondClass) << "|" << uint32(nextSecondClass)
            << "|" << GetSecondaryExpPoolForPlayer(player) << "|" << ConfigSecondaryExpPoolMaxPooled;

    for (int16 eqClassID = EQ_EQCLASS_NONE; eqClassID <= EQ_EQCLASS_ENCHANTER; ++eqClassID)
    {
        // None is always listed and every other class must be flagged as an eligible secondary
        if (eqClassID != EQ_EQCLASS_NONE)
        {
            uint32 classBit = 1u << (eqClassID - 1);
            if ((classMap.EQClassIDEligibleSecondMask & classBit) == 0)
                continue;
        }

        uint8 level = 1;
        auto levelItr = levelByEQClassID.find(static_cast<uint8>(eqClassID));
        if (levelItr != levelByEQClassID.end())
            level = levelItr->second;

        payload << "~R|" << uint32(eqClassID) << "|" << GetEQClassStringFromID(static_cast<uint8>(eqClassID))
                << "|" << uint32(level) << "|" << GetEQClassCommandNameFromID(static_cast<uint8>(eqClassID));
    }

    std::string addonMessage = "EQCLASS\t" + payload.str();
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_ADDON, nullptr, nullptr, addonMessage);
    player->SendDirectMessage(&data);
}

// Pushes a live secondary-experience-pool update to the client "(+X exp added to exp pool)"
// Payload (after the "EQEXPPOOL\t" prefix): <gainedExp>|<poolCurrent>|<poolMax>
void EverQuestMod::SendExpPoolAddonMessageToPlayer(Player* player, uint32 gainedExp)
{
    if (player == nullptr)
        return;

    std::string addonMessage = "EQEXPPOOL\t" + std::to_string(gainedExp) + "|"
        + std::to_string(GetSecondaryExpPoolForPlayer(player)) + "|"
        + std::to_string(ConfigSecondaryExpPoolMaxPooled);
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_ADDON, nullptr, nullptr, addonMessage);
    player->SendDirectMessage(&data);
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
