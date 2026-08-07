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

#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"

#include "EverQuest.h"
#include "EverQuest_SpellTalentAlignment.h"

#include <algorithm>

// Talents whose proc conditions are enforced in core spell scripts get their core script binding replaced (via converter
// spell_script_names rows) with the versions here, which keep the stock WOW behavior and add the EverQuest spell paths.
// The matching spell_proc family gates are dropped by converter override rows, so these scripts do the spell filtering.

static bool IsEQTalentInteractionEnabled()
{
    return EverQuest->IsEnabled == true && EverQuest->ConfigSpellTalentAlignmentEnabled == true;
}

// Replaces spell_mage_cold_snap in part: the core script only resets mage frost school cooldowns, this addition also
// resets the cooldowns of the player's EverQuest Frost spells.  The core binding stays, this script runs alongside it.
class EverQuest_ColdSnapSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_ColdSnapSpellScript);

    void HandleAfterCast()
    {
        if (IsEQTalentInteractionEnabled() == false)
            return;
        Player* caster = GetCaster() != nullptr ? GetCaster()->ToPlayer() : nullptr;
        if (caster == nullptr)
            return;

        // Mirror the core Cold Snap loop, but over the player's EverQuest spells
        std::vector<uint32> spellIDsToReset;
        PlayerSpellMap const& spellMap = caster->GetSpellMap();
        for (PlayerSpellMap::const_iterator spellIterator = spellMap.begin(); spellIterator != spellMap.end(); ++spellIterator)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellIterator->first);
            if (spellInfo == nullptr)
                continue;
            if (EverQuest->IsSpellAnEQSpell(spellInfo->Id) == false)
                continue;
            if ((spellInfo->GetSchoolMask() & SPELL_SCHOOL_MASK_FROST) == 0)
                continue;
            if (spellInfo->GetRecoveryTime() == 0 && spellInfo->CategoryRecoveryTime == 0)
                continue;
            spellIDsToReset.push_back(spellInfo->Id);
        }
        for (uint32 spellIDToReset : spellIDsToReset)
            caster->RemoveSpellCooldown(spellIDToReset, true);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(EverQuest_ColdSnapSpellScript::HandleAfterCast);
    }
};

// Replaces spell_mage_hot_streak.  The stock spell filter lived in the spell_proc family mask (Fireball, Fire Blast,
// Scorch, Frostfire Bolt, Living Bomb); the converter drops that mask so EverQuest spells can reach the aura, and this
// CheckProc re-imposes it for WOW spells while letting EverQuest direct Fire damage crits count toward the streak
class EverQuest_HotStreakAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_HotStreakAuraScript);

    static const uint32 EQ_SPELL_ID_MAGE_HOT_STREAK_PROC = 48108;

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* procSpellInfo = eventInfo.GetSpellInfo();
        if (procSpellInfo == nullptr)
            return false;

        // Stock Hot Streak trigger spells
        if (procSpellInfo->SpellFamilyName == SPELLFAMILY_MAGE)
            return (procSpellInfo->SpellFamilyFlags & flag96(0x13, 0x11000, 0)).IsEqual(0, 0, 0) == false;

        if (IsEQTalentInteractionEnabled() == false)
            return false;
        if (EverQuest->IsSpellAnEQSpell(procSpellInfo->Id) == false)
            return false;
        if ((procSpellInfo->SchoolMask & SPELL_SCHOOL_MASK_FIRE) == 0)
            return false;
        return EverQuestSpellTalentAlignment::DoesSpellInfoDealDirectDamage(procSpellInfo);
    }

    // Mirrors the core spell_mage_hot_streak counting
    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (!(eventInfo.GetHitMask() & PROC_EX_CRITICAL_HIT))
        {
            CritStreakCount = 0;
            return;
        }
        ++CritStreakCount;
        if (CritStreakCount >= 2)
        {
            CritStreakCount = 0;
            if (roll_chance_i(aurEff->GetAmount()))
                GetTarget()->CastSpell(GetTarget(), EQ_SPELL_ID_MAGE_HOT_STREAK_PROC, true, nullptr, aurEff);
        }
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(EverQuest_HotStreakAuraScript::CheckProc);
        OnEffectProc += AuraEffectProcFn(EverQuest_HotStreakAuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }

