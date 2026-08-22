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

// TODO: Getting too big, break up

#ifndef EVERQUEST_H
#define EVERQUEST_H

#include "Common.h"
#include "DataMap.h"
#include "DatabaseEnv.h"
#include "GameObject.h"
#include "ObjectGuid.h"
#include "CreatureData.h"
#include "Player.h"
#include "Chat.h"

#include <string>
#include <list>
#include <map>
#include <mutex>
#include <unordered_set>

using namespace std;

static uint32 ConfigMaxSkillIDCheck = 1000;         // The highest level of skill ID it will look for when doing copies

class Unit;
class Aura;
class AuraApplication;
class WorldPacket;
class ByteBuffer;
struct AreaTrigger;
struct BuildValuesCachePosPointers;

#define EQ_MOD_VERSION                              74

#define EQ_EQCLASS_NONE                             0
#define EQ_EQCLASS_WARRIOR                          1
#define EQ_EQCLASS_CLERIC                           2
#define EQ_EQCLASS_PALADIN                          3
#define EQ_EQCLASS_RANGER                           4
#define EQ_EQCLASS_SHADOWKNIGHT                     5
#define EQ_EQCLASS_DRUID                            6
#define EQ_EQCLASS_MONK                             7
#define EQ_EQCLASS_BARD                             8
#define EQ_EQCLASS_ROGUE                            9
#define EQ_EQCLASS_SHAMAN                           10
#define EQ_EQCLASS_NECROMANCER                      11
#define EQ_EQCLASS_WIZARD                           12
#define EQ_EQCLASS_MAGICIAN                         13
#define EQ_EQCLASS_ENCHANTER                        14

#define EQ_BASHKICKSTUN_BASE_CHANCE                 45
#define EQ_BASHKICKSTUN_BASE_CHANCE_ABOVE_LEVEL_60  40
#define EQ_BASHKICKSTUN_MIN_CHANCE                  2
#define EQ_BASHKICKSTUN_NPC_IMMUNE_ABOVE_LEVEL      55

// Pre-defined by the WoW core
#define EQ_DAZE_SPELL_ID                            1604
#define EQ_DEATHKNIGHT_DEATHGATE_SPELL_ID           50977
#define EQ_DEATHKNIGHT_RUNEFORGING_SPELL_ID         53428
#define EQ_DEATHKNIGHT_RUNEFORGING_SKILL_ID         776
#define EQ_DEATHKNIGHT_BLOODSTRIKE_SPELL_ID         45902
#define EQ_DEATHKNIGHT_SKILL_ID_BLOOD               770
#define EQ_DEATHKNIGHT_SKILL_ID_FROST               771
#define EQ_DEATHKNIGHT_SKILL_ID_UNHOLY              772
#define EQ_RACIAL_SKILL_ID_DWARF                    101
#define EQ_RACIAL_SKILL_ID_TAUREN                   124
#define EQ_RACIAL_SKILL_ID_ORC                      125
#define EQ_RACIAL_SKILL_ID_NIGHTELF                 126
#define EQ_RACIAL_SKILL_ID_UNDEAD                   220
#define EQ_RACIAL_SKILL_ID_TROLL                    733
#define EQ_RACIAL_SKILL_ID_GNOME                    753
#define EQ_RACIAL_SKILL_ID_HUMAN                    754
#define EQ_RACIAL_SKILL_ID_BLOODELF                 756
#define EQ_RACIAL_SKILL_ID_DRAENEI                  760
#define EQ_HEARTHSTONE_ITEM_ID                      6948
#define EQ_MASTER_TOTEM_ITEM_ID                     46978
#define EQ_SPELL_ID_MAGE_SHATTER_RANK1              11170
#define EQ_SPELL_ID_MAGE_SHATTER_RANK2              12982
#define EQ_SPELL_ID_MAGE_SHATTER_RANK3              12983
#define EQ_SPELL_ID_MAGE_FINGERS_OF_FROST_BUFF      44544
#define EQ_SPELL_ID_MAGE_FROST_CHANNELING_RANK1     11160
#define EQ_SPELL_ID_MAGE_FROST_CHANNELING_RANK2     12518
#define EQ_SPELL_ID_MAGE_FROST_CHANNELING_RANK3     12519
#define EQ_SPELL_ID_PRIEST_RENEWED_HOPE_RANK1       57470
#define EQ_SPELL_ID_PRIEST_RENEWED_HOPE_RANK2       57472
#define EQ_SPELL_ID_WARLOCK_TAMED_PET_PASSIVE       18727
#define EQ_SPELL_ID_MAGE_TORMENT_THE_WEAK_RANK1     29447
#define EQ_SPELL_ID_WARLOCK_DEATHS_EMBRACE_RANK1    47198
#define EQ_SPELL_ID_WARLOCK_SOUL_SIPHON_RANK1       17804
#define EQ_SPELL_ID_WARLOCK_PANDEMIC                58435
#define EQ_SPELL_ID_WARLOCK_MALEDICTION_RANK1       32477
#define EQ_SPELL_ID_PRIEST_MIND_MELT_RANK1          14910
#define EQ_SPELL_ID_PRIEST_IMPROVED_VAMPIRIC_EMBRACE_RANK1  27839
#define EQ_SPELL_ID_PRIEST_IMPROVED_FLASH_HEAL_RANK1        63504

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
#define EQ_SPELLDUMMYTYPE_FORAGE                    16
#define EQ_SPELLDUMMYTYPE_SUMMONACTIVE              17
#define EQ_SPELLDUMMYTYPE_SUCCOR                    18
#define EQ_SPELLDUMMYTYPE_TRACK                     19
#define EQ_SPELLDUMMYTYPE_SUMMONPC                  20

#define EQ_FACTION_ALIGNMENT_NONE                   0
#define EQ_FACTION_ALIGNMENT_NEUTRAL                1
#define EQ_FACTION_ALIGNMENT_GOOD                   2
#define EQ_FACTION_ALIGNMENT_EVIL                   3

#define EQ_BARDSONGAURATARGET_ENEMYAREA             1
#define EQ_BARDSONGAURATARGET_FRIENDLYPARTY         2
#define EQ_BARDSONGAURATARGET_SELF                  3
#define EQ_BARDSONGAURATARGET_ENEMYSINGLE           4
#define EQ_BARDSONGAURATARGET_FRIENDLYSINGLE        5
#define EQ_BARDSONGAURATARGET_ANY                   6

#define EQ_SPELLFAILABLETYPE_NONE                   0
#define EQ_SPELLFAILABLETYPE_FEIGNDEATH             1

#define EQ_SPELLFOCUSBOOSTTYPE_BARDPERCUSSION       1
#define EQ_SPELLFOCUSBOOSTTYPE_BARDBRASS            2
#define EQ_SPELLFOCUSBOOSTTYPE_BARDSINGING          3
#define EQ_SPELLFOCUSBOOSTTYPE_BARDSTRINGED         4
#define EQ_SPELLFOCUSBOOSTTYPE_BARDWIND             5

#define EQ_HASTE_TYPE_NONE                          0
#define EQ_HASTE_TYPE_WORNITEM                      1
#define EQ_HASTE_TYPE_SPELL_V1                      2
#define EQ_HASTE_TYPE_SPELL_V2                      3

#define EQ_PET_NAMING_TYPE_PET                      0
#define EQ_PET_NAMING_TYPE_FAMILIAR                 1
#define EQ_PET_NAMING_TYPE_WARDER                   2
#define EQ_PET_NAMING_TYPE_RANDOM                   3

#define EQ_CREATURE_DIFFICULTY_NORMAL               0
#define EQ_CREATURE_DIFFICULTY_RAIDTRASH            1
#define EQ_CREATURE_DIFFICULTY_RAIDBOSS             2
#define EQ_CREATURE_DIFFICULTY_RAIDMINIBOSS         3

#define EQ_QUEST_REACTION_UNKNOWN                   0
#define EQ_QUEST_REACTION_ATTACKPLAYER              1
#define EQ_QUEST_REACTION_DESPAWN                   2
#define EQ_QUEST_REACTION_EMOTE                     3
#define EQ_QUEST_REACTION_SAY                       4
#define EQ_QUEST_REACTION_SPAWN                     5
#define EQ_QUEST_REACTION_SPAWNUNIQUE               6
#define EQ_QUEST_REACTION_YELL                      7
#define EQ_QUEST_REACTION_KILLSPAWN                 8

#define EQ_KILLSPAWN_ACTION_SPAWN                   0
#define EQ_KILLSPAWN_ACTION_DESPAWN                 1
#define EQ_KILLSPAWN_ACTION_RESPAWNSELF             2
#define EQ_KILLSPAWN_ACTION_RESPAWNTARGET           3

#define EQ_KILLSPAWN_TRIGGER_DEATH                  0
#define EQ_KILLSPAWN_TRIGGER_COMBAT                 1
#define EQ_KILLSPAWN_TRIGGER_EVADE                  2
#define EQ_KILLSPAWN_TRIGGER_OOCTIMER               3   // Fires after DelayMinMS of continuous out-of-combat time

#define EQ_CYCLE_SPAWN_CHECK_INTERVAL_IN_MS         30000   // How often each map checks that its spawn cycles are still moving
#define EQ_CYCLE_SPAWN_PENDING_WINDOW_IN_SEC        300     // Respawn times within this window of a cycle respawn count as the cycle already moving

#define EQ_CREATURE_EMOTE_EVENT_LEAVECOMBAT         0
#define EQ_CREATURE_EMOTE_EVENT_ENTERCOMBAT         1
#define EQ_CREATURE_EMOTE_EVENT_ONDEATH             2
#define EQ_CREATURE_EMOTE_EVENT_AFTERDEATH          3
#define EQ_CREATURE_EMOTE_EVENT_HAILED              4
#define EQ_CREATURE_EMOTE_EVENT_KILLEDPC            5   // Was #9 in TAKP, but made sense here
#define EQ_CREATURE_EMOTE_EVENT_KILLEDNPC           6
#define EQ_CREATURE_EMOTE_EVENT_ONSPAWN             7
#define EQ_CREATURE_EMOTE_EVENT_ONDESPAWN           8
#define EQ_CREATURE_EMOTE_EVENT_RANDOMTIMER         10  // From EQ quest scripts, Param1/Param2 is min/max interval in MS
#define EQ_CREATURE_EMOTE_EVENT_PROXIMITY           11  // Param1 = radius in yards, Param2 = cooldown in MS

