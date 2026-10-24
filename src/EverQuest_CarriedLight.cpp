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

#include "Creature.h"
#include "Item.h"
#include "Log.h"
#include "LootMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

#include "EverQuest.h"

using namespace std;

// Carried light sources.  In EverQuest a torch, lantern or lightstone lights the area around whoever carries it.  The
// MergedWorlds client draws that light, and it learns which light a character carries from one of the converter's light
// source auras (a Dummy aura, misc value 112, misc value B = the EQ light type), which the server keeps on the player here.
// The aura is an ordinary non-passive aura, so the core gives it a visible aura slot and sends it to every client that can
// see the player (AuraApplication::ClientUpdate on a change, Player::GetAurasForTarget when the player comes into view).
//
// Creatures glow the same way.  TAKP lights an NPC from its innate light (0 for every NPC in its data), its equipped items
// and any light in its loot (NPC::UpdateEquipmentLight), so a will o' wisp that rolled a lightstone glows.  The creature gets
// the aura for the brightest light its spawn rolled (OnCreatureAddWorld, after the loot preroll).  The aura persists through
// death, and like a TAKP corpse (Corpse::UpdateEquipmentLight) the corpse keeps glowing until that light is looted.
//
// Threading: the periodic check runs from OnPlayerUpdate on the player's map thread.  Login, logout and startup run on the
// world thread, outside the map update window, and the item hooks run on whichever of the two is processing the player.
// The light types and spell ID start are loaded once at startup and only read after.  The per-player state map and its
// entries are guarded by RuntimeStateMutex, since a group roll can finish on another map's thread and queue a corpse
// re-check for its winner.  Queued corpses are only looked up in the player's own map on the player's own update.  The
// lock is never held across the aura calls, which can re-enter mod hooks.

bool EverQuestMod::IsCarriedLightSpell(uint32 spellID)
{
    return ConfigSystemCarriedLightSpellIDStart != 0 && spellID >= ConfigSystemCarriedLightSpellIDStart && spellID < ConfigSystemCarriedLightSpellIDStart + EQ_CARRIED_LIGHT_TYPE_COUNT;
}

void EverQuestMod::ValidateCarriedLightSpells()
{
    // Runs at startup once the spell store is loaded.  Without every light source aura the feature stays off
    CarriedLightSpellsAvailable = false;
    if (ConfigSystemCarriedLightSpellIDStart == 0)
        return;
    for (uint32 lightTypeIndex = 0; lightTypeIndex < EQ_CARRIED_LIGHT_TYPE_COUNT; ++lightTypeIndex)
    {
        uint32 spellID = ConfigSystemCarriedLightSpellIDStart + lightTypeIndex;
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellID);
        if (spellInfo == nullptr)
        {
            LOG_ERROR("module.EverQuest", "EverQuestMod carried light sources are disabled, since light source aura spell {} does not exist. Deploy the server spell data from the most recent converter output.", spellID);
            return;
        }

        // A passive aura gets no visible aura slot, so it would never reach other clients (Aura::CanBeSentToClient)
        if (spellInfo->IsPassive() == true)
            LOG_ERROR("module.EverQuest", "EverQuestMod light source aura spell {} is passive, so other players will not see that light.", spellID);
    }
    CarriedLightSpellsAvailable = true;
}

uint8 EverQuestMod::GetCarriedLightTypeForItemTemplateID(uint32 itemTemplateID)
{
    unordered_map<uint32, uint8>::const_iterator lightTypeItr = CarriedLightTypesByItemTemplateID.find(itemTemplateID);
    if (lightTypeItr == CarriedLightTypesByItemTemplateID.end())
        return 0;
    return lightTypeItr->second;
}

