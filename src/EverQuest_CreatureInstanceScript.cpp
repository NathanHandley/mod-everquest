//  Author: Nathan Handley (nathanhandley@protonmail.com)
//  Copyright (c) 2026 Nathan Handley
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

#include "Creature.h"
#include "GridTerrainData.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "EventMap.h"
#include "MotionMaster.h"
#include "MoveSplineInit.h"

#include "EverQuest.h"

#include <algorithm>
#include <random>

using namespace std;

class EverQuest_CreatureInstanceScript : public CreatureScript
{
public:
    EverQuest_CreatureInstanceScript() : CreatureScript("EverQuest_CreatureInstanceScript") {}

    struct EverQuest_CreatureInstanceScriptAI : public ScriptedAI
    {
        EverQuest_CreatureInstanceScriptAI(Creature* creature) : ScriptedAI(creature) {}

        bool doDebug = false;
        uint32 debugCreatureGUID = 0;

        // Shared Data
        uint32 MovementType = EQ_CREATURE_MOVEMENT_NO_CUSTOM;
        uint32 ActiveMovePhase = EQ_MOVE_PHASE_NONE;
        EverQuestCreatureInstance CreatureInstanceData;
        Position WaypointAndRoamTargetTravelPosition;

        // Waypoint
        vector<EverQuestCreatureWaypoint> CreatureWaypoints;
        uint32 WaypointPriorTargetWaypointIndex = 0;
        uint32 WaypointCurrentTargetWaypointIndex = 0;
        vector<uint32> WaypointRandom10Indices;
        uint32 WaypointRandomPathFinalTargetIndex = 0;

        // Agro
        Position LastAgroPosition;
        uint32 PreAgroCurrentTargetIdx = 0;
        uint32 PreAgroPriorTargetIdx = 0;
        uint32 PreAgroFinalTargetIdx = 0;
        Position PreAgroTravelPosition;

        // Events
        enum Events
        {
            EVENT_PAUSE_DONE = 1
        };
        EventMap events;

        void LoadCustomData()
        {
            uint32 creatureGUID = me->GetSpawnId();
            if (/*creatureGUID == 356451 && */me->GetMapId() == 830)
            {
                debugCreatureGUID = creatureGUID;
                doDebug = true;
            }
            CreatureInstanceData = EverQuest->GetCreatureInstanceData(creatureGUID);
            CreatureWaypoints.clear();
            if (CreatureInstanceData.WaypointListID != -1)
                CreatureWaypoints = EverQuest->GetWaypoints(CreatureInstanceData.MapID, CreatureInstanceData.WaypointListID);
        }

        void Reset() override
        {
            LoadCustomData();

            events.Reset();

            ActiveMovePhase = EQ_MOVE_PHASE_NONE;
            WaypointPriorTargetWaypointIndex = 0;
            WaypointCurrentTargetWaypointIndex = 0;
            WaypointRandomPathFinalTargetIndex = 0;

            me->SetWalk(false);
            me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);

            // Handle movement types
            if (CreatureInstanceData.DoesRoam == true)
            {
                if (CreatureInstanceData.RoamMinZ != 0 && CreatureInstanceData.RoamMaxZ != 0)
                    NormalizeRoamZRange(CreatureInstanceData.RoamMinZ, CreatureInstanceData.RoamMaxZ);
                MovementType = EQ_CREATURE_MOVEMENT_CUSTOM_ROAMING;
            }
            else if (CreatureWaypoints.empty() == true)
                MovementType = EQ_CREATURE_MOVEMENT_NO_CUSTOM;
            else if (CreatureInstanceData.WanderType == EQ_GRID_RANDOM_10)
            {
                MovementType = EQ_CREATURE_MOVEMENT_CUSTOM_WAYPOINT;
                GenerateRandom10WaypointIndicesAndSetPriorIndex();
            }
            else if (CreatureInstanceData.WanderType == EQ_GRID_RANDOM_PATH)
            {
                MovementType = EQ_CREATURE_MOVEMENT_CUSTOM_WAYPOINT;
                WaypointPriorTargetWaypointIndex = FindNearestWaypointIndex();
                WaypointRandomPathFinalTargetIndex = WaypointPriorTargetWaypointIndex;
            }
            else if (CreatureInstanceData.WanderType == EQ_GRID_RANDOM
                || CreatureInstanceData.WanderType == EQ_GRID_RAND_5_LOS
                || CreatureInstanceData.WanderType == EQ_GRID_RANDOM_CENTER_POINT)
            {
                MovementType = EQ_CREATURE_MOVEMENT_CUSTOM_WAYPOINT;
                WaypointPriorTargetWaypointIndex = 0;
            }
            else
                MovementType = EQ_CREATURE_MOVEMENT_NO_CUSTOM;

