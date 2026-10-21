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

#include "Chat.h"
#include "Map.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellInfo.h"

#include "EverQuest.h"

using namespace std;

// Crouch, EverQuest's duck: a toggle every player knows.  The converter's spell is a Dummy aura that the
// MergedWorlds client reads to play the sneak animation and halve the collision height.  Here the crouched
// player cannot cast or melee, and anything they were casting or swinging stops when they crouch.  Like EQ
// (which re-runs its headroom check before it lets a ducking player stand), they can only stand back up where
// there is room.  Everything here runs on the player's map thread (spell checks, aura hooks, the attack swing
// packet) or the world thread (login, the cancel aura packet), and only touches the player and load-time config.

bool EverQuestMod::IsCrouchSpell(uint32 spellID)
{
    return ConfigSystemCrouchSpellID != 0 && spellID == ConfigSystemCrouchSpellID;
}

bool EverQuestMod::IsPlayerCrouched(Player* player)
{
    return player != nullptr && ConfigSystemCrouchSpellID != 0 && player->HasAura(ConfigSystemCrouchSpellID) == true;
}

bool EverQuestMod::HasRoomToStandUpFromCrouch(Player* player)
{
    // The server never shrinks the player's collision height (only the client does), so this is the standing height.
    // Anything solid between crouched head height and standing head height means no room
    Map* map = player->GetMap();
    if (map == nullptr)
        return true;
    float standingHeight = player->GetCollisionHeight();
    float x = player->GetPositionX();
    float y = player->GetPositionY();
    float z = player->GetPositionZ();
    return map->isInLineOfSight(x, y, z + standingHeight * EQ_CROUCH_COLLISION_HEIGHT_SCALE, x, y, z + standingHeight + EQ_CROUCH_STAND_UP_HEADROOM_MARGIN,
        player->GetPhaseMask(), LINEOFSIGHT_ALL_CHECKS, VMAP::ModelIgnoreFlags::Nothing);
}

bool EverQuestMod::HandleCrouchOnCheckCast(Player* player, Spell* spell, SpellCastResult& result)
{
    if (player == nullptr || spell == nullptr || spell->GetSpellInfo() == nullptr || spell->IsTriggered() == true || ConfigSystemCrouchSpellID == 0)
        return false;
    bool isCrouched = player->HasAura(ConfigSystemCrouchSpellID);

    // Crouch is a toggle: casting it while crouched stands the player up, which needs room overhead
    if (spell->GetSpellInfo()->Id == ConfigSystemCrouchSpellID)
    {
        if (isCrouched == false)
            return false;
        if (HasRoomToStandUpFromCrouch(player) == false)
            ChatHandler(player->GetSession()).PSendSysMessage("There isn't enough room to stand up here.");
        else
            player->RemoveAurasDueToSpell(ConfigSystemCrouchSpellID);
        result = SPELL_FAILED_DONT_REPORT;
        return true;
    }

    // Nothing else can be cast while crouched
    if (isCrouched == true)
    {
        result = SPELL_FAILED_NOT_STANDING;
        return true;
    }
    return false;
}

void EverQuestMod::HandleCrouchAuraApply(Unit* unit, Aura* aura)
{
    if (unit == nullptr || aura == nullptr || IsCrouchSpell(aura->GetId()) == false || unit->IsPlayer() == false)
        return;

    // Stop the swing and whatever else is being cast or channeled (Crouch itself may still be the spell in progress)
    unit->AttackStop();
    for (uint32 i = CURRENT_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
    {
        Spell* currentSpell = unit->GetCurrentSpell(CurrentSpellTypes(i));
        if (currentSpell != nullptr && currentSpell->GetSpellInfo() != nullptr && IsCrouchSpell(currentSpell->GetSpellInfo()->Id) == false)
            unit->InterruptSpell(CurrentSpellTypes(i), false);
    }

    // Pacified stops melee swings and silenced greys out spells in the player's interface (the cast check above is what actually refuses them)
    unit->SetUnitFlag(UNIT_FLAG_PACIFIED);
    unit->SetUnitFlag(UNIT_FLAG_SILENCED);
}

void EverQuestMod::HandleCrouchAuraRemove(Unit* unit, Aura* aura)
{
    if (unit == nullptr || aura == nullptr || IsCrouchSpell(aura->GetId()) == false || unit->IsPlayer() == false)
        return;

    // Leave the flags to any real pacify or silence still on the player
    if (unit->HasPacifyAura() == false && unit->HasPacifySilenceAura() == false)
        unit->RemoveUnitFlag(UNIT_FLAG_PACIFIED);
    if (unit->HasSilenceAura() == false && unit->HasPacifySilenceAura() == false)
        unit->RemoveUnitFlag(UNIT_FLAG_SILENCED);
}

void EverQuestMod::RemoveCrouchAtLogin(Player* player)
{
    // A character always logs in standing
    if (IsPlayerCrouched(player) == true)
        player->RemoveAurasDueToSpell(ConfigSystemCrouchSpellID);
}

bool EverQuestMod::HandleCrouchCancelAuraPacketReceive(WorldSession* session, WorldPacket const& packet)
{
    // Standing up from the buff bar, or by pressing Crouch again (the client cancels an active aura spell instead of
    // casting it), needs the same headroom as the toggle.  Returns false to drop the packet.  CMSG_CANCEL_AURA is
    // handled in place on the world thread, outside the map update window
    if (IsEnabled == false || packet.GetOpcode() != CMSG_CANCEL_AURA || session == nullptr || ConfigSystemCrouchSpellID == 0)
        return true;
    Player* player = session->GetPlayer();
    if (IsPlayerCrouched(player) == false || player->IsInWorld() == false)
        return true;
    WorldPacket packetCopy(packet);
    packetCopy.rpos(0);
    uint32 spellID = 0;
    try
    {
        packetCopy >> spellID;
    }
    catch (ByteBufferException const&)
    {
        return true;
    }
    if (spellID != ConfigSystemCrouchSpellID || HasRoomToStandUpFromCrouch(player) == true)
        return true;
    ChatHandler(session).PSendSysMessage("There isn't enough room to stand up here.");
    return false;
}

bool EverQuestMod::HandleCrouchAttackSwingPacketReceive(WorldSession* session, WorldPacket const& packet)
{
    // A crouched player can't start a melee attack.  Returns false to drop the packet
    if (IsEnabled == false || packet.GetOpcode() != CMSG_ATTACKSWING || session == nullptr)
        return true;
    Player* player = session->GetPlayer();
    if (IsPlayerCrouched(player) == false)
        return true;
    player->SendMeleeAttackStop(nullptr);
    return false;
}
