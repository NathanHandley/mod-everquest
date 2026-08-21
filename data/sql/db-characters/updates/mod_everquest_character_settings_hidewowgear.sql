SET @dbname = DATABASE();

DELIMITER //

CREATE PROCEDURE IF NOT EXISTS `update_mod_everquest_character_settings_hidewowgear`()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'hideWoWGear') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `hideWoWGear` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0';
        SELECT 'Added hideWoWGear' AS status;
    ELSE
        SELECT 'hideWoWGear exists' AS status;
    END IF;
END //

DELIMITER ;

CALL update_mod_everquest_character_settings_hidewowgear();
DROP PROCEDURE IF EXISTS update_mod_everquest_character_settings_hidewowgear;