            if (MovementType != EQ_CREATURE_MOVEMENT_NO_CUSTOM)
                PerformMovementToNewPoint();
        }

        void PerformMovementToNewPoint()
        {
            if (MovementType == EQ_CREATURE_MOVEMENT_CUSTOM_ROAMING)
            {
                PerformRoamingMovement();
            }
            else if (MovementType == EQ_CREATURE_MOVEMENT_CUSTOM_WAYPOINT)
            {
                switch (CreatureInstanceData.WanderType)
                {
                case EQ_GRID_RANDOM_10:
                {
                    PerformWaypointMovementForRandom10();
                } break;
                case EQ_GRID_RANDOM_PATH:
                {
                    PerformWaypointMovementForRandomAny();
                } break;
                case EQ_GRID_RANDOM:
                case EQ_GRID_RAND_5_LOS:
                case EQ_GRID_RANDOM_CENTER_POINT:
                default:
                {
                    PerformWaypointMovementForRandomPath();
                } break;
                }
            }
        }

        void NormalizeRoamZRange(float& minZ, float& maxZ)
        {
            float currentZ = me->GetPositionZ();
            if (maxZ - minZ < 6.0f)
            {
                float halfGap = 3.0f;
                minZ = currentZ - halfGap;
                maxZ = currentZ + halfGap;
            }
        }

        float GetEffectiveDestinationZ(float x, float y, float referenceZ, bool& foundValidZ, float minZ = 0, float maxZ = 0)
        {
            if (doDebug)
            {
                LOG_ERROR("module.EverQuest", "{} GetEffectiveDestinationZ method entry - x y referenceZ : minZ maxZ {} {} {} : {} {}", debugCreatureGUID, x, y, referenceZ, minZ, maxZ);
            }

            foundValidZ = false;
            float floorZ = -20001;
            if (minZ != 0 && maxZ != 0)
            {
                floorZ = me->GetMapHeight(x, y, referenceZ, true, maxZ - minZ);
            }
            else
            {
                float targetTestZ = referenceZ;
                int floorLoopNum = 0;
                while (floorZ < -20000)
                {
                    targetTestZ = referenceZ + (floorLoopNum * 5.0f);
                    floorZ = me->GetMapHeight(x, y, targetTestZ, true, (floorLoopNum * 10.0f));

                    if (doDebug)
                    {
                        LOG_ERROR("module.EverQuest", "GetEffectiveDestinationZ {} find Z test {}, floorZ {}, loop {}", debugCreatureGUID, targetTestZ, floorZ, floorLoopNum);
                    }

                    floorLoopNum++;
                    if (floorLoopNum >= 5)
                        break;
                }
            }

            if (doDebug)
            {
                LOG_ERROR("module.EverQuest", "GetEffectiveDestinationZ {} floorZ {}", debugCreatureGUID, floorZ);
            }

            if (floorZ < -20000)
            {
                LiquidData targetLiquidDataBelow = me->GetMap()->GetLiquidData(me->GetPhaseMask(), x, y, referenceZ - 10.0f, 0, {});
                if (targetLiquidDataBelow.Status)
                {
                    if (doDebug)
                    {
                        LOG_ERROR("module.EverQuest", "GetEffectiveDestinationZ (end) {} was in liquid at z {} ({})", debugCreatureGUID, floorZ, referenceZ-5.0f);
                    }
                    foundValidZ = true;
                    return referenceZ - 5.0f;
                }
                else
                {
                    if (doDebug)
                    {
                        LOG_ERROR("module.EverQuest", "GetEffectiveDestinationZ (end) {} was not in liquid at z {} ({})", debugCreatureGUID, floorZ, referenceZ);
                    }
                    return referenceZ;
                }
            }
            else
            {
                foundValidZ = true;
                if (me->isSwimming())
                {


                    LiquidData targetLiquidDataAtRef = me->GetMap()->GetLiquidData(me->GetPhaseMask(), x, y, referenceZ, 0, {});
                    if (targetLiquidDataAtRef.Status)
                    {
                        if (floorZ > targetLiquidDataAtRef.DepthLevel)
                        {
                            if (doDebug)
                            {
                                LOG_ERROR("module.EverQuest", "GetEffectiveDestinationZ (end) {} was swimming and floorZ was > DepthLevel ({})", debugCreatureGUID, floorZ);
                            }
                            return floorZ;
                        }
                    }
                    return referenceZ;
                }
                if (doDebug)
                {
                    LOG_ERROR("module.EverQuest", "GetEffectiveDestinationZ (end) {} was not swimming ({})", debugCreatureGUID, floorZ);
                }
                return floorZ;
            }
        }

