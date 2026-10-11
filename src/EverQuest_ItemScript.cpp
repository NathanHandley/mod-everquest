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
#include "SpellScript.h"

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

        EverQuest->CastWeaponProcSpell(player, victim, spellInfo->Id);
        return false;
    }
};

class EverQuest_AllItemScript : public AllItemScript
{
public:
    EverQuest_AllItemScript() : AllItemScript("EverQuest_AllItemScript") {}

    bool CanItemRemove(Player* player, Item* item) override
    {
        if (EverQuest->IsEnabled == false)
            return true;

        // A transform destroys the old version of the item, and this is the last moment its enchantment can still be read off it
        EverQuest->RememberItemEnchantForTransformedItem(player, item);

        // Returning true lets the core carry on and destroy the item
        return true;
    }
};

// Slotshift Transform targets an item in the bags like Disenchant does, and turns it into the version of it worn in the next slot
class EverQuest_SlotshiftTransformSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_SlotshiftTransformSpellScript);

    SpellCastResult CheckCast()
    {
        if (EverQuest->IsEnabled == false)
            return SPELL_FAILED_DONT_REPORT;
        Player* player = GetCaster() != nullptr ? GetCaster()->ToPlayer() : nullptr;
        if (player == nullptr)
            return SPELL_FAILED_DONT_REPORT;
        if (EverQuest->CanPlayerSlotshiftItem(player, GetExplTargetItem(), true) == false)
            return SPELL_FAILED_DONT_REPORT;
        return SPELL_CAST_OK;
    }

    void HandleSlotshiftTransform(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetCaster() != nullptr ? GetCaster()->ToPlayer() : nullptr;
        if (player == nullptr)
            return;
        EverQuest->SlotshiftItemForPlayer(player, GetExplTargetItem(), GetSpell());
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(EverQuest_SlotshiftTransformSpellScript::CheckCast);
        OnEffectHit += SpellEffectFn(EverQuest_SlotshiftTransformSpellScript::HandleSlotshiftTransform, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

void AddEverQuestItemScripts()
{
    new EverQuest_ItemScript();
    new EverQuest_AllItemScript();
    RegisterSpellScript(EverQuest_SlotshiftTransformSpellScript);
}
