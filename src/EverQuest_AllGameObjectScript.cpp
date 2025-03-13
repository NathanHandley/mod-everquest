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

using namespace std;

class EverQuest_AllGameObjectScript: public AllGameObjectScript
{
public:
    EverQuest_AllGameObjectScript() : AllGameObjectScript("EverQuest_AllGameObjectScript") {}

    ObjectGuid KelethinNorthLiftGUID;
    ObjectGuid KelethinCenterLiftGUID;
    ObjectGuid KelethinEastLiftGUID;

    void OnGameObjectAddWorld(GameObject* go) override
    {
        switch (go->GetEntry())
        {
        case 279902: KelethinNorthLiftGUID = go->GetGUID(); break;
        case 279903: KelethinCenterLiftGUID = go->GetGUID(); break;
        case 279904: KelethinEastLiftGUID = go->GetGUID(); break;
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
        if (state != 0) // 0 = Pressed, 1 = Released
            return;

        switch (go->GetEntry())
        {
        case 279905: // Kelethin North Lift Button - Lower
        case 279906: // Kelethin North Lift Button - Upper
        {
            ProcessLiftTrigger(go->GetMap()->GetGameObject(KelethinNorthLiftGUID));
        } break;
        case 279907: // Kelethin East Lift Button - Lower
        case 279908: // Kelethin East Lift Button - Upper
        {
            ProcessLiftTrigger(go->GetMap()->GetGameObject(KelethinCenterLiftGUID));
        } break;
        case 279909: // Kelethin East Lift Button - Lower
        case 279910: // Kelethin East Lift Button - Upper
        {
            ProcessLiftTrigger(go->GetMap()->GetGameObject(KelethinEastLiftGUID));
        } break;
        default: break;
        }
    }
};

void AddEverQuestAllGameObjectScripts()
{
    new EverQuest_AllGameObjectScript();
}