private:
    uint8 CritStreakCount = 0;
};

// Replaces spell_mage_brain_freeze.  The stock spell filter lived in the spell_proc family mask (the chill-capable frost
// spells); the converter drops it, this CheckProc re-imposes it for WOW spells and lets EverQuest direct Frost damage
// spells count while the player has any rank of Frostbite, since that talent is what makes those spells apply a chill
class EverQuest_BrainFreezeAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_BrainFreezeAuraScript);

    static const uint32 EQ_SPELL_ID_MAGE_IMPROVED_BLIZZARD_CHILLED = 12486;
    static const uint32 EQ_SPELL_ID_MAGE_FROSTBITE_TALENT_RANK1 = 11071;
    static const uint32 EQ_SPELL_ID_MAGE_FROSTBITE_TALENT_RANK2 = 12496;
    static const uint32 EQ_SPELL_ID_MAGE_FROSTBITE_TALENT_RANK3 = 12497;

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* procSpellInfo = eventInfo.GetSpellInfo();
        if (procSpellInfo == nullptr)
            return false;

        if (procSpellInfo->SpellFamilyName == SPELLFAMILY_MAGE)
        {
            // Stock rule: of the chill effect auras themselves, only Improved Blizzard's Chilled counts...
            if (procSpellInfo->SpellFamilyFlags[0] & 0x100000)
                return procSpellInfo->Id == EQ_SPELL_ID_MAGE_IMPROVED_BLIZZARD_CHILLED;
            // ...otherwise only the chill-capable frost spells count (Frostbolt, Frost Nova, Blizzard, Cone of Cold, Frostfire Bolt)
            return (procSpellInfo->SpellFamilyFlags & flag96(0x2E0, 0x1000, 0)).IsEqual(0, 0, 0) == false;
        }

        if (IsEQTalentInteractionEnabled() == false)
            return false;
        if (EverQuest->IsSpellAnEQSpell(procSpellInfo->Id) == false)
            return false;
        if ((procSpellInfo->SchoolMask & SPELL_SCHOOL_MASK_FROST) == 0)
            return false;
        if (EverQuestSpellTalentAlignment::DoesSpellInfoDealDirectDamage(procSpellInfo) == false)
            return false;
        Unit* auraHolder = GetTarget();
        if (auraHolder == nullptr)
            return false;
        return auraHolder->HasAura(EQ_SPELL_ID_MAGE_FROSTBITE_TALENT_RANK1) == true
            || auraHolder->HasAura(EQ_SPELL_ID_MAGE_FROSTBITE_TALENT_RANK2) == true
            || auraHolder->HasAura(EQ_SPELL_ID_MAGE_FROSTBITE_TALENT_RANK3) == true;
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(EverQuest_BrainFreezeAuraScript::CheckProc);
    }
};

// Warlock Nightfall has no stock script; its spell filter (Corruption, Drain Life) lived in the spell_proc family mask.
// The converter drops that mask so this CheckProc can re-impose it for WOW spells while letting the player's EverQuest
// Shadow damage spells carry the same chance to trigger Shadow Trance
class EverQuest_NightfallAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_NightfallAuraScript);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* procSpellInfo = eventInfo.GetSpellInfo();
        if (procSpellInfo == nullptr)
            return false;

        // Stock Nightfall trigger spells (Corruption, Drain Life)
        if (procSpellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK)
            return (procSpellInfo->SpellFamilyFlags & flag96(0xA, 0, 0)).IsEqual(0, 0, 0) == false;

        if (IsEQTalentInteractionEnabled() == false)
            return false;
        if (EverQuest->IsSpellAnEQSpell(procSpellInfo->Id) == false)
            return false;
        if ((procSpellInfo->SchoolMask & SPELL_SCHOOL_MASK_SHADOW) == 0)
            return false;
        return EverQuestSpellTalentAlignment::DoesSpellInfoDamage(procSpellInfo);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(EverQuest_NightfallAuraScript::CheckProc);
    }
};

