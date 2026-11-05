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

#include "EQAreaTriggerHooks.h"
#include "EQMovementHooks.h"
#include "EverQuest.h"
#include "EverQuest_MovementGenerator.h"
#include "Spell.h"

// A worn focus lowers what the wearer's spells cost
static int32 ModifyEverQuestSpellPowerCost(Spell const* spell, int32 powerCost)
{
    if (powerCost <= 0 || EverQuest->IsEnabled == false || spell->GetCaster() == nullptr || spell->GetCaster()->IsPlayer() == false)
        return powerCost;
    // TAKP gives an item's cast no mana cost focus
    if (spell->m_CastItem != nullptr)
        return powerCost;
    // A focus past 100% in the data can't make the spell pay the caster
    uint32 focusPercent = std::min<uint32>(EverQuest->GetItemFocusPercentForPlayer(spell->GetCaster()->ToPlayer(), EQ_ITEM_FOCUS_TYPE_MANA_COST, spell->GetSpellInfo()), 100);
    if (focusPercent == 0)
        return powerCost;

    // As TAKP's Mob::SpellFinished: the saving is cut down to whole mana, and a focus that applies always saves at least one
    int32 savedMana = int32((int64(powerCost) * int64(focusPercent)) / 100);
    if (savedMana == 0)
        savedMana = 1;
    return std::max<int32>(0, powerCost - savedMana);
}

// EverQuest's Destroy is the only EQ effect the converter makes an instant kill.  The core asks this on the map thread the spell hits on
static bool HandleEverQuestSpellInstaKill(Spell* spell, Unit* target)
{
    if (EverQuest->IsEnabled == false || spell == nullptr || target == nullptr)
        return false;
    return EverQuest->HandleDestroySpellHit(spell, target);
}

// Worn reagent focuses roll to keep a spell's reagents.  As TAKP's Mob::HasSpellReagent, each unit of a reagent rolls on its own, and the few reagents EQ's
// client never lets a focus keep are always taken.  The core asks once for each reagent a spell names, on the caster's map thread
static uint32 SaveEverQuestSpellReagents(Spell const* spell, uint32 itemID, uint32 itemCount)
{
    if (EverQuest->IsEnabled == false || itemCount == 0 || spell->GetCaster() == nullptr || spell->GetCaster()->IsPlayer() == false)
        return 0;
    if (EverQuest->IsReagentExemptFromConservation(itemID) == true)
        return 0;
    uint32 focusPercent = EverQuest->GetItemFocusPercentForPlayer(spell->GetCaster()->ToPlayer(), EQ_ITEM_FOCUS_TYPE_REAGENT, spell->GetSpellInfo());
    if (focusPercent == 0)
        return 0;
    uint32 savedCount = 0;
    for (uint32 unitIndex = 0; unitIndex < itemCount; ++unitIndex)
    {
        if (roll_chance_i(int32(focusPercent)) == true)
            ++savedCount;
    }
    return savedCount;
}

// A worn range focus reaches the client as one spell mod for every spell, so what the focus doesn't cover comes back off here.  The core gives that mod to
// every spell that takes spell mods, and so not to one that ignores caster modifiers (every fixed damage EQ spell), and it gives the owner's to a pet's or a
// totem's spells.  The core asks this everywhere it applies the range mod (the range check, the channel range, and SpellInfo::GetMaxRange with a caster,
// which picks nearby targets), always on the caster's map thread
static float ModifyEverQuestSpellRange(Unit const* caster, SpellInfo const* spellInfo, float range)
{
    // No worn focus data (any server before Luclin) means no range focus aura to correct for
    if (EverQuest->IsEnabled == false || caster == nullptr || spellInfo == nullptr || EverQuest->ItemFocusByItemTemplateID.empty() == true)
        return range;
    Player* modOwner = caster->GetSpellModOwner();
    if (modOwner == nullptr)
        return range;
    uint32 appliedPercent = 0;
    if (spellInfo->IsAffectedBySpellMods() == true)
    {
        if (modOwner->HasAura(EQ_ITEM_FOCUS_RANGE_SPELL_ID_LONG) == true)
            appliedPercent = EQ_ITEM_FOCUS_RANGE_PERCENT_LONG;
        else if (modOwner->HasAura(EQ_ITEM_FOCUS_RANGE_SPELL_ID_SHORT) == true)
            appliedPercent = EQ_ITEM_FOCUS_RANGE_PERCENT_SHORT;
    }

    // A focus is for its wearer's own spells, never a pet's
    uint32 coveredPercent = caster->IsPlayer() == true ? EverQuest->GetItemFocusPercentForPlayer(modOwner, EQ_ITEM_FOCUS_TYPE_RANGE, spellInfo) : 0;
    if (appliedPercent == coveredPercent)
        return range;
    return range * float(100 + coveredPercent) / float(100 + appliedPercent);
}

// Worn hate focuses raise or lower the threat the wearer's spells make.  Only real threat is changed, as a focus scaling a spell's threat reduction would make it
// reduce more, and a decrease of 100% or more stops at none rather than turning to a reduction
static float ModifyEverQuestSpellThreat(Unit const* caster, SpellInfo const* spellInfo, float threat)
{
    if (EverQuest->IsEnabled == false || caster == nullptr || caster->IsPlayer() == false || threat <= 0.0f)
        return threat;
    Player* player = const_cast<Player*>(caster->ToPlayer());
    int32 percent = int32(EverQuest->GetItemFocusPercentForPlayer(player, EQ_ITEM_FOCUS_TYPE_HATE_INCREASE, spellInfo));
    percent -= int32(EverQuest->GetItemFocusPercentForPlayer(player, EQ_ITEM_FOCUS_TYPE_HATE_DECREASE, spellInfo));
    return threat * std::max(0.0f, float(100 + percent) / 100.0f);
}