// Same as TAKP's EQ::constants::EmoteTypes
#define EQ_CREATURE_EMOTE_TYPE_SAY                  0
#define EQ_CREATURE_EMOTE_TYPE_EMOTE                1
#define EQ_CREATURE_EMOTE_TYPE_SHOUT                2
#define EQ_CREATURE_EMOTE_TYPE_PROXIMITY            3

#define EQ_CREATURE_EMOTE_PROXIMITY_CHECK_MS        1000   // How often a proximity emote creature searches for a nearby player
#define EQ_CREATURE_EMOTE_PROXIMITY_MIN_COOLDOWN_MS 5000   // Minimum time between next proxy emote (avoids spam)

#define EQ_NONE                                     -1
#define EQ_GRID_CIRCULAR                            0
#define EQ_GRID_RANDOM_10                           1
#define EQ_GRID_RANDOM                              2
#define EQ_GRID_PATROL                              3
#define EQ_GRID_ONE_WAY_REPOP                       4
#define EQ_GRID_RAND_5_LOS                          5
#define EQ_GRID_ONE_WAY_DEPOP                       6
#define EQ_GRID_CENTER_POINT                        7
#define EQ_GRID_RANDOM_CENTER_POINT                 8
#define EQ_GRID_RANDOM_PATH                         9

#define EQ_CREATURE_MOVEMENT_NO_CUSTOM              0      
#define EQ_CREATURE_MOVEMENT_CUSTOM_WAYPOINT        1
#define EQ_CREATURE_MOVEMENT_CUSTOM_ROAMING         2

#define EQ_MOVE_RETURN_TO_AGRO_ID                   20000
#define EQ_MOVE_TO_ROAM_POINT                       20001
#define EQ_MOVE_TO_WAYPOINT_POINT                   20002

#define EQ_MOVE_SMALL_STEP_SIZE_DISTANCE            10.0f   // Minimum distance between nodes when making small steps
#define EQ_MOVE_SMALL_STEP_SIZE_LAST_DISTANCE       5.0f    // How much minimium distance to allow on the last node
#define EQ_MOVE_SMALL_STEP_DISTANCE_TO_END          0.25f   // If within this distance, end movement to current node
#define EQ_MOVE_UNDER_WATER_SURFACE_SKIM_REDICTION  3.0f    // How far to reduce a position if a tested z position hit water
#define EQ_MOVE_TEST_Z_DOWN_AMOUNT_FOR_WATER_TEST   10.0f   // How far to test down when looking for water in the Z testing
#define EQ_MOVE_MAX_PATH_NODES                      512     // Hard cap on generated step nodes

#define EQ_MOVE_PHASE_NONE                          0
#define EQ_MOVE_PHASE_TRAVELING                     1
#define EQ_MOVE_PHASE_WAITING_FOR_TIMER             2
#define EQ_MOVE_PHASE_AGRO                          3
#define EQ_MOVE_PHASE_RETURNING_FROM_AGRO           4

#define EQ_MOVE_PATH_MAX_RETRY_COUNT                10

#define EQ_FORAGE_TYPE_FOOD                         0
#define EQ_FORAGE_TYPE_DRINK                        1
#define EQ_FORAGE_TYPE_BAIT                         2
#define EQ_FORAGE_TYPE_OTHER                        3

#define EQ_CREATURE_CUSTOMDATA_RANGEDATTACK         "EQRangedAtk"
#define EQ_CREATURE_CUSTOMDATA_COMBATABILITY        "EQCombatAbility"
#define EQ_CREATURE_CUSTOMDATA_SUMMON               "EQSummon"
#define EQ_CREATURE_CUSTOMDATA_UNSTICK              "EQUnstick"
#define EQ_CREATURE_CUSTOMDATA_SOCIALAGGRO          "EQSocialAggro"
#define EQ_CREATURE_CUSTOMDATA_EMOTE                "EQEmote"
#define EQ_CREATURE_CUSTOMDATA_MOVEMENTSOUND        "EQMoveSound"
#define EQ_CREATURE_CUSTOMDATA_KILLSPAWNWATCH       "EQKillSpawnWatch"
#define EQ_CREATURE_CUSTOMDATA_VULAKLOCK            "EQVulakLock"
#define EQ_CREATURE_CUSTOMDATA_DEFENDPLAYERWATCH    "EQDefendPlayerWatch"
#define EQ_CREATURE_CUSTOMDATA_AGGROPOSITION        "EQAggroPos"
#define EQ_CREATURE_CUSTOMDATA_AGROZBLOCK           "EQAgroZBlock"
#define EQ_CREATURE_CUSTOMDATA_FEARDIMINISH         "EQFearDiminish"

#define EQ_AGRO_Z_BLOCK_SUPPRESS_MS                 2000

#define EQ_DEFEND_PLAYERS_CHECK_MS                  2000
#define EQ_DEFEND_PLAYERS_SEARCH_RADIUS             15.0f

#define EQ_PLAYER_CUSTOMDATA_TRACKING               "EQTracking"
#define EQ_TRACKING_ADDON_ROWS_PER_MESSAGE          4       // List rows batched per addon message to stay under client chat limits
#define EQ_TRACKING_LOST_DISTANCE_MULTIPLIER        1.25f   // Fraction of max track distance a tracked creature can stray before the trail goes cold
#define EQ_TRACKING_FOUND_DISTANCE                  15.0f   // Within this many yards, the tracked creature counts as found
#define EQ_TRACKING_SCAN_MIN_INTERVAL_MS            2000    // Minimum time between track scans for one player (guards against command spam)

#define EQ_AGILE_FIGHTER_REFRESH_INTERVAL_MS        2000    // How often to scan for gear changes since some forms of unequip have no hook

// Vulak`Aerr (Temple of Veeshan) spawns perma-rooted and "locked" (unattackable, non-aggro) until every required dragon is dead, matching Velious-era EQ
#define EQ_VULAK_CREATURE_TEMPLATE_ID               55045
#define EQ_VULAK_LOCK_RECHECK_MS                    3000

#define EQ_CREATURE_MOVEMENT_GAIT_NONE              0
#define EQ_CREATURE_MOVEMENT_GAIT_WALK              1
#define EQ_CREATURE_MOVEMENT_GAIT_RUN               2

#define EQ_CREATURE_MOVEMENT_SOUND_LISTENER_SCAN_MS 250

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
    uint32 EffectFailChancePercent = 0;
    uint32 EffectFailableType = 0;
    bool StunUsesBashKickChance = false;
    uint32 SpellIDCastOnTargetWhenStunLands = 0;
    bool AuraStaysOnSecondaryClassSwitch = false;
    uint32 MinTargetLevel = 0;
    uint32 MaxCreatureTargetLevel = 0;
    int32 ResistDiff = 0;
    uint32 HasteType = EQ_HASTE_TYPE_NONE;
    int32 ModFactionRepValue = 0;
    uint8 IllusionFormAlignment = EQ_FACTION_ALIGNMENT_NONE;
    uint32 IllusionFormEQRaceID = 0;
    bool PersistOnClassChange = false;
};

class EverQuestCreature
{
public:
    uint32 CreatureTemplateID = 0;
    bool CanShowHeldLootItems = 0;
    bool CanShowHeldLootShields = 0;
    uint32 SpawnLimit = 0;
    bool RangedAttackEnabled = false;
    uint32 RangedAttackMinRange = 0;
    uint32 RangedAttackMaxRange = 0;
    int32 RangedAttackDamageModPct = 0;
    float AgroSocialDistanceMod = 1.0f;
    bool EnrageEnabled = false;
    uint32 EnrageHPPct = 0;
    uint32 EnrageDurationInMS = 0;
    uint32 EnrageCooldownInMS = 0;
    bool FlurryEnabled = false;
    uint32 FlurryChancePct = 0;
    bool RampageEnabled = false;
    uint32 RampageChancePct = 0;
    uint32 RampageRange = 0;
    uint32 RampageDamagePct = 0;
    bool WildRampageEnabled = false;
    uint32 WildRampageChancePct = 0;
    uint32 WildRampageMaxTargets = 0;
    uint32 WildRampageDamagePct = 0;
    uint32 AttackRoundTimeInMS = 0;
    uint32 DifficultyType = EQ_CREATURE_DIFFICULTY_NORMAL;
};

class EverQuestCreatureSpawnPoint
{
public:
    uint32 CreatureGUID = 0;
    uint32 MapID = 0;
    uint32 SpawnPointID = 0;
    uint32 SpawnGroupID = 0;
    uint32 SpawnGroupLimit = 0;
    uint32 CycleRespawnTimeSec = 0;
    uint32 CycleChance = 0;
};

class EverQuestCycleSpawnCandidate
{
public:
    ObjectGuid::LowType CreatureGUID = 0;
    uint32 Chance = 0;
};

class EverQuestCycleSpawnGroup
{
public:
    uint32 MapID = 0;
    uint32 SpawnGroupID = 0;
    uint32 SpawnGroupLimit = 1;
    uint32 CycleRespawnTimeSec = 1;
    map<uint32, vector<EverQuestCycleSpawnCandidate>> CandidatesBySpawnPointID;
};

class EverQuestCreatureKillSpawn
{
public:
    uint32 ID = 0;
    uint32 TriggerCreatureTemplateID = 0;
    uint8 TriggerTypeID = EQ_KILLSPAWN_TRIGGER_DEATH;
    uint32 MapID = 0;
    uint8 ActionType = EQ_KILLSPAWN_ACTION_SPAWN;
    uint32 TargetCreatureTemplateID = 0;
    float Chance = 100;
    uint32 AltGroup = 0;
    uint32 AltID = 0;
    float AltWeight = 0;
    bool SpawnAtCorpse = false;
    float PositionX = 0;
    float PositionY = 0;
    float PositionZ = 0;
    float Orientation = 0;
    uint32 DelayMinMS = 0;
    uint32 DelayMaxMS = 0;
    uint32 OnlyIfNotAliveCreatureTemplateID = 0;
    vector<uint32> RequireDeadCreatureTemplateIDs;
    vector<uint32> RequireAliveCreatureTemplateIDs;
    bool AddToHateList = false;
    uint32 TriggerMinLevel = 0;
    uint32 TriggerMaxLevel = 0;
    uint32 RespawnTimeSec = 0;
    unordered_map<uint32, vector<ObjectGuid::LowType>> TargetSpawnIDsByMapID; // Spawn rows differ between the open world zone and its raid instance copy, so respawn targets resolve per map
};

