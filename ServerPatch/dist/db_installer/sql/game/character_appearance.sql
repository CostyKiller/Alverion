-- Alverion appearance extension: each character's appearance block, as the Alverion client sent it at creation.
CREATE TABLE IF NOT EXISTS `character_appearance` (
  `charId` INT UNSIGNED NOT NULL DEFAULT 0,
  `data` VARBINARY(64) NOT NULL,
  PRIMARY KEY (`charId`)
) DEFAULT CHARSET=latin1 COLLATE=latin1_general_ci;
