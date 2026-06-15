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

#include "CombatManager.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "Unit.h"

#include "EverQuest.h"

using namespace std;

// Intercept charm spells because otherwise the player gets stuck in combat
class EverQuest_CharmAuraScript: public AuraScript
{
    PrepareAuraScript(EverQuest_CharmAuraScript);

    void HandleAfterApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (EverQuest->IsEnabled == false)
            return;

        Unit* target = GetTarget();
        if (target == nullptr)
            return;

        target->GetCombatManager().RevalidateCombat();

        if (Unit* caster = GetCaster())
            caster->GetCombatManager().RevalidateCombat();
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(EverQuest_CharmAuraScript::HandleAfterApply, EFFECT_FIRST_FOUND, SPELL_AURA_MOD_CHARM, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddEverQuestCharmAuraScripts()
{
    RegisterSpellScript(EverQuest_CharmAuraScript);
}
