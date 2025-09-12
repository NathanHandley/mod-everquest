//  Author: Nathan Handley (nathanhandley@protonmail.com)
//  Copyright (c) 2025 Nathan Handley
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
#include "SpellAuras.h"
#include "SpellScript.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_FocusBoostScript: public AuraScript, public SpellScript
{
    PrepareAuraScript(EverQuest_FocusBoostScript);
    PrepareSpellScript(EverQuest_FocusBoostScript);

    uint32 CalculateBoostPercent(Unit* caster, uint32 spellID)
    {
        // Calculate the boost amount based on caster's auras
        EverQuestSpell curSpell = EverQuest->GetSpellDataForSpellID(spellID);
        uint32 boostPercent = 0;
        Unit::AuraMap const& auras = caster->GetOwnedAuras();
        for (auto const& aurIter : auras)
        {
            Aura* aura = aurIter.second;
            SpellInfo const* auraInfo = aura->GetSpellInfo();
            for (uint8 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
            {
                // Focus auras are always dummy
                if (auraInfo->Effects[effIndex].ApplyAuraName != SPELL_AURA_DUMMY)
                    continue;
                int auraDummyType = auraInfo->Effects[effIndex].MiscValue;

                // Match up focus types and add the boost
                if (curSpell.FocusBoostType == EQ_SPELLFOCUSBOOSTTYPE_BARDBRASS && (auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSBRASS || auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSALL))
                {
                    boostPercent += auraInfo->Effects[effIndex].MiscValueB;
                    continue;
                }
                else if (curSpell.FocusBoostType == EQ_SPELLFOCUSBOOSTTYPE_BARDSTRINGED && (auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSSTRING || auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSALL))
                {
                    boostPercent += auraInfo->Effects[effIndex].MiscValueB;
                    continue;
                }
                else if (curSpell.FocusBoostType == EQ_SPELLFOCUSBOOSTTYPE_BARDWIND && (auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSWIND || auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSALL))
                {
                    boostPercent += auraInfo->Effects[effIndex].MiscValueB;
                    continue;
                }
                else if (curSpell.FocusBoostType == EQ_SPELLFOCUSBOOSTTYPE_BARDPERCUSSION && (auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSPERCUSSION || auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSALL))
                {
                    boostPercent += auraInfo->Effects[effIndex].MiscValueB;
                    continue;
                }
                else if (curSpell.FocusBoostType == EQ_SPELLFOCUSBOOSTTYPE_BARDSINGING && auraDummyType == EQ_SPELLDUMMYTYPE_BARDFOCUSALL)
                {
                    boostPercent += auraInfo->Effects[effIndex].MiscValueB;
                    continue;
                }
            }
        }
        return boostPercent;
    }

    // Handle non-aura effects (e.g., direct damage, heals)
    void HandleEffectHitTarget(SpellEffIndex effIndex)
    {
        // Invalid casters need to be skipped
        Unit* caster = SpellScript::GetCaster();
        if (caster == nullptr)
            return;

        // Only EQ spells
        uint32 spellID = GetId();
        if (EverQuest->IsSpellAnEQSpell(spellID) == false)
            return;

        // Skip unsupported types
        uint32 effect = SpellScript::GetSpellInfo()->Effects[effIndex].Effect;
        if (effect != SPELL_EFFECT_SCHOOL_DAMAGE &&
            effect != SPELL_EFFECT_WEAPON_DAMAGE &&
            effect != SPELL_EFFECT_HEAL &&
            effect != SPELL_EFFECT_NORMALIZED_WEAPON_DMG &&
            effect != SPELL_EFFECT_ENERGIZE)
        {
            return;
        }

        uint32 boostPercent = CalculateBoostPercent(caster, spellID);
        if (effect == SPELL_EFFECT_HEAL)
        {
            int32 healAmount = GetHitHeal();
            if (healAmount != 0)
                SetHitHeal(int32(std::ceil((float)healAmount * (1.0f + (float)boostPercent / 100.0f))));
        }
        else
        {
            int32 damageAmount = GetHitDamage();
            if (damageAmount != 0)
                SetHitDamage(int32(std::ceil((float)damageAmount * (1.0f + (float)boostPercent / 100.0f))));
        }
    }

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        // Invalid casters need to be skipped
        Unit* caster = AuraScript::GetCaster();
        if (caster == nullptr)
            return;

        // Only EQ spells
        uint32 spellID = GetId();
        if (EverQuest->IsSpellAnEQSpell(spellID) == false)
            return;

        uint32 boostPercent = CalculateBoostPercent(caster, spellID);

        // Always round up
        if (boostPercent != 0)
            amount = int32(std::ceil((float)amount * (1.0f + (float)boostPercent / 100.0f)));
    }

    void Register() override
    {
        SpellInfo const* spellInfo = SpellScript::GetSpellInfo();
        bool isAura = false;
        bool isNonAura = false;
        for (uint8 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
        {
            if (spellInfo->Effects[effIndex].IsAura() == true)
                isAura = true;
            else if (spellInfo->Effects[effIndex].IsEffect() == true)
                isNonAura = true;
        }

        if (isAura == true)
            DoEffectCalcAmount += AuraEffectCalcAmountFn(EverQuest_FocusBoostScript::CalculateAmount, EFFECT_ALL, SPELL_AURA_ANY);
        if (isNonAura == true)
            OnEffectHitTarget += SpellEffectFn(EverQuest_FocusBoostScript::HandleEffectHitTarget, EFFECT_ALL, SPELL_EFFECT_ANY);
    }
};

void AddEverQuestFocusBoostScripts()
{
    RegisterSpellScript(EverQuest_FocusBoostScript);
}
