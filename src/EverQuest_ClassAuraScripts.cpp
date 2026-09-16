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
#include "Random.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "Unit.h"
#include "EverQuest.h"
#include <algorithm>
#include <limits>

using namespace std;

static bool IsClassAuraPeriodicTickProc(ProcEventInfo& eventInfo)
{
    return (eventInfo.GetTypeMask() & PERIODIC_PROC_FLAG_MASK) != 0;
}

// Rogue "Master Exploiter": any landed attack (autoattack, ability or harmful spell) stacks the momentum, and any attack that is missed, dodged or parried costs half of the stacks (rounded down)
// A critical of any kind (heals included) can spend the readied Lucky Strike
static const uint32 EQ_CLASSAURA_ROGUE_ATTACK_PROC_MASK = PROC_FLAG_DONE_MELEE_AUTO_ATTACK | PROC_FLAG_DONE_RANGED_AUTO_ATTACK | PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS
    | PROC_FLAG_DONE_SPELL_RANGED_DMG_CLASS | PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG | PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG;

class EverQuest_ClassAuraRogueAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ClassAuraRogueAuraScript);

    void HandleLuckyStrike(Player* rogue, ProcEventInfo& eventInfo)
    {
        if ((eventInfo.GetHitMask() & PROC_HIT_CRITICAL) == 0)
            return;
        if ((eventInfo.GetTypeMask() & (PROC_FLAG_DONE_MELEE_AUTO_ATTACK | PROC_FLAG_DONE_RANGED_AUTO_ATTACK)) != 0)
            return;
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        if (spellInfo == nullptr || spellInfo->HasAttribute(SPELL_ATTR2_AUTO_REPEAT) == true)
            return;
        uint32 luckyStrikeSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_ROGUE_LUCKY_STRIKE);
        if (luckyStrikeSpellID == 0 || rogue->HasAura(luckyStrikeSpellID) == false)
            return;
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        WeaponAttackType attackType = damageInfo != nullptr ? damageInfo->GetAttackType() : BASE_ATTACK;
        float ownCritChance = rogue->SpellDoneCritChance(eventInfo.GetProcTarget(), spellInfo, eventInfo.GetSchoolMask(), attackType, false);
        uint32 helperSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_ROGUE_LUCKY_STRIKE_HELPER);
        if (helperSpellID != 0 && rogue->HasAura(helperSpellID) == true)
            ownCritChance -= (float)EverQuest->ConfigSystemClassAuraRogueLuckyStrikeCritPercent;
        if (ownCritChance > 0.0f && roll_chance_f(ownCritChance) == true)
            return;
        EverQuest->SpendClassAuraRogueLuckyStrike(rogue);
    }

    void HandleProc(ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (EverQuest->IsClassAuraSystemEnabled() == false || IsClassAuraPeriodicTickProc(eventInfo) == true)
            return;
        Unit* rogue = GetTarget();
        if (rogue == nullptr || rogue->IsPlayer() == false || rogue->IsAlive() == false)
            return;
        HandleLuckyStrike(rogue->ToPlayer(), eventInfo);

        // Only attacks feed the momentum
        if ((eventInfo.GetTypeMask() & EQ_CLASSAURA_ROGUE_ATTACK_PROC_MASK) == 0)
            return;
        uint32 exploitSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_ROGUE_EXPLOIT);
        if (exploitSpellID == 0)
            return;
        uint32 hitMask = eventInfo.GetHitMask();
        if ((hitMask & (PROC_HIT_MISS | PROC_HIT_DODGE | PROC_HIT_PARRY)) != 0)
        {
            if (Aura* exploit = rogue->GetAura(exploitSpellID))
            {
                int32 stacksToLose = (int32)exploit->GetStackAmount() / 2;
                if (stacksToLose > 0)
                    exploit->ModStackAmount(-stacksToLose);
            }
            return;
        }
        if ((hitMask & (PROC_HIT_NORMAL | PROC_HIT_CRITICAL | PROC_HIT_BLOCK | PROC_HIT_ABSORB)) != 0)
            rogue->CastSpell(rogue, exploitSpellID, true);
    }

    void Register() override
    {
        OnProc += AuraProcFn(EverQuest_ClassAuraRogueAuraScript::HandleProc);
    }
};