uint8 EverQuestMod::GetCarriedLightLevelForLightType(uint8 lightType)
{
    // How bright each EQ light type is, from the EQ client, which lets the brightest equipped light win
    switch (lightType)
    {
    case 1: return 1;   // Candle
    case 2: return 2;   // Torch
    case 3: return 3;   // Tiny Glowing Skull
    case 9: return 3;   // Light Globe
    case 12: return 4;  // Fire Beetle Eye
    case 14: return 4;  // Red Glow
    case 13: return 5;  // Coldlight
    case 15: return 5;  // Blue Glow
    case 4: return 6;   // Small Lantern
    case 5: return 7;   // Stein of Moggok
    case 10: return 7;  // Lightstone
    case 6: return 8;   // Large Lantern
    case 7: return 9;   // Flameless Lantern
    case 11: return 9;  // Greater Lightstone
    case 8: return 10;  // Globe of Stars
    default: return 0;
    }
}

uint8 EverQuestMod::CalculateCarriedLightTypeForPlayer(Player* player)
{
    // Same choice the EQ client makes.  The brightest equipped light wins, and the first one found keeps a tie
    uint8 carriedLightType = 0;
    uint8 carriedLightLevel = 0;
    for (uint8 equipSlot = EQUIPMENT_SLOT_START; equipSlot < EQUIPMENT_SLOT_END; ++equipSlot)
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, equipSlot);
        if (item == nullptr)
            continue;
        uint8 lightType = GetCarriedLightTypeForItemTemplateID(item->GetEntry());
        uint8 lightLevel = GetCarriedLightLevelForLightType(lightType);
        if (lightLevel > carriedLightLevel)
        {
            carriedLightType = lightType;
            carriedLightLevel = lightLevel;
        }
    }

    // A light globe, lightstone, greater lightstone, fire beetle eye or coldlight loose in the main backpack (never inside a
    // bag) overrides that no matter how bright, and the last such slot wins
    for (uint8 backpackSlot = INVENTORY_SLOT_ITEM_START; backpackSlot < INVENTORY_SLOT_ITEM_END; ++backpackSlot)
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, backpackSlot);
        if (item == nullptr)
            continue;
        ItemTemplate const* itemTemplate = item->GetTemplate();
        if (itemTemplate == nullptr || item->IsBag() == true || itemTemplate->Class == ITEM_CLASS_CONTAINER || itemTemplate->Class == ITEM_CLASS_QUIVER)
            continue;
        uint8 lightType = GetCarriedLightTypeForItemTemplateID(item->GetEntry());
        if (lightType >= 9 && lightType <= 13)
            carriedLightType = lightType;
    }
    return carriedLightType;
}

uint8 EverQuestMod::CalculateCarriedLightTypeForItemTemplateIDs(const vector<uint32>& itemTemplateIDs)
{
    // A creature's loot: the brightest light wins, and the first one found keeps a tie.  TAKP tells equipped lights from
    // carried ones (only a lightstone type can beat an equipped light), which only differs when an unequipped lantern
    // outshines an equipped torch
    uint8 carriedLightType = 0;
    uint8 carriedLightLevel = 0;
    for (uint32 itemTemplateID : itemTemplateIDs)
    {
        uint8 lightType = GetCarriedLightTypeForItemTemplateID(itemTemplateID);
        uint8 lightLevel = GetCarriedLightLevelForLightType(lightType);
        if (lightLevel > carriedLightLevel)
        {
            carriedLightType = lightType;
            carriedLightLevel = lightLevel;
        }
    }
    return carriedLightType;
}

