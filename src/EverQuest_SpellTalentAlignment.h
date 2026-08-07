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

#ifndef MOD_EVERQUEST_SPELL_TALENT_ALIGNMENT_H
#define MOD_EVERQUEST_SPELL_TALENT_ALIGNMENT_H

#include "Define.h"
#include "SharedDefines.h"
#include "Util.h"

#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class SpellInfo;
struct SpellModifier;

enum EverQuestTalentAlignmentRestriction : uint8
{
    EQTALENTRESTRICTION_NONE = 0,
    EQTALENTRESTRICTION_DIRECT_DAMAGE = 1,           // Only EQ spells with a direct (non-periodic) damage effect
    EQTALENTRESTRICTION_AREA_DAMAGE = 2,             // Only EQ spells whose damage lands in an area
    EQTALENTRESTRICTION_POINT_BLANK_AREA_DAMAGE = 3, // Only EQ spells whose damage area is centered on the caster
    EQTALENTRESTRICTION_DIRECT_HEAL = 4,             // Only EQ spells with a direct (non-periodic) heal effect
    EQTALENTRESTRICTION_SINGLE_TARGET_DIRECT_HEAL = 5, // Direct heal that lands on one target (no group or area heal targets)
    EQTALENTRESTRICTION_AREA_HEAL = 6,               // Heal that lands on a group or area
    EQTALENTRESTRICTION_PERIODIC_HEAL = 7,           // Only EQ spells with a heal over time effect
    EQTALENTRESTRICTION_PERIODIC_DAMAGE = 8,         // Only EQ spells with a damage over time effect
    EQTALENTRESTRICTION_CURE = 9,                    // EQ dispel / cure spells, reached regardless of the damage and healing flags
    EQTALENTRESTRICTION_STRENGTH_DEBUFF = 10,        // The effect the modifier targets is a strength reduction, reached regardless of the damage and healing flags
    EQTALENTRESTRICTION_PET_SUMMON = 11              // EQ pet summoning spells, reached regardless of the damage and healing flags
};

struct EverQuestTalentModAlignment
{
    uint32 SchoolMask = 0;
    bool AffectsDamage = false;
    bool AffectsHealing = false;
    uint8 SpellRestriction = EQTALENTRESTRICTION_NONE;
    uint32 MaxBaseCastTimeInMS = 0; // If > 0, only EQ spells with a base cast time under this are affected
};

class EverQuestSpellTalentAlignment
{
public:
    static EverQuestSpellTalentAlignment* instance();

    void Load();
    bool ShouldTalentModAffectEQSpell(SpellInfo const* talentSpellInfo, SpellInfo const* eqSpellInfo, SpellModifier const* spellMod);

    static bool DoesSpellInfoDamage(SpellInfo const* spellInfo);
    static bool DoesSpellInfoHeal(SpellInfo const* spellInfo);
    static bool DoesSpellInfoDealDirectDamage(SpellInfo const* spellInfo);
    static bool DoesSpellInfoDealAreaDamage(SpellInfo const* spellInfo);
    static bool DoesSpellInfoDealPointBlankAreaDamage(SpellInfo const* spellInfo);
    static bool DoesSpellInfoDealDirectHeal(SpellInfo const* spellInfo);
    static bool DoesSpellInfoDealSingleTargetDirectHeal(SpellInfo const* spellInfo);
    static bool DoesSpellInfoDealAreaHeal(SpellInfo const* spellInfo);
    static bool DoesSpellInfoDealPeriodicHeal(SpellInfo const* spellInfo);
    static bool DoesSpellInfoDealPeriodicDamage(SpellInfo const* spellInfo);
    static bool DoesSpellInfoCure(SpellInfo const* spellInfo);
    static bool DoesSpellInfoLeech(SpellInfo const* spellInfo);
    static bool DoesSpellInfoSummonAPet(SpellInfo const* spellInfo);

private:
    bool IsLoaded = false;
    std::unordered_map<uint32, std::vector<SpellInfo const*>> PlayerSpellsByFamily;
    std::unordered_map<uint64, std::vector<EverQuestTalentModAlignment>> AlignmentsByTalentSpellIDAndEffect;
    std::set<uint32> ExcludedTalentRank1SpellIDs;
    std::set<uint32> ExcludedTalentSpellIDs;
    std::unordered_map<uint32, std::vector<std::pair<uint8, EverQuestTalentModAlignment>>> ExplicitAlignmentsBySpellID;

    void LoadExcludedTalents();
    void LoadExplicitAlignments();
    void BuildPlayerSpellsByFamily();
    bool BuildAlignmentForMask(SpellInfo const* talentSpellInfo, flag96 const& classMask, EverQuestTalentModAlignment& alignmentOut);
    void ApplyExplicitAlignmentsToSpell(uint32 targetSpellID, uint32 rowSpellID, std::vector<std::pair<uint8, EverQuestTalentModAlignment>> const& explicitAlignments, uint32& explicitModifierCount);
    bool DoesAlignmentReachEQSpell(EverQuestTalentModAlignment const& alignment, SpellInfo const* eqSpellInfo, SpellModifier const* spellMod);
    static bool IsEffectAStrengthReduction(SpellInfo const* spellInfo, uint8 effectIndex);
    static bool DoesModTargetAStrengthReduction(SpellInfo const* eqSpellInfo, SpellModifier const* spellMod);
    static bool DoesEffectDealDamage(SpellInfo const* spellInfo, uint8 effectIndex);
    static bool IsEQSpell(SpellInfo const* spellInfo);
    static uint64 MakeAlignmentKey(uint32 talentSpellID, uint8 effectIndex)
    {
        return (static_cast<uint64>(talentSpellID) << 8) | static_cast<uint64>(effectIndex);
    }
};

#define EverQuestTalentAlignment EverQuestSpellTalentAlignment::instance()

#endif // MOD_EVERQUEST_SPELL_TALENT_ALIGNMENT_H
