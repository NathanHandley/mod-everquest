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

        // Handle quest-specific events
        Map* map = player->GetMap();
        float x = player->GetPositionX();
        float y = player->GetPositionY();
        float z = player->GetPositionZ();
        float orientation = player->GetOrientation();
        switch (quest->GetQuestId())
        {
        case 30000: // airplane - #Master_of_Elements
        case 35000: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50840, map);
        }break;
        case 30007: // airplane - a_presence
        case 35007: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50866, map);
            EverQuest->SpawnCreature(50869, map, x, y, -2.32, orientation, false);
        }break;
        case 30008: // airplane - a_thunder_spirit_princess
        case 35008: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(50873, map, 192.12, 83.49, -15.69, 2.6826255, true);
        }break;
        case 30010: // airplane - Cilin_Spellsinger
        case 35010: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50842, map);
            EverQuest->SpawnCreature(50880, map, 402.78, 191.6, -222.4, 4.727115, false);
        }break;
        case 30011: // airplane - Cilin_Spellsinger
        case 35011: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50842, map);
            EverQuest->SpawnCreature(50881, map, 396.84, 191.6, -222.4, 4.727115, false);
        }break;
        case 30016: // airplane - Dason_Goldblade
        case 35016: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50837, map);
            EverQuest->SpawnCreature(50888, map, 386.02, 163.27, -222.4, 1.5560701, false);
        }break;
        case 30017: // airplane - Dason_Goldblade
        case 35017: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50837, map);
            EverQuest->SpawnCreature(50877, map, 386.02, 163.27, -222.4, 1.5560701, false);
        }break;
        case 30024: // airplane - Dirkog_Steelhand
        case 35024: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50877, map);
            EverQuest->SpawnCreature(50890, map, 222.43, -169.94, 51.04, 1.5707964, false);
        }break;
        case 30027: // airplane - Drakis_Bloodcaster
        case 35027: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50853, map);
            EverQuest->SpawnCreature(50874, map, 378.45, 189.92, -221.04, 0.5399612, false);
        }break;
        case 30028: // airplane - Drakis_Bloodcaster
        case 35028: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50853, map);
            EverQuest->SpawnCreature(50883, map, 378.45, 191.86, -221.04, 0.5399612, false);
        }break;
        case 30033: // airplane - Enchanter_Jolas
        case 35033: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50849, map);
            EverQuest->SpawnCreature(50884, map, 377.93, 185.6, -222.4, 0.009817477, false);
        }break;
        case 30034: // airplane - Enchanter_Jolas
        case 35034: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50849, map);
            EverQuest->SpawnCreature(50903, map, 377.93, 185.6, -222.4, 0.009817477, false);
        }break;
        case 30050: // airplane - Gkzzallk
        case 35050: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50873, map);
        }break;
        case 30061: // airplane - Holwin
        case 35061: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50846, map);
            EverQuest->SpawnCreature(50896, map, 386.37, 191.4, -222.4, 4.660847, false);
        }break;
        case 30062: // airplane - Holwin
        case 35062: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50846, map);
            EverQuest->SpawnCreature(50895, map, 381.35, 191.4, -222.4, 4.660847, false);
        }break;
        case 30065: // airplane - Inte_Akera
        case 35065: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50890, map);
        }break;
        case 30067: // airplane - Josin_Faithbringer
        case 35067: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50852, map);
            EverQuest->SpawnCreature(50879, map, 394.86, 191.6, -222.4, 4.7418413, false);
        }break;
        case 30068: // airplane - Josin_Faithbringer
        case 35068: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50852, map);
            EverQuest->SpawnCreature(50882, map, 388.02, 191.6, -222.4, 4.7418413, false);
        }break;
        case 30075: // airplane - Kihun_Solstin
        case 35075: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(50840, map, 390.43, 175.3, -222.14, 0, true);
        }break;
        case 30080: // airplane - Magus_Frinon
        case 35080: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50850, map);
            EverQuest->SpawnCreature(50887, map, 378.19, 178.2, -222.4, 6.2586417, false);
        }break;
        case 30081: // airplane - Magus_Frinon
        case 35081: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50850, map);
            EverQuest->SpawnCreature(50893, map, 378.19, 178.2, -222.4, 6.2586417, false);
        }break;
        case 30083: // airplane - Medicine_Man_Veetra
        case 35083: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50844, map);
            EverQuest->SpawnCreature(50886, map, 406.55, 183.25, -222.4, 3.1857712, true);
        }break;
        case 30084: // airplane - Medicine_Man_Veetra
        case 35084: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50844, map);
            EverQuest->SpawnCreature(50892, map, 405.71, 189.49, -222.4, 3.1857712, true);
        }break;
        case 30095: // airplane - Ranger_Spirit
        case 35095: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50854, map);
            EverQuest->SpawnCreature(50894, map, 405.86, 178.96, -222.4, 3.1759536, false);
        }break;
        case 30096: // airplane - Ranger_Spirit
        case 35096: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50854, map);
            EverQuest->SpawnCreature(50889, map, 406.03, 171.91, -222.4, 3.1759536, false);
        }break;
        case 30108: // airplane - Sarkis_Ebonblade
        case 35108: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50848, map);
            EverQuest->SpawnCreature(50863, map, 392.05, 163.36, -222.4, 1.5560701, false);
        }break;
        case 30109: // airplane - Sarkis_Ebonblade
        case 35109: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50848, map);
            EverQuest->SpawnCreature(50897, map, 392.05, 163.36, -222.4, 1.5560701, false);
        }break;
        case 30111: // airplane - Strandar_Pinemist
        case 35111: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50843, map);
            EverQuest->SpawnCreature(50898, map, 380.31, 163.27, -222.4, 1.6051575, false);
        }break;
        case 30112: // airplane - Strandar_Pinemist
        case 35112: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50843, map);
            EverQuest->SpawnCreature(50885, map, 385.5, 163.12, -222.4, 1.6051575, false);
        }break;
        case 30114: // airplane - Thalik_Silenthand
        case 35114: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50851, map);
            EverQuest->SpawnCreature(50861, map, 398.05, 163.36, -222.4, 1.5560701, false);
        }break;
        case 30115: // airplane - Thalik_Silenthand
        case 35115: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50851, map);
            EverQuest->SpawnCreature(50868, map, 398.05, 163.36, -222.4, 1.5560701, false);
        }break;
        case 30120: // airplane - Torgon_Blademaster
        case 35120: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50845, map);
            EverQuest->SpawnCreature(50867, map, 403.8, 163.36, -222.4, 1.5560701, false);
        }break;
        case 30121: // airplane - Torgon_Blademaster
        case 35121: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50845, map);
            EverQuest->SpawnCreature(50864, map, 403.8, 163.36, -222.4, 1.5560701, false);
        }break;
        case 30130: // airplane - Wizard_Schrock
        case 35130: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50847, map);
            EverQuest->SpawnCreature(50891, map, 378.25, 169.65, -222.4, 0.0024543693, false);
        }break;
        case 30131: // airplane - Wizard_Schrock
        case 35131: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50847, map);
            EverQuest->SpawnCreature(50878, map, 378.25, 169.65, -222.4, 0.0024543693, false);
        }break;
        case 30150: // akanon - Drekon_Vebnebber
        case 35150: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(49593, map, 436.16, -87, -34.8, 0, true);
        }break;
        case 30200: // akanon - Silox_Azrix
        case 35200: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(49598, map);
        }break;
        case 30221: // befallen - skeleton_Lrodd
        case 35221: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(48057, map, -184.73, -25.52, -19.14, 3.117049, false);
        }break;
        case 30224: // burningwood - Atheling_Plague
        case 35224: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(52010, map, x, y + 1.45, z, 0, true);
        }break;
        case 30228: // burningwood - Faelin_Bloodbriar
        case 35228: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(51986, map);
        }break;
        case 30229: // burningwood - Naxot_Deepwater
        case 35229: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(51969, map);
            EverQuest->SpawnCreature(51981, map, -580, 435, -87, 0, true);
        }break;
        case 30234: // burningwood - Telin_Darkforest
        case 35234: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(51986, map, 939.31, 827.08, -44.95, 2.6875343, false);
        }break;
        case 30259: // cabeast - a_froglok_slave
        case 35259: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(53142, map);
        }break;
        case 30294: // cabeast - Klok_Unlar
        case 35294: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(53257, map, 163.27, 34.8, 1.16, 0, true);
        }break;
        case 30352: // cabwest - an_Iksar_hermit
        case 35352: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(51518, map);
        }break;
        case 30388: // cauldron - Ghilanbiddle_Nylwadil
        case 35388: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(50803, map, -210.54, -618.57, 45.24, 0, true);
        }break;
        case 30390: // cazicthule - a_living_void
        case 35390: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(48884, map);
            EverQuest->SpawnCreature(48996, map, x, y, z, 0, false);
        }break;
        case 30391: // charasis - An_Iksar_Apparition
        case 35391: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(53108, map);
            EverQuest->SpawnCreature(53077, map, x, y, z, orientation, true);
            EverQuest->SpawnCreature(53128, map, -190.82, 3.77, 2.32, 3.9269907, true);
            EverQuest->SpawnCreature(53129, map, -200.1, 3.77, 2.32, 5.595962, true);
            EverQuest->SpawnCreature(53130, map, -200.1, -3.77, 2.32, 0.83448553, true);
            EverQuest->SpawnCreature(53131, map, -190.82, -3.77, 2.32, 2.4543693, true);
        }break;
        case 30393: // charasis - Gandan_Tailfist
        case 35393: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(53115, map);
        }break;
        case 30394: // charasis - the_spirit_of_Rile
        case 35394: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(53132, map);
        }break;
        case 30398: // citymist - #Marl_Kastane
        case 35398: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52144, map);
        }break;
        case 30399: // citymist - #Neh`Ashiir
        case 35399: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52139, map);
            EverQuest->SpawnCreature(52140, map, x, y, z, orientation, false);
        }break;
        case 30400: // citymist - a_human_skeleton
        case 35400: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52128, map);
        }break;
        case 30401: // citymist - Lhranc
        case 35401: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52130, map);
            EverQuest->SpawnCreature(52143, map, 0.26, 24.68, 2.26, 4.712389, false);
        }break;
        case 30424: // commons - Simon_Aldicott
        case 35424: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(47087, map, 75.31, 841.29, -14.79, 0.6135923, false);
        }break;
        case 30432: // crushbone - an_elven_priest
        case 35432: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50020, map);
        }break;
        case 30434: // crushbone - an_elven_slave
        case 35434: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50034, map);
        }break;
        case 30435: // crushbone - an_elven_slave
        case 35435: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50034, map);
        }break;
        case 30437: // crushbone - an_elven_slave
        case 35437: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50034, map);
        }break;
        case 30438: // crushbone - an_elven_slave
        case 35438: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50034, map);
        }break;
        case 30440: // crushbone - an_elven_slave
        case 35440: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50034, map);
        }break;
        case 30441: // crushbone - an_elven_slave
        case 35441: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50034, map);
        }break;
        case 30443: // crushbone - an_elven_slave
        case 35443: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50034, map);
        }break;
        case 30444: // crushbone - an_elven_slave
        case 35444: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50034, map);
        }break;
        case 30446: // crushbone - a_dwarven_slave
        case 35446: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50035, map);
        }break;
        case 30447: // crushbone - a_dwarven_slave
        case 35447: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50035, map);
        }break;
        case 30449: // crushbone - a_dwarven_slave
        case 35449: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50035, map);
        }break;
        case 30450: // crushbone - a_dwarven_slave
        case 35450: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50035, map);
        }break;
        case 30453: // crushbone - a_dwarven_smith
        case 35453: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50019, map);
        }break;
        case 30456: // crushbone - Retlon_Brenclog
        case 35456: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50018, map);
        }break;
        case 30457: // crushbone - Retlon_Brenclog
        case 35457: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50018, map);
        }break;
        case 30459: // crystal - Historian_Baenek
        case 35459: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54815, map);
            EverQuest->SpawnCreature(54803, map, -43.79, -89.32, -160.66, 0, false);
        }break;
        case 30461: // dalnir - a_coerced_channeler
        case 35461: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(53027, map);
        }break;
        case 30462: // dalnir - a_tormented_tradesman
        case 35462: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(53053, map);
            EverQuest->SpawnCreature(53057, map, x, y, z, orientation, false);
        }break;
        case 30463: // dalnir - a_tormented_tradesman
        case 35463: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(53053, map);
            EverQuest->SpawnCreature(53057, map, x, y, z, orientation, false);
        }break;
        case 30464: // dalnir - Haggle_Baron_Dalnir
        case 35464: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(53057, map);
        }break;
        case 30467: // dreadlands - Brother_Balatin
        case 35467: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(51958, map);
            EverQuest->SpawnCreature(51928, map, x, y, z, orientation, false);
        }break;
        case 30472: // droga - an_enslaved_iksar
        case 35472: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(51441, map);
        }break;
        case 30475: // eastkarana - Althele
        case 35475: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(46656, map, -752.55, -462.55, 1.16, 3.117049, true);
        }break;
        case 30476: // eastkarana - Althele
        case 35476: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(46651, map);
            EverQuest->DespawnCreature(46653, map);
            EverQuest->DespawnCreature(46656, map);
            EverQuest->DespawnCreature(46592, map);
            EverQuest->DespawnCreature(46590, map);
            EverQuest->DespawnCreature(46591, map);
        }break;
        case 30479: // eastkarana - Ganelorn_Oast
        case 35479: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(46658, map, -786.48, -232, 4.52, 3.3870296, true);
        }break;
        case 30481: // eastkarana - Milea_Clothspinner
        case 35481: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(46659, map, -542.3, -1601.09, 0.87, 5.5468745, true);
        }break;
        case 30483: // eastkarana - Nuien
        case 35483: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(46653, map, -1113.6, -827.66, 36.68, 1.5168002, true);
        }break;
        case 30484: // eastkarana - Sionae
        case 35484: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(46651, map, -1060.53, 87, 0.87, 4.4914956, true);
        }break;
        case 30491: // eastwastes - ##Garadain_Glacierbane
        case 35491: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54415, map);
        }break;
        case 30493: // eastwastes - #Dobbin_Crossaxe
        case 35493: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54383, map);
            EverQuest->SpawnCreature(54384, map, -166.46, -924.52, 45.82, 1.521709, true);
        }break;
        case 30494: // eastwastes - a_coldain_lookout
        case 35494: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(54369, map, -791.99, -26.39, 43.5, 4.9087386, false);
            EverQuest->SpawnCreature(54369, map, -781.84, -26.97, 43.21, 4.9087386, false);
            EverQuest->SpawnCreature(54370, map, -779.23, -33.35, 43.5, 4.9087386, false);
            EverQuest->SpawnCreature(54370, map, -790.83, -26.39, 43.5, 4.9087386, false);
            EverQuest->SpawnCreature(54370, map, -787.64, -28.13, 43.5, 4.9087386, false);
            EverQuest->SpawnCreature(54368, map, -787.35, -31.9, 44.08, 4.9087386, true);
        }break;
        case 30495: // eastwastes - an_ogre_mercenary
        case 35495: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54421, map);
        }break;
        case 30496: // eastwastes - Boridain_Glacierbane
        case 35496: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54453, map);
            EverQuest->SpawnCreature(54364, map, x, y, z, orientation, true);
        }break;
        case 30512: // eastwastes - Gloradin_Coldheart
        case 35512: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54447, map);
            EverQuest->DespawnCreature(54411, map);
            EverQuest->DespawnCreature(54423, map);
            EverQuest->DespawnCreature(54455, map);
            EverQuest->SpawnCreature(54406, map, x, y, z, orientation, true);
            EverQuest->SpawnCreature(54412, map, -786.77, -113.1, 52.64, 0.46633017, true);
        }break;
        case 30514: // eastwastes - Peffin_Ambersnow
        case 35514: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54427, map);
        }break;
        case 30516: // eastwastes - Tanik_Greskil
        case 35516: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54360, map);
        }break;
        case 30521: // emeraldjungle - #Scout_Vyrak
        case 35521: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52396, map);
        }break;
        case 30522: // emeraldjungle - Scout_Vyrak
        case 35522: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52382, map);
            EverQuest->SpawnCreature(52415, map, -200.1, 1175.66, -12.76, 0, false);
            EverQuest->SpawnCreature(52396, map, x, y, z, 0, true);
        }break;
        case 30607: // everfrost - Karg_IceBear
        case 35607: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(47715, map);
        }break;
        case 30608: // everfrost - Karg_IceBear
        case 35608: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(47715, map);
        }break;
        case 30609: // everfrost - Lich_of_Miragul
        case 35609: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(47709, map);
            EverQuest->SpawnCreature(47710, map, x, y, z, orientation, false);
        }break;
        case 30617: // fearplane - a_broken_golem
        case 35617: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50931, map);
            EverQuest->SpawnCreature(50941, map, x, y, z, orientation, false);
        }break;
        case 30621: // feerrott - #Scout_Ahlikal
        case 35621: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(48729, map);
        }break;
        case 30631: // feerrott - Scout_Ahlikal
        case 35631: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(48791, map);
            EverQuest->SpawnCreature(48798, map, 328.57, -828.53, 0.29, 0, false);
            EverQuest->SpawnCreature(48729, map, x, y, z, 0, true);
        }break;
        case 30632: // felwithea - Arrias_Arcanum
        case 35632: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50207, map);
        }break;
        case 30647: // felwithea - Tolon_Nurbyte
        case 35647: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50258, map);
        }break;
        case 30670: // fieldofbone - Oracle_Qulin
        case 35670: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(51255, map, x, y + 1.45, z, orientation, false);
        }break;
        case 30706: // firiona - Tracker_Azeal_
        case 35706: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(51691, map);
            EverQuest->SpawnCreature(51813, map, 178.93, -528.38, 41.32, 2.5280004, false);
            EverQuest->SpawnCreature(51814, map, x, y, z, orientation, false);
        }break;
        case 30707: // firiona - Vekis
        case 35707: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(51689, map, -1105.48, 854.05, -14.21, 0, true);
            EverQuest->SpawnCreature(51692, map, -750.81, 620.6, -22.62, 2.6507187, true);
        }break;
        case 30710: // freporte - Orc_Centurion
        case 35710: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(46043, map);
        }break;
        case 30723: // freporte - Groflah_Steadirt
        case 35723: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(46046, map, -60.9, -266.8, -15.11, 0, true);
        }break;
        case 30773: // freporte - Rigg_Nostra
        case 35773: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(46043, map, -132.24, -41.76, -15.08, 0, false);
        }break;
        case 30828: // freportn - Merko_Quetalis
        case 35828: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(45900, map, 38.28, -74.53, -4.93, 2.9452431, true);
        }break;
        case 30866: // freportw - Guard_Alayle
        case 35866: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(46037, map, -15.95, -44.66, -2.9, 3.1415927, true);
        }break;
        case 30915: // frontiermtns - a_human_skeleton
        case 35915: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52264, map);
        }break;
        case 30932: // gfaydark - Faelin_Bloodbriar
        case 35932: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(49557, map);
        }break;
        case 30978: // greatdivide - Bekerak_Coldbones
        case 35978: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(54517, map, -873.19, -805.04, 74.1, 1.6076119, false);
            EverQuest->MakeCreatureAttackPlayer(54517, map, player);
        }break;
        case 30982: // greatdivide - Bloodmaw
        case 35982: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54528, map);
            EverQuest->DespawnCreature(54527, map);
            EverQuest->SpawnCreature(54518, map, -1719.7, 919.88, -31.03, 2.0125828, false);
        }break;
        case 30985: // greatdivide - Sentry_Badain
        case 35985: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54504, map);
            EverQuest->SpawnCreature(54607, map, -5.51, -21.46, 60.32, 0, true);
        }break;
        case 30989: // grobb - ##a_skeleton
        case 35989: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(49420, map);
        }break;
        case 30990: // grobb - #a_skeleton
        case 35990: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(48724, map);
        }break;
        case 30994: // grobb - a_skeleton
        case 35994: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(49386, map);
        }break;
        case 31028: // halas - Arantir_Karondor
        case 36028: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(47658, map);
        }break;
        case 31036: // halas - Dargon
        case 36036: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(47580, map);
            EverQuest->SpawnCreature(47658, map, x, y, z, orientation, true);
        }break;
        case 31037: // halas - Dargon
        case 36037: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(47580, map);
            EverQuest->SpawnCreature(47658, map, x, y, z, orientation, true);
        }break;
        case 31038: // halas - Dargon
        case 36038: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(47580, map);
            EverQuest->SpawnCreature(47658, map, x, y, z, orientation, true);
        }break;
        case 31080: // highkeep - Tearon_Bleanix
        case 36080: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(45734, map);
            EverQuest->SpawnCreature(49324, map, 3.77, 15.08, 1.07, 3.9515345, true);
        }break;
        case 31089: // highkeep - Fenn_Kaedrick
        case 36089: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(45788, map, -29.58, -74.82, 1.16, 0.3926991, true);
            EverQuest->SpawnCreature(45789, map, -22.62, -75.11, 1.16, 0, true);
            EverQuest->SpawnCreature(45790, map, -22.62, -69.6, 1.16, 0, true);
        }break;
        case 31098: // highkeep - Tearon_Bleanix
        case 36098: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(45734, map);
        }break;
        case 31101: // highpass - Anson_McBale
        case 36101: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(45668, map, 2.9, 97.44, 13.05, 5.5223308, true);
        }break;
        case 31114: // highpass - Jovan
        case 36114: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(45638, map);
        }break;
        case 31116: // highpass - Prak
        case 36116: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(45674, map, 36.83, 134.56, 9.21, 1.1535535, true);
        }break;
        case 31118: // highpass - Stanos_Herkanor
        case 36118: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(45668, map);
        }break;
        case 31119: // highpass - Stanos_Herkanor
        case 36119: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(45668, map);
        }break;
        case 31120: // highpass - Stanos_Herkanor
        case 36120: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(45668, map);
        }break;
        case 31121: // hole - #High_Scale_Kirn_
        case 36121: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(48244, map);
            EverQuest->SpawnCreature(48225, map, x, y, z, orientation, false);
            EverQuest->MakeCreatureAttackPlayer(48225, map, player);
        }break;
        case 31122: // hole - Caradon
        case 36122: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(48241, map, 123.71, -56.78, -62.03, 2.8225245, false);
            EverQuest->MakeCreatureAttackPlayer(48221, map, player);
            EverQuest->MakeCreatureAttackPlayer(48241, map, player);
        }break;
        case 31123: // hole - Ghost_of_Glohnor
        case 36123: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(48218, map);
            EverQuest->SpawnCreature(48247, map, 237.51, 134.68, -196.62, 3.0679615, false);
        }break;
        case 31124: // hole - Jaeil_the_Wretched
        case 36124: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(48217, map);
            EverQuest->SpawnCreature(48240, map, x, y, z, orientation, false);
            EverQuest->MakeCreatureAttackPlayer(48240, map, player);
        }break;
        case 31128: // iceclad - #General_Bragmur
        case 36128: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(53311, map);
            EverQuest->SpawnCreature(53313, map, x, y, z, orientation, false);
        }break;
        case 31129: // iceclad - #Snowfang_fisher
        case 36129: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(53308, map, x, y, z, 3.1415927, false);
        }break;
        case 31132: // iceclad - a_lost_pirate
        case 36132: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(53306, map, x, y, z, orientation, false);
        }break;
        case 31143: // iceclad - General_Bragmur
        case 36143: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(53321, map);
            EverQuest->SpawnCreature(53311, map, x, y, z, orientation, false);
        }break;
        case 31153: // innothule - #a_skeleton
        case 36153: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(48724, map);
        }break;
        case 31161: // kael - a_barbarian_mercenary
        case 36161: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(53681, map);
        }break;
        case 31217: // kaesora - spectral_librarian
        case 36217: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52012, map);
            EverQuest->SpawnCreature(52035, map, x, y, z, orientation, false);
            EverQuest->MakeCreatureAttackPlayer(52035, map, player);
        }break;
        case 31218: // kaesora - tortured_librarian
        case 36218: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52013, map);
        }break;
        case 31258: // kaladimb - Harnoff_Splitrock
        case 36258: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50539, map);
        }break;
        case 31259: // kaladimb - Harnoff_Splitrock
        case 36259: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50539, map);
        }break;
        case 31267: // kaladimb - Kinlo_Strongarm
        case 36267: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(50561, map, 100.57, -54.96, -2, 0.8467574, false);
        }break;
        case 31271: // kaladimb - Nella_Stonebraids
        case 36271: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50562, map);
        }break;
        case 31277: // kaladimb - Usbak_the_Old
        case 36277: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50561, map);
        }break;
        case 31278: // karnor - a_human_skeleton
        case 36278: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52784, map);
        }break;
        case 31279: // karnor - Spirit_of_Venril_Sathir
        case 36279: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52787, map);
            EverQuest->SpawnCreature(52789, map, x, y, z, orientation, false);
            EverQuest->MakeCreatureAttackPlayer(52789, map, player);
        }break;
        case 31280: // karnor - Venril_Sathirs_remains
        case 36280: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52802, map);
            EverQuest->SpawnCreature(52787, map, x, y, z, orientation, false);
        }break;
        case 31288: // kerraridge - Kirran_Mirrah
        case 36288: // Fallthrough - Repeat Quest
        {
            EverQuest->MakeCreatureAttackPlayer(51000, map, player);
        }break;
        case 31290: // kerraridge - Marl_Kastane
        case 36290: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(51001, map);
        }break;
        case 31301: // kithicor - #Adjutant_D`kan
        case 36301: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(47015, map, 239.83, 666.13, 79.75, 3.1415927, true);
        }break;
        case 31302: // kithicor - #Advisor_C`zatl
        case 36302: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(47015, map, 239.83, 666.13, 79.75, 3.1415927, true);
        }break;
        case 31303: // kithicor - #Brigadier_G`tav
        case 36303: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(47015, map, 239.83, 666.13, 79.75, 3.1415927, true);
        }break;
        case 31304: // kithicor - #Coercer_Q`ioul
        case 36304: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(47015, map, 239.83, 666.13, 79.75, 3.1415927, true);
        }break;
        case 31305: // kithicor - #Ioltos_V`ghera
        case 36305: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(47015, map, 239.83, 666.13, 79.75, 3.1415927, true);
        }break;
        case 31306: // kithicor - #Tasi_V`ghera
        case 36306: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(47015, map, 239.83, 666.13, 79.75, 3.1415927, true);
        }break;
        case 31307: // kithicor - #War_Priestess_T`zan
        case 36307: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(47015, map, 239.83, 666.13, 79.75, 3.1415927, true);
        }break;
        case 31316: // kithicor - General_V`ghera
        case 36316: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(47015, map);
        }break;
        case 31317: // kithicor - Giz_X`Tin
        case 36317: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(47010, map, x + 5.8, y + 5.8, z, orientation, false);
            EverQuest->MakeCreatureAttackPlayer(47010, map, player);
        }break;
        case 31318: // kithicor - Grim_Oakfist
        case 36318: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(47021, map);
        }break;
        case 31327: // lakeofillomen - Astral_Projection
        case 36327: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(51893, map);
            EverQuest->SpawnCreature(51907, map, x, y + 1.45, z, orientation, false);
            EverQuest->SpawnCreature(51911, map, x, y - 1.45, z, orientation, false);
            EverQuest->MakeCreatureAttackPlayer(51907, map, player);
            EverQuest->MakeCreatureAttackPlayer(51911, map, player);
        }break;
        case 31329: // lakeofillomen - Klok_Sargin
        case 36329: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(51913, map, x, y + 1.45, z, orientation, true);
        }break;
        case 31337: // lakerathe - #Natasha_Whitewater
        case 36337: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(49307, map);
        }break;
        case 31338: // lakerathe - a_royal_fish
        case 36338: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(49251, map);
            EverQuest->SpawnCreature(49324, map, x, y, z, orientation, true);
        }break;
        case 31339: // lakerathe - Deep
        case 36339: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(49262, map);
        }break;
        case 31340: // lakerathe - Eldreth
        case 36340: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(49256, map);
        }break;
        case 31345: // lakerathe - Kazen_Fecae
        case 36345: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(49316, map, 82.3, -434.68, 16.94, 1.4137166, false);
            EverQuest->MakeCreatureAttackPlayer(49316, map, player);
        }break;
        case 31353: // lakerathe - Princess_Lenya_Thex
        case 36353: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(49324, map);
        }break;
        case 31357: // lakerathe - Shmendrik_Lavawalker
        case 36357: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(49307, map, 1052.79, 46.4, 14.79, 4.7222066, true);
        }break;
        case 31367: // lfaydark - #Scout_Rahjiq
        case 36367: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(49924, map);
        }break;
        case 31376: // lfaydark - Scout_Rahjiq
        case 36376: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(49993, map);
            EverQuest->SpawnCreature(49998, map, -101.21, -473.28, 3.19, 0, false);
            EverQuest->SpawnCreature(49924, map, x, y, z, 0, true);
        }break;
        case 31377: // mischiefplane - a_white_stallion
        case 36377: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(55265, map);
            EverQuest->SpawnCreature(55136, map, x, y, z, orientation, true);
        }break;
        case 31384: // mistmoore - an_avenging_caitiff
        case 36384: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50061, map);
        }break;
        case 31387: // misty - Bronin_Higginsbot
        case 36387: // Fallthrough - Repeat Quest
        {
            EverQuest->MakeCreatureAttackPlayer(47864, map, player);
        }break;
        case 31395: // najena - Akksstaff
        case 36395: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(48514, map);
        }break;
        case 31396: // najena - a_visiting_priestess
        case 36396: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(48529, map);
        }break;
        case 31398: // necropolis - #a_ghostly_presence
        case 36398: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54842, map);
            EverQuest->SpawnCreature(54843, map, x, y, z, 0, false);
            EverQuest->MakeCreatureAttackPlayer(54843, map, player);
        }break;
        case 31399: // necropolis - #Spirit_of_Garzicor
        case 36399: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54845, map);
        }break;
        case 31400: // necropolis - #Spirit_of_Garzicor_
        case 36400: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54846, map);
        }break;
        case 31432: // neriakb - Draxiz_N`Ryt
        case 36432: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(48404, map, -52.2, -162.4, -15.95, 0, true);
        }break;
        case 31454: // neriakb - vengeful_spirit
        case 36454: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(48405, map);
        }break;
        case 31458: // neriakb - Yegek_B`Larin
        case 36458: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(48412, map, -7.25, -353.22, -7.25, 3.1415927, false);
        }break;
        case 31461: // neriakb - Zanotix_Ixtaz
        case 36461: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(48405, map, 20.3, -251.43, -10.15, 0, false);
        }break;
        case 31507: // northkarana - Capt_Linarius
        case 36507: // Fallthrough - Repeat Quest
        {
            EverQuest->MakeCreatureAttackPlayer(46431, map, player);
        }break;
        case 31514: // northkarana - Watchman_Dexlin
        case 36514: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(46462, map);
        }break;
        case 31515: // northkarana - withered_treant
        case 36515: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(46474, map);
        }break;
        case 31520: // nurga - a_sleeping_ogre
        case 36520: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(53272, map);
            EverQuest->SpawnCreature(53258, map, x, y, z, orientation, false);
            EverQuest->MakeCreatureAttackPlayer(53258, map, player);
        }break;
        case 31568: // oot - Sentry_Xyrin
        case 36568: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50761, map);
        }break;
        case 31573: // overthere - Alchemist_Granika
        case 36573: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(52286, map, 801.56, 649.6, -14.21, 4.6633015, false);
            EverQuest->SpawnCreature(52288, map, 810.26, 655.4, -14.21, 4.6633015, false);
            EverQuest->SpawnCreature(52288, map, 782.42, 655.4, -14.21, 4.6633015, false);
            EverQuest->SpawnCreature(52288, map, 810.26, 661.2, -14.21, 4.6633015, false);
            EverQuest->SpawnCreature(52288, map, 782.42, 661.2, -14.21, 4.6633015, false);
            EverQuest->SpawnCreature(52289, map, 782.42, 649.6, -14.21, 4.6633015, false);
        }break;
        case 31588: // overthere - Kurron_Ni
        case 36588: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52315, map);
            EverQuest->SpawnCreature(52290, map, x, y, z, orientation, true);
        }break;
        case 31591: // overthere - Nekexin_Virulence
        case 36591: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52300, map);
        }break;
        case 31605: // paineel - avatar_of_dread
        case 36605: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(51187, map);
        }break;
        case 31606: // paineel - avatar_of_terror
        case 36606: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(51188, map);
        }break;
        case 31617: // paineel - Duriek_Bloodpool
        case 36617: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(51072, map);
        }break;
        case 31633: // paineel - Miadera_Shadowfyre
        case 36633: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(51188, map, 342.78, 122.09, -10.73, 3.1415927, true);
        }break;
        case 31634: // paineel - Nivold_Predd
        case 36634: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(51187, map, 356.7, 137.46, -10.73, 3.1415927, true);
        }break;
        case 31653: // paineel - Tormented_Soul
        case 36653: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(51093, map);
        }break;
        case 31656: // paw - Zixx_Nenix
        case 36656: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(46772, map);
        }break;
        case 31657: // permafrost - #Scout_Janomin
        case 36657: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50950, map);
        }break;
        case 31658: // permafrost - Scout_Janomin
        case 36658: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50977, map);
            EverQuest->SpawnCreature(50987, map, 282.17, -39.15, -30.45, 0, false);
            EverQuest->SpawnCreature(50950, map, x, y, z, 0, true);
        }break;
        case 31661: // qcat - an_investigator
        case 36661: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(48544, map, 109.62, -14.21, -11.31, 3.1415927, true);
        }break;
        case 31669: // qcat - Garuc_Anehm
        case 36669: // Fallthrough - Repeat Quest
        {
            EverQuest->MakeCreatureAttackPlayer(48608, map, player);
        }break;
        case 31690: // qcat - Sragg_Bloodheart
        case 36690: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(48579, map, 120.35, -93.09, -11.02, 1.5707964, true);
        }break;
        case 31707: // qey2hh1 - a_wandering_spirit
        case 36707: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(46326, map);
        }break;
        case 31708: // qey2hh1 - a_wandering_spirit
        case 36708: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(46326, map);
        }break;
        case 31712: // qey2hh1 - Chrislin_Baker
        case 36712: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(46319, map);
            EverQuest->SpawnCreature(46277, map, 221.27, -3338.48, 4.64, 3.3133984, true);
        }break;
        case 31727: // qey2hh1 - Lempeck_Hargrin
        case 36727: // Fallthrough - Repeat Quest
        {
            EverQuest->MakeCreatureAttackPlayer(46324, map, player);
        }break;
        case 31728: // qey2hh1 - Linaya_Sowlin
        case 36728: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(46391, map, -986, -2320, 6.67, 2.5255458, true);
        }break;
        case 31730: // qey2hh1 - Maligar
        case 36730: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(46320, map);
            EverQuest->SpawnCreature(46388, map, x, y, z, orientation, false);
            EverQuest->MakeCreatureAttackPlayer(46388, map, player);
        }break;
        case 31736: // qey2hh1 - Renux_Herkanor
        case 36736: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(46304, map);
        }break;
        case 31747: // qeynos - #Donally_Stultz
        case 36747: // Fallthrough - Repeat Quest
        {
            EverQuest->MakeCreatureAttackPlayer(45327, map, player);
        }break;
        case 31749: // qeynos - #Riley_Shplots
        case 36749: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(45330, map, -3.19, -120.06, -7.25, 0, true);
        }break;
        case 31750: // qeynos - #Willie_Garrote
        case 36750: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(45327, map, -98.89, 15.95, -4.64, 0, true);
        }break;
        case 31753: // qeynos - an_investigator
        case 36753: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(45322, map, 58, -93.96, 1.74, 2.9452431, true);
        }break;
        case 31754: // qeynos - an_investigator
        case 36754: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(45186, map);
        }break;
        case 31773: // qeynos - Captain_Tillin
        case 36773: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(45331, map, 21.75, -119.48, -6.96, 0, true);
        }break;
        case 31774: // qeynos - Captain_Tillin
        case 36774: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(45318, map, 64.38, -29, 0.87, 3.1415927, true);
        }break;
        case 31808: // qeynos - Grahan_Rothkar
        case 36808: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(45160, map, -34.8, -150.8, -7.1, 0, true);
        }break;
        case 31831: // qeynos - Lomarc
        case 36831: // Fallthrough - Repeat Quest
        {
            EverQuest->MakeCreatureAttackPlayer(45324, map, player);
        }break;
        case 31880: // qeynos - Yakem_Oreslinger
        case 36880: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(45185, map);
        }break;
        case 31881: // qeynos - Yakem_Oreslinger
        case 36881: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(45185, map);
        }break;
        case 31964: // qeytoqrg - Axe_Broadsmith
        case 36964: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(45604, map, 1118.82, 305.95, -5.51, 0, true);
        }break;
        case 32033: // rathemtn - Glaron_the_Wicked
        case 37033: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(49134, map);
        }break;
        case 32038: // rathemtn - Kazzel_D`Leryt
        case 37038: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(49231, map, x + 1.45, y + 1.45, z, 0, true);
        }break;
        case 32046: // rathemtn - Tabien_the_Goodly
        case 37046: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(49206, map);
        }break;
        case 32050: // rathemtn - Xenyari_Lisariel
        case 37050: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(49204, map);
        }break;
        case 32110: // sebilis - An_Undead_Bard
        case 37110: // Fallthrough - Repeat Quest
        {
            EverQuest->MakeCreatureAttackPlayer(52118, map, player);
        }break;
        case 32111: // sebilis - a_fallen_monk
        case 37111: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52102, map);
            EverQuest->SpawnCreature(52122, map, x, y, z, orientation, false);
            EverQuest->MakeCreatureAttackPlayer(52122, map, player);
        }break;
        case 32114: // skyfire - Warder_Cecilia
        case 37114: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52185, map);
            EverQuest->SpawnCreature(52173, map, x, y, z, 0, true);
            EverQuest->MakeCreatureAttackPlayer(52173, map, player);
        }break;
        case 32130: // skyshrine - Ralgyn
        case 37130: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(53975, map);
        }break;
        case 32132: // skyshrine - Sentry_Emil
        case 37132: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54090, map);
        }break;
        case 32134: // skyshrine - Sentry_Rotiart
        case 37134: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54153, map);
            EverQuest->SpawnCreature(53970, map, x, y, z, orientation, false);
            EverQuest->MakeCreatureAttackPlayer(53970, map, player);
        }break;
        case 32146: // soldungb - #Zordak_Ragefire
        case 37146: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(47790, map, x, y, z, orientation, false);
            EverQuest->MakeCreatureAttackPlayer(47790, map, player);
        }break;
        case 32147: // soldungb - Zordak_Ragefire
        case 37147: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(47802, map);
        }break;
        case 32148: // soltemple - a_seeker
        case 37148: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(51397, map);
            EverQuest->SpawnCreature(51426, map, x, y, z, orientation, false);
            EverQuest->MakeCreatureAttackPlayer(51426, map, player);
        }break;
        case 32236: // southkarana - Yeka_Ias
        case 37236: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(46510, map);
        }break;
        case 32242: // steamfont - a_female_rat
        case 37242: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(49844, map);
            EverQuest->SpawnCreature(49809, map, x, y, z, orientation, false);
        }break;
        case 32251: // steamfont - Yendar_Starpyre
        case 37251: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(49818, map);
            EverQuest->SpawnCreature(49898, map, x + 2.9, y - 2.9, z, orientation, true);
        }break;
        case 32255: // stonebrunt - #Scout_Malom
        case 37255: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52636, map);
        }break;
        case 32258: // stonebrunt - a_spirit
        case 37258: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52630, map);
            EverQuest->SpawnCreature(52631, map, 137.46, -20.3, 147.61, 0.8222137, true);
        }break;
        case 32260: // stonebrunt - Ghost_of_Ridossan
        case 37260: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52632, map);
        }break;
        case 32269: // stonebrunt - Scout_Malom
        case 37269: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52634, map);
            EverQuest->SpawnCreature(52633, map, 1237.14, -970.34, 596.53, 0, true);
            EverQuest->SpawnCreature(52636, map, x, y, z, 0, true);
        }break;
        case 32272: // swampofnohope - #Scout_Eyru
        case 37272: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(51572, map);
        }break;
        case 32282: // swampofnohope - Scout_Eyru
        case 37282: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(51563, map);
            EverQuest->SpawnCreature(51658, map, -841.87, 629.3, 1.16, 0, false);
            EverQuest->SpawnCreature(51572, map, x, y, z, 0, true);
        }break;
        case 32309: // thurgadina - Erdarf_Restil
        case 37309: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54260, map);
        }break;
        case 32310: // thurgadina - Erdarf_Restil
        case 37310: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54260, map);
        }break;
        case 32347: // thurgadinb - Councilor_Juliah_Lockheart
        case 37347: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(55414, map);
            EverQuest->SpawnCreature(55417, map, x, y, z, orientation, true);
            EverQuest->MakeCreatureAttackPlayer(55417, map, player);
        }break;
        case 32360: // timorous - #Scout_Sihmoj
        case 37360: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52501, map);
        }break;
        case 32362: // timorous - an_Iksar_master
        case 37362: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52492, map);
            EverQuest->SpawnCreature(52539, map, x, y, z, orientation, true);
        }break;
        case 32366: // timorous - Avatar_of_Water
        case 37366: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52532, map);
        }break;
        case 32368: // timorous - Dolgin_Codslayer
        case 37368: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52506, map);
            EverQuest->SpawnCreature(52525, map, -3406.05, -631.62, -0.18, 1.5462526, false);
            EverQuest->MakeCreatureAttackPlayer(52525, map, player);
        }break;
        case 32371: // timorous - Jhassad_Oceanson
        case 37371: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52526, map);
            EverQuest->SpawnCreature(52532, map, -3381.69, -546.94, 0.29, 4.712389, true);
        }break;
        case 32373: // timorous - Natasha_Whitewater
        case 37373: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52529, map);
        }break;
        case 32374: // timorous - Natasha_Whitewater
        case 37374: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52529, map);
        }break;
        case 32376: // timorous - Omat_Vastsea
        case 37376: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(52529, map, -3364.29, -637.42, 22.04, 4.712389, true);
        }break;
        case 32377: // timorous - Omat_Vastsea
        case 37377: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(52529, map, -3364.29, -637.42, 22.04, 4.712389, true);
        }break;
        case 32378: // timorous - Omat_Vastsea
        case 37378: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(52526, map, -3468.11, -516.49, 4.15, 0.024543693, true);
        }break;
        case 32379: // timorous - Scout_Sihmoj
        case 37379: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52490, map);
            EverQuest->SpawnCreature(52493, map, -2668.87, 576.52, 2.9, 0, false);
            EverQuest->SpawnCreature(52501, map, x, y, z, 0, true);
        }break;
        case 32380: // timorous - The_Great_Oowomp
        case 37380: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(52533, map, 1660.31, 905.61, 2.29, 0.319068, true);
        }break;
        case 32387: // tox - #Veisha_Fathomwalker
        case 37387: // Fallthrough - Repeat Quest
        {
            EverQuest->MakeCreatureAttackPlayer(48142, map, player);
        }break;
        case 32395: // tox - Shintar_Vinlail
        case 37395: // Fallthrough - Repeat Quest
        {
            EverQuest->MakeCreatureAttackPlayer(48163, map, player);
        }break;
        case 32399: // trakanon - a_human_skeleton
        case 37399: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52486, map);
        }break;
        case 32400: // trakanon - Crusader_Vragor
        case 37400: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52483, map);
            EverQuest->SpawnCreature(52488, map, -655.4, -522.29, -105.85, 0.93266034, false);
        }break;
        case 32403: // trakanon - Kaiaren
        case 37403: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(52460, map, 88.74, 716.3, -98.31, 0, true);
            EverQuest->MakeCreatureAttackPlayer(52460, map, player);
        }break;
        case 32406: // unrest - Khrix_Fritchoff
        case 37406: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(50331, map, 180.38, 17.4, 5.8, 0, true);
        }break;
        case 32408: // unrest - Serra
        case 37408: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(50307, map);
        }break;
        case 32411: // wakening - #Lieutenant_Krofer
        case 37411: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54649, map);
        }break;
        case 32414: // wakening - troll_mercenary
        case 37414: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54696, map);
        }break;
        case 32415: // wakening - a_troll_mercenary
        case 37415: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54661, map);
        }break;
        case 32416: // wakening - barbarian_mercenary
        case 37416: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54635, map);
        }break;
        case 32419: // wakening - Eysa_Florawhisper
        case 37419: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54673, map);
        }break;
        case 32422: // wakening - human_mercenary
        case 37422: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54690, map);
        }break;
        case 32428: // wakening - Lieutenant_Krofer
        case 37428: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(54641, map, -202.71, -1442.75, -52.78, 1.5462526, true);
            EverQuest->SpawnCreature(54644, map, -202.13, -1436.66, -52.78, 1.5462526, true);
            EverQuest->SpawnCreature(54646, map, -194.59, -1436.66, -52.78, 1.5462526, true);
            EverQuest->SpawnCreature(54647, map, -194.88, -1443.04, -52.78, 1.5462526, true);
        }break;
        case 32430: // wakening - Ogre_Mercenary
        case 37430: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54637, map);
        }break;
        case 32435: // wakening - Phenocryst
        case 37435: // Fallthrough - Repeat Quest
        {
            EverQuest->SpawnCreature(54651, map, -200.16, 118.35, -58.45, 4.5896707, false);
            EverQuest->SpawnCreature(54651, map, -285.88, -163.57, -55.48, 4.5896707, false);
            EverQuest->SpawnCreature(54653, map, 73.79, 87.88, -58.45, 4.5896707, false);
        }break;
        case 32440: // wakening - Teir`Dal_Mercenary
        case 37440: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54684, map);
        }break;
        case 32442: // wakening - Troll_Mercenary
        case 37442: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54634, map);
        }break;
        case 32445: // warrens - a_captured_erudite
        case 37445: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52734, map);
        }break;
        case 32446: // warrens - a_captured_kerran
        case 37446: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(52739, map);
        }break;
        case 32449: // warslikswood - an_Iksar_knight
        case 37449: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(51384, map);
        }break;
        case 32450: // warslikswood - a_shady_goblin
        case 37450: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(51353, map);
        }break;
        case 32458: // westwastes - #Scout_Charisa
        case 37458: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54700, map);
            EverQuest->SpawnCreature(54785, map, -1397.51, -994.7, -35.96, 0, false);
            EverQuest->SpawnCreature(54785, map, -1400.7, -997.89, -35.38, 0, false);
            EverQuest->SpawnCreature(54785, map, -1403.89, -1001.08, -34.51, 0, false);
            EverQuest->SpawnCreature(54785, map, -1407.08, -1004.27, -33.93, 0, false);
            EverQuest->SpawnCreature(54785, map, -1410.27, -1007.46, -33.06, 0, false);
            EverQuest->SpawnCreature(54785, map, -1413.46, -1013.84, -31.61, 0, false);
            EverQuest->SpawnCreature(54785, map, -1420.13, -1017.03, -30.74, 0, false);
            EverQuest->SpawnCreature(54701, map, x, y, z, orientation, true);
            EverQuest->SpawnCreature(54783, map, -1411.72, -1015.29, -31.32, 0, true);
        }break;
        case 32459: // westwastes - Derasinal
        case 37459: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54714, map);
        }break;
        case 32460: // westwastes - Draazak
        case 37460: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54719, map);
        }break;
        case 32461: // westwastes - Entariz
        case 37461: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54718, map);
        }break;
        case 32463: // westwastes - Ionat
        case 37463: // Fallthrough - Repeat Quest
        {
            EverQuest->DespawnCreature(54717, map);
        }break;
        default: break; // Do Nothing
        }
    }

    void OnPlayerRewardKillRewarder(Player* player, KillRewarder* rewarder, bool /*isDungeon*/, float& /*rate*/) override
    {
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
        // Skip if there this isn't an EQ zone
        if (player->GetMapId() < CONFIG_EQ_MIN_MAP_ID || player->GetMapId() > CONFIG_EQ_MAX_MAP_ID)
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
        else if (spell->m_spellInfo->Id < CONFIG_SPELLS_EQ_SPELLDBC_ID_MIN || spell->m_spellInfo->Id > CONFIG_SPELLS_EQ_SPELLDBC_ID_MAX)
            return;
        else if (spell->m_spellInfo->Effects[EFFECT_0].Effect == SPELL_EFFECT_DUMMY ||
            (spell->m_spellInfo->Effects[EFFECT_0].Effect == SPELL_EFFECT_APPLY_AURA && spell->m_spellInfo->Effects[EFFECT_0].ApplyAuraName == SPELL_AURA_DUMMY))
        {
            if (spell->m_spellInfo->Effects[EFFECT_0].MiscValue == 1) // Bind Self
            {
                if (player->GetMapId() < CONFIG_EQ_MIN_MAP_ID || player->GetMapId() > CONFIG_EQ_MAX_MAP_ID)
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("The spell failed, as it only works in Norrath.");
                    return;
                }
                EverQuest->SetNewBindHome(player);
            }
            else if (spell->m_spellInfo->Effects[EFFECT_0].MiscValue == 2) // Bind Any
            {
                if (player->GetMapId() < CONFIG_EQ_MIN_MAP_ID || player->GetMapId() > CONFIG_EQ_MAX_MAP_ID)
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
            else if (spell->m_spellInfo->Effects[EFFECT_0].MiscValue == 3) // Gate
            {
                EverQuest->StorePositionAsLastGate(player);
                EverQuest->SendPlayerToEQBindHome(player);
            }
        }
    }

    void OnPlayerDelete(ObjectGuid guid, uint32 /*accountId*/) override
    {
        EverQuest->DeletePlayerBindHome(guid);
    }

    void OnPlayerFirstLogin(Player* player) override
    {
        // If the player logged in for the first time and is in a norrath zone, set the bind and aura
        if (player->GetMap() != nullptr && player->GetMap()->GetId() >= CONFIG_EQ_MIN_MAP_ID && player->GetMap()->GetId() <= CONFIG_EQ_MAX_MAP_ID)
        {
            EverQuest->SetNewBindHome(player);
            EverQuest->SetPlayerDayOrNightAura(player);
        }
    }

    void OnPlayerLogin(Player* player) override
    {
        EverQuest->AllLoadedPlayers.push_back(player);
        EverQuest->SetPlayerDayOrNightAura(player);
    }

    void OnPlayerLogout(Player* player) override
    {
        EverQuest->AllLoadedPlayers.erase(std::remove(EverQuest->AllLoadedPlayers.begin(), EverQuest->AllLoadedPlayers.end(), player), EverQuest->AllLoadedPlayers.end());
    }

    // Note: this is AFTER the player changes maps
    void OnPlayerMapChanged(Player* player) override
    {
        // Restrict non-GMs to norrath if set
        if (CONFIG_RESTRICT_PLAYERS_TO_NORRATH == true && player->IsGameMaster() == false)
        {
            if (player->GetMap() != nullptr && (player->GetMap()->GetId() < CONFIG_EQ_MIN_MAP_ID || player->GetMap()->GetId() > CONFIG_EQ_MAX_MAP_ID))
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
        if (CONFIG_LOSE_EXP_ON_DEATH_RELEASE == true)
        {
            // Do nothing if the level is below the minimum
            uint8 playerLevel = player->GetLevel();
            if (playerLevel < CONFIG_LOSE_EXP_ON_DEATH_RELEASE_MIN_LEVEL)
                return;

            // Also do nothing if this isn't a Norrath map and configured to skip
            if (player->GetMap()->GetId() < CONFIG_EQ_MIN_MAP_ID || player->GetMap()->GetId() > CONFIG_EQ_MAX_MAP_ID)
                return;

            // Calculate how much experience to lose
            int nextLevelTotalEXP = player->GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
            int expToLose = nextLevelTotalEXP * (0.01 * CONFIG_LOSE_EXP_ON_DEATH_RELEASE_LOSS_PERCENT);

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
            if (CONFIG_LOSE_EXP_ON_DEATH_RELEASE_ADD_TO_REST == true)
                player->SetRestBonus(player->GetRestBonus() + expToLose);
        }
    }

    // This is done to ensure repeatable quests give EXP more than once
    void OnPlayerQuestComputeXP(Player* player, Quest const* quest, uint32& xpValue) override
    {
        if (CONFIG_QUEST_EXP_ON_REPEAT == false)
            return;

        if (quest->GetQuestId() >= CONFIG_QUEST_ID_LOW && quest->GetQuestId() <= CONFIG_QUEST_ID_HIGH)
            xpValue = player->CalculateQuestRewardXP(quest);
    }
};

void AddEverQuestPlayerScripts()
{
    new EverQuest_PlayerScript();
}
