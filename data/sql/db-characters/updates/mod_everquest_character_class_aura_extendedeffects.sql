-- A spell can carry up to 12 effects (SpellExtendedEffects.dbc), so the stashed auras of a secondary class need the wider effect masks and the amounts of
-- effects 3 to 11 that character_aura has.
-- Written as plain top-level statements (no DELIMITER / stored procedure) so it runs the same way pasted into a SQL window as it does through the mysql client.

SET @dbname = DATABASE();

SET @amount3Exists = (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = @dbname AND TABLE_NAME = 'mod_everquest_character_class_aura' AND COLUMN_NAME = 'amount3');

SET @addExtendedEffects = IF(@amount3Exists = 0,
	'ALTER TABLE `mod_everquest_character_class_aura` MODIFY COLUMN `effectMask` SMALLINT(5) UNSIGNED NOT NULL DEFAULT ''0'', MODIFY COLUMN `recalculateMask` SMALLINT(5) UNSIGNED NOT NULL DEFAULT ''0'', ADD COLUMN `amount3` INT(10) NOT NULL DEFAULT ''0'', ADD COLUMN `amount4` INT(10) NOT NULL DEFAULT ''0'', ADD COLUMN `amount5` INT(10) NOT NULL DEFAULT ''0'', ADD COLUMN `amount6` INT(10) NOT NULL DEFAULT ''0'', ADD COLUMN `amount7` INT(10) NOT NULL DEFAULT ''0'', ADD COLUMN `amount8` INT(10) NOT NULL DEFAULT ''0'', ADD COLUMN `amount9` INT(10) NOT NULL DEFAULT ''0'', ADD COLUMN `amount10` INT(10) NOT NULL DEFAULT ''0'', ADD COLUMN `amount11` INT(10) NOT NULL DEFAULT ''0'', ADD COLUMN `base_amount3` INT(10) NOT NULL DEFAULT ''0'', ADD COLUMN `base_amount4` INT(10) NOT NULL DEFAULT ''0'', ADD COLUMN `base_amount5` INT(10) NOT NULL DEFAULT ''0'', ADD COLUMN `base_amount6` INT(10) NOT NULL DEFAULT ''0'', ADD COLUMN `base_amount7` INT(10) NOT NULL DEFAULT ''0'', ADD COLUMN `base_amount8` INT(10) NOT NULL DEFAULT ''0'', ADD COLUMN `base_amount9` INT(10) NOT NULL DEFAULT ''0'', ADD COLUMN `base_amount10` INT(10) NOT NULL DEFAULT ''0'', ADD COLUMN `base_amount11` INT(10) NOT NULL DEFAULT ''0''',
	'SELECT ''extended effect columns exist'' AS status');

PREPARE addExtendedEffectsStatement FROM @addExtendedEffects;
EXECUTE addExtendedEffectsStatement;
DEALLOCATE PREPARE addExtendedEffectsStatement;
