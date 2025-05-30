/*table for data_copy/ res_gen is the db progres*/
CREATE TABLE status_codes (
 code INT(10) unsigned NOT NULL PRIMARY KEY CHECK (code>0),
 message VARCHAR(25) NOT NULL
);



CREATE TABLE progress (
 entry INT(50) unsigned AUTO_INCREMENT PRIMARY KEY,
 code INT(10) unsigned NOT NULL,
 which_loc  INT(20) unsigned NOT NULL,
 date_started DATE NOT NULL,
 active bool DEFAULT false,
 tstamp  TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
 CONSTRAINT foreign key (code) REFERENCES status_codes (code) 
 ON DELETE CASCADE 
 ON UPDATE CASCADE,
 CONSTRAINT foreign key(which_loc) REFERENCES location_info(loc_id) ON DELETE CASCADE ON UPDATE CASCADE
);


/*LOCATION EXCHANGE NAS_FOLDER(without_LoggedData_Suffix) IP*/

create Table location_info(
loc_id INT(20) unsigned AUTO_INCREMENT  PRIMARY KEY,
source VARCHAR(25) NOT NULL,
EXCHANGE VARCHAR(25) NOT NULL,
NAS_FOLDER VARCHAR(25) NOT NULL,
dest VARCHAR(15) NOT NULL
);


