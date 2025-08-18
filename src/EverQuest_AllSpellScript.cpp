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
        // Skip any non EQ spells
        uint32 spellID = aura->GetId();
        if (EverQuest->IsSpellAnEQSpell(spellID) == false)
            return;

        // Only work with spells that have a caster
        Unit* caster = aura->GetCaster();
        if (!caster)
            return;

        // Skip any EQ spells with no duration
        EverQuestSpell curSpell = EverQuest->GetSpellDataForSpellID(spellID);
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
};

void AddEverQuestAllSpellScripts()
{
    new EverQuest_AllSpellScript();
}
