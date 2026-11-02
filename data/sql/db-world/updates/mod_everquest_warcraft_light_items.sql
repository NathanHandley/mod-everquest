-- EQWOW light source slot and Warcraft's own held light sources.
-- Held Warcraft items that burn or glow are carried light sources like EverQuest's (EverQuest_CarriedLight.cpp). LightType is
-- an EverQuest light type: 2 torch, 3 small magic glow, 4 small lantern, 6 large lantern, 7 flameless lantern.

CREATE TABLE IF NOT EXISTS `mod_everquest_warcraft_light_item` (
  `ItemTemplateID` int unsigned NOT NULL,
  `LightType` tinyint unsigned NOT NULL,
  PRIMARY KEY (`ItemTemplateID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DELETE FROM `mod_everquest_warcraft_light_item`;
INSERT INTO `mod_everquest_warcraft_light_item` (`ItemTemplateID`, `LightType`) VALUES
(812, 3),
(1172, 2),
(2410, 2),
(2808, 2),
(5240, 2),
(5323, 7),
(6182, 2),
(6341, 4),
(6654, 2),
(7344, 2),
(9393, 6),
(10515, 2),
(13004, 2),
(18523, 3),
(20694, 3),
(23844, 4),
(25091, 2),
(25098, 2),
(25291, 2),
(27540, 2),
(27714, 4),
(28187, 6),
(28387, 6),
(28412, 6),
(30857, 4),
(32332, 2),
(35652, 2),
(38248, 2),
(39118, 3),
(40395, 2),
(42796, 4),
(43893, 2),
(44214, 2);

-- The same line EverQuest's light sources carry (ItemTemplate.GetLightSourceDescriptionLine)
UPDATE `item_template` it
INNER JOIN `mod_everquest_warcraft_light_item` li ON li.`ItemTemplateID` = it.`entry`
SET it.`description` = IF(it.`description` = '', 'Light source: gives off light while equipped.', CONCAT(it.`description`, '|nLight source: gives off light while equipped.'))
WHERE it.`description` NOT LIKE '%Light source:%' AND CHAR_LENGTH(it.`description`) + 47 <= 255;

-- EverQuest's own lights with no equip slot and no backpack rule used to give no light on a player; the light source slot lets them
UPDATE `item_template` it
INNER JOIN `mod_everquest_item_template` li ON li.`ItemTemplateID` = it.`entry`
SET it.`description` = IF(it.`description` = '', 'Light source: gives off light in the light source slot.', CONCAT(it.`description`, '|nLight source: gives off light in the light source slot.'))
WHERE li.`LightType` > 0 AND it.`InventoryType` = 0 AND it.`description` NOT LIKE '%Light source:%' AND CHAR_LENGTH(it.`description`) + 57 <= 255;