        void GenerateRandom10WaypointIndicesAndSetPriorIndex()
        {
            WaypointRandom10Indices.clear();
            vector<uint32> allIndices;
            for (uint32 i = 0; i < CreatureWaypoints.size(); ++i)
                allIndices.push_back(i);

            static mt19937 rng(random_device{}());
            shuffle(allIndices.begin(), allIndices.end(), rng);

            uint32 count = min<uint32>(10u, allIndices.size());
            WaypointRandom10Indices.assign(allIndices.begin(), allIndices.begin() + count);

            // Set prior position to the nearest
            uint32 nearestIndex = 0;
            float minDist = 1000000.0f;
            for (uint32 i = 0; i < WaypointRandom10Indices.size(); ++i)
            {
                const auto& wp = CreatureWaypoints[WaypointRandom10Indices[i]];
                float dist = me->GetExactDist(wp.X, wp.Y, wp.Z);
                if (dist < minDist)
                {
                    minDist = dist;
                    nearestIndex = i;
                }
            }
            WaypointPriorTargetWaypointIndex = nearestIndex;
        }

        bool BuildPathAndStartPointMovementToTarget(float initialTargetX, float initialTargetY, float initialTargetZ, uint32 moveType, bool run = false)
        {
            if (doDebug)
            {
                LOG_ERROR("module.EverQuest", "{} BuildPathAndStartPointMovementToTarget start", debugCreatureGUID);
            }

            // Snap the target Z
            bool foundValidZ = false;
            float terrainSnappedTargetZ = GetEffectiveDestinationZ(initialTargetX, initialTargetY, initialTargetZ, foundValidZ, CreatureInstanceData.RoamMinZ,
                CreatureInstanceData.RoamMaxZ);
            PathGenerator path(me);
            bool result = path.CalculatePath(initialTargetX, initialTargetY, initialTargetZ, false);
            bool pathFound = (result == true && path.GetPathType() != PATHFIND_NOPATH);
            float pathLength = path.getPathLength();
            if (foundValidZ == false || (pathFound == false && pathLength <= 0.1f))
            {
                if (doDebug)
                {
                    LOG_ERROR("module.EverQuest", "{} BuildPathAndStartPointMovementToTarget had invalid path", debugCreatureGUID);
                }
                return false;
            }

            // Build the full waypoint list, and put current creature position up front
            Movement::PointsArray waypointPath;
            waypointPath.emplace_back(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());

            if (doDebug)
            {
                LOG_ERROR("module.EverQuest", "{} BuildPathAndStartPointMovementToTarget first position calculated to {} {} {} {}", debugCreatureGUID, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), foundValidZ);
            }

