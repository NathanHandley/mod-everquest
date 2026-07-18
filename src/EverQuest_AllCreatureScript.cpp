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
#include "GameTime.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "LootMgr.h"
#include "ItemTemplate.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_AllCreatureScript : public AllCreatureScript
{
public:
    EverQuest_AllCreatureScript() : AllCreatureScript("EverQuest_AllCreatureScript") {}

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
            EverQuest->SetupCreatureEmoteState(creature);
            RestrictCreatureOwnedPetAggroRange(creature);
        }

        uint32 entryID = creature->GetEntry();
        if (entryID < EverQuest->ConfigSystemCreatureTemplateIDMin || entryID > EverQuest->ConfigSystemCreatureTemplateIDMax)
            return;
        SetVisualEquipment(creature);
        ApplyLootWornEffectAuras(creature);
        SetupRangedAttack(creature);
        SetupSummonedPet(creature);
        EverQuest->SetupCreatureCombatAbilities(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        // Remove EverQuest creatures from the trackers
        uint32 mapID = creature->GetMap()->GetId();
        if (mapID >= EverQuest->ConfigSystemMapDBCIDMin && mapID <= EverQuest->ConfigSystemMapDBCIDMax)
            EverQuest->RemoveCreatureAsLoaded(mapID, creature);
        EverQuest->RemoveCreatureRangedAttackState(creature);
        EverQuest->RemoveCreatureCombatAbilityState(creature);
        EverQuest->RemoveCreatureUnstickState(creature);
        EverQuest->RemoveCreatureSocialAggroState(creature);
        EverQuest->RemoveCreatureEmoteState(creature);
        EverQuest->RemoveCreatureMovementSoundState(creature);
        EverQuest->RemoveCreatureKillSpawnCombatWatchState(creature);
        EverQuest->RemoveVulakLockState(creature);
        EverQuest->RemoveCreatureDefendPlayerWatchState(creature);
    }

    void OnAllCreatureUpdate(Creature* creature, uint32 diff) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        uint32 entryID = creature->GetEntry();
        if (entryID < EverQuest->ConfigSystemCreatureTemplateIDMin || entryID > EverQuest->ConfigSystemCreatureTemplateIDMax)
            return;
        EverQuest->UpdateCreatureRangedAttack(creature, diff);
        EverQuest->UpdateCreatureCombatAbilities(creature, diff);
        EverQuest->UpdateCreatureUnstick(creature, diff);
        EverQuest->UpdateCreatureScaledSocialAggro(creature, diff);
        EverQuest->UpdateCreatureEmotes(creature, diff);
        EverQuest->UpdateCreatureMovementSound(creature, diff);
        EverQuest->UpdateCreatureKillSpawnCombatWatch(creature, diff);
        EverQuest->UpdateVulakLock(creature, diff);
        EverQuest->UpdateCreatureDefendFriendlyPlayers(creature, diff);
        EverQuest->UpdateCreatureDefendFactionRestore(creature);
    }

