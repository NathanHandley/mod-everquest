-- Beastlord (EQ class 15) came with Shadows of Luclin, so the adventurer loss snapshot gets a level column for it
SET @dbname = DATABASE();

DELIMITER //

CREATE PROCEDURE IF NOT EXISTS `update_mod_everquest_character_adventurer_loss_beastlord`()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_adventurer_loss' AND COLUMN_NAME = 'levelBeastlord') = 0 THEN
        ALTER TABLE `mod_everquest_character_adventurer_loss` ADD COLUMN `levelBeastlord` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' AFTER `levelEnchanter`;
        SELECT 'Added levelBeastlord' AS status;
    ELSE
        SELECT 'levelBeastlord exists' AS status;
    END IF;
END //

DELIMITER ;

CALL update_mod_everquest_character_adventurer_loss_beastlord();
DROP PROCEDURE IF EXISTS update_mod_everquest_character_adventurer_loss_beastlord;
