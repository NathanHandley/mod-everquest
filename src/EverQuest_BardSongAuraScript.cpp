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

#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "Map.h"
#include "ReputationMgr.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "Unit.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_BardSongAuraScript: public AuraScript
{
    PrepareAuraScript(EverQuest_BardSongAuraScript);

    list<Unit*> GetTargets(Unit* caster, bool isFriendly, uint32 radius)
    {
        Player* player = caster->ToPlayer();
        std::list<Unit*> targetCandidates;
        Acore::AnyUnitInObjectRangeCheck u_check(caster, radius);
        Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(caster, targetCandidates, u_check);
        TypeContainerVisitor<Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck>, GridTypeMapContainer> visitor(searcher);
        float x = caster->GetPositionX();
        float y = caster->GetPositionY();
        CellCoord cellCoord = Acore::ComputeCellCoord(x, y);
        Cell cell(cellCoord);
        caster->GetMap()->Visit(cell, visitor);
        std::list<Unit*> validTargets;
        if (isFriendly == true)
        {
            // Always include self
            if (caster->IsAlive())
                validTargets.push_back(caster);

            Group* group = player->GetGroup();
            if (!group)
                return validTargets;
            for (Unit* target : targetCandidates)
            {
                if (!target->IsPlayer() || !target->IsAlive() || target == caster || !caster->IsWithinLOSInMap(target))
                    continue;

                Player* targetPlayer = target->ToPlayer();
                if (group->SameSubGroup(player->GetGUID(), targetPlayer->GetGUID()) || group->IsMember(targetPlayer->GetGUID()))
                    validTargets.push_back(targetPlayer);
            }
        }
        else
        {
            for (Unit* target : targetCandidates)
            {
                if (!target->IsCreature() || !target->IsAlive() || target == caster || !caster->IsWithinLOSInMap(target))
                    continue;
                if (!caster->IsValidAttackTarget(target))
                    continue;

                uint32 factionTemplateId = target->GetFaction();
                FactionTemplateEntry const* ftEntry = sFactionTemplateStore.LookupEntry(factionTemplateId);
                if (!ftEntry)
                {
                    validTargets.push_back(target);
                    continue;
                }

                FactionEntry const* fEntry = sFactionStore.LookupEntry(ftEntry->faction);
                if (!fEntry)
                {
                    validTargets.push_back(target);
                    continue;
                }

                bool isRepFaction = (fEntry->reputationListID != 0);
                if (isRepFaction)
                {
                    if (player->GetReputationMgr().IsAtWar(fEntry->ID) || player->GetReputationRank(fEntry->ID) <= REP_HOSTILE)
                        validTargets.push_back(target);
                }
                else
                    validTargets.push_back(target);
            }
        }
        return validTargets;
    }

    void HandleOnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        Unit* caster = GetCaster();
        if (caster == nullptr)
            return;
        CastTriggerSpellOnTargets(caster, aurEff);
    }

    void HandlePeriodic(AuraEffect const* aurEff)
    {
        Unit* caster = GetCaster();
        if (caster == nullptr)
            return;
        CastTriggerSpellOnTargets(caster, aurEff);
    }

    void CastTriggerSpellOnTargets(Unit* caster, AuraEffect const* aurEff)
    {
        if (!caster->IsPlayer())
            return;
        if (aurEff == nullptr)
            return;
        bool isFriendly = true;
        if (aurEff->GetMiscValue() == EQ_SPELLDUMMYTYPE_BARDSONGENEMY)
            isFriendly = false;

        // Get spell details
        uint32 spellID = GetId();
        LOG_ERROR("module.EverQuest", "Debug: {} Tick", spellID);
        EverQuestSpell curSpell = EverQuest->GetSpellDataForSpellID(spellID);
        if (curSpell.SpellID == 0)
        {
            LOG_ERROR("module.EverQuest", "EverQuest_BardSongAuraScript::CastTriggerSpellOnTargets failure, as spellID {} had no definition in the database", spellID);
            return;
        }

        // Grab valid targets
        uint32 radius = curSpell.PeriodicAuraSpellRadius;
        list<Unit*> validTargets = GetTargets(caster, isFriendly, radius);        
        uint32 effectSpellID = curSpell.PeriodicAuraSpellID;

        // Cast the spell
        for (Unit* target : validTargets)
            caster->CastSpell(target, effectSpellID);
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(EverQuest_BardSongAuraScript::HandleOnApply, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectPeriodic += AuraEffectPeriodicFn(EverQuest_BardSongAuraScript::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

void AddEverQuestBardSongAuraScripts()
{
    RegisterSpellScript(EverQuest_BardSongAuraScript);
}
