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

#include "Bag.h"
#include "Chat.h"
#include "GameEventMgr.h"
#include "GameGraveyard.h"
#include "InstanceSaveMgr.h"
#include "Group.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "CreatureData.h"
#include "DBCStores.h"
#include "Formulas.h"
#include "GameTime.h"
#include "LootMgr.h"
#include "Mail.h"
#include "Pet.h"
#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "ReputationMgr.h"
#include "Spell.h"
#include "SpellAuraDefines.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "Timer.h"
#include "Tokenize.h"
#include "Map.h"
#include "MapMgr.h"
#include "MotionMaster.h"
#include "MovementGenerator.h"
#include "CharacterCache.h"
#include "ObjectAccessor.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "GossipDef.h"
#include "ScriptedGossip.h"
#include "TemporarySummon.h"
#include "CellImpl.h"
#include "Corpse.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "WorldSessionMgr.h"

#include "EverQuest.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <random>
#include <thread>

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
    ConfigSystemInvisVsUndeadDetectSpellID(0),
    ConfigSystemResistAdjustmentSpellID(0),
    ConfigSystemLegacyAchievementID(0),
    ConfigSystemItemTemplateIDMin(0),
    ConfigSystemItemTemplateIDMax(0),
    ConfigSystemAdventurerAchievementID(0),
    ConfigSystemAdventurerAuraSpellID(0),
    ConfigSystemAgileFighterSpellID(0),
    ConfigSystemAgileFighterCombatMasterSpellID(0),
    ConfigSystemAgileFighterCombatExpertSpellID(0),
    ConfigSystemFactionGoodClassMask(0),
    ConfigSystemFactionEvilClassMask(0),
    ConfigSystemFactionGoodRaceMask(0),
    ConfigSystemFactionEvilRaceMask(0),
    ConfigMapRestrictPlayersToNorrath(false),
    ConfigMapMaxExpansionID(-1),
    ConfigMapRestrictedMapCheckIntervalInSeconds(300),
    ConfigQuestGrantExpOnRepeatCompletion(true),
    ConfigExpLossOnDeathEnabled(true),
    ConfigExpLossOnDeathMinLevel(5),
    ConfigExpLossOnDeathLossPercent(10),
    ConfigExpLossOnDeathAddLostExpToRestExp(true),
    ConfigExpLossOnDeathResurrectRestorePercent(90.0f),
    ConfigAlternateGroupExperienceFormulaEnabled(false),
    ConfigAlternateGroupExperienceAddPercentPerAddedMember(20.0f),
    ConfigSpellDisableStackingOfSameDOT(false),
    ConfigSpellBuffLevelRestrictionsEnabled(true),
    ConfigSpellCrowdControlLevelRestrictionsEnabled(true),
    ConfigSpellHasteCapEnabled(true),
    ConfigSpellHasteCapPercent(100.0f),
    ConfigSpellBardFearDiminishingReturnsEnabled(true),
    ConfigSpellBardFearDiminishingReturnsResetTimeInMS(15000),
    ConfigSpellNoSwingTimerResetForEQSpells(true),
    ConfigSpellNoSwingTimerResetForWoWSpells(false),
    ConfigCombatSkillsDisableBashKickStunOnPlayers(false),
    ConfigCombatSkillsDisabledBashKickStunInterruptsPlayerCast(true),
    ConfigEvadeEnabled(true),
    ConfigEvadeUnreachableSeconds(10.0f),
    ConfigEvadeUnstickStallSeconds(3.0f),
    ConfigEvadeUnstickSettleSeconds(1.0f),
    ConfigEvadeUnstickMoveThreshold(3.0f),
    ConfigEvadeUnstickMaxAttempts(3),
    ConfigEvadeUnstickStepPercent(25),
    ConfigEvadeNonEQMapLeashRadius(30.0f),
    ConfigCharmCreatureCharmLimitsEnabled(true),
    ConfigCharmUncharmedPlayerCheckRadius(100.0f),
    ConfigCreatureEmotesEnabled(true),
    ConfigCreatureEmotesAmbientEnabled(true),
    ConfigIllusionGearRefreshTimeInMS(1000),
    ConfigShowClassMessageOnLogin(true),
    ConfigSecondaryExpPoolGainPercent(25.0f),
    ConfigSecondaryExpPoolMaxPooled(1000000),
    ConfigPlayerLevelCap(0),
    ConfigPlayerAddHearthstoneToNewCharacters(true),
    ConfigPlayerAddMasterTotemToShamans(true),
    ConfigPlayerAddRacialGuiseItemOnLogin(true),
    ConfigPlayerAddClassStartItems(true),
    ConfigAchievementAdventurerLevel(50),
    ConfigTrackingEnabled(true),
    ConfigTrackingRangerYardsPerLevel(20.0f),
    ConfigTrackingDruidYardsPerLevel(15.0f),
    ConfigTrackingBardYardsPerLevel(10.0f),
    ConfigTrackingRangerMaxRange(600.0f),
    ConfigTrackingDruidMaxRange(300.0f),
    ConfigTrackingBardMaxRange(180.0f),
    ConfigTrackingMaxResults(0),
    ConfigTrackingPulseIntervalInMS(5000),
    ConfigGroupZoneWideLootAndExperienceEnabled(true),
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
            else if (key == "FactionGoodClassMask")
                ConfigSystemFactionGoodClassMask = (uint32)atoi(value.c_str());
            else if (key == "FactionEvilClassMask")
                ConfigSystemFactionEvilClassMask = (uint32)atoi(value.c_str());
            else if (key == "FactionGoodRaceMask")
                ConfigSystemFactionGoodRaceMask = (uint32)atoi(value.c_str());
            else if (key == "FactionEvilRaceMask")
                ConfigSystemFactionEvilRaceMask = (uint32)atoi(value.c_str());
            else if (key == "InvisVsUndeadDetectSpellID")
                ConfigSystemInvisVsUndeadDetectSpellID = (uint32)atoi(value.c_str());
            else if (key == "ItemTemplateIDMin")
                ConfigSystemItemTemplateIDMin = (uint32)atoi(value.c_str());
            else if (key == "ItemTemplateIDMax")
                ConfigSystemItemTemplateIDMax = (uint32)atoi(value.c_str());
            else if (key == "LegacyAchievementID")
                ConfigSystemLegacyAchievementID = (uint32)atoi(value.c_str());
            else if (key == "LegacyAchievementAccountCreatedBefore")
                ConfigSystemLegacyAchievementAccountCreatedBefore = value;
            else if (key == "AdventurerAchievementID")
                ConfigSystemAdventurerAchievementID = (uint32)atoi(value.c_str());
            else if (key == "AdventurerAuraSpellID")
                ConfigSystemAdventurerAuraSpellID = (uint32)atoi(value.c_str());
            else if (key == "AgileFighterSpellID")
                ConfigSystemAgileFighterSpellID = (uint32)atoi(value.c_str());
            else if (key == "AgileFighterCombatMasterSpellID")
                ConfigSystemAgileFighterCombatMasterSpellID = (uint32)atoi(value.c_str());
            else if (key == "AgileFighterCombatExpertSpellID")
                ConfigSystemAgileFighterCombatExpertSpellID = (uint32)atoi(value.c_str());
            else if (key == "RaidBossRespawnVarianceInSec")
                ConfigSystemRaidBossRespawnVarianceInSec = (uint32)atoi(value.c_str());
            else if (key == "RaidMiniBossRespawnVarianceInSec")
                ConfigSystemRaidMiniBossRespawnVarianceInSec = (uint32)atoi(value.c_str());
            else if (key == "CompleteHealExhaustionSpellID")
                ConfigSystemCompleteHealExhaustionSpellID = (uint32)atoi(value.c_str());
            else if (key == "CompleteHealExhaustionManaCostPercentPerStack")
                ConfigSystemCompleteHealExhaustionManaCostPercentPerStack = (uint32)atoi(value.c_str());
            else if (key == "IllusionObjectMaxDistance")
                ConfigSystemIllusionObjectMaxDistance = (float)atof(value.c_str());
            else if (key == "IllusionObjectTreeMaxDistance")
                ConfigSystemIllusionObjectTreeMaxDistance = (float)atof(value.c_str());
            else if (key == "ClientDataVersion")
                ConfigSystemClientDataVersion = (uint32)atoi(value.c_str());
            else if (key == "ClientDataVersionMismatchMessage")
                ConfigSystemClientDataVersionMismatchMessage = value;
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
            else if (key == "ResistAdjustmentSpellID")
                ConfigSystemResistAdjustmentSpellID = (uint32)atoi(value.c_str());
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
    ConfigMapMaxExpansionID = sConfigMgr->GetOption<int>("EverQuest.Map.MaxExpansionID", -1);
    ConfigMapRestrictedMapCheckIntervalInSeconds = sConfigMgr->GetOption<uint32>("EverQuest.Map.RestrictedMapCheckIntervalInSeconds", 300);

    // Death
    ConfigDeathEnforceGraveyardDomain = sConfigMgr->GetOption<bool>("EverQuest.Death.EnforceGraveyardDomain", true);
    ConfigDeathFallbackGraveyardID = sConfigMgr->GetOption<uint32>("EverQuest.Death.FallbackGraveyardID", 1813);

    // Client Version Check
    ConfigClientVersionCheckEnabled = sConfigMgr->GetOption<bool>("EverQuest.ClientVersionCheck.Enabled", false);
    ConfigClientVersionCheckGraceTimeInSeconds = sConfigMgr->GetOption<uint32>("EverQuest.ClientVersionCheck.GraceTimeInSeconds", 30);
    ConfigClientVersionCheckKickDelayInSeconds = sConfigMgr->GetOption<uint32>("EverQuest.ClientVersionCheck.KickDelayInSeconds", 10);

    // Spell
    ConfigSpellTalentAlignmentEnabled = sConfigMgr->GetOption<bool>("EverQuest.Spell.TalentAlignmentEnabled", true);

    // Quest
    ConfigQuestGrantExpOnRepeatCompletion = sConfigMgr->GetOption<bool>("EverQuest.Quest.GrantExpOnRepeatCompletion", true);

    // Exp Loss on Death
    ConfigExpLossOnDeathEnabled = sConfigMgr->GetOption<bool>("EverQuest.ExpLossOnDeath.Enabled", true);
    ConfigExpLossOnDeathMinLevel = sConfigMgr->GetOption<uint32>("EverQuest.ExpLossOnDeath.MinLevel", 5);
    ConfigExpLossOnDeathLossPercent = sConfigMgr->GetOption<float>("EverQuest.ExpLossOnDeath.LossPercent", 10);
    ConfigExpLossOnDeathAddLostExpToRestExp = sConfigMgr->GetOption<bool>("EverQuest.ExpLossOnDeath.AddLostExpToRestExp", true);
    ConfigExpLossOnDeathResurrectRestorePercent = sConfigMgr->GetOption<float>("EverQuest.ExpLossOnDeath.ResurrectRestorePercent", 90.0f);

    // Group EXP rates
    ConfigAlternateGroupExperienceFormulaEnabled = sConfigMgr->GetOption<bool>("EverQuest.AlternateGroupExperienceFormula.Enabled", false);
    ConfigAlternateGroupExperienceAddPercentPerAddedMember = sConfigMgr->GetOption<float>("EverQuest.AlternateGroupExperienceFormula.AddPercentPerMember", 20.0f);

    // Spells
    ConfigSpellDisableStackingOfSameDOT = sConfigMgr->GetOption<bool>("EverQuest.Spells.DisableStackingOfSameDOT", false);
    ConfigSpellBuffLevelRestrictionsEnabled = sConfigMgr->GetOption<bool>("EverQuest.Spells.BuffLevelRestrictionsEnabled", true);
    ConfigSpellCrowdControlLevelRestrictionsEnabled = sConfigMgr->GetOption<bool>("EverQuest.Spells.CrowdControlLevelRestrictionsEnabled", true);
    ConfigSpellHasteCapEnabled = sConfigMgr->GetOption<bool>("EverQuest.Spells.HasteCapEnabled", true);
    ConfigSpellHasteCapPercent = sConfigMgr->GetOption<float>("EverQuest.Spells.HasteCapPercent", 100.0f);
    ConfigSpellBardFearDiminishingReturnsEnabled = sConfigMgr->GetOption<bool>("EverQuest.Spells.BardFearDiminishingReturnsEnabled", true);
    ConfigSpellBardFearDiminishingReturnsResetTimeInMS = sConfigMgr->GetOption<uint32>("EverQuest.Spells.BardFearDiminishingReturnsResetTimeInMS", 15000);
    ConfigSpellNoSwingTimerResetForEQSpells = sConfigMgr->GetOption<bool>("EverQuest.Spells.NoSwingTimerResetForEQSpells", true);
    ConfigSpellNoSwingTimerResetForWoWSpells = sConfigMgr->GetOption<bool>("EverQuest.Spells.NoSwingTimerResetForWoWSpells", false);

    // Combat Skills
    ConfigCombatSkillsDisableBashKickStunOnPlayers = sConfigMgr->GetOption<bool>("EverQuest.CombatSkills.DisableBashKickStunOnPlayers", false);
    ConfigCombatSkillsDisabledBashKickStunInterruptsPlayerCast = sConfigMgr->GetOption<bool>("EverQuest.CombatSkills.DisabledBashKickStunInterruptsPlayerCast", true);
    ConfigCombatSkillsRangedAttackEnabled = sConfigMgr->GetOption<bool>("EverQuest.CombatSkills.RangedAttackEnabled", true);
    ConfigCombatSkillsRangedAttackDefaultMinRange = sConfigMgr->GetOption<float>("EverQuest.CombatSkills.RangedAttackDefaultMinRange", 25.0f);
    ConfigCombatSkillsRangedAttackDefaultMaxRange = sConfigMgr->GetOption<float>("EverQuest.CombatSkills.RangedAttackDefaultMaxRange", 250.0f);
    ConfigCombatSkillsRangedAttackDamageMultiplier = sConfigMgr->GetOption<float>("EverQuest.CombatSkills.RangedAttackDamageMultiplier", 1.0f);
    ConfigCombatSkillsEnrageEnabled = sConfigMgr->GetOption<bool>("EverQuest.CombatSkills.EnrageEnabled", true);
    ConfigCombatSkillsEnrageDefaultHPPct = sConfigMgr->GetOption<uint32>("EverQuest.CombatSkills.EnrageDefaultHPPercent", 10);
    ConfigCombatSkillsEnrageDefaultDurationInMS = sConfigMgr->GetOption<uint32>("EverQuest.CombatSkills.EnrageDefaultDurationInMS", 10000);
    ConfigCombatSkillsEnrageDefaultCooldownInMS = sConfigMgr->GetOption<uint32>("EverQuest.CombatSkills.EnrageDefaultCooldownInMS", 360000);
    ConfigCombatSkillsFlurryEnabled = sConfigMgr->GetOption<bool>("EverQuest.CombatSkills.FlurryEnabled", true);
    ConfigCombatSkillsFlurryDefaultChancePct = sConfigMgr->GetOption<uint32>("EverQuest.CombatSkills.FlurryDefaultChancePercent", 20);
    ConfigCombatSkillsRampageEnabled = sConfigMgr->GetOption<bool>("EverQuest.CombatSkills.RampageEnabled", true);
    ConfigCombatSkillsRampageDefaultChancePct = sConfigMgr->GetOption<uint32>("EverQuest.CombatSkills.RampageDefaultChancePercent", 20);
    ConfigCombatSkillsRampageDefaultRange = sConfigMgr->GetOption<float>("EverQuest.CombatSkills.RampageDefaultRange", 75.0f);
    ConfigCombatSkillsWildRampageEnabled = sConfigMgr->GetOption<bool>("EverQuest.CombatSkills.WildRampageEnabled", true);
    ConfigCombatSkillsWildRampageDefaultChancePct = sConfigMgr->GetOption<uint32>("EverQuest.CombatSkills.WildRampageDefaultChancePercent", 20);
    ConfigCombatSkillsWildRampageDefaultMaxTargets = sConfigMgr->GetOption<uint32>("EverQuest.CombatSkills.WildRampageDefaultMaxTargets", 999);
    ConfigCombatSkillsRaidBossSummonEnabled = sConfigMgr->GetOption<bool>("EverQuest.CombatSkills.RaidBossSummonEnabled", true);
    ConfigCombatSkillsRaidBossSummonMaxHealthPct = sConfigMgr->GetOption<uint32>("EverQuest.CombatSkills.RaidBossSummonMaxHealthPercent", 97);
    ConfigCombatSkillsRaidBossSummonCooldownInMS = sConfigMgr->GetOption<uint32>("EverQuest.CombatSkills.RaidBossSummonCooldownInMS", 11000);

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
    ConfigEvadeNonEQMapLeashRadius = sConfigMgr->GetOption<float>("EverQuest.Evade.NonEQMapLeashRadius", 30.0f);

    // Charm
    ConfigCharmCreatureCharmLimitsEnabled = sConfigMgr->GetOption<bool>("EverQuest.Charm.CreatureCharmLimitsEnabled", true);
    ConfigCharmUncharmedPlayerCheckRadius = sConfigMgr->GetOption<float>("EverQuest.Charm.UncharmedPlayerCheckRadius", 100.0f);

    // Faction
    ConfigFactionDefendFriendlyPlayersEnabled = sConfigMgr->GetOption<bool>("EverQuest.Faction.DefendFriendlyPlayersEnabled", true);

    // Pet
    ConfigPetDisableInitialCreatureAgro = sConfigMgr->GetOption<bool>("EverQuest.Pet.DisableInitialCreatureAgro", true);

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

    // Player Armor
    ConfigPlayerShieldArmorIgnoresBearFormMultiplier = sConfigMgr->GetOption<bool>("EverQuest.Player.ShieldArmorIgnoresBearFormMultiplier", true);

    // Player Hearthstone
    ConfigPlayerAddHearthstoneToNewCharacters = sConfigMgr->GetOption<bool>("EverQuest.Player.AddHearthstoneToNewCharacters", true);

    // Player Master Totem
    ConfigPlayerAddMasterTotemToShamans = sConfigMgr->GetOption<bool>("EverQuest.Player.AddMasterTotemToShamans", true);

    // Player Racial Guise Item
    ConfigPlayerAddRacialGuiseItemOnLogin = sConfigMgr->GetOption<bool>("EverQuest.Player.AddRacialGuiseItemOnLogin", true);
    ConfigPlayerAddClassStartItems = sConfigMgr->GetOption<bool>("EverQuest.Player.AddClassStartItems", true);

    // Achievements
    ConfigAchievementAdventurerLevel = sConfigMgr->GetOption<uint32>("EverQuest.Achievement.AdventurerLevel", 50);

    // Tracking
    ConfigTrackingEnabled = sConfigMgr->GetOption<bool>("EverQuest.Tracking.Enabled", true);
    ConfigTrackingRangerYardsPerLevel = sConfigMgr->GetOption<float>("EverQuest.Tracking.RangerYardsPerLevel", 20.0f);
    ConfigTrackingDruidYardsPerLevel = sConfigMgr->GetOption<float>("EverQuest.Tracking.DruidYardsPerLevel", 15.0f);
    ConfigTrackingBardYardsPerLevel = sConfigMgr->GetOption<float>("EverQuest.Tracking.BardYardsPerLevel", 10.0f);
    ConfigTrackingRangerMaxRange = sConfigMgr->GetOption<float>("EverQuest.Tracking.RangerMaxRange", 600.0f);
    ConfigTrackingDruidMaxRange = sConfigMgr->GetOption<float>("EverQuest.Tracking.DruidMaxRange", 300.0f);
    ConfigTrackingBardMaxRange = sConfigMgr->GetOption<float>("EverQuest.Tracking.BardMaxRange", 180.0f);
    ConfigTrackingMaxResults = sConfigMgr->GetOption<uint32>("EverQuest.Tracking.MaxResults", 0);
    ConfigTrackingPulseIntervalInMS = sConfigMgr->GetOption<uint32>("EverQuest.Tracking.PulseIntervalInMS", 5000);
    if (ConfigTrackingPulseIntervalInMS < 1000)
        ConfigTrackingPulseIntervalInMS = 1000;

    // Spells
    ConfigSpellSummonPlayerAcrossZones = sConfigMgr->GetOption<bool>("EverQuest.Spell.SummonPlayerAcrossZones", false);

    // Group
    ConfigGroupZoneWideLootAndExperienceEnabled = sConfigMgr->GetOption<bool>("EverQuest.Group.ZoneWideLootAndExperienceEnabled", true);

    // Cross-Class values
    ConfigCrossClassIncludeSkillIDs = GetSetFromConfigString("EverQuest.CrossClass.IncludeSkillIDs");

    // The cross-class exempt spell cache derives from the skill list above, and is built alongside the racial and death knight spell caches.  A live ".reload config" lands here on the world thread while a map thread could be reading them
    {
        std::lock_guard<std::mutex> lock(CrossClassExemptSpellIDsMutex);
        CrossClassExemptSpellIDs.clear();
        RacialSpellIDs.clear();
        DeathKnightSpellIDs.clear();
        DeathKnightSpellMinLevelBySpellID.clear();
        CrossClassExemptSpellIDsBuilt = false;
    }
}

void EverQuestMod::LoadCreatureData()
{
    CreaturesByTemplateID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT CreatureTemplateID, CanShowHeldLootItems, CanShowHeldLootShields, SpawnLimit, RangedAttackEnabled, RangedAttackMinRange, RangedAttackMaxRange, RangedAttackDamageModPct, AgroSocialDistanceMod, EnrageEnabled, EnrageHPPct, EnrageDurationInMS, EnrageCooldownInMS, FlurryEnabled, FlurryChancePct, RampageEnabled, RampageChancePct, RampageRange, RampageDamagePct, WildRampageEnabled, WildRampageChancePct, WildRampageMaxTargets, WildRampageDamagePct, AttackRoundTimeInMS, DifficultyType, GossipIsOnlyFromHailText FROM mod_everquest_creature ORDER BY CreatureTemplateID;");
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
            everQuestCreature.EnrageEnabled = fields[9].Get<bool>();
            everQuestCreature.EnrageHPPct = fields[10].Get<uint32>();
            everQuestCreature.EnrageDurationInMS = fields[11].Get<uint32>();
            everQuestCreature.EnrageCooldownInMS = fields[12].Get<uint32>();
            everQuestCreature.FlurryEnabled = fields[13].Get<bool>();
            everQuestCreature.FlurryChancePct = fields[14].Get<uint32>();
            everQuestCreature.RampageEnabled = fields[15].Get<bool>();
            everQuestCreature.RampageChancePct = fields[16].Get<uint32>();
            everQuestCreature.RampageRange = fields[17].Get<uint32>();
            everQuestCreature.RampageDamagePct = fields[18].Get<uint32>();
            everQuestCreature.WildRampageEnabled = fields[19].Get<bool>();
            everQuestCreature.WildRampageChancePct = fields[20].Get<uint32>();
            everQuestCreature.WildRampageMaxTargets = fields[21].Get<uint32>();
            everQuestCreature.WildRampageDamagePct = fields[22].Get<uint32>();
            everQuestCreature.AttackRoundTimeInMS = fields[23].Get<uint32>();
            everQuestCreature.DifficultyType = fields[24].Get<uint32>();
            everQuestCreature.GossipIsOnlyFromHailText = fields[25].Get<bool>();
            CreaturesByTemplateID[everQuestCreature.CreatureTemplateID] = everQuestCreature;
        } while (queryResult->NextRow());
    }
}

void EverQuestMod::LoadCreatureSpawnPoints()
{
    CreatureSpawnPointsByCreatureGUID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT CreatureGUID, MapID, SpawnPointID, SpawnGroupID, SpawnGroupLimit, CycleRespawnTimeSec, CycleChance FROM mod_everquest_creature_spawn_point;");
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
            creatureSpawnPoint.CycleRespawnTimeSec = fields[5].Get<uint32>();
            creatureSpawnPoint.CycleChance = fields[6].Get<uint32>();
            CreatureSpawnPointsByCreatureGUID[creatureSpawnPoint.CreatureGUID] = creatureSpawnPoint;
        } while (queryResult->NextRow());
    }

    // Cycle group spawn metadata
    CycleSpawnGroupsByMapIDThenSpawnGroupID.clear();
    {
        std::lock_guard<std::mutex> lock(CycleSpawnCheckTimerMutex);
        CycleSpawnCheckTimerInMSByMapInstanceKey.clear();
    }
    for (auto& spawnPointPair : CreatureSpawnPointsByCreatureGUID)
    {
        const EverQuestCreatureSpawnPoint& spawnPoint = spawnPointPair.second;
        if (spawnPoint.CycleRespawnTimeSec == 0)
            continue;
        EverQuestCycleSpawnGroup& cycleSpawnGroup = CycleSpawnGroupsByMapIDThenSpawnGroupID[spawnPoint.MapID][spawnPoint.SpawnGroupID];
        cycleSpawnGroup.MapID = spawnPoint.MapID;
        cycleSpawnGroup.SpawnGroupID = spawnPoint.SpawnGroupID;
        if (spawnPoint.SpawnGroupLimit > 0)
            cycleSpawnGroup.SpawnGroupLimit = spawnPoint.SpawnGroupLimit;
        cycleSpawnGroup.CycleRespawnTimeSec = spawnPoint.CycleRespawnTimeSec;
        EverQuestCycleSpawnCandidate cycleSpawnCandidate;
        cycleSpawnCandidate.CreatureGUID = spawnPoint.CreatureGUID;
        cycleSpawnCandidate.Chance = spawnPoint.CycleChance;
        cycleSpawnGroup.CandidatesBySpawnPointID[spawnPoint.SpawnPointID].push_back(cycleSpawnCandidate);
    }
}

ObjectGuid::LowType EverQuestMod::RollCycleSpawnCreatureGUID(const EverQuestCycleSpawnGroup& cycleSpawnGroup, uint32 excludedSpawnPointID, Map* map)
{
    vector<uint32> eligibleSpawnPointIDs;
    unordered_set<ObjectGuid::LowType> corpseSpawnIDs;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto loadedMapIter = AllLoadedCreaturesByMapInstanceKeyThenSpawnPointID.find(GetMapInstanceKey(map));
        for (auto& candidatesPair : cycleSpawnGroup.CandidatesBySpawnPointID)
        {
            uint32 spawnPointID = candidatesPair.first;
            if (spawnPointID == excludedSpawnPointID)
                continue;
            bool hasAliveCreature = false;
            if (loadedMapIter != AllLoadedCreaturesByMapInstanceKeyThenSpawnPointID.end())
            {
                auto spawnPointIter = loadedMapIter->second.find(spawnPointID);
                if (spawnPointIter != loadedMapIter->second.end())
                {
                    for (Creature* loadedCreature : spawnPointIter->second)
                    {
                        if (loadedCreature->IsAlive() == true)
                            hasAliveCreature = true;
                        else if (loadedCreature->GetSpawnId() != 0)
                            corpseSpawnIDs.insert(loadedCreature->GetSpawnId());
                    }
                }
            }
            if (hasAliveCreature == false)
                eligibleSpawnPointIDs.push_back(spawnPointID);
        }
    }

    // If skipping the excluded point left nothing (a single point group), allow every point instead
    if (eligibleSpawnPointIDs.empty() == true && excludedSpawnPointID != 0)
        return RollCycleSpawnCreatureGUID(cycleSpawnGroup, 0, map);
    if (eligibleSpawnPointIDs.empty() == true)
        return 0;

    uint32 chosenSpawnPointID = eligibleSpawnPointIDs[urand(0, eligibleSpawnPointIDs.size() - 1)];
    const vector<EverQuestCycleSpawnCandidate>& candidates = cycleSpawnGroup.CandidatesBySpawnPointID.at(chosenSpawnPointID);
    vector<const EverQuestCycleSpawnCandidate*> rollableCandidates;
    uint32 totalChance = 0;
    for (const EverQuestCycleSpawnCandidate& candidate : candidates)
    {
        if (corpseSpawnIDs.find(candidate.CreatureGUID) != corpseSpawnIDs.end())
            continue;
        rollableCandidates.push_back(&candidate);
        totalChance += candidate.Chance;
    }
    if (rollableCandidates.empty() == true)
        return 0;
    if (totalChance == 0)
        return rollableCandidates[urand(0, rollableCandidates.size() - 1)]->CreatureGUID;
    uint32 chanceRoll = urand(1, totalChance);
    for (const EverQuestCycleSpawnCandidate* candidate : rollableCandidates)
    {
        if (chanceRoll <= candidate->Chance)
            return candidate->CreatureGUID;
        chanceRoll -= candidate->Chance;
    }
    return rollableCandidates[rollableCandidates.size() - 1]->CreatureGUID;
}

void EverQuestMod::ProcessCycleSpawnForCreatureDeath(Creature* deadCreature)
{
    if (deadCreature->GetSpawnId() == 0)
        return;
    auto spawnPointIter = CreatureSpawnPointsByCreatureGUID.find(deadCreature->GetSpawnId());
    if (spawnPointIter == CreatureSpawnPointsByCreatureGUID.end())
        return;
    const EverQuestCreatureSpawnPoint& spawnPoint = spawnPointIter->second;
    if (spawnPoint.CycleRespawnTimeSec == 0)
        return;
    uint32 mapID = deadCreature->GetMapId();
    auto cycleMapIter = CycleSpawnGroupsByMapIDThenSpawnGroupID.find(mapID);
    if (cycleMapIter == CycleSpawnGroupsByMapIDThenSpawnGroupID.end())
        return;
    auto cycleGroupIter = cycleMapIter->second.find(spawnPoint.SpawnGroupID);
    if (cycleGroupIter == cycleMapIter->second.end())
        return;
    const EverQuestCycleSpawnGroup& cycleSpawnGroup = cycleGroupIter->second;
    ObjectGuid::LowType nextCreatureGUID = RollCycleSpawnCreatureGUID(cycleSpawnGroup, spawnPoint.SpawnPointID, deadCreature->GetMap());
    if (nextCreatureGUID == 0)
        return;

    // Always defer through the pending queue so the respawn is prepped
    EverQuestPendingKillSpawnAction action;
    action.ActionType = EQ_KILLSPAWN_ACTION_RESPAWNTARGET;
    action.RespawnTimeSec = cycleSpawnGroup.CycleRespawnTimeSec;
    action.RespawnTargetSpawnIDs.push_back(nextCreatureGUID);
    action.RemainingMS = 1;
    EnqueuePendingKillSpawnAction(deadCreature->GetMap(), action);
}

void EverQuestMod::ApplyRaidBossRespawnVariance(Creature* deadCreature)
{
    if (HasCreatureDataForCreatureTemplateID(deadCreature->GetEntry()) == false)
        return;
    const EverQuestCreature& eqCreature = GetCreatureDataForCreatureTemplateID(deadCreature->GetEntry());

    // Both boss tiers randomize their respawn, just around their own center with their own swing
    uint32 configuredVarianceInSec;
    if (eqCreature.DifficultyType == EQ_CREATURE_DIFFICULTY_RAIDBOSS)
        configuredVarianceInSec = ConfigSystemRaidBossRespawnVarianceInSec;
    else if (eqCreature.DifficultyType == EQ_CREATURE_DIFFICULTY_RAIDMINIBOSS)
        configuredVarianceInSec = ConfigSystemRaidMiniBossRespawnVarianceInSec;
    else
        return;
    if (configuredVarianceInSec == 0)
        return;

    // Named raid creatures inside a raid instance are generated with a respawn time of the whole instance reset, so the variance must not reschedule them shorter
    // Everything else in a raid instance, and all of a dungeon instance, mirrors the open world
    if (IsMapInstanceRaidLow(deadCreature->GetMap()->GetId()) == true)
        return;

    // Only world spawns have a respawn to reschedule (summons and pets never do)
    ObjectGuid::LowType spawnID = deadCreature->GetSpawnId();
    if (spawnID == 0)
        return;

    // Base off spawn point's database timer since the creature's live respawn delay holds the previous roll and would drift with every death
    CreatureData const* creatureData = sObjectMgr->GetCreatureData(spawnID);
    if (creatureData == nullptr)
        return;
    uint32 centerInSec = creatureData->spawntimesecs;
    if (centerInSec == 0)
        return;

    // A variance configured wider than the center would roll a near-instant respawn, so keep the swing inside half of it
    uint32 varianceInSec = std::min(configuredVarianceInSec, centerInSec / 2);
    if (varianceInSec == 0)
        return;
    uint32 respawnInSec = (centerInSec - varianceInSec) + urand(0, varianceInSec * 2);

    // The engine already set a respawn using the unrandomized delay back in setDeathState, so overwrite both it and the delay that corpse removal recalculates from
    deadCreature->SetRespawnDelay(respawnInSec);
    deadCreature->SetRespawnTime(respawnInSec + deadCreature->GetCorpseDelay());
    deadCreature->SaveRespawnTime();
}

void EverQuestMod::UpdateCycleSpawns(Map* map, uint32 diff)
{
    // The MapInstanced container of an instanceable map holds no spawns itself, only its child instances do
    if (map->Instanceable() == true && map->GetInstanceId() == 0)
        return;

    // Instanced copies of a zone mirror the open world spawns and get their own spawn point rows, so they run their own cycles
    uint32 mapID = map->GetId();
    auto cycleMapIter = CycleSpawnGroupsByMapIDThenSpawnGroupID.find(mapID);
    if (cycleMapIter == CycleSpawnGroupsByMapIDThenSpawnGroupID.end())
        return;

    // Each instance of a map updates on its own thread, so the check timers are both per-instance and guarded
    {
        std::lock_guard<std::mutex> lock(CycleSpawnCheckTimerMutex);
        int32& cycleSpawnCheckTimerInMS = CycleSpawnCheckTimerInMSByMapInstanceKey[GetMapInstanceKey(map)];
        cycleSpawnCheckTimerInMS -= (int32)diff;
        if (cycleSpawnCheckTimerInMS > 0)
            return;
        cycleSpawnCheckTimerInMS = EQ_CYCLE_SPAWN_CHECK_INTERVAL_IN_MS;
    }

    time_t nowTime = GameTime::GetGameTime().count();
    for (auto& cycleSpawnGroupPair : cycleMapIter->second)
    {
        const EverQuestCycleSpawnGroup& cycleSpawnGroup = cycleSpawnGroupPair.second;

        // Nothing to do while the group is at its limit
        uint32 aliveCount = 0;
        {
            std::lock_guard<std::mutex> lock(RuntimeStateMutex);
            auto loadedMapIter = AllLoadedCreaturesByMapInstanceKeyThenSpawnGroupID.find(GetMapInstanceKey(map));
            if (loadedMapIter != AllLoadedCreaturesByMapInstanceKeyThenSpawnGroupID.end())
            {
                auto loadedGroupIter = loadedMapIter->second.find(cycleSpawnGroup.SpawnGroupID);
                if (loadedGroupIter != loadedMapIter->second.end())
                    for (Creature* loadedCreature : loadedGroupIter->second)
                        if (loadedCreature->IsAlive() == true)
                            aliveCount++;
            }
        }
        if (aliveCount >= cycleSpawnGroup.SpawnGroupLimit)
            continue;

        // A member with a near respawn time means the cycle is already moving
        bool hasPendingRespawn = false;
        for (auto& candidatesPair : cycleSpawnGroup.CandidatesBySpawnPointID)
        {
            for (const EverQuestCycleSpawnCandidate& candidate : candidatesPair.second)
            {
                time_t respawnTime = map->GetCreatureRespawnTime(candidate.CreatureGUID);
                if (respawnTime != 0 && respawnTime <= nowTime + (time_t)cycleSpawnGroup.CycleRespawnTimeSec + EQ_CYCLE_SPAWN_PENDING_WINDOW_IN_SEC)
                {
                    hasPendingRespawn = true;
                    break;
                }
            }
            if (hasPendingRespawn == true)
                break;
        }
        if (hasPendingRespawn == true)
            continue;

        ObjectGuid::LowType nextCreatureGUID = RollCycleSpawnCreatureGUID(cycleSpawnGroup, 0, map);
        if (nextCreatureGUID == 0)
            continue;
        time_t respawnTime = nowTime + (time_t)cycleSpawnGroup.CycleRespawnTimeSec;
        map->SaveCreatureRespawnTime(nextCreatureGUID, respawnTime);
    }
}

bool EverQuestMod::ShouldDespawnCreatureDueToSpawnRestrictions(Creature* creature)
{
    // Creatures loading in dead (corpses) never count against spawn restrictions
    if (creature->IsAlive() == false)
        return false;

    // Pets and summoned creatures are not placed spawns, so never restrict them
    if (creature->IsPet() == true || creature->IsSummon() == true)
        return false;

    // Restricted creatures (EQ "spawn_limit") can only have so many alive in a map at once
    uint64 mapInstanceKey = GetMapInstanceKey(creature->GetMap());
    uint32 spawnLimit = GetCreatureDataForCreatureTemplateID(creature->GetEntry()).SpawnLimit;
    if (spawnLimit > 0)
    {
        uint32 aliveCount = 0;
        for (Creature* loadedCreature : GetLoadedCreaturesWithEntryID(creature->GetMap(), creature->GetEntry()))
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
        if (AllLoadedCreaturesByMapInstanceKeyThenSpawnPointID.find(mapInstanceKey) != AllLoadedCreaturesByMapInstanceKeyThenSpawnPointID.end())
        {
            unordered_map<uint32, vector<Creature*>>& spawnPointMap = AllLoadedCreaturesByMapInstanceKeyThenSpawnPointID[mapInstanceKey];
            if (spawnPointMap.find(creatureSpawnPoint.SpawnPointID) != spawnPointMap.end())
                for (Creature* loadedCreature : spawnPointMap[creatureSpawnPoint.SpawnPointID])
                    if (loadedCreature != creature && loadedCreature->IsAlive() == true)
                        return true;
        }
        if (creatureSpawnPoint.SpawnGroupLimit > 0 && AllLoadedCreaturesByMapInstanceKeyThenSpawnGroupID.find(mapInstanceKey) != AllLoadedCreaturesByMapInstanceKeyThenSpawnGroupID.end())
        {
            unordered_map<uint32, vector<Creature*>>& spawnGroupMap = AllLoadedCreaturesByMapInstanceKeyThenSpawnGroupID[mapInstanceKey];
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
    EvadeKillSpawnTriggerCreatureTemplateIDs.clear();
    OocTimerKillSpawnDurationMSByCreatureTemplateID.clear();

    QueryResult queryResult = WorldDatabase.Query("SELECT ID, TriggerCreatureTemplateID, TriggerTypeID, MapID, ActionType, TargetCreatureTemplateID, Chance, AltGroup, AltID, AltWeight, SpawnAtCorpse, PositionX, PositionY, PositionZ, Orientation, DelayMinMS, DelayMaxMS, OnlyIfNotAliveCreatureTemplateID, RequireDeadCreatureTemplateIDs, RequireAliveCreatureTemplateIDs, AddToHateList, TriggerMinLevel, TriggerMaxLevel, RespawnTimeSec FROM mod_everquest_creature_kill_spawn;");
    if (queryResult)
    {
        do
        {
            Field* fields = queryResult->Fetch();
            EverQuestCreatureKillSpawn killSpawn;
            killSpawn.ID = fields[0].Get<uint32>();
            killSpawn.TriggerCreatureTemplateID = fields[1].Get<uint32>();
            killSpawn.TriggerTypeID = fields[2].Get<uint8>();
            killSpawn.MapID = fields[3].Get<uint32>();
            killSpawn.ActionType = fields[4].Get<uint8>();
            killSpawn.TargetCreatureTemplateID = fields[5].Get<uint32>();
            killSpawn.Chance = fields[6].Get<float>();
            killSpawn.AltGroup = fields[7].Get<uint32>();
            killSpawn.AltID = fields[8].Get<uint32>();
            killSpawn.AltWeight = fields[9].Get<float>();
            killSpawn.SpawnAtCorpse = fields[10].Get<uint8>() != 0;
            killSpawn.PositionX = fields[11].Get<float>();
            killSpawn.PositionY = fields[12].Get<float>();
            killSpawn.PositionZ = fields[13].Get<float>();
            killSpawn.Orientation = fields[14].Get<float>();
            killSpawn.DelayMinMS = fields[15].Get<uint32>();
            killSpawn.DelayMaxMS = fields[16].Get<uint32>();
            killSpawn.OnlyIfNotAliveCreatureTemplateID = fields[17].Get<uint32>();
            string requireDeadString = fields[18].Get<string>();
            for (std::string_view idToken : Acore::Tokenize(requireDeadString, ',', false))
                killSpawn.RequireDeadCreatureTemplateIDs.push_back(Acore::StringTo<uint32>(idToken).value_or(0));
            string requireAliveString = fields[19].Get<string>();
            for (std::string_view idToken : Acore::Tokenize(requireAliveString, ',', false))
                killSpawn.RequireAliveCreatureTemplateIDs.push_back(Acore::StringTo<uint32>(idToken).value_or(0));
            killSpawn.AddToHateList = fields[20].Get<uint8>() != 0;
            killSpawn.TriggerMinLevel = fields[21].Get<uint32>();
            killSpawn.TriggerMaxLevel = fields[22].Get<uint32>();
            killSpawn.RespawnTimeSec = fields[23].Get<uint32>();
            if (killSpawn.TriggerTypeID == EQ_KILLSPAWN_TRIGGER_EVADE)
                EvadeKillSpawnTriggerCreatureTemplateIDs.insert(killSpawn.TriggerCreatureTemplateID);
            else if (killSpawn.TriggerTypeID == EQ_KILLSPAWN_TRIGGER_OOCTIMER)
            {
                // This delay is the out-of-combat (ooc) countdouwn duration and the creature's countdown uses the longest duration
                if (killSpawn.DelayMinMS == 0)
                    LOG_ERROR("module.EverQuest", "EverQuestMod::LoadCreatureKillSpawnData kill spawn ID {} has an ooctimer trigger with no delay duration, so it will never fire", killSpawn.ID);
                else if (killSpawn.DelayMinMS > OocTimerKillSpawnDurationMSByCreatureTemplateID[killSpawn.TriggerCreatureTemplateID])
                    OocTimerKillSpawnDurationMSByCreatureTemplateID[killSpawn.TriggerCreatureTemplateID] = killSpawn.DelayMinMS;
            }
            CreatureKillSpawnsByTriggerCreatureTemplateID[killSpawn.TriggerCreatureTemplateID].push_back(killSpawn);
        } while (queryResult->NextRow());
    }
}

// Note: runs at world startup (OnStartup) which runs before sObjectMgr has any creature spawns
void EverQuestMod::ResolveKillSpawnRespawnTargetSpawnPoints()
{
    for (auto& killSpawnPair : CreatureKillSpawnsByTriggerCreatureTemplateID)
    {
        for (EverQuestCreatureKillSpawn& killSpawn : killSpawnPair.second)
        {
            if (killSpawn.ActionType != EQ_KILLSPAWN_ACTION_RESPAWNTARGET)
                continue;

            // The instance copies of the zone have their own spawn rows, so targets resolve for those maps too when they exist
            uint32 instanceRaidLowMapID = GetInstanceRaidLowMapIDForMap(killSpawn.MapID);
            uint32 instanceDungeonMapID = GetInstanceDungeonMapIDForMap(killSpawn.MapID);
            for (auto const& creatureDataPair : sObjectMgr->GetAllCreatureData())
            {
                CreatureData const& creatureData = creatureDataPair.second;
                if (creatureData.mapid != killSpawn.MapID && (instanceRaidLowMapID == 0 || creatureData.mapid != instanceRaidLowMapID) && (instanceDungeonMapID == 0 || creatureData.mapid != instanceDungeonMapID))
                    continue;
                if (creatureData.id != killSpawn.TargetCreatureTemplateID && creatureData.id2 != killSpawn.TargetCreatureTemplateID && creatureData.id3 != killSpawn.TargetCreatureTemplateID)
                    continue;
                killSpawn.TargetSpawnIDsByMapID[creatureData.mapid].push_back(creatureDataPair.first);
            }
            if (killSpawn.TargetSpawnIDsByMapID.find(killSpawn.MapID) == killSpawn.TargetSpawnIDsByMapID.end())
                LOG_ERROR("module.EverQuest", "EverQuestMod::ResolveKillSpawnRespawnTargetSpawnPoints found no spawn points for kill spawn ID {} with target creature template {} on map {}", killSpawn.ID, killSpawn.TargetCreatureTemplateID, killSpawn.MapID);
        }
    }
}

void EverQuestMod::LoadCreaturePresenceGroupData()
{
    CreaturePresenceGroupsByMapID.clear();

    QueryResult queryResult = WorldDatabase.Query("SELECT ID, MapID, PrimaryCreatureTemplateID, OtherCreatureTemplateID1, OtherCreatureTemplateID2, OtherCreatureTemplateID3, OtherCreatureTemplateID4, CheckIntervalMS FROM mod_everquest_creature_presence_group ORDER BY ID;");
    if (!queryResult)
        return;
    do
    {
        Field* fields = queryResult->Fetch();
        EverQuestCreaturePresenceGroup presenceGroup;
        presenceGroup.ID = fields[0].Get<uint32>();
        presenceGroup.MapID = fields[1].Get<uint32>();
        presenceGroup.PrimaryCreatureTemplateID = fields[2].Get<uint32>();
        for (uint32 otherIndex = 0; otherIndex < EQ_PRESENCE_GROUP_MAX_OTHER_CREATURES; otherIndex++)
        {
            uint32 otherCreatureTemplateID = fields[3 + otherIndex].Get<uint32>();
            if (otherCreatureTemplateID != 0)
                presenceGroup.OtherCreatureTemplateIDs.push_back(otherCreatureTemplateID);
        }
        presenceGroup.CheckIntervalMS = fields[3 + EQ_PRESENCE_GROUP_MAX_OTHER_CREATURES].Get<uint32>();
        if (presenceGroup.OtherCreatureTemplateIDs.empty() == true || presenceGroup.CheckIntervalMS == 0)
        {
            LOG_ERROR("module.EverQuest", "EverQuestMod::LoadCreaturePresenceGroupData skipped presence group ID {}, since it has no other creatures or no check interval", presenceGroup.ID);
            continue;
        }
        CreaturePresenceGroupsByMapID[presenceGroup.MapID].push_back(presenceGroup);
    } while (queryResult->NextRow());
}

// Note: runs at world startup (OnStartup), since sObjectMgr has no creature spawns when the data tables load
void EverQuestMod::ResolveCreaturePresenceGroupSpawnPoints()
{
    for (auto& presenceGroupPair : CreaturePresenceGroupsByMapID)
    {
        for (EverQuestCreaturePresenceGroup& presenceGroup : presenceGroupPair.second)
        {
            // The instance copies of the zone have their own spawn rows, so the primary resolves for those maps too
            uint32 instanceRaidLowMapID = GetInstanceRaidLowMapIDForMap(presenceGroup.MapID);
            uint32 instanceDungeonMapID = GetInstanceDungeonMapIDForMap(presenceGroup.MapID);
            for (auto const& creatureDataPair : sObjectMgr->GetAllCreatureData())
            {
                CreatureData const& creatureData = creatureDataPair.second;
                if (creatureData.mapid != presenceGroup.MapID && (instanceRaidLowMapID == 0 || creatureData.mapid != instanceRaidLowMapID) && (instanceDungeonMapID == 0 || creatureData.mapid != instanceDungeonMapID))
                    continue;
                if (creatureData.id != presenceGroup.PrimaryCreatureTemplateID && creatureData.id2 != presenceGroup.PrimaryCreatureTemplateID && creatureData.id3 != presenceGroup.PrimaryCreatureTemplateID)
                    continue;
                EverQuestCreaturePresenceSpawnPoint spawnPoint;
                spawnPoint.SpawnID = creatureDataPair.first;
                spawnPoint.PositionX = creatureData.posX;
                spawnPoint.PositionY = creatureData.posY;
                presenceGroup.PrimarySpawnPointsByMapID[creatureData.mapid].push_back(spawnPoint);
            }
            if (presenceGroup.PrimarySpawnPointsByMapID.find(presenceGroup.MapID) == presenceGroup.PrimarySpawnPointsByMapID.end())
                LOG_ERROR("module.EverQuest", "EverQuestMod::ResolveCreaturePresenceGroupSpawnPoints found no spawn points for presence group ID {} with primary creature template {} on map {}", presenceGroup.ID, presenceGroup.PrimaryCreatureTemplateID, presenceGroup.MapID);
        }
    }
}

void EverQuestMod::SetPresenceGroupPrimaryRespawnTime(Map* map, const EverQuestCreaturePresenceGroup& presenceGroup, uint32 respawnTimeSec)
{
    auto spawnPointsIter = presenceGroup.PrimarySpawnPointsByMapID.find(map->GetId());
    if (spawnPointsIter == presenceGroup.PrimarySpawnPointsByMapID.end() || spawnPointsIter->second.empty() == true)
        return;
    EverQuestPendingKillSpawnAction action;
    action.ActionType = EQ_KILLSPAWN_ACTION_RESPAWNTARGET;
    for (const EverQuestCreaturePresenceSpawnPoint& spawnPoint : spawnPointsIter->second)
        action.RespawnTargetSpawnIDs.push_back(spawnPoint.SpawnID);
    action.RespawnTimeSec = respawnTimeSec;
    ExecuteKillSpawnAction(map, action);
}

bool EverQuestMod::IsPresenceGroupPrimaryGridLoaded(Map* map, const EverQuestCreaturePresenceGroup& presenceGroup)
{
    auto spawnPointsIter = presenceGroup.PrimarySpawnPointsByMapID.find(map->GetId());
    if (spawnPointsIter == presenceGroup.PrimarySpawnPointsByMapID.end())
        return false;
    for (const EverQuestCreaturePresenceSpawnPoint& spawnPoint : spawnPointsIter->second)
        if (map->IsGridLoaded(spawnPoint.PositionX, spawnPoint.PositionY) == true)
            return true;
    return false;
}

void EverQuestMod::ClearCreaturePresenceGroupStateForMap(Map* map)
{
    if (map == nullptr)
        return;
    uint64 mapInstanceKey = GetMapInstanceKey(map);
    std::lock_guard<std::mutex> lock(CreaturePresenceGroupStateMutex);
    PresenceGroupCheckTimerInMSByMapInstanceKeyThenGroupID.erase(mapInstanceKey);
    SuppressedPresenceGroupIDsByMapInstanceKey.erase(mapInstanceKey);
}

void EverQuestMod::UpdateCreaturePresenceGroups(Map* map, uint32 diff)
{
    // The MapInstanced container of an instanceable map holds no spawns itself, only its child instances do
    if (map->Instanceable() == true && map->GetInstanceId() == 0)
        return;
    auto presenceGroupMapIter = CreaturePresenceGroupsByMapID.find(GetOpenWorldMapIDForMapID(map->GetId()));
    if (presenceGroupMapIter == CreaturePresenceGroupsByMapID.end())
        return;
    uint64 mapInstanceKey = GetMapInstanceKey(map);

    for (const EverQuestCreaturePresenceGroup& presenceGroup : presenceGroupMapIter->second)
    {
        // Each instance of a map updates on its own thread, so the check timers are both per-instance and guarded
        {
            std::lock_guard<std::mutex> lock(CreaturePresenceGroupStateMutex);
            int32& checkTimerInMS = PresenceGroupCheckTimerInMSByMapInstanceKeyThenGroupID[mapInstanceKey][presenceGroup.ID];
            checkTimerInMS -= (int32)diff;
            if (checkTimerInMS > 0)
                continue;
            checkTimerInMS = (int32)presenceGroup.CheckIntervalMS;
        }

        if (IsPresenceGroupPrimaryGridLoaded(map, presenceGroup) == false)
            continue;

        bool primaryIsAlive = HasAliveCreatureWithEntryInMap(map, presenceGroup.PrimaryCreatureTemplateID, nullptr);
        bool standInIsAlive = false;
        if (primaryIsAlive == false)
            for (uint32 otherCreatureTemplateID : presenceGroup.OtherCreatureTemplateIDs)
                if (HasAliveCreatureWithEntryInMap(map, otherCreatureTemplateID, nullptr) == true)
                {
                    standInIsAlive = true;
                    break;
                }

        // The primary is back on its feet, so the group is free to hold it down again on the next swap
        if (primaryIsAlive == true)
        {
            std::lock_guard<std::mutex> lock(CreaturePresenceGroupStateMutex);
            SuppressedPresenceGroupIDsByMapInstanceKey[mapInstanceKey].erase(presenceGroup.ID);
            continue;
        }

        // A stand-in took the primary's place.  Push the primary's spawn point out once, or its own respawn timer would eventually put a second member of the group in the room alongside the stand-in
        if (standInIsAlive == true)
        {
            {
                std::lock_guard<std::mutex> lock(CreaturePresenceGroupStateMutex);
                if (SuppressedPresenceGroupIDsByMapInstanceKey[mapInstanceKey].insert(presenceGroup.ID).second == false)
                    continue;
            }
            SetPresenceGroupPrimaryRespawnTime(map, presenceGroup, EQ_PRESENCE_GROUP_SUPPRESSED_RESPAWN_IN_SEC);
            continue;
        }

        // Nothing in the group is standing, so the spot is empty.  Bring the primary back now rather than leaving the zone short an NPC until whatever respawn timer it happens to be carrying runs out
        {
            std::lock_guard<std::mutex> lock(CreaturePresenceGroupStateMutex);
            SuppressedPresenceGroupIDsByMapInstanceKey[mapInstanceKey].erase(presenceGroup.ID);
        }
        SetPresenceGroupPrimaryRespawnTime(map, presenceGroup, 0);
    }
}

static const uint32 VulakRequiredDragonCreatureTemplateIDs[] =
{
    // Lord Koi'Doken is intentionally skipped
    54930,  // Aaryonar
    54923,  // Vyemm
    54928,  // Feshlak
    54993,  // Kreizenn
    54995,  // Nevederia
    54996,  // Mirenilla
};

// Note: Runs at world startup (OnStartup) since the creature spawn tables aren't loaded before that
void EverQuestMod::ResolveVulakRequiredDragonSpawnPoints()
{
    for (uint32 requiredDragonCreatureTemplateID : VulakRequiredDragonCreatureTemplateIDs)
    {
        bool foundSpawnPoint = false;
        for (auto const& creatureDataPair : sObjectMgr->GetAllCreatureData())
        {
            CreatureData const& creatureData = creatureDataPair.second;
            if (creatureData.id != requiredDragonCreatureTemplateID && creatureData.id2 != requiredDragonCreatureTemplateID && creatureData.id3 != requiredDragonCreatureTemplateID)
                continue;

            // Grouped by map so the open world zone and its raid instance copy each track only their own dragons
            VulakRequiredDragonSpawnIDsByMapID[creatureData.mapid].push_back(creatureDataPair.first);
            foundSpawnPoint = true;
        }
        if (foundSpawnPoint == false)
            LOG_ERROR("module.EverQuest", "EverQuestMod::ResolveVulakRequiredDragonSpawnPoints found no spawn points for required dragon creature template {}, so it will not prevent the Vulak`Aerr unlock", requiredDragonCreatureTemplateID);
    }
}

void EverQuestMod::SetVulakLocked(Creature* creature, bool locked)
{
    creature->SetImmuneToPC(locked);
    creature->SetImmuneToNPC(locked);
    creature->SetReactState(locked == true ? REACT_PASSIVE : REACT_AGGRESSIVE);
}

bool EverQuestMod::AreAllVulakRequiredDragonsDead(Map* map)
{
    auto dragonSpawnIDsIt = VulakRequiredDragonSpawnIDsByMapID.find(map->GetId());
    if (dragonSpawnIDsIt == VulakRequiredDragonSpawnIDsByMapID.end() || dragonSpawnIDsIt->second.empty() == true)
        return false;

    // Spawn points only carry a pending respawn time while the creature is dead
    for (ObjectGuid::LowType dragonSpawnID : dragonSpawnIDsIt->second)
        if (map->GetCreatureRespawnTime(dragonSpawnID) == 0)
            return false;
    return true;
}

void EverQuestMod::UpdateVulakLock(Creature* creature, uint32 diff)
{
    if (creature->GetEntry() != EQ_VULAK_CREATURE_TEMPLATE_ID)
        return;
    EverQuestVulakLockState* state = creature->CustomData.GetDefault<EverQuestVulakLockState>(EQ_CREATURE_CUSTOMDATA_VULAKLOCK);
    if (creature->IsAlive() == false)
    {
        state->WasAlive = false;
        return;
    }

    // Lock on fresh spawns (includes respawn and server boots)
    if (state->WasAlive == false)
    {
        state->WasAlive = true;
        state->Unlocked = false;
        state->RecheckRemainingMS = 0;
        creature->SetControlled(true, UNIT_STATE_ROOT);
        SetVulakLocked(creature, true);
    }

    if (state->RecheckRemainingMS > diff)
    {
        state->RecheckRemainingMS -= diff;
        return;
    }
    state->RecheckRemainingMS = EQ_VULAK_LOCK_RECHECK_MS;

    // Permarooted even while unlocked
    if (creature->HasUnitState(UNIT_STATE_ROOT) == false)
        creature->SetControlled(true, UNIT_STATE_ROOT);

    if (state->Unlocked == false)
    {
        if (AreAllVulakRequiredDragonsDead(creature->GetMap()) == true)
        {
            state->Unlocked = true;
            SetVulakLocked(creature, false);
        }
    }
    else if (creature->IsInCombat() == false && AreAllVulakRequiredDragonsDead(creature->GetMap()) == false)
    {
        state->Unlocked = false;
        SetVulakLocked(creature, true);
    }
}

void EverQuestMod::RemoveVulakLockState(Creature* creature)
{
    creature->CustomData.Erase(EQ_CREATURE_CUSTOMDATA_VULAKLOCK);
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

// EQ shouts reach the entire zone, so deliver them to every player on the map instead of using range-limited chat
void EverQuestMod::SendCreatureChatToAllPlayersOnMap(Creature* creature, ChatMsg chatMsg, const string& text)
{
    Map::PlayerList const& mapPlayers = creature->GetMap()->GetPlayers();
    for (Map::PlayerList::const_iterator playerIter = mapPlayers.begin(); playerIter != mapPlayers.end(); ++playerIter)
    {
        Player* mapPlayer = playerIter->GetSource();
        if (mapPlayer == nullptr || mapPlayer->IsInWorld() == false)
            continue;
        WorldPacket data;
        ChatHandler::BuildChatPacket(data, chatMsg, LANG_UNIVERSAL, creature, mapPlayer, text);
        mapPlayer->SendDirectMessage(&data);
    }
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
                SendCreatureChatToAllPlayersOnMap(creature, CHAT_MSG_MONSTER_EMOTE, creature->GetName() + " shouts, '" + formattedText + "'");
            else
                SendCreatureChatToAllPlayersOnMap(creature, CHAT_MSG_MONSTER_YELL, formattedText);
        } break;
        case EQ_CREATURE_EMOTE_TYPE_PROXIMITY:
        {
            creature->TextEmote(formattedText, target);
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

    // Pets (summoned, charmed, or otherwise player controlled) don't make movement sounds since they follow their owner constantly
    if (creature->IsPet() == true || creature->IsGuardian() == true || creature->IsControlledByPlayer() == true)
    {
        EverQuestCreatureMovementSoundState* petState = creature->CustomData.Get<EverQuestCreatureMovementSoundState>(EQ_CREATURE_CUSTOMDATA_MOVEMENTSOUND);
        if (petState != nullptr)
        {
            petState->CurGait = EQ_CREATURE_MOVEMENT_GAIT_NONE;
            petState->ListenersByGUID.clear();
        }
        return;
    }

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

bool EverQuestMod::HasAliveCreatureWithEntryInMap(Map* map, uint32 creatureTemplateID, Creature* ignoreCreature)
{
    uint64 mapInstanceKey = GetMapInstanceKey(map);
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto mapIter = AllLoadedCreaturesByMapInstanceKeyThenCreatureEntryID.find(mapInstanceKey);
    if (mapIter == AllLoadedCreaturesByMapInstanceKeyThenCreatureEntryID.end())
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

void EverQuestMod::ProcessKillSpawnsForCreatureEvent(Creature* eventCreature, Unit* otherUnit, uint8 triggerTypeID)
{
    auto killSpawnIter = CreatureKillSpawnsByTriggerCreatureTemplateID.find(eventCreature->GetEntry());
    if (killSpawnIter == CreatureKillSpawnsByTriggerCreatureTemplateID.end())
        return;
    Map* map = eventCreature->GetMap();

    // Kill spawn rows only exist for the open world copy of a zone, so events inside a raid instance copy match against its open world map
    uint32 mapID = GetOpenWorldMapIDForMapID(map->GetId());
    uint32 eventCreatureLevel = eventCreature->GetLevel();

    // Roll which alternative wins in each alt group (weighted across distinct alt IDs)
    unordered_map<uint32, vector<pair<uint32, float>>> altWeightsByGroup;
    for (const EverQuestCreatureKillSpawn& killSpawn : killSpawnIter->second)
    {
        if (killSpawn.MapID != mapID || killSpawn.TriggerTypeID != triggerTypeID || killSpawn.AltGroup == 0)
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
        if (killSpawn.TriggerTypeID != triggerTypeID)
            continue;
        if (killSpawn.TriggerMinLevel > 0 && eventCreatureLevel < killSpawn.TriggerMinLevel)
            continue;
        if (killSpawn.TriggerMaxLevel > 0 && eventCreatureLevel > killSpawn.TriggerMaxLevel)
            continue;
        if (killSpawn.AltGroup != 0 && chosenAltIDByGroup[killSpawn.AltGroup] != killSpawn.AltID)
            continue;
        if (killSpawn.Chance < 100 && roll_chance_f(killSpawn.Chance) == false)
            continue;
        bool requirementFailed = false;
        for (uint32 creatureTemplateID : killSpawn.RequireDeadCreatureTemplateIDs)
            if (HasAliveCreatureWithEntryInMap(map, creatureTemplateID, eventCreature) == true)
                requirementFailed = true;
        for (uint32 creatureTemplateID : killSpawn.RequireAliveCreatureTemplateIDs)
            if (HasAliveCreatureWithEntryInMap(map, creatureTemplateID, eventCreature) == false)
                requirementFailed = true;
        if (requirementFailed == true)
            continue;

        EverQuestPendingKillSpawnAction action;
        action.ActionType = killSpawn.ActionType;
        action.TargetCreatureTemplateID = killSpawn.TargetCreatureTemplateID;
        action.OnlyIfNotAliveCreatureTemplateID = killSpawn.OnlyIfNotAliveCreatureTemplateID;
        action.AddToHateList = killSpawn.AddToHateList;
        action.RespawnTimeSec = killSpawn.RespawnTimeSec;
        if (otherUnit != nullptr)
            action.KillerGUID = otherUnit->GetGUID();
        if (killSpawn.ActionType == EQ_KILLSPAWN_ACTION_RESPAWNSELF)
        {
            action.RespawnSpawnID = eventCreature->GetSpawnId();
            if (action.RespawnSpawnID == 0)
                continue;
        }
        else if (killSpawn.ActionType == EQ_KILLSPAWN_ACTION_RESPAWNTARGET)
        {
            // Use the spawn rows of the map the event actually happened on, since the raid instance copy has its own spawn rows
            auto targetSpawnIDsIt = killSpawn.TargetSpawnIDsByMapID.find(map->GetId());
            if (targetSpawnIDsIt == killSpawn.TargetSpawnIDsByMapID.end() || targetSpawnIDsIt->second.empty() == true)
                continue;
            action.RespawnTargetSpawnIDs = targetSpawnIDsIt->second;
        }
        else if (killSpawn.SpawnAtCorpse == true)
        {
            action.PositionX = eventCreature->GetPositionX() + killSpawn.PositionX;
            action.PositionY = eventCreature->GetPositionY() + killSpawn.PositionY;
            action.PositionZ = eventCreature->GetPositionZ() + killSpawn.PositionZ;
            action.Orientation = eventCreature->GetOrientation();
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

        // Out-of-combat timer rows consume their delay as the countdown duration, so the actions fire right away
        if (triggerTypeID == EQ_KILLSPAWN_TRIGGER_OOCTIMER)
            delayMS = 0;

        // Non-death triggers fire from inside the event creature's own hook or update, so always run those through the pending queue to keep actions (like despawning that same creature) out of the current call
        if (delayMS == 0 && triggerTypeID != EQ_KILLSPAWN_TRIGGER_DEATH)
            delayMS = 1;
        if (delayMS == 0)
            ExecuteKillSpawnAction(map, action);
        else
        {
            action.RemainingMS = (int32)delayMS;
            std::lock_guard<std::mutex> lock(PendingKillSpawnActionsMutex);
            PendingKillSpawnActionsByMapInstanceKey[GetMapInstanceKey(map)].push_back(action);
        }
    }
}

void EverQuestMod::ExecuteKillSpawnAction(Map* map, EverQuestPendingKillSpawnAction& action)
{
    switch (action.ActionType)
    {
        case EQ_KILLSPAWN_ACTION_SPAWN:
        {
            if (action.OnlyIfNotAliveCreatureTemplateID != 0 && HasAliveCreatureWithEntryInMap(map, action.OnlyIfNotAliveCreatureTemplateID, nullptr) == true)
                return;
            if (IsCreatureBlockedFromInstanceMap(action.TargetCreatureTemplateID, map) == true)
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
                auto mapIter = AllLoadedCreaturesByMapInstanceKeyThenCreatureEntryID.find(GetMapInstanceKey(map));
                if (mapIter != AllLoadedCreaturesByMapInstanceKeyThenCreatureEntryID.end())
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
        case EQ_KILLSPAWN_ACTION_SPAWNOBJECT:
        {
            SpawnReactionGameObject(map->GetCreature(action.MoverGUID), action.GameObjectEntryID,
                action.PositionX, action.PositionY, action.PositionZ, action.GameObjectLifetimeSec);
        } break;
        case EQ_KILLSPAWN_ACTION_WALKPATH:
        {
            Creature* walker = map->GetCreature(action.MoverGUID);
            if (walker == nullptr)
                return;
            const vector<EverQuestCreatureWaypoint>& pathNodes = GetWaypoints(0, action.PathListID);
            if (pathNodes.empty() == true)
                return;
            if (walker->GetExactDist2d(pathNodes[0].X, pathNodes[0].Y) > EQ_REACTION_WALK_PATH_JOIN_DISTANCE)
                return;
            vector<EverQuestPendingKillSpawnAction> noArrivalActions;
            StartReactionGridWalk(walker, action.PathListID, noArrivalActions);
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
        case EQ_KILLSPAWN_ACTION_RESPAWNTARGET:
        {
            for (ObjectGuid::LowType targetSpawnID : action.RespawnTargetSpawnIDs)
            {
                // Spawns stay in the world as dead placeholders while despawned, so work with the object directly if one is loaded
                bool spawnHandledInWorld = false;
                vector<Creature*> respawnCandidates;
                auto spawnIDRange = map->GetCreatureBySpawnIdStore().equal_range(targetSpawnID);
                for (auto spawnIDIter = spawnIDRange.first; spawnIDIter != spawnIDRange.second; ++spawnIDIter)
                {
                    if (spawnIDIter->second->IsAlive() == true)
                        spawnHandledInWorld = true; // Already up, leave it be
                    else
                        respawnCandidates.push_back(spawnIDIter->second);
                }
                for (Creature* respawnCandidate : respawnCandidates)
                {
                    spawnHandledInWorld = true;

                    // No respawn time means to bring it back now, otherwise reschedule the spawn point (persists to the DB)
                    if (action.RespawnTimeSec == 0)
                        respawnCandidate->Respawn(true);
                    else
                    {
                        respawnCandidate->SetRespawnTime(action.RespawnTimeSec);
                        respawnCandidate->SaveRespawnTime();
                    }
                }

                // Dynamic (non-compat) spawns are destroyed while despawned, so reschedule through the map's respawn queue instead.  ProcessRespawns will recreate the creature when appropriate (I hope...)
                if (spawnHandledInWorld == false)
                {
                    time_t respawnTime = GameTime::GetGameTime().count() + (time_t)action.RespawnTimeSec;
                    map->SaveCreatureRespawnTime(targetSpawnID, respawnTime);
                }
            }
        } break;
        default: break;
    }
}

void EverQuestMod::RemoveCreatureKillSpawnCombatWatchState(Creature* creature)
{
    creature->CustomData.Erase(EQ_CREATURE_CUSTOMDATA_KILLSPAWNWATCH);
}

void EverQuestMod::UpdateCreatureKillSpawnCombatWatch(Creature* creature, uint32 diff)
{
    bool hasEvadeRows = EvadeKillSpawnTriggerCreatureTemplateIDs.find(creature->GetEntry()) != EvadeKillSpawnTriggerCreatureTemplateIDs.end();
    auto oocTimerIter = OocTimerKillSpawnDurationMSByCreatureTemplateID.find(creature->GetEntry());
    bool hasOocTimerRows = oocTimerIter != OocTimerKillSpawnDurationMSByCreatureTemplateID.end();
    if (hasEvadeRows == false && hasOocTimerRows == false)
        return;
    if (creature->IsPet() == true || creature->IsControlledByPlayer() == true)
        return;
    if (creature->IsAlive() == false)
    {
        RemoveCreatureKillSpawnCombatWatchState(creature);
        return;
    }
    EverQuestCreatureKillSpawnWatchState* state = creature->CustomData.GetDefault<EverQuestCreatureKillSpawnWatchState>(EQ_CREATURE_CUSTOMDATA_KILLSPAWNWATCH);
    bool isInCombat = creature->IsInCombat();

    if (hasOocTimerRows == true)
    {
        uint32 oocTimerDurationMS = oocTimerIter->second;
        if (isInCombat == true || state->OocTimerRemainingMS == 0)
            state->OocTimerRemainingMS = oocTimerDurationMS;
        else if (state->OocTimerRemainingMS > diff)
            state->OocTimerRemainingMS -= diff;
        else
        {
            state->OocTimerRemainingMS = oocTimerDurationMS;
            ProcessKillSpawnsForCreatureEvent(creature, nullptr, EQ_KILLSPAWN_TRIGGER_OOCTIMER);
        }
    }

    if (hasEvadeRows == true)
    {
        if (isInCombat == true)
            state->WasInCombat = true;
        else if (state->WasInCombat == true)
        {
            state->WasInCombat = false;
            ProcessKillSpawnsForCreatureEvent(creature, nullptr, EQ_KILLSPAWN_TRIGGER_EVADE);
        }
    }
}

void EverQuestMod::UpdatePendingKillSpawnActions(Map* map, uint32 diff)
{
    vector<EverQuestPendingKillSpawnAction> dueActions;
    {
        std::lock_guard<std::mutex> lock(PendingKillSpawnActionsMutex);
        auto pendingIter = PendingKillSpawnActionsByMapInstanceKey.find(GetMapInstanceKey(map));
        if (pendingIter == PendingKillSpawnActionsByMapInstanceKey.end())
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
            PendingKillSpawnActionsByMapInstanceKey.erase(pendingIter);
    }
    for (EverQuestPendingKillSpawnAction& action : dueActions)
        ExecuteKillSpawnAction(map, action);
}

void EverQuestMod::TriggerQuestKillSpawn(Map* map, const EverQuestQuestReaction& questReaction)
{
    EverQuestTriggeredQuestKillSpawn triggeredKillSpawn;
    triggeredKillSpawn.TriggerCreatureTemplateID = questReaction.QuestgiverCreatureTemplateID;
    triggeredKillSpawn.TargetCreatureTemplateID = questReaction.CreatureTemplateID;
    triggeredKillSpawn.PositionX = questReaction.PositionX;
    triggeredKillSpawn.PositionY = questReaction.PositionY;
    triggeredKillSpawn.PositionZ = questReaction.PositionZ;
    triggeredKillSpawn.Orientation = questReaction.Orientation;

    // Repeat turn-ins only trigger one spawn, like the flag in the EQ quest scripts
    uint64 mapInstanceKey = GetMapInstanceKey(map);
    std::lock_guard<std::mutex> lock(PendingKillSpawnActionsMutex);
    for (EverQuestTriggeredQuestKillSpawn& existing : TriggeredQuestKillSpawnsByMapInstanceKey[mapInstanceKey])
        if (existing.TriggerCreatureTemplateID == triggeredKillSpawn.TriggerCreatureTemplateID && existing.TargetCreatureTemplateID == triggeredKillSpawn.TargetCreatureTemplateID)
            return;
    TriggeredQuestKillSpawnsByMapInstanceKey[mapInstanceKey].push_back(triggeredKillSpawn);
}

void EverQuestMod::ProcessTriggeredQuestKillSpawnsForCreatureDeath(Creature* deadCreature, Unit* killer)
{
    vector<EverQuestPendingKillSpawnAction> dueActions;
    {
        std::lock_guard<std::mutex> lock(PendingKillSpawnActionsMutex);
        auto triggeredIter = TriggeredQuestKillSpawnsByMapInstanceKey.find(GetMapInstanceKey(deadCreature->GetMap()));
        if (triggeredIter == TriggeredQuestKillSpawnsByMapInstanceKey.end())
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
            TriggeredQuestKillSpawnsByMapInstanceKey.erase(triggeredIter);
    }
    for (EverQuestPendingKillSpawnAction& action : dueActions)
        ExecuteKillSpawnAction(deadCreature->GetMap(), action);
}

void EverQuestMod::EnqueuePendingKillSpawnAction(Map* map, EverQuestPendingKillSpawnAction& action)
{
    std::lock_guard<std::mutex> lock(PendingKillSpawnActionsMutex);
    PendingKillSpawnActionsByMapInstanceKey[GetMapInstanceKey(map)].push_back(action);
}

Creature* EverQuestMod::GetNearestLoadedCreatureWithEntryID(Map* map, uint32 entryID, WorldObject* referenceObject)
{
    if (referenceObject == nullptr)
        return nullptr;
    vector<Creature*> loadedCreatures = GetLoadedCreaturesWithEntryID(map, entryID);
    Creature* nearestCreature = nullptr;
    float nearestDistance = 0;
    for (Creature* creature : loadedCreatures)
    {
        if (creature == nullptr || creature->IsAlive() == false)
            continue;
        float distance = creature->GetExactDist(referenceObject);
        if (nearestCreature == nullptr || distance < nearestDistance)
        {
            nearestCreature = creature;
            nearestDistance = distance;
        }
    }
    return nearestCreature;
}

void EverQuestMod::SpeakReactionText(Creature* creature, uint8 actionType, const string& text, Player* listener)
{
    if (creature == nullptr || text.empty() == true)
        return;
    string formattedText = text;
    if (listener != nullptr)
        formattedText = FormatGossipTextForPlayer(listener, text);
    if (actionType == EQ_KILLSPAWN_ACTION_EMOTE)
    {
        // Monster emote text renders raw on the client (no speaker name), so bake the name in
        creature->TextEmote(creature->GetName() + " " + formattedText, listener);
    }
    else if (actionType == EQ_KILLSPAWN_ACTION_YELL)
        creature->Yell(formattedText, LANG_UNIVERSAL, listener);
    else
        creature->Say(formattedText, LANG_UNIVERSAL, listener);
}

bool EverQuestMod::IsCreatureInReactionWalk(ObjectGuid creatureGUID)
{
    // This runs from the per-creature AI tick, so the common case of nothing walking must not take the lock at all
    if (ReactionWalkCreatureCount.load() == 0)
        return false;
    std::lock_guard<std::mutex> lock(PendingArrivalActionsMutex);
    return ReactionWalkCreatureGUIDs.find(creatureGUID) != ReactionWalkCreatureGUIDs.end();
}

bool EverQuestMod::StartReactionWalk(Creature* creature, float x, float y, float z, float orientation, bool hasOrientation, bool isRun, vector<EverQuestPendingKillSpawnAction>& actionsOnArrival)
{
    if (creature == nullptr || creature->IsAlive() == false)
        return false;

    // Talking to a creature makes the core pause its movement before any script gets a say, so a creature on a reaction walk is made non-interactable for the trip and gets its flags back when it arrives
    uint32 savedNpcFlags = (uint32)creature->GetCreatureTemplate()->npcflag;
    {
        std::lock_guard<std::mutex> lock(PendingArrivalActionsMutex);
        vector<EverQuestPendingArrivalAction>& watchers = PendingArrivalActionsByMapInstanceKey[GetMapInstanceKey(creature->GetMap())];
        for (size_t i = watchers.size(); i > 0; --i)
            if (watchers[i - 1].MoverGUID == creature->GetGUID())
            {
                // A walk that replaces another inherits the flags the first one put aside, never the blanked ones
                if (watchers[i - 1].HasSavedNpcFlags == true)
                    savedNpcFlags = watchers[i - 1].SavedNpcFlags;
                watchers.erase(watchers.begin() + (i - 1));
            }
        ReactionWalkCreatureGUIDs.erase(creature->GetGUID());

        EverQuestPendingArrivalAction watcher;
        watcher.MoverGUID = creature->GetGUID();
        watcher.DestinationX = x;
        watcher.DestinationY = y;
        watcher.DestinationZ = z;
        watcher.DestinationOrientation = orientation;
        watcher.HasDestinationOrientation = hasOrientation;
        watcher.SavedNpcFlags = savedNpcFlags;
        watcher.HasSavedNpcFlags = true;
        watcher.ActionsOnArrival = actionsOnArrival;
        watchers.push_back(watcher);
        ReactionWalkCreatureGUIDs.insert(creature->GetGUID());
        ReactionWalkCreatureCount.store((uint32)ReactionWalkCreatureGUIDs.size());
    }
    creature->ReplaceAllNpcFlags(UNIT_NPC_FLAG_NONE);

    // Take the creature off whatever it was doing so its normal waypoint or roaming generator does not fight the walk
    creature->SetWalk(isRun == false);
    creature->GetMotionMaster()->Clear();
    creature->GetMotionMaster()->MovePoint(EQ_REACTION_WALK_POINT_ID, x, y, z);
    return true;
}

void EverQuestMod::SpawnReactionGameObject(Creature* summoner, uint32 gameObjectEntryID, float x, float y, float z, uint32 lifetimeSec)
{
    if (summoner == nullptr || gameObjectEntryID == 0)
        return;
    if (lifetimeSec == 0)
        lifetimeSec = EQ_REACTION_OBJECT_DEFAULT_LIFETIME_SEC;
    summoner->SummonGameObject(gameObjectEntryID, x, y, z, 0, 0, 0, 0, 0, lifetimeSec, true, GO_SUMMON_TIMED_DESPAWN);
}

bool EverQuestMod::StartReactionGridWalk(Creature* creature, uint32 pathListID, vector<EverQuestPendingKillSpawnAction>& actionsOnArrival)
{
    if (creature == nullptr || creature->IsAlive() == false)
        return false;

    // Reaction path lists live under map 0, since every instance copy of a zone walks the same nodes
    const vector<EverQuestCreatureWaypoint>& pathNodes = GetWaypoints(0, pathListID);
    if (pathNodes.size() < 2)
    {
        LOG_ERROR("module", "EverQuest: reaction path list {} holds no nodes, so creature {} has nowhere to walk", pathListID, creature->GetEntry());
        return false;
    }

    size_t startIndex = 0;
    float nearestDistanceSquared = -1.0f;
    for (size_t i = 0; i < pathNodes.size(); i++)
    {
        float nodeDeltaX = pathNodes[i].X - creature->GetPositionX();
        float nodeDeltaY = pathNodes[i].Y - creature->GetPositionY();
        float nodeDistanceSquared = (nodeDeltaX * nodeDeltaX) + (nodeDeltaY * nodeDeltaY);
        if (nearestDistanceSquared < 0.0f || nodeDistanceSquared < nearestDistanceSquared)
        {
            nearestDistanceSquared = nodeDistanceSquared;
            startIndex = i;
        }
    }
    if (nearestDistanceSquared > (EQ_REACTION_WALK_PATH_JOIN_DISTANCE * EQ_REACTION_WALK_PATH_JOIN_DISTANCE))
        startIndex = 0;
    if (startIndex + 1 >= pathNodes.size())
        startIndex = 0;

    Movement::PointsArray pathPoints;
    float travelDistance = 0.0f;
    float priorX = creature->GetPositionX();
    float priorY = creature->GetPositionY();
    float priorZ = creature->GetPositionZ();
    for (size_t i = startIndex; i < pathNodes.size(); i++)
    {
        const EverQuestCreatureWaypoint& pathNode = pathNodes[i];
        if (pathPoints.empty() == false && pathNode.X == priorX && pathNode.Y == priorY && pathNode.Z == priorZ)
            continue;
        pathPoints.push_back(G3D::Vector3(pathNode.X, pathNode.Y, pathNode.Z));
        float deltaX = pathNode.X - priorX;
        float deltaY = pathNode.Y - priorY;
        float deltaZ = pathNode.Z - priorZ;
        travelDistance += std::sqrt((deltaX * deltaX) + (deltaY * deltaY) + (deltaZ * deltaZ));
        priorX = pathNode.X;
        priorY = pathNode.Y;
        priorZ = pathNode.Z;
    }
    if (pathPoints.size() < 2)
    {
        LOG_ERROR("module", "EverQuest: reaction path list {} left creature {} with nothing to walk", pathListID, creature->GetEntry());
        return false;
    }

    float walkSpeed = creature->GetSpeed(MOVE_WALK);
    if (walkSpeed < 0.1f)
        walkSpeed = 0.1f;
    uint32 timeoutMS = EQ_REACTION_WALK_TIMEOUT_MS + static_cast<uint32>((travelDistance / walkSpeed) * 2000.0f);
    if (timeoutMS > EQ_REACTION_WALK_MAX_TIMEOUT_MS)
        timeoutMS = EQ_REACTION_WALK_MAX_TIMEOUT_MS;

    const EverQuestCreatureWaypoint& finalNode = pathNodes[pathNodes.size() - 1];

    // Talking to a creature makes the core pause its movement before any script gets a say, so a creature on a reaction walk is made non-interactable for the trip and gets its flags back when it arrives
    uint32 savedNpcFlags = (uint32)creature->GetCreatureTemplate()->npcflag;
    {
        // A second walk on the same creature replaces the first, otherwise the two sets of queued actions would race
        std::lock_guard<std::mutex> lock(PendingArrivalActionsMutex);
        vector<EverQuestPendingArrivalAction>& watchers = PendingArrivalActionsByMapInstanceKey[GetMapInstanceKey(creature->GetMap())];
        for (size_t i = watchers.size(); i > 0; --i)
            if (watchers[i - 1].MoverGUID == creature->GetGUID())
            {
                // A walk that replaces another inherits the flags the first one put aside, never the blanked ones
                if (watchers[i - 1].HasSavedNpcFlags == true)
                    savedNpcFlags = watchers[i - 1].SavedNpcFlags;
                watchers.erase(watchers.begin() + (i - 1));
            }
        ReactionWalkCreatureGUIDs.erase(creature->GetGUID());

        EverQuestPendingArrivalAction watcher;
        watcher.MoverGUID = creature->GetGUID();
        watcher.DestinationX = finalNode.X;
        watcher.DestinationY = finalNode.Y;
        watcher.DestinationZ = finalNode.Z;
        watcher.TimeoutMS = timeoutMS;
        watcher.SavedNpcFlags = savedNpcFlags;
        watcher.HasSavedNpcFlags = true;
        watcher.ActionsOnArrival = actionsOnArrival;
        watchers.push_back(watcher);
        ReactionWalkCreatureGUIDs.insert(creature->GetGUID());
        ReactionWalkCreatureCount.store((uint32)ReactionWalkCreatureGUIDs.size());
    }
    creature->ReplaceAllNpcFlags(UNIT_NPC_FLAG_NONE);

    // Take the creature off whatever it was doing so its normal waypoint or roaming generator does not fight the walk
    creature->SetWalk(true);
    creature->GetMotionMaster()->Clear();
    creature->GetMotionMaster()->MoveSplinePath(&pathPoints, FORCED_MOVEMENT_WALK);
    return true;
}

void EverQuestMod::UpdatePendingArrivalActions(Map* map, uint32 diff)
{
    vector<EverQuestPendingArrivalAction> arrivedWatchers;
    vector<pair<ObjectGuid, uint32>> npcFlagRestores;
    {
        std::lock_guard<std::mutex> lock(PendingArrivalActionsMutex);
        auto watcherIter = PendingArrivalActionsByMapInstanceKey.find(GetMapInstanceKey(map));
        if (watcherIter == PendingArrivalActionsByMapInstanceKey.end())
            return;
        vector<EverQuestPendingArrivalAction>& watchers = watcherIter->second;
        for (size_t i = watchers.size(); i > 0; --i)
        {
            EverQuestPendingArrivalAction& watcher = watchers[i - 1];
            watcher.ElapsedMS += diff;

            Creature* mover = map->GetCreature(watcher.MoverGUID);
            bool moverIsGone = (mover == nullptr || mover->IsInWorld() == false || mover->IsAlive() == false);

            // Arriving finishes the walk.  A creature that died, despawned, or got stuck long enough drops its queued actions on the floor rather than firing them somewhere it never reached
            bool hasArrived = false;
            if (moverIsGone == false)
                hasArrived = mover->GetExactDist2d(watcher.DestinationX, watcher.DestinationY) <= EQ_REACTION_WALK_ARRIVE_DISTANCE;
            bool hasTimedOut = watcher.ElapsedMS >= watcher.TimeoutMS;

            if (moverIsGone == false && hasArrived == false && hasTimedOut == false)
                continue;

            ReactionWalkCreatureGUIDs.erase(watcher.MoverGUID);
            ReactionWalkCreatureCount.store((uint32)ReactionWalkCreatureGUIDs.size());
            if (watcher.HasSavedNpcFlags == true)
                npcFlagRestores.push_back(std::make_pair(watcher.MoverGUID, watcher.SavedNpcFlags));
            if (hasArrived == true)
                arrivedWatchers.push_back(watcher);
            watchers.erase(watchers.begin() + (i - 1));
        }
        if (watchers.empty() == true)
            PendingArrivalActionsByMapInstanceKey.erase(watcherIter);
    }

    // The walk is over either way, so the creature can be talked to again before anything it has to say fires
    for (size_t i = 0; i < npcFlagRestores.size(); i++)
    {
        Creature* restoredCreature = map->GetCreature(npcFlagRestores[i].first);
        if (restoredCreature == nullptr)
            continue;
        uint32 restoredNpcFlags = npcFlagRestores[i].second;

        // A creature that walked somewhere as part of a sequence must not offer the quest that started it again until it is home, or the player can hand in a second time and send it walking from the wrong place
        if (restoredCreature->GetExactDist2d(restoredCreature->GetHomePosition().GetPositionX(),
            restoredCreature->GetHomePosition().GetPositionY()) > EQ_REACTION_WALK_HOME_DISTANCE)
            restoredNpcFlags = restoredNpcFlags & ~((uint32)UNIT_NPC_FLAG_QUESTGIVER);
        restoredCreature->ReplaceAllNpcFlags((NPCFlags)restoredNpcFlags);
    }

    for (EverQuestPendingArrivalAction& watcher : arrivedWatchers)
    {
        Creature* mover = map->GetCreature(watcher.MoverGUID);
        if (mover != nullptr)
        {
            mover->GetMotionMaster()->Clear();
            if (watcher.HasDestinationOrientation == true)
                mover->SetFacingTo(watcher.DestinationOrientation);
        }
        for (EverQuestPendingKillSpawnAction& action : watcher.ActionsOnArrival)
        {
            // Anything positioned relative to the walker resolves against where it actually ended up
            if (mover != nullptr)
            {
                if (action.UseMoverPositionX == true)
                    action.PositionX = mover->GetPositionX();
                if (action.UseMoverPositionY == true)
                    action.PositionY = mover->GetPositionY();
                if (action.UseMoverPositionZ == true)
                    action.PositionZ = mover->GetPositionZ();
                if (action.UseMoverOrientation == true)
                    action.Orientation = mover->GetOrientation();
            }
            if (action.ActionType == EQ_KILLSPAWN_ACTION_SAY || action.ActionType == EQ_KILLSPAWN_ACTION_EMOTE || action.ActionType == EQ_KILLSPAWN_ACTION_YELL)
            {
                Creature* speaker = (action.SpeakerGUID ? map->GetCreature(action.SpeakerGUID) : mover);
                Player* listener = (action.ListenerGUID ? ObjectAccessor::GetPlayer(map, action.ListenerGUID) : nullptr);
                SpeakReactionText(speaker, action.ActionType, action.SayText, listener);
                continue;
            }
            if (action.ActionType == EQ_KILLSPAWN_ACTION_ATTACKPLAYER)
            {
                Player* listener = (action.ListenerGUID ? ObjectAccessor::GetPlayer(map, action.ListenerGUID) : nullptr);
                if (listener != nullptr)
                    MakeCreatureAttackPlayer(action.TargetCreatureTemplateID, map, listener);
                continue;
            }
            if (action.ActionType == EQ_KILLSPAWN_ACTION_DESPAWN && mover != nullptr && action.TargetCreatureTemplateID == mover->GetEntry())
            {
                // Only the copy that made the walk goes away, not every creature of its type in the zone
                action.DespawnNearestToPositionOnly = true;
                action.PositionX = mover->GetPositionX();
                action.PositionY = mover->GetPositionY();
                action.PositionZ = mover->GetPositionZ();
            }
            if (action.MoverGUID == ObjectGuid::Empty)
                action.MoverGUID = watcher.MoverGUID;
            // An arrival action with a delay waits its turn in the pending queue, which is how EQ's pause at the end of a leg becomes the gap before the creature heads back
            if (action.RemainingMS > 0)
                EnqueuePendingKillSpawnAction(map, action);
            else
                ExecuteKillSpawnAction(map, action);
        }
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

bool EverQuestMod::IsItemTemplateIDAnEQItemTemplateID(uint32 itemTemplateID)
{
    return itemTemplateID >= ConfigSystemItemTemplateIDMin && itemTemplateID <= ConfigSystemItemTemplateIDMax;
}

static uint64 GetGearSwapLookupKey(uint32 inventoryType, uint32 itemClassID, uint32 itemSubClassID, uint32 eqClassID)
{
    // A gear swap pool is keyed on the properties of the WoW item being worn, plus the EverQuest class of the wearer
    return ((uint64)inventoryType << 48) | ((uint64)itemClassID << 32) | ((uint64)itemSubClassID << 16) | (uint64)eqClassID;
}

static uint32 GetGearSwapStableRoll(uint32 seedA, uint32 seedB, uint32 seedC)
{
    // Picks are rolled from the seeds instead of urand so that a given character keeps the same disguise across every
    // values update, since a fresh roll per update would make the gear visibly churn
    // Note: Claude came up with these values, but they seem to work well but if an issue do analysis on how to make them proper
    uint64 mixedValue = ((uint64)seedA + 0x9E3779B97F4A7C15ull) * 0xBF58476D1CE4E5B9ull;
    mixedValue ^= ((uint64)seedB + 0x165667B19E3779F9ull) * 0x94D049BB133111EBull;
    mixedValue ^= ((uint64)seedC + 0x27D4EB2F165667C5ull) * 0xC2B2AE3D27D4EB4Full;
    mixedValue ^= mixedValue >> 31;
    mixedValue *= 0xFF51AFD7ED558CCDull;
    mixedValue ^= mixedValue >> 29;
    return (uint32)(mixedValue & 0x7FFFFFFFull);
}

static bool IsGearSwapRenderedEquipSlot(uint8 equipSlot)
{
    // Only the slots that actually draw on a character get swapped, so slots like rings still inspect correctly
    switch (equipSlot)
    {
        case EQUIPMENT_SLOT_HEAD:
        case EQUIPMENT_SLOT_SHOULDERS:
        case EQUIPMENT_SLOT_BODY:
        case EQUIPMENT_SLOT_CHEST:
        case EQUIPMENT_SLOT_WAIST:
        case EQUIPMENT_SLOT_LEGS:
        case EQUIPMENT_SLOT_FEET:
        case EQUIPMENT_SLOT_WRISTS:
        case EQUIPMENT_SLOT_HANDS:
        case EQUIPMENT_SLOT_BACK:
        case EQUIPMENT_SLOT_MAINHAND:
        case EQUIPMENT_SLOT_OFFHAND:
        case EQUIPMENT_SLOT_RANGED:
        case EQUIPMENT_SLOT_TABARD: return true;
        default: return false;
    }
}

void EverQuestMod::LoadItemWoWToEQSwapData()
{
    GearSwapCandidatesByLookupKey.clear();

    QueryResult queryResult = WorldDatabase.Query("SELECT InventoryType, ItemClassID, ItemSubClassID, EQClassID, ItemTemplateID, ItemDisplayID FROM mod_everquest_item_wow_to_eq_swap;");
    if (!queryResult)
    {
        LOG_INFO("module.EverQuest", "EverQuestMod::LoadItemWoWToEQSwapData found no mod_everquest_item_wow_to_eq_swap rows, so .eqhidewowgear will hide WoW gear instead of swapping it for EverQuest looks");
        return;
    }
    uint32 candidateCount = 0;
    do
    {
        Field* fields = queryResult->Fetch();
        uint32 inventoryType = fields[0].Get<uint32>();
        uint32 itemClassID = fields[1].Get<uint32>();
        uint32 itemSubClassID = fields[2].Get<uint32>();
        uint32 eqClassID = fields[3].Get<uint32>();
        EverQuestGearSwapCandidate swapCandidate;
        swapCandidate.ItemTemplateID = fields[4].Get<uint32>();
        swapCandidate.ItemDisplayID = fields[5].Get<uint32>();
        GearSwapCandidatesByLookupKey[GetGearSwapLookupKey(inventoryType, itemClassID, itemSubClassID, eqClassID)].push_back(swapCandidate);
        ++candidateCount;
    } while (queryResult->NextRow());
    LOG_INFO("module.EverQuest", "EverQuestMod::LoadItemWoWToEQSwapData loaded {} gear swap candidates across {} pools", candidateCount, (uint32)GearSwapCandidatesByLookupKey.size());
}

bool EverQuestMod::TryGetGearSwapPlayerState(Player* player, bool& hideWoWGear, uint8& secondEQClassID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto controllerDataIt = ActivePlayerClassControllerDataByGUID.find(player->GetGUID());
    if (controllerDataIt == ActivePlayerClassControllerDataByGUID.end())
        return false;
    hideWoWGear = controllerDataIt->second.HideWoWGear;
    secondEQClassID = controllerDataIt->second.CurrentSecondClass;
    return true;
}

uint32 EverQuestMod::GetGearSwapItemTemplateIDForWornItem(uint32 wearingPlayerGUIDCounter, uint8 rolledEQClassID, uint8 fallbackEQClassID, uint8 equipSlot, uint32 itemTemplateID)
{
    ItemTemplate const* itemProto = sObjectMgr->GetItemTemplate(itemTemplateID);
    if (itemProto == nullptr)
        return 0;

    // The rolled class is used when it has a pool for this item, and otherwise the other class fills in
    const vector<EverQuestGearSwapCandidate>* swapCandidates = nullptr;
    auto swapCandidatesItr = GearSwapCandidatesByLookupKey.find(GetGearSwapLookupKey(itemProto->InventoryType, itemProto->Class, itemProto->SubClass, rolledEQClassID));
    if (swapCandidatesItr != GearSwapCandidatesByLookupKey.end())
        swapCandidates = &swapCandidatesItr->second;
    else if (fallbackEQClassID != EQ_EQCLASS_NONE && fallbackEQClassID != rolledEQClassID)
    {
        swapCandidatesItr = GearSwapCandidatesByLookupKey.find(GetGearSwapLookupKey(itemProto->InventoryType, itemProto->Class, itemProto->SubClass, fallbackEQClassID));
        if (swapCandidatesItr != GearSwapCandidatesByLookupKey.end())
            swapCandidates = &swapCandidatesItr->second;
    }

    // Anything with no EverQuest stand-in at all (cloaks, helms, tabards, wands) simply does not render
    if (swapCandidates == nullptr || swapCandidates->empty() == true)
        return 0;

    uint32 candidateIndex = GetGearSwapStableRoll(wearingPlayerGUIDCounter, (uint32)equipSlot, itemTemplateID) % (uint32)swapCandidates->size();
    return swapCandidates->at(candidateIndex).ItemTemplateID;
}

void EverQuestMod::PatchVisibleGearFieldsInValuesUpdate(Player* wearingPlayer, ByteBuffer& valuesUpdateBuf, BuildValuesCachePosPointers& posPointers)
{
    // The wearer's classes are resolved once up front instead of per slot, so building a values update takes RuntimeStateMutex a single time no matter how many pieces of gear travel in it
    uint32 wearingPlayerGUIDCounter = wearingPlayer->GetGUID().GetCounter();
    uint8 primaryEQClassID = GetClassMapForWOWClassID(wearingPlayer->getClass()).EQClassIDBase;
    bool wearingPlayerHideWoWGear = false;
    uint8 secondaryEQClassID = EQ_EQCLASS_NONE;
    TryGetGearSwapPlayerState(wearingPlayer, wearingPlayerHideWoWGear, secondaryEQClassID);

    // Either of the character's two EverQuest classes can drive the look, and which one it is stays fixed per character
    uint8 rolledEQClassID = primaryEQClassID;
    uint8 fallbackEQClassID = secondaryEQClassID;
    if (secondaryEQClassID != EQ_EQCLASS_NONE && secondaryEQClassID != primaryEQClassID && (GetGearSwapStableRoll(wearingPlayerGUIDCounter, 0, 0) % 2) == 1)
    {
        rolledEQClassID = secondaryEQClassID;
        fallbackEQClassID = primaryEQClassID;
    }

    for (uint8 equipSlot = 0; equipSlot < EQUIPMENT_SLOT_END; ++equipSlot)
    {
        uint16 entryFieldIndex = (uint16)(PLAYER_VISIBLE_ITEM_1_ENTRYID + (equipSlot * 2));
        unordered_map<uint16, uint32>::const_iterator entryPosItr = posPointers.other.find(entryFieldIndex);
        if (entryPosItr != posPointers.other.end() && IsGearSwapRenderedEquipSlot(equipSlot) == true)
        {
            uint32 visibleItemID = valuesUpdateBuf.read<uint32>(entryPosItr->second);
            if (visibleItemID != 0 && IsItemTemplateIDAnEQItemTemplateID(visibleItemID) == false)
                valuesUpdateBuf.put(entryPosItr->second, GetGearSwapItemTemplateIDForWornItem(wearingPlayerGUIDCounter, rolledEQClassID, fallbackEQClassID, equipSlot, visibleItemID));
        }

        // EverQuest has no enchant visuals (weapon glows, shoulder inscriptions), so clear any enchant being sent
        unordered_map<uint16, uint32>::const_iterator enchantPosItr = posPointers.other.find((uint16)(entryFieldIndex + 1));
        if (enchantPosItr != posPointers.other.end())
        {
            if (valuesUpdateBuf.read<uint32>(enchantPosItr->second) != 0)
                valuesUpdateBuf.put(enchantPosItr->second, (uint32)0);
        }
    }
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

bool EverQuestMod::TryGetEQClassFilterBitsForPlayer(Player* player, uint32& eqClassBits)
{
    eqClassBits = 0;
    if (player == nullptr)
        return false;
    const EverQuestClassMap classMap = GetClassMapForWOWClassID(player->getClass());
    if (classMap.EQClassIDBase == 0)
        return false;
    eqClassBits = 1u << (classMap.EQClassIDBase - 1);
    uint8 secondEQClass = GetCurrentSecondEQClassForPlayer(player);
    if (secondEQClass != EQ_EQCLASS_NONE)
        eqClassBits |= 1u << (secondEQClass - 1);
    return true;
}

bool EverQuestMod::IsItemEQClassAllowedForClassBits(uint32 itemTemplateID, uint32 eqClassBits, bool eqClassBitsKnown)
{
    unordered_map<uint32, EverQuestItemTemplate>::const_iterator itemTemplateItr = ItemTemplatesByEntryID.find(itemTemplateID);
    if (itemTemplateItr == ItemTemplatesByEntryID.end())
        return true;
    uint32 allowedEQClassMask = itemTemplateItr->second.AllowedEQClassMask;
    if (allowedEQClassMask == 0)
        return true;
    if (eqClassBitsKnown == false)
        return true;
    return (allowedEQClassMask & eqClassBits) != 0;
}

void EverQuestMod::SetAuctionUsableFilterActiveForPlayer(ObjectGuid playerGUID, bool active)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    if (active == true)
        PlayersWithAuctionUsableFilterActive.insert(playerGUID);
    else
        PlayersWithAuctionUsableFilterActive.erase(playerGUID);
}

bool EverQuestMod::IsAuctionUsableFilterActiveForPlayer(ObjectGuid playerGUID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    return PlayersWithAuctionUsableFilterActive.find(playerGUID) != PlayersWithAuctionUsableFilterActive.end();
}

class EverQuestAuctionCategory
{
public:
    uint32 ItemClassID = 0;
    uint32 SubClassCount = 0;
    uint32 SubClassIDs[17] = { 0 };
};

static const uint32 AuctionCategoryCount = 12;
static const EverQuestAuctionCategory AuctionCategories[AuctionCategoryCount] =
{
    {  2, 17, {  0,  1,  2,  3,  4,  5,  6,  7,  8, 10, 13, 14, 15, 16, 18, 19, 20 } },  // Weapon
    {  4, 10, {  0,  1,  2,  3,  4,  6,  7,  8,  9, 10 } },                              // Armor
    {  1,  9, {  0,  1,  2,  3,  4,  5,  6,  7,  8 } },                                  // Container
    {  0,  8, {  5,  1,  2,  3,  7,  6,  4,  8 } },                                      // Consumable
    { 16, 10, {  1,  2,  3,  4,  5,  6,  7,  8,  9, 11 } },                              // Glyph
    {  7, 15, { 10,  5,  6,  7,  8,  9, 12,  4,  1,  3,  2, 13, 11, 14, 15 } },          // Trade Goods
    {  6,  2, {  2,  3 } },                                                              // Projectile
    { 11,  2, {  2,  3 } },                                                              // Quiver
    {  9, 12, {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11 } },                      // Recipe
    {  3,  9, {  0,  1,  2,  3,  4,  5,  6,  7,  8 } },                                  // Gem
    { 15,  6, {  0,  1,  2,  3,  4,  5 } },                                              // Miscellaneous
    { 12,  0, { } },                                                                     // Quest (no subcategories)
};

static size_t GetAuctionRealmFilterPayloadLength()
{
    // One character for the global fallback, then per category one character for the category itself followed by one for each of its subcategories
    size_t payloadLength = 1;
    for (uint32 categoryIndex = 0; categoryIndex < AuctionCategoryCount; ++categoryIndex)
        payloadLength += 1 + AuctionCategories[categoryIndex].SubClassCount;
    return payloadLength;
}

static bool TryParseAuctionRealmFilterPayload(const string& payload, EverQuestAuctionRealmFilter& filter)
{
    if (payload.size() != GetAuctionRealmFilterPayloadLength())
        return false;
    for (size_t characterIndex = 0; characterIndex < payload.size(); ++characterIndex)
        if (payload[characterIndex] < '0' || payload[characterIndex] > '2')
            return false;

    EverQuestAuctionRealmFilter parsedFilter;
    parsedFilter.Payload = payload;
    parsedFilter.GlobalMode = static_cast<uint8>(payload[0] - '0');
    parsedFilter.FiltersAnything = (parsedFilter.GlobalMode != EQ_AUCTION_REALM_BOTH);

    size_t readPosition = 1;
    for (uint32 categoryIndex = 0; categoryIndex < AuctionCategoryCount; ++categoryIndex)
    {
        const EverQuestAuctionCategory& category = AuctionCategories[categoryIndex];
        uint8 categoryMode = static_cast<uint8>(payload[readPosition] - '0');
        readPosition++;
        parsedFilter.ModeByItemClassID[category.ItemClassID] = categoryMode;
        if (categoryMode != EQ_AUCTION_REALM_BOTH)
            parsedFilter.FiltersAnything = true;
        for (uint32 subClassIndex = 0; subClassIndex < category.SubClassCount; ++subClassIndex)
        {
            uint8 subClassMode = static_cast<uint8>(payload[readPosition] - '0');
            readPosition++;
            parsedFilter.ModeByItemClassAndSubClassKey[(static_cast<uint64>(category.ItemClassID) << 32) | static_cast<uint64>(category.SubClassIDs[subClassIndex])] = subClassMode;
            if (subClassMode != EQ_AUCTION_REALM_BOTH)
                parsedFilter.FiltersAnything = true;
        }
    }

    filter = parsedFilter;
    return true;
}

static uint8 GetAuctionRealmModeForItemClassAndSubClass(const EverQuestAuctionRealmFilter& filter, uint32 itemClassID, uint32 itemSubClassID)
{
    unordered_map<uint64, uint8>::const_iterator subClassItr = filter.ModeByItemClassAndSubClassKey.find((static_cast<uint64>(itemClassID) << 32) | static_cast<uint64>(itemSubClassID));
    if (subClassItr != filter.ModeByItemClassAndSubClassKey.end())
        return subClassItr->second;
    unordered_map<uint32, uint8>::const_iterator classItr = filter.ModeByItemClassID.find(itemClassID);
    if (classItr != filter.ModeByItemClassID.end())
        return classItr->second;
    return filter.GlobalMode;
}

void EverQuestMod::LoadAuctionRealmFilterForPlayer(Player* player)
{
    if (player == nullptr || player->GetSession() == nullptr)
        return;
    uint32 accountID = player->GetSession()->GetAccountId();

    EverQuestAuctionRealmFilter filter;
    QueryResult queryResult = CharacterDatabase.Query("SELECT auctionRealmFilter FROM mod_everquest_account_settings WHERE accountid = {}", accountID);
    if (queryResult)
    {
        string payload = (*queryResult)[0].Get<string>();
        if (payload.empty() == false && TryParseAuctionRealmFilterPayload(payload, filter) == false)
        {
            LOG_WARN("module.EverQuest", "EverQuestMod::LoadAuctionRealmFilterForPlayer could not read the stored auction realm filter for account {}, so it was cleared", accountID);
            CharacterDatabase.Execute("UPDATE mod_everquest_account_settings SET auctionRealmFilter = '' WHERE accountid = {}", accountID);
        }
    }

    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    AuctionRealmFiltersByAccountID[accountID] = filter;
}

void EverQuestMod::ClearAuctionRealmFilterForPlayer(Player* player)
{
    if (player == nullptr || player->GetSession() == nullptr)
        return;
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    AuctionRealmFiltersByAccountID.erase(player->GetSession()->GetAccountId());
}

bool EverQuestMod::TryGetActiveAuctionRealmFilterForPlayer(Player* player, EverQuestAuctionRealmFilter& filter)
{
    if (player == nullptr || player->GetSession() == nullptr)
        return false;
    if (ConfigSystemItemTemplateIDMax == 0)
        return false;
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    unordered_map<uint32, EverQuestAuctionRealmFilter>::const_iterator filterItr = AuctionRealmFiltersByAccountID.find(player->GetSession()->GetAccountId());
    if (filterItr == AuctionRealmFiltersByAccountID.end() || filterItr->second.FiltersAnything == false)
        return false;
    filter = filterItr->second;
    return true;
}

bool EverQuestMod::SetAuctionRealmFilterForPlayer(Player* player, const string& payload)
{
    if (player == nullptr || player->GetSession() == nullptr)
        return false;
    EverQuestAuctionRealmFilter filter;
    if (TryParseAuctionRealmFilterPayload(payload, filter) == false)
        return false;

    uint32 accountID = player->GetSession()->GetAccountId();
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        AuctionRealmFiltersByAccountID[accountID] = filter;
    }

    // The payload is already known to hold nothing but '0', '1' and '2' characters, so it goes into the statement as-is
    CharacterDatabase.Execute("INSERT INTO `mod_everquest_account_settings` (`accountid`, `auctionRealmFilter`) VALUES ({}, '{}') ON DUPLICATE KEY UPDATE `auctionRealmFilter` = '{}'",
        accountID, payload, payload);
    return true;
}

void EverQuestMod::SendAuctionRealmFilterToPlayer(Player* player)
{
    if (player == nullptr || player->GetSession() == nullptr)
        return;

    string payload;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        unordered_map<uint32, EverQuestAuctionRealmFilter>::const_iterator filterItr = AuctionRealmFiltersByAccountID.find(player->GetSession()->GetAccountId());
        if (filterItr != AuctionRealmFiltersByAccountID.end())
            payload = filterItr->second.Payload;
    }

    // An account that never set anything gets the all "both" payload, which is what the addon shows by default anyway
    if (payload.empty() == true)
        payload = string(GetAuctionRealmFilterPayloadLength(), '0');

    std::string addonMessage = "EQAHFILTER\t" + payload;
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_ADDON, nullptr, nullptr, addonMessage);
    player->GetSession()->SendPacket(&data);
}

bool EverQuestMod::IsItemAllowedByAuctionRealmFilter(const EverQuestAuctionRealmFilter& filter, uint32 itemTemplateID)
{
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemTemplateID);
    if (itemTemplate == nullptr)
        return true;

    uint8 mode = GetAuctionRealmModeForItemClassAndSubClass(filter, itemTemplate->Class, itemTemplate->SubClass);
    if (mode == EQ_AUCTION_REALM_BOTH)
        return true;
    bool isNorrathItem = IsItemTemplateIDAnEQItemTemplateID(itemTemplateID);
    if (mode == EQ_AUCTION_REALM_NORRATH)
        return isNorrathItem;
    return isNorrathItem == false;
}

// This is such an obscene hack...
// Fixed byte size of one auction entry as written by SearchableAuctionEntry::BuildAuctionInfo (auction ID, item, entry ID, inspected enchants, random property, suffix factor, stack count, spell charges, flags, owner guid,
static const size_t AuctionEntrySizeInBytes = 4 + 4 + (MAX_INSPECTED_ENCHANTMENT_SLOT * 12) + 4 + 4 + 4 + 4 + 4 + 8 + 4 + 4 + 4 + 4 + 8 + 4;
static thread_local bool SendingOwnAuctionResult = false;

bool EverQuestMod::IsSendingOwnAuctionResult()
{
    return SendingOwnAuctionResult;
}

class EverQuestOwnAuctionResultSendMarker
{
public:
    EverQuestOwnAuctionResultSendMarker() { SendingOwnAuctionResult = true; }
    ~EverQuestOwnAuctionResultSendMarker() { SendingOwnAuctionResult = false; }
};

class EverQuestAuctionScanRow
{
public:
    string EntryData;
    uint32 ItemTemplateID = 0;
    int32 RandomPropertyID = 0;
    uint32 StackCount = 0;
    ObjectGuid OwnerGUID;
    uint32 StartBid = 0;
    uint32 Buyout = 0;
    uint32 TimeLeftInMS = 0;
    uint64 BidderGUIDCounter = 0;
    uint32 Bid = 0;
    ItemTemplate const* ItemTemplateData = nullptr;
    wstring ItemName;                                       // Lower case and with the random suffix, only built when something needs it
    string OwnerName;                                       // Only looked up when the sort asks for it
};

// Identifies a search by everything except which page of it was asked for
static string BuildAuctionSearchKey(const AuctionHouseSearchInfo& searchInfo, AuctionHouseFaction listFaction, bool applyEQClassFilter, const EverQuestAuctionRealmFilter& realmFilter, bool applyRealmFilter)
{
    string searchKey;
    searchKey += (applyEQClassFilter ? "e|" : "-|");
    searchKey += (applyRealmFilter ? realmFilter.Payload : "") + "|";
    searchKey += std::to_string(static_cast<uint32>(listFaction)) + "|";
    searchKey += std::to_string(static_cast<uint32>(searchInfo.levelmin)) + "|";
    searchKey += std::to_string(static_cast<uint32>(searchInfo.levelmax)) + "|";
    searchKey += std::to_string(searchInfo.usable ? 1 : 0) + "|";
    searchKey += std::to_string(searchInfo.inventoryType) + "|";
    searchKey += std::to_string(searchInfo.itemClass) + "|";
    searchKey += std::to_string(searchInfo.itemSubClass) + "|";
    searchKey += std::to_string(searchInfo.quality) + "|";
    for (uint32 sortIndex = 0; sortIndex < searchInfo.sorting.size(); ++sortIndex)
        searchKey += std::to_string(static_cast<uint32>(searchInfo.sorting[sortIndex].sortOrder)) + (searchInfo.sorting[sortIndex].isDesc ? "d," : "a,");
    searchKey += "|";
    for (size_t nameIndex = 0; nameIndex < searchInfo.wsearchedname.size(); ++nameIndex)
        searchKey += std::to_string(static_cast<uint32>(searchInfo.wsearchedname[nameIndex])) + ".";
    return searchKey;
}

static void BuildAuctionScanRowItemName(EverQuestAuctionScanRow& row, int localeIndex, int dbcLocaleIndex)
{
    if (row.ItemTemplateData == nullptr || row.ItemTemplateData->Name1.empty() == true)
        return;

    string itemName = row.ItemTemplateData->Name1;
    ItemLocale const* itemLocale = sObjectMgr->GetItemLocale(row.ItemTemplateData->ItemId);
    if (dbcLocaleIndex >= LOCALE_enUS && itemLocale != nullptr)
        ObjectMgr::GetLocaleString(itemLocale->Name, localeIndex, itemName);

    if (row.RandomPropertyID != 0)
    {
        std::array<char const*, 16> const* suffix = nullptr;
        if (row.RandomPropertyID < 0)
        {
            ItemRandomSuffixEntry const* randomSuffixEntry = sItemRandomSuffixStore.LookupEntry(-row.RandomPropertyID);
            if (randomSuffixEntry != nullptr)
                suffix = &randomSuffixEntry->Name;
        }
        else
        {
            ItemRandomPropertiesEntry const* randomPropertiesEntry = sItemRandomPropertiesStore.LookupEntry(row.RandomPropertyID);
            if (randomPropertiesEntry != nullptr)
                suffix = &randomPropertiesEntry->Name;
        }
        if (suffix != nullptr)
        {
            itemName += ' ';
            itemName += (*suffix)[dbcLocaleIndex >= 0 ? dbcLocaleIndex : LOCALE_enUS];
        }
    }

    if (Utf8toWStr(itemName, row.ItemName) == false)
        return;
    wstrToLower(row.ItemName);
}

// Mirrors SearchableAuctionEntry::CompareAuctionEntry
static int CompareAuctionScanRows(uint32 column, const EverQuestAuctionScanRow& firstRow, const EverQuestAuctionScanRow& secondRow)
{
    switch (column)
    {
    case AUCTION_SORT_MINLEVEL:
    {
        if (firstRow.ItemTemplateData->RequiredLevel > secondRow.ItemTemplateData->RequiredLevel)
            return -1;
        else if (firstRow.ItemTemplateData->RequiredLevel < secondRow.ItemTemplateData->RequiredLevel)
            return +1;
        break;
    }
    case AUCTION_SORT_RARITY:
    {
        if (firstRow.ItemTemplateData->Quality < secondRow.ItemTemplateData->Quality)
            return -1;
        else if (firstRow.ItemTemplateData->Quality > secondRow.ItemTemplateData->Quality)
            return +1;
        break;
    }
    case AUCTION_SORT_BUYOUT:
    {
        if (firstRow.Buyout != secondRow.Buyout)
        {
            if (firstRow.Buyout < secondRow.Buyout)
                return -1;
            else if (firstRow.Buyout > secondRow.Buyout)
                return +1;
        }
        else
        {
            if (firstRow.Bid < secondRow.Bid)
                return -1;
            else if (firstRow.Bid > secondRow.Bid)
                return +1;
        }
        break;
    }
    case AUCTION_SORT_TIMELEFT:
    {
        // The packet carries how long is left rather than when it ends, which sorts the same way
        if (firstRow.TimeLeftInMS < secondRow.TimeLeftInMS)
            return -1;
        else if (firstRow.TimeLeftInMS > secondRow.TimeLeftInMS)
            return +1;
        break;
    }
    case AUCTION_SORT_UNK4:
    {
        if (firstRow.BidderGUIDCounter < secondRow.BidderGUIDCounter)
            return -1;
        else if (firstRow.BidderGUIDCounter > secondRow.BidderGUIDCounter)
            return +1;
        break;
    }
    case AUCTION_SORT_ITEM:
    {
        int comparison = firstRow.ItemName.compare(secondRow.ItemName);
        if (comparison > 0)
            return -1;
        else if (comparison < 0)
            return +1;
        break;
    }
    case AUCTION_SORT_MINBIDBUY:
    {
        if (firstRow.Buyout != secondRow.Buyout)
        {
            if (firstRow.Buyout > secondRow.Buyout)
                return -1;
            else if (firstRow.Buyout < secondRow.Buyout)
                return +1;
        }
        else
        {
            if (firstRow.Bid < secondRow.Bid)
                return -1;
            else if (firstRow.Bid > secondRow.Bid)
                return +1;
        }
        break;
    }
    case AUCTION_SORT_OWNER:
    {
        int comparison = firstRow.OwnerName.compare(secondRow.OwnerName);
        if (comparison > 0)
            return -1;
        else if (comparison < 0)
            return +1;
        break;
    }
    case AUCTION_SORT_BID:
    {
        uint32 firstBid = firstRow.Bid != 0 ? firstRow.Bid : firstRow.StartBid;
        uint32 secondBid = secondRow.Bid != 0 ? secondRow.Bid : secondRow.StartBid;
        if (firstBid > secondBid)
            return -1;
        else if (firstBid < secondBid)
            return +1;
        break;
    }
    case AUCTION_SORT_STACK:
    {
        if (firstRow.StackCount < secondRow.StackCount)
            return -1;
        else if (firstRow.StackCount > secondRow.StackCount)
            return +1;
        break;
    }
    case AUCTION_SORT_BUYOUT_2:
    {
        if (firstRow.Buyout < secondRow.Buyout)
            return -1;
        else if (firstRow.Buyout > secondRow.Buyout)
            return +1;
        break;
    }
    default:
        break;
    }
    return 0;
}

// Mirrors AuctionSorter
class EverQuestAuctionScanRowSorter
{
public:
    EverQuestAuctionScanRowSorter(const AuctionSortOrderVector* sorting) : Sorting(sorting) { }

    bool operator()(const EverQuestAuctionScanRow& firstRow, const EverQuestAuctionScanRow& secondRow) const
    {
        for (uint32 sortIndex = 0; sortIndex < Sorting->size(); ++sortIndex)
        {
            int comparison = CompareAuctionScanRows(static_cast<uint32>((*Sorting)[sortIndex].sortOrder), firstRow, secondRow);
            if (comparison == 0)
                continue;
            return (comparison < 0) == (*Sorting)[sortIndex].isDesc;
        }
        return false;
    }

private:
    const AuctionSortOrderVector* Sorting;
};

// Mirrors AuctionHouseWorkerThread::BuildListAuctionItems
static bool DoesAuctionScanRowPassSearch(const EverQuestAuctionScanRow& row, const AuctionHouseSearchInfo& searchInfo, const AuctionHousePlayerInfo& playerInfo)
{
    ItemTemplate const* proto = row.ItemTemplateData;
    if (searchInfo.itemClass != 0xffffffff && proto->Class != searchInfo.itemClass)
        return false;
    if (searchInfo.itemSubClass != 0xffffffff && proto->SubClass != searchInfo.itemSubClass)
        return false;
    if (searchInfo.inventoryType != 0xffffffff && proto->InventoryType != searchInfo.inventoryType)
    {
        // Robes are counted as chests
        if (searchInfo.inventoryType != INVTYPE_CHEST || proto->InventoryType != INVTYPE_ROBE)
            return false;
    }
    if (searchInfo.quality != 0xffffffff && proto->Quality < searchInfo.quality)
        return false;
    if (searchInfo.levelmin != 0x00 && (proto->RequiredLevel < searchInfo.levelmin || (searchInfo.levelmax != 0x00 && proto->RequiredLevel > searchInfo.levelmax)))
        return false;
    if (searchInfo.usable != 0x00 && playerInfo.usablePlayerInfo.has_value() == true && playerInfo.usablePlayerInfo.value().PlayerCanUseItem(proto) == false)
        return false;

    // Matches a suffix ("of the Monkey") or any part of the name, same as the core search
    if (searchInfo.wsearchedname.empty() == false && row.ItemName.find(searchInfo.wsearchedname) == wstring::npos)
        return false;
    return true;
}

void EverQuestMod::QueueFullAuctionScan(EverQuestAuctionSearchScan& scan)
{
    AuctionHouseSearchInfo scanSearchInfo = scan.SearchInfo;
    scanSearchInfo.listfrom = 0;
    scanSearchInfo.getAll = true;
    scanSearchInfo.sorting.clear();
    AuctionHousePlayerInfo scanPlayerInfo = scan.PlayerInfo;
    scan.AwaitingResponse = true;
    sAuctionMgr->GetAuctionHouseSearcher()->QueueSearchRequest(new AuctionSearchListRequest(scan.ListFaction, std::move(scanSearchInfo), std::move(scanPlayerInfo)));
}

void EverQuestMod::BuildAuctionScanResultPacket(EverQuestAuctionSearchScan& scan, WorldPacket& resultPacket)
{
    uint32 keptEntryCount = static_cast<uint32>(scan.KeptEntries.size());
    uint32 sentEntryCount = 0;
    ByteBuffer sentEntryData;
    for (uint32 entryIndex = scan.RequestedListFrom; entryIndex < keptEntryCount && sentEntryCount < MAX_AUCTIONS_PER_PAGE; ++entryIndex)
    {
        sentEntryData.append(reinterpret_cast<const uint8*>(scan.KeptEntries[entryIndex].data()), scan.KeptEntries[entryIndex].size());
        sentEntryCount++;
    }

    resultPacket.Initialize(SMSG_AUCTION_LIST_RESULT, 4 + sentEntryData.size() + 4 + 4);
    resultPacket << uint32(sentEntryCount);
    if (sentEntryData.size() > 0)
        resultPacket.append(sentEntryData);
    resultPacket << uint32(keptEntryCount);
    resultPacket << uint32(AUCTION_SEARCH_DELAY);
}

// Must be called with the scan lock let go of
void EverQuestMod::SendAuctionScanResultPacket(WorldSession* session, WorldPacket& resultPacket)
{
    EverQuestOwnAuctionResultSendMarker sendMarker;
    session->SendPacket(&resultPacket);
}

void EverQuestMod::ClearAuctionSearchScanForPlayer(Player* player)
{
    if (player == nullptr)
        return;
    std::lock_guard<std::mutex> lock(AuctionScanMutex);
    AuctionSearchScansByPlayerGUID.erase(player->GetGUID());
}

bool EverQuestMod::TakeOverAuctionListRequest(WorldSession* session, WorldPacket const& packet)
{
    Player* player = session->GetPlayer();
    if (player == nullptr)
        return false;

    // Read the same way WorldSession::HandleAuctionListItems reads it
    WorldPacket packetCopy(packet);
    packetCopy.rpos(0);
    ObjectGuid auctioneerGUID;
    uint32 listFrom = 0;
    string searchedName;
    uint8 levelMin = 0, levelMax = 0, usable = 0, getAll = 0, sortOrderCount = 0;
    uint32 auctionSlotID = 0, auctionMainCategory = 0, auctionSubCategory = 0, quality = 0;
    AuctionSortOrderVector sortOrder;
    try
    {
        packetCopy >> auctioneerGUID;
        packetCopy >> listFrom;
        packetCopy >> searchedName;
        packetCopy >> levelMin >> levelMax;
        packetCopy >> auctionSlotID >> auctionMainCategory >> auctionSubCategory;
        packetCopy >> quality >> usable;
        packetCopy >> getAll;
        packetCopy >> sortOrderCount;
        if (sortOrderCount > AUCTION_SORT_MAX)
            return false;
        for (uint8 sortIndex = 0; sortIndex < sortOrderCount; ++sortIndex)
        {
            uint8 sortMode = 0;
            uint8 isDesc = 0;
            packetCopy >> sortMode;
            packetCopy >> isDesc;
            AuctionSortInfo sortInfo;
            sortInfo.isDesc = (isDesc == 1);
            sortInfo.sortOrder = static_cast<AuctionSortOrder>(sortMode);
            sortOrder.push_back(sortInfo);
        }
    }
    catch (ByteBufferException const&)
    {
        return false;
    }

    // The client asking to read the whole auction house itself is left to the core handler and the plain per page filter
    if (getAll != 0)
        return false;

    bool applyEQClassFilter = IsAuctionUsableFilterActiveForPlayer(player->GetGUID());
    EverQuestAuctionRealmFilter realmFilter;
    bool applyRealmFilter = TryGetActiveAuctionRealmFilterForPlayer(player, realmFilter);
    if (applyEQClassFilter == false && applyRealmFilter == false)
    {
        ClearAuctionSearchScanForPlayer(player);
        return false;
    }

    std::wstring searchedNameWide;
    if (Utf8toWStr(searchedName, searchedNameWide) == false)
        return false;
    wstrToLower(searchedNameWide);

    Creature* creature = player->GetNPCIfCanInteractWith(auctioneerGUID, UNIT_NPC_FLAG_AUCTIONEER);
    if (creature == nullptr)
        return false;
    AuctionHouseEntry const* auctionHouseEntry = AuctionHouseMgr::GetAuctionHouseEntryFromFactionTemplate(creature->GetFaction());
    if (auctionHouseEntry == nullptr)
        return false;

    if (player->HasUnitState(UNIT_STATE_DIED))
        player->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    AuctionHouseSearchInfo searchInfo;
    searchInfo.wsearchedname = searchedNameWide;
    searchInfo.listfrom = 0;
    searchInfo.levelmin = levelMin;
    searchInfo.levelmax = levelMax;
    searchInfo.usable = usable != 0;
    searchInfo.inventoryType = auctionSlotID;
    searchInfo.itemClass = auctionMainCategory;
    searchInfo.itemSubClass = auctionSubCategory;
    searchInfo.quality = quality;
    searchInfo.getAll = false;
    searchInfo.sorting = sortOrder;

    AuctionHousePlayerInfo playerInfo;
    playerInfo.playerGuid = player->GetGUID();
    playerInfo.faction = player->GetFaction();
    playerInfo.loc_idx = session->GetSessionDbLocaleIndex();
    playerInfo.locdbc_idx = session->GetSessionDbcLocale();
    if (usable != 0)
    {
        AuctionHouseUsablePlayerInfo usablePlayerInfo;
        usablePlayerInfo.classMask = player->getClassMask();
        usablePlayerInfo.raceMask = player->getRaceMask();
        usablePlayerInfo.level = player->GetLevel();
        SkillStatusMap const& skillMap = player->GetSkillStatusMap();
        for (auto const& skillPair : skillMap)
            usablePlayerInfo.skills.insert(std::make_pair(skillPair.first, player->GetSkillValue(skillPair.first)));
        PlayerSpellMap const& spellMap = player->GetSpellMap();
        for (auto const& spellPair : spellMap)
            if (spellPair.second->State != PLAYERSPELL_REMOVED && spellPair.second->IsInSpec(player->GetActiveSpec()))
                usablePlayerInfo.spells.insert(spellPair.first);
        playerInfo.usablePlayerInfo = std::move(usablePlayerInfo);
    }

    AuctionHouseFaction listFaction = AuctionHouseMgr::GetAuctionHouseFactionFromHouseId(AuctionHouseId(auctionHouseEntry->houseId));
    string searchKey = BuildAuctionSearchKey(searchInfo, listFaction, applyEQClassFilter, realmFilter, applyRealmFilter);

    std::unique_lock<std::mutex> scanLock(AuctionScanMutex);
    EverQuestAuctionSearchScan& scan = AuctionSearchScansByPlayerGUID[player->GetGUID()];
    if (scan.Active == false || scan.SearchKey != searchKey)
    {
        // An answer still owed to the search being replaced would otherwise be read as an answer to this one
        uint32 staleResponsesToDiscard = scan.StaleResponsesToDiscard;
        if (scan.AwaitingResponse == true)
            staleResponsesToDiscard++;
        scan = EverQuestAuctionSearchScan();
        scan.StaleResponsesToDiscard = staleResponsesToDiscard;
        scan.SearchKey = searchKey;
        scan.ListFaction = listFaction;
        scan.SearchInfo = searchInfo;
        scan.PlayerInfo = playerInfo;
        scan.ApplyEQClassFilter = applyEQClassFilter;
        scan.EQClassBitsKnown = TryGetEQClassFilterBitsForPlayer(player, scan.EQClassBits);
        scan.ApplyRealmFilter = applyRealmFilter;
        scan.RealmFilter = realmFilter;
    }
    scan.Active = true;
    scan.RequestedListFrom = listFrom;

    // Paging through a search already read is answered straight from the rows in hand
    WorldPacket resultPacket;
    bool hasResultToSend = false;
    if (scan.ResultsReady == true)
    {
        BuildAuctionScanResultPacket(scan, resultPacket);
        hasResultToSend = true;
    }
    else if (scan.AwaitingResponse == false)
        QueueFullAuctionScan(scan);
    scanLock.unlock();

    if (hasResultToSend == true)
        SendAuctionScanResultPacket(session, resultPacket);
    return true;
}

bool EverQuestMod::ConsumeAuctionListResultForScan(WorldSession* session, WorldPacket const& packet)
{
    Player* player = session->GetPlayer();
    if (player == nullptr)
        return false;

    std::unique_lock<std::mutex> scanLock(AuctionScanMutex);
    unordered_map<ObjectGuid, EverQuestAuctionSearchScan>::iterator scanItr = AuctionSearchScansByPlayerGUID.find(player->GetGUID());
    if (scanItr == AuctionSearchScansByPlayerGUID.end())
        return false;
    EverQuestAuctionSearchScan& scan = scanItr->second;
    if (scan.StaleResponsesToDiscard > 0)
    {
        scan.StaleResponsesToDiscard--;
        return true;
    }
    if (scan.Active == false || scan.AwaitingResponse == false)
        return false;
    scan.AwaitingResponse = false;

    // Names cost more to build than everything else here put together, so they are only built when the search or the sort actually reads them
    bool needsItemNames = scan.SearchInfo.wsearchedname.empty() == false;
    bool needsOwnerNames = false;
    for (uint32 sortIndex = 0; sortIndex < scan.SearchInfo.sorting.size(); ++sortIndex)
    {
        if (scan.SearchInfo.sorting[sortIndex].sortOrder == AUCTION_SORT_ITEM)
            needsItemNames = true;
        if (scan.SearchInfo.sorting[sortIndex].sortOrder == AUCTION_SORT_OWNER)
            needsOwnerNames = true;
    }

    vector<EverQuestAuctionScanRow> keptRows;
    WorldPacket packetCopy(packet);
    packetCopy.rpos(0);
    try
    {
        uint32 entryCount = 0;
        packetCopy >> entryCount;

        // Anything not shaped like the searcher output is not an answer this scan can use
        if (packetCopy.size() != 4 + (entryCount * AuctionEntrySizeInBytes) + 4 + 4)
        {
            scan.Active = false;
            return false;
        }

        keptRows.reserve(entryCount / 4);
        for (uint32 entryIndex = 0; entryIndex < entryCount; ++entryIndex)
        {
            size_t entryStartPos = packetCopy.rpos();
            EverQuestAuctionScanRow row;
            uint32 auctionID = 0;
            uint32 unusedValue = 0;
            ObjectGuid bidderGUID;
            packetCopy >> auctionID;
            packetCopy >> row.ItemTemplateID;
            for (uint8 enchantSlot = 0; enchantSlot < MAX_INSPECTED_ENCHANTMENT_SLOT; ++enchantSlot)
                packetCopy >> unusedValue >> unusedValue >> unusedValue;
            packetCopy >> row.RandomPropertyID;
            packetCopy >> unusedValue;                      // Suffix factor
            packetCopy >> row.StackCount;
            packetCopy >> unusedValue;                      // Spell charges
            packetCopy >> unusedValue;                      // Flags
            packetCopy >> row.OwnerGUID;
            packetCopy >> row.StartBid;
            packetCopy >> unusedValue;                      // Minimum outbid
            packetCopy >> row.Buyout;
            packetCopy >> row.TimeLeftInMS;
            packetCopy >> bidderGUID;
            packetCopy >> row.Bid;
            row.BidderGUIDCounter = bidderGUID.GetCounter();

            row.ItemTemplateData = sObjectMgr->GetItemTemplate(row.ItemTemplateID);
            if (row.ItemTemplateData == nullptr)
                continue;
            if (scan.ApplyEQClassFilter == true && IsItemEQClassAllowedForClassBits(row.ItemTemplateID, scan.EQClassBits, scan.EQClassBitsKnown) == false)
                continue;
            if (scan.ApplyRealmFilter == true && IsItemAllowedByAuctionRealmFilter(scan.RealmFilter, row.ItemTemplateID) == false)
                continue;
            if (needsItemNames == true)
                BuildAuctionScanRowItemName(row, scan.PlayerInfo.loc_idx, scan.PlayerInfo.locdbc_idx);
            if (DoesAuctionScanRowPassSearch(row, scan.SearchInfo, scan.PlayerInfo) == false)
                continue;
            if (needsOwnerNames == true)
                sCharacterCache->GetCharacterNameByGuid(row.OwnerGUID, row.OwnerName);

            row.EntryData = string(reinterpret_cast<const char*>(packetCopy.contents()) + entryStartPos, AuctionEntrySizeInBytes);
            keptRows.push_back(std::move(row));
        }
    }
    catch (ByteBufferException const&)
    {
        scan.Active = false;
        return false;
    }

    if (scan.SearchInfo.sorting.empty() == false)
    {
        EverQuestAuctionScanRowSorter sorter(&scan.SearchInfo.sorting);
        std::sort(keptRows.begin(), keptRows.end(), sorter);
    }

    scan.KeptEntries.clear();
    scan.KeptEntries.reserve(keptRows.size());
    for (uint32 rowIndex = 0; rowIndex < keptRows.size(); ++rowIndex)
        scan.KeptEntries.push_back(std::move(keptRows[rowIndex].EntryData));
    scan.ResultsReady = true;

    WorldPacket resultPacket;
    BuildAuctionScanResultPacket(scan, resultPacket);
    scanLock.unlock();

    SendAuctionScanResultPacket(session, resultPacket);
    return true;
}

// Used for a "get all" scan
bool EverQuestMod::BuildFilteredAuctionListPacket(Player* player, WorldPacket const& packet, bool applyEQClassFilter, const EverQuestAuctionRealmFilter* realmFilter, WorldPacket& filteredPacket)
{
    static const size_t auctionEntrySizeInBytes = AuctionEntrySizeInBytes;

    // Worked out once rather than per row: the per player form of this test takes a lock and can go to the database, and a "get all" answer can run to tens of thousands of rows
    uint32 eqClassBits = 0;
    bool eqClassBitsKnown = false;
    if (applyEQClassFilter == true)
        eqClassBitsKnown = TryGetEQClassFilterBitsForPlayer(player, eqClassBits);

    WorldPacket packetCopy(packet);
    packetCopy.rpos(0);
    try
    {
        uint32 entryCount;
        packetCopy >> entryCount;

        // Skip anything not "shaped" like the searcher's output (entry count + entries + total count + search delay)
        if (packetCopy.size() != 4 + (entryCount * auctionEntrySizeInBytes) + 4 + 4)
            return false;

        uint32 keptEntryCount = 0;
        ByteBuffer keptEntryData;
        for (uint32 i = 0; i < entryCount; ++i)
        {
            size_t entryStartPos = packetCopy.rpos();
            uint32 auctionID;
            uint32 itemTemplateID;
            packetCopy >> auctionID;
            packetCopy >> itemTemplateID;
            packetCopy.rpos(entryStartPos + auctionEntrySizeInBytes);
            bool keepEntry = true;
            if (applyEQClassFilter == true && IsItemEQClassAllowedForClassBits(itemTemplateID, eqClassBits, eqClassBitsKnown) == false)
                keepEntry = false;
            if (keepEntry == true && realmFilter != nullptr && IsItemAllowedByAuctionRealmFilter(*realmFilter, itemTemplateID) == false)
                keepEntry = false;
            if (keepEntry == true)
            {
                keptEntryData.append(packetCopy.contents() + entryStartPos, auctionEntrySizeInBytes);
                keptEntryCount++;
            }
        }
        if (keptEntryCount == entryCount)
            return false;

        uint32 totalCount;
        uint32 searchDelay;
        packetCopy >> totalCount;
        packetCopy >> searchDelay;

        filteredPacket.Initialize(SMSG_AUCTION_LIST_RESULT, 4 + (keptEntryCount * auctionEntrySizeInBytes) + 4 + 4);
        filteredPacket << uint32(keptEntryCount);
        if (keptEntryCount > 0)
            filteredPacket.append(keptEntryData);
        filteredPacket << uint32(totalCount);
        filteredPacket << uint32(searchDelay);
        return true;
    }
    catch (ByteBufferException const&)
    {
        return false;
    }
}

void EverQuestMod::LoadSpellData()
{
    SpellDataBySpellID.clear();
    BardSongTickSpellIDs.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT SpellID, AuraDurationBaseInMS, AuraDurationAddPerLevelInMS, AuraDurationMaxInMS, AuraDurationCalcMinLevel, AuraDurationCalcMaxLevel, RecourseSpellID, SpellIDCastOnMeleeAttacker, FocusBoostType, PeriodicAuraSpellID, PeriodicAuraSpellRadius, MaleFormSpellID, FemaleFormSpellID, EffectFailChancePercent, EffectFailableType, StunUsesBashKickChance, SpellIDCastOnTargetWhenStunLands, AuraStaysOnSecondaryClassSwitch, MinTargetLevel, MaxCreatureTargetLevel, ResistDiff, HasteType, ModFactionRepValue, IllusionFormAlignment, IllusionFormEQRaceID, PersistOnClassChange, IllusionObjectClass FROM mod_everquest_spell ORDER BY SpellID;");
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
            everQuestSpell.ResistDiff = fields[20].Get<int32>();
            everQuestSpell.HasteType = fields[21].Get<uint8>();
            everQuestSpell.ModFactionRepValue = fields[22].Get<int32>();
            everQuestSpell.IllusionFormAlignment = fields[23].Get<uint8>();
            everQuestSpell.IllusionFormEQRaceID = fields[24].Get<uint32>();
            everQuestSpell.PersistOnClassChange = fields[25].Get<bool>();
            everQuestSpell.IllusionObjectClass = fields[26].Get<uint8>();
            SpellDataBySpellID[everQuestSpell.SpellID] = everQuestSpell;
            if (everQuestSpell.PeriodicAuraSpellID != 0)
                BardSongTickSpellIDs.insert(everQuestSpell.PeriodicAuraSpellID);
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

uint32 EverQuestMod::GetActiveShapeshiftModelIDForPlayer(Player* player)
{
    // Zero means no shapeshift form with its own model is active (like stances and other model-less forms)
    Unit::AuraEffectList const& shapeshiftAuras = player->GetAuraEffectsByType(SPELL_AURA_MOD_SHAPESHIFT);
    if (shapeshiftAuras.empty() == true)
        return 0;
    return player->GetModelForForm(player->GetShapeshiftForm(), shapeshiftAuras.front()->GetId());
}

void EverQuestMod::ApplyIllusionGearDisplayIfChanged(Player* player, EverQuestPlayerIllusionState* illusionState)
{
    // Shapeshift forms with their own model (druid forms, ghost wolf) show over the illusion
    uint32 shapeshiftModelID = GetActiveShapeshiftModelIDForPlayer(player);
    if (shapeshiftModelID != 0)
    {
        if (player->GetDisplayId() != shapeshiftModelID)
            player->SetDisplayId(shapeshiftModelID);
        return;
    }

    // A zero result means no one doesn't exist, so leave the core's transform display alone
    uint32 gearDisplayID = GetIllusionGearDisplayIDForPlayer(player, illusionState->FormSpellID);
    if (gearDisplayID == 0)
        return;

    // Swap in the player's selected face version of the display when one exists
    uint32 faceDisplayID = GetIllusionFaceDisplayIDForPlayer(player, gearDisplayID);
    if (faceDisplayID == player->GetDisplayId())
        return;
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
            // A different form fell off, so the tracked form is still active and the next refresh cycle covers any display change
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

void EverQuestMod::LoadIllusionObjectData()
{
    IllusionObjectsByMapID.clear();

    QueryResult queryResult = WorldDatabase.Query("SELECT MapID, X, Y, Z, DisplayID, IsTree FROM mod_everquest_illusion_object;");
    if (!queryResult)
    {
        LOG_INFO("module.EverQuest", "EverQuestMod::LoadIllusionObjectData found no mod_everquest_illusion_object rows, so object illusions will always fall back to their form model");
        return;
    }
    uint32 loadedObjectCount = 0;
    do
    {
        // Pull the data out
        Field* fields = queryResult->Fetch();
        uint32 mapID = fields[0].Get<uint32>();
        EverQuestIllusionObject illusionObject;
        illusionObject.X = fields[1].Get<float>();
        illusionObject.Y = fields[2].Get<float>();
        illusionObject.Z = fields[3].Get<float>();
        illusionObject.DisplayID = fields[4].Get<uint32>();
        illusionObject.IsTree = fields[5].Get<bool>();
        IllusionObjectsByMapID[mapID].push_back(illusionObject);
        loadedObjectCount++;
    } while (queryResult->NextRow());
    LOG_INFO("module.EverQuest", "EverQuestMod::LoadIllusionObjectData loaded {} illusion objects across {} maps", loadedObjectCount, (uint32)IllusionObjectsByMapID.size());
}

uint8 EverQuestMod::GetIllusionObjectClassForSpellID(uint32 spellID)
{
    auto spellDataItr = SpellDataBySpellID.find(spellID);
    if (spellDataItr == SpellDataBySpellID.end())
        return EQ_ILLUSION_OBJECT_CLASS_NONE;
    return spellDataItr->second.IllusionObjectClass;
}

float EverQuestMod::GetIllusionObjectMaxDistanceForClass(uint8 illusionObjectClass)
{
    // Zero or less means the whole zone is in range
    if (illusionObjectClass == EQ_ILLUSION_OBJECT_CLASS_TREE)
        return ConfigSystemIllusionObjectTreeMaxDistance;
    return ConfigSystemIllusionObjectMaxDistance;
}

bool EverQuestMod::TryGetNearestIllusionObject(uint32 mapID, float x, float y, float z, uint8 illusionObjectClass, EverQuestIllusionObject& illusionObjectOut)
{
    if (illusionObjectClass == EQ_ILLUSION_OBJECT_CLASS_NONE)
        return false;

    // Object rows are only generated for the open world copy of a zone, since every instance copy sits on the same geometry
    auto illusionObjectsItr = IllusionObjectsByMapID.find(GetOpenWorldMapIDForMapID(mapID));
    if (illusionObjectsItr == IllusionObjectsByMapID.end())
        return false;

    bool foundObject = false;
    float maxDistance = GetIllusionObjectMaxDistanceForClass(illusionObjectClass);
    float bestDistanceSquared = std::numeric_limits<float>::max();
    if (maxDistance > 0.0f)
        bestDistanceSquared = maxDistance * maxDistance;
    for (const EverQuestIllusionObject& illusionObject : illusionObjectsItr->second)
    {
        if (illusionObjectClass == EQ_ILLUSION_OBJECT_CLASS_TREE && illusionObject.IsTree == false)
            continue;
        float deltaX = illusionObject.X - x;
        float deltaY = illusionObject.Y - y;
        float deltaZ = illusionObject.Z - z;
        float distanceSquared = (deltaX * deltaX) + (deltaY * deltaY) + (deltaZ * deltaZ);
        if (distanceSquared > bestDistanceSquared)
            continue;
        bestDistanceSquared = distanceSquared;
        illusionObjectOut = illusionObject;
        foundObject = true;
    }
    return foundObject;
}

bool EverQuestMod::HasIllusionObjectInRangeForCaster(Unit* caster, uint32 spellID)
{
    uint8 illusionObjectClass = GetIllusionObjectClassForSpellID(spellID);
    if (illusionObjectClass == EQ_ILLUSION_OBJECT_CLASS_NONE)
        return true;
    if (caster == nullptr)
        return true;
    EverQuestIllusionObject illusionObject;
    return TryGetNearestIllusionObject(caster->GetMapId(), caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ(), illusionObjectClass, illusionObject);
}

void EverQuestMod::ApplyIllusionObjectDisplayOnFormAuraApply(Player* player, uint32 formSpellID)
{
    uint8 illusionObjectClass = GetIllusionObjectClassForSpellID(formSpellID);
    if (illusionObjectClass == EQ_ILLUSION_OBJECT_CLASS_NONE)
        return;

    // Nothing in range leaves the form's own fallback object model in place, which the core transform already applied
    EverQuestIllusionObject illusionObject;
    if (TryGetNearestIllusionObject(player->GetMapId(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), illusionObjectClass, illusionObject) == false)
        return;
    player->SetDisplayId(illusionObject.DisplayID);
}

void EverQuestMod::RefreshIllusionObjectDisplayForPlayer(Player* player)
{
    for (auto const& appliedAuraItr : player->GetAppliedAuras())
    {
        AuraApplication const* appliedAurApp = appliedAuraItr.second;
        if (appliedAurApp == nullptr || appliedAurApp->GetBase() == nullptr)
            continue;
        uint32 appliedSpellID = appliedAurApp->GetBase()->GetId();
        if (GetIllusionObjectClassForSpellID(appliedSpellID) == EQ_ILLUSION_OBJECT_CLASS_NONE)
            continue;
        ApplyIllusionObjectDisplayOnFormAuraApply(player, appliedSpellID);
        return;
    }
}

bool EverQuestMod::IsUnitInIllusionObjectForm(Unit* unit)
{
    if (unit == nullptr)
        return false;
    for (auto const& appliedAuraItr : unit->GetAppliedAuras())
    {
        AuraApplication const* appliedAurApp = appliedAuraItr.second;
        if (appliedAurApp == nullptr || appliedAurApp->GetBase() == nullptr)
            continue;
        if (GetIllusionObjectClassForSpellID(appliedAurApp->GetBase()->GetId()) != EQ_ILLUSION_OBJECT_CLASS_NONE)
            return true;
    }
    return false;
}

bool EverQuestMod::IsUnitLevitating(Unit* unit)
{
    if (unit == nullptr)
        return false;
    if (unit->HasAuraType(SPELL_AURA_HOVER) == true)
        return true;
    return unit->HasAuraType(SPELL_AURA_FEATHER_FALL);
}

bool EverQuestMod::DoesSpellApplyLevitation(SpellInfo const* spellInfo)
{
    // EQ levitation lands as hover plus feather fall
    if (spellInfo == nullptr)
        return false;
    if (spellInfo->HasAura(SPELL_AURA_HOVER) == true)
        return true;
    return spellInfo->HasAura(SPELL_AURA_FEATHER_FALL);
}

bool EverQuestMod::IsLevitationBlockedByIllusionObjectForm(SpellInfo const* spellInfo, Unit* target)
{
    // A levitating object model crashes the client, so levitation and an object illusion form can never be up at the same time
    if (DoesSpellApplyLevitation(spellInfo) == false)
        return false;
    return IsUnitInIllusionObjectForm(target);
}

bool EverQuestMod::IsIllusionObjectFormBlockedByLevitation(uint32 spellID, Unit* target)
{
    // The two can never overlap even for an instant, so a levitating target simply does not get the form
    if (GetIllusionObjectClassForSpellID(spellID) == EQ_ILLUSION_OBJECT_CLASS_NONE)
        return false;
    return IsUnitLevitating(target);
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

bool EverQuestMod::ApplyBardSongFearDiminishingReturnsOnAuraApply(Unit* target, Aura* aura)
{
    // Diminishing returns will be 100% / 50% / 25% / immune chain for creature targets here, and returns true when shouldn't fear at all
    if (ConfigSpellBardFearDiminishingReturnsEnabled == false)
        return false;
    if (target == nullptr || aura == nullptr)
        return false;
    Creature* creature = target->ToCreature();
    if (creature == nullptr)
        return false;
    if (BardSongTickSpellIDs.find(aura->GetId()) == BardSongTickSpellIDs.end())
        return false;
    SpellInfo const* spellInfo = aura->GetSpellInfo();
    if (spellInfo == nullptr || spellInfo->HasAura(SPELL_AURA_MOD_FEAR) == false)
        return false;
    Unit* caster = aura->GetCaster();
    if (caster == nullptr || caster->IsCharmedOwnedByPlayerOrPlayer() == false)
        return false;
    if (caster->IsPlayer() == true && caster->ToPlayer()->IsGameMaster() == true)
        return false;

    int32 fullDurationInMS = aura->GetMaxDuration();
    if (fullDurationInMS <= 0)
        return false;

    uint32 nowMS = GameTime::GetGameTimeMS().count();
    EverQuestCreatureFearDiminishingReturnState* state = creature->CustomData.GetDefault<EverQuestCreatureFearDiminishingReturnState>(EQ_CREATURE_CUSTOMDATA_FEARDIMINISH);

    // The chain resets once the creature has gone the whole window (the last landed fear's duration plus the reset time)
    uint32 curLevel = state->Level;
    if (curLevel > 0 && getMSTimeDiff(state->LastApplyTimeMS, nowMS) > state->ResetWindowInMS)
        curLevel = 0;

    float durationMod;
    switch (curLevel)
    {
        case 0: durationMod = 1.0f; break;
        case 1: durationMod = 0.5f; break;
        case 2: durationMod = 0.25f; break;
        default: durationMod = 0.0f; break;
    }
    if (durationMod == 0.0f)
    {
        state->Level = 3;
        return true;
    }

    int32 newDurationInMS = int32(float(fullDurationInMS) * durationMod);
    state->Level = curLevel + 1;
    state->LastApplyTimeMS = nowMS;
    state->ResetWindowInMS = uint32(newDurationInMS) + ConfigSpellBardFearDiminishingReturnsResetTimeInMS;
    aura->SetMaxDuration(newDurationInMS);
    aura->SetDuration(newDurationInMS);
    return false;
}

void EverQuestMod::RemoveCreatureFearDiminishingReturnState(Creature* creature)
{
    creature->CustomData.Erase(EQ_CREATURE_CUSTOMDATA_FEARDIMINISH);
}

static const uint64 EQ_HASTE_TRACKING_KEY_PLAYERS = UINT64_MAX;

uint64 EverQuestMod::GetHasteTrackingKeyForUnit(Unit* unit)
{
    if (unit->IsPlayer() == true)
        return EQ_HASTE_TRACKING_KEY_PLAYERS;
    return GetMapInstanceKey(unit->GetMap());
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
        trackedHasteAuraEffects = &EQHasteAuraEffectsByMapInstanceKeyThenUnitGUID[GetHasteTrackingKeyForUnit(unit)][unit->GetGUID()];
    }

    // EQ haste category comes from the spell row, falling back to worn-spell lookup then the spell/song for safety
    uint32 hasteType = GetSpellDataForSpellID(spellID).HasteType;
    if (hasteType == EQ_HASTE_TYPE_NONE)
        hasteType = IsWornEffectSpell(spellID) == true ? EQ_HASTE_TYPE_WORNITEM : EQ_HASTE_TYPE_SPELL_V1;

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
            trackedHasteAuraEffect.HasteType = hasteType;
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
        auto trackedMapIter = EQHasteAuraEffectsByMapInstanceKeyThenUnitGUID.find(GetHasteTrackingKeyForUnit(unit));
        if (trackedMapIter == EQHasteAuraEffectsByMapInstanceKeyThenUnitGUID.end())
            return;
        auto trackedIter = trackedMapIter->second.find(unit->GetGUID());
        if (trackedIter == trackedMapIter->second.end())
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
        auto trackedMapIter = EQHasteAuraEffectsByMapInstanceKeyThenUnitGUID.find(GetHasteTrackingKeyForUnit(unit));
        if (trackedMapIter != EQHasteAuraEffectsByMapInstanceKeyThenUnitGUID.end())
        {
            trackedMapIter->second.erase(unit->GetGUID());
            if (trackedMapIter->second.empty())
                EQHasteAuraEffectsByMapInstanceKeyThenUnitGUID.erase(trackedMapIter);
        }
        return;
    }

    EnforceEQHastePercentCapOnUnit(unit, *trackedHasteAuraEffects);
}

void EverQuestMod::EnforceEQHastePercentCapOnUnit(Unit* unit, vector<EverQuestUnitHasteAuraEffect>& trackedHasteAuraEffects)
{
    float capPercent = GetEQHasteCapPercentForUnit(unit);
    uint8 unitLevel = unit->GetLevel();

    // EQ haste never stacks within a category (worn/item, spell/song/clicky, v2) - only the strongest effect of each category applies and
    // the categories then add together additively, matching TAKP Mob::GetHaste.  WoW stacks haste auras multiplicatively, so walk the effects
    // in apply order and clamp each applied amount such that the combined multiplier equals what the capped additive total of the category
    // winners would give.  Losing effects stay on the unit as visible buffs but get clamped to zero, and get restored if their category winner
    // is removed.  Melee and ranged process independently
    uint32 auraTypesToProcess[2] = { SPELL_AURA_MOD_MELEE_HASTE, SPELL_AURA_MOD_RANGED_HASTE };
    for (uint32 auraType : auraTypesToProcess)
    {
        // Find the strongest active effect per category (haste types 1 = worn, 2 = spell/song/clicky, 3 = v2)
        int winnerIndexByHasteType[4] = { -1, -1, -1, -1 };
        for (size_t i = 0; i < trackedHasteAuraEffects.size(); ++i)
        {
            EverQuestUnitHasteAuraEffect& trackedHasteAuraEffect = trackedHasteAuraEffects[i];
            if (trackedHasteAuraEffect.AuraType != auraType)
                continue;
            if (unit->GetAuraEffect(trackedHasteAuraEffect.SpellID, trackedHasteAuraEffect.EffectIndex, trackedHasteAuraEffect.CasterGUID) == nullptr)
                continue;
            uint32 hasteType = trackedHasteAuraEffect.HasteType;
            if (hasteType < EQ_HASTE_TYPE_WORNITEM || hasteType > EQ_HASTE_TYPE_SPELL_V2)
                hasteType = EQ_HASTE_TYPE_SPELL_V1;
            int winnerIndex = winnerIndexByHasteType[hasteType];
            if (winnerIndex == -1 || trackedHasteAuraEffect.NaturalAmount > trackedHasteAuraEffects[winnerIndex].NaturalAmount)
                winnerIndexByHasteType[hasteType] = (int)i;
        }

        // In TAKP worn haste is capped at 10 until level 26, v2 haste only works at level 50+ and adds at most 10
        float contributionsByHasteType[4] = { 0, 0, 0, 0 };
        if (winnerIndexByHasteType[1] >= 0)
        {
            float wornAmount = (float)trackedHasteAuraEffects[winnerIndexByHasteType[1]].NaturalAmount;
            contributionsByHasteType[1] = unitLevel > 25 ? wornAmount : std::min(wornAmount, 10.0f);
        }
        if (winnerIndexByHasteType[2] >= 0)
            contributionsByHasteType[2] = (float)trackedHasteAuraEffects[winnerIndexByHasteType[2]].NaturalAmount;
        if (winnerIndexByHasteType[3] >= 0 && unitLevel > 49)
            contributionsByHasteType[3] = std::min((float)trackedHasteAuraEffects[winnerIndexByHasteType[3]].NaturalAmount, 10.0f);

        float runningTotalPercent = 0;
        float previousCappedTotalPercent = 0;
        for (size_t i = 0; i < trackedHasteAuraEffects.size(); ++i)
        {
            EverQuestUnitHasteAuraEffect& trackedHasteAuraEffect = trackedHasteAuraEffects[i];
            if (trackedHasteAuraEffect.AuraType != auraType)
                continue;
            AuraEffect* auraEffect = unit->GetAuraEffect(trackedHasteAuraEffect.SpellID, trackedHasteAuraEffect.EffectIndex, trackedHasteAuraEffect.CasterGUID);
            if (auraEffect == nullptr)
                continue;
            uint32 hasteType = trackedHasteAuraEffect.HasteType;
            if (hasteType < EQ_HASTE_TYPE_WORNITEM || hasteType > EQ_HASTE_TYPE_SPELL_V2)
                hasteType = EQ_HASTE_TYPE_SPELL_V1;
            if (winnerIndexByHasteType[hasteType] == (int)i)
                runningTotalPercent += contributionsByHasteType[hasteType];
            float cappedTotalPercent = std::min(runningTotalPercent, capPercent);
            int32 newAmount = (int32)std::lround(100.0f * ((100.0f + cappedTotalPercent) / (100.0f + previousCappedTotalPercent)) - 100.0f);
            previousCappedTotalPercent = cappedTotalPercent;
            if (auraEffect->GetAmount() != newAmount)
                auraEffect->ChangeAmount(newAmount);
        }
    }
}

float EverQuestMod::GetEQHasteCapPercentForUnit(Unit* unit)
{
    // Follows TAKP Client::GetHasteCap and NPC::GetHasteCap, with the config value taking the place of the 60+ player rule value
    if (unit->IsPlayer() == true)
    {
        uint8 level = unit->GetLevel();
        if (level > 59)
            return ConfigSpellHasteCapPercent < 0 ? 0 : ConfigSpellHasteCapPercent;
        else if (level > 50)
            return 85;
        else
            return (float)(level + 25);
    }

    // Non-charmed pets cap based on their and their owner's level, and all other creatures are effectively uncapped
    Unit* owner = unit->GetOwner();
    if (owner != nullptr && unit->GetCharmerGUID().IsEmpty() == true)
    {
        int capPercent = 110 + (int)unit->GetLevel();
        capPercent += std::max(0, (int)owner->GetLevel() - 39);
        capPercent += std::max(0, (int)owner->GetLevel() - 60);
        return (float)capPercent;
    }
    return 250;
}

uint32 EverQuestMod::GetEquippedShieldBaseArmorForPlayer(Player* player)
{
    if (player == nullptr)
        return 0;
    Item* offHandItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
    if (offHandItem == nullptr)
        return 0;

    // The core skips item bonuses entirely for a broken item, so a broken shield has put no armor into the base value
    if (offHandItem->IsBroken() == true)
        return 0;
    ItemTemplate const* itemTemplate = offHandItem->GetTemplate();
    if (itemTemplate == nullptr)
        return 0;
    if (itemTemplate->Class != ITEM_CLASS_ARMOR || itemTemplate->SubClass != ITEM_SUBCLASS_ARMOR_SHIELD)
        return 0;

    // Scaling stat (heirloom) armor is resolved against the player's level inside the core and is not reproduced here
    if (itemTemplate->ScalingStatValue > 0)
        return 0;

    // Mirrors Player::_ApplyItemBonuses, which pulls the armor damage modifier back out before adding item armor to the base value
    uint32 shieldArmor = itemTemplate->Armor;
    if (shieldArmor != 0 && itemTemplate->ArmorDamageModifier != 0)
    {
        if ((uint32)itemTemplate->ArmorDamageModifier >= shieldArmor)
            return 0;
        shieldArmor -= (uint32)itemTemplate->ArmorDamageModifier;
    }
    return shieldArmor;
}

void EverQuestMod::RefreshBearFormShieldArmorShiftForPlayer(Player* player)
{
    if (ConfigPlayerShieldArmorIgnoresBearFormMultiplier == false)
        return;
    if (player == nullptr)
        return;

    // Bear and dire bear form multiply every point of armor that equipped items put into the armor base value.  Shields are usable by druids in EQ, and EQ shield AC should come through
    // at face value, so while the player is in one of those forms the shield's armor is moved out of the base value (which the form multiplies) and into the total value (which it does not).
    // Everything else about the armor calculation, including percent auras, is unchanged
    uint32 desiredShiftAmount = 0;
    uint8 currentForm = player->GetShapeshiftForm();
    if (currentForm == FORM_BEAR || currentForm == FORM_DIREBEAR)
        desiredShiftAmount = GetEquippedShieldBaseArmorForPlayer(player);

    uint32 appliedShiftAmount = 0;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto appliedShiftItr = BearFormShieldArmorShiftAmountByPlayerGUID.find(player->GetGUID());
        if (appliedShiftItr != BearFormShieldArmorShiftAmountByPlayerGUID.end())
            appliedShiftAmount = appliedShiftItr->second;
    }
    if (appliedShiftAmount == desiredShiftAmount)
        return;

    // Take back any prior shift first, since the shield (and so the amount moved) can change while the form is held
    if (appliedShiftAmount != 0)
    {
        player->HandleStatFlatModifier(UNIT_MOD_ARMOR, BASE_VALUE, (float)appliedShiftAmount, true);
        player->HandleStatFlatModifier(UNIT_MOD_ARMOR, TOTAL_VALUE, (float)appliedShiftAmount, false);
    }
    if (desiredShiftAmount != 0)
    {
        player->HandleStatFlatModifier(UNIT_MOD_ARMOR, BASE_VALUE, (float)desiredShiftAmount, false);
        player->HandleStatFlatModifier(UNIT_MOD_ARMOR, TOTAL_VALUE, (float)desiredShiftAmount, true);
    }

    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    if (desiredShiftAmount == 0)
        BearFormShieldArmorShiftAmountByPlayerGUID.erase(player->GetGUID());
    else
        BearFormShieldArmorShiftAmountByPlayerGUID[player->GetGUID()] = desiredShiftAmount;
}

void EverQuestMod::ClearBearFormShieldArmorShiftForPlayer(ObjectGuid playerGUID)
{
    // Only the tracking is dropped, since the stat modifiers themselves live on the player object and are rebuilt on the next login
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    BearFormShieldArmorShiftAmountByPlayerGUID.erase(playerGUID);
}

uint32 EverQuestMod::GetAgileFighterCombatAuraSpellIDForPlayer(Player* player)
{
    if (player == nullptr)
        return 0;
    if (ConfigSystemAgileFighterSpellID == 0)
        return 0;
    if (player->HasSpell(ConfigSystemAgileFighterSpellID) == false)
        return 0;

    bool isWearingLeather = false;
    bool isWearingMailOrPlate = false;
    bool isUsingShield = false;
    for (uint8 equipSlotIndex = EQUIPMENT_SLOT_START; equipSlotIndex < EQUIPMENT_SLOT_END; ++equipSlotIndex)
    {
        Item* equippedItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, equipSlotIndex);
        if (equippedItem == nullptr)
            continue;
        ItemTemplate const* itemTemplate = equippedItem->GetTemplate();
        if (itemTemplate == nullptr || itemTemplate->Class != ITEM_CLASS_ARMOR)
            continue;
        switch (itemTemplate->SubClass)
        {
        case ITEM_SUBCLASS_ARMOR_LEATHER:
            isWearingLeather = true;
            break;
        case ITEM_SUBCLASS_ARMOR_MAIL:
        case ITEM_SUBCLASS_ARMOR_PLATE:
            isWearingMailOrPlate = true;
            break;
        case ITEM_SUBCLASS_ARMOR_SHIELD:
            isUsingShield = true;
            break;
        default:
            break;
        }
    }

    // Mail, plate or a shield disqualifies both tiers, and cloth-only (or nothing) will elevate the player to Combat Master
    if (isWearingMailOrPlate == true || isUsingShield == true)
        return 0;
    if (isWearingLeather == false)
        return ConfigSystemAgileFighterCombatMasterSpellID;
    return ConfigSystemAgileFighterCombatExpertSpellID;
}

void EverQuestMod::RefreshAgileFighterCombatAuraForPlayer(Player* player)
{
    if (player == nullptr)
        return;
    if (ConfigSystemAgileFighterSpellID == 0)
        return;

    uint32 desiredAuraSpellID = GetAgileFighterCombatAuraSpellIDForPlayer(player);

    // Can't have both Combat Master and Combat Expert, so use higher
    if (ConfigSystemAgileFighterCombatMasterSpellID != 0 && desiredAuraSpellID != ConfigSystemAgileFighterCombatMasterSpellID && player->HasAura(ConfigSystemAgileFighterCombatMasterSpellID) == true)
        player->RemoveAurasDueToSpell(ConfigSystemAgileFighterCombatMasterSpellID);
    if (ConfigSystemAgileFighterCombatExpertSpellID != 0 && desiredAuraSpellID != ConfigSystemAgileFighterCombatExpertSpellID && player->HasAura(ConfigSystemAgileFighterCombatExpertSpellID) == true)
        player->RemoveAurasDueToSpell(ConfigSystemAgileFighterCombatExpertSpellID);

    if (desiredAuraSpellID != 0 && player->HasAura(desiredAuraSpellID) == false)
        player->AddAura(desiredAuraSpellID, player);
}

void EverQuestMod::ReapplyAgileFighterCombatAuraForPlayer(Player* player)
{
    if (player == nullptr)
        return;
    if (ConfigSystemAgileFighterSpellID == 0)
        return;
    if (ConfigSystemAgileFighterCombatMasterSpellID != 0 && player->HasAura(ConfigSystemAgileFighterCombatMasterSpellID) == true)
        player->RemoveAurasDueToSpell(ConfigSystemAgileFighterCombatMasterSpellID);
    if (ConfigSystemAgileFighterCombatExpertSpellID != 0 && player->HasAura(ConfigSystemAgileFighterCombatExpertSpellID) == true)
        player->RemoveAurasDueToSpell(ConfigSystemAgileFighterCombatExpertSpellID);
    RefreshAgileFighterCombatAuraForPlayer(player);
}

void EverQuestMod::UpdateAgileFighterCombatAura(Player* player, uint32 diffInMS)
{
    if (player == nullptr)
        return;
    if (ConfigSystemAgileFighterSpellID == 0)
        return;

    // Check occassionally in case equipment changed by a mechanism with no hook
    uint32 refreshTimerMS = 0;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        uint32& storedRefreshTimerMS = AgileFighterRefreshTimerMSByPlayerGUID[player->GetGUID()];
        storedRefreshTimerMS += diffInMS;
        refreshTimerMS = storedRefreshTimerMS;
        if (refreshTimerMS >= EQ_AGILE_FIGHTER_REFRESH_INTERVAL_MS)
            storedRefreshTimerMS = 0;
    }
    if (refreshTimerMS < EQ_AGILE_FIGHTER_REFRESH_INTERVAL_MS)
        return;
    RefreshAgileFighterCombatAuraForPlayer(player);
}

void EverQuestMod::ClearAgileFighterTrackingForPlayer(ObjectGuid playerGUID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    AgileFighterRefreshTimerMSByPlayerGUID.erase(playerGUID);
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
    QueryResult queryResult = WorldDatabase.Query("SELECT QuestTemplateID, ReactionType, UsePlayerX, UsePlayerY, UsePlayerZ, AddedPlayerX, AddedPlayerY, UsePlayerOrientation, PositionX, PositionY, PositionZ, Orientation, CreatureTemplateID, QuestgiverCreatureTemplateID, DelayInMS, SayText, UseNpcX, UseNpcY, UseNpcZ, UseNpcOrientation, MovementIsRun, FiresOnArrival, PathListID, GameObjectEntryID, GameObjectLifetimeSec FROM mod_everquest_quest_reaction ORDER BY QuestTemplateID, ID;");
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
            everQuestQuestReaction.SayText = fields[15].Get<std::string>();
            everQuestQuestReaction.UseNpcX = fields[16].Get<bool>();
            everQuestQuestReaction.UseNpcY = fields[17].Get<bool>();
            everQuestQuestReaction.UseNpcZ = fields[18].Get<bool>();
            everQuestQuestReaction.UseNpcOrientation = fields[19].Get<bool>();
            everQuestQuestReaction.MovementIsRun = fields[20].Get<bool>();
            everQuestQuestReaction.FiresOnArrival = fields[21].Get<bool>();
            everQuestQuestReaction.PathListID = fields[22].Get<uint32>();
            everQuestQuestReaction.GameObjectEntryID = fields[23].Get<uint32>();
            everQuestQuestReaction.GameObjectLifetimeSec = fields[24].Get<uint32>();
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
    QueryResult queryResult = WorldDatabase.Query("SELECT GossipCreatureTemplateID, NpcTextID, OptionID, OptionText, ReactionType, SayText, TargetCreatureTemplateID, UsePlayerX, UsePlayerY, UsePlayerZ, AddedPlayerX, AddedPlayerY, UsePlayerOrientation, UseNpcX, UseNpcY, UseNpcZ, UseNpcOrientation, PositionX, PositionY, PositionZ, Orientation, DelayInMS, MovementIsRun, FiresOnArrival, PathListID, GameObjectEntryID, GameObjectLifetimeSec, RequiredQuestID, RequiredNearX, RequiredNearY, RequiredNearZ, RequiredNearDistance FROM mod_everquest_gossip_reaction ORDER BY GossipCreatureTemplateID, ID;");
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
            gossipReaction.MovementIsRun = fields[22].Get<bool>();
            gossipReaction.FiresOnArrival = fields[23].Get<bool>();
            gossipReaction.PathListID = fields[24].Get<uint32>();
            gossipReaction.GameObjectEntryID = fields[25].Get<uint32>();
            gossipReaction.GameObjectLifetimeSec = fields[26].Get<uint32>();
            gossipReaction.RequiredQuestID = fields[27].Get<uint32>();
            gossipReaction.RequiredNearX = fields[28].Get<float>();
            gossipReaction.RequiredNearY = fields[29].Get<float>();
            gossipReaction.RequiredNearZ = fields[30].Get<float>();
            gossipReaction.RequiredNearDistance = fields[31].Get<float>();
            GossipReactionsByGossipCreatureTemplateID[gossipReaction.GossipCreatureTemplateID].push_back(gossipReaction);
        } while (queryResult->NextRow());
    }
}

bool EverQuestMod::DoesPlayerMeetGossipRequirements(Player* player, Creature* creature, const EverQuestGossipReaction& gossipReaction)
{
    if (gossipReaction.RequiredQuestID != 0)
    {
        if (player == nullptr)
            return false;
        if (player->GetQuestRewardStatus(gossipReaction.RequiredQuestID) == false)
            return false;
    }
    if (gossipReaction.RequiredNearDistance > 0)
    {
        if (creature == nullptr)
            return false;
        if (creature->GetExactDist2d(gossipReaction.RequiredNearX, gossipReaction.RequiredNearY) > gossipReaction.RequiredNearDistance)
            return false;
    }
    return true;
}

bool EverQuestMod::HandleGossipHello(Player* player, Creature* creature)
{
    if (IsEnabled == false)
        return false;

    // Talking to the creature is the closest analog to saying 'Hail' in EQ
    bool firedHailedEmote = DoCreatureEmoteEvent(creature, EQ_CREATURE_EMOTE_EVENT_HAILED, player);

    unordered_map<uint32, vector<EverQuestGossipReaction>>::const_iterator gossipReactionsIterator = GossipReactionsByGossipCreatureTemplateID.find(creature->GetEntry());
    bool hasAnyAvailableOption = false;
    set<uint32> blockedOptionIDs;
    if (gossipReactionsIterator != GossipReactionsByGossipCreatureTemplateID.end())
    {
        // An option whose requirements are not met is not on the menu at all
        for (const EverQuestGossipReaction& gossipReaction : gossipReactionsIterator->second)
            if (DoesPlayerMeetGossipRequirements(player, creature, gossipReaction) == false)
                blockedOptionIDs.insert(gossipReaction.OptionID);
        for (const EverQuestGossipReaction& gossipReaction : gossipReactionsIterator->second)
            if (blockedOptionIDs.find(gossipReaction.OptionID) == blockedOptionIDs.end())
                hasAnyAvailableOption = true;
    }

    if (hasAnyAvailableOption == false)
    {
        // Creatures that only exist as gossip targets for a hailed emote shouldn't open an empty gossip window, but any creature with a real role
        // (or with a hail response of its own to show, which is a gossip menu on the template) should fall through to its normal handling
        if (firedHailedEmote == true && creature->GetCreatureTemplate()->GossipMenuId == 0 && creature->IsQuestGiver() == false && creature->IsVendor() == false && creature->IsTrainer() == false
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
        if (blockedOptionIDs.find(gossipReaction.OptionID) != blockedOptionIDs.end())
            continue;
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

    // The menu the player is looking at can be stale by the time they click, so the requirements are checked again
    for (const EverQuestGossipReaction& gossipReaction : gossipReactionsIterator->second)
    {
        if (gossipReaction.OptionID != action)
            continue;
        if (DoesPlayerMeetGossipRequirements(player, creature, gossipReaction) == false)
        {
            CloseGossipMenuFor(player);
            return true;
        }
    }

    Map* map = creature->GetMap();
    bool reactionMatched = false;

    // Rows behind a walkto do not run now - they are handed to the walk and fire when the creature gets there
    vector<EverQuestPendingKillSpawnAction> arrivalActions;
    for (const EverQuestGossipReaction& gossipReaction : gossipReactionsIterator->second)
    {
        if (gossipReaction.OptionID != action || gossipReaction.FiresOnArrival == false)
            continue;
        EverQuestPendingKillSpawnAction arrivalAction;
        arrivalAction.TargetCreatureTemplateID = gossipReaction.TargetCreatureTemplateID;
        arrivalAction.RemainingMS = (int32)gossipReaction.DelayInMS;
        arrivalAction.PathListID = gossipReaction.PathListID;
        arrivalAction.GameObjectEntryID = gossipReaction.GameObjectEntryID;
        arrivalAction.GameObjectLifetimeSec = gossipReaction.GameObjectLifetimeSec;
        arrivalAction.SayText = gossipReaction.SayText;
        arrivalAction.ListenerGUID = player->GetGUID();
        arrivalAction.UseMoverPositionX = gossipReaction.UseNpcX;
        arrivalAction.UseMoverPositionY = gossipReaction.UseNpcY;
        arrivalAction.UseMoverPositionZ = gossipReaction.UseNpcZ;
        arrivalAction.UseMoverOrientation = gossipReaction.UseNpcOrientation;
        arrivalAction.PositionX = gossipReaction.PositionX;
        arrivalAction.PositionY = gossipReaction.PositionY;
        arrivalAction.PositionZ = gossipReaction.PositionZ;
        arrivalAction.Orientation = gossipReaction.Orientation;
        if (gossipReaction.UsePlayerX == true)
            arrivalAction.PositionX = player->GetPositionX() + gossipReaction.AddedPlayerX;
        if (gossipReaction.UsePlayerY == true)
            arrivalAction.PositionY = player->GetPositionY() + gossipReaction.AddedPlayerY;
        if (gossipReaction.UsePlayerZ == true)
            arrivalAction.PositionZ = player->GetPositionZ();
        if (gossipReaction.UsePlayerOrientation == true)
            arrivalAction.Orientation = player->GetOrientation();
        switch (gossipReaction.ReactionType)
        {
        case EQ_QUEST_REACTION_SAY: arrivalAction.ActionType = EQ_KILLSPAWN_ACTION_SAY; break;
        case EQ_QUEST_REACTION_EMOTE: arrivalAction.ActionType = EQ_KILLSPAWN_ACTION_EMOTE; break;
        case EQ_QUEST_REACTION_YELL: arrivalAction.ActionType = EQ_KILLSPAWN_ACTION_YELL; break;
        case EQ_QUEST_REACTION_ATTACKPLAYER: arrivalAction.ActionType = EQ_KILLSPAWN_ACTION_ATTACKPLAYER; break;
        case EQ_QUEST_REACTION_DESPAWN: arrivalAction.ActionType = EQ_KILLSPAWN_ACTION_DESPAWN; break;
        case EQ_QUEST_REACTION_SPAWN:
        case EQ_QUEST_REACTION_SPAWNUNIQUE:
        {
            arrivalAction.ActionType = EQ_KILLSPAWN_ACTION_SPAWN;
            if (gossipReaction.ReactionType == EQ_QUEST_REACTION_SPAWNUNIQUE)
                arrivalAction.OnlyIfNotAliveCreatureTemplateID = gossipReaction.TargetCreatureTemplateID;
        } break;
        case EQ_QUEST_REACTION_WALKGRID: arrivalAction.ActionType = EQ_KILLSPAWN_ACTION_WALKPATH; break;
        case EQ_QUEST_REACTION_SPAWNOBJECT: arrivalAction.ActionType = EQ_KILLSPAWN_ACTION_SPAWNOBJECT; break;
        default: continue; // Nothing else is meaningful once the walk is over
        }
        arrivalActions.push_back(arrivalAction);
    }

    for (const EverQuestGossipReaction& gossipReaction : gossipReactionsIterator->second)
    {
        if (gossipReaction.OptionID != action)
            continue;
        reactionMatched = true;
        if (gossipReaction.FiresOnArrival == true)
            continue;

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
                EnqueuePendingKillSpawnAction(map, pendingAction);
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
                EnqueuePendingKillSpawnAction(map, pendingAction);
            }
            else
                SpawnCreature(gossipReaction.TargetCreatureTemplateID, map, x, y, z, orientation, gossipReaction.ReactionType == EQ_QUEST_REACTION_SPAWNUNIQUE);
        } break;
        case EQ_QUEST_REACTION_WALKTO:
        {
            StartReactionWalk(creature, x, y, z, orientation, gossipReaction.Orientation != 0 || gossipReaction.UseNpcOrientation == true || gossipReaction.UsePlayerOrientation == true, gossipReaction.MovementIsRun, arrivalActions);
        } break;
        case EQ_QUEST_REACTION_SPAWNOBJECT:
        {
            SpawnReactionGameObject(creature, gossipReaction.GameObjectEntryID, x, y, z, gossipReaction.GameObjectLifetimeSec);
        } break;
        case EQ_QUEST_REACTION_WALKGRID:
        {
            StartReactionGridWalk(creature, gossipReaction.PathListID, arrivalActions);
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

void EverQuestMod::LoadPetSilentDisplayData()
{
    SilentFidgetDisplayIDsByDisplayID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT DisplayID, SilentDisplayID FROM mod_everquest_pet_silent_display;");
    if (queryResult)
    {
        do
        {
            Field* fields = queryResult->Fetch();
            uint32 displayID = fields[0].Get<uint32>();
            uint32 silentDisplayID = fields[1].Get<uint32>();
            SilentFidgetDisplayIDsByDisplayID[displayID] = silentDisplayID;
        } while (queryResult->NextRow());
    }
}

void EverQuestMod::RemoveInvalidPetSilentDisplays()
{
    unordered_map<uint32, uint32>::iterator displayIter = SilentFidgetDisplayIDsByDisplayID.begin();
    while (displayIter != SilentFidgetDisplayIDsByDisplayID.end())
    {
        if (sCreatureDisplayInfoStore.LookupEntry(displayIter->second) == nullptr)
        {
            LOG_ERROR("module.EverQuest", "EverQuestMod::RemoveInvalidPetSilentDisplays dropped display ID {} as its silent display ID {} is missing from CreatureDisplayInfo.dbc.  Deploy the current DBC files.", displayIter->first, displayIter->second);
            displayIter = SilentFidgetDisplayIDsByDisplayID.erase(displayIter);
        }
        else
            ++displayIter;
    }
}

uint32 EverQuestMod::GetSilentFidgetDisplayIDForDisplayID(uint32 displayID) const
{
    unordered_map<uint32, uint32>::const_iterator displayIter = SilentFidgetDisplayIDsByDisplayID.find(displayID);
    if (displayIter == SilentFidgetDisplayIDsByDisplayID.end())
        return 0;
    return displayIter->second;
}

void EverQuestMod::UpdatePetFidgetSilence(Creature* creature)
{
    // Tamed pets copies the display of the world creature it came from, but swap out that display ID for a silent one
    if (creature->IsPet() == false)
        return;
    uint32 silentDisplayID = GetSilentFidgetDisplayIDForDisplayID(creature->GetDisplayId());
    if (silentDisplayID == 0)
        return;

    // The object scale must be passed back in, since it otherwise resets to 1.  Only the visible display is changed, as the native display is what gets saved into character_pet
    creature->SetDisplayId(silentDisplayID, creature->GetObjectScale());
}

void EverQuestMod::FixInvalidCharacterPetModelIDs()
{
    // character_pet model IDs can become 'bad' if the server doesn't recast the same model ID. Shouldn't happen anymore, but many saved now can be broken and crash the server
    QueryResult queryResult = CharacterDatabase.Query("SELECT DISTINCT entry, modelid FROM character_pet");
    if (!queryResult)
        return;
    do
    {
        Field* fields = queryResult->Fetch();
        uint32 creatureTemplateID = fields[0].Get<uint32>();
        uint32 modelID = fields[1].Get<uint32>();
        if (modelID != 0 && sCreatureDisplayInfoStore.LookupEntry(modelID) != nullptr)
            continue;
        CreatureTemplate const* creatureTemplate = sObjectMgr->GetCreatureTemplate(creatureTemplateID);
        if (creatureTemplate == nullptr)
        {
            LOG_ERROR("module.EverQuest", "EverQuestMod::FixInvalidCharacterPetModelIDs found saved pet(s) with invalid modelid {} but no creature template {} to repair from, skipping (these pets will fail to summon)", modelID, creatureTemplateID);
            continue;
        }
        uint32 replacementModelID = 0;
        for (CreatureModel const& creatureModel : creatureTemplate->Models)
        {
            if (sCreatureDisplayInfoStore.LookupEntry(creatureModel.CreatureDisplayID) != nullptr)
            {
                replacementModelID = creatureModel.CreatureDisplayID;
                break;
            }
        }
        if (replacementModelID == 0)
        {
            LOG_ERROR("module.EverQuest", "EverQuestMod::FixInvalidCharacterPetModelIDs found saved pet(s) with invalid modelid {} but creature template {} has no model with a valid display ID either, skipping", modelID, creatureTemplateID);
            continue;
        }
        CharacterDatabase.DirectExecute("UPDATE character_pet SET modelid = {} WHERE entry = {} AND modelid = {}", replacementModelID, creatureTemplateID, modelID);
        LOG_INFO("module.EverQuest", "EverQuestMod::FixInvalidCharacterPetModelIDs replaced stale saved pet modelid {} with {} for pet creature template {}", modelID, replacementModelID, creatureTemplateID);
    } while (queryResult->NextRow());
}

void EverQuestMod::RemoveStaleSavedPetSpells()
{
    QueryResult queryResult = CharacterDatabase.Query("SELECT pet_spell.guid, pet_spell.spell, character_pet.entry FROM pet_spell INNER JOIN character_pet ON character_pet.id = pet_spell.guid");
    if (!queryResult)
        return;
    uint32 removedPetSpellCount = 0;
    do
    {
        Field* fields = queryResult->Fetch();
        uint32 petNumber = fields[0].Get<uint32>();
        uint32 spellID = fields[1].Get<uint32>();
        uint32 creatureTemplateID = fields[2].Get<uint32>();

        // Only converted EQ spells are in scope.  Anything else reached the pet through a WOW system this can't model, pet talents especially, and must be left alone
        if (spellID < ConfigSystemSpellDBCIDMin || spellID > ConfigSystemSpellDBCIDMax)
            continue;
        if (CanPetCreatureTemplateTeachSpell(creatureTemplateID, spellID) == true)
            continue;

        CharacterDatabase.DirectExecute("DELETE FROM pet_spell WHERE guid = {} AND spell = {}", petNumber, spellID);
        ++removedPetSpellCount;
    } while (queryResult->NextRow());
    if (removedPetSpellCount > 0)
        LOG_INFO("module.EverQuest", "EverQuestMod::RemoveStaleSavedPetSpells removed {} saved pet spell(s) that the pet's creature can no longer teach", removedPetSpellCount);
}

bool EverQuestMod::CanPetCreatureTemplateTeachSpell(uint32 creatureTemplateID, uint32 spellID)
{
    CreatureTemplate const* creatureTemplate = sObjectMgr->GetCreatureTemplate(creatureTemplateID);
    if (creatureTemplate == nullptr)
        return true; // Without a creature template the pet can't be summoned at all, so leave its saved rows alone

    // creature_template_spell.  The core only reads the first MAX_CREATURE_SPELL_DATA_SLOT of these into a pet's default spells, but a spell in any slot was put there deliberately and shouldn't be called stale
    for (uint8 spellIndex = 0; spellIndex < MAX_CREATURE_SPELLS; ++spellIndex)
        if (creatureTemplate->spells[spellIndex] == spellID)
            return true;

    // CreatureSpellData.dbc, for the pets that carry a PetSpellDataId instead
    if (creatureTemplate->PetSpellDataId != 0)
    {
        PetDefaultSpellsEntry const* petDefaultSpells = sSpellMgr->GetPetDefaultSpellsEntry(-int32(creatureTemplate->PetSpellDataId));
        if (petDefaultSpells != nullptr)
            for (uint8 spellIndex = 0; spellIndex < MAX_CREATURE_SPELL_DATA_SLOT; ++spellIndex)
                if (petDefaultSpells->spellid[spellIndex] == spellID)
                    return true;
    }

    if (creatureTemplate->family == 0)
        return false;

    // Creature family level-up spells, which is how the EQ pet Taunt and Area Taunt ranks reach a pet
    PetLevelupSpellSet const* petLevelupSpells = sSpellMgr->GetPetLevelupSpellList(creatureTemplate->family);
    if (petLevelupSpells != nullptr)
        for (PetLevelupSpellSet::const_iterator petLevelupSpell = petLevelupSpells->begin(); petLevelupSpell != petLevelupSpells->end(); ++petLevelupSpell)
            if (petLevelupSpell->second == spellID)
                return true;

    // Creature family passives
    CreatureFamilyEntry const* creatureFamily = sCreatureFamilyStore.LookupEntry(creatureTemplate->family);
    if (creatureFamily == nullptr)
        return false;
    PetFamilySpellsStore::const_iterator petFamilySpells = sPetFamilySpellsStore.find(creatureFamily->ID);
    if (petFamilySpells != sPetFamilySpellsStore.end() && petFamilySpells->second.find(spellID) != petFamilySpells->second.end())
        return true;

    return false;
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

bool EverQuestMod::DoesPlayerHaveActiveEQPet(Player* player)
{
    if (player == nullptr)
        return false;
    Pet* pet = player->GetPet();
    if (pet == nullptr)
        return false;
    return HasPetDataForCreatureTemplateID(pet->GetEntry());
}

uint32 EverQuestMod::GetActiveEQPetCreatureTypeForPlayer(Player* player)
{
    if (player == nullptr)
        return 0;
    Pet* pet = player->GetPet();
    if (pet == nullptr)
        return 0;
    if (HasPetDataForCreatureTemplateID(pet->GetEntry()) == false)
        return 0;
    CreatureTemplate const* creatureTemplate = pet->GetCreatureTemplate();
    if (creatureTemplate == nullptr)
        return 0;
    return creatureTemplate->type;
}

void EverQuestMod::LoadCreatePlayerData()
{
    PlayerCreateInfoByRaceIDThenClassID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT race, class, map, zone, position_x, position_y, position_z, orientation, illusionitem FROM mod_everquest_playercreateinfo;");
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
            everQuestPlayerCreateInfo.IllusionItemID = fields[8].Get<uint32>();
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

void EverQuestMod::LoadPlayerClassStartItemData()
{
    PlayerClassStartItemWOWIDsByEQClassID.clear();
    QueryResult queryResult = WorldDatabase.Query("SELECT eqclass, itemid FROM mod_everquest_playerclassstartitems;");
    if (queryResult)
    {
        do
        {
            Field* fields = queryResult->Fetch();
            uint8 eqClassID = fields[0].Get<uint8>();
            uint32 itemID = fields[1].Get<uint32>();
            PlayerClassStartItemWOWIDsByEQClassID[eqClassID].push_back(itemID);
        } while (queryResult->NextRow());
    }
}

bool EverQuestMod::GrantClassStartItemsForPlayer(Player* player, uint8 eqClassID)
{
    if (ConfigPlayerAddClassStartItems == false)
        return true;
    if (player == nullptr || eqClassID == EQ_EQCLASS_NONE)
        return true;
    auto startItemsItr = PlayerClassStartItemWOWIDsByEQClassID.find(eqClassID);
    if (startItemsItr == PlayerClassStartItemWOWIDsByEQClassID.end())
        return true;

    bool grantedEverything = true;
    for (uint32 startItemID : startItemsItr->second)
    {
        // A player already holding the item does not need another copy
        if (player->HasItemCount(startItemID, 1, true) == true)
            continue;

        ItemPosCountVec destPosition;
        InventoryResult invResult = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, destPosition, startItemID, 1);
        if (invResult != EQUIP_ERR_OK || player->StoreNewItem(destPosition, startItemID, true) == nullptr)
            grantedEverything = false;
    }
    return grantedEverything;
}

void EverQuestMod::SetPendingStartItemEQClassForPlayer(Player* player, uint8 eqClassID)
{
    GetOrLoadActivePlayerClassControllerData(player)->PendingStartItemEQClass = eqClassID;

    EverQuestPlayerControllerData controllerData;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto controllerDataIt = ActivePlayerClassControllerDataByGUID.find(player->GetGUID());
        if (controllerDataIt == ActivePlayerClassControllerDataByGUID.end())
            return;
        controllerData = controllerDataIt->second;
    }

    CharacterDatabase.Execute("INSERT INTO `mod_everquest_character_settings` (`guid`, `currentSecondaryClass`, `nextSecondaryClass`, `secondaryExpPool`, `pendingStartItemEQClass`) VALUES ({}, {}, {}, {}, {}) ON DUPLICATE KEY UPDATE `pendingStartItemEQClass` = {}",
        player->GetGUID().GetCounter(),
        controllerData.CurrentSecondClass,
        controllerData.NextSecondClass,
        controllerData.SecondaryExpPool,
        eqClassID,
        eqClassID);
}

void EverQuestMod::GrantPendingClassStartItemsForPlayer(Player* player)
{
    uint8 pendingEQClassID = GetOrLoadActivePlayerClassControllerData(player)->PendingStartItemEQClass;
    if (pendingEQClassID == EQ_EQCLASS_NONE)
        return;

    // Only clear the pending class once the items were provided
    if (GrantClassStartItemsForPlayer(player, pendingEQClassID) == false)
        return;
    SetPendingStartItemEQClassForPlayer(player, EQ_EQCLASS_NONE);
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

    // Grant starter DK abilities
    GrantDeathKnightStarterAbilitiesIfNeeded(player);
}

void EverQuestMod::GrantDeathKnightStarterAbilitiesIfNeeded(Player* player)
{
    if (ConfigDeathKnightsStartLikeOtherClasses == false)
        return;
    if (player->getClass() != CLASS_DEATH_KNIGHT)
        return;

    // Avoid granting lower ranks if higher ranks exist
    uint32 curRankSpellID = EQ_DEATHKNIGHT_BLOODSTRIKE_SPELL_ID;
    while (curRankSpellID != 0)
    {
        if (player->HasSpell(curRankSpellID) == true)
            return;
        curRankSpellID = sSpellMgr->GetNextSpellInChain(curRankSpellID);
    }

    player->learnSpell(EQ_DEATHKNIGHT_BLOODSTRIKE_SPELL_ID);
}

void EverQuestMod::LowerDeathKnightGlyphRequiredLevels()
{
    if (ConfigDeathKnightsStartLikeOtherClasses == false)
        return;

    std::vector<ItemTemplate*> const* itemTemplates = sObjectMgr->GetItemTemplateStoreFast();
    if (itemTemplates == nullptr)
        return;

    uint32 loweredGlyphCount = 0;
    for (ItemTemplate* itemTemplate : *itemTemplates)
    {
        if (itemTemplate == nullptr)
            continue;
        if (itemTemplate->Class != (uint32)ITEM_CLASS_GLYPH || itemTemplate->SubClass != (uint32)ITEM_SUBCLASS_GLYPH_DEATH_KNIGHT)
            continue;
        if (itemTemplate->RequiredLevel <= EQ_DEATHKNIGHT_GLYPH_REQUIRED_LEVEL)
            continue;
        itemTemplate->RequiredLevel = EQ_DEATHKNIGHT_GLYPH_REQUIRED_LEVEL;
        ++loweredGlyphCount;
    }

    LOG_INFO("module.EverQuest", "EverQuestMod::LowerDeathKnightGlyphRequiredLevels lowered the required level of {} death knight glyphs to {}", loweredGlyphCount, (uint32)EQ_DEATHKNIGHT_GLYPH_REQUIRED_LEVEL);
}

void EverQuestMod::AddHearthstoneForNewCharacter(Player* player)
{
    if (ConfigPlayerAddHearthstoneToNewCharacters == false)
        return;
    if (player->HasItemCount(EQ_HEARTHSTONE_ITEM_ID, 1, true) == true)
        return;

    ItemPosCountVec destPosition;
    InventoryResult invResult = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, destPosition, EQ_HEARTHSTONE_ITEM_ID, 1);
    if (invResult == EQUIP_ERR_OK)
        player->StoreNewItem(destPosition, EQ_HEARTHSTONE_ITEM_ID, true);
}

bool EverQuestMod::IsItemTemplateAMasterTotem(Player* player, ItemTemplate const* itemTemplate)
{
    // A master totem is a single item that satisfies the air, earth, fire and water totem requirements all by itself
    if (itemTemplate == nullptr || itemTemplate->TotemCategory == 0)
        return false;
    if (player->IsTotemCategoryCompatiableWith(itemTemplate, TC_EARTH_TOTEM) == false)
        return false;
    if (player->IsTotemCategoryCompatiableWith(itemTemplate, TC_AIR_TOTEM) == false)
        return false;
    if (player->IsTotemCategoryCompatiableWith(itemTemplate, TC_FIRE_TOTEM) == false)
        return false;
    if (player->IsTotemCategoryCompatiableWith(itemTemplate, TC_WATER_TOTEM) == false)
        return false;
    return true;
}

bool EverQuestMod::IsPlayerCarryingMasterTotem(Player* player)
{
    // Equipped items, equipped bag slots and the backpack (the bank is intentionally not searched)
    for (uint8 slotIndex = EQUIPMENT_SLOT_START; slotIndex < INVENTORY_SLOT_ITEM_END; ++slotIndex)
    {
        Item* curItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slotIndex);
        if (curItem != nullptr && IsItemTemplateAMasterTotem(player, curItem->GetTemplate()) == true)
            return true;
    }

    // Contents of any equipped bags
    for (uint8 bagSlotIndex = INVENTORY_SLOT_BAG_START; bagSlotIndex < INVENTORY_SLOT_BAG_END; ++bagSlotIndex)
    {
        Bag* curBag = player->GetBagByPos(bagSlotIndex);
        if (curBag == nullptr)
            continue;
        for (uint8 itemSlotIndex = 0; itemSlotIndex < curBag->GetBagSize(); ++itemSlotIndex)
        {
            Item* curItem = player->GetItemByPos(bagSlotIndex, itemSlotIndex);
            if (curItem != nullptr && IsItemTemplateAMasterTotem(player, curItem->GetTemplate()) == true)
                return true;
        }
    }

    return false;
}

void EverQuestMod::AddMasterTotemForShaman(Player* player)
{
    if (ConfigPlayerAddMasterTotemToShamans == false)
        return;
    if (player->getClass() != CLASS_SHAMAN)
        return;
    if (IsPlayerCarryingMasterTotem(player) == true)
        return;

    ItemPosCountVec destPosition;
    InventoryResult invResult = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, destPosition, EQ_MASTER_TOTEM_ITEM_ID, 1);
    if (invResult == EQUIP_ERR_OK)
        player->StoreNewItem(destPosition, EQ_MASTER_TOTEM_ITEM_ID, true);
}

void EverQuestMod::AddRacialGuiseItemForPlayer(Player* player)
{
    if (ConfigPlayerAddRacialGuiseItemOnLogin == false)
        return;
    if (HasCreatePlayerData(player->getRace(), player->getClass()) == false)
        return;
    uint32 guiseItemID = GetPlayerCreateInfo(player->getRace(), player->getClass()).IllusionItemID;
    if (guiseItemID == 0)
        return;
    if (GetIssuedIllusionItemIDForPlayer(player) != 0)
        return;

    // Only mark it if the player got it, otherwise try again next time
    ItemPosCountVec destPosition;
    InventoryResult invResult = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, destPosition, guiseItemID, 1);
    if (invResult != EQUIP_ERR_OK)
        return;
    if (player->StoreNewItem(destPosition, guiseItemID, true) != nullptr)
        SetIssuedIllusionItemIDForPlayer(player, guiseItemID);
}

void EverQuestMod::ApplyCorpseIllusionNativeDisplayOnDeath(Player* player)
{
    // Corspe object copies the native display id at creation and clients only pick the corpse model from the first recieved packet and nothing updates it
    // so while an illusion that persists through death is at play, make the native display the same as the illusion display while the body exists
    if (player->GetDisplayId() == player->GetNativeDisplayId())
        return;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        if (CorpseIllusionOriginalNativeDisplayByPlayerGUID.find(player->GetGUID()) != CorpseIllusionOriginalNativeDisplayByPlayerGUID.end())
            return;
        CorpseIllusionOriginalNativeDisplayByPlayerGUID[player->GetGUID()] = player->GetNativeDisplayId();
    }
    player->SetNativeDisplayId(player->GetDisplayId());
}

void EverQuestMod::RestoreNativeDisplayAfterCorpseIllusion(Player* player)
{
    uint32 originalNativeDisplayID = 0;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto storedItr = CorpseIllusionOriginalNativeDisplayByPlayerGUID.find(player->GetGUID());
        if (storedItr == CorpseIllusionOriginalNativeDisplayByPlayerGUID.end())
            return;
        originalNativeDisplayID = storedItr->second;
        CorpseIllusionOriginalNativeDisplayByPlayerGUID.erase(storedItr);
    }
    player->SetNativeDisplayId(originalNativeDisplayID);
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

void EverQuestMod::ApplyAdventurerAuraStateOnLogin(Player* player)
{
    if (ConfigSystemAdventurerAuraSpellID == 0)
        return;

    bool hasAura = player->HasAura(ConfigSystemAdventurerAuraSpellID);
    if (IsPlayerDisqualifiedFromAdventurer(player) == true)
    {
        // Losing it is permanent, so clear the aura if it came back through some other path
        if (hasAura == true)
            player->RemoveAura(ConfigSystemAdventurerAuraSpellID);
        return;
    }

    if (hasAura == false)
        player->CastSpell(player, ConfigSystemAdventurerAuraSpellID, true);
}

bool EverQuestMod::IsMapIDAnEverQuestMap(uint32 mapID)
{
    if (ConfigSystemMapDBCIDMin == 0 || ConfigSystemMapDBCIDMax == 0)
        return false;
    if (mapID < ConfigSystemMapDBCIDMin || mapID > ConfigSystemMapDBCIDMax)
        return false;
    return true;
}

bool EverQuestMod::IsZoneWideGroupRewardEnabledForMap(uint32 mapID)
{
    if (ConfigGroupZoneWideLootAndExperienceEnabled == false)
        return false;
    return IsMapIDAnEverQuestMap(mapID);
}

bool EverQuestMod::IsInZoneWideGroupRewardRange(Player* member, WorldObject* rewardSource)
{
    if (member == nullptr || rewardSource == nullptr)
        return false;
    return member->IsInMap(rewardSource);
}

// Mirrors KillRewarder::_GetPlayerLevel so the level cap hook still gets a say in what counts for experience
uint8 EverQuestMod::GetPlayerLevelForExperienceGain(Player* player)
{
    uint8 level = player->GetLevel();
    sScriptMgr->OnPlayerBeforeGetLevelForXPGain(player, level);
    return level;
}

// Rebuilds the totals KillRewarder::_InitGroupData produces, over every group member in the zone rather than only those within the core's group reward distance
void EverQuestMod::BuildZoneWideKillReward(Group* group, Player* killer, Unit* victim, EverQuestZoneWideKillReward& outReward)
{
    if (group == nullptr || killer == nullptr || victim == nullptr)
        return;
    if (IsZoneWideGroupRewardEnabledForMap(victim->GetMapId()) == false)
        return;

    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (member == nullptr)
            continue;
        if (member != killer && IsInZoneWideGroupRewardRange(member, victim) == false)
            continue;

        uint8 memberLevel = GetPlayerLevelForExperienceGain(member);
        if (member->IsAlive() == true)
        {
            outReward.AliveMemberCount++;
            outReward.AliveSumLevel += memberLevel;
            if (outReward.MaxLevel < memberLevel)
                outReward.MaxLevel = memberLevel;

            uint32 grayLevel = Acore::XP::GetGrayLevel(memberLevel);
            if (victim->GetLevel() > grayLevel && (outReward.MaxNotGrayMember == nullptr || outReward.MaxNotGrayMemberLevel < memberLevel))
            {
                outReward.MaxNotGrayMember = member;
                outReward.MaxNotGrayMemberLevel = memberLevel;
            }
        }
    }

    // Nobody alive in the zone means the core awarded nothing either, so there is nothing
    if (outReward.MaxLevel == 0 || outReward.AliveSumLevel == 0)
        return;

    outReward.IsFullXP = outReward.MaxNotGrayMember != nullptr && (outReward.MaxLevel == outReward.MaxNotGrayMemberLevel);

    // Base experience comes from the highest level member the victim is not gray to, matching KillRewarder::_InitXP
    if (outReward.MaxNotGrayMember != nullptr)
    {
        outReward.BaseExperience = Acore::XP::Gain(outReward.MaxNotGrayMember, victim, false);
        if (outReward.BaseExperience > 0 && victim->IsCreature() == true)
        {
            CreatureTemplate const* creatureTemplate = victim->ToCreature()->GetCreatureTemplate();
            if (creatureTemplate != nullptr && creatureTemplate->ModHealth <= 0.75f && creatureTemplate->ModHealth >= 0.0f)
                outReward.BaseExperience = static_cast<uint32>(outReward.BaseExperience * creatureTemplate->ModHealth);
        }
    }
    bool isPvPKill = false;
    if (victim->IsPlayer() == true)
        isPvPKill = true;
    else if (victim->GetCharmerOrOwnerGUID().IsPlayer() == true)
        isPvPKill = victim->IsVehicle() == false;

    bool isRaidKill = false;
    if (isPvPKill == false && group->isRaidGroup() == true)
    {
        MapEntry const* mapEntry = sMapStore.LookupEntry(killer->GetMapId());
        isRaidKill = mapEntry != nullptr && mapEntry->IsRaid();
    }

    outReward.GroupRate = Acore::XP::xp_in_group_rate(outReward.AliveMemberCount, isRaidKill);
    outReward.IsValid = true;
}

float EverQuestMod::GetZoneWideGroupExperienceRate(Player* player, const EverQuestZoneWideKillReward& reward)
{
    if (reward.IsValid == false || reward.AliveSumLevel == 0)
        return 1.0f;
    return reward.GroupRate * static_cast<float>(GetPlayerLevelForExperienceGain(player)) / static_cast<float>(reward.AliveSumLevel);
}

float EverQuestMod::GetGroupExperienceRateForMember(Player* member, const EverQuestZoneWideKillReward& reward)
{
    // The alternate formula is an even split plus a bonus per added member, and only covers party sized groups
    if (ConfigAlternateGroupExperienceFormulaEnabled == true && reward.AliveMemberCount >= 2 && reward.AliveMemberCount <= 5)
    {
        float bonusTotalRatePercent = static_cast<float>(reward.AliveMemberCount - 1) * (ConfigAlternateGroupExperienceAddPercentPerAddedMember * 0.01f);
        float splitBaseRate = 1.0f / static_cast<float>(reward.AliveMemberCount);
        return splitBaseRate * (1.0f + bonusTotalRatePercent);
    }

    return GetZoneWideGroupExperienceRate(member, reward);
}

void EverQuestMod::ApplyEQOnkillReputationsForPlayer(Player* player, Unit* victim)
{
    if (player == nullptr || victim == nullptr || victim->IsPlayer() == true)
        return;
    Creature* victimCreature = victim->ToCreature();
    if (victimCreature == nullptr || victimCreature->IsReputationRewardDisabled() == true)
        return;

    const list<EverQuestCreatureOnkillReputation>& onkillReputations = GetOnkillReputationsForCreatureTemplate(victimCreature->GetCreatureTemplate()->Entry);
    for (const auto& onkillReputation : onkillReputations)
    {
        float repChange = player->CalculateReputationGain(REPUTATION_SOURCE_KILL, victim->GetLevel(), static_cast<float>(onkillReputation.KillRewardValue), onkillReputation.FactionID);

        FactionEntry const* factionEntry = sFactionStore.LookupEntry(onkillReputation.FactionID);
        if (factionEntry && repChange != 0)
            player->GetReputationMgr().ModifyReputation(factionEntry, repChange, false, static_cast<ReputationRank>(7));
    }
}

void EverQuestMod::GrantZoneWideGroupRewardsForKill(Player* killer, Unit* victim, const EverQuestZoneWideKillReward& reward)
{
    if (reward.IsValid == false)
        return;
    if (killer == nullptr || victim == nullptr)
        return;

    Group* group = killer->GetGroup();
    if (group == nullptr)
        return;

    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (member == nullptr || member == killer)
            continue;

        // Anything the core already paid out is left alone
        if (member->IsAtGroupRewardDistance(victim) == true)
            continue;
        if (IsInZoneWideGroupRewardRange(member, victim) == false)
            continue;

        // The core rewards reputation off the back of the kill whether or not any experience came with it, and a dead member still earns it, so this runs ahead of the experience rules and outside of them
        member->RewardReputation(victim);
        ApplyEQOnkillReputationsForPlayer(member, victim);

        if (reward.BaseExperience == 0)
            continue;

        // Mirrors KillRewarder::_RewardXP: gray members earn nothing and a partly gray group is only worth half
        uint32 experience = 0;
        if (member->IsAlive() == true && reward.MaxNotGrayMemberLevel >= GetPlayerLevelForExperienceGain(member))
        {
            float memberRate = GetGroupExperienceRateForMember(member, reward);
            if (reward.IsFullXP == true)
                experience = static_cast<uint32>(reward.BaseExperience * memberRate);
            else
                experience = static_cast<uint32>(reward.BaseExperience * memberRate / 2) + 1;
        }
        if (experience == 0)
            continue;

        // The core's power leveling guard, applied here too so distance is not a way around it
        if (victim->IsCreature() == true)
        {
            uint8 highestAttackerLevel = victim->ToCreature()->GetHighestPlayerAttackerLevel();
            if (highestAttackerLevel > reward.MaxLevel && victim->GetLevel() <= Acore::XP::GetGrayLevel(highestAttackerLevel))
                experience = experience / 2 + 1;
        }

        experience = static_cast<uint32>(experience * member->GetTotalAuraMultiplier(SPELL_AURA_MOD_XP_PCT));
        sScriptMgr->OnPlayerGiveXP(member, experience, victim, PlayerXPSource::XPSOURCE_KILL);
        member->GiveXP(experience, victim, reward.GroupRate);

        Pet* pet = member->GetPet();
        if (pet != nullptr)
            pet->GivePetXP(experience / 2);
    }
}

void EverQuestMod::ApplyZoneWideGroupLootAccess(Loot* loot, Player* lootOwner, bool personal)
{
    if (loot == nullptr || lootOwner == nullptr || personal == true)
        return;
    if (IsZoneWideGroupRewardEnabledForMap(lootOwner->GetMapId()) == false)
        return;

    Group* group = lootOwner->GetGroup();
    if (group == nullptr)
        return;

    Map* map = lootOwner->GetMap();
    if (map == nullptr)
        return;
    Creature* lootSource = map->GetCreature(loot->sourceWorldObjectGUID);
    if (lootSource == nullptr)
        return;

    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (member == nullptr || member->GetSession() == nullptr)
            continue;

        // Creature::SetLootRecipient leaves these two off the list it builds for bosses, and converted content includes instanced maps where that list is already in play, so the same exclusion is kept here
        if (member->IsGameMaster() == true || member->IsSpectator() == true)
            continue;
        if (IsInZoneWideGroupRewardRange(member, lootSource) == false)
            continue;
        lootSource->AddAllowedLooter(member->GetGUID());
        if (member->IsAtLootRewardDistance(lootSource) == false)
            loot->FillNotNormalLootFor(member);
    }
}

void EverQuestMod::ApplyZoneWideGroupMoneyShare(Player* looter, Loot* loot)
{
    if (looter == nullptr || loot == nullptr || loot->gold == 0)
        return;
    if (IsZoneWideGroupRewardEnabledForMap(looter->GetMapId()) == false)
        return;

    Group* group = looter->GetGroup();
    if (group == nullptr)
        return;

    // Only a killed creature's corpse splits its coin, so container, gameobject and pickpocket loot is left untouched
    if (loot->loot_type != LOOT_CORPSE || loot->containerGUID.IsEmpty() == false || loot->sourceGameObject != nullptr)
        return;
    Map* map = looter->GetMap();
    if (map == nullptr)
        return;
    Creature* lootSource = map->GetCreature(loot->sourceWorldObjectGUID);
    if (lootSource == nullptr || lootSource->IsAlive() == true)
        return;

    uint32 nearMemberCount = 0;
    uint32 totalMemberCount = 0;
    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (member == nullptr)
            continue;

        // The same test the core uses to build its near list
        if (looter->IsAtLootRewardDistance(member) == true)
            nearMemberCount++;
        else if (IsInZoneWideGroupRewardRange(member, lootSource) == true)
            totalMemberCount++;
    }
    if (totalMemberCount == 0 || nearMemberCount == 0)
        return;
    totalMemberCount += nearMemberCount;

    uint32 goldPerPlayer = loot->gold / totalMemberCount;
    if (goldPerPlayer == 0)
        return;

    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (member == nullptr)
            continue;
        if (looter->IsAtLootRewardDistance(member) == true)
            continue;
        if (IsInZoneWideGroupRewardRange(member, lootSource) == false)
            continue;

        uint32 finalGold = goldPerPlayer;
        if (member->HasPlayerFlag(PLAYER_FLAGS_NO_PLAY_TIME) == true)
            continue;
        if (member->HasPlayerFlag(PLAYER_FLAGS_PARTIAL_PLAY_TIME) == true)
        {
            finalGold /= 2;
            if (finalGold == 0)
                continue;
        }

        member->ModifyMoney(finalGold);
        member->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY, finalGold);

        WorldPacket moneyPacket(SMSG_LOOT_MONEY_NOTIFY, 4 + 1);
        moneyPacket << uint32(finalGold);
        moneyPacket << uint8(0); // "Your share is..."
        member->SendDirectMessage(&moneyPacket);
    }

    // What is left is one share for each member the core is about to pay
    loot->gold = goldPerPlayer * nearMemberCount;
}

bool EverQuestMod::IsCreatureKillDisqualifyingForAdventurer(Player* player, Unit* victim)
{
    if (player == nullptr || victim == nullptr || victim->IsCreature() == false)
        return false;
    if (ConfigSystemCreatureTemplateIDMin == 0 || ConfigSystemCreatureTemplateIDMax == 0)
        return false;

    // Only a kill made outside of an EverQuest map can ever count against the player
    if (IsMapIDAnEverQuestMap(player->GetMapId()) == true)
        return false;

    // Pets, guardians, totems, vehicles and other summons should be ignored since their parent is what matters
    if (victim->IsSummon() == true || victim->IsPet() == true || victim->IsGuardian() == true || victim->IsTotem() == true || victim->IsVehicle() == true)
        return false;
    if (victim->IsControlledByPlayer() == true)
        return false;
    if (victim->GetCharmerOrOwnerGUID().IsEmpty() == false)
        return false;

    uint32 victimEntry = victim->GetEntry();
    if (victimEntry >= ConfigSystemCreatureTemplateIDMin && victimEntry <= ConfigSystemCreatureTemplateIDMax)
        return false;
    return true;
}

bool EverQuestMod::IsQuestDisqualifyingForAdventurer(Player* player, uint32 questID)
{
    if (player == nullptr)
        return false;
    if (ConfigSystemQuestSQLIDMin == 0 || ConfigSystemQuestSQLIDMax == 0)
        return false;

    // Only a quest completed outside of an EverQuest map can ever count against the player
    if (IsMapIDAnEverQuestMap(player->GetMapId()) == true)
        return false;
    if (questID >= ConfigSystemQuestSQLIDMin && questID <= ConfigSystemQuestSQLIDMax)
        return false;
    return true;
}

bool EverQuestMod::IsPlayerDisqualifiedFromAdventurer(Player* player)
{
    return GetOrLoadActivePlayerClassControllerData(player)->AdventurerDisqualified;
}

bool EverQuestMod::DisqualifyPlayerFromAdventurer(Player* player)
{
    if (ConfigSystemAdventurerAuraSpellID == 0)
        return false;

    EverQuestPlayerControllerData& controllerData = *GetOrLoadActivePlayerClassControllerData(player);
    if (controllerData.AdventurerDisqualified == true)
        return false;
    controllerData.AdventurerDisqualified = true;
    SaveAdventurerDisqualifiedForPlayer(player);

    // This is the one moment the buff is lost, so the levels are recorded here for `.eqadventurer restore` to put back
    SaveAdventurerLevelSnapshotForPlayer(player);

    if (player->HasAura(ConfigSystemAdventurerAuraSpellID) == true)
        player->RemoveAura(ConfigSystemAdventurerAuraSpellID);
    return true;
}

void EverQuestMod::SaveAdventurerDisqualifiedForPlayer(Player* player)
{
    EverQuestPlayerControllerData controllerData;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto controllerDataIt = ActivePlayerClassControllerDataByGUID.find(player->GetGUID());
        if (controllerDataIt == ActivePlayerClassControllerDataByGUID.end())
            return;
        controllerData = controllerDataIt->second;
    }

    CharacterDatabase.Execute("INSERT INTO `mod_everquest_character_settings` (`guid`, `currentSecondaryClass`, `nextSecondaryClass`, `secondaryExpPool`, `adventurerDisqualified`) VALUES ({}, {}, {}, {}, {}) ON DUPLICATE KEY UPDATE `adventurerDisqualified` = {}",
        player->GetGUID().GetCounter(),
        controllerData.CurrentSecondClass,
        controllerData.NextSecondClass,
        controllerData.SecondaryExpPool,
        controllerData.AdventurerDisqualified == true ? 1 : 0,
        controllerData.AdventurerDisqualified == true ? 1 : 0);
}

// Columns are explicit for EQ classes so resolve it here
static const char* const EQ_ADVENTURER_LOSS_LEVEL_COLUMNS[EQ_EQCLASS_HIGHEST_ID + 1] =
{
    "levelNone", "levelWarrior", "levelCleric", "levelPaladin", "levelRanger", "levelShadowKnight", "levelDruid", "levelMonk",
    "levelBard", "levelRogue", "levelShaman", "levelNecromancer", "levelWizard", "levelMagician", "levelEnchanter"
};

static std::string GetEquipmentSlotNameFromSlot(uint8 equipSlot)
{
    switch (equipSlot)
    {
    case EQUIPMENT_SLOT_HEAD:       return "Head";
    case EQUIPMENT_SLOT_NECK:       return "Neck";
    case EQUIPMENT_SLOT_SHOULDERS:  return "Shoulders";
    case EQUIPMENT_SLOT_BODY:       return "Shirt";
    case EQUIPMENT_SLOT_CHEST:      return "Chest";
    case EQUIPMENT_SLOT_WAIST:      return "Waist";
    case EQUIPMENT_SLOT_LEGS:       return "Legs";
    case EQUIPMENT_SLOT_FEET:       return "Feet";
    case EQUIPMENT_SLOT_WRISTS:     return "Wrists";
    case EQUIPMENT_SLOT_HANDS:      return "Hands";
    case EQUIPMENT_SLOT_FINGER1:    return "Finger 1";
    case EQUIPMENT_SLOT_FINGER2:    return "Finger 2";
    case EQUIPMENT_SLOT_TRINKET1:   return "Trinket 1";
    case EQUIPMENT_SLOT_TRINKET2:   return "Trinket 2";
    case EQUIPMENT_SLOT_BACK:       return "Back";
    case EQUIPMENT_SLOT_MAINHAND:   return "Main Hand";
    case EQUIPMENT_SLOT_OFFHAND:    return "Off Hand";
    case EQUIPMENT_SLOT_RANGED:     return "Ranged";
    case EQUIPMENT_SLOT_TABARD:     return "Tabard";
    default:                        return "Slot " + std::to_string(uint32(equipSlot));
    }
}

void EverQuestMod::SaveAdventurerLevelSnapshotForPlayer(Player* player)
{
    if (player == nullptr)
        return;

    uint8 levelsByEQClass[EQ_EQCLASS_HIGHEST_ID + 1] = {};
    map<uint8, uint8> currentLevelsByEQClass = GetClassLevelsByClassForPlayer(player);
    for (auto const& levelPair : currentLevelsByEQClass)
    {
        if (levelPair.first > EQ_EQCLASS_HIGHEST_ID)
        {
            LOG_ERROR("module.EverQuest", "EverQuestMod SaveAdventurerLevelSnapshotForPlayer saw EQ class {} for player guid {}, which is past the highest known class {} and has no column to be stored in",
                uint32(levelPair.first), player->GetGUID().GetCounter(), uint32(EQ_EQCLASS_HIGHEST_ID));
            continue;
        }
        levelsByEQClass[levelPair.first] = levelPair.second;
    }

    CharacterDatabase.Execute("INSERT INTO `mod_everquest_character_adventurer_loss` "
        "(`guid`, `lossTimestamp`, `lossSecondaryClass`, `levelNone`, `levelWarrior`, `levelCleric`, `levelPaladin`, `levelRanger`, `levelShadowKnight`, `levelDruid`, `levelMonk`, `levelBard`, `levelRogue`, `levelShaman`, `levelNecromancer`, `levelWizard`, `levelMagician`, `levelEnchanter`) "
        "VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}) "
        "ON DUPLICATE KEY UPDATE `lossTimestamp` = VALUES(`lossTimestamp`), `lossSecondaryClass` = VALUES(`lossSecondaryClass`), "
        "`levelNone` = VALUES(`levelNone`), `levelWarrior` = VALUES(`levelWarrior`), `levelCleric` = VALUES(`levelCleric`), `levelPaladin` = VALUES(`levelPaladin`), "
        "`levelRanger` = VALUES(`levelRanger`), `levelShadowKnight` = VALUES(`levelShadowKnight`), `levelDruid` = VALUES(`levelDruid`), `levelMonk` = VALUES(`levelMonk`), "
        "`levelBard` = VALUES(`levelBard`), `levelRogue` = VALUES(`levelRogue`), `levelShaman` = VALUES(`levelShaman`), `levelNecromancer` = VALUES(`levelNecromancer`), "
        "`levelWizard` = VALUES(`levelWizard`), `levelMagician` = VALUES(`levelMagician`), `levelEnchanter` = VALUES(`levelEnchanter`)",
        player->GetGUID().GetCounter(),
        uint32(GameTime::GetGameTime().count()),
        uint32(GetCurrentSecondEQClassForPlayer(player)),
        uint32(levelsByEQClass[EQ_EQCLASS_NONE]),
        uint32(levelsByEQClass[EQ_EQCLASS_WARRIOR]),
        uint32(levelsByEQClass[EQ_EQCLASS_CLERIC]),
        uint32(levelsByEQClass[EQ_EQCLASS_PALADIN]),
        uint32(levelsByEQClass[EQ_EQCLASS_RANGER]),
        uint32(levelsByEQClass[EQ_EQCLASS_SHADOWKNIGHT]),
        uint32(levelsByEQClass[EQ_EQCLASS_DRUID]),
        uint32(levelsByEQClass[EQ_EQCLASS_MONK]),
        uint32(levelsByEQClass[EQ_EQCLASS_BARD]),
        uint32(levelsByEQClass[EQ_EQCLASS_ROGUE]),
        uint32(levelsByEQClass[EQ_EQCLASS_SHAMAN]),
        uint32(levelsByEQClass[EQ_EQCLASS_NECROMANCER]),
        uint32(levelsByEQClass[EQ_EQCLASS_WIZARD]),
        uint32(levelsByEQClass[EQ_EQCLASS_MAGICIAN]),
        uint32(levelsByEQClass[EQ_EQCLASS_ENCHANTER]));
}

bool EverQuestMod::TryGetAdventurerLevelSnapshotForPlayerGUID(uint32 playerGUIDCounter, EverQuestAdventurerLossSnapshot& snapshotOut)
{
    std::ostringstream columnList;
    for (uint8 eqClassID = EQ_EQCLASS_NONE; eqClassID <= EQ_EQCLASS_HIGHEST_ID; eqClassID++)
        columnList << ", `" << EQ_ADVENTURER_LOSS_LEVEL_COLUMNS[eqClassID] << "`";

    QueryResult queryResult = CharacterDatabase.Query("SELECT `lossTimestamp`, `lossSecondaryClass`{} FROM `mod_everquest_character_adventurer_loss` WHERE `guid` = {}", columnList.str(), playerGUIDCounter);
    if (!queryResult)
        return false;

    Field* fields = queryResult->Fetch();
    snapshotOut.Exists = true;
    snapshotOut.LossTimestamp = fields[0].Get<uint32>();
    snapshotOut.LossSecondaryClass = fields[1].Get<uint8>();
    for (uint8 eqClassID = EQ_EQCLASS_NONE; eqClassID <= EQ_EQCLASS_HIGHEST_ID; eqClassID++)
        snapshotOut.LevelsByEQClass[eqClassID] = fields[2 + eqClassID].Get<uint8>();
    return true;
}

void EverQuestMod::CollectNonEverQuestItemsFromLiveInventory(Player* player, vector<EverQuestAdventurerStrippedItem>& strippedItems, EverQuestAdventurerRestoreReport& report)
{
    // Positions are recorded first, since detaching an item changes what the slots hold
    vector<std::pair<std::pair<uint8, uint8>, std::string>> positionsToStrip;

    for (uint8 equipSlot = EQUIPMENT_SLOT_START; equipSlot < EQUIPMENT_SLOT_END; equipSlot++)
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, equipSlot);
        if (item == nullptr || IsItemTemplateIDAnEQItemTemplateID(item->GetEntry()) == true)
            continue;
        positionsToStrip.push_back(std::make_pair(std::make_pair(uint8(INVENTORY_SLOT_BAG_0), equipSlot), "equipped, " + GetEquipmentSlotNameFromSlot(equipSlot) + ", as " + GetEQClassStringFromID(GetCurrentSecondEQClassForPlayer(player))));
    }

    for (uint8 backpackSlot = INVENTORY_SLOT_ITEM_START; backpackSlot < INVENTORY_SLOT_ITEM_END; backpackSlot++)
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, backpackSlot);
        if (item == nullptr || IsItemTemplateIDAnEQItemTemplateID(item->GetEntry()) == true)
            continue;
        positionsToStrip.push_back(std::make_pair(std::make_pair(uint8(INVENTORY_SLOT_BAG_0), backpackSlot), "backpack"));
    }

    for (uint8 keyringSlot = KEYRING_SLOT_START; keyringSlot < KEYRING_SLOT_END; keyringSlot++)
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, keyringSlot);
        if (item == nullptr || IsItemTemplateIDAnEQItemTemplateID(item->GetEntry()) == true)
            continue;
        positionsToStrip.push_back(std::make_pair(std::make_pair(uint8(INVENTORY_SLOT_BAG_0), keyringSlot), "keyring"));
    }

    for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot < INVENTORY_SLOT_BAG_END; bagSlot++)
    {
        Bag* bag = player->GetBagByPos(bagSlot);
        if (bag == nullptr)
            continue;
        for (uint32 bagIndex = 0; bagIndex < bag->GetBagSize(); bagIndex++)
        {
            Item* item = player->GetItemByPos(bagSlot, uint8(bagIndex));
            if (item == nullptr || IsItemTemplateIDAnEQItemTemplateID(item->GetEntry()) == true)
                continue;
            positionsToStrip.push_back(std::make_pair(std::make_pair(bagSlot, uint8(bagIndex)), "bag in slot " + std::to_string(uint32(bagSlot - INVENTORY_SLOT_BAG_START + 1))));
        }
    }

    for (auto const& positionToStrip : positionsToStrip)
    {
        Item* item = player->GetItemByPos(positionToStrip.first.first, positionToStrip.first.second);
        if (item == nullptr)
            continue;
        EverQuestAdventurerStrippedItem strippedItem;
        strippedItem.SourceDescription = positionToStrip.second;
        strippedItem.WasLiveOnPlayer = true;
        player->MoveItemFromInventory(positionToStrip.first.first, positionToStrip.first.second, true);
        strippedItem.StrippedItem = item;
        strippedItems.push_back(strippedItem);
    }

    // Equipped bags come last, now that everything non-EverQuest inside them is gone
    for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot < INVENTORY_SLOT_BAG_END; bagSlot++)
    {
        Bag* bag = player->GetBagByPos(bagSlot);
        if (bag == nullptr || IsItemTemplateIDAnEQItemTemplateID(bag->GetEntry()) == true)
            continue;

        // Whatever EverQuest gear is still inside needs somewhere else to live before the bag can go
        bool everyRemainingItemRelocated = true;
        for (uint32 bagIndex = 0; bagIndex < bag->GetBagSize(); bagIndex++)
        {
            Item* remainingItem = player->GetItemByPos(bagSlot, uint8(bagIndex));
            if (remainingItem == nullptr)
                continue;

            ItemPosCountVec relocationDestination;
            if (player->CanStoreItem(NULL_BAG, NULL_SLOT, relocationDestination, remainingItem, true) != EQUIP_ERR_OK)
            {
                everyRemainingItemRelocated = false;
                break;
            }
            player->MoveItemFromInventory(bagSlot, uint8(bagIndex), true);
            remainingItem->SetState(ITEM_UNCHANGED);
            player->MoveItemToInventory(relocationDestination, remainingItem, true);
        }

        if (everyRemainingItemRelocated == false)
        {
            report.Notes.push_back("Left the container '" + bag->GetTemplate()->Name1 + "' in bag slot " + std::to_string(uint32(bagSlot - INVENTORY_SLOT_BAG_START + 1)) + " in place, as the EverQuest items inside it had nowhere else to go.");
            continue;
        }

        EverQuestAdventurerStrippedItem strippedItem;
        strippedItem.SourceDescription = "equipped container, bag slot " + std::to_string(uint32(bagSlot - INVENTORY_SLOT_BAG_START + 1));
        strippedItem.WasLiveOnPlayer = true;
        player->MoveItemFromInventory(INVENTORY_SLOT_BAG_0, bagSlot, true);
        strippedItem.StrippedItem = bag;
        strippedItems.push_back(strippedItem);
    }
}

void EverQuestMod::CollectNonEverQuestItemsFromClassStorage(Player* player, vector<EverQuestAdventurerStrippedItem>& strippedItems, CharacterDatabaseTransaction& transaction)
{
    QueryResult queryResult = CharacterDatabase.Query("SELECT storage.`eqclass`, storage.`slot`, storage.`item`, instance.`itemEntry` FROM `mod_everquest_character_class_inventory` storage "
        "JOIN `item_instance` instance ON instance.`guid` = storage.`item` WHERE storage.`guid` = {}", player->GetGUID().GetCounter());
    if (!queryResult)
        return;

    do
    {
        Field* fields = queryResult->Fetch();
        uint8 eqClassID = fields[0].Get<uint8>();
        uint8 equipSlot = fields[1].Get<uint8>();
        uint32 itemGUIDCounter = fields[2].Get<uint32>();
        uint32 itemEntry = fields[3].Get<uint32>();

        if (IsItemTemplateIDAnEQItemTemplateID(itemEntry) == true)
            continue;

        // A stored row whose item is live on the player is the stale kind the switch-in restore leaves behind, and the live copy was already handled by the inventory sweep
        Item* item = LoadDetachedItemForPlayer(itemGUIDCounter, player);
        if (item == nullptr)
        {
            transaction->Append("DELETE FROM `mod_everquest_character_class_inventory` WHERE `item` = {}", itemGUIDCounter);
            continue;
        }

        EverQuestAdventurerStrippedItem strippedItem;
        strippedItem.StrippedItem = item;
        strippedItem.WasLiveOnPlayer = false;
        strippedItem.SourceDescription = "equipped, " + GetEquipmentSlotNameFromSlot(equipSlot) + ", stored as " + GetEQClassStringFromID(eqClassID);
        strippedItems.push_back(strippedItem);

        transaction->Append("DELETE FROM `mod_everquest_character_class_inventory` WHERE `item` = {}", itemGUIDCounter);
    } while (queryResult->NextRow());
}

bool EverQuestMod::RestoreAdventurerForPlayer(Player* targetPlayer, Player* gmPlayer, EverQuestAdventurerRestoreReport& reportOut)
{
    if (IsEnabled == false)
    {
        reportOut.FailureReason = "The EverQuest module is disabled.";
        return false;
    }
    if (targetPlayer == nullptr || gmPlayer == nullptr || targetPlayer->GetSession() == nullptr || gmPlayer->GetSession() == nullptr)
    {
        reportOut.FailureReason = "Both the target character and the game master have to be online.";
        return false;
    }
    if (ConfigSystemAdventurerAuraSpellID == 0)
    {
        reportOut.FailureReason = "No EverQuest Adventurer aura spell is configured, so the buff cannot be given back.";
        return false;
    }

    if (ConfigSystemItemTemplateIDMin == 0 || ConfigSystemItemTemplateIDMax == 0)
    {
        reportOut.FailureReason = "The EverQuest item template ID range is not configured, so an EverQuest item cannot be told from a WoW one.";
        return false;
    }
    if (IsEquipmentStorageCommitPendingForPlayer(targetPlayer) == true)
    {
        reportOut.FailureReason = "That character has a secondary class equipment change still saving. Try again in a moment.";
        return false;
    }

    uint32 targetGUIDCounter = targetPlayer->GetGUID().GetCounter();
    CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
    AppendCharacterRowLockAnchor(transaction, targetGUIDCounter);

    vector<EverQuestAdventurerStrippedItem> strippedItems;
    CollectNonEverQuestItemsFromLiveInventory(targetPlayer, strippedItems, reportOut);
    CollectNonEverQuestItemsFromClassStorage(targetPlayer, strippedItems, transaction);

    // Every item that came off the character is handed to the GM, in mails of at most MAX_MAIL_ITEMS
    size_t mailedItemIndex = 0;
    while (mailedItemIndex < strippedItems.size())
    {
        size_t batchEnd = std::min(mailedItemIndex + MAX_MAIL_ITEMS, strippedItems.size());

        // The body has to be complete before the draft is built, so where each item came from is gathered up first
        std::ostringstream mailBody;
        mailBody << "Items taken from " << targetPlayer->GetName() << " by an EverQuest Adventurer restore:";
        for (size_t batchIndex = mailedItemIndex; batchIndex < batchEnd; batchIndex++)
        {
            Item* item = strippedItems[batchIndex].StrippedItem;
            mailBody << "\n" << item->GetTemplate()->Name1 << " (entry " << item->GetEntry() << ") - " << strippedItems[batchIndex].SourceDescription;
        }

        MailDraft draft("EverQuest Adventurer restore: " + targetPlayer->GetName(), mailBody.str());
        for (size_t batchIndex = mailedItemIndex; batchIndex < batchEnd; batchIndex++)
        {
            // MoveItemFromInventory already cleared any refund data, and a stored item never had inventory rows to begin with
            Item* item = strippedItems[batchIndex].StrippedItem;
            item->DeleteFromInventoryDB(transaction);
            if (item->GetState() == ITEM_UNCHANGED)
                item->FSetState(ITEM_CHANGED);
            item->SetOwnerGUID(gmPlayer->GetGUID());
            item->SaveToDB(transaction);
            draft.AddItem(item);
        }

        // Sent as if from the character itself, so the mail names who it came from
        draft.SendMailTo(transaction, MailReceiver(gmPlayer), MailSender(MAIL_NORMAL, targetGUIDCounter, MAIL_STATIONERY_GM), MAIL_CHECK_MASK_COPIED, 0, 90);
        reportOut.MailsSent++;
        mailedItemIndex = batchEnd;
    }
    reportOut.ItemsMailed = uint32(strippedItems.size());

    // The live inventory rows have to catch up with everything the sweep took off the character
    targetPlayer->SaveInventoryAndGoldToDB(transaction);

    // Levels come back only for a character that has a recorded loss
    uint8 pendingActiveClassLevel = 0;
    EverQuestAdventurerLossSnapshot snapshot;
    if (TryGetAdventurerLevelSnapshotForPlayerGUID(targetGUIDCounter, snapshot) == true)
    {
        reportOut.HadLevelSnapshot = true;
        uint8 activeSecondaryClass = GetCurrentSecondEQClassForPlayer(targetPlayer);
        map<uint8, uint8> currentLevelsByEQClass = GetClassLevelsByClassForPlayer(targetPlayer);

        // Walked by class rather than by what the character has now, so a profile that has since been dropped is reported instead of being passed over in silence
        for (uint8 eqClassID = EQ_EQCLASS_NONE; eqClassID <= EQ_EQCLASS_HIGHEST_ID; eqClassID++)
        {
            // A recorded level of zero means that class had never been played at the time of the loss, so it is left alone
            uint8 snapshotLevel = snapshot.LevelsByEQClass[eqClassID];
            if (snapshotLevel == 0)
                continue;

            auto currentLevelIter = currentLevelsByEQClass.find(eqClassID);
            if (currentLevelIter == currentLevelsByEQClass.end())
            {
                reportOut.Notes.push_back(GetEQClassStringFromID(eqClassID) + " was level " + std::to_string(uint32(snapshotLevel)) + " at the loss but has no stored profile any more, so nothing was put back for it.");
                continue;
            }
            if (snapshotLevel == currentLevelIter->second)
                continue;

            if (eqClassID == activeSecondaryClass)
            {
                pendingActiveClassLevel = snapshotLevel;
                continue;
            }

            // The parked profile's experience is progress into a level that is no longer the one it is on, so it starts over
            transaction->Append("UPDATE `mod_everquest_characters` SET `level` = {}, `xp` = 0 WHERE `guid` = {} AND `eqclass` = {}", uint32(snapshotLevel), targetGUIDCounter, uint32(eqClassID));
            reportOut.Notes.push_back(GetEQClassStringFromID(eqClassID) + " moved from level " + std::to_string(uint32(currentLevelIter->second)) + " back to level " + std::to_string(uint32(snapshotLevel)) + ".");
            reportOut.ClassLevelsChanged++;
        }
    }
    else
        reportOut.Notes.push_back("No recorded loss for this character, so levels were left exactly as they are.");

    // Committed through the equipment storage queue
    QueuePendingEquipmentStorageTransaction(targetPlayer, GetCurrentSecondEQClassForPlayer(targetPlayer), transaction);

    // The active character's level is in memory rather than in the transaction, and its own save cycle carries it to the database
    if (pendingActiveClassLevel != 0)
    {
        uint8 previousLevel = targetPlayer->GetLevel();
        targetPlayer->GiveLevel(pendingActiveClassLevel);
        targetPlayer->InitTalentForLevel();
        targetPlayer->SetUInt32Value(PLAYER_XP, 0);
        reportOut.Notes.push_back(GetEQClassStringFromID(GetCurrentSecondEQClassForPlayer(targetPlayer)) + " (the active profile) moved from level " + std::to_string(uint32(previousLevel)) + " back to level " + std::to_string(uint32(pendingActiveClassLevel)) + ".");
        reportOut.ClassLevelsChanged++;
    }

    // The buff itself
    EverQuestPlayerControllerData& controllerData = *GetOrLoadActivePlayerClassControllerData(targetPlayer);
    controllerData.AdventurerDisqualified = false;
    SaveAdventurerDisqualifiedForPlayer(targetPlayer);
    if (targetPlayer->HasAura(ConfigSystemAdventurerAuraSpellID) == false)
        targetPlayer->CastSpell(targetPlayer, ConfigSystemAdventurerAuraSpellID, true);

    LOG_INFO("module.EverQuest", "EverQuestMod restored the EverQuest Adventurer buff on player {} (guid {}) for game master {} (guid {}): {} items mailed across {} mails, {} class levels reset, level snapshot {}",
        targetPlayer->GetName(), targetGUIDCounter, gmPlayer->GetName(), gmPlayer->GetGUID().GetCounter(),
        reportOut.ItemsMailed, reportOut.MailsSent, reportOut.ClassLevelsChanged, reportOut.HadLevelSnapshot == true ? "found" : "not found");

    reportOut.Succeeded = true;
    return true;
}

void EverQuestMod::GrantAdventurerAchievementIfAccountEarned(Player* player)
{
    if (ConfigSystemAdventurerAchievementID == 0)
        return;
    if (player->HasAchieved(ConfigSystemAdventurerAchievementID) == true)
        return;

    AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(ConfigSystemAdventurerAchievementID);
    if (achievementEntry == nullptr)
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod::GrantAdventurerAchievementIfAccountEarned error, no achievement with ID {} exists", ConfigSystemAdventurerAchievementID);
        return;
    }

    // Only grant if a character on this account already earned it
    uint32 accountID = player->GetSession()->GetAccountId();
    QueryResult accountEarnedQueryResult = CharacterDatabase.Query("SELECT 1 FROM mod_everquest_account_settings WHERE accountid = {} AND earnedAdventurerAchievement = 1", accountID);
    if (!accountEarnedQueryResult)
        return;
    player->CompletedAchievement(achievementEntry);
}

void EverQuestMod::ProcessAdventurerStateOnLevelChange(Player* player)
{
    if (ConfigSystemAdventurerAchievementID == 0 || ConfigSystemAdventurerAuraSpellID == 0 || ConfigAchievementAdventurerLevel == 0)
        return;
    if (player->GetLevel() < ConfigAchievementAdventurerLevel)
        return;

    // The tracked setting decides this, not the aura, since the aura can be missing for reasons the player did not cause
    if (IsPlayerDisqualifiedFromAdventurer(player) == true)
        return;

    if (player->HasAchieved(ConfigSystemAdventurerAchievementID) == false)
    {
        AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(ConfigSystemAdventurerAchievementID);
        if (achievementEntry == nullptr)
        {
            LOG_ERROR("module.EverQuest", "EverQuestMod::ProcessAdventurerStateOnLevelChange error, no achievement with ID {} exists", ConfigSystemAdventurerAchievementID);
            return;
        }
        player->CompletedAchievement(achievementEntry);
    }

    // Record it account-wide so characters made later on this account also get the achievement
    CharacterDatabase.Execute("INSERT INTO mod_everquest_account_settings (accountid, earnedAdventurerAchievement) VALUES ({}, 1) ON DUPLICATE KEY UPDATE earnedAdventurerAchievement = 1", player->GetSession()->GetAccountId());
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

bool EverQuestMod::HasPreloadedLootItemIDsForCreatureGUID(Map* map, ObjectGuid creatureGUID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto mapIt = PreloadedLootItemIDsByMapInstanceKeyThenCreatureGUID.find(GetMapInstanceKey(map));
    if (mapIt == PreloadedLootItemIDsByMapInstanceKeyThenCreatureGUID.end())
        return false;
    return mapIt->second.find(creatureGUID) != mapIt->second.end();
}

bool EverQuestMod::HasPreloadedLootItemIDForCreatureGUID(Map* map, ObjectGuid creatureGUID, uint32 itemTemplateID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto mapIt = PreloadedLootItemIDsByMapInstanceKeyThenCreatureGUID.find(GetMapInstanceKey(map));
    if (mapIt == PreloadedLootItemIDsByMapInstanceKeyThenCreatureGUID.end())
        return false;
    auto preloadedIt = mapIt->second.find(creatureGUID);
    if (preloadedIt == mapIt->second.end())
        return false;

    for (uint32 preloadedLootItemTemplateID : preloadedIt->second)
    {
        if (preloadedLootItemTemplateID == itemTemplateID)
            return true;
    }
    return false;
}

uint32 EverQuestMod::GetPreloadedLootCountForCreatureGUID(Map* map, ObjectGuid creatureGUID, uint32 itemTemplateID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto mapIt = PreloadedLootCountsByMapInstanceKeyThenCreatureGUID.find(GetMapInstanceKey(map));
    if (mapIt == PreloadedLootCountsByMapInstanceKeyThenCreatureGUID.end())
        return 0;
    auto countsByItem = mapIt->second.find(creatureGUID);
    if (countsByItem == mapIt->second.end())
        return 0;
    auto count = countsByItem->second.find(itemTemplateID);
    if (count == countsByItem->second.end())
        return 0;
    return count->second;
}

const vector<uint32>& EverQuestMod::GetPreloadedLootIDsForCreatureGUID(Map* map, ObjectGuid creatureGUID)
{
    static const vector<uint32> returnEmpty;
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto mapIt = PreloadedLootItemIDsByMapInstanceKeyThenCreatureGUID.find(GetMapInstanceKey(map));
    if (mapIt == PreloadedLootItemIDsByMapInstanceKeyThenCreatureGUID.end())
        return returnEmpty;
    auto preloadedIt = mapIt->second.find(creatureGUID);
    if (preloadedIt != mapIt->second.end())
        return preloadedIt->second;
    return returnEmpty;
}

void EverQuestMod::ClearPreloadedLootIDsForCreatureGUID(Map* map, ObjectGuid creatureGUID)
{
    uint64 mapInstanceKey = GetMapInstanceKey(map);
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto preloadedMapIt = PreloadedLootItemIDsByMapInstanceKeyThenCreatureGUID.find(mapInstanceKey);
    if (preloadedMapIt != PreloadedLootItemIDsByMapInstanceKeyThenCreatureGUID.end())
    {
        preloadedMapIt->second.erase(creatureGUID);
        if (preloadedMapIt->second.empty())
            PreloadedLootItemIDsByMapInstanceKeyThenCreatureGUID.erase(preloadedMapIt);
    }
    auto countsMapIt = PreloadedLootCountsByMapInstanceKeyThenCreatureGUID.find(mapInstanceKey);
    if (countsMapIt != PreloadedLootCountsByMapInstanceKeyThenCreatureGUID.end())
    {
        countsMapIt->second.erase(creatureGUID);
        if (countsMapIt->second.empty())
            PreloadedLootCountsByMapInstanceKeyThenCreatureGUID.erase(countsMapIt);
    }
}

void EverQuestMod::TrackVisualEquippedItemsForCreatureGUID(Map* map, ObjectGuid creatureGUID, uint32 mainhandItemID, uint32 offhandItemID, bool isDualWielding)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    EverQuestLoadedCreatureEquippedVisualItems& visualItems = VisualEquippedItemsByMapInstanceKeyThenCreatureGUID[GetMapInstanceKey(map)][creatureGUID];
    visualItems.MainhandItemID = mainhandItemID;
    visualItems.OffhandItemID = offhandItemID;
    visualItems.IsDualWielding = isDualWielding;
}

bool EverQuestMod::IsCreatureDualWielding(Map* map, ObjectGuid creatureGUID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto mapIt = VisualEquippedItemsByMapInstanceKeyThenCreatureGUID.find(GetMapInstanceKey(map));
    if (mapIt == VisualEquippedItemsByMapInstanceKeyThenCreatureGUID.end())
        return false;
    auto it = mapIt->second.find(creatureGUID);
    if (it == mapIt->second.end())
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

    Creature* creature = attacker->ToCreature();

    // Restrict to EverQuest zones
    uint32 mapID = creature->GetMap()->GetId();
    if (mapID < ConfigSystemMapDBCIDMin || mapID > ConfigSystemMapDBCIDMax)
        return;

    // Prevent injected swings from repeating
    ObjectGuid attackerGUID = attacker->GetGUID();
    uint64 mapInstanceKey = GetMapInstanceKey(creature->GetMap());
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto guardMapIt = CreaturesResolvingEQMeleeExtraAttacksByMapInstanceKey.find(mapInstanceKey);
        if (guardMapIt != CreaturesResolvingEQMeleeExtraAttacksByMapInstanceKey.end() && guardMapIt->second.count(attackerGUID) > 0)
            return;
    }

    uint32 level = creature->GetLevel();
    uint32 weaponSkill = GetEQNPCMeleeWeaponSkillForLevel(level);
    if (weaponSkill == 0)
        return;

    uint32 effectiveSkill = weaponSkill;
    if (level > 35)
        effectiveSkill += level;

    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        CreaturesResolvingEQMeleeExtraAttacksByMapInstanceKey[mapInstanceKey].insert(attackerGUID);
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
    if (victim->IsAlive() == true && IsCreatureDualWielding(creature->GetMap(), attackerGUID) == true)
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
        auto guardMapIt = CreaturesResolvingEQMeleeExtraAttacksByMapInstanceKey.find(mapInstanceKey);
        if (guardMapIt != CreaturesResolvingEQMeleeExtraAttacksByMapInstanceKey.end())
        {
            guardMapIt->second.erase(attackerGUID);
            if (guardMapIt->second.empty())
                CreaturesResolvingEQMeleeExtraAttacksByMapInstanceKey.erase(guardMapIt);
        }
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

void EverQuestMod::SetupCreatureSummon(Creature* creature)
{
    if (HasCreatureDataForCreatureTemplateID(creature->GetEntry()) == false)
        return;
    const EverQuestCreature& eqCreature = GetCreatureDataForCreatureTemplateID(creature->GetEntry());

    // Only raid bosses and raid mini bosses summon (for now)
    if (eqCreature.DifficultyType != EQ_CREATURE_DIFFICULTY_RAIDBOSS && eqCreature.DifficultyType != EQ_CREATURE_DIFFICULTY_RAIDMINIBOSS)
        return;

    // Reset runtime fields in case this creature object was recycled
    EverQuestCreatureSummonState* state = creature->CustomData.GetDefault<EverQuestCreatureSummonState>(EQ_CREATURE_CUSTOMDATA_SUMMON);
    state->CooldownRemainingMS = 0;
}

void EverQuestMod::RemoveCreatureSummonState(Creature* creature)
{
    creature->CustomData.Erase(EQ_CREATURE_CUSTOMDATA_SUMMON);
}

// Reference was TAKP's Mob::CheckHateSummon and Mob::HateSummon
void EverQuestMod::UpdateCreatureSummon(Creature* creature, uint32 diff)
{
    if (creature == nullptr)
        return;
    if (ConfigCombatSkillsRaidBossSummonEnabled == false)
        return;

    EverQuestCreatureSummonState* state = creature->CustomData.Get<EverQuestCreatureSummonState>(EQ_CREATURE_CUSTOMDATA_SUMMON);
    if (state == nullptr)
        return;

    // Tick down the reuse timer
    if (state->CooldownRemainingMS > 0)
    {
        if (state->CooldownRemainingMS > diff)
        {
            state->CooldownRemainingMS -= diff;
            return;
        }
        state->CooldownRemainingMS = 0;
    }

    if (creature->IsAlive() == false || creature->IsInCombat() == false)
        return;

    // A charmed or player-controlled boss should never summon its target
    if (creature->IsCharmed() == true || creature->IsControlledByPlayer() == true)
        return;

    // Boss has to have taken damage
    if (creature->GetHealthPct() > (float)ConfigCombatSkillsRaidBossSummonMaxHealthPct)
        return;

    Unit* victim = creature->GetVictim();
    if (victim == nullptr || victim->IsAlive() == false)
        return;
    if (creature->IsValidAttackTarget(victim) == false)
        return;

    // Summoning only happens once the target has gotten out of melee reach
    if (creature->IsWithinMeleeRange(victim) == true)
        return;

    // Put the target right in front of the boss
    float destX = 0.0f;
    float destY = 0.0f;
    float destZ = 0.0f;
    creature->GetClosePoint(destX, destY, destZ, victim->GetCombatReach(), 0.0f, 0.0f, victim, true);

    creature->Say("You will not evade me, " + victim->GetName() + "!", LANG_UNIVERSAL, victim);
    victim->NearTeleportTo(destX, destY, destZ, victim->GetOrientation());

    // The teleport can despawn creatures (and erase state) through scripted side effects, so look the state up fresh instead of writing through the earlier pointer
    EverQuestCreatureSummonState* stateAfterSummon = creature->CustomData.Get<EverQuestCreatureSummonState>(EQ_CREATURE_CUSTOMDATA_SUMMON);
    if (stateAfterSummon != nullptr)
        stateAfterSummon->CooldownRemainingMS = ConfigCombatSkillsRaidBossSummonCooldownInMS;
}

void EverQuestMod::SetupCreatureCombatAbilities(Creature* creature)
{
    if (HasCreatureDataForCreatureTemplateID(creature->GetEntry()) == false)
        return;
    const EverQuestCreature& eqCreature = GetCreatureDataForCreatureTemplateID(creature->GetEntry());
    bool enrageEnabled = eqCreature.EnrageEnabled == true && ConfigCombatSkillsEnrageEnabled == true;
    bool flurryEnabled = eqCreature.FlurryEnabled == true && ConfigCombatSkillsFlurryEnabled == true;
    bool rampageEnabled = eqCreature.RampageEnabled == true && ConfigCombatSkillsRampageEnabled == true;
    bool wildRampageEnabled = eqCreature.WildRampageEnabled == true && ConfigCombatSkillsWildRampageEnabled == true;
    if (enrageEnabled == false && flurryEnabled == false && rampageEnabled == false && wildRampageEnabled == false)
        return;

    // Reset runtime fields in case this creature object was recycled
    EverQuestCreatureCombatAbilityState* state = creature->CustomData.GetDefault<EverQuestCreatureCombatAbilityState>(EQ_CREATURE_CUSTOMDATA_COMBATABILITY);
    state->EnrageEnabled = enrageEnabled;
    state->EnrageHPPct = eqCreature.EnrageHPPct;
    state->EnrageDurationInMS = eqCreature.EnrageDurationInMS;
    state->EnrageCooldownInMS = eqCreature.EnrageCooldownInMS;
    state->FlurryEnabled = flurryEnabled;
    state->FlurryChancePct = eqCreature.FlurryChancePct;
    state->RampageEnabled = rampageEnabled;
    state->RampageChancePct = eqCreature.RampageChancePct;
    state->RampageRange = (float)eqCreature.RampageRange;
    state->RampageDamagePct = eqCreature.RampageDamagePct;
    state->WildRampageEnabled = wildRampageEnabled;
    state->WildRampageChancePct = eqCreature.WildRampageChancePct;
    state->WildRampageMaxTargets = eqCreature.WildRampageMaxTargets;
    state->WildRampageDamagePct = eqCreature.WildRampageDamagePct;
    state->AttackRoundTimeInMS = eqCreature.AttackRoundTimeInMS;
    state->IsEnraged = false;
    state->EnrageDurationRemainingMS = 0;
    state->EnrageCooldownRemainingMS = 0;
    state->SpecialAttackTimerRemainingMS = 0;
    state->ActiveSwingDamageModPct = 100;
}

void EverQuestMod::RemoveCreatureCombatAbilityState(Creature* creature)
{
    creature->CustomData.Erase(EQ_CREATURE_CUSTOMDATA_COMBATABILITY);
}

void EverQuestMod::UpdateCreatureCombatAbilities(Creature* creature, uint32 diff)
{
    if (creature == nullptr)
        return;
    EverQuestCreatureCombatAbilityState* state = creature->CustomData.Get<EverQuestCreatureCombatAbilityState>(EQ_CREATURE_CUSTOMDATA_COMBATABILITY);
    if (state == nullptr)
        return;
    UpdateCreatureEnrage(creature, state, diff);
    UpdateCreatureSpecialAttacks(creature, state, diff);
}

// This was written referencing TAKP's Mob::CheckEnrage, Mob::StartEnrage, Mob:ProcessEnrage
void EverQuestMod::UpdateCreatureEnrage(Creature* creature, EverQuestCreatureCombatAbilityState* state, uint32 diff)
{
    if (state->EnrageEnabled == false)
        return;

    // Wind down an active enrage, and start the reuse cooldown when it ends
    if (state->IsEnraged == true)
    {
        if (state->EnrageDurationRemainingMS > diff)
        {
            state->EnrageDurationRemainingMS -= diff;
            return;
        }
        state->IsEnraged = false;
        state->EnrageDurationRemainingMS = 0;
        state->EnrageCooldownRemainingMS = state->EnrageCooldownInMS > 0 ? state->EnrageCooldownInMS : ConfigCombatSkillsEnrageDefaultCooldownInMS;
        if (creature->IsAlive() == true)
            creature->TextEmote(creature->GetName() + " is no longer enraged.", nullptr, true);
        return;
    }

    // Tick down the reuse cooldown
    if (state->EnrageCooldownRemainingMS > 0)
    {
        if (state->EnrageCooldownRemainingMS > diff)
        {
            state->EnrageCooldownRemainingMS -= diff;
            return;
        }
        state->EnrageCooldownRemainingMS = 0;
    }

    // Start an enrage when low enough on health.
    // Note: TAKP also blocks starting one while feared and not rooted
    if (creature->IsAlive() == false || creature->IsInCombat() == false)
        return;
    if (creature->HasUnitState(UNIT_STATE_FLEEING) == true && creature->HasUnitState(UNIT_STATE_ROOT) == false)
        return;
    uint32 hpPctTrigger = state->EnrageHPPct > 0 ? state->EnrageHPPct : ConfigCombatSkillsEnrageDefaultHPPct;
    if (creature->GetHealthPct() > (float)hpPctTrigger)
        return;
    state->IsEnraged = true;
    state->EnrageDurationRemainingMS = state->EnrageDurationInMS > 0 ? state->EnrageDurationInMS : ConfigCombatSkillsEnrageDefaultDurationInMS;
    creature->TextEmote(creature->GetName() + " has become ENRAGED.", nullptr, true);
}

// Written by referencing TAKP's Mod:AI_Process for special attacks
void EverQuestMod::UpdateCreatureSpecialAttacks(Creature* creature, EverQuestCreatureCombatAbilityState* state, uint32 diff)
{
    if (state->FlurryEnabled == false && state->RampageEnabled == false && state->WildRampageEnabled == false)
        return;

    // Roll at the creature's main-hand swing rate to mirror TAKP checking after each attack round
    if (state->SpecialAttackTimerRemainingMS > diff)
    {
        state->SpecialAttackTimerRemainingMS -= diff;
        return;
    }
    state->SpecialAttackTimerRemainingMS = 0;

    if (creature->IsAlive() == false || creature->IsInCombat() == false)
        return;
    if (creature->IsCharmed() == true)
        return;
    if (creature->HasUnitState(UNIT_STATE_CASTING) || creature->IsNonMeleeSpellCast(false))
        return;
    if (creature->HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_FLEEING | UNIT_STATE_CONFUSED))
        return;
    Unit* victim = creature->GetVictim();
    if (victim == nullptr || victim->IsAlive() == false)
        return;
    if (creature->IsWithinMeleeRange(victim) == false)
        return;

    // Rebalanced WoW swing time is typically faster than EQ attack delay, so control for that
    uint32 swingTime = state->AttackRoundTimeInMS > 0 ? state->AttackRoundTimeInMS : creature->GetAttackTime(BASE_ATTACK);
    if (swingTime < 1000)
        swingTime = 1000;
    state->SpecialAttackTimerRemainingMS = swingTime;

    if (state->FlurryEnabled == true)
    {
        uint32 chance = state->FlurryChancePct > 0 ? state->FlurryChancePct : ConfigCombatSkillsFlurryDefaultChancePct;
        if (urand(0, 99) < chance)
        {
            DoCreatureFlurry(creature, victim);
            return;
        }
    }
    if (state->RampageEnabled == true)
    {
        uint32 chance = state->RampageChancePct > 0 ? state->RampageChancePct : ConfigCombatSkillsRampageDefaultChancePct;
        if (urand(0, 99) < chance)
        {
            float range = state->RampageRange > 0.0f ? state->RampageRange : ConfigCombatSkillsRampageDefaultRange * ConfigWorldScale;
            uint32 damagePct = state->RampageDamagePct > 0 ? state->RampageDamagePct : 100;
            DoCreatureRampage(creature, victim, range, damagePct);
            return;
        }
    }
    if (state->WildRampageEnabled == true)
    {
        uint32 chance = state->WildRampageChancePct > 0 ? state->WildRampageChancePct : ConfigCombatSkillsWildRampageDefaultChancePct;
        if (urand(0, 99) < chance)
        {
            uint32 maxTargets = state->WildRampageMaxTargets > 0 ? state->WildRampageMaxTargets : ConfigCombatSkillsWildRampageDefaultMaxTargets;
            uint32 damagePct = state->WildRampageDamagePct > 0 ? state->WildRampageDamagePct : 100;
            DoCreatureWildRampage(creature, victim, maxTargets, damagePct);
        }
    }
}

// Similar to TAKP's "DoMainHandRound" + "DoOffHandRound". Intentionally not adding an explicit off-hand swing here
void EverQuestMod::DoCreatureCombatAbilitySwingRound(Creature* creature, Unit* target, uint32 damagePct)
{
    if (target == nullptr || target->IsAlive() == false)
        return;

    EverQuestCreatureCombatAbilityState* state = creature->CustomData.Get<EverQuestCreatureCombatAbilityState>(EQ_CREATURE_CUSTOMDATA_COMBATABILITY);
    if (state != nullptr)
        state->ActiveSwingDamageModPct = damagePct;

    creature->AttackerStateUpdate(target, BASE_ATTACK, true);

    // Make sure the custom data was not changed via scripting or whatever
    state = creature->CustomData.Get<EverQuestCreatureCombatAbilityState>(EQ_CREATURE_CUSTOMDATA_COMBATABILITY);
    if (state != nullptr)
        state->ActiveSwingDamageModPct = 100;
}

// Based on TAKP's Mob::Flurry of having one extra full attack round on the current target
void EverQuestMod::DoCreatureFlurry(Creature* creature, Unit* victim)
{
    creature->TextEmote(creature->GetName() + " executes a FLURRY of attacks on " + victim->GetName() + "!", victim);
    DoCreatureCombatAbilitySwingRound(creature, victim, 100);
}

// Based on TAKP's Mob::Rampage, but use the hate list instead of engage list
void EverQuestMod::DoCreatureRampage(Creature* creature, Unit* victim, float range, uint32 damagePct)
{
    // TAKP still emits the message even when nobody else ends up eligible to hit, so let's do that too
    creature->TextEmote(creature->GetName() + " goes on a RAMPAGE!", nullptr);

    // Take a snapshot of the hate list in case it changes mid execution
    vector<ObjectGuid> hatedUnitGUIDs;
    for (ThreatReference const* threatReference : creature->GetThreatMgr().GetSortedThreatList())
    {
        if (threatReference == nullptr || threatReference->IsAvailable() == false)
            continue;
        Unit* hatedUnit = threatReference->GetVictim();
        if (hatedUnit == nullptr)
            continue;
        hatedUnitGUIDs.push_back(hatedUnit->GetGUID());
    }

    for (ObjectGuid hatedUnitGUID : hatedUnitGUIDs)
    {
        if (victim != nullptr && hatedUnitGUID == victim->GetGUID())
            continue;
        Unit* rampageTarget = ObjectAccessor::GetUnit(*creature, hatedUnitGUID);
        if (rampageTarget == nullptr || rampageTarget->IsAlive() == false)
            continue;
        if (creature->GetExactDist(rampageTarget) > range)
            continue;

        // Regular rampage hits exactly one extra target
        DoCreatureCombatAbilitySwingRound(creature, rampageTarget, damagePct);
        return;
    }
}

// Based on TAKP's Mob::WildRampage + HateList::WildRampage.
void EverQuestMod::DoCreatureWildRampage(Creature* creature, Unit* victim, uint32 maxTargets, uint32 damagePct)
{
    creature->TextEmote(creature->GetName() + " goes on a WILD RAMPAGE!", nullptr);

    // Take a snapshot of the hate list in case it changes mid execution
    vector<ObjectGuid> hatedUnitGUIDs;
    for (ThreatReference const* threatReference : creature->GetThreatMgr().GetSortedThreatList())
    {
        if (threatReference == nullptr || threatReference->IsAvailable() == false)
            continue;
        Unit* hatedUnit = threatReference->GetVictim();
        if (hatedUnit == nullptr)
            continue;
        hatedUnitGUIDs.push_back(hatedUnit->GetGUID());
    }

    bool includeCurrentVictim = (hatedUnitGUIDs.size() == 1);
    uint32 targetsHit = 0;
    for (ObjectGuid hatedUnitGUID : hatedUnitGUIDs)
    {
        if (targetsHit >= maxTargets)
            break;
        if (creature->IsAlive() == false)
            break;
        if (includeCurrentVictim == false && victim != nullptr && hatedUnitGUID == victim->GetGUID())
            continue;
        Unit* rampageTarget = ObjectAccessor::GetUnit(*creature, hatedUnitGUID);
        if (rampageTarget == nullptr || rampageTarget->IsAlive() == false)
            continue;
        if (creature->IsWithinMeleeRange(rampageTarget) == false)
            continue;
        DoCreatureCombatAbilitySwingRound(creature, rampageTarget, damagePct);
        targetsHit++;
    }
}

bool EverQuestMod::IsCreatureEnragedForRiposte(Unit const* unit, Unit const* attacker)
{
    if (unit == nullptr || attacker == nullptr)
        return false;
    if (unit->IsCreature() == false)
        return false;
    const EverQuestCreatureCombatAbilityState* state = unit->CustomData.Get<EverQuestCreatureCombatAbilityState>(EQ_CREATURE_CUSTOMDATA_COMBATABILITY);
    if (state == nullptr || state->IsEnraged == false)
        return false;
    if (unit->IsAlive() == false)
        return false;
    if (unit->ToCreature()->HasFlagsExtra(CREATURE_FLAG_EXTRA_NO_PARRY))
        return false;
    if (unit->HasInArc(M_PI, attacker) == false)
        return false;
    if (unit->IsNonMeleeSpellCast(false, false, true) == true || unit->HasUnitState(UNIT_STATE_CONTROLLED))
        return false;
    return true;
}

void EverQuestMod::TryDoCreatureEnrageRiposteCounter(Unit* victim, Unit* attacker)
{
    if (IsCreatureEnragedForRiposte(victim, attacker) == false)
        return;

    Creature* enragedCreature = victim->ToCreature();
    ObjectGuid enragedCreatureGUID = enragedCreature->GetGUID();
    uint64 mapInstanceKey = GetMapInstanceKey(enragedCreature->GetMap());
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        if (CreaturesResolvingEQMeleeExtraAttacksByMapInstanceKey[mapInstanceKey].count(enragedCreatureGUID) > 0)
            return;
        CreaturesResolvingEQMeleeExtraAttacksByMapInstanceKey[mapInstanceKey].insert(enragedCreatureGUID);
    }

    enragedCreature->AttackerStateUpdate(attacker, BASE_ATTACK, true);

    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto guardMapIt = CreaturesResolvingEQMeleeExtraAttacksByMapInstanceKey.find(mapInstanceKey);
        if (guardMapIt != CreaturesResolvingEQMeleeExtraAttacksByMapInstanceKey.end())
        {
            guardMapIt->second.erase(enragedCreatureGUID);
            if (guardMapIt->second.empty())
                CreaturesResolvingEQMeleeExtraAttacksByMapInstanceKey.erase(guardMapIt);
        }
    }
}

void EverQuestMod::ApplyCreatureCombatAbilityDamageMod(Unit* attacker, uint32& damage)
{
    if (attacker == nullptr || attacker->IsCreature() == false || damage == 0)
        return;
    EverQuestCreatureCombatAbilityState* state = attacker->CustomData.Get<EverQuestCreatureCombatAbilityState>(EQ_CREATURE_CUSTOMDATA_COMBATABILITY);
    if (state == nullptr || state->ActiveSwingDamageModPct == 100)
        return;
    damage = damage * state->ActiveSwingDamageModPct / 100;
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

    // This forces creatures to continue chasing if they are doing so legitimately
    creature->UpdateLeashExtensionTime();

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

// This is a re-implementation of AzerothCore's CreatureLeashRadius since worldselver has to have "CreatureLeashRadius = 0" in order for bosses not to tether back too quickly in EQ zones
void EverQuestMod::UpdateNonEQCreatureLeash(Creature* creature)
{
    if (creature == nullptr)
        return;
    if (ConfigEvadeNonEQMapLeashRadius <= 0.0f)
        return;

    uint32 mapID = creature->GetMap()->GetId();
    if (mapID >= ConfigSystemMapDBCIDMin && mapID <= ConfigSystemMapDBCIDMax)
        return;

    // Core skips all leash logic on instanced maps
    if (creature->GetMap()->IsDungeon() == true)
        return;

    if (creature->IsAlive() == false || creature->IsInCombat() == false || creature->IsInEvadeMode() == true)
        return;
    if (creature->AI() == nullptr || creature->GetVictim() == nullptr)
        return;

    // Owned/charmed units take the owner-distance branch in the core
    if (creature->GetCharmerOrOwnerGUID().IsEmpty() == false)
        return;

    if (creature->isWorldBoss() == false)
    {
        if (creature->GetLastLeashExtensionTime() + creature->GetLeashTimer() > GameTime::GetGameTime().count())
            return;
        if (creature->HasTauntAura() == true)
            return;
    }

    float x, y, z;
    x = y = z = 0.0f;
    bool insideLeashRadius = false;
    MovementGenerator* idleSlot = creature->GetMotionMaster()->GetMotionSlot(MOTION_SLOT_IDLE);
    if (idleSlot != nullptr && idleSlot->GetResetPosition(x, y, z))
        insideLeashRadius = creature->IsInDist2d(x, y, ConfigEvadeNonEQMapLeashRadius);
    else
        insideLeashRadius = creature->IsInDist2d(&creature->GetHomePosition(), ConfigEvadeNonEQMapLeashRadius);

    if (insideLeashRadius == false)
        creature->AI()->EnterEvadeMode(CreatureAI::EVADE_REASON_BOUNDARY);
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

bool EverQuestMod::IsSocialAggroOverrideNeededForCreature(Creature* creature, float& scaleOut, float& maxAgroZDistanceOut)
{
    scaleOut = 1.0f;
    maxAgroZDistanceOut = -1.0f;
    if (creature == nullptr)
        return false;

    bool hasCustomScale = TryGetCustomSocialAggroScale(creature, scaleOut);
    if (hasCustomScale == false)
        scaleOut = 1.0f;

    uint32 mapID = creature->GetMapId();
    if (mapID >= ConfigSystemMapDBCIDMin && mapID <= ConfigSystemMapDBCIDMax)
        maxAgroZDistanceOut = GetMaxAgroZDistanceForMap(mapID);

    return hasCustomScale == true || maxAgroZDistanceOut >= 0.0f;
}

// Kinda-sorta a mirror of Creature:CallAssistance, but need to override to make custom social behavior
void EverQuestMod::DoScaledSocialAggroSearch(Creature* caller, Unit* victim, float scale, float maxAgroZDistance)
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
    std::vector<ObjectGuid> assistantGUIDs;
    assistantGUIDs.reserve(assistList.size());
    for (Creature* assistant : assistList)
    {
        if (assistant == nullptr)
            continue;
        if (IsBlockedByAgroZDistance(assistant, caller, maxAgroZDistance) == true)
            continue;

        if (IsBlockedByAgroZDistance(assistant, victim, maxAgroZDistance) == true)
            continue;
        assistantGUIDs.push_back(assistant->GetGUID());
    }

    for (ObjectGuid assistantGUID : assistantGUIDs)
    {
        Creature* assistant = ObjectAccessor::GetCreature(*caller, assistantGUID);
        if (assistant == nullptr || assistant->IsAlive() == false)
            continue;

        // Suppress immediate call, link leash timers
        assistant->SetNoCallAssistance(true);
        assistant->EngageWithTarget(victim);
        if (assistant->IsEngaged() == true)
            assistant->SetLastLeashExtensionTimePtr(caller->GetLastLeashExtensionTimePtr());
    }
}

void EverQuestMod::ApplyScaledCreatureSocialAggroOnEngage(Creature* creature, Unit* victim)
{
    // Only apply if a creature has a custom setting, or the zone restricts vertical agro
    float scale = 1.0f;
    float maxAgroZDistance = -1.0f;
    if (IsSocialAggroOverrideNeededForCreature(creature, scale, maxAgroZDistance) == false)
        return;

    creature->SetNoCallAssistance(true);
    DoScaledSocialAggroSearch(creature, victim, scale, maxAgroZDistance);
}

void EverQuestMod::RemoveCreatureSocialAggroState(Creature* creature)
{
    creature->CustomData.Erase(EQ_CREATURE_CUSTOMDATA_SOCIALAGGRO);
}

void EverQuestMod::MarkCreatureAgroZBlockOnEngage(Creature* creature, Unit* victim)
{
    if (creature == nullptr || victim == nullptr || creature == victim)
        return;

    uint32 mapID = creature->GetMapId();
    if (mapID < ConfigSystemMapDBCIDMin || mapID > ConfigSystemMapDBCIDMax)
        return;

    // Must match the gate on UpdateCreatureAgroZBlock's caller, otherwise a pending block could be recorded and never consumed
    uint32 entryID = creature->GetEntry();
    if (entryID < ConfigSystemCreatureTemplateIDMin || entryID > ConfigSystemCreatureTemplateIDMax)
        return;

    float maxAgroZDistance = GetMaxAgroZDistanceForMap(mapID);
    if (maxAgroZDistance < 0.0f)
        return;

    // Player owned creatures keep stock engine behavior
    if (creature->IsPet() == true || creature->IsControlledByPlayer() == true || creature->IsCharmed() == true)
        return;
    if (IsBlockedByAgroZDistance(creature, victim, maxAgroZDistance) == false)
        return;

    // Only unprovoked agro is blocked.  Anything that is actually fighting this creature still gets a response, so the
    // rule can never leave a creature unkillable from above or below
    if (victim->GetVictim() == creature)
        return;
    if (creature->GetThreatMgr().GetThreat(victim) > 0.0f)
        return;

    EverQuestCreatureAgroZBlockState* state = creature->CustomData.GetDefault<EverQuestCreatureAgroZBlockState>(EQ_CREATURE_CUSTOMDATA_AGROZBLOCK);
    state->BlockedVictimGUID = victim->GetGUID();
    state->DropPending = true;
}

void EverQuestMod::UpdateCreatureAgroZBlock(Creature* creature, uint32 diff)
{
    if (creature == nullptr)
        return;

    // Only creatures that actually tripped the rule ever carry this state, so the normal path costs one lookup
    EverQuestCreatureAgroZBlockState* state = creature->CustomData.Get<EverQuestCreatureAgroZBlockState>(EQ_CREATURE_CUSTOMDATA_AGROZBLOCK);
    if (state == nullptr)
        return;

    // Release the post-block re-agro suppression when it runs out
    if (state->SuppressRemainingMS > 0)
    {
        if (state->SuppressRemainingMS <= diff)
        {
            state->SuppressRemainingMS = 0;
            if (state->RestoreAggressiveReactState == true)
            {
                creature->SetReactState(REACT_AGGRESSIVE);
                state->RestoreAggressiveReactState = false;
            }
        }
        else
            state->SuppressRemainingMS -= diff;
    }

    if (state->DropPending == false)
        return;
    state->DropPending = false;

    if (creature->IsAlive() == false || creature->IsInCombat() == false)
        return;
    if (creature->IsAIEnabled == false || creature->AI() == nullptr)
        return;

    // Re-verify from scratch, since a tick has passed since the engage and the situation may have changed.
    // Resolving the GUID rather than holding a pointer keeps a despawned or deleted victim from being touched
    Unit* victim = ObjectAccessor::GetUnit(*creature, state->BlockedVictimGUID);
    if (victim == nullptr || victim->IsAlive() == false)
        return;
    if (IsBlockedByAgroZDistance(creature, victim, GetMaxAgroZDistanceForMap(creature->GetMapId())) == false)
        return;
    if (victim->GetVictim() == creature)
        return;
    if (creature->GetThreatMgr().GetThreat(victim) > 0.0f)
        return;

    // Every write to the state happens before the combat drop below.  Dropping combat can despawn a temporary summon,
    // which runs OnCreatureRemoveWorld and erases this creature's CustomData, so 'state' must not be touched afterward.
    // Holding the creature defensive briefly keeps the same out-of-range unit from re-triggering proximity agro on the
    // very next relocation tick.  Defensive still fights back when attacked, so this never makes a creature passive to a real attack
    if (creature->HasReactState(REACT_AGGRESSIVE) == true)
    {
        state->RestoreAggressiveReactState = true;
        creature->SetReactState(REACT_DEFENSIVE);
    }
    state->SuppressRemainingMS = EQ_AGRO_Z_BLOCK_SUPPRESS_MS;
    state = nullptr;

    creature->GetThreatMgr().ClearThreat(victim);

    // Anything else still on the threat list keeps the fight going, otherwise the creature heads home
    if (creature->GetThreatMgr().IsThreatListEmpty(true) == true)
        creature->AI()->EnterEvadeMode(CreatureAI::EVADE_REASON_OTHER);
}

void EverQuestMod::RemoveCreatureAgroZBlockState(Creature* creature)
{
    creature->CustomData.Erase(EQ_CREATURE_CUSTOMDATA_AGROZBLOCK);
}

bool EverQuestMod::ShouldBlockCreatureInitialAgroOnPet(Unit const* unit, Unit const* target)
{
    if (ConfigPetDisableInitialCreatureAgro == false)
        return false;
    if (unit == nullptr || target == nullptr || unit == target)
        return false;

    // Only player owned creatures are protected (pets, guardians, charmed creatures), never players themselves
    if (target->IsControlledByPlayer() == false || target->IsCreature() == false)
        return false;

    // Only creatures that aren't themselves player owned ever have their reaction changed
    if (unit->IsCreature() == false || unit->IsControlledByPlayer() == true)
        return false;

    uint32 mapID = unit->GetMapId();
    if (mapID < ConfigSystemMapDBCIDMin || mapID > ConfigSystemMapDBCIDMax)
        return false;

    // Fighting pets should still be allowed
    if (unit->IsEngagedBy(target) == true)
        return false;
    if (unit->GetVictim() == target)
        return false;
    if (target->getAttackerForHelper() != nullptr)
        return false;

    return true;
}

// WoW doesn't have neutral creatures hit by AoE fight back, but EQ should
void EverQuestMod::ProcessCreatureRetaliationOnDamage(Unit* attacker, Unit* victim)
{
    if (attacker == nullptr || victim == nullptr || attacker == victim)
        return;
    Creature* creature = victim->ToCreature();
    if (creature == nullptr)
        return;
    uint32 mapID = creature->GetMapId();
    if (mapID < ConfigSystemMapDBCIDMin || mapID > ConfigSystemMapDBCIDMax)
        return;
    if (creature->HasUnitFlag(UNIT_FLAG_PLAYER_CONTROLLED) == true)
        return;
    if (attacker->HasUnitFlag(UNIT_FLAG_PLAYER_CONTROLLED) == false)
        return;
    if (creature->IsAlive() == false || creature->IsAIEnabled == false || creature->GetVictim() != nullptr)
        return;
    if (creature->IsCharmed() == true || creature->IsInEvadeMode() == true)
        return;
    if (creature->HasReactState(REACT_PASSIVE) == true)
        return;
    if (creature->CanCreatureAttack(attacker) == false)
        return;
    creature->AI()->AttackStart(attacker);
}

void EverQuestMod::RemoveCreatureCrowdControlAurasFromPlayersOnDeath(Creature* deadCreature)
{
    if (deadCreature == nullptr)
        return;

    // Pets and charmed creatures cast under player control, so their crowd control follows the player rules instead
    if (deadCreature->IsPet() == true || deadCreature->IsControlledByPlayer() == true)
        return;

    // Charm and possess are left out since the core already breaks those through RemoveAllControlled on the death state change
    static const AuraType crowdControlAuraTypes[] =
    {
        SPELL_AURA_MOD_ROOT,
        SPELL_AURA_MOD_STUN,
        SPELL_AURA_MOD_FEAR,
        SPELL_AURA_MOD_CONFUSE,
        SPELL_AURA_MOD_PACIFY,
        SPELL_AURA_MOD_SILENCE,
        SPELL_AURA_MOD_PACIFY_SILENCE,
        SPELL_AURA_MOD_DECREASE_SPEED
    };

    ObjectGuid deadCreatureGUID = deadCreature->GetGUID();
    Map::PlayerList const& mapPlayers = deadCreature->GetMap()->GetPlayers();
    for (Map::PlayerList::const_iterator playerIter = mapPlayers.begin(); playerIter != mapPlayers.end(); ++playerIter)
    {
        Player* mapPlayer = playerIter->GetSource();
        if (mapPlayer == nullptr || mapPlayer->IsInWorld() == false)
            continue;
        for (AuraType curAuraType : crowdControlAuraTypes)
        {
            if (mapPlayer->HasAuraTypeWithCaster(curAuraType, deadCreatureGUID) == false)
                continue;
            mapPlayer->RemoveAurasByType(curAuraType, deadCreatureGUID);
        }
    }
}

void EverQuestMod::UpdateCreatureScaledSocialAggro(Creature* creature, uint32 diff)
{
    if (creature == nullptr)
        return;

    float scale = 1.0f;
    float maxAgroZDistance = -1.0f;
    bool eligible = IsSocialAggroOverrideNeededForCreature(creature, scale, maxAgroZDistance) == true && creature->IsAlive() == true && creature->IsInCombat() == true && creature->IsPet() == false && creature->IsControlledByPlayer() == false;
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
        DoScaledSocialAggroSearch(creature, victim, scale, maxAgroZDistance);
        state->RecallTimerMS = periodMS;
    }
    else
        state->RecallTimerMS -= diff;
}

void EverQuestMod::StoreCreatureAggroPosition(Creature* creature)
{
    EverQuestCreatureAggroPositionState* state = creature->CustomData.GetDefault<EverQuestCreatureAggroPositionState>(EQ_CREATURE_CUSTOMDATA_AGGROPOSITION);
    state->X = creature->GetPositionX();
    state->Y = creature->GetPositionY();
    state->Z = creature->GetPositionZ();
    state->Orientation = creature->GetOrientation();
    state->HasPosition = true;
}

void EverQuestMod::RemoveCreatureAggroPositionState(Creature* creature)
{
    creature->CustomData.Erase(EQ_CREATURE_CUSTOMDATA_AGGROPOSITION);
}

void EverQuestMod::TeleportCreatureToLastAggroPosition(Creature* creature, uint32 gateSpellID)
{
    if (creature == nullptr)
        return;
    if (creature->IsAlive() == false)
        return;
    if (creature->IsPet() == true || creature->IsControlledByPlayer() == true)
        return;

    EverQuestCreatureAggroPositionState* state = creature->CustomData.Get<EverQuestCreatureAggroPositionState>(EQ_CREATURE_CUSTOMDATA_AGGROPOSITION);
    if (state == nullptr || state->HasPosition == false)
        return;

    // The gate tether aura only has meaning for players
    creature->RemoveAurasDueToSpell(gateSpellID);
    creature->NearTeleportTo(state->X, state->Y, state->Z, state->Orientation);
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

bool EverQuestMod::ShouldSuppressBashKickStunOnDefender(uint32 spellID, Unit* defender)
{
    if (defender == nullptr)
        return false;
    if (ConfigCombatSkillsDisableBashKickStunOnPlayers == false)
        return false;
    if (defender->IsPlayer() == false)
        return false;
    if (spellID < ConfigSystemSpellDBCIDMin || spellID > ConfigSystemSpellDBCIDMax)
        return false;
    if (IsSpellAnEQSpell(spellID) == false)
        return false;
    return GetSpellDataForSpellID(spellID).StunUsesBashKickChance;
}

bool EverQuestMod::ShouldStripBashKickStunBeforeItLands(uint32 spellID, Unit* defender)
{
    if (ConfigCombatSkillsDisabledBashKickStunInterruptsPlayerCast == true)
        return false;
    return ShouldSuppressBashKickStunOnDefender(spellID, defender);
}

void EverQuestMod::ClearVisualEquippedItemsForCreatureGUID(Map* map, ObjectGuid creatureGUID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto mapIt = VisualEquippedItemsByMapInstanceKeyThenCreatureGUID.find(GetMapInstanceKey(map));
    if (mapIt == VisualEquippedItemsByMapInstanceKeyThenCreatureGUID.end())
        return;
    mapIt->second.erase(creatureGUID);
    if (mapIt->second.empty())
        VisualEquippedItemsByMapInstanceKeyThenCreatureGUID.erase(mapIt);
}

void EverQuestMod::RemoveVisualEquippedItemForCreatureGUIDIfExists(Map* map, ObjectGuid creatureGUID, uint32 itemTemplateID)
{
    EverQuestLoadedCreatureEquippedVisualItems* visualItems = nullptr;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto mapIt = VisualEquippedItemsByMapInstanceKeyThenCreatureGUID.find(GetMapInstanceKey(map));
        if (mapIt == VisualEquippedItemsByMapInstanceKeyThenCreatureGUID.end())
            return;
        auto visualItemsIt = mapIt->second.find(creatureGUID);
        if (visualItemsIt == mapIt->second.end())
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
    ShipWaitNodesByGameObjectTemplateEntryID.clear();

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

static thread_local bool IsResolvingGraveyardDomain = false;

class EverQuestGraveyardDomainProbeGuard
{
public:
    EverQuestGraveyardDomainProbeGuard() { IsResolvingGraveyardDomain = true; }
    ~EverQuestGraveyardDomainProbeGuard() { IsResolvingGraveyardDomain = false; }
};

uint32 EverQuestMod::GetNearestEverQuestGraveyardIDForPosition(uint32 mapID, float x, float y, float z)
{
    if (IsMapIDAnEverQuestMap(mapID) == false)
        return 0;

    uint32 nearestGraveyardID = 0;
    float nearestDistanceSquared = 0.0f;
    for (auto const& graveyardIter : sGraveyard->GetGraveyardData())
    {
        GraveyardStruct const& graveyard = graveyardIter.second;
        if (graveyard.Map != mapID)
            continue;

        float deltaX = graveyard.x - x;
        float deltaY = graveyard.y - y;
        float deltaZ = graveyard.z - z;
        float distanceSquared = (deltaX * deltaX) + (deltaY * deltaY) + (deltaZ * deltaZ);

        // The core's store is unordered, so an exact tie is settled by the lower ID to keep the same death resolving the same way
        bool isCloser = (distanceSquared < nearestDistanceSquared) || (distanceSquared == nearestDistanceSquared && graveyard.ID < nearestGraveyardID);
        if (nearestGraveyardID == 0 || isCloser == true)
        {
            nearestGraveyardID = graveyard.ID;
            nearestDistanceSquared = distanceSquared;
        }
    }
    return nearestGraveyardID;
}

uint32 EverQuestMod::GetFallbackEverQuestGraveyardID()
{
    // The configured catch-all is used only when it really is a graveyard inside an EverQuest zone, since sending a corpse to it is the last thing tried before giving up
    GraveyardStruct const* configuredGraveyard = sGraveyard->GetGraveyard(ConfigDeathFallbackGraveyardID);
    if (configuredGraveyard != nullptr && IsMapIDAnEverQuestMap(configuredGraveyard->Map) == true)
        return ConfigDeathFallbackGraveyardID;

    // Otherwise the lowest EverQuest graveyard ID stands in for it, which stays the same across server starts
    uint32 lowestGraveyardID = 0;
    for (auto const& graveyardIter : sGraveyard->GetGraveyardData())
    {
        GraveyardStruct const& graveyard = graveyardIter.second;
        if (IsMapIDAnEverQuestMap(graveyard.Map) == false)
            continue;
        if (lowestGraveyardID == 0 || graveyard.ID < lowestGraveyardID)
            lowestGraveyardID = graveyard.ID;
    }
    return lowestGraveyardID;
}

void EverQuestMod::ValidateGraveyardDomainConfiguration()
{
    if (ConfigDeathEnforceGraveyardDomain == false)
    {
        LOG_INFO("module.EverQuest", "EverQuestMod has EverQuest.Death.EnforceGraveyardDomain turned off, so a death in an EverQuest zone can be sent to a WoW graveyard");
        return;
    }

    uint32 everQuestGraveyardCount = 0;
    for (auto const& graveyardIter : sGraveyard->GetGraveyardData())
    {
        if (IsMapIDAnEverQuestMap(graveyardIter.second.Map) == true)
            everQuestGraveyardCount++;
    }

    if (everQuestGraveyardCount == 0)
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod found no graveyards on maps {} - {}, so a death in an EverQuest zone cannot be held out of a WoW graveyard. Check that the converter's game_graveyard rows were deployed", ConfigSystemMapDBCIDMin, ConfigSystemMapDBCIDMax);
        return;
    }

    GraveyardStruct const* configuredGraveyard = sGraveyard->GetGraveyard(ConfigDeathFallbackGraveyardID);
    if (configuredGraveyard == nullptr || IsMapIDAnEverQuestMap(configuredGraveyard->Map) == false)
        LOG_ERROR("module.EverQuest", "EverQuestMod has EverQuest.Death.FallbackGraveyardID set to {}, which is not a graveyard inside an EverQuest zone. Graveyard {} is used in its place", ConfigDeathFallbackGraveyardID, GetFallbackEverQuestGraveyardID());
    else
        LOG_INFO("module.EverQuest", "EverQuestMod is holding EverQuest deaths to the {} graveyards in Norrath, with graveyard {} as the fallback", everQuestGraveyardCount, ConfigDeathFallbackGraveyardID);
}

void EverQuestMod::EnforceGraveyardDomainForDeath(Player* player, TeamId teamId, bool nearCorpse, uint32& graveyardOverride)
{
    if (ConfigDeathEnforceGraveyardDomain == false || player == nullptr)
        return;
    if (IsResolvingGraveyardDomain == true)
        return;

    // Where the death is anchored.  A corpse left behind on a different map is not a place to search from, so the player's own position stands in for it there, which is also what keeps a ghost that walked into another zone out of the corpse zone's graveyard
    WorldLocation deathLocation = player->GetWorldLocation();
    bool useCorpseLocation = false;
    if (nearCorpse == true && player->HasCorpse() == true)
    {
        WorldLocation corpseLocation = player->GetCorpseLocation();
        if (corpseLocation.GetMapId() == deathLocation.GetMapId())
        {
            deathLocation = corpseLocation;
            useCorpseLocation = true;
        }
    }

    // What the core lands on by itself, which is left alone unless it crossed worlds.  The returned pointer lives in the core's graveyard store, so the ID comes off it right away instead of being held on to
    uint32 coreGraveyardID = 0;
    uint32 coreGraveyardMapID = 0;
    bool hasCoreGraveyard = false;
    {
        EverQuestGraveyardDomainProbeGuard probeGuard;
        GraveyardStruct const* coreGraveyard = sGraveyard->GetClosestGraveyard(player, teamId, useCorpseLocation);
        if (coreGraveyard != nullptr)
        {
            coreGraveyardID = coreGraveyard->ID;
            coreGraveyardMapID = coreGraveyard->Map;
            hasCoreGraveyard = true;
        }
    }

    bool isEverQuestDeath = IsMapIDAnEverQuestMap(deathLocation.GetMapId());
    if (hasCoreGraveyard == true && IsMapIDAnEverQuestMap(coreGraveyardMapID) == isEverQuestDeath)
    {
        graveyardOverride = coreGraveyardID;
        return;
    }

    uint32 replacementGraveyardID = 0;
    if (isEverQuestDeath == true)
    {
        // The zone died in comes first, then the open world copy of it for a death inside an instance, and the catch-all last
        replacementGraveyardID = GetNearestEverQuestGraveyardIDForPosition(deathLocation.GetMapId(), deathLocation.GetPositionX(), deathLocation.GetPositionY(), deathLocation.GetPositionZ());
        if (replacementGraveyardID == 0)
        {
            uint32 openWorldMapID = GetOpenWorldMapIDForMapID(deathLocation.GetMapId());
            if (openWorldMapID != deathLocation.GetMapId())
                replacementGraveyardID = GetNearestEverQuestGraveyardIDForPosition(openWorldMapID, deathLocation.GetPositionX(), deathLocation.GetPositionY(), deathLocation.GetPositionZ());
        }
        if (replacementGraveyardID == 0)
            replacementGraveyardID = GetFallbackEverQuestGraveyardID();
    }
    else
    {
        // A WoW death that reached an EverQuest graveyard has broken zone links of its own, so the core's own faction default stands in
        GraveyardStruct const* defaultGraveyard = sGraveyard->GetDefaultGraveyard(teamId);
        if (defaultGraveyard != nullptr && IsMapIDAnEverQuestMap(defaultGraveyard->Map) == false)
            replacementGraveyardID = defaultGraveyard->ID;
    }

    if (replacementGraveyardID == 0)
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod could not hold the death of player {} on map {} (area {}) to a graveyard in {}, as no replacement graveyard was found, so graveyard {} is used",
            player->GetName(), deathLocation.GetMapId(), player->GetAreaId(), isEverQuestDeath == true ? "Norrath" : "Azeroth", coreGraveyardID);
        return;
    }

    LOG_WARN("module.EverQuest", "EverQuestMod moved the death of player {} on map {} (area {}) from graveyard {} to graveyard {}, since the first one was not in {}. Check the graveyard_zone rows for that area",
        player->GetName(), deathLocation.GetMapId(), player->GetAreaId(), coreGraveyardID, replacementGraveyardID, isEverQuestDeath == true ? "Norrath" : "Azeroth");
    graveyardOverride = replacementGraveyardID;
}

void EverQuestMod::LoadZoneData()
{
    ZoneByMapID.clear();
    InstanceRaidLowMapIDs.clear();
    InstanceDungeonMapIDs.clear();
    OpenWorldMapIDByInstanceMapID.clear();

    QueryResult queryResult = WorldDatabase.Query("SELECT MapID, AllowBind, ExpansionID, MaxAgroZDistance, InstanceRaidLowMapID, InstanceDungeonMapID, RequiredKeyItemID FROM mod_everquest_zone;");
    if (queryResult)
    {
        do
        {
            Field* fields = queryResult->Fetch();
            EverQuestZone zone;
            zone.MapID = fields[0].Get<uint32>();
            zone.AllowBind = fields[1].Get<uint8>() != 0;
            zone.ExpansionID = fields[2].Get<int32>();
            zone.MaxAgroZDistance = fields[3].Get<float>();
            zone.InstanceRaidLowMapID = fields[4].Get<uint32>();
            zone.InstanceDungeonMapID = fields[5].Get<uint32>();
            zone.RequiredKeyItemID = fields[6].Get<uint32>();
            ZoneByMapID[zone.MapID] = zone;
            if (zone.InstanceRaidLowMapID != 0)
            {
                InstanceRaidLowMapIDs.insert(zone.InstanceRaidLowMapID);
                OpenWorldMapIDByInstanceMapID[zone.InstanceRaidLowMapID] = zone.MapID;
            }
            if (zone.InstanceDungeonMapID != 0)
            {
                InstanceDungeonMapIDs.insert(zone.InstanceDungeonMapID);
                OpenWorldMapIDByInstanceMapID[zone.InstanceDungeonMapID] = zone.MapID;
            }
        } while (queryResult->NextRow());
    }
}

void EverQuestMod::LoadZoneTeleportDestinationData()
{
    ZoneTeleportDestinationsByMapID.clear();

    // 62 is SMART_ACTION_TELEPORT, which carries its destination map in the first action parameter and its landing spot in the target columns
    QueryResult queryResult = WorldDatabase.Query("SELECT action_param1, target_x, target_y, target_z FROM smart_scripts WHERE action_type = 62;");
    if (queryResult)
    {
        do
        {
            Field* fields = queryResult->Fetch();
            uint32 destinationMapID = fields[0].Get<uint32>();

            // Only a zone that has a private copy can ever be rerouted into one, so the rest are not worth holding on to
            if (GetInstanceDungeonMapIDForMap(destinationMapID) == 0 && GetInstanceRaidLowMapIDForMap(destinationMapID) == 0)
                continue;

            EverQuestZoneTeleportDestination zoneTeleportDestination;
            zoneTeleportDestination.X = fields[1].Get<float>();
            zoneTeleportDestination.Y = fields[2].Get<float>();
            zoneTeleportDestination.Z = fields[3].Get<float>();
            ZoneTeleportDestinationsByMapID[destinationMapID].push_back(zoneTeleportDestination);
        } while (queryResult->NextRow());
    }
}

bool EverQuestMod::IsZoneTeleportDestination(uint32 mapID, float x, float y, float z)
{
    auto zoneTeleportDestinationsIter = ZoneTeleportDestinationsByMapID.find(mapID);
    if (zoneTeleportDestinationsIter == ZoneTeleportDestinationsByMapID.end())
        return false;

    // The position comes back out of the same rows it went in through
    const float positionTolerance = 0.1f;
    for (const EverQuestZoneTeleportDestination& zoneTeleportDestination : zoneTeleportDestinationsIter->second)
    {
        if (std::fabs(zoneTeleportDestination.X - x) > positionTolerance)
            continue;
        if (std::fabs(zoneTeleportDestination.Y - y) > positionTolerance)
            continue;
        if (std::fabs(zoneTeleportDestination.Z - z) > positionTolerance)
            continue;
        return true;
    }
    return false;
}

bool EverQuestMod::IsBindAllowedForMap(uint32 mapID)
{
    // Any zone missing from the zone data is considered bind-restricted
    auto zoneIt = ZoneByMapID.find(mapID);
    if (zoneIt == ZoneByMapID.end())
        return false;
    return zoneIt->second.AllowBind;
}

uint32 EverQuestMod::GetRequiredKeyItemIDForMap(uint32 mapID)
{
    auto zoneIt = ZoneByMapID.find(mapID);
    if (zoneIt == ZoneByMapID.end())
        return 0;
    return zoneIt->second.RequiredKeyItemID;
}

bool EverQuestMod::DoesPlayerHaveRequiredKeyForMap(Player* player, uint32 mapID)
{
    uint32 requiredKeyItemID = GetRequiredKeyItemIDForMap(mapID);
    if (requiredKeyItemID == 0)
        return true;
    if (player == nullptr)
        return false;
    if (player->IsGameMaster() == true)
        return true;

    // The key must be carried, so what sits in the bank is no help
    return player->HasItemCount(requiredKeyItemID, 1, false);
}

std::string EverQuestMod::GetRequiredKeyItemName(uint32 requiredKeyItemID)
{
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(requiredKeyItemID);
    if (itemTemplate == nullptr)
        return "a key you do not have";
    return itemTemplate->Name1;
}

std::string EverQuestMod::GetZoneNameForMap(uint32 mapID)
{
    // An instance copy's own map name carries a suffix, so the zone's plain name comes off the open world version
    MapEntry const* mapEntry = sMapStore.LookupEntry(GetOpenWorldMapIDForMapID(mapID));
    if (mapEntry == nullptr)
        return "that place";
    return mapEntry->name[sWorld->GetDefaultDbcLocale()];
}

float EverQuestMod::GetMaxAgroZDistanceForMap(uint32 mapID)
{
    auto zoneIt = ZoneByMapID.find(mapID);
    if (zoneIt == ZoneByMapID.end())
        return -1.0f;
    return zoneIt->second.MaxAgroZDistance;
}

uint32 EverQuestMod::GetInstanceRaidLowMapIDForMap(uint32 mapID)
{
    auto zoneIt = ZoneByMapID.find(mapID);
    if (zoneIt == ZoneByMapID.end())
        return 0;
    return zoneIt->second.InstanceRaidLowMapID;
}

bool EverQuestMod::IsMapInstanceRaidLow(uint32 mapID)
{
    return InstanceRaidLowMapIDs.find(mapID) != InstanceRaidLowMapIDs.end();
}

uint32 EverQuestMod::GetOpenWorldMapIDForMapID(uint32 mapID)
{
    // Any data rows keyed by a map ID (kill spawns, forage) are only generated for the open world copy so instanced copies resolve back to the open version
    auto openWorldMapIDIt = OpenWorldMapIDByInstanceMapID.find(mapID);
    if (openWorldMapIDIt == OpenWorldMapIDByInstanceMapID.end())
        return mapID;
    return openWorldMapIDIt->second;
}

uint32 EverQuestMod::GetInstanceDungeonMapIDForMap(uint32 mapID)
{
    auto zoneIt = ZoneByMapID.find(mapID);
    if (zoneIt == ZoneByMapID.end())
        return 0;
    return zoneIt->second.InstanceDungeonMapID;
}

bool EverQuestMod::IsMapInstanceDungeon(uint32 mapID)
{
    return InstanceDungeonMapIDs.find(mapID) != InstanceDungeonMapIDs.end();
}

bool EverQuestMod::IsCreatureBlockedFromInstanceMap(uint32 creatureTemplateID, Map* map)
{
    if (map == nullptr)
        return false;
    bool isDungeonInstanceMap = IsMapInstanceDungeon(map->GetId());
    bool isRaidInstanceMap = IsMapInstanceRaidLow(map->GetId());
    if (isDungeonInstanceMap == false && isRaidInstanceMap == false)
        return false;
    if (HasCreatureDataForCreatureTemplateID(creatureTemplateID) == false)
        return false;

    uint32 difficultyType = GetCreatureDataForCreatureTemplateID(creatureTemplateID).DifficultyType;
    bool isRaidCreature = (difficultyType == EQ_CREATURE_DIFFICULTY_RAIDTRASH || difficultyType == EQ_CREATURE_DIFFICULTY_RAIDBOSS || difficultyType == EQ_CREATURE_DIFFICULTY_RAIDMINIBOSS);
    if (isRaidInstanceMap == true)
        return isRaidCreature == false;
    return isRaidCreature;
}

void EverQuestMod::UpdateRaidLowInstanceStateForPlayer(Player* player)
{
    if (player == nullptr)
        return;

    uint32 mapID = player->GetMapId();
    uint32 instanceID = player->GetInstanceId();
    bool isInsideRaidLowInstance = (instanceID != 0 && IsMapInstanceRaidLow(mapID) == true);

    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    if (isInsideRaidLowInstance == true)
    {
        EverQuestPlayerRaidLowInstanceState& raidLowInstanceState = RaidLowInstanceStateByPlayerGUID[player->GetGUID()];
        raidLowInstanceState.RaidLowMapID = mapID;
        raidLowInstanceState.InstanceID = instanceID;
        raidLowInstanceState.IsInside = true;
        return;
    }

    // Which instance was left is remembered, since that's what a zone line back into the zone needs to route to
    auto raidLowInstanceStateIt = RaidLowInstanceStateByPlayerGUID.find(player->GetGUID());
    if (raidLowInstanceStateIt != RaidLowInstanceStateByPlayerGUID.end())
        raidLowInstanceStateIt->second.IsInside = false;
}

void EverQuestMod::ClearRaidLowInstanceStateForPlayer(ObjectGuid playerGUID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    RaidLowInstanceStateByPlayerGUID.erase(playerGUID);
}

// True when the player's own last raid instance for this map still has somebody else standing in it
bool EverQuestMod::HasOccupiedRaidLowInstanceForMap(ObjectGuid playerGUID, uint32 raidLowMapID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto raidLowInstanceStateIt = RaidLowInstanceStateByPlayerGUID.find(playerGUID);
    if (raidLowInstanceStateIt == RaidLowInstanceStateByPlayerGUID.end())
        return false;
    if (raidLowInstanceStateIt->second.RaidLowMapID != raidLowMapID || raidLowInstanceStateIt->second.InstanceID == 0)
        return false;

    uint32 instanceID = raidLowInstanceStateIt->second.InstanceID;
    for (auto const& raidLowInstanceStateByPlayerGUID : RaidLowInstanceStateByPlayerGUID)
    {
        if (raidLowInstanceStateByPlayerGUID.first == playerGUID)
            continue;
        if (raidLowInstanceStateByPlayerGUID.second.IsInside == false)
            continue;
        if (raidLowInstanceStateByPlayerGUID.second.RaidLowMapID == raidLowMapID && raidLowInstanceStateByPlayerGUID.second.InstanceID == instanceID)
            return true;
    }
    return false;
}

bool EverQuestMod::ShouldZoneLineEnterInstanceRaidLow(Player* player, uint32 raidLowMapID)
{
    if (player == nullptr || raidLowMapID == 0)
        return false;

    // Running back to a corpse left inside the instance
    if (player->HasCorpse() == true && player->GetCorpseLocation().GetMapId() == raidLowMapID)
        return true;

    // The instance last entered is still occupied, so the rest of the raid is in there
    return HasOccupiedRaidLowInstanceForMap(player->GetGUID(), raidLowMapID);
}

bool EverQuestMod::TryZoneLineIntoInstanceRaidLow(Player* player, AreaTrigger const* trigger)
{
    if (player == nullptr || trigger == nullptr)
        return false;

    AreaTriggerTeleport const* areaTriggerTeleport = sObjectMgr->GetAreaTriggerTeleport(trigger->entry);
    if (areaTriggerTeleport == nullptr)
        return false;

    uint32 raidLowMapID = GetInstanceRaidLowMapIDForMap(areaTriggerTeleport->target_mapId);
    if (raidLowMapID == 0)
        return false;
    if (player->GetMapId() == raidLowMapID)
        return false;
    if (ShouldZoneLineEnterInstanceRaidLow(player, raidLowMapID) == false)
        return false;

    // If the core refuses the instance (no raid group, bound elsewhere, full) then fall through and let the zone line work normally
    return player->TeleportTo(raidLowMapID, areaTriggerTeleport->target_X, areaTriggerTeleport->target_Y, areaTriggerTeleport->target_Z,
        areaTriggerTeleport->target_Orientation, TELE_TO_NOT_LEAVE_TRANSPORT);
}

bool EverQuestMod::TryZoneLineIntoInstanceDungeon(Player* player, AreaTrigger const* trigger)
{
    if (player == nullptr || trigger == nullptr)
        return false;

    AreaTriggerTeleport const* areaTriggerTeleport = sObjectMgr->GetAreaTriggerTeleport(trigger->entry);
    if (areaTriggerTeleport == nullptr)
        return false;

    uint32 dungeonMapID = GetInstanceDungeonMapIDForMap(areaTriggerTeleport->target_mapId);
    if (dungeonMapID == 0)
        return false;
    if (player->GetMapId() == dungeonMapID)
        return false;
    if (GetDungeonModeInstancedForPlayer(player) == false)
        return false;

    // If the core refuses the instance (full, someone else's group already inside, too many instances this hour) then fall through and let the zone line put the player in the open world copy of the zone instead
    return player->TeleportTo(dungeonMapID, areaTriggerTeleport->target_X, areaTriggerTeleport->target_Y, areaTriggerTeleport->target_Z,
        areaTriggerTeleport->target_Orientation, TELE_TO_NOT_LEAVE_TRANSPORT);
}

uint32 EverQuestMod::GetInstanceMapIDForZoneTeleport(Player* player, uint32 destinationMapID)
{
    uint32 instanceDungeonMapID = GetInstanceDungeonMapIDForMap(destinationMapID);
    uint32 instanceRaidLowMapID = GetInstanceRaidLowMapIDForMap(destinationMapID);

    // Already standing in a private copy of the destination zone, so a teleport from one part of it to another stays in that copy.  Without this a pad inside an instance would quietly drop the character back out
    // into the shared world, since the pad only knows the open world map
    if (instanceDungeonMapID != 0 && player->GetMapId() == instanceDungeonMapID)
        return instanceDungeonMapID;
    if (instanceRaidLowMapID != 0 && player->GetMapId() == instanceRaidLowMapID)
        return instanceRaidLowMapID;

    // Travelling into the zone from outside it, which is the same decision a zone line makes
    if (instanceDungeonMapID != 0 && GetDungeonModeInstancedForPlayer(player) == true)
        return instanceDungeonMapID;
    if (instanceRaidLowMapID != 0 && ShouldZoneLineEnterInstanceRaidLow(player, instanceRaidLowMapID) == true)
        return instanceRaidLowMapID;
    return 0;
}

bool EverQuestMod::TryRerouteZoneTeleportIntoInstance(Player* player, uint32 destinationMapID, float x, float y, float z, float orientation, uint32 options)
{
    if (player == nullptr)
        return false;
    if (player->m_InstanceValid == false)
        return false;
    if (IsZoneTeleportDestination(destinationMapID, x, y, z) == false)
        return false;

    uint32 instanceMapID = GetInstanceMapIDForZoneTeleport(player, destinationMapID);
    if (instanceMapID == 0)
        return false;

    return player->TeleportTo(instanceMapID, x, y, z, orientation, options);
}

void EverQuestMod::SendInstanceDungeonEntryMessageToPlayer(Player* player)
{
    if (player == nullptr || player->GetSession() == nullptr)
        return;

    // The MapInstanced container of an instanceable map holds no players itself, only its child instances do
    Map* map = player->FindMap();
    if (map == nullptr || map->GetInstanceId() == 0)
        return;
    if (IsMapInstanceDungeon(map->GetId()) == false)
        return;

    // The instance copy's own map name carries a suffix, so the zone's plain name comes off the open world version
    std::string zoneName = map->GetMapName();
    MapEntry const* openWorldMapEntry = sMapStore.LookupEntry(GetOpenWorldMapIDForMapID(map->GetId()));
    if (openWorldMapEntry != nullptr)
        zoneName = openWorldMapEntry->name[sWorld->GetDefaultDbcLocale()];

    ChatHandler(player->GetSession()).PSendSysMessage("You have entered an |cff4CFF00instanced|r version of {}, a private copy for you and your group.", zoneName);
}

void EverQuestMod::RestoreInstanceValidityOutsideInstances(Player* player)
{
    if (player == nullptr || player->m_InstanceValid == true)
        return;
    Map* map = player->FindMap();
    if (map == nullptr || map->IsDungeon() == true)
        return;
    player->m_InstanceValid = true;
}

bool EverQuestMod::TeleportPlayerOutOfInstanceForEviction(Player* player)
{
    Map* map = player->FindMap();
    if (map == nullptr)
        return false;

    uint32 openWorldMapID = GetOpenWorldMapIDForMapID(map->GetId());
    if (openWorldMapID != map->GetId())
    {
        auto zoneSafePointIter = ZoneSafePointByMapID.find(openWorldMapID);
        if (zoneSafePointIter != ZoneSafePointByMapID.end())
        {
            const EverQuestZoneSafePoint& zoneSafePoint = zoneSafePointIter->second;
            player->TeleportTo({ openWorldMapID, { zoneSafePoint.X, zoneSafePoint.Y, zoneSafePoint.Z, zoneSafePoint.Orientation } });
            return true;
        }

        // An instance copy is a clone of the open world zone's geometry, so the position stood in is a valid one over there
        player->TeleportTo({ openWorldMapID, { player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation() } });
        return true;
    }

    // No open world copy to land in, so the character's own bind point stands in for it
    uint32 bindMapID = 0;
    float bindX = 0;
    float bindY = 0;
    float bindZ = 0;
    if (TryGetEQBindHomePosition(player, bindMapID, bindX, bindY, bindZ) == true && bindMapID != map->GetId())
    {
        player->TeleportTo({ bindMapID, { bindX, bindY, bindZ, player->GetOrientation() } });
        return true;
    }
    return false;
}

bool EverQuestMod::HandleInstanceEvictionRepop(Player* player)
{
    if (player == nullptr || player->m_InstanceValid == true)
        return true;

    // Player::UpdateHomebindTime never evicts a game master, so neither does this
    if (player->IsGameMaster() == true)
        return true;
    Map* map = player->FindMap();
    if (map == nullptr || map->IsDungeon() == false)
        return true;
    if (IsMapInstanceDungeon(map->GetId()) == false && IsMapInstanceRaidLow(map->GetId()) == false)
        return true;
    return TeleportPlayerOutOfInstanceForEviction(player) == false;
}

bool EverQuestMod::IsBlockedByAgroZDistance(WorldObject const* source, WorldObject const* target, float maxAgroZDistance)
{
    if (maxAgroZDistance < 0.0f)
        return false;
    if (source == nullptr || target == nullptr)
        return false;
    return std::fabs(source->GetPositionZ() - target->GetPositionZ()) > maxAgroZDistance;
}

bool EverQuestMod::IsMapRestrictedByExpansion(uint32 mapID)
{
    // A negative maximum disables the restriction entirely
    if (ConfigMapMaxExpansionID < 0)
        return false;

    // Only EverQuest zones carry an expansion, so anything else (like Azeroth) is left to other rules
    auto zoneIt = ZoneByMapID.find(mapID);
    if (zoneIt == ZoneByMapID.end())
        return false;

    return zoneIt->second.ExpansionID > ConfigMapMaxExpansionID;
}

bool EverQuestMod::IsMapRestrictedForPlayers(uint32 mapID)
{
    if (ConfigMapRestrictPlayersToNorrath == true && (mapID < ConfigSystemMapDBCIDMin || mapID > ConfigSystemMapDBCIDMax))
        return true;
    return IsMapRestrictedByExpansion(mapID);
}

bool EverQuestMod::RelocatePlayerOutOfRestrictedMap(Player* player)
{
    // The bind point is preferred, but it is no help if it sits on a restricted map itself
    uint32 bindMapID = 0;
    float bindX = 0;
    float bindY = 0;
    float bindZ = 0;
    if (TryGetEQBindHomePosition(player, bindMapID, bindX, bindY, bindZ) == true && IsMapRestrictedForPlayers(bindMapID) == false)
    {
        player->TeleportTo({ bindMapID, {bindX, bindY, bindZ, player->GetOrientation()} });
        return true;
    }

    // Fall back on where this race and class starts out in Norrath
    if (HasCreatePlayerData(player->getRace(), player->getClass()) == false)
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod could not relocate player {} with GUID {} off of restricted map {}, as they have no EverQuest bind point and no EverQuest create data for race {} and class {}",
            player->GetName(), player->GetGUID().GetCounter(), player->GetMapId(), player->getRace(), player->getClass());
        return false;
    }

    const EverQuestPlayerCreateInfo& createInfo = GetPlayerCreateInfo(player->getRace(), player->getClass());
    if (IsMapRestrictedForPlayers(createInfo.MapID) == true)
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod could not relocate player {} with GUID {} off of restricted map {}, as their EverQuest start map {} is restricted as well. Check EverQuest.Map.MaxExpansionID and EverQuest.Map.RestrictPlayersToNorrath",
            player->GetName(), player->GetGUID().GetCounter(), player->GetMapId(), createInfo.MapID);
        return false;
    }

    player->TeleportTo({ createInfo.MapID, {createInfo.PositionX, createInfo.PositionY, createInfo.PositionZ, createInfo.Orientation} });
    return true;
}

void EverQuestMod::UpdateRestrictedMapPlayerCheck(uint32 diff)
{
    if (ConfigMapRestrictedMapCheckIntervalInSeconds == 0)
        return;

    // Nothing can be restricted unless at least one of the map rules is turned on
    if (ConfigMapRestrictPlayersToNorrath == false && ConfigMapMaxExpansionID < 0)
        return;

    uint32 intervalInMS = ConfigMapRestrictedMapCheckIntervalInSeconds * IN_MILLISECONDS;
    RestrictedMapCheckTimerInMS += diff;
    if (RestrictedMapCheckTimerInMS < intervalInMS)
        return;
    RestrictedMapCheckTimerInMS = 0;

    WorldSessionMgr::SessionMap const& sessions = sWorldSessionMgr->GetAllSessions();
    for (WorldSessionMgr::SessionMap::const_iterator sessionIter = sessions.begin(); sessionIter != sessions.end(); ++sessionIter)
    {
        if (sessionIter->second == nullptr)
            continue;
        Player* player = sessionIter->second->GetPlayer();
        if (player == nullptr || player->IsInWorld() == false || player->IsGameMaster() == true)
            continue;

        // A teleport already in flight gets to land before being judged on where it left from
        if (player->IsBeingTeleported() == true)
            continue;

        if (IsMapRestrictedForPlayers(player->GetMapId()) == false)
            continue;

        if (RelocatePlayerOutOfRestrictedMap(player) == true)
            ChatHandler(player->GetSession()).PSendSysMessage("You have been returned, as you were somewhere you are not permitted to be.");
    }
}

void EverQuestMod::BeginClientVersionCheckForPlayer(Player* player)
{
    if (ConfigClientVersionCheckEnabled == false || ConfigSystemClientDataVersion == 0)
        return;
    if (player == nullptr || player->GetSession() == nullptr)
        return;

    // GM accounts are exempt so work-in-progress clients can still log in
    if (player->GetSession()->GetSecurity() > SEC_PLAYER)
        return;

    EverQuestPlayerClientVersionCheckState checkState;
    checkState.MSUntilDeadline = (int32)(ConfigClientVersionCheckGraceTimeInSeconds * IN_MILLISECONDS);
    checkState.FailedPendingKick = false;
    PendingClientVersionChecksByPlayerGUID[player->GetGUID()] = checkState;
}

void EverQuestMod::HandleClientVersionReportForPlayer(Player* player, uint32 reportedVersion)
{
    if (ConfigClientVersionCheckEnabled == false || ConfigSystemClientDataVersion == 0)
        return;
    if (player == nullptr || player->GetSession() == nullptr)
        return;

    if (reportedVersion == ConfigSystemClientDataVersion)
    {
        PendingClientVersionChecksByPlayerGUID.erase(player->GetGUID());
        return;
    }

    // No pending entry means the player is exempt or already resolved, and an already-failed check keeps its kick countdown
    unordered_map<ObjectGuid, EverQuestPlayerClientVersionCheckState>::iterator checkIter = PendingClientVersionChecksByPlayerGUID.find(player->GetGUID());
    if (checkIter == PendingClientVersionChecksByPlayerGUID.end())
        return;
    if (checkIter->second.FailedPendingKick == true)
        return;
    FailClientVersionCheckForPlayer(player, checkIter->second);
}

void EverQuestMod::FailClientVersionCheckForPlayer(Player* player, EverQuestPlayerClientVersionCheckState& checkState)
{
    checkState.FailedPendingKick = true;
    checkState.MSUntilDeadline = (int32)(ConfigClientVersionCheckKickDelayInSeconds * IN_MILLISECONDS);
    ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000{}|r", ConfigSystemClientDataVersionMismatchMessage);
    ChatHandler(player->GetSession()).SendNotification(ConfigSystemClientDataVersionMismatchMessage);
}

void EverQuestMod::UpdateClientVersionChecks(uint32 diff)
{
    if (ConfigClientVersionCheckEnabled == false || ConfigSystemClientDataVersion == 0)
        return;
    if (PendingClientVersionChecksByPlayerGUID.empty() == true)
        return;

    vector<ObjectGuid> playerGUIDsToKick;
    for (unordered_map<ObjectGuid, EverQuestPlayerClientVersionCheckState>::iterator checkIter = PendingClientVersionChecksByPlayerGUID.begin(); checkIter != PendingClientVersionChecksByPlayerGUID.end(); ++checkIter)
    {
        checkIter->second.MSUntilDeadline -= (int32)diff;
        if (checkIter->second.MSUntilDeadline > 0)
            continue;

        if (checkIter->second.FailedPendingKick == false)
        {
            // Grace time ran out with no version report
            Player* player = ObjectAccessor::FindConnectedPlayer(checkIter->first);
            if (player == nullptr || player->GetSession() == nullptr)
            {
                playerGUIDsToKick.push_back(checkIter->first);
                continue;
            }
            FailClientVersionCheckForPlayer(player, checkIter->second);
        }
        else
            playerGUIDsToKick.push_back(checkIter->first);
    }

    for (ObjectGuid& playerGUID : playerGUIDsToKick)
    {
        PendingClientVersionChecksByPlayerGUID.erase(playerGUID);
        Player* player = ObjectAccessor::FindConnectedPlayer(playerGUID);
        if (player != nullptr && player->GetSession() != nullptr)
            player->GetSession()->KickPlayer("EverQuest client data version mismatch");
    }
}

void EverQuestMod::ClearClientVersionCheckForPlayer(ObjectGuid playerGUID)
{
    PendingClientVersionChecksByPlayerGUID.erase(playerGUID);
}

void EverQuestMod::LoadFactionData()
{
    FactionsByFactionTemplateID.clear();
    DefendCombatFactionTemplateIDs.clear();

    QueryResult queryResult = WorldDatabase.Query("SELECT FactionTemplateID, FactionID, BaseAlignment, PredominantEQRaceID, WillDefendFriendlyPlayers, DefendersWillAttackToDefendPlayer, DefendCombatFactionTemplateID FROM mod_everquest_faction;");
    if (queryResult)
    {
        do
        {
            Field* fields = queryResult->Fetch();
            EverQuestFaction faction;
            faction.FactionTemplateID = fields[0].Get<uint32>();
            faction.FactionID = fields[1].Get<uint32>();
            faction.BaseAlignment = fields[2].Get<uint8>();
            faction.PredominantEQRaceID = fields[3].Get<uint32>();
            faction.WillDefendFriendlyPlayers = fields[4].Get<uint8>() != 0;
            faction.DefendersWillAttackToDefendPlayer = fields[5].Get<uint8>() != 0;
            faction.DefendCombatFactionTemplateID = fields[6].Get<uint32>();
            FactionsByFactionTemplateID[faction.FactionTemplateID] = faction;
        } while (queryResult->NextRow());
    }
}

// Note: Runs at world startup (OnStartup) since the DBC stores aren't loaded when LoadFactionData loads with the config
void EverQuestMod::ResolveDefendCombatFactionTemplates()
{
    // Defend combat templates only work if the deployed FactionTemplate.dbc contains them
    for (auto& factionPair : FactionsByFactionTemplateID)
    {
        EverQuestFaction& faction = factionPair.second;
        if (faction.DefendCombatFactionTemplateID != 0 && sFactionTemplateStore.LookupEntry(faction.DefendCombatFactionTemplateID) == nullptr)
        {
            LOG_ERROR("module.EverQuest", "EverQuestMod::ResolveDefendCombatFactionTemplates faction template {} has defend combat faction template {} which does not exist in FactionTemplate.dbc, disabling the defend combat swap for it", faction.FactionTemplateID, faction.DefendCombatFactionTemplateID);
            faction.DefendCombatFactionTemplateID = 0;
        }
    }

    // Register the combat versions under their own IDs too
    vector<EverQuestFaction> combatVariantFactions;
    for (auto& factionPair : FactionsByFactionTemplateID)
    {
        if (factionPair.second.DefendCombatFactionTemplateID == 0)
            continue;
        DefendCombatFactionTemplateIDs.insert(factionPair.second.DefendCombatFactionTemplateID);
        EverQuestFaction combatVariantFaction = factionPair.second;
        combatVariantFaction.FactionTemplateID = factionPair.second.DefendCombatFactionTemplateID;
        combatVariantFactions.push_back(combatVariantFaction);
    }
    for (EverQuestFaction& combatVariantFaction : combatVariantFactions)
        FactionsByFactionTemplateID[combatVariantFaction.FactionTemplateID] = combatVariantFaction;
}

// Note: Runs at world startup (OnStartup) since the DBC stores aren't loaded when LoadFactionData loads with the config
void EverQuestMod::ResolveEQReputationFactions()
{
    EQReputationFactionInfoByFactionID.clear();
    for (auto& factionPair : FactionsByFactionTemplateID)
    {
        uint32 factionID = factionPair.second.FactionID;
        if (factionID == 0)
            continue;
        FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionID);
        if (factionEntry == nullptr || factionEntry->CanHaveReputation() == false)
            continue;
        EverQuestReputationFactionInfo factionInfo;
        factionInfo.BaseAlignment = factionPair.second.BaseAlignment;
        factionInfo.PredominantEQRaceID = factionPair.second.PredominantEQRaceID;
        EQReputationFactionInfoByFactionID[factionID] = factionInfo;
    }
}

void EverQuestMod::HandleModFactionAuraApplyOnCreature(Creature* creature, Aura* aura)
{
    if (creature == nullptr || aura == nullptr)
        return;
    int32 modFactionRepValue = GetSpellDataForSpellID(aura->GetId()).ModFactionRepValue;
    if (modFactionRepValue == 0)
        return;
    Unit* casterUnit = aura->GetCaster();
    if (casterUnit == nullptr || casterUnit->IsPlayer() == false)
        return;
    Player* casterPlayer = casterUnit->ToPlayer();

    // The creature's faction has to be a reputation faction, which mirrors EQ requiring the target to have a primary faction
    auto factionIter = FactionsByFactionTemplateID.find(creature->GetFaction());
    if (factionIter == FactionsByFactionTemplateID.end() || EQReputationFactionInfoByFactionID.find(factionIter->second.FactionID) == EQReputationFactionInfoByFactionID.end())
    {
        creature->RemoveAura(aura);
        ChatHandler(casterPlayer->GetSession()).PSendSysMessage("Your spell would have no effect on that target.");
        return;
    }

    // EQ clears prior spell faction bonuses when a new one lands, so only the latest bonus holds
    EverQuestPlayerTempFactionBonus bonus;
    bonus.FactionID = factionIter->second.FactionID;
    bonus.Amount = modFactionRepValue;
    bonus.SpellID = aura->GetId();
    bonus.TargetCreatureGUID = creature->GetGUID();
    uint32 priorSpellID = 0;
    ObjectGuid priorTargetCreatureGUID;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto priorBonusIter = TempFactionBonusByPlayerGUID.find(casterPlayer->GetGUID());
        if (priorBonusIter != TempFactionBonusByPlayerGUID.end())
        {
            priorSpellID = priorBonusIter->second.SpellID;
            priorTargetCreatureGUID = priorBonusIter->second.TargetCreatureGUID;
        }
        TempFactionBonusByPlayerGUID[casterPlayer->GetGUID()] = bonus;
    }

    // The displaced bonus leaves its aura behind, so take that aura off unless this apply is a refresh of it
    if (priorSpellID != 0 && (priorTargetCreatureGUID != creature->GetGUID() || priorSpellID != aura->GetId()))
    {
        Creature* priorCreature = ObjectAccessor::GetCreature(*casterPlayer, priorTargetCreatureGUID);
        if (priorCreature != nullptr)
            priorCreature->RemoveAurasDueToSpell(priorSpellID, casterPlayer->GetGUID());
    }

    // The caster shares the creature's map, so it's safe to recalculate directly
    RecalculateTemporaryFactionReactionsForPlayer(casterPlayer);
}

void EverQuestMod::HandleModFactionAuraRemoveFromCreature(Creature* creature, AuraApplication* aurApp)
{
    if (creature == nullptr || aurApp == nullptr || aurApp->GetBase() == nullptr)
        return;
    uint32 spellID = aurApp->GetBase()->GetId();
    if (GetSpellDataForSpellID(spellID).ModFactionRepValue == 0)
        return;

    // Only clear the bonus if this exact aura granted the one currently held
    ObjectGuid casterGUID = aurApp->GetBase()->GetCasterGUID();
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto bonusIter = TempFactionBonusByPlayerGUID.find(casterGUID);
        if (bonusIter == TempFactionBonusByPlayerGUID.end())
            return;
        if (bonusIter->second.SpellID != spellID || bonusIter->second.TargetCreatureGUID != creature->GetGUID())
            return;
        TempFactionBonusByPlayerGUID.erase(bonusIter);
    }

    // The caster may be on another map by now, so recalculate on the caster's own update instead of directly
    QueueTemporaryFactionRecalculationForPlayer(casterGUID);
}

void EverQuestMod::RecalculateTemporaryFactionReactionsForPlayer(Player* player)
{
    if (player == nullptr || player->GetSession() == nullptr)
        return;

    // An active illusion form makes factions react to the player by the form's alignment and race instead of their own
    uint8 illusionAlignment = EQ_FACTION_ALIGNMENT_NONE;
    uint32 illusionEQRaceID = 0;
    for (auto const& appliedAuraItr : player->GetAppliedAuras())
    {
        AuraApplication const* appliedAurApp = appliedAuraItr.second;
        if (appliedAurApp == nullptr || appliedAurApp->GetBase() == nullptr)
            continue;
        const EverQuestSpell& appliedSpell = GetSpellDataForSpellID(appliedAurApp->GetBase()->GetId());
        if (appliedSpell.IllusionFormEQRaceID != 0)
        {
            illusionAlignment = appliedSpell.IllusionFormAlignment;
            illusionEQRaceID = appliedSpell.IllusionFormEQRaceID;
            break;
        }
    }

    // Grab any held spell faction bonus
    uint32 bonusFactionID = 0;
    int32 bonusAmount = 0;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto bonusIter = TempFactionBonusByPlayerGUID.find(player->GetGUID());
        if (bonusIter != TempFactionBonusByPlayerGUID.end())
        {
            bonusFactionID = bonusIter->second.FactionID;
            bonusAmount = bonusIter->second.Amount;
        }
    }

    // The form's alignment vs the player's baseline alignment determines how many faction bands to step
    int32 stepsTowardGoodFactions = 0;
    int32 stepsTowardEvilFactions = 0;
    if (illusionAlignment != EQ_FACTION_ALIGNMENT_NONE)
        GetIllusionFactionBandSteps(GetPlayerBaselineFactionAlignment(player), illusionAlignment, stepsTowardGoodFactions, stepsTowardEvilFactions);

    // Force a reaction on every faction where the adjustments land in a different band than the real standing
    ReputationMgr& reputationMgr = player->GetReputationMgr();
    vector<uint32> newForcedFactionIDs;
    for (auto& factionInfoPair : EQReputationFactionInfoByFactionID)
    {
        uint32 factionID = factionInfoPair.first;
        const EverQuestReputationFactionInfo& factionInfo = factionInfoPair.second;
        FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionID);
        if (factionEntry == nullptr)
            continue;

        // A ModFaction bonus adjusts the standing value before it becomes a band
        int32 naturalStanding = reputationMgr.GetReputation(factionEntry);
        int32 adjustedStanding = naturalStanding;
        if (bonusFactionID == factionID)
            adjustedStanding += bonusAmount;
        if (adjustedStanding > ReputationMgr::Reputation_Cap)
            adjustedStanding = ReputationMgr::Reputation_Cap;
        else if (adjustedStanding < ReputationMgr::Reputation_Bottom)
            adjustedStanding = ReputationMgr::Reputation_Bottom;
        ReputationRank naturalRank = ReputationMgr::ReputationToRank(naturalStanding);
        int32 adjustedRankValue = (int32)ReputationMgr::ReputationToRank(adjustedStanding);

        // Illusion band steps only move factions with a good or evil baseline; None and Neutral factions never move
        if (factionInfo.BaseAlignment == EQ_FACTION_ALIGNMENT_GOOD)
            adjustedRankValue += stepsTowardGoodFactions;
        else if (factionInfo.BaseAlignment == EQ_FACTION_ALIGNMENT_EVIL)
            adjustedRankValue += stepsTowardEvilFactions;

        // Bonus band step when the form's race matches the faction's predominant member race (like Illusion: Human to EQ humans)
        if ((factionInfo.BaseAlignment == EQ_FACTION_ALIGNMENT_GOOD || factionInfo.BaseAlignment == EQ_FACTION_ALIGNMENT_EVIL) &&
            illusionEQRaceID != 0 && factionInfo.PredominantEQRaceID == illusionEQRaceID)
            adjustedRankValue += 1;

        if (adjustedRankValue > (int32)REP_EXALTED)
            adjustedRankValue = (int32)REP_EXALTED;
        else if (adjustedRankValue < (int32)REP_HATED)
            adjustedRankValue = (int32)REP_HATED;
        if ((ReputationRank)adjustedRankValue == naturalRank)
            continue;
        reputationMgr.ApplyForceReaction(factionID, (ReputationRank)adjustedRankValue, true);
        newForcedFactionIDs.push_back(factionID);
    }

    vector<uint32> noLongerForcedFactionIDs;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto priorIter = ForcedFactionReactionIDsByPlayerGUID.find(player->GetGUID());
        if (priorIter != ForcedFactionReactionIDsByPlayerGUID.end())
        {
            for (uint32 priorFactionID : priorIter->second)
            {
                bool stillForced = false;
                for (uint32 newFactionID : newForcedFactionIDs)
                {
                    if (newFactionID == priorFactionID)
                    {
                        stillForced = true;
                        break;
                    }
                }
                if (stillForced == false)
                    noLongerForcedFactionIDs.push_back(priorFactionID);
            }
        }
        if (newForcedFactionIDs.empty() == true)
        {
            if (priorIter != ForcedFactionReactionIDsByPlayerGUID.end())
                ForcedFactionReactionIDsByPlayerGUID.erase(priorIter);
        }
        else
            ForcedFactionReactionIDsByPlayerGUID[player->GetGUID()] = newForcedFactionIDs;
    }
    for (uint32 noLongerForcedFactionID : noLongerForcedFactionIDs)
        reputationMgr.ApplyForceReaction(noLongerForcedFactionID, REP_NEUTRAL, false);

    // Push the current forced reaction set to the client so con colors and interactions update immediately
    reputationMgr.SendForceReactions();
}

void EverQuestMod::QueueTemporaryFactionRecalculationForPlayer(ObjectGuid playerGUID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    PlayersPendingTempFactionRecalculation.insert(playerGUID);
}

void EverQuestMod::ConsumePendingTemporaryFactionRecalculation(Player* player)
{
    if (player == nullptr)
        return;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        if (PlayersPendingTempFactionRecalculation.empty() == true)
            return;
        auto pendingIter = PlayersPendingTempFactionRecalculation.find(player->GetGUID());
        if (pendingIter == PlayersPendingTempFactionRecalculation.end())
            return;
        PlayersPendingTempFactionRecalculation.erase(pendingIter);
    }
    RecalculateTemporaryFactionReactionsForPlayer(player);
}

uint8 EverQuestMod::GetPlayerBaselineFactionAlignment(Player* player)
{
    // A player's baseline alignment starts with their class and, only if the class is neutral, falls back to their race
    // Example: a warrior is evil if undead, neutral if gnome, good if dwarf - but a paladin is always good regardless of race
    uint32 classMask = player->getClassMask();
    if ((classMask & ConfigSystemFactionGoodClassMask) != 0)
        return EQ_FACTION_ALIGNMENT_GOOD;
    if ((classMask & ConfigSystemFactionEvilClassMask) != 0)
        return EQ_FACTION_ALIGNMENT_EVIL;
    uint32 raceMask = player->getRaceMask();
    if ((raceMask & ConfigSystemFactionGoodRaceMask) != 0)
        return EQ_FACTION_ALIGNMENT_GOOD;
    if ((raceMask & ConfigSystemFactionEvilRaceMask) != 0)
        return EQ_FACTION_ALIGNMENT_EVIL;
    return EQ_FACTION_ALIGNMENT_NEUTRAL;
}

void EverQuestMod::GetIllusionFactionBandSteps(uint8 playerAlignment, uint8 illusionAlignment, int32& stepsTowardGoodOut, int32& stepsTowardEvilOut)
{
    // How many faction bands an illusion form steps the player toward good-baseline and evil-baseline factions, based on the gap between the player's baseline alignment and the form's alignment
    stepsTowardGoodOut = 0;
    stepsTowardEvilOut = 0;
    if (illusionAlignment == playerAlignment)
        return;
    if (playerAlignment == EQ_FACTION_ALIGNMENT_GOOD)
    {
        if (illusionAlignment == EQ_FACTION_ALIGNMENT_NEUTRAL)
            stepsTowardEvilOut = 2;
        else if (illusionAlignment == EQ_FACTION_ALIGNMENT_EVIL)
        {
            stepsTowardGoodOut = -2;
            stepsTowardEvilOut = 3;
        }
    }
    else if (playerAlignment == EQ_FACTION_ALIGNMENT_EVIL)
    {
        if (illusionAlignment == EQ_FACTION_ALIGNMENT_NEUTRAL)
            stepsTowardGoodOut = 2;
        else if (illusionAlignment == EQ_FACTION_ALIGNMENT_GOOD)
        {
            stepsTowardGoodOut = 3;
            stepsTowardEvilOut = -2;
        }
    }
    else // Neutral player
    {
        if (illusionAlignment == EQ_FACTION_ALIGNMENT_GOOD)
        {
            stepsTowardGoodOut = 2;
            stepsTowardEvilOut = -2;
        }
        else if (illusionAlignment == EQ_FACTION_ALIGNMENT_EVIL)
        {
            stepsTowardGoodOut = -2;
            stepsTowardEvilOut = 2;
        }
    }
}

void EverQuestMod::ClearTemporaryFactionStateForPlayer(ObjectGuid playerGUID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    TempFactionBonusByPlayerGUID.erase(playerGUID);
    ForcedFactionReactionIDsByPlayerGUID.erase(playerGUID);
    PlayersPendingTempFactionRecalculation.erase(playerGUID);
}

void EverQuestMod::ClearTempFactionBonusForPlayer(Player* player)
{
    if (player == nullptr)
        return;
    uint32 bonusSpellID = 0;
    ObjectGuid bonusTargetCreatureGUID;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto bonusIter = TempFactionBonusByPlayerGUID.find(player->GetGUID());
        if (bonusIter == TempFactionBonusByPlayerGUID.end())
            return;
        bonusSpellID = bonusIter->second.SpellID;
        bonusTargetCreatureGUID = bonusIter->second.TargetCreatureGUID;
        TempFactionBonusByPlayerGUID.erase(bonusIter);
    }

    // Take the orphaned aura off before the map change (when it won't be possible)
    if (player->IsInWorld() == true)
    {
        Creature* bonusTargetCreature = ObjectAccessor::GetCreature(*player, bonusTargetCreatureGUID);
        if (bonusTargetCreature != nullptr)
            bonusTargetCreature->RemoveAurasDueToSpell(bonusSpellID, player->GetGUID());
    }

    // Recalculate on the player's own update since this can run mid-death or mid-teleport
    QueueTemporaryFactionRecalculationForPlayer(player->GetGUID());
}

bool EverQuestMod::IsPlayerFriendlyWithCreatureByReputation(Creature* creature, Player* player)
{
    FactionTemplateEntry const* factionTemplateEntry = creature->GetFactionTemplateEntry();
    if (factionTemplateEntry == nullptr)
        return false;
    FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionTemplateEntry->faction);
    if (factionEntry == nullptr || factionEntry->CanHaveReputation() == false)
        return false;
    return player->GetReputationMgr().GetRank(factionEntry) >= REP_FRIENDLY;
}

void EverQuestMod::DoDefendFriendlyPlayersSearch(Creature* attacker, Player* attackedPlayer)
{
    if (ConfigFactionDefendFriendlyPlayersEnabled == false)
        return;
    if (attacker == nullptr || attackedPlayer == nullptr)
        return;

    // The zone's vertical agro limit applies to defenders too, since this is agro like any other
    float maxAgroZDistance = -1.0f;
    uint32 mapID = attackedPlayer->GetMapId();
    if (mapID >= ConfigSystemMapDBCIDMin && mapID <= ConfigSystemMapDBCIDMax)
        maxAgroZDistance = GetMaxAgroZDistanceForMap(mapID);

    std::list<Creature*> nearbyCreatures;
    Acore::AnyUnitInObjectRangeCheck check(attackedPlayer, EQ_DEFEND_PLAYERS_SEARCH_RADIUS);
    Acore::CreatureListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(attackedPlayer, nearbyCreatures, check);
    Cell::VisitObjects(attackedPlayer, searcher, EQ_DEFEND_PLAYERS_SEARCH_RADIUS);

    for (Creature* defender : nearbyCreatures)
    {
        if (defender == attacker)
            continue;

        // Both ends of the fight are checked, as the defender has to be able to reach the player it is coming to help as well as the attacker it would be engaging
        if (IsBlockedByAgroZDistance(defender, attackedPlayer, maxAgroZDistance) == true)
            continue;
        if (IsBlockedByAgroZDistance(defender, attacker, maxAgroZDistance) == true)
            continue;
        auto factionIter = FactionsByFactionTemplateID.find(defender->GetFaction());
        if (factionIter == FactionsByFactionTemplateID.end() || factionIter->second.WillDefendFriendlyPlayers == false)
            continue;
        if (defender->IsPet() == true || defender->IsControlledByPlayer() == true)
            continue;
        if (defender->GetReactionTo(attackedPlayer) < REP_FRIENDLY && IsPlayerFriendlyWithCreatureByReputation(defender, attackedPlayer) == false)
            continue;
        if (defender->IsInCombatWith(attacker) == true)
            continue;
        if (defender->IsValidAttackTarget(attacker) == false)
        {
            // Creature-vs-creature combat is only valid when the factions are hostile
            uint32 defendCombatFactionTemplateID = factionIter->second.DefendCombatFactionTemplateID;
            uint32 originalFactionTemplateID = defender->GetFaction();
            if (defendCombatFactionTemplateID == 0 || originalFactionTemplateID == defendCombatFactionTemplateID)
                continue;
            defender->SetFaction(defendCombatFactionTemplateID);
            if (defender->IsValidAttackTarget(attacker) == false)
            {
                defender->SetFaction(originalFactionTemplateID);
                continue;
            }
        }
        defender->EngageWithTarget(attacker);
    }
}

void EverQuestMod::UpdateCreatureDefendFactionRestore(Creature* creature)
{
    if (DefendCombatFactionTemplateIDs.empty() == true)
        return;
    if (creature->IsInCombat() == true)
        return;
    if (DefendCombatFactionTemplateIDs.find(creature->GetFaction()) == DefendCombatFactionTemplateIDs.end())
        return;
    creature->SetFaction(creature->GetCreatureTemplate()->faction);
}

void EverQuestMod::UpdateCreatureDefendFriendlyPlayers(Creature* creature, uint32 diff)
{
    if (ConfigFactionDefendFriendlyPlayersEnabled == false)
        return;
    if (creature == nullptr || FactionsByFactionTemplateID.empty() == true)
        return;

    auto factionIter = FactionsByFactionTemplateID.find(creature->GetFaction());
    bool eligible = factionIter != FactionsByFactionTemplateID.end() && factionIter->second.DefendersWillAttackToDefendPlayer == true &&
        creature->IsAlive() == true && creature->IsInCombat() == true &&
        creature->IsPet() == false && creature->IsControlledByPlayer() == false;
    Player* attackedPlayer = nullptr;
    if (eligible == true)
    {
        Unit* victim = creature->GetVictim();
        if (victim != nullptr && victim->IsAlive() == true)
            attackedPlayer = victim->ToPlayer();
    }
    if (attackedPlayer == nullptr)
    {
        RemoveCreatureDefendPlayerWatchState(creature);
        return;
    }

    EverQuestCreatureDefendPlayerWatchState* state = creature->CustomData.GetDefault<EverQuestCreatureDefendPlayerWatchState>(EQ_CREATURE_CUSTOMDATA_DEFENDPLAYERWATCH);
    if (state->RecheckTimerMS <= diff)
    {
        DoDefendFriendlyPlayersSearch(creature, attackedPlayer);
        state->RecheckTimerMS = EQ_DEFEND_PLAYERS_CHECK_MS;
    }
    else
        state->RecheckTimerMS -= diff;
}

void EverQuestMod::RemoveCreatureDefendPlayerWatchState(Creature* creature)
{
    creature->CustomData.Erase(EQ_CREATURE_CUSTOMDATA_DEFENDPLAYERWATCH);
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

bool EverQuestMod::DoesPlayerHaveEQClassOfWOWClass(Player* player, uint8 wowClassID)
{
    if (player == nullptr)
        return false;
    if (player->getClass() == wowClassID)
        return true;

    // Resolve the EQ class that this WoW class represents, since the mapping is data-driven and can change
    uint8 mappedEQClassID = GetClassMapForWOWClassID(wowClassID).EQClassIDBase;
    if (mappedEQClassID == EQ_EQCLASS_NONE)
        return false;
    if (GetClassMapForWOWClassID(player->getClass()).EQClassIDBase == mappedEQClassID)
        return true;
    return (GetCurrentSecondEQClassForPlayer(player) == mappedEQClassID);
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

    // Gates inside instances can only be returned to while the copy still exists (open world map will be zero here)
    uint32 instanceID = player->GetMap()->GetInstanceId();

    // Upsert only the last-gate columns so the class-controller and home-bind data sharing this row is preserved
    CharacterDatabase.Execute("INSERT INTO `mod_everquest_character_settings` (`guid`, `lastgateMapId`, `lastgateZoneId`, `lastgatePosX`, `lastgatePosY`, `lastgatePosZ`, `lastgateOrientation`, `lastgateInstanceId`) VALUES ({}, {}, {}, {}, {}, {}, {}, {}) "
        "ON DUPLICATE KEY UPDATE `lastgateMapId` = {}, `lastgateZoneId` = {}, `lastgatePosX` = {}, `lastgatePosY` = {}, `lastgatePosZ` = {}, `lastgateOrientation` = {}, `lastgateInstanceId` = {}",
        guidCounter, mapID, zoneID, playerX, playerY, playerZ, playerOrientation, instanceID,
        mapID, zoneID, playerX, playerY, playerZ, playerOrientation, instanceID);
}

void EverQuestMod::SendPlayerToLastGate(Player* player)
{
    if (player == nullptr || player->GetSession() == nullptr)
        return;

    // Fail if in combat
    if (player->IsInCombat() == true)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("Your gate tether broke due to being in combat!");
        return;
    }

    // Pull the last gate position
    QueryResult queryResult = CharacterDatabase.Query("SELECT lastgateMapId, lastgateZoneId, lastgatePosX, lastgatePosY, lastgatePosZ, lastgateOrientation, lastgateInstanceId FROM mod_everquest_character_settings WHERE guid = {} AND lastgateMapId IS NOT NULL", player->GetGUID().GetCounter());
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

    // A row written before the gate remembered its instance has a null here, which reads back as zero and is treated as a copy that is gone
    uint32 instanceId = fields[6].Get<uint32>();

    MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
    if (mapEntry == nullptr)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("No tethered gate could be found. Spell failed.");
        return;
    }

    // Returning to an instance needs to be queued because of map threading
    if (mapEntry->Instanceable() == true)
    {
        QueuePendingGateReturn(player, mapId, instanceId, posX, posY, posZ, orientation);
        return;
    }

    // Teleport the player
    player->TeleportTo({ mapId, {posX, posY, posZ, orientation} });
}

void EverQuestMod::QueuePendingGateReturn(Player* player, uint32 mapID, uint32 instanceID, float x, float y, float z, float orientation)
{
    EverQuestPendingGateReturn pendingGateReturn;
    pendingGateReturn.PlayerGUID = player->GetGUID();
    pendingGateReturn.MapID = mapID;
    pendingGateReturn.InstanceID = instanceID;
    pendingGateReturn.PositionX = x;
    pendingGateReturn.PositionY = y;
    pendingGateReturn.PositionZ = z;
    pendingGateReturn.Orientation = orientation;

    std::lock_guard<std::mutex> lock(PendingGateReturnsMutex);

    // Nothing stops the tether aura being cancelled more than once before the world update runs, and that must not queue a second teleport
    for (EverQuestPendingGateReturn& existingGateReturn : PendingGateReturns)
    {
        if (existingGateReturn.PlayerGUID == pendingGateReturn.PlayerGUID)
        {
            existingGateReturn = pendingGateReturn;
            return;
        }
    }
    PendingGateReturns.push_back(pendingGateReturn);
}

void EverQuestMod::ClearPendingGateReturnForPlayer(ObjectGuid playerGUID)
{
    std::lock_guard<std::mutex> lock(PendingGateReturnsMutex);
    for (auto pendingGateReturnIter = PendingGateReturns.begin(); pendingGateReturnIter != PendingGateReturns.end(); ++pendingGateReturnIter)
    {
        if (pendingGateReturnIter->PlayerGUID == playerGUID)
        {
            PendingGateReturns.erase(pendingGateReturnIter);
            return;
        }
    }
}

void EverQuestMod::ProcessPendingGateReturns()
{
    vector<EverQuestPendingGateReturn> gateReturnsToExecute;
    {
        std::lock_guard<std::mutex> lock(PendingGateReturnsMutex);
        if (PendingGateReturns.empty() == true)
            return;
        gateReturnsToExecute.swap(PendingGateReturns);
    }

    // Teleporting reaches back into the maps and the instance bindings, so none of it runs while holding the lock
    for (const EverQuestPendingGateReturn& pendingGateReturn : gateReturnsToExecute)
        ExecuteGateReturn(pendingGateReturn);
}

void EverQuestMod::ExecuteGateReturn(const EverQuestPendingGateReturn& pendingGateReturn)
{
    // The character can log out, or be part way through another teleport, between cancelling the tether and this update
    Player* player = ObjectAccessor::FindConnectedPlayer(pendingGateReturn.PlayerGUID);
    if (player == nullptr || player->IsInWorld() == false || player->GetSession() == nullptr)
        return;

    if (IsGateReturnInstanceStillAvailableForPlayer(player, pendingGateReturn.MapID, pendingGateReturn.InstanceID) == false)
    {
        SendPlayerToGateReturnFallback(player, pendingGateReturn.MapID, pendingGateReturn.PositionX, pendingGateReturn.PositionY, pendingGateReturn.PositionZ, pendingGateReturn.Orientation);
        return;
    }

    player->TeleportTo({ pendingGateReturn.MapID, { pendingGateReturn.PositionX, pendingGateReturn.PositionY, pendingGateReturn.PositionZ, pendingGateReturn.Orientation } });
}

bool EverQuestMod::IsGateReturnInstanceStillAvailableForPlayer(Player* player, uint32 mapID, uint32 instanceID)
{
    // A gate stored before the instance was tracked, or one taken on a map that turned out not to be instanced, has no copy to go back to
    if (instanceID == 0)
        return false;

    MapEntry const* mapEntry = sMapStore.LookupEntry(mapID);
    if (mapEntry == nullptr)
        return false;

    // Battlegrounds and arenas are placed by battleground ID rather than by an instance binding, and a gate tether has no meaning inside one
    if (mapEntry->IsBattlegroundOrArena() == true)
        return false;

    // Standing in the zone already makes this a same-map teleport, which stays inside the current copy no matter what the binding says.  Without this check a
    // character could gate at a boss, take a fresh copy of the zone, walk in the front door and then cancel the tether to skip straight back to the boss
    if (player->GetMapId() == mapID)
        return player->GetInstanceId() == instanceID;

    // This is the same lookup MapInstanced::CreateInstanceForPlayer performs to decide which copy of the map to place the character in
    return sInstanceSaveMgr->PlayerGetDestinationInstanceId(player, mapID, player->GetDifficulty(mapEntry->IsRaid())) == instanceID;
}

void EverQuestMod::SendPlayerToGateReturnFallback(Player* player, uint32 mapID, float x, float y, float z, float orientation)
{
    // An EverQuest dungeon or raid instance is a clone of an open world zone, so the shared version of that zone is where the tether lands instead and the position held onto is a valid one over there
    uint32 openWorldMapID = GetOpenWorldMapIDForMapID(mapID);
    if (openWorldMapID != mapID)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("The private copy of the zone you gated from is gone, so your tether pulled you into the shared version of it.");
        player->TeleportTo({ openWorldMapID, { x, y, z, orientation } });
        return;
    }

    // A stock WoW instance has no shared version to fall back on, so the tether reaches no further than the normal way in
    AreaTriggerTeleport const* entranceTeleport = sObjectMgr->GetMapEntranceTrigger(mapID);
    if (entranceTeleport != nullptr && entranceTeleport->target_mapId == mapID)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("The instance you gated from has been reset, so your tether pulled you to its entrance.");
        player->TeleportTo(entranceTeleport->target_mapId, entranceTeleport->target_X, entranceTeleport->target_Y, entranceTeleport->target_Z, entranceTeleport->target_Orientation);
        return;
    }

    // Nothing safe to aim at, so the tether fails outright rather than dropping the character somewhere unknown
    ChatHandler(player->GetSession()).PSendSysMessage("Your gate tether broke, as the instance it was anchored to no longer exists.");
}

// Reads the EverQuest bind point, returning false when the player has never bound in Norrath
bool EverQuestMod::TryGetEQBindHomePosition(Player* player, uint32& mapIDOut, float& xOut, float& yOut, float& zOut)
{
    QueryResult queryResult = CharacterDatabase.Query("SELECT homebindMapId, homebindZoneId, homebindPosX, homebindPosY, homebindPosZ FROM mod_everquest_character_settings WHERE guid = {} AND homebindMapId IS NOT NULL", player->GetGUID().GetCounter());
    if (!queryResult || queryResult->GetRowCount() == 0)
        return false;

    // Pull the fields out
    Field* fields = queryResult->Fetch();
    mapIDOut = fields[0].Get<uint32>();
    //uint32 zoneId = fields[1].Get<uint32>();
    xOut = fields[2].Get<float>();
    yOut = fields[3].Get<float>();
    zOut = fields[4].Get<float>();
    return true;
}

void EverQuestMod::SendPlayerToEQBindHome(Player* player)
{
    // Pull the bind position
    uint32 mapId = 0;
    float posX = 0;
    float posY = 0;
    float posZ = 0;
    if (TryGetEQBindHomePosition(player, mapId, posX, posY, posZ) == false)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("You have no bind point in Norrath. Spell failed.");
        return;
    }

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
    // Clear only the home-bind and last-gate columns. The class-controller data sharing this row is left intact
    CharacterDatabase.Execute("UPDATE `mod_everquest_character_settings` SET "
        "`homebindMapId` = NULL, `homebindZoneId` = NULL, `homebindPosX` = NULL, `homebindPosY` = NULL, `homebindPosZ` = NULL, "
        "`lastgateMapId` = NULL, `lastgateZoneId` = NULL, `lastgatePosX` = NULL, `lastgatePosY` = NULL, `lastgatePosZ` = NULL, `lastgateOrientation` = NULL, `lastgateInstanceId` = NULL "
        "WHERE guid = {}", guid.GetCounter());
}

// Instanced maps (like the raid instance zone versions) run one copy of the map per instance ID, so all per-map runtime creature state must be keyed by map AND instance or
// concurrent instances pollute each other (world maps always have an instance ID of 0)
uint64 EverQuestMod::GetMapInstanceKey(Map* map)
{
    return (uint64(map->GetId()) << 32) | uint64(map->GetInstanceId());
}

void EverQuestMod::AddCreatureAsLoaded(Creature* creature)
{
    uint64 mapInstanceKey = GetMapInstanceKey(creature->GetMap());
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    AllLoadedCreaturesByMapInstanceKeyThenCreatureEntryID[mapInstanceKey][creature->GetEntry()].push_back(creature);

    // Track by spawn point and spawn group, if this creature has one
    if (creature->GetSpawnId() != 0 && CreatureSpawnPointsByCreatureGUID.find(creature->GetSpawnId()) != CreatureSpawnPointsByCreatureGUID.end())
    {
        const EverQuestCreatureSpawnPoint& creatureSpawnPoint = CreatureSpawnPointsByCreatureGUID[creature->GetSpawnId()];
        AllLoadedCreaturesByMapInstanceKeyThenSpawnPointID[mapInstanceKey][creatureSpawnPoint.SpawnPointID].push_back(creature);
        AllLoadedCreaturesByMapInstanceKeyThenSpawnGroupID[mapInstanceKey][creatureSpawnPoint.SpawnGroupID].push_back(creature);
    }
}

void EverQuestMod::RemoveCreatureAsLoaded(Creature* creature)
{
    uint64 mapInstanceKey = GetMapInstanceKey(creature->GetMap());
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto entryMapIt = AllLoadedCreaturesByMapInstanceKeyThenCreatureEntryID.find(mapInstanceKey);
    if (entryMapIt != AllLoadedCreaturesByMapInstanceKeyThenCreatureEntryID.end())
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
                    AllLoadedCreaturesByMapInstanceKeyThenCreatureEntryID.erase(entryMapIt);
            }
        }
    }

    // Remove from the spawn point and spawn group trackers, if this creature has one
    if (creature->GetSpawnId() != 0 && CreatureSpawnPointsByCreatureGUID.find(creature->GetSpawnId()) != CreatureSpawnPointsByCreatureGUID.end())
    {
        const EverQuestCreatureSpawnPoint& creatureSpawnPoint = CreatureSpawnPointsByCreatureGUID[creature->GetSpawnId()];
        if (AllLoadedCreaturesByMapInstanceKeyThenSpawnPointID.find(mapInstanceKey) != AllLoadedCreaturesByMapInstanceKeyThenSpawnPointID.end())
        {
            unordered_map<uint32, vector<Creature*>>& spawnPointMap = AllLoadedCreaturesByMapInstanceKeyThenSpawnPointID[mapInstanceKey];
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
                            AllLoadedCreaturesByMapInstanceKeyThenSpawnPointID.erase(mapInstanceKey);
                    }
                }
            }
        }
        if (AllLoadedCreaturesByMapInstanceKeyThenSpawnGroupID.find(mapInstanceKey) != AllLoadedCreaturesByMapInstanceKeyThenSpawnGroupID.end())
        {
            unordered_map<uint32, vector<Creature*>>& spawnGroupMap = AllLoadedCreaturesByMapInstanceKeyThenSpawnGroupID[mapInstanceKey];
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
                            AllLoadedCreaturesByMapInstanceKeyThenSpawnGroupID.erase(mapInstanceKey);
                    }
                }
            }
        }
    }

    auto preloadedMapIt = PreloadedLootItemIDsByMapInstanceKeyThenCreatureGUID.find(mapInstanceKey);
    if (preloadedMapIt != PreloadedLootItemIDsByMapInstanceKeyThenCreatureGUID.end())
    {
        preloadedMapIt->second.erase(creature->GetGUID());
        if (preloadedMapIt->second.empty())
            PreloadedLootItemIDsByMapInstanceKeyThenCreatureGUID.erase(preloadedMapIt);
    }
    auto countsMapIt = PreloadedLootCountsByMapInstanceKeyThenCreatureGUID.find(mapInstanceKey);
    if (countsMapIt != PreloadedLootCountsByMapInstanceKeyThenCreatureGUID.end())
    {
        countsMapIt->second.erase(creature->GetGUID());
        if (countsMapIt->second.empty())
            PreloadedLootCountsByMapInstanceKeyThenCreatureGUID.erase(countsMapIt);
    }
    auto visualMapIt = VisualEquippedItemsByMapInstanceKeyThenCreatureGUID.find(mapInstanceKey);
    if (visualMapIt != VisualEquippedItemsByMapInstanceKeyThenCreatureGUID.end())
    {
        visualMapIt->second.erase(creature->GetGUID());
        if (visualMapIt->second.empty())
            VisualEquippedItemsByMapInstanceKeyThenCreatureGUID.erase(visualMapIt);
    }
}

vector<Creature*> EverQuestMod::GetLoadedCreaturesWithEntryID(Map* map, uint32 entryID)
{
    uint64 mapInstanceKey = GetMapInstanceKey(map);
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto entryMapIt = AllLoadedCreaturesByMapInstanceKeyThenCreatureEntryID.find(mapInstanceKey);
    if (entryMapIt == AllLoadedCreaturesByMapInstanceKeyThenCreatureEntryID.end())
        return vector<Creature*>();
    auto bucketIt = entryMapIt->second.find(entryID);
    if (bucketIt == entryMapIt->second.end())
        return vector<Creature*>();
    return bucketIt->second;
}

void EverQuestMod::RollLootItemsForCreature(Creature* creature)
{
    ObjectGuid creatureGUID = creature->GetGUID();
    uint64 mapInstanceKey = GetMapInstanceKey(creature->GetMap());

    // Clear previous rolls (and empty counts map means it drops nothing). The values are only touched by this
    // creature's own map thread after this, so only the lookups need the lock
    vector<uint32>* preloadedItemIDs = nullptr;
    unordered_map<uint32, uint32>* counts = nullptr;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        preloadedItemIDs = &PreloadedLootItemIDsByMapInstanceKeyThenCreatureGUID[mapInstanceKey][creatureGUID];
        counts = &PreloadedLootCountsByMapInstanceKeyThenCreatureGUID[mapInstanceKey][creatureGUID];
    }
    preloadedItemIDs->clear();
    counts->clear();

    // Skip creatures with no loot data
    auto creatureLootGroups = CreatureLootGroupsByCreatureTemplateID.find(creature->GetEntry());
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

void EverQuestMod::ClearPerMapRuntimeStateForMap(Map* map)
{
    if (map == nullptr)
        return;
    uint64 mapInstanceKey = GetMapInstanceKey(map);

    {
        std::lock_guard<std::mutex> lock(PendingKillSpawnActionsMutex);
        PendingKillSpawnActionsByMapInstanceKey.erase(mapInstanceKey);
        TriggeredQuestKillSpawnsByMapInstanceKey.erase(mapInstanceKey);
    }

    {
        std::lock_guard<std::mutex> lock(PendingArrivalActionsMutex);
        auto watcherIter = PendingArrivalActionsByMapInstanceKey.find(mapInstanceKey);
        if (watcherIter != PendingArrivalActionsByMapInstanceKey.end())
        {
            // The walkers themselves are gone with the map, so their entries in the shared walk set have to go too
            for (const EverQuestPendingArrivalAction& watcher : watcherIter->second)
                ReactionWalkCreatureGUIDs.erase(watcher.MoverGUID);
            PendingArrivalActionsByMapInstanceKey.erase(watcherIter);
            ReactionWalkCreatureCount.store((uint32)ReactionWalkCreatureGUIDs.size());
        }
    }

    {
        std::lock_guard<std::mutex> lock(CycleSpawnCheckTimerMutex);
        CycleSpawnCheckTimerInMSByMapInstanceKey.erase(mapInstanceKey);
    }

    // Map::UnloadAll removes every creature ahead of this, and OnCreatureRemoveWorld clears these per creature, so anything still here means a creature left the map without the hook firing
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        size_t strandedCreatureCount = 0;
        auto entryMapIt = AllLoadedCreaturesByMapInstanceKeyThenCreatureEntryID.find(mapInstanceKey);
        if (entryMapIt != AllLoadedCreaturesByMapInstanceKeyThenCreatureEntryID.end())
        {
            for (const auto& bucket : entryMapIt->second)
                strandedCreatureCount += bucket.second.size();
            AllLoadedCreaturesByMapInstanceKeyThenCreatureEntryID.erase(entryMapIt);
        }
        AllLoadedCreaturesByMapInstanceKeyThenSpawnPointID.erase(mapInstanceKey);
        AllLoadedCreaturesByMapInstanceKeyThenSpawnGroupID.erase(mapInstanceKey);
        PreloadedLootItemIDsByMapInstanceKeyThenCreatureGUID.erase(mapInstanceKey);
        PreloadedLootCountsByMapInstanceKeyThenCreatureGUID.erase(mapInstanceKey);
        VisualEquippedItemsByMapInstanceKeyThenCreatureGUID.erase(mapInstanceKey);
        CreaturesResolvingEQMeleeExtraAttacksByMapInstanceKey.erase(mapInstanceKey);
        EQHasteAuraEffectsByMapInstanceKeyThenUnitGUID.erase(mapInstanceKey);
        if (strandedCreatureCount > 0)
            LOG_ERROR("module.EverQuest", "EverQuestMod::ClearPerMapRuntimeStateForMap dropped {} creature(s) still tracked on map {} instance {} at destruction, which means OnCreatureRemoveWorld did not fire for them", strandedCreatureCount, map->GetId(), map->GetInstanceId());
    }
}

void EverQuestMod::SpawnCreature(uint32 entryID, Map* map, float x, float y, float z, float orientation, bool enforceUniqueSpawn)
{
    if (map == nullptr)
        return;
    if (!sObjectMgr->GetCreatureTemplate(entryID))
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod::SpawnCreature failure, as creature with entryID of {} did not exist in creature templates", entryID);
        return;
    }

    // Quest and gossip reactions can name creatures that don't belong in the instanced copy of the zone they were triggered in
    if (IsCreatureBlockedFromInstanceMap(entryID, map) == true)
        return;

    EverQuestPendingReactionSpawn pendingSpawn;
    pendingSpawn.CreatureTemplateID = entryID;
    pendingSpawn.MapID = map->GetId();
    pendingSpawn.InstanceID = map->GetInstanceId();
    pendingSpawn.PositionX = x;
    pendingSpawn.PositionY = y;
    pendingSpawn.PositionZ = z;
    pendingSpawn.Orientation = orientation;
    pendingSpawn.EnforceUniqueSpawn = enforceUniqueSpawn;

    std::lock_guard<std::mutex> lock(ReactionSpawnedCreaturesMutex);
    ReactionSpawnsPendingCreation.push_back(pendingSpawn);
}

// Only ever called from the world update, where every map thread has been waited on
void EverQuestMod::ExecutePendingReactionSpawn(const EverQuestPendingReactionSpawn& pendingSpawn)
{
    // The map can be gone by now (an instance torn down between the trigger and this update)
    Map* map = sMapMgr->FindMap(pendingSpawn.MapID, pendingSpawn.InstanceID);
    if (map == nullptr)
        return;

    uint32 entryID = pendingSpawn.CreatureTemplateID;
    float x = pendingSpawn.PositionX;
    float y = pendingSpawn.PositionY;
    float z = pendingSpawn.PositionZ;
    float orientation = pendingSpawn.Orientation;

    // Cancel out if it should be a unique spawn, and the creature exists
    if (pendingSpawn.EnforceUniqueSpawn == true)
    {
        vector<Creature*> loadedCreatures = GetLoadedCreaturesWithEntryID(map, entryID);
        if (loadedCreatures.size() > 0)
            return;
    }

    Creature* creature = new Creature();
    if (!creature->Create(map->GenerateLowGuid<HighGuid::Unit>(), map, PHASEMASK_NORMAL, entryID, 0, x, y, z, orientation)) // Players are always in phase 1
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod::ExecutePendingReactionSpawn failure, error calling creature->Create with entryID of {}", entryID);
        delete creature;
        return;
    }

    creature->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), PHASEMASK_NORMAL);
    ObjectGuid::LowType spawnId = creature->GetSpawnId();

    // Taken from .npc add in AzerothCore core: "To call _LoadGoods(); _LoadQuests(); CreateTrainerSpells(), current "creature" variable is deleted
    // and created fresh new, otherwise old values might trigger asserts or cause undefined behavior"
    creature->CleanupsBeforeDelete();
    delete creature;
    creature = new Creature();
    if (!creature->LoadCreatureFromDB(spawnId, map, true, true))
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod::ExecutePendingReactionSpawn failure, as creature with entryID of {} could not be loaded from the database", entryID);
        delete creature;
        return;
    }
    sObjectMgr->AddCreatureToGrid(spawnId, sObjectMgr->GetCreatureData(spawnId));

    // Remove only the database rows so the spawn doesn't persist across restarts.
    WorldDatabase.Execute("DELETE FROM `creature_addon` WHERE `guid` = {}", spawnId);
    WorldDatabase.Execute("DELETE FROM `creature` WHERE `guid` = {}", spawnId);

    // In EverQuest a scripted spawn has no spawn point behind it, so it is gone for good once it dies or depops.
    TrackReactionSpawnedCreature(creature);
}

void EverQuestMod::ProcessPendingReactionSpawnCreations()
{
    vector<EverQuestPendingReactionSpawn> spawnsToCreate;
    {
        std::lock_guard<std::mutex> lock(ReactionSpawnedCreaturesMutex);
        if (ReactionSpawnsPendingCreation.empty() == true)
            return;
        spawnsToCreate.swap(ReactionSpawnsPendingCreation);
    }

    // Creating reaches back into the shared trackers, so none of it runs while holding the lock
    for (const EverQuestPendingReactionSpawn& pendingSpawn : spawnsToCreate)
        ExecutePendingReactionSpawn(pendingSpawn);
}

void EverQuestMod::TrackReactionSpawnedCreature(Creature* creature)
{
    if (creature == nullptr || creature->GetSpawnId() == 0)
        return;
    std::lock_guard<std::mutex> lock(ReactionSpawnedCreaturesMutex);
    ReactionSpawnedCreatureSpawnIDsByMapInstanceKey[GetMapInstanceKey(creature->GetMap())].push_back(creature->GetSpawnId());
    RefreshReactionSpawnedCreatureCount();
}

void EverQuestMod::RefreshReactionSpawnedCreatureCount()
{
    uint32 trackedCount = 0;
    for (auto& spawnIDsByMapInstanceKey : ReactionSpawnedCreatureSpawnIDsByMapInstanceKey)
        trackedCount += (uint32)spawnIDsByMapInstanceKey.second.size();
    ReactionSpawnedCreatureCount.store(trackedCount);
}

void EverQuestMod::RetireReactionSpawnedCreature(Map* map, ObjectGuid::LowType spawnID)
{
    // The respawn queue belongs to this map and this runs on the map's own thread, so it can be cleared right here
    if (map != nullptr)
        map->RemoveCreatureRespawnTime(spawnID);

    // The grid registration is shared state that every map thread reads while loading grids, so dropping it waits for the world update instead
    std::lock_guard<std::mutex> lock(ReactionSpawnedCreaturesMutex);
    ReactionSpawnedCreatureSpawnIDsPendingGridRemoval.push_back(spawnID);
}

void EverQuestMod::ProcessPendingReactionSpawnGridRemovals()
{
    vector<ObjectGuid::LowType> spawnIDsToRemove;
    {
        std::lock_guard<std::mutex> lock(ReactionSpawnedCreaturesMutex);
        if (ReactionSpawnedCreatureSpawnIDsPendingGridRemoval.empty() == true)
            return;
        spawnIDsToRemove.swap(ReactionSpawnedCreatureSpawnIDsPendingGridRemoval);
    }

    for (ObjectGuid::LowType spawnID : spawnIDsToRemove)
    {
        CreatureData const* creatureData = sObjectMgr->GetCreatureData(spawnID);
        if (creatureData != nullptr)
            sObjectMgr->RemoveCreatureFromGrid(spawnID, creatureData);
    }
}

void EverQuestMod::UpdateReactionSpawnedCreatures(Map* map)
{
    // This runs on every map tick, so the common case of nothing reaction-spawned must not take the lock at all
    if (ReactionSpawnedCreatureCount.load() == 0)
        return;
    if (map == nullptr)
        return;
    uint64 mapInstanceKey = GetMapInstanceKey(map);

    vector<ObjectGuid::LowType> spawnIDsToRetire;
    vector<Creature*> creaturesToRemoveFromWorld;
    {
        std::lock_guard<std::mutex> lock(ReactionSpawnedCreaturesMutex);
        auto trackedIter = ReactionSpawnedCreatureSpawnIDsByMapInstanceKey.find(mapInstanceKey);
        if (trackedIter == ReactionSpawnedCreatureSpawnIDsByMapInstanceKey.end())
            return;

        vector<ObjectGuid::LowType> stillSpawnedIDs;
        for (ObjectGuid::LowType spawnID : trackedIter->second)
        {
            // A spawn point can briefly hold more than one creature, a fresh one standing next to its own corpse, so the whole range decides whether the spot is still occupied rather than the first entry
            bool isStillStanding = false;
            vector<Creature*> deadCreatures;
            auto creatureBySpawnIDBounds = map->GetCreatureBySpawnIdStore().equal_range(spawnID);
            for (auto creatureIter = creatureBySpawnIDBounds.first; creatureIter != creatureBySpawnIDBounds.second; ++creatureIter)
            {
                Creature* spawnedCreature = creatureIter->second;

                // Still up, or a corpse that players may not have finished looting, so leave it be
                if (spawnedCreature->IsAlive() == true || spawnedCreature->getDeathState() == DeathState::Corpse)
                {
                    isStillStanding = true;
                    break;
                }
                deadCreatures.push_back(spawnedCreature);
            }
            if (isStillStanding == true)
            {
                stillSpawnedIDs.push_back(spawnID);
                continue;
            }
            for (Creature* deadCreature : deadCreatures)
                creaturesToRemoveFromWorld.push_back(deadCreature);
            spawnIDsToRetire.push_back(spawnID);
        }
        if (stillSpawnedIDs.empty() == true)
            ReactionSpawnedCreatureSpawnIDsByMapInstanceKey.erase(trackedIter);
        else
            trackedIter->second = stillSpawnedIDs;
        RefreshReactionSpawnedCreatureCount();
    }

    // Both of these reach back into the map and the shared queue, so neither runs while holding the lock
    for (Creature* creatureToRemove : creaturesToRemoveFromWorld)
        creatureToRemove->AddObjectToRemoveList();
    for (ObjectGuid::LowType spawnID : spawnIDsToRetire)
        RetireReactionSpawnedCreature(map, spawnID);
}

void EverQuestMod::ClearReactionSpawnedCreaturesForMap(Map* map)
{
    // A map going away takes its creatures with it, so every spawn still tracked on it can be retired right now. Skipping this would leave grid registrations behind that spawn the creature into the next copy of the map
    if (ReactionSpawnedCreatureCount.load() == 0)
        return;
    if (map == nullptr)
        return;
    uint64 mapInstanceKey = GetMapInstanceKey(map);

    vector<ObjectGuid::LowType> spawnIDsToRetire;
    {
        std::lock_guard<std::mutex> lock(ReactionSpawnedCreaturesMutex);
        auto trackedIter = ReactionSpawnedCreatureSpawnIDsByMapInstanceKey.find(mapInstanceKey);
        if (trackedIter == ReactionSpawnedCreatureSpawnIDsByMapInstanceKey.end())
            return;
        spawnIDsToRetire.swap(trackedIter->second);
        ReactionSpawnedCreatureSpawnIDsByMapInstanceKey.erase(trackedIter);
        RefreshReactionSpawnedCreatureCount();
    }

    for (ObjectGuid::LowType spawnID : spawnIDsToRetire)
        RetireReactionSpawnedCreature(map, spawnID);
}

void EverQuestMod::DespawnCreature(uint32 entryID, Map* map)
{
    vector<Creature*> loadedCreatures = GetLoadedCreaturesWithEntryID(map, entryID);
    for (Creature* creature : loadedCreatures)
        if (creature != nullptr)
        {
            DoCreatureEmoteEvent(creature, EQ_CREATURE_EMOTE_EVENT_ONDESPAWN, nullptr);
            creature->DespawnOrUnsummon(0ms);
        }
}

void EverQuestMod::MakeCreatureAttackPlayer(uint32 entryID, Map* map, Player* player)
{
    // Unit::Attack writes into the victim's attacker list, so the player has to belong to the same map as the creatures.
    // Attacking across maps would be writing to another map thread's objects, and means nothing in game anyway
    if (map == nullptr || player == nullptr || player->FindMap() != map)
        return;

    vector<Creature*> loadedCreatures = GetLoadedCreaturesWithEntryID(map, entryID);
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

static thread_local EverQuestPendingSwingTimerRestore PendingSwingTimerRestore;

bool EverQuestMod::ShouldSpellPreserveSwingTimers(uint32 spellID)
{
    if (spellID >= ConfigSystemSpellDBCIDMin && spellID <= ConfigSystemSpellDBCIDMax)
        return ConfigSpellNoSwingTimerResetForEQSpells;
    else
        return ConfigSpellNoSwingTimerResetForWoWSpells;
}

float EverQuestMod::GetFullSwingTimeInMS(Unit* unit, uint8 attackType)
{
    if (unit == nullptr)
        return 0.0f;

    // This is the same value resetAttackTimer writes into the swing timer, since GetAttackTime divides the base attack time by the same attack speed modifier that resetAttackTimer multiplies it back by
    return unit->GetFloatValue(static_cast<uint16>(UNIT_FIELD_BASEATTACKTIME) + attackType);
}

float EverQuestMod::GetSwingTimerRemainingPercent(Unit* unit, uint8 attackType)
{
    float fullSwingTimeInMS = GetFullSwingTimeInMS(unit, attackType);
    if (fullSwingTimeInMS <= 0.0f)
        return -1.0f;

    int32 remainingTimeInMS = unit->getAttackTimer(static_cast<WeaponAttackType>(attackType));
    if (remainingTimeInMS < 0)
        remainingTimeInMS = 0;

    float remainingPercent = static_cast<float>(remainingTimeInMS) / fullSwingTimeInMS;
    if (remainingPercent > 1.0f)
        remainingPercent = 1.0f;
    return remainingPercent;
}

void EverQuestMod::StashSwingTimersBeforeSpellCast(Player* player, Spell* spell)
{
    // Triggered casts never reset the swing timer in the core, so there is nothing to stash or give back for them.
    if (spell != nullptr && spell->IsTriggered() == true)
        return;

    // Clear first on every non-triggered player cast.  A cast that bails out between here and the reset then cannot leave a stale entry sitting on this thread waiting to be matched against some later spell
    PendingSwingTimerRestore.IsActive = false;
    PendingSwingTimerRestore.SpellToken = nullptr;
    PendingSwingTimerRestore.CasterGUID.Clear();

    if (IsEnabled == false)
        return;
    if (player == nullptr || spell == nullptr)
        return;
    SpellInfo const* spellInfo = spell->GetSpellInfo();
    if (spellInfo == nullptr)
        return;
    if (ShouldSpellPreserveSwingTimers(spellInfo->Id) == false)
        return;

    for (uint8 attackType = BASE_ATTACK; attackType < MAX_ATTACK; ++attackType)
        PendingSwingTimerRestore.RemainingPercentByAttackType[attackType] = GetSwingTimerRemainingPercent(player, attackType);

    PendingSwingTimerRestore.SpellToken = static_cast<const void*>(spell);
    PendingSwingTimerRestore.CasterGUID = player->GetGUID();
    PendingSwingTimerRestore.IsActive = true;
}

void EverQuestMod::RestoreSwingTimersAfterSpellCast(Unit* caster, Spell* spell)
{
    if (PendingSwingTimerRestore.IsActive == false)
        return;
    if (spell == nullptr || static_cast<const void*>(spell) != PendingSwingTimerRestore.SpellToken)
        return;

    // Consume the snapshot no matter what happens below, so it can never be applied twice
    PendingSwingTimerRestore.IsActive = false;
    PendingSwingTimerRestore.SpellToken = nullptr;

    if (caster == nullptr || caster->IsPlayer() == false)
        return;
    if (caster->GetGUID() != PendingSwingTimerRestore.CasterGUID)
        return;

    for (uint8 attackType = BASE_ATTACK; attackType < MAX_ATTACK; ++attackType)
    {
        float remainingPercent = PendingSwingTimerRestore.RemainingPercentByAttackType[attackType];
        if (remainingPercent < 0.0f)
            continue;
        float fullSwingTimeInMS = GetFullSwingTimeInMS(caster, attackType);
        if (fullSwingTimeInMS <= 0.0f)
            continue;

        int32 restoredTimeInMS = static_cast<int32>(fullSwingTimeInMS * remainingPercent);

        // Only ever hand swing time back, never take any away, so anything the spell itself legitimately did to the timer is left alone
        if (restoredTimeInMS < caster->getAttackTimer(static_cast<WeaponAttackType>(attackType)))
            caster->setAttackTimer(static_cast<WeaponAttackType>(attackType), restoredTimeInMS);
    }
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

    // Forage rows only exist for the open world copy of a zone, so a raid instance copy forages from its open world map's list
    mapID = GetOpenWorldMapIDForMapID(mapID);
    vector<EverQuestForageZoneItem> forageZoneItems = GetForageZoneItemsInMap(mapID);
    if (forageZoneItems.empty() == true)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("This area has nothing to forage.");
        return;
    }
    // Looked up rather than indexed, since operator[] on a miss would insert into a table that every map thread reads without a lock
    auto totalChanceIter = ForageZoneItemTotalChanceByMapID.find(mapID);
    if (totalChanceIter == ForageZoneItemTotalChanceByMapID.end())
    {
        ChatHandler(player->GetSession()).PSendSysMessage("This area has nothing to forage.");
        return;
    }
    int32 roll = (int32)urand(0, totalChanceIter->second);
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

bool EverQuestMod::IsSummonPlayerSpellBlockedByTarget(uint32 spellID, Unit* target, Unit* caster)
{
    if (caster == nullptr || caster->IsPlayer() == false)
        return false;
    if (spellID < ConfigSystemSpellDBCIDMin || spellID > ConfigSystemSpellDBCIDMax)
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellID);
    if (spellInfo == nullptr)
        return false;
    if (spellInfo->Effects[EFFECT_0].Effect != SPELL_EFFECT_DUMMY)
        return false;
    if (spellInfo->Effects[EFFECT_0].MiscValue != EQ_SPELLDUMMYTYPE_SUMMONPC)
        return false;

    // A non-player was picked on purpose, which never works regardless of where it stands
    if (target != nullptr && target != caster && target->IsPlayer() == false)
        return true;

    Player* targetPlayer = ResolveSummonPlayerTarget(caster->ToPlayer(), target);
    return targetPlayer == nullptr || targetPlayer == caster;
}

bool EverQuestMod::IsSummonPlayerSpell(SpellInfo const* spellInfo)
{
    if (spellInfo == nullptr)
        return false;

    // Converted EQ summons (Call of the Hero and friends) run through the mod's own dummy effect
    if (spellInfo->Id >= ConfigSystemSpellDBCIDMin && spellInfo->Id <= ConfigSystemSpellDBCIDMax && spellInfo->Effects[EFFECT_0].Effect == SPELL_EFFECT_DUMMY && spellInfo->Effects[EFFECT_0].MiscValue == EQ_SPELLDUMMYTYPE_SUMMONPC)
        return true;

    // WoW's own summons (Ritual of Summoning) use the core effect
    for (uint8 effectIndex = EFFECT_0; effectIndex < MAX_SPELL_EFFECTS; effectIndex++)
        if (spellInfo->Effects[effectIndex].Effect == SPELL_EFFECT_SUMMON_PLAYER)
            return true;
    return false;
}

bool EverQuestMod::IsSummonPlayerSpellBlockedByRequiredKey(SpellInfo const* spellInfo, Unit* target, Unit* caster, std::string& outDeniedMessage)
{
    outDeniedMessage.clear();
    if (caster == nullptr || caster->IsPlayer() == false)
        return false;
    if (IsSummonPlayerSpell(spellInfo) == false)
        return false;

    // A summon lands where the caster stands, so the caster's zone is what the target needs the key for
    uint32 requiredKeyItemID = GetRequiredKeyItemIDForMap(caster->GetMapId());
    if (requiredKeyItemID == 0)
        return false;

    // The core resolves a summon's victim off the caster's selection rather than the spell targets, so both are consulted
    Player* targetPlayer = (target != nullptr && target != caster) ? target->ToPlayer() : nullptr;
    if (targetPlayer == nullptr)
        targetPlayer = ObjectAccessor::FindPlayer(caster->ToPlayer()->GetTarget());
    if (targetPlayer == nullptr || targetPlayer == caster)
        return false;

    // A target on another map belongs to another map thread, so its inventory cannot be read from here.  The teleport gate in OnPlayerBeforeTeleport runs the same key check on the target's own thread and still turns them away
    if (targetPlayer->FindMap() != caster->FindMap())
        return false;
    if (DoesPlayerHaveRequiredKeyForMap(targetPlayer, caster->GetMapId()) == true)
        return false;

    outDeniedMessage = Acore::StringFormat("{} cannot be brought into {}, as they do not carry {}.", targetPlayer->GetName(),
        GetZoneNameForMap(caster->GetMapId()), GetRequiredKeyItemName(requiredKeyItemID));
    return true;
}

Player* EverQuestMod::ResolveSummonPlayerTarget(Player* caster, Unit* target)
{
    if (caster == nullptr)
        return nullptr;

    if (target != nullptr && target != caster)
        return target->ToPlayer();

    if (ConfigSpellSummonPlayerAcrossZones == false)
        return nullptr;

    ObjectGuid selectionGUID = caster->GetTarget();
    if (selectionGUID.IsEmpty() == true || selectionGUID.IsPlayer() == false || selectionGUID == caster->GetGUID())
        return nullptr;
    return ObjectAccessor::FindPlayer(selectionGUID);
}

void EverQuestMod::SendSummonRequestToPlayer(Player* targetPlayer, ObjectGuid summonerGUID, uint32 summonerZoneID, uint32 mapID, float x, float y, float z)
{
    targetPlayer->SetSummonPoint(mapID, x, y, z);

    WorldPacket data(SMSG_SUMMON_REQUEST, 8 + 4 + 4);
    data << summonerGUID;                                       // Summoner
    data << uint32(summonerZoneID);                             // Summoner's zone
    data << uint32(MAX_PLAYER_SUMMON_DELAY * IN_MILLISECONDS);  // Auto-decline delay
    targetPlayer->SendDirectMessage(&data);
}

void EverQuestMod::QueueCrossZoneSummonRequest(Player* caster, Player* targetPlayer)
{
    EverQuestPendingSummonRequest request;
    request.CasterGUID = caster->GetGUID();
    request.CasterName = caster->GetName();
    request.MapID = caster->GetMapId();
    request.ZoneID = caster->GetZoneId();
    caster->GetPosition(request.X, request.Y, request.Z);

    ObjectGuid targetPlayerGUID = targetPlayer->GetGUID();
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        PendingSummonRequestByTargetPlayerGUID[targetPlayerGUID] = request;
    }

    ChatHandler(caster->GetSession()).PSendSysMessage("You open a mystic portal for {}.", targetPlayer->GetName());
}

void EverQuestMod::ConsumePendingSummonRequest(Player* player)
{
    if (player == nullptr || player->GetSession() == nullptr)
        return;

    EverQuestPendingSummonRequest request;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        if (PendingSummonRequestByTargetPlayerGUID.empty() == true)
            return;
        auto pendingIter = PendingSummonRequestByTargetPlayerGUID.find(player->GetGUID());
        if (pendingIter == PendingSummonRequestByTargetPlayerGUID.end())
            return;
        request = pendingIter->second;
        PendingSummonRequestByTargetPlayerGUID.erase(pendingIter);
    }

    if (player->IsInWorld() == false || player->IsAlive() == false)
        return;
    if (player->InBattleground() == true || player->InArena() == true)
        return;
    if (player->GetSummonExpireTimer() > GameTime::GetGameTime().count())
        return;

    SendSummonRequestToPlayer(player, request.CasterGUID, request.ZoneID, request.MapID, request.X, request.Y, request.Z);
    ChatHandler(player->GetSession()).PSendSysMessage("{} beckons you through a mystic portal.", request.CasterName);
}

void EverQuestMod::ClearPendingSummonRequestForPlayer(ObjectGuid playerGUID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    PendingSummonRequestByTargetPlayerGUID.erase(playerGUID);
}

void EverQuestMod::ProcessSummonPlayerToCaster(Player* caster, Unit* target)
{
    if (caster == nullptr || caster->GetSession() == nullptr)
        return;

    // Players only
    if (target != nullptr && target != caster && target->IsPlayer() == false)
    {
        ChatHandler(caster->GetSession()).PSendSysMessage("This spell can only be cast on players.");
        return;
    }

    // Default no target to self
    Player* targetPlayer = ResolveSummonPlayerTarget(caster, target);
    if (targetPlayer == nullptr || targetPlayer == caster || targetPlayer->GetSession() == nullptr)
    {
        ChatHandler(caster->GetSession()).PSendSysMessage("The spell failed, as it requires another player as the target.");
        return;
    }

    if (targetPlayer->IsInWorld() == false)
    {
        ChatHandler(caster->GetSession()).PSendSysMessage("The spell failed, as the target could not be reached.");
        return;
    }

    if (caster->GetMap() == nullptr || targetPlayer->GetMap() != caster->GetMap())
    {
        if (ConfigSpellSummonPlayerAcrossZones == false)
        {
            ChatHandler(caster->GetSession()).PSendSysMessage("The spell failed, as the target is not in this zone.");
            return;
        }
        QueueCrossZoneSummonRequest(caster, targetPlayer);
        return;
    }

    if (targetPlayer->IsAlive() == false)
    {
        ChatHandler(caster->GetSession()).PSendSysMessage("The spell failed, as the target could not be reached.");
        return;
    }

    // Don't try to summon PVP
    if (targetPlayer->InBattleground() == true || targetPlayer->InArena() == true)
    {
        ChatHandler(caster->GetSession()).PSendSysMessage("The spell failed, as the target cannot be summoned from where they are.");
        return;
    }

    if (targetPlayer->GetSummonExpireTimer() > GameTime::GetGameTime().count())
    {
        ChatHandler(caster->GetSession()).PSendSysMessage("The spell failed, as the target already has a summon pending.");
        return;
    }

    float casterX = 0.0f;
    float casterY = 0.0f;
    float casterZ = 0.0f;
    caster->GetPosition(casterX, casterY, casterZ);
    SendSummonRequestToPlayer(targetPlayer, caster->GetGUID(), caster->GetZoneId(), caster->GetMapId(), casterX, casterY, casterZ);

    ChatHandler(caster->GetSession()).PSendSysMessage("You open a mystic portal for {}.", targetPlayer->GetName());
    ChatHandler(targetPlayer->GetSession()).PSendSysMessage("{} beckons you through a mystic portal.", caster->GetName());
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
    QueryResult queryResult = CharacterDatabase.Query("SELECT nextSecondaryClass, currentSecondaryClass, secondaryExpPool, illusionFaceId, showBardPulse, issuedIllusionItemId, hideWoWGear, dungeonMode, adventurerDisqualified, deathExpLost, deathExpRestGranted, deathExpLostClass, hailWindowOnRightClick, showDispelMessage, dispelMessageColor, pendingStartItemEQClass FROM mod_everquest_character_settings WHERE guid = {}", player->GetGUID().GetCounter());
    if (!queryResult || queryResult->GetRowCount() == 0)
    {
        const EverQuestClassMap classMap = GetClassMapForWOWClassID(player->getClass());
        controllerData.CurrentSecondClass = classMap.EQClassIDDefaultSecond;
        controllerData.NextSecondClass = classMap.EQClassIDDefaultSecond;
        controllerData.SecondaryExpPool = 0;
        controllerData.IllusionFaceID = 0;
        controllerData.ShowBardPulse = true;
        controllerData.IssuedIllusionItemID = 0;
        controllerData.HideWoWGear = false;
        controllerData.DungeonModeInstanced = false;
        controllerData.HailWindowOnRightClick = false;
        controllerData.ShowDispelMessage = false;
        controllerData.DispelMessageColor = EQ_DISPEL_MESSAGE_DEFAULT_COLOR;
        controllerData.AdventurerDisqualified = false;
        controllerData.DeathExpLost = 0;
        controllerData.DeathExpRestGranted = 0;
        controllerData.DeathExpLostSecondaryClass = 0;
        controllerData.PendingStartItemEQClass = EQ_EQCLASS_NONE;
    }
    else
    {
        Field* fields = queryResult->Fetch();
        controllerData.NextSecondClass = fields[0].Get<uint8>();
        controllerData.CurrentSecondClass = fields[1].Get<uint8>();
        controllerData.SecondaryExpPool = fields[2].Get<uint32>();
        controllerData.IllusionFaceID = (uint32)std::max(0, fields[3].Get<int32>());
        controllerData.ShowBardPulse = fields[4].Get<bool>();
        controllerData.IssuedIllusionItemID = fields[5].Get<uint32>();
        controllerData.HideWoWGear = fields[6].Get<bool>();
        controllerData.DungeonModeInstanced = fields[7].Get<bool>();
        controllerData.AdventurerDisqualified = fields[8].Get<bool>();
        controllerData.DeathExpLost = fields[9].Get<uint32>();
        controllerData.DeathExpRestGranted = fields[10].Get<uint32>();
        controllerData.DeathExpLostSecondaryClass = fields[11].Get<uint8>();
        controllerData.HailWindowOnRightClick = fields[12].Get<bool>();
        controllerData.ShowDispelMessage = fields[13].Get<bool>();
        controllerData.DispelMessageColor = fields[14].Get<uint32>() & 0xFFFFFF;
        controllerData.PendingStartItemEQClass = fields[15].Get<uint8>();
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

bool EverQuestMod::GetShowBardPulseForPlayer(Player* player)
{
    return GetOrLoadActivePlayerClassControllerData(player)->ShowBardPulse;
}

void EverQuestMod::SetShowBardPulseForPlayer(Player* player, bool showBardPulse)
{
    GetOrLoadActivePlayerClassControllerData(player)->ShowBardPulse = showBardPulse;
    SaveShowBardPulseForPlayer(player);
}

void EverQuestMod::SaveShowBardPulseForPlayer(Player* player)
{
    EverQuestPlayerControllerData controllerData;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto controllerDataIt = ActivePlayerClassControllerDataByGUID.find(player->GetGUID());
        if (controllerDataIt == ActivePlayerClassControllerDataByGUID.end())
            return;
        controllerData = controllerDataIt->second;
    }

    CharacterDatabase.Execute("INSERT INTO `mod_everquest_character_settings` (`guid`, `currentSecondaryClass`, `nextSecondaryClass`, `secondaryExpPool`, `showBardPulse`) VALUES ({}, {}, {}, {}, {}) ON DUPLICATE KEY UPDATE `showBardPulse` = {}",
        player->GetGUID().GetCounter(),
        controllerData.CurrentSecondClass,
        controllerData.NextSecondClass,
        controllerData.SecondaryExpPool,
        controllerData.ShowBardPulse == true ? 1 : 0,
        controllerData.ShowBardPulse == true ? 1 : 0);
}

bool EverQuestMod::GetHideWoWGearForPlayer(Player* player)
{
    return GetOrLoadActivePlayerClassControllerData(player)->HideWoWGear;
}

void EverQuestMod::SetHideWoWGearForPlayer(Player* player, bool hideWoWGear)
{
    GetOrLoadActivePlayerClassControllerData(player)->HideWoWGear = hideWoWGear;
    SaveHideWoWGearForPlayer(player);
}

void EverQuestMod::SaveHideWoWGearForPlayer(Player* player)
{
    EverQuestPlayerControllerData controllerData;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto controllerDataIt = ActivePlayerClassControllerDataByGUID.find(player->GetGUID());
        if (controllerDataIt == ActivePlayerClassControllerDataByGUID.end())
            return;
        controllerData = controllerDataIt->second;
    }

    CharacterDatabase.Execute("INSERT INTO `mod_everquest_character_settings` (`guid`, `currentSecondaryClass`, `nextSecondaryClass`, `secondaryExpPool`, `hideWoWGear`) VALUES ({}, {}, {}, {}, {}) ON DUPLICATE KEY UPDATE `hideWoWGear` = {}",
        player->GetGUID().GetCounter(),
        controllerData.CurrentSecondClass,
        controllerData.NextSecondClass,
        controllerData.SecondaryExpPool,
        controllerData.HideWoWGear == true ? 1 : 0,
        controllerData.HideWoWGear == true ? 1 : 0);
}

bool EverQuestMod::GetDungeonModeInstancedForPlayer(Player* player)
{
    return GetOrLoadActivePlayerClassControllerData(player)->DungeonModeInstanced;
}

void EverQuestMod::SetDungeonModeInstancedForPlayer(Player* player, bool dungeonModeInstanced)
{
    GetOrLoadActivePlayerClassControllerData(player)->DungeonModeInstanced = dungeonModeInstanced;
    SaveDungeonModeInstancedForPlayer(player);
}

void EverQuestMod::SaveDungeonModeInstancedForPlayer(Player* player)
{
    EverQuestPlayerControllerData controllerData;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto controllerDataIt = ActivePlayerClassControllerDataByGUID.find(player->GetGUID());
        if (controllerDataIt == ActivePlayerClassControllerDataByGUID.end())
            return;
        controllerData = controllerDataIt->second;
    }

    CharacterDatabase.Execute("INSERT INTO `mod_everquest_character_settings` (`guid`, `currentSecondaryClass`, `nextSecondaryClass`, `secondaryExpPool`, `dungeonMode`) VALUES ({}, {}, {}, {}, {}) ON DUPLICATE KEY UPDATE `dungeonMode` = {}",
        player->GetGUID().GetCounter(),
        controllerData.CurrentSecondClass,
        controllerData.NextSecondClass,
        controllerData.SecondaryExpPool,
        controllerData.DungeonModeInstanced == true ? 1 : 0,
        controllerData.DungeonModeInstanced == true ? 1 : 0);
}

void EverQuestMod::SendDungeonModeStateToPlayer(Player* player, bool showChatMessage)
{
    if (player == nullptr || player->GetSession() == nullptr)
        return;

    bool isInstanced = GetDungeonModeInstancedForPlayer(player);
    if (showChatMessage == true)
    {
        if (isInstanced == true)
            ChatHandler(player->GetSession()).PSendSysMessage("EQ Dungeon Mode is set to |cff4CFF00Instanced|r, so entering an EverQuest dungeon makes a private copy for you and your group.");
        else
            ChatHandler(player->GetSession()).PSendSysMessage("EQ Dungeon Mode is set to |cff4CFF00Shared World|r, so EverQuest dungeons are the shared world versions everyone else is in.");
    }

    std::string addonMessage = "EQDUNGEONMODE\t" + std::string(isInstanced == true ? "1" : "0");
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_ADDON, nullptr, nullptr, addonMessage);
    player->GetSession()->SendPacket(&data);
}

bool EverQuestMod::GetHailWindowOnRightClickForPlayer(Player* player)
{
    return GetOrLoadActivePlayerClassControllerData(player)->HailWindowOnRightClick;
}

void EverQuestMod::SetHailWindowOnRightClickForPlayer(Player* player, bool hailWindowOnRightClick)
{
    GetOrLoadActivePlayerClassControllerData(player)->HailWindowOnRightClick = hailWindowOnRightClick;
    SaveHailWindowOnRightClickForPlayer(player);
}

void EverQuestMod::SaveHailWindowOnRightClickForPlayer(Player* player)
{
    EverQuestPlayerControllerData controllerData;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto controllerDataIt = ActivePlayerClassControllerDataByGUID.find(player->GetGUID());
        if (controllerDataIt == ActivePlayerClassControllerDataByGUID.end())
            return;
        controllerData = controllerDataIt->second;
    }

    CharacterDatabase.Execute("INSERT INTO `mod_everquest_character_settings` (`guid`, `currentSecondaryClass`, `nextSecondaryClass`, `secondaryExpPool`, `hailWindowOnRightClick`) VALUES ({}, {}, {}, {}, {}) ON DUPLICATE KEY UPDATE `hailWindowOnRightClick` = {}",
        player->GetGUID().GetCounter(),
        controllerData.CurrentSecondClass,
        controllerData.NextSecondClass,
        controllerData.SecondaryExpPool,
        controllerData.HailWindowOnRightClick == true ? 1 : 0,
        controllerData.HailWindowOnRightClick == true ? 1 : 0);
}

bool EverQuestMod::TryGetHailWindowOnRightClickForPlayer(Player* player, bool& hailWindowOnRightClick)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto controllerDataIt = ActivePlayerClassControllerDataByGUID.find(player->GetGUID());
    if (controllerDataIt == ActivePlayerClassControllerDataByGUID.end())
        return false;
    hailWindowOnRightClick = controllerDataIt->second.HailWindowOnRightClick;
    return true;
}

bool EverQuestMod::GetShowDispelMessageForPlayer(Player* player)
{
    return GetOrLoadActivePlayerClassControllerData(player)->ShowDispelMessage;
}

void EverQuestMod::SetShowDispelMessageForPlayer(Player* player, bool showDispelMessage)
{
    GetOrLoadActivePlayerClassControllerData(player)->ShowDispelMessage = showDispelMessage;
    SaveShowDispelMessageForPlayer(player);
}

void EverQuestMod::SaveShowDispelMessageForPlayer(Player* player)
{
    EverQuestPlayerControllerData controllerData;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto controllerDataIt = ActivePlayerClassControllerDataByGUID.find(player->GetGUID());
        if (controllerDataIt == ActivePlayerClassControllerDataByGUID.end())
            return;
        controllerData = controllerDataIt->second;
    }

    CharacterDatabase.Execute("INSERT INTO `mod_everquest_character_settings` (`guid`, `currentSecondaryClass`, `nextSecondaryClass`, `secondaryExpPool`, `showDispelMessage`) VALUES ({}, {}, {}, {}, {}) ON DUPLICATE KEY UPDATE `showDispelMessage` = {}",
        player->GetGUID().GetCounter(),
        controllerData.CurrentSecondClass,
        controllerData.NextSecondClass,
        controllerData.SecondaryExpPool,
        controllerData.ShowDispelMessage == true ? 1 : 0,
        controllerData.ShowDispelMessage == true ? 1 : 0);
}

uint32 EverQuestMod::GetDispelMessageColorForPlayer(Player* player)
{
    return GetOrLoadActivePlayerClassControllerData(player)->DispelMessageColor;
}

void EverQuestMod::SetDispelMessageColorForPlayer(Player* player, uint32 dispelMessageColor)
{
    GetOrLoadActivePlayerClassControllerData(player)->DispelMessageColor = dispelMessageColor & 0xFFFFFF;
    SaveDispelMessageColorForPlayer(player);
}

void EverQuestMod::SaveDispelMessageColorForPlayer(Player* player)
{
    EverQuestPlayerControllerData controllerData;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto controllerDataIt = ActivePlayerClassControllerDataByGUID.find(player->GetGUID());
        if (controllerDataIt == ActivePlayerClassControllerDataByGUID.end())
            return;
        controllerData = controllerDataIt->second;
    }

    CharacterDatabase.Execute("INSERT INTO `mod_everquest_character_settings` (`guid`, `currentSecondaryClass`, `nextSecondaryClass`, `secondaryExpPool`, `dispelMessageColor`) VALUES ({}, {}, {}, {}, {}) ON DUPLICATE KEY UPDATE `dispelMessageColor` = {}",
        player->GetGUID().GetCounter(),
        controllerData.CurrentSecondClass,
        controllerData.NextSecondClass,
        controllerData.SecondaryExpPool,
        controllerData.DispelMessageColor,
        controllerData.DispelMessageColor);
}

bool EverQuestMod::TryGetDispelMessageSettingsForPlayer(Player* player, bool& showDispelMessage, uint32& dispelMessageColor)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto controllerDataIt = ActivePlayerClassControllerDataByGUID.find(player->GetGUID());
    if (controllerDataIt == ActivePlayerClassControllerDataByGUID.end())
        return false;
    showDispelMessage = controllerDataIt->second.ShowDispelMessage;
    dispelMessageColor = controllerDataIt->second.DispelMessageColor;
    return true;
}

void EverQuestMod::NotifyPlayerOfDispelledAura(Player* player, AuraApplication* auraApplication)
{
    if (player == nullptr || player->GetSession() == nullptr || auraApplication == nullptr)
        return;

    Aura* aura = auraApplication->GetBase();
    if (aura == nullptr)
        return;
    const SpellInfo* spellInfo = aura->GetSpellInfo();
    if (spellInfo == nullptr)
        return;

    // An absorb shield that soaked its last point is removed with the same reason code as a dispel, so skip a spent shield
    if (IsAuraASpentAbsorb(aura) == true)
        return;

    bool showDispelMessage = false;
    uint32 dispelMessageColor = EQ_DISPEL_MESSAGE_DEFAULT_COLOR;
    if (TryGetDispelMessageSettingsForPlayer(player, showDispelMessage, dispelMessageColor) == false)
        return;
    if (showDispelMessage == false)
        return;

    uint8 localeIndex = static_cast<uint8>(player->GetSession()->GetSessionDbcLocale());
    const char* spellName = spellInfo->SpellName[localeIndex];
    if (spellName == nullptr || spellName[0] == '\0')
        spellName = spellInfo->SpellName[0];
    if (spellName == nullptr || spellName[0] == '\0')
        return;

    // A buff being taken and a debuff being cleansed are both dispels, but they do not read the same way
    if (auraApplication->IsPositive() == true)
        ChatHandler(player->GetSession()).PSendSysMessage("|cff{:06X}Your {} was dispelled.|r", dispelMessageColor & 0xFFFFFF, spellName);
    else
        ChatHandler(player->GetSession()).PSendSysMessage("|cff{:06X}{} was dispelled from you.|r", dispelMessageColor & 0xFFFFFF, spellName);
}

bool EverQuestMod::IsAuraASpentAbsorb(Aura* aura)
{
    bool hasAbsorbEffect = false;
    for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
    {
        AuraEffect* auraEffect = aura->GetEffect(effectIndex);
        if (auraEffect == nullptr)
            continue;
        uint32 auraType = auraEffect->GetAuraType();
        if (auraType != SPELL_AURA_SCHOOL_ABSORB && auraType != SPELL_AURA_MANA_SHIELD && auraType != SPELL_AURA_SCHOOL_HEAL_ABSORB)
            continue;
        hasAbsorbEffect = true;
        if (auraEffect->GetAmount() > 0)
            return false;
    }
    return hasAbsorbEffect;
}

void EverQuestMod::SendPlayerOptionsToPlayer(Player* player)
{
    if (player == nullptr || player->GetSession() == nullptr)
        return;

    EverQuestPlayerControllerData controllerData;
    {
        GetOrLoadActivePlayerClassControllerData(player);
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto controllerDataIt = ActivePlayerClassControllerDataByGUID.find(player->GetGUID());
        if (controllerDataIt == ActivePlayerClassControllerDataByGUID.end())
            return;
        controllerData = controllerDataIt->second;
    }

    std::string addonMessage = fmt::format("EQOPTIONS\t{}\t{}\t{}\t{}\t{}\t{}\t{:06X}",
        controllerData.IllusionFaceID,
        IllusionMaxFaceIndex,
        controllerData.ShowBardPulse == true ? 1 : 0,
        controllerData.HideWoWGear == true ? 1 : 0,
        controllerData.HailWindowOnRightClick == true ? 1 : 0,
        controllerData.ShowDispelMessage == true ? 1 : 0,
        controllerData.DispelMessageColor & 0xFFFFFF);
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_ADDON, nullptr, nullptr, addonMessage);
    player->GetSession()->SendPacket(&data);
}

bool EverQuestMod::IsCreatureGossipOnlyFromHailText(uint32 creatureTemplateID)
{
    unordered_map<uint32, EverQuestCreature>::const_iterator creatureIterator = CreaturesByTemplateID.find(creatureTemplateID);
    if (creatureIterator == CreaturesByTemplateID.end())
        return false;
    return creatureIterator->second.GossipIsOnlyFromHailText;
}

void EverQuestMod::ResendNpcFlagsOfNearbyCreaturesToPlayer(Player* player)
{
    // Forcing the field dirty rebroadcasts it to every viewer of each nearby creature
    Map* map = player->FindMap();
    if (map == nullptr)
        return;
    float visibilityRange = map->GetVisibilityRange();
    std::list<Creature*> nearbyCreatures;
    Acore::AnyUnitInObjectRangeCheck check(player, visibilityRange);
    Acore::CreatureListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(player, nearbyCreatures, check);
    Cell::VisitObjects(player, searcher, visibilityRange);
    for (Creature* nearbyCreature : nearbyCreatures)
    {
        if (nearbyCreature == nullptr)
            continue;
        if (IsCreatureGossipOnlyFromHailText(nearbyCreature->GetEntry()) == false)
            continue;
        nearbyCreature->ForceValuesUpdateAtIndex(UNIT_NPC_FLAGS);
    }
}

void EverQuestMod::ResendVisibleGearOfNearbyPlayersToPlayer(Player* player)
{
    // Forcing the fields dirty rebroadcasts them to every viewer of each nearby player
    Map* map = player->FindMap();
    if (map == nullptr)
        return;
    Map::PlayerList const& mapPlayers = map->GetPlayers();
    for (Map::PlayerList::const_iterator playerIter = mapPlayers.begin(); playerIter != mapPlayers.end(); ++playerIter)
    {
        Player* mapPlayer = playerIter->GetSource();
        if (mapPlayer == nullptr || mapPlayer == player || mapPlayer->IsInWorld() == false)
            continue;
        if (player->IsWithinDistInMap(mapPlayer, map->GetVisibilityRange()) == false)
            continue;
        for (uint8 equipSlot = 0; equipSlot < EQUIPMENT_SLOT_END; ++equipSlot)
        {
            uint16 entryFieldIndex = (uint16)(PLAYER_VISIBLE_ITEM_1_ENTRYID + (equipSlot * 2));
            mapPlayer->ForceValuesUpdateAtIndex(entryFieldIndex);
            mapPlayer->ForceValuesUpdateAtIndex(entryFieldIndex + 1);
        }
    }
}

uint32 EverQuestMod::GetIssuedIllusionItemIDForPlayer(Player* player)
{
    return GetOrLoadActivePlayerClassControllerData(player)->IssuedIllusionItemID;
}

void EverQuestMod::SetIssuedIllusionItemIDForPlayer(Player* player, uint32 itemID)
{
    GetOrLoadActivePlayerClassControllerData(player)->IssuedIllusionItemID = itemID;
    SaveIssuedIllusionItemIDForPlayer(player);
}

void EverQuestMod::SaveIssuedIllusionItemIDForPlayer(Player* player)
{
    EverQuestPlayerControllerData controllerData;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto controllerDataIt = ActivePlayerClassControllerDataByGUID.find(player->GetGUID());
        if (controllerDataIt == ActivePlayerClassControllerDataByGUID.end())
            return;
        controllerData = controllerDataIt->second;
    }

    CharacterDatabase.Execute("INSERT INTO `mod_everquest_character_settings` (`guid`, `currentSecondaryClass`, `nextSecondaryClass`, `secondaryExpPool`, `issuedIllusionItemId`) VALUES ({}, {}, {}, {}, {}) ON DUPLICATE KEY UPDATE `issuedIllusionItemId` = {}",
        player->GetGUID().GetCounter(),
        controllerData.CurrentSecondClass,
        controllerData.NextSecondClass,
        controllerData.SecondaryExpPool,
        controllerData.IssuedIllusionItemID,
        controllerData.IssuedIllusionItemID);
}

static uint64 GetCumulativeExperienceForLevelAndProgress(uint8 level, uint32 experienceIntoLevel)
{
    uint64 cumulativeExperience = experienceIntoLevel;
    for (uint8 curLevel = 1; curLevel < level; ++curLevel)
        cumulativeExperience += sObjectMgr->GetXPForLevel(curLevel);
    return cumulativeExperience;
}

bool EverQuestMod::WasLastDeathPlayerVersusPlayerForPlayer(Player* player)
{
    Corpse* corpse = player->GetCorpse();
    if (corpse == nullptr)
        return false;
    return corpse->GetType() == CORPSE_RESURRECTABLE_PVP;
}

void EverQuestMod::ApplyExpLossForSpiritReleaseForPlayer(Player* player)
{
    if (ConfigExpLossOnDeathEnabled == false)
        return;

    // Do nothing if the level is below the minimum
    uint8 playerLevel = player->GetLevel();
    if (playerLevel < ConfigExpLossOnDeathMinLevel)
        return;

    // Also do nothing if this isn't a Norrath map and configured to skip
    if (player->GetMap()->GetId() < ConfigSystemMapDBCIDMin || player->GetMap()->GetId() > ConfigSystemMapDBCIDMax)
        return;

    // Falling to another player never costs experience, whether that was a duel or any other kind of player kill
    if (WasLastDeathPlayerVersusPlayerForPlayer(player) == true)
        return;

    // Where the character stood before any of this, so what the loss really came to can be measured against it
    uint64 cumulativeExperienceBeforeLoss = GetCumulativeExperienceForLevelAndProgress(playerLevel, player->GetUInt32Value(PLAYER_XP));

    // Determine the XP span of the current level
    uint32 levelXPSpan = player->GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
    if (levelXPSpan == 0 && playerLevel > 1)
        levelXPSpan = sObjectMgr->GetXPForLevel(playerLevel - 1);

    // Calculate how much experience to lose
    int expToLose = (int)((float)levelXPSpan * (0.01 * ConfigExpLossOnDeathLossPercent));

    int curLevelEXP = player->GetUInt32Value(PLAYER_XP);
    int expLost = expToLose;

    if (playerLevel >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
    {
        uint8 newLevel = playerLevel - 1;
        uint32 belowLevelSpan = sObjectMgr->GetXPForLevel(newLevel);
        int newExperience = (belowLevelSpan > 1 ? (int)belowLevelSpan - 1 : 0) - expToLose;
        if (newExperience < 0)
            newExperience = 0;
        player->SetLevel(newLevel, true);
        player->SetUInt32Value(PLAYER_NEXT_LEVEL_XP, belowLevelSpan);
        player->SetUInt32Value(PLAYER_XP, (uint32)newExperience);
        ChatHandler(player->GetSession()).PSendSysMessage("You lost|cffFF0000 {} |rexperience for releasing your spirit, which dropped your level to |cffFF0000{}|r!", expToLose, newLevel);
    }
    else if (curLevelEXP > expToLose)
    {
        // Reduce experience within the current level
        player->SetUInt32Value(PLAYER_XP, curLevelEXP - expToLose);
        ChatHandler(player->GetSession()).PSendSysMessage("You lost|cffFF0000 {} |rexperience for releasing your spirit!", expToLose);
    }
    else
    {
        // Underflow, so drop level if above level 1
        if (playerLevel == 1)
        {
            player->SetUInt32Value(PLAYER_XP, 0);
            ChatHandler(player->GetSession()).PSendSysMessage("You lost what little experience you had for releasing your spirit!");
        }
        else
        {
            player->SetLevel(playerLevel - 1, true);
            int newExperience = (int)player->GetUInt32Value(PLAYER_NEXT_LEVEL_XP) - (expToLose - curLevelEXP);
            if (newExperience < 0)
                newExperience = 0;
            player->SetUInt32Value(PLAYER_XP, (uint32)newExperience);
            ChatHandler(player->GetSession()).PSendSysMessage("You lost|cffFF0000 {} |rexperience for releasing your spirit, which dropped your level to |cffFF0000{}|r!", expToLose, playerLevel - 1);
        }
    }

    // If set, give it back as rest exp
    uint32 restExperienceGranted = 0;
    if (ConfigExpLossOnDeathAddLostExpToRestExp == true)
    {
        float restBonusBeforeGrant = player->GetRestBonus();
        player->SetRestBonus(restBonusBeforeGrant + expLost);
        float restBonusGranted = player->GetRestBonus() - restBonusBeforeGrant;
        if (restBonusGranted > 0.0f)
            restExperienceGranted = (uint32)restBonusGranted;
    }

    uint64 cumulativeExperienceAfterLoss = GetCumulativeExperienceForLevelAndProgress(player->GetLevel(), player->GetUInt32Value(PLAYER_XP));
    if (cumulativeExperienceAfterLoss >= cumulativeExperienceBeforeLoss)
        return;
    uint32 actualExperienceLost = (uint32)(cumulativeExperienceBeforeLoss - cumulativeExperienceAfterLoss);

    EverQuestPlayerControllerData& controllerData = *GetOrLoadActivePlayerClassControllerData(player);

    // Only the most recent death is ever restorable
    controllerData.DeathExpLost = actualExperienceLost;
    controllerData.DeathExpRestGranted = restExperienceGranted;
    controllerData.DeathExpLostSecondaryClass = controllerData.CurrentSecondClass;
    SaveDeathExpLossForPlayer(player);
}

void EverQuestMod::RestoreDeathExpLossOnResurrectForPlayer(Player* player)
{
    if (ConfigExpLossOnDeathEnabled == false)
        return;

    EverQuestPlayerControllerData& controllerData = *GetOrLoadActivePlayerClassControllerData(player);
    if (controllerData.DeathExpLost == 0)
        return;
    if (controllerData.DeathExpLostSecondaryClass != controllerData.CurrentSecondClass)
    {
        ClearDeathExpLossForPlayer(player);
        return;
    }

    // The debt is settled by this resurrection whether or not any of it converts into experience below
    uint32 owedExperience = controllerData.DeathExpLost;
    uint32 grantedRestExperience = controllerData.DeathExpRestGranted;
    ClearDeathExpLossForPlayer(player);

    if (ConfigExpLossOnDeathResurrectRestorePercent <= 0.0f)
        return;
    float restoreFraction = ConfigExpLossOnDeathResurrectRestorePercent * 0.01f;
    if (restoreFraction > 1.0f)
        restoreFraction = 1.0f;

    uint32 experienceToRestore = (uint32)((float)owedExperience * restoreFraction);
    if (experienceToRestore == 0)
        return;

    // The rest experience handed out at release was a stand-in for the loss, so the restored share of it comes back off
    uint32 restExperienceToRemove = (uint32)((float)grantedRestExperience * restoreFraction);
    if (restExperienceToRemove > 0)
    {
        float newRestBonus = player->GetRestBonus() - (float)restExperienceToRemove;
        player->SetRestBonus(newRestBonus < 0.0f ? 0.0f : newRestBonus);
    }

    // The core stops all experience at MaxPlayerLevel
    uint8 coreMaxLevel = (uint8)sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);
    uint8 parkedCapLevel = 0;
    if (ConfigPlayerLevelCap > 1 && (ConfigPlayerLevelCap - 1) < (uint32)coreMaxLevel)
        parkedCapLevel = (uint8)(ConfigPlayerLevelCap - 1);

    uint8 startingLevel = player->GetLevel();
    uint8 newLevel = startingLevel;
    uint32 newExperience = player->GetUInt32Value(PLAYER_XP);
    uint32 remainingExperience = experienceToRestore;
    while (remainingExperience > 0 && newLevel < coreMaxLevel)
    {
        uint32 levelXPSpan = sObjectMgr->GetXPForLevel(newLevel);
        if (levelXPSpan == 0 || newExperience >= levelXPSpan)
            break;

        uint32 experienceToNextLevel = levelXPSpan - newExperience;
        if (remainingExperience < experienceToNextLevel)
        {
            newExperience += remainingExperience;
            remainingExperience = 0;
            break;
        }

        // Sitting on the mod's level cap, so the bar refills to one point short of leveling and the rest is dropped
        if (parkedCapLevel != 0 && newLevel >= parkedCapLevel)
        {
            newExperience = levelXPSpan - 1;
            remainingExperience = 0;
            break;
        }

        remainingExperience -= experienceToNextLevel;
        newExperience = 0;
        newLevel++;
    }

    // The core max level has no experience bar of its own
    if (newLevel >= coreMaxLevel)
        newExperience = 0;

    if (newLevel != startingLevel)
    {
        player->SetLevel(newLevel, true);
        player->SetUInt32Value(PLAYER_NEXT_LEVEL_XP, sObjectMgr->GetXPForLevel(newLevel));
    }
    player->SetUInt32Value(PLAYER_XP, newExperience);

    if (player->GetSession() == nullptr)
        return;
    if (newLevel != startingLevel)
        ChatHandler(player->GetSession()).PSendSysMessage("The resurrection restored|cff00FF00 {} |rexperience, returning you to level |cff00FF00{}|r!", experienceToRestore, newLevel);
    else
        ChatHandler(player->GetSession()).PSendSysMessage("The resurrection restored|cff00FF00 {} |rexperience!", experienceToRestore);
}

void EverQuestMod::ClearDeathExpLossForPlayer(Player* player)
{
    EverQuestPlayerControllerData& controllerData = *GetOrLoadActivePlayerClassControllerData(player);
    if (controllerData.DeathExpLost == 0 && controllerData.DeathExpRestGranted == 0 && controllerData.DeathExpLostSecondaryClass == 0)
        return;

    controllerData.DeathExpLost = 0;
    controllerData.DeathExpRestGranted = 0;
    controllerData.DeathExpLostSecondaryClass = 0;
    SaveDeathExpLossForPlayer(player);
}

void EverQuestMod::SaveDeathExpLossForPlayer(Player* player)
{
    EverQuestPlayerControllerData controllerData;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        auto controllerDataIt = ActivePlayerClassControllerDataByGUID.find(player->GetGUID());
        if (controllerDataIt == ActivePlayerClassControllerDataByGUID.end())
            return;
        controllerData = controllerDataIt->second;
    }

    CharacterDatabase.Execute("INSERT INTO `mod_everquest_character_settings` (`guid`, `currentSecondaryClass`, `nextSecondaryClass`, `secondaryExpPool`, `deathExpLost`, `deathExpRestGranted`, `deathExpLostClass`) VALUES ({}, {}, {}, {}, {}, {}, {}) ON DUPLICATE KEY UPDATE `deathExpLost` = {}, `deathExpRestGranted` = {}, `deathExpLostClass` = {}",
        player->GetGUID().GetCounter(),
        controllerData.CurrentSecondClass,
        controllerData.NextSecondClass,
        controllerData.SecondaryExpPool,
        controllerData.DeathExpLost,
        controllerData.DeathExpRestGranted,
        controllerData.DeathExpLostSecondaryClass,
        controllerData.DeathExpLost,
        controllerData.DeathExpRestGranted,
        controllerData.DeathExpLostSecondaryClass);
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
    uint32 resetTalentsCost = 0;
    uint32 resetTalentsTime = 0;
    QueryResult queryResult = CharacterDatabase.Query("SELECT resettalents_cost, resettalents_time FROM characters WHERE guid = {}", player->GetGUID().GetCounter());
    if (!queryResult)
        LOG_ERROR("module.EverQuest", "EverQuestMod Error pulling character data for guid {}, so the stored talent reset cost and timer for eqclass {} fall back to zero", player->GetGUID().GetCounter(), curEQClass);
    else
    {
        Field* fields = queryResult->Fetch();
        resetTalentsCost = fields[0].Get<uint32>();
        resetTalentsTime = fields[1].Get<uint32>();
    }

    auto finiteAlways = [](float f) { return std::isfinite(f) ? f : 0.0f; };
    transaction->Append("DELETE FROM `mod_everquest_characters` WHERE guid = {} and eqclass = {}", player->GetGUID().GetCounter(), curEQClass);
    transaction->Append("INSERT IGNORE INTO mod_everquest_characters (guid, class, eqclass, `level`, xp, leveltime, rest_bonus, resettalents_cost, resettalents_time, health, power1, power2, power3, power4, power5, power6, power7, talentGroupsCount, activeTalentGroup) VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {})",
        player->GetGUID().GetCounter(),
        player->getClass(),
        curEQClass,
        player->GetLevel(),
        player->GetUInt32Value(PLAYER_XP),
        player->GetLevelPlayedTime(),           // leveltime
        finiteAlways(player->GetRestBonus()),   // rest_bonus
        resetTalentsCost,
        resetTalentsTime,
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

void EverQuestMod::MoveTalentsToModTalentsTable(Player* player, CharacterDatabaseTransaction& transaction)
{
    uint8 curEQClass = GetCurrentSecondEQClassForPlayer(player);

    transaction->Append("DELETE FROM `mod_everquest_character_class_talent` WHERE guid = {} and eqclass = {}", player->GetGUID().GetCounter(), curEQClass);
    transaction->Append("INSERT IGNORE INTO mod_everquest_character_class_talent (guid, class, eqclass, spell, specMask) SELECT guid, {}, {}, spell, specMask FROM character_talent WHERE guid = {}", player->getClass(), curEQClass, player->GetGUID().GetCounter());
    transaction->Append("DELETE FROM `character_talent` WHERE guid = {}", player->GetGUID().GetCounter());
}

bool EverQuestMod::IsRacialSkillID(uint32 skillID)
{
    // Racial skill lines hold each race's innate abilities, which belong to the character rather than any secondary class
    switch (skillID)
    {
    case EQ_RACIAL_SKILL_ID_DWARF:
    case EQ_RACIAL_SKILL_ID_TAUREN:
    case EQ_RACIAL_SKILL_ID_ORC:
    case EQ_RACIAL_SKILL_ID_NIGHTELF:
    case EQ_RACIAL_SKILL_ID_UNDEAD:
    case EQ_RACIAL_SKILL_ID_TROLL:
    case EQ_RACIAL_SKILL_ID_GNOME:
    case EQ_RACIAL_SKILL_ID_HUMAN:
    case EQ_RACIAL_SKILL_ID_BLOODELF:
    case EQ_RACIAL_SKILL_ID_DRAENEI:
        return true;
    default:
        return false;
    }
}

void EverQuestMod::EnsureCrossClassExemptSpellIDsBuilt()
{
    std::lock_guard<std::mutex> lock(CrossClassExemptSpellIDsMutex);
    if (CrossClassExemptSpellIDsBuilt == true)
        return;

    // Cache every spell that is tied to a cross-class skill, a racial skill line, or a death knight skill line, so that they don't wipe on secondary class switch
    unordered_set<uint32> deathKnightAutoGrantedSpellIDs;
    for (SkillLineAbilityEntry const* skillLineAbility : sSkillLineAbilityStore)
    {
        if (skillLineAbility == nullptr)
            continue;
        if (ConfigCrossClassIncludeSkillIDs.find(skillLineAbility->SkillLine) != ConfigCrossClassIncludeSkillIDs.end())
            CrossClassExemptSpellIDs.insert(skillLineAbility->Spell);
        if (IsRacialSkillID(skillLineAbility->SkillLine) == true)
            RacialSpellIDs.insert(skillLineAbility->Spell);
        if (IsDeathKnightSkillID(skillLineAbility->SkillLine) == true)
        {
            DeathKnightSpellIDs.insert(skillLineAbility->Spell);
            if (skillLineAbility->AcquireMethod != 0)
                deathKnightAutoGrantedSpellIDs.insert(skillLineAbility->Spell);
        }
    }

    BuildDeathKnightSpellMinLevels(deathKnightAutoGrantedSpellIDs);
    CrossClassExemptSpellIDsBuilt = true;
}

void EverQuestMod::BuildDeathKnightSpellMinLevels(const unordered_set<uint32>& autoGrantedSpellIDs)
{
    DeathKnightSpellMinLevelBySpellID.clear();

    // Pull the trainer ladder levels, taking the lowest when a spell is sold by more than one trainer
    unordered_map<uint32, uint32> trainerMinLevelBySpellID;
    QueryResult trainerSpellQueryResult = WorldDatabase.Query("SELECT SpellId, ReqLevel FROM trainer_spell");
    if (!trainerSpellQueryResult)
        LOG_ERROR("module.EverQuest", "EverQuestMod could not read trainer_spell, so death knight ability levels fall back to what Spell.dbc reports");
    else
    {
        do
        {
            Field* fields = trainerSpellQueryResult->Fetch();
            uint32 trainerSpellID = fields[0].Get<uint32>();
            uint32 trainerReqLevel = fields[1].Get<uint8>();
            if (DeathKnightSpellIDs.find(trainerSpellID) == DeathKnightSpellIDs.end())
                continue;
            auto trainerMinLevelItr = trainerMinLevelBySpellID.find(trainerSpellID);
            if (trainerMinLevelItr == trainerMinLevelBySpellID.end() || trainerReqLevel < trainerMinLevelItr->second)
                trainerMinLevelBySpellID[trainerSpellID] = trainerReqLevel;
        } while (trainerSpellQueryResult->NextRow());
    }

    for (uint32 deathKnightSpellID : DeathKnightSpellIDs)
    {
        // A level of zero means the ability always follows the character
        if (autoGrantedSpellIDs.find(deathKnightSpellID) != autoGrantedSpellIDs.end())
        {
            DeathKnightSpellMinLevelBySpellID[deathKnightSpellID] = 0;
            continue;
        }

        // A talent ability is paid for with talent points, and talent points already reset on a class switch, so it can never be allowed to outlive them no matter what level the destination class is
        if (GetTalentSpellCost(deathKnightSpellID) > 0)
        {
            DeathKnightSpellMinLevelBySpellID[deathKnightSpellID] = std::numeric_limits<uint8>::max();
            continue;
        }

        uint32 minimumLevel = 0;
        auto trainerMinLevelItr = trainerMinLevelBySpellID.find(deathKnightSpellID);
        if (trainerMinLevelItr != trainerMinLevelBySpellID.end())
            minimumLevel = trainerMinLevelItr->second;

        // Spell.dbc is only a floor under the trainer level, since the low level death knight rebalance deliberately zeroes BaseLevel and SpellLevel on every ability it rescales
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(deathKnightSpellID);
        if (spellInfo != nullptr)
        {
            if (spellInfo->BaseLevel > minimumLevel)
                minimumLevel = spellInfo->BaseLevel;
            if (spellInfo->SpellLevel > minimumLevel)
                minimumLevel = spellInfo->SpellLevel;
        }

        if (minimumLevel > (uint32)std::numeric_limits<uint8>::max())
            minimumLevel = std::numeric_limits<uint8>::max();
        DeathKnightSpellMinLevelBySpellID[deathKnightSpellID] = (uint8)minimumLevel;
    }
}

bool EverQuestMod::IsDeathKnightSkillID(uint32 skillID)
{
    // These skill lines carry the death knight class kit, which belongs to the fixed WoW class rather than any secondary EQ class
    switch (skillID)
    {
    case EQ_DEATHKNIGHT_SKILL_ID_BLOOD:
    case EQ_DEATHKNIGHT_SKILL_ID_FROST:
    case EQ_DEATHKNIGHT_SKILL_ID_UNHOLY:
    case EQ_DEATHKNIGHT_RUNEFORGING_SKILL_ID:
        return true;
    default:
        return false;
    }
}

bool EverQuestMod::IsSpellExemptFromClassMove(uint32 spellID, uint8 nextClassLevel)
{
    // Death Knight abilities belong to the fixed WoW class rather than the active EQ secondary class, so they persist across switches
    if (spellID == EQ_DEATHKNIGHT_DEATHGATE_SPELL_ID || spellID == EQ_DEATHKNIGHT_RUNEFORGING_SPELL_ID)
        return true;
    auto deathKnightSpellMinLevelItr = DeathKnightSpellMinLevelBySpellID.find(spellID);
    if (deathKnightSpellMinLevelItr != DeathKnightSpellMinLevelBySpellID.end())
        return deathKnightSpellMinLevelItr->second <= nextClassLevel;

    // Racial abilities are a property of the character's race, so they follow the character and not the active secondary class
    if (RacialSpellIDs.find(spellID) != RacialSpellIDs.end())
        return true;

    // Spells flagged by the converter as character-wide (like the racial guise spells) persist across switches
    auto spellDataItr = SpellDataBySpellID.find(spellID);
    if (spellDataItr != SpellDataBySpellID.end() && spellDataItr->second.PersistOnClassChange == true)
        return true;

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

void EverQuestMod::MoveClassSpellsToModSpellsTable(Player* player, CharacterDatabaseTransaction& transaction, uint8 nextClassLevel)
{
    uint8 curEQClass = GetCurrentSecondEQClassForPlayer(player);

    // Build (once) the set of profession/tradeskill-bound spells that should not migrate
    EnsureCrossClassExemptSpellIDsBuilt();

    // Purge old spell list in mod table
    transaction->Append("DELETE FROM `mod_everquest_character_class_spell` WHERE guid = {} and eqclass = {}", player->GetGUID().GetCounter(), curEQClass);

    // Move class spells (including EverQuest spells) from the character table into the mod table
    std::lock_guard<std::mutex> lock(CrossClassExemptSpellIDsMutex);
    for (auto& curSpell : player->GetSpellMap())
    {
        if (IsSpellExemptFromClassMove(curSpell.first, nextClassLevel) == true)
            continue;

        // Special consideration for WoW rogues when it comes to lockpicking
        if (player->getClass() == CLASS_ROGUE && curSpell.first == 633)
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

bool EverQuestMod::IsSkillExemptFromClassMove(uint32 skillID)
{
    // The Blood/Frost/Unholy/Runeforging skill lines belong to the fixed WoW Death Knight class, so they persist across secondary switches
    if (IsDeathKnightSkillID(skillID) == true)
        return true;

    // Racial skill lines belong to the character's race, and taking them away would strip the racial abilities hung off them
    if (IsRacialSkillID(skillID) == true)
        return true;

    // Shared skills (mounts/riding, tradeskills, etc.) configured to persist across secondary classes
    if (ConfigCrossClassIncludeSkillIDs.find(skillID) != ConfigCrossClassIncludeSkillIDs.end())
        return true;

    return false;
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
        // Ignore shared skills (and class-fixed skills like Death Knight runeforging) that persist across secondary classes
        if (IsSkillExemptFromClassMove(curSkillID) == true)
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
    bool adventurerAuraAlreadyKept = false;
    for (auto const& spellPair : SpellDataBySpellID)
    {
        if (spellPair.second.AuraStaysOnSecondaryClassSwitch == true)
        {
            if (keptSpellsList.empty() == false)
                keptSpellsList += ",";
            keptSpellsList += std::to_string(spellPair.second.SpellID);
            if (spellPair.second.SpellID == ConfigSystemAdventurerAuraSpellID)
                adventurerAuraAlreadyKept = true;
        }
    }

    // Make certain that the adventurer aura is never missed in retention
    if (ConfigSystemAdventurerAuraSpellID != 0 && adventurerAuraAlreadyKept == false)
    {
        if (keptSpellsList.empty() == false)
            keptSpellsList += ",";
        keptSpellsList += std::to_string(ConfigSystemAdventurerAuraSpellID);
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
    transaction->Append("DELETE storage FROM `mod_everquest_character_class_inventory` storage INNER JOIN `character_inventory` live ON live.`item` = storage.`item` WHERE live.`guid` = {}", player->GetGUID().GetCounter());
    transaction->Append("INSERT INTO `mod_everquest_character_class_inventory` (`guid`, `class`, `eqclass`, `bag`, `slot`, `item`) SELECT `guid`, {}, {}, `bag`, `slot`, `item` FROM character_inventory WHERE guid = {} AND `bag` = 0 AND `slot` <= 18", player->getClass(), curEQClass, player->GetGUID().GetCounter());
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

bool EverQuestMod::UpdateCharacterFromModCharacterTable(Player* player, uint8 pullEQClassID, CharacterDatabaseTransaction& transaction)
{
    QueryResult queryResult = CharacterDatabase.Query("SELECT `level`, `xp`, `leveltime`, `rest_bonus`, `resettalents_cost`, `resettalents_time`, `health`, `power1`, `power2`, `power3`, `power4`, `power5`, `power6`, `power7`, `talentGroupsCount`, `activeTalentGroup` FROM mod_everquest_characters WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), pullEQClassID);
    if (!queryResult)
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod Error pulling character data for guid {} eqclass {}", player->GetGUID().GetCounter(), pullEQClassID);
        return false;
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
    return true;
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

    // A brand new secondary class starts at the configured start level, and one that has been played before comes back at the
    // level it was parked at.  Anything that carries across the switch has to be measured against that level, and not against
    // the level of the class being left behind
    uint32 startLevel = player->getClass() != CLASS_DEATH_KNIGHT
        ? sWorld->getIntConfig(CONFIG_START_PLAYER_LEVEL)
        : sWorld->getIntConfig(CONFIG_START_HEROIC_PLAYER_LEVEL);
    uint8 nextClassLevel = (uint8)startLevel;
    if (isNew == false)
    {
        map<uint8, uint8> levelsByEQClass = GetClassLevelsByClassForPlayer(player);
        auto nextClassLevelItr = levelsByEQClass.find(nextSecondaryEQClass);
        if (nextClassLevelItr != levelsByEQClass.end())
            nextClassLevel = nextClassLevelItr->second;
    }

    // Set up the transaction
    CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
    AppendCharacterRowLockAnchor(transaction, player->GetGUID().GetCounter());

    // Perform moves into the mod tables to reflect this character's class
    CopyCharacterDataIntoModCharacterTable(player, transaction);
    MoveTalentsToModTalentsTable(player, transaction);
    MoveClassSpellsToModSpellsTable(player, transaction, nextClassLevel);
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
        // For health and mana
        PlayerClassLevelInfo classInfo;
        sObjectMgr->GetPlayerClassLevelInfo(player->getClass(), startLevel, &classInfo);

        // Update the character core table to reflect the switch
        transaction->Append("UPDATE characters SET `level` = {}, `xp` = 0, `leveltime` = 0, `rest_bonus` = 0, `resettalents_cost` = 0, `resettalents_time` = 0, health = {}, power1 = {}, power2 = 0, power3 = 0, power4 = 100, power5 = 0, power6 = 0, power7 = 0, `talentGroupsCount` = 1, `activeTalentGroup` = 0 WHERE guid = {}", startLevel, classInfo.basehealth, classInfo.basemana, player->GetGUID().GetCounter());

        // Give blank action mappings
        transaction->Append("DELETE FROM `character_action` WHERE guid = {}", player->GetGUID().GetCounter());

        // Pull in any equipment staged for this class through the Secondary Class Equipment window (a class with no saved character data can still have stored equipment rows)
        transaction->Append("INSERT IGNORE INTO `character_inventory` (`guid`, `bag`, `slot`, `item`) SELECT `guid`, `bag`, `slot`, `item` FROM mod_everquest_character_class_inventory WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), nextSecondaryEQClass);

        // The rows just went live, so they have to stop being storage rows
        transaction->Append("DELETE FROM `mod_everquest_character_class_inventory` WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), nextSecondaryEQClass);
    }
    // Existing
    else
    {
        // Copy in the stored version for existing.  Nothing has been committed yet, so a failed read here can still abandon the whole transaction
        if (UpdateCharacterFromModCharacterTable(player, nextSecondaryEQClass, transaction) == false)
            return false;
        CopyModSpellTableIntoCharacterSpells(player, nextSecondaryEQClass, transaction);
        CopyModActionTableIntoCharacterAction(player, nextSecondaryEQClass, transaction);
        CopyModSkillTableIntoCharacterSkills(player, nextSecondaryEQClass, transaction);
        CopyModQuestTablesIntoCharacterQuests(player, nextSecondaryEQClass, transaction);

        transaction->Append("INSERT IGNORE INTO character_talent (guid, spell, specMask) SELECT guid, spell, specMask FROM mod_everquest_character_class_talent WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), nextSecondaryEQClass);
        transaction->Append("INSERT IGNORE INTO character_glyphs (guid, talentGroup, glyph1, glyph2, glyph3, glyph4, glyph5, glyph6) SELECT guid, talentGroup, glyph1, glyph2, glyph3, glyph4, glyph5, glyph6 FROM mod_everquest_character_class_glyphs WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), nextSecondaryEQClass);
        transaction->Append("INSERT IGNORE INTO character_aura (guid, casterGuid, itemGuid, spell, effectMask, recalculateMask, stackCount, amount0, amount1, amount2, base_amount0, base_amount1, base_amount2, maxDuration, remainTime, remainCharges) SELECT guid, casterGuid, itemGuid, spell, effectMask, recalculateMask, stackCount, amount0, amount1, amount2, base_amount0, base_amount1, base_amount2, maxDuration, remainTime, remainCharges FROM mod_everquest_character_class_aura WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), nextSecondaryEQClass);
        transaction->Append("INSERT IGNORE INTO `character_inventory` (`guid`, `bag`, `slot`, `item`) SELECT `guid`, `bag`, `slot`, `item` FROM mod_everquest_character_class_inventory WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), nextSecondaryEQClass);

        // The rows just went live, so they have to stop being storage rows
        transaction->Append("DELETE FROM `mod_everquest_character_class_inventory` WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), nextSecondaryEQClass);
    }

    // Update current class
    UpdatePlayerControllerForClassChange(player, nextSecondaryEQClass, transaction);
    GetOrLoadActivePlayerClassControllerData(player)->CurrentSecondClass = nextSecondaryEQClass;

    // A class taken on for the first time is owed its start items, but this runs at logout where nothing can reach the character's bags
    if (isNew == true && PlayerClassStartItemWOWIDsByEQClassID.find(nextSecondaryEQClass) != PlayerClassStartItemWOWIDsByEQClassID.end())
    {
        GetOrLoadActivePlayerClassControllerData(player)->PendingStartItemEQClass = nextSecondaryEQClass;
        transaction->Append("UPDATE `mod_everquest_character_settings` SET `pendingStartItemEQClass` = {} WHERE `guid` = {}", nextSecondaryEQClass, player->GetGUID().GetCounter());
    }

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
        PendingEquipmentStorageCommitMSByGUID.erase(guid);
        AgileFighterRefreshTimerMSByPlayerGUID.erase(guid);
    }
    {
        std::lock_guard<std::mutex> lock(PendingStorageTransactionMutex);
        PendingStorageTransactionCallbacksByGUID.erase(guid);
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

static bool CanInventoryTypeGoIntoEquipSlot(uint32 inventoryType, uint8 equipSlot)
{
    switch (inventoryType)
    {
    case INVTYPE_HEAD:              return equipSlot == EQUIPMENT_SLOT_HEAD;
    case INVTYPE_NECK:              return equipSlot == EQUIPMENT_SLOT_NECK;
    case INVTYPE_SHOULDERS:         return equipSlot == EQUIPMENT_SLOT_SHOULDERS;
    case INVTYPE_BODY:              return equipSlot == EQUIPMENT_SLOT_BODY;
    case INVTYPE_CHEST:             return equipSlot == EQUIPMENT_SLOT_CHEST;
    case INVTYPE_ROBE:              return equipSlot == EQUIPMENT_SLOT_CHEST;
    case INVTYPE_WAIST:             return equipSlot == EQUIPMENT_SLOT_WAIST;
    case INVTYPE_LEGS:              return equipSlot == EQUIPMENT_SLOT_LEGS;
    case INVTYPE_FEET:              return equipSlot == EQUIPMENT_SLOT_FEET;
    case INVTYPE_WRISTS:            return equipSlot == EQUIPMENT_SLOT_WRISTS;
    case INVTYPE_HANDS:             return equipSlot == EQUIPMENT_SLOT_HANDS;
    case INVTYPE_FINGER:            return equipSlot == EQUIPMENT_SLOT_FINGER1 || equipSlot == EQUIPMENT_SLOT_FINGER2;
    case INVTYPE_TRINKET:           return equipSlot == EQUIPMENT_SLOT_TRINKET1 || equipSlot == EQUIPMENT_SLOT_TRINKET2;
    case INVTYPE_CLOAK:             return equipSlot == EQUIPMENT_SLOT_BACK;
    case INVTYPE_WEAPON:            return equipSlot == EQUIPMENT_SLOT_MAINHAND || equipSlot == EQUIPMENT_SLOT_OFFHAND;
    case INVTYPE_2HWEAPON:          return equipSlot == EQUIPMENT_SLOT_MAINHAND;
    case INVTYPE_WEAPONMAINHAND:    return equipSlot == EQUIPMENT_SLOT_MAINHAND;
    case INVTYPE_WEAPONOFFHAND:     return equipSlot == EQUIPMENT_SLOT_OFFHAND;
    case INVTYPE_SHIELD:            return equipSlot == EQUIPMENT_SLOT_OFFHAND;
    case INVTYPE_HOLDABLE:          return equipSlot == EQUIPMENT_SLOT_OFFHAND;
    case INVTYPE_RANGED:            return equipSlot == EQUIPMENT_SLOT_RANGED;
    case INVTYPE_RANGEDRIGHT:       return equipSlot == EQUIPMENT_SLOT_RANGED;
    case INVTYPE_THROWN:            return equipSlot == EQUIPMENT_SLOT_RANGED;
    case INVTYPE_RELIC:             return equipSlot == EQUIPMENT_SLOT_RANGED;
    case INVTYPE_TABARD:            return equipSlot == EQUIPMENT_SLOT_TABARD;
    default:                        return false;
    }
}

static bool ConvertClientBagPositionToServer(uint8 clientBagID, uint8 clientSlotID, uint8& serverBagOut, uint8& serverSlotOut)
{
    // Backpack with 1-based slots
    if (clientBagID == 0)
    {
        if (clientSlotID < 1 || clientSlotID > (INVENTORY_SLOT_ITEM_END - INVENTORY_SLOT_ITEM_START))
            return false;
        serverBagOut = INVENTORY_SLOT_BAG_0;
        serverSlotOut = INVENTORY_SLOT_ITEM_START + (clientSlotID - 1);
        return true;
    }
    // Bags 1-4 with 1 based slots
    else if (clientBagID <= 4)
    {
        if (clientSlotID < 1 || clientSlotID > MAX_BAG_SIZE)
            return false;
        serverBagOut = INVENTORY_SLOT_BAG_START + (clientBagID - 1);
        serverSlotOut = clientSlotID - 1;
        return true;
    }
    return false;
}

bool EverQuestMod::IsEQClassValidEquipmentStorageTargetForPlayer(Player* player, uint8 eqClassID)
{
    if (eqClassID > EQ_EQCLASS_ENCHANTER)
        return false;
    if (eqClassID == GetCurrentSecondEQClassForPlayer(player))
        return false;
    if (eqClassID == EQ_EQCLASS_NONE)
        return true;
    const EverQuestClassMap classMap = GetClassMapForWOWClassID(player->getClass());
    uint32 classBit = 1u << (eqClassID - 1);
    return (classMap.EQClassIDEligibleSecondMask & classBit) != 0;
}

static const uint64 EQ_EQUIPSTORAGE_PENDING_EXPIRY_MS = 10000;

bool EverQuestMod::IsEquipmentStorageCommitPendingForPlayer(Player* player)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    auto pendingItr = PendingEquipmentStorageCommitMSByGUID.find(player->GetGUID());
    if (pendingItr == PendingEquipmentStorageCommitMSByGUID.end())
        return false;
    uint64 nowMS = uint64(GameTime::GetGameTimeMS().count());
    if (nowMS - pendingItr->second > EQ_EQUIPSTORAGE_PENDING_EXPIRY_MS)
    {
        PendingEquipmentStorageCommitMSByGUID.erase(pendingItr);
        return false;
    }
    return true;
}

void EverQuestMod::SetEquipmentStorageCommitPendingForPlayerGUID(ObjectGuid playerGUID, bool pending)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    if (pending == true)
        PendingEquipmentStorageCommitMSByGUID[playerGUID] = uint64(GameTime::GetGameTimeMS().count());
    else
        PendingEquipmentStorageCommitMSByGUID.erase(playerGUID);
}

bool EverQuestMod::IsItemEQClassAllowedForPlayerSecondaryClass(Player* player, uint8 eqClassID, uint32 itemTemplateID)
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

    // Passed secondary class
    if (eqClassID == EQ_EQCLASS_NONE)
        return false;
    uint32 secondEQClassBit = 1u << (eqClassID - 1);
    return (allowedEQClassMask & secondEQClassBit) != 0;
}

uint8 EverQuestMod::GetLevelForPlayerSecondaryEQClass(Player* player, uint8 eqClassID)
{
    // The active class carries its level on the player itself, and every other class has it parked in the mod character table
    if (eqClassID == GetCurrentSecondEQClassForPlayer(player))
        return player->GetLevel();

    QueryResult queryResult = CharacterDatabase.Query("SELECT `level` FROM mod_everquest_characters WHERE guid = {} AND eqclass = {}", player->GetGUID().GetCounter(), eqClassID);
    if (queryResult)
        return queryResult->Fetch()[0].Get<uint8>();

    // A class that has never been played starts at the configured start level, matching what PerformClassSwitch does for a new class
    uint32 startLevel = player->getClass() != CLASS_DEATH_KNIGHT
        ? sWorld->getIntConfig(CONFIG_START_PLAYER_LEVEL)
        : sWorld->getIntConfig(CONFIG_START_HEROIC_PLAYER_LEVEL);
    return uint8(startLevel);
}

bool EverQuestMod::IsItemRequiredLevelMetByPlayerSecondaryClass(Player* player, uint8 eqClassID, ItemTemplate const* itemTemplate, uint8& classLevelOut)
{
    classLevelOut = GetLevelForPlayerSecondaryEQClass(player, eqClassID);
    if (itemTemplate == nullptr)
        return true;
    return uint32(classLevelOut) >= itemTemplate->RequiredLevel;
}

static std::string GetSecondaryClassRequiredLevelErrorText(uint8 eqClassID, uint8 classLevel, uint32 requiredLevel, const std::string& itemDescription)
{
    return itemDescription + " requires level " + std::to_string(requiredLevel) + ", and your " + GetEQClassStringFromID(eqClassID) + " is level " + std::to_string(uint32(classLevel)) + ".";
}

void EverQuestMod::AppendCharacterRowLockAnchor(CharacterDatabaseTransaction& transaction, uint32 playerGUIDCounter)
{
    transaction->Append("UPDATE characters SET online = online WHERE guid = {}", playerGUIDCounter);
}

Item* EverQuestMod::LoadDetachedItemForPlayer(uint32 itemGUIDCounter, Player* player)
{
    // Never create a second live object for an item the player already holds
    if (player->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGUIDCounter)) != nullptr)
    {
        LOG_ERROR("module.EverQuest", "EverQuestMod LoadDetachedItemForPlayer refused item guid {} for player guid {} because that item is already live on the player", itemGUIDCounter, player->GetGUID().GetCounter());
        return nullptr;
    }

    QueryResult queryResult = CharacterDatabase.Query("SELECT creatorGuid, giftCreatorGuid, count, duration, charges, flags, enchantments, randomPropertyId, durability, playedTime, text, itemEntry FROM item_instance WHERE guid = {}", itemGUIDCounter);
    if (!queryResult)
        return nullptr;
    Field* fields = queryResult->Fetch();
    uint32 itemEntry = fields[11].Get<uint32>();
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemEntry);
    if (itemTemplate == nullptr)
        return nullptr;
    Item* item = NewItemOrBag(itemTemplate);
    if (item->LoadFromDB(itemGUIDCounter, player->GetGUID(), fields, itemEntry) == false)
    {
        delete item;
        return nullptr;
    }
    return item;
}

static void SendClassEquipmentAddonMessageAfterCommit(ObjectGuid playerGUID, uint8 eqClassID, bool /*commitSucceeded*/)
{
    // The commit landed (or definitively failed), so the next storage mutation may proceed
    EverQuest->SetEquipmentStorageCommitPendingForPlayerGUID(playerGUID, false);

    Player* player = ObjectAccessor::FindConnectedPlayer(playerGUID);
    if (player != nullptr)
        EverQuest->SendClassEquipmentAddonMessageToPlayer(player, eqClassID);
}

void EverQuestMod::QueuePendingEquipmentStorageTransaction(Player* player, uint8 eqClassID, CharacterDatabaseTransaction& transaction)
{
    SetEquipmentStorageCommitPendingForPlayerGUID(player->GetGUID(), true);
    TransactionCallback callback = CharacterDatabase.AsyncCommitTransaction(transaction);
    callback.AfterComplete(std::bind(&SendClassEquipmentAddonMessageAfterCommit, player->GetGUID(), eqClassID, std::placeholders::_1));

    std::lock_guard<std::mutex> lock(PendingStorageTransactionMutex);
    PendingStorageTransactionCallbacksByGUID.erase(player->GetGUID());
    PendingStorageTransactionCallbacksByGUID.emplace(player->GetGUID(), std::move(callback));
}

void EverQuestMod::ProcessPendingEquipmentStorageTransactions()
{
    std::lock_guard<std::mutex> lock(PendingStorageTransactionMutex);
    for (auto callbackItr = PendingStorageTransactionCallbacksByGUID.begin(); callbackItr != PendingStorageTransactionCallbacksByGUID.end();)
    {
        if (callbackItr->second.InvokeIfReady() == true)
            callbackItr = PendingStorageTransactionCallbacksByGUID.erase(callbackItr);
        else
            ++callbackItr;
    }
}

void EverQuestMod::WaitForPendingEquipmentStorageCommitForPlayer(ObjectGuid playerGUID)
{
    for (uint32 waitedMS = 0; waitedMS < 5000; ++waitedMS)
    {
        {
            std::lock_guard<std::mutex> lock(PendingStorageTransactionMutex);
            auto callbackItr = PendingStorageTransactionCallbacksByGUID.find(playerGUID);
            if (callbackItr == PendingStorageTransactionCallbacksByGUID.end())
                return;
            if (callbackItr->second.InvokeIfReady() == true)
            {
                PendingStorageTransactionCallbacksByGUID.erase(callbackItr);
                return;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    LOG_ERROR("module.EverQuest", "EverQuestMod Timed out waiting on a pending equipment storage commit for player guid {}", playerGUID.GetCounter());
}

bool EverQuestMod::EquipItemIntoSecondaryClassStorage(Player* player, uint8 eqClassID, uint8 clientBagID, uint8 clientSlotID, uint8 equipSlot, uint32 expectedItemTemplateID, std::string& errorTextOut)
{
    if (IsEQClassValidEquipmentStorageTargetForPlayer(player, eqClassID) == false)
    {
        errorTextOut = "That is not one of your inactive secondary EQ classes.";
        return false;
    }
    if (IsEquipmentStorageCommitPendingForPlayer(player) == true)
    {
        errorTextOut = "Your previous equipment change is still processing, try again in a moment.";
        return false;
    }
    if (equipSlot > EQUIPMENT_SLOT_TABARD)
    {
        errorTextOut = "That is not a valid equipment slot.";
        return false;
    }

    // Convert the client bag position into server terms
    uint8 serverBag;
    uint8 serverSlot;
    if (ConvertClientBagPositionToServer(clientBagID, clientSlotID, serverBag, serverSlot) == false)
    {
        errorTextOut = "Only items in your bags can be stored.";
        return false;
    }

    Item* item = player->GetItemByPos(serverBag, serverSlot);
    if (item == nullptr || item->GetEntry() != expectedItemTemplateID)
    {
        errorTextOut = "That item could not be found in your bags.";
        return false;
    }
    if (item->IsNotEmptyBag() == true)
    {
        errorTextOut = "That item cannot be stored.";
        return false;
    }
    ItemTemplate const* itemTemplate = item->GetTemplate();
    if (CanInventoryTypeGoIntoEquipSlot(itemTemplate->InventoryType, equipSlot) == false)
    {
        errorTextOut = "That item cannot go into that equipment slot.";
        return false;
    }
    if (IsItemEQClassAllowedForPlayerSecondaryClass(player, eqClassID, item->GetEntry()) == false)
    {
        errorTextOut = "That item cannot be used by a " + GetEQClassStringFromID(eqClassID) + ".";
        return false;
    }
    uint8 secondaryClassLevel = 0;
    if (IsItemRequiredLevelMetByPlayerSecondaryClass(player, eqClassID, itemTemplate, secondaryClassLevel) == false)
    {
        errorTextOut = GetSecondaryClassRequiredLevelErrorText(eqClassID, secondaryClassLevel, itemTemplate->RequiredLevel, "That item");
        return false;
    }

    uint32 playerGUIDCounter = player->GetGUID().GetCounter();

    // A stored two-hander demands an empty off hand and the reverse, since the restore-at-login path skips equip validation
    if (equipSlot == EQUIPMENT_SLOT_MAINHAND && itemTemplate->InventoryType == INVTYPE_2HWEAPON)
    {
        QueryResult offHandResult = CharacterDatabase.Query("SELECT item FROM mod_everquest_character_class_inventory WHERE guid = {} AND eqclass = {} AND bag = 0 AND slot = {}", playerGUIDCounter, eqClassID, EQUIPMENT_SLOT_OFFHAND);
        if (offHandResult)
        {
            errorTextOut = "Remove the stored off hand item before storing a two-handed weapon.";
            return false;
        }
    }
    if (equipSlot == EQUIPMENT_SLOT_OFFHAND)
    {
        QueryResult mainHandResult = CharacterDatabase.Query("SELECT II.itemEntry FROM mod_everquest_character_class_inventory CI INNER JOIN item_instance II ON II.guid = CI.item WHERE CI.guid = {} AND CI.eqclass = {} AND CI.bag = 0 AND CI.slot = {}", playerGUIDCounter, eqClassID, EQUIPMENT_SLOT_MAINHAND);
        if (mainHandResult)
        {
            ItemTemplate const* mainHandTemplate = sObjectMgr->GetItemTemplate(mainHandResult->Fetch()[0].Get<uint32>());
            if (mainHandTemplate != nullptr && mainHandTemplate->InventoryType == INVTYPE_2HWEAPON)
            {
                errorTextOut = "Remove the stored two-handed weapon before storing an off hand item.";
                return false;
            }
        }
    }

    // Pre-load any stored occupant of the target slot so the two can swap
    Item* occupantItem = nullptr;
    QueryResult occupantResult = CharacterDatabase.Query("SELECT item FROM mod_everquest_character_class_inventory WHERE guid = {} AND eqclass = {} AND bag = 0 AND slot = {}", playerGUIDCounter, eqClassID, equipSlot);
    if (occupantResult)
    {
        occupantItem = LoadDetachedItemForPlayer(occupantResult->Fetch()[0].Get<uint32>(), player);
        if (occupantItem == nullptr)
        {
            errorTextOut = "The stored item in that slot could not be loaded.";
            return false;
        }
    }

    // Pull the incoming item out of the live inventory, freeing its bag position for the displaced occupant
    uint32 itemGUIDCounter = item->GetGUID().GetCounter();
    player->MoveItemFromInventory(serverBag, serverSlot, true);

    // Make sure the displaced occupant has a home before committing to anything
    ItemPosCountVec occupantDest;
    if (occupantItem != nullptr)
    {
        InventoryResult storeResult = player->CanStoreItem(serverBag, serverSlot, occupantDest, occupantItem, false);
        if (storeResult != EQUIP_ERR_OK)
            storeResult = player->CanStoreItem(NULL_BAG, NULL_SLOT, occupantDest, occupantItem, false);
        if (storeResult != EQUIP_ERR_OK)
        {
            // Undo the removal (the source slot was just freed, so this cannot fail in practice)
            ItemPosCountVec revertDest;
            if (player->CanStoreItem(serverBag, serverSlot, revertDest, item, false) == EQUIP_ERR_OK)
            {
                item->SetState(ITEM_UNCHANGED);
                player->MoveItemToInventory(revertDest, item, true);
            }
            else
                LOG_ERROR("module.EverQuest", "EverQuestMod EquipItemIntoSecondaryClassStorage could not revert item {} for guid {} after a failed swap", itemGUIDCounter, playerGUIDCounter);
            delete occupantItem;
            errorTextOut = "You do not have enough bag space to swap that item.";
            return false;
        }
    }

    CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
    AppendCharacterRowLockAnchor(transaction, playerGUIDCounter);

    // Persist the incoming item's latest state and detach it from the live inventory rows
    item->FSetState(ITEM_NEW);
    item->SaveToDB(transaction);
    Item::DeleteFromInventoryDB(transaction, itemGUIDCounter);

    // A live item that was restored from storage at a past class switch still has its old row under the now-active class (the switch-in restore copies rows without deleting them)
    transaction->Append("DELETE FROM mod_everquest_character_class_inventory WHERE item = {}", itemGUIDCounter);
    transaction->Append("DELETE FROM mod_everquest_character_class_inventory WHERE guid = {} AND eqclass = {} AND bag = 0 AND slot = {}", playerGUIDCounter, eqClassID, equipSlot);
    transaction->Append("INSERT INTO mod_everquest_character_class_inventory (guid, class, eqclass, bag, slot, item) VALUES ({}, {}, {}, 0, {}, {})", playerGUIDCounter, player->getClass(), eqClassID, equipSlot, itemGUIDCounter);

    // Hand the displaced occupant to the player
    if (occupantItem != nullptr)
    {
        occupantItem->SetState(ITEM_UNCHANGED);
        player->MoveItemToInventory(occupantDest, occupantItem, true);
    }

    player->SaveInventoryAndGoldToDB(transaction);
    QueuePendingEquipmentStorageTransaction(player, eqClassID, transaction);

    delete item;
    return true;
}

static bool IsInventoryResultAnItemUniquenessFailure(InventoryResult inventoryResult)
{
    return inventoryResult == EQUIP_ERR_CANT_CARRY_MORE_OF_THIS || inventoryResult == EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_COUNT_EXCEEDED;
}

static const std::string EQ_EQUIPSTORAGE_UNIQUE_ITEM_ERROR_TEXT = "This item is unique and you already have one, move failed.";

bool EverQuestMod::RemoveItemFromSecondaryClassStorage(Player* player, uint8 eqClassID, uint8 equipSlot, uint8 clientBagID, uint8 clientSlotID, bool useSpecificBagPosition, std::string& errorTextOut)
{
    if (IsEQClassValidEquipmentStorageTargetForPlayer(player, eqClassID) == false)
    {
        errorTextOut = "That is not one of your inactive secondary EQ classes.";
        return false;
    }
    if (IsEquipmentStorageCommitPendingForPlayer(player) == true)
    {
        errorTextOut = "Your previous equipment change is still processing, try again in a moment.";
        return false;
    }
    if (equipSlot > EQUIPMENT_SLOT_TABARD)
    {
        errorTextOut = "That is not a valid equipment slot.";
        return false;
    }

    uint32 playerGUIDCounter = player->GetGUID().GetCounter();
    QueryResult storedResult = CharacterDatabase.Query("SELECT item FROM mod_everquest_character_class_inventory WHERE guid = {} AND eqclass = {} AND bag = 0 AND slot = {}", playerGUIDCounter, eqClassID, equipSlot);
    if (!storedResult)
    {
        errorTextOut = "There is no stored item in that slot.";
        return false;
    }
    uint32 itemGUIDCounter = storedResult->Fetch()[0].Get<uint32>();
    Item* item = LoadDetachedItemForPlayer(itemGUIDCounter, player);
    if (item == nullptr)
    {
        errorTextOut = "The stored item could not be loaded.";
        return false;
    }

    ItemPosCountVec dest;
    if (useSpecificBagPosition == true)
    {
        uint8 serverBag;
        uint8 serverSlot;
        if (ConvertClientBagPositionToServer(clientBagID, clientSlotID, serverBag, serverSlot) == false)
        {
            delete item;
            errorTextOut = "That is not a valid bag slot.";
            return false;
        }
        if (player->GetItemByPos(serverBag, serverSlot) != nullptr)
        {
            delete item;
            errorTextOut = "That bag slot is occupied.";
            return false;
        }
        InventoryResult storeResult = player->CanStoreItem(serverBag, serverSlot, dest, item, false);
        if (storeResult != EQUIP_ERR_OK)
        {
            delete item;
            errorTextOut = IsInventoryResultAnItemUniquenessFailure(storeResult) ? EQ_EQUIPSTORAGE_UNIQUE_ITEM_ERROR_TEXT : "The item cannot go in that bag slot.";
            return false;
        }
    }
    else
    {
        InventoryResult storeResult = player->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false);
        if (storeResult != EQUIP_ERR_OK)
        {
            delete item;
            errorTextOut = IsInventoryResultAnItemUniquenessFailure(storeResult) ? EQ_EQUIPSTORAGE_UNIQUE_ITEM_ERROR_TEXT : "You do not have enough bag space.";
            return false;
        }
    }

    CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
    AppendCharacterRowLockAnchor(transaction, playerGUIDCounter);
    transaction->Append("DELETE FROM mod_everquest_character_class_inventory WHERE guid = {} AND eqclass = {} AND bag = 0 AND slot = {} AND item = {}", playerGUIDCounter, eqClassID, equipSlot, itemGUIDCounter);
    item->SetState(ITEM_UNCHANGED);
    player->MoveItemToInventory(dest, item, true);
    player->SaveInventoryAndGoldToDB(transaction);

    QueuePendingEquipmentStorageTransaction(player, eqClassID, transaction);
    return true;
}

bool EverQuestMod::MoveItemWithinSecondaryClassStorage(Player* player, uint8 eqClassID, uint8 fromEquipSlot, uint8 toEquipSlot, std::string& errorTextOut)
{
    if (IsEQClassValidEquipmentStorageTargetForPlayer(player, eqClassID) == false)
    {
        errorTextOut = "That is not one of your inactive secondary EQ classes.";
        return false;
    }
    if (IsEquipmentStorageCommitPendingForPlayer(player) == true)
    {
        errorTextOut = "Your previous equipment change is still processing, try again in a moment.";
        return false;
    }
    if (fromEquipSlot > EQUIPMENT_SLOT_TABARD || toEquipSlot > EQUIPMENT_SLOT_TABARD || fromEquipSlot == toEquipSlot)
    {
        errorTextOut = "That is not a valid equipment slot.";
        return false;
    }

    // Pull the stored items in both slots (from must exist, to may be empty)
    uint32 playerGUIDCounter = player->GetGUID().GetCounter();
    uint32 fromItemGUIDCounter = 0;
    uint32 toItemGUIDCounter = 0;
    ItemTemplate const* fromItemTemplate = nullptr;
    ItemTemplate const* toItemTemplate = nullptr;
    QueryResult storedResult = CharacterDatabase.Query("SELECT CI.`slot`, CI.`item`, II.`itemEntry` FROM `mod_everquest_character_class_inventory` CI INNER JOIN `item_instance` II ON II.guid = CI.item WHERE CI.`guid` = {} AND CI.`eqclass` = {} AND CI.`bag` = 0 AND CI.`slot` IN ({}, {})", playerGUIDCounter, eqClassID, fromEquipSlot, toEquipSlot);
    if (storedResult)
    {
        do
        {
            Field* fields = storedResult->Fetch();
            uint8 slot = fields[0].Get<uint8>();
            if (slot == fromEquipSlot)
            {
                fromItemGUIDCounter = fields[1].Get<uint32>();
                fromItemTemplate = sObjectMgr->GetItemTemplate(fields[2].Get<uint32>());
            }
            else
            {
                toItemGUIDCounter = fields[1].Get<uint32>();
                toItemTemplate = sObjectMgr->GetItemTemplate(fields[2].Get<uint32>());
            }
        } while (storedResult->NextRow());
    }
    if (fromItemGUIDCounter == 0 || fromItemTemplate == nullptr)
    {
        errorTextOut = "There is no stored item in that slot.";
        return false;
    }
    if (CanInventoryTypeGoIntoEquipSlot(fromItemTemplate->InventoryType, toEquipSlot) == false)
    {
        errorTextOut = "That item cannot go into that equipment slot.";
        return false;
    }
    if (toItemGUIDCounter != 0 && (toItemTemplate == nullptr || CanInventoryTypeGoIntoEquipSlot(toItemTemplate->InventoryType, fromEquipSlot) == false))
    {
        errorTextOut = "The stored items cannot swap slots.";
        return false;
    }

    // Simulate the resulting main/off hand pair so a two-hander never ends up alongside an off hand item
    if (fromEquipSlot == EQUIPMENT_SLOT_MAINHAND || fromEquipSlot == EQUIPMENT_SLOT_OFFHAND || toEquipSlot == EQUIPMENT_SLOT_MAINHAND || toEquipSlot == EQUIPMENT_SLOT_OFFHAND)
    {
        ItemTemplate const* resultingTemplates[2] = { nullptr, nullptr }; // 0 = main hand, 1 = off hand
        QueryResult handsResult = CharacterDatabase.Query("SELECT CI.`slot`, II.`itemEntry` FROM `mod_everquest_character_class_inventory` CI INNER JOIN `item_instance` II ON II.guid = CI.item WHERE CI.`guid` = {} AND CI.`eqclass` = {} AND CI.`bag` = 0 AND CI.`slot` IN ({}, {})", playerGUIDCounter, eqClassID, EQUIPMENT_SLOT_MAINHAND, EQUIPMENT_SLOT_OFFHAND);
        if (handsResult)
        {
            do
            {
                Field* fields = handsResult->Fetch();
                resultingTemplates[fields[0].Get<uint8>() == EQUIPMENT_SLOT_MAINHAND ? 0 : 1] = sObjectMgr->GetItemTemplate(fields[1].Get<uint32>());
            } while (handsResult->NextRow());
        }
        if (fromEquipSlot == EQUIPMENT_SLOT_MAINHAND)
            resultingTemplates[0] = toItemTemplate; // The displaced item (or nothing) swaps back into the vacated slot
        else if (fromEquipSlot == EQUIPMENT_SLOT_OFFHAND)
            resultingTemplates[1] = toItemTemplate;
        if (toEquipSlot == EQUIPMENT_SLOT_MAINHAND)
            resultingTemplates[0] = fromItemTemplate;
        else if (toEquipSlot == EQUIPMENT_SLOT_OFFHAND)
            resultingTemplates[1] = fromItemTemplate;
        if (resultingTemplates[0] != nullptr && resultingTemplates[0]->InventoryType == INVTYPE_2HWEAPON && resultingTemplates[1] != nullptr)
        {
            errorTextOut = "A stored two-handed weapon cannot be paired with an off hand item.";
            return false;
        }
    }

    CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
    transaction->Append("DELETE FROM mod_everquest_character_class_inventory WHERE guid = {} AND eqclass = {} AND bag = 0 AND slot IN ({}, {})", playerGUIDCounter, eqClassID, fromEquipSlot, toEquipSlot);
    transaction->Append("INSERT INTO mod_everquest_character_class_inventory (guid, class, eqclass, bag, slot, item) VALUES ({}, {}, {}, 0, {}, {})", playerGUIDCounter, player->getClass(), eqClassID, toEquipSlot, fromItemGUIDCounter);
    if (toItemGUIDCounter != 0)
        transaction->Append("INSERT INTO mod_everquest_character_class_inventory (guid, class, eqclass, bag, slot, item) VALUES ({}, {}, {}, 0, {}, {})", playerGUIDCounter, player->getClass(), eqClassID, fromEquipSlot, toItemGUIDCounter);

    CharacterDatabase.DirectCommitTransaction(transaction);
    return true;
}

bool EverQuestMod::SwapSecondaryClassStorageItemWithLiveEquipment(Player* player, uint8 eqClassID, uint8 storageEquipSlot, uint8 liveEquipSlot, std::string& errorTextOut)
{
    if (IsEQClassValidEquipmentStorageTargetForPlayer(player, eqClassID) == false)
    {
        errorTextOut = "That is not one of your inactive secondary EQ classes.";
        return false;
    }
    if (IsEquipmentStorageCommitPendingForPlayer(player) == true)
    {
        errorTextOut = "Your previous equipment change is still processing, try again in a moment.";
        return false;
    }
    if (storageEquipSlot > EQUIPMENT_SLOT_TABARD || liveEquipSlot > EQUIPMENT_SLOT_TABARD)
    {
        errorTextOut = "That is not a valid equipment slot.";
        return false;
    }

    uint32 playerGUIDCounter = player->GetGUID().GetCounter();

    uint32 storedItemGUIDCounter = 0;
    QueryResult storedResult = CharacterDatabase.Query("SELECT item FROM mod_everquest_character_class_inventory WHERE guid = {} AND eqclass = {} AND bag = 0 AND slot = {}", playerGUIDCounter, eqClassID, storageEquipSlot);
    if (storedResult)
        storedItemGUIDCounter = storedResult->Fetch()[0].Get<uint32>();
    Item* liveItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, liveEquipSlot);
    if (storedItemGUIDCounter == 0 && liveItem == nullptr)
    {
        errorTextOut = "There is no item to move.";
        return false;
    }

    Item* storedItem = nullptr;
    if (storedItemGUIDCounter != 0)
    {
        storedItem = LoadDetachedItemForPlayer(storedItemGUIDCounter, player);
        if (storedItem == nullptr)
        {
            errorTextOut = "The stored item could not be loaded.";
            return false;
        }
    }
    if (liveItem != nullptr)
    {
        ItemTemplate const* liveItemTemplate = liveItem->GetTemplate();
        if (CanInventoryTypeGoIntoEquipSlot(liveItemTemplate->InventoryType, storageEquipSlot) == false)
        {
            delete storedItem;
            errorTextOut = "Your equipped item cannot be stored in that slot.";
            return false;
        }
        if (IsItemEQClassAllowedForPlayerSecondaryClass(player, eqClassID, liveItem->GetEntry()) == false)
        {
            delete storedItem;
            errorTextOut = "Your equipped item cannot be used by a " + GetEQClassStringFromID(eqClassID) + ".";
            return false;
        }
        uint8 secondaryClassLevel = 0;
        if (IsItemRequiredLevelMetByPlayerSecondaryClass(player, eqClassID, liveItemTemplate, secondaryClassLevel) == false)
        {
            delete storedItem;
            errorTextOut = GetSecondaryClassRequiredLevelErrorText(eqClassID, secondaryClassLevel, liveItemTemplate->RequiredLevel, "Your equipped item");
            return false;
        }
        // A two-hander entering storage main hand demands an empty stored off hand (the type check above already forces storageEquipSlot to be the main hand for a two-hander)
        if (liveItemTemplate->InventoryType == INVTYPE_2HWEAPON)
        {
            QueryResult offHandResult = CharacterDatabase.Query("SELECT item FROM mod_everquest_character_class_inventory WHERE guid = {} AND eqclass = {} AND bag = 0 AND slot = {}", playerGUIDCounter, eqClassID, EQUIPMENT_SLOT_OFFHAND);
            if (offHandResult)
            {
                delete storedItem;
                errorTextOut = "Remove the stored off hand item before storing a two-handed weapon.";
                return false;
            }
        }

        // And an off hand item cannot enter an empty storage off hand slot alongside a stored two-handed main hand (when the slot was occupied, the storage invariant already rules a 2H main hand out)
        if (storageEquipSlot == EQUIPMENT_SLOT_OFFHAND && storedItem == nullptr)
        {
            QueryResult mainHandResult = CharacterDatabase.Query("SELECT II.itemEntry FROM mod_everquest_character_class_inventory CI INNER JOIN item_instance II ON II.guid = CI.item WHERE CI.guid = {} AND CI.eqclass = {} AND CI.bag = 0 AND CI.slot = {}", playerGUIDCounter, eqClassID, EQUIPMENT_SLOT_MAINHAND);
            if (mainHandResult)
            {
                ItemTemplate const* mainHandTemplate = sObjectMgr->GetItemTemplate(mainHandResult->Fetch()[0].Get<uint32>());
                if (mainHandTemplate != nullptr && mainHandTemplate->InventoryType == INVTYPE_2HWEAPON)
                {
                    errorTextOut = "Remove the stored two-handed weapon before storing an off hand item.";
                    return false;
                }
            }
        }
    }

    uint16 equipDestination = 0;
    if (storedItem != nullptr)
    {
        InventoryResult equipResult = player->CanEquipItem(liveEquipSlot, equipDestination, storedItem, liveItem != nullptr);
        if (equipResult != EQUIP_ERR_OK)
        {
            player->SendEquipError(equipResult, storedItem, nullptr);
            delete storedItem;
            errorTextOut = "";
            return false;
        }
    }

    CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
    AppendCharacterRowLockAnchor(transaction, playerGUIDCounter);

    // Pull the displaced live item off the character and detach it into the storage rows
    uint32 liveItemGUIDCounter = 0;
    if (liveItem != nullptr)
    {
        liveItemGUIDCounter = liveItem->GetGUID().GetCounter();
        player->MoveItemFromInventory(INVENTORY_SLOT_BAG_0, liveEquipSlot, true);
        liveItem->FSetState(ITEM_NEW);
        liveItem->SaveToDB(transaction);
        Item::DeleteFromInventoryDB(transaction, liveItemGUIDCounter);
    }

    transaction->Append("DELETE FROM mod_everquest_character_class_inventory WHERE guid = {} AND eqclass = {} AND bag = 0 AND slot = {}", playerGUIDCounter, eqClassID, storageEquipSlot);
    if (liveItem != nullptr)
    {
        transaction->Append("DELETE FROM mod_everquest_character_class_inventory WHERE item = {}", liveItemGUIDCounter);
        transaction->Append("INSERT INTO mod_everquest_character_class_inventory (guid, class, eqclass, bag, slot, item) VALUES ({}, {}, {}, 0, {}, {})", playerGUIDCounter, player->getClass(), eqClassID, storageEquipSlot, liveItemGUIDCounter);
    }

    if (storedItem != nullptr)
    {
        storedItem->SetState(ITEM_UNCHANGED);
        player->EquipItem(equipDestination, storedItem, true);
    }

    player->SaveInventoryAndGoldToDB(transaction);

    QueuePendingEquipmentStorageTransaction(player, eqClassID, transaction);

    if (liveItem != nullptr)
        delete liveItem;
    return true;
}

// Sends the stored equipment of one of the player's non-active secondary classes to the client UI (the Secondary Class Equipment window) as a hidden addon message.
// Payload (after the "EQCLASSEQUIP\t" prefix the 3.3.5 client strips): H|<classId>|<className> ~S|<slot>|<itemEntry>|<randomPropertyId>|<permEnchant>   (one per stored equipment slot)
void EverQuestMod::SendClassEquipmentAddonMessageToPlayer(Player* player, uint8 eqClassID)
{
    if (player == nullptr)
        return;

    std::ostringstream payload;
    payload << "H|" << uint32(eqClassID) << "|" << GetEQClassStringFromID(eqClassID);

    QueryResult queryResult = CharacterDatabase.Query("SELECT CI.`slot`, II.`itemEntry`, II.`randomPropertyId`, II.`enchantments` FROM `mod_everquest_character_class_inventory` CI INNER JOIN `item_instance` II ON II.guid = CI.item WHERE CI.`guid` = {} AND CI.`eqclass` = {} AND CI.`bag` = 0 AND CI.`slot` <= 18 ORDER BY CI.`slot`", player->GetGUID().GetCounter(), eqClassID);
    if (queryResult)
    {
        do
        {
            Field* fields = queryResult->Fetch();
            uint8 slot = fields[0].Get<uint8>();
            uint32 itemEntry = fields[1].Get<uint32>();
            int32 randomPropertyID = fields[2].Get<int32>();
            string enchantString = fields[3].Get<string>();

            // Only the permanent enchant matters for the tooltip link
            std::vector<std::string_view> tokens = Acore::Tokenize(enchantString, ' ', false);
            uint32 permEnchant = 0;
            if (tokens.size() > PERM_ENCHANTMENT_SLOT * MAX_ENCHANTMENT_OFFSET)
                permEnchant = Acore::StringTo<uint32>(tokens[PERM_ENCHANTMENT_SLOT * MAX_ENCHANTMENT_OFFSET]).value_or(0);

            payload << "~S|" << uint32(slot) << "|" << itemEntry << "|" << randomPropertyID << "|" << permEnchant;
        } while (queryResult->NextRow());
    }

    std::string addonMessage = "EQCLASSEQUIP\t" + payload.str();
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_ADDON, nullptr, nullptr, addonMessage);
    player->SendDirectMessage(&data);
}

static const char* GetTrackingCompassDirection(float angle)
{
    static const char* directionNames[8] = { "north", "northwest", "west", "southwest", "south", "southeast", "east", "northeast" };
    const float pi = 3.14159265f;
    const float twoPi = pi * 2.0f;
    while (angle < 0)
        angle += twoPi;
    while (angle >= twoPi)
        angle -= twoPi;
    uint32 sectorIndex = uint32((angle + (pi / 8.0f)) / (pi / 4.0f)) % 8;
    return directionNames[sectorIndex];
}

static const char* GetTrackingProximityDescription(float distance, float maxTrackDistance)
{
    if (maxTrackDistance <= 0 || distance <= maxTrackDistance * 0.33f)
        return "close by";
    else if (distance <= maxTrackDistance * 0.66f)
        return "some distance away";
    else
        return "far away";
}

static bool IsCreatureTrackableForPlayer(Player* player, Creature* creature)
{
    if (creature == nullptr || creature->IsInWorld() == false || creature->IsAlive() == false)
        return false;
    if (creature->IsTrigger() == true || creature->IsTotem() == true)
        return false;
    if (player->CanSeeOrDetect(creature, true) == false)
        return false;
    return true;
}

static bool CompareTrackingEntriesByDistance(const std::pair<float, Creature*>& leftEntry, const std::pair<float, Creature*>& rightEntry)
{
    return leftEntry.first < rightEntry.first;
}

static void SendTrackingDirectionMessage(Player* player, EverQuestPlayerTrackingState* trackingState, Creature* creature, float distance)
{
    if (distance <= EQ_TRACKING_FOUND_DISTANCE)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("|cff4CFF00{}|r is very close by!", trackingState->TrackedCreatureName);
        return;
    }
    const char* direction = GetTrackingCompassDirection(player->GetAngle(creature));
    const char* proximity = GetTrackingProximityDescription(distance, trackingState->MaxTrackDistance);
    ChatHandler(player->GetSession()).PSendSysMessage("The trail of |cff4CFF00{}|r leads {}, {}.", trackingState->TrackedCreatureName, direction, proximity);
}

float EverQuestMod::GetTrackingRangeForEQClassAtLevel(uint8 eqClassID, uint8 level)
{
    float yardsPerLevel = 0;
    float maxRange = 0;
    switch (eqClassID)
    {
    case EQ_EQCLASS_RANGER: yardsPerLevel = ConfigTrackingRangerYardsPerLevel; maxRange = ConfigTrackingRangerMaxRange; break;
    case EQ_EQCLASS_DRUID:  yardsPerLevel = ConfigTrackingDruidYardsPerLevel;  maxRange = ConfigTrackingDruidMaxRange;  break;
    case EQ_EQCLASS_BARD:   yardsPerLevel = ConfigTrackingBardYardsPerLevel;   maxRange = ConfigTrackingBardMaxRange;   break;
    default:                return 0;
    }

    float range = yardsPerLevel * float(level);
    if (range > maxRange)
        range = maxRange;
    return range;
}

float EverQuestMod::GetTrackingMaxDistanceForPlayer(Player* player)
{
    if (ConfigTrackingEnabled == false || player == nullptr)
        return 0;

    uint8 level = player->GetLevel();
    float primaryRange = GetTrackingRangeForEQClassAtLevel(GetClassMapForWOWClassID(player->getClass()).EQClassIDBase, level);
    float secondaryRange = GetTrackingRangeForEQClassAtLevel(GetCurrentSecondEQClassForPlayer(player), level);
    return primaryRange > secondaryRange ? primaryRange : secondaryRange;
}

void EverQuestMod::HandleTrackingRangeChangeForPlayer(Player* player)
{
    if (ConfigTrackingEnabled == false || player == nullptr)
        return;

    float maxTrackDistance = GetTrackingMaxDistanceForPlayer(player);
    if (maxTrackDistance <= 0)
        return;

    EverQuestPlayerTrackingState* trackingState = player->CustomData.Get<EverQuestPlayerTrackingState>(EQ_PLAYER_CUSTOMDATA_TRACKING);
    if (trackingState != nullptr && trackingState->TrackedCreatureGUID.IsEmpty() == false)
        trackingState->MaxTrackDistance = maxTrackDistance;

    std::ostringstream rangePayload;
    rangePayload << "D|" << uint32(maxTrackDistance);
    SendTrackingAddonMessageToPlayer(player, rangePayload.str());
}

void EverQuestMod::SendTrackingAddonMessageToPlayer(Player* player, const std::string& payload)
{
    if (player == nullptr)
        return;

    std::string addonMessage = "EQTRACK\t" + payload;
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_ADDON, nullptr, nullptr, addonMessage);
    player->SendDirectMessage(&data);
}

// Message stream (after the "EQTRACK\t" prefix): "H|<rowCount>|<maxDistance>|<trackedGUIDRaw or empty>", then rows batched a few per message as
// "R|<guidRaw>|<level>|<distance>|<name>" joined with "~", then a final "F". Separately, "T|<guidRaw>" / "T|" is pushed when tracking starts / stops
// so the addon can mark the tracked row live, and "D|<maxRange>" is pushed when the player's track range changes (level up).  Creature GUIDs are 64 bit values, so
// the addon must keep them as strings (Lua numbers lose precision above 2^53) and echo them back in ".track start"
void EverQuestMod::SendTrackingListToPlayer(Player* player)
{
    if (player == nullptr || player->IsInWorld() == false)
        return;

    float maxTrackDistance = GetTrackingMaxDistanceForPlayer(player);
    if (maxTrackDistance <= 0)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("You have no tracking ability.");
        return;
    }

    // Throttle scans, since the addon (or a chat macro) can request them at will
    EverQuestPlayerTrackingState* trackingState = player->CustomData.GetDefault<EverQuestPlayerTrackingState>(EQ_PLAYER_CUSTOMDATA_TRACKING);
    uint64 nowMS = uint64(GameTime::GetGameTimeMS().count());
    if (trackingState->LastScanMSTime != 0 && nowMS >= trackingState->LastScanMSTime && nowMS - trackingState->LastScanMSTime < EQ_TRACKING_SCAN_MIN_INTERVAL_MS)
        return;
    trackingState->LastScanMSTime = nowMS;

    // Gather every trackable creature in range, nearest first
    std::list<Creature*> nearbyCreatures;
    Acore::AnyUnitInObjectRangeCheck check(player, maxTrackDistance);
    Acore::CreatureListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(player, nearbyCreatures, check);
    Cell::VisitObjects(player, searcher, maxTrackDistance);
    std::vector<std::pair<float, Creature*>> sortedCreatureEntries;
    for (Creature* creature : nearbyCreatures)
    {
        if (IsCreatureTrackableForPlayer(player, creature) == false)
            continue;
        sortedCreatureEntries.push_back(std::make_pair(player->GetDistance(creature), creature));
    }
    std::sort(sortedCreatureEntries.begin(), sortedCreatureEntries.end(), CompareTrackingEntriesByDistance);
    if (ConfigTrackingMaxResults > 0 && sortedCreatureEntries.size() > size_t(ConfigTrackingMaxResults))
        sortedCreatureEntries.resize(ConfigTrackingMaxResults);

    std::ostringstream headerPayload;
    headerPayload << "H|" << sortedCreatureEntries.size() << "|" << uint32(maxTrackDistance) << "|";
    if (trackingState->TrackedCreatureGUID.IsEmpty() == false)
        headerPayload << trackingState->TrackedCreatureGUID.GetRawValue();
    SendTrackingAddonMessageToPlayer(player, headerPayload.str());

    std::ostringstream rowPayload;
    int rowsInMessage = 0;
    for (const std::pair<float, Creature*>& creatureEntry : sortedCreatureEntries)
    {
        if (rowsInMessage > 0)
            rowPayload << "~";
        rowPayload << "R|" << creatureEntry.second->GetGUID().GetRawValue() << "|" << uint32(creatureEntry.second->GetLevel())
            << "|" << uint32(creatureEntry.first) << "|" << creatureEntry.second->GetName();
        rowsInMessage++;
        if (rowsInMessage >= EQ_TRACKING_ADDON_ROWS_PER_MESSAGE)
        {
            SendTrackingAddonMessageToPlayer(player, rowPayload.str());
            rowPayload.str("");
            rowPayload.clear();
            rowsInMessage = 0;
        }
    }
    if (rowsInMessage > 0)
        SendTrackingAddonMessageToPlayer(player, rowPayload.str());
    SendTrackingAddonMessageToPlayer(player, "F");
}

void EverQuestMod::StartTrackingForPlayer(Player* player, uint64 rawCreatureGUID)
{
    if (player == nullptr || player->IsInWorld() == false)
        return;

    float maxTrackDistance = GetTrackingMaxDistanceForPlayer(player);
    if (maxTrackDistance <= 0)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("You have no tracking ability.");
        return;
    }

    // The creature can be gone, dead, or out of range by the time the player picks it from the list
    ObjectGuid creatureGUID = ObjectGuid(rawCreatureGUID);
    Creature* creature = ObjectAccessor::GetCreature(*player, creatureGUID);
    if (creature == nullptr || IsCreatureTrackableForPlayer(player, creature) == false)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("You are unable to pick up that trail.");
        return;
    }
    float distance = player->GetDistance(creature);
    if (distance > maxTrackDistance * EQ_TRACKING_LOST_DISTANCE_MULTIPLIER)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("You are unable to pick up that trail.");
        return;
    }

    EverQuestPlayerTrackingState* trackingState = player->CustomData.GetDefault<EverQuestPlayerTrackingState>(EQ_PLAYER_CUSTOMDATA_TRACKING);
    trackingState->TrackedCreatureGUID = creatureGUID;
    trackingState->TrackedCreatureName = creature->GetName();
    trackingState->MaxTrackDistance = maxTrackDistance;
    trackingState->PulseTimerMS = 0;
    ChatHandler(player->GetSession()).PSendSysMessage("You begin tracking |cff4CFF00{}|r.", trackingState->TrackedCreatureName);

    // Tell the addon which creature is now tracked so it can mark the row
    std::ostringstream trackedPayload;
    trackedPayload << "T|" << creatureGUID.GetRawValue();
    SendTrackingAddonMessageToPlayer(player, trackedPayload.str());
    SendTrackingDirectionMessage(player, trackingState, creature, distance);
}

void EverQuestMod::StopTrackingForPlayer(Player* player, bool sendMessage)
{
    if (player == nullptr)
        return;

    EverQuestPlayerTrackingState* trackingState = player->CustomData.Get<EverQuestPlayerTrackingState>(EQ_PLAYER_CUSTOMDATA_TRACKING);
    if (trackingState == nullptr || trackingState->TrackedCreatureGUID.IsEmpty() == true)
    {
        if (sendMessage == true)
            ChatHandler(player->GetSession()).PSendSysMessage("You are not tracking anything.");
        return;
    }

    trackingState->TrackedCreatureGUID.Clear();
    trackingState->TrackedCreatureName.clear();
    trackingState->PulseTimerMS = 0;

    // Tell the addon nothing is tracked anymore so it can clear the row marker (covers manual stops and auto-drops)
    SendTrackingAddonMessageToPlayer(player, "T|");
    if (sendMessage == true)
        ChatHandler(player->GetSession()).PSendSysMessage("You stop tracking.");
}

void EverQuestMod::UpdatePlayerTracking(Player* player, uint32 diffInMS)
{
    if (ConfigTrackingEnabled == false || player == nullptr || player->IsInWorld() == false)
        return;

    EverQuestPlayerTrackingState* trackingState = player->CustomData.Get<EverQuestPlayerTrackingState>(EQ_PLAYER_CUSTOMDATA_TRACKING);
    if (trackingState == nullptr || trackingState->TrackedCreatureGUID.IsEmpty() == true)
        return;

    trackingState->PulseTimerMS += diffInMS;
    if (trackingState->PulseTimerMS < ConfigTrackingPulseIntervalInMS)
        return;
    trackingState->PulseTimerMS = 0;

    Creature* creature = ObjectAccessor::GetCreature(*player, trackingState->TrackedCreatureGUID);
    if (creature == nullptr || creature->IsInWorld() == false)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("You have lost the trail of |cff4CFF00{}|r.", trackingState->TrackedCreatureName);
        StopTrackingForPlayer(player, false);
        return;
    }
    if (creature->IsAlive() == false)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("The trail of |cff4CFF00{}|r ends at its corpse.", trackingState->TrackedCreatureName);
        StopTrackingForPlayer(player, false);
        return;
    }
    float distance = player->GetDistance(creature);
    if (distance > trackingState->MaxTrackDistance * EQ_TRACKING_LOST_DISTANCE_MULTIPLIER)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("The trail of |cff4CFF00{}|r has gone cold.", trackingState->TrackedCreatureName);
        StopTrackingForPlayer(player, false);
        return;
    }
    SendTrackingDirectionMessage(player, trackingState, creature, distance);
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
