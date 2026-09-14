SET @dbname = DATABASE();

DELIMITER //

CREATE PROCEDURE IF NOT EXISTS `update_mod_everquest_character_settings_mezbreakmessage`()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'showMezBreakMessage') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `showMezBreakMessage` TINYINT(3) UNSIGNED NOT NULL DEFAULT '1' COMMENT 'When 1, a chat line names whoever broke a mesmerize this character cast';
        SELECT 'Added showMezBreakMessage' AS status;
    ELSE
        SELECT 'showMezBreakMessage exists' AS status;
    END IF;
END //

DELIMITER ;

CALL update_mod_everquest_character_settings_mezbreakmessage();
DROP PROCEDURE IF EXISTS update_mod_everquest_character_settings_mezbreakmessage;
