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
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "Unit.h"

#include "EverQuest.h"

#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <vector>

using namespace std;

namespace
{
    // Mirrors the hardcoded cap in Spell::DoAllEffectOnLaunchTarget
    const uint32 CORE_AREA_DAMAGE_TARGET_CAP = 10;

    struct EverQuestRainTargetBudget
    {
        int32 LandingsRemaining = 0;
        bool HitsEveryTarget = false;
        unordered_map<ObjectGuid, int32> LandingsByTargetGUID; // Only filled when HitsEveryTarget is on
        ObjectGuid CloudGUID; // The cloud's DynamicObject, once the cloud has claimed this budget
        uint32 StartTimeInMS = 0;
        uint32 LifetimeInMS = 0;
    };

    // Casts and aura ticks run on parallel map threads, so every read or write of the containers below holds this lock
    std::mutex RainTargetBudgetsMutex;
    uint32 LastRainTargetBudgetID = 0;
    unordered_map<uint32, EverQuestRainTargetBudget> RainTargetBudgetsByID;
    unordered_map<uint64, uint32> UnclaimedRainTargetBudgetIDsByCasterAndCloudSpell;
    unordered_map<ObjectGuid, uint32> RainTargetBudgetIDsByCloudGUID;

    uint64 GetRainCasterAndCloudSpellKey(ObjectGuid casterGUID, uint32 cloudSpellID)
    {
        return (uint64(casterGUID.GetCounter()) << 32) | uint64(cloudSpellID);
    }

    // Mirrors TAKP HasDirectDamageEffect, which is what picks between the two MAX_TARGETS_ALLOWED values
    int32 GetRainTargetHitCap(SpellInfo const* spellInfo)
    {
        if (spellInfo == nullptr)
            return 0;
        for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
        {
            if (spellInfo->Effects[effectIndex].ApplyAuraName != SPELL_AURA_NONE)
                continue;
            if (spellInfo->Effects[effectIndex].Effect == SPELL_EFFECT_SCHOOL_DAMAGE ||
                spellInfo->Effects[effectIndex].Effect == SPELL_EFFECT_HEALTH_LEECH)
                return EverQuest->ConfigRainTargetHitCap;
        }
        return EverQuest->ConfigRainTargetHitCapNoDirectDamage;
    }

    // A rain's cast chains its cloud through spell_linked_spell (SPELL_LINK_CAST), and the cloud is the only thing it chains that sits on the ground and triggers a spell on a timer
    uint32 GetRainCloudSpellID(uint32 rainSpellID)
    {
        std::vector<int32> const* linkedSpellIDs = sSpellMgr->GetSpellLinked(int32(rainSpellID));
        if (linkedSpellIDs == nullptr)
            return 0;
        for (int32 linkedSpellID : *linkedSpellIDs)
        {
            if (linkedSpellID <= 0)
                continue;
            SpellInfo const* linkedSpellInfo = sSpellMgr->GetSpellInfo(uint32(linkedSpellID));
            if (linkedSpellInfo == nullptr)
                continue;
            for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
                if (linkedSpellInfo->Effects[effectIndex].Effect == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                    linkedSpellInfo->Effects[effectIndex].ApplyAuraName == SPELL_AURA_PERIODIC_TRIGGER_SPELL)
                    return linkedSpellInfo->Id;
        }
        return 0;
    }

    // The Wizard toggle is read off the caster, and the toggle code already takes it away from anyone who is no longer a Wizard
    bool IsIntensifiedSkyfallOnForCaster(Unit* caster)
    {
        if (caster == nullptr || caster->IsPlayer() == false || EverQuest->IsClassAuraSystemEnabled() == false)
            return false;
        uint32 skyfallSpellID = EverQuest->GetClassAuraSpellID(EQ_CLASSAURA_SPELL_WIZARD_INTENSIFIED_SKYFALL);
        return skyfallSpellID != 0 && caster->HasAura(skyfallSpellID) == true;
    }

