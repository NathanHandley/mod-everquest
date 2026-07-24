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

// EQ targeted AE spells also land on the caster, but only when the caster is a player (TAKP Mob::SpellFinished, 'affect_caster = !IsNPC()').
// The converter chains a caster-targeted "Self Hit" copy of such spells that fires on cast, and this script clears the caster target when an NPC cast it so NPCs never self-hit
class EverQuest_PlayerCasterOnlySpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_PlayerCasterOnlySpellScript);

    void ClearTargetIfNPCCaster(WorldObject*& target)
    {
        Unit* caster = GetCaster();
        if (caster == nullptr)
            return;
        if (caster->IsPlayer() == false)
            target = nullptr;
    }

    void Register() override
    {
        OnObjectTargetSelect += SpellObjectTargetSelectFn(EverQuest_PlayerCasterOnlySpellScript::ClearTargetIfNPCCaster, EFFECT_ALL, TARGET_UNIT_CASTER);
    }
};

void AddEverQuestPlayerCasterOnlySpellScripts()
{
    RegisterSpellScript(EverQuest_PlayerCasterOnlySpellScript);
}
