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
#include "Unit.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_UnitScript : public UnitScript
{
public:
    EverQuest_UnitScript() : UnitScript("EverQuest_UnitScript") {}

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (!unit->IsPlayer())
            return;

        Player* player = unit->ToPlayer();
        uint32 spellID = aura->GetId();
        if (EverQuest->IsSpellAnEQBardSong(spellID) == true && EverQuest->ConfigBardMaxConcurrentSongs != 0)
        {
            auto& queue = EverQuest->PlayerCasterConcurrentBardSongs[player->GetGUID()];

            // Refresh the cast
            auto it = std::find(queue.begin(), queue.end(), spellID);
            if (it != queue.end())
                queue.erase(it);
            queue.push_back(spellID);

            // Enforce a limit
            while (queue.size() > EverQuest->ConfigBardMaxConcurrentSongs)
            {
                uint32 oldest = queue.front();
                queue.pop_front();
                player->RemoveAurasDueToSpell(oldest);
            }
        }
    }

    void OnAuraRemove(Unit* unit, AuraApplication* aurApp, AuraRemoveMode mode) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (unit == nullptr)
            return;
        if (unit->IsPlayer())
        {
            if (aurApp == nullptr)
                return;
            if (aurApp->GetBase() == nullptr)
                return;
            if (aurApp->GetBase()->GetId() < EverQuest->ConfigSystemSpellDBCIDMin || aurApp->GetBase()->GetId() > EverQuest->ConfigSystemSpellDBCIDMax)
                return;
            if (aurApp->GetBase()->GetEffect(0) == nullptr)
                return;
            if (aurApp->GetBase()->GetEffect(0)->GetAuraType() != SPELL_AURA_DUMMY && aurApp->GetBase()->GetEffect(0)->GetAuraType() != SPELL_AURA_PERIODIC_DUMMY)
                return;

            // Handle gate recall
            if (mode == AURA_REMOVE_BY_CANCEL && aurApp->GetBase()->GetEffect(0)->GetMiscValue() == EQ_SPELLDUMMYTYPE_GATE)
            {
                EverQuest->SendPlayerToLastGate(unit->ToPlayer());
                return;
            }

            // Handle bard songs
            uint32 spellID = aurApp->GetBase()->GetId();
            if (EverQuest->IsSpellAnEQBardSong(spellID) == true)
            {
                Player* player = unit->ToPlayer();

                // Concurrence restrict                
                if (EverQuest->ConfigBardMaxConcurrentSongs != 0)
                {
                    
                    auto& queue = EverQuest->PlayerCasterConcurrentBardSongs[player->GetGUID()];
                    auto it = std::find(queue.begin(), queue.end(), spellID);
                    if (it != queue.end())
                        queue.erase(it);
                }
            }
        }
    }

    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (target == nullptr)
            return;
        if (attacker == nullptr)
            return;
        if (damage <= 0)
            return;

        // Check auras for any melee attacker behavior
        Unit::AuraApplicationMap& auraMap = target->GetAppliedAuras();
        for (Unit::AuraApplicationMap::iterator iter = auraMap.begin(); iter != auraMap.end(); ++iter)
        {
            AuraApplication* aurApp = iter->second;
            if (aurApp == nullptr)
                continue;
            Aura* aura = aurApp->GetBase();
            if (aura == nullptr)
                continue;
            uint32 spellID = aura->GetId();
            if (spellID < EverQuest->ConfigSystemSpellDBCIDMin || spellID > EverQuest->ConfigSystemSpellDBCIDMax)
                continue;
            if (EverQuest->IsSpellAnEQSpell(spellID) == false)
                continue;
            const EverQuestSpell& curSpell = EverQuest->GetSpellDataForSpellID(spellID);
            if (curSpell.SpellIDCastOnMeleeAttacker == 0)
                continue;
            attacker->CastSpell(attacker, curSpell.SpellIDCastOnMeleeAttacker);
        }
    }
};

void AddEverQuestUnitScripts()
{
    new EverQuest_UnitScript();
}
