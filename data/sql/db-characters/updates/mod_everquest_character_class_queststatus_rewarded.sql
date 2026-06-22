DROP TABLE IF EXISTS `mod_everquest_character_class_queststatus_rewarded`;
CREATE TABLE `mod_everquest_character_class_queststatus_rewarded` (
	`guid` INT(10) UNSIGNED NOT NULL DEFAULT '0',
	`eqclass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`quest` INT(10) UNSIGNED NOT NULL DEFAULT '0' ,
	`active` TINYINT(3) UNSIGNED NOT NULL DEFAULT '1',
	PRIMARY KEY (`guid`, `eqclass`, `quest`) USING BTREE,
	INDEX `idx_guidclass` (`guid`, `eqclass`) USING BTREE,
	INDEX `idx_guid` (`guid`) USING BTREE
);
