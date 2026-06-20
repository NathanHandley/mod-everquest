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
        static ChatCommandTable classCommandTable =
        {
            { "change", HandleMultiClassChangeClass,    SEC_PLAYER, Console::No },
            { "info",   HandleMultiClassInfo,           SEC_PLAYER, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "eqgps",  HandleEQGPSCommand,             SEC_PLAYER, Console::No },
            { "class",  classCommandTable                                       },
        };

        return commandTable;
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

    static bool HandleMultiClassChangeClass(ChatHandler* handler, const char* args)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        if (!*args)
        {
            handler->PSendSysMessage(".class change 'class'");
            handler->PSendSysMessage("Changes the player EverQuest class on next logout.  Example: '.class change warrior'");
            handler->PSendSysMessage("Valid Class Values: warrior, cleric, paladin, ranger, shadowknight, druid, monk, bard, rogue, shaman, necromancer, wizard, magician, enchanter");
            return true;
        }

        uint8 classInt = CLASS_NONE;
        std::string className = strtok((char*)args, " ");
        if (className.starts_with("Wa") || className.starts_with("wa") || className.starts_with("WA"))
            classInt = EQ_EQCLASS_WARRIOR;
        else if (className.starts_with("C") || className.starts_with("c"))
            classInt = EQ_EQCLASS_CLERIC;
        else if (className.starts_with("P") || className.starts_with("p"))
            classInt = EQ_EQCLASS_PALADIN;
        else if (className.starts_with("Ra") || className.starts_with("ra") || className.starts_with("RA"))
            classInt = EQ_EQCLASS_RANGER;
        else if (className.starts_with("Shad") || className.starts_with("shad") || className.starts_with("SHAD"))
            classInt = EQ_EQCLASS_SHADOWKNIGHT;
        else if (className.starts_with("D") || className.starts_with("d"))
            classInt = EQ_EQCLASS_DRUID;
        else if (className.starts_with("M") || className.starts_with("m"))
            classInt = EQ_EQCLASS_MONK;
        else if (className.starts_with("B") || className.starts_with("b"))
            classInt = EQ_EQCLASS_BARD;
        else if (className.starts_with("Ro") || className.starts_with("ro") || className.starts_with("RO"))
            classInt = EQ_EQCLASS_ROGUE;
        else if (className.starts_with("Sham") || className.starts_with("sham") || className.starts_with("SHAM"))
            classInt = EQ_EQCLASS_SHAMAN;
        else if (className.starts_with("N") || className.starts_with("n"))
            classInt = EQ_EQCLASS_NECROMANCER;
        else if (className.starts_with("Wi") || className.starts_with("wi") || className.starts_with("WI"))
            classInt = EQ_EQCLASS_WIZARD;
        else if (className.starts_with("M") || className.starts_with("m"))
            classInt = EQ_EQCLASS_MAGICIAN;
        else if (className.starts_with("E") || className.starts_with("e"))
            classInt = EQ_EQCLASS_ENCHANTER;
        else
        {
            handler->PSendSysMessage(".class change 'class'");
            handler->PSendSysMessage("Changes the player class.  Example: '.class change warrior'");
            handler->PSendSysMessage("Valid Class Values: warrior, cleric, paladin, ranger, shadowknight, druid, monk, bard, rogue, shaman, necromancer, wizard, magician, enchanter");
            std::string enteredValueLine = "Entered Value was ";
            enteredValueLine.append(className);
            handler->PSendSysMessage(enteredValueLine.c_str());
            return true;
        }

        Player* player = handler->GetPlayer();
        EverQuest->SetNextEQClassForPlayer(player, classInt);

        string text = fmt::format("Your EQ class will change to |cff4CFF00{}|r on the next login", GetEQClassStringFromID(classInt));
        ChatHandler(player->GetSession()).SendSysMessage(text);

        // Class change accepted
        return true;
    }

    static bool HandleMultiClassInfo(ChatHandler* handler, const char* /*args*/)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        handler->PSendSysMessage("Class List:");

        // Get the player data
        Player* player = handler->GetPlayer();
        map<string, EverQuestPlayerClassInfoItem> playerClassInfoItems = EverQuest->GetPlayerClassInfoByClassNameForPlayer(player);

        // Write the information out
        for (auto& playerClassInfoItem : playerClassInfoItems)
        {
            string currentLine = " - " + playerClassInfoItem.second.ClassName + "(" + std::to_string(playerClassInfoItem.second.Level) + ")";
            handler->PSendSysMessage(currentLine.c_str());
        }

        return true;
    }
};

void AddEverQuestCommandScripts()
{
    new EverQuest_CommandScript();
}

