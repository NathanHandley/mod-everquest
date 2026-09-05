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

using namespace std;

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
        if (EverQuest->IsClassAuraSystemEnabled() == false)
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

// Ranger "Swift Reactions": every landed melee or ranged autoattack quickens the ranger's stride, and every landed ranged autoattack (bow, gun, thrown), ranged ability or harmful spell tacks its target
class EverQuest_ClassAuraRangerAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ClassAuraRangerAuraScript);

    void HandleProc(ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (EverQuest->IsClassAuraSystemEnabled() == false)
            return;
        Unit* ranger = GetTarget();
        if (ranger == nullptr || ranger->IsPlayer() == false || ranger->IsAlive() == false)
            return;
        uint32 typeMask = eventInfo.GetTypeMask();
        uint32 speedSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_RANGER_SPEED);
        if (speedSpellID != 0 && (typeMask & (PROC_FLAG_DONE_MELEE_AUTO_ATTACK | PROC_FLAG_DONE_RANGED_AUTO_ATTACK)) != 0)
            ranger->CastSpell(ranger, speedSpellID, true);

        if ((typeMask & (PROC_FLAG_DONE_RANGED_AUTO_ATTACK | PROC_FLAG_DONE_SPELL_RANGED_DMG_CLASS | PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG | PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG)) == 0)
            return;
        uint32 tackShotSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_RANGER_TACK_SHOT);
        if (tackShotSpellID == 0)
            return;
        Unit* target = eventInfo.GetProcTarget();
        if (target == nullptr || target == ranger || target->IsAlive() == false || target->FindMap() != ranger->FindMap())
            return;
        if (ranger->IsValidAttackTarget(target) == false)
            return;
        ranger->CastSpell(target, tackShotSpellID, true);
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
        if (EverQuest->IsClassAuraSystemEnabled() == false)
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

// Shadow Knight "Spellsword": a melee critical strike readies the edge (the proc row's cooldown spaces the triggers)
class EverQuest_ClassAuraShadowKnightAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ClassAuraShadowKnightAuraScript);

    void HandleProc(ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        if (EverQuest->IsClassAuraSystemEnabled() == false)
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

// Monk "Agile Fighter": the proc row on the armor aura already rolled the double attack, and in light armor some of those become a triple.
static void DoClassAuraMonkDoubleAttack(Unit* monk, ProcEventInfo& eventInfo, uint32 tripleChancePercent)
{
    if (EverQuest->IsClassAuraSystemEnabled() == false)
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

    void HandleProc(ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        if (EverQuest->IsClassAuraSystemEnabled() == false)
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

    void HandleProc(ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        if (EverQuest->IsClassAuraSystemEnabled() == false)
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

// Druid "Skin of the Wild": a direct heal leaves a regeneration worth a share of it behind (periodic heals are not in the proc flags), and a
// landed melee or ranged autoattack exposes the target (the pet's strikes do the same through the mod's landed-swing hook)
class EverQuest_ClassAuraDruidAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ClassAuraDruidAuraScript);

    void HandleProc(ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (EverQuest->IsClassAuraSystemEnabled() == false)
            return;
        Unit* druid = GetTarget();
        if (druid == nullptr || druid->IsPlayer() == false || druid->IsAlive() == false)
            return;

        if ((eventInfo.GetTypeMask() & (PROC_FLAG_DONE_MELEE_AUTO_ATTACK | PROC_FLAG_DONE_RANGED_AUTO_ATTACK)) != 0)
        {
            uint32 exposureSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_DRUID_EXPOSURE);
            Unit* target = eventInfo.GetProcTarget();
            if (exposureSpellID != 0 && target != nullptr && target != druid && target->IsAlive() == true && target->FindMap() == druid->FindMap()
                && druid->IsValidAttackTarget(target) == true)
                druid->CastSpell(target, exposureSpellID, true);
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

// Shaman "Spirit Channeler": healing an ally stacks vigor on them
class EverQuest_ClassAuraShamanAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_ClassAuraShamanAuraScript);

    void HandleProc(ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (EverQuest->IsClassAuraSystemEnabled() == false)
            return;
        Unit* shaman = GetTarget();
        if (shaman == nullptr || shaman->IsPlayer() == false || shaman->IsAlive() == false)
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

void AddEverQuestClassAuraScripts()
{
    RegisterSpellScript(EverQuest_ClassAuraRogueAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraRangerAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraPaladinAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraShadowKnightAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraMonkLightArmorAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraMonkHeavyArmorAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraMagicianAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraMagicianPetAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraDruidAuraScript);
    RegisterSpellScript(EverQuest_ClassAuraShamanAuraScript);
}
