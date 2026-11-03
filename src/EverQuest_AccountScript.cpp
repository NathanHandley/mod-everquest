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

#include "EverQuest.h"

class EverQuest_AccountScript : public AccountScript
{
public:
    EverQuest_AccountScript() : AccountScript("EverQuest_AccountScript", { ACCOUNTHOOK_CAN_ACCOUNT_LOGIN_WITH_CLIENT_DATA_VERSION }) {}

    bool CanAccountLoginWithClientDataVersion(uint32 /*accountId*/, AccountTypes security, uint32 clientDataVersion, uint32& expectedClientDataVersion) override
    {
        return EverQuest->IsClientDataVersionAllowed(security, clientDataVersion, expectedClientDataVersion);
    }
};

void AddEverQuestAccountScripts()
{
    new EverQuest_AccountScript();
}
