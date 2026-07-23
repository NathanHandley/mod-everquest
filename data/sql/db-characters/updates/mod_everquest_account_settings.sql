CREATE TABLE IF NOT EXISTS `mod_everquest_account_settings` (
	`accountid` INT(10) UNSIGNED NOT NULL DEFAULT '0',
	`earnedAdventurerAchievement` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (`accountid`) USING BTREE
);