// Warlock Shadow Embrace has no stock script; its spell filter (Shadow Bolt, Haunt) lived in the spell_proc family mask.
// The converter drops that mask so this CheckProc can re-impose it for WOW spells while letting the player's EverQuest
// lifetap spells apply the Shadow Embrace debuff too
class EverQuest_ShadowEmbraceAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ShadowEmbraceAuraScript);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* procSpellInfo = eventInfo.GetSpellInfo();
        if (procSpellInfo == nullptr)
            return false;

        // Stock Shadow Embrace trigger spells (Shadow Bolt, Haunt)
        if (procSpellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK)
            return (procSpellInfo->SpellFamilyFlags & flag96(0x1, 0x40000, 0)).IsEqual(0, 0, 0) == false;

        if (IsEQTalentInteractionEnabled() == false)
            return false;
        if (EverQuest->IsSpellAnEQSpell(procSpellInfo->Id) == false)
            return false;
        return EverQuestSpellTalentAlignment::DoesSpellInfoLeech(procSpellInfo);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(EverQuest_ShadowEmbraceAuraScript::CheckProc);
    }
};

// Warlock Eradication has no stock script; its spell filter (Corruption ticks) lived in the spell_proc family mask.
// The converter drops that mask so this CheckProc can re-impose it for WOW spells while letting the ticks of the
// player's EverQuest Shadow damage over time spells carry the same chance to grant the haste buff
class EverQuest_EradicationAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_EradicationAuraScript);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* procSpellInfo = eventInfo.GetSpellInfo();
        if (procSpellInfo == nullptr)
            return false;

        // Stock Eradication trigger spell (Corruption)
        if (procSpellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK)
            return (procSpellInfo->SpellFamilyFlags & flag96(0x2, 0, 0)).IsEqual(0, 0, 0) == false;

        if (IsEQTalentInteractionEnabled() == false)
            return false;
        if (EverQuest->IsSpellAnEQSpell(procSpellInfo->Id) == false)
            return false;
        if ((procSpellInfo->SchoolMask & SPELL_SCHOOL_MASK_SHADOW) == 0)
            return false;
        return EverQuestSpellTalentAlignment::DoesSpellInfoDealPeriodicDamage(procSpellInfo);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(EverQuest_EradicationAuraScript::CheckProc);
    }
};

// Warlock Improved Fear lives in a core aura-removal handler that only recognizes warlock family fears, so the converter
// attaches this script to every EQ spell block applying a fear.  When the fear ends for any reason, the caster's
// Improved Fear rank determines which Nightmare slow lands on the target, matching the core behavior
class EverQuest_ImprovedFearAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ImprovedFearAuraScript);

    static const uint32 EQ_SPELL_ID_WARLOCK_IMPROVED_FEAR_RANK1 = 53754;
    static const uint32 EQ_SPELL_ID_WARLOCK_IMPROVED_FEAR_RANK2 = 53759;
    static const uint32 EQ_SPELL_ID_WARLOCK_NIGHTMARE_RANK1 = 60946;
    static const uint32 EQ_SPELL_ID_WARLOCK_NIGHTMARE_RANK2 = 60947;

    void HandleAfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (IsEQTalentInteractionEnabled() == false)
            return;
        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsPlayer() == false)
            return;
        Unit* target = GetTarget();
        if (target == nullptr)
            return;
        AuraEffect const* improvedFearEffect = caster->GetAuraEffectOfRankedSpell(EQ_SPELL_ID_WARLOCK_IMPROVED_FEAR_RANK1, EFFECT_0);
        if (improvedFearEffect == nullptr)
            return;
        uint32 nightmareSpellID = improvedFearEffect->GetId() == EQ_SPELL_ID_WARLOCK_IMPROVED_FEAR_RANK2
            ? EQ_SPELL_ID_WARLOCK_NIGHTMARE_RANK2 : EQ_SPELL_ID_WARLOCK_NIGHTMARE_RANK1;
        caster->CastSpell(target, nightmareSpellID, true);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(EverQuest_ImprovedFearAuraScript::HandleAfterRemove, EFFECT_ALL, SPELL_AURA_MOD_FEAR, AURA_EFFECT_HANDLE_REAL);
    }
};

