DROP TABLE IF EXISTS `mod_everquest_character_class_aura`;
CREATE TABLE `mod_everquest_character_class_aura` (
	`guid` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
	`class` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`eqclass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`casterGuid` BIGINT(20) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Full Global Unique Identifier',
	`itemGuid` BIGINT(20) UNSIGNED NOT NULL DEFAULT '0',
	`spell` INT(10) UNSIGNED NOT NULL DEFAULT '0',
	`effectMask` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`recalculateMask` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`stackCount` TINYINT(3) UNSIGNED NOT NULL DEFAULT '1',
	`amount0` INT(10) NOT NULL DEFAULT '0',
	`amount1` INT(10) NOT NULL DEFAULT '0',
	`amount2` INT(10) NOT NULL DEFAULT '0',
	`base_amount0` INT(10) NOT NULL DEFAULT '0',
	`base_amount1` INT(10) NOT NULL DEFAULT '0',
	`base_amount2` INT(10) NOT NULL DEFAULT '0',
	`maxDuration` INT(10) NOT NULL DEFAULT '0',
	`remainTime` INT(10) NOT NULL DEFAULT '0',
	`remainCharges` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (`guid`, `eqclass`, `casterGuid`, `itemGuid`, `spell`, `effectMask`) USING BTREE,
	INDEX `idx_guidclass` (`guid`, `eqclass`) USING BTREE
);