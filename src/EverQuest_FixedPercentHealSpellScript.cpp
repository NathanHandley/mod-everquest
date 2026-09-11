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

class EverQuest_FixedPercentHealSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_FixedPercentHealSpellScript);

    void CapHealToBasePercent()
    {
        if (EverQuest->IsEnabled == false)
            return;
        Unit* healedUnit = GetHitUnit();
        if (healedUnit == nullptr)
            return;
        int32 heal = GetHitHeal();
        if (heal <= 0)
            return;

        // Percents come straight from the spell data with no caster, so no spell mod or level scaling can have raised them
        SpellInfo const* spellInfo = GetSpellInfo();
        int32 maxHeal = 0;
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->Effects[i].Effect != SPELL_EFFECT_HEAL_PCT)
                continue;
            maxHeal += int32(healedUnit->CountPctFromMaxHealth(spellInfo->Effects[i].CalcValue()));
        }
        if (heal > maxHeal)
            SetHitHeal(maxHeal);
    }

    void Register() override
    {
        // OnHit runs after the launch has computed the heal (bonuses included) and before Spell::DoAllEffectOnTarget hands it to HealBySpell
        OnHit += SpellHitFn(EverQuest_FixedPercentHealSpellScript::CapHealToBasePercent);
    }
};

void AddEverQuestFixedPercentHealSpellScripts()
{
    RegisterSpellScript(EverQuest_FixedPercentHealSpellScript);
}
