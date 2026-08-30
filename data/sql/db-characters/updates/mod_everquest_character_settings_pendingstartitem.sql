SET @dbname = DATABASE();

DELIMITER //

CREATE PROCEDURE IF NOT EXISTS `update_mod_everquest_character_settings_pendingstartitem`()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'pendingStartItemEQClass') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `pendingStartItemEQClass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'EQ class that was taken on for the first time at the last class switch and is still owed its start items, since the switch runs at logout.  0 means nothing is owed';
        SELECT 'Added pendingStartItemEQClass' AS status;
    ELSE
        SELECT 'pendingStartItemEQClass exists' AS status;
    END IF;
END //

DELIMITER ;

CALL update_mod_everquest_character_settings_pendingstartitem();
DROP PROCEDURE IF EXISTS update_mod_everquest_character_settings_pendingstartitem;
