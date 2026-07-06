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

#include "Configuration/Config.h"
#include "Chat.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "ThreatManager.h"
#include "Unit.h"

#include "EverQuest.h"

#include <unordered_map>
#include <vector>

using namespace std;

class EverQuest_AllSpellScript: public AllSpellScript
{
public:
    EverQuest_AllSpellScript() : AllSpellScript("EverQuest_AllSpellScript") {}

    struct PendingSpellFailure
    {
        uint32 FailableType = EQ_SPELLFAILABLETYPE_NONE;
        std::vector<ObjectGuid> FeignDeathThreatenerGUIDs;
    };

    uint32 GetEffectFailChance(Unit* caster, SpellInfo const* spellInfo)
    {
        if (EverQuest->IsEnabled == false)
            return 0;
        if (caster == nullptr || spellInfo == nullptr)
            return 0;
        if (caster->IsPlayer() == false)
            return 0;
        if (spellInfo->Id < EverQuest->ConfigSystemSpellDBCIDMin || spellInfo->Id > EverQuest->ConfigSystemSpellDBCIDMax)
            return 0;
        if (EverQuest->IsSpellAnEQSpell(spellInfo->Id) == false)
            return 0;
        return EverQuest->GetSpellDataForSpellID(spellInfo->Id).EffectFailChancePercent;
    }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& res) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (res != SPELL_CAST_OK)
            return;
        if (spell == nullptr || spell->GetCaster() == nullptr)
            return;

        // Creature-cast charms follow extra limit rules
        Unit* target = spell->m_targets.GetUnitTarget();
        if (EverQuest->IsCreatureCharmBlockedByCharmLimits(spell->GetSpellInfo()->Id, target, spell->GetCaster()) == true)
        {
            res = SPELL_FAILED_DONT_REPORT;
            return;
        }

        // Enforce buff restriction up front to avoid mana/cooldown triggers
        if (target == nullptr)
            return;
        if (EverQuest->IsSpellBlockedByMinTargetLevel(spell->GetSpellInfo()->Id, target, spell->GetCaster()) == true)
            res = SPELL_FAILED_LOWLEVEL;
        else if (EverQuest->IsSpellBlockedByMaxCreatureTargetLevel(spell->GetSpellInfo()->Id, target, spell->GetCaster()) == true)
            res = SPELL_FAILED_HIGHLEVEL;
    }

    void OnSpellPrepare(Spell* /*spell*/, Unit* caster, SpellInfo const* spellInfo) override
    {
        uint32 failChancePercent = GetEffectFailChance(caster, spellInfo);
        if (failChancePercent == 0)
            return;

        // Clear any stale pending failure from a previous failure
        {
            std::lock_guard<std::mutex> lock(PendingSpellFailuresMutex);
            PendingSpellFailuresByCasterGUID.erase(caster->GetGUID());
        }

        if (roll_chance_i((int)failChancePercent) == false)
            return; // Behave normally

        PendingSpellFailure pendingFailure;
        pendingFailure.FailableType = EverQuest->GetSpellDataForSpellID(spellInfo->Id).EffectFailableType;
        switch (pendingFailure.FailableType)
        {
            case EQ_SPELLFAILABLETYPE_FEIGNDEATH:
                // The core feign aura will wipe our threat on apply, so remember who is attacking now
                for (auto const& threatenerPair : caster->GetThreatMgr().GetThreatenedByMeList())
                    pendingFailure.FeignDeathThreatenerGUIDs.push_back(threatenerPair.first);
                break;
            default:
                break;
        }
        std::lock_guard<std::mutex> lock(PendingSpellFailuresMutex);
        PendingSpellFailuresByCasterGUID[caster->GetGUID()] = std::move(pendingFailure);
    }

    void OnSpellCastCancel(Spell* /*spell*/, Unit* caster, SpellInfo const* /*spellInfo*/, bool /*bySelf*/) override
    {
        if (caster == nullptr)
            return;
        std::lock_guard<std::mutex> lock(PendingSpellFailuresMutex);
        PendingSpellFailuresByCasterGUID.erase(caster->GetGUID());
    }

    void ApplySpellFailure(Player* player, uint32 spellID, PendingSpellFailure const& pendingFailure)
    {
        switch (pendingFailure.FailableType)
        {
            case EQ_SPELLFAILABLETYPE_FEIGNDEATH:
                ApplyFeignDeathFailure(player, spellID, pendingFailure.FeignDeathThreatenerGUIDs);
                break;
            default:
                break;
        }
    }

    void ApplyFeignDeathFailure(Player* player, uint32 spellID, std::vector<ObjectGuid> const& threatenerGUIDs)
    {
        if (player == nullptr)
            return;

        player->RemoveAurasDueToSpell(spellID);

        for (ObjectGuid const& threatenerGUID : threatenerGUIDs)
        {
            Creature* threatener = ObjectAccessor::GetCreature(*player, threatenerGUID);
            if (threatener == nullptr || threatener->IsAlive() == false)
                continue;
            threatener->EngageWithTarget(player);
        }

        // Show the failure as an emote to the player and everyone nearby
        WorldPacket failMessageData;
        ChatHandler::BuildChatPacket(failMessageData, CHAT_MSG_EMOTE, LANG_UNIVERSAL, player, player, "tries to feign death, but fails!");
        player->SendMessageToSet(&failMessageData, true);
    }

    void OnCalcMaxDuration(Aura const* aura, int32& maxDuration) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        if (aura == nullptr)
            return;

        // Skip any non EQ spells
        uint32 spellID = aura->GetId();
        if (spellID < EverQuest->ConfigSystemSpellDBCIDMin || spellID > EverQuest->ConfigSystemSpellDBCIDMax)
            return;
        if (EverQuest->IsSpellAnEQSpell(spellID) == false)
            return;

        // Only work with spells that have a caster
        Unit* caster = aura->GetCaster();
        if (!caster)
            return;

        // Skip any EQ spells with no duration
        const EverQuestSpell& curSpell = EverQuest->GetSpellDataForSpellID(spellID);
        if (curSpell.AuraDurationMaxInMS == 0)
            return;

        // Calculate the duration
        if (curSpell.AuraDurationBaseInMS == curSpell.AuraDurationMaxInMS)
        {
            maxDuration = curSpell.AuraDurationMaxInMS;
            return;
        }
        uint8 casterLevel = caster->GetLevel();
        if (casterLevel < curSpell.AuraDurationCalcMinLevel)
        {
            maxDuration = curSpell.AuraDurationBaseInMS;
            return;
        }
        else if (casterLevel > curSpell.AuraDurationCalcMaxLevel)
        {
            maxDuration = curSpell.AuraDurationMaxInMS;
            return;
        }
        else if (curSpell.AuraDurationAddPerLevelInMS == 0)
        {
            maxDuration = curSpell.AuraDurationMaxInMS;
            return;
        }
        else
        {
            uint32 levelDiff = (uint32)casterLevel - curSpell.AuraDurationCalcMinLevel;
            maxDuration = (int32)(curSpell.AuraDurationBaseInMS + (curSpell.AuraDurationAddPerLevelInMS * levelDiff));
            return;
        }
    }

    void OnSpellCast(Spell* spell, Unit* caster , SpellInfo const* spellInfo, bool /*skipCheck*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;

        // Verify it's an EQ spell that is mapped
        if (spell == nullptr)
            return;
        if (spellInfo == nullptr)
            return;
        if (spellInfo->Id < EverQuest->ConfigSystemSpellDBCIDMin || spellInfo->Id > EverQuest->ConfigSystemSpellDBCIDMax)
            return;
        if (EverQuest->IsSpellAnEQSpell(spellInfo->Id) == false)
            return;
        EverQuestSpell curSpell = EverQuest->GetSpellDataForSpellID(spellInfo->Id);

        // Handle a rolled effect failure (decided at cast start in OnSpellPrepare). Pull the entry out under the
        // lock, then apply it after release since applying casts/engages targets
        if (caster->IsPlayer())
        {
            bool hasPendingFailure = false;
            PendingSpellFailure pendingFailure;
            {
                std::lock_guard<std::mutex> lock(PendingSpellFailuresMutex);
                std::unordered_map<ObjectGuid, PendingSpellFailure>::iterator pendingFailureItr = PendingSpellFailuresByCasterGUID.find(caster->GetGUID());
                if (pendingFailureItr != PendingSpellFailuresByCasterGUID.end())
                {
                    hasPendingFailure = true;
                    pendingFailure = std::move(pendingFailureItr->second);
                    PendingSpellFailuresByCasterGUID.erase(pendingFailureItr);
                }
            }
            if (hasPendingFailure == true)
                ApplySpellFailure(caster->ToPlayer(), spellInfo->Id, pendingFailure);
        }

        // Handle any recourse
        if (curSpell.RecourseSpellID != 0)
        {
            const SpellInfo* recourseSpellInfo = sSpellMgr->GetSpellInfo(curSpell.RecourseSpellID);
            if (recourseSpellInfo == nullptr)
                return;

            // Calculate the target
            Unit* target = nullptr;
            if (recourseSpellInfo->Targets & TARGET_FLAG_UNIT)
            {
                target = spell->m_targets.GetUnitTarget();
                if (!target)
                    target = caster;
            }
            else if (recourseSpellInfo->Targets & TARGET_FLAG_DEST_LOCATION)
            {
                if (WorldLocation const* dest = spell->m_targets.GetDstPos())
                {
                    caster->CastSpell(dest->GetPositionX(), dest->GetPositionY(), dest->GetPositionZ(), curSpell.RecourseSpellID, true);
                    return;
                }
                else
                {
                    caster->CastSpell(caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ(), curSpell.RecourseSpellID, true);
                    return;
                }
            }
            else
                target = caster;

            // Cast on the target
            if (target)
                caster->CastSpell(target, curSpell.RecourseSpellID, true);
        }       
    }

private:
    // Casts process on parallel map update threads, so guard this shared map
    std::mutex PendingSpellFailuresMutex;
    std::unordered_map<ObjectGuid, PendingSpellFailure> PendingSpellFailuresByCasterGUID;
};

void AddEverQuestAllSpellScripts()
{
    new EverQuest_AllSpellScript();
}
