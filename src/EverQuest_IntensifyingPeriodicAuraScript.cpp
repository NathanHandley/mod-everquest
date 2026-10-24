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
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellScript.h"

#include "EverQuest.h"

#include <cmath>

using namespace std;

class EverQuest_IntensifyingPeriodicAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_IntensifyingPeriodicAuraScript);

    void CaptureBaseAmount(AuraEffect const* aurEff, int32& amount, bool& /*canBeRecalculated*/)
    {
        uint8 effectIndex = aurEff->GetEffIndex();
        if (effectIndex >= MAX_SPELL_EFFECTS)
            return;
        BaseAmountByEffectIndex[effectIndex] = amount;

        // A new or recast effect starts the ramp over (a recast restarts the tick count before the amount is recalculated).  A tick count the shaman
        // class aura just restarted is also at 0 until the next tick, so a recalculation then only starts over if the duration was put back to full,
        // which a recast does and the time given back almost never does
        if (aurEff->GetTickNumber() == 0)
        {
            if (IsTickRestartPendingByEffectIndex[effectIndex] == true && GetDuration() < GetMaxDuration())
                return;
            TicksBeforeRestartByEffectIndex[effectIndex] = 0;
            IsTickRestartPendingByEffectIndex[effectIndex] = false;
        }
    }

public:
    // The shaman class aura restarts the tick count of an effect it gives time back to once that count runs out, and the ramp carries on from the ticks
    // already done rather than starting over
    void NoteTicksRestarting(AuraEffect const* aurEff)
    {
        uint8 effectIndex = aurEff->GetEffIndex();
        if (effectIndex >= MAX_SPELL_EFFECTS)
            return;
        TicksBeforeRestartByEffectIndex[effectIndex] += aurEff->GetTickNumber();
        IsTickRestartPendingByEffectIndex[effectIndex] = true;
    }

private:

    void ApplyRampForTick(AuraEffect* aurEff)
    {
        if (EverQuest->IsEnabled == false)
            return;

        uint8 effectIndex = aurEff->GetEffIndex();
        if (effectIndex >= MAX_SPELL_EFFECTS)
            return;

        // The restarted count is ticking again
        IsTickRestartPendingByEffectIndex[effectIndex] = false;

        float rampStartMultiplier = EverQuest->GetSpellIntensifyingRampStartMultiplier(GetId(), effectIndex);
        if (rampStartMultiplier <= 0.0f)
            return;

        int32 baseAmount = BaseAmountByEffectIndex[effectIndex];
        if (baseAmount == 0)
            return;

        // The last tick is as far above the average as the first tick is below it, so the ramp walks from the stored fraction to (2 - that fraction)
        float rampMultiplier = 1.0f;
        int32 totalTicks = aurEff->GetTotalTicks();
        if (totalTicks > 1)
        {
            float tickProgress = float(TicksBeforeRestartByEffectIndex[effectIndex] + aurEff->GetTickNumber() - 1) / float(totalTicks - 1);
            if (tickProgress > 1.0f)
                tickProgress = 1.0f;
            rampMultiplier = rampStartMultiplier + ((2.0f - (2.0f * rampStartMultiplier)) * tickProgress);
        }

        // Rounding can wipe out a small opening tick, and a tick that does nothing at all reads as the spell being broken
        int32 rampedAmount = int32(std::lround(float(baseAmount) * rampMultiplier));
        if (rampedAmount == 0)
            rampedAmount = baseAmount > 0 ? 1 : -1;
        aurEff->SetAmount(rampedAmount);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(EverQuest_IntensifyingPeriodicAuraScript::CaptureBaseAmount, EFFECT_ALL, SPELL_AURA_ANY);
        OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(EverQuest_IntensifyingPeriodicAuraScript::ApplyRampForTick, EFFECT_ALL, SPELL_AURA_ANY);
    }

    int32 BaseAmountByEffectIndex[MAX_SPELL_EFFECTS] = { 0, 0, 0 };
    uint32 TicksBeforeRestartByEffectIndex[MAX_SPELL_EFFECTS] = { 0, 0, 0 };
    bool IsTickRestartPendingByEffectIndex[MAX_SPELL_EFFECTS] = { };
};

void NoteEverQuestIntensifyingPeriodicTicksRestarting(AuraEffect const* auraEffect)
{
    EverQuest_IntensifyingPeriodicAuraScript* script = auraEffect->GetBase()->GetScript<EverQuest_IntensifyingPeriodicAuraScript>("EverQuest_IntensifyingPeriodicAuraScript");
    if (script != nullptr)
        script->NoteTicksRestarting(auraEffect);
}

void AddEverQuestIntensifyingPeriodicAuraScripts()
{
    RegisterSpellScript(EverQuest_IntensifyingPeriodicAuraScript);
}
