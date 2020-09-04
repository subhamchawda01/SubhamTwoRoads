#!/bin/bash

today=`date +"%Y%m%d"`;
#today="20200903";
yyyy=${today:0:4}
mm=${today:4:2}
IND17Stat_project_path="/home/subham/subham/htmlProject"
IND17Stat_html_file_path="/var/www/html/IND17_OEBU_PNL_STAT"
html_files_path="$IND17Stat_project_path/htmlFiles"
html_product_file_path="$IND17Stat_html_file_path/product"
IND17Stat_30days_data_file="$html_files_path/last_30days_data.txt"
log_file_name="$IND17Stat_project_path/log/$yyyy/$mm/log_$today.txt"

copy_log_file() {
	
	is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today T`
	if [ $is_holiday = "1" ] ; then
   		echo "NSE Holiday. Exiting...";
   		exit;
	fi

	log_file_path="$IND17Stat_project_path/log/$yyyy/$mm"
	mkdir -p $log_file_path
	scp dvcinfra@10.23.227.63:/home/dvcinfra/important/IND17_OEBU_STAT/log_$today.txt $log_file_path

	if [ -f $log_file_name ]; then
                echo "copy log_$today file success"
        else
                echo "copy log_$today file failed"
                exit 1
        fi

}

delete_last_30th_day_entries() {

	last_30days_date="$html_files_path/last_30days_date.txt"
	day_30th=`tac $last_30days_date | sed -n 30p`

	if [ ! -z "$day_30th" ]; then
		sed '/$30th_day/d' $IND17Stat_30days_data_file > $IND17Stat_30days_data_file
		sed '/$30th_day/d' $last_30days_date > $last_30days_date
		echo "old entry deleted"
	fi

}

generate_product_html_files() {

	IND17Stat_html_report_file="$IND17Stat_html_file_path/IND17Stat_report.html"
	head -n 19 $html_files_path/html_daily.txt > $IND17Stat_html_report_file
	#product_array=($(awk '{print $1}' $IND17Stat_30days_data_file | sort | uniq))
	IFS=$'\n' read -r -d '' -a product_array < <( awk '{print $1}' $IND17Stat_30days_data_file | sort | uniq && printf '\0' )

	for product in "${product_array[@]}"
	do
		html_product_file="$html_product_file_path/$product.html"

		head -n 19 $html_files_path/html_product.txt > $html_product_file
		grep -w $product $IND17Stat_30days_data_file | cut -f2 -d ' ' >> $html_product_file
		tail -n 14 $html_files_path/html_product.txt >> $html_product_file

	        product_count=`grep -w -o $product $IND17Stat_30days_data_file | wc -w`
		total_pnl=`grep -w $product $IND17Stat_30days_data_file | sed 's/<td>/ /g; s/<\/td>/ /g;' | awk '{sum += $(NF-1)} END {print sum}'`
	        total_vV=`grep -w $product $IND17Stat_30days_data_file | sed 's/<td>/ /g; s/<\/td>/ /g;' | awk '{sum += $(NF-2)} END {print sum}'`
	        average_pnl=$(( total_pnl / product_count ))
		average_vV=`echo "scale=4; $(printf "%.4f" $total_vV) / $product_count" | bc`

		echo "<tr><td><a href=product/$product.html style=color:blue>$product</a></td><td>$product_count</td><td>$(printf "%.4f" $average_vV)</td><td>$(printf "%.4f" $total_vV)</td><td>$average_pnl</td><td>$total_pnl</td></tr>" >> $IND17Stat_html_report_file
	
	done
	tail -n 14 $html_files_path/html_daily.txt >> $IND17Stat_html_report_file

}

generate_html_files() {

	delete_last_30th_day_entries;
	mkdir -p $html_product_file_path
	chown root:root $IND17Stat_html_file_path

	echo "$today" >> $html_files_path/last_30days_date.txt

	IFS=' '
	while read -r f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 
	do
		echo "${f1%=>} <tr><td>$today</td><td>$(printf "%.4f" $f5)</td><td>$(printf "%.4f" $f9)</td><td>$f12</td></tr>" >> $IND17Stat_30days_data_file

	done <"$log_file_name"

	generate_product_html_files

	echo "IND17STAT_report generated"
	
}

copy_log_file;

generate_html_files;

