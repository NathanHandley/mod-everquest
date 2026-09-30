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

#include "ByteBuffer.h"
#include "CharmInfo.h"
#include "ObjectAccessor.h"
#include "Opcodes.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraDefines.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include "EverQuest.h"

using namespace std;

// Both SMSG_SPELL_START and SMSG_SPELL_GO open with two packed guids (cast item or caster, then caster), a cast count byte, and then the spell ID.
static uint32 ExtractSpellIDFromSpellStartOrGoPacket(WorldPacket const& packet)
{
    WorldPacket packetCopy(packet);
    packetCopy.rpos(0);
    try
    {
        uint64 itemOrCasterGUID = 0;
        packetCopy.readPackGUID(itemOrCasterGUID);
        uint64 casterGUID = 0;
        packetCopy.readPackGUID(casterGUID);
        uint8 castCount = 0;
        packetCopy >> castCount;
        uint32 spellID = 0;
        packetCopy >> spellID;
        return spellID;
    }
    catch (ByteBufferException const&)
    {
        // Can this even happen?
        return 0;
    }
}

static bool HandleAuctionListResultPacketSend(WorldSession* session, WorldPacket const& packet)
{
    if (EverQuest->IsEnabled == false)
        return true;

    // The mod is putting its own already filtered answer on the wire, which arrives back here on the way out
    if (EverQuest->IsSendingOwnAuctionResult() == true)
        return true;

    Player* player = session->GetPlayer();
    if (player == nullptr)
        return true;

    // A browse the mod took over is answered a page at a time out of its own scan rather than from this packet
    if (EverQuest->ConsumeAuctionListResultForScan(session, packet) == true)
        return false;

    bool applyEQClassFilter = EverQuest->IsAuctionUsableFilterActiveForPlayer(player->GetGUID());
    EverQuestAuctionRealmFilter realmFilter;
    bool applyRealmFilter = EverQuest->TryGetActiveAuctionRealmFilterForPlayer(player, realmFilter);
    if (applyEQClassFilter == false && applyRealmFilter == false)
        return true;

    WorldPacket filteredPacket;
    if (EverQuest->BuildFilteredAuctionListPacket(player, packet, applyEQClassFilter, applyRealmFilter == true ? &realmFilter : nullptr, filteredPacket) == false)
        return true;
    session->SendPacket(&filteredPacket);
    return false;
}

// Reads a packed guid starting at pos without moving the packet's read position, and returns the position just past it
static size_t ReadPackedGUIDAtPosition(WorldPacket const& packet, size_t pos, uint64& guid)
{
    guid = 0;
    uint8 guidMask = packet.read<uint8>(pos);
    ++pos;
    for (uint8 i = 0; i < 8; ++i)
    {
        if ((guidMask & (uint8(1) << i)) == 0)
            continue;
        guid |= (uint64(packet.read<uint8>(pos)) << (i * 8));
        ++pos;
    }
    return pos;
}

// Returns the position just past one aura entry of SMSG_AURA_UPDATE / SMSG_AURA_UPDATE_ALL that starts at its slot byte.  Field layout must match AuraApplication::BuildUpdatePacket
static size_t GetAuraUpdateEntryEndPosition(WorldPacket const& packet, size_t pos, uint32& spellID)
{
    spellID = packet.read<uint32>(pos + 1);
    pos += 5;
    if (spellID == 0)
        return pos;
    uint8 flags = packet.read<uint8>(pos);
    pos += 3; // Flags, caster level, stack amount or charges
    if ((flags & AFLAG_CASTER) == 0)
    {
        uint64 casterGUID = 0;
        pos = ReadPackedGUIDAtPosition(packet, pos, casterGUID);
    }
    if ((flags & AFLAG_DURATION) != 0)
        pos += 8; // Max duration, duration
    return pos;
}

// Creatures carry the worn effect auras of the loot they rolled (ApplyLootWornEffectAuras), and EverQuest.CreatureWornEffects.HideAuraIcons keeps those icons off the client while
// the effect stays.  Split blocks that are only left visible so their damage or healing reaches the combat log (mod_everquest_spell.HideAuraIcon) are kept off every unit, since
// the base spell already shows the icon.  The visible aura slot belongs to the core, so the entries are taken out of the aura packets on the way out instead.  Players' own worn auras are untouched
static bool IsAuraIconHiddenFromClient(bool isCreatureTarget, uint32 spellID)
{
    if (EverQuest->IsHiddenAuraIconSpell(spellID) == true)
        return true;
    return isCreatureTarget == true && EverQuest->ConfigCreatureWornEffectsHideAuraIcons == true && EverQuest->IsWornEffectSpell(spellID) == true;
}