class EverQuestPendingKillSpawnAction
{
public:
    int32 RemainingMS = 0;
    uint8 ActionType = EQ_KILLSPAWN_ACTION_SPAWN;
    uint32 TargetCreatureTemplateID = 0;
    ObjectGuid::LowType RespawnSpawnID = 0;
    float PositionX = 0;
    float PositionY = 0;
    float PositionZ = 0;
    float Orientation = 0;
    uint32 OnlyIfNotAliveCreatureTemplateID = 0;
    bool DespawnNearestToPositionOnly = false;
    bool AddToHateList = false;
    uint32 RespawnTimeSec = 0;
    vector<ObjectGuid::LowType> RespawnTargetSpawnIDs;
    ObjectGuid KillerGUID;
};

class EverQuestLoadedCreatureEquippedVisualItems
{
public:
    uint32 MainhandItemID = 0;
    uint32 OffhandItemID = 0;
    bool IsDualWielding = false;
};

class EverQuestCreatureRangedAttackState : public DataMap::Base
{
public:
    float MinRange = 0.0f;
    float MaxRange = 0.0f;
    int32 DamageModPct = 0;
    uint32 SwingTimerRemainingMS = 0;
};

class EverQuestCreatureCombatAbilityState : public DataMap::Base
{
public:
    bool EnrageEnabled = false;
    uint32 EnrageHPPct = 0;
    uint32 EnrageDurationInMS = 0;
    uint32 EnrageCooldownInMS = 0;
    bool FlurryEnabled = false;
    uint32 FlurryChancePct = 0;
    bool RampageEnabled = false;
    uint32 RampageChancePct = 0;
    float RampageRange = 0.0f;
    uint32 RampageDamagePct = 0;
    bool WildRampageEnabled = false;
    uint32 WildRampageChancePct = 0;
    uint32 WildRampageMaxTargets = 0;
    uint32 WildRampageDamagePct = 0;
    uint32 AttackRoundTimeInMS = 0;
    bool IsEnraged = false;
    uint32 EnrageDurationRemainingMS = 0;
    uint32 EnrageCooldownRemainingMS = 0;
    uint32 SpecialAttackTimerRemainingMS = 0;
    uint32 ActiveSwingDamageModPct = 100;
};

class EverQuestCreatureSummonState : public DataMap::Base
{
public:
    uint32 CooldownRemainingMS = 0;
};

class EverQuestCreatureUnstickState : public DataMap::Base
{
public:
    float AnchorX = 0.0f;
    float AnchorY = 0.0f;
    bool HasAnchor = false;
    uint32 StuckTimerMS = 0;
    uint32 SettleRemainingMS = 0;
    uint32 TeleportAttemptsUsed = 0;
};

class EverQuestCreatureSocialAggroState : public DataMap::Base
{
public:
    uint32 RecallTimerMS = 0;
};

class EverQuestCreatureAggroPositionState : public DataMap::Base
{
public:
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;
    float Orientation = 0.0f;
    bool HasPosition = false;
};

class EverQuestCreatureFearDiminishingReturnState : public DataMap::Base
{
public:
    uint32 Level = 0;                 // 0 = full duration, 1 = half, 2 = quarter, 3 = immune
    uint32 LastApplyTimeMS = 0;
    uint32 ResetWindowInMS = 0;
};

class EverQuestCreatureAgroZBlockState : public DataMap::Base
{
public:
    ObjectGuid BlockedVictimGUID;
    bool DropPending = false;
    uint32 SuppressRemainingMS = 0;
    bool RestoreAggressiveReactState = false;
};

class EverQuestPendingSummonRequest
{
public:
    ObjectGuid CasterGUID;
    string CasterName;
    uint32 MapID = 0;
    uint32 ZoneID = 0;
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;
};

class EverQuestPlayerClientVersionCheckState
{
public:
    int32 MSUntilDeadline = 0;
    bool FailedPendingKick = false;
};

class EverQuestCreatureEmote
{
public:
    uint8 EventType = 0;
    uint8 EmoteType = 0;
    float ChancePct = 100;
    int32 Param1 = 0;
    int32 Param2 = 0;
    string EmoteText;
};

class EverQuestCreatureEmoteState : public DataMap::Base
{
public:
    bool WasAlive = false;
    uint32 RandomTimerRemainingMS = 0;
    uint32 ProximityCheckRemainingMS = 0;
    uint32 ProximityCooldownRemainingMS = 0;
};

class EverQuestCreatureKillSpawnWatchState : public DataMap::Base
{
public:
    bool WasInCombat = false;
    uint32 OocTimerRemainingMS = 0; // 0 = not yet armed
};

class EverQuestVulakLockState : public DataMap::Base
{
public:
    bool WasAlive = false;
    bool Unlocked = false;
    uint32 RecheckRemainingMS = 0;
};

class EverQuestCreatureMovementSound
{
public:
    vector<uint32> WalkPieceSoundEntryIDs;
    vector<uint32> WalkPieceDurationsMS;
    vector<uint32> RunPieceSoundEntryIDs;
    vector<uint32> RunPieceDurationsMS;
    float MaxHearingDistance = 20.0f;
};

class EverQuestCreatureMovementSoundListener
{
public:
    uint32 PieceIndex = 0;
    uint32 ReplayRemainingMS = 0;
};

class EverQuestCreatureMovementSoundState : public DataMap::Base
{
public:
    uint8 CurGait = EQ_CREATURE_MOVEMENT_GAIT_NONE;
    uint32 ListenerScanRemainingMS = 0;
    unordered_map<ObjectGuid, EverQuestCreatureMovementSoundListener> ListenersByGUID;
};

class EverQuestItemTemplate
{
public:
    uint32 ItemTemplateEntryID = 0;
    uint32 ItemTemplateEntryIDForNPCEquip = 0;
    uint32 WornEffectSpellID = 0;
    uint32 AllowedEQClassMask = 0;
    uint32 EQArmorMaterial = 0;
    uint32 IllusionTintID = 0;
};

class EverQuestGearSwapCandidate
{
public:
    uint32 ItemTemplateID = 0;
    uint32 ItemDisplayID = 0;
};

class EverQuestPlayerIllusionState
{
public:
    uint32 FormSpellID = 0;
    uint32 RefreshTimerMS = 0;
};

