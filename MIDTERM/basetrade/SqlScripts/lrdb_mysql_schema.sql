CREATE TABLE session_timings (
 session_id INT(50) unsigned  PRIMARY KEY AUTO_INCREMENT,
 exchange VARCHAR(25) NOT NULL ,
 start_time VARCHAR(25) NOT NULL,
 end_time VARCHAR(25) NOT NULL

);



CREATE TABLE LRDB_Pair_Timings (
 dep VARCHAR(25) NOT NULL,
 indep VARCHAR(25) NOT NULL,
 start_time VARCHAR(25) ,
 end_time VARCHAR(25) ,	
 session_id INT(50) unsigned ,
 foreign key(session_id) REFERENCES session_timings(session_id),
 PRIMARY KEY(dep, indep,session_id)
);




CREATE TABLE LRDB_dated (
  dep VARCHAR(25) NOT NULL,
 indep VARCHAR(25) NOT NULL,
 session_id INT(50) unsigned NOT NULL,
 date_generated DATE NOT NULL,
 beta DECIMAL(20,10) ,
 correlation DECIMAL(5,4) ,
 tstamp  TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
 dirty_bit bool DEFAULT false,
 foreign key(session_id) REFERENCES session_timings(session_id),
 PRIMARY KEY(dep, indep, session_id, date_generated),
 CONSTRAINT chk_corr CHECK (correlation>=-1 AND correlation<=1 )
);

CREATE TABLE Ret_LRDB_dated (
  dep VARCHAR(25) NOT NULL,
 indep VARCHAR(25) NOT NULL,
 session_id INT(50) unsigned NOT NULL,
 date_generated DATE NOT NULL,
 beta DECIMAL(20,10) ,
 correlation DECIMAL(5,4) ,
 tstamp  TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
 dirty_bit bool DEFAULT false,
 foreign key(session_id) REFERENCES session_timings(session_id),
 PRIMARY KEY(dep, indep, session_id, date_generated),
 CONSTRAINT chk_corr CHECK (correlation>=-1 AND correlation<=1 )
);


/*LOAD DATA LOCAL INFILE 'session_time.csv' INTO TABLE session_timings FIELDS TERMINATED BY ',' LINES TERMINATED BY '\n' IGNORE 1 LINES;*/
/*CREATE TABLE tradingInfo (
 shortcode VARCHAR(255) PRIMARY KEY,
 exchange VARCHAR(255) ,
 open  VARCHAR(255) Not Null ,
 close  VARCHAR(255) Not Null ,
 TimeZone VARCHAR(255) Not Null ,
 break_start VARCHAR(255) Not Null,
 break_end VARCHAR(255) Not Null,
);*/

