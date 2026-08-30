SET @dbname = DATABASE();

DELIMITER //

CREATE PROCEDURE IF NOT EXISTS `update_mod_everquest_character_settings_dispelmessage`()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'showDispelMessage') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `showDispelMessage` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'When 1, a chat line names each spell dispelled off of this character';
        SELECT 'Added showDispelMessage' AS status;
    ELSE
        SELECT 'showDispelMessage exists' AS status;
    END IF;

    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'dispelMessageColor') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `dispelMessageColor` INT(10) UNSIGNED NOT NULL DEFAULT '16755200' COMMENT 'Color of that chat line as 0xRRGGBB, defaulting to amber (0xFFAA00)';
        SELECT 'Added dispelMessageColor' AS status;
    ELSE
        SELECT 'dispelMessageColor exists' AS status;
    END IF;
END //

DELIMITER ;

CALL update_mod_everquest_character_settings_dispelmessage();
DROP PROCEDURE IF EXISTS update_mod_everquest_character_settings_dispelmessage;