            // Break up straight longer paths
            if (pathLength >= EQ_MOVE_SMALL_STEP_SIZE_LAST_DISTANCE)
            {
                float remainingDistance = pathLength;
                Position previousPosition = me->GetPosition();
                float ux = (initialTargetX - me->GetPositionX()) / pathLength;
                float uy = (initialTargetY - me->GetPositionY()) / pathLength;
                float uz = (terrainSnappedTargetZ - me->GetPositionZ()) / pathLength;
                while (remainingDistance > EQ_MOVE_SMALL_STEP_SIZE_LAST_DISTANCE)
                {
                    float interimX = previousPosition.GetPositionX() + ux * EQ_MOVE_SMALL_STEP_SIZE_DISTANCE;
                    float interimY = previousPosition.GetPositionY() + uy * EQ_MOVE_SMALL_STEP_SIZE_DISTANCE;
                    float interimZ = previousPosition.GetPositionZ() + uz * EQ_MOVE_SMALL_STEP_SIZE_DISTANCE;
                    interimZ = GetEffectiveDestinationZ(interimX, interimY, interimZ, foundValidZ, CreatureInstanceData.RoamMinZ, CreatureInstanceData.RoamMaxZ);

                    if (doDebug)
                    {
                        LOG_ERROR("module.EverQuest", "{} BuildPathAndStartPointMovementToTarget small step calculated to {} {} {} {}", debugCreatureGUID, interimX, interimY, interimZ, foundValidZ);
                    }
                    
                    waypointPath.emplace_back(interimX, interimY, interimZ);
                    previousPosition = Position(interimX, interimY, interimZ);
                    remainingDistance -= EQ_MOVE_SMALL_STEP_SIZE_DISTANCE;
                }
            }

            // Add the last position
            waypointPath.emplace_back(initialTargetX, initialTargetY, terrainSnappedTargetZ);
            if (moveType == EQ_MOVE_PHASE_TRAVELING)
            {
                Position newTargetPosition(initialTargetX, initialTargetY, terrainSnappedTargetZ);
                WaypointAndRoamTargetTravelPosition = newTargetPosition;
            }

            if (doDebug)
            {
                LOG_ERROR("module.EverQuest", "{} BuildPathAndStartPointMovementToTarget last position calculated to {} {} {}", debugCreatureGUID, initialTargetX, initialTargetY, terrainSnappedTargetZ);
                LOG_ERROR("module.EverQuest", "{} BuildPathAndStartPointMovementToTarget path calculated with {} nodes", debugCreatureGUID, waypointPath.size());
            }

