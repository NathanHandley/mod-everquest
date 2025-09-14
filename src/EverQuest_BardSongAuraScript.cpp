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

    list<Unit*> GetTargets(Unit* caster, int bardTargetType, uint32 radius)
    {
        Player* player = caster->ToPlayer();
        std::list<Unit*> validTargets;

        // Do self and single target first since they are faster
        if (bardTargetType == EQ_BARDSONGAURATARGET_SELF)
        {
            validTargets.push_back(caster);
            return validTargets;
        }
        else if (bardTargetType == EQ_BARDSONGAURATARGET_ENEMYSINGLE)
        {
            Unit* currentTarget = player->GetSelectedUnit();
            if (currentTarget != nullptr && caster->IsWithinLOSInMap(currentTarget) && caster->IsValidAttackTarget(currentTarget) && currentTarget->IsAlive())
                validTargets.push_back(currentTarget);
            return validTargets;
        }
        else if (bardTargetType == EQ_BARDSONGAURATARGET_FRIENDLYSINGLE)
        {
            Unit* currentTarget = player->GetSelectedUnit();
            if (currentTarget != nullptr && caster->IsWithinLOSInMap(currentTarget) && (caster->IsValidAttackTarget(currentTarget) == false) && currentTarget->IsAlive())
                validTargets.push_back(currentTarget);
            return validTargets;
        }

        // Do large groups otherwise
        std::list<Unit*> targetCandidates;
        Acore::AnyUnitInObjectRangeCheck u_check(caster, radius);
        Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(caster, targetCandidates, u_check);
        TypeContainerVisitor<Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck>, GridTypeMapContainer> visitor(searcher);
        float x = caster->GetPositionX();
        float y = caster->GetPositionY();
        CellCoord cellCoord = Acore::ComputeCellCoord(x, y);
        Cell cell(cellCoord);
        caster->GetMap()->Visit(cell, visitor);        
        if (bardTargetType == EQ_BARDSONGAURATARGET_FRIENDLYPARTY)
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
        else if (bardTargetType == EQ_BARDSONGAURATARGET_ENEMYAREA)
        {
            for (Unit* target : targetCandidates)
            {
                if (!target->IsCreature() || !target->IsAlive() || target == caster || !caster->IsWithinLOSInMap(target))
                    continue;
                if (!caster->IsValidAttackTarget(target))
                    continue;

                uint32 factionTemplateId = target->GetFaction();
                FactionTemplateEntry const* ftEntry = sFactionTemplateStore.LookupEntry(factionTemplateId);
                if (ftEntry == nullptr)
                {
                    validTargets.push_back(target);
                    continue;
                }

                FactionEntry const* fEntry = sFactionStore.LookupEntry(ftEntry->faction);
                if (fEntry->reputationListID == -1)
                {
                    validTargets.push_back(target);
                    continue;
                }

                if (player->GetReputationMgr().IsAtWar(fEntry->ID) == true || player->GetReputationRank(fEntry->ID) <= REP_HOSTILE)
                    validTargets.push_back(target);
            }
        }
        return validTargets;
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

        int targetType = 0;
        switch (aurEff->GetMiscValue())
        {
            case EQ_SPELLDUMMYTYPE_BARDSONGENEMYAREA:       targetType = EQ_BARDSONGAURATARGET_ENEMYAREA;       break;
            case EQ_SPELLDUMMYTYPE_BARDSONGFRIENDLYPARTY:   targetType = EQ_BARDSONGAURATARGET_FRIENDLYPARTY;   break;
            case EQ_SPELLDUMMYTYPE_BARDSONGSELF:            targetType = EQ_BARDSONGAURATARGET_SELF;            break;
            case EQ_SPELLDUMMYTYPE_BARDSONGENEMYSINGLE:     targetType = EQ_BARDSONGAURATARGET_ENEMYSINGLE;     break;
            case EQ_SPELLDUMMYTYPE_BARDSONGFRIENDLYSINGLE:  targetType = EQ_BARDSONGAURATARGET_FRIENDLYSINGLE;  break;
            default:
            {
                LOG_ERROR("module.EverQuest", "EverQuest_BardSongAuraScript::CastTriggerSpellOnTargets failure, unhandled EQ_SPELLDUMMYTYPE_ of ", aurEff->GetMiscValue());
                return;
            }
        }

        // Get spell details
        uint32 spellID = GetId();
        EverQuestSpell curSpell = EverQuest->GetSpellDataForSpellID(spellID);
        if (curSpell.SpellID == 0)
        {
            LOG_ERROR("module.EverQuest", "EverQuest_BardSongAuraScript::CastTriggerSpellOnTargets failure, as spellID {} had no definition in the database", spellID);
            return;
        }

        // Grab valid targets
        uint32 radius = curSpell.PeriodicAuraSpellRadius;
        list<Unit*> validTargets = GetTargets(caster, targetType, radius);        
        uint32 effectSpellID = curSpell.PeriodicAuraSpellID;

        // Cast the spell
        for (Unit* target : validTargets)
            caster->CastSpell(target, effectSpellID, true);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(EverQuest_BardSongAuraScript::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

void AddEverQuestBardSongAuraScripts()
{
    RegisterSpellScript(EverQuest_BardSongAuraScript);
}
