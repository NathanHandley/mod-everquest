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

#include "Cell.h"
#include "CellImpl.h"
#include "GameTime.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "ObjectAccessor.h"
#include "Pet.h"
#include "Player.h"
#include "Random.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Unit.h"
#include "EverQuest.h"
#include <algorithm>
#include <cmath>

using namespace std;

static const char* EQ_CLASSAURA_SPELL_TYPE_NAMES[EQ_CLASSAURA_SPELL_TYPE_COUNT] =
{
    "EnchanterPassive", "EnchanterAura", "EnchanterFocus",
    "BardPassive", "BardAura", "BardInstrument",
    "MonkPassive", "MonkAura", "MonkLightArmor", "MonkHeavyArmor",
    "RangerPassive", "RangerAura", "RangerSpeed", "RangerTackShot",
    "RoguePassive", "RogueAura", "RogueExploit",
    "PaladinPassive", "PaladinAura", "PaladinHeal",
    "ShadowKnightPassive", "ShadowKnightAura", "ShadowKnightEdge",
    "WarriorPassive", "WarriorAura",
    "WizardPassive", "WizardAura", "WizardFocus",
    "MagicianPassive", "MagicianAura", "MagicianPetPassive", "MagicianOwnerFocus", "MagicianPetFury",
    "NecromancerPassive", "NecromancerAura", "NecromancerMark",
    "ClericPassive", "ClericAura", "ClericCadence", "ClericHaste",
    "DruidPassive", "DruidAura", "DruidRegrowth",
    "ShamanPassive", "ShamanAura", "ShamanSlowMark", "ShamanVigor",
    "CastSpeedHelper",
    "DruidExposure", "WarriorUnassailed", "WarriorRiposte", "BardVigor", "MonkChiSurge", "PaladinDeflection", "RogueLuckyStrike", "RogueLuckyStrikeHelper"
};

// The (passive, permanent aura) pair for each class
static const EverQuestClassAuraSpellType EQ_CLASSAURA_CLASS_PASSIVE_TYPES[] =
{
    EQ_CLASSAURA_SPELL_ENCHANTER_PASSIVE, EQ_CLASSAURA_SPELL_BARD_PASSIVE, EQ_CLASSAURA_SPELL_MONK_PASSIVE, EQ_CLASSAURA_SPELL_RANGER_PASSIVE,
    EQ_CLASSAURA_SPELL_ROGUE_PASSIVE, EQ_CLASSAURA_SPELL_PALADIN_PASSIVE, EQ_CLASSAURA_SPELL_SHADOWKNIGHT_PASSIVE, EQ_CLASSAURA_SPELL_WARRIOR_PASSIVE,
    EQ_CLASSAURA_SPELL_WIZARD_PASSIVE, EQ_CLASSAURA_SPELL_MAGICIAN_PASSIVE, EQ_CLASSAURA_SPELL_NECROMANCER_PASSIVE, EQ_CLASSAURA_SPELL_CLERIC_PASSIVE,
    EQ_CLASSAURA_SPELL_DRUID_PASSIVE, EQ_CLASSAURA_SPELL_SHAMAN_PASSIVE
};
static const EverQuestClassAuraSpellType EQ_CLASSAURA_CLASS_AURA_TYPES[] =
{
    EQ_CLASSAURA_SPELL_ENCHANTER_AURA, EQ_CLASSAURA_SPELL_BARD_AURA, EQ_CLASSAURA_SPELL_MONK_AURA, EQ_CLASSAURA_SPELL_RANGER_AURA,
    EQ_CLASSAURA_SPELL_ROGUE_AURA, EQ_CLASSAURA_SPELL_PALADIN_AURA, EQ_CLASSAURA_SPELL_SHADOWKNIGHT_AURA, EQ_CLASSAURA_SPELL_WARRIOR_AURA,
    EQ_CLASSAURA_SPELL_WIZARD_AURA, EQ_CLASSAURA_SPELL_MAGICIAN_AURA, EQ_CLASSAURA_SPELL_NECROMANCER_AURA, EQ_CLASSAURA_SPELL_CLERIC_AURA,
    EQ_CLASSAURA_SPELL_DRUID_AURA, EQ_CLASSAURA_SPELL_SHAMAN_AURA
};
static const size_t EQ_CLASSAURA_CLASS_COUNT = sizeof(EQ_CLASSAURA_CLASS_PASSIVE_TYPES) / sizeof(EQ_CLASSAURA_CLASS_PASSIVE_TYPES[0]);

// Auras the mod owns outright (put on and taken off by the mod, never by a cast), rebuilt from scratch on login
static const EverQuestClassAuraSpellType EQ_CLASSAURA_MOD_OWNED_TYPES[] =
{
    EQ_CLASSAURA_SPELL_ENCHANTER_AURA, EQ_CLASSAURA_SPELL_ENCHANTER_FOCUS, EQ_CLASSAURA_SPELL_BARD_AURA, EQ_CLASSAURA_SPELL_BARD_INSTRUMENT,
    EQ_CLASSAURA_SPELL_MONK_AURA, EQ_CLASSAURA_SPELL_MONK_LIGHT_ARMOR, EQ_CLASSAURA_SPELL_MONK_HEAVY_ARMOR, EQ_CLASSAURA_SPELL_RANGER_AURA,
    EQ_CLASSAURA_SPELL_ROGUE_AURA, EQ_CLASSAURA_SPELL_PALADIN_AURA, EQ_CLASSAURA_SPELL_SHADOWKNIGHT_AURA, EQ_CLASSAURA_SPELL_WARRIOR_AURA,
    EQ_CLASSAURA_SPELL_WIZARD_AURA, EQ_CLASSAURA_SPELL_MAGICIAN_AURA, EQ_CLASSAURA_SPELL_NECROMANCER_AURA, EQ_CLASSAURA_SPELL_CLERIC_AURA,
    EQ_CLASSAURA_SPELL_DRUID_AURA, EQ_CLASSAURA_SPELL_SHAMAN_AURA, EQ_CLASSAURA_SPELL_CAST_SPEED_HELPER, EQ_CLASSAURA_SPELL_WARRIOR_UNASSAILED,
    EQ_CLASSAURA_SPELL_MONK_CHI_SURGE, EQ_CLASSAURA_SPELL_ROGUE_LUCKY_STRIKE, EQ_CLASSAURA_SPELL_ROGUE_LUCKY_STRIKE_HELPER
};
static const size_t EQ_CLASSAURA_MOD_OWNED_COUNT = sizeof(EQ_CLASSAURA_MOD_OWNED_TYPES) / sizeof(EQ_CLASSAURA_MOD_OWNED_TYPES[0]);

void EverQuestMod::SetClassAuraSpellIDFromConfigKey(const string& spellTypeName, uint32 spellID)
{
    for (uint32 i = 0; i < EQ_CLASSAURA_SPELL_TYPE_COUNT; ++i)
    {
        if (spellTypeName != EQ_CLASSAURA_SPELL_TYPE_NAMES[i])
            continue;
        ConfigSystemClassAuraSpellIDs[i] = spellID;
        if (spellID != 0)
        {
            if (ConfigSystemClassAuraSpellIDMin == 0 || spellID < ConfigSystemClassAuraSpellIDMin)
                ConfigSystemClassAuraSpellIDMin = spellID;
            if (spellID > ConfigSystemClassAuraSpellIDMax)
                ConfigSystemClassAuraSpellIDMax = spellID;
        }
        return;
    }
    LOG_ERROR("module.EverQuest", "EverQuest: Unknown class aura spell type '{}' in mod_everquest_systemconfigs", spellTypeName);
}

bool EverQuestMod::IsClassAuraSystemEnabled()
{
    return IsEnabled == true && ConfigSpellClassAurasEnabled == true && ConfigSystemClassAuraEnabled == true;
}

