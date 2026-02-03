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

#include "ScriptMgr.h"
#include "Transport.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_TransportScript : public TransportScript
{
public:
    EverQuest_TransportScript() : TransportScript("EverQuest_TransportScript") {}

    void OnRelocate(Transport* transport, uint32 waypointId, uint32 mapId, float /*x*/, float /*y*/, float /*z*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Trigger any dependent ships
        for (EverQuestTransportShipTrigger shipTrigger : EverQuest->GetShipTriggersForShip(transport->GetEntry()))
        {
            // TEMP
            LOG_ERROR("module.EverQuest", "EverQuestMod::TransportTest {} - {}", transport->GetEntry(), waypointId);

            // Only trigger if a trigger point was reached
            if (waypointId != shipTrigger.TriggeringNodeID)
                continue;

            LOG_ERROR("module.EverQuest", "EverQuestMod::A");

            // Get the triggered ship, respawning if needed
            GameObject* triggeredShipGameObject = EverQuest->ShipGameObjectsByTemplateEntryID[shipTrigger.TriggeredShipGameObjectTemplateEntryID];
            Transport* triggerShipTransport = triggeredShipGameObject->ToTransport();
            if (triggerShipTransport->IsInWorld() == false)
                triggerShipTransport->Respawn();
            MotionTransport* triggerShipMotionTransport = dynamic_cast<MotionTransport*>(triggerShipTransport);

            LOG_ERROR("module.EverQuest", "EverQuestMod::B TriggeredID {}, Node {}", triggeredShipGameObject->GetEntry(), shipTrigger.TriggerActivateNodeID);

            // Jump the timer to the node required
            auto& frames = triggerShipMotionTransport->GetKeyFrames();
            uint32 jumpTime = 0;
            bool foundNodeTime = false;
            for (auto const& frame : frames)
            {
                if (frame.Node->index == shipTrigger.TriggerActivateNodeID - 1) // Previous Node
                {
                    jumpTime = frame.DepartureTime;
                    foundNodeTime = true;
                    LOG_ERROR("module.EverQuest", "EverQuestMod::C");
                    break;
                }
            }
            if (foundNodeTime == true)
            {
                triggerShipMotionTransport->SetPathProgress(jumpTime);
                triggerShipMotionTransport->EnableMovement(true);
                triggerShipMotionTransport->Update(1);
                LOG_ERROR("module.EverQuest", "EverQuestMod::D jumpTime {} - PathProgress() {}", jumpTime, triggerShipMotionTransport->GetPathProgress());
            }
            else
                LOG_ERROR("module.EverQuest", "Ship Trigger: Could not find node {} for ship entry {}", shipTrigger.TriggerActivateNodeID, transport->GetEntry());
        }          
    }
};

void AddEverQuestTransportScripts()
{
    new EverQuest_TransportScript();
}
