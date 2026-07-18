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

#include <cctype>
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


static string GetEligibleSecondClassListForPlayer(Player* player)
{
    const EverQuestClassMap classMap = EverQuest->GetClassMapForWOWClassID(player->getClass());
    std::string list = "none";
    for (uint8 eqClassID = EQ_EQCLASS_WARRIOR; eqClassID <= EQ_EQCLASS_ENCHANTER; ++eqClassID)
    {
        uint32 classBit = 1u << (eqClassID - 1);
        if ((classMap.EQClassIDEligibleSecondMask & classBit) != 0)
        {
            list.append(", ");
            list.append(GetEQClassCommandNameFromID(eqClassID));
        }
    }
    return list;
}

class EverQuest_CommandScript : public CommandScript
{
public:
    EverQuest_CommandScript() : CommandScript("EverQuest_CommandScript") { }

    ChatCommandTable GetCommands() const
    {
        static ChatCommandTable classCommandTable =
        {
            { "change",    HandleMultiClassChangeClass,         SEC_PLAYER, Console::No },
            { "info",      HandleMultiClassInfo,                SEC_PLAYER, Console::No },
            { "uiinfo",    HandleMultiClassUIInfo,              SEC_PLAYER, Console::No },
            { "poolspend", HandleSecondaryExpPoolSpend,         SEC_PLAYER, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "eqgps",  HandleEQGPSCommand,                     SEC_PLAYER, Console::No },
            { "eqface", HandleEQFaceCommand,                    SEC_PLAYER, Console::No },
            { "eqshowbardpulse", HandleEQShowBardPulseCommand,  SEC_PLAYER, Console::No },
            { "class",  classCommandTable                                               },
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

    static bool HandleEQFaceCommand(ChatHandler* handler, const char* args)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        Player* player = handler->GetPlayer();
        uint32 maxFaceIndex = EverQuest->IllusionMaxFaceIndex;

        // Validate the passed value is a number between 0 and the highest known face index
        bool isValidFaceID = false;
        uint32 faceID = 0;
        if (*args)
        {
            char* faceToken = strtok((char*)args, " ");
            std::string faceString = faceToken != nullptr ? faceToken : "";
            if (faceString.empty() == false && faceString.size() <= 6)
            {
                bool isAllDigits = true;
                for (size_t i = 0; i < faceString.size(); ++i)
                {
                    if (isdigit(static_cast<unsigned char>(faceString[i])) == 0)
                    {
                        isAllDigits = false;
                        break;
                    }
                }
                if (isAllDigits == true)
                {
                    faceID = static_cast<uint32>(atoi(faceString.c_str()));
                    if (faceID <= maxFaceIndex)
                        isValidFaceID = true;
                }
            }
        }
        if (isValidFaceID == false)
        {
            handler->PSendSysMessage(".eqface 'number'");
            handler->PSendSysMessage("Sets the face shown when you are under an illusion. Example: '.eqface 3' will show you with face ID 3");
            handler->PSendSysMessage("Valid Face Values: 0 - {} (0 is the default face, and races with fewer faces use the default)", maxFaceIndex);
            return true;
        }

        // Store the setting
        EverQuest->SetIllusionFaceIDForPlayer(player, faceID);
        handler->PSendSysMessage("Your illusion face is now |cff4CFF00{}|r.", faceID);

        // If an illusion form is active, update the shown model right away
        EverQuest->RefreshIllusionGearDisplayForPlayer(player);
        return true;
    }

    static bool HandleEQShowBardPulseCommand(ChatHandler* handler, const char* args)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        Player* player = handler->GetPlayer();

        // Validate the passed value is either "on" or "off"
        bool isValidValue = false;
        bool showBardPulse = true;
        if (*args)
        {
            char* valueToken = strtok((char*)args, " ");
            std::string valueString = valueToken != nullptr ? valueToken : "";
            boost::algorithm::to_lower(valueString);
            if (valueString == "on")
            {
                showBardPulse = true;
                isValidValue = true;
            }
            else if (valueString == "off")
            {
                showBardPulse = false;
                isValidValue = true;
            }
        }
        if (isValidValue == false)
        {
            string currentStateString = EverQuest->GetShowBardPulseForPlayer(player) == true ? "shown" : "hidden";
            handler->PSendSysMessage(".eqshowbardpulse 'on' or 'off'");
            handler->PSendSysMessage("Shows or hides the recurring bard song pulse graphic for you. Example: '.eqshowbardpulse off' will hide the pulse graphics");
            handler->PSendSysMessage("Only the recurring pulses are affected (the graphic when a song starts always shows), and bard song pulses are currently |cff4CFF00{}|r for you.", currentStateString);
            return true;
        }

        // Store the setting, which the packet filter picks up on the next pulse
        EverQuest->SetShowBardPulseForPlayer(player, showBardPulse);
        if (showBardPulse == true)
            handler->PSendSysMessage("Bard song pulse graphics are now |cff4CFF00shown|r for you.");
        else
            handler->PSendSysMessage("Bard song pulse graphics are now |cff4CFF00hidden|r for you.");
        return true;
    }

