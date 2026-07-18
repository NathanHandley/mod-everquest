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
#include "DataMap.h"
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

#define EQ_MOD_VERSION                              51

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

#define EQ_DAZE_SPELL_ID                            1604   // Pre-defined by the WoW core

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
#define EQ_CREATURE_CUSTOMDATA_UNSTICK              "EQUnstick"
#define EQ_CREATURE_CUSTOMDATA_SOCIALAGGRO          "EQSocialAggro"
#define EQ_CREATURE_CUSTOMDATA_EMOTE                "EQEmote"
#define EQ_CREATURE_CUSTOMDATA_MOVEMENTSOUND        "EQMoveSound"
#define EQ_CREATURE_CUSTOMDATA_KILLSPAWNWATCH       "EQKillSpawnWatch"
#define EQ_CREATURE_CUSTOMDATA_VULAKLOCK            "EQVulakLock"
#define EQ_CREATURE_CUSTOMDATA_DEFENDPLAYERWATCH    "EQDefendPlayerWatch"

#define EQ_DEFEND_PLAYERS_CHECK_MS                  2000
#define EQ_DEFEND_PLAYERS_SEARCH_RADIUS             30.0f

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
    vector<ObjectGuid::LowType> TargetSpawnIDs;
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

class EverQuestPlayerIllusionState
{
public:
    uint32 FormSpellID = 0;
    uint32 RefreshTimerMS = 0;
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
};

class EverQuestFaction
{
public:
    uint32 FactionTemplateID = 0;
    bool WillDefendFriendlyPlayers = false;
    bool DefendersWillAttackToDefendPlayer = false;
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
    uint32 ConfigSystemLegacyAchievementID;
    string ConfigSystemLegacyAchievementAccountCreatedBefore;

    // Configs (from server file)
    bool ConfigMapRestrictPlayersToNorrath;
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
    bool ConfigEvadeEnabled;
    float ConfigEvadeUnreachableSeconds;
    float ConfigEvadeUnstickStallSeconds;
    float ConfigEvadeUnstickSettleSeconds;
    float ConfigEvadeUnstickMoveThreshold;
    uint32 ConfigEvadeUnstickMaxAttempts;
    uint32 ConfigEvadeUnstickStepPercent;
    bool ConfigCharmCreatureCharmLimitsEnabled;
    float ConfigCharmUncharmedPlayerCheckRadius;
    bool ConfigCreatureEmotesEnabled;
    bool ConfigCreatureEmotesAmbientEnabled;
    bool ConfigCreatureMovementSoundsEnabled;
    uint32 ConfigIllusionGearRefreshTimeInMS;
    bool ConfigShowClassMessageOnLogin;
    float ConfigSecondaryExpPoolGainPercent;
    uint32 ConfigSecondaryExpPoolMaxPooled;
    uint32 ConfigPlayerLevelCap;
    std::set<uint32> ConfigCrossClassIncludeSkillIDs;

    unordered_set<uint32> CrossClassExemptSpellIDs;
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
    vector<ObjectGuid::LowType> VulakRequiredDragonSpawnIDs;
    unordered_map<uint32, vector<EverQuestCreatureEmote>> CreatureEmotesByCreatureTemplateID;
    unordered_map<uint32, EverQuestCreatureMovementSound> CreatureMovementSoundsByDisplayID;