void EverQuestMod::ApplyCarriedLightAuraForUnit(Unit* unit, uint8 lightType)
{
    uint32 wantedSpellID = 0;
    if (lightType != 0 && lightType <= EQ_CARRIED_LIGHT_TYPE_COUNT)
        wantedSpellID = ConfigSystemCarriedLightSpellIDStart + lightType - 1;

    // Only one light source aura at a time, and nothing is touched when the right one is already there
    for (uint32 lightTypeIndex = 0; lightTypeIndex < EQ_CARRIED_LIGHT_TYPE_COUNT; ++lightTypeIndex)
    {
        uint32 spellID = ConfigSystemCarriedLightSpellIDStart + lightTypeIndex;
        if (spellID != wantedSpellID && unit->HasAura(spellID) == true)
            unit->RemoveAurasDueToSpell(spellID);
    }
    if (wantedSpellID == 0)
        return;

    // The aura never runs out.  One restored from the character's saved auras or stretched by a duration mod is put back to infinite
    Aura* existingAura = unit->GetAura(wantedSpellID);
    if (existingAura != nullptr)
    {
        if (existingAura->GetMaxDuration() != -1 || existingAura->GetDuration() != -1)
        {
            existingAura->SetMaxDuration(-1);
            existingAura->SetDuration(-1);
        }
        return;
    }

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(wantedSpellID);
    if (spellInfo == nullptr)
        return;

    // Unit::_CreateAuraApplication refuses a dead target unless the spell persists through death or allows dead targets.  Skip it
    // rather than leave an aura that never applies, and the check after the resurrection adds it
    if (unit->IsAlive() == false && spellInfo->IsDeathPersistent() == false && spellInfo->IsAllowingDeadTarget() == false)
        return;

    // Added straight onto the unit rather than cast, so there is no cast, spell go packet or combat log line, no cast checks to fail,
    // and no cast item for the core to tie the aura to.  The unit is the caster, so on a player it saves and reloads with the character
    // like a self buff
    Aura* aura = unit->AddAura(spellInfo, MAX_EFFECT_MASK, unit);
    if (aura != nullptr && (aura->GetMaxDuration() != -1 || aura->GetDuration() != -1))
    {
        aura->SetMaxDuration(-1);
        aura->SetDuration(-1);
    }
}

void EverQuestMod::RefreshCarriedLightForPlayer(Player* player)
{
    if (player == nullptr || CarriedLightSpellsAvailable == false || player->IsInWorld() == false)
        return;
    ApplyCarriedLightAuraForUnit(player, CalculateCarriedLightTypeForPlayer(player));
}

void EverQuestMod::RefreshCarriedLightForCreatureFromPreloadedLoot(Creature* creature)
{
    // Runs from OnCreatureAddWorld on the creature's map thread (the loot preroll that fills this list ran just before)
    if (creature == nullptr || CarriedLightSpellsAvailable == false)
        return;
    uint8 lightType = 0;
    if (HasPreloadedLootItemIDsForCreatureGUID(creature->GetMap(), creature->GetGUID()) == true)
        lightType = CalculateCarriedLightTypeForItemTemplateIDs(GetPreloadedLootIDsForCreatureGUID(creature->GetMap(), creature->GetGUID()));
    ApplyCarriedLightAuraForUnit(creature, lightType);
}

// A free for all item is never marked looted on the corpse, since everyone gets their own copy, so it counts as taken once anyone has taken theirs
static bool HasAnyPlayerLootedFreeForAllItem(QuestItemMap const& playerItems, uint8 lootIndex)
{
    for (QuestItemMap::const_iterator playerItr = playerItems.begin(); playerItr != playerItems.end(); ++playerItr)
    {
        if (playerItr->second == nullptr)
            continue;
        for (QuestItem const& playerItem : *playerItr->second)
            if (playerItem.index == lootIndex && playerItem.is_looted == true)
                return true;
    }
    return false;
}

void EverQuestMod::RefreshCarriedLightForCreatureCorpse(Creature* creature)
{
    // Runs on the corpse's map thread, from OnPlayerLootItem or the update of a player who handed out or won its loot.  Only the items still on the corpse count
    if (creature == nullptr || CarriedLightSpellsAvailable == false || creature->IsAlive() == true)
        return;
    vector<uint32> remainingItemTemplateIDs;
    for (size_t itemIndex = 0; itemIndex < creature->loot.items.size(); ++itemIndex)
    {
        LootItem const& lootItem = creature->loot.items[itemIndex];
        if (lootItem.is_looted == true)
            continue;
        if (lootItem.freeforall == true && HasAnyPlayerLootedFreeForAllItem(creature->loot.GetPlayerFFAItems(), (uint8)itemIndex) == true)
            continue;
        remainingItemTemplateIDs.push_back(lootItem.itemid);
    }
    for (size_t itemIndex = 0; itemIndex < creature->loot.quest_items.size(); ++itemIndex)
    {
        LootItem const& lootItem = creature->loot.quest_items[itemIndex];
        if (lootItem.is_looted == true)
            continue;
        if (lootItem.freeforall == true && HasAnyPlayerLootedFreeForAllItem(creature->loot.GetPlayerQuestItems(), (uint8)itemIndex) == true)
            continue;
        remainingItemTemplateIDs.push_back(lootItem.itemid);
    }
    ApplyCarriedLightAuraForUnit(creature, CalculateCarriedLightTypeForItemTemplateIDs(remainingItemTemplateIDs));
}