// Priest Grace has no stock script; its spell filter (Flash Heal, Greater Heal, Penance) lived in the spell_proc family
// mask.  The converter drops that mask so this CheckProc can re-impose it for WOW spells while letting the player's
// EverQuest single target direct heals carry the same chance to apply Grace to the target
class EverQuest_GraceAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_GraceAuraScript);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* procSpellInfo = eventInfo.GetSpellInfo();
        if (procSpellInfo == nullptr)
            return false;

        if (procSpellInfo->SpellFamilyName == SPELLFAMILY_PRIEST)
            return (procSpellInfo->SpellFamilyFlags & flag96(0x1800, 0x10000, 0)).IsEqual(0, 0, 0) == false;

        if (IsEQTalentInteractionEnabled() == false)
            return false;
        if (EverQuest->IsSpellAnEQSpell(procSpellInfo->Id) == false)
            return false;
        return EverQuestSpellTalentAlignment::DoesSpellInfoDealSingleTargetDirectHeal(procSpellInfo);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(EverQuest_GraceAuraScript::CheckProc);
    }
};

// Priest Holy Concentration: same pattern as Grace (stock filter was Flash Heal, Greater Heal, Binding Heal and
// Empowered Renew's heal), triggering off critical single target direct heals.  The critical requirement stays in the
// converter's spell_proc row
class EverQuest_HolyConcentrationAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_HolyConcentrationAuraScript);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* procSpellInfo = eventInfo.GetSpellInfo();
        if (procSpellInfo == nullptr)
            return false;

        if (procSpellInfo->SpellFamilyName == SPELLFAMILY_PRIEST)
            return (procSpellInfo->SpellFamilyFlags & flag96(0x1800, 0x4, 0x1000)).IsEqual(0, 0, 0) == false;

        if (IsEQTalentInteractionEnabled() == false)
            return false;
        if (EverQuest->IsSpellAnEQSpell(procSpellInfo->Id) == false)
            return false;
        return EverQuestSpellTalentAlignment::DoesSpellInfoDealSingleTargetDirectHeal(procSpellInfo);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(EverQuest_HolyConcentrationAuraScript::CheckProc);
    }
};

// Replaces spell_pri_improved_spirit_tap, whose CheckProc is a hard whitelist (Shadow Word: Death, Mind Blast, and a 50%
// roll for Mind Flay) that would veto any EverQuest spell.  The stock rules are kept, and EverQuest single target direct
// damage criticals trigger it at the talent's advertised chance: 50% at rank 1 and 100% at rank 2
class EverQuest_ImprovedSpiritTapAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ImprovedSpiritTapAuraScript);

    static const uint32 EQ_SPELL_ID_PRIEST_SHADOW_WORD_DEATH_RANK1 = 32379;
    static const uint32 EQ_SPELL_ID_PRIEST_MIND_BLAST_RANK1 = 8092;
    static const uint32 EQ_SPELL_ID_PRIEST_MIND_FLAY_DAMAGE = 58381;
    static const uint32 EQ_SPELL_ID_PRIEST_IMPROVED_SPIRIT_TAP_RANK1 = 15337;

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* procSpellInfo = eventInfo.GetSpellInfo();
        if (procSpellInfo == nullptr)
            return false;

        // Stock rules
        if (procSpellInfo->IsRankOf(sSpellMgr->AssertSpellInfo(EQ_SPELL_ID_PRIEST_SHADOW_WORD_DEATH_RANK1)) == true
            || procSpellInfo->IsRankOf(sSpellMgr->AssertSpellInfo(EQ_SPELL_ID_PRIEST_MIND_BLAST_RANK1)) == true)
            return true;
        if (procSpellInfo->Id == EQ_SPELL_ID_PRIEST_MIND_FLAY_DAMAGE)
            return roll_chance_i(50);

        if (IsEQTalentInteractionEnabled() == false)
            return false;
        if (EverQuest->IsSpellAnEQSpell(procSpellInfo->Id) == false)
            return false;
        if (EverQuestSpellTalentAlignment::DoesSpellInfoDealDirectDamage(procSpellInfo) == false)
            return false;
        if (EverQuestSpellTalentAlignment::DoesSpellInfoDealAreaDamage(procSpellInfo) == true)
            return false;
        if (GetId() == EQ_SPELL_ID_PRIEST_IMPROVED_SPIRIT_TAP_RANK1)
            return roll_chance_i(50);
        return true;
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(EverQuest_ImprovedSpiritTapAuraScript::CheckProc);
    }
};