    // The same range test Spell::CheckRange puts a triggered wave through (strict, since triggered casts are checked at prepare)
    bool IsRainWaveOutOfRange(Unit* caster, Unit* target, SpellInfo const* waveSpellInfo)
    {
        if (caster == target || waveSpellInfo->RangeEntry == nullptr || waveSpellInfo->RangeEntry->ID == 1 || waveSpellInfo->RangeEntry->Flags == SPELL_RANGE_MELEE)
            return false;
        float maxRange = caster->GetSpellMaxRangeForTarget(target, waveSpellInfo);
        float minRange = caster->GetSpellMinRangeForTarget(target, waveSpellInfo);
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(waveSpellInfo->Id, SPELLMOD_RANGE, maxRange);
        if (caster->IsWithinCombatRange(target, maxRange) == false)
            return true;
        return minRange > 0.0f && caster->IsWithinCombatRange(target, minRange) == true;
    }

    // Must be called with RainTargetBudgetsMutex held
    bool HasRainTargetBudgetExpired(EverQuestRainTargetBudget const& budget)
    {
        return GetMSTimeDiffToNow(budget.StartTimeInMS) >= budget.LifetimeInMS;
    }

    // Must be called with RainTargetBudgetsMutex held.  Budgets outlive their rain by a wave so that a late tick still finds one
    void PurgeExpiredRainTargetBudgetsLocked()
    {
        for (auto budgetIterator = RainTargetBudgetsByID.begin(); budgetIterator != RainTargetBudgetsByID.end();)
        {
            if (HasRainTargetBudgetExpired(budgetIterator->second) == false)
            {
                ++budgetIterator;
                continue;
            }
            auto cloudIterator = RainTargetBudgetIDsByCloudGUID.find(budgetIterator->second.CloudGUID);
            if (cloudIterator != RainTargetBudgetIDsByCloudGUID.end() && cloudIterator->second == budgetIterator->first)
                RainTargetBudgetIDsByCloudGUID.erase(cloudIterator);
            budgetIterator = RainTargetBudgetsByID.erase(budgetIterator);
        }
        for (auto unclaimedIterator = UnclaimedRainTargetBudgetIDsByCasterAndCloudSpell.begin(); unclaimedIterator != UnclaimedRainTargetBudgetIDsByCasterAndCloudSpell.end();)
        {
            if (RainTargetBudgetsByID.find(unclaimedIterator->second) == RainTargetBudgetsByID.end())
                unclaimedIterator = UnclaimedRainTargetBudgetIDsByCasterAndCloudSpell.erase(unclaimedIterator);
            else
                ++unclaimedIterator;
        }
    }

    // Must be called with RainTargetBudgetsMutex held.  ID 0 stands for "no budget", so it is never handed out
    uint32 CreateRainTargetBudgetLocked(int32 hitCap, uint32 lifetimeInMS, bool hitsEveryTarget)
    {
        PurgeExpiredRainTargetBudgetsLocked();
        ++LastRainTargetBudgetID;
        if (LastRainTargetBudgetID == 0)
            ++LastRainTargetBudgetID;
        EverQuestRainTargetBudget& budget = RainTargetBudgetsByID[LastRainTargetBudgetID];
        budget = EverQuestRainTargetBudget();
        budget.LandingsRemaining = hitCap;
        budget.HitsEveryTarget = hitsEveryTarget;
        budget.StartTimeInMS = getMSTime();
        budget.LifetimeInMS = lifetimeInMS;
        return LastRainTargetBudgetID;
    }

    uint32 StartRainTargetBudget(ObjectGuid casterGUID, uint32 cloudSpellID, int32 hitCap, uint32 lifetimeInMS, bool hitsEveryTarget)
    {
        std::lock_guard<std::mutex> lock(RainTargetBudgetsMutex);
        uint32 budgetID = CreateRainTargetBudgetLocked(hitCap, lifetimeInMS, hitsEveryTarget);
        UnclaimedRainTargetBudgetIDsByCasterAndCloudSpell[GetRainCasterAndCloudSpellKey(casterGUID, cloudSpellID)] = budgetID;
        return budgetID;
    }

