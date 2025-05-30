#!/bin/bash
inst_file=/spare/local/files/CMEMDP/cme_instrument_dict.txt
temp_inst_file=/tmp/cme_instrument_dict.txt
cur_date=`date +%Y%m%d`
mod_date=`date +%Y%m%d -r $inst_file`
if [ ! $cur_date == $mod_date ]
then
  /bin/mail -s "FPGA config file: $inst_file not updated today on CHI15" -r "nseall@tworoads.co.in" "nseall@tworoads.co.in"
  exit 0
fi

scp ivan@10.23.82.100:/srv/ivan/application-configurations/cmemdp3-prod-*/cme_instrument_dict.txt $temp_inst_file
cmp $temp_inst_file $inst_file || /bin/mail -s "FPGA config file: $inst_file not consistent on CHI15" -r "nseall@tworoads.co.in" "nseall@tworoads.co.in"
