SET @dbname = DATABASE();

DELIMITER //

CREATE PROCEDURE IF NOT EXISTS `update_mod_everquest_character_settings_movewhilecasting`()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'moveWhileCasting') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `moveWhileCasting` TINYINT(3) UNSIGNED NOT NULL DEFAULT '1' COMMENT 'When 1, this character keeps casting while moving; when 0, moving breaks those casts the way it normally does';
        SELECT 'Added moveWhileCasting' AS status;
    ELSE
        SELECT 'moveWhileCasting exists' AS status;
    END IF;
END //

DELIMITER ;

CALL update_mod_everquest_character_settings_movewhilecasting();
DROP PROCEDURE IF EXISTS update_mod_everquest_character_settings_movewhilecasting;
