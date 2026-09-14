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

#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellScript.h"
#include "Unit.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_MesmerizeAuraScript : public AuraScript
{
    PrepareAuraScript(EverQuest_MesmerizeAuraScript);

    void HandleDispel(DispelInfo* dispelInfo)
    {
        if (EverQuest->IsEnabled == false || dispelInfo == nullptr)
            return;
        EverQuest->RecordMesmerizeDispeller(GetUnitOwner(), GetAura(), dispelInfo->GetDispeller(), dispelInfo->GetDispellerSpellId());
    }

    void Register() override
    {
        OnDispel += AuraDispelFn(EverQuest_MesmerizeAuraScript::HandleDispel);
    }
};

void AddEverQuestMesmerizeAuraScripts()
{
    RegisterSpellScript(EverQuest_MesmerizeAuraScript);
}
