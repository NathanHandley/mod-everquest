-- eqadvrestore: The level of every EverQuest class profile at the moment a character lost the EverQuest Adventurer buff.
-- Written once by DisqualifyPlayerFromAdventurer on the false->true transition, and read by `.eqadventurer restore` to put
-- those levels back.  A character with no row here lost the buff before this tracking existed, and its levels are left alone.
-- Levels are keyed by SECONDARY EQ class, which is how mod_everquest_characters stores a parked profile (class 0 / None is a
-- real profile slot in that scheme, so it gets a column of its own).  A level of 0 means that class was never played.
-- Re-runnable on purpose: never DROP, so an existing deployment keeps its history.
CREATE TABLE IF NOT EXISTS `mod_everquest_character_adventurer_loss` (
	`guid` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
	`lossTimestamp` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Unix time the buff was lost',
	`lossSecondaryClass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Secondary EQ class the character was playing at the time',
	`levelNone` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`levelWarrior` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`levelCleric` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`levelPaladin` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`levelRanger` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`levelShadowKnight` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`levelDruid` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`levelMonk` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`levelBard` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`levelRogue` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`levelShaman` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`levelNecromancer` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`levelWizard` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`levelMagician` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`levelEnchanter` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (`guid`) USING BTREE
);
