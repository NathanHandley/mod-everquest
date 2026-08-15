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
#include "EverQuest_SpellTalentAlignment.h"

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

    void OnDamage(Unit* attacker, Unit* victim, uint32& /*damage*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        EverQuest->ProcessCreatureRetaliationOnDamage(attacker, victim);
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

        // Record (do not act on) a zone vertical agro violation before the social call, so a blocked pull does not seed a chain
        EverQuest->MarkCreatureAgroZBlockOnEngage(creature, victim);

        EverQuest->ApplyScaledCreatureSocialAggroOnEngage(creature, victim);

        // Enter combat emotes skip pets (like TAKP's EnterCombat DoNPCEmote)
        if (creature->IsPet() == false && creature->IsControlledByPlayer() == false)
            EverQuest->DoCreatureEmoteEvent(creature, EQ_CREATURE_EMOTE_EVENT_ENTERCOMBAT, victim);

        uint32 mapID = creature->GetMapId();
        if (mapID >= EverQuest->ConfigSystemMapDBCIDMin && mapID <= EverQuest->ConfigSystemMapDBCIDMax)
        {
            EverQuest->StoreCreatureAggroPosition(creature);
            EverQuest->ProcessKillSpawnsForCreatureEvent(creature, victim, EQ_KILLSPAWN_TRIGGER_COMBAT);
        }
    }

    void OnUnitDeath(Unit* unit, Unit* killer) override
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

        // TAKP fires 'OnDeath' at death and 'AfterDeath' right after the corpse forms, so both fire here in order
        EverQuest->DoCreatureEmoteEvent(creature, EQ_CREATURE_EMOTE_EVENT_ONDEATH, killer);
        EverQuest->DoCreatureEmoteEvent(creature, EQ_CREATURE_EMOTE_EVENT_AFTERDEATH, killer);
        if (killer != nullptr && killer != unit && killer->IsCreature() == true)
        {
            Creature* killerCreature = killer->ToCreature();
            if (killerCreature->IsPet() == false && killerCreature->IsControlledByPlayer() == false)
                EverQuest->DoCreatureEmoteEvent(killerCreature, EQ_CREATURE_EMOTE_EVENT_KILLEDNPC, creature);
        }

        EverQuest->ProcessKillSpawnsForCreatureEvent(creature, killer, EQ_KILLSPAWN_TRIGGER_DEATH);
        EverQuest->ProcessTriggeredQuestKillSpawnsForCreatureDeath(creature, killer);
        EverQuest->ProcessCycleSpawnForCreatureDeath(creature);
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

        // Note: SmartAI creatures never trigger this hook (SmartAI::EnterEvadeMode skips CreatureAI::EnterEvadeMode), which is why UpdateCreatureKillSpawnCombatWatch has the evade/ooc timer and kill spawn triggers
        if (creature->IsPet() == false && creature->IsControlledByPlayer() == false)
            EverQuest->DoCreatureEmoteEvent(creature, EQ_CREATURE_EMOTE_EVENT_LEAVECOMBAT, nullptr);

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

        EverQuest->TrackEQHasteAurasAndEnforceCapOnAuraApply(unit, aura);

        if (EverQuest->IsSpellBlockedByMaxCreatureTargetLevel(aura->GetId(), unit, aura->GetCaster()) == true)
        {
            Unit* ccAuraCaster = aura->GetCaster();
            unit->RemoveAura(aura);
            if (ccAuraCaster != nullptr && ccAuraCaster->IsPlayer() == true)
                ChatHandler(ccAuraCaster->ToPlayer()->GetSession()).PSendSysMessage("Your target is too high of a level for your spell to affect.");
            return;
        }

        // ModFaction (Alliance line) auras landing on creatures grant the caster a temporary reputation bonus with the creature's faction
        if (unit->IsCreature() == true)
            EverQuest->HandleModFactionAuraApplyOnCreature(unit->ToCreature(), aura);

        if (!unit->IsPlayer())
            return;

        Player* player = unit->ToPlayer();
        uint32 spellID = aura->GetId();

        // The core transform has already set the illusion model by now, so swap in the gear-matched version
        if (EverQuest->IsIllusionFormSpell(spellID) == true)
            EverQuest->ApplyIllusionGearDisplayOnFormAuraApply(player, spellID);

        // Illusion forms change how factions perceive the player
        if (EverQuest->GetSpellDataForSpellID(spellID).IllusionFormEQRaceID != 0)
            EverQuest->RecalculateTemporaryFactionReactionsForPlayer(player);

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
            // Only the lookup needs the lock; the queue itself is only touched by this player's own thread
            deque<uint32>* queue = nullptr;
            {
                std::lock_guard<std::mutex> lock(EverQuest->RuntimeStateMutex);
                queue = &EverQuest->PlayerCasterConcurrentBardSongs[player->GetGUID()];
            }

            // Refresh the cast
            auto it = std::find(queue->begin(), queue->end(), spellID);
            if (it != queue->end())
                queue->erase(it);
            queue->push_back(spellID);

            // Enforce a limit
            while (queue->size() > EverQuest->ConfigBardMaxConcurrentSongs)
            {
                uint32 oldest = queue->front();
                queue->pop_front();
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

        if (aurApp != nullptr && aurApp->GetBase() != nullptr)
            EverQuest->UntrackEQHasteAurasAndEnforceCapOnAuraRemove(unit, aurApp->GetBase());

        // A fading ModFaction (Alliance line) aura takes the caster's temporary reputation bonus with it
        if (unit->IsCreature() == true && aurApp != nullptr && aurApp->GetBase() != nullptr)
            EverQuest->HandleModFactionAuraRemoveFromCreature(unit->ToCreature(), aurApp);

        if (unit->IsPlayer())
        {
            if (aurApp == nullptr)
                return;
            if (aurApp->GetBase() == nullptr)
                return;

            EverQuest->HandleIllusionFormAuraRemove(unit->ToPlayer(), aurApp->GetBase()->GetId());

            // Leaving a shapeshift form uncovers the illusion, so put the gear-matched illusion display back (the core has already restored the base transform model by this point)
            if (aurApp->GetBase()->GetSpellInfo() != nullptr && aurApp->GetBase()->GetSpellInfo()->HasAura(SPELL_AURA_MOD_SHAPESHIFT) == true)
                EverQuest->RefreshIllusionGearDisplayForPlayer(unit->ToPlayer());

            // Dropping an illusion form restores how factions naturally perceive the player
            if (EverQuest->GetSpellDataForSpellID(aurApp->GetBase()->GetId()).IllusionFormEQRaceID != 0)
                EverQuest->RecalculateTemporaryFactionReactionsForPlayer(unit->ToPlayer());

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
                    deque<uint32>* queue = nullptr;
                    {
                        std::lock_guard<std::mutex> lock(EverQuest->RuntimeStateMutex);
                        auto queueIt = EverQuest->PlayerCasterConcurrentBardSongs.find(player->GetGUID());
                        if (queueIt != EverQuest->PlayerCasterConcurrentBardSongs.end())
                            queue = &queueIt->second;
                    }
                    if (queue != nullptr)
                    {
                        auto it = std::find(queue->begin(), queue->end(), spellID);
                        if (it != queue->end())
                            queue->erase(it);
                    }
                }
            }
        }
    }

    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker, uint32& damage, SpellInfo const* spellInfo) override
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
        if (boostPercent != 0)
        {
            // Rounding is to match the non-periodic focus boost behavior
            damage = uint32(std::ceil((float)damage * (1.0f + (float)boostPercent / 100.0f)));
        }

        // Also called for periodic heal ticks
        if (EverQuest->ConfigSpellTalentAlignmentEnabled == true && target != nullptr && attacker->IsPlayer() == true && EverQuestSpellTalentAlignment::DoesSpellInfoDealPeriodicDamage(spellInfo) == true)
        {
            // Warlock "Death's Embrace" boosts periodic Shadow damage against low health targets too
            int32 deathsEmbracePercent = GetDeathsEmbraceDamagePercent(target, attacker, spellInfo);
            if (deathsEmbracePercent > 0)
                damage += (damage * uint32(deathsEmbracePercent)) / 100;

            // Warlock "Soul Siphon" boosts the periodic ticks of EverQuest lifetap spells too (the heal follows the damage)
            int32 soulSiphonPercent = GetSoulSiphonDamagePercent(target, attacker, spellInfo);
            if (soulSiphonPercent > 0)
                damage += (damage * uint32(soulSiphonPercent)) / 100;

            // Priest "Mind Melt" lets EverQuest Shadow damage over time spells critically tick.  A family 0 spell can never satisfy the aura that normally enables periodic crits, so the roll happens here (spell crit + the talent bonus)
            if ((spellInfo->SchoolMask & SPELL_SCHOOL_MASK_SHADOW) != 0)
            {
                AuraEffect const* mindMeltEffect = attacker->GetAuraEffectOfRankedSpell(EQ_SPELL_ID_PRIEST_MIND_MELT_RANK1, EFFECT_1);
                if (mindMeltEffect != nullptr)
                {
                    float critChance = attacker->ToPlayer()->GetFloatValue(static_cast<uint16>(PLAYER_SPELL_CRIT_PERCENTAGE1) + SPELL_SCHOOL_SHADOW)
                        + float(mindMeltEffect->GetAmount());
                    if (roll_chance_f(critChance) == true)
                        damage += damage / 2;
                }
            }

            // Warlock "Pandemic" grants EverQuest Shadow damage over time spells the same manual critical ticks, at the player's Shadow crit plus "Malediction"'s bonus, with Pandemic's increased critical damage (the base spell
            // critical bonus of half the damage, raised by Pandemic's percent)
            if ((spellInfo->SchoolMask & SPELL_SCHOOL_MASK_SHADOW) != 0)
            {
                AuraEffect const* pandemicEffect = attacker->GetAuraEffect(EQ_SPELL_ID_WARLOCK_PANDEMIC, EFFECT_0);
                if (pandemicEffect != nullptr)
                {
                    float critChance = attacker->ToPlayer()->GetFloatValue(static_cast<uint16>(PLAYER_SPELL_CRIT_PERCENTAGE1) + SPELL_SCHOOL_SHADOW);
                    AuraEffect const* maledictionEffect = attacker->GetAuraEffectOfRankedSpell(EQ_SPELL_ID_WARLOCK_MALEDICTION_RANK1, EFFECT_0);
                    if (maledictionEffect != nullptr)
                        critChance += float(maledictionEffect->GetAmount());
                    if (roll_chance_f(critChance) == true)
                    {
                        uint32 criticalBonus = damage / 2;
                        criticalBonus += (criticalBonus * uint32(pandemicEffect->GetAmount())) / 100;
                        damage += criticalBonus;
                    }
                }
            }
        }
    }

    void ModifyHealReceived(Unit* target, Unit* healer, uint32& heal, SpellInfo const* spellInfo) override
    {
        if (EverQuest->IsEnabled == false || EverQuest->ConfigSpellTalentAlignmentEnabled == false)
            return;
        if (target == nullptr || healer == nullptr || spellInfo == nullptr || heal == 0)
            return;
        if (EverQuest->IsSpellAnEQSpell(spellInfo->Id) == false)
            return;

        // Priest "Improved Vampiric Embrace" also boosts the health returned by EverQuest lifetap spells with only the healing side is boosted here
        if (target->IsPlayer() == true && EverQuestSpellTalentAlignment::DoesSpellInfoLeech(spellInfo) == true)
        {
            AuraEffect const* improvedVampiricEmbraceEffect = target->GetAuraEffectOfRankedSpell(EQ_SPELL_ID_PRIEST_IMPROVED_VAMPIRIC_EMBRACE_RANK1, EFFECT_0);
            if (improvedVampiricEmbraceEffect != nullptr)
                heal += (heal * uint32(improvedVampiricEmbraceEffect->GetAmount())) / 100;
        }

        // For Priest "Improved Flash Heal"
        if (EverQuestSpellTalentAlignment::DoesSpellInfoDealDirectHeal(spellInfo) == true && EverQuestSpellTalentAlignment::DoesSpellInfoDealPeriodicHeal(spellInfo) == false)
        {
            Unit* healCaster = target;
            Unit* healedUnit = healer;
            if (healCaster->IsPlayer() == true && healedUnit->GetHealthPct() <= 50.0f)
            {
                AuraEffect const* improvedFlashHealEffect = healCaster->GetAuraEffectOfRankedSpell(EQ_SPELL_ID_PRIEST_IMPROVED_FLASH_HEAL_RANK1, EFFECT_0);
                if (improvedFlashHealEffect != nullptr && roll_chance_i(improvedFlashHealEffect->GetAmount()) == true)
                    heal += heal / 2;
            }
        }
    }

    uint32 GetEverQuestShadowDoTCountOnTarget(Unit* target, Unit* attacker)
    {
        uint32 shadowDoTCount = 0;
        Unit::AuraApplicationMap const& targetAuraMap = target->GetAppliedAuras();
        for (Unit::AuraApplicationMap::const_iterator iter = targetAuraMap.begin(); iter != targetAuraMap.end(); ++iter)
        {
            if (iter->second == nullptr)
                continue;
            Aura const* aura = iter->second->GetBase();
            if (aura == nullptr || aura->GetCasterGUID() != attacker->GetGUID())
                continue;
            SpellInfo const* auraSpellInfo = aura->GetSpellInfo();
            if (EverQuest->IsSpellAnEQSpell(auraSpellInfo->Id) == false)
                continue;
            if ((auraSpellInfo->SchoolMask & SPELL_SCHOOL_MASK_SHADOW) == 0)
                continue;
            if (EverQuestSpellTalentAlignment::DoesSpellInfoDealPeriodicDamage(auraSpellInfo) == false)
                continue;
            shadowDoTCount++;
        }
        return shadowDoTCount;
    }

    int32 GetSoulSiphonDamagePercent(Unit* target, Unit* attacker, SpellInfo const* spellInfo)
    {
        if (EverQuestSpellTalentAlignment::DoesSpellInfoLeech(spellInfo) == false)
            return 0;
        AuraEffect const* soulSiphonStepEffect = attacker->GetAuraEffectOfRankedSpell(EQ_SPELL_ID_WARLOCK_SOUL_SIPHON_RANK1, EFFECT_0);
        if (soulSiphonStepEffect == nullptr)
            return 0;
        int32 modPercent = int32(GetEverQuestShadowDoTCountOnTarget(target, attacker)) * soulSiphonStepEffect->GetAmount();
        if (modPercent <= 0)
            return 0;
        AuraEffect const* soulSiphonCapEffect = attacker->GetAuraEffectOfRankedSpell(EQ_SPELL_ID_WARLOCK_SOUL_SIPHON_RANK1, EFFECT_1);
        if (soulSiphonCapEffect != nullptr && modPercent > soulSiphonCapEffect->GetAmount())
            modPercent = soulSiphonCapEffect->GetAmount();
        return modPercent;
    }

    int32 GetDeathsEmbraceDamagePercent(Unit* target, Unit* attacker, SpellInfo const* spellInfo)
    {
        if ((spellInfo->SchoolMask & SPELL_SCHOOL_MASK_SHADOW) == 0)
            return 0;
        if (target->HealthBelowPct(35) == false)
            return 0;
        AuraEffect const* deathsEmbraceEffect = attacker->GetAuraEffectOfRankedSpell(EQ_SPELL_ID_WARLOCK_DEATHS_EMBRACE_RANK1, EFFECT_1);
        if (deathsEmbraceEffect == nullptr)
            return 0;
        return deathsEmbraceEffect->GetAmount();
    }

    // Only fires for direct spell damage, periodic runs through ModifyPeriodicDamageAurasTick above
    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
    {
        if (EverQuest->IsEnabled == false || EverQuest->ConfigSpellTalentAlignmentEnabled == false)
            return;
        if (target == nullptr || attacker == nullptr || spellInfo == nullptr)
            return;
        if (damage <= 0)
            return;
        if (attacker->IsPlayer() == false)
            return;
        if (EverQuest->IsSpellAnEQSpell(spellInfo->Id) == false)
            return;

        // Mage "Torment the Weak"
        AuraEffect const* tormentTheWeakEffect = attacker->GetAuraEffectOfRankedSpell(EQ_SPELL_ID_MAGE_TORMENT_THE_WEAK_RANK1, EFFECT_0);
        if (tormentTheWeakEffect != nullptr && target->HasAuraWithMechanic((1 << MECHANIC_SNARE) | (1 << MECHANIC_SLOW_ATTACK)) == true)
            damage += (damage * tormentTheWeakEffect->GetAmount()) / 100;

        // Warlock "Death's Embrace"
        int32 deathsEmbracePercent = GetDeathsEmbraceDamagePercent(target, attacker, spellInfo);
        if (deathsEmbracePercent > 0)
            damage += (damage * deathsEmbracePercent) / 100;

        // Warlock "Soul Siphon"
        int32 soulSiphonPercent = GetSoulSiphonDamagePercent(target, attacker, spellInfo);
        if (soulSiphonPercent > 0)
            damage += (damage * soulSiphonPercent) / 100;
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

        // An enraged creature ripostes frontal melee
        EverQuest->TryDoCreatureEnrageRiposteCounter(target, attacker);

        // Rampage and wild rampage swings can carry a damage percent modifier
        EverQuest->ApplyCreatureCombatAbilityDamageMod(attacker, damage);

        if (damage <= 0)
            return;

        // Check auras for any melee attacker behavior. Snapshot the spell IDs first, since the casts can mutate
        // the target's aura map (procs, charge consumption) and invalidate the iterator mid-loop
        vector<uint32> spellIDsToCastOnAttacker;
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
            spellIDsToCastOnAttacker.push_back(curSpell.SpellIDCastOnMeleeAttacker);
        }
        for (uint32 spellIDToCastOnAttacker : spellIDsToCastOnAttacker)
            attacker->CastSpell(attacker, spellIDToCastOnAttacker);
    }

    void OnBeforeRollMeleeOutcomeAgainst(Unit const* attacker, Unit const* victim, WeaponAttackType attType,
        int32& /*attackerMaxSkillValueForLevel*/, int32& /*victimMaxSkillValueForLevel*/, int32& /*attackerWeaponSkill*/,
        int32& /*victimDefenseSkill*/, int32& crit_chance, int32& miss_chance, int32& dodge_chance,
        int32& parry_chance, int32& block_chance) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (victim == nullptr || attacker == nullptr)
            return;

        // For enrage, parry all frontal melee (to simulate riposte counterattack)
        if (attType != RANGED_ATTACK && EverQuest->IsCreatureEnragedForRiposte(victim, attacker) == true)
        {
            miss_chance = 0;
            dodge_chance = 0;
            block_chance = 0;
            crit_chance = 0;
            parry_chance = 30000; // This makes parry always win the roll
            return;
        }

        if (victim->IsPlayer() == false)
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

    void OnUnitSetShapeshiftForm(Unit* unit, uint8 /*form*/) override
    {
        if (EverQuest->IsEnabled == false)
            return;
        if (unit == nullptr || unit->IsPlayer() == false)
            return;

        // Entering or leaving bear/dire bear form changes whether an equipped shield's armor gets multiplied by the form
        EverQuest->RefreshBearFormShieldArmorShiftForPlayer(unit->ToPlayer());
    }
};

void AddEverQuestUnitScripts()
{
    new EverQuest_UnitScript();
}
