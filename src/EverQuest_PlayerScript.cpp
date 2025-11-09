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
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ReputationMgr.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "QuestDef.h"

#include "EverQuest.h"

#include <list>
#include <map>

using namespace std;

class EverQuest_PlayerScript : public PlayerScript
{
public:
    EverQuest_PlayerScript() : PlayerScript("EverQuest_PlayerScript") {}

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
            for (const EverQuestQuestReaction questReaction : questReactions)
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

    void OnPlayerRewardKillRewarder(Player* player, KillRewarder* rewarder, bool /*isDungeon*/, float& /*rate*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;

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
        }
    }

    void OnPlayerDelete(ObjectGuid guid, uint32 /*accountId*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        EverQuest->DeletePlayerBindHome(guid);
    }

    void OnPlayerFirstLogin(Player* player) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // If the player logged in for the first time and is in a norrath zone, set the bind and aura
        if (player->GetMap() != nullptr && player->GetMap()->GetId() >= EverQuest->ConfigSystemMapDBCIDMin && player->GetMap()->GetId() <= EverQuest->ConfigSystemMapDBCIDMax)
        {
            EverQuest->SetNewBindHome(player);
            EverQuest->SetPlayerDayOrNightAura(player);
        }
    }

    void OnPlayerLogin(Player* player) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        EverQuest->AllLoadedPlayers.push_back(player);
        EverQuest->SetPlayerDayOrNightAura(player);

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
    }

    void OnPlayerLogout(Player* player) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        EverQuest->AllLoadedPlayers.erase(std::remove(EverQuest->AllLoadedPlayers.begin(), EverQuest->AllLoadedPlayers.end(), player), EverQuest->AllLoadedPlayers.end());
        if (EverQuest->ConfigBardMaxConcurrentSongs != 0)
            EverQuest->PlayerCasterConcurrentBardSongs[player->GetGUID()].clear();
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

        // Set aura if not set
        EverQuest->SetPlayerDayOrNightAura(player);
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
};

void AddEverQuestPlayerScripts()
{
    new EverQuest_PlayerScript();
}
