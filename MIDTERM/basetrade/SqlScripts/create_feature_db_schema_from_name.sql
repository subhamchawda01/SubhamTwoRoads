USE results;

DROP TABLE IF EXISTS RollingAvgL1Size;

CREATE TABLE RollingAvgL1Size (
  date int unsigned NOT NULL,
  shcid int unsigned NOT NULL,
  tz_id int unsigned NOT NULL,
  mean double NOT NULL,
  median double NOT NULL,
  stdev double NOT NULL,
  mean_hquart double NOT NULL,
  mean_lquart double NOT NULL,
  duration_sec double NOT NULL,
  regenerate char(1) NOT NULL COMMENT 'to regenerate(Y) else (N) ...' , 
  PRIMARY KEY (shcid, date, tz_id, duration_sec),
  KEY duration_sec (duration_sec),
  KEY tz_id (tz_id),
  KEY date (date),
  FOREIGN KEY (shcid) REFERENCES ShortCodes(shcid),
  FOREIGN KEY (tz_id) REFERENCES TimeZones(tz_id)
) ENGINE=InnoDB;
