CREATE TABLE IF NOT EXISTS `mod_everquest_quest_complete_reputation` (
	`QuestTemplateID` INT(10) UNSIGNED NOT NULL DEFAULT '0',
	`SortOrder` TINYINT(3) NOT NULL DEFAULT '0',
	`FactionID` SMALLINT(5) NOT NULL DEFAULT '0',
	`CompletionRewardValue` INT(10) NOT NULL DEFAULT '0',
	PRIMARY KEY (`QuestTemplateID`, `SortOrder`) USING BTREE
);
