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
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "EventMap.h"
#include "MotionMaster.h"

#include "EverQuest.h"

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
        Position TargetWaypointPosition;

        enum Events
        {
            EVENT_PAUSE_DONE = 1
        };
        EventMap events;

        void LoadCustomData()
        {
            CreatureWaypoints.clear();

            uint32 creatureGUID = me->GetGUID().GetCounter();
            CreatureInstanceData = EverQuest->GetCreatureInstanceData(creatureGUID);
            if (CreatureInstanceData.WaypointListID != -1)
                CreatureWaypoints = EverQuest->GetWaypoints(CreatureInstanceData.MapID, CreatureInstanceData.WaypointListID);
        }

        void Reset() override
        {
            events.Reset();
            CurrentWaypointIndex = 0;

            LoadCustomData();

            if (CreatureWaypoints.empty() == false)
                MoveToNextWaypoint();
        }

        void MoveToNextWaypoint()
        {
            if (CreatureWaypoints.empty())
                return;

            // Loop back to start after last waypoint
            if (CurrentWaypointIndex >= CreatureWaypoints.size())
                CurrentWaypointIndex = 0;

            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[CurrentWaypointIndex];
            TargetWaypointPosition = Position(wp.X, wp.Y, wp.Z);

            // Motion with collision detection and environment contouring
            me->GetMotionMaster()->MovePoint(10000 + CurrentWaypointIndex, TargetWaypointPosition);
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            // Special ID used when returning to aggro position after evade
            if (id == EQ_MOVE_RETURN_TO_AGRO_ID)
            {
                MoveToNextWaypoint();
                return;
            }

            // Did we reach the waypoint we were heading to?
            if (id >= 10000 && (id - 10000) == CurrentWaypointIndex && !CreatureWaypoints.empty())
            {
                const EverQuestCreatureWaypoint& wp = CreatureWaypoints[CurrentWaypointIndex];

                if (wp.PauseInSec > 0)
                {
                    events.ScheduleEvent(EVENT_PAUSE_DONE, Seconds(wp.PauseInSec));
                }
                else
                {
                    CurrentWaypointIndex++;
                    MoveToNextWaypoint();
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
                    CurrentWaypointIndex++;
                    MoveToNextWaypoint();
                }
            }

            if (UpdateVictim() == false)
                return;

            DoMeleeAttackIfReady();
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            events.CancelEvent(EVENT_PAUSE_DONE);

            // Remember exact spot where aggro happened, and stop movement
            AgroPosition = me->GetPosition();
            me->GetMotionMaster()->Clear(false);
        }

        void EnterEvadeMode(EvadeReason why) override
        {
            ScriptedAI::EnterEvadeMode(why);

            // Path back to the exact point where aggro started, then continue the waypoint sequence
            if (me->GetDistance(AgroPosition) > 3.0f)
            {
                me->GetMotionMaster()->MovePoint(EQ_MOVE_RETURN_TO_AGRO_ID, AgroPosition);
            }
            else
            {
                MoveToNextWaypoint();
            }
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
