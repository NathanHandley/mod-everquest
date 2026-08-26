SET @dbname = DATABASE();

DELIMITER //

CREATE PROCEDURE IF NOT EXISTS `update_mod_everquest_account_settings_auctionrealmfilter`()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_account_settings' AND COLUMN_NAME = 'auctionRealmFilter') = 0 THEN
        ALTER TABLE `mod_everquest_account_settings` ADD COLUMN `auctionRealmFilter` VARCHAR(160) NOT NULL DEFAULT '';
        SELECT 'Added auctionRealmFilter' AS status;
    ELSE
        SELECT 'auctionRealmFilter exists' AS status;
    END IF;
END //

DELIMITER ;

CALL update_mod_everquest_account_settings_auctionrealmfilter();
DROP PROCEDURE IF EXISTS update_mod_everquest_account_settings_auctionrealmfilter;