uint32 EverQuestMod::GetClassAuraSpellID(EverQuestClassAuraSpellType spellType)
{
    if (spellType >= EQ_CLASSAURA_SPELL_TYPE_COUNT)
        return 0;
    return ConfigSystemClassAuraSpellIDs[spellType];
}

bool EverQuestMod::IsClassAuraSpell(uint32 spellID)
{
    if (ConfigSystemClassAuraSpellIDMin == 0)
        return false;
    return spellID >= ConfigSystemClassAuraSpellIDMin && spellID <= ConfigSystemClassAuraSpellIDMax;
}

bool EverQuestMod::PlayerHasClassAura(Player* player, EverQuestClassAuraSpellType auraSpellType)
{
    if (player == nullptr)
        return false;
    uint32 spellID = GetClassAuraSpellID(auraSpellType);
    if (spellID == 0)
        return false;
    return player->HasAura(spellID);
}

EverQuestPlayerClassAuraState* EverQuestMod::GetClassAuraStateForPlayer(Player* player)
{
    return player->CustomData.GetDefault<EverQuestPlayerClassAuraState>(EQ_PLAYER_CUSTOMDATA_CLASSAURA);
}

Unit* EverQuestMod::GetActiveClassAuraPetForPlayer(Player* player)
{
    // Only the player's own pet counts
    if (player == nullptr)
        return nullptr;
    Pet* pet = player->GetPet();
    if (pet != nullptr && pet->IsAlive() == true && pet->IsInWorld() == true)
        return pet;
    Unit* charm = player->GetCharm();
    if (charm != nullptr && charm->IsCreature() == true && charm->IsAlive() == true && charm->IsInWorld() == true && charm->HasUnitState(UNIT_STATE_POSSESSED) == false)
        return charm;
    return nullptr;
}

void EverQuestMod::ReapplyClassAurasForPlayer(Player* player)
{
    // Strip and rebuild whatever this mod actually owns (is this the best way?)
    if (player == nullptr)
        return;
    for (size_t i = 0; i < EQ_CLASSAURA_MOD_OWNED_COUNT; ++i)
    {
        uint32 spellID = GetClassAuraSpellID(EQ_CLASSAURA_MOD_OWNED_TYPES[i]);
        if (spellID != 0 && player->HasAura(spellID) == true)
            player->RemoveAurasDueToSpell(spellID);
    }
    player->SetInstantCast(false);
    if (IsClassAuraSystemEnabled() == false)
        return;
    RefreshClassAurasForPlayer(player);
}

void EverQuestMod::RefreshClassAurasForPlayer(Player* player)
{
    if (player == nullptr || IsClassAuraSystemEnabled() == false)
        return;
    for (size_t i = 0; i < EQ_CLASSAURA_CLASS_COUNT; ++i)
    {
        uint32 passiveSpellID = GetClassAuraSpellID(EQ_CLASSAURA_CLASS_PASSIVE_TYPES[i]);
        uint32 auraSpellID = GetClassAuraSpellID(EQ_CLASSAURA_CLASS_AURA_TYPES[i]);
        if (passiveSpellID == 0 || auraSpellID == 0)
            continue;
        bool shouldHaveAura = player->HasSpell(passiveSpellID);
        bool hasAura = player->HasAura(auraSpellID);
        if (shouldHaveAura == true && hasAura == false)
            player->AddAura(auraSpellID, player);
        else if (shouldHaveAura == false && hasAura == true)
            player->RemoveAurasDueToSpell(auraSpellID);
    }
    RefreshClassAuraGearAurasForPlayer(player);
    UpdateEnchanterFocusForPlayer(player);
    RefreshMagicianPetAuraForPlayer(player);
    RefreshPaladinBlockForPlayer(player);
}

void EverQuestMod::RefreshPaladinBlockForPlayer(Player* player)
{
    if (player == nullptr)
        return;
    EverQuestPlayerClassAuraState* state = GetClassAuraStateForPlayer(player);
    bool hasPaladinAura = IsClassAuraSystemEnabled() == true && PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_PALADIN_AURA) == true;
    if (hasPaladinAura == true)
    {
        // A class without the block skill (a priest holding a shield, say) is given it for as long as the aura is there
        if (player->CanBlock() == false)
        {
            player->SetCanBlock(true);
            state->BlockGrantedByClassAura = true;
        }
    }
    else if (state->BlockGrantedByClassAura == true)
    {
        // Taken back with the aura, unless the character has since learned block
        state->BlockGrantedByClassAura = false;
        if (player->HasSpell(EQ_SPELL_ID_BLOCK) == false)
            player->SetCanBlock(false);
    }
}

void EverQuestMod::RefreshClassAuraGearAurasForPlayer(Player* player)
{
    if (player == nullptr || IsClassAuraSystemEnabled() == false)
        return;
    RefreshMonkArmorAuraForPlayer(player);
    RefreshBardInstrumentAuraForPlayer(player);
}

void EverQuestMod::UpdateClassAurasForPlayer(Player* player, uint32 diffInMS)
{
    if (player == nullptr || IsClassAuraSystemEnabled() == false)
        return;
    EverQuestPlayerClassAuraState* state = GetClassAuraStateForPlayer(player);
    state->GearRefreshTimerMS += diffInMS;
    if (state->GearRefreshTimerMS >= EQ_CLASS_AURA_GEAR_REFRESH_INTERVAL_MS)
    {
        state->GearRefreshTimerMS = 0;
        RefreshClassAurasForPlayer(player);
    }
    state->ManaCheckTimerMS += diffInMS;
    if (state->ManaCheckTimerMS >= EQ_CLASS_AURA_MANA_CHECK_INTERVAL_MS)
    {
        state->ManaCheckTimerMS = 0;
        UpdateEnchanterFocusForPlayer(player);
    }
    UpdateWizardFocusMovementForPlayer(player, diffInMS);
    UpdateWarriorClassAuraForPlayer(player);
    UpdateMonkChiSurgeForPlayer(player);
    UpdateRogueLuckyStrikeForPlayer(player);
}

void EverQuestMod::UpdateRogueLuckyStrikeForPlayer(Player* player)
{
    uint32 luckyStrikeSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_ROGUE_LUCKY_STRIKE);
    if (luckyStrikeSpellID == 0)
        return;
    EverQuestPlayerClassAuraState* state = GetClassAuraStateForPlayer(player);
    bool shouldHave = PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_ROGUE_AURA) == true && GameTime::GetGameTimeMS().count() >= state->LuckyStrikeReadyAtMS;
    bool hasAura = player->HasAura(luckyStrikeSpellID);
    if (shouldHave == true && hasAura == false)
        player->AddAura(luckyStrikeSpellID, player);
    else if (shouldHave == false && hasAura == true)
        player->RemoveAurasDueToSpell(luckyStrikeSpellID);
}

void EverQuestMod::SpendClassAuraRogueLuckyStrike(Player* player)
{
    if (player == nullptr)
        return;
    uint32 luckyStrikeSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_ROGUE_LUCKY_STRIKE);
    if (luckyStrikeSpellID != 0)
        player->RemoveAurasDueToSpell(luckyStrikeSpellID);
    GetClassAuraStateForPlayer(player)->LuckyStrikeReadyAtMS = GameTime::GetGameTimeMS().count() + ConfigSystemClassAuraRogueLuckyStrikeCooldownInMS;
}

void EverQuestMod::RemoveClassAuraRogueLuckyStrikeHelper(Player* player)
{
    if (player == nullptr)
        return;
    uint32 helperSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_ROGUE_LUCKY_STRIKE_HELPER);
    if (helperSpellID != 0 && player->HasAura(helperSpellID) == true)
        player->RemoveAurasDueToSpell(helperSpellID);
}

