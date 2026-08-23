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

#include <mutex>
#include <set>

using namespace std;

class EverQuest_TransportScript : public TransportScript
{
private:
    // Ships on different maps relocate/update on different map threads, so guard these shared maps
    std::mutex PendingResyncMutex;
    std::map<uint32, GOState> PendingResync;
    std::mutex PendingStartsMutex;
    std::set<uint32> PendingStartsByShipEntryID;

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

    // Looks up a registered ship and returns it as a MotionTransport, along with the map it is registered on.  Returns nullptr if it was never
    // registered (its map may not be loaded yet) or isn't a moving transport
    MotionTransport* GetRegisteredShipMotionTransport(uint32 shipGameObjectTemplateEntryID, uint32& shipMapIDOut)
    {
        GameObject* shipGameObject = nullptr;
        shipMapIDOut = 0;
        {
            std::lock_guard<std::mutex> lock(EverQuest->RuntimeStateMutex);
            auto shipIt = EverQuest->ShipGameObjectsByTemplateEntryID.find(shipGameObjectTemplateEntryID);
            if (shipIt != EverQuest->ShipGameObjectsByTemplateEntryID.end())
            {
                shipGameObject = shipIt->second.ShipGameObject;
                shipMapIDOut = shipIt->second.MapID;
            }
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

    void QueuePendingShipStart(uint32 shipGameObjectTemplateEntryID)
    {
        std::lock_guard<std::mutex> lock(PendingStartsMutex);
        PendingStartsByShipEntryID.insert(shipGameObjectTemplateEntryID);
    }

    bool ConsumePendingShipStart(uint32 shipGameObjectTemplateEntryID)
    {
        std::lock_guard<std::mutex> lock(PendingStartsMutex);
        return PendingStartsByShipEntryID.erase(shipGameObjectTemplateEntryID) > 0;
    }

    // Only ever called on the ship's own map thread
    void StartShip(MotionTransport* shipMotionTransport)
    {
        if (shipMotionTransport->IsInWorld() == false)
            shipMotionTransport->Respawn();
        shipMotionTransport->EnableMovement(true);
        StorePendingResync(shipMotionTransport->GetEntry(), GO_STATE_ACTIVE);
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

        // Pause this ship if it just reached its own wait node.  This is the relocating ship itself, so it is always safe to act on here
        auto waitNodeIt = EverQuest->ShipWaitNodesByGameObjectTemplateEntryID.find(transport->GetEntry());
        if (waitNodeIt != EverQuest->ShipWaitNodesByGameObjectTemplateEntryID.end() && waypointId == (uint32)waitNodeIt->second)
        {
            MotionTransport* waitingShipMotionTransport = dynamic_cast<MotionTransport*>(transport);
            if (waitingShipMotionTransport != nullptr)
            {
                waitingShipMotionTransport->EnableMovement(false);
                StorePendingResync(transport->GetEntry(), GO_STATE_READY);
            }
        }

        // Trigger any dependent ships
        uint32 relocatingShipMapID = transport->GetMapId();
        for (const EverQuestTransportShipTrigger& shipTrigger : EverQuest->GetShipTriggersForShip(transport->GetEntry()))
        {
            // Only trigger if a trigger point was reached
            if (waypointId != shipTrigger.TriggeringNodeID)
                continue;

            // Get the triggered ship
            uint32 triggeredShipMapID = 0;
            MotionTransport* triggeredShipMotionTransport = GetRegisteredShipMotionTransport(shipTrigger.TriggeredShipGameObjectTemplateEntryID, triggeredShipMapID);
            if (triggeredShipMotionTransport == nullptr)
                continue;

            // A ship on another map updates on another map's thread, so respawning and restarting it from here would be writing to that map's
            // objects from the wrong thread.  Hand it off instead, and that ship picks the start up on its own next update
            if (triggeredShipMapID != relocatingShipMapID)
            {
                QueuePendingShipStart(shipTrigger.TriggeredShipGameObjectTemplateEntryID);
                continue;
            }

            StartShip(triggeredShipMotionTransport);
        }
    }

    void OnUpdate(Transport* transport, uint32 /*diff*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (IsEQShipEntry(transport->GetEntry()) == false)
            return;

        // Pick up a start handed over by a trigger that fired on another map's thread
        if (ConsumePendingShipStart(transport->GetEntry()) == true)
        {
            MotionTransport* shipMotionTransport = dynamic_cast<MotionTransport*>(transport);
            if (shipMotionTransport != nullptr)
                StartShip(shipMotionTransport);
        }

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
