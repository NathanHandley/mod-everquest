DROP TABLE IF EXISTS `mod_everquest_character_class_spell`;
CREATE TABLE `mod_everquest_character_class_spell` (
	`guid` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
	`class` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`eqclass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`spell` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Spell Identifier',
	`specMask` TINYINT(3) UNSIGNED NOT NULL DEFAULT '1',
	PRIMARY KEY (`guid`, `eqclass`, `spell`) USING BTREE,
	INDEX `idx_guidclass` (`guid`, `eqclass`) USING BTREE
);