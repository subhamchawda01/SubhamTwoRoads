create table shortcodes(
shc_id int(10) unsigned NOT NULL AUTO_INCREMENT,
shortcode varbinary(20) NOT NULL,
PRIMARY KEY (shc_id),
KEY shortcode (shortcode)
) ENGINE=InnoDB;

create table times(
tp_id int(10) unsigned  NOT NULL AUTO_INCREMENT,
time varchar(20) NOT NULL,
PRIMARY KEY (tp_id),
KEY time(time)
)ENGINE=InnoDB;

create table products(
prod_id int(10) unsigned NOT NULL AUTO_INCREMENT,
shc_id int(10) unsigned NOT NULL,
start_time_id int(10) unsigned NOT NULL,
end_time_id int(10) unsigned NOT NULL,
PRIMARY KEY (prod_id),
FOREIGN KEY (shc_id) REFERENCES shortcodes(shc_id) ON DELETE CASCADE,
FOREIGN KEY (start_time_id) REFERENCES times(tp_id) ON DELETE CASCADE,
FOREIGN KEY (end_time_id) REFERENCES times(tp_id) ON DELETE CASCADE
)ENGINE=InnoDB;

create table indicators(
ind_id int(10) unsigned NOT NULL AUTO_INCREMENT,
indicator varchar(1000) NOT NULL,
PRIMARY KEY (ind_id),
KEY indicator(indicator)
)ENGINE=InnoDB;

create table prod_indicators(
prod_ind_id int(10) unsigned NOT NULL AUTO_INCREMENT,
prod_id int(10) unsigned NOT NULL,
ind_id int(10) unsigned NOT NULL,
PRIMARY KEY (prod_ind_id),
FOREIGN KEY (ind_id) REFERENCES indicators(ind_id) ON DELETE CASCADE,
FOREIGN KEY (prod_id) REFERENCES products(prod_id) ON DELETE CASCADE
)ENGINE=InnoDB;

create table self_terms(
prod_ind_id int(10) unsigned NOT NULL,
date int(10) unsigned NOT NULL,
mean varchar(15) NOT NULL,
mean_squared varchar(15) NOT NULL,
num_rows int(10) unsigned NOT NULL,
FOREIGN KEY (prod_ind_id) REFERENCES prod_indicators(prod_ind_id) ON DELETE CASCADE,
KEY date(date)
)ENGINE=InnoDB;

create table cross_terms(
prod_ind_id1 int(10) unsigned NOT NULL,
prod_ind_id2 int(10) unsigned NOT NULL,
date int(10) unsigned NOT NULL,
cross_term varchar(15) NOT NULL,
FOREIGN KEY (prod_ind_id1) REFERENCES prod_indicators(prod_ind_id) ON DELETE CASCADE,
FOREIGN KEY (prod_ind_id2) REFERENCES prod_indicators(prod_ind_id) ON DELETE CASCADE,
KEY date(date)
)ENGINE=InnoDB;