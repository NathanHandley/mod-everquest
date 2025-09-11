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
            if (aurApp->GetBase()->GetId() < EverQuest->ConfigSystemSpellDBCIDMin || aurApp->GetBase()->GetId() > EverQuest->ConfigSystemSpellDBCIDMax)
                return;
            if (aurApp->GetBase()->GetEffect(0) == nullptr)
                return;
            if (aurApp->GetBase()->GetEffect(0)->GetAuraType() != SPELL_AURA_DUMMY)
                return;

            // Handle gate recall
            if (mode == AURA_REMOVE_BY_CANCEL && aurApp->GetBase()->GetEffect(0)->GetMiscValue() == EQ_SPELLDUMMYTYPE_GATE)
            {
                EverQuest->SendPlayerToLastGate(unit->ToPlayer());
                return;
            }
        }
    }

    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
    {
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
