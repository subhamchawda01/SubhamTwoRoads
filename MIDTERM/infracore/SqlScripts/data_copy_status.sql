/*table for data_copy/ res_gen is the db progres*/
CREATE TABLE status_codes (
 code INT(10)  NOT NULL PRIMARY KEY,
 message VARCHAR(25) NOT NULL
);



CREATE TABLE progress (
 entry INT(50) unsigned AUTO_INCREMENT UNIQUE ,
 code INT(10) unsigned NOT NULL,
 who VARCHAR(15) NOT NULL,
 which_loc  INT(20) unsigned NOT NULL ,
 date_for DATE NOT NULL,
 active bool DEFAULT true,
 des_ip VARCHAR(15) NOT NULL,
 tstamp  TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
 CONSTRAINT foreign key (code) REFERENCES status_codes (code) 
 ON DELETE CASCADE 
 ON UPDATE CASCADE,
 CONSTRAINT foreign key(which_loc) REFERENCES location_info(loc_id) ON DELETE CASCADE ON UPDATE CASCADE,
 PRIMARY KEY(which_loc, date_for)
);



CREATE TABLE history (
 entry INT(50) unsigned AUTO_INCREMENT UNIQUE PRIMARY KEY ,
 code INT(10) unsigned NOT NULL,
 who VARCHAR(15) NOT NULL,
 which_loc  INT(20) unsigned NOT NULL ,
 date_for DATE NOT NULL,
 active bool DEFAULT true,
 des_ip VARCHAR(15) NOT NULL,
 tstamp  TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
 CONSTRAINT foreign key (code) REFERENCES status_codes (code) 
 ON DELETE CASCADE 
 ON UPDATE CASCADE,
 CONSTRAINT foreign key(which_loc) REFERENCES location_info(loc_id) ON DELETE CASCADE ON UPDATE CASCADE
 
);

/*LOAD DATA LOCAL INFILE 'data_copy_timings_utc.txt' INTO TABLE location_timings FIELDS TERMINATED BY ' ' LINES TERMINATED BY '\n' (location,time_UTC,sub_tag);*/

create Table location_timings(
location VARCHAR(25) NOT NULL,
time_UTC INT(6) NOT NULL,
sub_tag VARCHAR(10) NOT NULL,
PRIMARY KEY(location, sub_tag)
);

/*LOCATION EXCHANGE NAS_FOLDER(without_LoggedData_Suffix) IP*/

create Table location_info(
loc_id INT(20) unsigned AUTO_INCREMENT Unique not null,
location VARCHAR(25) NOT NULL,
EXCHANGE VARCHAR(25) NOT NULL,
NAS_FOLDER VARCHAR(25) NOT NULL,
src_ip VARCHAR(15) NOT NULL,
PRIMARY KEY (source,EXCHANGE,NAS_FOLDER)
);

/*LOAD DATA LOCAL INFILE 'nse.cfg' INTO TABLE location_info FIELDS TERMINATED BY ',' LINES TERMINATED BY '\n' (location,EXCHANGE,NAS_FOLDER,src_ip);
 */


create Table dc_filecount_state(
 which_loc  INT(20) unsigned NOT NULL ,
 date_for DATE NOT NULL,
 NAS_COUNT INT(20) unsigned,
 HS1_COUNT INT(20) unsigned,
 S3_COUNT INT(20) unsigned,
 PROD_COUNT INT(20) unsigned,
 matched bool DEFAULT true,
 logs VARCHAR(250) NOT NULL,
 PRIMARY KEY(which_loc, date_for),
 CONSTRAINT foreign key(which_loc) REFERENCES location_info(loc_id) 
);

