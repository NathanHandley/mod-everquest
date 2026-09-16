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
// the effect stays.  The visible aura slot belongs to the core, so the entries are taken out of the aura packets on the way out instead.  Players' own worn auras are untouched
static bool HandleAuraUpdatePacketSend(WorldSession* session, WorldPacket const& packet)
{
    if (EverQuest->IsEnabled == false || EverQuest->ConfigCreatureWornEffectsHideAuraIcons == false)
        return true;

    try
    {
        uint64 rawTargetGUID = 0;
        size_t entriesStartPosition = ReadPackedGUIDAtPosition(packet, 0, rawTargetGUID);
        if (ObjectGuid(rawTargetGUID).IsAnyTypeCreature() == false)
            return true;

        // A single slot update is either the icon or its removal, and a removal carries no spell so it always goes through
        if (packet.GetOpcode() == SMSG_AURA_UPDATE)
            return EverQuest->IsWornEffectSpell(packet.read<uint32>(entriesStartPosition + 1)) == false;

        // The full list is resent without the hidden entries, and only when there was one to hide.  The resent packet comes back through here with nothing left to strip
        WorldPacket filteredPacket(SMSG_AURA_UPDATE_ALL, packet.size());
        filteredPacket.append(packet.contents(), entriesStartPosition);
        bool hasHiddenEntry = false;
        size_t position = entriesStartPosition;
        while (position < packet.size())
        {
            uint32 spellID = 0;
            size_t entryEndPosition = GetAuraUpdateEntryEndPosition(packet, position, spellID);
            if (entryEndPosition > packet.size())
                return true;
            if (EverQuest->IsWornEffectSpell(spellID) == true)
                hasHiddenEntry = true;
            else
                filteredPacket.append(packet.contents() + position, entryEndPosition - position);
            position = entryEndPosition;
        }
        if (hasHiddenEntry == false)
            return true;
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

class EverQuest_ServerScript : public ServerScript
{
public:
    EverQuest_ServerScript() : ServerScript("EverQuest_ServerScript", { SERVERHOOK_CAN_PACKET_SEND, SERVERHOOK_CAN_PACKET_RECEIVE }) { }

    bool CanPacketReceive(WorldSession* session, WorldPacket const& packet) override
    {
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
        if (opcode == MSG_LIST_STABLED_PETS)
            return EverQuest->HandleMentorshipStablePacketSend(session, packet);
        if (opcode == SMSG_AUCTION_LIST_RESULT)
            return HandleAuctionListResultPacketSend(session, packet);
        if (opcode == SMSG_AURA_UPDATE || opcode == SMSG_AURA_UPDATE_ALL)
            return HandleAuraUpdatePacketSend(session, packet);
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