// Ranger: every landed melee or ranged autoattack, and every harmful single target spell, compounds the target's injuries.  The pet's strikes do the same through
// HandleClassAuraPetStrike, since a proc row on the ranger never sees them
class EverQuest_ClassAuraRangerAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ClassAuraRangerAuraScript);

    void HandleProc(ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (EverQuest->IsClassAuraSystemEnabled() == false || IsClassAuraPeriodicTickProc(eventInfo) == true)
            return;
        Unit* ranger = GetTarget();
        if (ranger == nullptr || ranger->IsPlayer() == false || ranger->IsAlive() == false)
            return;
        uint32 typeMask = eventInfo.GetTypeMask();
        if ((typeMask & (PROC_FLAG_DONE_MELEE_AUTO_ATTACK | PROC_FLAG_DONE_RANGED_AUTO_ATTACK | PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS
            | PROC_FLAG_DONE_SPELL_RANGED_DMG_CLASS | PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG | PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG)) == 0)
            return;

        // Only a spell aimed at one target counts, so an area spell never compounds everything it washes over.  An autoattack carries no spell to check
        SpellInfo const* procSpellInfo = eventInfo.GetSpellInfo();
        if (procSpellInfo != nullptr && procSpellInfo->IsTargetingArea() == true)
            return;
        EverQuest->ApplyClassAuraRangerCompoundInjury(ranger->ToPlayer(), eventInfo.GetProcTarget());
    }

    void Register() override
    {
        OnProc += AuraProcFn(EverQuest_ClassAuraRangerAuraScript::HandleProc);
    }
};

static const uint32 EQ_CLASSAURA_PALADIN_TAKEN_ATTACK_PROC_MASK = PROC_FLAG_TAKEN_MELEE_AUTO_ATTACK | PROC_FLAG_TAKEN_SPELL_MELEE_DMG_CLASS
    | PROC_FLAG_TAKEN_RANGED_AUTO_ATTACK | PROC_FLAG_TAKEN_SPELL_RANGED_DMG_CLASS;

class EverQuest_ClassAuraPaladinAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ClassAuraPaladinAuraScript);

    void HandleBlockedAttack(Unit* paladin, ProcEventInfo& eventInfo)
    {
        if ((eventInfo.GetHitMask() & (PROC_HIT_BLOCK | PROC_HIT_FULL_BLOCK)) == 0)
            return;
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (damageInfo == nullptr || damageInfo->GetBlock() == 0)
            return;
        uint32 deflectionSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_PALADIN_DEFLECTION);
        if (deflectionSpellID == 0 || paladin->IsInWorld() == false)
            return;
        int32 damageAmount = (int32)(((uint64)damageInfo->GetBlock() * (uint64)EverQuest->ConfigSystemClassAuraPaladinBlockDeflectionDamagePercent) / 100);
        if (damageAmount <= 0)
            return;
        paladin->CastCustomSpell(paladin, deflectionSpellID, &damageAmount, nullptr, nullptr, true);
    }

    void HandleProc(ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (EverQuest->IsClassAuraSystemEnabled() == false || IsClassAuraPeriodicTickProc(eventInfo) == true)
            return;
        Unit* paladin = GetTarget();
        if (paladin == nullptr || paladin->IsPlayer() == false || paladin->IsAlive() == false)
            return;
        if ((eventInfo.GetTypeMask() & EQ_CLASSAURA_PALADIN_TAKEN_ATTACK_PROC_MASK) != 0)
        {
            HandleBlockedAttack(paladin, eventInfo);
            return;
        }
        HealInfo* healInfo = eventInfo.GetHealInfo();
        if (healInfo == nullptr || healInfo->GetHeal() == 0)
            return;
        // The reward heal itself (or any other class aura heal) never feeds back into another reward
        SpellInfo const* healSpellInfo = healInfo->GetSpellInfo();
        if (healSpellInfo != nullptr && EverQuest->IsClassAuraSpell(healSpellInfo->Id) == true)
            return;
        uint32 healSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_PALADIN_HEAL);
        if (healSpellID == 0)
            return;
        int32 healAmount = (int32)(((uint64)healInfo->GetHeal() * (uint64)EverQuest->ConfigSystemClassAuraPaladinHealSelfPercent) / 100);
        if (healAmount <= 0)
            return;
        paladin->CastCustomSpell(paladin, healSpellID, &healAmount, nullptr, nullptr, true);
    }

    void Register() override
    {
        OnProc += AuraProcFn(EverQuest_ClassAuraPaladinAuraScript::HandleProc);
    }
};

