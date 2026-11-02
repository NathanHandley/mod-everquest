SET @dbname = DATABASE();

DELIMITER //

CREATE PROCEDURE IF NOT EXISTS `update_mod_everquest_character_settings_eqorigin`()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'eqRace') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings`
            ADD COLUMN `eqRace` SMALLINT(5) UNSIGNED NOT NULL DEFAULT '0',
            ADD COLUMN `eqStartChoice` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
            ADD COLUMN `eqStartSecondClass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
            ADD COLUMN `eqHairStyle` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
            ADD COLUMN `eqHairColor` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
            ADD COLUMN `eqFacialHair` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
            ADD COLUMN `eqFactionAlignment` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
            ADD COLUMN `eqFactionWoWRace` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0';
        SELECT 'Added eq origin columns' AS status;
    ELSE
        SELECT 'eq origin columns exist' AS status;
    END IF;
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'eqEyeColor') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings`
            ADD COLUMN `eqEyeColor` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0';
        SELECT 'Added eq eye color column' AS status;
    END IF;
END //

DELIMITER ;

CALL update_mod_everquest_character_settings_eqorigin();
DROP PROCEDURE IF EXISTS update_mod_everquest_character_settings_eqorigin;
