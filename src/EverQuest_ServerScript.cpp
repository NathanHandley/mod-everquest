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

class EverQuest_ServerScript : public ServerScript
{
public:
    EverQuest_ServerScript() : ServerScript("EverQuest_ServerScript", { SERVERHOOK_CAN_PACKET_SEND }) { }

    // Hides the recurring bard song pulse graphic from players who turned it off (.eqshowbardpulse). The pulse is the SMSG_SPELL_START / SMSG_SPELL_GO pair of the generated bard tick spell
    // and the buff itself travels in separate aura packets so dropping these only removes the graphic for the receiving player
    bool CanPacketSend(WorldSession* session, WorldPacket const& packet) override
    {
        uint16 opcode = packet.GetOpcode();
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
