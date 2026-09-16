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

#include "GameTime.h"
#include "Map.h"
#include "MotionMaster.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellScript.h"

#include "EverQuest.h"

using namespace std;

class EverQuest_ClamberSpellScript : public SpellScript
{
    PrepareSpellScript(EverQuest_ClamberSpellScript);

    Movement::PointsArray ClamberPath;

    SpellCastResult FailClamber(Player* player, char const* message)
    {
        ClamberPath.clear();
        ChatHandler(player->GetSession()).SendSysMessage(message);
        return SPELL_FAILED_DONT_REPORT;
    }

    // The first floor at or below z within the search distance, or INVALID_HEIGHT when there isn't one
    float GetFloorHeight(Player* player, float x, float y, float z, float searchDistance)
    {
        return player->GetMap()->GetHeight(player->GetPhaseMask(), x, y, z, true, searchDistance);
    }

    // A swimmer only has to reach somewhere to stand
    SpellCastResult BuildSwimmingClamberPath(Player* player, float destX, float destY, float destZ)
    {
        ClamberPath.clear();

        float startX = player->GetPositionX();
        float startY = player->GetPositionY();
        float startZ = player->GetPositionZ();

        float destFloorZ = GetFloorHeight(player, destX, destY, destZ + EQ_CLAMBER_FLOOR_SEARCH_MARGIN, EQ_CLAMBER_FLOOR_SEARCH_MARGIN + EQ_CLAMBER_STANDING_TOLERANCE);
        if (destFloorZ <= INVALID_HEIGHT || destZ - destFloorZ > EQ_CLAMBER_STANDING_TOLERANCE)
            return FailClamber(player, "There is nothing to stand on there.");
        destZ = destFloorZ;

        float deltaX = destX - startX;
        float deltaY = destY - startY;
        float distance2D = std::sqrt(deltaX * deltaX + deltaY * deltaY);
        if (distance2D < EQ_CLAMBER_MIN_DISTANCE)
            return FailClamber(player, "That spot is too close to clamber to.");

        uint32 segmentCount = std::max<uint32>(2, uint32(std::ceil(distance2D / EQ_CLAMBER_SAMPLE_STEP_DISTANCE)));
        segmentCount = std::min<uint32>(segmentCount, EQ_CLAMBER_MAX_PATH_NODES - 1);
        float searchAboveLine = player->GetCollisionHeight() + EQ_CLAMBER_FLOOR_SEARCH_MARGIN;
        float searchDistance = searchAboveLine + EverQuest->ConfigSystemClamberMaxGapDepthInYards;

        ClamberPath.push_back(G3D::Vector3(startX, startY, startZ));
        for (uint32 segment = 1; segment < segmentCount; ++segment)
        {
            float progress = float(segment) / float(segmentCount);
            float sampleX = startX + deltaX * progress;
            float sampleY = startY + deltaY * progress;
            float lineZ = startZ + (destZ - startZ) * progress;

            // Stay at swimming level while the spot is above, since the floor there is under the liquid, and follow the ground once it rises past it
            float swimLevelZ = (destZ < startZ) ? lineZ : startZ;
            float sampleFloorZ = GetFloorHeight(player, sampleX, sampleY, lineZ + searchAboveLine, searchDistance);
            float sampleZ = (sampleFloorZ > INVALID_HEIGHT && sampleFloorZ > swimLevelZ) ? sampleFloorZ : swimLevelZ;
            ClamberPath.push_back(G3D::Vector3(sampleX, sampleY, sampleZ));
        }
        ClamberPath.push_back(G3D::Vector3(destX, destY, destZ));

        return SPELL_CAST_OK;
    }