// Warrior "Warmaster": a direct attack landing on the warrior (melee, ranged, or a single target spell, partial blocks included) takes a stack of Unrelenting Assault away.
// Misses, dodges and parries never reach the proc's hit mask, full blocks are turned away here, and area attacks are skipped by their spell
class EverQuest_ClassAuraWarriorAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ClassAuraWarriorAuraScript);

    void HandleProc(ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (EverQuest->IsClassAuraSystemEnabled() == false || IsClassAuraPeriodicTickProc(eventInfo) == true)
            return;
        Unit* warrior = GetTarget();
        if (warrior == nullptr || warrior->IsPlayer() == false || warrior->IsAlive() == false)
            return;
        uint32 hitMask = eventInfo.GetHitMask();
        if ((hitMask & PROC_HIT_FULL_BLOCK) != 0 || (hitMask & (PROC_HIT_NORMAL | PROC_HIT_CRITICAL | PROC_HIT_ABSORB)) == 0)
            return;
        Unit* attacker = eventInfo.GetActor();
        if (attacker == nullptr || attacker == warrior)
            return;
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        if (spellInfo != nullptr && spellInfo->IsAffectingArea() == true)
            return;

        // A creature's wild rampage swings at everyone in reach, which makes it an area attack even though each swing is a plain melee hit
        if (EverQuest->IsCreatureAreaSwingRoundInProgress() == true)
            return;
        uint32 assaultSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_WARRIOR_UNRELENTING_ASSAULT);
        if (assaultSpellID == 0)
            return;
        if (Aura* assault = warrior->GetAura(assaultSpellID))
            assault->ModStackAmount(-1);
    }

    void Register() override
    {
        OnProc += AuraProcFn(EverQuest_ClassAuraWarriorAuraScript::HandleProc);
    }
};

// Shadow Knight "Spellsword": a melee critical strike readies the edge (the proc row's cooldown spaces the triggers)
class EverQuest_ClassAuraShadowKnightAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ClassAuraShadowKnightAuraScript);

    void HandleProc(ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (EverQuest->IsClassAuraSystemEnabled() == false || IsClassAuraPeriodicTickProc(eventInfo) == true)
            return;
        Unit* shadowKnight = GetTarget();
        if (shadowKnight == nullptr || shadowKnight->IsPlayer() == false || shadowKnight->IsAlive() == false)
            return;
        uint32 edgeSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_SHADOWKNIGHT_EDGE);
        if (edgeSpellID == 0)
            return;
        shadowKnight->CastSpell(shadowKnight, edgeSpellID, true);
    }

    void Register() override
    {
        OnProc += AuraProcFn(EverQuest_ClassAuraShadowKnightAuraScript::HandleProc);
    }
};

