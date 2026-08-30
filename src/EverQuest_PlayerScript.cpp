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

#include "Chat.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "Pet.h"
#include "Player.h"
#include "ReputationMgr.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "QuestDef.h"
#include "World.h"
#include "WorldSession.h"

#include "EverQuest.h"

#include <list>
#include <map>
#include <vector>

using namespace std;

class EverQuest_PlayerScript : public PlayerScript
{
public:
    EverQuest_PlayerScript() : PlayerScript("EverQuest_PlayerScript") {}

    Optional<bool> OnPlayerIsClass(Player const* player, Classes playerClass, ClassContext context) override
    {
        if (EverQuest->IsEnabled == false)
            return std::nullopt;

        // Pet::IsPermanentPetFor only counts a summoned pet as permanent when its creature type matches the owner's class (warlock pairs with demon, death knight pairs with undead)
        // and that permanence is what keeps the pet spellbook tab, the owner's pet auras and the saved pet number working.  EQ pet spells reach every wow class, and EQ pets are
        // demons except for the undead skeleton line, so report whichever of those two classes matches the pet that is out.
        if (context == CLASS_CONTEXT_PET)
        {
            uint32 activeEQPetCreatureType = EverQuest->GetActiveEQPetCreatureTypeForPlayer(const_cast<Player*>(player));
            if (activeEQPetCreatureType == CREATURE_TYPE_UNDEAD)
            {
                // Claiming warlock here would make Pet::IsPermanentPetFor demand a demon and drop the pet to non-permanent
                if (playerClass == CLASS_DEATH_KNIGHT)
                    return true;
                if (playerClass == CLASS_WARLOCK)
                    return false;
            }
            else if (activeEQPetCreatureType != 0 && playerClass == CLASS_WARLOCK)
                return true;
        }

        // Allows for non-rogues to pick pocket
        if (context == CLASS_CONTEXT_ABILITY && playerClass == CLASS_ROGUE && EverQuest->DoesPlayerHaveEQClassOfWOWClass(const_cast<Player*>(player), CLASS_ROGUE) == true)
            return true;

        return std::nullopt;
    }