class EverQuestPlayerTrackingState : public DataMap::Base
{
public:
    ObjectGuid TrackedCreatureGUID;
    std::string TrackedCreatureName;
    float MaxTrackDistance = 0;
    uint32 PulseTimerMS = 0;
    uint64 LastScanMSTime = 0;
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

class EverQuestPlayerCreateInfo
{
public:
    uint8 RaceID = 0;
    uint8 ClassID = 0;
    uint32 MapID = 0;
    uint32 ZoneID = 0;
    float PositionX = 0;
    float PositionY = 0;
    float PositionZ = 0;
    float Orientation = 0;
    uint32 IllusionItemID = 0;
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
    uint32 QuestgiverCreatureTemplateID = 0;
    uint32 DelayInMS = 0;
};

class EverQuestGossipReaction
{
public:
    uint32 GossipCreatureTemplateID = 0;
    uint32 NpcTextID = 0;
    uint32 OptionID = 0;
    string OptionText;
    int ReactionType = 0;
    string SayText;
    uint32 TargetCreatureTemplateID = 0;
    bool UsePlayerX = false;
    bool UsePlayerY = false;
    bool UsePlayerZ = false;
    float AddedPlayerX = 0;
    float AddedPlayerY = 0;
    bool UsePlayerOrientation = false;
    bool UseNpcX = false;
    bool UseNpcY = false;
    bool UseNpcZ = false;
    bool UseNpcOrientation = false;
    float PositionX = 0;
    float PositionY = 0;
    float PositionZ = 0;
    float Orientation = 0;
    uint32 DelayInMS = 0;
};

class EverQuestTriggeredQuestKillSpawn
{
public:
    uint32 TriggerCreatureTemplateID = 0;
    uint32 TargetCreatureTemplateID = 0;
    float PositionX = 0;
    float PositionY = 0;
    float PositionZ = 0;
    float Orientation = 0;
};

class EverQuestCreatureLootEntry
{
public:
    uint32 ItemTemplateID = 0;
    float Chance = 0;
    uint32 ItemMultiplier = 1;
    uint32 ItemCharges = 1;
};

class EverQuestCreatureLootGroup
{
public:
    uint32 LootGroupID = 0;
    uint32 GroupMultiplier = 1;
    uint32 GroupMultiplierMin = 0;
    float GroupProbability = 100;
    uint32 DropLimit = 0;
    uint32 MinDrop = 0;
    vector<EverQuestCreatureLootEntry> Entries;
};

class EverQuestTransportShipTrigger
{
public:
    uint32 TriggeringShipGameObjectEntryTemplateID = 0;
    uint32 TriggeredShipGameObjectTemplateEntryID = 0;
    uint32 TriggeringNodeID = 0;
    uint32 TriggerActivateNodeID = 0;
};

class EverQuestCreatureInstance
{
public:
    uint32 CreatureGUID = 0;
    int8 WanderType = 0;
    int8 PauseType = 0;
    uint32 MapID = 0;
    uint32 WaypointListID = -1;
    bool DoesRoam = false;
    float RoamMinX = 0;
    float RoamMaxX = 0;
    float RoamMinY = 0;
    float RoamMaxY = 0;
    float RoamMinZ = 0;
    float RoamMaxZ = 0;
    uint32 RoamMinDelayInMS = 0;
    uint32 RoamMaxDelayInMS = 0;
    int32 DespawnAtWaypointNum = -1;
    bool DisableGroundContour = false;
};

class EverQuestCreatureWaypoint
{
public:
    uint32 MapID = 0;
    uint32 WaypointID = 0;
    uint32 Number = 0;
    float X = 0;
    float Y = 0;
    float Z = 0;
    uint32 PauseInSec = 0;
};

class EverQuestAutoLearnSpell
{
public:
    uint8 EQClassID = 0;
    uint8 RaceID = 0;
    uint32 SpellID = 0;
    uint8 Level = 1;
};

class EverQuestForageZoneItem
{
public:
    uint32 MapID = 0;
    uint32 ItemTemplateID = 0;
    uint32 Chance = 0;
    uint32 ForageType = EQ_FORAGE_TYPE_FOOD;
};

class EverQuestZoneSafePoint
{
public:
    uint32 MapID = 0;
    float X = 0;
    float Y = 0;
    float Z = 0;
    float Orientation = 0;
};

class EverQuestZone
{
public:
    uint32 MapID = 0;
    bool AllowBind = true;
    int32 ExpansionID = 0;
    float MaxAgroZDistance = -1.0f;
    uint32 InstanceRaidLowMapID = 0;
};

// Where a player is (or last was) with respect to the raid instance copies of zones.  The last visited instance is remembered after leaving,
// so a zone line can put a player back with their raid
class EverQuestPlayerRaidLowInstanceState
{
public:
    uint32 RaidLowMapID = 0;
    uint32 InstanceID = 0;
    bool IsInside = false;
};

// Mirror of the group totals that KillRewarder builds in _InitGroupData, but gathered for every group member in the zone
// instead of only those inside the core's group reward distance
struct EverQuestZoneWideKillReward
{
    bool IsValid = false;
    uint32 AliveMemberCount = 0;
    uint32 AliveSumLevel = 0;
    uint8 MaxLevel = 0;
    Player* MaxNotGrayMember = nullptr;
    uint8 MaxNotGrayMemberLevel = 0;
    bool IsFullXP = false;
    uint32 BaseExperience = 0;
    float GroupRate = 1.0f;
};

class EverQuestFaction
{
public:
    uint32 FactionTemplateID = 0;
    uint32 FactionID = 0;
    uint8 BaseAlignment = EQ_FACTION_ALIGNMENT_NONE;
    uint32 PredominantEQRaceID = 0;
    bool WillDefendFriendlyPlayers = false;
    bool DefendersWillAttackToDefendPlayer = false;
    uint32 DefendCombatFactionTemplateID = 0;
};

struct EverQuestReputationFactionInfo
{
    uint8 BaseAlignment = EQ_FACTION_ALIGNMENT_NONE;
    uint32 PredominantEQRaceID = 0;
};

struct EverQuestPlayerTempFactionBonus
{
    uint32 FactionID = 0;
    int32 Amount = 0;
    uint32 SpellID = 0;
    ObjectGuid TargetCreatureGUID;
};

class EverQuestCreatureDefendPlayerWatchState : public DataMap::Base
{
public:
    uint32 RecheckTimerMS = 0;
};

struct EverQuestPlayerControllerData
{
    uint32 GUID = 0;
    uint8 CurrentSecondClass = 0;
    uint8 NextSecondClass = 0;
    uint32 SecondaryExpPool = 0;
    uint32 IllusionFaceID = 0;
    bool ShowBardPulse = true;
    uint32 IssuedIllusionItemID = 0;
    bool HideWoWGear = false;
};

class EverQuestPlayerClassInfoItem
{
public:
    uint8 ClassID = 0;
    string ClassName = "";
    uint8 Level = 1;
};

struct EverQuestPlayerEquipedItemData
{
    uint8 Slot;
    uint32 ItemID;
    uint32 TempEnchant;
    uint32 PermEnchant;
    uint32 ItemInstanceGUID;
};

struct EverQuestUnitHasteAuraEffect
{
    uint32 SpellID;
    ObjectGuid CasterGUID;
    uint8 EffectIndex;
    uint32 AuraType;
    int32 NaturalAmount;    // The amount the effect would apply if there were no cap
    uint32 HasteType;
};

class EverQuestClassMap
{
public:
    uint8 WOWClassID;
    uint8 EQClassIDBase;
    uint8 EQClassIDDefaultSecond;
    uint32 EQClassIDEligibleSecondMask;
};

class EverQuestMod
{
private:
    EverQuestMod();
    unordered_map<ObjectGuid, EverQuestPlayerControllerData> ActivePlayerClassControllerDataByGUID;
    unordered_map<ObjectGuid, uint64> PendingEquipmentStorageCommitMSByGUID;
    std::mutex PendingStorageTransactionMutex;
    unordered_map<ObjectGuid, TransactionCallback> PendingStorageTransactionCallbacksByGUID;

public:
    bool IsEnabled;

    // Configs (from database)
    float ConfigWorldScale;
    uint32 ConfigBardMaxConcurrentSongs;
    uint32 ConfigSystemMapDBCIDMin;
    uint32 ConfigSystemMapDBCIDMax;
    uint32 ConfigSystemSpellDBCIDMin;
    uint32 ConfigSystemSpellDBCIDMax;
    uint32 ConfigSystemQuestSQLIDMin;
    uint32 ConfigSystemQuestSQLIDMax;
    uint32 ConfigSystemCreatureTemplateIDMin;
    uint32 ConfigSystemCreatureTemplateIDMax;
    bool ConfigDeathKnightsStartLikeOtherClasses;
    bool ConfigDazeEnabledInEQZones;
    uint32 ConfigSystemGameObjectTemplateIDMin;
    uint32 ConfigSystemGameObjectTemplateIDMax;
    uint32 ConfigSystemShipEntryTemplateIDMin;
    uint32 ConfigSystemShipEntryTemplateIDMax;
    uint32 ConfigSystemInvisVsUndeadDetectSpellID;
    uint32 ConfigSystemRangedAttackSpellID;
    uint32 ConfigSystemResistAdjustmentSpellID;
    uint32 ConfigSystemLegacyAchievementID;
    string ConfigSystemLegacyAchievementAccountCreatedBefore;
    uint32 ConfigSystemItemTemplateIDMin;
    uint32 ConfigSystemItemTemplateIDMax;
    uint32 ConfigSystemAdventurerAchievementID;
    uint32 ConfigSystemAdventurerAuraSpellID;
    uint32 ConfigSystemAgileFighterSpellID;
    uint32 ConfigSystemAgileFighterCombatMasterSpellID;
    uint32 ConfigSystemAgileFighterCombatExpertSpellID;
    uint32 ConfigSystemRaidBossRespawnVarianceInSec;
    uint32 ConfigSystemRaidMiniBossRespawnVarianceInSec = 0;
    uint32 ConfigSystemClientDataVersion = 0;
    string ConfigSystemClientDataVersionMismatchMessage;
    uint32 ConfigSystemFactionGoodClassMask;
    uint32 ConfigSystemFactionEvilClassMask;
    uint32 ConfigSystemFactionGoodRaceMask;
    uint32 ConfigSystemFactionEvilRaceMask;

    // Configs (from server file)
    bool ConfigMapRestrictPlayersToNorrath;
    int ConfigMapMaxExpansionID;
    uint32 ConfigMapRestrictedMapCheckIntervalInSeconds;
    bool ConfigClientVersionCheckEnabled = false;
    uint32 ConfigClientVersionCheckGraceTimeInSeconds = 30;
    uint32 ConfigClientVersionCheckKickDelayInSeconds = 10;
    bool ConfigSpellTalentAlignmentEnabled;
    bool ConfigQuestGrantExpOnRepeatCompletion;
    bool ConfigExpLossOnDeathEnabled;
    int ConfigExpLossOnDeathMinLevel;
    float ConfigExpLossOnDeathLossPercent;
    bool ConfigExpLossOnDeathAddLostExpToRestExp;
    bool ConfigAlternateGroupExperienceFormulaEnabled;
    float ConfigAlternateGroupExperienceAddPercentPerAddedMember;
    bool ConfigSpellDisableStackingOfSameDOT;
    bool ConfigSpellBuffLevelRestrictionsEnabled;
    bool ConfigSpellCrowdControlLevelRestrictionsEnabled;
    bool ConfigSpellHasteCapEnabled;
    float ConfigSpellHasteCapPercent;
    bool ConfigSpellBardFearDiminishingReturnsEnabled;
    uint32 ConfigSpellBardFearDiminishingReturnsResetTimeInMS;
    bool ConfigCombatSkillsDisableBashKickStunOnPlayers;
    bool ConfigCombatSkillsRangedAttackEnabled;
    float ConfigCombatSkillsRangedAttackDefaultMinRange;
    float ConfigCombatSkillsRangedAttackDefaultMaxRange;
    float ConfigCombatSkillsRangedAttackDamageMultiplier;
    bool ConfigCombatSkillsEnrageEnabled;
    uint32 ConfigCombatSkillsEnrageDefaultHPPct;
    uint32 ConfigCombatSkillsEnrageDefaultDurationInMS;
    uint32 ConfigCombatSkillsEnrageDefaultCooldownInMS;
    bool ConfigCombatSkillsFlurryEnabled;
    uint32 ConfigCombatSkillsFlurryDefaultChancePct;
    bool ConfigCombatSkillsRampageEnabled;
    uint32 ConfigCombatSkillsRampageDefaultChancePct;
    float ConfigCombatSkillsRampageDefaultRange;
    bool ConfigCombatSkillsWildRampageEnabled;
    uint32 ConfigCombatSkillsWildRampageDefaultChancePct;
    uint32 ConfigCombatSkillsWildRampageDefaultMaxTargets;
    bool ConfigCombatSkillsRaidBossSummonEnabled;
    uint32 ConfigCombatSkillsRaidBossSummonMaxHealthPct;
    uint32 ConfigCombatSkillsRaidBossSummonCooldownInMS;
    bool ConfigEvadeEnabled;
    float ConfigEvadeUnreachableSeconds;
    float ConfigEvadeUnstickStallSeconds;
    float ConfigEvadeUnstickSettleSeconds;
    float ConfigEvadeUnstickMoveThreshold;
    uint32 ConfigEvadeUnstickMaxAttempts;
    uint32 ConfigEvadeUnstickStepPercent;
    float ConfigEvadeNonEQMapLeashRadius;
    bool ConfigCharmCreatureCharmLimitsEnabled;
    float ConfigCharmUncharmedPlayerCheckRadius;
    bool ConfigFactionDefendFriendlyPlayersEnabled;
    bool ConfigPetDisableInitialCreatureAgro;
    bool ConfigCreatureEmotesEnabled;
    bool ConfigCreatureEmotesAmbientEnabled;
    bool ConfigCreatureMovementSoundsEnabled;
    uint32 ConfigIllusionGearRefreshTimeInMS;
    bool ConfigShowClassMessageOnLogin;
    float ConfigSecondaryExpPoolGainPercent;
    uint32 ConfigSecondaryExpPoolMaxPooled;
    uint32 ConfigPlayerLevelCap;
    bool ConfigPlayerShieldArmorIgnoresBearFormMultiplier;
    bool ConfigPlayerAddHearthstoneToNewCharacters;
    bool ConfigPlayerAddMasterTotemToShamans;
    bool ConfigPlayerAddRacialGuiseItemOnLogin;
    uint32 ConfigAchievementAdventurerLevel;
    bool ConfigAchievementAdventurerProtectedInEQZones;
    bool ConfigAchievementAdventurerGrantAuraOnLoginIfMissing;
    std::set<uint32> ConfigCrossClassIncludeSkillIDs;
    bool ConfigTrackingEnabled;
    float ConfigTrackingRangerYardsPerLevel;
    float ConfigTrackingDruidYardsPerLevel;
    float ConfigTrackingBardYardsPerLevel;
    float ConfigTrackingRangerMaxRange;
    float ConfigTrackingDruidMaxRange;
    float ConfigTrackingBardMaxRange;
    uint32 ConfigTrackingMaxResults;
    uint32 ConfigTrackingPulseIntervalInMS;
    bool ConfigSpellSummonPlayerAcrossZones;
    bool ConfigGroupZoneWideLootAndExperienceEnabled;

