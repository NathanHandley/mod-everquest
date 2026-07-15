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
#include "GameGraveyard.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "Pet.h"
#include "Player.h"
#include "ReputationMgr.h"
#include "ScriptMgr.h"
#include "Spell.h"
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

        // TODO: This needs to be dynamic from the server in case SK is mapped to a different class
        if (context == CLASS_CONTEXT_PET && playerClass == CLASS_WARLOCK && player->getClass() == CLASS_DEATH_KNIGHT)
            return true;

        return std::nullopt;
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

    void OnPlayerBeforeLoadPetFromDB(Player* player, uint32& petEntry, uint32& petNumber, bool& current, bool& forceLoadFromDB) override
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
            for (const EverQuestQuestReaction& questReaction : questReactions)
            {
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
                        EverQuest->EnqueuePendingKillSpawnAction(map->GetId(), action);
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
                        EverQuest->EnqueuePendingKillSpawnAction(map->GetId(), action);
                    }
                    else
                        EverQuest->SpawnCreature(questReaction.CreatureTemplateID, map, x, y, z, orientation, questReaction.ReactionType == EQ_QUEST_REACTION_SPAWNUNIQUE);
                } break;
                case EQ_QUEST_REACTION_KILLSPAWN:
                {
                    EverQuest->TriggerQuestKillSpawn(map->GetId(), questReaction);
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
    }

    void OnPlayerUpdate(Player* player, uint32 p_time) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        EverQuest->ProcessLevelCapStateForPlayer(player);
        EverQuest->UpdatePlayerIllusionGearDisplay(player, p_time);
    }

    void OnPlayerRewardKillRewarder(Player* player, KillRewarder* rewarder, bool /*isDungeon*/, float& rate) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Disable any group exp reduction if needed
        if (EverQuest->ConfigAlternateGroupExperienceFormulaEnabled == true)
        {
            Group* group = player->GetGroup();
            if (group != nullptr)
            {
                // Only count members that are online, alive, and in reward range 
                Player* killer = rewarder->GetKiller();
                Unit* rewardVictim = rewarder->GetVictim();
                uint32 eligibleMemberCount = 0;
                for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
                {
                    Player* member = itr->GetSource();
                    if (member == nullptr || member->IsAlive() == false)
                        continue;
                    if (member != killer && (rewardVictim == nullptr || member->IsAtGroupRewardDistance(rewardVictim) == false))
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

        // Skip invalid victims
        Unit* victim = rewarder->GetVictim();
        if (!victim || victim->IsPlayer() || victim->ToCreature()->IsReputationRewardDisabled())
            return;
        Creature* victimCreature = victim->ToCreature();

        // Grab the kill rewards, and apply any in the list
        const list<EverQuestCreatureOnkillReputation>& onkillReputations = EverQuest->GetOnkillReputationsForCreatureTemplate(victimCreature->GetCreatureTemplate()->Entry);
        for (const auto& onkillReputation : onkillReputations)
        {
            float repChange = player->CalculateReputationGain(REPUTATION_SOURCE_KILL, victim->GetLevel(), static_cast<float>(onkillReputation.KillRewardValue), onkillReputation.FactionID);

            FactionEntry const* factionEntry = sFactionStore.LookupEntry(onkillReputation.FactionID);
            if (factionEntry && repChange != 0)
            {
                player->GetReputationMgr().ModifyReputation(factionEntry, repChange, false, static_cast<ReputationRank>(7));
            }
        }
    }

    void OnPlayerBeforeChooseGraveyard(Player* player, TeamId teamId, bool nearCorpse, uint32& graveyardOverride) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Skip if there this isn't an EQ zone
        if (player->GetMapId() < EverQuest->ConfigSystemMapDBCIDMin || player->GetMapId() > EverQuest->ConfigSystemMapDBCIDMax)
            return;

        // If the player's corpse is in a different zone than the player, then use the player zone (by setting nearcorpse to false)
        if (nearCorpse == true && player->HasCorpse())
        {
            WorldLocation playerLocation = player->GetWorldLocation();
            WorldLocation playerCorpseLocation = player->GetCorpseLocation();
            if (playerLocation.GetMapId() != playerCorpseLocation.GetMapId())
            {
                GraveyardStruct const* ClosestGrave = sGraveyard->GetClosestGraveyard(player, teamId, false);
                if (ClosestGrave != nullptr)
                {
                    graveyardOverride = ClosestGrave->ID;
                    return;
                }
            }
        }
    }

    void OnPlayerSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        if (spell == nullptr)
            return;
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

        // Stone gate tether auras can sit in any effect slot, as the spell's own teleport effect comes first
        for (uint8 effectIndex = EFFECT_0; effectIndex <= EFFECT_2; effectIndex++)
        {
            auto const& stoneGateEffect = spell->m_spellInfo->Effects[effectIndex];
            if ((stoneGateEffect.Effect == SPELL_EFFECT_DUMMY || (stoneGateEffect.Effect == SPELL_EFFECT_APPLY_AURA && stoneGateEffect.ApplyAuraName == SPELL_AURA_DUMMY))
                && stoneGateEffect.MiscValue == EQ_SPELLDUMMYTYPE_STONEGATE)
            {
                // Store where the player cast from, since the spell's teleport effect will move them after this fires
                EverQuest->StorePositionAsLastStoneGate(player);
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
                EverQuest->SetNewBindHome(player);
            }
            else if (spell->m_spellInfo->Effects[EFFECT_0].MiscValue == EQ_SPELLDUMMYTYPE_BINDANY) // Bind Any
            {
                if (player->GetMapId() < EverQuest->ConfigSystemMapDBCIDMin || player->GetMapId() > EverQuest->ConfigSystemMapDBCIDMax)
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("The spell failed, as it only works in Norrath.");
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
                EverQuest->StorePositionAsLastGate(player);
                EverQuest->SendPlayerToEQBindHome(player);
            }
            else if (spell->m_spellInfo->Effects[EFFECT_0].MiscValue == EQ_SPELLDUMMYTYPE_FORAGE) // Forage
            {
                EverQuest->ProcessForage(player);
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
        }

        // Give players the ability to see invis vs undead
        if (EverQuest->ConfigSystemInvisVsUndeadDetectSpellID != 0 && player->HasAura(EverQuest->ConfigSystemInvisVsUndeadDetectSpellID) == false)
            player->CastSpell(player, EverQuest->ConfigSystemInvisVsUndeadDetectSpellID, true);

        // Grant the legacy account feat of strength if the account is old enough
        EverQuest->GrantLegacyAchievementIfEligible(player);

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

        // Auto-add class items based on EQ classes (primary and secondary)
        EverQuest->ApplyAutoAddedClassItems(player);

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
    }

    void OnPlayerLevelChanged(Player* player, uint8 /*oldlevel*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Grant any auto-learned class spells/skills that unlock at the player's new level
        EverQuest->ApplyAutoLearnedClassSkillsAndSpells(player);
    }

    void OnPlayerLogout(Player* player) override
    {
        if (EverQuest->IsEnabled == false)
            return;

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

        // Class switch
        if (EverQuest->GetCurrentSecondEQClassForPlayer(player) != EverQuest->GetNextSecondEQClassForPlayer(player))
        {
            if (!EverQuest->PerformClassSwitch(player))
            {
                LOG_ERROR("module.EverQuest", "EverQuestMod Could not successfully complete the class switch on logout for player {} with GUID {}", player->GetName(), player->GetGUID().GetCounter());
            }
        }
    }

    // Note: this is AFTER the player changes maps
    void OnPlayerMapChanged(Player* player) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Restrict non-GMs to norrath if set
        if (EverQuest->ConfigMapRestrictPlayersToNorrath == true && player->IsGameMaster() == false)
        {
            if (player->GetMap() != nullptr && (player->GetMap()->GetId() < EverQuest->ConfigSystemMapDBCIDMin || player->GetMap()->GetId() > EverQuest->ConfigSystemMapDBCIDMax))
            {
                EverQuest->SendPlayerToEQBindHome(player);
                ChatHandler(player->GetSession()).PSendSysMessage("You are not permitted to step into Azeroth.");
            }
        }
    }

    void OnPlayerReleasedGhost(Player* player) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        if (EverQuest->ConfigExpLossOnDeathEnabled == true)
        {
            // Do nothing if the level is below the minimum
            uint8 playerLevel = player->GetLevel();
            if (playerLevel < EverQuest->ConfigExpLossOnDeathMinLevel)
                return;

            // Also do nothing if this isn't a Norrath map and configured to skip
            if (player->GetMap()->GetId() < EverQuest->ConfigSystemMapDBCIDMin || player->GetMap()->GetId() > EverQuest->ConfigSystemMapDBCIDMax)
                return;

            // Determine the XP span of the current level
            uint32 levelXPSpan = player->GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
            if (levelXPSpan == 0 && playerLevel > 1)
                levelXPSpan = sObjectMgr->GetXPForLevel(playerLevel - 1);

            // Calculate how much experience to lose
            int expToLose = (int)((float)levelXPSpan * (0.01 * EverQuest->ConfigExpLossOnDeathLossPercent));

            int curLevelEXP = player->GetUInt32Value(PLAYER_XP);
            int expLost = expToLose;

            if (playerLevel >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
            {
                uint8 newLevel = playerLevel - 1;
                uint32 belowLevelSpan = sObjectMgr->GetXPForLevel(newLevel);
                int newExperience = (belowLevelSpan > 1 ? (int)belowLevelSpan - 1 : 0) - expToLose;
                if (newExperience < 0)
                    newExperience = 0;
                player->SetLevel(newLevel, true);
                player->SetUInt32Value(PLAYER_NEXT_LEVEL_XP, belowLevelSpan);
                player->SetUInt32Value(PLAYER_XP, (uint32)newExperience);
                ChatHandler(player->GetSession()).PSendSysMessage("You lost|cffFF0000 {} |rexperience for releasing your spirit, which dropped your level to |cffFF0000{}|r!", expToLose, newLevel);
            }
            else if (curLevelEXP > expToLose)
            {
                // Reduce experience within the current level
                player->SetUInt32Value(PLAYER_XP, curLevelEXP - expToLose);
                ChatHandler(player->GetSession()).PSendSysMessage("You lost|cffFF0000 {} |rexperience for releasing your spirit!", expToLose);
            }
            else
            {
                // Underflow, so drop level if above level 1
                if (playerLevel == 1)
                {
                    player->SetUInt32Value(PLAYER_XP, 0);
                    ChatHandler(player->GetSession()).PSendSysMessage("You lost what little experience you had for releasing your spirit!");
                }
                else
                {
                    player->SetLevel(playerLevel - 1, true);
                    int newExperience = player->GetUInt32Value(PLAYER_NEXT_LEVEL_XP) - (expToLose - curLevelEXP);
                    player->SetUInt32Value(PLAYER_XP, newExperience);
                    ChatHandler(player->GetSession()).PSendSysMessage("You lost|cffFF0000 {} |rexperience for releasing your spirit, which dropped your level to |cffFF0000{}|r!", expToLose, playerLevel - 1);
                }
            }

            // If set, give it back as rest exp
            if (EverQuest->ConfigExpLossOnDeathAddLostExpToRestExp == true)
                player->SetRestBonus(player->GetRestBonus() + expLost);
        }
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

    void OnPlayerBeforeLogout(Player* player) override
    {
        if (EverQuest->IsEnabled == false)
            return;

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