    SpellCastResult BuildClamberPath(Player* player, float destX, float destY, float destZ)
    {
        ClamberPath.clear();

        float startX = player->GetPositionX();
        float startY = player->GetPositionY();
        float startZ = player->GetPositionZ();
        float collisionHeight = player->GetCollisionHeight();

        // Anyone not on a floor here is in the air (swimmers never reach this), where clambers could be chained up a wall from the top of a jump.
        // A clamber already under way is the exception: that movement is server driven along the ground, and a client side jump never has it,
        // so a new climb can be started part way up one without the interpolated position between two path nodes counting as being in the air
        float climbStartZ = startZ;
        bool isAlreadyBeingMoved = (player->GetMotionMaster()->GetMotionSlot(MOTION_SLOT_CONTROLLED) != nullptr);
        float startFloorZ = GetFloorHeight(player, startX, startY, startZ + EQ_CLAMBER_FLOOR_SEARCH_MARGIN, EQ_CLAMBER_FLOOR_SEARCH_MARGIN + EQ_CLAMBER_STANDING_TOLERANCE);
        if (isAlreadyBeingMoved == false && (startFloorZ <= INVALID_HEIGHT || startZ - startFloorZ > EQ_CLAMBER_STANDING_TOLERANCE))
            return FailClamber(player, "You need to be standing on the ground or swimming to clamber.");

        // The client picks the spot, so it has to actually be on a floor (not the top of the water or lava)
        float destFloorZ = GetFloorHeight(player, destX, destY, destZ + EQ_CLAMBER_FLOOR_SEARCH_MARGIN, EQ_CLAMBER_FLOOR_SEARCH_MARGIN + EQ_CLAMBER_STANDING_TOLERANCE);
        if (destFloorZ <= INVALID_HEIGHT || destZ - destFloorZ > EQ_CLAMBER_STANDING_TOLERANCE)
            return FailClamber(player, "There is nothing to stand on there.");
        destZ = destFloorZ;

        float deltaX = destX - startX;
        float deltaY = destY - startY;
        float distance2D = std::sqrt(deltaX * deltaX + deltaY * deltaY);
        if (distance2D < EQ_CLAMBER_MIN_DISTANCE)
            return FailClamber(player, "That spot is too close to clamber to.");

        float rise = destZ - climbStartZ;
        if (rise < -EverQuest->ConfigSystemClamberMaxDropInYards)
            return FailClamber(player, "You can only clamber upward.");

        float maxSlopeAngleInDegrees = std::clamp(EverQuest->ConfigSystemClamberMaxSlopeAngleInDegrees, 1.0f, 89.0f);
        float maxRisePerYard = std::tan(maxSlopeAngleInDegrees * float(M_PI) / 180.0f);
        if (rise > maxRisePerYard * distance2D)
            return FailClamber(player, "That is too steep to clamber up.");

        // At least two segments, so the movement uses this path rather than asking the navmesh for one
        uint32 segmentCount = std::max<uint32>(2, uint32(std::ceil(distance2D / EQ_CLAMBER_SAMPLE_STEP_DISTANCE)));
        segmentCount = std::min<uint32>(segmentCount, EQ_CLAMBER_MAX_PATH_NODES - 1);
        float segmentLength2D = distance2D / float(segmentCount);
        float maxSegmentRise = maxRisePerYard * segmentLength2D + EQ_CLAMBER_STEP_ALLOWANCE;

        // Line of sight passed at head height, so the ground can't be much more than a head height above the straight line
        float searchAboveLine = collisionHeight + EQ_CLAMBER_FLOOR_SEARCH_MARGIN;
        float searchDistance = searchAboveLine + EverQuest->ConfigSystemClamberMaxGapDepthInYards;

        ClamberPath.push_back(G3D::Vector3(startX, startY, startZ));
        float priorZ = climbStartZ;
        for (uint32 segment = 1; segment < segmentCount; ++segment)
        {
            float progress = float(segment) / float(segmentCount);
            float sampleX = startX + deltaX * progress;
            float sampleY = startY + deltaY * progress;
            float lineZ = climbStartZ + rise * progress;
            float sampleFloorZ = GetFloorHeight(player, sampleX, sampleY, lineZ + searchAboveLine, searchDistance);

            // Ground that drops well below the straight line is a gap (or a roof edge), not a slope
            if (sampleFloorZ <= INVALID_HEIGHT || sampleFloorZ < lineZ - EverQuest->ConfigSystemClamberMaxGapDepthInYards)
                return FailClamber(player, "You can't clamber across a gap.");

            // Each stretch has to be climbable too, so a gentle average can't hide a wall part way up
            if (sampleFloorZ - priorZ > maxSegmentRise)
                return FailClamber(player, "That is too steep to clamber up.");

            ClamberPath.push_back(G3D::Vector3(sampleX, sampleY, sampleFloorZ));
            priorZ = sampleFloorZ;
        }
        if (destZ - priorZ > maxSegmentRise)
            return FailClamber(player, "That is too steep to clamber up.");
        ClamberPath.push_back(G3D::Vector3(destX, destY, destZ));

        return SPELL_CAST_OK;
    }

