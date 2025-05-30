USE results;

DROP TABLE IF EXISTS wf_model_coeffs;
DROP TABLE IF EXISTS wf_strats;
DROP TABLE IF EXISTS wf_configs;

-- Table for maintaining the configs for structured strategies
CREATE TABLE wf_structured_configs (
st_id int unsigned NOT NULL AUTO_INCREMENT,
st_name varchar(512) NOT NULL UNIQUE,
shortcode VARCHAR(20) NOT NULL,
execlogic VARCHAR(100) NOT NULL,
start_time VARCHAR(20),
end_time VARCHAR(20),
strat_type varchar(50),
event_token varchar(50),
query_id int unsigned,
config_type int unsigned, 
config_json varchar(8192),
simula_approved tinyint(1),
type char(1),
description varchar(512),
tstamp timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
PRIMARY KEY st_id(st_id),
UNIQUE KEY st_name(st_name)
)ENGINE=InnoDB;

CREATE TABLE wf_model_coeffs (
  date int(10) unsigned NOT NULL,
  modelid int(10) unsigned NOT NULL,
  coeffs varchar(4096) NOT NULL,
  configid int(10) unsigned NOT NULL,
  PRIMARY KEY (configid,modelid,date),
  KEY date (date),
  KEY modelid (modelid),
  CONSTRAINT FOREIGN KEY (configid) REFERENCES wf_configs (configid) ON DELETE CASCADE,
  CONSTRAINT FOREIGN KEY (modelid) REFERENCES models (modelid)
)ENGINE=InnoDB;

CREATE TABLE wf_strats (
  configid int unsigned NOT NULL,
  date int unsigned NOT NULL,
  paramid int unsigned NOT NULL,
  modelid int unsigned NOT NULL,
  FOREIGN KEY (configid) REFERENCES wf_configs(configid) ON DELETE CASCADE,
  FOREIGN KEY (`modelid`, `configid`) REFERENCES `models` (`modelid`, `configid`) ON DELETE CASCADE,
  FOREIGN KEY (`paramid`, `configid`) REFERENCES `params` (`paramid`, `configid`) ON DELETE CASCADE,
  KEY configid(configid),
  KEY date(date),
  PRIMARY KEY (configid, date)
)ENGINE=InnoDB;


CREATE TABLE wf_configs(
  configid int unsigned NOT NULL AUTO_INCREMENT,
  cname varchar(512) NOT NULL UNIQUE,
  shortcode varchar(20) NOT NULL,
  execlogic varchar(100) NOT NULL, 
  start_time varchar(20),
  end_time varchar(20),
  sname varchar(512),
  strat_type varchar(50),
  event_token varchar(50),
  query_id int unsigned,
  config_type int unsigned,
  config_json varchar(4096),
  simula_approved tinyint(1),
  type char(1),
  description varchar(512),
  pooltag varchar(20) DEFAULT NULL,
  tstamp timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  expect0vol tinyint(1) NOT NULL DEFAULT '0',
  is_structured tinyint(1) NOT NULL DEFAULT 0, -- 0 for normal strats, 1 for structured , 2  for structured substrat
  structured_id int unsigned,
  PRIMARY KEY configid(configid),
  UNIQUE KEY cname(cname)
)ENGINE=InnoDB;


CREATE TABLE wf_results (
  date int unsigned NOT NULL,
  configid int unsigned NOT NULL,
  pnl int NOT NULL,
  vol int unsigned NOT NULL,
  supp_per int ,
  best_per int ,
  agg_per int ,
  imp_per int ,
  apos double ,
  median_ttc int ,  
  avg_ttc int ,
  med_closed_trd_pnl int ,
  avg_closed_trd_pnl int ,
  std_closed_trd_pnl int ,
  sharpe_closed_trade_pnls_ double ,
  fracpos_closed_trd_pnl double ,
  min_pnl int ,
  max_pnl int ,  
  drawdown int ,
  max_ttc int ,
  msg_count int ,
  vol_norm_avg_ttc int ,
  otl_hits int ,
  abs_open_pos int ,
  uts int ,
  ptrds int ,
  ttrds int ,
  pnl_samples varchar(1024),
  regenerate char(1) NOT NULL COMMENT 'to regenerate(Y) else (N) ...' , 
  PRIMARY KEY (configid, date),
  KEY configid (configid),
  KEY date (date),
  KEY regenerate (regenerate),
  FOREIGN KEY (configid) REFERENCES wf_configs(configid)
) ENGINE=InnoDB;


ALTER TABLE params ADD COLUMN configid INT(10) UNSIGNED,  ADD FOREIGN KEY configid(configid) REFERENCES wf_configs(configid) ON DELETE CASCADE ;

ALTER TABLE models ADD COLUMN configid INT(10) UNSIGNED,  ADD FOREIGN KEY configid(configid) REFERENCES wf_configs(configid) ON DELETE CASCADE ;