    std::mutex PendingKillSpawnActionsMutex;
    unordered_map<uint32, vector<EverQuestPendingKillSpawnAction>> PendingKillSpawnActionsByMapID;
    unordered_map<uint32, vector<EverQuestTriggeredQuestKillSpawn>> TriggeredQuestKillSpawnsByMapID;
    unordered_map<uint32, EverQuestItemTemplate> ItemTemplatesByEntryID;
    unordered_set<uint32> WornEffectSpellIDs;
    unordered_map<uint32, EverQuestSpell> SpellDataBySpellID;
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
    unordered_map<uint8, list<uint32>> PlayerAutoAddItemsByEQClassID;
    unordered_map<int, unordered_map<int, vector<Creature*>>> AllLoadedCreaturesByMapIDThenCreatureEntryID;
    unordered_map<uint32, EverQuestCreatureSpawnPoint> CreatureSpawnPointsByCreatureGUID;
    unordered_map<int, unordered_map<uint32, vector<Creature*>>> AllLoadedCreaturesByMapIDThenSpawnPointID;
    unordered_map<int, unordered_map<uint32, vector<Creature*>>> AllLoadedCreaturesByMapIDThenSpawnGroupID;
    unordered_map<uint32, unordered_map<uint32, EverQuestCycleSpawnGroup>> CycleSpawnGroupsByMapIDThenSpawnGroupID;
    unordered_map<uint32, int32> CycleSpawnCheckTimerInMSByMapID;
    unordered_map<ObjectGuid, deque<uint32>> PlayerCasterConcurrentBardSongs;
    unordered_set<ObjectGuid> PlayersGainingExperience;
    unordered_set<ObjectGuid> PlayersPendingLevelCapExperiencePark;
    unordered_map<ObjectGuid, vector<EverQuestUnitHasteAuraEffect>> EQHasteAuraEffectsByUnitGUID;
    unordered_map<uint32, vector<EverQuestCreatureLootGroup>> CreatureLootGroupsByCreatureTemplateID;
    unordered_map<ObjectGuid, vector<uint32>> PreloadedLootItemIDsByCreatureGUID;
    unordered_map<ObjectGuid, unordered_map<uint32, uint32>> PreloadedLootCountsByCreatureGUID;
    unordered_map<ObjectGuid, EverQuestLoadedCreatureEquippedVisualItems> VisualEquippedItemsByCreatureGUID;
    unordered_set<ObjectGuid> CreaturesResolvingEQMeleeExtraAttacks;
    unordered_map<uint32, vector<EverQuestTransportShipTrigger>> ShipTriggersByTriggeringGameObjectTemplateEntryID;
    unordered_map<uint32, int> ShipWaitNodesByGameObjectTemplateEntryID;
    unordered_map<uint32, GameObject*> ShipGameObjectsByTemplateEntryID;
    unordered_map<uint32, EverQuestCreatureInstance> CreatureInstancesByCreatureGUID;
    unordered_map<uint32, unordered_map<uint32, vector<EverQuestCreatureWaypoint>>> CreatureWaypointsByMapIDAndWaypointID;
    unordered_map<uint32, vector<EverQuestForageZoneItem>> ForageZoneItemsByMapID;
    unordered_map<uint32, uint32> ForageZoneItemTotalChanceByMapID;
    unordered_map<uint32, EverQuestZoneSafePoint> ZoneSafePointByMapID;
    unordered_map<uint32, EverQuestZone> ZoneByMapID;
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
    bool ShouldDespawnCreatureDueToSpawnRestrictions(int mapID, Creature* creature);
    ObjectGuid::LowType RollCycleSpawnCreatureGUID(const EverQuestCycleSpawnGroup& cycleSpawnGroup, uint32 excludedSpawnPointID, uint32 mapID);
    void ProcessCycleSpawnForCreatureDeath(Creature* deadCreature);
    void UpdateCycleSpawns(Map* map, uint32 diff);
    void LoadCreatureKillSpawnData();
    void ResolveKillSpawnRespawnTargetSpawnPoints();
    void LoadCreatureEmoteData();
    bool DoCreatureEmoteEvent(Creature* creature, uint8 emoteEventType, Unit* target);
    void EmitCreatureEmote(Creature* creature, const EverQuestCreatureEmote& emote, Unit* target);
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
    void TriggerQuestKillSpawn(uint32 mapID, const EverQuestQuestReaction& questReaction);
    void EnqueuePendingKillSpawnAction(uint32 mapID, EverQuestPendingKillSpawnAction& action);
    void UpdatePendingKillSpawnActions(Map* map, uint32 diff);
    bool HasAliveCreatureWithEntryInMap(uint32 mapID, uint32 creatureTemplateID, Creature* ignoreCreature);
    void ExecuteKillSpawnAction(Map* map, EverQuestPendingKillSpawnAction& action);
    void LoadCreatureOnkillReputations();
    const list<EverQuestCreatureOnkillReputation>& GetOnkillReputationsForCreatureTemplate(uint32 creatureTemplateID);
    void LoadItemTemplateData();
    uint32 GetNPCEquipItemTemplateIDForItemTemplate(uint32 itemTemplateID);
    uint32 GetWornEffectSpellIDForItemTemplate(uint32 itemTemplateID);
    bool IsItemEQClassAllowedForPlayer(Player* player, uint32 itemTemplateID);
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
    void ApplyIllusionGearDisplayIfChanged(Player* player, EverQuestPlayerIllusionState* illusionState);
    void ApplyIllusionGearDisplayOnFormAuraApply(Player* player, uint32 formSpellID);
    void HandleIllusionFormAuraRemove(Player* player, uint32 spellID);
    void RefreshIllusionGearDisplayForPlayer(Player* player);
    void UpdatePlayerIllusionGearDisplay(Player* player, uint32 diffInMS);
    void ClearIllusionTrackingForPlayer(ObjectGuid playerGUID);
    bool IsSpellBlockedByMinTargetLevel(uint32 spellID, Unit* target, Unit* caster);
    bool IsSpellBlockedByMaxCreatureTargetLevel(uint32 spellID, Unit* target, Unit* caster);
    bool IsCreatureCharmBlockedByCharmLimits(uint32 spellID, Unit* target, Unit* caster);
    void TrackEQHasteAurasAndEnforceCapOnAuraApply(Unit* unit, Aura* aura);
    void UntrackEQHasteAurasAndEnforceCapOnAuraRemove(Unit* unit, Aura* aura);
    void EnforceEQHastePercentCapOnUnit(Unit* unit, vector<EverQuestUnitHasteAuraEffect>& trackedHasteAuraEffects);
    void LoadQuestCompletionReputations();
    const list<EverQuestQuestCompletionReputation>& GetQuestCompletionReputationsForQuestTemplate(uint32 questTemplateID);
    void LoadQuestReactions();
    const list<EverQuestQuestReaction>& GetQuestReactions(uint32 questTemplateID);
    void LoadGossipReactions();
    bool HandleGossipHello(Player* player, Creature* creature);
    bool HandleGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action);
    string FormatGossipTextForPlayer(Player* player, const string& text);
    void LoadPetData();
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
    void LoadAutoAddItemsData();
    const list<uint32>& GetAutoAddItemsForClass(uint8 classID);
    void ApplyAutoAddedClassItems(Player* player);
    void GrantLegacyAchievementIfEligible(Player* player);
    void LoadCreatureLootData();
    bool HasCreatureLootDataForCreatureTemplateEntryID(uint32 creatureTemplateEntryID);
    bool HasPreloadedLootItemIDsForCreatureGUID(ObjectGuid creatureGUID);
    bool HasPreloadedLootItemIDForCreatureGUID(ObjectGuid creatureGUID, uint32 itemTemplateID);
    uint32 GetPreloadedLootCountForCreatureGUID(ObjectGuid creatureGUID, uint32 itemTemplateID);
    const vector<uint32>& GetPreloadedLootIDsForCreatureGUID(ObjectGuid creatureGUID);
    void ClearPreloadedLootIDsForCreatureGUID(ObjectGuid creatureGUID);
    void TrackVisualEquippedItemsForCreatureGUID(ObjectGuid creatureGUID, uint32 mainhandItemID, uint32 offhandItemID, bool isDualWielding);
    void ClearVisualEquippedItemsForCreatureGUID(ObjectGuid creatureGUID);
    bool IsCreatureDualWielding(ObjectGuid creatureGUID);
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
    bool TryGetCustomSocialAggroScale(Creature* creature, float& scaleOut);
    void DoScaledSocialAggroSearch(Creature* caller, Unit* victim, float scale);
    void ApplyScaledCreatureSocialAggroOnEngage(Creature* creature, Unit* victim);
    void UpdateCreatureScaledSocialAggro(Creature* creature, uint32 diff);
    void RemoveCreatureSocialAggroState(Creature* creature);
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
    bool IsBindAllowedForMap(uint32 mapID);
    void LoadFactionData();
    void UpdateCreatureDefendFriendlyPlayers(Creature* creature, uint32 diff);
    void DoDefendFriendlyPlayersSearch(Creature* attacker, Player* attackedPlayer);
    void RemoveCreatureDefendPlayerWatchState(Creature* creature);
    void LoadClassMapData();
    const EverQuestClassMap& GetClassMapForWOWClassID(uint8 wowClassID);
    bool IsEQClassABaseEQClass(uint8 eqClassID);

    void StorePositionAsLastGate(Player* player);
    void SendPlayerToLastGate(Player* player);
    void SendPlayerToEQBindHome(Player* player);
    void SetNewBindHome(Player* player);
    void SetNewBindHome(Player* player, uint32 playerGUIDCounter, int mapID, int zoneID, float playerX, float playerY, float playerZ);
    void DeletePlayerBindHome(ObjectGuid guid);
    void AddCreatureAsLoaded(int mapID, Creature* creature);
    void RemoveCreatureAsLoaded(int mapID, Creature* creature);
    vector<Creature*> GetLoadedCreaturesWithEntryID(int mapID, uint32 entryID);
    void RollLootItemsForCreature(ObjectGuid creatureGUID, uint32 creatureTemplateEntryID);
    void RollLootGroupIntoCounts(const EverQuestCreatureLootGroup& lootGroup, unordered_map<uint32, uint32>& counts);
    void SpawnCreature(uint32 entryID, Map* map, float x, float y, float z, float orientation, bool enforceUniqueSpawn);
    void DespawnCreature(uint32 entryID, Map* map);
    void MakeCreatureAttackPlayer(uint32 entryID, Map* map, Player* player);
    bool IsSpellAnEQSpell(uint32 spellID);
    bool IsSpellAnEQBardSong(uint32 spellID);
    bool RollBashKickStunLands(Unit* attacker, Unit* defender);
    uint32 CalculateSpellFocusBoostValue(Unit* caster, uint32 spellID);
    void ProcessForage(Player* player);

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
    bool IsSpellExemptFromClassMove(uint32 spellID);
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
    bool PerformClassSwitch(Player* player);
    bool PerformPlayerDelete(ObjectGuid guid);
};

std::string GetEQClassStringFromID(uint8 classID);
std::string GetEQClassCommandNameFromID(uint8 classID);
std::set<uint32> GetSetFromConfigString(string configStringName);

#define EverQuest EverQuestMod::instance()

#endif //EVERQUEST
