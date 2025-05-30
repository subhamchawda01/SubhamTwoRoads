--------PNL Report-----------

scripts=>
grep_historic_stratPNL.sh
perMonthPNL.sh

daily_generate_pnl.sh

To generate html page=>
 For genereating Historical reports=> generate_backup_pnl_reports.sh  
 For current day reports=> daily_generate_pnl_reports.sh  
 For weekly reports=> generate_pnl_reports.sh  

Html Content=> pnl_report_header.txt
For getting log files=> logFileCopy.sh




to create HISTORICAL DATA for all server
NOTE: take backup of /NAS1/data/PNLReportsIND/www/PNLReportsIND/ , /home/hardik/PNLProject/PNLCount1/
for older trade logs check 5.42 /run/media/dvcinfra/NonMDBackup/SERVERDATA
sync file to the copy folder from the 5.26
1)  ./logFileCopy_backup.sh 
Generate all the stratPNL Historic file strat and profit on that day in PNLCOUNT1 folder
2)  ./grep_historic_stratPNL.sh
will gen per month and year strat report
3)  ./perMonthPNL.sh
Generate daily PNL report (update current year in script)
4) ./get_top5_gainer_loser_product.sh H2
Generate Last 5 day PNL report
5) ./html/generate_pnl_reports.sh <YYYYMMDD>
Generate pnl report for each month and year (where as daily_generate_pnl_report only for current month and year)
6)  ./html/generate_backup_pnl_reports.sh
gen historic data for products (update current year in script)
7)  ./Historic_Year_Month.sh

to create HISTORICAL DATA for one server
NOTE: take backup of /NAS1/data/PNLReportsIND/www/PNLReportsIND/ , /home/hardik/PNLProject/PNLCount1/
      update the server in all below script and check rm cmd (only remove that server files)
for older trade logs check 5.42 /run/media/dvcinfra/NonMDBackup/SERVERDATA
sync file to the copy folder from the 5.26
1)  ./logFileCopy_backup.sh
Generate all the stratPNL Historic file strat and profit on that day in PNLCOUNT1 folder
2)  ./grep_historic_stratPNL.sh
will gen per month and year strat report
3)  ./perMonthPNL.sh
Generate daily PNL report (update current year in script)
4) ./get_top5_gainer_loser_product.sh H2
Generate Last 5 day PNL report
5) ./html/generate_pnl_reports.sh <YYYYMMDD>
Generate pnl report for each month and year (where as daily_generate_pnl_report only for current month and year)
6)  ./html/generate_backup_pnl_reports.sh
gen historic data for products (update current year in script)
7)  ./Historic_Year_Month.sh

# run cron process to update the html if not updated for yesterday date

#=========================== PNL REPORT GEN ========================================
#copy file from 5.13
10 1 * * 2-6 /home/hardik/PNLProject/logFileCopy.sh YESTERDAY >/dev/null 2>&1 &
# generate data for the current day (also update month and year)
20 1 * * 2-6 /home/hardik/PNLProject/daily_generate_pnl.sh YESTERDAY >/tmp/pnl_dump4 2>&1 &
#generate top and bottom 5 products daily
30 1 * * 2-6 /home/hardik/PNLProject/get_top5_gainer_loser_product.sh DAILY YESTERDAY >/dev/null 2>&1 &
#weekly pnl report index page daily.index
40 1 * * 2-6 /home/hardik/PNLProject/html/generate_pnl_reports.sh YESTERDAY >/dev/null 2>&1 &
#generate each day report
40 1 * * 2-6 /home/hardik/PNLProject/html/daily_generate_pnl_reports.sh YESTERDAY >/dev/null 2>&1 &
#cal monthly gainer and losser
00 2 * * 6 YYYYMM=`date +\%Y\%m` ;/home/hardik/PNLProject/get_top5_gainer_loser_product.sh MONTHLY  ${YYYYMM} >/dev/null 2>&1 &
# gen pnl yearly
#00 3 * * 6 YYYY=`date +\%Y` ;/home/hardik/PNLProject/get_top5_gainer_loser_product.sh YEARLY  ${YYYY} >/dev/null 2>&1 &
# gen pnl for the previous month runs on 1 of every  month
00 0 4 * *   /home/hardik/PNLProject/last_day_script.sh >/dev/null 2>&1 &