    unordered_set<uint32> CrossClassExemptSpellIDs;
    unordered_set<uint32> RacialSpellIDs;
    unordered_set<uint32> DeathKnightSpellIDs;
    bool CrossClassExemptSpellIDsBuilt;

    // Guards the runtime state containers (the trackers keyed by creature/player GUID below). Maps update on parallel
    // worker threads, so any insert/erase/find on these must hold this lock. Never hold it across engine calls
    // (casts, teleports, evades, etc.), since those can re-enter mod hooks that also take it. Values obtained under
    // the lock stay valid after release (unordered containers do not move nodes), and are only mutated by the
    // owning entity's thread.
    std::mutex RuntimeStateMutex;

    unordered_map<uint32, EverQuestCreature> CreaturesByTemplateID;
    unordered_map<uint32, list<EverQuestCreatureOnkillReputation>> CreatureOnkillReputationsByCreatureTemplateID;
    unordered_map<uint32, vector<EverQuestCreatureKillSpawn>> CreatureKillSpawnsByTriggerCreatureTemplateID;
    unordered_set<uint32> EvadeKillSpawnTriggerCreatureTemplateIDs;
    unordered_map<uint32, uint32> OocTimerKillSpawnDurationMSByCreatureTemplateID;
    unordered_map<uint32, vector<ObjectGuid::LowType>> VulakRequiredDragonSpawnIDsByMapID; // Keyed by map ID, since the raid instance copy of the zone has its own dragon spawn rows
    unordered_map<uint32, vector<EverQuestCreatureEmote>> CreatureEmotesByCreatureTemplateID;
    unordered_map<uint32, EverQuestCreatureMovementSound> CreatureMovementSoundsByDisplayID;
    unordered_map<uint32, uint32> SilentFidgetDisplayIDsByDisplayID;

