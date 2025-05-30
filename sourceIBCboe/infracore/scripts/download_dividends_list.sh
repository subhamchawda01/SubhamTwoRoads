/home/dvctrader/.conda/envs/env/bin/python /home/pengine/prod/live_scripts/dividends_mailer.py >~/trash/dividends_out 2>~/trash/dividends_err

[ $? -ne 0 ] && echo "" | mailx -s "FAILED DOWNLOADING DIVIDENDS REPORT" -r \
	"${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" \
	hardik.dhakate@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in \
	raghunandan.sharma@tworoads-trading.co.in

#sync to all  prod machines

rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.64:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.65:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.69:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.81:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.82:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.83:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.84:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.63:/spare/local/tradeinfo --delete-after

