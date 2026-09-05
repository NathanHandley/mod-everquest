SET @dbname = DATABASE();

DELIMITER //

CREATE PROCEDURE IF NOT EXISTS `update_mod_everquest_character_settings_druidforms`()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'druidFormBear') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `druidFormBear` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT '0 keeps whatever the core picks for this character (which is per race and per hair or skin color), 1 alliance bear, 2 horde bear, 3 Norrath grizzly';
        SELECT 'Added druidFormBear' AS status;
    ELSE
        SELECT 'druidFormBear exists' AS status;
    END IF;

    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'druidFormCat') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `druidFormCat` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT '0 keeps whatever the core picks for this character, 1 alliance cat, 2 horde cat, 3 Norrath panther, 4 Norrath sabertooth';
        SELECT 'Added druidFormCat' AS status;
    ELSE
        SELECT 'druidFormCat exists' AS status;
    END IF;

    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'druidFormTravel') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `druidFormTravel` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT '0 Azeroth cheetah, 1 Norrath leopard';
        SELECT 'Added druidFormTravel' AS status;
    ELSE
        SELECT 'druidFormTravel exists' AS status;
    END IF;

    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'druidFormTree') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `druidFormTree` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT '0 Azeroth treant, 1 Norrath treant';
        SELECT 'Added druidFormTree' AS status;
    ELSE
        SELECT 'druidFormTree exists' AS status;
    END IF;

    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_settings' AND COLUMN_NAME = 'druidFormMoonkin') = 0 THEN
        ALTER TABLE `mod_everquest_character_settings` ADD COLUMN `druidFormMoonkin` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT '0 the moonkin look is shown, 1 no form graphic at all and the character keeps their own look';
        SELECT 'Added druidFormMoonkin' AS status;
    ELSE
        SELECT 'druidFormMoonkin exists' AS status;
    END IF;

END //

DELIMITER ;

CALL update_mod_everquest_character_settings_druidforms();
DROP PROCEDURE IF EXISTS update_mod_everquest_character_settings_druidforms;
