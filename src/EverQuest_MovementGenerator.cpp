//  Author: Nathan Handley (nathanhandley@protonmail.com)
//  Copyright (c) 2026 Nathan Handley + Automation
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
#include "EQZoneMap.h"
#include "Map.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include "Random.h"

#include "EverQuest_MovementGenerator.h"

using namespace std;

EverQuestMovementGenerator::EverQuestMovementGenerator(EverQuestCreatureInstance const& creatureInstance, vector<EverQuestCreatureWaypoint> const& waypoints) :
    CreatureInstance(creatureInstance), Waypoints(waypoints)
{
}

bool EverQuestMovementGenerator::IsOneWay() const
{
    return CreatureInstance.WanderType == EQ_GRID_ONE_WAY_REPOP || CreatureInstance.WanderType == EQ_GRID_ONE_WAY_DEPOP;
}

void EverQuestMovementGenerator::DoInitialize(Creature* owner)
{
    owner->AddUnitState(UNIT_STATE_ROAMING);
    owner->StopMoving();
    IsDepopPending = false;
    IsScriptPaused = false;
    CurrentState = State::Waiting;
    WaitTimeLeftInMS = 0;
    ResetPosition = owner->GetPosition();
    HasResetPosition = true;

    // Where a grid starts (the EQ server's InitializeGrid): the random types pick a waypoint right away, the random path starts at the closest
    // waypoint, and the rest walk to the first waypoint first.  Roamers pick a point in their box right away
    if (CreatureInstance.DoesRoam == false)
    {
        IsPatrolGoingBack = false;
        IsCenterPointNext = false;
        switch (CreatureInstance.WanderType)
        {
            case EQ_GRID_RANDOM_10:
            case EQ_GRID_RANDOM:
            case EQ_GRID_RAND_5_LOS:
            {
                CurrentWaypointIndex = GetClosestWaypointIndex(owner);
                PickNextWaypoint(owner);
                StartTravel(owner);
            } break;
            case EQ_GRID_RANDOM_PATH:
            {
                CurrentWaypointIndex = GetClosestWaypointIndex(owner);
                RandomPathDestinationIndex = CurrentWaypointIndex;
                PickNextWaypoint(owner);
                StartTravel(owner);
            } break;
            default:
            {
                CurrentWaypointIndex = 0;
                EverQuestCreatureWaypoint const& waypoint = Waypoints[CurrentWaypointIndex];
                TravelTarget.Relocate(waypoint.X, waypoint.Y, waypoint.Z);
                StartTravel(owner);
            } break;
        }
    }
}

void EverQuestMovementGenerator::DoFinalize(Creature* owner)
{
    owner->ClearUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);
}

// Back from combat, an evade, or a script's move: carry on to the point it was going to (the EQ server resumed a mover from where it was after a
// short wait)
void EverQuestMovementGenerator::DoReset(Creature* owner)
{
    owner->AddUnitState(UNIT_STATE_ROAMING);
    if (CurrentState == State::Travelling)
    {
        CurrentState = State::Waiting;
        WaitTimeLeftInMS = EQ_MOVEMENT_RESUME_DELAY_IN_MS;
        IsResumingTravel = true;
    }
    else
        WaitTimeLeftInMS = max<uint32>(WaitTimeLeftInMS, EQ_MOVEMENT_RESUME_DELAY_IN_MS);
}

bool EverQuestMovementGenerator::GetResetPosition(float& x, float& y, float& z)
{
    if (HasResetPosition == false)
        return false;
    ResetPosition.GetPosition(x, y, z);
    return true;
}

void EverQuestMovementGenerator::Pause(uint32 timer)
{
    IsScriptPaused = true;
    ScriptPauseTimeLeftInMS = timer;
}

void EverQuestMovementGenerator::Resume(uint32 overrideTimer)
{
    IsScriptPaused = false;
    if (overrideTimer != 0)
        WaitTimeLeftInMS = overrideTimer;
}

