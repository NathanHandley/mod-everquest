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

    ObjectGuid KelethinNorthLiftGUID;
    ObjectGuid KelethinCenterLiftGUID;
    ObjectGuid KelethinEastLiftGUID;
    ObjectGuid PaineelLiftGUID;
    ObjectGuid PaineelLiftTriggerTop;
    ObjectGuid PaineelLiftTriggerBottom;
    bool PaineelLiftTriggerTopInitialized = false;
    bool PaineelLiftTriggerBottomInitialized = false;

    void OnGameObjectAddWorld(GameObject* go) override
    {
        switch (go->GetEntry())
        {
        case LIFT_KELETHIN_NORTH_ENTRY: KelethinNorthLiftGUID = go->GetGUID(); break;
        case LIFT_KELETHIN_CENTER_ENTRY: KelethinCenterLiftGUID = go->GetGUID(); break;
        case LIFT_KELETHIN_EAST_ENTRY: KelethinEastLiftGUID = go->GetGUID(); break;
        case LIFT_PAINEEL_ENTRY: PaineelLiftGUID = go->GetGUID(); break;
        case LIFT_PAINEEL_TOP_TRIGGER_ENTRY: PaineelLiftTriggerTop = go->GetGUID(); break;
        case LIFT_PAINEEL_BOTTOM_TRIGGER_ENTRY: PaineelLiftTriggerBottom = go->GetGUID(); break;
        default: break;
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
        // Kelethin lifts
        if (state == 0)
        {
            switch (go->GetEntry())
            {
            case LIFT_KELETHIN_NORTH_TRIGGER_BOTTOM_ENTRY:
            case LIFT_KELETHIN_NORTH_TRIGGER_TOP_ENTRY:
            {
                ProcessLiftTrigger(go->GetMap()->GetGameObject(KelethinNorthLiftGUID));
            } break;
            case LIFT_KELETHIN_CENTER_TRIGGER_BOTTOM_ENTRY:
            case LIFT_KELETHIN_CENTER_TRIGGER_TOP_ENTRY:
            {
                ProcessLiftTrigger(go->GetMap()->GetGameObject(KelethinCenterLiftGUID));
            } break;
            case LIFT_KELETHIN_EAST_TRIGGER_BOTTOM_ENTRY:
            case LIFT_KELETHIN_EAST_TRIGGER_TOP_ENTRY:
            {
                ProcessLiftTrigger(go->GetMap()->GetGameObject(KelethinEastLiftGUID));
            } break;
            default: break;
            }
        }

        // Paineel lifts
        switch (go->GetEntry())
        {
            case LIFT_PAINEEL_BOTTOM_TRIGGER_ENTRY:
            {
                if (PaineelLiftTriggerBottomInitialized == true)
                {
                    ProcessLiftTrigger(go->GetMap()->GetGameObject(PaineelLiftGUID));
                    GameObject* topTrigger = go->GetMap()->GetGameObject(PaineelLiftTriggerTop);
                    if (topTrigger != nullptr && topTrigger->GetGoState() != go->GetGoState())
                        topTrigger->SetGoState(go->GetGoState());
                }
                else
                    PaineelLiftTriggerBottomInitialized = true;
            } break;
            case LIFT_PAINEEL_TOP_TRIGGER_ENTRY:
            {
                if (PaineelLiftTriggerTopInitialized == true)
                {
                    ProcessLiftTrigger(go->GetMap()->GetGameObject(PaineelLiftGUID));
                    GameObject* bottomTrigger = go->GetMap()->GetGameObject(PaineelLiftTriggerBottom);
                    if (bottomTrigger != nullptr && bottomTrigger->GetGoState() != go->GetGoState())
                        bottomTrigger->SetGoState(go->GetGoState());
                }
                else
                    PaineelLiftTriggerTopInitialized = true;
            } break;
        default: break;
        }
    }
};

void AddEverQuestAllGameObjectScripts()
{
    new EverQuest_AllGameObjectScript();
}
