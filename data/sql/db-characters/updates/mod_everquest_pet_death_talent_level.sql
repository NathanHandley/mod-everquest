-- The level a hunter pet's talent points are still counted from after its owner's releases on death pulled it down, so a death never resets the pet's talent build.
-- Written by RecordDeathTalentLevelsForPlayer for every hunter pet standing above the owner's new level, and removed once the pet has earned that level back.
-- Keyed by pet number alone (character_pet.id), which is unique across the server and follows the pet through stable slots and secondary class switches.
-- Re-runnable on purpose: never DROP, so an existing deployment keeps what its pets are owed.
CREATE TABLE IF NOT EXISTS `mod_everquest_pet_death_talent_level` (
	`petNumber` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'character_pet.id',
	`owner` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier of the owning character',
	`talentLevel` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Highest level held before the owner''s deaths lowered it, which talent points are still counted from',
	PRIMARY KEY (`petNumber`) USING BTREE,
	INDEX `idx_owner` (`owner`) USING BTREE
);