bool EverQuestMovementGenerator::DoUpdate(Creature* owner, uint32 diff)
{
    if (owner->IsAlive() == false)
        return true;

    // A script's pause (talking to a player) holds everything
    if (IsScriptPaused == true)
    {
        if (owner->movespline->Finalized() == false)
            owner->StopMoving();
        if (ScriptPauseTimeLeftInMS == 0)
            return true;
        if (ScriptPauseTimeLeftInMS > diff)
        {
            ScriptPauseTimeLeftInMS -= diff;
            return true;
        }
        IsScriptPaused = false;
        ScriptPauseTimeLeftInMS = 0;
        if (CurrentState == State::Travelling)
            StartTravel(owner);
        return true;
    }

    // Rooted, stunned or casting: stop where it is, and set off again once free
    if (owner->HasUnitState(UNIT_STATE_NOT_MOVE) || owner->IsMovementPreventedByCasting())
    {
        if (owner->movespline->Finalized() == false)
            owner->StopMoving();
        if (CurrentState == State::Travelling)
        {
            CurrentState = State::Waiting;
            WaitTimeLeftInMS = 0;
            IsResumingTravel = true;
        }
        return true;
    }

    ResetPosition = owner->GetPosition();
    HasResetPosition = true;

    if (CurrentState == State::Travelling)
    {
        if (owner->movespline->Finalized() == false)
            return true;

        // A long trip can arrive at the end of its path before the point (paths are capped), so it walks on from there
        if (owner->GetExactDist2d(TravelTarget.GetPositionX(), TravelTarget.GetPositionY()) > EQ_MOVEMENT_ARRIVE_DISTANCE
            && TravelLegCount < EQ_MOVEMENT_MAX_TRAVEL_LEGS)
        {
            StartTravel(owner);
            return true;
        }
        OnArrived(owner);
        return true;
    }

    // Waiting
    if (WaitTimeLeftInMS > diff)
    {
        WaitTimeLeftInMS -= diff;
        return true;
    }
    WaitTimeLeftInMS = 0;
    if (IsDepopPending == true)
    {
        // A one-way grid's end (the EQ server's depop; the spawn comes back on its timer)
        IsDepopPending = false;
        owner->DespawnOrUnsummon();
        return true;
    }
    if (IsResumingTravel == true)
    {
        IsResumingTravel = false;
        StartTravel(owner);
        return true;
    }
    PickNextPoint(owner);
    StartTravel(owner);
    return true;
}

void EverQuestMovementGenerator::StartTravel(Creature* owner)
{
    if (CurrentState != State::Travelling)
        TravelLegCount = 0;
    ++TravelLegCount;
    CurrentState = State::Travelling;
    owner->AddUnitState(UNIT_STATE_ROAMING_MOVE);

    Movement::MoveSplineInit init(owner);
    init.MoveTo(TravelTarget.GetPositionX(), TravelTarget.GetPositionY(), TravelTarget.GetPositionZ(), true, false);
    init.SetWalk(true);
    init.Launch();
}

void EverQuestMovementGenerator::OnArrived(Creature* owner)
{
    owner->ClearUnitState(UNIT_STATE_ROAMING_MOVE);
    CurrentState = State::Waiting;

    // Roamers loiter for a while (the spawn group's delay range)
    if (CreatureInstance.DoesRoam == true)
    {
        WaitTimeLeftInMS = GetRandomTimerInMS(CreatureInstance.RoamMinDelayInMS, CreatureInstance.RoamMaxDelayInMS);
        return;
    }

    // Grid walkers pause for the waypoint's time (a random path only pauses at its destination), turning to the waypoint's heading
    EverQuestCreatureWaypoint const& waypoint = Waypoints[CurrentWaypointIndex];
    uint32 pauseInSec = waypoint.PauseInSec;
    if (CreatureInstance.WanderType == EQ_GRID_RANDOM_PATH)
    {
        if (CurrentWaypointIndex != RandomPathDestinationIndex)
            pauseInSec = 0;
        else if (pauseInSec == 0)
            pauseInSec = Waypoints[0].PauseInSec != 0 ? Waypoints[0].PauseInSec : EQ_MOVEMENT_RANDOM_PATH_DEFAULT_PAUSE_IN_SEC;
    }
    WaitTimeLeftInMS = GetWaypointPauseInMS(pauseInSec);
    if (pauseInSec != 0 && waypoint.Heading >= 0.0f)
        owner->SetFacingTo(waypoint.Heading);

    // The end of a one-way grid depops the creature, after the last waypoint's pause
    if (IsOneWay() == true && CurrentWaypointIndex + 1 >= int32(Waypoints.size()))
        IsDepopPending = true;
}