static bool HandleAuraUpdatePacketSend(WorldSession* session, WorldPacket const& packet)
{
    if (EverQuest->IsEnabled == false)
        return true;
    if (EverQuest->ConfigCreatureWornEffectsHideAuraIcons == false && EverQuest->HiddenAuraIconSpellIDs.empty() == true)
        return true;

    try
    {
        uint64 rawTargetGUID = 0;
        size_t entriesStartPosition = ReadPackedGUIDAtPosition(packet, 0, rawTargetGUID);
        bool isCreatureTarget = ObjectGuid(rawTargetGUID).IsAnyTypeCreature();

        // A single slot update is either the icon or its removal, and a removal carries no spell so it always goes through
        if (packet.GetOpcode() == SMSG_AURA_UPDATE)
            return IsAuraIconHiddenFromClient(isCreatureTarget, packet.read<uint32>(entriesStartPosition + 1)) == false;

        // The full list is resent without the hidden entries, and only when there was one to hide (checked first, so the common case copies nothing).  The resent packet
        // comes back through here with nothing left to strip
        bool hasHiddenEntry = false;
        size_t position = entriesStartPosition;
        while (position < packet.size())
        {
            uint32 spellID = 0;
            size_t entryEndPosition = GetAuraUpdateEntryEndPosition(packet, position, spellID);
            if (entryEndPosition > packet.size())
                return true;
            if (IsAuraIconHiddenFromClient(isCreatureTarget, spellID) == true)
            {
                hasHiddenEntry = true;
                break;
            }
            position = entryEndPosition;
        }
        if (hasHiddenEntry == false)
            return true;

        WorldPacket filteredPacket(SMSG_AURA_UPDATE_ALL, packet.size());
        filteredPacket.append(packet.contents(), entriesStartPosition);
        position = entriesStartPosition;
        while (position < packet.size())
        {
            uint32 spellID = 0;
            size_t entryEndPosition = GetAuraUpdateEntryEndPosition(packet, position, spellID);
            if (entryEndPosition > packet.size())
                return true;
            if (IsAuraIconHiddenFromClient(isCreatureTarget, spellID) == false)
                filteredPacket.append(packet.contents() + position, entryEndPosition - position);
            position = entryEndPosition;
        }
        session->SendPacket(&filteredPacket);
        return false;
    }
    catch (ByteBufferException const&)
    {
        return true;
    }
}

static bool HandleSetFactionAtWarPacketReceive(WorldSession* session)
{
    if (EverQuest->IsEnabled == false)
        return true;
    Player* player = session->GetPlayer();
    if (player == nullptr)
        return true;
    EverQuest->QueueTemporaryFactionRecalculationForPlayer(player->GetGUID());
    return true;
}

// WorldSession::HandlePetActionHelper only checks an attack command's target through the pet's owner, and a charmed creature has a charmer but no owner, so its attack order is never
// validated at all. That let a charmed creature be sent at a same-faction or non PvP flagged player (flagging them) or at a friendly NPC.  The same check the core makes for an owned pet is made
// here against the controlling player, which also covers every other unit it controls since the command goes to all of them.  CMSG_PET_ACTION is a thread safe opcode, so this runs on the player's
// map thread just like the handler it stands in front of
static bool HandlePetActionPacketReceive(WorldSession* session, WorldPacket const& packet)
{
    if (EverQuest->IsEnabled == false)
        return true;
    Player* player = session->GetPlayer();
    if (player == nullptr)
        return true;

    // Field layout must match WorldSession::HandlePetAction
    WorldPacket packetCopy(packet);
    packetCopy.rpos(0);
    ObjectGuid petGUID;
    uint32 actionData = 0;
    ObjectGuid targetGUID;
    try
    {
        packetCopy >> petGUID;
        packetCopy >> actionData;
        packetCopy >> targetGUID;
    }
    catch (ByteBufferException const&)
    {
        return true;
    }
    if (UNIT_ACTION_BUTTON_TYPE(actionData) != ACT_COMMAND || UNIT_ACTION_BUTTON_ACTION(actionData) != COMMAND_ATTACK)
        return true;

    Unit* target = ObjectAccessor::GetUnit(*player, targetGUID);
    if (target == nullptr)
        return true;
    return player->IsValidAttackTarget(target);
}

