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

#include "DynamicObject.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellScript.h"
#include "Unit.h"

#include "EverQuest.h"

using namespace std;

// EQ blind (SE_Blind).  The screen effect blacks out a player's view, and a blinded creature gets a separate hidden confuse so it wanders (see
// EverQuestMod::IsBlindWanderAllowedForUnit), which keeps any confuse off players and bosses.  Every 6 second tick gets TAKP's blind save, a chance
// at a fresh resist roll that breaks the blind if it resists
class EverQuest_BlindAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_BlindAuraScript);

    void HandleBlindApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (EverQuest->IsEnabled == false)
            return;
        EverQuest->ApplyBlindWander(GetTarget(), GetAura());
    }

    // Not gated on the mod being enabled, so a wander never outlives its blind
    void HandleBlindRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        EverQuest->RemoveBlindWanderIfNoBlindRemains(GetTarget(), GetAura());
    }

    void CalcBlindPeriodic(AuraEffect const* /*aurEff*/, bool& isPeriodic, int32& amplitude)
    {
        isPeriodic = true;
        amplitude = EQ_BLIND_BREAK_CHECK_INTERVAL_IN_MS;
    }

    void HandleBlindTick(AuraEffect const* /*aurEff*/)
    {
        if (EverQuest->IsEnabled == false)
            return;
        Unit* target = GetTarget();
        if (target == nullptr || target->IsAlive() == false)
            return;
        if (EverQuest->RollEQBlindBreakCheck(GetCaster(), target, GetSpellInfo()) == true)
        {
            Remove(AURA_REMOVE_BY_ENEMY_SPELL);
            return;
        }
        EverQuest->ApplyBlindWander(target, GetAura());
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(EverQuest_BlindAuraScript::HandleBlindApply, EFFECT_ALL, SPELL_AURA_SCREEN_EFFECT, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(EverQuest_BlindAuraScript::HandleBlindRemove, EFFECT_ALL, SPELL_AURA_SCREEN_EFFECT, AURA_EFFECT_HANDLE_REAL);
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(EverQuest_BlindAuraScript::CalcBlindPeriodic, EFFECT_ALL, SPELL_AURA_SCREEN_EFFECT);
        OnEffectPeriodic += AuraEffectPeriodicFn(EverQuest_BlindAuraScript::HandleBlindTick, EFFECT_ALL, SPELL_AURA_SCREEN_EFFECT);
    }
};

// EQ memory blur (SE_WipeHateList) with no duration, rolled once as it hits
class EverQuest_WipeHateListSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_WipeHateListSpellScript);

    void HandleDummy(SpellEffIndex effIndex)
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (GetSpellInfo()->Effects[effIndex].MiscValue != EQ_SPELLDUMMYTYPE_WIPEHATELIST)
            return;
        Unit* target = GetHitUnit();
        if (target == nullptr)
            return;
        Unit* caster = GetOriginalCaster() != nullptr ? GetOriginalCaster() : GetCaster();
        EverQuest->RollWipeHateListOnUnit(caster, target, EverQuest->GetWipeHateListChanceOnLanding(target, GetEffectValue()));
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(EverQuest_WipeHateListSpellScript::HandleDummy, EFFECT_ALL, SPELL_EFFECT_DUMMY);
    }
};

