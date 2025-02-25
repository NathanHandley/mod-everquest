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

#include "EverQuest.h"

#include <list>
#include <map>

using namespace std;

class EverQuest_PlayerScript : public PlayerScript
{
public:
    EverQuest_PlayerScript() : PlayerScript("EverQuest_PlayerScript") {}

    void OnPlayerRewardKillRewarder(Player* player, KillRewarder* rewarder, bool /*isDungeon*/, float& /*rate*/) override
    {
        // Skip invalid victims
        Unit* victim = rewarder->GetVictim();
        if (!victim || victim->IsPlayer() || victim->ToCreature()->IsReputationRewardDisabled())
            return;
        Creature* victimCreature = victim->ToCreature();

        // Grab the kill rewards, and apply any in the list
        list<CreatureOnkillReputation> onkillReputations = EverQuest->GetOnkillReputationsForCreatureTemplate(victimCreature->GetCreatureTemplate()->Entry);
        for (auto& onkillReputation : onkillReputations)
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
        // Skip if there this isn't an EQ zone
        if (player->GetMapId() < CONFIG_SPELLS_BIND_MIN_MAP_ID || player->GetMapId() > CONFIG_SPELLS_BIND_MAX_MAP_ID)
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

    void OnPlayerSpellCast(Player* player, Spell* spell, bool skipCheck) override
    {
        if (spell == nullptr)
            return;
        else if (spell->m_spellInfo->Id == CONFIG_SPELLS_GATE_SPELLDBC_ID)
            EverQuest->SendPlayerToEQBindHome(player);
        else if (spell->m_spellInfo->Id == CONFIG_SPELLS_BINDSELF_SPELLDBC_ID || spell->m_spellInfo->Id == CONFIG_SPELLS_BINDANY_SPELLDBC_ID)
        {
            // Make sure it only works in EverQuest zones
            if (player->GetMapId() < CONFIG_SPELLS_BIND_MIN_MAP_ID || player->GetMapId() > CONFIG_SPELLS_BIND_MAX_MAP_ID)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("The spell failed, as it only works in Norrath.");
                return;
            }

            // Use the target if it's the any version
            if (spell->m_spellInfo->Id == CONFIG_SPELLS_BINDANY_SPELLDBC_ID)
            {
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
            else if (spell->m_spellInfo->Id == CONFIG_SPELLS_BINDSELF_SPELLDBC_ID)
                EverQuest->SetNewBindHome(player);
        }
    }
};

void AddEverQuestPlayerScripts()
{
    new EverQuest_PlayerScript();
}