// Shadow Knight "Blood Debt": drains the stored damage from the target as shadow damage and heals the knight for the full amount stored.  The charge is only spent
// on a hit that can take the damage, so a miss or an immune target costs nothing
class EverQuest_ClassAuraShadowKnightBloodDebtSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_ClassAuraShadowKnightBloodDebtSpellScript);

    uint32 SpentAmount = 0;

    SpellCastResult CheckStoredDamage()
    {
        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsPlayer() == false)
            return SPELL_FAILED_SPELL_UNAVAILABLE;
        Player* shadowKnight = caster->ToPlayer();
        if (EverQuest->IsClassAuraSystemEnabled() == false || EverQuest->PlayerHasClassAura(shadowKnight, EQ_CLASSAURA_SPELL_SHADOWKNIGHT_AURA) == false)
            return SPELL_FAILED_SPELL_UNAVAILABLE;
        if (EverQuest->GetClassAuraShadowKnightBloodDebtAmount(shadowKnight) == 0)
            return SPELL_FAILED_CASTER_AURASTATE;
        return SPELL_CAST_OK;
    }

    void SpendStoredDamageOnHit()
    {
        SpentAmount = 0;
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (caster == nullptr || caster->IsPlayer() == false || target == nullptr)
            return;

        // Reflected back onto the knight, it does nothing at all
        if (target == caster)
        {
            SetHitDamage(0);
            return;
        }

        // A miss or an immune target arrives here without the placeholder damage
        if (GetHitDamage() <= 0)
            return;

        // Damage immunity is only checked after this hook, so it is caught here before the charge is spent
        if (target->IsImmunedToDamage(caster, GetSpellInfo()) == true)
            return;
        uint32 amount = EverQuest->SpendClassAuraShadowKnightBloodDebt(caster->ToPlayer());
        SetHitDamage(int32(min<uint32>(amount, uint32(numeric_limits<int32>::max()))));
        SpentAmount = amount;
    }

    void HealFromSpentDebt()
    {
        if (SpentAmount == 0)
            return;
        int32 healAmount = int32(min<uint32>(SpentAmount, uint32(numeric_limits<int32>::max())));
        SpentAmount = 0;
        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsAlive() == false || caster->IsInWorld() == false)
            return;
        uint32 healSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_SHADOWKNIGHT_BLOOD_DEBT_HEAL);
        if (healSpellID == 0)
            return;
        caster->CastCustomSpell(caster, healSpellID, &healAmount, nullptr, nullptr, true);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(EverQuest_ClassAuraShadowKnightBloodDebtSpellScript::CheckStoredDamage);
        OnHit += SpellHitFn(EverQuest_ClassAuraShadowKnightBloodDebtSpellScript::SpendStoredDamageOnHit);
        AfterHit += SpellHitFn(EverQuest_ClassAuraShadowKnightBloodDebtSpellScript::HealFromSpentDebt);
    }
};

// Monk "Agile Fighter": the proc row on the armor aura already rolled the double attack, and in light armor some of those become a triple.
static void DoClassAuraMonkDoubleAttack(Unit* monk, ProcEventInfo& eventInfo, uint32 tripleChancePercent)
{
    if (EverQuest->IsClassAuraSystemEnabled() == false || IsClassAuraPeriodicTickProc(eventInfo) == true)
        return;
    if (monk == nullptr || monk->IsPlayer() == false || monk->IsAlive() == false)
        return;
    // A swing that is itself an extra attack (ours or any other extra attack effect) never chains
    if (monk->GetLastExtraAttackSpell() != 0)
        return;
    Unit* victim = eventInfo.GetProcTarget();
    if (victim == nullptr || victim->IsAlive() == false)
        return;
    int32 extraAttackCount = 1;
    if (tripleChancePercent > 0 && roll_chance_i((int32)tripleChancePercent) == true)
        extraAttackCount = 2;
    // Thrash has one die side, which the core adds on top of the custom base points, so the base points sit one below the swing count
    int32 extraAttackBasePoints = extraAttackCount - 1;
    monk->CastCustomSpell(monk, EQ_SPELL_ID_THRASH, &extraAttackBasePoints, nullptr, nullptr, true);
}

class EverQuest_ClassAuraMonkLightArmorAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ClassAuraMonkLightArmorAuraScript);

    void HandleProc(ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        DoClassAuraMonkDoubleAttack(GetTarget(), eventInfo, EverQuest->ConfigSystemClassAuraMonkDoubleToTripleAttackChancePercent);
    }

    void Register() override
    {
        OnProc += AuraProcFn(EverQuest_ClassAuraMonkLightArmorAuraScript::HandleProc);
    }
};

