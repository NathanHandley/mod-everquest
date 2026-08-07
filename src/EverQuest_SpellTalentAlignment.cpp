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

#include "EverQuest_SpellTalentAlignment.h"

#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "Log.h"
#include "Player.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

#include "EverQuest.h"

EverQuestSpellTalentAlignment* EverQuestSpellTalentAlignment::instance()
{
    static EverQuestSpellTalentAlignment instance;
    return &instance;
}

void EverQuestSpellTalentAlignment::BuildPlayerSpellsByFamily()
{
    for (uint32 i = 0; i < sSkillLineAbilityStore.GetNumRows(); ++i)
    {
        SkillLineAbilityEntry const* skillLineAbility = sSkillLineAbilityStore.LookupEntry(i);
        if (skillLineAbility == nullptr)
            continue;

        // Class restricted entries only, which is what separates a learnable class spell from world and profession spells
        if (skillLineAbility->ClassMask == 0)
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(skillLineAbility->Spell);
        if (spellInfo == nullptr || spellInfo->SpellFamilyName == SPELLFAMILY_GENERIC)
            continue;

        PlayerSpellsByFamily[spellInfo->SpellFamilyName].push_back(spellInfo);
    }
}

bool EverQuestSpellTalentAlignment::BuildAlignmentForMask(SpellInfo const* talentSpellInfo, flag96 const& classMask,  EverQuestTalentModAlignment& alignmentOut)
{
    if (classMask.IsEqual(0, 0, 0) == true)
        return false;

    auto familyEntry = PlayerSpellsByFamily.find(talentSpellInfo->SpellFamilyName);
    if (familyEntry == PlayerSpellsByFamily.end())
        return false;

    std::set<std::string> coveredSpellNames;
    uint32 sharedSchoolMask = 0;
    bool hasAnyCovered = false;
    for (SpellInfo const* coveredSpellInfo : familyEntry->second)
    {
        if ((classMask & coveredSpellInfo->SpellFamilyFlags).IsEqual(0, 0, 0) == true)
            continue;

        // Ranks of one spell share a name, so they count once toward how many spells the modifier reaches
        coveredSpellNames.insert(coveredSpellInfo->SpellName[0]);
        if (hasAnyCovered == false)
        {
            sharedSchoolMask = coveredSpellInfo->SchoolMask;
            hasAnyCovered = true;
        }
        else
            sharedSchoolMask &= coveredSpellInfo->SchoolMask;

        if (DoesSpellInfoHeal(coveredSpellInfo) == true)
            alignmentOut.AffectsHealing = true;
        if (DoesSpellInfoDamage(coveredSpellInfo) == true)
            alignmentOut.AffectsDamage = true;
    }

    // One spell means the modifier is about that spell, and no EQ spell is that spell
    if (coveredSpellNames.size() < 2)
        return false;
    if (alignmentOut.AffectsDamage == false && alignmentOut.AffectsHealing == false)
        return false;

    // A damage modifier needs a school to align on, a healing one does not
    alignmentOut.SchoolMask = sharedSchoolMask;
    if (alignmentOut.AffectsHealing == false && alignmentOut.SchoolMask == 0)
        return false;
    return true;
}

void EverQuestSpellTalentAlignment::LoadExcludedTalents()
{
    QueryResult queryResult = WorldDatabase.Query("SELECT TalentRank1SpellID FROM mod_everquest_talent_exclusion;");
    if (queryResult == nullptr)
    {
        LOG_INFO("module", "EverQuest: No talent alignment exclusions found");
        return;
    }
    do
    {
        ExcludedTalentRank1SpellIDs.insert((*queryResult)[0].Get<uint32>());
    } while (queryResult->NextRow());
}

