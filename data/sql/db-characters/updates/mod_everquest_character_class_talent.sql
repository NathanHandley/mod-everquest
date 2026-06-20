DROP TABLE IF EXISTS `mod_everquest_character_class_talent`;
CREATE TABLE `mod_everquest_character_class_talent` (
	`guid` INT(10) UNSIGNED NOT NULL,
	`class` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`eqclass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`spell` INT(10) UNSIGNED NOT NULL,
	`specMask` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (`guid`, `eqclass`, `spell`) USING BTREE,
	INDEX `idx_guidclass` (`guid`, `eqclass`) USING BTREE
);