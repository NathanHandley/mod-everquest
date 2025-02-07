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

#include "Player.h"
#include "ScriptMgr.h"
#include "ReputationMgr.h"

#include "EverQuest.h"

#include <list>
#include <map>

using namespace std;



class EverQuest_PlayerScript : public PlayerScript
{
public:
    EverQuest_PlayerScript() : PlayerScript("EverQuest_PlayerScript") {}

    void OnRewardKillRewarder(Player* player, KillRewarder* rewarder, bool /*isDungeon*/, float& /*rate*/)
    {
        // Skip invalid victims
        Unit* victim = rewarder->GetVictim();
        if (!victim || victim->IsPlayer() || victim->ToCreature()->IsReputationGainDisabled())
            return;
        Creature* victimCreature = victim->ToCreature();

        LOG_INFO("server.loading", "1");

        // Grab the kill rewards, and apply any in the list
        list<CreatureOnkillReputation> onkillReputations = EverQuest->GetOnkillReputationsForCreatureTemplate(victimCreature->GetCreatureTemplate()->Entry);
        for (auto& onkillReputation : onkillReputations)
        {
            LOG_INFO("server.loading", "2 FactionID:{}", onkillReputation.FactionID);
            float repChange = player->CalculateReputationGain(REPUTATION_SOURCE_KILL, victim->GetLevel(), static_cast<float>(onkillReputation.KillRewardValue), onkillReputation.FactionID);

            FactionEntry const* factionEntry = sFactionStore.LookupEntry(onkillReputation.FactionID);
            if (factionEntry && repChange != 0)
            {
                LOG_INFO("server.loading", "3 repChange:{}", repChange);
                player->GetReputationMgr().ModifyReputation(factionEntry, repChange, false, static_cast<ReputationRank>(7));
            }
        }
    }
};

void AddEverQuestPlayerScripts()
{
    new EverQuest_PlayerScript();
}
