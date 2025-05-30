USE results;

DROP TABLE IF EXISTS TimeZones;
DROP TABLE IF EXISTS TimeZonesStr;
DROP TABLE IF EXISTS ShortCodes;

CREATE TABLE ShortCodes (
  shortcode varchar(40) NOT NULL UNIQUE,
  shcid int unsigned NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (shcid)
) ENGINE=InnoDB;

CREATE TABLE TimeZonesStr (
  tz_str varchar(10) NOT NULL UNIQUE,
  tz_id int unsigned NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (tz_id)
) ENGINE=InnoDB;


CREATE TABLE TimeZones (
  tz_id int unsigned NOT NULL,
  shcid int unsigned NOT NULL,
  start_time varchar(20) NOT NULL,
  end_time varchar(20) NOT NULL,
  PRIMARY KEY (shcid, tz_id),
  FOREIGN KEY (tz_id) REFERENCES TimeZonesStr(tz_id),
  FOREIGN KEY (shcid) REFERENCES ShortCodes(shcid),
  CONSTRAINT uc_tz UNIQUE (shcid, start_time, end_time)
) ENGINE=InnoDB;


