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

        vector<EverQuestCreatureWaypoint> CreatureWaypoints;
        EverQuestCreatureInstance CreatureInstanceData;
        uint32 CurrentWaypointIndex = 0;
        Position AgroPosition;
        Position CurrentTargetPos;
        bool IsMovingToWaypoint = false;
        bool IsRoaming = false;

        // For EQ_GRID_RANDOM_10
        vector<uint32> Random10Indices;
        uint32 Random10Current = 0;

        // For EQ_GRID_RANDOM_PATH
        uint32 TargetWaypointIndex = 0;
        uint32 NextPathWaypointIndex = 0;
        bool HasInitializedPath = false;

        enum Events
        {
            EVENT_PAUSE_DONE = 1,
            EVENT_NEXT_SMALL_STEP = 2
        };
        EventMap events;

        void LoadCustomData()
        {
            CreatureWaypoints.clear();
            uint32 creatureGUID = me->GetSpawnId();
            CreatureInstanceData = EverQuest->GetCreatureInstanceData(creatureGUID);
            if (CreatureInstanceData.WaypointListID != -1)
                CreatureWaypoints = EverQuest->GetWaypoints(CreatureInstanceData.MapID, CreatureInstanceData.WaypointListID);
        }

        void Reset() override
        {
            events.Reset();
            CurrentWaypointIndex = 0;
            IsMovingToWaypoint = false;
            Random10Indices.clear();
            Random10Current = 0;
            IsRoaming = false;
            TargetWaypointIndex = 0;
            NextPathWaypointIndex = 0;
            HasInitializedPath = false;

            me->SetWalk(false); // true = walk
            me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);

            LoadCustomData();

            if (CreatureInstanceData.DoesRoam)
            {
                IsRoaming = true;
                StartRoamingMovement();
            }
            else if (CreatureInstanceData.WanderType != EQ_NONE)
                StartWanderMovement();
        }

        float TempMapZHelper(float x, float y, float z)
        {
            float newZ = me->GetMapHeight(x, y, z);
            if (newZ > INVALID_HEIGHT)
                return newZ;
            else
                return z;
        }

        float GetEffectiveDestinationZ(float x, float y, float referenceZ)
        {
            // Find a test target that has a floor
            float targetTestZ = referenceZ;
            int floorLoopNum = 0;
            float floorZ = -20001;
            while (floorZ < -20000)
            {
                targetTestZ = referenceZ + 10.0f + (floorLoopNum * 5.0f);
                floorZ = me->GetMapHeight(x, y, targetTestZ);
                floorLoopNum++;
                if (floorLoopNum >= 3)
                    break;
            }

            // If there was no floor, it's probably liquid
            if (floorZ < -20000)
            {
                // Look for water below the feet
                LiquidData targetLiquidDataBelow = me->GetMap()->GetLiquidData(me->GetPhaseMask(), x, y, referenceZ - 10.0f, 0, {});
                if (targetLiquidDataBelow.Status)
                {
                    // Drop down a little to 'skim' under the water
                    return referenceZ - 5.0f;
                }
                else
                    return referenceZ;
            }
            else
            {
                // Test first if in water since it may need to go straight sideways
                if (me->isSwimming() == true)
                {
                    // Straight out is just swiming sideways
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

        void StartWanderMovement()
        {
            if (CreatureWaypoints.empty())
                return;

            switch (CreatureInstanceData.WanderType)
            {
            case EQ_GRID_RANDOM_10:
            {
                GenerateRandom10Indices();
                Random10Current = 0;
                GoToRandom10Point();
            } break;
            case EQ_GRID_RANDOM_PATH:
            {
                StartRandomPathMovement();
            } break;
            case EQ_GRID_RANDOM:
            case EQ_GRID_RANDOM_CENTER_POINT:
            {
                PickAndGoToRandomWaypoint();
            } break;
            case EQ_GRID_RAND_5_LOS:
            {
                PickRandomFrom5ClosestWithLOS();
            } break;
            default:
            {
                PickAndGoToRandomWaypoint();
            } break;
            }
        }

        void GenerateRandom10Indices()
        {
            Random10Indices.clear();
            vector<uint32> allIndices;
            for (uint32 i = 0; i < CreatureWaypoints.size(); ++i)
                allIndices.push_back(i);

            static mt19937 rng(random_device{}());
            shuffle(allIndices.begin(), allIndices.end(), rng);

            uint32 count = min<uint32>(10u, allIndices.size());
            Random10Indices.assign(allIndices.begin(), allIndices.begin() + count);
        }

        void GoToRandom10Point()
        {
            if (Random10Indices.empty())
            {
                PickAndGoToRandomWaypoint();
                return;
            }

            CurrentWaypointIndex = Random10Indices[Random10Current];
            Random10Current = (Random10Current + 1) % Random10Indices.size();

            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[CurrentWaypointIndex];
            CurrentTargetPos = Position(wp.X, wp.Y, wp.Z);

            IsMovingToWaypoint = true;
            events.ScheduleEvent(EVENT_NEXT_SMALL_STEP, 100ms);
        }

        void PickAndGoToRandomWaypoint()
        {
            if (CreatureWaypoints.empty())
                return;

            CurrentWaypointIndex = urand(0, CreatureWaypoints.size() - 1);

            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[CurrentWaypointIndex];
            CurrentTargetPos = Position(wp.X, wp.Y, wp.Z);

            IsMovingToWaypoint = true;
            events.ScheduleEvent(EVENT_NEXT_SMALL_STEP, 100ms);
        }

        void PickRandomFrom5ClosestWithLOS()
        {
            if (CreatureWaypoints.empty())
            {
                PickAndGoToRandomWaypoint();
                return;
            }

            // Collect all waypoints with their distance
            vector<pair<float, uint32>> distList;
            for (uint32 i = 0; i < CreatureWaypoints.size(); ++i)
            {
                const auto& wp = CreatureWaypoints[i];
                float dist = me->GetExactDist(wp.X, wp.Y, wp.Z);
                distList.emplace_back(dist, i);
            }

            // Sort by distance (closest first)
            sort(distList.begin(), distList.end());

            // Take up to 5 closest
            vector<uint32> candidates;
            uint32 maxCandidates = min<uint32>(5u, distList.size());
            for (uint32 i = 0; i < maxCandidates; ++i)
            {
                uint32 idx = distList[i].second;
                const auto& wp = CreatureWaypoints[idx];

                // Check Line of Sight
                if (me->IsWithinLOS(wp.X, wp.Y, wp.Z))
                    candidates.push_back(idx);
            }

            // If no LOS candidates, fall back to fully random
            if (candidates.empty())
            {
                PickAndGoToRandomWaypoint();
                return;
            }

            // Pick one randomly from the valid LOS candidates
            uint32 chosen = candidates[urand(0, candidates.size() - 1)];
            CurrentWaypointIndex = chosen;

            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[CurrentWaypointIndex];
            CurrentTargetPos = Position(wp.X, wp.Y, wp.Z);

            IsMovingToWaypoint = true;
            events.ScheduleEvent(EVENT_NEXT_SMALL_STEP, 100ms);
        }

        void StartRoamingMovement()
        {
            if (!CreatureInstanceData.DoesRoam)
                return;

            PickRandomRoamPoint();
        }

        void PickRandomRoamPoint()
        {
            // Try to find a valid point inside the roam box.
            float x = frand(CreatureInstanceData.RoamMinX, CreatureInstanceData.RoamMaxX);
            float y = frand(CreatureInstanceData.RoamMinY, CreatureInstanceData.RoamMaxY);
            float referenceZ = me->GetPositionZ();

            float z = GetEffectiveDestinationZ(x, y, referenceZ);

            CurrentTargetPos = Position(x, y, z);
            IsMovingToWaypoint = true;
            events.ScheduleEvent(EVENT_NEXT_SMALL_STEP, 100ms);
        }

        void StartMovingToNextWaypoint()
        {
            if (CreatureWaypoints.empty())
                return;
            if (CurrentWaypointIndex >= CreatureWaypoints.size())
                CurrentWaypointIndex = 0;
            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[CurrentWaypointIndex];
            CurrentTargetPos = Position(wp.X, wp.Y, wp.Z);
            IsMovingToWaypoint = true;
            events.ScheduleEvent(EVENT_NEXT_SMALL_STEP, 100ms);
        }

        void TakeNextSmallStep()
        {
            if (!IsMovingToWaypoint)
                return;

            float dx = CurrentTargetPos.GetPositionX() - me->GetPositionX();
            float dy = CurrentTargetPos.GetPositionY() - me->GetPositionY();
            float dist = sqrt(dx * dx + dy * dy);

            if (dist < 5.0f)
            {
                FinishCurrentWaypoint();
                return;
            }

            // X/Y
            float nx = dx / dist;
            float ny = dy / dist;
            float newX = me->GetPositionX() + nx * (float)EQ_MOVE_SMALL_STEP_SIZE;
            float newY = me->GetPositionY() + ny * (float)EQ_MOVE_SMALL_STEP_SIZE;

            float referenceZ = me->GetPositionZ();
            float newZ = GetEffectiveDestinationZ(newX, newY, referenceZ);

            me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);

            // Move to this small terrain/water-aware point
            me->GetMotionMaster()->MovePoint(EQ_MOVE_SMALL_TERRAIN_MOVE_ID, newX, newY, newZ);
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

        void InitializePathIfNeeded()
        {
            if (CreatureWaypoints.empty())
                return;

            if (!HasInitializedPath)
            {
                CurrentWaypointIndex = FindNearestWaypointIndex();
                HasInitializedPath = true;
                return;
            }

            // Re-snap if far from current waypoint (post-combat safety)
            const auto& currWp = CreatureWaypoints[CurrentWaypointIndex];
            float distToCurr = me->GetExactDist(currWp.X, currWp.Y, currWp.Z);
            if (distToCurr > 25.0f)
            {
                CurrentWaypointIndex = FindNearestWaypointIndex();
            }
        }

        void PickRandomTargetWaypoint()
        {
            if (CreatureWaypoints.empty())
                return;

            uint32 newTarget = CurrentWaypointIndex;
            uint32 attempts = 0;
            while (newTarget == CurrentWaypointIndex && attempts < 20 && CreatureWaypoints.size() > 1)
            {
                newTarget = urand(0, CreatureWaypoints.size() - 1);
                attempts++;
            }
            TargetWaypointIndex = newTarget;
        }

        void StartRandomPathMovement()
        {
            InitializePathIfNeeded();
            PickRandomTargetWaypoint();
            StartMovingTowardsTargetInPath();
        }

        void StartMovingTowardsTargetInPath()
        {
            if (CreatureWaypoints.empty())
                return;

            if (CurrentWaypointIndex == TargetWaypointIndex)
            {
                PickRandomTargetWaypoint();
                if (CurrentWaypointIndex == TargetWaypointIndex)
                {
                    events.ScheduleEvent(EVENT_PAUSE_DONE, 1500ms);
                    return;
                }
            }

            int32 direction = (TargetWaypointIndex > CurrentWaypointIndex) ? 1 : -1;
            int32 nextInt = static_cast<int32>(CurrentWaypointIndex) + direction;
            uint32 nextIdx = (nextInt < 0) ? 0 : (nextInt >= static_cast<int32>(CreatureWaypoints.size()) ? CreatureWaypoints.size() - 1 : static_cast<uint32>(nextInt));

            NextPathWaypointIndex = nextIdx;

            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[nextIdx];
            CurrentTargetPos = Position(wp.X, wp.Y, wp.Z);

            IsMovingToWaypoint = true;
            events.ScheduleEvent(EVENT_NEXT_SMALL_STEP, 100ms);
        }

        void FinishCurrentWaypoint()
        {
            IsMovingToWaypoint = false;

            if (IsRoaming == true)
            {
                // Roaming always waits a random delay after reaching a point
                uint32 delay = urand(CreatureInstanceData.RoamMinDelayInMS, CreatureInstanceData.RoamMaxDelayInMS);
                events.ScheduleEvent(EVENT_PAUSE_DONE, Milliseconds(delay));
                return;
            }

            if (CreatureInstanceData.WanderType == EQ_GRID_RANDOM_PATH)
            {
                CurrentWaypointIndex = NextPathWaypointIndex;
                const EverQuestCreatureWaypoint& wp = CreatureWaypoints[CurrentWaypointIndex];

                if (wp.PauseInSec > 0)
                {
                    events.ScheduleEvent(EVENT_PAUSE_DONE, Seconds(wp.PauseInSec));
                }
                else
                {
                    StartMovingTowardsTargetInPath();
                }
                return;
            }

            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[CurrentWaypointIndex];

            if (wp.PauseInSec > 0)
            {
                events.ScheduleEvent(EVENT_PAUSE_DONE, Seconds(wp.PauseInSec));
            }
            else
            {
                if (CreatureInstanceData.WanderType != EQ_NONE)
                {
                    if (CreatureInstanceData.WanderType == EQ_GRID_RANDOM_10)
                        GoToRandom10Point();
                    else
                        StartWanderMovement();
                }
                else
                {
                    CurrentWaypointIndex++;
                    StartMovingToNextWaypoint();
                }
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == EQ_MOVE_RETURN_TO_AGRO_ID)
            {
                if (CreatureInstanceData.DoesRoam)
                    StartRoamingMovement();
                else if (CreatureInstanceData.WanderType != EQ_NONE)
                    StartWanderMovement();
                else
                    StartMovingToNextWaypoint();
                return;
            }

            if (id == EQ_MOVE_SMALL_TERRAIN_MOVE_ID && IsMovingToWaypoint)
                events.ScheduleEvent(EVENT_NEXT_SMALL_STEP, 50ms);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);
            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_PAUSE_DONE)
                {
                    if (IsRoaming == true)
                    {
                        StartRoamingMovement();
                    }
                    else if (CreatureInstanceData.WanderType == EQ_GRID_RANDOM_PATH)
                    {
                        StartMovingTowardsTargetInPath();
                    }
                    else if (CreatureInstanceData.WanderType != EQ_NONE)
                    {
                        if (CreatureInstanceData.WanderType == EQ_GRID_RANDOM_10)
                            GoToRandom10Point();
                        else
                            StartWanderMovement();
                    }
                    else
                    {
                        CurrentWaypointIndex++;
                        StartMovingToNextWaypoint();
                    }
                }
                else if (eventId == EVENT_NEXT_SMALL_STEP)
                {
                    TakeNextSmallStep();
                }
            }

            if (UpdateVictim() == false)
                return;

            DoMeleeAttackIfReady();
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            events.CancelEvent(EVENT_PAUSE_DONE);
            events.CancelEvent(EVENT_NEXT_SMALL_STEP);
            AgroPosition = me->GetPosition();
            me->GetMotionMaster()->Clear(false);
            IsMovingToWaypoint = false;
        }

        void EnterEvadeMode(EvadeReason why) override
        {
            ScriptedAI::EnterEvadeMode(why);

            float x = AgroPosition.GetPositionX();
            float y = AgroPosition.GetPositionY();
            float referenceZ = AgroPosition.GetPositionZ();

            float z = GetEffectiveDestinationZ(x, y, referenceZ);

            me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);

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
