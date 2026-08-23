//  Author: Nathan Handley (nathanhandley@protonmail.com)
//  Copyright (c) 2025 Nathan Handley
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

#include "Configuration/Config.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "Transport.h"

#include "EverQuest.h"

#include <map>
#include <mutex>
#include <utility>

#define LIFT_KELETHIN_NORTH_ENTRY                   279902
#define LIFT_KELETHIN_CENTER_ENTRY                  279903
#define LIFT_KELETHIN_EAST_ENTRY                    279904
#define LIFT_KELETHIN_NORTH_TRIGGER_BOTTOM_ENTRY    279905
#define LIFT_KELETHIN_NORTH_TRIGGER_TOP_ENTRY       279906
#define LIFT_KELETHIN_CENTER_TRIGGER_BOTTOM_ENTRY   279907
#define LIFT_KELETHIN_CENTER_TRIGGER_TOP_ENTRY      279908
#define LIFT_KELETHIN_EAST_TRIGGER_BOTTOM_ENTRY     279909
#define LIFT_KELETHIN_EAST_TRIGGER_TOP_ENTRY        279910

#define LIFT_PAINEEL_ENTRY                          279911
#define LIFT_PAINEEL_TOP_TRIGGER_ENTRY              279912
#define LIFT_PAINEEL_BOTTOM_TRIGGER_ENTRY           279913

using namespace std;

class EverQuest_AllGameObjectScript: public AllGameObjectScript
{
public:
    EverQuest_AllGameObjectScript() : AllGameObjectScript("EverQuest_AllGameObjectScript") {}

    // Lifts are tracked per map instance rather than in flat members, since every instance copy of a zone has its own lift objects with their own GUIDs
    std::mutex LiftGUIDsMutex;
    std::map<std::pair<uint64, uint32>, ObjectGuid> LiftGUIDsByMapInstanceKeyAndEntry;

    static bool IsLiftEntry(uint32 entryID)
    {
        switch (entryID)
        {
        case LIFT_KELETHIN_NORTH_ENTRY:
        case LIFT_KELETHIN_CENTER_ENTRY:
        case LIFT_KELETHIN_EAST_ENTRY:
        case LIFT_PAINEEL_ENTRY:
            return true;
        default:
            return false;
        }
    }

    void StoreLiftGUID(GameObject* go)
    {
        std::lock_guard<std::mutex> lock(LiftGUIDsMutex);
        LiftGUIDsByMapInstanceKeyAndEntry[std::make_pair(EverQuest->GetMapInstanceKey(go->GetMap()), go->GetEntry())] = go->GetGUID();
    }

    void ClearLiftGUID(GameObject* go)
    {
        std::lock_guard<std::mutex> lock(LiftGUIDsMutex);
        auto liftIt = LiftGUIDsByMapInstanceKeyAndEntry.find(std::make_pair(EverQuest->GetMapInstanceKey(go->GetMap()), go->GetEntry()));
        if (liftIt != LiftGUIDsByMapInstanceKeyAndEntry.end() && liftIt->second == go->GetGUID())
            LiftGUIDsByMapInstanceKeyAndEntry.erase(liftIt);
    }

    ObjectGuid GetLiftGUID(Map* map, uint32 liftEntryID)
    {
        std::lock_guard<std::mutex> lock(LiftGUIDsMutex);
        auto liftIt = LiftGUIDsByMapInstanceKeyAndEntry.find(std::make_pair(EverQuest->GetMapInstanceKey(map), liftEntryID));
        if (liftIt == LiftGUIDsByMapInstanceKeyAndEntry.end())
            return ObjectGuid::Empty;
        return liftIt->second;
    }

    void OnGameObjectAddWorld(GameObject* go) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (go->GetEntry() < EverQuest->ConfigSystemGameObjectTemplateIDMin || go->GetEntry() > EverQuest->ConfigSystemGameObjectTemplateIDMax)
            return;

        // In the core, Instances don't apply the 'state' for door/button types of objects, so they need to be done here otherwise drawbridges will be down etc
        if (go->FindMap() != nullptr && go->GetMap()->Instanceable() == true && go->GetInstanceScript() == nullptr)
        {
            if (GameObjectData const* gameObjectData = go->GetGameObjectData())
            {
                if (go->GetGoState() != gameObjectData->go_state)
                    go->SetGoState(gameObjectData->go_state);
            }
        }

