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

#include "Player.h"
#include "CreatureScript.h"
#include "InstanceScript.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_EastDockmaster : public CreatureScript
{
public:
    EverQuest_EastDockmaster() : CreatureScript("eq_eastdockmaster") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 /*action*/) override
    {
        // return false to display last windows
        return false;
    }
};

void AddEverQuestCreatureScripts()
{
    new EverQuest_EastDockmaster();
}