void EverQuestSpellTalentAlignment::LoadExplicitAlignments()
{
    QueryResult queryResult = WorldDatabase.Query("SELECT SpellID, EffectIndex, SchoolMask, AffectsDamage, AffectsHealing, SpellRestriction, MaxBaseCastTimeMS FROM mod_everquest_talent_alignment;");
    if (queryResult == nullptr)
    {
        LOG_INFO("module", "EverQuest: No explicit talent alignments found");
        return;
    }
    do
    {
        Field* fields = queryResult->Fetch();
        uint32 spellID = fields[0].Get<uint32>();
        uint8 effectIndex = fields[1].Get<uint8>();
        if (effectIndex >= MAX_SPELL_EFFECTS)
        {
            LOG_ERROR("module", "EverQuest: Talent alignment row for spell ID {} has an invalid effect index of {}", spellID, effectIndex);
            continue;
        }
        EverQuestTalentModAlignment alignment;
        alignment.SchoolMask = fields[2].Get<uint32>();
        alignment.AffectsDamage = fields[3].Get<uint8>() != 0;
        alignment.AffectsHealing = fields[4].Get<uint8>() != 0;
        alignment.SpellRestriction = fields[5].Get<uint8>();
        alignment.MaxBaseCastTimeInMS = fields[6].Get<uint32>();
        if (alignment.SpellRestriction > EQTALENTRESTRICTION_PET_SUMMON)
        {
            LOG_ERROR("module", "EverQuest: Talent alignment row for spell ID {} has an invalid SpellRestriction of {}", spellID, alignment.SpellRestriction);
            continue;
        }
        ExplicitAlignmentsBySpellID[spellID].push_back(std::make_pair(effectIndex, alignment));
    } while (queryResult->NextRow());
}

void EverQuestSpellTalentAlignment::ApplyExplicitAlignmentsToSpell(uint32 targetSpellID, uint32 rowSpellID, std::vector<std::pair<uint8, EverQuestTalentModAlignment>> const& explicitAlignments, uint32& explicitModifierCount)
{
    SpellInfo const* targetSpellInfo = sSpellMgr->GetSpellInfo(targetSpellID);
    if (targetSpellInfo == nullptr)
        return;

    std::set<uint8> clearedEffectIndexes;
    for (std::pair<uint8, EverQuestTalentModAlignment> const& alignmentByEffectIndex : explicitAlignments)
    {
        uint8 effectIndex = alignmentByEffectIndex.first;
        SpellEffectInfo const& effectInfo = targetSpellInfo->Effects[effectIndex];
        if (effectInfo.ApplyAuraName != SPELL_AURA_ADD_FLAT_MODIFIER
            && effectInfo.ApplyAuraName != SPELL_AURA_ADD_PCT_MODIFIER)
        {
            LOG_ERROR("module", "EverQuest: Talent alignment row for spell ID {} points at effect index {} of spell {}, but that effect is not a spell modifier",
                rowSpellID, effectIndex, targetSpellID);
            continue;
        }
        uint64 alignmentKey = MakeAlignmentKey(targetSpellID, effectIndex);
        if (clearedEffectIndexes.count(effectIndex) == 0)
        {
            AlignmentsByTalentSpellIDAndEffect[alignmentKey].clear();
            clearedEffectIndexes.insert(effectIndex);
        }
        AlignmentsByTalentSpellIDAndEffect[alignmentKey].push_back(alignmentByEffectIndex.second);
        explicitModifierCount++;
    }
}

