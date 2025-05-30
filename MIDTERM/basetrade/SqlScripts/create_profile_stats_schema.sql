USE profilestats;

DROP TABLE IF EXISTS summary;
DROP TABLE IF EXISTS t2t_data;
DROP TABLE IF EXISTS dpi_data;


-- Create the tables

-- Table containing unique id for each of the cpucycle-profiler tags we are trying to log,
-- We can add items to it later.
CREATE TABLE summary(
summary_id INT unsigned NOT NULL,
description VARCHAR(100),
PRIMARY KEY(summary_id),
UNIQUE KEY (description)
)ENGINE=InnoDB;


-- Data to be logged for each of the summary tag
-- For now we are adding columns which are already logged
-- We will keep adding new columns as required
CREATE TABLE t2t_data(
did INT unsigned NOT NULL AUTO_INCREMENT, -- data id
query_id INT unsigned NOT NULL,
saci INT unsigned,
mean DOUBLE,
median DOUBLE,
twenty_five DOUBLE,
seventy_five DOUBLE,
ninty DOUBLE,
ninty_five DOUBLE,
ninty_nine DOUBLE,
max_val DOUBLE,
min_val DOUBLE,
total_count DOUBLE,
summary_id INT unsigned,
tradingdate INT unsigned,
FOREIGN KEY (summary_id) REFERENCES summary(summary_id) ON DELETE CASCADE, -- cascade required
PRIMARY KEY (did)
)ENGINE=InnoDB;


-- Table to maintain daily data for DPI

CREATE TABLE dpi_data(
query_id INT unsigned NOT NULL,
saci INT unsigned,
tradingdate INT unsigned,
shortcode VARCHAR(50), -- Having shortcode, exch, sess tags to easily club
exchange VARCHAR(20),
session VARCHAR(10),
send_queue DOUBLE,
send_weight DOUBLE,
cancel_queue DOUBLE,
cancel_weight DOUBLE,
t2t_stats DOUBLE,
t2t_stats_weight DOUBLE,

PRIMARY KEY (query_id)
)ENGINE=InnoDB;
