-- Adds the hearthstone tether position, kept apart from the gate tether so a character can hold both at once.
-- Written as plain top-level statements (no DELIMITER / stored procedure) so it runs the same way pasted into a SQL window as it does through the mysql client.

SET @dbname = DATABASE();

SET @lasthearthMapIdExists = (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'lasthearthMapId');

SET @addLasthearth = IF(@lasthearthMapIdExists = 0,
	'ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `lasthearthMapId` SMALLINT(5) UNSIGNED NULL DEFAULT NULL AFTER `lastgateInstanceId`, ADD COLUMN `lasthearthZoneId` SMALLINT(5) UNSIGNED NULL DEFAULT NULL AFTER `lasthearthMapId`, ADD COLUMN `lasthearthPosX` FLOAT NULL DEFAULT NULL AFTER `lasthearthZoneId`, ADD COLUMN `lasthearthPosY` FLOAT NULL DEFAULT NULL AFTER `lasthearthPosX`, ADD COLUMN `lasthearthPosZ` FLOAT NULL DEFAULT NULL AFTER `lasthearthPosY`, ADD COLUMN `lasthearthOrientation` FLOAT NULL DEFAULT NULL AFTER `lasthearthPosZ`, ADD COLUMN `lasthearthInstanceId` INT(10) UNSIGNED NULL DEFAULT NULL AFTER `lasthearthOrientation`',
	'SELECT ''lasthearth columns exist'' AS status');

PREPARE addLasthearthStatement FROM @addLasthearth;
EXECUTE addLasthearthStatement;
DEALLOCATE PREPARE addLasthearthStatement;
