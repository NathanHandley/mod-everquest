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

        // Shared Data
        uint32 MovementType = EQ_CREATURE_MOVEMENT_NO_CUSTOM;
        EverQuestCreatureInstance CreatureInstanceData;

        // Waypoint
        vector<EverQuestCreatureWaypoint> CreatureWaypoints;
        uint32 WaypointCurrentIndex = 0;
        vector<uint32> WaypointRandom10Indices;
        uint32 WaypointRandom10CurrentRandomIndex = 0;
        uint32 RandomPathWaypoint

        // Roaming


        // Agro

        enum Events
        {
            EVENT_PAUSE_DONE = 1
        };
        EventMap events;





        Position AgroPosition;
        Position CurrentTargetPos;
        bool IsMovingToWaypoint = false;
        //bool IsRoaming = false;

        // For EQ_GRID_RANDOM_10        


        // For EQ_GRID_RANDOM_PATH
        uint32 TargetWaypointIndex = 0;
        uint32 NextPathWaypointIndex = 0;
        bool HasInitializedPath = false;

        // For smooth small-step movement
        vector<Position> CurrentSmallStepPath;

        // State saved when combat starts so we can resume after evade
        uint32 LastWaypointIndexBeforeCombat = 0;
        uint32 LastTargetWaypointIndex = 0;
        bool   WasInRandomPath = false;



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

            // TODO: Reset members

            me->SetWalk(false);
            me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);


            /*WaypointCurrentIndex = 0;
            IsMovingToWaypoint = false;
            WaypointRandom10Indices.clear();
            WaypointRandom10CurrentRandomIndex = 0;
            IsRoaming = false;
            TargetWaypointIndex = 0;
            NextPathWaypointIndex = 0;
            HasInitializedPath = false;
            CurrentSmallStepPath.clear();
            LastWaypointIndexBeforeCombat = 0;
            LastTargetWaypointIndex = 0;
            WasInRandomPath = false;*/

            // Handle movement types
            if (CreatureInstanceData.DoesRoam == true)
            {
                MovementType = EQ_CREATURE_MOVEMENT_CUSTOM_ROAMING;
                NormalizeRoamZRange(CreatureInstanceData.RoamMinZ, CreatureInstanceData.RoamMaxZ);
                StartRoamingMovement();
            }
            else if (CreatureWaypoints.empty() == true)
            {
                MovementType = EQ_CREATURE_MOVEMENT_NO_CUSTOM;
            }
            else if (CreatureInstanceData.WanderType == EQ_GRID_RANDOM_10)
            {
                MovementType = EQ_CREATURE_MOVEMENT_CUSTOM_WAYPOINT;
                GenerateRandom10WaypointIndices();
                WaypointRandom10CurrentRandomIndex = 0;
                PerformWaypointMovementForRandom10();
            }
            else if (CreatureInstanceData.WanderType == EQ_GRID_RANDOM
                || CreatureInstanceData.WanderType == EQ_GRID_RAND_5_LOS
                || CreatureInstanceData.WanderType == EQ_GRID_RANDOM_CENTER_POINT)
            {
                MovementType = EQ_CREATURE_MOVEMENT_CUSTOM_WAYPOINT;
                PerformWaypointMovementForRandom();
            }
            else if (CreatureInstanceData.WanderType == EQ_GRID_RANDOM_PATH)
            {
                MovementType = EQ_CREATURE_MOVEMENT_CUSTOM_WAYPOINT;
                StartRandomPathMovement();
            }
            else
            {
                MovementType = EQ_CREATURE_MOVEMENT_NO_CUSTOM;
            }
        }

        // Keep
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

        // Keep
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

        // Keep
        void GenerateRandom10WaypointIndices()
        {
            WaypointRandom10Indices.clear();
            vector<uint32> allIndices;
            for (uint32 i = 0; i < CreatureWaypoints.size(); ++i)
                allIndices.push_back(i);

            static mt19937 rng(random_device{}());
            shuffle(allIndices.begin(), allIndices.end(), rng);

            uint32 count = min<uint32>(10u, allIndices.size());
            WaypointRandom10Indices.assign(allIndices.begin(), allIndices.begin() + count);
        }

        void GoToTargetWithSmallSteps(const Position& targetPos)
        {
            CurrentTargetPos = targetPos;
            CurrentSmallStepPath.clear();
            IsMovingToWaypoint = true;

            Position currentPos = me->GetPosition();
            float dx = CurrentTargetPos.GetPositionX() - currentPos.GetPositionX();
            float dy = CurrentTargetPos.GetPositionY() - currentPos.GetPositionY();
            float dist = sqrt(dx * dx + dy * dy);

            me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);
            me->GetMotionMaster()->Clear(false);

            if (dist < 10.0f)
            {
                // Very close — just do a direct MovePoint
                me->GetMotionMaster()->MovePoint(EQ_MOVE_SMALL_TERRAIN_MOVE_ID, CurrentTargetPos);
                return;
            }

            // Build full spline for longer distances
            while (true)
            {
                dx = CurrentTargetPos.GetPositionX() - currentPos.GetPositionX();
                dy = CurrentTargetPos.GetPositionY() - currentPos.GetPositionY();
                dist = sqrt(dx * dx + dy * dy);

                if (dist < 6.0f)
                {
                    CurrentSmallStepPath.push_back(CurrentTargetPos);
                    break;
                }

                float stepSize = static_cast<float>(EQ_MOVE_SMALL_STEP_SIZE);
                if (stepSize > dist - 3.0f)
                    stepSize = dist - 3.0f;

                if (stepSize < 2.0f)
                {
                    CurrentSmallStepPath.push_back(CurrentTargetPos);
                    break;
                }

                float nx = dx / dist;
                float ny = dy / dist;

                float newX = currentPos.GetPositionX() + nx * stepSize;
                float newY = currentPos.GetPositionY() + ny * stepSize;

                bool foundValidZ = false;
                float newZ = GetEffectiveDestinationZ(newX, newY, currentPos.GetPositionZ(), foundValidZ);

                CurrentSmallStepPath.emplace_back(newX, newY, newZ);
                currentPos = CurrentSmallStepPath.back();
            }

            if (CurrentSmallStepPath.size() < 2)
            {
                me->GetMotionMaster()->MovePoint(EQ_MOVE_SMALL_TERRAIN_MOVE_ID, CurrentTargetPos);
                return;
            }

            Movement::PointsArray splinePoints;
            splinePoints.reserve(CurrentSmallStepPath.size());
            for (const auto& p : CurrentSmallStepPath)
                splinePoints.emplace_back(p.GetPositionX(), p.GetPositionY(), p.GetPositionZ());

            me->GetMotionMaster()->MoveSplinePath(&splinePoints, FORCED_MOVEMENT_NONE);
        }

        // Keep (in progress)
        void BuildAndStartPointMovementToTarget(float x, float y, float z)
        {




        }

        uint32 GetUniqueRandomWaypointIndex(uint32 currentIndex)
        {
            // Make sure it's always a new waypoint, when possible
            uint32 priorIndex = currentIndex;
            uint32 newIndex = currentIndex;
            if (CreatureWaypoints.size() > 1)
            {
                do
                {
                    newIndex = urand(0, CreatureWaypoints.size() - 1);
                } while (WaypointCurrentIndex == priorIndex);
            }
            return newIndex;
        }

        // Keep
        void PerformWaypointMovementForRandom10()
        {
            WaypointCurrentIndex = GetUniqueRandomWaypointIndex(WaypointCurrentIndex);
            WaypointRandom10CurrentRandomIndex = (WaypointRandom10CurrentRandomIndex + 1) % WaypointRandom10Indices.size();
            WaypointCurrentIndex = WaypointRandom10Indices[WaypointRandom10CurrentRandomIndex];            
            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[WaypointCurrentIndex];
            BuildAndStartPointMovementToTarget(wp.X, wp.Y, wp.Z);
        }

        // Keep
        void PerformWaypointMovementForRandom()
        {
            WaypointCurrentIndex = GetUniqueRandomWaypointIndex(WaypointCurrentIndex);
            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[WaypointCurrentIndex];
            BuildAndStartPointMovementToTarget(wp.X, wp.Y, wp.Z);
        }

        void StartRandomPathMovement()
        {
            InitializePathIfNeeded();
            PickRandomTargetWaypoint();
            StartMovingTowardsTargetInPath();
        }

        // Keep
        void PerformWaypointMovementForRandomPath()
        {
            WaypointCurrentIndex = GetUniqueRandomWaypointIndex(WaypointCurrentIndex);


        }







        void StartRoamingMovement()
        {
            if (!CreatureInstanceData.DoesRoam)
                return;
            PickRandomRoamPoint();
        }

        void PickRandomRoamPoint()
        {
            float x = frand(CreatureInstanceData.RoamMinX, CreatureInstanceData.RoamMaxX);
            float y = frand(CreatureInstanceData.RoamMinY, CreatureInstanceData.RoamMaxY);
            float referenceZ = me->GetPositionZ();

            bool isValidPoint = false;
            float z = GetEffectiveDestinationZ(x, y, referenceZ, isValidPoint, CreatureInstanceData.RoamMinZ, CreatureInstanceData.RoamMaxZ);

            if (isValidPoint)
            {
                PathGenerator path(me);
                path.SetPathLengthLimit(200.0f);
                bool result = path.CalculatePath(x, y, z, false);
                bool pathFound = (result && path.GetPathType() != PATHFIND_NOPATH);
                isValidPoint = pathFound;
            }

            if (!isValidPoint)
            {
                events.ScheduleEvent(EVENT_PAUSE_DONE, 100ms);
                return;
            }

            CurrentTargetPos = Position(x, y, z);
            IsMovingToWaypoint = true;

            me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);
            me->GetMotionMaster()->Clear(false);
            me->GetMotionMaster()->MovePoint(EQ_MOVE_SMALL_TERRAIN_MOVE_ID, x, y, z);
        }

        void StartMovingToNextWaypoint()
        {
            if (CreatureWaypoints.empty())
                return;
            if (WaypointCurrentIndex >= CreatureWaypoints.size())
                WaypointCurrentIndex = 0;

            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[WaypointCurrentIndex];
            GoToTargetWithSmallSteps(Position(wp.X, wp.Y, wp.Z));
        }



        void InitializePathIfNeeded()
        {
            if (CreatureWaypoints.empty())
                return;

            if (!HasInitializedPath)
            {
                WaypointCurrentIndex = FindNearestWaypointIndex();
                HasInitializedPath = true;
                return;
            }

            const auto& currWp = CreatureWaypoints[WaypointCurrentIndex];
            float distToCurr = me->GetExactDist(currWp.X, currWp.Y, currWp.Z);
            if (distToCurr > 25.0f)
                WaypointCurrentIndex = FindNearestWaypointIndex();
        }

        void PickRandomTargetWaypoint()
        {
            if (CreatureWaypoints.empty())
                return;

            uint32 newTarget = WaypointCurrentIndex;
            uint32 attempts = 0;
            while (newTarget == WaypointCurrentIndex && attempts < 20 && CreatureWaypoints.size() > 1)
            {
                newTarget = urand(0, CreatureWaypoints.size() - 1);
                attempts++;
            }
            TargetWaypointIndex = newTarget;
        }

        void StartMovingTowardsTargetInPath()
        {
            if (CreatureWaypoints.empty())
                return;

            if (WaypointCurrentIndex == TargetWaypointIndex)
            {
                PickRandomTargetWaypoint();
                if (WaypointCurrentIndex == TargetWaypointIndex)
                {
                    events.ScheduleEvent(EVENT_PAUSE_DONE, 1500ms);
                    return;
                }
            }

            int32 direction = (TargetWaypointIndex > WaypointCurrentIndex) ? 1 : -1;
            int32 nextInt = static_cast<int32>(WaypointCurrentIndex) + direction;
            uint32 nextIdx = (nextInt < 0) ? 0 :
                (nextInt >= static_cast<int32>(CreatureWaypoints.size()) ? CreatureWaypoints.size() - 1 : static_cast<uint32>(nextInt));

            NextPathWaypointIndex = nextIdx;
            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[nextIdx];
            GoToTargetWithSmallSteps(Position(wp.X, wp.Y, wp.Z));
        }

        void FinishCurrentWaypoint()
        {
            IsMovingToWaypoint = false;
            CurrentSmallStepPath.clear();

            if (IsRoaming)
            {
                me->GetMotionMaster()->Clear(false);
                me->StopMoving();
                uint32 delay = urand(CreatureInstanceData.RoamMinDelayInMS, CreatureInstanceData.RoamMaxDelayInMS);
                events.ScheduleEvent(EVENT_PAUSE_DONE, Milliseconds(delay));
                return;
            }
            else if (CreatureInstanceData.WanderType == EQ_GRID_RANDOM_PATH)
            {
                WaypointCurrentIndex = NextPathWaypointIndex;
                const EverQuestCreatureWaypoint& wp = CreatureWaypoints[WaypointCurrentIndex];

                if (wp.PauseInSec > 0)
                    events.ScheduleEvent(EVENT_PAUSE_DONE, Seconds(wp.PauseInSec));
                else
                    StartMovingTowardsTargetInPath();
                return;
            }

            // For EQ_GRID_RANDOM and similar
            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[WaypointCurrentIndex];

            if (wp.PauseInSec > 0)
            {
                events.ScheduleEvent(EVENT_PAUSE_DONE, Seconds(wp.PauseInSec));
            }
            else
            {
                // Zero pause → immediately pick next random waypoint
                if (CreatureInstanceData.WanderType != EQ_NONE)
                {
                    if (CreatureInstanceData.WanderType == EQ_GRID_RANDOM_10)
                        StartRandom10PointWaypointMovement();
                    else
                        StartCustomWanderMovement();
                }
                else
                {
                    WaypointCurrentIndex = (WaypointCurrentIndex + 1) % CreatureWaypoints.size();
                    StartMovingToNextWaypoint();
                }
            }
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

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == POINT_MOTION_TYPE)
            {
                if (id == EQ_MOVE_RETURN_TO_AGRO_ID)
                {
                    if (CreatureInstanceData.DoesRoam)
                    {
                        StartRoamingMovement();
                        return;
                    }

                    if (LastWaypointIndexBeforeCombat < CreatureWaypoints.size())
                        WaypointCurrentIndex = LastWaypointIndexBeforeCombat;
                    else
                        WaypointCurrentIndex = FindNearestWaypointIndex();

                    if (WasInRandomPath)
                    {
                        TargetWaypointIndex = LastTargetWaypointIndex;
                        HasInitializedPath = true;
                        events.ScheduleEvent(EVENT_PAUSE_DONE, 500ms);
                    }
                    else if (CreatureInstanceData.WanderType != EQ_NONE)
                    {
                        events.ScheduleEvent(EVENT_PAUSE_DONE, 500ms);
                    }
                    else
                    {
                        StartMovingToNextWaypoint();
                    }
                    return;
                }

                if (IsMovingToWaypoint)
                {
                    FinishCurrentWaypoint();
                    return;
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
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_PAUSE_DONE)
                {
                    if (IsRoaming)
                        StartRoamingMovement();
                    else if (CreatureInstanceData.WanderType == EQ_GRID_RANDOM_PATH)
                        StartMovingTowardsTargetInPath();
                    else if (CreatureInstanceData.WanderType != EQ_NONE)
                        StartCustomWanderMovement();
                    else
                    {
                        WaypointCurrentIndex = (WaypointCurrentIndex + 1) % CreatureWaypoints.size();
                        StartMovingToNextWaypoint();
                    }
                }
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            events.CancelEvent(EVENT_PAUSE_DONE);
            AgroPosition = me->GetPosition();

            LastWaypointIndexBeforeCombat = WaypointCurrentIndex;
            LastTargetWaypointIndex = TargetWaypointIndex;
            WasInRandomPath = (CreatureInstanceData.WanderType == EQ_GRID_RANDOM_PATH);

            me->GetMotionMaster()->Clear(false);
            IsMovingToWaypoint = false;
            CurrentSmallStepPath.clear();
        }

        void EnterEvadeMode(EvadeReason why) override
        {
            ScriptedAI::EnterEvadeMode(why);

            me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);

            float x = AgroPosition.GetPositionX();
            float y = AgroPosition.GetPositionY();
            float z = AgroPosition.GetPositionZ();
            me->GetMotionMaster()->MovePoint(EQ_MOVE_RETURN_TO_AGRO_ID, x, y, z);
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
