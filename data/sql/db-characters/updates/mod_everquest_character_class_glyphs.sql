DROP TABLE IF EXISTS `mod_everquest_character_class_glyphs`;
CREATE TABLE `mod_everquest_character_class_glyphs` (
	`guid` INT(10) UNSIGNED NOT NULL,
	`class` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`eqclass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`talentGroup` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`glyph1` SMALLINT(5) UNSIGNED NULL DEFAULT '0',
	`glyph2` SMALLINT(5) UNSIGNED NULL DEFAULT '0',
	`glyph3` SMALLINT(5) UNSIGNED NULL DEFAULT '0',
	`glyph4` SMALLINT(5) UNSIGNED NULL DEFAULT '0',
	`glyph5` SMALLINT(5) UNSIGNED NULL DEFAULT '0',
	`glyph6` SMALLINT(5) UNSIGNED NULL DEFAULT '0',
	PRIMARY KEY (`guid`, `eqclass`, `talentGroup`) USING BTREE,
	INDEX `idx_guidclass` (`guid`, `eqclass`) USING BTREE
);