void EverQuestMovementGenerator::PickNextPoint(Creature* owner)
{
    if (CreatureInstance.DoesRoam == true)
        PickRoamPoint(owner);
    else
        PickNextWaypoint(owner);
}

// The next waypoint (the EQ server's CalculateNewWaypoint)
void EverQuestMovementGenerator::PickNextWaypoint(Creature* owner)
{
    int32 waypointCount = int32(Waypoints.size());
    int32 oldIndex = CurrentWaypointIndex;
    switch (CreatureInstance.WanderType)
    {
        case EQ_GRID_CIRCULAR:
        {
            CurrentWaypointIndex = (CurrentWaypointIndex + 1) % waypointCount;
        } break;
        case EQ_GRID_RANDOM_10:
        {
            vector<int32> closestIndexes = GetClosestWaypointIndexes(owner, 10);
            CurrentWaypointIndex = closestIndexes[urand(0, uint32(closestIndexes.size() - 1))];
        } break;
        case EQ_GRID_RANDOM:
        {
            CurrentWaypointIndex = int32(urand(0, uint32(waypointCount - 1)));
            if (CurrentWaypointIndex == oldIndex)
            {
                if (oldIndex == waypointCount - 1)
                    CurrentWaypointIndex = oldIndex - 1;
                else if (oldIndex == 0)
                    CurrentWaypointIndex = oldIndex + 1;
            }
        } break;
        case EQ_GRID_PATROL:
        {
            if (CurrentWaypointIndex >= waypointCount - 1)
                IsPatrolGoingBack = true;
            else if (CurrentWaypointIndex <= 0)
                IsPatrolGoingBack = false;
            CurrentWaypointIndex += IsPatrolGoingBack ? -1 : 1;
        } break;
        case EQ_GRID_ONE_WAY_REPOP:
        case EQ_GRID_ONE_WAY_DEPOP:
        {
            CurrentWaypointIndex = min(CurrentWaypointIndex + 1, waypointCount - 1);
        } break;
        case EQ_GRID_RAND_5_LOS:
        {
            // The closest five in line of sight, or the closest five when none are
            vector<int32> closestIndexes = GetClosestWaypointIndexes(owner, 5);
            vector<int32> visibleIndexes;
            EQZoneMap const* eqZoneMap = owner->GetMap()->GetEQZoneMap();
            for (int32 index : closestIndexes)
            {
                EverQuestCreatureWaypoint const& waypoint = Waypoints[index];
                if (eqZoneMap == nullptr || eqZoneMap->IsInLineOfSight(owner->GetPositionX(), owner->GetPositionY(), owner->GetPositionZ() + EQ_MOVEMENT_LOS_HEIGHT,
                    waypoint.X, waypoint.Y, waypoint.Z + EQ_MOVEMENT_LOS_HEIGHT))
                    visibleIndexes.push_back(index);
            }
            if (visibleIndexes.empty() == true)
                visibleIndexes = closestIndexes;
            CurrentWaypointIndex = visibleIndexes[urand(0, uint32(visibleIndexes.size() - 1))];
        } break;
        case EQ_GRID_CENTER_POINT:
        {
            // Out from the first waypoint to a random other one, and back each time
            if (CurrentWaypointIndex != 0 || waypointCount < 2)
                CurrentWaypointIndex = 0;
            else
            {
                CurrentWaypointIndex = int32(urand(1, uint32(waypointCount - 1)));
                if (CurrentWaypointIndex == oldIndex && waypointCount > 2)
                    CurrentWaypointIndex = (CurrentWaypointIndex % (waypointCount - 1)) + 1;
            }
        } break;
        case EQ_GRID_RANDOM_CENTER_POINT:
        {
            // Between the waypoints marked as center points and the rest, in turn
            vector<int32> candidates;
            for (int32 i = 0; i < waypointCount; ++i)
                if (Waypoints[i].IsCenterPoint == IsCenterPointNext)
                    candidates.push_back(i);
            if (candidates.empty() == true)
                for (int32 i = 0; i < waypointCount; ++i)
                    candidates.push_back(i);
            CurrentWaypointIndex = candidates[urand(0, uint32(candidates.size() - 1))];
            IsCenterPointNext = !IsCenterPointNext;
        } break;
        case EQ_GRID_RANDOM_PATH:
        {
            // A random destination, preferring center points when leaving a waypoint that isn't one, walked to one waypoint at a time
            if (CurrentWaypointIndex == RandomPathDestinationIndex)
            {
                vector<int32> centerPoints;
                vector<int32> otherPoints;
                for (int32 i = 0; i < waypointCount; ++i)
                {
                    if (i == CurrentWaypointIndex)
                        continue;
                    if (Waypoints[i].IsCenterPoint == true)
                        centerPoints.push_back(i);
                    else
                        otherPoints.push_back(i);
                }
                vector<int32>& candidates = (Waypoints[CurrentWaypointIndex].IsCenterPoint == false && centerPoints.empty() == false) ? centerPoints : otherPoints;
                if (candidates.empty() == false)
                    RandomPathDestinationIndex = candidates[urand(0, uint32(candidates.size() - 1))];
            }
            if (RandomPathDestinationIndex > CurrentWaypointIndex)
                ++CurrentWaypointIndex;
            else if (RandomPathDestinationIndex < CurrentWaypointIndex)
                --CurrentWaypointIndex;
        } break;
        default:
        {
            CurrentWaypointIndex = (CurrentWaypointIndex + 1) % waypointCount;
        } break;
    }
    CurrentWaypointIndex = max(0, min(CurrentWaypointIndex, waypointCount - 1));
    EverQuestCreatureWaypoint const& waypoint = Waypoints[CurrentWaypointIndex];
    TravelTarget.Relocate(waypoint.X, waypoint.Y, waypoint.Z);
}

