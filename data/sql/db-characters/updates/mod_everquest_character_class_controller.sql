DROP TABLE IF EXISTS `mod_everquest_character_class_controller`;
CREATE TABLE `mod_everquest_character_class_controller` (
	`guid` INT(10) UNSIGNED NOT NULL DEFAULT '0',
	`currentClass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	`nextClass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (`guid`) USING BTREE
);