void EverQuestMod::HandleClassAuraRogueLuckyStrikeOnCheckCast(Player* player, Spell* spell, bool strict)
{
    if (player == nullptr || spell == nullptr)
        return;
    RemoveClassAuraRogueLuckyStrikeHelper(player);
    if (spell->IsTriggered() == true || player->IsInWorld() == false || IsClassAuraSystemEnabled() == false)
        return;
    SpellInfo const* spellInfo = spell->GetSpellInfo();
    if (spellInfo == nullptr || IsClassAuraSpell(spellInfo->Id) == true || spellInfo->IsPassive() == true || spellInfo->HasAttribute(SPELL_ATTR2_AUTO_REPEAT) == true)
        return;
    uint32 luckyStrikeSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_ROGUE_LUCKY_STRIKE);
    uint32 helperSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_ROGUE_LUCKY_STRIKE_HELPER);
    if (luckyStrikeSpellID == 0 || helperSpellID == 0 || player->HasAura(luckyStrikeSpellID) == false || PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_ROGUE_AURA) == false)
        return;
    player->AddAura(helperSpellID, player);
}

void EverQuestMod::UpdateMonkChiSurgeForPlayer(Player* player)
{
    uint32 chiSurgeSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_MONK_CHI_SURGE);
    if (chiSurgeSpellID == 0)
        return;
    EverQuestPlayerClassAuraState* state = GetClassAuraStateForPlayer(player);
    bool shouldHave = PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_MONK_AURA) == true && GameTime::GetGameTimeMS().count() >= state->ChiSurgeReadyAtMS;
    bool hasAura = player->HasAura(chiSurgeSpellID);
    if (shouldHave == true && hasAura == false)
        player->AddAura(chiSurgeSpellID, player);
    else if (shouldHave == false && hasAura == true)
        player->RemoveAurasDueToSpell(chiSurgeSpellID);
}

void EverQuestMod::ClearClassAuraStateForPlayer(Player* player)
{
    if (player == nullptr)
        return;
    ClearClassAuraCastAdjustmentsForPlayer(player);
    player->CustomData.Erase(EQ_PLAYER_CUSTOMDATA_CLASSAURA);
}

void EverQuestMod::UpdateWarriorClassAuraForPlayer(Player* player)
{
    EverQuestPlayerClassAuraState* state = GetClassAuraStateForPlayer(player);
    bool hasWarriorAura = PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_WARRIOR_AURA);

    // The counter swing runs here, on the warrior's own update, rather than nested inside the attacker's swing that earned it.  It is an
    // extra attack (no swing timer reset) with the main hand, and the core's own gates (stunned, casting, line of sight) can still refuse it
    if (state->PendingRiposteTargetGUID.IsEmpty() == false)
    {
        ObjectGuid targetGUID = state->PendingRiposteTargetGUID;
        state->PendingRiposteTargetGUID.Clear();
        if (hasWarriorAura == true && player->IsAlive() == true)
        {
            Unit* target = ObjectAccessor::GetUnit(*player, targetGUID);
            if (target != nullptr && target->IsAlive() == true && player->IsValidAttackTarget(target) == true && player->IsWithinMeleeRange(target) == true)
            {
                uint32 riposteVisualSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_WARRIOR_RIPOSTE);
                if (riposteVisualSpellID != 0)
                    player->CastSpell(player, riposteVisualSpellID, true);
                player->AttackerStateUpdate(target, BASE_ATTACK, true);
            }
        }
    }

    uint32 unassailedSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_WARRIOR_UNASSAILED);
    if (unassailedSpellID == 0)
        return;
    uint32 nowMS = GameTime::GetGameTimeMS().count();
    if (state->LastMeleeAttackedMS == 0)
        state->LastMeleeAttackedMS = nowMS;
    bool shouldHave = hasWarriorAura == true && player->IsAlive() == true && (nowMS - state->LastMeleeAttackedMS) >= ConfigSystemClassAuraWarriorUnassailedDelayInMS;
    bool hasAura = player->HasAura(unassailedSpellID);
    if (shouldHave == true && hasAura == false)
        player->AddAura(unassailedSpellID, player);
    else if (shouldHave == false && hasAura == true)
        player->RemoveAurasDueToSpell(unassailedSpellID);
}

void EverQuestMod::HandleClassAuraWarriorMeleeAttackedOnRoll(Player* warrior, Unit const* attacker, int32& missChance, int32& dodgeChance, int32& parryChance, int32& blockChance, int32& critChance)
{
    if (IsClassAuraSystemEnabled() == false)
        return;
    if (warrior == nullptr || attacker == nullptr || attacker == warrior)
        return;
    if (PlayerHasClassAura(warrior, EQ_CLASSAURA_SPELL_WARRIOR_AURA) == false)
        return;
    EverQuestPlayerClassAuraState* state = GetClassAuraStateForPlayer(warrior);
    state->LastMeleeAttackedMS = GameTime::GetGameTimeMS().count();

    if (warrior->IsAlive() == false)
        return;
    if (warrior->HasUnitState(UNIT_STATE_CONTROLLED) == true || warrior->IsNonMeleeSpellCast(false, false, true) == true)
        return;
    if (roll_chance_i((int32)ConfigSystemClassAuraWarriorRiposteChancePercent) == false)
        return;

    missChance = 0;
    dodgeChance = 0;
    blockChance = 0;
    critChance = 0;
    if (warrior->HasInArc(M_PI, attacker) == true)
        parryChance = 30000; // Always wins the roll
    else
    {
        parryChance = 0;
        missChance = 30000;
    }
    state->PendingRiposteTargetGUID = attacker->GetGUID();
}

void EverQuestMod::NoteClassAuraWarriorMeleeAttacked(Unit* target, SpellInfo const* spellInfo)
{
    if (target == nullptr || spellInfo == nullptr || target->IsPlayer() == false)
        return;
    if (spellInfo->DmgClass != SPELL_DAMAGE_CLASS_MELEE)
        return;
    Player* warrior = target->ToPlayer();
    if (PlayerHasClassAura(warrior, EQ_CLASSAURA_SPELL_WARRIOR_AURA) == false)
        return;
    GetClassAuraStateForPlayer(warrior)->LastMeleeAttackedMS = GameTime::GetGameTimeMS().count();
}

void EverQuestMod::RefreshMonkArmorAuraForPlayer(Player* player)
{
    uint32 lightArmorSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_MONK_LIGHT_ARMOR);
    uint32 heavyArmorSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_MONK_HEAVY_ARMOR);
    if (lightArmorSpellID == 0 && heavyArmorSpellID == 0)
        return;

    // Mail or plate anywhere makes it heavy, and anything else (cloth, leather, nothing, a shield) is light
    uint32 desiredSpellID = 0;
    if (PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_MONK_AURA) == true)
    {
        bool isWearingHeavy = false;
        for (uint8 equipSlotIndex = EQUIPMENT_SLOT_START; equipSlotIndex < EQUIPMENT_SLOT_END; ++equipSlotIndex)
        {
            Item* equippedItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, equipSlotIndex);
            if (equippedItem == nullptr)
                continue;
            ItemTemplate const* itemTemplate = equippedItem->GetTemplate();
            if (itemTemplate == nullptr || itemTemplate->Class != ITEM_CLASS_ARMOR)
                continue;
            if (itemTemplate->SubClass == ITEM_SUBCLASS_ARMOR_MAIL || itemTemplate->SubClass == ITEM_SUBCLASS_ARMOR_PLATE)
            {
                isWearingHeavy = true;
                break;
            }
        }
        desiredSpellID = isWearingHeavy == true ? heavyArmorSpellID : lightArmorSpellID;
    }

    if (lightArmorSpellID != 0 && desiredSpellID != lightArmorSpellID && player->HasAura(lightArmorSpellID) == true)
        player->RemoveAurasDueToSpell(lightArmorSpellID);
    if (heavyArmorSpellID != 0 && desiredSpellID != heavyArmorSpellID && player->HasAura(heavyArmorSpellID) == true)
        player->RemoveAurasDueToSpell(heavyArmorSpellID);
    if (desiredSpellID != 0 && player->HasAura(desiredSpellID) == false)
        player->AddAura(desiredSpellID, player);
}

