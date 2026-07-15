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
    // Ships on different maps relocate/update on different map threads, so guard this shared map
    std::mutex PendingResyncMutex;
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

    // Looks up a registered ship and returns it as a MotionTransport, or nullptr if it was never registered
    // (its map may not be loaded yet) or isn't a moving transport
    MotionTransport* GetRegisteredShipMotionTransport(uint32 shipGameObjectTemplateEntryID)
    {
        GameObject* shipGameObject = nullptr;
        {
            std::lock_guard<std::mutex> lock(EverQuest->RuntimeStateMutex);
            auto shipIt = EverQuest->ShipGameObjectsByTemplateEntryID.find(shipGameObjectTemplateEntryID);
            if (shipIt != EverQuest->ShipGameObjectsByTemplateEntryID.end())
                shipGameObject = shipIt->second;
        }
        if (shipGameObject == nullptr)
            return nullptr;
        Transport* shipTransport = shipGameObject->ToTransport();
        if (shipTransport == nullptr)
            return nullptr;
        return dynamic_cast<MotionTransport*>(shipTransport);
    }

    void StorePendingResync(uint32 shipGameObjectTemplateEntryID, GOState goState)
    {
        std::lock_guard<std::mutex> lock(PendingResyncMutex);
        PendingResync[shipGameObjectTemplateEntryID] = goState;
    }

public:
    EverQuest_TransportScript() : TransportScript("EverQuest_TransportScript") {}

    bool IsEQShipEntry(uint32 gameObjectTemplateEntryID)
    {
        if (gameObjectTemplateEntryID < EverQuest->ConfigSystemShipEntryTemplateIDMin || gameObjectTemplateEntryID > EverQuest->ConfigSystemShipEntryTemplateIDMax)
            return false;
        return true;
    }

    void OnRelocate(Transport* transport, uint32 waypointId, uint32 /*mapId*/, float /*x*/, float /*y*/, float /*z*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (IsEQShipEntry(transport->GetEntry()) == false)
            return;

        // Pause any triggered ships
        auto waitNodeIt = EverQuest->ShipWaitNodesByGameObjectTemplateEntryID.find(transport->GetEntry());
        if (waitNodeIt != EverQuest->ShipWaitNodesByGameObjectTemplateEntryID.end() && waypointId == (uint32)waitNodeIt->second)
        {
            MotionTransport* waitingShipMotionTransport = GetRegisteredShipMotionTransport(transport->GetEntry());
            if (waitingShipMotionTransport != nullptr)
            {
                waitingShipMotionTransport->EnableMovement(false);
                StorePendingResync(transport->GetEntry(), GO_STATE_READY);
            }
        }

        // Trigger any dependent ships
        for (const EverQuestTransportShipTrigger& shipTrigger : EverQuest->GetShipTriggersForShip(transport->GetEntry()))
        {
            // Only trigger if a trigger point was reached
            if (waypointId != shipTrigger.TriggeringNodeID)
                continue;

            // Get the triggered ship, respawning if needed
            MotionTransport* triggeredShipMotionTransport = GetRegisteredShipMotionTransport(shipTrigger.TriggeredShipGameObjectTemplateEntryID);
            if (triggeredShipMotionTransport == nullptr)
                continue;
            if (triggeredShipMotionTransport->IsInWorld() == false)
                triggeredShipMotionTransport->Respawn();

            // Restart movement
            triggeredShipMotionTransport->EnableMovement(true);
            StorePendingResync(shipTrigger.TriggeredShipGameObjectTemplateEntryID, GO_STATE_ACTIVE);
        }
    }

    void OnUpdate(Transport* transport, uint32 /*diff*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (IsEQShipEntry(transport->GetEntry()) == false)
            return;

        // Force any needed client states
        {
            std::lock_guard<std::mutex> lock(PendingResyncMutex);
            auto it = PendingResync.find(transport->GetEntry());
            if (it == PendingResync.end() || transport->GetGoState() != it->second)
                return;
            PendingResync.erase(it);
        }
        ForceTransportResyncToPlayers(transport);
    }
};

void AddEverQuestTransportScripts()
{
    new EverQuest_TransportScript();
}