// A point in the roam box (the EQ server's roambox pick, quirks included: a random distance from the creature, on a random circle, measured from
// the box's EQ x range, reflected back into the box, and now and then straight back to the spawn point)
void EverQuestMovementGenerator::PickRoamPoint(Creature* owner)
{
    float currentX = owner->GetPositionX();
    float currentY = owner->GetPositionY();
    float edge = owner->GetMap()->GetEQZoneMap() ? owner->GetMap()->GetEQZoneMap()->GetWorldScale() : 0.29f; // One EQ unit

    // EQ's x is WoW's y
    float lowDistance = (currentY - CreatureInstance.RoamMinY) + edge;
    float highDistance = (CreatureInstance.RoamMaxY - currentY) - edge;
    float distance = frand(min(lowDistance, highDistance), max(lowDistance, highDistance));
    float distanceSquared = distance * distance;
    float xPartSquared = frand(0.0f, distanceSquared);
    float yPartSquared = distanceSquared - xPartSquared;
    float moveX = sqrt(xPartSquared) * (urand(0, 1) == 0 ? -1.0f : 1.0f);
    float moveY = sqrt(yPartSquared) * (urand(0, 1) == 0 ? -1.0f : 1.0f);
    float targetX = currentX + moveX;
    float targetY = currentY + moveY;
    if (targetX < CreatureInstance.RoamMinX || targetX > CreatureInstance.RoamMaxX)
        targetX -= moveX * 2.0f;
    if (targetY < CreatureInstance.RoamMinY || targetY > CreatureInstance.RoamMaxY)
        targetY -= moveY * 2.0f;
    if (targetX < CreatureInstance.RoamMinX || targetX > CreatureInstance.RoamMaxX)
        targetX = frand(min(CreatureInstance.RoamMinX + edge, CreatureInstance.RoamMaxX - edge), max(CreatureInstance.RoamMinX + edge, CreatureInstance.RoamMaxX - edge));
    if (targetY < CreatureInstance.RoamMinY || targetY > CreatureInstance.RoamMaxY)
        targetY = frand(min(CreatureInstance.RoamMinY + edge, CreatureInstance.RoamMaxY - edge), max(CreatureInstance.RoamMinY + edge, CreatureInstance.RoamMaxY - edge));
    float targetZ = owner->GetPositionZ();

    // Now and then it goes back to its spawn point, which keeps roamers from getting stuck in corners
    if (urand(1, 100) <= EQ_MOVEMENT_ROAM_RETURN_HOME_PERCENT)
        owner->GetHomePosition().GetPosition(targetX, targetY, targetZ);
    else if (owner->IsInWater() == false)
    {
        // The ground at the point, looked for from under the ceiling above the spawn point (a swimmer keeps its depth)
        if (EQZoneMap const* eqZoneMap = owner->GetMap()->GetEQZoneMap())
        {
            Position const& home = owner->GetHomePosition();
            float ceilingZ = eqZoneMap->GetCeilingZ(home.GetPositionX(), home.GetPositionY(), home.GetPositionZ() + EQ_MOVEMENT_ROAM_CEILING_START_HEIGHT, EQ_MOVEMENT_ROAM_CEILING_SEARCH);
            if (ceilingZ <= EQ_ZONE_MAP_INVALID_HEIGHT)
                ceilingZ = home.GetPositionZ() + EQ_MOVEMENT_ROAM_CEILING_SEARCH;
            float groundZ = eqZoneMap->GetFloorZ(targetX, targetY, ceilingZ - EQ_MOVEMENT_ROAM_CEILING_CLEARANCE, EQ_MOVEMENT_ROAM_GROUND_SEARCH);
            if (groundZ > EQ_ZONE_MAP_INVALID_HEIGHT)
                targetZ = groundZ;
        }
    }
    TravelTarget.Relocate(targetX, targetY, targetZ);
}