    std::mutex PendingKillSpawnActionsMutex;
    unordered_map<uint64, vector<EverQuestPendingKillSpawnAction>> PendingKillSpawnActionsByMapInstanceKey;
    unordered_map<uint64, vector<EverQuestTriggeredQuestKillSpawn>> TriggeredQuestKillSpawnsByMapInstanceKey;
    unordered_map<uint32, EverQuestItemTemplate> ItemTemplatesByEntryID;
    unordered_map<uint64, vector<EverQuestGearSwapCandidate>> GearSwapCandidatesByLookupKey;
    unordered_set<uint32> WornEffectSpellIDs;
    unordered_map<uint32, EverQuestSpell> SpellDataBySpellID;
    unordered_set<uint32> BardSongTickSpellIDs;
    unordered_map<uint64, uint32> IllusionDisplayIDsByLookupKey;
    unordered_map<uint64, uint32> IllusionFaceDisplayIDsByLookupKey;
    uint32 IllusionMaxFaceIndex;
    unordered_set<uint32> IllusionFormSpellIDs;
    unordered_map<ObjectGuid, EverQuestPlayerIllusionState> PlayerIllusionStatesByPlayerGUID;
    unordered_map<uint32, list<EverQuestQuestCompletionReputation>> QuestCompletionReputationsByQuestTemplateID;
    unordered_map<uint32, list<EverQuestQuestReaction>> QuestReactionListByQuestTemplateID;
    unordered_map<uint32, vector<EverQuestGossipReaction>> GossipReactionsByGossipCreatureTemplateID;
    unordered_map<uint32, EverQuestPet> PetDataByCreatureTemplateID;
    unordered_map<uint8, unordered_map<uint8, EverQuestPlayerCreateInfo>> PlayerCreateInfoByRaceIDThenClassID;
    unordered_map<uint8, list<uint32>> PlayerAutoLearnSkillsByEQClassID;
    unordered_map<uint8, list<EverQuestAutoLearnSpell>> PlayerAutoLearnSpellsByClassID;
    unordered_map<uint64, unordered_map<int, vector<Creature*>>> AllLoadedCreaturesByMapInstanceKeyThenCreatureEntryID;
    unordered_map<uint32, EverQuestCreatureSpawnPoint> CreatureSpawnPointsByCreatureGUID;
    unordered_map<uint64, unordered_map<uint32, vector<Creature*>>> AllLoadedCreaturesByMapInstanceKeyThenSpawnPointID;
    unordered_map<uint64, unordered_map<uint32, vector<Creature*>>> AllLoadedCreaturesByMapInstanceKeyThenSpawnGroupID;
    unordered_map<uint32, unordered_map<uint32, EverQuestCycleSpawnGroup>> CycleSpawnGroupsByMapIDThenSpawnGroupID;
    unordered_map<uint32, int32> CycleSpawnCheckTimerInMSByMapID;
    uint32 RestrictedMapCheckTimerInMS = 0;
    unordered_map<ObjectGuid, EverQuestPlayerClientVersionCheckState> PendingClientVersionChecksByPlayerGUID;
    unordered_map<ObjectGuid, deque<uint32>> PlayerCasterConcurrentBardSongs;
    unordered_set<ObjectGuid> PlayersWithAuctionUsableFilterActive;
    unordered_set<ObjectGuid> PlayersGainingExperience;
    unordered_set<ObjectGuid> PlayersPendingLevelCapExperiencePark;
    unordered_map<uint64, unordered_map<ObjectGuid, vector<EverQuestUnitHasteAuraEffect>>> EQHasteAuraEffectsByMapInstanceKeyThenUnitGUID; // Map-instance keyed since creature GUIDs repeat across instance copies of a map
    unordered_map<ObjectGuid, uint32> BearFormShieldArmorShiftAmountByPlayerGUID;
    unordered_map<ObjectGuid, uint32> AgileFighterRefreshTimerMSByPlayerGUID;
    unordered_map<uint32, vector<EverQuestCreatureLootGroup>> CreatureLootGroupsByCreatureTemplateID;
    unordered_map<uint64, unordered_map<ObjectGuid, vector<uint32>>> PreloadedLootItemIDsByMapInstanceKeyThenCreatureGUID; // Map-instance keyed since creature GUIDs repeat across instance copies of a map
    unordered_map<uint64, unordered_map<ObjectGuid, unordered_map<uint32, uint32>>> PreloadedLootCountsByMapInstanceKeyThenCreatureGUID;
    unordered_map<uint64, unordered_map<ObjectGuid, EverQuestLoadedCreatureEquippedVisualItems>> VisualEquippedItemsByMapInstanceKeyThenCreatureGUID;
    unordered_map<uint64, unordered_set<ObjectGuid>> CreaturesResolvingEQMeleeExtraAttacksByMapInstanceKey; // Map-instance keyed since creature GUIDs repeat across instance copies of a map
    unordered_map<uint32, vector<EverQuestTransportShipTrigger>> ShipTriggersByTriggeringGameObjectTemplateEntryID;
    unordered_map<uint32, int> ShipWaitNodesByGameObjectTemplateEntryID;
    unordered_map<uint32, GameObject*> ShipGameObjectsByTemplateEntryID;
    unordered_map<uint32, EverQuestCreatureInstance> CreatureInstancesByCreatureGUID;
    unordered_map<uint32, unordered_map<uint32, vector<EverQuestCreatureWaypoint>>> CreatureWaypointsByMapIDAndWaypointID;
    unordered_map<uint32, vector<EverQuestForageZoneItem>> ForageZoneItemsByMapID;
    unordered_map<uint32, uint32> ForageZoneItemTotalChanceByMapID;
    unordered_map<uint32, EverQuestZoneSafePoint> ZoneSafePointByMapID;
    unordered_map<uint32, EverQuestZone> ZoneByMapID;
    unordered_set<uint32> InstanceRaidLowMapIDs;
    unordered_map<uint32, uint32> OpenWorldMapIDByInstanceRaidLowMapID;
    unordered_map<ObjectGuid, EverQuestPlayerRaidLowInstanceState> RaidLowInstanceStateByPlayerGUID;
    unordered_map<uint32, EverQuestFaction> FactionsByFactionTemplateID;
    unordered_set<uint32> DefendCombatFactionTemplateIDs;
    unordered_map<uint32, EverQuestReputationFactionInfo> EQReputationFactionInfoByFactionID;
    unordered_map<ObjectGuid, EverQuestPlayerTempFactionBonus> TempFactionBonusByPlayerGUID;
    unordered_map<ObjectGuid, vector<uint32>> ForcedFactionReactionIDsByPlayerGUID;
    unordered_set<ObjectGuid> PlayersPendingTempFactionRecalculation;
    unordered_map<ObjectGuid, uint32> CorpseIllusionOriginalNativeDisplayByPlayerGUID;
    unordered_map<ObjectGuid, EverQuestPendingSummonRequest> PendingSummonRequestByTargetPlayerGUID;
    unordered_map<uint8, EverQuestClassMap> ClassMapByWOWClassID;

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
    void LoadCreatureSpawnPoints();
    bool ShouldDespawnCreatureDueToSpawnRestrictions(Creature* creature);
    ObjectGuid::LowType RollCycleSpawnCreatureGUID(const EverQuestCycleSpawnGroup& cycleSpawnGroup, uint32 excludedSpawnPointID, Map* map);
    void ProcessCycleSpawnForCreatureDeath(Creature* deadCreature);
    void ApplyRaidBossRespawnVariance(Creature* deadCreature);
    void UpdateCycleSpawns(Map* map, uint32 diff);
    void LoadCreatureKillSpawnData();
    void ResolveKillSpawnRespawnTargetSpawnPoints();
    void LoadCreatureEmoteData();
    bool DoCreatureEmoteEvent(Creature* creature, uint8 emoteEventType, Unit* target);
    void EmitCreatureEmote(Creature* creature, const EverQuestCreatureEmote& emote, Unit* target);
    void SendCreatureChatToAllPlayersOnMap(Creature* creature, ChatMsg chatMsg, const string& text);
    string FormatCreatureEmoteText(Creature* creature, Unit* target, const string& text);
    void SetupCreatureEmoteState(Creature* creature);
    void RemoveCreatureEmoteState(Creature* creature);
    void UpdateCreatureEmotes(Creature* creature, uint32 diff);
    void LoadCreatureMovementSoundData();
    void RemoveCreatureMovementSoundState(Creature* creature);
    void UpdateCreatureMovementSound(Creature* creature, uint32 diff);
    void ProcessKillSpawnsForCreatureEvent(Creature* eventCreature, Unit* otherUnit, uint8 triggerTypeID);
    void UpdateCreatureKillSpawnCombatWatch(Creature* creature, uint32 diff);
    void RemoveCreatureKillSpawnCombatWatchState(Creature* creature);
    void ResolveVulakRequiredDragonSpawnPoints();
    void SetVulakLocked(Creature* creature, bool locked);
    bool AreAllVulakRequiredDragonsDead(Map* map);
    void UpdateVulakLock(Creature* creature, uint32 diff);
    void RemoveVulakLockState(Creature* creature);
    void ProcessTriggeredQuestKillSpawnsForCreatureDeath(Creature* deadCreature, Unit* killer);
    void TriggerQuestKillSpawn(Map* map, const EverQuestQuestReaction& questReaction);
    void EnqueuePendingKillSpawnAction(Map* map, EverQuestPendingKillSpawnAction& action);
    void UpdatePendingKillSpawnActions(Map* map, uint32 diff);
    bool HasAliveCreatureWithEntryInMap(Map* map, uint32 creatureTemplateID, Creature* ignoreCreature);
    void ExecuteKillSpawnAction(Map* map, EverQuestPendingKillSpawnAction& action);
    void LoadCreatureOnkillReputations();
    const list<EverQuestCreatureOnkillReputation>& GetOnkillReputationsForCreatureTemplate(uint32 creatureTemplateID);
    void LoadItemTemplateData();
    uint32 GetNPCEquipItemTemplateIDForItemTemplate(uint32 itemTemplateID);
    uint32 GetWornEffectSpellIDForItemTemplate(uint32 itemTemplateID);
    bool IsItemEQClassAllowedForPlayer(Player* player, uint32 itemTemplateID);
    bool IsItemTemplateIDAnEQItemTemplateID(uint32 itemTemplateID);
    void LoadItemWoWToEQSwapData();
    bool TryGetGearSwapPlayerState(Player* player, bool& hideWoWGear, uint8& secondEQClassID);
    uint32 GetGearSwapItemTemplateIDForWornItem(uint32 wearingPlayerGUIDCounter, uint8 rolledEQClassID, uint8 fallbackEQClassID, uint8 equipSlot, uint32 itemTemplateID);
    void PatchVisibleGearFieldsInValuesUpdate(Player* wearingPlayer, ByteBuffer& valuesUpdateBuf, BuildValuesCachePosPointers& posPointers);
    void SetAuctionUsableFilterActiveForPlayer(ObjectGuid playerGUID, bool active);
    bool IsAuctionUsableFilterActiveForPlayer(ObjectGuid playerGUID);
    bool BuildEQClassFilteredAuctionListPacket(Player* player, WorldPacket const& packet, WorldPacket& filteredPacket);
    bool IsWornEffectSpell(uint32 spellID);
    void LoadSpellData();
    const EverQuestSpell& GetSpellDataForSpellID(uint32 spellID);
    void LoadIllusionDisplayData();
    bool IsIllusionFormSpell(uint32 spellID);
    uint64 GetIllusionDisplayLookupKey(uint32 formSpellID, uint32 bodySet, uint32 tintID, bool helmOn);
    bool TryGetIllusionDisplayID(uint32 formSpellID, uint32 bodySet, uint32 tintID, bool helmOn, uint32& displayIDOut);
    uint32 GetIllusionDisplayIDWithFallback(uint32 formSpellID, uint32 bodySet, uint32 tintID, bool helmOn);
    uint32 GetIllusionBodySetForEQArmorMaterial(uint32 eqArmorMaterial);
    void LoadIllusionFaceData();
    uint64 GetIllusionFaceLookupKey(uint32 baseDisplayID, uint32 faceIndex);
    uint32 GetIllusionFaceDisplayIDForPlayer(Player* player, uint32 baseDisplayID);
    uint32 GetIllusionGearDisplayIDForPlayer(Player* player, uint32 formSpellID);
    uint32 GetActiveShapeshiftModelIDForPlayer(Player* player);
    void ApplyIllusionGearDisplayIfChanged(Player* player, EverQuestPlayerIllusionState* illusionState);
    void ApplyIllusionGearDisplayOnFormAuraApply(Player* player, uint32 formSpellID);
    void HandleIllusionFormAuraRemove(Player* player, uint32 spellID);
    void RefreshIllusionGearDisplayForPlayer(Player* player);
    void UpdatePlayerIllusionGearDisplay(Player* player, uint32 diffInMS);
    void ClearIllusionTrackingForPlayer(ObjectGuid playerGUID);
    bool IsSpellBlockedByMinTargetLevel(uint32 spellID, Unit* target, Unit* caster);
    bool IsSpellBlockedByMaxCreatureTargetLevel(uint32 spellID, Unit* target, Unit* caster);
    bool IsCreatureCharmBlockedByCharmLimits(uint32 spellID, Unit* target, Unit* caster);
    bool ApplyBardSongFearDiminishingReturnsOnAuraApply(Unit* target, Aura* aura);
    void RemoveCreatureFearDiminishingReturnState(Creature* creature);
    uint64 GetHasteTrackingKeyForUnit(Unit* unit);
    void TrackEQHasteAurasAndEnforceCapOnAuraApply(Unit* unit, Aura* aura);
    void UntrackEQHasteAurasAndEnforceCapOnAuraRemove(Unit* unit, Aura* aura);
    void EnforceEQHastePercentCapOnUnit(Unit* unit, vector<EverQuestUnitHasteAuraEffect>& trackedHasteAuraEffects);
    float GetEQHasteCapPercentForUnit(Unit* unit);
    uint32 GetEquippedShieldBaseArmorForPlayer(Player* player);
    void RefreshBearFormShieldArmorShiftForPlayer(Player* player);
    void ClearBearFormShieldArmorShiftForPlayer(ObjectGuid playerGUID);
    uint32 GetAgileFighterCombatAuraSpellIDForPlayer(Player* player);
    void RefreshAgileFighterCombatAuraForPlayer(Player* player);
    void ReapplyAgileFighterCombatAuraForPlayer(Player* player);
    void UpdateAgileFighterCombatAura(Player* player, uint32 diffInMS);
    void ClearAgileFighterTrackingForPlayer(ObjectGuid playerGUID);
    void LoadQuestCompletionReputations();
    const list<EverQuestQuestCompletionReputation>& GetQuestCompletionReputationsForQuestTemplate(uint32 questTemplateID);
    void LoadQuestReactions();
    const list<EverQuestQuestReaction>& GetQuestReactions(uint32 questTemplateID);
    void LoadGossipReactions();
    bool HandleGossipHello(Player* player, Creature* creature);
    bool HandleGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action);
    string FormatGossipTextForPlayer(Player* player, const string& text);
    void LoadPetData();
    void LoadPetSilentDisplayData();
    void RemoveInvalidPetSilentDisplays();
    uint32 GetSilentFidgetDisplayIDForDisplayID(uint32 displayID) const;
    void UpdatePetFidgetSilence(Creature* creature);
    bool HasPetDataForCreatureTemplateID(uint32 creatureTemplateID);
    const EverQuestPet& GetPetDataForCreatureTemplateID(uint32 creatureTemplateID);
    void FixInvalidCharacterPetModelIDs();
    void LoadCreatePlayerData();
    bool HasCreatePlayerData(uint8 raceID, uint8 classID);
    const EverQuestPlayerCreateInfo& GetPlayerCreateInfo(uint8 raceID, uint8 classID);
    void LoadAutoLearnSkillsData();
    const list<uint32>& GetAutoLearnSkillsForClass(uint8 classID);
    void LoadAutoLearnSpellsData();
    const list<EverQuestAutoLearnSpell>& GetAutoLearnSpellsForClass(uint8 classID);
    void ApplyAutoLearnedClassSkillsAndSpells(Player* player);
    void GrantDeathKnightStarterAbilitiesIfNeeded(Player* player);
    void AddHearthstoneForNewCharacter(Player* player);
    bool IsItemTemplateAMasterTotem(Player* player, ItemTemplate const* itemTemplate);
    bool IsPlayerCarryingMasterTotem(Player* player);
    void AddMasterTotemForShaman(Player* player);
    void AddRacialGuiseItemForPlayer(Player* player);
    void ApplyCorpseIllusionNativeDisplayOnDeath(Player* player);
    void RestoreNativeDisplayAfterCorpseIllusion(Player* player);
    void GrantLegacyAchievementIfEligible(Player* player);
    void AddAdventurerAuraForNewCharacter(Player* player);
    void GrantAdventurerAuraOnLoginIfMissing(Player* player);
    void PersistAdventurerAuraOnPlayerSave(Player* player);
    bool IsMapIDAnEverQuestMap(uint32 mapID);
    bool IsZoneWideGroupRewardEnabledForMap(uint32 mapID);
    bool IsInZoneWideGroupRewardRange(Player* member, WorldObject* rewardSource);
    uint8 GetPlayerLevelForExperienceGain(Player* player);
    void BuildZoneWideKillReward(Group* group, Player* killer, Unit* victim, EverQuestZoneWideKillReward& outReward);
    float GetZoneWideGroupExperienceRate(Player* player, const EverQuestZoneWideKillReward& reward);
    float GetGroupExperienceRateForMember(Player* member, const EverQuestZoneWideKillReward& reward);
    void ApplyEQOnkillReputationsForPlayer(Player* player, Unit* victim);
    void GrantZoneWideGroupRewardsForKill(Player* killer, Unit* victim, const EverQuestZoneWideKillReward& reward);
    void ApplyZoneWideGroupLootAccess(Loot* loot, Player* lootOwner, bool personal);
    void ApplyZoneWideGroupMoneyShare(Player* looter, Loot* loot);
    bool IsCreatureKillOutsideEverQuestForAdventurer(Unit* victim);
    bool IsQuestOutsideEverQuestForAdventurer(uint32 questID);
    bool RevokeAdventurerAuraIfPresent(Player* player);
    void GrantAdventurerAchievementIfAccountEarned(Player* player);
    void ProcessAdventurerStateOnLevelChange(Player* player);
    void LoadCreatureLootData();
    bool HasCreatureLootDataForCreatureTemplateEntryID(uint32 creatureTemplateEntryID);
    bool HasPreloadedLootItemIDsForCreatureGUID(Map* map, ObjectGuid creatureGUID);
    bool HasPreloadedLootItemIDForCreatureGUID(Map* map, ObjectGuid creatureGUID, uint32 itemTemplateID);
    uint32 GetPreloadedLootCountForCreatureGUID(Map* map, ObjectGuid creatureGUID, uint32 itemTemplateID);
    const vector<uint32>& GetPreloadedLootIDsForCreatureGUID(Map* map, ObjectGuid creatureGUID);
    void ClearPreloadedLootIDsForCreatureGUID(Map* map, ObjectGuid creatureGUID);
    void TrackVisualEquippedItemsForCreatureGUID(Map* map, ObjectGuid creatureGUID, uint32 mainhandItemID, uint32 offhandItemID, bool isDualWielding);
    void ClearVisualEquippedItemsForCreatureGUID(Map* map, ObjectGuid creatureGUID);
    bool IsCreatureDualWielding(Map* map, ObjectGuid creatureGUID);
    uint32 GetEQNPCMeleeWeaponSkillForLevel(uint32 level);
    void TryDoCreatureEQMeleeExtraAttacks(Unit* attacker, Unit* victim);
    void StoreCreatureRangedAttackState(Creature* creature, float minRange, float maxRange, int32 damageModPct);
    void RemoveCreatureRangedAttackState(Creature* creature);
    void UpdateCreatureRangedAttack(Creature* creature, uint32 diff);
    void SetupCreatureCombatAbilities(Creature* creature);
    void RemoveCreatureCombatAbilityState(Creature* creature);
    void UpdateCreatureCombatAbilities(Creature* creature, uint32 diff);
    void UpdateCreatureEnrage(Creature* creature, EverQuestCreatureCombatAbilityState* state, uint32 diff);
    void UpdateCreatureSpecialAttacks(Creature* creature, EverQuestCreatureCombatAbilityState* state, uint32 diff);
    void SetupCreatureSummon(Creature* creature);
    void RemoveCreatureSummonState(Creature* creature);
    void UpdateCreatureSummon(Creature* creature, uint32 diff);
    void DoCreatureCombatAbilitySwingRound(Creature* creature, Unit* target, uint32 damagePct);
    void DoCreatureFlurry(Creature* creature, Unit* victim);
    void DoCreatureRampage(Creature* creature, Unit* victim, float range, uint32 damagePct);
    void DoCreatureWildRampage(Creature* creature, Unit* victim, uint32 maxTargets, uint32 damagePct);
    bool IsCreatureEnragedForRiposte(Unit const* unit, Unit const* attacker);
    void TryDoCreatureEnrageRiposteCounter(Unit* victim, Unit* attacker);
    void ApplyCreatureCombatAbilityDamageMod(Unit* attacker, uint32& damage);
    void RemoveCreatureUnstickState(Creature* creature);
    void CalculateUnstickTeleportPosition(Creature* creature, Unit* victim, float& xOut, float& yOut, float& zOut);
    void UpdateCreatureUnstick(Creature* creature, uint32 diff);
    void UpdateNonEQCreatureLeash(Creature* creature);
    bool TryGetCustomSocialAggroScale(Creature* creature, float& scaleOut);
    void DoScaledSocialAggroSearch(Creature* caller, Unit* victim, float scale, float maxAgroZDistance);
    void ApplyScaledCreatureSocialAggroOnEngage(Creature* creature, Unit* victim);
    void ProcessCreatureRetaliationOnDamage(Unit* attacker, Unit* victim);
    void RemoveCreatureCrowdControlAurasFromPlayersOnDeath(Creature* deadCreature);
    void UpdateCreatureScaledSocialAggro(Creature* creature, uint32 diff);
    void RemoveCreatureSocialAggroState(Creature* creature);
    float GetMaxAgroZDistanceForMap(uint32 mapID);
    bool IsBlockedByAgroZDistance(WorldObject const* source, WorldObject const* target, float maxAgroZDistance);
    bool IsSocialAggroOverrideNeededForCreature(Creature* creature, float& scaleOut, float& maxAgroZDistanceOut);
    void MarkCreatureAgroZBlockOnEngage(Creature* creature, Unit* victim);
    void UpdateCreatureAgroZBlock(Creature* creature, uint32 diff);
    void RemoveCreatureAgroZBlockState(Creature* creature);
    bool ShouldBlockCreatureInitialAgroOnPet(Unit const* unit, Unit const* target);
    void StoreCreatureAggroPosition(Creature* creature);
    void RemoveCreatureAggroPositionState(Creature* creature);
    void TeleportCreatureToLastAggroPosition(Creature* creature, uint32 gateSpellID);
    void RemoveVisualEquippedItemForCreatureGUIDIfExists(Map* map, ObjectGuid creatureGUID, uint32 itemTemplateID);
    void LoadShipTriggerData();
    const vector<EverQuestTransportShipTrigger>& GetShipTriggersForShip(int triggeringGameObjectTemplateEntryID);
    void LoadCreatureInstanceData();
    const EverQuestCreatureInstance& GetCreatureInstanceData(uint32 creatureInstanceGUID);
    void LoadCreatureWaypointData();
    const vector<EverQuestCreatureWaypoint>& GetWaypoints(uint32 mapID, uint32 waypointListID);
    void LoadForageData();
    const vector<EverQuestForageZoneItem>& GetForageZoneItemsInMap(uint32 mapID);
    void LoadZoneSafePointData();
    void SendPlayerToZoneSafePoint(Player* player, bool includeGroup);
    void LoadZoneData();
    uint32 GetInstanceRaidLowMapIDForMap(uint32 mapID);
    uint32 GetOpenWorldMapIDForMapID(uint32 mapID);
    bool IsMapInstanceRaidLow(uint32 mapID);
    void UpdateRaidLowInstanceStateForPlayer(Player* player);
    void ClearRaidLowInstanceStateForPlayer(ObjectGuid playerGUID);
    bool HasOccupiedRaidLowInstanceForMap(ObjectGuid playerGUID, uint32 raidLowMapID);
    bool ShouldZoneLineEnterInstanceRaidLow(Player* player, uint32 raidLowMapID);
    bool TryZoneLineIntoInstanceRaidLow(Player* player, AreaTrigger const* trigger);
    bool IsBindAllowedForMap(uint32 mapID);
    bool IsMapRestrictedByExpansion(uint32 mapID);
    bool IsMapRestrictedForPlayers(uint32 mapID);
    bool RelocatePlayerOutOfRestrictedMap(Player* player);
    void UpdateRestrictedMapPlayerCheck(uint32 diff);
    void BeginClientVersionCheckForPlayer(Player* player);
    void HandleClientVersionReportForPlayer(Player* player, uint32 reportedVersion);
    void FailClientVersionCheckForPlayer(Player* player, EverQuestPlayerClientVersionCheckState& checkState);
    void UpdateClientVersionChecks(uint32 diff);
    void ClearClientVersionCheckForPlayer(ObjectGuid playerGUID);
    void LoadFactionData();
    void ResolveDefendCombatFactionTemplates();
    void ResolveEQReputationFactions();
    void HandleModFactionAuraApplyOnCreature(Creature* creature, Aura* aura);
    void HandleModFactionAuraRemoveFromCreature(Creature* creature, AuraApplication* aurApp);
    void RecalculateTemporaryFactionReactionsForPlayer(Player* player);
    void QueueTemporaryFactionRecalculationForPlayer(ObjectGuid playerGUID);
    void ConsumePendingTemporaryFactionRecalculation(Player* player);
    uint8 GetPlayerBaselineFactionAlignment(Player* player);
    void GetIllusionFactionBandSteps(uint8 playerAlignment, uint8 illusionAlignment, int32& stepsTowardGoodOut, int32& stepsTowardEvilOut);
    void ClearTemporaryFactionStateForPlayer(ObjectGuid playerGUID);
    void ClearTempFactionBonusForPlayer(Player* player);
    void UpdateCreatureDefendFriendlyPlayers(Creature* creature, uint32 diff);
    bool IsPlayerFriendlyWithCreatureByReputation(Creature* creature, Player* player);
    void DoDefendFriendlyPlayersSearch(Creature* attacker, Player* attackedPlayer);
    void RemoveCreatureDefendPlayerWatchState(Creature* creature);
    void UpdateCreatureDefendFactionRestore(Creature* creature);
    void LoadClassMapData();
    const EverQuestClassMap& GetClassMapForWOWClassID(uint8 wowClassID);
    bool IsEQClassABaseEQClass(uint8 eqClassID);
    bool DoesPlayerHaveEQClassOfWOWClass(Player* player, uint8 wowClassID);

