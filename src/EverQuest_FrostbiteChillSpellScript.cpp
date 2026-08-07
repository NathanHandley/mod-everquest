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
#include "SpellScript.h"
#include "Unit.h"

#include "EverQuest.h"

using namespace std;

// Mage Frostbite talent works off "Chill" effects, and no EQ spell carries one.  The converter attaches this script to every EQ spell block that deals direct frost damage,
// and any rank of Frostbite makes those hits chill the target.  The chill used is the mage "Chilled" (Frost Armor's, spell 6136)
class EverQuest_FrostbiteChillSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_FrostbiteChillSpellScript);

    static const uint32 CHILLED_SPELL_ID = 6136;
    static const uint32 FROSTBITE_TALENT_EQ_SPELL_ID_RANK1 = 11071;
    static const uint32 FROSTBITE_TALENT_EQ_SPELL_ID_RANK2 = 12496;
    static const uint32 FROSTBITE_TALENT_EQ_SPELL_ID_RANK3 = 12497;

    void HandleOnHit()
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (EverQuest->ConfigSpellTalentAlignmentEnabled == false)
            return;

        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsPlayer() == false)
            return;

        // Any rank of Frostbite sets the chill
        if (caster->HasAura(FROSTBITE_TALENT_EQ_SPELL_ID_RANK1) == false
            && caster->HasAura(FROSTBITE_TALENT_EQ_SPELL_ID_RANK2) == false
            && caster->HasAura(FROSTBITE_TALENT_EQ_SPELL_ID_RANK3) == false)
            return;

        Unit* target = GetHitUnit();
        if (target == nullptr || target->IsAlive() == false)
            return;
        if (caster->IsFriendlyTo(target) == true)
            return;

        caster->CastSpell(target, CHILLED_SPELL_ID, true);
    }

    void Register() override
    {
        OnHit += SpellHitFn(EverQuest_FrostbiteChillSpellScript::HandleOnHit);
    }
};

void AddEverQuestFrostbiteChillSpellScripts()
{
    RegisterSpellScript(EverQuest_FrostbiteChillSpellScript);
}
