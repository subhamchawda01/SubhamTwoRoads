#! /bin/bash

YYYYMMDD=`date +"%Y%m%d"` ;
TOMORROW_DATE=`date --date='tomorrow' +"%Y%m%d"` ;
TODAY_DATE=`date +"%Y%m%d"`;

TAG_PNL_DIR="/spare/local/logs/pnl_data/hft/tag_pnl/";
TAG_PNL_SCRIPT="/home/pengine/prod/live_scripts/show_pnl_generic_tagwise.pl";
TAG_PNL_FILE="${TAG_PNL_DIR}tagwise_pnl.txt";
TAG_PNL_BKP_FILE="${TAG_PNL_DIR}tagwise_pnl_bkp.txt";
TAG_PNL_ERR_FILE="${TAG_PNL_DIR}pnl_tagwise_err.log";
TAG_PNL_ERR_BKP_FILE="${TAG_PNL_DIR}tagwise_err_bkp.log";
UTAG_MAP_FILE="${TAG_PNL_DIR}unknown_qid_saci_tag";
UTAG_MAP_BKP_FILE="${TAG_PNL_DIR}saci_maps_hist/unknown_qid_saci_tag_${TODAY_DATE}";
TAG_MAP_FILE="/spare/local/logs/risk_logs/qid_saci_tag";
TAG_MAP_BKP_FILE="${TAG_PNL_DIR}saci_maps_hist/qid_saci_tag_${TODAY_DATE}";
REMAP_FILE="${TAG_PNL_DIR}remap_tag.txt";
REMAP_BKP_FILE="${TAG_PNL_DIR}saci_maps_hist/remap_tag_${TODAY_DATE}.txt";
#Take logs for Tag Individual Files
TI_PNL_FILE="${TAG_PNL_DIR}ti_pnl.txt";
TI_PNL_BKP_FILE="${TAG_PNL_DIR}ti_pnl_bkp.txt";
TI_PNL_ERR_FILE="${TAG_PNL_DIR}pnl_ti_err.log";
TI_PNL_ERR_BKP_FILE="${TAG_PNL_DIR}pnl_ti_err_bkp.log";


touch "$UTAG_MAP_FILE";
touch "$REMAP_FILE";

cp -f "$UTAG_MAP_FILE" "$UTAG_MAP_BKP_FILE";
cp -f "$TAG_MAP_FILE" "$TAG_MAP_BKP_FILE";
cp -f "$REMAP_FILE" "$REMAP_BKP_FILE";
> "$UTAG_MAP_FILE";
> "$REMAP_FILE";
rm -f /spare/local/logs/pnl_data/hft/delta_files/* ;
cat "$TAG_PNL_FILE" > "$TAG_PNL_BKP_FILE" ; 
> "$TAG_PNL_FILE" ;
cat "$TAG_PNL_ERR_FILE" > "$TAG_PNL_ERR_BKP_FILE" ;
> "$TAG_PNL_ERR_FILE" ;

cat "$TI_PNL_FILE" > "$TI_PNL_BKP_FILE" ;
> "$TI_PNL_FILE" ;
cat "$TI_PNL_ERR_FILE" > "$TI_PNL_ERR_BKP_FILE";
> "$TI_PNL_ERR_FILE";
#perl /home/dvcinfra/LiveExec/scripts/calc_ors_pnl.pl 'C' 'H' $TOMORROW_DATE > /spare/local/logs/pnl_data/hft/pnls.txt 2>>/spare/local/logs/pnl_data/hft/pnl_log &
ssh dvcinfra@10.23.43.51 "cat /spare/local/files/.saci_record.txt" >> "$TAG_MAP_FILE"
/home/pengine/prod/live_scripts/manage_pnl_setup.sh stop H
perl "$TAG_PNL_SCRIPT" 'C' 'H' "$TOMORROW_DATE"  TT N >"$TAG_PNL_FILE" 2>"$TAG_PNL_ERR_FILE" &
perl "$TAG_PNL_SCRIPT" 'C' 'H' "$TOMORROW_DATE"  TI N >"$TI_PNL_FILE" 2>"$TI_PNL_ERR_FILE" &
