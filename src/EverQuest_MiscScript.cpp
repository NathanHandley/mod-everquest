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

#include "LootMgr.h"
#include "Player.h"
#include "ScriptMgr.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_MiscScript : public MiscScript
{
public:
    EverQuest_MiscScript() : MiscScript("EverQuest_MiscScript") {}

    void OnAfterLootTemplateProcess(Loot* loot, LootTemplate const* /*lootTemplate*/, LootStore const& /*store*/, Player* lootOwner, bool personal, bool /*noEmptyError*/, uint16 /*lootMode*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        EverQuest->ApplyZoneWideGroupLootAccess(loot, lootOwner, personal);
    }
};

void AddEverQuestMiscScripts()
{
    new EverQuest_MiscScript();
}