void EverQuestMod::RefreshBardInstrumentAuraForPlayer(Player* player)
{
    uint32 instrumentSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_BARD_INSTRUMENT);
    if (instrumentSpellID == 0)
        return;

    bool shouldHave = false;
    if (PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_BARD_AURA) == true)
    {
        bool isHoldingWeapon = false;
        bool isHoldingInstrument = false;
        uint8 heldSlots[3] = { EQUIPMENT_SLOT_MAINHAND, EQUIPMENT_SLOT_OFFHAND, EQUIPMENT_SLOT_RANGED };
        for (uint8 heldSlot : heldSlots)
        {
            Item* heldItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, heldSlot);
            if (heldItem == nullptr)
                continue;
            ItemTemplate const* itemTemplate = heldItem->GetTemplate();
            if (itemTemplate == nullptr)
                continue;
            if (itemTemplate->Class == ITEM_CLASS_WEAPON)
            {
                if (heldSlot != EQUIPMENT_SLOT_RANGED && itemTemplate->SubClass != ITEM_SUBCLASS_WEAPON_MISC && itemTemplate->SubClass != ITEM_SUBCLASS_WEAPON_FISHING_POLE)
                    isHoldingWeapon = true;
            }
            else if (itemTemplate->TotemCategory != 0)
                isHoldingInstrument = true;
        }
        shouldHave = isHoldingWeapon == true && isHoldingInstrument == true;
    }

    bool hasAura = player->HasAura(instrumentSpellID);
    if (shouldHave == true && hasAura == false)
        player->AddAura(instrumentSpellID, player);
    else if (shouldHave == false && hasAura == true)
        player->RemoveAurasDueToSpell(instrumentSpellID);
}

void EverQuestMod::UpdateEnchanterFocusForPlayer(Player* player)
{
    uint32 focusSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_ENCHANTER_FOCUS);
    if (focusSpellID == 0)
        return;
    bool shouldHave = false;
    if (PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_ENCHANTER_AURA) == true)
    {
        uint64 maxMana = player->GetMaxPower(POWER_MANA);
        if (maxMana > 0)
            shouldHave = (uint64)player->GetPower(POWER_MANA) * 100 >= maxMana * (uint64)ConfigSystemClassAuraEnchanterFocusManaThresholdPercent;
    }
    bool hasAura = player->HasAura(focusSpellID);
    if (shouldHave == true && hasAura == false)
        player->AddAura(focusSpellID, player);
    else if (shouldHave == false && hasAura == true)
        player->RemoveAurasDueToSpell(focusSpellID);
}

void EverQuestMod::UpdateWizardFocusMovementForPlayer(Player* player, uint32 diffInMS)
{
    uint32 focusSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_WIZARD_FOCUS);
    if (focusSpellID == 0)
        return;

    // Deliberate movement only, so a jump or a fall in place does not count
    EverQuestPlayerClassAuraState* state = GetClassAuraStateForPlayer(player);
    bool isMoving = player->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING);
    if (isMoving == false)
    {
        state->WasMoving = false;
        state->MovingAccumulatedMS = 0;
        return;
    }
    int32 stacksLostPerEvent = (int32)ConfigSystemClassAuraWizardFocusStacksLostPerMovementEvent;
    uint32 intervalInMS = ConfigSystemClassAuraWizardFocusMovementIntervalInMS < 100 ? 100 : ConfigSystemClassAuraWizardFocusMovementIntervalInMS;
    bool movementJustStarted = state->WasMoving == false;
    state->WasMoving = true;
    if (movementJustStarted == true)
        state->MovingAccumulatedMS = 0;
    state->MovingAccumulatedMS += diffInMS;
    int32 eventsToApply = movementJustStarted == true ? 1 : 0;
    while (state->MovingAccumulatedMS >= intervalInMS)
    {
        state->MovingAccumulatedMS -= intervalInMS;
        eventsToApply++;
    }
    if (eventsToApply == 0 || stacksLostPerEvent <= 0)
        return;
    Aura* focusAura = player->GetAura(focusSpellID);
    if (focusAura == nullptr)
        return;
    for (int32 i = 0; i < eventsToApply; ++i)
    {
        if (focusAura->ModStackAmount(-stacksLostPerEvent) == true)
            break;
    }
}

void EverQuestMod::RefreshMagicianPetAuraForPlayer(Player* player)
{
    uint32 petPassiveSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_MAGICIAN_PET_PASSIVE);
    if (petPassiveSpellID == 0)
        return;
    if (PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_MAGICIAN_AURA) == false)
        return;
    Pet* pet = player->GetPet();
    if (pet == nullptr || pet->IsInWorld() == false || pet->IsAlive() == false)
        return;
    if (pet->HasAura(petPassiveSpellID) == false)
        pet->AddAura(petPassiveSpellID, pet);
}

void EverQuestMod::ApplyMagicianPetAuraToPet(Pet* pet)
{
    if (pet == nullptr || IsClassAuraSystemEnabled() == false)
        return;
    uint32 petPassiveSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_MAGICIAN_PET_PASSIVE);
    if (petPassiveSpellID == 0)
        return;
    Player* owner = pet->GetOwner();
    if (owner == nullptr)
        return;
    if (PlayerHasClassAura(owner, EQ_CLASSAURA_SPELL_MAGICIAN_AURA) == false)
        return;
    if (pet->HasAura(petPassiveSpellID) == false)
        pet->AddAura(petPassiveSpellID, pet);
}

void EverQuestMod::HandleClassAuraPetStrike(Unit* attacker, Unit* victim)
{
    if (IsClassAuraSystemEnabled() == false)
        return;
    if (attacker == nullptr || victim == nullptr || attacker->IsCreature() == false)
        return;
    Player* owner = attacker->GetCharmerOrOwnerPlayerOrPlayerItself();
    if (owner == nullptr || owner == attacker)
        return;
    if (owner->FindMap() == nullptr || owner->FindMap() != attacker->FindMap())
        return;

    uint32 markSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_NECROMANCER_MARK);
    if (markSpellID != 0 && victim->IsAlive() == true && PlayerHasClassAura(owner, EQ_CLASSAURA_SPELL_NECROMANCER_AURA) == true && owner->IsValidAttackTarget(victim) == true)
        owner->CastSpell(victim, markSpellID, true);

    // Druid: the pet's strikes expose the target the same way the druid's own autoattacks do
    uint32 exposureSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_DRUID_EXPOSURE);
    if (exposureSpellID != 0 && victim->IsAlive() == true && PlayerHasClassAura(owner, EQ_CLASSAURA_SPELL_DRUID_AURA) == true && owner->IsValidAttackTarget(victim) == true)
        owner->CastSpell(victim, exposureSpellID, true);
}

static bool IsPeriodicDamageAura(Aura* aura)
{
    return aura->HasEffectType(SPELL_AURA_PERIODIC_DAMAGE) == true || aura->HasEffectType(SPELL_AURA_PERIODIC_LEECH) == true || aura->HasEffectType(SPELL_AURA_PERIODIC_DAMAGE_PERCENT) == true;
}

