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
#include "Player.h"
#include "Transport.h"

#include "MapReference.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_TransportScript : public TransportScript
{
private:
    std::map<uint32, GOState> PendingResync;
    void ForceTransportResyncToPlayers(Transport* transport)
    {
        // Force updates with client so that players see the server values set
        Map::PlayerList const& players = transport->GetMap()->GetPlayers();
        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            if (Player* player = itr->GetSource())
            {
                transport->DestroyForPlayer(player);
                transport->SendUpdateToPlayer(player);
            }
        }
    }

public:
    EverQuest_TransportScript() : TransportScript("EverQuest_TransportScript") {}

    void OnRelocate(Transport* transport, uint32 waypointId, uint32 /*mapId*/, float /*x*/, float /*y*/, float /*z*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Pause any triggered ships
        if (EverQuest->ShipWaitNodesByGameObjectTemplateEntryID.find((int)transport->GetEntry()) != EverQuest->ShipWaitNodesByGameObjectTemplateEntryID.end())
        {
            if (waypointId == EverQuest->ShipWaitNodesByGameObjectTemplateEntryID[(int)transport->GetEntry()])
            {
                GameObject* triggeredShipGameObject = EverQuest->ShipGameObjectsByTemplateEntryID[transport->GetEntry()];
                Transport* triggerShipTransport = triggeredShipGameObject->ToTransport();
                MotionTransport* triggerShipMotionTransport = dynamic_cast<MotionTransport*>(triggerShipTransport);
                triggerShipMotionTransport->EnableMovement(false);
                PendingResync[transport->GetEntry()] = GO_STATE_READY;
            }
        }

        // Trigger any dependent ships
        for (EverQuestTransportShipTrigger shipTrigger : EverQuest->GetShipTriggersForShip(transport->GetEntry()))
        {
            // Only trigger if a trigger point was reached
            if (waypointId != shipTrigger.TriggeringNodeID)
                continue;

            // Get the triggered ship, respawning if needed
            GameObject* triggeredShipGameObject = EverQuest->ShipGameObjectsByTemplateEntryID[shipTrigger.TriggeredShipGameObjectTemplateEntryID];
            Transport* triggerShipTransport = triggeredShipGameObject->ToTransport();
            if (triggerShipTransport->IsInWorld() == false)
                triggerShipTransport->Respawn();
            MotionTransport* triggerShipMotionTransport = dynamic_cast<MotionTransport*>(triggerShipTransport);

            // Restart movement
            triggerShipMotionTransport->EnableMovement(true);
            PendingResync[shipTrigger.TriggeredShipGameObjectTemplateEntryID] = GO_STATE_ACTIVE;
        }          
    }

    void OnUpdate(Transport* transport, uint32 /*diff*/) override
    {
        // Force any needed client states
        auto it = PendingResync.find(transport->GetEntry());
        if (it != PendingResync.end() && transport->GetGoState() == it->second)
        {
            ForceTransportResyncToPlayers(transport);
            PendingResync.erase(it);
        }
    }
};

void AddEverQuestTransportScripts()
{
    new EverQuest_TransportScript();
}
