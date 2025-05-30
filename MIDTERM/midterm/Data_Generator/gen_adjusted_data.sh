cd /NAS1/data/NSEBarData/FUT_BarData
date_="$(date +'%Y%m%d')"

for ticker in `ls -p | grep -v "/"`; do 
   /home/dvcinfra/midterm/Data_Generator/Execs/backadjustment_exec --start_date 20051101 --end_date $date_ --index_file /NAS1/data/NSEMidTerm/MachineReadableCorpAdjustmentFiles/Complete_Index_No_Header.csv --output_file /NAS1/data/NSEBarData/FUT_BarData_Adjusted/$ticker --ticker $ticker
done
