-- The level a character's talent points are still counted from after releases on death took levels away, so a death never resets a talent build.
-- Written by RecordDeathTalentLevelsForPlayer when a spirit release lowers the level, and removed once the character has earned that level back.
-- Keyed by SECONDARY EQ class, which is how mod_everquest_characters stores a parked profile, since every profile has a level and a talent build of its own.
-- Re-runnable on purpose: never DROP, so an existing deployment keeps what its characters are owed.
CREATE TABLE IF NOT EXISTS `mod_everquest_character_death_talent_level` (
	`guid` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
	`eqclass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Secondary EQ class of the profile this belongs to',
	`talentLevel` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Highest level held before deaths lowered it, which talent points are still counted from',
	PRIMARY KEY (`guid`, `eqclass`) USING BTREE
);
