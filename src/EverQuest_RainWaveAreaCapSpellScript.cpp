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
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "Unit.h"

#include "EverQuest.h"

using namespace std;

// An EverQuest "rain" re-lands the whole spell at the spot it was aimed at once per wave.  The first wave is the player's own
// area cast, so the core splits its damage past ten targets like any other area spell (Spell::DoAllEffectOnLaunchTarget, which
// does "m_damage = m_damage * 10 / targetAmount").  The follow-up waves arrive as single-target casts, one per unit standing in
// the rain, triggered by the hidden cloud aura the converter chains off the cast - so the core sees one target on each of them
// and never splits them.  Without this a rain dropped on twenty targets would split its first wave and pay full damage on the rest.
//
// This restores the same split for the follow-up waves.  The number of units in the rain is exactly the number of applications
// of the cloud aura that triggered the cast, so no extra bookkeeping is needed.  The converter attaches this script to the
// generated "<Name> Wave" spells only.
class EverQuest_RainWaveAreaCapSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_RainWaveAreaCapSpellScript);

    // Mirrors the hardcoded cap in Spell::DoAllEffectOnLaunchTarget.  If the core's value ever moves, move this with it
    static const uint32 CORE_AREA_DAMAGE_TARGET_CAP = 10;

    void HandleOnHit()
    {
        if (EverQuest->IsEnabled == false)
            return;

        int32 damage = GetHitDamage();
        if (damage <= 0)
            return;

        // The core only splits area damage when a player cast it, so the waves follow the same rule
        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsPlayer() == false)
            return;

        // Every follow-up wave is triggered by the rain's cloud aura, and that aura sits on each unit standing in the rain
        SpellInfo const* cloudSpellInfo = GetSpell()->GetTriggeredByAuraSpellInfo();
        if (cloudSpellInfo == nullptr)
            return;
        Unit* target = GetHitUnit();
        if (target == nullptr)
            return;
        Aura* cloudAura = target->GetAura(cloudSpellInfo->Id, caster->GetGUID());
        if (cloudAura == nullptr)
            return;

        uint32 targetCount = uint32(cloudAura->GetApplicationMap().size());
        if (targetCount <= CORE_AREA_DAMAGE_TARGET_CAP)
            return;

        SetHitDamage(int32((int64(damage) * int64(CORE_AREA_DAMAGE_TARGET_CAP)) / int64(targetCount)));
    }

    void Register() override
    {
        OnHit += SpellHitFn(EverQuest_RainWaveAreaCapSpellScript::HandleOnHit);
    }
};

void AddEverQuestRainWaveAreaCapSpellScripts()
{
    RegisterSpellScript(EverQuest_RainWaveAreaCapSpellScript);
}
