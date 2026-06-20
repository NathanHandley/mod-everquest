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

#include "ScriptMgr.h"
#include "SpellInfo.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_GlobalScript: public GlobalScript
{
public:
    EverQuest_GlobalScript() : GlobalScript("EverQuest_GlobalScript") {}

    void OnLoadSpellCustomAttr(SpellInfo* spell) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (spell == nullptr)
            return;

        // Used to keep worn spell effects on creatures to persist when the creature evades
        if (EverQuest->IsWornEffectSpell(spell->Id))
        {
            spell->AttributesCu |= SPELL_ATTR0_CU_IGNORE_EVADE;
            spell->AttributesEx3 |= SPELL_ATTR3_ALLOW_AURA_WHILE_DEAD;
        }

        // Only adjust EQ-generated spells
        if (spell->Id < EverQuest->ConfigSystemSpellDBCIDMin || spell->Id > EverQuest->ConfigSystemSpellDBCIDMax)
            return;
        if (EverQuest->IsSpellAnEQSpell(spell->Id) == false)
            return;

        bool hasHarmfulPeriodic = false;
        bool allSelfTargeted = true;
        bool hasPositiveEffect = false;
        bool hasNegativeEffect = false;
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spell->Effects[i].IsEffect() == false)
                continue;

            switch (spell->Effects[i].ApplyAuraName)
            {
                case SPELL_AURA_PERIODIC_DAMAGE:
                case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
                case SPELL_AURA_PERIODIC_LEECH:
                case SPELL_AURA_PERIODIC_MANA_LEECH:
                case SPELL_AURA_POWER_BURN:
                    hasHarmfulPeriodic = true;
                    break;
                default:
                    break;
            }

            uint32 targetA = spell->Effects[i].TargetA.GetTarget();
            if (targetA != TARGET_UNIT_CASTER && targetA != 0)
                allSelfTargeted = false;

            if (spell->IsPositiveEffect(i))
                hasPositiveEffect = true;
            else
                hasNegativeEffect = true;
        }

        if (EverQuest->ConfigSpellDisableStackingOfSameDOT == true)
        {
            if (hasHarmfulPeriodic)
                spell->AttributesCu |= SPELL_ATTR0_CU_SINGLE_AURA_STACK;
        }

        // By default in WoW, buffs that have a cost are treated as a debuff (can't be removed).  That's not EQ behavior.
        if (allSelfTargeted && hasPositiveEffect && hasNegativeEffect)
        {
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                if (spell->Effects[i].IsEffect() == false)
                    continue;
                spell->AttributesCu &= ~(SPELL_ATTR0_CU_NEGATIVE_EFF0 << i);
                spell->AttributesCu |= (SPELL_ATTR0_CU_POSITIVE_EFF0 << i);
            }
        }
    }

    bool OnItemRoll(Player const* /*player*/, LootStoreItem const* lootStoreItem, float& chance, Loot& loot, LootStore const& /*lootStore*/) override
    {
        if (EverQuest->IsEnabled == false)
            return true;

        // For any items that are on prerolled creatures, restrict drops to align to what was prerolled
        if (EverQuest->HasPreloadedLootItemIDsForCreatureGUID(loot.sourceWorldObjectGUID) == false)
            return true;

        if (EverQuest->HasPreloadedLootItemIDForCreatureGUID(loot.sourceWorldObjectGUID, lootStoreItem->itemid))
            chance = 100.0f;
        else
            chance = 0.0f;
        return true;
    }

    void OnBeforeDropAddItem(Player const* /*player*/, Loot& loot, bool /*canRate*/, uint16 /*lootMode*/, LootStoreItem* lootStoreItem, LootStore const& /*store*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        if (EverQuest->HasPreloadedLootItemIDsForCreatureGUID(loot.sourceWorldObjectGUID) == false)
            return;

        uint32 prerolledCount = EverQuest->GetPreloadedLootCountForCreatureGUID(loot.sourceWorldObjectGUID, lootStoreItem->itemid);
        if (prerolledCount > 0)
        {
            // Clamp so a large quantity doesn't truncate vs the uint8 limit
            uint8 clampedCount = prerolledCount > 255 ? 255 : uint8(prerolledCount);
            lootStoreItem->mincount = clampedCount;
            lootStoreItem->maxcount = clampedCount;
        }
    }

    // Called when GroupID > 0 and chance == 0
    bool OnBeforeLootEqualChanced(Player const* /*player*/, list<LootStoreItem*> /*equalChanced*/, Loot& loot, LootStore const& /*lootStore*/) override
    {
        if (EverQuest->IsEnabled == false)
            return true;

        // Fail it so only prerolled items drop
        if (EverQuest->HasPreloadedLootItemIDsForCreatureGUID(loot.sourceWorldObjectGUID))
            return false;

        return true;
    }
};

void AddEverQuestGlobalScripts()
{
    new EverQuest_GlobalScript();
}
