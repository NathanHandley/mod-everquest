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

        // Shared Data
        uint32 MovementType = EQ_CREATURE_MOVEMENT_NO_CUSTOM;
        uint32 ActiveMovePhase = EQ_MOVE_PHASE_NONE;
        EverQuestCreatureInstance CreatureInstanceData;
        Position WaypointAndRoamTargetTravelPosition;
        uint32 PathRetryCount = 0;

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
                    PerformWaypointMovementForRandomPath();
                } break;
                case EQ_GRID_RANDOM:
                case EQ_GRID_RAND_5_LOS:
                case EQ_GRID_RANDOM_CENTER_POINT:
                default:
                {
                    PerformWaypointMovementForRandomAny();
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

        float GetEffectiveDestinationZ(float priorX, float priorY, float priorZ, float initialTargetX, float initialTargetY, float initialTargetZ,
            bool& foundValidZ, float minZ = 0, float maxZ = 0)
        {
            if (CreatureInstanceData.DisableGroundContour == true)
            {
                foundValidZ = true;
                return initialTargetZ;
            }
            
            foundValidZ = false;

            // Prior point might be in water
            bool isPriorPointInWater = false;
            if (priorX != 0 && priorY != 0 && priorZ != 0)
            {
                LiquidData priorLiquidDataBelow = me->GetMap()->GetLiquidData(me->GetPhaseMask(), priorX, priorY, priorZ, 0, {});
                if (priorLiquidDataBelow.Status)
                    isPriorPointInWater = true;
            }

            // Calculate a solid floor
            float solidFloorZ = -20001;
            if (minZ != 0 && maxZ != 0)
            {
                solidFloorZ = me->GetMapHeight(initialTargetX, initialTargetY, maxZ, true, maxZ - minZ);
            }
            else
            {
                float targetTestZ = initialTargetZ;
                int floorLoopNum = 0;
                float curAddedZStep = 0;
                if (isPriorPointInWater == false)
                    curAddedZStep = 1.0f;
                while (solidFloorZ < -20000)
                {
                    targetTestZ = initialTargetZ + (floorLoopNum * curAddedZStep);
                    curAddedZStep += 1.0f;
                    solidFloorZ = me->GetMapHeight(initialTargetX, initialTargetY, targetTestZ, true, (floorLoopNum * 20.0f));
                    floorLoopNum++;
                    if (floorLoopNum >= 10)
                        break;
                }
            }

            if (solidFloorZ < -20000)
            {
                // No solid floor means it's out of bounds or over a large body of water, so first test if it's a body of water
                LiquidData targetLiquidDataBelow = me->GetMap()->GetLiquidData(me->GetPhaseMask(), initialTargetX, initialTargetY, initialTargetZ - EQ_MOVE_TEST_Z_DOWN_AMOUNT_FOR_WATER_TEST, 0, {});
                if (targetLiquidDataBelow.Status)
                {
                    foundValidZ = true;
                    if (isPriorPointInWater == true && priorZ <= (targetLiquidDataBelow.Level - EQ_MOVE_UNDER_WATER_SURFACE_SKIM_REDICTION))
                        return priorZ;
                    else
                        return targetLiquidDataBelow.Level - EQ_MOVE_UNDER_WATER_SURFACE_SKIM_REDICTION;
                }
                else
                    return initialTargetZ;
            }
            else
            {
                foundValidZ = true;
                if (isPriorPointInWater == true)
                {
                    LiquidData targetLiquidDataAtRef = me->GetMap()->GetLiquidData(me->GetPhaseMask(), initialTargetX, initialTargetY, initialTargetZ, 0, {});
                    if (targetLiquidDataAtRef.Status)
                    {
                        float skimLevel = targetLiquidDataAtRef.Level - EQ_MOVE_UNDER_WATER_SURFACE_SKIM_REDICTION;
                        if (solidFloorZ > priorZ || solidFloorZ > skimLevel)
                            return solidFloorZ;
                        else if (priorZ > skimLevel)
                            return skimLevel;
                        else
                            return priorZ;
                    }
                }
                return solidFloorZ;
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
            bool foundValidZ = false;
            float terrainSnappedTargetZ = GetEffectiveDestinationZ(0, 0, 0, initialTargetX, initialTargetY, initialTargetZ, foundValidZ,
                CreatureInstanceData.RoamMinZ, CreatureInstanceData.RoamMaxZ);

            // Generate a base path to make sure it exists
            PathGenerator path(me);
            bool result = path.CalculatePath(initialTargetX, initialTargetY, initialTargetZ, false);
            PathType pathType = path.GetPathType();
            bool pathFound = ((result == true) && ((pathType & PATHFIND_NOPATH) == 0));
            Movement::PointsArray fallbackPath;
            const Movement::PointsArray* pathNodesPtr = &path.GetPath();

            // If there is no single spline-based path, it was probably too long so break it into parts
            if (pathFound == false)
            {
                float distToTarget = me->GetExactDist(initialTargetX, initialTargetY, terrainSnappedTargetZ);
                if (distToTarget > 30.0f)
                {
                    const float MAX_SAFE_SEGMENT = 220.0f;

                    Position current = me->GetPosition();
                    float dx = initialTargetX - current.GetPositionX();
                    float dy = initialTargetY - current.GetPositionY();
                    float dz = terrainSnappedTargetZ - current.GetPositionZ();
                    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

                    if (dist > 5.0f)
                    {
                        float ratio = std::min(1.0f, MAX_SAFE_SEGMENT / dist);
                        float interX = current.GetPositionX() + dx * ratio;
                        float interY = current.GetPositionY() + dy * ratio;
                        float interZ = current.GetPositionZ() + dz * ratio;

                        PathGenerator shortPath(me);
                        bool shortResult = shortPath.CalculatePath(interX, interY, interZ, false);
                        if (shortResult && (shortPath.GetPathType() & PATHFIND_NOPATH) == 0)
                        {
                            fallbackPath = shortPath.GetPath();
                            if (fallbackPath.size() >= 2)
                            {
                                pathNodesPtr = &fallbackPath;
                                pathFound = true;
                            }
                        }
                    }
                }
            }

            if (pathFound == false && foundValidZ == false && MovementType == EQ_CREATURE_MOVEMENT_CUSTOM_ROAMING)
                return false;

            const Movement::PointsArray& pathNodes = *pathNodesPtr;
            if (pathNodes.size() < 2)
                return false;

            // Walk the path to generate steps that are small enough to snap the character to the Z
            Movement::PointsArray waypointPath;
            waypointPath.emplace_back(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
            Position previousPosition = me->GetPosition();
            float priorInterimZ = -100001.0f;
            for (int cornerIndex = 1; cornerIndex < (int)pathNodes.size(); ++cornerIndex)
            {
                Position corner(pathNodes[cornerIndex].x, pathNodes[cornerIndex].y, pathNodes[cornerIndex].z);
                bool isFinalCorner = (cornerIndex == pathNodes.size() - 1);

                float segDX = corner.GetPositionX() - previousPosition.GetPositionX();
                float segDY = corner.GetPositionY() - previousPosition.GetPositionY();
                float segDZ = corner.GetPositionZ() - previousPosition.GetPositionZ();
                float segLength = std::sqrt(segDX * segDX + segDY * segDY + segDZ * segDZ);

                if (segLength < 0.001f)
                    continue;

                float ux = segDX / segLength;
                float uy = segDY / segLength;
                float uz = segDZ / segLength;

                float endThreshold = isFinalCorner ? EQ_MOVE_SMALL_STEP_SIZE_LAST_DISTANCE : EQ_MOVE_SMALL_STEP_SIZE_DISTANCE;
                float remainingDistance = segLength;

                while (remainingDistance > endThreshold)
                {
                    float interimX = previousPosition.GetPositionX() + ux * EQ_MOVE_SMALL_STEP_SIZE_DISTANCE;
                    float interimY = previousPosition.GetPositionY() + uy * EQ_MOVE_SMALL_STEP_SIZE_DISTANCE;
                    float interimZ = previousPosition.GetPositionZ() + uz * EQ_MOVE_SMALL_STEP_SIZE_DISTANCE;

                    if (priorInterimZ < -100000)
                        priorInterimZ = interimZ;

                    interimZ = GetEffectiveDestinationZ(previousPosition.GetPositionX(), previousPosition.GetPositionY(),
                        previousPosition.GetPositionZ(), interimX, interimY, priorInterimZ, foundValidZ,
                        CreatureInstanceData.RoamMinZ, CreatureInstanceData.RoamMaxZ);

                    waypointPath.emplace_back(interimX, interimY, interimZ);
                    previousPosition = Position(interimX, interimY, interimZ);
                    remainingDistance -= EQ_MOVE_SMALL_STEP_SIZE_DISTANCE;
                    priorInterimZ = interimZ;
                }

                // Add corner
                float refZ = (priorInterimZ < -100000.0f) ? corner.GetPositionZ() : priorInterimZ;
                float cornerZ = GetEffectiveDestinationZ(previousPosition.GetPositionX(), previousPosition.GetPositionY(),
                    previousPosition.GetPositionZ(), corner.GetPositionX(), corner.GetPositionY(), refZ,
                    foundValidZ, CreatureInstanceData.RoamMinZ, CreatureInstanceData.RoamMaxZ);

                waypointPath.emplace_back(corner.GetPositionX(), corner.GetPositionY(), cornerZ);
                previousPosition = Position(corner.GetPositionX(), corner.GetPositionY(), cornerZ);
                priorInterimZ = cornerZ;
            }

            // Add for saftey, though this shouldn't happen...
            if (waypointPath.size() <= 1)
                waypointPath.emplace_back(initialTargetX, initialTargetY, terrainSnappedTargetZ);

            // Track intended destination
            if (moveType == EQ_MOVE_PHASE_TRAVELING)
            {
                Position newTargetPosition(initialTargetX, initialTargetY, terrainSnappedTargetZ);
                WaypointAndRoamTargetTravelPosition = newTargetPosition;
            }

            // Start movement
            if (run == true)
                me->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
            else
                me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);

            me->GetMotionMaster()->Clear(false);
            me->GetMotionMaster()->MoveSplinePath(&waypointPath, run == true ? FORCED_MOVEMENT_RUN : FORCED_MOVEMENT_WALK);
            ActiveMovePhase = moveType;
            if (moveType == EQ_MOVE_PHASE_TRAVELING)
                PathRetryCount = 0;

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

        void ScheduleMovementRetry()
        {
            PathRetryCount++;
            if (PathRetryCount > EQ_MOVE_PATH_MAX_RETRY_COUNT)
            {
                LOG_ERROR("module.EverQuest", "Creature {} has retried {} times to path and will no longer retry", me->GetSpawnId(), PathRetryCount);
                ActiveMovePhase = EQ_MOVE_PHASE_NONE;
                return;
            }
            
            events.ScheduleEvent(EVENT_PAUSE_DONE, 100ms);
            ActiveMovePhase = EQ_MOVE_PHASE_WAITING_FOR_TIMER;
        }

        void PerformWaypointMovementForRandom10()
        {
            WaypointCurrentTargetWaypointIndex = GetUniqueRandomWaypointIndexFromRandom10(WaypointCurrentTargetWaypointIndex);
            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[WaypointCurrentTargetWaypointIndex];
            if (BuildPathAndStartPointMovementToTarget(wp.X, wp.Y, wp.Z, EQ_MOVE_PHASE_TRAVELING) == false)
                ScheduleMovementRetry();
        }

        void PerformWaypointMovementForRandomAny()
        {
            WaypointCurrentTargetWaypointIndex = GetUniqueRandomWaypointIndex(WaypointCurrentTargetWaypointIndex);
            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[WaypointCurrentTargetWaypointIndex];
            if (BuildPathAndStartPointMovementToTarget(wp.X, wp.Y, wp.Z, EQ_MOVE_PHASE_TRAVELING) == false)
                ScheduleMovementRetry();
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
            if (BuildPathAndStartPointMovementToTarget(wp.X, wp.Y, wp.Z, EQ_MOVE_PHASE_TRAVELING) == false)
                ScheduleMovementRetry();
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
            float z = GetEffectiveDestinationZ(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), x, y, referenceZ, isValidPoint, CreatureInstanceData.RoamMinZ, CreatureInstanceData.RoamMaxZ);

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
