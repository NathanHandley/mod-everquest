SET @dbname = DATABASE();

DELIMITER //

CREATE PROCEDURE IF NOT EXISTS `update_mod_everquest_character_settings_showbardpulse`()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'showBardPulse') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `showBardPulse` TINYINT(3) UNSIGNED NOT NULL DEFAULT '1';
        SELECT 'Added showBardPulse' AS status;
    ELSE
        SELECT 'showBardPulse exists' AS status;
    END IF;
END //

DELIMITER ;

CALL update_mod_everquest_character_settings_showbardpulse();
DROP PROCEDURE IF EXISTS update_mod_everquest_character_settings_showbardpulse;
