DROP TABLE IF EXISTS `mod_everquest_character_class_skills`;
CREATE TABLE `mod_everquest_character_class_skills` (
	`guid` INT(10) UNSIGNED NOT NULL COMMENT 'Global Unique Identifier',
	`class` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`eqclass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`skill` SMALLINT(5) UNSIGNED NOT NULL,
	`value` SMALLINT(5) UNSIGNED NOT NULL,
	`max` SMALLINT(5) UNSIGNED NOT NULL,
	PRIMARY KEY (`guid`, `eqclass`, `skill`) USING BTREE,
	INDEX `idx_guidclass` (`guid`, `eqclass`) USING BTREE
);