    static bool HandleMultiClassChangeClass(ChatHandler* handler, const char* args)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        Player* player = handler->GetPlayer();
        std::string eligibleClassList = GetEligibleSecondClassListForPlayer(player);

        if (!*args)
        {
            handler->PSendSysMessage(".class change 'class'");
            handler->PSendSysMessage("Changes the player EverQuest class on next logout.  Example: '.class change warrior'");
            handler->PSendSysMessage("Valid Class Values: {}", eligibleClassList);
            return true;
        }

        uint8 classInt = EQ_EQCLASS_NONE;
        char* classNameToken = strtok((char*)args, " ");
        std::string className = classNameToken != nullptr ? classNameToken : "";
        if (className.starts_with("No") || className.starts_with("no") || className.starts_with("NO"))
            classInt = EQ_EQCLASS_NONE;
        else if (className.starts_with("Wa") || className.starts_with("wa") || className.starts_with("WA"))
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
        else if (className.starts_with("Mo") || className.starts_with("mo") || className.starts_with("MO"))
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
        else if (className.starts_with("Ma") || className.starts_with("ma") || className.starts_with("MA"))
            classInt = EQ_EQCLASS_MAGICIAN;
        else if (className.starts_with("E") || className.starts_with("e"))
            classInt = EQ_EQCLASS_ENCHANTER;
        else
        {
            handler->PSendSysMessage(".class change 'class'");
            handler->PSendSysMessage("Changes the player secondary class.  Example: '.class change warrior'");
            handler->PSendSysMessage("Valid Class Values: {}", eligibleClassList);
            std::string enteredValueLine = "Entered Value was ";
            enteredValueLine.append(className);
            handler->PSendSysMessage(enteredValueLine.c_str());
            return true;
        }

        // Restrict to defined eq class list
        if (classInt != EQ_EQCLASS_NONE)
        {
            const EverQuestClassMap classMap = EverQuest->GetClassMapForWOWClassID(player->getClass());
            uint32 classBit = 1u << (classInt - 1);
            if ((classMap.EQClassIDEligibleSecondMask & classBit) == 0)
            {
                handler->PSendSysMessage("|cffFF0000{}|r is not a valid secondary EQ class for your character.", GetEQClassStringFromID(classInt));
                handler->PSendSysMessage("Valid EQ Class Values: {}", eligibleClassList);
                return true;
            }
        }

        uint8 currentSecondClass = EverQuest->GetCurrentSecondEQClassForPlayer(player);
        EverQuest->SetNextSecondEQClassForPlayer(player, classInt);

        string text;
        if (classInt == currentSecondClass)
            text = fmt::format("Your secondary EQ class will no longer change at next login, and will remain |cff4CFF00{}|r", GetEQClassStringFromID(classInt));
        else
            text = fmt::format("Your secondary EQ class will change to |cff4CFF00{}|r on the next login", GetEQClassStringFromID(classInt));
        ChatHandler(player->GetSession()).SendSysMessage(text);

