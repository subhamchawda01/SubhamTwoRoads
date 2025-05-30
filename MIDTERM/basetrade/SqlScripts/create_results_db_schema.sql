USE results;

DROP TABLE IF EXISTS results;
DROP TABLE IF EXISTS wf_model_coeffs;
DROP TABLE IF EXISTS strats;
DROP TABLE IF EXISTS wf_strats;
DROP TABLE IF EXISTS models;
DROP TABLE IF EXISTS params;

CREATE TABLE params (
  paramid int unsigned NOT NULL AUTO_INCREMENT,
  paramfilename varchar(512) NOT NULL UNIQUE,
  last_update_date int,
  param_desc varchar(5000),
  PRIMARY KEY (paramid)
) ENGINE=InnoDB;

CREATE TABLE models (
  modelid int unsigned NOT NULL AUTO_INCREMENT,
  shortcode varchar(20) ,
  modelfilename varchar(512) NOT NULL UNIQUE,
  regression varchar(30),
  modelmath varchar(30),
  training_sd int,
  training_ed int,
  training_st varchar(20),
  training_et varchar(20),
  filter varchar(10),
  pred_dur double,
  pred_algo varchar(20),
  sample_timeouts varchar(20),
  stdev_or_l1norm double,
  change_or_return char(1) COMMENT 'change(C) or return(R) ...',
  last_update_date int,
  model_desc varchar(50000),
  PRIMARY KEY (modelid),
  KEY shortcode (shortcode)
) ENGINE=InnoDB;


CREATE TABLE strats (
  stratid int unsigned NOT NULL AUTO_INCREMENT ,
  shortcode varchar(20) NOT NULL,
  sname varchar(512) NOT NULL UNIQUE,
  execlogic varchar(50),
  modelid int unsigned NOT NULL,
  paramid int unsigned NOT NULL,
  start_time varchar(20),
  end_time varchar(20),
  type char(1) NOT NULL COMMENT 'normal(N) or staged(S) or pruned(P) ...',  
  descriptin varchar(512),
  simula_approved tinyint(1),
  PRIMARY KEY (stratid),
  KEY shortcode (shortcode),
  KEY type (type),
  FOREIGN KEY (paramid) REFERENCES params(paramid),
  FOREIGN KEY (modelid) REFERENCES models(modelid)
) ENGINE=InnoDB;

CREATE TABLE results (
  date int unsigned NOT NULL,
  stratid int unsigned NOT NULL,
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
  last_update timestamp,
  PRIMARY KEY (stratid, date),
  KEY stratid (stratid),
  KEY date (date),
  KEY regenerate (regenerate),
  FOREIGN KEY (stratid) REFERENCES strats(stratid)
) ENGINE=InnoDB;

CREATE TABLE SimRealPnlsMRT (
  query_id int unsigned NOT NULL,
  date int unsigned NOT NULL,
  sname varchar(512),
  real_pnl int,
  simreal_sim_pnl int,
  sim_pnl int, 
  PRIMARY KEY (query_id, date)
)


ALTER TABLE params ADD COLUMN configid INT(10) UNSIGNED,  ADD FOREIGN KEY configid(configid) REFERENCES wf_configs(configid) ON DELETE CASCADE ;

ALTER TABLE models ADD COLUMN configid INT(10) UNSIGNED,  ADD FOREIGN KEY configid(configid) REFERENCES wf_configs(configid) ON DELETE CASCADE ;