void EverQuestMod::HandleClassAuraShamanStrike(Unit* attacker, Unit* victim)
{
    if (IsClassAuraSystemEnabled() == false)
        return;
    if (attacker == nullptr || victim == nullptr || attacker->IsPlayer() == false)
        return;
    Player* shaman = attacker->ToPlayer();
    if (PlayerHasClassAura(shaman, EQ_CLASSAURA_SPELL_SHAMAN_AURA) == false)
        return;
    if (roll_chance_i((int32)ConfigSystemClassAuraShamanDotExtendChancePercent) == false)
        return;
    int32 extendInMS = (int32)ConfigSystemClassAuraShamanDotExtendInMS;
    if (extendInMS <= 0)
        return;
    ObjectGuid shamanGUID = shaman->GetGUID();
    Unit::AuraApplicationMap const& victimAuras = victim->GetAppliedAuras();
    for (Unit::AuraApplicationMap::const_iterator auraIter = victimAuras.begin(); auraIter != victimAuras.end(); ++auraIter)
    {
        Aura* aura = auraIter->second->GetBase();
        if (aura == nullptr || aura->GetCasterGUID() != shamanGUID || aura->IsPermanent() == true)
            continue;
        if (IsPeriodicDamageAura(aura) == false)
            continue;
        int32 newDurationInMS = aura->GetDuration() + extendInMS;
        if (newDurationInMS > aura->GetMaxDuration())
            aura->SetMaxDuration(newDurationInMS);
        aura->SetDuration(newDurationInMS);
        aura->SetNeedClientUpdateForTargets();
    }
}

void EverQuestMod::ApplyClassAuraMeleeDamageMods(Unit* attacker, Unit* victim, uint32& damage)
{
    if (IsClassAuraSystemEnabled() == false)
        return;
    if (attacker == nullptr || victim == nullptr || damage == 0)
        return;

    // Ranger
    int32 tackShotDamage = (int32)damage;
    ApplyClassAuraTackShotDamageBonus(attacker, victim, tackShotDamage);
    damage = (uint32)tackShotDamage;

    if (attacker->IsPlayer() == false)
        return;
    Player* player = attacker->ToPlayer();

    // Bard
    uint32 instrumentSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_BARD_INSTRUMENT);
    if (instrumentSpellID != 0 && player->HasAura(instrumentSpellID) == true && PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_BARD_AURA) == true)
        damage += (damage * ConfigSystemClassAuraBardInstrumentMeleeAutoAttackDamagePercent) / 100;

    // Paladin
    int32 paladinDamage = (int32)damage;
    ApplyClassAuraPaladinUndeadDemonDamageBonus(player, victim, paladinDamage);
    damage = (uint32)paladinDamage;
}

void EverQuestMod::ApplyClassAuraPaladinUndeadDemonDamageBonus(Unit* attacker, Unit* victim, int32& damage)
{
    if (attacker == nullptr || victim == nullptr || damage <= 0)
        return;
    if (attacker->IsPlayer() == false || victim->IsCreature() == false)
        return;
    uint32 creatureType = victim->GetCreatureType();
    if (creatureType != CREATURE_TYPE_UNDEAD && creatureType != CREATURE_TYPE_DEMON)
        return;
    if (PlayerHasClassAura(attacker->ToPlayer(), EQ_CLASSAURA_SPELL_PALADIN_AURA) == false)
        return;
    if (roll_chance_i((int32)ConfigSystemClassAuraPaladinUndeadDemonDoubleDamageChancePercent) == false)
        return;
    damage *= 2;
}

static bool HasPeriodicDamageFromCaster(Unit* target, ObjectGuid casterGUID)
{
    AuraType periodicTypes[3] = { SPELL_AURA_PERIODIC_DAMAGE, SPELL_AURA_PERIODIC_LEECH, SPELL_AURA_PERIODIC_DAMAGE_PERCENT };
    for (AuraType periodicType : periodicTypes)
    {
        Unit::AuraEffectList const& effects = target->GetAuraEffectsByType(periodicType);
        for (AuraEffect* effect : effects)
            if (effect != nullptr && effect->GetCasterGUID() == casterGUID)
                return true;
    }
    return false;
}

void EverQuestMod::ApplyClassAuraTackShotDamageBonus(Unit* attacker, Unit* victim, int32& damage)
{
    if (attacker == nullptr || victim == nullptr || damage <= 0)
        return;
    uint32 tackShotSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_RANGER_TACK_SHOT);
    if (tackShotSpellID == 0)
        return;
    if (victim->HasAura(tackShotSpellID) == false)
        return;
    Player* ranger = attacker->GetCharmerOrOwnerPlayerOrPlayerItself();
    if (ranger == nullptr)
        return;
    if (ranger != attacker && (ranger->FindMap() == nullptr || ranger->FindMap() != attacker->FindMap()))
        return;
    if (PlayerHasClassAura(ranger, EQ_CLASSAURA_SPELL_RANGER_AURA) == false)
        return;
    Aura* mark = victim->GetAura(tackShotSpellID, ranger->GetGUID());
    if (mark == nullptr)
        return;
    int32 bonusPercent = (int32)ConfigSystemClassAuraRangerTackShotDamagePercentPerStack * (int32)mark->GetStackAmount();
    if (victim->isMoving() == true)
        bonusPercent *= 2;
    if (bonusPercent <= 0)
        return;
    damage += (damage * bonusPercent) / 100;
}

void EverQuestMod::ApplyClassAuraDirectSpellDamageMods(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo)
{
    if (IsClassAuraSystemEnabled() == false)
        return;
    if (target == nullptr || attacker == nullptr || spellInfo == nullptr || damage <= 0)
        return;

    // Warrior: a melee ability hitting them counts as being attacked
    NoteClassAuraWarriorMeleeAttacked(target, spellInfo);

    // Rangers and their pets alike
    ApplyClassAuraTackShotDamageBonus(attacker, target, damage);

    if (attacker->IsPlayer() == false)
        return;
    Player* player = attacker->ToPlayer();

    // Paladin
    ApplyClassAuraPaladinUndeadDemonDamageBonus(player, target, damage);

    uint32 markSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_NECROMANCER_MARK);
    if (markSpellID != 0 && PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_NECROMANCER_AURA) == true)
    {
        Aura* mark = target->GetAura(markSpellID);
        if (mark != nullptr)
            damage += (damage * (int32)ConfigSystemClassAuraNecromancerMarkDirectDamagePercentPerStack * (int32)mark->GetStackAmount()) / 100;
    }

    if (PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_DRUID_AURA) == true)
    {
        bool isImpaired = target->HasAuraType(SPELL_AURA_MOD_ROOT) == true || target->HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED) == true
            || target->HasAuraWithMechanic((1ULL << MECHANIC_SNARE) | (1ULL << MECHANIC_ROOT)) == true || HasPeriodicDamageFromCaster(target, player->GetGUID()) == true;
        if (isImpaired == true)
            damage += (damage * (int32)ConfigSystemClassAuraDruidImpairedTargetDamagePercent) / 100;
    }
}

void EverQuestMod::ApplyClassAuraPeriodicTickMods(Unit* target, Unit* attacker, uint32& amount, SpellInfo const* spellInfo)
{
    if (IsClassAuraSystemEnabled() == false)
        return;
    if (attacker == nullptr || spellInfo == nullptr || amount == 0)
        return;

    // This hook also fires for heal ticks, which the bonuses below must never touch
    bool isHealTick = spellInfo->HasAura(SPELL_AURA_PERIODIC_HEAL) == true && spellInfo->HasAura(SPELL_AURA_PERIODIC_DAMAGE) == false
        && spellInfo->HasAura(SPELL_AURA_PERIODIC_LEECH) == false && spellInfo->HasAura(SPELL_AURA_PERIODIC_DAMAGE_PERCENT) == false;
    if (isHealTick == true)
        return;

    // Ranger
    int32 tackShotAmount = (int32)amount;
    ApplyClassAuraTackShotDamageBonus(attacker, target, tackShotAmount);
    amount = (uint32)tackShotAmount;

    if (attacker->IsPlayer() == false)
        return;
    Player* player = attacker->ToPlayer();

    // Paladin
    int32 paladinAmount = (int32)amount;
    ApplyClassAuraPaladinUndeadDemonDamageBonus(player, target, paladinAmount);
    amount = (uint32)paladinAmount;

    uint32 markSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_NECROMANCER_MARK);
    if (markSpellID != 0 && target != nullptr && PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_NECROMANCER_AURA) == true)
    {
        Aura* mark = target->GetAura(markSpellID);
        if (mark != nullptr)
            amount += (amount * ConfigSystemClassAuraNecromancerMarkDotDamagePercentPerStack * (uint32)mark->GetStackAmount()) / 100;
    }
}