// An EQ pet whose owner hovers over a spot with no ground comes in at its owner, on a Luclin server
static bool ShouldEverQuestPetLoadAtOwner(Pet const* pet)
{
    if (EverQuest->IsEnabled == false)
        return false;
    return EverQuest->ShouldPetLoadAtOwnerWhenPositionInvalid(pet);
}

void AddEverQuestPlayerScripts();
void AddEverQuestWorldScripts();
void AddEverQuestUnitScripts();
void AddEverQuestAllGameObjectScripts();
void AddEverQuestGameEventScripts();
void AddEverQuestAllCreatureScripts();
void AddEverQuestAccountScripts();
void AddEverQuestAllMapScripts();
void AddEverQuestAllSpellScripts();
void AddEverQuestFocusBoostAuraScripts();
void AddEverQuestFocusBoostNonAuraScripts();
void AddEverQuestBardSongAuraScripts();
void AddEverQuestCharmAuraScripts();
void AddEverQuestMesmerizeAuraScripts();
void AddEverQuestIllusionSpellScripts();
void AddEverQuestResistDiffSpellScripts();
void AddEverQuestPlayerCasterOnlySpellScripts();
void AddEverQuestFrostbiteChillSpellScripts();
void AddEverQuestManaGainSpellPowerSpellScripts();
void AddEverQuestLifeCostSpellScripts();
void AddEverQuestFixedPercentHealSpellScripts();
void AddEverQuestHarmTouchSpellScripts();
void AddEverQuestDamageShieldMarkScripts();
void AddEverQuestIntenseHealingExhaustionScripts();
void AddEverQuestClassAuraScripts();
void AddEverQuestRainTargetBudgetScripts();
void AddEverQuestIntensifyingPeriodicAuraScripts();
void AddEverQuestTalentInteractionScripts();
void AddEverQuestItemScripts();
void AddEverQuestPetScripts();
void AddEverQuestCommandScripts();
void AddEverQuestServerScripts();
void AddEverQuestGlobalScripts();
void AddEverQuestTransportScripts();
void AddEverQuestGossipScripts();
void AddEverQuestMiscScripts();
void AddEverQuestAreaTriggerScripts();
bool HandleEverQuestZoneLineAreaTriggerForGameMaster(Player* player, AreaTrigger const* trigger);
void AddEverQuestSpellEffectScripts();

void Addmod_everquestScripts()
{
    AddEverQuestWorldScripts();
    AddEverQuestPlayerScripts();
    AddEverQuestUnitScripts();
    AddEverQuestAllGameObjectScripts();
    AddEverQuestGameEventScripts();
    AddEverQuestAllCreatureScripts();
    AddEverQuestAccountScripts();
    AddEverQuestAllMapScripts();
    AddEverQuestAllSpellScripts();
    AddEverQuestFocusBoostAuraScripts();
    AddEverQuestFocusBoostNonAuraScripts();
    AddEverQuestBardSongAuraScripts();
    AddEverQuestCharmAuraScripts();
    AddEverQuestMesmerizeAuraScripts();
    AddEverQuestIllusionSpellScripts();
    AddEverQuestResistDiffSpellScripts();
    AddEverQuestPlayerCasterOnlySpellScripts();
    AddEverQuestFrostbiteChillSpellScripts();
    AddEverQuestManaGainSpellPowerSpellScripts();
    AddEverQuestLifeCostSpellScripts();
    AddEverQuestFixedPercentHealSpellScripts();
    AddEverQuestHarmTouchSpellScripts();
    AddEverQuestDamageShieldMarkScripts();
    AddEverQuestIntenseHealingExhaustionScripts();
    AddEverQuestClassAuraScripts();
    AddEverQuestRainTargetBudgetScripts();
    AddEverQuestIntensifyingPeriodicAuraScripts();
    AddEverQuestTalentInteractionScripts();
    AddEverQuestItemScripts();
    AddEverQuestPetScripts();
    AddEverQuestCommandScripts();
    AddEverQuestServerScripts();
    AddEverQuestGlobalScripts();
    AddEverQuestTransportScripts();

    // EverQuest grids and roam boxes are the idle movement of spawns on EverQuest maps (the core asks through this hook)
    EverQuestMovementHooks::IdleMovementSelector = &SelectEverQuestIdleMovementGenerator;
    EverQuestMovementHooks::SpellPowerCostModifier = &ModifyEverQuestSpellPowerCost;
    EverQuestMovementHooks::SpellReagentSaver = SaveEverQuestSpellReagents;
    EverQuestMovementHooks::SpellRangeModifier = ModifyEverQuestSpellRange;
    EverQuestMovementHooks::SpellThreatModifier = ModifyEverQuestSpellThreat;
    EverQuestMovementHooks::PetLoadAtOwnerSelector = ShouldEverQuestPetLoadAtOwner;
    EverQuestMovementHooks::SpellInstaKillHandler = HandleEverQuestSpellInstaKill;

    // Game masters skip area trigger scripts, so the core hands their EverQuest zone lines over through this
    EverQuestAreaTriggerHooks::GameMasterZoneLineHandler = &HandleEverQuestZoneLineAreaTriggerForGameMaster;
    AddEverQuestGossipScripts();
    AddEverQuestMiscScripts();
    AddEverQuestAreaTriggerScripts();
    AddEverQuestSpellEffectScripts();
}