    void OnPlayerBeforeGuardianInitStatsForLevel(Player* /*player*/, Guardian* /*guardian*/, CreatureTemplate const* cinfo, PetType& petType) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (cinfo == nullptr)
            return;
        if (EverQuest->HasPetDataForCreatureTemplateID(cinfo->Entry) == true)
            petType = SUMMON_PET;
    }

    bool OnPlayerHasActivePowerType(Player const* /*player*/, Powers /*power*/) override
    {
        // Enable all powers for all classes
        return true;
    }

    void OnPlayerKilledByCreature(Creature* killer, Player* killed) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (killer == nullptr || killed == nullptr)
            return;

        // TAKP fires both its 'KilledPC' and 'Killed' emote events when an NPC kills a player, and the converter folds those together into KilledPC
        if (killer->IsPet() == false && killer->IsControlledByPlayer() == false)
            EverQuest->DoCreatureEmoteEvent(killer, EQ_CREATURE_EMOTE_EVENT_KILLEDPC, killed);
    }

    bool OnPlayerCanEquipItem(Player* player, uint8 /*slot*/, uint16& /*dest*/, Item* pItem, bool /*swap*/, bool not_loading) override
    {
        if (EverQuest->IsEnabled == false)
            return true;
        // Don't strip already-equipped items
        if (not_loading == false)
            return true;
        if (pItem == nullptr)
            return true;
        if (EverQuest->IsItemEQClassAllowedForPlayer(player, pItem->GetEntry()) == true)
            return true;

        ChatHandler(player->GetSession()).PSendSysMessage("Your EQ classes cannot equip that item.");
        return false;
    }

    bool OnPlayerCanUseItem(Player* player, ItemTemplate const* proto, InventoryResult& result) override
    {
        if (EverQuest->IsEnabled == false)
            return true;
        if (proto == nullptr)
            return true;
        // Don't remove already-equipped items during inventory load
        if (player->GetSession() != nullptr && player->GetSession()->PlayerLoading() == true)
            return true;
        if (EverQuest->IsItemEQClassAllowedForPlayer(player, proto->ItemId) == true)
            return true;

        result = EQUIP_ERR_YOU_CAN_NEVER_USE_THAT_ITEM;
        return false;
    }

    void OnPlayerBeforeLoadPetFromDB(Player* player, uint32& petEntry, uint32& petNumber, bool& /*current*/, bool& forceLoadFromDB) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (forceLoadFromDB == true)
            return;

        uint32 creatureTemplateID = petEntry;
        if (creatureTemplateID == 0)
        {
            PetStable const* petStable = player->GetPetStable();
            if (petStable == nullptr)
                return;
            if (petNumber != 0)
            {
                if (petStable->CurrentPet && petStable->CurrentPet->PetNumber == petNumber)
                    creatureTemplateID = petStable->CurrentPet->CreatureId;
                if (creatureTemplateID == 0)
                    for (auto const& stabledPet : petStable->StabledPets)
                        if (stabledPet && stabledPet->PetNumber == petNumber)
                            creatureTemplateID = stabledPet->CreatureId;
                if (creatureTemplateID == 0)
                    for (PetStable::PetInfo const& unslottedPet : petStable->UnslottedPets)
                        if (unslottedPet.PetNumber == petNumber)
                            creatureTemplateID = unslottedPet.CreatureId;
            }
            else if (petStable->CurrentPet)
                creatureTemplateID = petStable->CurrentPet->CreatureId;
            else if (petStable->UnslottedPets.empty() == false)
                creatureTemplateID = petStable->UnslottedPets.front().CreatureId;
        }

        if (creatureTemplateID != 0 && EverQuest->HasPetDataForCreatureTemplateID(creatureTemplateID) == true)
            forceLoadFromDB = true;
    }

    // Called when a player completes a quest
    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Completing a non-EQ quest outside of an EQ zone permanently costs the player the adventurer aura
        if (EverQuest->IsQuestDisqualifyingForAdventurer(player, quest->GetQuestId()) == true)
        {
            if (EverQuest->DisqualifyPlayerFromAdventurer(player) == true && player->GetSession() != nullptr)
                ChatHandler(player->GetSession()).SendSysMessage("|cffFF0000You are no longer an Everquest Adventurer, as you completed a quest that is not from Everquest.|r");
        }

        // Grab the quest rewards, and apply any in the list
        const list<EverQuestQuestCompletionReputation>& questCompletionReputations = EverQuest->GetQuestCompletionReputationsForQuestTemplate(quest->GetQuestId());
        for (auto& completionReputation : questCompletionReputations)
        {
            float repChange = player->CalculateReputationGain(REPUTATION_SOURCE_QUEST, quest->GetQuestLevel(), static_cast<float>(completionReputation.CompletionRewardValue), completionReputation.FactionID);

            FactionEntry const* factionEntry = sFactionStore.LookupEntry(completionReputation.FactionID);
            if (factionEntry && repChange != 0)
            {
                player->GetReputationMgr().ModifyReputation(factionEntry,  repChange, false, static_cast<ReputationRank>(7));
            }
        }

        // Handle any quest reactions
        list<EverQuestQuestReaction> questReactions = EverQuest->GetQuestReactions(quest->GetQuestId());
        if (questReactions.empty() == false)
        {
            Map* map = player->GetMap();

            // Rows behind a walkto do not run at turn-in, they are handed to the walk and fire when the creature arrives
            vector<EverQuestPendingKillSpawnAction> arrivalActions;
            for (const EverQuestQuestReaction& questReaction : questReactions)
            {
                if (questReaction.FiresOnArrival == false)
                    continue;
                EverQuestPendingKillSpawnAction arrivalAction;
                arrivalAction.TargetCreatureTemplateID = questReaction.CreatureTemplateID;
                arrivalAction.RemainingMS = (int32)questReaction.DelayInMS;
                arrivalAction.PathListID = questReaction.PathListID;
                arrivalAction.GameObjectEntryID = questReaction.GameObjectEntryID;
                arrivalAction.GameObjectLifetimeSec = questReaction.GameObjectLifetimeSec;
                arrivalAction.SayText = questReaction.SayText;
                arrivalAction.ListenerGUID = player->GetGUID();
                arrivalAction.UseMoverPositionX = questReaction.UseNpcX;
                arrivalAction.UseMoverPositionY = questReaction.UseNpcY;
                arrivalAction.UseMoverPositionZ = questReaction.UseNpcZ;
                arrivalAction.UseMoverOrientation = questReaction.UseNpcOrientation;
                arrivalAction.PositionX = questReaction.PositionX;
                arrivalAction.PositionY = questReaction.PositionY;
                arrivalAction.PositionZ = questReaction.PositionZ;
                arrivalAction.Orientation = questReaction.Orientation;
                if (questReaction.UsePlayerX == true)
                    arrivalAction.PositionX = player->GetPositionX() + questReaction.AddedPlayerX;
                if (questReaction.UsePlayerY == true)
                    arrivalAction.PositionY = player->GetPositionY() + questReaction.AddedPlayerY;
                if (questReaction.UsePlayerZ == true)
                    arrivalAction.PositionZ = player->GetPositionZ();
                if (questReaction.UsePlayerOrientation == true)
                    arrivalAction.Orientation = player->GetOrientation();
                switch (questReaction.ReactionType)
                {
                case EQ_QUEST_REACTION_SAY: arrivalAction.ActionType = EQ_KILLSPAWN_ACTION_SAY; break;
                case EQ_QUEST_REACTION_EMOTE: arrivalAction.ActionType = EQ_KILLSPAWN_ACTION_EMOTE; break;
                case EQ_QUEST_REACTION_YELL: arrivalAction.ActionType = EQ_KILLSPAWN_ACTION_YELL; break;
                case EQ_QUEST_REACTION_ATTACKPLAYER: arrivalAction.ActionType = EQ_KILLSPAWN_ACTION_ATTACKPLAYER; break;
                case EQ_QUEST_REACTION_DESPAWN: arrivalAction.ActionType = EQ_KILLSPAWN_ACTION_DESPAWN; break;
                case EQ_QUEST_REACTION_SPAWN:
                case EQ_QUEST_REACTION_SPAWNUNIQUE:
                {
                    arrivalAction.ActionType = EQ_KILLSPAWN_ACTION_SPAWN;
                    if (questReaction.ReactionType == EQ_QUEST_REACTION_SPAWNUNIQUE)
                        arrivalAction.OnlyIfNotAliveCreatureTemplateID = questReaction.CreatureTemplateID;
                } break;
                case EQ_QUEST_REACTION_WALKGRID: arrivalAction.ActionType = EQ_KILLSPAWN_ACTION_WALKPATH; break;
                case EQ_QUEST_REACTION_SPAWNOBJECT: arrivalAction.ActionType = EQ_KILLSPAWN_ACTION_SPAWNOBJECT; break;
                default: continue; // Nothing else is meaningful once the walk is over
                }
                arrivalActions.push_back(arrivalAction);
            }

            for (const EverQuestQuestReaction& questReaction : questReactions)
            {
                if (questReaction.FiresOnArrival == true)
                    continue;
                float x = questReaction.PositionX;
                if (questReaction.UsePlayerX == true)
                    x = player->GetPositionX() + questReaction.AddedPlayerX;
                float y = questReaction.PositionY;
                if (questReaction.UsePlayerY == true)
                    y = player->GetPositionY() + questReaction.AddedPlayerY;
                float z = questReaction.PositionZ;
                if (questReaction.UsePlayerZ == true)
                    z = player->GetPositionZ();
                float orientation = questReaction.Orientation;
                if (questReaction.UsePlayerOrientation == true)
                    orientation = player->GetOrientation();

                switch (questReaction.ReactionType)
                {
                case EQ_QUEST_REACTION_ATTACKPLAYER:
                {
                    EverQuest->MakeCreatureAttackPlayer(questReaction.CreatureTemplateID, map, player);
                } break;
                case EQ_QUEST_REACTION_DESPAWN:
                {
                    if (questReaction.DelayInMS > 0)
                    {
                        EverQuestPendingKillSpawnAction action;
                        action.ActionType = EQ_KILLSPAWN_ACTION_DESPAWN;
                        action.TargetCreatureTemplateID = questReaction.CreatureTemplateID;
                        action.RemainingMS = (int32)questReaction.DelayInMS;
                        EverQuest->EnqueuePendingKillSpawnAction(map, action);
                    }
                    else
                        EverQuest->DespawnCreature(questReaction.CreatureTemplateID, map);
                } break;
                case EQ_QUEST_REACTION_SPAWN:
                case EQ_QUEST_REACTION_SPAWNUNIQUE:
                {
                    if (questReaction.DelayInMS > 0)
                    {
                        EverQuestPendingKillSpawnAction action;
                        action.ActionType = EQ_KILLSPAWN_ACTION_SPAWN;
                        action.TargetCreatureTemplateID = questReaction.CreatureTemplateID;
                        if (questReaction.ReactionType == EQ_QUEST_REACTION_SPAWNUNIQUE)
                            action.OnlyIfNotAliveCreatureTemplateID = questReaction.CreatureTemplateID;
                        action.PositionX = x;
                        action.PositionY = y;
                        action.PositionZ = z;
                        action.Orientation = orientation;
                        action.RemainingMS = (int32)questReaction.DelayInMS;
                        EverQuest->EnqueuePendingKillSpawnAction(map, action);
                    }
                    else
                        EverQuest->SpawnCreature(questReaction.CreatureTemplateID, map, x, y, z, orientation, questReaction.ReactionType == EQ_QUEST_REACTION_SPAWNUNIQUE);
                } break;
                case EQ_QUEST_REACTION_KILLSPAWN:
                {
                    EverQuest->TriggerQuestKillSpawn(map, questReaction);
                } break;
                case EQ_QUEST_REACTION_WALKTO:
                {
                    // The questgiver copy the player just handed in to is the one that walks
                    Creature* walker = EverQuest->GetNearestLoadedCreatureWithEntryID(map, questReaction.QuestgiverCreatureTemplateID, player);
                    if (walker != nullptr)
                        EverQuest->StartReactionWalk(walker, x, y, z, orientation, questReaction.Orientation != 0 || questReaction.UsePlayerOrientation == true, questReaction.MovementIsRun, arrivalActions);
                } break;
                case EQ_QUEST_REACTION_SPAWNOBJECT:
                {
                    Creature* dropper = EverQuest->GetNearestLoadedCreatureWithEntryID(map, questReaction.QuestgiverCreatureTemplateID, player);
                    EverQuest->SpawnReactionGameObject(dropper, questReaction.GameObjectEntryID, x, y, z, questReaction.GameObjectLifetimeSec);
                } break;
                case EQ_QUEST_REACTION_WALKGRID:
                {
                    Creature* gridWalker = EverQuest->GetNearestLoadedCreatureWithEntryID(map, questReaction.QuestgiverCreatureTemplateID, player);
                    if (gridWalker != nullptr)
                        EverQuest->StartReactionGridWalk(gridWalker, questReaction.PathListID, arrivalActions);
                } break;
                default: break; // Nothing
                }
            }
        }
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* victim, uint8 xpSource) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (EverQuest->ConfigSecondaryExpPoolGainPercent <= 0.0f)
            return;

        // Pool only fills from creature kills
        if (xpSource != PlayerXPSource::XPSOURCE_KILL)
            return;
        if (victim == nullptr || victim->IsCreature() == false)
            return;

        uint32 added = EverQuest->AddToSecondaryExpPoolForPlayer(player, amount);
        if (added > 0)
            EverQuest->SendExpPoolAddonMessageToPlayer(player, added);
    }

    void OnPlayerBeforeGetLevelForXPGain(Player const* player, uint8& level) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        EverQuest->HandleLevelCapOnBeforeExperienceGain(player, level);
    }

    bool OnPlayerCanGiveLevel(Player* player, uint8 newLevel) override
    {
        if (EverQuest->IsEnabled == false)
            return true;

        return EverQuest->HandleLevelCapOnCanGiveLevel(player, newLevel);
    }

    void OnPlayerEquip(Player* player, Item* /*it*/, uint8 /*bag*/, uint8 /*slot*/, bool /*update*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Equipping gear while illusioned can change change gear under some situations
        EverQuest->RefreshIllusionGearDisplayForPlayer(player);

        // Swapping a shield in or out changes how much armor bear/dire bear form should leave unmultiplied
        EverQuest->RefreshBearFormShieldArmorShiftForPlayer(player);

        // Armor type and shield influence "Agile Fighter" (Monk ability)
        EverQuest->RefreshAgileFighterCombatAuraForPlayer(player);
    }

    void OnPlayerUnequip(Player* player, Item* /*it*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        EverQuest->RefreshBearFormShieldArmorShiftForPlayer(player);
        EverQuest->RefreshAgileFighterCombatAuraForPlayer(player);
    }

    void OnPlayerUpdate(Player* player, uint32 p_time) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        EverQuest->RestoreInstanceValidityOutsideInstances(player);
        EverQuest->ProcessLevelCapStateForPlayer(player);
        EverQuest->UpdatePlayerIllusionGearDisplay(player, p_time);
        EverQuest->ConsumePendingTemporaryFactionRecalculation(player);
        EverQuest->UpdatePlayerTracking(player, p_time);
        EverQuest->UpdateAgileFighterCombatAura(player, p_time);
        if (EverQuest->ConfigSpellSummonPlayerAcrossZones == true)
            EverQuest->ConsumePendingSummonRequest(player);

        // Some ways a shield leaves the offhand have no unequip hook (auto-unequip when a two-hander goes on, item destruction),
        // so revalidate while the form that cares about it is held
        uint8 currentForm = player->GetShapeshiftForm();
        if (currentForm == FORM_BEAR || currentForm == FORM_DIREBEAR)
            EverQuest->RefreshBearFormShieldArmorShiftForPlayer(player);
    }

    void OnPlayerReputationRankChange(Player* player, uint32 factionID, ReputationRank /*newRank*/, ReputationRank /*oldRank*/, bool /*increased*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Earned reputation crossing a rank boundary can change temporary faction adjustments
        if (EverQuest->EQReputationFactionInfoByFactionID.find(factionID) != EverQuest->EQReputationFactionInfoByFactionID.end())
            EverQuest->QueueTemporaryFactionRecalculationForPlayer(player->GetGUID());
    }

    void OnPlayerUpdateZone(Player* player, uint32 /*newZone*/, uint32 /*newArea*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        EverQuest->RefreshIllusionGearDisplayForPlayer(player);
    }

    bool IsInGroupExperienceRangeOfKill(Player* member, Unit* victim)
    {
        if (victim == nullptr || member->IsInWorld() == false || victim->IsInWorld() == false)
            return false;
        if (member->GetMapId() != victim->GetMapId() || member->GetInstanceId() != victim->GetInstanceId())
            return false;

        // In an EverQuest zone the whole zone counts as being at the kill
        if (EverQuest->IsZoneWideGroupRewardEnabledForMap(victim->GetMapId()) == true)
            return true;

        return victim->GetDistance(member) <= sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE);
    }

    void OnPlayerRewardKillRewarder(Player* player, KillRewarder* rewarder, bool /*isDungeon*/, float& rate) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Handle zone-wide award support
        Unit* zoneWideVictim = rewarder->GetVictim();
        Player* zoneWideKiller = rewarder->GetKiller();
        bool zoneWideRateApplied = false;
        if (zoneWideKiller != nullptr && zoneWideKiller->GetGroup() != nullptr && EverQuest->IsZoneWideGroupRewardEnabledForMap(zoneWideKiller->GetMapId()) == true)
        {
            EverQuestZoneWideKillReward zoneWideReward;
            EverQuest->BuildZoneWideKillReward(zoneWideKiller->GetGroup(), zoneWideKiller, zoneWideVictim, zoneWideReward);
            if (zoneWideReward.IsValid == true)
            {
                // This already accounts for the alternate group formula when it is turned on
                rate = EverQuest->GetGroupExperienceRateForMember(player, zoneWideReward);
                zoneWideRateApplied = true;
                if (player == zoneWideKiller)
                    EverQuest->GrantZoneWideGroupRewardsForKill(zoneWideKiller, zoneWideVictim, zoneWideReward);
            }
        }

        // Disable any group exp reduction if needed. Kills that the zone wide share above already rated are left alone, so this covers Azeroth and any EverQuest kill that share did not apply to
        if (zoneWideRateApplied == false && EverQuest->ConfigAlternateGroupExperienceFormulaEnabled == true)
        {
            Group* group = player->GetGroup();
            if (group != nullptr)
            {
                // Only count members that are online, alive, and near the kill
                Unit* rewardVictim = rewarder->GetVictim();
                uint32 eligibleMemberCount = 0;
                for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
                {
                    Player* member = itr->GetSource();
                    if (member == nullptr || member->IsAlive() == false)
                        continue;
                    if (IsInGroupExperienceRangeOfKill(member, rewardVictim) == false)
                        continue;
                    eligibleMemberCount++;
                }

                if (eligibleMemberCount >= 2 && eligibleMemberCount <= 5)
                {
                    float bonusTotalRatePercent = static_cast<float>(eligibleMemberCount - 1) * (EverQuest->ConfigAlternateGroupExperienceAddPercentPerAddedMember * 0.01f);
                    float splitBaseRate = 1.0f / static_cast<float>(eligibleMemberCount);
                    rate = splitBaseRate * (1.0f + bonusTotalRatePercent);
                }
            }
        }

        // Kill credit for a non-EQ creature outside of an EQ zone permanently costs the player the adventurer aura
        if (EverQuest->IsCreatureKillDisqualifyingForAdventurer(player, rewarder->GetVictim()) == true)
        {
            if (EverQuest->DisqualifyPlayerFromAdventurer(player) == true && player->GetSession() != nullptr)
                ChatHandler(player->GetSession()).SendSysMessage("|cffFF0000You are no longer an Everquest Adventurer, as you gained kill credit for a creature that is not from Everquest.|r");
        }

        // Grab the kill rewards, and apply any in the list
        EverQuest->ApplyEQOnkillReputationsForPlayer(player, rewarder->GetVictim());
    }

    bool OnPlayerCanRepopAtGraveyard(Player* player) override
    {
        if (EverQuest->IsEnabled == false)
            return true;

        return EverQuest->HandleInstanceEvictionRepop(player);
    }

    void OnPlayerBeforeChooseGraveyard(Player* player, TeamId teamId, bool nearCorpse, uint32& graveyardOverride) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        EverQuest->EnforceGraveyardDomainForDeath(player, teamId, nearCorpse, graveyardOverride);
    }

    void OnPlayerSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        if (spell == nullptr)
            return;

        // Remember how much of the auto attack swings were left before the core resets them at the end of this same cast.  This has to happen ahead of the EverQuest spell ID range check below, since WoW spells can be configured for it too
        EverQuest->StashSwingTimersBeforeSpellCast(player, spell);

        if (spell->m_spellInfo->Id < EverQuest->ConfigSystemSpellDBCIDMin || spell->m_spellInfo->Id > EverQuest->ConfigSystemSpellDBCIDMax)
            return;

        for (uint8 effectIndex = EFFECT_0; effectIndex <= EFFECT_2; effectIndex++)
        {
            auto const& succorEffect = spell->m_spellInfo->Effects[effectIndex];
            if ((succorEffect.Effect == SPELL_EFFECT_DUMMY ||
                (succorEffect.Effect == SPELL_EFFECT_APPLY_AURA && succorEffect.ApplyAuraName == SPELL_AURA_DUMMY)) &&
                succorEffect.MiscValue == EQ_SPELLDUMMYTYPE_SUCCOR)
            {
                // Party target succor (evacuate) moves the group, where self is only the caster
                bool includeGroup = (succorEffect.TargetA.GetTarget() == TARGET_UNIT_CASTER_AREA_PARTY);
                EverQuest->SendPlayerToZoneSafePoint(player, includeGroup);
                return;
            }
        }

        // Spells with their own teleport effect (like the legacy stone) apply the gate tether by triggering the gate spell from a later effect slot.
        for (uint8 effectIndex = EFFECT_1; effectIndex <= EFFECT_2; effectIndex++)
        {
            auto const& triggerEffect = spell->m_spellInfo->Effects[effectIndex];
            if (triggerEffect.Effect != SPELL_EFFECT_TRIGGER_SPELL || triggerEffect.TriggerSpell == 0)
                continue;
            SpellInfo const* triggeredSpellInfo = sSpellMgr->GetSpellInfo(triggerEffect.TriggerSpell);
            if (triggeredSpellInfo == nullptr)
                continue;
            if (triggeredSpellInfo->Effects[EFFECT_0].Effect == SPELL_EFFECT_APPLY_AURA && triggeredSpellInfo->Effects[EFFECT_0].ApplyAuraName == SPELL_AURA_DUMMY
                && triggeredSpellInfo->Effects[EFFECT_0].MiscValue == EQ_SPELLDUMMYTYPE_GATE)
            {
                EverQuest->StorePositionAsLastGate(player);
                return;
            }
        }

        if (spell->m_spellInfo->Effects[EFFECT_0].Effect == SPELL_EFFECT_DUMMY ||
            (spell->m_spellInfo->Effects[EFFECT_0].Effect == SPELL_EFFECT_APPLY_AURA && spell->m_spellInfo->Effects[EFFECT_0].ApplyAuraName == SPELL_AURA_DUMMY))
        {
            if (spell->m_spellInfo->Effects[EFFECT_0].MiscValue == EQ_SPELLDUMMYTYPE_BINDSELF) // Bind Self
            {
                if (player->GetMapId() < EverQuest->ConfigSystemMapDBCIDMin || player->GetMapId() > EverQuest->ConfigSystemMapDBCIDMax)
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("The spell failed, as it only works in Norrath.");
                    return;
                }
                if (EverQuest->IsBindAllowedForMap(player->GetMapId()) == false)
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("Spell failed, as binding to this area is not allowed.");
                    return;
                }
                EverQuest->SetNewBindHome(player);
            }
            else if (spell->m_spellInfo->Effects[EFFECT_0].MiscValue == EQ_SPELLDUMMYTYPE_BINDANY) // Bind Any
            {
                if (player->GetMapId() < EverQuest->ConfigSystemMapDBCIDMin || player->GetMapId() > EverQuest->ConfigSystemMapDBCIDMax)
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("The spell failed, as it only works in Norrath.");
                    return;
                }
                if (EverQuest->IsBindAllowedForMap(player->GetMapId()) == false)
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("Spell failed, as binding to this area is not allowed.");
                    return;
                }
                ObjectGuid const target = player->GetTarget();
                if (target.IsPlayer())
                {
                    // The target can log out, die and release, or leave the map during the cast
                    Player* targetPlayer = ObjectAccessor::GetPlayer(player->GetMap(), target);
                    if (targetPlayer == nullptr)
                        ChatHandler(player->GetSession()).PSendSysMessage("The spell failed, as the target player could not be found.");
                    else if (targetPlayer->GetGUID().GetCounter() == player->GetGUID().GetCounter())
                        EverQuest->SetNewBindHome(player);
                    else
                        EverQuest->SetNewBindHome(targetPlayer);
                }
                else
                    ChatHandler(player->GetSession()).PSendSysMessage("The spell failed, as it requires a target player.");

            }
            else if (spell->m_spellInfo->Effects[EFFECT_0].MiscValue == EQ_SPELLDUMMYTYPE_GATE) // Gate
            {
                // Triggered casts are tether-aura-only applications (like from the legacy stone), so they should not gate the player home
                if (spell->IsTriggered() == false)
                {
                    EverQuest->StorePositionAsLastGate(player);
                    EverQuest->SendPlayerToEQBindHome(player);
                }
            }
            else if (spell->m_spellInfo->Effects[EFFECT_0].MiscValue == EQ_SPELLDUMMYTYPE_FORAGE) // Forage
            {
                EverQuest->ProcessForage(player);
            }
            else if (spell->m_spellInfo->Effects[EFFECT_0].MiscValue == EQ_SPELLDUMMYTYPE_TRACK) // Tracking
            {
                EverQuest->SendTrackingListToPlayer(player);
            }
            else if (spell->m_spellInfo->Effects[EFFECT_0].MiscValue == EQ_SPELLDUMMYTYPE_SUMMONPC) // Summon a player to the caster
            {
                EverQuest->ProcessSummonPlayerToCaster(player, spell->m_targets.GetUnitTarget());
            }
        }
    }

    void OnPlayerDelete(ObjectGuid guid, uint32 /*accountId*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        EverQuest->DeletePlayerBindHome(guid);
        EverQuest->PerformPlayerDelete(guid);
    }

    void OnPlayerCreate(Player* player) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        EverQuest->SetInitialCreatePositionForPlayer(player);
    }

    bool OnPlayerCheckItemInSlotAtLoadInventory(Player* player, Item* /*item*/, uint8 /*slot*/, uint8& /*err*/, uint16& /*dest*/) override
    {
        if (EverQuest->IsEnabled == false)
            return true;

        // Equipped items load before login autolearn, so without this some items can go away if you switch sub classes (for the first time) in some situations
        EverQuest->ApplyAutoLearnedClassSkillsAndSpells(player);
        return true;
    }

    void OnPlayerLogin(Player* player) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Start the grace timer for the client data version report (kicks stale clients that bypassed the update launcher)
        EverQuest->BeginClientVersionCheckForPlayer(player);

        // Pick up a character that logged out inside a raid instance
        EverQuest->UpdateRaidLowInstanceStateForPlayer(player);

        // Report which dungeon mode this character is on, and prime the client UI with it
        EverQuest->SendDungeonModeStateToPlayer(player, true);

        // Pull the account's auction realm filter into memory and prime the client UI with it
        EverQuest->LoadAuctionRealmFilterForPlayer(player);
        EverQuest->SendAuctionRealmFilterToPlayer(player);

        // Set the in game options page with what this character is actually set to
        EverQuest->SendPlayerOptionsToPlayer(player);

        // A character that logged out inside a private dungeon copy is still in it, and the map entry announcement could not be sent while loading
        EverQuest->SendInstanceDungeonEntryMessageToPlayer(player);

        // First login behavior
        if (player->HasAtLoginFlag(AT_LOGIN_FIRST) == true)
        {
            // Special logic for deathknights
            if (EverQuest->ConfigDeathKnightsStartLikeOtherClasses == true && player->getClass() == CLASS_DEATH_KNIGHT)
            {
                // If the DK doesn't learn DeathGate, teleport will fail
                player->learnSpell(50977);

                // Teach runeforging
                player->learnSpell(53428);
                if (player->GetSkillValue(776) == 0)
                {
                    player->SetSkill((uint16)776, 0, 1, 1);
                }
            }

            if (EverQuest->HasCreatePlayerData(player->getRace(), player->getClass()) == true)
            {
                EverQuestPlayerCreateInfo createInfo = EverQuest->GetPlayerCreateInfo(player->getRace(), player->getClass());
                EverQuest->SetNewBindHome(player, player->GetGUID().GetCounter(), createInfo.MapID, createInfo.ZoneID, createInfo.PositionX,
                    createInfo.PositionY, createInfo.PositionZ);
                if (EverQuest->ConfigDeathKnightsStartLikeOtherClasses == true && player->getClass() == CLASS_DEATH_KNIGHT)
                {
                    player->RemoveAura(48266); // Take off Blood Presence
                }
            }

            // Set EQ class
            EverQuest->SetInitialEQClassesForPlayer(player);

            // Give a hearthstone if configured to do so, since the EQ start items don't include one
            EverQuest->AddHearthstoneForNewCharacter(player);

            // Give start items for both of the EQ classes the character begins with
            EverQuest->GrantClassStartItemsForPlayer(player, EverQuest->GetClassMapForWOWClassID(player->getClass()).EQClassIDBase);
            EverQuest->GrantClassStartItemsForPlayer(player, EverQuest->GetCurrentSecondEQClassForPlayer(player));
        }

        // Give players the ability to see invis vs undead
        if (EverQuest->ConfigSystemInvisVsUndeadDetectSpellID != 0 && player->HasAura(EverQuest->ConfigSystemInvisVsUndeadDetectSpellID) == false)
            player->CastSpell(player, EverQuest->ConfigSystemInvisVsUndeadDetectSpellID, true);

        // Grant the legacy account feat of strength if the account is old enough
        EverQuest->GrantLegacyAchievementIfEligible(player);

        // Grant the adventurer feat of strength if another character on this account earned it
        EverQuest->GrantAdventurerAchievementIfAccountEarned(player);

        // Put the adventurer aura back if the character never did anything to lose it (it is granted here for new characters too)
        EverQuest->ApplyAdventurerAuraStateOnLogin(player);

        // Grab any cast bard songs for the player
        if (EverQuest->ConfigBardMaxConcurrentSongs != 0)
        {
            deque<uint32>* queue = nullptr;
            {
                std::lock_guard<std::mutex> lock(EverQuest->RuntimeStateMutex);
                queue = &EverQuest->PlayerCasterConcurrentBardSongs[player->GetGUID()];
            }
            queue->clear();
            for (auto const& itr : player->GetAppliedAuras())
            {
                AuraApplication const* aurApp = itr.second;
                uint32 spellID = aurApp->GetBase()->GetId();
                if (EverQuest->IsSpellAnEQBardSong(spellID) == true)
                    queue->push_back(spellID);
            }
        }

        // Autolearning is based on EQ classes (primary and secondary)
        EverQuest->ApplyAutoLearnedClassSkillsAndSpells(player);

        // Shamans need a master totem in order to cast their totem spells, so hand one out if they aren't carrying one
        EverQuest->AddMasterTotemForShaman(player);

        // A secondary class taken on for the first time has its start items waiting from the class switch, which ran at logout
        EverQuest->GrantPendingClassStartItemsForPlayer(player);

        // Hand out the character's racial guise item if they have never received one
        EverQuest->AddRacialGuiseItemForPlayer(player);

        // When a corpse and in an illusion that persists through death, the display won't survive logout so reapply when logging in dead but corpse isn't released
        if (player->IsAlive() == false && player->HasPlayerFlag(PLAYER_FLAGS_GHOST) == false)
            EverQuest->ApplyCorpseIllusionNativeDisplayOnDeath(player);

        // Grab EQ class info for the login summary message
        EverQuestClassMap classMap = EverQuest->GetClassMapForWOWClassID(player->getClass());
        uint8 secondClassID = EverQuest->GetCurrentSecondEQClassForPlayer(player);
        if (EverQuest->ConfigShowClassMessageOnLogin == true)
        {
            if (secondClassID != EQ_EQCLASS_NONE)
            {
                string text = fmt::format("Your EQ class primary is |cff4CFF00'{}'|r and secondary is |cff4CFF00'{}'|r.", GetEQClassStringFromID(classMap.EQClassIDBase), GetEQClassStringFromID(secondClassID));
                ChatHandler(player->GetSession()).SendSysMessage(text);
            }
            else
            {
                string text = fmt::format("Your EQ class primary is |cff4CFF00'{}'|r and you have no secondary EQ class.", GetEQClassStringFromID(classMap.EQClassIDBase));
                ChatHandler(player->GetSession()).SendSysMessage(text);
            }
        }

        // Seed the EQ Class character-pane tab with the player's class state
        EverQuest->SendClassInfoAddonMessageToPlayer(player);

        // Forced reactions don't survive logout
        EverQuest->RecalculateTemporaryFactionReactionsForPlayer(player);

        // A saved bear/dire bear form comes back with the character, and armor is rebuilt from scratch on login
        EverQuest->RefreshBearFormShieldArmorShiftForPlayer(player);

        // Check gear to see if the agile fighter buff should trigger a sub buff
        EverQuest->ReapplyAgileFighterCombatAuraForPlayer(player);
    }

    void OnPlayerLevelChanged(Player* player, uint8 /*oldlevel*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Grant any auto-learned class spells/skills that unlock at the player's new level
        EverQuest->ApplyAutoLearnedClassSkillsAndSpells(player);

        // Award the adventurer feat of strength if the required level is hit while the aura is still held
        EverQuest->ProcessAdventurerStateOnLevelChange(player);

        // Track range scales with level, so push the new range to any active tracking and the client addon
        EverQuest->HandleTrackingRangeChangeForPlayer(player);

        // Autolearn may have just granted the Agile Fighter passive that gates the combat auras
        EverQuest->RefreshAgileFighterCombatAuraForPlayer(player);
    }

    void OnPlayerResurrect(Player* player, float restore_percent, bool& applySickness) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        EverQuest->RestoreNativeDisplayAfterCorpseIllusion(player);

        // Only an accepted resurrection gives experience back, never walking to the corpse or taking the spirit healer
        if (restore_percent <= 0.0f && applySickness == false)
            EverQuest->RestoreDeathExpLossOnResurrectForPlayer(player);
    }

    void OnPlayerLogout(Player* player) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        EverQuest->ClearClientVersionCheckForPlayer(player->GetGUID());

        // Stop counting the character as being inside a raid instance
        EverQuest->ClearRaidLowInstanceStateForPlayer(player->GetGUID());

        // A gate tether cancelled on the very tick the character logged out has nothing left to teleport
        EverQuest->ClearPendingGateReturnForPlayer(player->GetGUID());

        // Don't leave a swapped native display behind (it is not saved, but the tracking entry must not linger)
        EverQuest->RestoreNativeDisplayAfterCorpseIllusion(player);

        if (EverQuest->ConfigBardMaxConcurrentSongs != 0)
        {
            std::lock_guard<std::mutex> lock(EverQuest->RuntimeStateMutex);
            EverQuest->PlayerCasterConcurrentBardSongs.erase(player->GetGUID());
        }

        // Apply any pending level cap experience park before the character saves
        if (EverQuest->ConfigPlayerLevelCap != 0)
            EverQuest->ProcessLevelCapStateForPlayer(player);

        // Stop tracking any illusion gear display state
        EverQuest->ClearIllusionTrackingForPlayer(player->GetGUID());

        // Stop tracking any bear form shield armor shift
        EverQuest->ClearBearFormShieldArmorShiftForPlayer(player->GetGUID());

        // Stop tracking the Agile Fighter combat aura refresh timer
        EverQuest->ClearAgileFighterTrackingForPlayer(player->GetGUID());

        // Drop any cross-zone summon that never got picked up, so it cannot fire on a later login
        EverQuest->ClearPendingSummonRequestForPlayer(player->GetGUID());

        // Take any Alliance-line faction bonus aura off its creature while the player is still on the map
        EverQuest->ClearTempFactionBonusForPlayer(player);

        // Stop tracking any temporary faction adjustment state
        EverQuest->ClearTemporaryFactionStateForPlayer(player->GetGUID());

        // Stop tracking the auction "Usable Items" filter state
        EverQuest->SetAuctionUsableFilterActiveForPlayer(player->GetGUID(), false);

        // Drop the cached auction realm filter, it is read back from the database on the next login
        EverQuest->ClearAuctionRealmFilterForPlayer(player);
        EverQuest->ClearAuctionSearchScanForPlayer(player);

        // Class switch
        if (EverQuest->GetCurrentSecondEQClassForPlayer(player) != EverQuest->GetNextSecondEQClassForPlayer(player))
        {
            if (!EverQuest->PerformClassSwitch(player))
            {
                LOG_ERROR("module.EverQuest", "EverQuestMod Could not successfully complete the class switch on logout for player {} with GUID {}", player->GetName(), player->GetGUID().GetCounter());
            }
        }
    }

    bool OnPlayerBeforeTeleport(Player* player, uint32 mapid, float x, float y, float z, float orientation, uint32 options, Unit* /*target*/) override
    {
        if (EverQuest->IsEnabled == false)
            return true;

        // Alliance-line faction bonuses do not survive leaving the zone, and the creature's aura is only reachable before the map switch
        if (player != nullptr && mapid != player->GetMapId())
            EverQuest->ClearTempFactionBonusForPlayer(player);

        // Block non-GMs from entering zones past the configured expansion. A player already standing in a restricted zone is
        // let through, so that the relocation below can always move them (even if their bind is restricted too)
        if (player != nullptr && player->IsGameMaster() == false && mapid != player->GetMapId())
        {
            if (EverQuest->IsMapRestrictedByExpansion(mapid) == true && EverQuest->IsMapRestrictedByExpansion(player->GetMapId()) == false)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("That land has not yet been discovered.");
                return false;
            }
        }

        // A zone that takes a key turns away anyone arriving without it, no matter how they got sent (raid coordinator, summon, zone line)
        if (player != nullptr && mapid != player->GetMapId() && EverQuest->DoesPlayerHaveRequiredKeyForMap(player, mapid) == false)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("You cannot enter {} without {}.", EverQuest->GetZoneNameForMap(mapid),
                EverQuest->GetRequiredKeyItemName(EverQuest->GetRequiredKeyItemIDForMap(mapid)));
            return false;
        }

        // A converted teleport into a zone that has a private copy is aimed at the open world version, so point it at the copy when the character belongs in one
        if (EverQuest->TryRerouteZoneTeleportIntoInstance(player, mapid, x, y, z, orientation, options) == true)
            return false;

        return true;
    }

    // Note: this is AFTER the player changes maps
    void OnPlayerMapChanged(Player* player) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Catch map switches that bypass TeleportTo
        EverQuest->ClearTempFactionBonusForPlayer(player);

        // Track entering and leaving raid instances, which drives whether a zone line back in should return the player to theirs
        EverQuest->UpdateRaidLowInstanceStateForPlayer(player);

        // Announce arriving in a private dungeon copy
        if (player->GetSession() != nullptr && player->GetSession()->PlayerLoading() == false)
            EverQuest->SendInstanceDungeonEntryMessageToPlayer(player);

        // Restrict non-GMs to norrath if set
        if (EverQuest->ConfigMapRestrictPlayersToNorrath == true && player->IsGameMaster() == false)
        {
            if (player->GetMap() != nullptr && (player->GetMap()->GetId() < EverQuest->ConfigSystemMapDBCIDMin || player->GetMap()->GetId() > EverQuest->ConfigSystemMapDBCIDMax))
            {
                // Relocation falls back to the EverQuest start position when the character never bound
                if (EverQuest->RelocatePlayerOutOfRestrictedMap(player) == true)
                    ChatHandler(player->GetSession()).PSendSysMessage("You are not permitted to step into Azeroth.");
                return;
            }
        }

        // Catch non-GMs sitting in a zone past the configured expansion (log in, unblockable summons, config changes)
        if (player->IsGameMaster() == false && player->GetMap() != nullptr)
        {
            if (EverQuest->IsMapRestrictedByExpansion(player->GetMap()->GetId()) == true)
            {
                if (EverQuest->RelocatePlayerOutOfRestrictedMap(player) == true)
                    ChatHandler(player->GetSession()).PSendSysMessage("That land has not yet been discovered.");
            }
        }
    }

    void OnPlayerJustDied(Player* player) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Alliance-line faction bonuses do not survive the caster's death, mirroring EQ death
        EverQuest->ClearTempFactionBonusForPlayer(player);

        // If a cosmetic illusion is active, swap the native display so the corpse created on release shows the illusion
        EverQuest->ApplyCorpseIllusionNativeDisplayOnDeath(player);
    }

    void OnPlayerReleasedGhost(Player* player) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // The corpse now exists and copied the swapped native display, so put the real native display back
        EverQuest->RestoreNativeDisplayAfterCorpseIllusion(player);

        // Releasing the spirit is what costs experience, so a resurrection accepted before this point costs nothing
        EverQuest->ApplyExpLossForSpiritReleaseForPlayer(player);
    }

    // This is done to ensure repeatable quests give EXP more than once
    void OnPlayerQuestComputeXP(Player* player, Quest const* quest, uint32& xpValue) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        if (EverQuest->ConfigQuestGrantExpOnRepeatCompletion == false)
            return;

        if (quest->GetQuestId() >= EverQuest->ConfigSystemQuestSQLIDMin && quest->GetQuestId() <= EverQuest->ConfigSystemQuestSQLIDMax)
            xpValue = player->CalculateQuestRewardXP(quest);
    }

    void OnPlayerLootItem(Player* player, Item* item, uint32 /*count*/, ObjectGuid lootguid) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        EverQuest->RemoveVisualEquippedItemForCreatureGUIDIfExists(player->GetMap(), lootguid, item->GetTemplate()->ItemId);
    }

    void OnPlayerBeforeLootMoney(Player* player, Loot* loot) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        EverQuest->ApplyZoneWideGroupMoneyShare(player, loot);
    }

    void OnPlayerBeforeLogout(Player* player) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Any pending equipment storage commit must have executed before the logout save and class switch are queued
        EverQuest->WaitForPendingEquipmentStorageCommitForPlayer(player->GetGUID());

        // If a class change is in progress, update the item visuals
        if (EverQuest->GetCurrentSecondEQClassForPlayer(player) != EverQuest->GetNextSecondEQClassForPlayer(player))
        {
            map<uint8, EverQuestPlayerEquipedItemData> visibleItemsBySlot = EverQuest->GetVisibleItemsBySlotForPlayerClass(player, EverQuest->GetNextSecondEQClassForPlayer(player));
            for (uint8 i = 0; i < 18; ++i)
            {
                if (visibleItemsBySlot[i].ItemID == 0)
                    player->SetVisibleItemSlot(i, NULL);
                else
                {
                    player->SetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + (i * 2), visibleItemsBySlot[i].ItemID);
                    player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (i * 2), 0, visibleItemsBySlot[i].PermEnchant);
                    player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (i * 2), 1, visibleItemsBySlot[i].TempEnchant);
                }
            }
        }
    }
};

void AddEverQuestPlayerScripts()
{
    new EverQuest_PlayerScript();
}
