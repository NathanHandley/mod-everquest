SET @dbname = DATABASE();

DELIMITER //

CREATE PROCEDURE IF NOT EXISTS `update_mod_everquest_character_settings_deathexp`()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'deathExpLost') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `deathExpLost` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Experience taken by spirit releases since the last resurrection, and still restorable';
        SELECT 'Added deathExpLost' AS status;
    ELSE
        SELECT 'deathExpLost exists' AS status;
    END IF;
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'deathExpRestGranted') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `deathExpRestGranted` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'How much of that loss was handed back as rest experience';
        SELECT 'Added deathExpRestGranted' AS status;
    ELSE
        SELECT 'deathExpRestGranted exists' AS status;
    END IF;
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'deathExpLostClass') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `deathExpLostClass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Secondary EQ class the loss belongs to, since a class switch parks the level it would be restored onto';
        SELECT 'Added deathExpLostClass' AS status;
    ELSE
        SELECT 'deathExpLostClass exists' AS status;
    END IF;
END //

DELIMITER ;

CALL update_mod_everquest_character_settings_deathexp();
DROP PROCEDURE IF EXISTS update_mod_everquest_character_settings_deathexp;