class EverQuest_ClassAuraMonkHeavyArmorAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ClassAuraMonkHeavyArmorAuraScript);

    void HandleProc(ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        DoClassAuraMonkDoubleAttack(GetTarget(), eventInfo, 0);
    }

    void Register() override
    {
        OnProc += AuraProcFn(EverQuest_ClassAuraMonkHeavyArmorAuraScript::HandleProc);
    }
};

// Magician "Bound Conjurer" (owner side): the owner's spell critical strikes stack fury on the pet
class EverQuest_ClassAuraMagicianAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ClassAuraMagicianAuraScript);

    void HandleProc(ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (EverQuest->IsClassAuraSystemEnabled() == false || IsClassAuraPeriodicTickProc(eventInfo) == true)
            return;
        Unit* owner = GetTarget();
        if (owner == nullptr || owner->IsPlayer() == false || owner->IsAlive() == false)
            return;
        uint32 petFurySpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_MAGICIAN_PET_FURY);
        if (petFurySpellID == 0)
            return;
        Unit* pet = EverQuest->GetActiveClassAuraPetForPlayer(owner->ToPlayer());
        if (pet == nullptr || pet->FindMap() != owner->FindMap())
            return;
        owner->CastSpell(pet, petFurySpellID, true);
    }

    void Register() override
    {
        OnProc += AuraProcFn(EverQuest_ClassAuraMagicianAuraScript::HandleProc);
    }
};

// Magician "Bound Conjurer" (pet side): the pet's landed strikes stack insight on the owner
class EverQuest_ClassAuraMagicianPetAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ClassAuraMagicianPetAuraScript);

    void HandleProc(ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (EverQuest->IsClassAuraSystemEnabled() == false || IsClassAuraPeriodicTickProc(eventInfo) == true)
            return;
        Unit* pet = GetTarget();
        if (pet == nullptr || pet->IsAlive() == false)
            return;
        Player* owner = pet->GetCharmerOrOwnerPlayerOrPlayerItself();
        if (owner == nullptr || owner == pet || owner->IsAlive() == false || owner->FindMap() != pet->FindMap())
            return;
        if (EverQuest->PlayerHasClassAura(owner, EQ_CLASSAURA_SPELL_MAGICIAN_AURA) == false)
            return;
        uint32 ownerFocusSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_MAGICIAN_OWNER_FOCUS);
        if (ownerFocusSpellID == 0)
            return;
        owner->CastSpell(owner, ownerFocusSpellID, true);
    }

    void Register() override
    {
        OnProc += AuraProcFn(EverQuest_ClassAuraMagicianPetAuraScript::HandleProc);
    }
};

