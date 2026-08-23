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

#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"

#include "EverQuest.h"

using namespace std;

// Attached by the converter (areatrigger_scripts) to every zone line whose destination zone has an instance copy
class EverQuest_ZoneLineAreaTriggerScript : public AreaTriggerScript
{
public:
    EverQuest_ZoneLineAreaTriggerScript() : AreaTriggerScript("EverQuest_ZoneLineAreaTriggerScript") {}

    bool OnTrigger(Player* player, AreaTrigger const* trigger) override
    {
        if (EverQuest->IsEnabled == false)
            return false;

        // A raid still inside (or a corpse left behind) wins over dungeon mode, since a zone can have both kinds of instance copy
        if (EverQuest->TryZoneLineIntoInstanceRaidLow(player, trigger) == true)
            return true;

        return EverQuest->TryZoneLineIntoInstanceDungeon(player, trigger);
    }
};

void AddEverQuestAreaTriggerScripts()
{
    new EverQuest_ZoneLineAreaTriggerScript();
}
