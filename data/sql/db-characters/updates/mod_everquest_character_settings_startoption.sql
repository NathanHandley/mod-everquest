SET @dbname = DATABASE();

DELIMITER //

CREATE PROCEDURE IF NOT EXISTS `update_mod_everquest_character_settings_startoption`()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'startOption') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `startOption` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0';
        SELECT 'Added startOption' AS status;
    ELSE
        SELECT 'startOption exists' AS status;
    END IF;
END //

DELIMITER ;

CALL update_mod_everquest_character_settings_startoption();
DROP PROCEDURE IF EXISTS update_mod_everquest_character_settings_startoption;