    void StorePositionAsLastGate(Player* player);
    void SendPlayerToLastGate(Player* player);
    bool TryGetEQBindHomePosition(Player* player, uint32& mapIDOut, float& xOut, float& yOut, float& zOut);
    void SendPlayerToEQBindHome(Player* player);
    void SetNewBindHome(Player* player);
    void SetNewBindHome(Player* player, uint32 playerGUIDCounter, int mapID, int zoneID, float playerX, float playerY, float playerZ);
    void DeletePlayerBindHome(ObjectGuid guid);
    uint64 GetMapInstanceKey(Map* map);
    void AddCreatureAsLoaded(Creature* creature);
    void RemoveCreatureAsLoaded(Creature* creature);
    vector<Creature*> GetLoadedCreaturesWithEntryID(Map* map, uint32 entryID);
    void RollLootItemsForCreature(Creature* creature);
    void RollLootGroupIntoCounts(const EverQuestCreatureLootGroup& lootGroup, unordered_map<uint32, uint32>& counts);
    void SpawnCreature(uint32 entryID, Map* map, float x, float y, float z, float orientation, bool enforceUniqueSpawn);
    void DespawnCreature(uint32 entryID, Map* map);
    void MakeCreatureAttackPlayer(uint32 entryID, Map* map, Player* player);
    bool IsSpellAnEQSpell(uint32 spellID);
    bool IsSpellAnEQBardSong(uint32 spellID);
    bool RollBashKickStunLands(Unit* attacker, Unit* defender);
    uint32 CalculateSpellFocusBoostValue(Unit* caster, uint32 spellID);
    void ProcessForage(Player* player);
    bool IsSummonPlayerSpellBlockedByTarget(uint32 spellID, Unit* target, Unit* caster);
    void ProcessSummonPlayerToCaster(Player* caster, Unit* target);
    Player* ResolveSummonPlayerTarget(Player* caster, Unit* target);
    void SendSummonRequestToPlayer(Player* targetPlayer, ObjectGuid summonerGUID, uint32 summonerZoneID, uint32 mapID, float x, float y, float z);
    void QueueCrossZoneSummonRequest(Player* caster, Player* targetPlayer);
    void ConsumePendingSummonRequest(Player* player);
    void ClearPendingSummonRequestForPlayer(ObjectGuid playerGUID);

