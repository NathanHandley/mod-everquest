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
#include "Pet.h"
#include "Player.h"
#include "ReputationMgr.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "QuestDef.h"

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
                    EverQuest->DespawnCreature(questReaction.CreatureTemplateID, map);
                } break;
                case EQ_QUEST_REACTION_SPAWN:
                {
                    EverQuest->SpawnCreature(questReaction.CreatureTemplateID, map, x, y, z, orientation, false);
                } break;
                case EQ_QUEST_REACTION_SPAWNUNIQUE:
                {
                    EverQuest->SpawnCreature(questReaction.CreatureTemplateID, map, x, y, z, orientation, true);
                } break;
                default: break; // Nothing
                }
            }
        }
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
        else if (spell->m_spellInfo->Id < EverQuest->ConfigSystemSpellDBCIDMin || spell->m_spellInfo->Id > EverQuest->ConfigSystemSpellDBCIDMax)
            return;
        else if (spell->m_spellInfo->Effects[EFFECT_0].Effect == SPELL_EFFECT_DUMMY ||
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
                    Player* targetPlayer = ObjectAccessor::GetPlayer(player->GetMap(), target);
                    if (targetPlayer->GetGUID().GetCounter() == player->GetGUID().GetCounter())
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

    void OnPlayerLogin(Player* player) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // First login behavior
        bool firstLoginTeleported = false;
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

            // If there is create data, move the player to the related zone and set initial bind
            if (EverQuest->HasCreatePlayerData(player->getRace(), player->getClass()) == true)
            {
                EverQuestPlayerCreateInfo createInfo = EverQuest->GetPlayerCreateInfo(player->getRace(), player->getClass());
                player->TeleportTo(createInfo.MapID, createInfo.PositionX, createInfo.PositionY, createInfo.PositionZ, createInfo.Orientation);
                EverQuest->SetNewBindHome(player, player->GetGUID().GetCounter(), createInfo.MapID, createInfo.ZoneID, createInfo.PositionX,
                    createInfo.PositionY, createInfo.PositionZ);
                firstLoginTeleported = true;
                if (EverQuest->ConfigDeathKnightsStartLikeOtherClasses == true && player->getClass() == CLASS_DEATH_KNIGHT)
                {
                    player->RemoveAura(48266); // Take off Blood Presence
                }
            }

            // Set EQ class
            EverQuest->SetInitialEQClassesForPlayer(player);
        }

        EverQuest->AllLoadedPlayers.push_back(player);

        // Give players the ability to see invis vs undead
        if (EverQuest->ConfigSystemInvisVsUndeadDetectSpellID != 0 && player->HasAura(EverQuest->ConfigSystemInvisVsUndeadDetectSpellID) == false)
            player->CastSpell(player, EverQuest->ConfigSystemInvisVsUndeadDetectSpellID, true);

        // Grab any cast bard songs for the player
        if (EverQuest->ConfigBardMaxConcurrentSongs != 0)
        {
            auto& queue = EverQuest->PlayerCasterConcurrentBardSongs[player->GetGUID()];
            queue.clear();
            for (auto const& itr : player->GetAppliedAuras())
            {
                AuraApplication const* aurApp = itr.second;
                uint32 spellID = aurApp->GetBase()->GetId();
                if (EverQuest->IsSpellAnEQBardSong(spellID) == true)
                    queue.push_back(spellID);
            }
        }

        // Autolearning is based on EQ classes (primary and secondary)
        // Note: First login teleport messes with this a bit, so there need sto be a delay in that case
        if (firstLoginTeleported == true)
            EverQuest->PlayersPendingFirstLoginAutoLearn.insert(player->GetGUID());
        else
            EverQuest->ApplyAutoLearnedClassSkillsAndSpells(player);

        // Grab EQ class info for the login summary message
        EverQuestClassMap classMap = EverQuest->GetClassMapForWOWClassID(player->getClass());
        uint8 secondClassID = EverQuest->GetCurrentSecondEQClassForPlayer(player);
        if (secondClassID != EQ_EQCLASS_NONE)
        {
            string text = fmt::format("Your EQ class primary is |cff4CFF00'{}'|r and secondary is |cff4CFF00'{}'|r. Type |cff4CFF00.class |rto change or edit your secondary EQ class.", GetEQClassStringFromID(classMap.EQClassIDBase), GetEQClassStringFromID(secondClassID));
            ChatHandler(player->GetSession()).SendSysMessage(text);
        }
        else
        {
            string text = fmt::format("Your EQ class primary is |cff4CFF00'{}'|r and you have no secondary EQ class. Type |cff4CFF00.class |rto change or edit your secondary EQ class.", GetEQClassStringFromID(classMap.EQClassIDBase));
            ChatHandler(player->GetSession()).SendSysMessage(text);
        }

        // Seed the EQ Class character-pane tab with the player's class state
        EverQuest->SendClassInfoAddonMessageToPlayer(player);
    }

    void OnPlayerLogout(Player* player) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        EverQuest->AllLoadedPlayers.erase(std::remove(EverQuest->AllLoadedPlayers.begin(), EverQuest->AllLoadedPlayers.end(), player), EverQuest->AllLoadedPlayers.end());
        if (EverQuest->ConfigBardMaxConcurrentSongs != 0)
            EverQuest->PlayerCasterConcurrentBardSongs[player->GetGUID()].clear();
        EverQuest->PlayersPendingFirstLoginAutoLearn.erase(player->GetGUID());

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

        // Handle any delayed first-login skill/spell adds
        auto pendingAutoLearnItr = EverQuest->PlayersPendingFirstLoginAutoLearn.find(player->GetGUID());
        if (pendingAutoLearnItr != EverQuest->PlayersPendingFirstLoginAutoLearn.end())
        {
            EverQuest->PlayersPendingFirstLoginAutoLearn.erase(pendingAutoLearnItr);
            EverQuest->ApplyAutoLearnedClassSkillsAndSpells(player);
        }

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

            // Calculate how much experience to lose
            int nextLevelTotalEXP = player->GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
            int expToLose = (int)((float)nextLevelTotalEXP * (0.01 * EverQuest->ConfigExpLossOnDeathLossPercent));

            // Reduce experience
            int curLevelEXP = player->GetUInt32Value(PLAYER_XP);
            if (curLevelEXP > expToLose)
            {
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
                player->SetRestBonus(player->GetRestBonus() + expToLose);
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
