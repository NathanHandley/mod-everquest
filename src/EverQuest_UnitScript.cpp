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

#include "Chat.h"
#include "Player.h"
#include "Unit.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "SpellInfo.h"

#include "EverQuest.h"

#include <cmath>

using namespace std;

class EverQuest_UnitScript : public UnitScript
{
public:
    EverQuest_UnitScript() : UnitScript("EverQuest_UnitScript") {}

    bool TryHandleBashKickStunChance(Unit* defender, Aura* aura)
    {
        if (defender == nullptr || aura == nullptr)
            return false;

        uint32 spellID = aura->GetId();
        if (spellID < EverQuest->ConfigSystemSpellDBCIDMin || spellID > EverQuest->ConfigSystemSpellDBCIDMax)
            return false;
        if (EverQuest->IsSpellAnEQSpell(spellID) == false)
            return false;
        const EverQuestSpell& curSpell = EverQuest->GetSpellDataForSpellID(spellID);
        if (curSpell.StunUsesBashKickChance == false)
            return false;

        if (EverQuest->ConfigCombatSkillsDisableBashKickStunOnPlayers == true && defender->IsPlayer() == true)
        {
            defender->RemoveAura(aura);
            return true;
        }

        Unit* attacker = aura->GetCaster();
        if (attacker == nullptr)
            return true;

        if (attacker->isSpellBlocked(defender, aura->GetSpellInfo(), BASE_ATTACK) == true)
        {
            defender->RemoveAura(aura);
            return true;
        }

        if (EverQuest->RollBashKickStunLands(attacker, defender) == false)
        {
            defender->RemoveAura(aura);
            return true;
        }

        if (curSpell.SpellIDCastOnTargetWhenStunLands != 0)
            attacker->CastSpell(defender, curSpell.SpellIDCastOnTargetWhenStunLands, true);
        return true;
    }