            // Start Movement
            if (run == true)
                me->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
            else
                me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);
            me->GetMotionMaster()->Clear(false);
            me->GetMotionMaster()->MoveSplinePath(&waypointPath, run ? FORCED_MOVEMENT_RUN : FORCED_MOVEMENT_WALK);
            ActiveMovePhase = moveType;

            return true;
        }

        void OnCustomPathCompleted()
        {
            if (MovementType == EQ_CREATURE_MOVEMENT_CUSTOM_WAYPOINT)
            {
                ActiveMovePhase = EQ_MOVE_PHASE_NONE;
                WaypointPriorTargetWaypointIndex = WaypointCurrentTargetWaypointIndex;
                const EverQuestCreatureWaypoint& wp = CreatureWaypoints[WaypointPriorTargetWaypointIndex];
                if (wp.PauseInSec > 0)
                    events.ScheduleEvent(EVENT_PAUSE_DONE, Seconds(wp.PauseInSec));
                else
                    PerformMovementToNewPoint();
            }
            else if (MovementType == EQ_CREATURE_MOVEMENT_CUSTOM_ROAMING)
            {
                ActiveMovePhase = EQ_MOVE_PHASE_NONE;
                uint32 delay = urand(CreatureInstanceData.RoamMinDelayInMS, CreatureInstanceData.RoamMaxDelayInMS);
                if (delay > 0)
                    events.ScheduleEvent(EVENT_PAUSE_DONE, Milliseconds(delay));
                else
                    PerformMovementToNewPoint();
            }
        }

        uint32 GetUniqueRandomWaypointIndex(uint32 currentIndex) const
        {
            // Make sure it's always a new waypoint, when possible
            uint32 priorIndex = currentIndex;
            uint32 newIndex = currentIndex;
            if (CreatureWaypoints.size() > 1)
            {
                do
                {
                    newIndex = urand(0, CreatureWaypoints.size() - 1);
                } while (newIndex == priorIndex);
            }
            return newIndex;
        }

        uint32 GetUniqueRandomWaypointIndexFromRandom10(uint32 currentIndex) const
        {
            // Make sure it's always a new waypoint, when possible
            uint32 priorIndex = currentIndex;
            uint32 newIndex = currentIndex;
            if (WaypointRandom10Indices.size() > 1)
            {
                do
                {
                    uint32 curRandom10Index = urand(0, WaypointRandom10Indices.size() - 1);
                    newIndex = WaypointRandom10Indices[curRandom10Index];
                } while (newIndex == priorIndex);
            }
            return newIndex;
        }

        void PerformWaypointMovementForRandom10()
        {
            WaypointCurrentTargetWaypointIndex = GetUniqueRandomWaypointIndexFromRandom10(WaypointCurrentTargetWaypointIndex);
            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[WaypointCurrentTargetWaypointIndex];
            BuildPathAndStartPointMovementToTarget(wp.X, wp.Y, wp.Z, EQ_MOVE_PHASE_TRAVELING);
        }

        void PerformWaypointMovementForRandomAny()
        {
            WaypointCurrentTargetWaypointIndex = GetUniqueRandomWaypointIndex(WaypointCurrentTargetWaypointIndex);
            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[WaypointCurrentTargetWaypointIndex];
            BuildPathAndStartPointMovementToTarget(wp.X, wp.Y, wp.Z, EQ_MOVE_PHASE_TRAVELING);
        }

        void PerformWaypointMovementForRandomPath()
        {
            if (WaypointPriorTargetWaypointIndex == WaypointRandomPathFinalTargetIndex)
                WaypointRandomPathFinalTargetIndex = GetUniqueRandomWaypointIndex(WaypointCurrentTargetWaypointIndex);

            // Step up or down the list to the waypoint
            if (WaypointRandomPathFinalTargetIndex > WaypointPriorTargetWaypointIndex)
                WaypointCurrentTargetWaypointIndex = WaypointPriorTargetWaypointIndex + 1;
            else
                WaypointCurrentTargetWaypointIndex = WaypointPriorTargetWaypointIndex - 1;

            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[WaypointCurrentTargetWaypointIndex];
            BuildPathAndStartPointMovementToTarget(wp.X, wp.Y, wp.Z, EQ_MOVE_PHASE_TRAVELING);
        }

        uint32 FindNearestWaypointIndex() const
        {
            if (CreatureWaypoints.empty())
                return 0;

            uint32 nearest = 0;
            float minDist = 1000000.0f;
            for (uint32 i = 0; i < CreatureWaypoints.size(); ++i)
            {
                const auto& wp = CreatureWaypoints[i];
                float dist = me->GetExactDist(wp.X, wp.Y, wp.Z);
                if (dist < minDist)
                {
                    minDist = dist;
                    nearest = i;
                }
            }
            return nearest;
        }

        void PerformRoamingMovement()
        {
            float x = frand(CreatureInstanceData.RoamMinX, CreatureInstanceData.RoamMaxX);
            float y = frand(CreatureInstanceData.RoamMinY, CreatureInstanceData.RoamMaxY);
            float referenceZ = me->GetPositionZ();

            bool isValidPoint = false;
            float z = GetEffectiveDestinationZ(x, y, referenceZ, isValidPoint, CreatureInstanceData.RoamMinZ, CreatureInstanceData.RoamMaxZ);

            if (isValidPoint == true)
                isValidPoint = BuildPathAndStartPointMovementToTarget(x, y, z, EQ_MOVE_PHASE_TRAVELING);

            if (isValidPoint == false)
            {
                events.ScheduleEvent(EVENT_PAUSE_DONE, 100ms);
                ActiveMovePhase = EQ_MOVE_PHASE_WAITING_FOR_TIMER;
                return;
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == WAYPOINT_MOTION_TYPE)
            {
                if (CreatureInstanceData.DespawnAtWaypointNum != -1 && id == static_cast<uint32>(CreatureInstanceData.DespawnAtWaypointNum))
                    me->DespawnOrUnsummon();
            }
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);
            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_PAUSE_DONE)
                    PerformMovementToNewPoint();
            }

            // Handle spline ends
            if (me->GetMotionMaster()->HasMovementGeneratorType(ESCORT_MOTION_TYPE) == false)
            {
                if (ActiveMovePhase == EQ_MOVE_PHASE_TRAVELING)
                    OnCustomPathCompleted();
                else if (ActiveMovePhase == EQ_MOVE_PHASE_RETURNING_FROM_AGRO)
                    BuildPathAndStartPointMovementToTarget(WaypointAndRoamTargetTravelPosition.GetPositionX(),
                        WaypointAndRoamTargetTravelPosition.GetPositionY(), WaypointAndRoamTargetTravelPosition.GetPositionZ(), EQ_MOVE_PHASE_TRAVELING);
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            events.CancelEvent(EVENT_PAUSE_DONE);
            if (MovementType == EQ_CREATURE_MOVEMENT_NO_CUSTOM)
                return;

            // Snapshot the full waypoint state so EnterEvadeMode can restore it after since Reset() overwrites everything
            PreAgroCurrentTargetIdx = WaypointCurrentTargetWaypointIndex;
            PreAgroPriorTargetIdx = WaypointPriorTargetWaypointIndex;
            PreAgroFinalTargetIdx = WaypointRandomPathFinalTargetIndex;
            PreAgroTravelPosition = WaypointAndRoamTargetTravelPosition;

            LastAgroPosition = me->GetPosition();
            me->GetMotionMaster()->Clear(false);
            ActiveMovePhase = EQ_MOVE_PHASE_AGRO;
        }

        void EnterEvadeMode(EvadeReason why) override
        {
            ScriptedAI::EnterEvadeMode(why);
            if (MovementType == EQ_CREATURE_MOVEMENT_NO_CUSTOM)
                return;

            // Restore any prior states
            WaypointCurrentTargetWaypointIndex = PreAgroCurrentTargetIdx;
            WaypointPriorTargetWaypointIndex = PreAgroPriorTargetIdx;
            WaypointRandomPathFinalTargetIndex = PreAgroFinalTargetIdx;
            WaypointAndRoamTargetTravelPosition = PreAgroTravelPosition;

            float x = LastAgroPosition.GetPositionX();
            float y = LastAgroPosition.GetPositionY();
            float z = LastAgroPosition.GetPositionZ();
            BuildPathAndStartPointMovementToTarget(x, y, z, EQ_MOVE_PHASE_RETURNING_FROM_AGRO, true);
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new EverQuest_CreatureInstanceScriptAI(creature);
    }
};

void AddEverQuestCreatureInstanceScripts()
{
    new EverQuest_CreatureInstanceScript();
}
