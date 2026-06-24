-- Re-runnable: create the table only if it does not already exist (never drop, so existing
-- player data such as the secondary experience pool is preserved when this file is re-applied).
CREATE TABLE IF NOT EXISTS `mod_everquest_character_class_controller` (
	`guid` INT(10) UNSIGNED NOT NULL DEFAULT '0',
	`currentClass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`nextClass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`secondaryExpPool` INT(10) UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (`guid`) USING BTREE
);

-- Re-runnable: add the secondaryExpPool column only if it is missing (covers tables created
-- before this column existed).
SET @col_exists = (
	SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
	WHERE TABLE_SCHEMA = DATABASE()
		AND TABLE_NAME = 'mod_everquest_character_class_controller'
		AND COLUMN_NAME = 'secondaryExpPool'
);
SET @ddl = IF(@col_exists = 0,
	'ALTER TABLE `mod_everquest_character_class_controller` ADD COLUMN `secondaryExpPool` INT(10) UNSIGNED NOT NULL DEFAULT ''0''',
	'SELECT 1');
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