    SpellCastResult CheckCast()
    {
        if (EverQuest->IsEnabled == false)
            return SPELL_FAILED_DONT_REPORT;

        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsPlayer() == false)
            return SPELL_FAILED_DONT_REPORT;
        Player* player = caster->ToPlayer();

        if (EverQuest->IsMapIDAnEverQuestMap(player->GetMapId()) == false)
            return SPELL_FAILED_INCORRECT_AREA;
        if (player->HasUnitState(UNIT_STATE_ROOT) == true)
            return SPELL_FAILED_ROOTED;
        // Mounted casting is allowed so the mount can be dropped, which also skips the core's taxi check
        if (player->IsInFlight() == true)
            return SPELL_FAILED_NOT_ON_TAXI;
        if (player->GetVehicle() != nullptr)
            return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
        WorldLocation const* destination = GetExplTargetDest();
        if (destination == nullptr)
            return SPELL_FAILED_BAD_TARGETS;

        // Swimming is settled before anything else, because the client reports a swimmer in an EQ zone as falling or flying
        if (player->isSwimming() == true || player->IsInWater() == true)
            return BuildSwimmingClamberPath(player, destination->GetPositionX(), destination->GetPositionY(), destination->GetPositionZ());

        if (player->IsFalling() == true || player->IsFlying() == true)
            return FailClamber(player, "You can't clamber while in the air.");

        return BuildClamberPath(player, destination->GetPositionX(), destination->GetPositionY(), destination->GetPositionZ());
    }

    void HandleClamber(SpellEffIndex /*effIndex*/)
    {
        if (EverQuest->IsEnabled == false)
            return;
        Unit* caster = GetCaster();
        if (caster == nullptr || caster->IsPlayer() == false)
            return;
        Player* player = caster->ToPlayer();
        if (ClamberPath.size() < 3)
            return;

        if (player->IsMounted() == true)
            player->RemoveAurasByType(SPELL_AURA_MOUNTED);

        // Normal run speed, or slower when snared (the mount is already gone, so it can't add to this)
        float runSpeed = playerBaseMoveSpeed[MOVE_RUN] * std::min(1.0f, player->GetSpeedRate(MOVE_RUN));
        if (runSpeed < 0.1f)
            return;

        // The path has to begin where the player is now
        ClamberPath[0] = G3D::Vector3(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ());
        G3D::Vector3 const& endPoint = ClamberPath.back();

        // Landing higher up must not count as a fall from where the clamber began
        player->SetFallInformation(GameTime::GetGameTime().count(), player->GetPositionZ());

        // MoveCharge refuses to start while the controlled slot is taken, so a clamber in progress is dropped first and this one takes over from where the player is now
        MotionMaster* motionMaster = player->GetMotionMaster();
        if (motionMaster->GetMotionSlot(MOTION_SLOT_CONTROLLED) != nullptr)
            motionMaster->MovementExpiredOnSlot(MOTION_SLOT_CONTROLLED, false);

        // Not a charge ID, so a root or stun part way still stops the run
        motionMaster->MoveCharge(endPoint.x, endPoint.y, endPoint.z, runSpeed, 0, &ClamberPath, false);
        sScriptMgr->AnticheatSetUnderACKmount(player);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(EverQuest_ClamberSpellScript::CheckCast);
        OnEffectHit += SpellEffectFn(EverQuest_ClamberSpellScript::HandleClamber, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

void AddEverQuestClamberSpellScripts()
{
    RegisterSpellScript(EverQuest_ClamberSpellScript);
}
