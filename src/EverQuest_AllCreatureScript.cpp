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
#include "LootMgr.h"
#include "ItemTemplate.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_AllCreatureScript: public AllCreatureScript
{
public:
    EverQuest_AllCreatureScript() : AllCreatureScript("EverQuest_AllCreatureScript") {}

    void OnCreatureSelectLevel(const CreatureTemplate* /*cinfo*/, Creature* creature) override
    {
        // Roll items
        if (EverQuest->HasLootTemplateRowsByCreatureTemplateEntryID(creature->GetEntry()))
            EverQuest->RollLootItemsForCreature(creature->GetGUID(), creature->GetEntry());

        // Add visual information
        if (EverQuest->HasCreatureDataForCreatureTemplateID(creature->GetEntry()) == true)
        {
            // Reset the held visuals
            creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 0, 0);
            creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, 0);

            EverQuestCreature eqCreature = EverQuest->GetCreatureDataForCreatureTemplateID(creature->GetEntry());
            if (eqCreature.CanShowHeldLootItems > 0 && EverQuest->HasPreloadedLootItemIDsForCreatureGUID(creature->GetGUID()))
            {
                // Prioritize what items to show as worn
                vector<ItemTemplate const*> oneHandWeapons;
                vector<ItemTemplate const*> twoHandWeapons;
                vector<ItemTemplate const*> shields;
                vector<ItemTemplate const*> heldItems;
                vector<ItemTemplate const*> fishingPoles;
                for (uint32 itemTemplateID : EverQuest->GetPreloadedLootIDsForCreatureGUID(creature->GetGUID()))
                {
                    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemTemplateID);
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
                    else if (itemTemplate->Class == ITEM_CLASS_ARMOR && itemTemplate->InventoryType == INVTYPE_SHIELD)
                        shields.push_back(itemTemplate);
                    else if (itemTemplate->InventoryType == INVTYPE_HOLDABLE)
                        heldItems.push_back(itemTemplate);

                    // Assign positions
                    ItemTemplate const* mainHandItem = nullptr;
                    ItemTemplate const* offHandItem = nullptr;
                    bool doHoldFishingPole = (fishingPoles.size() > 0) && (oneHandWeapons.size() == 0) && (twoHandWeapons.size() == 0) && (shields.size() == 0) && (heldItems.size() == 0);
                    if (doHoldFishingPole == true && fishingPoles.size() > 0)
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
                            offHandItem = oneHandWeapons[1];
                        else if (heldItems.size() > 0)
                            offHandItem = heldItems[0];
                    }

                    // Show needed visuals
                    if (mainHandItem != nullptr)
                        creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 0, mainHandItem->ItemId);
                    if (offHandItem != nullptr)
                        creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, offHandItem->ItemId);
                    if (mainHandItem != nullptr || offHandItem != nullptr)
                        creature->SetSheath(SHEATH_STATE_MELEE);
                }
            }
        }
    }

    void OnCreatureAddWorld(Creature* creature) override
    {
        // Store EverQuest creatures on the tracker
        uint32 mapID = creature->GetMap()->GetId();
        if (mapID >= EverQuest->ConfigSystemMapDBCIDMin && mapID <= EverQuest->ConfigSystemMapDBCIDMax)
            EverQuest->AddCreatureAsLoaded(mapID, creature);        
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        // Remove EverQuest creatures from the trackers
        uint32 mapID = creature->GetMap()->GetId();
        if (mapID >= EverQuest->ConfigSystemMapDBCIDMin && mapID <= EverQuest->ConfigSystemMapDBCIDMax)
            EverQuest->RemoveCreatureAsLoaded(mapID, creature);
    }
};

void AddEverQuestAllCreatureScripts()
{
    new EverQuest_AllCreatureScript();
}