void EverQuestSpellTalentAlignment::Load()
{
    if (IsLoaded == true)
        return;
    IsLoaded = true;

    LoadExcludedTalents();
    LoadExplicitAlignments();
    BuildPlayerSpellsByFamily();

    uint32 alignedModifierCount = 0;
    uint32 explicitModifierCount = 0;
    std::set<uint32> appliedExplicitSpellIDs;
    for (uint32 i = 0; i < sTalentStore.GetNumRows(); ++i)
    {
        TalentEntry const* talentEntry = sTalentStore.LookupEntry(i);
        if (talentEntry == nullptr)
            continue;

        // An excluded talent is excluded at every rank, and the table only lists the first one
        if (talentEntry->RankID[0] != 0 && ExcludedTalentRank1SpellIDs.count(talentEntry->RankID[0]) > 0)
        {
            for (uint8 rankIndex = 0; rankIndex < MAX_TALENT_RANK; ++rankIndex)
                if (talentEntry->RankID[rankIndex] != 0)
                    ExcludedTalentSpellIDs.insert(talentEntry->RankID[rankIndex]);
            continue;
        }

        for (uint8 rankIndex = 0; rankIndex < MAX_TALENT_RANK; ++rankIndex)
        {
            uint32 talentSpellID = talentEntry->RankID[rankIndex];
            if (talentSpellID == 0)
                continue;
            SpellInfo const* talentSpellInfo = sSpellMgr->GetSpellInfo(talentSpellID);
            if (talentSpellInfo == nullptr || talentSpellInfo->SpellFamilyName == SPELLFAMILY_GENERIC)
                continue;

            for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
            {
                SpellEffectInfo const& effectInfo = talentSpellInfo->Effects[effectIndex];
                if (effectInfo.ApplyAuraName != SPELL_AURA_ADD_FLAT_MODIFIER
                    && effectInfo.ApplyAuraName != SPELL_AURA_ADD_PCT_MODIFIER)
                    continue;

                EverQuestTalentModAlignment alignment;
                if (BuildAlignmentForMask(talentSpellInfo, effectInfo.SpellClassMask, alignment) == false)
                    continue;

                AlignmentsByTalentSpellIDAndEffect[MakeAlignmentKey(talentSpellID, effectIndex)] = { alignment };
                alignedModifierCount++;
            }

            // Explicit alignments come from the converter for talents whose masks can't be generalized from, and they win over a derived alignment
            auto explicitEntry = ExplicitAlignmentsBySpellID.find(talentEntry->RankID[0]);
            if (explicitEntry != ExplicitAlignmentsBySpellID.end())
            {
                ApplyExplicitAlignmentsToSpell(talentSpellID, talentEntry->RankID[0], explicitEntry->second, explicitModifierCount);
                appliedExplicitSpellIDs.insert(talentEntry->RankID[0]);
            }
        }
    }

    // Rows that matched no talent's first rank point at a non-talent spell (a proc buff like Clearcasting or Fireball), whose modifiers apply to just that spell without any rank expansion
    for (auto const& explicitEntry : ExplicitAlignmentsBySpellID)
    {
        if (appliedExplicitSpellIDs.count(explicitEntry.first) > 0)
            continue;
        if (sSpellMgr->GetSpellInfo(explicitEntry.first) == nullptr)
        {
            LOG_ERROR("module", "EverQuest: Talent alignment row points at spell ID {} which is neither a talent first rank nor a known spell", explicitEntry.first);
            continue;
        }
        ApplyExplicitAlignmentsToSpell(explicitEntry.first, explicitEntry.first, explicitEntry.second, explicitModifierCount);
        appliedExplicitSpellIDs.insert(explicitEntry.first);
    }

    LOG_INFO("module", "EverQuest: Talent alignment loaded {} generalizable and {} explicit talent modifiers, with {} talents ({} ranks) excluded",
        alignedModifierCount, explicitModifierCount, ExcludedTalentRank1SpellIDs.size(), ExcludedTalentSpellIDs.size());

    // A listed talent that never resolved to a rank means the table is pointing at something that is not a talent's first rank
    if (ExcludedTalentSpellIDs.empty() == true && ExcludedTalentRank1SpellIDs.empty() == false)
        LOG_ERROR("module", "EverQuest: Talent alignment has {} exclusion rows but matched no talents, check that mod_everquest_talent_exclusion holds the first rank spell IDs",
            ExcludedTalentRank1SpellIDs.size());
}

bool EverQuestSpellTalentAlignment::DoesSpellInfoDamage(SpellInfo const* spellInfo)
{
    for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
    {
        SpellEffectInfo const& effectInfo = spellInfo->Effects[effectIndex];
        if (effectInfo.Effect == SPELL_EFFECT_SCHOOL_DAMAGE || effectInfo.Effect == SPELL_EFFECT_HEALTH_LEECH)
            return true;
        if (effectInfo.ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE
            || effectInfo.ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE_PERCENT
            || effectInfo.ApplyAuraName == SPELL_AURA_PERIODIC_LEECH)
            return true;
    }
    return false;
}

bool EverQuestSpellTalentAlignment::DoesSpellInfoHeal(SpellInfo const* spellInfo)
{
    for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
    {
        SpellEffectInfo const& effectInfo = spellInfo->Effects[effectIndex];
        if (effectInfo.Effect == SPELL_EFFECT_HEAL || effectInfo.Effect == SPELL_EFFECT_HEAL_MAX_HEALTH)
            return true;
        if (effectInfo.ApplyAuraName == SPELL_AURA_PERIODIC_HEAL)
            return true;
    }
    return false;
}

bool EverQuestSpellTalentAlignment::DoesEffectDealDamage(SpellInfo const* spellInfo, uint8 effectIndex)
{
    SpellEffectInfo const& effectInfo = spellInfo->Effects[effectIndex];
    if (effectInfo.Effect == SPELL_EFFECT_SCHOOL_DAMAGE || effectInfo.Effect == SPELL_EFFECT_HEALTH_LEECH)
        return true;
    if (effectInfo.ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE
        || effectInfo.ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE_PERCENT
        || effectInfo.ApplyAuraName == SPELL_AURA_PERIODIC_LEECH)
        return true;
    return false;
}