// Answers the client's mirror image data request for players rendering as a dressed EQ character model. The mod puts UNIT_FLAG2_MIRROR_IMAGE on players whose illusion form has a character model version,
// and the reply here dictates the race, customization bytes, and per-slot equipment display IDs the client composes onto that model - the client's only packet-driven dressed-character render path. The
// EQ race rides the client-side ChrRaces row generated by the converter, and the player's selected EQ face rides the hair style byte (EQ heads bake the face into the head texture). The core handler no-ops
// for these units (it requires a clone-caster aura), so answering in its place changes nothing else
static bool HandleGetMirrorImageDataPacketReceive(WorldSession* session, WorldPacket const& packet)
{
    if (EverQuest->IsEnabled == false)
        return true;
    Player* requester = session->GetPlayer();
    if (requester == nullptr)
        return true;

    WorldPacket packetCopy(packet);
    packetCopy.rpos(0);
    ObjectGuid targetGUID;
    try
    {
        packetCopy >> targetGUID;
    }
    catch (ByteBufferException const&)
    {
        return true;
    }

    Unit* unit = ObjectAccessor::GetUnit(*requester, targetGUID);
    if (unit == nullptr || unit->IsPlayer() == false)
        return true;
    if (unit->HasUnitFlag2(UNIT_FLAG2_MIRROR_IMAGE) == false || unit->HasCloneCasterAura() == true)
        return true;
    Player* subject = unit->ToPlayer();

    uint8 chrRaceID = 0;
    uint8 eqFaceByte = 0;
    if (EverQuest->TryGetIllusionCharacterMirrorDataForPlayer(subject, chrRaceID, eqFaceByte) == false)
        return true;

    WorldPacket data;
    EverQuest->BuildMirrorImageDataPacketForPlayer(subject, chrRaceID, eqFaceByte, data);
    session->SendPacket(&data);
    return false;
}

// The 3.3.5a client hard locks (100% of one core, forever) when its own active mover carries ROOT and HOVER at the same time while any turn, pitch, fall or ascend/descend bit is up, which
// is simply a levitating player getting rooted or stunned while holding a turn key.  CMovement::Update (Wow.exe 0x6F09F0) steps the mover in its own loop (0x6EAC40) until the step's consumed time
// reaches the target.  With no displacement that loop normally bails out, except while hovering (0x6EAD73), and then the local player's collide step (0x762E00) returns zero consumed time as soon
// as it sees ROOT (0x762F25), so the loop never finishes.  EQ levitation is hover, and spells like Whirlbolt root and levitate together, so the client is never allowed to have both: hover is
// taken off the client for as long as it is rooted and put back after the unroot.  The core only tracks the player's own flags from what the client reports back, and never checks hover there,
// so only the client's side changes.  Everything that roots, stuns or hovers the player reaches the client through these few packets, which is why this sits on the way out rather than on auras
static thread_local bool IsSendingOwnClientMovePacket = false;

class EverQuestOwnClientMovePacketGuard
{
public:
    EverQuestOwnClientMovePacketGuard() { IsSendingOwnClientMovePacket = true; }
    ~EverQuestOwnClientMovePacketGuard() { IsSendingOwnClientMovePacket = false; }
};

static void SendOwnClientMovePacket(WorldSession* session, WorldPacket const& packet)
{
    EverQuestOwnClientMovePacketGuard guard;
    session->SendPacket(&packet);
}

// Field layout must match Unit::SetHover
static void SendPlayerClientHoverPacket(WorldSession* session, Player* player, bool enable)
{
    WorldPacket data(enable == true ? SMSG_MOVE_SET_HOVER : SMSG_MOVE_UNSET_HOVER, player->GetPackGUID().size() + 4);
    data << player->GetPackGUID();
    data << uint32(session->GetOrderCounter());
    SendOwnClientMovePacket(session, data);
    session->IncrementOrderCounter();
}

