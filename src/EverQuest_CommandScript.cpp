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

#include "EverQuest.h"
#include "Chat.h"
#include "ScriptMgr.h"
#include "CommandScript.h"
#include "boost/algorithm/string.hpp"

#include <cctype>
#include <iomanip>
#include <string>
#include <vector>

using namespace Acore::ChatCommands;
using namespace std;

// Splits the command argument string on spaces.  strtok is avoided because it keeps its scan position in a process-wide static on glibc and
// writes terminators into the buffer it is handed, which here is the argument string owned by the command dispatcher
static std::vector<std::string> SplitCommandArgs(const char* args, size_t maxTokens)
{
    std::vector<std::string> tokens;
    if (args == nullptr || maxTokens == 0)
        return tokens;
    std::string currentToken;
    for (const char* cursor = args; *cursor != '\0'; ++cursor)
    {
        if (*cursor != ' ')
        {
            currentToken.push_back(*cursor);
            continue;
        }
        if (currentToken.empty() == true)
            continue;
        tokens.push_back(currentToken);
        currentToken.clear();
        if (tokens.size() >= maxTokens)
            return tokens;
    }
    if (currentToken.empty() == false)
        tokens.push_back(currentToken);
    return tokens;
}

static std::string GetFirstCommandArg(const char* args)
{
    std::vector<std::string> tokens = SplitCommandArgs(args, 1);
    return tokens.empty() == true ? std::string() : tokens[0];
}

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
            { "change",      HandleMultiClassChangeClass,       SEC_PLAYER, Console::No },
            { "info",        HandleMultiClassInfo,              SEC_PLAYER, Console::No },
            { "uiinfo",      HandleMultiClassUIInfo,            SEC_PLAYER, Console::No },
            { "poolspend",   HandleSecondaryExpPoolSpend,       SEC_PLAYER, Console::No },
            { "equipinfo",   HandleSecondaryEquipInfo,          SEC_PLAYER, Console::No },
            { "equipset",    HandleSecondaryEquipSet,           SEC_PLAYER, Console::No },
            { "equipremove", HandleSecondaryEquipRemove,        SEC_PLAYER, Console::No },
            { "equipmove",   HandleSecondaryEquipMove,          SEC_PLAYER, Console::No },
            { "equipswap",   HandleSecondaryEquipSwap,          SEC_PLAYER, Console::No },
        };

        static ChatCommandTable adventurerCommandTable =
        {
            { "info",    HandleAdventurerInfo,    SEC_GAMEMASTER, Console::No },
            { "restore", HandleAdventurerRestore, SEC_GAMEMASTER, Console::No },
        };

        static ChatCommandTable trackCommandTable =
        {
            { "list",  HandleTrackList,  SEC_PLAYER, Console::No },
            { "start", HandleTrackStart, SEC_PLAYER, Console::No },
            { "stop",  HandleTrackStop,  SEC_PLAYER, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "eqgps",  HandleEQGPSCommand,                     SEC_PLAYER, Console::No },
            { "eqver",  HandleEQVerCommand,                     SEC_PLAYER, Console::No },
            { "eqface", HandleEQFaceCommand,                    SEC_PLAYER, Console::No },
            { "eqshowbardpulse", HandleEQShowBardPulseCommand,  SEC_PLAYER, Console::No },
            { "eqhidewowgear", HandleEQHideWoWGearCommand,      SEC_PLAYER, Console::No },
            { "eqdungeonmode", HandleEQDungeonModeCommand,      SEC_PLAYER, Console::No },
            { "eqhailwindow", HandleEQHailWindowCommand,        SEC_PLAYER, Console::No },
            { "eqauctionfilter", HandleEQAuctionFilterCommand,  SEC_PLAYER, Console::No },
            { "class",  classCommandTable                                               },
            { "track",  trackCommandTable                                               },
            { "eqadventurer", adventurerCommandTable                                    },
        };

        return commandTable;
    }

    static Player* GetConnectedAdventurerTarget(ChatHandler* handler, Optional<PlayerIdentifier>& target)
    {
        if (!target)
            target = PlayerIdentifier::FromTargetOrSelf(handler);
        if (!target)
        {
            handler->SendSysMessage("No character was named, and nothing is selected.");
            return nullptr;
        }
        if (target->IsConnected() == false)
        {
            handler->PSendSysMessage("{} is not online. This command only works on a character that is logged in.", target->GetName());
            return nullptr;
        }
        return target->GetConnectedPlayer();
    }

    static bool HandleAdventurerInfo(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        if (EverQuest->IsEnabled == false)
        {
            handler->SendSysMessage("The EverQuest module is disabled.");
            return true;
        }

        Player* targetPlayer = GetConnectedAdventurerTarget(handler, target);
        if (targetPlayer == nullptr)
            return true;

        handler->PSendSysMessage("=== EverQuest Adventurer: {} ===", targetPlayer->GetName());
        handler->PSendSysMessage("Buff currently held: {}", EverQuest->IsPlayerDisqualifiedFromAdventurer(targetPlayer) == false ? "yes" : "no");

        EverQuestAdventurerLossSnapshot snapshot;
        if (EverQuest->TryGetAdventurerLevelSnapshotForPlayerGUID(targetPlayer->GetGUID().GetCounter(), snapshot) == false)
        {
            handler->SendSysMessage("No recorded loss for this character, so a restore would leave its levels alone.");
            return true;
        }

        handler->PSendSysMessage("Lost the buff at unix time {}, while playing {}.", snapshot.LossTimestamp, GetEQClassStringFromID(snapshot.LossSecondaryClass));
        handler->SendSysMessage("Levels at the time of the loss, against the levels now:");

        map<uint8, uint8> currentLevelsByEQClass = EverQuest->GetClassLevelsByClassForPlayer(targetPlayer);
        for (uint8 eqClassID = EQ_EQCLASS_NONE; eqClassID <= EQ_EQCLASS_HIGHEST_ID; eqClassID++)
        {
            uint8 snapshotLevel = snapshot.LevelsByEQClass[eqClassID];
            auto currentLevelIter = currentLevelsByEQClass.find(eqClassID);
            uint8 currentLevel = (currentLevelIter == currentLevelsByEQClass.end()) ? 0 : currentLevelIter->second;
            if (snapshotLevel == 0 && currentLevel == 0)
                continue;
            handler->PSendSysMessage("  {}: was {}, now {}{}", GetEQClassStringFromID(eqClassID), uint32(snapshotLevel), uint32(currentLevel),
                (snapshotLevel != 0 && snapshotLevel != currentLevel) ? "  <- a restore would change this" : "");
        }
        return true;
    }

    static bool HandleAdventurerRestore(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        Player* gmPlayer = handler->GetSession() != nullptr ? handler->GetSession()->GetPlayer() : nullptr;
        if (gmPlayer == nullptr)
        {
            handler->SendSysMessage("This command has to be run by a character in the world, since the stripped items are mailed to them.");
            return true;
        }

        Player* targetPlayer = GetConnectedAdventurerTarget(handler, target);
        if (targetPlayer == nullptr)
            return true;

        EverQuestAdventurerRestoreReport report;
        if (EverQuest->RestoreAdventurerForPlayer(targetPlayer, gmPlayer, report) == false)
        {
            handler->PSendSysMessage("EverQuest Adventurer restore failed for {}: {}", targetPlayer->GetName(), report.FailureReason);
            return true;
        }

        handler->PSendSysMessage("Restored the EverQuest Adventurer buff on {}.", targetPlayer->GetName());
        handler->PSendSysMessage("  Non-EverQuest items mailed to you: {} across {} mail(s).", report.ItemsMailed, report.MailsSent);
        if (report.HadLevelSnapshot == true)
            handler->PSendSysMessage("  Class levels reset to the recorded loss: {}.", report.ClassLevelsChanged);
        for (std::string const& note : report.Notes)
            handler->PSendSysMessage("  {}", note);
        return true;
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
            std::string faceString = GetFirstCommandArg(args);
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
            std::string valueString = GetFirstCommandArg(args);
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

    static bool HandleEQHideWoWGearCommand(ChatHandler* handler, const char* args)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        Player* player = handler->GetPlayer();

        // Validate the passed value is either "on" or "off"
        bool isValidValue = false;
        bool hideWoWGear = false;
        if (*args)
        {
            std::string valueString = GetFirstCommandArg(args);
            boost::algorithm::to_lower(valueString);
            if (valueString == "on")
            {
                hideWoWGear = true;
                isValidValue = true;
            }
            else if (valueString == "off")
            {
                hideWoWGear = false;
                isValidValue = true;
            }
        }
        if (isValidValue == false)
        {
            string currentStateString = EverQuest->GetHideWoWGearForPlayer(player) == true ? "on" : "off";
            handler->PSendSysMessage(".eqhidewowgear 'on' or 'off'");
            handler->PSendSysMessage("When on, WoW equipment on other players is replaced for you with plain EverQuest looks of the same kind, anything with no EverQuest equivalent (cloaks, helms, shoulders, tabards) is hidden, and weapon enchant glows are suppressed. Example: '.eqhidewowgear on'");
            handler->PSendSysMessage("Only what you see is affected (nobody's actual gear changes), and WoW gear hiding is currently |cff4CFF00{}|r for you.", currentStateString);
            return true;
        }

        // Store the setting, then re-send nearby players' visible gear so the change shows without needing to re-enter view range
        EverQuest->SetHideWoWGearForPlayer(player, hideWoWGear);
        EverQuest->ResendVisibleGearOfNearbyPlayersToPlayer(player);
        if (hideWoWGear == true)
            handler->PSendSysMessage("WoW gear on other players is now |cff4CFF00hidden|r for you.");
        else
            handler->PSendSysMessage("WoW gear on other players is now |cff4CFF00shown|r for you.");
        return true;
    }

    static bool HandleEQHailWindowCommand(ChatHandler* handler, const char* args)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        Player* player = handler->GetPlayer();
        if (player == nullptr)
            return true;

        // Validate the passed value is either "on" or "off"
        bool isValidValue = false;
        bool hailWindowOnRightClick = false;
        if (*args)
        {
            std::string valueString = GetFirstCommandArg(args);
            boost::algorithm::to_lower(valueString);
            if (valueString == "on")
            {
                hailWindowOnRightClick = true;
                isValidValue = true;
            }
            else if (valueString == "off")
            {
                hailWindowOnRightClick = false;
                isValidValue = true;
            }
        }
        if (isValidValue == false)
        {
            string currentStateString = EverQuest->GetHailWindowOnRightClickForPlayer(player) == true ? "on" : "off";
            handler->PSendSysMessage(".eqhailwindow 'on' or 'off'");
            handler->PSendSysMessage("Many EverQuest creatures answer a hail with a line of text and nothing else. When on, right clicking one of those opens its reply in a window, the way talking to any other NPC does. When off, they behave like ordinary creatures and right clicking attacks them instead. Creatures that also sell, train, bank or hand out quests are unaffected either way. Example: '.eqhailwindow on'");
            handler->PSendSysMessage("Opening hail replies on right click is currently |cff4CFF00{}|r for you.", currentStateString);
            return true;
        }

        // Store the setting, then re-send nearby creatures' flags so the change takes hold without needing to re-enter view range
        EverQuest->SetHailWindowOnRightClickForPlayer(player, hailWindowOnRightClick);
        EverQuest->ResendNpcFlagsOfNearbyCreaturesToPlayer(player);
        if (hailWindowOnRightClick == true)
            handler->PSendSysMessage("Right clicking a creature that only answers hails now |cff4CFF00opens its reply|r.");
        else
            handler->PSendSysMessage("Right clicking a creature that only answers hails now |cff4CFF00attacks it|r.");
        return true;
    }

    static bool HandleEQDungeonModeCommand(ChatHandler* handler, const char* args)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        Player* player = handler->GetPlayer();

        // Validate the passed value is either "shared" or "instanced".  "sync" is what the client addon uses to pull the current value into its menu without printing anything
        bool isValidValue = false;
        bool dungeonModeInstanced = false;
        if (*args)
        {
            std::string valueString = GetFirstCommandArg(args);
            boost::algorithm::to_lower(valueString);
            if (valueString == "sync")
            {
                EverQuest->SendDungeonModeStateToPlayer(player, false);
                return true;
            }
            if (valueString == "shared")
            {
                dungeonModeInstanced = false;
                isValidValue = true;
            }
            else if (valueString == "instanced")
            {
                dungeonModeInstanced = true;
                isValidValue = true;
            }
        }
        if (isValidValue == false)
        {
            handler->PSendSysMessage(".eqdungeonmode 'shared' or 'instanced'");
            handler->PSendSysMessage("When instanced, walking into a dungeon that supports it puts you into a private copy of that dungeon for you and your group, which you can release with 'Reset all instances' on your portrait menu. When shared, dungeons are the shared world versions everyone else is in.  Applies to EverQuest dungeons.  Example: '.eqdungeonmode instanced'");
            EverQuest->SendDungeonModeStateToPlayer(player, true);
            return true;
        }

        EverQuest->SetDungeonModeInstancedForPlayer(player, dungeonModeInstanced);
        EverQuest->SendDungeonModeStateToPlayer(player, true);
        return true;
    }

    // Used by the EQ_AuctionFilter addon
    static bool HandleEQAuctionFilterCommand(ChatHandler* handler, const char* args)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        Player* player = handler->GetPlayer();
        if (player == nullptr)
            return true;

        std::vector<std::string> tokens = SplitCommandArgs(args, 2);
        if (tokens.empty() == false)
        {
            std::string subCommand = tokens[0];
            boost::algorithm::to_lower(subCommand);
            if (subCommand == "sync")
            {
                EverQuest->SendAuctionRealmFilterToPlayer(player);
                return true;
            }
            if (subCommand == "set" && tokens.size() > 1)
            {
                if (EverQuest->SetAuctionRealmFilterForPlayer(player, tokens[1]) == true)
                    EverQuest->SendAuctionRealmFilterToPlayer(player);
                return true;
            }
        }

        handler->PSendSysMessage("Auction results can be limited to Norrath (EverQuest) or Azeroth (World of Warcraft) items per auction house category, using the |cff4CFF00Realm Filter|r button on the auction house window.");
        return true;
    }

    static bool HandleEQVerCommand(ChatHandler* handler, const char* args)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        // Sent automatically by the client UI shortly after entering the world, so bad input drops silently
        uint32 values[1];
        if (ParseUnsignedArgs(args, values, 1) != 1)
            return true;
        EverQuest->HandleClientVersionReportForPlayer(handler->GetPlayer(), values[0]);
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
        std::string className = GetFirstCommandArg(args);
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

    static bool HandleTrackList(ChatHandler* handler, const char* /*args*/)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        EverQuest->SendTrackingListToPlayer(handler->GetPlayer());
        return true;
    }

    static bool HandleTrackStart(ChatHandler* handler, const char* args)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        if (!args || !*args)
            return true;
        std::string guidString = GetFirstCommandArg(args);
        if (guidString.empty() == true || guidString.size() > 20)
            return true;
        for (size_t i = 0; i < guidString.size(); ++i)
        {
            if (isdigit(static_cast<unsigned char>(guidString[i])) == 0)
                return true;
        }

        uint64 rawCreatureGUID = strtoull(guidString.c_str(), nullptr, 10);
        EverQuest->StartTrackingForPlayer(handler->GetPlayer(), rawCreatureGUID);
        return true;
    }

    static bool HandleTrackStop(ChatHandler* handler, const char* /*args*/)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        EverQuest->StopTrackingForPlayer(handler->GetPlayer(), true);
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

    static int ParseUnsignedArgs(const char* args, uint32* valuesOut, int maxCount)
    {
        if (maxCount <= 0)
            return 0;
        std::vector<std::string> tokens = SplitCommandArgs(args, static_cast<size_t>(maxCount));
        int parsedCount = 0;
        for (std::string const& token : tokens)
        {
            for (size_t i = 0; i < token.size(); ++i)
            {
                if (isdigit(static_cast<unsigned char>(token[i])) == 0)
                    return parsedCount;
            }
            valuesOut[parsedCount] = static_cast<uint32>(strtoul(token.c_str(), nullptr, 10));
            parsedCount++;
        }
        return parsedCount;
    }

    static bool HandleSecondaryEquipInfo(ChatHandler* handler, const char* args)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        Player* player = handler->GetPlayer();
        uint32 values[1];
        if (ParseUnsignedArgs(args, values, 1) != 1)
            return true;
        uint8 eqClassID = static_cast<uint8>(values[0]);
        if (EverQuest->IsEQClassValidEquipmentStorageTargetForPlayer(player, eqClassID) == false)
            return true;
        EverQuest->SendClassEquipmentAddonMessageToPlayer(player, eqClassID);
        return true;
    }

    static bool HandleSecondaryEquipSet(ChatHandler* handler, const char* args)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        Player* player = handler->GetPlayer();
        uint32 values[5];
        if (ParseUnsignedArgs(args, values, 5) != 5)
            return true;
        uint8 eqClassID = static_cast<uint8>(values[0]);

        std::string errorText;
        if (EverQuest->EquipItemIntoSecondaryClassStorage(player, eqClassID, static_cast<uint8>(values[1]), static_cast<uint8>(values[2]), static_cast<uint8>(values[3]), values[4], errorText) == false)
        {
            handler->PSendSysMessage("|cffFF0000{}|r", errorText);

            // Resync the window after a rejected action
            if (EverQuest->IsEQClassValidEquipmentStorageTargetForPlayer(player, eqClassID) == true)
                EverQuest->SendClassEquipmentAddonMessageToPlayer(player, eqClassID);
        }
        return true;
    }

    static bool HandleSecondaryEquipRemove(ChatHandler* handler, const char* args)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        Player* player = handler->GetPlayer();
        uint32 values[4];
        int parsedCount = ParseUnsignedArgs(args, values, 4);
        if (parsedCount != 2 && parsedCount != 4)
            return true;
        uint8 eqClassID = static_cast<uint8>(values[0]);
        bool useSpecificBagPosition = (parsedCount == 4);

        std::string errorText;
        if (EverQuest->RemoveItemFromSecondaryClassStorage(player, eqClassID, static_cast<uint8>(values[1]), useSpecificBagPosition ? static_cast<uint8>(values[2]) : 0, useSpecificBagPosition ? static_cast<uint8>(values[3]) : 0, useSpecificBagPosition, errorText) == false)
        {
            handler->PSendSysMessage("|cffFF0000{}|r", errorText);

            if (EverQuest->IsEQClassValidEquipmentStorageTargetForPlayer(player, eqClassID) == true)
                EverQuest->SendClassEquipmentAddonMessageToPlayer(player, eqClassID);
        }
        return true;
    }

    static bool HandleSecondaryEquipMove(ChatHandler* handler, const char* args)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        Player* player = handler->GetPlayer();
        uint32 values[3];
        if (ParseUnsignedArgs(args, values, 3) != 3)
            return true;
        uint8 eqClassID = static_cast<uint8>(values[0]);

        std::string errorText;
        if (EverQuest->MoveItemWithinSecondaryClassStorage(player, eqClassID, static_cast<uint8>(values[1]), static_cast<uint8>(values[2]), errorText) == false)
            handler->PSendSysMessage("|cffFF0000{}|r", errorText);

        if (EverQuest->IsEQClassValidEquipmentStorageTargetForPlayer(player, eqClassID) == true)
            EverQuest->SendClassEquipmentAddonMessageToPlayer(player, eqClassID);
        return true;
    }

    static bool HandleSecondaryEquipSwap(ChatHandler* handler, const char* args)
    {
        if (EverQuest->IsEnabled == false)
            return true;

        Player* player = handler->GetPlayer();
        uint32 values[3];
        if (ParseUnsignedArgs(args, values, 3) != 3)
            return true;
        uint8 eqClassID = static_cast<uint8>(values[0]);

        std::string errorText;
        if (EverQuest->SwapSecondaryClassStorageItemWithLiveEquipment(player, eqClassID, static_cast<uint8>(values[1]), static_cast<uint8>(values[2]), errorText) == false)
        {
            // Equip failures already showed the standard client equip error and leave errorText empty
            if (errorText.empty() == false)
                handler->PSendSysMessage("|cffFF0000{}|r", errorText);

            if (EverQuest->IsEQClassValidEquipmentStorageTargetForPlayer(player, eqClassID) == true)
                EverQuest->SendClassEquipmentAddonMessageToPlayer(player, eqClassID);
        }
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

