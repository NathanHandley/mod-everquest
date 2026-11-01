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
    uint32 focusPercent = EverQuest->GetItemFocusPercentForPlayer(spell->GetCaster()->ToPlayer(), EQ_ITEM_FOCUS_TYPE_MANA_COST, spell->GetSpellInfo());
    return powerCost - int32(float(powerCost) * float(focusPercent) / 100.0f);
}

// Worn reagent focuses roll to keep a spell's reagents
static bool SaveEverQuestSpellReagents(Spell const* spell)
{
    if (EverQuest->IsEnabled == false || spell->GetCaster() == nullptr || spell->GetCaster()->IsPlayer() == false)
        return false;
    uint32 focusPercent = EverQuest->GetItemFocusPercentForPlayer(spell->GetCaster()->ToPlayer(), EQ_ITEM_FOCUS_TYPE_REAGENT, spell->GetSpellInfo());
    return focusPercent != 0 && roll_chance_i(int32(focusPercent));
}

// A worn range focus reaches the client as one spell mod for every spell, so what the focus doesn't cover comes back off here
static float ModifyEverQuestSpellRange(Unit const* caster, SpellInfo const* spellInfo, float range)
{
    if (EverQuest->IsEnabled == false || caster->IsPlayer() == false)
        return range;
    Player* player = const_cast<Player*>(caster->ToPlayer());
    uint32 appliedPercent = 0;
    if (player->HasAura(EQ_ITEM_FOCUS_RANGE_SPELL_ID_LONG) == true)
        appliedPercent = EQ_ITEM_FOCUS_RANGE_PERCENT_LONG;
    else if (player->HasAura(EQ_ITEM_FOCUS_RANGE_SPELL_ID_SHORT) == true)
        appliedPercent = EQ_ITEM_FOCUS_RANGE_PERCENT_SHORT;
    if (appliedPercent == 0)
        return range;
    uint32 coveredPercent = EverQuest->GetItemFocusPercentForPlayer(player, EQ_ITEM_FOCUS_TYPE_RANGE, spellInfo);
    return range * float(100 + coveredPercent) / float(100 + appliedPercent);
}

// Worn hate focuses raise or lower the threat the wearer's spells make
static float ModifyEverQuestSpellThreat(Unit const* caster, SpellInfo const* spellInfo, float threat)
{
    if (EverQuest->IsEnabled == false || caster == nullptr || caster->IsPlayer() == false)
        return threat;
    Player* player = const_cast<Player*>(caster->ToPlayer());
    int32 percent = int32(EverQuest->GetItemFocusPercentForPlayer(player, EQ_ITEM_FOCUS_TYPE_HATE_INCREASE, spellInfo));
    percent -= int32(EverQuest->GetItemFocusPercentForPlayer(player, EQ_ITEM_FOCUS_TYPE_HATE_DECREASE, spellInfo));
    return threat * float(100 + percent) / 100.0f;
}

void AddEverQuestPlayerScripts();
void AddEverQuestWorldScripts();
void AddEverQuestUnitScripts();
void AddEverQuestAllGameObjectScripts();
void AddEverQuestGameEventScripts();
void AddEverQuestAllCreatureScripts();
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

    // Game masters skip area trigger scripts, so the core hands their EverQuest zone lines over through this
    EverQuestAreaTriggerHooks::GameMasterZoneLineHandler = &HandleEverQuestZoneLineAreaTriggerForGameMaster;
    AddEverQuestGossipScripts();
    AddEverQuestMiscScripts();
    AddEverQuestAreaTriggerScripts();
    AddEverQuestSpellEffectScripts();
}
