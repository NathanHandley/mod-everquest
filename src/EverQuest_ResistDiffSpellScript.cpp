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

#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellScript.h"
#include "Unit.h"

#include "EverQuest.h"

using namespace std;

// EQ spells can be modified by "ResistDiff" which modifies the resist roll (see TAKP Mob::CheckResistSpell)
// Since Wow has no per-spell hit modifier, there is a hidden hit chance aura on the caster sized from resistdiff then removes it after
// the cast completes. The aura is really short in case the cast was aborted
// It also rolls each target's EQ resist (the target's resist of the spell's EQ resist type), which WoW has no per-target hook for
class EverQuest_ResistDiffSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_ResistDiffSpellScript);

    void HandleBeforeCast()
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (EverQuest->ConfigSystemResistAdjustmentSpellID == 0)
            return;

        Unit* caster = GetCaster();
        if (caster == nullptr)
            return;

        // Only EQ spells
        uint32 spellID = GetSpellInfo()->Id;
        if (EverQuest->IsSpellAnEQSpell(spellID) == false)
            return;

        // Skip spells with no resist modifier
        int32 resistDiff = EverQuest->GetSpellDataForSpellID(spellID).ResistDiff;
        if (resistDiff == 0)
            return;

        // EQ resist rolls are 0-200, so 2 points of resist modifier is about 1% of land chance.  Negative ResistDiff makes an EQ spell harder to resist (lifetaps, druid magic DoTs, dragon breath AoEs)
        int32 hitChanceMod = (-resistDiff) / 2;
        if (hitChanceMod > 100)
            hitChanceMod = 100;
        else if (hitChanceMod < -100)
            hitChanceMod = -100;
        caster->CastCustomSpell(EverQuest->ConfigSystemResistAdjustmentSpellID, SPELLVALUE_BASE_POINT0, hitChanceMod, caster, true);
        _addedResistAdjustment = true;
    }

    void HandleAfterCast()
    {
        // Only take off the adjustment this cast put on, since another cast still in progress may own the one that's there
        if (_addedResistAdjustment == false || EverQuest->ConfigSystemResistAdjustmentSpellID == 0)
            return;
        _addedResistAdjustment = false;
        Unit* caster = GetCaster();
        if (caster == nullptr)
            return;
        caster->RemoveAurasDueToSpell(EverQuest->ConfigSystemResistAdjustmentSpellID);
    }

    // Runs after the targets and the core's hit rolls are settled, but before the client is told the outcome, so a resist shows as one
    void HandleOnCast()
    {
        if (EverQuest->IsEnabled == false)
            return;
        SpellInfo const* spellInfo = GetSpellInfo();
        if (EverQuest->GetSpellDataForSpellID(spellInfo->Id).EQResistType == 0)
            return;

        // Spells carrying effects for a parent spell already had their resist rolled with the parent
        if (spellInfo->HasAttribute(SPELL_ATTR7_NO_ATTACK_MISS) || spellInfo->HasAttribute(SPELL_ATTR3_ALWAYS_HIT))
            return;

        Spell* spell = GetSpell();
        Unit* caster = spell->GetOriginalCaster();
        if (caster == nullptr)
            caster = GetCaster();
        if (caster == nullptr)
            return;

        for (TargetInfo& targetInfo : *spell->GetUniqueTargetInfo())
        {
            // Skip targets that already missed, and ones the mod already dropped from the spell
            if (targetInfo.missCondition != SPELL_MISS_NONE || targetInfo.processed == true || targetInfo.effectMask == 0)
                continue;
            Unit* target = ObjectAccessor::GetUnit(*caster, targetInfo.targetGUID);
            if (target == nullptr || caster->IsFriendlyTo(target) == true)
                continue;
            if (EverQuest->RollEQResist(caster, target, spellInfo) == true)
                targetInfo.missCondition = SPELL_MISS_RESIST;
        }
    }

    bool _addedResistAdjustment = false;

    void Register() override
    {
        BeforeCast += SpellCastFn(EverQuest_ResistDiffSpellScript::HandleBeforeCast);
        OnCast += SpellCastFn(EverQuest_ResistDiffSpellScript::HandleOnCast);
        AfterCast += SpellCastFn(EverQuest_ResistDiffSpellScript::HandleAfterCast);
    }
};

void AddEverQuestResistDiffSpellScripts()
{
    RegisterSpellScript(EverQuest_ResistDiffSpellScript);
}
