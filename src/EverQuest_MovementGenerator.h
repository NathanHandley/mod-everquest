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

#ifndef EVERQUEST_MOVEMENT_GENERATOR_H
#define EVERQUEST_MOVEMENT_GENERATOR_H

#include "MovementGenerator.h"
#include "Position.h"

#include "EverQuest.h"

// The idle movement of an EQ spawn that walks a grid or roams a box, done the way the EQ server (TAKP) did it: the same choice of next
// waypoint for every wander type, the same pauses for every pause type, the same roam box picks and loiter times, arriving facing the
// waypoint's heading, and one-way grids depopping at their end.  Walking between points is a plain move, which on an EverQuest map the
// core paths the EQ way (EQPathBuilder).  After combat the creature carries on from where it was, as EQ's movers did
class EverQuestMovementGenerator : public MovementGeneratorMedium<Creature, EverQuestMovementGenerator>
{
public:
    EverQuestMovementGenerator(EverQuestCreatureInstance const& creatureInstance, vector<EverQuestCreatureWaypoint> const& waypoints);

    void DoInitialize(Creature* owner);
    void DoFinalize(Creature* owner);
    void DoReset(Creature* owner);
    bool DoUpdate(Creature* owner, uint32 diff);
    MovementGeneratorType GetMovementGeneratorType() override { return EVERQUEST_MOTION_TYPE; }
    bool GetResetPosition(float& x, float& y, float& z) override;
    void Pause(uint32 timer) override;
    void Resume(uint32 overrideTimer) override;

private:
    enum class State
    {
        Waiting,    // Paused or loitering, then picks the next point
        Travelling  // Walking to the current point
    };

    void StartTravel(Creature* owner);
    void OnArrived(Creature* owner);
    void PickNextPoint(Creature* owner);
    void PickNextWaypoint(Creature* owner);
    void PickRoamPoint(Creature* owner);
    uint32 GetWaypointPauseInMS(uint32 pauseInSec) const;
    int32 GetClosestWaypointIndex(Creature* owner) const;
    vector<int32> GetClosestWaypointIndexes(Creature* owner, uint32 count) const;
    bool IsOneWay() const;
    static uint32 GetRandomTimerInMS(uint32 minInMS, uint32 maxInMS);

    EverQuestCreatureInstance CreatureInstance;
    vector<EverQuestCreatureWaypoint> const& Waypoints;
    State CurrentState = State::Waiting;
    uint32 WaitTimeLeftInMS = 0;
    uint32 ScriptPauseTimeLeftInMS = 0;
    bool IsScriptPaused = false;
    bool IsDepopPending = false;
    bool IsResumingTravel = false;
    uint32 TravelLegCount = 0;
    int32 CurrentWaypointIndex = 0;
    bool IsPatrolGoingBack = false;
    int32 RandomPathDestinationIndex = 0;
    bool IsCenterPointNext = false;
    Position TravelTarget;
    Position ResetPosition;
    bool HasResetPosition = false;
};

// The EverQuest idle movement for a creature on an EverQuest map, or nullptr when the spawn has no grid or roam box (core hook, see EQMovementHooks.h)
MovementGenerator* SelectEverQuestIdleMovementGenerator(Creature* creature);

#endif
