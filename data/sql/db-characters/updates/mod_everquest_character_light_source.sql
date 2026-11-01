-- The light source a character picked from the character sheet, while it stays in effect
CREATE TABLE IF NOT EXISTS `mod_everquest_character_light_source` (
  `guid` int unsigned NOT NULL,
  `itemEntry` int unsigned NOT NULL,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