bool EverQuestMod::TryTransferDebuffToNecromancerPet(Player* player, Aura* aura)
{
    if (IsClassAuraSystemEnabled() == false)
        return false;
    if (player == nullptr || aura == nullptr)
        return false;
    if (PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_NECROMANCER_AURA) == false)
        return false;
    SpellInfo const* spellInfo = aura->GetSpellInfo();
    if (spellInfo == nullptr || spellInfo->IsPositive() == true || spellInfo->IsPassive() == true || spellInfo->HasAreaAuraEffect() == true)
        return false;
    if (aura->IsPermanent() == true || IsClassAuraSpell(aura->GetId()) == true)
        return false;

    // Control effects that would make no sense on a pet, and anything the necromancer put on themself
    if (spellInfo->HasAura(SPELL_AURA_MOD_CHARM) == true || spellInfo->HasAura(SPELL_AURA_MOD_POSSESS) == true || spellInfo->HasAura(SPELL_AURA_AOE_CHARM) == true
        || spellInfo->HasAura(SPELL_AURA_MOD_POSSESS_PET) == true)
        return false;
    if (aura->GetCasterGUID().IsEmpty() == true || aura->GetCasterGUID() == player->GetGUID())
        return false;
    Unit* caster = aura->GetCaster();
    if (caster == nullptr || caster->IsFriendlyTo(player) == true)
        return false;
    EverQuestPlayerClassAuraState* state = GetClassAuraStateForPlayer(player);
    uint32 nowMS = GameTime::GetGameTimeMS().count();
    if (nowMS < state->NextDebuffTransferAllowedMS)
        return false;
    Unit* pet = GetActiveClassAuraPetForPlayer(player);
    if (pet == nullptr || pet->FindMap() != player->FindMap())
        return false;

    // A pet that is immune gets no copy, and then the necromancer keeps the effect and the cooldown is not spent
    Aura* petAura = caster->AddAura(spellInfo, MAX_EFFECT_MASK, pet);
    if (petAura == nullptr)
        return false;
    petAura->SetMaxDuration(aura->GetMaxDuration());
    petAura->SetDuration(aura->GetDuration());
    if (aura->GetStackAmount() > 1)
        petAura->SetStackAmount(aura->GetStackAmount());
    petAura->SetNeedClientUpdateForTargets();
    state->NextDebuffTransferAllowedMS = nowMS + ConfigSystemClassAuraNecromancerDebuffTransferCooldownInMS;
    player->RemoveAura(aura);
    return true;
}

static bool IsSlowAura(Aura* aura)
{
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        AuraEffect* effect = aura->GetEffect(i);
        if (effect == nullptr)
            continue;
        AuraType auraType = effect->GetAuraType();
        if (auraType != SPELL_AURA_MOD_MELEE_HASTE && auraType != SPELL_AURA_MOD_MELEE_RANGED_HASTE && auraType != SPELL_AURA_MELEE_SLOW)
            continue;
        if (effect->GetAmount() < 0)
            return true;
    }
    return false;
}

void EverQuestMod::HandleClassAuraSlowAuraApply(Unit* target, Aura* aura)
{
    if (IsClassAuraSystemEnabled() == false)
        return;
    if (target == nullptr || aura == nullptr)
        return;
    uint32 slowMarkSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_SHAMAN_SLOW_MARK);
    if (slowMarkSpellID == 0 || aura->GetId() == slowMarkSpellID)
        return;
    if (IsSlowAura(aura) == false)
        return;
    Unit* caster = aura->GetCaster();
    if (caster == nullptr || caster == target || caster->IsPlayer() == false)
        return;
    Player* shaman = caster->ToPlayer();
    if (PlayerHasClassAura(shaman, EQ_CLASSAURA_SPELL_SHAMAN_AURA) == false)
        return;
    if (target->IsAlive() == false || shaman->IsValidAttackTarget(target) == false)
        return;

    // One shared mark per target (single aura stack), so a second shaman's cast refreshes the first one's copy
    int32 existingRemainingInMS = 0;
    if (Aura* existingMark = target->GetAura(slowMarkSpellID))
        existingRemainingInMS = existingMark->GetDuration();
    shaman->CastSpell(target, slowMarkSpellID, true);
    Aura* mark = target->GetAura(slowMarkSpellID);
    if (mark == nullptr || aura->IsPermanent() == true)
        return;
    if (aura->GetMaxDuration() > 0 && aura->GetDuration() > 0)
    {
        int32 durationInMS = std::max(aura->GetDuration(), existingRemainingInMS);
        if (mark->GetMaxDuration() < durationInMS)
            mark->SetMaxDuration(durationInMS);
        mark->SetDuration(durationInMS);
        mark->SetNeedClientUpdateForTargets();
    }
}

void EverQuestMod::HandleClassAuraSlowAuraRemove(Unit* target, Aura* aura)
{
    if (IsClassAuraSystemEnabled() == false)
        return;
    if (target == nullptr || aura == nullptr)
        return;
    uint32 slowMarkSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_SHAMAN_SLOW_MARK);
    if (slowMarkSpellID == 0 || aura->GetId() == slowMarkSpellID)
        return;
    if (IsSlowAura(aura) == false)
        return;
    if (target->HasAura(slowMarkSpellID) == false)
        return;

    // Another qualifying slow still on the target keeps the mark
    Unit::AuraApplicationMap const& targetAuras = target->GetAppliedAuras();
    for (Unit::AuraApplicationMap::const_iterator auraIter = targetAuras.begin(); auraIter != targetAuras.end(); ++auraIter)
    {
        Aura* otherAura = auraIter->second->GetBase();
        if (otherAura == nullptr || otherAura == aura || otherAura->IsRemoved() == true)
            continue;
        if (IsSlowAura(otherAura) == false)
            continue;
        Unit* otherCaster = otherAura->GetCaster();
        if (otherCaster == nullptr || otherCaster->IsPlayer() == false)
            continue;
        if (PlayerHasClassAura(otherCaster->ToPlayer(), EQ_CLASSAURA_SPELL_SHAMAN_AURA) == true)
            return;
    }
    target->RemoveAurasDueToSpell(slowMarkSpellID);
}

static SpellInfo const* FindClassAuraDirectHealSpellInfo(SpellInfo const* spellInfo, uint8 depth)
{
    if (spellInfo == nullptr)
        return nullptr;
    if (spellInfo->HasEffect(SPELL_EFFECT_HEAL) == true)
        return spellInfo;
    if (depth >= 3)
        return nullptr;
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        uint32 effectType = spellInfo->Effects[i].Effect;
        if (effectType != SPELL_EFFECT_TRIGGER_SPELL && effectType != SPELL_EFFECT_TRIGGER_SPELL_WITH_VALUE && effectType != SPELL_EFFECT_TRIGGER_MISSILE)
            continue;
        uint32 triggerSpellID = spellInfo->Effects[i].TriggerSpell;
        if (triggerSpellID == 0 || triggerSpellID == spellInfo->Id)
            continue;
        SpellInfo const* healSpellInfo = FindClassAuraDirectHealSpellInfo(sSpellMgr->GetSpellInfo(triggerSpellID), depth + 1);
        if (healSpellInfo != nullptr)
            return healSpellInfo;
    }
    int32 linkKeys[2] = { (int32)spellInfo->Id + SPELL_LINK_CAST, (int32)spellInfo->Id + SPELL_LINK_HIT };
    for (uint8 k = 0; k < 2; ++k)
    {
        std::vector<int32> const* linkedSpellIDs = sSpellMgr->GetSpellLinked(linkKeys[k]);
        if (linkedSpellIDs == nullptr)
            continue;
        for (size_t j = 0; j < linkedSpellIDs->size(); ++j)
        {
            // A negative entry removes an aura rather than casting anything
            int32 linkedSpellID = (*linkedSpellIDs)[j];
            if (linkedSpellID <= 0 || (uint32)linkedSpellID == spellInfo->Id)
                continue;
            SpellInfo const* healSpellInfo = FindClassAuraDirectHealSpellInfo(sSpellMgr->GetSpellInfo((uint32)linkedSpellID), depth + 1);
            if (healSpellInfo != nullptr)
                return healSpellInfo;
        }
    }
    return nullptr;
}

