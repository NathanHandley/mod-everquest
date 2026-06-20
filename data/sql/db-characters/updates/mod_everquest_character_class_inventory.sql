DROP TABLE IF EXISTS `mod_everquest_character_class_inventory`;
CREATE TABLE `mod_everquest_character_class_inventory` (
	`guid` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
	`class` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`eqclass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`bag` INT(10) UNSIGNED NOT NULL DEFAULT '0',
	`slot` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`item` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Item Global Unique Identifier',
	PRIMARY KEY (`item`) USING BTREE,
	UNIQUE INDEX `guid` (`guid`, `eqclass`, `bag`, `slot`) USING BTREE,
	INDEX `idx_guid` (`guid`) USING BTREE,
	INDEX `idx_guidclass` (`guid`, `eqclass`) USING BTREE
);