// The EQ server's waypoint pauses: pause type 0 is half random, 1 is the full time, 2 is fully random
uint32 EverQuestMovementGenerator::GetWaypointPauseInMS(uint32 pauseInSec) const
{
    if (pauseInSec == 0)
        return 0;
    switch (CreatureInstance.PauseType)
    {
        case EQ_GRID_PAUSE_TYPE_RANDOM_HALF: return (pauseInSec - urand(0, pauseInSec - 1) / 2) * IN_MILLISECONDS;
        case EQ_GRID_PAUSE_TYPE_FULL: return pauseInSec * IN_MILLISECONDS;
        case EQ_GRID_PAUSE_TYPE_RANDOM: return urand(0, pauseInSec - 1) * IN_MILLISECONDS;
        default: return pauseInSec * IN_MILLISECONDS;
    }
}

// The EQ server's random timer, which waits 14 seconds whenever the range is empty or backwards
uint32 EverQuestMovementGenerator::GetRandomTimerInMS(uint32 minInMS, uint32 maxInMS)
{
    if (minInMS == 0 || maxInMS == 0 || minInMS >= maxInMS)
        return EQ_MOVEMENT_DEFAULT_RANDOM_TIMER_IN_MS;
    return urand(minInMS, maxInMS);
}

int32 EverQuestMovementGenerator::GetClosestWaypointIndex(Creature* owner) const
{
    int32 closestIndex = 0;
    float closestDistance = FLT_MAX;
    for (int32 i = 0; i < int32(Waypoints.size()); ++i)
    {
        float distance = owner->GetExactDistSq(Waypoints[i].X, Waypoints[i].Y, Waypoints[i].Z);
        if (distance < closestDistance)
        {
            closestDistance = distance;
            closestIndex = i;
        }
    }
    return closestIndex;
}

vector<int32> EverQuestMovementGenerator::GetClosestWaypointIndexes(Creature* owner, uint32 count) const
{
    vector<pair<float, int32>> distances;
    for (int32 i = 0; i < int32(Waypoints.size()); ++i)
        distances.push_back(make_pair(owner->GetExactDistSq(Waypoints[i].X, Waypoints[i].Y, Waypoints[i].Z), i));
    sort(distances.begin(), distances.end());
    vector<int32> closestIndexes;
    for (uint32 i = 0; i < distances.size() && i < count; ++i)
        closestIndexes.push_back(distances[i].second);
    return closestIndexes;
}

MovementGenerator* SelectEverQuestIdleMovementGenerator(Creature* creature)
{
    if (EverQuest->IsEnabled == false || creature->GetSpawnId() == 0)
        return nullptr;
    EverQuestCreatureInstance const& creatureInstance = EverQuest->GetCreatureInstanceData(creature->GetSpawnId());
    if (creatureInstance.CreatureGUID == 0)
        return nullptr;
    vector<EverQuestCreatureWaypoint> const& waypoints = EverQuest->GetWaypoints(creatureInstance.MapID, creatureInstance.WaypointListID);

    // A grid needs two waypoints to walk (as on the EQ server), and a roam box needs room
    if (creatureInstance.DoesRoam == true)
    {
        if (creatureInstance.RoamMaxX - creatureInstance.RoamMinX < 1.0f || creatureInstance.RoamMaxY - creatureInstance.RoamMinY < 1.0f)
            return nullptr;
    }
    else if (waypoints.size() < 2)
        return nullptr;
    return new EverQuestMovementGenerator(creatureInstance, waypoints);
}