        // Capture lifts
        if (IsLiftEntry(go->GetEntry()) == true)
            StoreLiftGUID(go);

        // Capture ships, along with the map they belong to so a trigger running on another map's thread can tell that it must not touch them directly
        if (go->GetEntry() >= EverQuest->ConfigSystemShipEntryTemplateIDMin && go->GetEntry() <= EverQuest->ConfigSystemShipEntryTemplateIDMax)
        {
            EverQuestRegisteredShip registeredShip;
            registeredShip.ShipGameObject = go;
            registeredShip.MapID = go->GetMapId();
            std::lock_guard<std::mutex> lock(EverQuest->RuntimeStateMutex);
            EverQuest->ShipGameObjectsByTemplateEntryID[go->GetEntry()] = registeredShip;
        }
    }

    void OnGameObjectRemoveWorld(GameObject* go) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Clear cached lift GUIDs so nothing points at an object that left the world
        if (IsLiftEntry(go->GetEntry()) == true)
            ClearLiftGUID(go);

        // Unregister ships so the trigger system can't call through a deleted GameObject
        if (go->GetEntry() >= EverQuest->ConfigSystemShipEntryTemplateIDMin && go->GetEntry() <= EverQuest->ConfigSystemShipEntryTemplateIDMax)
        {
            std::lock_guard<std::mutex> lock(EverQuest->RuntimeStateMutex);
            auto shipIt = EverQuest->ShipGameObjectsByTemplateEntryID.find(go->GetEntry());
            if (shipIt != EverQuest->ShipGameObjectsByTemplateEntryID.end() && shipIt->second.ShipGameObject == go)
                EverQuest->ShipGameObjectsByTemplateEntryID.erase(shipIt);
        }
    }

    void ProcessLiftTrigger(GameObject* platformGameObject)
    {
        if (platformGameObject)
        {
            if (StaticTransport* staticTransport = platformGameObject->ToStaticTransport())
            {
                if (staticTransport->GetGoState() == GO_STATE_ACTIVE && staticTransport->GetPathProgress() == staticTransport->GetPauseTime())
                    staticTransport->SetGoState(GO_STATE_READY);
                else if (staticTransport->GetGoState() == GO_STATE_READY && staticTransport->GetPathProgress() == 0)
                    staticTransport->SetGoState(GO_STATE_ACTIVE);
            }
        }
    }

    void OnGameObjectStateChanged(GameObject* go, uint32 state) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (go->GetEntry() < EverQuest->ConfigSystemGameObjectTemplateIDMin || go->GetEntry() > EverQuest->ConfigSystemGameObjectTemplateIDMax)
            return;

        // Skip if not in world
        if (go->IsInWorld() == false)
            return;

        // Lifts
        if (state == 0)
        {
            switch (go->GetEntry())
            {
            case LIFT_KELETHIN_NORTH_TRIGGER_BOTTOM_ENTRY:
            case LIFT_KELETHIN_NORTH_TRIGGER_TOP_ENTRY:
            {
                ProcessLiftTrigger(go->GetMap()->GetGameObject(GetLiftGUID(go->GetMap(), LIFT_KELETHIN_NORTH_ENTRY)));
            } break;
            case LIFT_KELETHIN_CENTER_TRIGGER_BOTTOM_ENTRY:
            case LIFT_KELETHIN_CENTER_TRIGGER_TOP_ENTRY:
            {
                ProcessLiftTrigger(go->GetMap()->GetGameObject(GetLiftGUID(go->GetMap(), LIFT_KELETHIN_CENTER_ENTRY)));
            } break;
            case LIFT_KELETHIN_EAST_TRIGGER_BOTTOM_ENTRY:
            case LIFT_KELETHIN_EAST_TRIGGER_TOP_ENTRY:
            {
                ProcessLiftTrigger(go->GetMap()->GetGameObject(GetLiftGUID(go->GetMap(), LIFT_KELETHIN_EAST_ENTRY)));
            } break;
            case LIFT_PAINEEL_BOTTOM_TRIGGER_ENTRY:
            case LIFT_PAINEEL_TOP_TRIGGER_ENTRY:
            {
                ProcessLiftTrigger(go->GetMap()->GetGameObject(GetLiftGUID(go->GetMap(), LIFT_PAINEEL_ENTRY)));
            } break;
            default: break;
            }
        }
    }
};

void AddEverQuestAllGameObjectScripts()
{
    new EverQuest_AllGameObjectScript();
}
