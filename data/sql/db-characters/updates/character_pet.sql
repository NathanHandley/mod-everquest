SET @dbname = DATABASE();

DELIMITER //

CREATE PROCEDURE IF NOT EXISTS `update_character_pet_mc`()
BEGIN
    -- Columns
    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'character_pet' AND COLUMN_NAME = 'eq_owner') = 0 THEN
        ALTER TABLE `character_pet` ADD COLUMN `eq_owner` INT(10) UNSIGNED NOT NULL DEFAULT '0';
        SELECT 'Added eq_owner' AS status;
    ELSE
        SELECT 'eq_owner exists' AS status;
    END IF;

    IF (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'character_pet' AND COLUMN_NAME = 'eq_eqclass') = 0 THEN
        ALTER TABLE `character_pet` ADD COLUMN `eq_eqclass` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0';
        SELECT 'Added eq_eqclass' AS status;
    ELSE
        SELECT 'eq_eqclass exists' AS status;
    END IF;

    -- Indexes
    IF (SELECT COUNT(*) FROM information_schema.STATISTICS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'character_pet' AND INDEX_NAME = 'idx_eqowner') = 0 THEN
        ALTER TABLE `character_pet` ADD INDEX `idx_eqowner` (`eq_owner`);
        SELECT 'Added idx_eqowner' AS status;
    ELSE
        SELECT 'idx_eqowner exists' AS status;
    END IF;

    IF (SELECT COUNT(*) FROM information_schema.STATISTICS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'character_pet' AND INDEX_NAME = 'idx_eq_eqclass') = 0 THEN
        ALTER TABLE `character_pet` ADD INDEX `idx_eq_eqclass` (`eq_eqclass`);
        SELECT 'Added idx_eq_eqclass' AS status;
    ELSE
        SELECT 'idx_eq_eqclass exists' AS status;
    END IF;
END //

DELIMITER ;

CALL update_character_pet_mc();
DROP PROCEDURE IF EXISTS update_character_pet_mc;