// EQ memory blur on a lasting spell.  It rolls as the aura lands (a mesmerize refreshing itself doesn't land again, so it doesn't roll, like TAKP),
// and the converter sets MiscValueB on the ones that don't mesmerize, which roll the plain chance again every tick.  A creature that forgets under a
// mesmerize stays mesmerized (the forgetting here is an evade, which would wake it and send it home healed), and only heads home if the mesmerize
// runs out.  One broken by damage or a dispel leaves it fighting whoever broke it, much like TAKP where only they would be on its fresh hate list
class EverQuest_WipeHateListAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_WipeHateListAuraScript);

    bool ForgetsWhenMesmerizeRunsOut = false;

    bool IsTickingWipeHateList(AuraEffect const* aurEff)
    {
        return aurEff->GetMiscValue() == EQ_SPELLDUMMYTYPE_WIPEHATELIST && aurEff->GetMiscValueB() != 0;
    }

    void HandleApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        if (EverQuest->IsEnabled == false || aurEff->GetMiscValue() != EQ_SPELLDUMMYTYPE_WIPEHATELIST)
            return;
        Unit* target = GetTarget();
        if (target == nullptr)
            return;
        int32 chance = EverQuest->GetWipeHateListChanceOnLanding(target, aurEff->GetAmount());
        if (IsTickingWipeHateList(aurEff) == true || target->IsCreature() == false)
            EverQuest->RollWipeHateListOnUnit(GetCaster(), target, chance);
        else if (roll_chance_i(chance) == true)
            ForgetsWhenMesmerizeRunsOut = true;
    }

    void HandleRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        if (ForgetsWhenMesmerizeRunsOut == false || EverQuest->IsEnabled == false || aurEff->GetMiscValue() != EQ_SPELLDUMMYTYPE_WIPEHATELIST)
            return;
        AuraApplication const* targetApplication = GetTargetApplication();
        if (targetApplication == nullptr || targetApplication->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
            return;
        EverQuest->RollWipeHateListOnUnit(GetCaster(), GetTarget(), 100);
    }

    void CalcTickPeriodic(AuraEffect const* aurEff, bool& isPeriodic, int32& amplitude)
    {
        if (IsTickingWipeHateList(aurEff) == false)
            return;
        isPeriodic = true;
        amplitude = EQ_WIPE_HATE_LIST_TICK_INTERVAL_IN_MS;
    }

    void HandleTick(AuraEffect const* aurEff)
    {
        if (EverQuest->IsEnabled == false || IsTickingWipeHateList(aurEff) == false)
            return;
        EverQuest->RollWipeHateListOnUnit(GetCaster(), GetTarget(), aurEff->GetAmount());
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(EverQuest_WipeHateListAuraScript::HandleApply, EFFECT_ALL, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(EverQuest_WipeHateListAuraScript::HandleRemove, EFFECT_ALL, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(EverQuest_WipeHateListAuraScript::CalcTickPeriodic, EFFECT_ALL, SPELL_AURA_DUMMY);
        OnEffectPeriodic += AuraEffectPeriodicFn(EverQuest_WipeHateListAuraScript::HandleTick, EFFECT_ALL, SPELL_AURA_DUMMY);
    }
};

// EQ lull (SE_Harmony).  The assist checks read the radius straight off the aura (EverQuestMod::IsCreatureAssistBlockedByHarmony), and this only
// keeps count of lulled creatures so those checks cost nothing while there are none.  Not gated on the mod being enabled, so the count stays paired
class EverQuest_HarmonyAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_HarmonyAuraScript);

    void HandleApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        if (aurEff->GetMiscValue() != EQ_SPELLDUMMYTYPE_HARMONY)
            return;
        Unit* target = GetTarget();
        if (target != nullptr && target->IsCreature() == true)
            EverQuest->HarmonyAuraCreatureCount.fetch_add(1);
    }

    void HandleRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        if (aurEff->GetMiscValue() != EQ_SPELLDUMMYTYPE_HARMONY)
            return;
        Unit* target = GetTarget();
        if (target == nullptr || target->IsCreature() == false)
            return;
        uint32 harmonyCount = EverQuest->HarmonyAuraCreatureCount.load();
        while (harmonyCount > 0 && EverQuest->HarmonyAuraCreatureCount.compare_exchange_weak(harmonyCount, harmonyCount - 1) == false)
        {
        }
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(EverQuest_HarmonyAuraScript::HandleApply, EFFECT_ALL, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(EverQuest_HarmonyAuraScript::HandleRemove, EFFECT_ALL, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// EQ lull (SE_ChangeFrenzyRad), the aggro half.  The converted SPELL_AURA_MOD_DETECT_RANGE carries the lull radius in its misc value, and its amount
// (which the core adds to the creature's aggro radius) is set here so only the smallest lull on a creature counts, like TAKP.  Not gated on the mod
// being enabled, so amounts are always put right as lulls come and go
class EverQuest_LullAggroRangeAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_LullAggroRangeAuraScript);

    void CalculateAmount(AuraEffect const* aurEff, int32& amount, bool& canBeRecalculated)
    {
        canBeRecalculated = true;
        amount = EverQuest->GetLullAggroRangeAmountForNewEffect(GetUnitOwner(), aurEff);
    }

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        EverQuest->RefreshLullAggroRanges(GetTarget(), nullptr);
    }

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        EverQuest->RefreshLullAggroRanges(GetTarget(), GetAura());
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(EverQuest_LullAggroRangeAuraScript::CalculateAmount, EFFECT_ALL, SPELL_AURA_MOD_DETECT_RANGE);
        AfterEffectApply += AuraEffectApplyFn(EverQuest_LullAggroRangeAuraScript::HandleApply, EFFECT_ALL, SPELL_AURA_MOD_DETECT_RANGE, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(EverQuest_LullAggroRangeAuraScript::HandleRemove, EFFECT_ALL, SPELL_AURA_MOD_DETECT_RANGE, AURA_EFFECT_HANDLE_REAL);
    }
};

// EQ call pet (SE_CallPet).  The spell's own teleport moves the pet, and this refuses the cast while something is fighting the pet, like TAKP
class EverQuest_CallPetSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_CallPetSpellScript);

    SpellCastResult CheckCast()
    {
        if (EverQuest->IsEnabled == false)
            return SPELL_CAST_OK;
        return EverQuest->GetCallPetCastResult(GetCaster());
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(EverQuest_CallPetSpellScript::CheckCast);
    }
};

