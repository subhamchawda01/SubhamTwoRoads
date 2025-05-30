#a helper script which otputs a sed script which updates #events cutoff for datagen
#for US hours
#make minor changes for others like EU/AS

YYYYMMDD=20130122
START_UTC=1200
END_UTC=2000

cd ~/modelling/indicatorwork/prod_configs

ls -l comb_config_*US_MORN_DAY* | sed 's/.*config_// ; s/_na_e3.*//' | uniq | while read a ; do ~/basetrade_install/bin/get_l1events_on_day $a $YYYYMMDD $START_UTC $END_UTC | awk '{print $1" " $NF " " int ( $NF/(8*3600)) }'; done  | awk ' { if ( $3 == 0 )$3 = 1 ; print "sed -i \"s/\\(.*DATAGEN_TIMEOUT [0-9]*\\).*/\\1 "$3" 0/\""  " comb_config_"$1"_na_e3_US_MORN_DAY*" } '