    void OnUnitEnterCombat(Unit* unit, Unit* victim) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (unit == nullptr || victim == nullptr)
            return;
        Creature* creature = unit->ToCreature();
        if (creature == nullptr)
            return;
        EverQuest->ApplyScaledCreatureSocialAggroOnEngage(creature, victim);
    }

    void OnUnitEnterEvadeMode(Unit* unit, uint8 /*evadeReason*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (unit == nullptr)
            return;
        Creature* creature = unit->ToCreature();
        if (creature == nullptr)
            return;
        uint32 mapID = creature->GetMapId();
        if (mapID < EverQuest->ConfigSystemMapDBCIDMin || mapID > EverQuest->ConfigSystemMapDBCIDMax)
            return;

        // When a creature evades, let the player break charm so they aren't stuck
        Unit* charmed = creature->GetCharm();
        if (charmed == nullptr || charmed->IsPlayer() == false)
            return;
        charmed->RemoveCharmAuras();
    }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (unit == nullptr || aura == nullptr)
            return;

        if (EverQuest->ConfigDazeEnabledInEQZones == false && aura->GetId() == EQ_DAZE_SPELL_ID)
        {
            uint32 mapID = unit->GetMapId();
            if (mapID >= EverQuest->ConfigSystemMapDBCIDMin && mapID <= EverQuest->ConfigSystemMapDBCIDMax)
            {
                unit->RemoveAura(aura);
                return;
            }
        }

        if (TryHandleBashKickStunChance(unit, aura) == true)
            return;

        if (!unit->IsPlayer())
            return;

        Player* player = unit->ToPlayer();
        uint32 spellID = aura->GetId();

        if (EverQuest->IsSpellBlockedByMinTargetLevel(spellID, unit, aura->GetCaster()) == true)
        {
            Unit* auraCaster = aura->GetCaster();
            unit->RemoveAura(aura);
            if (auraCaster != nullptr && auraCaster->IsPlayer() == true)
                ChatHandler(auraCaster->ToPlayer()->GetSession()).PSendSysMessage("Your spell is too powerful for your intended target.");
            return;
        }

        if (EverQuest->IsSpellAnEQBardSong(spellID) == true && EverQuest->ConfigBardMaxConcurrentSongs != 0)
        {
            auto& queue = EverQuest->PlayerCasterConcurrentBardSongs[player->GetGUID()];

            // Refresh the cast
            auto it = std::find(queue.begin(), queue.end(), spellID);
            if (it != queue.end())
                queue.erase(it);
            queue.push_back(spellID);

            // Enforce a limit
            while (queue.size() > EverQuest->ConfigBardMaxConcurrentSongs)
            {
                uint32 oldest = queue.front();
                queue.pop_front();
                player->RemoveAurasDueToSpell(oldest);
            }
        }
    }

    void OnAuraRemove(Unit* unit, AuraApplication* aurApp, AuraRemoveMode mode) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (unit == nullptr)
            return;
        if (unit->IsPlayer())
        {
            if (aurApp == nullptr)
                return;
            if (aurApp->GetBase() == nullptr)
                return;
            if (aurApp->GetBase()->GetId() < EverQuest->ConfigSystemSpellDBCIDMin || aurApp->GetBase()->GetId() > EverQuest->ConfigSystemSpellDBCIDMax)
                return;
            if (aurApp->GetBase()->GetEffect(0) == nullptr)
                return;
            if (aurApp->GetBase()->GetEffect(0)->GetAuraType() != SPELL_AURA_DUMMY && aurApp->GetBase()->GetEffect(0)->GetAuraType() != SPELL_AURA_PERIODIC_DUMMY)
                return;

            // Handle gate recall
            if (mode == AURA_REMOVE_BY_CANCEL && aurApp->GetBase()->GetEffect(0)->GetMiscValue() == EQ_SPELLDUMMYTYPE_GATE)
            {
                EverQuest->SendPlayerToLastGate(unit->ToPlayer());
                return;
            }

            // Handle bard songs
            uint32 spellID = aurApp->GetBase()->GetId();
            if (EverQuest->IsSpellAnEQBardSong(spellID) == true)
            {
                Player* player = unit->ToPlayer();

                // Concurrence restrict                
                if (EverQuest->ConfigBardMaxConcurrentSongs != 0)
                {
                    
                    auto& queue = EverQuest->PlayerCasterConcurrentBardSongs[player->GetGUID()];
                    auto it = std::find(queue.begin(), queue.end(), spellID);
                    if (it != queue.end())
                        queue.erase(it);
                }
            }
        }
    }

    void ModifyPeriodicDamageAurasTick(Unit* /*target*/, Unit* attacker, uint32& damage, SpellInfo const* spellInfo) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (attacker == nullptr || spellInfo == nullptr)
            return;
        if (damage == 0)
            return;

        uint32 spellID = spellInfo->Id;
        if (EverQuest->IsSpellAnEQSpell(spellID) == false)
            return;

        uint32 boostPercent = EverQuest->CalculateSpellFocusBoostValue(attacker, spellID);
        if (boostPercent == 0)
            return;

        // Rounding is to match the non-periodic focus boost behavior
        damage = uint32(std::ceil((float)damage * (1.0f + (float)boostPercent / 100.0f)));
    }

    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (target == nullptr)
            return;
        if (attacker == nullptr)
            return;

        // Some creatures double/triple attack and off-hand attack
        EverQuest->TryDoCreatureEQMeleeExtraAttacks(attacker, target);

        if (damage <= 0)
            return;

        // Check auras for any melee attacker behavior
        Unit::AuraApplicationMap& auraMap = target->GetAppliedAuras();
        for (Unit::AuraApplicationMap::iterator iter = auraMap.begin(); iter != auraMap.end(); ++iter)
        {
            AuraApplication* aurApp = iter->second;
            if (aurApp == nullptr)
                continue;
            Aura* aura = aurApp->GetBase();
            if (aura == nullptr)
                continue;
            uint32 spellID = aura->GetId();
            if (spellID < EverQuest->ConfigSystemSpellDBCIDMin || spellID > EverQuest->ConfigSystemSpellDBCIDMax)
                continue;
            if (EverQuest->IsSpellAnEQSpell(spellID) == false)
                continue;
            const EverQuestSpell& curSpell = EverQuest->GetSpellDataForSpellID(spellID);
            if (curSpell.SpellIDCastOnMeleeAttacker == 0)
                continue;
            attacker->CastSpell(attacker, curSpell.SpellIDCastOnMeleeAttacker);
        }
    }

    void OnBeforeRollMeleeOutcomeAgainst(Unit const* /*attacker*/, Unit const* victim, WeaponAttackType /*attType*/,
        int32& /*attackerMaxSkillValueForLevel*/, int32& /*victimMaxSkillValueForLevel*/, int32& /*attackerWeaponSkill*/,
        int32& /*victimDefenseSkill*/, int32& /*crit_chance*/, int32& miss_chance, int32& dodge_chance,
        int32& parry_chance, int32& /*block_chance*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (victim == nullptr || victim->IsPlayer() == false)
            return;

        // This is a bit 'hacky', but this logic will fold dodge and parry into 'miss' during the casting of a bard song to
        // somewhat simulate how bard songs don't stop defensive/avoidance during combat. The only exception is that
        // block is not converted AND since everything is turning into a 'miss' any combat abilities that fire on dodge
        // or parry will simply not fire during the cast.
        if (victim->IsNonMeleeSpellCast(false, false, true) == false)
            return;
        Spell* currentSongCast = victim->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (currentSongCast == nullptr)
            return;
        if (EverQuest->IsSpellAnEQBardSong(currentSongCast->GetSpellInfo()->Id) == false)
            return;

        if (dodge_chance > 0)
        {
            miss_chance += dodge_chance;
            dodge_chance = 0;
        }
        if (parry_chance > 0)
        {
            miss_chance += parry_chance;
            parry_chance = 0;
        }
    }
};

void AddEverQuestUnitScripts()
{
    new EverQuest_UnitScript();
}
