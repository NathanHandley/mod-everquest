SET @dbname = DATABASE();

DELIMITER //

CREATE PROCEDURE IF NOT EXISTS `update_mod_everquest_character_settings_mentorship`()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'mentorshipRole') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `mentorshipRole` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT '0 none, 1 mentor, 2 apprentice. Non zero only while this character is standing at a borrowed mentorship level';
        SELECT 'Added mentorshipRole' AS status;
    ELSE
        SELECT 'mentorshipRole exists' AS status;
    END IF;

    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'mentorshipRealLevel') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `mentorshipRealLevel` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'The level to put this character back to if the mentorship never ends cleanly';
        SELECT 'Added mentorshipRealLevel' AS status;
    ELSE
        SELECT 'mentorshipRealLevel exists' AS status;
    END IF;

    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'mentorshipRealExp') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `mentorshipRealExp` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'The experience bar to put back alongside mentorshipRealLevel';
        SELECT 'Added mentorshipRealExp' AS status;
    ELSE
        SELECT 'mentorshipRealExp exists' AS status;
    END IF;

    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'mentorshipBankedProgress') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `mentorshipBankedProgress` FLOAT NOT NULL DEFAULT '0' COMMENT 'Levels an apprentice has earned toward their real level so far, so a crash costs at most the last few seconds of it';
        SELECT 'Added mentorshipBankedProgress' AS status;
    ELSE
        SELECT 'mentorshipBankedProgress exists' AS status;
    END IF;
END //

DELIMITER ;

CALL update_mod_everquest_character_settings_mentorship();
DROP PROCEDURE IF EXISTS update_mod_everquest_character_settings_mentorship;