bool EverQuestSpellTalentAlignment::DoesSpellInfoDealDirectDamage(SpellInfo const* spellInfo)
{
    for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
    {
        SpellEffectInfo const& effectInfo = spellInfo->Effects[effectIndex];
        if (effectInfo.Effect == SPELL_EFFECT_SCHOOL_DAMAGE || effectInfo.Effect == SPELL_EFFECT_HEALTH_LEECH)
            return true;
    }
    return false;
}

bool EverQuestSpellTalentAlignment::DoesSpellInfoDealAreaDamage(SpellInfo const* spellInfo)
{
    for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
    {
        if (DoesEffectDealDamage(spellInfo, effectIndex) == false)
            continue;
        SpellEffectInfo const& effectInfo = spellInfo->Effects[effectIndex];
        uint32 targetA = effectInfo.TargetA.GetTarget();
        uint32 targetB = effectInfo.TargetB.GetTarget();

        // The converter set area damage with implicit targets 15 (source area enemy) or 16 (destination area enemy)
        if (targetA == TARGET_UNIT_SRC_AREA_ENEMY || targetA == TARGET_UNIT_DEST_AREA_ENEMY || targetB == TARGET_UNIT_SRC_AREA_ENEMY || targetB == TARGET_UNIT_DEST_AREA_ENEMY)
            return true;
    }
    return false;
}

bool EverQuestSpellTalentAlignment::DoesSpellInfoDealPointBlankAreaDamage(SpellInfo const* spellInfo)
{
    for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
    {
        if (DoesEffectDealDamage(spellInfo, effectIndex) == false)
            continue;
        SpellEffectInfo const& effectInfo = spellInfo->Effects[effectIndex];

        // The converter set caster-centered area damage as target A 22 (source is caster) with target B 15 (source area enemy)
        if (effectInfo.TargetA.GetTarget() == TARGET_SRC_CASTER && effectInfo.TargetB.GetTarget() == TARGET_UNIT_SRC_AREA_ENEMY)
            return true;
    }
    return false;
}

bool EverQuestSpellTalentAlignment::DoesSpellInfoDealDirectHeal(SpellInfo const* spellInfo)
{
    for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
    {
        SpellEffectInfo const& effectInfo = spellInfo->Effects[effectIndex];
        if (effectInfo.Effect == SPELL_EFFECT_HEAL || effectInfo.Effect == SPELL_EFFECT_HEAL_MAX_HEALTH)
            return true;
    }
    return false;
}


bool EverQuestSpellTalentAlignment::DoesSpellInfoDealAreaHeal(SpellInfo const* spellInfo)
{
    for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
    {
        // The converter set group heals with implicit target 20 (caster area party), caster-centered area heals with target A 22 (source is caster), and targeted area heals with target B 31 (destination area ally) or 34 (destination area party)
        SpellEffectInfo const& effectInfo = spellInfo->Effects[effectIndex];
        bool healsInSomeWay = effectInfo.Effect == SPELL_EFFECT_HEAL || effectInfo.Effect == SPELL_EFFECT_HEAL_MAX_HEALTH
            || effectInfo.ApplyAuraName == SPELL_AURA_PERIODIC_HEAL;
        if (healsInSomeWay == false)
            continue;
        uint32 targetA = effectInfo.TargetA.GetTarget();
        uint32 targetB = effectInfo.TargetB.GetTarget();
        if (targetA == TARGET_UNIT_CASTER_AREA_PARTY || targetA == TARGET_SRC_CASTER
            || targetB == TARGET_UNIT_DEST_AREA_ALLY || targetB == TARGET_UNIT_DEST_AREA_PARTY)
            return true;
    }
    return false;
}

bool EverQuestSpellTalentAlignment::DoesSpellInfoDealSingleTargetDirectHeal(SpellInfo const* spellInfo)
{
    return DoesSpellInfoDealDirectHeal(spellInfo) == true && DoesSpellInfoDealAreaHeal(spellInfo) == false;
}

bool EverQuestSpellTalentAlignment::DoesSpellInfoDealPeriodicHeal(SpellInfo const* spellInfo)
{
    for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
        if (spellInfo->Effects[effectIndex].ApplyAuraName == SPELL_AURA_PERIODIC_HEAL)
            return true;
    return false;
}