void EverQuestMod::ApplyClassAuraCastAdjustmentsOnCheckCast(Player* player, Spell* spell, bool strict)
{
    if (player == nullptr || spell == nullptr)
        return;
    HandleClassAuraRogueLuckyStrikeOnCheckCast(player, spell, strict);
    if (strict == false)
        return;
    if (spell->IsTriggered() == true || player->IsInWorld() == false)
        return;
    SpellInfo const* spellInfo = spell->GetSpellInfo();
    if (spellInfo == nullptr)
        return;

    // Every non-triggered cast check starts from a clean slate, so a check that failed after this point can never leave an adjustment behind
    player->SetInstantCast(false);
    uint32 helperSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_CAST_SPEED_HELPER);
    if (helperSpellID != 0 && player->HasAura(helperSpellID) == true)
        player->RemoveAurasDueToSpell(helperSpellID);
    if (IsClassAuraSystemEnabled() == false)
        return;
    EverQuestPlayerClassAuraState* state = GetClassAuraStateForPlayer(player);
    state->PendingCastAdjustSpellID = 0;
    state->PendingCadenceConsume = false;
    state->PendingEdgeConsume = false;
    state->PendingChiSurgeConsume = false;
    if (IsClassAuraSpell(spellInfo->Id) == true)
        return;

    uint32 baseCastTimeInMS = spellInfo->CalcCastTime();
    Unit* target = spell->m_targets.GetUnitTarget();
    SpellInfo const* healSpellInfo = FindClassAuraDirectHealSpellInfo(spellInfo, 0);
    bool isDirectHeal = healSpellInfo != nullptr;
    bool isSingleTarget = spellInfo->IsAffectingArea() == false && (healSpellInfo == nullptr || healSpellInfo->IsAffectingArea() == false);
    float castTimeMultiplier = 1.0f;
    uint32 castTimeReductionPercent = 0;
    bool isBardSong = IsSpellAnEQBardSong(spellInfo->Id);

    // Bard, a song's cast time never changes
    if (isBardSong == true && baseCastTimeInMS > 0)
    {
        uint32 currentCastTimeInMS = spellInfo->CalcCastTime(player, spell);
        if (currentCastTimeInMS > 0 && currentCastTimeInMS != baseCastTimeInMS)
            castTimeMultiplier = (float)baseCastTimeInMS / (float)currentCastTimeInMS;
    }

    // Monk, a ready surge speeds up one non-channeled spell whose base cast time is under the limit, and is spent when that cast succeeds
    uint32 chiSurgeSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_MONK_CHI_SURGE);
    if (isBardSong == false && chiSurgeSpellID != 0 && baseCastTimeInMS > 0 && baseCastTimeInMS < ConfigSystemClassAuraMonkChiSurgeMaxBaseCastTimeInMS
        && spellInfo->IsChanneled() == false && ConfigSystemClassAuraMonkChiSurgeCastTimeReductionPercent > 0 && ConfigSystemClassAuraMonkChiSurgeCastTimeReductionPercent < 100
        && player->HasAura(chiSurgeSpellID) == true && PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_MONK_AURA) == true)
    {
        castTimeReductionPercent += ConfigSystemClassAuraMonkChiSurgeCastTimeReductionPercent;
        state->PendingChiSurgeConsume = true;
        state->PendingCastAdjustSpellID = spellInfo->Id;
    }

    // Cleric
    uint32 cadenceSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_CLERIC_CADENCE);
    bool isCompleteHeal = ConfigSystemClassAuraPrivateSpellFamilyID != 0 && spellInfo->SpellFamilyName == ConfigSystemClassAuraPrivateSpellFamilyID;
    if (isBardSong == false && cadenceSpellID != 0 && isDirectHeal == true && isSingleTarget == true && target != nullptr && isCompleteHeal == false
        && ConfigSystemClassAuraClericCadenceReductionPercent > 0 && ConfigSystemClassAuraClericCadenceReductionPercent < 100
        && player->HasAura(cadenceSpellID) == true && PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_CLERIC_AURA) == true)
    {
        // Only the cast time is adjusted through the helper aura
        if (baseCastTimeInMS > 0)
            castTimeReductionPercent += ConfigSystemClassAuraClericCadenceReductionPercent;
        state->PendingCadenceConsume = true;
        state->PendingCastAdjustSpellID = spellInfo->Id;
    }

    // Shadow Knight
    uint32 edgeSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_SHADOWKNIGHT_EDGE);
    if (isBardSong == false && edgeSpellID != 0 && baseCastTimeInMS > 0 && spellInfo->IsPositive() == false && spellInfo->DmgClass == SPELL_DAMAGE_CLASS_MAGIC
        && player->HasAura(edgeSpellID) == true && PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_SHADOWKNIGHT_AURA) == true)
    {
        player->SetInstantCast(true);
        state->PendingEdgeConsume = true;
        state->PendingCastAdjustSpellID = spellInfo->Id;
    }

    // A surge and a focus charge spent on the same cast add their cuts together (50% and 20% make the cast 70% faster) rather than compounding
    if (castTimeReductionPercent >= 100)
        player->SetInstantCast(true);
    else if (castTimeReductionPercent > 0)
        castTimeMultiplier *= (100.0f - (float)castTimeReductionPercent) / 100.0f;

    if (helperSpellID == 0)
        return;
    if (castTimeMultiplier <= 0.0f || std::fabs(castTimeMultiplier - 1.0f) < 0.001f)
        return;
    SpellInfo const* helperSpellInfo = sSpellMgr->GetSpellInfo(helperSpellID);
    if (helperSpellInfo == nullptr)
        return;

    // The helper's amounts are handed in at creation, found by effect type so the converter's effect order is not assumed.  Any cost modifier effect the helper carries is left at zero,
    // since the cast being prepared has already been priced
    int32 baseAmounts[MAX_SPELL_EFFECTS] = { 0, 0, 0 };
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (helperSpellInfo->Effects[i].ApplyAuraName == SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK)
        {
            // A positive amount divides the cast speed and a negative one multiplies it (Unit::ApplyCastTimePercentMod), so each direction has its own formula
            if (castTimeMultiplier < 1.0f)
                baseAmounts[i] = (int32)std::lround(100.0f * ((1.0f / castTimeMultiplier) - 1.0f));
            else
                baseAmounts[i] = -(int32)std::lround(100.0f * (castTimeMultiplier - 1.0f));
        }
    }
    Aura* helperAura = Aura::TryRefreshStackOrCreate(helperSpellInfo, MAX_EFFECT_MASK, player, player, baseAmounts);
    if (helperAura != nullptr)
        helperAura->ApplyForTargets();
}

