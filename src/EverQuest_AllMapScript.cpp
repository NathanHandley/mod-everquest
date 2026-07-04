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

#include "Map.h"
#include "ScriptMgr.h"

#include "EverQuest.h"

class EverQuest_AllMapScript : public AllMapScript
{
public:
    EverQuest_AllMapScript() : AllMapScript("EverQuest_AllMapScript") {}

    void OnMapUpdate(Map* map, uint32 diff) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        uint32 mapID = map->GetId();
        if (mapID < EverQuest->ConfigSystemMapDBCIDMin || mapID > EverQuest->ConfigSystemMapDBCIDMax)
            return;
        EverQuest->UpdatePendingKillSpawnActions(map, diff);
    }
};

void AddEverQuestAllMapScripts()
{
    new EverQuest_AllMapScript();
}
