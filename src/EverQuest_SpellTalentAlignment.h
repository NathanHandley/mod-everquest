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
#include <vector>

class SpellInfo;
struct SpellModifier;

struct EverQuestTalentModAlignment
{
    uint32 SchoolMask = 0;
    bool AffectsDamage = false;
    bool AffectsHealing = false;
};

// Every SPELL_AURA_ADD_FLAT_MODIFIER / SPELL_AURA_ADD_PCT_MODIFIER talent is checked by SpelllInfo::IsAffected(talentFamily, mask).  An EQ spell carries no WOW family,
// and a spell only has room for one family so there is never "a talent that adds shadow damage should add shadow damage to any shadow spell the player casts".
// SpellInfo::IsAffectedBySpellMod calls OnIsAffectedBySpellModCheck before it, and treats a false return as "affected", so the decision can be made here on meaning instead.
class EverQuestSpellTalentAlignment
{
public:
    static EverQuestSpellTalentAlignment* instance();

    void Load();
    bool ShouldTalentModAffectEQSpell(SpellInfo const* talentSpellInfo, SpellInfo const* eqSpellInfo, SpellModifier const* spellMod);

private:
    bool IsLoaded = false;
    std::unordered_map<uint32, std::vector<SpellInfo const*>> PlayerSpellsByFamily;
    std::unordered_map<uint64, EverQuestTalentModAlignment> AlignmentByTalentSpellIDAndEffect;
    std::set<uint32> ExcludedTalentRank1SpellIDs;
    std::set<uint32> ExcludedTalentSpellIDs;

    void LoadExcludedTalents();
    void BuildPlayerSpellsByFamily();
    bool BuildAlignmentForMask(SpellInfo const* talentSpellInfo, flag96 const& classMask, EverQuestTalentModAlignment& alignmentOut);
    static bool DoesSpellInfoDamage(SpellInfo const* spellInfo);
    static bool DoesSpellInfoHeal(SpellInfo const* spellInfo);
    static bool IsEQSpell(SpellInfo const* spellInfo);
    static uint64 MakeAlignmentKey(uint32 talentSpellID, uint8 effectIndex)
    {
        return (static_cast<uint64>(talentSpellID) << 8) | static_cast<uint64>(effectIndex);
    }
};

#define EverQuestTalentAlignment EverQuestSpellTalentAlignment::instance()

#endif // MOD_EVERQUEST_SPELL_TALENT_ALIGNMENT_H
