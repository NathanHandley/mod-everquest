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
        /*if (MultiClass->ConfigEnabled == false)
            return true;

        if (!*args)
        {
            handler->PSendSysMessage(".class change 'class'");
            handler->PSendSysMessage("Changes the player class on next logout.  Example: '.class change warrior'");
            handler->PSendSysMessage("Valid Class Values: warrior, paladin, hunter, rogue, priest, deathknight, shaman, mage, warlock, druid");
            return true;
        }

        uint8 classInt = CLASS_NONE;
        std::string className = strtok((char*)args, " ");
        if (className.starts_with("Warr") || className.starts_with("warr") || className.starts_with("WARR"))
            classInt = CLASS_WARRIOR;
        else if (className.starts_with("Pa") || className.starts_with("pa") || className.starts_with("PA"))
            classInt = CLASS_PALADIN;
        else if (className.starts_with("H") || className.starts_with("h"))
            classInt = CLASS_HUNTER;
        else if (className.starts_with("R") || className.starts_with("r"))
            classInt = CLASS_ROGUE;
        else if (className.starts_with("Pr") || className.starts_with("pr") || className.starts_with("PR"))
            classInt = CLASS_PRIEST;
        else if (className.starts_with("De") || className.starts_with("de") || className.starts_with("DE"))
            classInt = CLASS_DEATH_KNIGHT;
        else if (className.starts_with("S") || className.starts_with("s"))
            classInt = CLASS_SHAMAN;
        else if (className.starts_with("M") || className.starts_with("m"))
            classInt = CLASS_MAGE;
        else if (className.starts_with("Warl") || className.starts_with("warl") || className.starts_with("WARL"))
            classInt = CLASS_WARLOCK;
        else if (className.starts_with("Dr") || className.starts_with("dr") || className.starts_with("DR"))
            classInt = CLASS_DRUID;
        else
        {
            handler->PSendSysMessage(".class change 'class'");
            handler->PSendSysMessage("Changes the player class.  Example: '.class change warrior'");
            handler->PSendSysMessage("Valid Class Values: warrior, paladin, hunter, rogue, priest, deathknight, shaman, mage warlock, druid");
            std::string enteredValueLine = "Entered Value was ";
            enteredValueLine.append(className);
            handler->PSendSysMessage(enteredValueLine.c_str());
            return true;
        }

        Player* player = handler->GetPlayer();
        MultiClass->MarkClassChangeOnNextLogout(handler, player, classInt);
        player->SaveToDB(false, false);*/

        // Class change accepted
        return true;
    }

    static bool HandleMultiClassInfo(ChatHandler* handler, const char* /*args*/)
    {
        //if (MultiClass->ConfigEnabled == false)
        //    return true;

        //handler->PSendSysMessage("Class List:");

        //// Get the player data
        //Player* player = handler->GetPlayer();
        //map<string, PlayerClassInfoItem> playerClassInfoItems = MultiClass->GetPlayerClassInfoByClassNameForPlayer(player);

        //// Write the information out
        //for (auto& playerClassInfoItem : playerClassInfoItems)
        //{
        //    string currentLine = " - " + playerClassInfoItem.second.ClassName + "(" + std::to_string(playerClassInfoItem.second.Level) + "), Shared: Quests(";
        //    if (playerClassInfoItem.second.UseSharedQuests)
        //        currentLine += "|cff4CFF00ON|r";
        //    else
        //        currentLine += "|cffff0000OFF|r";
        //    currentLine += "), Reputation(";
        //    if (playerClassInfoItem.second.UseSharedReputation)
        //        currentLine += "|cff4CFF00ON|r";
        //    else
        //        currentLine += "|cffff0000OFF|r";
        //    currentLine += ")";
        //    handler->PSendSysMessage(currentLine.c_str());
        //}

        return true;
    }
};

void AddEverQuestCommandScripts()
{
    new EverQuest_CommandScript();
}

