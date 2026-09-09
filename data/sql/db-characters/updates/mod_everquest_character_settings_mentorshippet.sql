SET @dbname = DATABASE();

DELIMITER //

CREATE PROCEDURE IF NOT EXISTS `update_mod_everquest_character_settings_mentorshippet`()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'mentorshipPetNumber') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `mentorshipPetNumber` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'character_pet.id of the hunter pet that came down to the borrowed mentorship level with this character, so the right pet is the one put back';
        SELECT 'Added mentorshipPetNumber' AS status;
    ELSE
        SELECT 'mentorshipPetNumber exists' AS status;
    END IF;

    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'mentorshipPetLevel') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `mentorshipPetLevel` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'The level to put the pet named by mentorshipPetNumber back to, since the core only ever returns a hunter pet to within five levels of its owner';
        SELECT 'Added mentorshipPetLevel' AS status;
    ELSE
        SELECT 'mentorshipPetLevel exists' AS status;
    END IF;
END //

DELIMITER ;

CALL update_mod_everquest_character_settings_mentorshippet();
DROP PROCEDURE IF EXISTS update_mod_everquest_character_settings_mentorshippet;