    float GetTrackingRangeForEQClassAtLevel(uint8 eqClassID, uint8 level);
    float GetTrackingMaxDistanceForPlayer(Player* player);
    void HandleTrackingRangeChangeForPlayer(Player* player);
    void SendTrackingAddonMessageToPlayer(Player* player, const std::string& payload);
    void SendTrackingListToPlayer(Player* player);
    void StartTrackingForPlayer(Player* player, uint64 rawCreatureGUID);
    void StopTrackingForPlayer(Player* player, bool sendMessage);
    void UpdatePlayerTracking(Player* player, uint32 diffInMS);

    uint8 GetCurrentSecondEQClassForPlayer(Player* player);
    uint8 GetNextSecondEQClassForPlayer(Player* player);
    void SetNextSecondEQClassForPlayer(Player* player, uint8 nextEQClass);
    void SendClassInfoAddonMessageToPlayer(Player* player);
    uint32 GetSecondaryExpPoolForPlayer(Player* player);
    uint32 AddToSecondaryExpPoolForPlayer(Player* player, uint32 grantedExp);
    uint32 SpendSecondaryExpPoolForPlayer(Player* player);
    void SaveSecondaryExpPoolForPlayer(Player* player);
    uint32 GetIllusionFaceIDForPlayer(Player* player);
    void SetIllusionFaceIDForPlayer(Player* player, uint32 faceID);
    void SaveIllusionFaceIDForPlayer(Player* player);
    bool GetShowBardPulseForPlayer(Player* player);
    void SetShowBardPulseForPlayer(Player* player, bool showBardPulse);
    void SaveShowBardPulseForPlayer(Player* player);
    bool GetHideWoWGearForPlayer(Player* player);
    void SetHideWoWGearForPlayer(Player* player, bool hideWoWGear);
    void SaveHideWoWGearForPlayer(Player* player);
    void ResendVisibleGearOfNearbyPlayersToPlayer(Player* player);
    uint32 GetIssuedIllusionItemIDForPlayer(Player* player);
    void SetIssuedIllusionItemIDForPlayer(Player* player, uint32 itemID);
    void SaveIssuedIllusionItemIDForPlayer(Player* player);
    void HandleLevelCapOnBeforeExperienceGain(Player const* player, uint8& levelForExpGain);
    bool HandleLevelCapOnCanGiveLevel(Player* player, uint8 newLevel);
    void ProcessLevelCapStateForPlayer(Player* player);
    void SendExpPoolAddonMessageToPlayer(Player* player, uint32 gainedExp);
    void SetInitialEQClassesForPlayer(Player* player);
    void SetInitialCreatePositionForPlayer(Player* player);
    EverQuestPlayerControllerData GetPlayerControllerData(Player* player);
    EverQuestPlayerControllerData* GetOrLoadActivePlayerClassControllerData(Player* player);

    std::map<std::string, EverQuestPlayerClassInfoItem> GetPlayerClassInfoByClassNameForPlayer(Player* player);
    std::map<uint8, uint8> GetClassLevelsByClassForPlayer(Player* player);

    bool DoesSavedClassDataExistForPlayer(Player* player, uint8 lookupClass);
    void CopyCharacterDataIntoModCharacterTable(Player* player, CharacterDatabaseTransaction& transaction);
    void MoveTalentsToModTalentsTable(Player* player, CharacterDatabaseTransaction& transaction);
    void MoveClassSpellsToModSpellsTable(Player* player, CharacterDatabaseTransaction& transaction);
    void EnsureCrossClassExemptSpellIDsBuilt();
    bool IsRacialSkillID(uint32 skillID);
    bool IsDeathKnightSkillID(uint32 skillID);
    bool IsSpellExemptFromClassMove(uint32 spellID);
    bool IsSkillExemptFromClassMove(uint32 skillID);
    void MoveClassSkillsToModSkillsTable(Player* player, CharacterDatabaseTransaction& transaction);
    void ReplaceModClassActionCopy(Player* player, CharacterDatabaseTransaction& transaction);
    void MoveGlyphsToModGlyhpsTable(Player* player, CharacterDatabaseTransaction& transaction);
    void MoveAuraToModAuraTable(Player* player, CharacterDatabaseTransaction& transaction);
    void MoveEquipToModInventoryTable(Player* player, CharacterDatabaseTransaction& transaction);
    void MoveQuestDataToModQuestTables(Player* player, CharacterDatabaseTransaction& transaction);

    void UpdateCharacterFromModCharacterTable(Player* player, uint8 pullEQClassID, CharacterDatabaseTransaction& transaction);
    void CopyModSpellTableIntoCharacterSpells(Player* player, uint8 pullEQClassID, CharacterDatabaseTransaction& transaction);
    void CopyModActionTableIntoCharacterAction(Player* player, uint8 pullEQClassID, CharacterDatabaseTransaction& transaction);
    void CopyModSkillTableIntoCharacterSkills(Player* player, uint8 pullEQClassID, CharacterDatabaseTransaction& transaction);
    void CopyModQuestTablesIntoCharacterQuests(Player* player, uint8 pullEQClassID, CharacterDatabaseTransaction& transaction);
    void UpdatePlayerControllerForClassChange(Player* player, uint8 newEQClassID, CharacterDatabaseTransaction& transaction);

    std::map<uint8, EverQuestPlayerEquipedItemData> GetVisibleItemsBySlotForPlayerClass(Player* player, uint8 classID);
    bool IsEQClassValidEquipmentStorageTargetForPlayer(Player* player, uint8 eqClassID);
    bool IsEquipmentStorageCommitPendingForPlayer(Player* player);
    void SetEquipmentStorageCommitPendingForPlayerGUID(ObjectGuid playerGUID, bool pending);
    void QueuePendingEquipmentStorageTransaction(Player* player, uint8 eqClassID, CharacterDatabaseTransaction& transaction);
    void ProcessPendingEquipmentStorageTransactions();
    void WaitForPendingEquipmentStorageCommitForPlayer(ObjectGuid playerGUID);
    bool IsItemEQClassAllowedForPlayerSecondaryClass(Player* player, uint8 eqClassID, uint32 itemTemplateID);
    void AppendCharacterRowLockAnchor(CharacterDatabaseTransaction& transaction, uint32 playerGUIDCounter);
    Item* LoadDetachedItemForPlayer(uint32 itemGUIDCounter, Player* player);
    bool EquipItemIntoSecondaryClassStorage(Player* player, uint8 eqClassID, uint8 clientBagID, uint8 clientSlotID, uint8 equipSlot, uint32 expectedItemTemplateID, std::string& errorTextOut);
    bool RemoveItemFromSecondaryClassStorage(Player* player, uint8 eqClassID, uint8 equipSlot, uint8 clientBagID, uint8 clientSlotID, bool useSpecificBagPosition, std::string& errorTextOut);
    bool MoveItemWithinSecondaryClassStorage(Player* player, uint8 eqClassID, uint8 fromEquipSlot, uint8 toEquipSlot, std::string& errorTextOut);
    bool SwapSecondaryClassStorageItemWithLiveEquipment(Player* player, uint8 eqClassID, uint8 storageEquipSlot, uint8 liveEquipSlot, std::string& errorTextOut);
    void SendClassEquipmentAddonMessageToPlayer(Player* player, uint8 eqClassID);
    bool PerformClassSwitch(Player* player);
    bool PerformPlayerDelete(ObjectGuid guid);
};

std::string GetEQClassStringFromID(uint8 classID);
std::string GetEQClassCommandNameFromID(uint8 classID);
std::set<uint32> GetSetFromConfigString(string configStringName);

#define EverQuest EverQuestMod::instance()

#endif //EVERQUEST
