date_=$1
while [ $date_ -le $2 ];do
        echo "Generating  for : "$date_
        rm Sorted_Data/*
        rm Sorted_Options_Data/*
        rm Sorted_Weekly_Options_Data/*
        rm Sorted_Cash_Data/*
        rm FILENAMES

        #bash cash_data_gen.sh $date_ > Logs/cash_logs
        bash fut_data_gen.sh $date_ > Logs/fut_logs
        bash opt_data_gen.sh $date_ > Logs/opt_logso
        bash weekly_opt_data_gen.sh $date_ > Logs/weekly_option_logs
        #bash fut_data_gen.sh $date_ > Logs/fut_logs
        #bash cd_opt_data_gen.sh $date_ > Logs/cd_opt_logs
	      date_=$(/home/dvctrader/cvquant_install/dvccode/bin/update_date $date_ N W 1)
done;
bash auto_merger.sh
