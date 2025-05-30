bse_fetch_dir='/tmp/bse_fetch'

YYYYMMDD=$1 ; 
DD=${YYYYMMDD:6:2} 
MM=${YYYYMMDD:4:2} 
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4}

next_working_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD N W`

DN=${next_working_day:6:2} 

sync_ips=("10.23.5.66" "10.23.5.50" "10.23.5.22" "192.168.132.11" "192.168.132.12")
 
FTP_DIR="/spare/local/files/BSEFTPFiles/$YYYY/$MM/$DD"

fetch_files=${bse_fetch_dir}/fetch_files

>${fetch_files}

# echo ExposureFiles/EQ${DD}${MM}${YY}.csv >> $fetch_files
# echo SpanFiles/BSERISK${YYYY}${MM}${DD}-FINAL.spn >> $fetch_files
# echo VAR_FILES/VarFiles${DD}${MM}${YYYY}.xls >> $fetch_files
# echo VAR_FILES/VarFile${YYYY}${MM}${DD}.csv >> $fetch_files
# echo cds/${MM}${YY}/CurrencyBhavCopy_${YYYY}${MM}${DD}.csv >> $fetch_files
# echo cds/${MM}${YY}/${DD}${MM}cd_0000.md >> $fetch_files
# echo fo/${MM}${YY}/bhavcopy${DD}-${MM}-${YY}.csv >> $fetch_files
# echo fo/${MM}${YY}/${DD}${MM}fo_0000.md >> $fetch_files
# echo onlyBseEQ/onlyBseBhavcopy${YYYY}${MM}${DD}.csv >> $fetch_files
# echo fo_bse_loy_${YYYY}${MM}${DN} >> $fetch_files
# echo EQ_PRICE_LESS_100_${YYYY}${MM}${DD} >> $fetch_files
# echo bse_eq_${YYYY}${MM}${DN}_contracts.txt >> $fetch_files
# echo bse_fo_${YYYY}${MM}${DN}_contracts.txt >> $fetch_files
# echo bse_cd_${YYYY}${MM}${DN}_contracts.txt >> $fetch_files
# echo bse_groupAB_symbol_bhavcopy_${YYYY}${MM}${DN} >> $fetch_files
# echo BannedScrips-${DD}${MM}${YYYY}.csv >> $fetch_files
# echo fo_secban_${YYYY}${MM}${DN}.csv >> $fetch_files
# echo security_margin_${YYYY}${MM}${DN}.txt >> $fetch_files

# Functions to rsync files.
sync_ftp_file () {
  echo "sync file: $1"

  file=$1  
  for ip in "${sync_ips[@]}";
  do
    rsync -ravz $file $ip:$FTP_DIR/
    echo "sync done for $ip"
  done
  echo "sync completed for $1"
}

sync_tradeinfo_file () {
  echo "sync file: $1"

  file=$1  
  for ip in "${sync_ips[@]}";
  do
    rsync -ravz $file $ip:/spare/local/tradeinfo/BSE_Files/
    echo "sync done for $ip"
  done
  echo "sync completed for $1"
}

ftp_sync_cm_scrip () {
  sync_ftp_file $FTP_DIR/SCRIP
}

ftp_sync_cd_scrip () {
  sync_ftp_file $FTP_DIR/BFX_CC_CO${DD}-${MM}-${YY}.csv
  sync_ftp_file $FTP_DIR/BFX_CO${DD}-${MM}-${YY}.csv
  sync_ftp_file $FTP_DIR/BFX_SPD_CO${DD}-${MM}-${YY}.csv
  sync_ftp_file $FTP_DIR/BFX_DP${DD}-${MM}-${YY}
  sync_ftp_file $FTP_DIR/BSE_BFX_CONTRACT_${DD}-${MM}-${YY}.csv
  sync_ftp_file $FTP_DIR/BSE_BFX_SPDCONTRACT_${DD}-${MM}-${YY}.csv
}

ftp_sync_fo_scrip () {
  sync_ftp_file $FTP_DIR/EQD_DP${DD}${MM}${YY}
  sync_ftp_file $FTP_DIR/EQD_CO${DD}${MM}${YY}.csv
  sync_ftp_file $FTP_DIR/EQD_SPD_CO${DD}${MM}${YY}.csv
  sync_ftp_file $FTP_DIR/BSE_EQD_CONTRACT_${DD}${MM}${YY}.csv
  sync_ftp_file $FTP_DIR/BSE_EQD_SPDCONTRACT_${DD}${MM}${YY}.csv

}

ftp_sync_fo_bhavcopy () {
  sync_ftp_file ${FTP_DIR}/bhavcopy${DD}-${MM}-${YY}.csv
}

# Functions to sync tradeinfo files.
tradeinfo_sync_secban() {
  sync_tradeinfo_file /spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/BannedScrips-${DD}${MM}${YYYY}.csv
  sync_tradeinfo_file /spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/fo_secban_${next_working_day}.csv
}

# Functions for downloading files.
fetch_cm_scrip () {
  file_downloaded=0

  echo "==================== CM SCRIP ===================="

  rm scrip.zip
  wget -t 10 --timeout=20 --referer https://www.bseindia.com --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.bseindia.com/downloads/Help/file/scrip.zip
  
  # if failed to download then exit.
  [ ! -f scrip.zip ] && return
  
  echo "**** CM SCRIP DOWNLOADED ****"

  #TODO check files based on file update date FO<CM<CD
  unzip scrip.zip
  mv SCRIP $FTP_DIR/
  chown -R dvctrader:infra $FTP_DIR/SCRIP
  rm scrip.zip

  # sync to other machines.
  # sync_ftp_file $FTP_DIR/SCRIP

  # Files successfully downloaded.
  file_downloaded=1

  # Generate Ref data using CM SCRIP Files.
  cat $FTP_DIR/SCRIP/SCRIP_${DD}${MM}${YY}.TXT | grep "|EQ|" | awk -F"|" '{sub(" ", "_"); print $1,0,"STK",$3,0,0,"XX","STK",0,0}'  > /spare/local/tradeinfo/BSE_Files/RefData/bse_eq_"$next_working_day"_contracts.txt

  echo "**** /RefData/bse_eq_next_working_day_contracts.txt generated ****"

  chgrp infra /spare/local/tradeinfo/BSE_Files/RefData/bse_eq_${next_working_day}_contracts.txt
  
  fetch_cm_bhavcopy ;

  if [ $file_downloaded = "0" ] ; then
    echo "fetch_cm_bhavcopy FAILED" >> $files_missing
    return
  fi

  # Generate groupAB product file.
  echo "Updating groupAB product file"
  groupAB_file="/spare/local/tradeinfo/BSE_Files/RefData/bse_groupAB_symbol_bhavcopy_${next_working_day}"
  >$groupAB_file
  
  while IFS= read -r line;
  do
    sc_group=`echo "$line"| awk -F, '{print $5}'`
    if [ $sc_group = 'A' ] || [ $sc_group = 'B' ]; then
      stk_code=`echo "$line" | awk -F, '{print $1}'`
      sc_symbol=`grep $stk_code "/spare/local/tradeinfo/BSE_Files/RefData/bse_eq_${next_working_day}_contracts.txt" | awk '{print $4}'`
      if [ ! -z "$sc_symbol" ]; then
         echo  "$sc_symbol, $line" >>"$groupAB_file"
      fi
    fi
  done <"${FTP_DIR}/SCRIP/REG_IND${DD}${MM}${YY}.csv"

  echo "**** /RefData/bse_groupAB_symbol_bhavcopy_next_working_day generated using SCRIP_DD_MM_YY ****"

  echo "==================== CM SCRIP DONE ===================="
}

## ===DONE===
fetch_fo_scrip () {
  echo
  echo "==================== FO SCRIP ===================="
  rm CO.zip
  wget -t 2 --timeout=20 --referer https://www.bseindia.com --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0"  https://www.bseindia.com/downloads/Help/file/CO.zip

  # if failed to download then exit.
  [ ! -f CO.zip ] && return

  unzip CO.zip -d $FTP_DIR/
  rm CO.zip

  # Files successfully downloaded.
  file_downloaded=1

  # Generate Ref data using FO SCRIP Files.
  awk -F',' 'NR==FNR{a[$1]=$6;b[$1]=$7; next} ($1 in a) {x=$9;if($9== ""){x="XX"};print $1,$2,$3,$4,$7,$8,x,$54,a[$1],b[$1]}' $FTP_DIR/EQD_DP${DD}${MM}${YY} $FTP_DIR/EQD_CO${DD}${MM}${YY}.csv > /spare/local/tradeinfo/BSE_Files/RefData/bse_fo_$next_working_day"_contracts.txt"

  echo "**** /RefData/bse_fo_next_working_day_contracts.txt generated ****"
  
  chgrp infra /spare/local/tradeinfo/BSE_Files/RefData/bse_fo_${next_working_day}_contracts.txt

  # Generate Lot size files using FO SCRIP Files.
  echo "Updating LOT file"
  
  grep FUT $FTP_DIR/EQD_CO${DD}${MM}${YY}.csv | cut -d',' -f1,5,7,31 > /spare/local/tradeinfo/BSE_Files/Lotsizes/fo_bse_lot_$next_working_day

  chgrp infra /spare/local/tradeinfo/BSE_Files/Lotsizes/fo_bse_lot_$next_working_day

  echo "**** Lotsizes/fo_bse_lot_next_working_day generated ****"

  echo "==================== FO SCRIP DONE ===================="
}

## ===DONE===
fetch_cd_scrip () {
  echo
  echo "==================== CD SCRIP ===================="
  rm Master.zip

  wget -t 2 --timeout=20 --referer https://www.bseindia.com --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.bseindia.com/downloads1/Master.zip

  # if failed to download then exit.
  [ ! -f Master.zip ] && return

  unzip Master.zip -d $FTP_DIR/
  rm Master.zip

  mv BFX_CC_CO${DD}${MM}${YY}.csv $FTP_DIR/
  mv BFX_CO${DD}${MM}${YY}.csv $FTP_DIR/
  mv BFX_SPD_CO${DD}${MM}${YY}.csv $FTP_DIR/
  mv BFX_DP${DD}${MM}${YY} $FTP_DIR/
  mv BSE_BFX_CONTRACT_${DD}${MM}${YY}.csv $FTP_DIR/
  mv BSE_BFX_SPDCONTRACT_${DD}${MM}${YY}.csv $FTP_DIR/

  # Files successfully downloaded.
  file_downloaded=1

  # Generate Red data using CD SCRIP Files.
  awk -F',' 'NR==FNR{a[$1]=$6;b[$1]=$7; next} ($1 in a) {x=$9;if($9== ""){x="XX"};print $1,$2,$3,$4,$7,$8,x,$54,a[$1],b[$1]}' $FTP_DIR/BFX_DP${DD}${MM}${YY} $FTP_DIR/BFX_CO${DD}${MM}${YY}.csv > /spare/local/tradeinfo/BSE_Files/RefData/bse_cd_$next_working_day"_contracts.txt"

  chgrp infra /spare/local/tradeinfo/BSE_Files/RefData/bse_cd_${next_working_day}_contracts.txt

  echo "**** /RefData/bse_cd_next_working_day_contracts.txt generated ****"

  echo "==================== CD SCRIP DONE ===================="
} 

fetch_cm_bhavcopy () {
  echo
  echo "==================== CM BHAVCOPY ===================="
  wget -t 2 --timeout=20 --referer https://www.bseindia.com --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.bseindia.com/download/BhavCopy/Equity/EQ${DD}${MM}${YY}_CSV.ZIP

  # if failed to download then exit.
  [ ! -f EQ${DD}${MM}${YY}_CSV.ZIP ] && return

  unzip -o EQ${DD}${MM}${YY}_CSV.ZIP;
  rm EQ${DD}${MM}${YY}_CSV.ZIP

  # Files successfully downloaded.
  file_downloaded=1

  # Move the file to /spare/local/tradeinfo/BSE_Files/Margin_Files/Exposure_Files/
  mv EQ${DD}${MM}${YY}.CSV /spare/local/tradeinfo/BSE_Files/Margin_Files/Exposure_Files/

  chown dvctrader:infra /spare/local/tradeinfo/BSE_Files/Margin_Files/Exposure_Files/EQ${DD}${MM}${YY}.CSV

  # Generate VarFiles using python script
  fetch_daily_margin 

  # Generate Security margin file.
  awk -F"," 'NR==FNR{a[$1]=$10; next} ($1 in a){print $1"|"(a[$1]*$8)/100}' /spare/local/tradeinfo/BSE_Files/Margin_Files/VAR_FILES/VarFile${YYYYMMDD}.csv /spare/local/tradeinfo/BSE_Files/Margin_Files/Exposure_Files/EQ${DD}${MM}${YY}.CSV  > /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt_tmp

  awk -F"|" 'NR==FNR{a[$1]=$3; next} ($1 in a){print "BSE_"a[$1]" "$2}'  $FTP_DIR/SCRIP/SCRIP_${DD}${MM}${YY}.TXT /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt_tmp > /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt

  rm /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt_tmp

  chgrp infra /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt

  echo "**** /SecuritiesMarginFiles/security_margin_next_working_day.txt generated ****"
  echo "SM File generated"

  # Generating Products less than 100.
  echo "Product less then Rs100 file"
  next_MMYYYY=${next_working_day:4:2}${next_working_day:0:4}
  if [ $next_MMYYYY -ne ${MM}${YYYY} ]; then
    awk -F',' '{if ($8 <= 100) print $1}' EQ${DD}${MM}${YY}.CSV > /tmp/exchange_id_bse
    awk '(NR==FNR) {secid[$1]=1; next;} { if ($1 in secid) print "BSE_"$4 }' /tmp/exchange_id_bse/spare/local/tradeinfo/BSE_Files/RefData/bse_eq_"$next_working_day"_contracts.txt | sort | uniq > /spare/local/tradeinfo/BSE_Files/SecuritiesPriceLess100/EQ_PRICE_LESS_100_${YYYYMMDD}
  else
    prev_work_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
    cp /spare/local/tradeinfo/BSE_Files/SecuritiesPriceLess100/EQ_PRICE_LESS_100_${prev_work_day} /spare/local/tradeinfo/BSE_Files/SecuritiesPriceLess100/EQ_PRICE_LESS_100_${YYYYMMDD}
  fi

  echo "**** /SecuritiesPriceLess100/EQ_PRICE_LESS_100_YYYYMMDD generated ****"

  # Generating groupAB product file.
  echo "Updating groupAB product file"
  groupAB_file="/spare/local/tradeinfo/BSE_Files/RefData/bse_groupAB_symbol_bhavcopy_${next_working_day}"
  >$groupAB_file
  while IFS= read -r line;
  do
     sc_group=`echo "$line"| awk -F, '{print $3}'`
     if [ $sc_group = 'A' ] || [ $sc_group = 'B' ]; then
        stk_code=`echo "$line" | awk -F, '{print $1}'`
        sc_symbol=`grep $stk_code "/spare/local/tradeinfo/BSE_Files/RefData/bse_eq_${next_working_day}_contracts.txt" | awk '{print $4}'`
        if [ ! -z "$sc_symbol" ]; then
           echo  "$sc_symbol, $line" >>"$groupAB_file"
        fi
     fi
  done <"/spare/local/tradeinfo/BSE_Files/Margin_Files/Exposure_Files/EQ${DD}${MM}${YY}.CSV"

  chgrp infra /spare/local/tradeinfo/BSE_Files/RefData/bse_eq_"$next_working_day"_contracts.txt
  echo "**** /RefData/bse_groupAB_symbol_bhavcopy_next_working_day generated ****"
  
  echo "==================== CM BHAVCOPY DONE ===================="
}

## ===DONE===
fetch_fo_bhavcopy () {
  echo
  echo "==================== FO BHAVCOPY ===================="
  wget -t 2 --timeout=20 --referer https://www.bseindia.com --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.bseindia.com/download/Bhavcopy/Derivative/bhavcopy${DD}-${MM}-${YY}.zip

  # if failed to download then exit.
  [ ! -f bhavcopy${DD}-${MM}-${YY}.zip ] && return

  unzip -o bhavcopy${DD}-${MM}-${YY}.zip -d $FTP_DIR/
  rm bhavcopy${DD}-${MM}-${YY}.zip;

  cp $FTP_DIR/bhavcopy${DD}-${MM}-${YY}.csv /spare/local/tradeinfo/BSE_Files/BhavCopy/fo/$MM$YY/

  # Files successfully downloaded.
  file_downloaded=1

  # Verify BHAVCOPY FO File.
  check_bhavcopy_expiry /spare/local/tradeinfo/BSE_Files/BhavCopy/fo/$MM$YY/bhavcopy${DD}-${MM}-${YY}.csv;

  echo "**** /BhavCopy/fo/MMYY/bhavcopyDD-MM-YY.csv generated ****"

  # Generate MD Files using FO BhavCopy Files.
  echo "generating MD files"

  cd /spare/local/tradeinfo/BSE_Files/BhavCopy/fo/$MM$YY/
  /home/pengine/prod/live_scripts/generate_bhav_md_file_bse.sh
  #cat bhavcopy${DD}-${MM}-${YY}.csv | tail -n +2 | awk -F"," '{ OFS=","; print "NORMAL",$1,$2,toupper($3),$4,$5,"-",$6,$7,$8,$9,$14,$15,$16,$17}' > ${DD}${MM}fo_0000.md
  
  chgrp infra bhavcopy${DD}-${MM}-${YY}.csv
  chgrp infra ${DD}${MM}fo_0000.md

  echo "**** BhavCopy/fo/MMYY/DDMMfo_0000.md generated ****"

  echo "==================== FO BHAVCOPY DONE ===================="
}

## ===DONE===
fetch_cd_bhavcopy () {
  echo
  echo "==================== CD BHAVCOPY ===================="
  wget -t 2 --timeout=20 --referer https://www.bseindia.com --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.bseindia.com/bsedata/CIML_bhavcopy/CurrencyBhavCopy_${YYYY}${MM}${DD}.zip

  # if failed to download then exit.
  [ ! -f CurrencyBhavCopy_${YYYY}${MM}${DD}.zip ] && return

  unzip -o CurrencyBhavCopy_${YYYY}${MM}${DD}.zip -d $FTP_DIR/;
  rm CurrencyBhavCopy_${YYYY}${MM}${DD}.zip;

  # Files successfully downloaded.
  file_downloaded=1

  # Move the file to /spare/local/tradeinfo/BSE_Files/BhavCopy/cds/$MM$YY/
  mv $FTP_DIR/CurrencyBhavCopy_${YYYY}${MM}${DD}.csv /spare/local/tradeinfo/BSE_Files/BhavCopy/cds/$MM$YY/

  echo "**** /BhavCopy/cds/MMYY/CurrencyBhavCopy_${YYYY}${MM}${DD}.csv generated ****"

  # Generate CD Files using CD BhavCopy Files.
  cd /spare/local/tradeinfo/BSE_Files/BhavCopy/cds/$MM$YY/
  cat CurrencyBhavCopy_${YYYY}${MM}${DD}.csv | tail -n +2 | awk -F"," '{ OFS=","; print "NORMAL",$1,$2,toupper($3),$4,$5,"-",$6,$7,$8,$9,$14,$15,$16,$17}' > ${DD}${MM}cd_0000.md

  chgrp infra CurrencyBhavCopy_${YYYY}${MM}${DD}.csv
  chgrp infra ${DD}${MM}cd_0000.md

  echo "**** /BhavCopy/cds/MMYY/DDMMcd_0000.md generated ****"

  echo "==================== CD BHAVCOPY DONE ===================="
}

# Functions to get some additional files.
fetch_sec_ban_file() {
  echo
  echo "==================== Updating SecBan file ===================="

  cd /spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/

  wget -t 2 --timeout=20 --referer https://www.bseindia.com --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.bseindia.com/Download/Derivative/MWPL/BannedScrips-${DD}${MM}${YYYY}.zip

  [ ! -f BannedScrips-${DD}${MM}${YYYY}.zip ] && return

  unzip BannedScrips-${DD}${MM}${YYYY}.zip
  rm BannedScrips-${DD}${MM}${YYYY}.zip

  # Files successfully downloaded.
  file_downloaded=1

  mv BannedScrips-${DD}${MM}${YYYY}.csv /spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/

  tail -n +2 /spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/BannedScrips-${DD}${MM}${YYYY}.csv | cut -d',' -f4 > /spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/fo_secban_${next_working_day}.csv

  # Update the status of these files as well.
  # verify_given_files "/spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/BannedScrips-${DD}${MM}${YYYY}.csv"

  chgrp infra "/spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/BannedScrips-${DD}${MM}${YYYY}.csv"
  chgrp infra /spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/fo_secban_${next_working_day}.csv

  # Syncing to other servers.
  echo "Syncing tradeinfo: sec ban files: 2"
  # tradeinfo_sync_secban

  echo "==================== Updating SecBan file DONE ===================="
}

fetch_span_file() {
  echo
  echo "==================== Updating Span file ===================="

  cd /spare/local/tradeinfo/BSE_Files/Margin_Files/Span_Files/

  wget -t 2 --timeout=20 --referer https://www.bseindia.com --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.bseindia.com/bsedata/Risk_Automate/SPN/BSERISK${YYYY}${MM}${DD}-FINAL.ZIP

  [ ! -f BSERISK${YYYY}${MM}${DD}-FINAL.ZIP ] && return

  unzip -o BSERISK${YYYY}${MM}${DD}-FINAL.ZIP
  rm BSERISK${YYYY}${MM}${DD}-FINAL.ZIP

  # Files successfully downloaded.
  file_downloaded=1
    
  chmod 666 BSERISK${YYYY}${MM}${DD}-FINAL.spn

  # verify_given_files /spare/local/tradeinfo/BSE_Files/Margin_Files/Span_Files/BSERISK${YYYY}${MM}${DD}-FINAL.spn
  chgrp infra /spare/local/tradeinfo/BSE_Files/Margin_Files/Span_Files/BSERISK${YYYY}${MM}${DD}-FINAL.spn

  echo /Span_Files/BSERISK${YYYY}${MM}${DD}-FINAL.spn downloaded > $report
  echo > $report

  echo "==================== Updating Span file DONE ===================="
}


fetch_daily_margin () {
  /home/dvctrader/.conda/envs/env/bin/python /home/dvctrader/important/BSE/download_daily_margin.py "${DD}${MM}${YYYY}"
  cd /spare/local/tradeinfo/BSE_Files/Margin_Files/VAR_FILES/
  /home/dvctrader/.conda/envs/env/bin/html2csv VarFile${DD}${MM}${YYYY}.xls -o VarFile${YYYYMMDD}.csv

  chown dvctrader:infra /spare/local/tradeinfo/BSE_Files/Margin_Files/VAR_FILES/VarFile${DD}${MM}${YYYY}.xls
  chown dvctrader:infra /spare/local/tradeinfo/BSE_Files/Margin_Files/VAR_FILES/VarFile${YYYYMMDD}.csv

  # verify_given_files /spare/local/tradeinfo/BSE_Files/Margin_Files/VAR_FILES/VarFile${YYYYMMDD}.csv
  
  echo "==================== VAR FILES DONE ===================="
}

# Utility functions.
check_bhavcopy_expiry () {
least_expiry=`cat $1 | cut -d',' -f3 | cut -d' ' -f3 | sort | head -1`
  if [ $least_expiry -lt $YYYY ]; then
    echo "FAILed Older expirires present in bhavcopy file"
    # printf "Older expirires present in bhavcopy file" | /bin/mail -s "BSEDailyFilesGenerationFailure - $1" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in;
  fi
}

verify_FTP_files () {
  GIVEN_FILE=${FTP_DIR}/${1};
  [ -f $GIVEN_FILE -a -s $GIVEN_FILE -a -r $GIVEN_FILE ] || printf "$GIVEN_FILE FTP FILE not present for today" | /bin/mail -s "BSE Fetch Status" -r "${HOSTNAME}-${USER}<gopi.m.tatiraju@tworoads-trading.co.in>" "infra_alerts@tworoads-trading.co.in"
}

verify_given_files () {
  GIVEN_FILE=$1;
    [ -f $GIVEN_FILE -a -s $GIVEN_FILE -a -r $GIVEN_FILE ] || printf "File not present for today $GIVEN_FILE" | /bin/mail -s "BSEDailyFilesGenerationFailure - $GIVEN_FILE" -r "${HOSTNAME}-${USER}<gopi.m.tatiraju@tworoads-trading.co.in>" "infra_alerts@tworoads-trading.co.in"
#    [ -f $GIVEN_FILE -a -s $GIVEN_FILE -a -r $GIVEN_FILE ] || exit ;
}

report_file=/tmp/fail_bse

report_fail=/tmp/bse_fetch_status
>${report_fail}

# Main
echo "========= Starting BSE FETCH $DD$MM$YYYY ========="

echo "Deleting $FTP_DIR"
rm -rf $FTP_DIR
mkdir -p $FTP_DIR

cd $FTP_DIR
echo "In $FTP_DIR"

file_downloaded=0
fetch_sec_ban_file

file_downloaded=0
fetch_span_file
if [ $file_downloaded = "0" ] ; then
  echo "fetch_span_file FAILED" >> $report_fail
fi

file_downloaded=0
fetch_cm_scrip ;
if [ $file_downloaded = "0" ] ; then
  echo "fetch_cm_scrip" >> $report_fail
fi

file_downloaded=0
fetch_fo_scrip ;
if [ $file_downloaded = "0" ] ; then
  echo "fetch_fo_scrip" >> $report_fail
fi

file_downloaded=0
fetch_cd_scrip ;
if [ $file_downloaded = "0" ] ; then
  echo "fetch_cd_scrip" >> $report_fail
fi

file_downloaded=0
fetch_fo_bhavcopy ;
if [ $file_downloaded = "0" ] ; then
  echo "fetch_fo_bhavcopy" >> $report_fail
fi

file_downloaded=0
fetch_cd_bhavcopy
if [ $file_downloaded = "0" ] ; then
  echo "fetch_cd_bhavcopy" >> $report_fail
fi

# errors=`wc -l $files_missing | awk '{print $1}'`

echo "Syncing..."
rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 10.23.5.66:/spare/local/tradeinfo
rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 192.168.132.11:/spare/local/tradeinfo
rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 192.168.132.12:/spare/local/tradeinfo
rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 10.23.5.50:/spare/local/tradeinfo
rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 10.23.5.22:/spare/local/tradeinfo
rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 10.23.5.68:/spare/local/tradeinfo

rsync -ravz /spare/local/files/BSEFTPFiles 10.23.5.66:/spare/local/files
rsync -ravz /spare/local/files/BSEFTPFiles 10.23.5.50:/spare/local/files
rsync -ravz /spare/local/files/BSEFTPFiles 10.23.5.22:/spare/local/files
rsync -ravz /spare/local/files/BSEFTPFiles 192.168.132.11:/spare/local/files
rsync -ravz /spare/local/files/BSEFTPFiles 192.168.132.12:/spare/local/files
rsync -ravz --timeout=60 /spare/local/files/BSEFTPFiles 10.23.5.68:/spare/local/files

cat $report_fail | /bin/mail -s "BSE Fetch Status" -r "${HOSTNAME}-${USER}<gopi.m.tatiraju@tworoads-trading.co.in>" infra_alerts@tworoads-trading.co.in;
