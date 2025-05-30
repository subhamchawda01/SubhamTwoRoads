#!/bin/bash

USAGE="$0 YYYYMMDD ";

if [ $# -ne 1 ] ; 
then 
    echo -e $USAGE;
    exit;
fi

YYYYMMDD=$1;

#delete yesterday's files
find /apps/data/ReconData/ -type f -exec rm -f {} \;

prev_day=`/home/pengine/prod/live_execs/calc_prev_week_day $YYYYMMDD`;
prev_prev_day=`/home/pengine/prod/live_execs/calc_prev_week_day $prev_day`;

#broker file path
newedge_file_today=/apps/data/MFGlobalTrades/MFGFiles/GMITRN_$YYYYMMDD".csv.gz";
newedge_file_prev_day=/apps/data/MFGlobalTrades/MFGFiles/GMITRN_$prev_day".csv.gz";

#edf broker file
edf_file=/apps/data/EDFTrades/EDFFiles/PFDFST4_$prev_day".CSV.gz";

#plural broker file
plural_file=/apps/data/PluralTrades/PluralFiles/InvoiceDetailed_$prev_day".csv.gz";

#asx broker file
asx_file=/apps/data/AbnamroTrades/AbnamroFiles/$prev_day"_abnamro_trx.csv.gz"

#rts broker file
rts_file_prev_day=/apps/data/RencapTrades/RencapFiles"/D"$prev_day"_DCF491_1.xls.gz"
rts_file_today=/apps/data/RencapTrades/RencapFiles"/D"$YYYYMMDD"_DCF491_1.xls.gz"

#micex broker file
micex_file=/apps/data/RencapTrades/RencapFiles"/D"$prev_day"_DMS491_1.xls.gz"

#get ors trades
dated_dir_prev_day=${prev_day:0:4}/${prev_day:4:2}/${prev_day:6:2};
dated_dir_today=${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2};
ose13_trades=/NAS1/logs/ORSTrades/OSE/T3DVC22512/$dated_dir_prev_day/trades.$prev_day
ose14_trades=/NAS1/logs/ORSTrades/OSE/T4DVC22512/$dated_dir_prev_day/trades.$prev_day
hk11_trades=/NAS1/logs/ORSTrades/HKEX/FITGEN/$dated_dir_prev_day/trades.$prev_day

#for eurex, tmx and liffe trades are not distributed across multiple days
eurex_trades=`find /NAS1/logs/ORSTrades/EUREX/ -type f -name "trades.$prev_day" | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9`;
tmx_trades=`find /NAS1/logs/ORSTrades/TMX/ -type f -name "trades.$prev_day" | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9`;
liffe_trades=`find /NAS1/logs/ORSTrades/LIFFE/ -type f -name "trades.$prev_day" | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9`;
cme_trades=`find /NAS1/logs/ORSTrades/CME/ -type f -name "trades.$prev_day" | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9`;
cfe_trades=`find /NAS1/logs/ORSTrades/CFE/ -type f -name "trades.$prev_day" | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9`;
ice_trades=`find /NAS1/logs/ORSTrades/ICE/ -type f -name "trades.$prev_day" | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9`;
bmf_trades=`find /NAS1/logs/ORSTrades/BMFEP/ -type f -name "trades.$prev_day" | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9`;
asx_trades_d=`find /NAS1/logs/ORSTrades/ASX/ -type f -name "trades.$prev_day" | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9`;
asx_trades_d_1=`find /NAS1/logs/ORSTrades/ASX/ -type f -name "trades.$prev_prev_day" | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9`;
rts_trades=`find /NAS1/logs/ORSTrades/RTS/ -type f -name "trades.$prev_day" | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9`;
micex_trades=`find /NAS1/logs/ORSTrades/MICEX/ -type f -name "trades.$prev_day" | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9`;

recon_folder=/apps/data/ReconData/trades_recon_files
pscript=/home/pengine/prod/live_scripts
all_cme_cfe_ice_trades=$recon_folder"/CFE_CME_ICE/cme_cfe_ice_trades"
all_bmf_trades=$recon_folder"/BMF/bmf_trades"
all_eurex_liffe_tmx_trades=$recon_folder"/EUREX_LIFFE_TMX/eurex_liffe_tmx_all_trades";
all_hk_trades=$recon_folder"/HKEX/hk11_trades";
all_ose_trades=$recon_folder"/OSE/ose_all_trades"
all_asx_trade_d=$recon_folder"/ASX/asx_trades_d"
all_asx_trade_d_1=$recon_folder"/ASX/asx_trades_d_1"
all_rts_trade=$recon_folder"/RTS/rts_trades"
all_micex_trade=$recon_folder"/MICEX/micex_trades"
logs="Please find below files for Trade recon.";

#rts files
> $all_rts_trade
for f in `echo $rts_trades`;
do
	cat $f | tr '\001' ' ' >> $all_rts_trade
done

gzip -f $all_rts_trade

#micex files
> $all_micex_trade
for f in `echo $micex_trades`;
do
	cat $f | tr '\001' ' ' >> $all_micex_trade
done

gzip -f $all_micex_trade

#zip asx files
>$all_asx_trade_d
>$all_asx_trade_d_1

#split XT-YT spreads trades to individual legs
python $pscript/split_spreads_trades.py $asx_trades_d 10 33 > $all_asx_trade_d ; gzip -f $all_asx_trade_d
python $pscript/split_spreads_trades.py $asx_trades_d_1 10 33  > $all_asx_trade_d_1 ; gzip -f $all_asx_trade_d_1

#concat all eurex files and zip
> $all_eurex_liffe_tmx_trades
for f in `echo $eurex_trades`;
do
	cat $f | tr '\001' ' ' >> $all_eurex_liffe_tmx_trades
done

for f in `echo $tmx_trades`;
do
	#split BAX spreads trades to individual legs
	python $pscript/split_spreads_trades.py $f 10 33 >> $all_eurex_liffe_tmx_trades
done

for f in `echo $liffe_trades`;
do
	cat $f | tr '\001' ' ' >> $all_eurex_liffe_tmx_trades
done

gzip -f $all_eurex_liffe_tmx_trades

#concat all cme cfe ice files and zip
> $all_cme_cfe_ice_trades
for f in `echo $cme_trades`;
do
	cat $f | tr '\001' ' ' >> $all_cme_cfe_ice_trades
done

for f in `echo $cfe_trades`;
do
	#split VX spreads trades to individual legs
	python $pscript/split_spreads_trades.py $f 10 33 >> $all_cme_cfe_ice_trades
done

for f in `echo $ice_trades`;
do
	cat $f | tr -d " " | tr '\001' ' ' >> $all_cme_cfe_ice_trades
done

gzip -f $all_cme_cfe_ice_trades

#concat all bmf files and zip
> $all_bmf_trades
for f in `echo $bmf_trades`;
do
	cat $f | tr '\001' ' ' >> $all_bmf_trades
done

gzip -f $all_bmf_trades

#gzip hk trade file
> $all_hk_trades
if [ ! -f "$hk11_trades" ];then
 logs=$logs" "`echo "HK11 trades.$prev_day not found at $hk11_trades"`;
else
	cat $hk11_trades | tr '\001' ' ' > $all_hk_trades
	gzip -f $all_hk_trades
fi

#check if ose trades files exists
> $all_ose_trades
if [ ! -f "$ose13_trades" ];then
 logs=$logs" "`echo "ose13 trades.$prev_day not found at $ose13_trades"`;
fi

if [ ! -f "$ose14_trades" ]; then 
 logs=$logs" "`echo "ose14 trades.$prev_day not found at $ose13_trades"`;
fi

#zip the trades file
cat $ose13_trades $ose14_trades | tr '\001' ' ' > $all_ose_trades
gzip -f $all_ose_trades

#decide which files to attach based on availability
attach_file_str="";

#if brokers file not present, don't zip and try attach them
if [ ! -f "$newedge_file_prev_day" ];then
  logs=$logs" "`echo $newedge_file_prev_day not found`;
else
  attach_file_str=$attach_file_str" $newedge_file_prev_day "
fi

if [ ! -f "$newedge_file_today" ];then
  logs=$logs" "`echo $newedge_file_today not found.`;
else
  attach_file_str=$attach_file_str" $newedge_file_today "
fi

echo $logs
#collect all files to be emailed
cp $plural_file $recon_folder"/BMF/"
cp $asx_file $recon_folder"/ASX/"
cp $edf_file $recon_folder"/CFE_CME_ICE/"
cp `echo $attach_file_str` $recon_folder"/EUREX_LIFFE_TMX/"
cp $rts_file_prev_day $rts_file_today $recon_folder"/RTS/"
cp $micex_file $recon_folder"/MICEX/"

#CREATE a tar file
tar -zcvf /apps/data/ReconData/trade_recon_files_"$prev_day".tar.gz $recon_folder"/"
#email to Joseph the files for recon
echo "$logs" | mailx -s "Trade Recon Files $prev_day" -a /apps/data/ReconData/trade_recon_files_"$prev_day".tar.gz -r "abhishek.anand@tworoads.co.in"  "joseph.padiyara@tworoads-trading.co.in"