bool EverQuestSpellTalentAlignment::DoesSpellInfoDealPeriodicDamage(SpellInfo const* spellInfo)
{
    for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
    {
        SpellEffectInfo const& effectInfo = spellInfo->Effects[effectIndex];
        if (effectInfo.ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE
            || effectInfo.ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE_PERCENT
            || effectInfo.ApplyAuraName == SPELL_AURA_PERIODIC_LEECH)
            return true;
    }
    return false;
}

bool EverQuestSpellTalentAlignment::DoesSpellInfoLeech(SpellInfo const* spellInfo)
{
    for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
    {
        SpellEffectInfo const& effectInfo = spellInfo->Effects[effectIndex];

        // The converter set EQ lifetaps as health leech (direct) or periodic leech (over time) effects on the same spell
        if (effectInfo.Effect == SPELL_EFFECT_HEALTH_LEECH || effectInfo.ApplyAuraName == SPELL_AURA_PERIODIC_LEECH)
            return true;
    }
    return false;
}

bool EverQuestSpellTalentAlignment::DoesSpellInfoSummonAPet(SpellInfo const* spellInfo)
{
    for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
    {
        // The converter set EQ pet summons as summon pet effects, or as plain summons whose generated summon properties mark an ally pet when the EQ level and behavior pet config is on
        SpellEffectInfo const& effectInfo = spellInfo->Effects[effectIndex];
        if (effectInfo.Effect == SPELL_EFFECT_SUMMON_PET)
            return true;
        if (effectInfo.Effect == SPELL_EFFECT_SUMMON)
        {
            SummonPropertiesEntry const* summonProperties = sSummonPropertiesStore.LookupEntry(effectInfo.MiscValueB);
            if (summonProperties != nullptr && summonProperties->Category == SUMMON_CATEGORY_ALLY && summonProperties->Type == SUMMON_TYPE_PET)
                return true;
        }
    }
    return false;
}

bool EverQuestSpellTalentAlignment::DoesSpellInfoCure(SpellInfo const* spellInfo)
{
    // The converter set EQ cancel magic / cure poison / cure disease as dispel effects
    for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
        if (spellInfo->Effects[effectIndex].Effect == SPELL_EFFECT_DISPEL)
            return true;
    return false;
}

bool EverQuestSpellTalentAlignment::IsEQSpell(SpellInfo const* spellInfo)
{
    return spellInfo->Id >= EverQuest->ConfigSystemSpellDBCIDMin && spellInfo->Id <= EverQuest->ConfigSystemSpellDBCIDMax;
}

bool EverQuestSpellTalentAlignment::IsEffectAStrengthReduction(SpellInfo const* spellInfo, uint8 effectIndex)
{
    // The converter set EQ strength reductions as mod stat auras with misc value 0 (strength) and a negative amount
    SpellEffectInfo const& effectInfo = spellInfo->Effects[effectIndex];
    if (effectInfo.ApplyAuraName != SPELL_AURA_MOD_STAT)
        return false;
    if (effectInfo.MiscValue != STAT_STRENGTH)
        return false;
    return effectInfo.BasePoints < 0;
}

bool EverQuestSpellTalentAlignment::DoesModTargetAStrengthReduction(SpellInfo const* eqSpellInfo, SpellModifier const* spellMod)
{
    // A modifier like Improved Curse of Weakness targets one effect index, so it only aligns when the EQ effect in that slot is the strength reduction (a debuff can bundle several stats, one per effect slot)
    switch (spellMod->op)
    {
        case SPELLMOD_EFFECT1: return IsEffectAStrengthReduction(eqSpellInfo, EFFECT_0);
        case SPELLMOD_EFFECT2: return IsEffectAStrengthReduction(eqSpellInfo, EFFECT_1);
        case SPELLMOD_EFFECT3: return IsEffectAStrengthReduction(eqSpellInfo, EFFECT_2);
        default:
        {
            for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
                if (IsEffectAStrengthReduction(eqSpellInfo, effectIndex) == true)
                    return true;
            return false;
        }
    }
}