    uint32 ClaimRainTargetBudgetForCloud(ObjectGuid casterGUID, uint32 cloudSpellID, ObjectGuid cloudGUID)
    {
        std::lock_guard<std::mutex> lock(RainTargetBudgetsMutex);
        auto unclaimedIterator = UnclaimedRainTargetBudgetIDsByCasterAndCloudSpell.find(GetRainCasterAndCloudSpellKey(casterGUID, cloudSpellID));
        if (unclaimedIterator == UnclaimedRainTargetBudgetIDsByCasterAndCloudSpell.end())
            return 0;
        uint32 budgetID = unclaimedIterator->second;
        UnclaimedRainTargetBudgetIDsByCasterAndCloudSpell.erase(unclaimedIterator);
        auto budgetIterator = RainTargetBudgetsByID.find(budgetID);
        if (budgetIterator == RainTargetBudgetsByID.end())
            return 0;
        budgetIterator->second.CloudGUID = cloudGUID;
        RainTargetBudgetIDsByCloudGUID[cloudGUID] = budgetID;
        return budgetID;
    }

    bool TryConsumeRainTargetLanding(uint32& budgetID, ObjectGuid targetGUID, int32 hitCap, uint32 lifetimeInMS)
    {
        if (hitCap <= 0)
            return true;
        std::lock_guard<std::mutex> lock(RainTargetBudgetsMutex);
        auto budgetIterator = RainTargetBudgetsByID.find(budgetID);
        if (budgetID == 0 || budgetIterator == RainTargetBudgetsByID.end() || HasRainTargetBudgetExpired(budgetIterator->second) == true)
        {
            budgetID = CreateRainTargetBudgetLocked(hitCap, lifetimeInMS, false);
            budgetIterator = RainTargetBudgetsByID.find(budgetID);
        }
        EverQuestRainTargetBudget& budget = budgetIterator->second;
        if (budget.HitsEveryTarget == true)
        {
            int32& targetLandings = budget.LandingsByTargetGUID[targetGUID];
            if (targetLandings >= hitCap)
                return false;
            ++targetLandings;
            return true;
        }
        if (budget.LandingsRemaining <= 0)
            return false;
        --budget.LandingsRemaining;
        return true;
    }

    bool DoesRainHitEveryTarget(ObjectGuid cloudGUID)
    {
        std::lock_guard<std::mutex> lock(RainTargetBudgetsMutex);
        auto cloudIterator = RainTargetBudgetIDsByCloudGUID.find(cloudGUID);
        if (cloudIterator == RainTargetBudgetIDsByCloudGUID.end())
            return false;
        auto budgetIterator = RainTargetBudgetsByID.find(cloudIterator->second);
        if (budgetIterator == RainTargetBudgetsByID.end() || HasRainTargetBudgetExpired(budgetIterator->second) == true)
            return false;
        return budgetIterator->second.HitsEveryTarget;
    }

    uint32 GetRainTargetBudgetLifetimeInMS(SpellInfo const* cloudSpellInfo)
    {
        uint32 cloudDurationInMS = cloudSpellInfo == nullptr ? 0 : uint32(std::max(0, cloudSpellInfo->GetMaxDuration()));
        return cloudDurationInMS + 5000;
    }

    bool IsGUIDInList(std::vector<ObjectGuid> const& guids, ObjectGuid guid)
    {
        return std::find(guids.begin(), guids.end(), guid) != guids.end();
    }
}