// Druid "One With Nature": a direct heal leaves a regeneration worth a share of it behind (periodic heals are not in the proc flags), and a landed physical attack entangles the target
class EverQuest_ClassAuraDruidAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ClassAuraDruidAuraScript);

    void HandleProc(ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (EverQuest->IsClassAuraSystemEnabled() == false || IsClassAuraPeriodicTickProc(eventInfo) == true)
            return;
        Unit* druid = GetTarget();
        if (druid == nullptr || druid->IsPlayer() == false || druid->IsAlive() == false)
            return;

        if ((eventInfo.GetTypeMask() & (PROC_FLAG_DONE_MELEE_AUTO_ATTACK | PROC_FLAG_DONE_RANGED_AUTO_ATTACK | PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS | PROC_FLAG_DONE_SPELL_RANGED_DMG_CLASS)) != 0)
        {
            // Physical attacks only, so a melee or ranged ability that lands as fire, cold, or nature damage does not entangle
            DamageInfo* damageInfo = eventInfo.GetDamageInfo();
            if (damageInfo == nullptr || (damageInfo->GetSchoolMask() & SPELL_SCHOOL_MASK_NORMAL) == 0)
                return;
            uint32 entangleSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_DRUID_ENTANGLE_STRIKE);
            Unit* target = eventInfo.GetProcTarget();
            if (entangleSpellID != 0 && target != nullptr && target != druid && target->IsAlive() == true && target->FindMap() == druid->FindMap()
                && druid->IsValidAttackTarget(target) == true)
                druid->CastSpell(target, entangleSpellID, true);
            return;
        }

        HealInfo* healInfo = eventInfo.GetHealInfo();
        if (healInfo == nullptr || healInfo->GetHeal() == 0)
            return;
        Unit* healTarget = healInfo->GetTarget();
        if (healTarget == nullptr || healTarget->IsAlive() == false || healTarget->FindMap() != druid->FindMap())
            return;
        SpellInfo const* healSpellInfo = healInfo->GetSpellInfo();
        if (healSpellInfo != nullptr && EverQuest->IsClassAuraSpell(healSpellInfo->Id) == true)
            return;
        uint32 regrowthSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_DRUID_REGROWTH);
        if (regrowthSpellID == 0)
            return;
        uint32 tickCount = EverQuest->ConfigSystemClassAuraDruidDirectHealRegenTickCount == 0 ? 1 : EverQuest->ConfigSystemClassAuraDruidDirectHealRegenTickCount;
        int32 healPerTick = (int32)((((uint64)healInfo->GetHeal() * (uint64)EverQuest->ConfigSystemClassAuraDruidDirectHealRegenPercent) / 100) / tickCount);
        if (healPerTick <= 0)
            return;
        druid->CastCustomSpell(healTarget, regrowthSpellID, &healPerTick, nullptr, nullptr, true);
    }

    void Register() override
    {
        OnProc += AuraProcFn(EverQuest_ClassAuraDruidAuraScript::HandleProc);
    }
};

// Shaman "Spirit Channeler": directly healing an ally stacks vigor on them (heal over time ticks are not in the proc flags)
// With Warspirit on, the shaman's own landed attacks and damaging spells stack warspirit vigor on the shaman instead, and heals grant nothing
class EverQuest_ClassAuraShamanAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ClassAuraShamanAuraScript);

    void HandleProc(ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (EverQuest->IsClassAuraSystemEnabled() == false || IsClassAuraPeriodicTickProc(eventInfo) == true)
            return;
        Unit* shaman = GetTarget();
        if (shaman == nullptr || shaman->IsPlayer() == false || shaman->IsAlive() == false)
            return;

        uint32 warspiritSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_SHAMAN_WARSPIRIT);
        if (warspiritSpellID != 0 && shaman->HasAura(warspiritSpellID) == true)
        {
            if ((eventInfo.GetTypeMask() & EQ_CLASSAURA_ROGUE_ATTACK_PROC_MASK) == 0)
                return;
            uint32 warspiritVigorSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_SHAMAN_WARSPIRIT_VIGOR);
            if (warspiritVigorSpellID == 0)
                return;
            shaman->CastSpell(shaman, warspiritVigorSpellID, true);
            return;
        }

        if ((eventInfo.GetTypeMask() & (PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS | PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS)) == 0)
            return;
        HealInfo* healInfo = eventInfo.GetHealInfo();
        if (healInfo == nullptr || healInfo->GetHeal() == 0)
            return;
        Unit* healTarget = healInfo->GetTarget();
        if (healTarget == nullptr || healTarget->IsAlive() == false || healTarget->FindMap() != shaman->FindMap())
            return;
        if (shaman->IsFriendlyTo(healTarget) == false)
            return;
        uint32 vigorSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_SHAMAN_VIGOR);
        if (vigorSpellID == 0)
            return;
        shaman->CastSpell(healTarget, vigorSpellID, true);
    }

    void Register() override
    {
        OnProc += AuraProcFn(EverQuest_ClassAuraShamanAuraScript::HandleProc);
    }
};

