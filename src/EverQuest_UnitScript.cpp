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

#include "Unit.h"
#include "ScriptMgr.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_UnitScript : public UnitScript
{
public:
    EverQuest_UnitScript() : UnitScript("EverQuest_UnitScript") {}

    bool IfNormalReaction(Unit const* unit, Unit const* target, ReputationRank& repRank)
    {
        // Skip if the unit is a player
        if (unit->IsPlayer() == true)
            return true;

        // Skip if the target isn't a player
        if (target->IsPlayer() == false)
            return true;

        // Always return a friendly response if it's a druid/hunter for the
        uint32 unitFaction = 0;
        if (unit->GetFaction() == EverQuest->DruidHunterFriendlyFactionTemplateID)
        {
            if (target->getClass() == CLASS_HUNTER || target->getClass() == CLASS_DRUID)
            {
                repRank = REP_FRIENDLY;
                return false;
            }
        }

        // Do nothing special at this point
        return true;
    }
};

void AddEverQuestUnitScripts()
{
    new EverQuest_UnitScript();
}
