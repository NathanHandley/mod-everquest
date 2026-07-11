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

#include "Item.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellInfo.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_ItemScript : public ItemScript
{
public:
    EverQuest_ItemScript() : ItemScript("EverQuest_ItemScript") {}

    bool OnCastItemCombatSpell(Player* player, Unit* victim, SpellInfo const* spellInfo, Item* /*item*/) override
    {
        if (EverQuest->IsEnabled == false)
            return true;
        if (spellInfo->Id < EverQuest->ConfigSystemSpellDBCIDMin || spellInfo->Id > EverQuest->ConfigSystemSpellDBCIDMax)
            return true;

        // In EverQuest, a proc buff outlives the weapon that trigger it, so needs to cast in WoW without linking to the item
        player->CastSpell(victim, spellInfo->Id, TriggerCastFlags(TRIGGERED_FULL_MASK & ~TRIGGERED_IGNORE_SPELL_AND_CATEGORY_CD));
        return false;
    }
};

void AddEverQuestItemScripts()
{
    new EverQuest_ItemScript();
}
