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

#include "Configuration/Config.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"
#include "LootMgr.h"
#include "ItemTemplate.h"

#include "EverQuest.h"

using namespace std;

// Unique gossip action for mod-injected secondary class train options
#define EQ_GOSSIP_ACTION_SECONDARY_TRAIN    0xEC0001

class EverQuest_AllCreatureScript : public AllCreatureScript
{
public:
    EverQuest_AllCreatureScript() : AllCreatureScript("EverQuest_AllCreatureScript") {}

    // When a player has the current secondary eq class that matches a guild master, offer the native trainer
    // option.  Base / Primary class non-matching guild masters use normal behavior
    bool CanCreatureGossipHello(Player* player, Creature* creature) override
    {
        if (EverQuest->IsEnabled == false)
            return false;
        if (EverQuest->HasCreatureDataForCreatureTemplateID(creature->GetEntry()) == false)
            return false;
        uint8 trainerEQClass = EverQuest->GetCreatureDataForCreatureTemplateID(creature->GetEntry()).EQClassTrainerType;
        if (trainerEQClass == EQ_EQCLASS_NONE)
            return false;

        // When the player's current secondary EQ class matches, offer the native trainer for their logged-in class
        if (ShouldOfferSecondaryClassTraining(player, creature) == true)
        {
            ClearGossipMenuFor(player);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "I would like to train.", GOSSIP_SENDER_MAIN, EQ_GOSSIP_ACTION_SECONDARY_TRAIN);

            // Preserve quest access for guild masters that also give quests (our custom menu replaces the default)
            if (creature->HasNpcFlag(UNIT_NPC_FLAG_QUESTGIVER))
                player->PrepareQuestMenu(creature->GetGUID());

            SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());
            return true;
        }

        // On no-match, use the original gossip menu.  Secondary have gossip menu, so fall to a generic train option
        if (EverQuest->IsEQClassABaseEQClass(trainerEQClass) == true)
            return false;

        ClearGossipMenuFor(player);
        if (creature->HasNpcFlag(UNIT_NPC_FLAG_QUESTGIVER))
            player->PrepareQuestMenu(creature->GetGUID());
        SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool CanCreatureGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        if (EverQuest->IsEnabled == false)
            return false;
        if (sender != GOSSIP_SENDER_MAIN || action != EQ_GOSSIP_ACTION_SECONDARY_TRAIN)
            return false;

        // Re-validate so the trainer is only opened for a genuine current secondary match
        if (ShouldOfferSecondaryClassTraining(player, creature) == false)
            return false;

        // Mirror the GOSSIP_OPTION_TRAINER handler exactly
        player->GetSession()->SendTrainerList(creature);
        return true;
    }

    void OnCreatureAddWorld(Creature* creature) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        // Store EverQuest creatures on the tracker
        uint32 mapID = creature->GetMap()->GetId();
        if (mapID >= EverQuest->ConfigSystemMapDBCIDMin && mapID <= EverQuest->ConfigSystemMapDBCIDMax)
        {
            // Despawn creatures that violate spawn restrictions (creature spawn limits, one creature alive per
            // spawn point, spawn group limits), with a respawn timer so the spawn re-rolls on a later cycle
            if (EverQuest->ShouldDespawnCreatureDueToSpawnRestrictions(mapID, creature) == true)
            {
                creature->DespawnOrUnsummon(Milliseconds(1), Seconds(creature->GetRespawnDelay()));
                return;
            }
            EverQuest->AddCreatureAsLoaded(mapID, creature);
        }
        SetVisualEquipment(creature);
        ApplyLootWornEffectAuras(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        // Remove EverQuest creatures from the trackers
        uint32 mapID = creature->GetMap()->GetId();
        if (mapID >= EverQuest->ConfigSystemMapDBCIDMin && mapID <= EverQuest->ConfigSystemMapDBCIDMax)
            EverQuest->RemoveCreatureAsLoaded(mapID, creature);
    }