void EverQuestMod::FinishClassAuraCastAdjustmentsOnPrepare(Player* player, Spell* spell)
{
    if (player == nullptr || spell == nullptr)
        return;
    if (spell->IsTriggered() == true)
        return;
    player->SetInstantCast(false);
    uint32 helperSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_CAST_SPEED_HELPER);
    if (helperSpellID == 0)
        return;
    Aura* helperAura = player->GetAura(helperSpellID);
    if (helperAura == nullptr)
        return;
    EverQuestPlayerClassAuraState* state = GetClassAuraStateForPlayer(player);
    if (state->PendingCadenceConsume == true && state->PendingCastAdjustSpellID == spell->GetSpellInfo()->Id)
    {
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            AuraEffect* effect = helperAura->GetEffect(i);
            if (effect != nullptr && effect->GetAuraType() == SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK && effect->GetAmount() != 0)
                effect->ChangeAmount(0);
        }
        return;
    }
    player->RemoveAurasDueToSpell(helperSpellID);
}

void EverQuestMod::HandleClassAuraSpellCastCancel(Player* player, Spell* spell)
{
    if (player == nullptr || spell == nullptr)
        return;
    if (spell->IsTriggered() == true)
        return;
    player->SetInstantCast(false);
    EverQuestPlayerClassAuraState* state = player->CustomData.Get<EverQuestPlayerClassAuraState>(EQ_PLAYER_CUSTOMDATA_CLASSAURA);
    if (state != nullptr && state->PendingCastAdjustSpellID != 0 && spell->GetSpellInfo() != nullptr && state->PendingCastAdjustSpellID != spell->GetSpellInfo()->Id)
        return;
    ClearClassAuraCastAdjustmentsForPlayer(player);
}

void EverQuestMod::ClearClassAuraCastAdjustmentsForPlayer(Player* player)
{
    if (player == nullptr)
        return;
    player->SetInstantCast(false);
    RemoveClassAuraRogueLuckyStrikeHelper(player);
    uint32 helperSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_CAST_SPEED_HELPER);
    if (helperSpellID != 0 && player->HasAura(helperSpellID) == true)
        player->RemoveAurasDueToSpell(helperSpellID);
    EverQuestPlayerClassAuraState* state = player->CustomData.Get<EverQuestPlayerClassAuraState>(EQ_PLAYER_CUSTOMDATA_CLASSAURA);
    if (state != nullptr)
    {
        state->PendingCastAdjustSpellID = 0;
        state->PendingCadenceConsume = false;
        state->PendingEdgeConsume = false;
        state->PendingChiSurgeConsume = false;
    }
}

static bool IsSpellCountedForWizardFocus(SpellInfo const* spellInfo, Spell* spell)
{
    if (spell->m_CastItem != nullptr)
        return false;
    if (spellInfo->IsPassive() == true || spellInfo->HasAttribute(SPELL_ATTR2_AUTO_REPEAT) == true)
        return false;
    if (spellInfo->DmgClass == SPELL_DAMAGE_CLASS_MELEE || spellInfo->DmgClass == SPELL_DAMAGE_CLASS_RANGED)
        return false;
    if (spellInfo->CalcCastTime() > 0)
        return true;
    return spellInfo->PowerType == POWER_MANA && (spellInfo->ManaCost > 0 || spellInfo->ManaCostPercentage > 0);
}

void EverQuestMod::HandleClassAuraSpellCast(Player* player, Spell* spell)
{
    if (player == nullptr || spell == nullptr)
        return;
    // The lucky strike's crit share ends with the cast it was put on for (its criticals were rolled before this hook), whatever cast reaches here first
    RemoveClassAuraRogueLuckyStrikeHelper(player);
    if (spell->IsTriggered() == true)
        return;
    SpellInfo const* spellInfo = spell->GetSpellInfo();
    if (spellInfo == nullptr || IsClassAuraSystemEnabled() == false)
        return;
    if (IsClassAuraSpell(spellInfo->Id) == true)
        return;

    EverQuestPlayerClassAuraState* state = GetClassAuraStateForPlayer(player);
    if (state->PendingCastAdjustSpellID == spellInfo->Id)
    {
        if (state->PendingCadenceConsume == true)
        {
            uint32 cadenceSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_CLERIC_CADENCE);
            Aura* cadenceAura = cadenceSpellID != 0 ? player->GetAura(cadenceSpellID) : nullptr;
            if (cadenceAura != nullptr)
            {
                cadenceAura->ModStackAmount(-1);

                // The mana share of the cadence.  Spell::cast has already charged the full price (TakePower runs ahead of this hook), and the price itself was fixed before the check cast
                // could discount it, so the discount is given back here
                if (spell->m_CastItem == nullptr && spellInfo->PowerType == POWER_MANA && spell->GetPowerCost() > 0)
                {
                    int32 cadenceRefund = (spell->GetPowerCost() * (int32)ConfigSystemClassAuraClericCadenceReductionPercent) / 100;
                    if (cadenceRefund > 0)
                        player->ModifyPower(POWER_MANA, cadenceRefund);
                }
            }
        }
        if (state->PendingEdgeConsume == true)
        {
            uint32 edgeSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_SHADOWKNIGHT_EDGE);
            if (edgeSpellID != 0)
                player->RemoveAurasDueToSpell(edgeSpellID);
        }
        if (state->PendingChiSurgeConsume == true)
        {
            uint32 chiSurgeSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_MONK_CHI_SURGE);
            if (chiSurgeSpellID != 0)
                player->RemoveAurasDueToSpell(chiSurgeSpellID);
            state->ChiSurgeReadyAtMS = GameTime::GetGameTimeMS().count() + ConfigSystemClassAuraMonkChiSurgeReturnInMS;
        }
        state->PendingCastAdjustSpellID = 0;
        state->PendingCadenceConsume = false;
        state->PendingEdgeConsume = false;
        state->PendingChiSurgeConsume = false;
        player->SetInstantCast(false);
    }

    uint32 helperSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_CAST_SPEED_HELPER);
    if (helperSpellID != 0 && state->PendingCastAdjustSpellID == 0 && player->HasAura(helperSpellID) == true)
        player->RemoveAurasDueToSpell(helperSpellID);

    // Bard, every song that finishes casting (a restarted one included) grants the vigor
    uint32 bardVigorSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_BARD_VIGOR);
    if (bardVigorSpellID != 0 && IsSpellAnEQBardSong(spellInfo->Id) == true && PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_BARD_AURA) == true)
        player->CastSpell(player, bardVigorSpellID, true);

    // Wizard
    uint32 wizardFocusSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_WIZARD_FOCUS);
    if (wizardFocusSpellID != 0 && PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_WIZARD_AURA) == true && IsSpellCountedForWizardFocus(spellInfo, spell) == true)
        player->CastSpell(player, wizardFocusSpellID, true);

    // Cleric, the heal may live on a spell the cast triggers or is linked to (Holy Nova) and that spell has a say in whether this was an area heal
    SpellInfo const* healSpellInfo = FindClassAuraDirectHealSpellInfo(spellInfo, 0);
    if (healSpellInfo != nullptr && PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_CLERIC_AURA) == true)
    {
        if (spellInfo->IsAffectingArea() == true || healSpellInfo->IsAffectingArea() == true)
        {
            uint32 cadenceSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_CLERIC_CADENCE);
            if (cadenceSpellID != 0)
                player->CastSpell(player, cadenceSpellID, true);
        }
        else
        {
            uint32 hasteSpellID = GetClassAuraSpellID(EQ_CLASSAURA_SPELL_CLERIC_HASTE);
            Unit* target = spell->m_targets.GetUnitTarget();
            if (hasteSpellID != 0 && target != nullptr && target->IsAlive() == true && target->IsInWorld() == true && player->IsFriendlyTo(target) == true)
                player->CastSpell(target, hasteSpellID, true);
        }
    }
}

bool EverQuestMod::IsMovementCastSnareExemptForPlayer(Player* player)
{
    if (IsClassAuraSystemEnabled() == false)
        return false;
    return PlayerHasClassAura(player, EQ_CLASSAURA_SPELL_WIZARD_AURA);
}
