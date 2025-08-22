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

    void OnAuraRemove(Unit* unit, AuraApplication* aurApp, AuraRemoveMode mode) override
    {
        if (unit == nullptr)
            return;
        if (unit->IsPlayer())
        {
            if (aurApp == nullptr)
                return;
            if (aurApp->GetBase() == nullptr)
                return;
            if (aurApp->GetBase()->GetId() < CONFIG_SPELLS_EQ_SPELLDBC_ID_MIN || aurApp->GetBase()->GetId() > CONFIG_SPELLS_EQ_SPELLDBC_ID_MAX)
                return;
            if (aurApp->GetBase()->GetEffect(0) == nullptr)
                return;
            if (aurApp->GetBase()->GetEffect(0)->GetAuraType() != SPELL_AURA_DUMMY)
                return;

            // Handle gate recall
            if (mode == AURA_REMOVE_BY_CANCEL && aurApp->GetBase()->GetEffect(0)->GetMiscValue() == 3)
            {
                EverQuest->SendPlayerToLastGate(unit->ToPlayer());
                return;
            }
        }
    }
};

void AddEverQuestUnitScripts()
{
    new EverQuest_UnitScript();
}
