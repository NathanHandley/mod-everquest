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

#include "LootMgr.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"

#include "EverQuest.h"
#include "EverQuest_SpellTalentAlignment.h"

using namespace std;

class EverQuest_GlobalScript: public GlobalScript
{
public:
    EverQuest_GlobalScript() : GlobalScript("EverQuest_GlobalScript") {}

    bool CouldLootSourceBePrerolledEQCreature(Loot& loot, LootStore const& lootStore)
    {
        // Pick Pocket loot is its own loot table and is not prerolled, so the filter needs to ignore it
        if (&lootStore == &LootTemplates_Pickpocketing)
            return false;
        if (loot.sourceWorldObjectGUID.IsCreature() == false)
            return false;
        uint32 entryID = loot.sourceWorldObjectGUID.GetEntry();
        if (entryID < EverQuest->ConfigSystemCreatureTemplateIDMin || entryID > EverQuest->ConfigSystemCreatureTemplateIDMax)
            return false;
        return true;
    }

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

        // A few WOW talent auras hide behind AuraEffect::IsAffectedOnSpell, which has no runtime hook the way spell modifiers do (see EverQuest_SpellTalentAlignment),
        // so the only way to let them reach EverQuest spells is to adjust their loaded spell data
        if (EverQuest->ConfigSpellTalentAlignmentEnabled == true)
        {
            // Shatter's crit bonus against frozen targets: family 0 is a wildcard in IsAffectedOnSpell, letting EverQuest spells benefit
            // The mage's other spells benefit too, which the stock mask nearly covered anyway
            if (spell->Id == EQ_SPELL_ID_MAGE_SHATTER_RANK1 || spell->Id == EQ_SPELL_ID_MAGE_SHATTER_RANK2 || spell->Id == EQ_SPELL_ID_MAGE_SHATTER_RANK3)
                spell->SpellFamilyName = SPELLFAMILY_GENERIC;

            // Fingers of Frost's "treated as frozen" buff has a similar condition.  The wildcard makes every aura-state check by this caster pass while the buff is up, which is safe for a mage
            if (spell->Id == EQ_SPELL_ID_MAGE_FINGERS_OF_FROST_BUFF)
                spell->SpellFamilyName = SPELLFAMILY_GENERIC;

            // Frost Channeling's mana cost reduction only covers the three mage schools, so EverQuest spells of other schools would/ miss it.  Widening to all magic schools changes nothing for the mage's own spells, which all sit inside the original mask
            if (spell->Id == EQ_SPELL_ID_MAGE_FROST_CHANNELING_RANK1 || spell->Id == EQ_SPELL_ID_MAGE_FROST_CHANNELING_RANK2 || spell->Id == EQ_SPELL_ID_MAGE_FROST_CHANNELING_RANK3)
            {
                if (spell->Effects[EFFECT_0].ApplyAuraName == SPELL_AURA_MOD_POWER_COST_SCHOOL_PCT)
                    spell->Effects[EFFECT_0].MiscValue = SPELL_SCHOOL_MASK_MAGIC;
            }

            // Renewed Hope's crit bonus against Weakened Soul targets sits behind the same IsAffectedOnSpell gate as Shatter, so the wildcard treatment lets EverQuest heals benefit (the Weakened Soul condition itself stays in the core)
            if (spell->Id == EQ_SPELL_ID_PRIEST_RENEWED_HOPE_RANK1 || spell->Id == EQ_SPELL_ID_PRIEST_RENEWED_HOPE_RANK2)
                spell->SpellFamilyName = SPELLFAMILY_GENERIC;
        }

        // Only adjust EQ-generated spells
        if (spell->Id < EverQuest->ConfigSystemSpellDBCIDMin || spell->Id > EverQuest->ConfigSystemSpellDBCIDMax)
            return;
        if (EverQuest->IsSpellAnEQSpell(spell->Id) == false)
            return;

        bool hasHarmfulPeriodic = false;
        bool hasAuraEffect = false;
        bool allSelfTargeted = true;
        bool hasPositiveEffect = false;
        bool hasNegativeEffect = false;
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spell->Effects[i].IsEffect() == false)
                continue;

            if (spell->Effects[i].ApplyAuraName != 0)
                hasAuraEffect = true;

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

        // In EQ, a recast of the same spell should refresh it
        if (hasAuraEffect == true && EverQuest->IsWornEffectSpell(spell->Id) == false)
            spell->AttributesCu |= SPELL_ATTR0_CU_SINGLE_AURA_STACK;

        // Self buffs that tick damage shouldn't break effects
        if (allSelfTargeted && hasHarmfulPeriodic)
            spell->AttributesEx4 |= SPELL_ATTR4_DAMAGE_DOESNT_BREAK_AURAS;

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

    bool OnItemRoll(Player const* /*player*/, LootStoreItem const* lootStoreItem, float& chance, Loot& loot, LootStore const& lootStore) override
    {
        if (EverQuest->IsEnabled == false)
            return true;
        if (CouldLootSourceBePrerolledEQCreature(loot, lootStore) == false)
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

    void OnBeforeDropAddItem(Player const* /*player*/, Loot& loot, bool /*canRate*/, uint16 /*lootMode*/, LootStoreItem* lootStoreItem, LootStore const& store) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (CouldLootSourceBePrerolledEQCreature(loot, store) == false)
            return;

        if (EverQuest->HasPreloadedLootItemIDsForCreatureGUID(loot.sourceWorldObjectGUID) == false)
            return;

        uint32 prerolledCount = EverQuest->GetPreloadedLootCountForCreatureGUID(loot.sourceWorldObjectGUID, lootStoreItem->itemid);
        if (prerolledCount > 0)
        {
            // Clamp so a large quantity doesn't truncate vs the uint8 limit
            uint8 clampedCount = prerolledCount > 255 ? 255 : uint8(prerolledCount);
            // Note: this writes to the SHARED loot store item, which the core reads right after this hook returns.
            // Every drop sets it before use so it self-corrects, but two maps looting the same template at the same
            // instant can cross counts (worst case: wrong stack size). Fixing that fully needs a core hook change.
            lootStoreItem->mincount = clampedCount;
            lootStoreItem->maxcount = clampedCount;
        }
    }

    // Called when GroupID > 0 and chance == 0
    bool OnBeforeLootEqualChanced(Player const* /*player*/, list<LootStoreItem*> /*equalChanced*/, Loot& loot, LootStore const& lootStore) override
    {
        if (EverQuest->IsEnabled == false)
            return true;
        if (CouldLootSourceBePrerolledEQCreature(loot, lootStore) == false)
            return true;

        // Fail it so only prerolled items drop
        if (EverQuest->HasPreloadedLootItemIDsForCreatureGUID(loot.sourceWorldObjectGUID))
            return false;

        return true;
    }

    bool OnIsAffectedBySpellModCheck(SpellInfo const* affectSpell, SpellInfo const* checkSpell, SpellModifier const* mod) override
    {
        // To make talents work with EQ spells, need to bypass this check that'll never match due to family class mismatches
        if (EverQuest->IsEnabled == false)
            return true;
        if (EverQuest->ConfigSpellTalentAlignmentEnabled == false)
            return true;
        if (affectSpell == nullptr || checkSpell == nullptr || mod == nullptr)
            return true;
        if (EverQuestTalentAlignment->ShouldTalentModAffectEQSpell(affectSpell, checkSpell, mod) == true)
            return false;
        return true;
    }
};

void AddEverQuestGlobalScripts()
{
    new EverQuest_GlobalScript();
}
