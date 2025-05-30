
CREATE TABLE wf_config_story(
  configid int(10) unsigned DEFAULT NULL,
  pnl_metrics varchar(200) DEFAULT NULL,
  feature_metrics varchar(6000) DEFAULT NULL,
  KEY configid (configid),
  FOREIGN KEY (configid) REFERENCES wf_configs (configid) ON DELETE CASCADE
)ENGINE=InnoDB;


CREATE TABLE wf_config_correlation (
  configid1 int(10) unsigned NOT NULL,
  configid2 int(10) unsigned NOT NULL,
  correlation float(4,2) unsigned NOT NULL,
  last_updated timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (configid1,configid2),
  KEY configid2 (configid2),
  FOREIGN KEY (configid1) REFERENCES wf_configs (configid) ON DELETE CASCADE,
  FOREIGN KEY (configid2) REFERENCES wf_configs (configid) ON DELETE CASCADE,
  FOREIGN KEY (configid1) REFERENCES wf_configs (configid) ON DELETE CASCADE,
  FOREIGN KEY (configid2) REFERENCES wf_configs (configid) ON DELETE CASCADE
)ENGINE=InnoDB;



CREATE TABLE StratCorrelation (
  stratid1 int(10) unsigned NOT NULL,
  stratid2 int(10) unsigned NOT NULL,
  correlation float(4,2) unsigned NOT NULL,
  last_updated timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (stratid1,stratid2),
  KEY stratid2 (stratid2),
  FOREIGN KEY (stratid1) REFERENCES strats (stratid) ON DELETE CASCADE,
  FOREIGN KEY (stratid2) REFERENCES strats (stratid) ON DELETE CASCADE,
  FOREIGN KEY (stratid1) REFERENCES strats (stratid) ON DELETE CASCADE,
  FOREIGN KEY (stratid2) REFERENCES strats (stratid) ON DELETE CASCADE
)ENGINE=InnoDB;


CREATE TABLE StratStory2 (
  stratid int(10) unsigned DEFAULT NULL,
  pnl_metrics varchar(200) DEFAULT NULL,
  feature_metrics varchar(6000) DEFAULT NULL,
  KEY stratid (stratid),
  FOREIGN KEY (stratid) REFERENCES strats (stratid) ON DELETE CASCADE
)ENGINE=InnoDB;


