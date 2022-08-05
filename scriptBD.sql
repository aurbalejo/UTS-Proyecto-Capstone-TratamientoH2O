CREATE DATABASE `iot` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci */ /*!80016 DEFAULT ENCRYPTION='N' */;
USE iot;
CREATE TABLE `eventossolid` (
  `ideventos` int NOT NULL AUTO_INCREMENT,
  `fechaevento` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `valvulasolid` tinyint(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`ideventos`)
) ENGINE=InnoDB AUTO_INCREMENT=57 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
CREATE TABLE `lecturasensorppm` (
  `idlecturasensores` int NOT NULL AUTO_INCREMENT,
  `fechalectura` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `lecturappm` float NOT NULL,
  PRIMARY KEY (`idlecturasensores`)
) ENGINE=InnoDB AUTO_INCREMENT=5047 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
CREATE TABLE `lecturasensortemp` (
  `idlecturasensortemp` int NOT NULL AUTO_INCREMENT,
  `fechalectura` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `lecturatemp` float NOT NULL,
  PRIMARY KEY (`idlecturasensortemp`)
) ENGINE=InnoDB AUTO_INCREMENT=848 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
CREATE TABLE `eventostratada` (
  `ideventostratada` int NOT NULL AUTO_INCREMENT,
  `fechaevento` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `valvulatratada` tinyint(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`ideventostratada`)
) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
