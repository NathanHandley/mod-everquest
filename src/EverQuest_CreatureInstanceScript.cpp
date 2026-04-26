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
        EverQuestCreatureInstance CreatureInstanceData;
        deque<Position> PathToCurrentTargetPos;
        Position CurrentTargetPos;
        bool IsMovingToTargetPos = false; // Needed?

        // Waypoint
        vector<EverQuestCreatureWaypoint> CreatureWaypoints;
        uint32 WaypointPriorTargetWaypointIndex = 0;
        uint32 WaypointCurrentTargetWaypointIndex = 0;
        vector<uint32> WaypointRandom10Indices;
        uint32 WaypointRandomPathFinalTargetIndex = 0;

        // Agro
        Position LastAgroPosition;
        bool IsReturningToAgroPosition = false;

        // Events
        enum Events
        {
            EVENT_PAUSE_DONE = 1
        };
        EventMap events;

        void LoadCustomData()
        {
            uint32 creatureGUID = me->GetSpawnId();
            if (creatureGUID == 389523 || creatureGUID == 389155 || creatureGUID == 389525 || creatureGUID == 389533 || creatureGUID == 389534 || creatureGUID == 389528
                || creatureGUID == 389527 || creatureGUID == 389524 || creatureGUID == 389529 || creatureGUID == 389531 || creatureGUID == 389535 || creatureGUID == 389532
                || creatureGUID == 389530 || creatureGUID == 389526)
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
            deque<Position>().swap(PathToCurrentTargetPos);

            IsMovingToTargetPos = false;
            WaypointPriorTargetWaypointIndex = 0;
            WaypointCurrentTargetWaypointIndex = 0;
            WaypointRandomPathFinalTargetIndex = 0;
            IsReturningToAgroPosition = false;

            me->SetWalk(false);
            me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);

            // Handle movement types
            if (CreatureInstanceData.DoesRoam == true)
            {
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
                    floorLoopNum++;
                    if (floorLoopNum >= 5)
                        break;
                }
            }

            if (floorZ < -20000)
            {
                LiquidData targetLiquidDataBelow = me->GetMap()->GetLiquidData(me->GetPhaseMask(), x, y, referenceZ - 10.0f, 0, {});
                if (targetLiquidDataBelow.Status)
                {
                    foundValidZ = true;
                    return referenceZ - 5.0f;
                }
                else
                    return referenceZ;
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
                            return floorZ;
                    }
                    return referenceZ;
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

        bool BuildPathAndStartPointMovementToTarget(float initialTargetX, float initialTargetY, float initialTargetZ)
        {
            if (doDebug)
            {
                LOG_ERROR("module.EverQuest", "{} BuildPathAndStartPointMovementToTarget start", debugCreatureGUID);
            }

            // Clear prior path
            deque<Position>().swap(PathToCurrentTargetPos);

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
                PathToCurrentTargetPos.emplace_front(Position(initialTargetX, initialTargetY, initialTargetZ));
                IsMovingToTargetPos = false;
                if (doDebug)
                {
                    LOG_ERROR("module.EverQuest", "{} BuildPathAndStartPointMovementToTarget had invalid path", debugCreatureGUID);
                }
                return false;
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
                    if (doDebug)
                    {
                        LOG_ERROR("module.EverQuest", "{} BuildPathAndStartPointMovementToTarget small step calculated to {} {} {}", debugCreatureGUID, interimX, interimY, interimZ);
                    }
                    Position interimPosition = Position(interimX, interimY, interimZ);
                    PathToCurrentTargetPos.emplace_back(interimPosition);
                    previousPosition = interimPosition;
                    remainingDistance -= EQ_MOVE_SMALL_STEP_SIZE_DISTANCE;
                }
            }

            // Add the last position
            Position lastPosition = Position(initialTargetX, initialTargetY, terrainSnappedTargetZ);
            PathToCurrentTargetPos.emplace_back(lastPosition);

            if (doDebug)
            {
                int pathSize = (int)path.GetPath().size();
                LOG_ERROR("module.EverQuest", "{} BuildPathAndStartPointMovementToTarget path calculated with {} nodes", debugCreatureGUID, PathToCurrentTargetPos.size());
            }

            // Start Movement
            IsMovingToTargetPos = true;
            me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);
            me->GetMotionMaster()->Clear(false);
            ProcessNextMovementPoint();

            return true;
        }

        void ProcessNextMovementPoint()
        {
            if (PathToCurrentTargetPos.empty() == true || MovementType == EQ_CREATURE_MOVEMENT_NO_CUSTOM)
                return;

            Position nextPosition = PathToCurrentTargetPos.front();
            PathToCurrentTargetPos.pop_front();

            if (doDebug)
            {
                LOG_ERROR("module.EverQuest", "{} ProcessNextMovementPoint {} {} {}", debugCreatureGUID, nextPosition.GetPositionX(), nextPosition.GetPositionY(), nextPosition.GetPositionZ());
            }

            me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);
            if (MovementType == EQ_CREATURE_MOVEMENT_CUSTOM_WAYPOINT)
                me->GetMotionMaster()->MovePoint(EQ_MOVE_TO_WAYPOINT_POINT, nextPosition);
            else if (MovementType == EQ_CREATURE_MOVEMENT_CUSTOM_ROAMING)
                me->GetMotionMaster()->MovePoint(EQ_MOVE_TO_ROAM_POINT, nextPosition);
            CurrentTargetPos = nextPosition;

            if (doDebug)
            {
                LOG_ERROR("module.EverQuest", "{} ProcessNextMovementPoint ended", debugCreatureGUID);
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
            BuildPathAndStartPointMovementToTarget(wp.X, wp.Y, wp.Z);
        }

        void PerformWaypointMovementForRandomAny()
        {
            WaypointCurrentTargetWaypointIndex = GetUniqueRandomWaypointIndex(WaypointCurrentTargetWaypointIndex);
            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[WaypointCurrentTargetWaypointIndex];
            BuildPathAndStartPointMovementToTarget(wp.X, wp.Y, wp.Z);
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
            BuildPathAndStartPointMovementToTarget(wp.X, wp.Y, wp.Z);
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

            if (isValidPoint)
                isValidPoint = BuildPathAndStartPointMovementToTarget(x, y, z);

            if (isValidPoint == false)
            {
                // Clear path
                deque<Position>().swap(PathToCurrentTargetPos);
                events.ScheduleEvent(EVENT_PAUSE_DONE, 100ms);
                return;
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (doDebug)
            {
                LOG_ERROR("module.EverQuest", "{} MovementInform movement started type {} id {}", debugCreatureGUID, type, id);
            }
            if (type == POINT_MOTION_TYPE)
            {
                //if (IsDoingSmallStep == true)
                //{
                //    IsDoingSmallStep = false;
                //    return;
                //}

                /*if (CurrentTargetPos.GetExactDist(me->GetPosition()) > EQ_MOVE_SMALL_STEP_DISTANCE_TO_END)
                    return;*/
                if (PathToCurrentTargetPos.empty() == false)
                {
                    ProcessNextMovementPoint();
                    if (doDebug)
                    {
                        LOG_ERROR("module.EverQuest", "{} MovementInform movement ended after running ProcessNextMovementPoint()", debugCreatureGUID);
                    }
                    return;
                }

                IsMovingToTargetPos = false;

                if (id == EQ_MOVE_RETURN_TO_AGRO_ID)
                {
                    PerformMovementToNewPoint();
                }
                else if (id == EQ_MOVE_TO_ROAM_POINT)
                {
                    uint32 delay = urand(CreatureInstanceData.RoamMinDelayInMS, CreatureInstanceData.RoamMaxDelayInMS);
                    if (delay > 0)
                        events.ScheduleEvent(EVENT_PAUSE_DONE, Milliseconds(delay));
                    else
                        PerformMovementToNewPoint();
                }
                else if (id == EQ_MOVE_TO_WAYPOINT_POINT)
                {
                    WaypointPriorTargetWaypointIndex = WaypointCurrentTargetWaypointIndex;
                    const EverQuestCreatureWaypoint& wp = CreatureWaypoints[WaypointPriorTargetWaypointIndex];
                    if (wp.PauseInSec > 0)
                        events.ScheduleEvent(EVENT_PAUSE_DONE, Seconds(wp.PauseInSec));
                    else
                        PerformMovementToNewPoint();
                }
            }
            else if (type == WAYPOINT_MOTION_TYPE)
            {
                if (CreatureInstanceData.DespawnAtWaypointNum != -1 &&
                    id == static_cast<uint32>(CreatureInstanceData.DespawnAtWaypointNum))
                {
                    me->DespawnOrUnsummon();
                }
            }
            if (doDebug)
            {
                LOG_ERROR("module.EverQuest", "{} MovementInform movement ended at end of method - type {} id {}", debugCreatureGUID, type, id);
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

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            events.CancelEvent(EVENT_PAUSE_DONE); // TODO: Handle agro so that it doesn't also call UpdateAI?
            if (MovementType == EQ_CREATURE_MOVEMENT_NO_CUSTOM)
                return;
            LastAgroPosition = me->GetPosition();
            IsReturningToAgroPosition = false;
            me->GetMotionMaster()->Clear(false);
        }

        void EnterEvadeMode(EvadeReason why) override
        {
            ScriptedAI::EnterEvadeMode(why);
            if (MovementType == EQ_CREATURE_MOVEMENT_NO_CUSTOM)
                return;

            me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);

            float x = LastAgroPosition.GetPositionX();
            float y = LastAgroPosition.GetPositionY();
            float z = LastAgroPosition.GetPositionZ();
            me->GetMotionMaster()->MovePoint(EQ_MOVE_RETURN_TO_AGRO_ID, x, y, z);
            //CurrentTargetPos = LastAgroPosition; TODO: Do something with this...
            IsReturningToAgroPosition = true;
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