uint32 EverQuestMod::GetRainWaveSpellID(uint32 rainSpellID, uint32& cloudDurationInMS)
{
    cloudDurationInMS = 0;
    SpellInfo const* cloudSpellInfo = sSpellMgr->GetSpellInfo(GetRainCloudSpellID(rainSpellID));
    if (cloudSpellInfo == nullptr)
        return 0;
    for (uint8 effectIndex = 0; effectIndex < MAX_SPELL_EFFECTS; ++effectIndex)
    {
        if (cloudSpellInfo->Effects[effectIndex].ApplyAuraName != SPELL_AURA_PERIODIC_TRIGGER_SPELL)
            continue;
        cloudDurationInMS = uint32(std::max(0, cloudSpellInfo->GetMaxDuration()));
        return cloudSpellInfo->Effects[effectIndex].TriggerSpell;
    }
    return 0;
}

class EverQuest_RainTargetBudgetSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_RainTargetBudgetSpellScript);

    uint32 RainTargetBudgetID = 0;
    bool BudgetWasStarted = false;
    std::vector<ObjectGuid> AllowedTargetGUIDs;

    void HandleAreaTargetSelect(std::list<WorldObject*>& targets)
    {
        if (EverQuest->IsEnabled == false)
            return;

        // EQ leaves a creature's own rain uncapped (MAX_TARGETS_ALLOWED is 999 for NPC casters)
        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsPlayer() == false)
            return;

        uint32 cloudSpellID = GetRainCloudSpellID(GetSpellInfo()->Id);
        if (cloudSpellID == 0)
            return;
        int32 hitCap = GetRainTargetHitCap(GetSpellInfo());
        if (hitCap <= 0)
            return;
        uint32 lifetimeInMS = GetRainTargetBudgetLifetimeInMS(sSpellMgr->GetSpellInfo(cloudSpellID));

        // Two effects aimed at the same area each call this, so the budget is spent once and both lists are cut to the same units
        if (BudgetWasStarted == false)
        {
            RainTargetBudgetID = StartRainTargetBudget(caster->GetGUID(), cloudSpellID, hitCap, lifetimeInMS, IsIntensifiedSkyfallOnForCaster(caster));
            BudgetWasStarted = true;

            // Whatever the rain was aimed at gets the first landing, since the core hits an explicit target whether or not it survives this filter, and it should be paying for that landing like any other
            Unit* explicitTarget = GetExplTargetUnit();
            if (explicitTarget != nullptr)
                if (TryConsumeRainTargetLanding(RainTargetBudgetID, explicitTarget->GetGUID(), hitCap, lifetimeInMS) == true)
                    AllowedTargetGUIDs.push_back(explicitTarget->GetGUID());

            for (WorldObject* targetObject : targets)
            {
                Unit* targetUnit = targetObject->ToUnit();
                if (targetUnit == nullptr)
                    continue;
                if (IsGUIDInList(AllowedTargetGUIDs, targetUnit->GetGUID()) == true)
                    continue;
                if (TryConsumeRainTargetLanding(RainTargetBudgetID, targetUnit->GetGUID(), hitCap, lifetimeInMS) == true)
                    AllowedTargetGUIDs.push_back(targetUnit->GetGUID());
            }
        }

        // Only units spend from the budget, so anything else the selection handed over is left alone
        for (std::list<WorldObject*>::iterator targetIterator = targets.begin(); targetIterator != targets.end();)
        {
            WorldObject* targetObject = *targetIterator;
            if (targetObject != nullptr && targetObject->ToUnit() != nullptr && IsGUIDInList(AllowedTargetGUIDs, targetObject->GetGUID()) == false)
                targetIterator = targets.erase(targetIterator);
            else
                ++targetIterator;
        }
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(EverQuest_RainTargetBudgetSpellScript::HandleAreaTargetSelect, EFFECT_ALL, TARGET_UNIT_DEST_AREA_ENEMY);
    }
};

class EverQuest_RainCloudTargetBudgetAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_RainCloudTargetBudgetAuraScript);

    uint32 RainTargetBudgetID = 0;

    bool Load() override
    {
        WorldObject* cloudOwner = GetOwner();
        if (GetCasterGUID().IsPlayer() == true && cloudOwner != nullptr)
            RainTargetBudgetID = ClaimRainTargetBudgetForCloud(GetCasterGUID(), GetId(), cloudOwner->GetGUID());
        return true;
    }

    void HandleUpdatePeriodic(AuraEffect* /*auraEffect*/)
    {
        if (EverQuest->IsEnabled == false || GetCasterGUID().IsPlayer() == false)
            return;
        // The cloud's own target search (DynObjAura::FillTargetMap) builds its checks from the caster, so there is nothing to refresh without one
        Unit* caster = GetCaster();
        Aura* cloudAura = GetAura();
        if (caster == nullptr || cloudAura == nullptr || cloudAura->IsRemoved() == true)
            return;
        cloudAura->UpdateTargetMap(caster);
    }

    void HandlePeriodic(AuraEffect const* auraEffect)
    {
        if (EverQuest->IsEnabled == false)
            return;

        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsPlayer() == false)
            return;
        Unit* target = GetTarget();
        if (target == nullptr)
            return;
        SpellInfo const* waveSpellInfo = sSpellMgr->GetSpellInfo(auraEffect->GetSpellInfo()->Effects[auraEffect->GetEffIndex()].TriggerSpell);
        if (waveSpellInfo == nullptr)
            return;

        // Skipped before any landing is spent, since the wave would fail and take the whole rain down with it
        if (IsRainWaveOutOfRange(caster, target, waveSpellInfo) == true)
        {
            PreventDefaultAction();
            return;
        }

        // The budget size comes from the wave spell, since the cloud itself carries no damage of its own
        int32 hitCap = GetRainTargetHitCap(waveSpellInfo);
        if (hitCap <= 0)
            return;

        if (TryConsumeRainTargetLanding(RainTargetBudgetID, target->GetGUID(), hitCap, GetRainTargetBudgetLifetimeInMS(GetSpellInfo())) == false)
            PreventDefaultAction();
    }

    void Register() override
    {
        OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(EverQuest_RainCloudTargetBudgetAuraScript::HandleUpdatePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
        OnEffectPeriodic += AuraEffectPeriodicFn(EverQuest_RainCloudTargetBudgetAuraScript::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

class EverQuest_RainWaveAreaCapSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_RainWaveAreaCapSpellScript);

    void HandleOnHit()
    {
        if (EverQuest->IsEnabled == false)
            return;

        int32 damage = GetHitDamage();
        if (damage <= 0)
            return;

        // The core only splits area damage when a player cast it, so the waves follow the same rule
        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsPlayer() == false)
            return;

        // Every follow-up wave is triggered by the rain's cloud aura, and that aura sits on each unit standing in the rain
        SpellInfo const* cloudSpellInfo = GetSpell()->GetTriggeredByAuraSpellInfo();
        if (cloudSpellInfo == nullptr)
            return;
        Unit* target = GetHitUnit();
        if (target == nullptr)
            return;
        Aura* cloudAura = target->GetAura(cloudSpellInfo->Id, caster->GetGUID());
        if (cloudAura == nullptr || cloudAura->GetOwner() == nullptr)
            return;
        if (DoesRainHitEveryTarget(cloudAura->GetOwner()->GetGUID()) == false)
            return;

        uint32 targetCount = uint32(cloudAura->GetApplicationMap().size());
        if (targetCount <= CORE_AREA_DAMAGE_TARGET_CAP)
            return;

        SetHitDamage(int32((int64(damage) * int64(CORE_AREA_DAMAGE_TARGET_CAP)) / int64(targetCount)));
    }

    void Register() override
    {
        OnHit += SpellHitFn(EverQuest_RainWaveAreaCapSpellScript::HandleOnHit);
    }
};

void AddEverQuestRainTargetBudgetScripts()
{
    RegisterSpellScript(EverQuest_RainTargetBudgetSpellScript);
    RegisterSpellScript(EverQuest_RainCloudTargetBudgetAuraScript);
    RegisterSpellScript(EverQuest_RainWaveAreaCapSpellScript);
}