void EverQuestMod::QueueCarriedLightCorpseRefreshForPlayer(Player* player, ObjectGuid corpseGUID)
{
    // Master loot and group rolls take items off a corpse without OnPlayerLootItem, and the item is only marked looted after the hooks they do have (a roll
    // can also finish on the corpse's map thread for a winner on another map).  So the corpse is re-checked on this player's next update on their own map thread
    if (player == nullptr || CarriedLightSpellsAvailable == false || corpseGUID.IsCreatureOrVehicle() == false)
        return;
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    unordered_map<ObjectGuid, EverQuestPlayerCarriedLightState>::iterator carriedLightStateItr = PlayerCarriedLightStatesByPlayerGUID.find(player->GetGUID());
    if (carriedLightStateItr != PlayerCarriedLightStatesByPlayerGUID.end())
        carriedLightStateItr->second.PendingCorpseRefreshGUIDs.push_back(corpseGUID);
}

void EverQuestMod::QueueCarriedLightRefreshForPlayer(Player* player)
{
    // Item and state hooks only mark the check as due, so it runs on the player's next update once the item move or death has finished.
    // A player not yet tracked (still loading) gets checked on login anyway
    if (player == nullptr || CarriedLightSpellsAvailable == false)
        return;
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    unordered_map<ObjectGuid, EverQuestPlayerCarriedLightState>::iterator carriedLightStateItr = PlayerCarriedLightStatesByPlayerGUID.find(player->GetGUID());
    if (carriedLightStateItr != PlayerCarriedLightStatesByPlayerGUID.end())
        carriedLightStateItr->second.RefreshTimerMS = EQ_CARRIED_LIGHT_REFRESH_TIME_IN_MS;
}

void EverQuestMod::UpdateCarriedLightForPlayer(Player* player, uint32 diffInMS)
{
    if (player == nullptr || CarriedLightSpellsAvailable == false)
        return;

    // The periodic check is the authority, since moving items around the backpack has no hook and other things (death, class switches,
    // aura clears) can take the aura away.  The state is only read and written under the lock, since a group roll can queue work for this
    // player from another map's thread
    bool isRefreshDue = false;
    vector<ObjectGuid> corpseRefreshGUIDs;
    {
        std::lock_guard<std::mutex> lock(RuntimeStateMutex);
        EverQuestPlayerCarriedLightState& carriedLightState = PlayerCarriedLightStatesByPlayerGUID[player->GetGUID()];
        carriedLightState.RefreshTimerMS += diffInMS;
        if (carriedLightState.RefreshTimerMS >= EQ_CARRIED_LIGHT_REFRESH_TIME_IN_MS)
        {
            carriedLightState.RefreshTimerMS = 0;
            isRefreshDue = true;
        }
        corpseRefreshGUIDs.swap(carriedLightState.PendingCorpseRefreshGUIDs);
    }

    // A corpse on another map than this player's is left alone
    for (ObjectGuid const& corpseGUID : corpseRefreshGUIDs)
        RefreshCarriedLightForCreatureCorpse(player->GetMap()->GetCreature(corpseGUID));

    if (isRefreshDue == true)
        RefreshCarriedLightForPlayer(player);
}

void EverQuestMod::ClearCarriedLightTrackingForPlayer(ObjectGuid playerGUID)
{
    std::lock_guard<std::mutex> lock(RuntimeStateMutex);
    PlayerCarriedLightStatesByPlayerGUID.erase(playerGUID);
}
