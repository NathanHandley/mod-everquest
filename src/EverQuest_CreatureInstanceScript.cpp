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
        Position CurrentTargetPos;
        bool IsMovingToWaypoint = false;

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

            me->SetWalk(false); // true = walk
            me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);

            LoadCustomData();
            if (CreatureWaypoints.empty() == false)
                StartMovingToNextWaypoint();
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

        // Added this since not taking mini-steps caused a flying/floating issue over gaps and through mountains
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

            // For Z, do multiple steps to catch inclines properly
            float newZ = me->GetPositionZ() + 8.0f;
            me->UpdateGroundPositionZ(newX, newY, newZ);
            newZ += 3.0f;
            me->UpdateGroundPositionZ(newX, newY, newZ);
            newZ += 2.0f;
            me->UpdateGroundPositionZ(newX, newY, newZ);

            me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);

            // Move to this small terrain-aligned point
            me->GetMotionMaster()->MovePoint(EQ_MOVE_SMALL_TERRAIN_MOVE_ID, newX, newY, newZ);
        }

        void FinishCurrentWaypoint()
        {
            IsMovingToWaypoint = false;
            const EverQuestCreatureWaypoint& wp = CreatureWaypoints[CurrentWaypointIndex];

            if (wp.PauseInSec > 0)
            {
                events.ScheduleEvent(EVENT_PAUSE_DONE, Seconds(wp.PauseInSec));
            }
            else
            {
                CurrentWaypointIndex++;
                StartMovingToNextWaypoint();
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == EQ_MOVE_RETURN_TO_AGRO_ID)
            {
                StartMovingToNextWaypoint();
                return;
            }

            if (id == EQ_MOVE_SMALL_TERRAIN_MOVE_ID && IsMovingToWaypoint)
            {
                events.ScheduleEvent(EVENT_NEXT_SMALL_STEP, 50ms);
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
                    StartMovingToNextWaypoint();
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

            if (me->GetDistance(AgroPosition) > 3.0f)
            {
                float x = AgroPosition.GetPositionX();
                float y = AgroPosition.GetPositionY();
                float z = AgroPosition.GetPositionZ() + 8.0f;

                // Cast twice to better handle steep ground
                me->UpdateGroundPositionZ(x, y, z);
                z += 3.0f;
                me->UpdateGroundPositionZ(x, y, z);

                me->SetCanFly(false);
                me->SetDisableGravity(false);
                me->SetHover(false);
                me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);

                me->GetMotionMaster()->MovePoint(EQ_MOVE_RETURN_TO_AGRO_ID, x, y, z);
            }
            else
            {
                StartMovingToNextWaypoint();
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
