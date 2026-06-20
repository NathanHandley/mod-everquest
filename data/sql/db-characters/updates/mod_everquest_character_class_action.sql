DROP TABLE IF EXISTS `mod_everquest_character_class_action`;
CREATE TABLE `mod_everquest_character_class_action` (
	`guid` INT(10) UNSIGNED NOT NULL DEFAULT '0',
	`eqclass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`class` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`spec` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`button` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`action` INT(10) UNSIGNED NOT NULL DEFAULT '0',
	`type` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (`guid`, `eqclass`, `spec`, `button`) USING BTREE,
	INDEX `idx_guidclass` (`guid`, `eqclass`) USING BTREE
);