// Priest Shadow Weaving uses an "add target trigger" aura the proc system never sees, and its family mask gate has no
// hook.  The converter attaches this script to every EQ spell block dealing shadow damage, and it rolls the talent's
// chance to stack the school-wide shadow damage buff, just like casting the priest's own shadow spells would
class EverQuest_ShadowWeavingSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_ShadowWeavingSpellScript);

    static const uint32 EQ_SPELL_ID_PRIEST_SHADOW_WEAVING_TALENT_RANK1 = 15257;
    static const uint32 EQ_SPELL_ID_PRIEST_SHADOW_WEAVING_BUFF = 15258;

    void HandleOnHit()
    {
        if (IsEQTalentInteractionEnabled() == false)
            return;
        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsPlayer() == false)
            return;
        Unit* target = GetHitUnit();
        if (target == nullptr || caster->IsFriendlyTo(target) == true)
            return;
        AuraEffect const* shadowWeavingEffect = caster->GetAuraEffectOfRankedSpell(EQ_SPELL_ID_PRIEST_SHADOW_WEAVING_TALENT_RANK1, EFFECT_0);
        if (shadowWeavingEffect == nullptr)
            return;
        if (roll_chance_i(shadowWeavingEffect->GetAmount()) == false)
            return;
        caster->CastSpell(caster, EQ_SPELL_ID_PRIEST_SHADOW_WEAVING_BUFF, true);
    }

    void Register() override
    {
        OnHit += SpellHitFn(EverQuest_ShadowWeavingSpellScript::HandleOnHit);
    }
};

// Priest Empowered Renew's up-front heal only runs from the script the core attaches to Renew.  The converter attaches
// this equivalent to every EQ spell block with a heal over time, mirroring the core behavior: an immediate heal worth a
// percent of the full periodic amount
class EverQuest_EmpoweredRenewSpellScript : public AuraScript
{
    PrepareAuraScript(EverQuest_EmpoweredRenewSpellScript);

    static const uint32 EQ_SPELL_ID_PRIEST_EMPOWERED_RENEW_TALENT_RANK1 = 63534;
    static const uint32 EQ_SPELL_ID_PRIEST_EMPOWERED_RENEW_HEAL = 63544;

    void HandleApplyEffect(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        if (IsEQTalentInteractionEnabled() == false)
            return;
        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsPlayer() == false)
            return;
        AuraEffect const* empoweredRenewEffect = caster->GetAuraEffectOfRankedSpell(EQ_SPELL_ID_PRIEST_EMPOWERED_RENEW_TALENT_RANK1, EFFECT_1);
        if (empoweredRenewEffect == nullptr)
            return;
        uint32 tickHeal = uint32(std::max(aurEff->GetAmount(), 0));
        tickHeal = GetTarget()->SpellHealingBonusTaken(caster, GetSpellInfo(), tickHeal, DOT);
        int32 basePoints = empoweredRenewEffect->GetAmount() * aurEff->GetTotalTicks() * int32(tickHeal) / 100;
        if (basePoints <= 0)
            return;
        caster->CastCustomSpell(GetTarget(), EQ_SPELL_ID_PRIEST_EMPOWERED_RENEW_HEAL, &basePoints, nullptr, nullptr, true, nullptr, aurEff);
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(EverQuest_EmpoweredRenewSpellScript::HandleApplyEffect, EFFECT_ALL, SPELL_AURA_PERIODIC_HEAL, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
    }
};

void AddEverQuestTalentInteractionScripts()
{
    RegisterSpellScript(EverQuest_ColdSnapSpellScript);
    RegisterSpellScript(EverQuest_HotStreakAuraScript);
    RegisterSpellScript(EverQuest_BrainFreezeAuraScript);
    RegisterSpellScript(EverQuest_NightfallAuraScript);
    RegisterSpellScript(EverQuest_ShadowEmbraceAuraScript);
    RegisterSpellScript(EverQuest_EradicationAuraScript);
    RegisterSpellScript(EverQuest_ImprovedFearAuraScript);
    RegisterSpellScript(EverQuest_GraceAuraScript);
    RegisterSpellScript(EverQuest_HolyConcentrationAuraScript);
    RegisterSpellScript(EverQuest_ImprovedSpiritTapAuraScript);
    RegisterSpellScript(EverQuest_ShadowWeavingSpellScript);
    RegisterSpellScript(EverQuest_EmpoweredRenewSpellScript);
}