private:
    bool ShouldOfferSecondaryClassTraining(Player* player, Creature* creature)
    {
        if (EverQuest->HasCreatureDataForCreatureTemplateID(creature->GetEntry()) == false)
            return false;
        uint8 trainerEQClass = EverQuest->GetCreatureDataForCreatureTemplateID(creature->GetEntry()).EQClassTrainerType;
        if (trainerEQClass == EQ_EQCLASS_NONE)
            return false;

        const EverQuestClassMap classMap = EverQuest->GetClassMapForWOWClassID(player->getClass());
        if (trainerEQClass == classMap.EQClassIDBase)
            return false;
        if (trainerEQClass != EverQuest->GetCurrentSecondEQClassForPlayer(player))
            return false;

        return true;
    }

    void ApplyLootWornEffectAuras(Creature* creature)
    {
        if (EverQuest->HasPreloadedLootItemIDsForCreatureGUID(creature->GetGUID()) == false)
            return;
        for (uint32 itemTemplateID : EverQuest->GetPreloadedLootIDsForCreatureGUID(creature->GetGUID()))
        {
            uint32 wornEffectSpellID = EverQuest->GetWornEffectSpellIDForItemTemplate(itemTemplateID);
            if (wornEffectSpellID == 0)
                continue;
            if (creature->HasAura(wornEffectSpellID) == true)
                continue;
            creature->AddAura(wornEffectSpellID, creature);
        }
    }

    void SetVisualEquipment(Creature* creature)
    {
        // Reset first
        creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 0, 0);
        creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, 0);
        EverQuest->ClearVisualEquippedItemsForCreatureGUID(creature->GetGUID());

        // Roll items
        if (EverQuest->HasCreatureLootDataForCreatureTemplateEntryID(creature->GetEntry()))
            EverQuest->RollLootItemsForCreature(creature->GetGUID(), creature->GetEntry());

        // Add visual information
        if (EverQuest->HasCreatureDataForCreatureTemplateID(creature->GetEntry()) == false)
            return;

        EverQuestCreature eqCreature = EverQuest->GetCreatureDataForCreatureTemplateID(creature->GetEntry());
        if (eqCreature.CanShowHeldLootItems == false || EverQuest->HasPreloadedLootItemIDsForCreatureGUID(creature->GetGUID()) == false)
            return;

        // Prioritize what items to show as worn
        vector<ItemTemplate const*> oneHandWeapons;
        vector<ItemTemplate const*> twoHandWeapons;
        vector<ItemTemplate const*> shields;
        vector<ItemTemplate const*> heldItems;
        vector<ItemTemplate const*> fishingPoles;
        for (uint32 itemTemplateID : EverQuest->GetPreloadedLootIDsForCreatureGUID(creature->GetGUID()))
        {
            uint32 itemTemplateForNPCEquipID = EverQuest->GetNPCEquipItemTemplateIDForItemTemplate(itemTemplateID);
            ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemTemplateForNPCEquipID);
            if (!itemTemplate)
            {
                LOG_ERROR("module.EverQuest", "EverQuestMod::OnCreatureAddWorld failure, as item template ID {} could not be found", itemTemplateID);
                continue;
            }

            // Sort items by types
            if (itemTemplate->Class == ITEM_CLASS_WEAPON)
            {
                if (itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_FISHING_POLE)
                    fishingPoles.push_back(itemTemplate);
                else if (itemTemplate->InventoryType == INVTYPE_2HWEAPON)
                    twoHandWeapons.push_back(itemTemplate);
                else if (itemTemplate->InventoryType == INVTYPE_WEAPONMAINHAND || itemTemplate->InventoryType == INVTYPE_WEAPONOFFHAND || itemTemplate->InventoryType == INVTYPE_WEAPON)
                    oneHandWeapons.push_back(itemTemplate);
            }
            else if (eqCreature.CanShowHeldLootShields == true && itemTemplate->Class == ITEM_CLASS_ARMOR && itemTemplate->InventoryType == INVTYPE_SHIELD)
                shields.push_back(itemTemplate);
            else if (itemTemplate->InventoryType == INVTYPE_HOLDABLE)
                heldItems.push_back(itemTemplate);
        }

        // Assign positions
        ItemTemplate const* mainHandItem = nullptr;
        ItemTemplate const* offHandItem = nullptr;
        bool isDualWielding = false;
        bool doHoldFishingPole = (fishingPoles.size() > 0) && (oneHandWeapons.size() == 0) && (twoHandWeapons.size() == 0) && (shields.size() == 0) && (heldItems.size() == 0);
        if (doHoldFishingPole == true)
            mainHandItem = fishingPoles[0];
        else if (twoHandWeapons.size() > 0)
            mainHandItem = twoHandWeapons[0];
        else
        {
            // Mainhand
            if (oneHandWeapons.size() > 0)
                mainHandItem = oneHandWeapons[0];

            // Offhand
            if (shields.size() > 0)
                offHandItem = shields[0];
            else if (oneHandWeapons.size() > 1)
            {
                offHandItem = oneHandWeapons[1];
                isDualWielding = true; // Need this for attacking with offhand
            }
            else if (heldItems.size() > 0)
                offHandItem = heldItems[0];
        }

        // Show needed visuals
        uint32 mainhandItemID = 0;
        if (mainHandItem != nullptr)
        {
            creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 0, mainHandItem->ItemId);
            mainhandItemID = mainHandItem->ItemId;
        }

        uint32 offhandItemID = 0;
        if (offHandItem != nullptr)
        {
            creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, offHandItem->ItemId);
            offhandItemID = offHandItem->ItemId;
        }

        // Update combat posture and equipped visual tracking
        if (mainHandItem != nullptr || offHandItem != nullptr)
        {
            creature->SetSheath(SHEATH_STATE_MELEE);
            EverQuest->TrackVisualEquippedItemsForCreatureGUID(creature->GetGUID(), mainhandItemID, offhandItemID, isDualWielding);
        }

        // Reset combat type
        creature->UpdateDamagePhysical(BASE_ATTACK);
    }
};

void AddEverQuestAllCreatureScripts()
{
    new EverQuest_AllCreatureScript();
}