private:
    void SetupSummonedPet(Creature* creature)
    {
        if (creature->IsSummon() == false || creature->IsPet() == true)
            return;
        if (EverQuest->HasPetDataForCreatureTemplateID(creature->GetEntry()) == false)
            return;
        EverQuestPet petData = EverQuest->GetPetDataForCreatureTemplateID(creature->GetEntry());
        if (petData.NamingType == EQ_PET_NAMING_TYPE_RANDOM)
        {
            creature->SetName(sObjectMgr->GeneratePetName(creature->GetEntry()));
            creature->SetUInt32Value(UNIT_FIELD_PETNUMBER, sObjectMgr->GeneratePetNumber());
            creature->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(GameTime::GetGameTime().count()));
        }

        // Pet equipment
        if (petData.MainhandItemTemplateID != 0)
            creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 0, petData.MainhandItemTemplateID);
        if (petData.OffhandItemTemplateID != 0)
            creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, petData.OffhandItemTemplateID);
    }

    void RestrictCreatureOwnedPetAggroRange(Creature* creature)
    {
        if (creature->IsPet() == false && creature->IsSummon() == false)
            return;
        if (creature->GetCharmerOrOwnerGUID().IsCreature() == false)
            return;
        creature->SetDetectionDistance(0.0f);
    }

    // Ranged attack is either if the creature has the special ability for it, or they have a bow+arrow in inventory
    // Note: This is based on TAKP's GetSpecialAbility / HasBowAndArrowEquipped logic
    void SetupRangedAttack(Creature* creature)
    {
        // Only apply this to EQ creatures
        if (EverQuest->HasCreatureDataForCreatureTemplateID(creature->GetEntry()) == false
            && EverQuest->HasCreatureLootDataForCreatureTemplateEntryID(creature->GetEntry()) == false)
            return;

        // Reset the ranged visual slot in case this creature object is being recycled
        creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 2, 0);

        if (EverQuest->ConfigCombatSkillsRangedAttackEnabled == false || EverQuest->ConfigSystemRangedAttackSpellID == 0)
            return;

        // Special ability path
        bool hasSpecialAbility = false;
        float minRange = 0.0f;
        float maxRange = 0.0f;
        int32 damageModPct = 0;
        if (EverQuest->HasCreatureDataForCreatureTemplateID(creature->GetEntry()) == true)
        {
            EverQuestCreature eqCreature = EverQuest->GetCreatureDataForCreatureTemplateID(creature->GetEntry());
            if (eqCreature.RangedAttackEnabled == true)
            {
                hasSpecialAbility = true;
                minRange = (float)eqCreature.RangedAttackMinRange;
                maxRange = (float)eqCreature.RangedAttackMaxRange;
                damageModPct = eqCreature.RangedAttackDamageModPct;
            }
        }

        // Bow + arrow path
        bool hasBow = false;
        bool hasArrow = false;
        uint32 bowVisualItemID = 0;
        if (EverQuest->HasPreloadedLootItemIDsForCreatureGUID(creature->GetGUID()) == true)
        {
            for (uint32 itemTemplateID : EverQuest->GetPreloadedLootIDsForCreatureGUID(creature->GetGUID()))
            {
                uint32 itemTemplateForNPCEquipID = EverQuest->GetNPCEquipItemTemplateIDForItemTemplate(itemTemplateID);
                ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemTemplateForNPCEquipID);
                if (!itemTemplate)
                    continue;
                if (itemTemplate->Class == ITEM_CLASS_WEAPON && itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_BOW)
                {
                    hasBow = true;
                    bowVisualItemID = itemTemplate->ItemId;
                }
                else if (itemTemplate->Class == ITEM_CLASS_PROJECTILE && itemTemplate->SubClass == ITEM_SUBCLASS_ARROW)
                    hasArrow = true;
            }
        }

        if (hasSpecialAbility == false && (hasBow == false || hasArrow == false))
            return;

        // Show the bow (in the ranged virtual slot) so the client renders the arrow projectile when the shot is cast
        // Note, some (all?) creatures don't have a mount point in the cast animation so it disappears. Will try to fix.
        if (hasBow == true)
        {
            creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 2, bowVisualItemID);
            if (creature->GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 0) == 0)
                creature->SetSheath(SHEATH_STATE_RANGED);
        }

        EverQuest->StoreCreatureRangedAttackState(creature, minRange, maxRange, damageModPct);
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
        // Skip non-eq creatures
        if (EverQuest->HasCreatureDataForCreatureTemplateID(creature->GetEntry()) == false)
            return;

        // Roll items for only EQ creatures
        if (EverQuest->HasCreatureLootDataForCreatureTemplateEntryID(creature->GetEntry()))
            EverQuest->RollLootItemsForCreature(creature->GetGUID(), creature->GetEntry());

        // Reset
        creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 0, 0);
        creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, 0);
        EverQuest->ClearVisualEquippedItemsForCreatureGUID(creature->GetGUID());

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