// Necromancer "Shadow Exchange": the necromancer and the pet trade places, and every harmful effect an enemy left on the necromancer goes to the pet
class EverQuest_ClassAuraNecromancerShadowExchangeSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_ClassAuraNecromancerShadowExchangeSpellScript);

    SpellCastResult CheckPet()
    {
        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsPlayer() == false)
            return SPELL_FAILED_SPELL_UNAVAILABLE;
        Player* necromancer = caster->ToPlayer();
        if (EverQuest->IsClassAuraSystemEnabled() == false || EverQuest->PlayerHasClassAura(necromancer, EQ_CLASSAURA_SPELL_NECROMANCER_AURA) == false)
            return SPELL_FAILED_SPELL_UNAVAILABLE;
        bool isOutOfRange = false;
        if (EverQuest->GetClassAuraNecromancerShadowExchangePet(necromancer, isOutOfRange) == nullptr)
        {
            if (isOutOfRange == true)
                return SPELL_FAILED_OUT_OF_RANGE;
            return SPELL_FAILED_NO_PET;
        }
        return SPELL_CAST_OK;
    }

    void SwapWithPet(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(effIndex);
        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsPlayer() == false)
            return;
        EverQuest->DoClassAuraNecromancerShadowExchange(caster->ToPlayer());
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(EverQuest_ClassAuraNecromancerShadowExchangeSpellScript::CheckPet);
        OnEffectHitTarget += SpellEffectFn(EverQuest_ClassAuraNecromancerShadowExchangeSpellScript::SwapWithPet, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// Ranger "Compound Injury (Moving)": held at its full duration for as long as the target keeps moving, so the doubling lasts the whole duration after the target stops
class EverQuest_ClassAuraRangerCompoundInjuryMovingAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ClassAuraRangerCompoundInjuryMovingAuraScript);

    void HoldFullWhileMoving(AuraEffect const* /*aurEff*/)
    {
        Unit* target = GetUnitOwner();
        if (target == nullptr || target->isMoving() == false)
            return;

        // SetDuration rather than RefreshDuration, which does nothing once the ranger who applied it has left the map
        Aura* movingAura = GetAura();
        if (movingAura->GetDuration() < movingAura->GetMaxDuration())
            movingAura->SetDuration(movingAura->GetMaxDuration());
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(EverQuest_ClassAuraRangerCompoundInjuryMovingAuraScript::HoldFullWhileMoving, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// Magician "Detonate Summoned": the magician's summoned pet explodes for its current health as fire damage to every enemy near it, and is unsummoned
class EverQuest_ClassAuraMagicianDetonateSummonedSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_ClassAuraMagicianDetonateSummonedSpellScript);

    // Runs when the cast starts and again when it finishes, so a pet that died or was dismissed during the cast stops it
    SpellCastResult CheckPet()
    {
        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsPlayer() == false)
            return SPELL_FAILED_SPELL_UNAVAILABLE;
        Player* magician = caster->ToPlayer();
        if (EverQuest->IsClassAuraSystemEnabled() == false || EverQuest->PlayerHasClassAura(magician, EQ_CLASSAURA_SPELL_MAGICIAN_AURA) == false)
            return SPELL_FAILED_SPELL_UNAVAILABLE;
        if (EverQuest->GetClassAuraMagicianDetonatePet(magician) == nullptr)
            return SPELL_FAILED_NO_PET;
        return SPELL_CAST_OK;
    }

    void Detonate(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(effIndex);
        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsPlayer() == false)
            return;
        EverQuest->DoClassAuraMagicianDetonateSummoned(caster->ToPlayer());
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(EverQuest_ClassAuraMagicianDetonateSummonedSpellScript::CheckPet);
        OnEffectHitTarget += SpellEffectFn(EverQuest_ClassAuraMagicianDetonateSummonedSpellScript::Detonate, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

void AddEverQuestClassAuraScripts()
{
    RegisterSpellScript(EverQuest_ClassAuraRogueAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraRangerAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraRangerCompoundInjuryMovingAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraPaladinAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraWarriorAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraShadowKnightAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraShadowKnightBloodDebtSpellScript);
    RegisterSpellScript(EverQuest_ClassAuraNecromancerShadowExchangeSpellScript);
    RegisterSpellScript(EverQuest_ClassAuraMagicianDetonateSummonedSpellScript);
    RegisterSpellScript(EverQuest_ClassAuraMonkLightArmorAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraMonkHeavyArmorAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraMagicianAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraMagicianPetAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraDruidAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraShamanAuraScript);
}