bool EverQuestSpellTalentAlignment::DoesAlignmentReachEQSpell(EverQuestTalentModAlignment const& alignment, SpellInfo const* eqSpellInfo, SpellModifier const* spellMod)
{
    // Cures and strength debuffs are neither damage nor healing, so those restrictions are their own reach tests
    if (alignment.SpellRestriction == EQTALENTRESTRICTION_CURE)
    {
        if (DoesSpellInfoCure(eqSpellInfo) == false)
            return false;
    }
    else if (alignment.SpellRestriction == EQTALENTRESTRICTION_STRENGTH_DEBUFF)
    {
        if (DoesModTargetAStrengthReduction(eqSpellInfo, spellMod) == false)
            return false;
    }
    else if (alignment.SpellRestriction == EQTALENTRESTRICTION_PET_SUMMON)
    {
        if (DoesSpellInfoSummonAPet(eqSpellInfo) == false)
            return false;
    }
    else
    {
        bool reaches = false;

        // A healing modifier reaches any EQ heal, whatever school the EQ resist gave it
        if (alignment.AffectsHealing == true && DoesSpellInfoHeal(eqSpellInfo) == true)
            reaches = true;

        // A damage modifier reaches EQ damage of the same school, and only that school
        if (reaches == false && alignment.AffectsDamage == true && DoesSpellInfoDamage(eqSpellInfo) == true
            && (alignment.SchoolMask & eqSpellInfo->SchoolMask) != 0)
            reaches = true;

        if (reaches == false)
            return false;

        switch (alignment.SpellRestriction)
        {
            case EQTALENTRESTRICTION_DIRECT_DAMAGE:
            {
                if (DoesSpellInfoDealDirectDamage(eqSpellInfo) == false)
                    return false;
            } break;
            case EQTALENTRESTRICTION_AREA_DAMAGE:
            {
                if (DoesSpellInfoDealAreaDamage(eqSpellInfo) == false)
                    return false;
            } break;
            case EQTALENTRESTRICTION_POINT_BLANK_AREA_DAMAGE:
            {
                if (DoesSpellInfoDealPointBlankAreaDamage(eqSpellInfo) == false)
                    return false;
            } break;
            case EQTALENTRESTRICTION_DIRECT_HEAL:
            {
                if (DoesSpellInfoDealDirectHeal(eqSpellInfo) == false)
                    return false;
            } break;
            case EQTALENTRESTRICTION_SINGLE_TARGET_DIRECT_HEAL:
            {
                if (DoesSpellInfoDealSingleTargetDirectHeal(eqSpellInfo) == false)
                    return false;
            } break;
            case EQTALENTRESTRICTION_AREA_HEAL:
            {
                if (DoesSpellInfoDealAreaHeal(eqSpellInfo) == false)
                    return false;
            } break;
            case EQTALENTRESTRICTION_PERIODIC_HEAL:
            {
                if (DoesSpellInfoDealPeriodicHeal(eqSpellInfo) == false)
                    return false;
            } break;
            case EQTALENTRESTRICTION_PERIODIC_DAMAGE:
            {
                if (DoesSpellInfoDealPeriodicDamage(eqSpellInfo) == false)
                    return false;
            } break;
            default: break;
        }
    }

    // A cast time ceiling keeps "next spell is instant" abilities off long EQ casts, the way Presence of Mind skips 10+ second WOW casts
    if (alignment.MaxBaseCastTimeInMS > 0)
    {
        int32 baseCastTime = eqSpellInfo->CastTimeEntry != nullptr ? eqSpellInfo->CastTimeEntry->CastTime : 0;
        if (baseCastTime >= static_cast<int32>(alignment.MaxBaseCastTimeInMS))
            return false;
    }
    return true;
}

bool EverQuestSpellTalentAlignment::ShouldTalentModAffectEQSpell(SpellInfo const* talentSpellInfo, SpellInfo const* eqSpellInfo,
    SpellModifier const* spellMod)
{
    if (IsLoaded == false)
        return false;
    if (IsEQSpell(eqSpellInfo) == false)
        return false;
    if (ExcludedTalentSpellIDs.count(talentSpellInfo->Id) > 0)
        return false;

    for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
    {
        if (talentSpellInfo->Effects[effectIndex].SpellClassMask != spellMod->mask)
            continue;

        auto alignmentEntry = AlignmentsByTalentSpellIDAndEffect.find(MakeAlignmentKey(talentSpellInfo->Id, effectIndex));
        if (alignmentEntry == AlignmentsByTalentSpellIDAndEffect.end())
            continue;

        for (EverQuestTalentModAlignment const& alignment : alignmentEntry->second)
        {
            if (DoesAlignmentReachEQSpell(alignment, eqSpellInfo, spellMod) == true)
                return true;
        }
    }
    return false;
}
