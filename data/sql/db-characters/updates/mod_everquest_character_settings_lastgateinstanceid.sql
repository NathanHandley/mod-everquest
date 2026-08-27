-- Adds the instance a gate tether was taken in, so returning from a gate can tell an instance that is still standing from one that has been reset.
-- Written as plain top-level statements (no DELIMITER / stored procedure) so it runs the same way pasted into a SQL window as it does through the mysql client.

SET @dbname = DATABASE();

SET @lastgateInstanceIdExists = (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'lastgateInstanceId');

SET @addLastgateInstanceId = IF(@lastgateInstanceIdExists = 0,
	'ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `lastgateInstanceId` INT(10) UNSIGNED NULL DEFAULT NULL AFTER `lastgateOrientation`',
	'SELECT ''lastgateInstanceId exists'' AS status');

PREPARE addLastgateInstanceIdStatement FROM @addLastgateInstanceId;
EXECUTE addLastgateInstanceIdStatement;
DEALLOCATE PREPARE addLastgateInstanceIdStatement;
