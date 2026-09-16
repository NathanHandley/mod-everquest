-- Permanent enchantments owed back to an item version that a slotshift or a transform/combine destroyed.
-- A slotshift ring member and a tradeskill "transform" (click the container, the old item is eaten as a reagent) both
-- DESTROY the old item and CREATE a new one, so the enchantment on the old version would otherwise die with it.  A row
-- here says "this character is owed enchantId the next time a transform hands them itemEntry again", which is what makes
-- an enchantment stick to the form it was applied to instead of following the item through the ring.
-- Memory is per character and per item entry rather than per item guid on purpose: a combine consumes its reagents with
-- DestroyItemCount(entry, count), which picks whichever copy it likes, so a per-guid chain cannot survive that path.
-- `count` lets one character hold several copies of the same entry+enchantment without needing a surrogate key.
-- Re-runnable on purpose: never DROP, so an existing deployment keeps what it has remembered.
CREATE TABLE IF NOT EXISTS `mod_everquest_character_item_enchant_memory` (
	`guid` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Character Global Unique Identifier',
	`itemEntry` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'item_template entry the enchantment is owed back to',
	`enchantId` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'SpellItemEnchantment id that was in the permanent slot',
	`count` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'How many copies of this entry+enchantment are owed back',
	PRIMARY KEY (`guid`, `itemEntry`, `enchantId`) USING BTREE,
	INDEX `idx_guid` (`guid`) USING BTREE
);
