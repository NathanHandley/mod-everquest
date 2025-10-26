/*
** Made by Nathan Handley https://github.com/NathanHandley
** AzerothCore 2019 http://www.azerothcore.org/
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU Affero General Public License as published by the
* Free Software Foundation; either version 3 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "EverQuest.h"
#include "Chat.h"
#include "ScriptMgr.h"
#include "CommandScript.h"
#include "boost/algorithm/string.hpp"

#include <iomanip>

using namespace Acore::ChatCommands;
using namespace std;

static std::string RoundVal(float value, int precision)
{
    // Scale, round, and scale back
    float scale = 100000.0f;
    float roundedValue;
    if (value < std::numeric_limits<float>::epsilon() && value > -std::numeric_limits<float>::epsilon())
        roundedValue = 0;
    else
        roundedValue = std::round((value + std::numeric_limits<float>::epsilon()) * scale) / scale;

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << roundedValue;
    return stream.str();
}

class EverQuest_CommandScript : public CommandScript
{
public:
    EverQuest_CommandScript() : CommandScript("EverQuest_CommandScript") { }

    ChatCommandTable GetCommands() const
    {
        static ChatCommandTable designCommandTable =
        {
            { "eqgps",                   HandleEQGPSCommand,                   SEC_PLAYER,          Console::No  },
        };

        return designCommandTable;
    }

    static bool HandleEQGPSCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        // Determine target, and validate
        if (!target)
            target = PlayerIdentifier::FromTargetOrSelf(handler);
        WorldObject* object = handler->getSelectedUnit();
        if (!object && !target)
            return false;
        if (!object && target && target->IsConnected())
            object = target->GetConnectedPlayer();
        if (!object)
            return false;

        string headerText = fmt::format("=== EQGPS (O = Orientation | H = Heading) ===");
        handler->PSendSysMessage(headerText);

        string targetText = fmt::format("Information is for target named:  {}", object->GetName());
        handler->PSendSysMessage(targetText);

        // True (World of Warcraft) values
        string trueText = fmt::format("True X: {}  Y: {} Z: {} O: {}", RoundVal(object->GetPositionX(), 6), RoundVal(object->GetPositionY(), 6), RoundVal(object->GetPositionZ(), 6), RoundVal(object->GetOrientation(), 6));
        handler->PSendSysMessage(trueText);

        float worldScale = EverQuest->ConfigWorldScale;

        // Prescaled (World of Warcraft, before scaling)
        string preScaledText = fmt::format("Prescale X: {} Y: {} Z: {} O: {}", RoundVal(object->GetPositionX() / worldScale, 6), RoundVal(object->GetPositionY() / worldScale, 6), RoundVal(object->GetPositionZ() / worldScale, 6), RoundVal(object->GetOrientation(), 6));
        handler->PSendSysMessage(preScaledText);

        // EverQuest
        float eqHeadingFloat = ((((object->GetOrientation() - 3.14159265359) * 180.0) / 3.14159265359f) / 360.0) * 512.0;
        string eqText = fmt::format("EverQuest X: {} Y: {} Z: {} H: {}", RoundVal(object->GetPositionY() / worldScale, 6), RoundVal(object->GetPositionX() / worldScale, 6), RoundVal(object->GetPositionZ() / worldScale, 6), RoundVal(eqHeadingFloat, 6));
        handler->PSendSysMessage(eqText);
        return true;
    }
};

void AddEverQuestCommandScripts()
{
    new EverQuest_CommandScript();
}

