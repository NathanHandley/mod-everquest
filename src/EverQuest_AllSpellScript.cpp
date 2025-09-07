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
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "Unit.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_AllSpellScript: public AllSpellScript
{
public:
    EverQuest_AllSpellScript() : AllSpellScript("EverQuest_AllSpellScript") {}

    void OnCalcMaxDuration(Aura const* aura, int32& maxDuration) override
    {
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
};

void AddEverQuestAllSpellScripts()
{
    new EverQuest_AllSpellScript();
}
