SET @dbname = DATABASE();

DELIMITER //

CREATE PROCEDURE IF NOT EXISTS `update_mod_everquest_character_settings_hailwindow`()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'hailWindowOnRightClick') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `hailWindowOnRightClick` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'When 1, right clicking a creature that only answers hails opens its reply instead of attacking it';
        SELECT 'Added hailWindowOnRightClick' AS status;
    ELSE
        SELECT 'hailWindowOnRightClick exists' AS status;
    END IF;
END //

DELIMITER ;

CALL update_mod_everquest_character_settings_hailwindow();
DROP PROCEDURE IF EXISTS update_mod_everquest_character_settings_hailwindow;
