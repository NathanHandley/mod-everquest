DROP TABLE IF EXISTS `mod_everquest_creature_onkill_reputation`;

CREATE TABLE `mod_everquest_creature_onkill_reputation` (
	`CreatureTemplateID` INT(10) UNSIGNED NOT NULL DEFAULT '0',
	`SortOrder` TINYINT(3) NOT NULL DEFAULT '0',
	`FactionID` SMALLINT(5) NOT NULL DEFAULT '0',
	`KillRewardValue` INT(10) NOT NULL DEFAULT '0',
	PRIMARY KEY (`CreatureTemplateID`, `SortOrder`) USING BTREE
);
