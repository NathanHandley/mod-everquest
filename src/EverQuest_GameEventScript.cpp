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
#include "ScriptMgr.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_GameEventScript: public GameEventScript
{
public:
    EverQuest_GameEventScript() : GameEventScript("EverQuest_GameEventScript") {}

    // Runs on start event
    void OnStart(uint16 EventID) override
    {
        if (EventID == EverQuest->ConfigSystemDayEventID || EventID == EverQuest->ConfigSystemNightEventID)
            EverQuest->SetAllLoadedPlayersDayOrNightAura();
    }

    // Runs on stop event
    void OnStop(uint16 EventID) override
    {
        if (EventID == EverQuest->ConfigSystemDayEventID || EventID == EverQuest->ConfigSystemNightEventID)
            EverQuest->SetAllLoadedPlayersDayOrNightAura();
    }
};

void AddEverQuestGameEventScripts()
{
    new EverQuest_GameEventScript();
}