// EQ eye of zomm (SE_EyeOfZomm), converted to work like Eye of Kilrogg (126): the channel summons an eye the caster possesses.  This does what
// Eye of Kilrogg's aura script (spell_warl_eye_of_kilrogg) does, setting the pet aside for the channel and ending the eye with it, but only
// unsummons the charm when it really is this spell's eye (that script assumes whatever is charmed is a summon)
class EverQuest_EyeOfZommSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_EyeOfZommSpellScript);

    // A possessed summon needs a player owner, so a creature can't cast it
    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsPlayer() == false)
            return SPELL_FAILED_DONT_REPORT;
        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(EverQuest_EyeOfZommSpellScript::CheckCast);
    }
};

class EverQuest_EyeOfZommAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_EyeOfZommAuraScript);

    uint32 GetEyeCreatureEntry()
    {
        for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
            if (GetSpellInfo()->Effects[effectIndex].Effect == SPELL_EFFECT_SUMMON)
                return uint32(GetSpellInfo()->Effects[effectIndex].MiscValue);
        return 0;
    }

    // Not gated on the mod being enabled, so the pet always comes back and the eye always goes
    void HandleApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        if (aurEff->GetMiscValue() != EQ_SPELLDUMMYTYPE_EYEOFZOMM)
            return;
        Player* player = GetTarget()->ToPlayer();
        if (player == nullptr)
            return;
        player->UnsummonPetTemporaryIfAny();

        // The eye was summoned with the spell's endless DBC duration, so it's given the channel's own EQ duration (and a little slack) as a backstop
        int32 durationInMS = GetAura()->GetMaxDuration();
        Unit* charm = player->GetCharm();
        uint32 eyeCreatureEntry = GetEyeCreatureEntry();
        if (durationInMS > 0 && charm != nullptr && eyeCreatureEntry != 0 && charm->GetEntry() == eyeCreatureEntry)
            if (TempSummon* eye = charm->ToTempSummon())
                eye->SetTimer(uint32(durationInMS) + EQ_EYE_OF_ZOMM_LIFETIME_SLACK_IN_MS);
    }

    void HandleRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        if (aurEff->GetMiscValue() != EQ_SPELLDUMMYTYPE_EYEOFZOMM)
            return;
        Player* player = GetTarget()->ToPlayer();
        if (player == nullptr)
            return;
        Unit* charm = player->GetCharm();
        uint32 eyeCreatureEntry = GetEyeCreatureEntry();
        if (charm != nullptr && eyeCreatureEntry != 0 && charm->GetEntry() == eyeCreatureEntry)
            if (TempSummon* eye = charm->ToTempSummon())
                eye->UnSummon();
        player->ResummonPetTemporaryUnSummonedIfAny();
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(EverQuest_EyeOfZommAuraScript::HandleApply, EFFECT_ALL, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(EverQuest_EyeOfZommAuraScript::HandleRemove, EFFECT_ALL, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// EQ telescope (SE_MagnifyVision on a spell with nothing else), converted to a channeled far sight like Eagle Eye.  The far sight object takes the
// spell's endless DBC duration, so it would expire on its first update; this gives it the caster aura's EQ duration instead, and takes it away
// with that aura so the view always comes back
class EverQuest_TelescopeAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_TelescopeAuraScript);

    void HandleApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        if (aurEff->GetMiscValue() != EQ_SPELLDUMMYTYPE_TELESCOPE)
            return;
        int32 durationInMS = GetAura()->GetMaxDuration();
        if (durationInMS <= 0)
            return;
        if (DynamicObject* farSightObject = GetTarget()->GetDynObject(GetId()))
            farSightObject->SetDuration(durationInMS);
    }

    void HandleRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        if (aurEff->GetMiscValue() != EQ_SPELLDUMMYTYPE_TELESCOPE)
            return;
        GetTarget()->RemoveDynObject(GetId());
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(EverQuest_TelescopeAuraScript::HandleApply, EFFECT_ALL, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(EverQuest_TelescopeAuraScript::HandleRemove, EFFECT_ALL, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddEverQuestSpellEffectScripts()
{
    RegisterSpellScript(EverQuest_TelescopeAuraScript);
    RegisterSpellScript(EverQuest_LullAggroRangeAuraScript);
    RegisterSpellAndAuraScriptPair(EverQuest_EyeOfZommSpellScript, EverQuest_EyeOfZommAuraScript);
    RegisterSpellScript(EverQuest_BlindAuraScript);
    RegisterSpellScript(EverQuest_WipeHateListSpellScript);
    RegisterSpellScript(EverQuest_WipeHateListAuraScript);
    RegisterSpellScript(EverQuest_HarmonyAuraScript);
    RegisterSpellScript(EverQuest_CallPetSpellScript);
}