// Sent once from Player::SendInitialPacketsAfterAddToMap, and carries root and hover together when the player comes into the world rooted and levitating.  Each entry is a length byte, then
// the opcode and payload of an ordinary move packet
static bool HandleMultipleMovesPacketSend(WorldSession* session, Player* player, WorldPacket const& packet)
{
    EverQuestPlayerClientMoveState* state = player->CustomData.GetDefault<EverQuestPlayerClientMoveState>(EQ_PLAYER_CUSTOMDATA_CLIENTMOVESTATE);
    uint64 playerRawGUID = player->GetGUID().GetRawValue();
    bool hasRoot = false;
    bool hasHover = false;
    size_t hoverEntryPosition = 0;
    size_t hoverEntryLength = 0;
    size_t position = 4;
    while (position < packet.size())
    {
        size_t entryLength = size_t(packet.read<uint8>(position)) + 1;
        if (position + entryLength > packet.size())
            return true;
        uint16 entryOpcode = packet.read<uint16>(position + 1);
        uint64 entryRawGUID = 0;
        ReadPackedGUIDAtPosition(packet, position + 3, entryRawGUID);
        if (entryRawGUID == playerRawGUID)
        {
            if (entryOpcode == SMSG_FORCE_MOVE_ROOT)
                hasRoot = true;
            else if (entryOpcode == SMSG_MOVE_SET_HOVER)
            {
                hasHover = true;
                hoverEntryPosition = position;
                hoverEntryLength = entryLength;
            }
        }
        position += entryLength;
    }

    state->ClientRooted = hasRoot;
    state->ClientHovering = hasHover && hasRoot == false;
    state->HoverHeldBack = hasHover && hasRoot;
    if (state->HoverHeldBack == false)
        return true;

    WorldPacket filteredPacket(SMSG_MULTIPLE_MOVES, packet.size());
    filteredPacket << uint32(0);
    filteredPacket.append(packet.contents() + 4, hoverEntryPosition - 4);
    filteredPacket.append(packet.contents() + hoverEntryPosition + hoverEntryLength, packet.size() - hoverEntryPosition - hoverEntryLength);
    filteredPacket.put<uint32>(0, uint32(filteredPacket.size() - 4));
    SendOwnClientMovePacket(session, filteredPacket);
    return false;
}

static bool HandleRootOrHoverPacketSend(WorldSession* session, WorldPacket const& packet)
{
    if (IsSendingOwnClientMovePacket == true || EverQuest->IsEnabled == false)
        return true;
    Player* player = session->GetPlayer();
    if (player == nullptr)
        return true;

    uint16 opcode = packet.GetOpcode();
    try
    {
        if (opcode == SMSG_MULTIPLE_MOVES)
            return HandleMultipleMovesPacketSend(session, player, packet);
        uint64 rawGUID = 0;
        ReadPackedGUIDAtPosition(packet, 0, rawGUID);
        if (rawGUID != player->GetGUID().GetRawValue())
            return true;
    }
    catch (ByteBufferException const&)
    {
        return true;
    }

    EverQuestPlayerClientMoveState* state = player->CustomData.GetDefault<EverQuestPlayerClientMoveState>(EQ_PLAYER_CUSTOMDATA_CLIENTMOVESTATE);
    switch (opcode)
    {
        case SMSG_FORCE_MOVE_ROOT:
        {
            state->ClientRooted = true;
            if (state->ClientHovering == false)
                return true;
            // The hover has to be gone before the root lands, so the root is resent behind it
            SendPlayerClientHoverPacket(session, player, false);
            state->ClientHovering = false;
            state->HoverHeldBack = true;
            SendOwnClientMovePacket(session, packet);
            return false;
        }
        case SMSG_FORCE_MOVE_UNROOT:
        {
            state->ClientRooted = false;
            if (state->HoverHeldBack == false)
                return true;
            // And comes back only once the unroot is on its way
            state->HoverHeldBack = false;
            SendOwnClientMovePacket(session, packet);
            if (player->HasAuraType(SPELL_AURA_HOVER) == true)
            {
                SendPlayerClientHoverPacket(session, player, true);
                state->ClientHovering = true;
            }
            return false;
        }
        case SMSG_MOVE_SET_HOVER:
        {
            if (state->ClientRooted == true)
            {
                state->HoverHeldBack = true;
                return false;
            }
            state->ClientHovering = true;
            return true;
        }
        case SMSG_MOVE_UNSET_HOVER:
        {
            state->ClientHovering = false;
            state->HoverHeldBack = false;
            return true;
        }
        default:
            return true;
    }
}

static void ResetPlayerClientMoveState(WorldSession* session)
{
    // A new world rebuilds the client's own mover from scratch, and the initial move packets that follow re-establish root and hover
    Player* player = session->GetPlayer();
    if (player == nullptr)
        return;
    EverQuestPlayerClientMoveState* state = player->CustomData.Get<EverQuestPlayerClientMoveState>(EQ_PLAYER_CUSTOMDATA_CLIENTMOVESTATE);
    if (state == nullptr)
        return;
    state->ClientRooted = false;
    state->ClientHovering = false;
    state->HoverHeldBack = false;
}

