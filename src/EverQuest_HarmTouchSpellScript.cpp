//  Author: Nathan Handley (nathanhandley@protonmail.com)
//  Copyright (c) 2026 Nathan Handley
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

#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "Unit.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_HarmTouchSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_HarmTouchSpellScript);

    void SetDamageFromMaxHealth()
    {
        if (EverQuest->IsEnabled == false)
            return;
        Unit* caster = GetCaster();
        if (caster == nullptr)
            return;
        Unit* target = GetHitUnit();
        if (target == nullptr)
            return;

        // Against players and what they own or charm, deal a percent of the target's max health instead of the caster's
        uint32 damage = caster->GetMaxHealth();
        if (target->GetCharmerOrOwnerPlayerOrPlayerItself() != nullptr)
            damage = target->CountPctFromMaxHealth(int32(min<uint32>(EverQuest->ConfigSystemHarmTouchPlayerPvPDamagePercent, 100)));
        SetHitDamage(int32(min<uint32>(damage, uint32(numeric_limits<int32>::max()))));
    }

    void Register() override
    {
        OnHit += SpellHitFn(EverQuest_HarmTouchSpellScript::SetDamageFromMaxHealth);
    }
};

void AddEverQuestHarmTouchSpellScripts()
{
    RegisterSpellScript(EverQuest_HarmTouchSpellScript);
}
