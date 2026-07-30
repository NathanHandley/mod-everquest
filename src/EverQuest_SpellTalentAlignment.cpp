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

void EverQuestSpellTalentAlignment::Load()
{
    if (IsLoaded == true)
        return;
    IsLoaded = true;

    LoadExcludedTalents();
    BuildPlayerSpellsByFamily();

    uint32 alignedModifierCount = 0;
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

                AlignmentByTalentSpellIDAndEffect[MakeAlignmentKey(talentSpellID, effectIndex)] = alignment;
                alignedModifierCount++;
            }
        }
    }
    LOG_INFO("module", "EverQuest: Talent alignment loaded {} generalizable talent modifiers, with {} talents ({} ranks) excluded",
        alignedModifierCount, ExcludedTalentRank1SpellIDs.size(), ExcludedTalentSpellIDs.size());

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

bool EverQuestSpellTalentAlignment::IsEQSpell(SpellInfo const* spellInfo)
{
    return spellInfo->Id >= EverQuest->ConfigSystemSpellDBCIDMin && spellInfo->Id <= EverQuest->ConfigSystemSpellDBCIDMax;
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

        auto alignmentEntry = AlignmentByTalentSpellIDAndEffect.find(MakeAlignmentKey(talentSpellInfo->Id, effectIndex));
        if (alignmentEntry == AlignmentByTalentSpellIDAndEffect.end())
            continue;
        EverQuestTalentModAlignment const& alignment = alignmentEntry->second;

        // A healing modifier reaches any EQ heal, whatever school the EQ resist model gave it
        if (alignment.AffectsHealing == true && DoesSpellInfoHeal(eqSpellInfo) == true)
            return true;

        // A damage modifier reaches EQ damage of the same school, and only that school
        if (alignment.AffectsDamage == true && DoesSpellInfoDamage(eqSpellInfo) == true
            && (alignment.SchoolMask & eqSpellInfo->SchoolMask) != 0)
            return true;
    }
    return false;
}