class EverQuest_ServerScript : public ServerScript
{
public:
    EverQuest_ServerScript() : ServerScript("EverQuest_ServerScript", { SERVERHOOK_CAN_PACKET_SEND, SERVERHOOK_CAN_PACKET_RECEIVE }) { }

    bool CanPacketReceive(WorldSession* session, WorldPacket const& packet) override
    {
        if (packet.GetOpcode() == CMSG_GET_MIRRORIMAGE_DATA)
            return HandleGetMirrorImageDataPacketReceive(session, packet);
        if (packet.GetOpcode() == CMSG_PET_ACTION)
            return HandlePetActionPacketReceive(session, packet);
        if (EverQuest->HandleMentorshipTrainerPacketReceive(session, packet) == false)
            return false;
        if (EverQuest->HandleMentorshipStablePacketReceive(session, packet) == false)
            return false;
        if (packet.GetOpcode() == CMSG_SET_FACTION_ATWAR)
            return HandleSetFactionAtWarPacketReceive(session);
        if (packet.GetOpcode() != CMSG_AUCTION_LIST_ITEMS)
            return true;
        if (EverQuest->IsEnabled == false)
            return true;
        Player* player = session->GetPlayer();
        if (player == nullptr)
            return true;

        // Field layout must match WorldSession::HandleAuctionListItems
        WorldPacket packetCopy(packet);
        packetCopy.rpos(0);
        try
        {
            ObjectGuid auctioneerGUID;
            uint32 listFrom;
            std::string searchedName;
            uint8 levelMin, levelMax, usable;
            uint32 auctionSlotID, auctionMainCategory, auctionSubCategory, quality;
            packetCopy >> auctioneerGUID;
            packetCopy >> listFrom;
            packetCopy >> searchedName;
            packetCopy >> levelMin >> levelMax;
            packetCopy >> auctionSlotID >> auctionMainCategory >> auctionSubCategory;
            packetCopy >> quality >> usable;
            EverQuest->SetAuctionUsableFilterActiveForPlayer(player->GetGUID(), usable != 0);
        }
        catch (ByteBufferException const&)
        {
        }

        // A filter that can empty a whole page has to be applied while the search is being paged
        if (EverQuest->TakeOverAuctionListRequest(session, packet) == true)
            return false;
        return true;
    }

    // Hides the recurring bard song pulse graphic from players who turned it off (.eqshowbardpulse). The pulse is the SMSG_SPELL_START / SMSG_SPELL_GO pair of the generated bard tick spell
    // and the buff itself travels in separate aura packets so dropping these only removes the graphic for the receiving player
    bool CanPacketSend(WorldSession* session, WorldPacket const& packet) override
    {
        uint16 opcode = packet.GetOpcode();
        if (opcode == SMSG_FORCE_MOVE_ROOT || opcode == SMSG_FORCE_MOVE_UNROOT || opcode == SMSG_MOVE_SET_HOVER || opcode == SMSG_MOVE_UNSET_HOVER || opcode == SMSG_MULTIPLE_MOVES)
            return HandleRootOrHoverPacketSend(session, packet);
        if (opcode == SMSG_NEW_WORLD || opcode == SMSG_LOGIN_VERIFY_WORLD)
        {
            ResetPlayerClientMoveState(session);
            return true;
        }
        if (opcode == MSG_LIST_STABLED_PETS)
            return EverQuest->HandleMentorshipStablePacketSend(session, packet);
        if (opcode == SMSG_AUCTION_LIST_RESULT)
            return HandleAuctionListResultPacketSend(session, packet);
        if (opcode == SMSG_AURA_UPDATE || opcode == SMSG_AURA_UPDATE_ALL)
            return HandleAuraUpdatePacketSend(session, packet);
        if (opcode == SMSG_ATTACKERSTATEUPDATE)
            return EverQuest->HandlePaladinFullBlockSwingLogPacketSend(session, packet);
        if (opcode != SMSG_SPELL_GO && opcode != SMSG_SPELL_START)
            return true;
        if (EverQuest->IsEnabled == false || EverQuest->BardSongTickSpellIDs.empty() == true)
            return true;
        Player* player = session->GetPlayer();
        if (player == nullptr)
            return true;

        uint32 spellID = ExtractSpellIDFromSpellStartOrGoPacket(packet);
        if (spellID == 0 || EverQuest->BardSongTickSpellIDs.find(spellID) == EverQuest->BardSongTickSpellIDs.end())
            return true;
        return EverQuest->GetShowBardPulseForPlayer(player);
    }
};

void AddEverQuestServerScripts()
{
    new EverQuest_ServerScript();
}