        // Refresh the EQ Class UI tab so the chosen class shows as pending
        EverQuest->SendClassInfoAddonMessageToPlayer(player);

        // Class change accepted
        return true;
    }

    static bool HandleMultiClassUIInfo(ChatHandler* handler, const char* /*args*/)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        // Pushes the current EQ class state to the client UI (the EQ Class character tab) as an addon message
        EverQuest->SendClassInfoAddonMessageToPlayer(handler->GetPlayer());
        return true;
    }

    static bool HandleSecondaryExpPoolSpend(ChatHandler* handler, const char* /*args*/)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        Player* player = handler->GetPlayer();
        uint32 spent = EverQuest->SpendSecondaryExpPoolForPlayer(player);
        if (spent > 0)
            handler->PSendSysMessage("Applied |cff4CFF00{}|r experience from your secondary bonus experience pool.", spent);
        else
            handler->PSendSysMessage("No experience could be applied from your secondary bonus experience pool.");

        // Refresh the EQ Class UI so the pool readout (and the experience bar) reflect the spend
        EverQuest->SendClassInfoAddonMessageToPlayer(player);
        return true;
    }

    static bool HandleMultiClassInfo(ChatHandler* handler, const char* /*args*/)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        // Get the player data
        Player* player = handler->GetPlayer();
        EverQuestClassMap classMap = EverQuest->GetClassMapForWOWClassID(player->getClass());
        uint8 currentSecondClass = EverQuest->GetCurrentSecondEQClassForPlayer(player);

        // Eligible classes the player has never been has no stored data, so assume 1
        map<string, EverQuestPlayerClassInfoItem> playerClassInfoItems = EverQuest->GetPlayerClassInfoByClassNameForPlayer(player);
        map<uint8, uint8> levelByEQClassID;
        for (auto& playerClassInfoItem : playerClassInfoItems)
            levelByEQClassID[playerClassInfoItem.second.ClassID] = playerClassInfoItem.second.Level;
        levelByEQClassID[currentSecondClass] = player->GetLevel();

        // Primary Class
        handler->PSendSysMessage("==== Primary and Secondary EQ Classes ====");
        string primaryLine = "Primary EQ Class: |cff4CFF00" + GetEQClassStringFromID(classMap.EQClassIDBase) + "|r";
        handler->PSendSysMessage(primaryLine);

        // Secondary Classes
        handler->PSendSysMessage("Secondary EQ Class List:");
        for (int16 eqClassID = EQ_EQCLASS_NONE; eqClassID <= EQ_EQCLASS_ENCHANTER; ++eqClassID)
        {
            // None is always top result
            if (eqClassID != EQ_EQCLASS_NONE)
            {
                uint32 classBit = 1u << (eqClassID - 1);
                if ((classMap.EQClassIDEligibleSecondMask & classBit) == 0)
                    continue;
            }

            uint8 level = 1;
            auto levelItr = levelByEQClassID.find(static_cast<uint8>(eqClassID));
            if (levelItr != levelByEQClassID.end())
                level = levelItr->second;

            string currentLine = "";
            if (eqClassID == currentSecondClass)
            {
                currentLine = " - " + std::to_string(level) + " |cff4CFF00" + GetEQClassStringFromID(static_cast<uint8>(eqClassID)) + "|r";
                currentLine += " (|cff4CFF00ACTIVE|r)";
            }
            else
                currentLine = " - " + std::to_string(level) + " " + GetEQClassStringFromID(static_cast<uint8>(eqClassID));
            handler->PSendSysMessage(currentLine.c_str());
        }

        // Footer
        handler->PSendSysMessage("Type |cff4CFF00.class |rto change or edit your secondary EQ class.");

        return true;
    }
};

void AddEverQuestCommandScripts()
{
    new EverQuest_CommandScript();
}

