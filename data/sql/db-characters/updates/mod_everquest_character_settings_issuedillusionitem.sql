SET @dbname = DATABASE();

DELIMITER //

CREATE PROCEDURE IF NOT EXISTS `update_mod_everquest_character_settings_issuedillusionitem`()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'issuedIllusionItemId') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `issuedIllusionItemId` INT(10) UNSIGNED NOT NULL DEFAULT '0';
        SELECT 'Added issuedIllusionItemId' AS status;
    ELSE
        SELECT 'issuedIllusionItemId exists' AS status;
    END IF;
END //

DELIMITER ;

CALL update_mod_everquest_character_settings_issuedillusionitem();
DROP PROCEDURE IF EXISTS update_mod_everquest_character_settings_issuedillusionitem;
