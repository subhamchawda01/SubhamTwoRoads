#!/bin/bash

today=`date +"%Y%m%d"`;
yyyy=${today:0:4}
mm=${today:4:2}
flag=0
IND17Stat_project_path="/home/subham/subham/htmlProject_"
IND17Stat_html_file_path="/var/www/html/IND17_OEBU_PNL_STAT"
html_files_path="$IND17Stat_project_path/htmlFiles"
html_monthly_file_path="$IND17Stat_html_file_path/$yyyy/$mm"
IND17Stat_data_files_path="$html_files_path/IND17Stat_data_files"
IND17Stat_monthly_data_path="$IND17Stat_data_files_path/$yyyy/$mm"
log_file_name="$IND17Stat_project_path/log/$yyyy/$mm/log_$today.txt"

copy_log_file() {
	
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

generate_monthly_html_page() {

	monthly_report_body_file_path="$IND17Stat_data_files_path/html_body_monthly_report.txt"
	monthly_report_html_file_path="$IND17Stat_html_file_path/IND17_monthly_report.html"
	monthly_report_html_style_file="$html_files_path/html_monthly_report.txt"

	if [ "$flag" -eq 1 ]; then
		echo "<tr><td><a href=$yyyy/$mm/IND17_mean_median_stddeviation.html style=color:blue>$yyyy$mm</a></td><td><a href=$yyyy/$mm/IND17_max_loss.html style=color:blue>$yyyy$mm</a></td></tr>" >> $monthly_report_body_file_path
	fi

	head -n 21 $monthly_report_html_style_file > $monthly_report_html_file_path
	tail -n 5 $IND17Stat_data_files_path/last_5days_max_loss.txt >> $monthly_report_html_file_path	
	sed -n '22,30 p' $monthly_report_html_style_file >> $monthly_report_html_file_path
	tail -n 5 $IND17Stat_data_files_path/last_5days_mean_median_std_dev.txt >> $monthly_report_html_file_path
	sed -n '31,39 p' $monthly_report_html_style_file >> $monthly_report_html_file_path
	cat $monthly_report_body_file_path >> $monthly_report_html_file_path
	tail -n 24 $monthly_report_html_style_file >> $monthly_report_html_file_path

	
	if [ -f $monthly_report_body_file_path ] && [ -f $monthly_report_html_file_path ]; then
                echo "generate monthly html file success"
        else
                echo "generete monthly html file failed"
		exit 1
        fi

}

generate_max_loss_html_file() {

	max_loss_body_file_path="$IND17Stat_monthly_data_path/html_body_max_loss.txt"
	max_loss_html_file_path="$html_monthly_file_path/IND17_max_loss.html"
	data=`awk '{print $NF,$0}' $log_file_name | sort -n | cut -f2- -d' ' | head -n+1`
	product=`echo $data | awk '{print $1}'`
	traded_value=`echo $data | awk '{print $5}'`
	vv=`echo $data | awk '{print $9}'`
	pnl=`echo $data | awk '{print $12}'`

	echo "<tr><td><a href=IND17Stat_daily_files/IND17Stat.$today.html style=color:blue>$today</a></td><td>${product%=>}</td><td>$traded_value</td><td>$vv</td><td>$pnl</td></tr>" >> $max_loss_body_file_path
	echo "<tr><td><a href=$yyyy/$mm/IND17Stat_daily_files/IND17Stat.$today.html style=color:blue>$today</a></td><td>${product%=>}</td><td>$traded_value</td><td>$vv</td><td>$pnl</td></tr>" >> $IND17Stat_data_files_path/last_5days_max_loss.txt

	head -n 19 $html_files_path/html_max_loss.txt > $max_loss_html_file_path
	cat $max_loss_body_file_path >> $max_loss_html_file_path
	tail -n 14 $html_files_path/html_max_loss.txt >> $max_loss_html_file_path

	if [ -f $max_loss_body_file_path ] && [ -f $max_loss_html_file_path ]; then
                echo "generate max loss html file success"
        else
                echo "generete max loss html file failed"
		exit 1
        fi

}	

generate_mean_median_standard_deviation_html_file() {
	
	mean_median_stddeviation_body_file_path="$IND17Stat_monthly_data_path/html_body_mean_median_stddeviation.txt"
	mean_median_stddeviation_html_file_path="$html_monthly_file_path/IND17_mean_median_stddeviation.html"
	total_pnl=`awk '{sum += $NF} END {print sum}' $log_file_name`
	total_product=`wc -l $log_file_name | cut -f1 -d' '`
	mid=$(( total_product / 2 ))
	mean=$(( total_pnl / total_product ))
	median=`awk '{print $NF}' $log_file_name | sort -nr | sed -n "$mid"p`
	standard_deviation=`awk '{sum += ($NF - m)^2} END {print sqrt(sum / tp)}' m=$mean tp=$total_product $log_file_name`

	echo "<tr><td><a href=IND17Stat_daily_files/IND17Stat.$today.html style=color:blue>$today</a></td><td>$mean</td><td>$median</td><td>$standard_deviation</td></tr>" >> $mean_median_stddeviation_body_file_path
	echo "<tr><td><a href=$yyyy/$mm/IND17Stat_daily_files/IND17Stat.$today.html style=color:blue>$today</a></td><td>$mean</td><td>$median</td><td>$standard_deviation</td></tr>" >> $IND17Stat_data_files_path/last_5days_mean_median_std_dev.txt

	head -n 19 $html_files_path/html_mean_median_stddeviation.txt > $mean_median_stddeviation_html_file_path
	cat $mean_median_stddeviation_body_file_path >> $mean_median_stddeviation_html_file_path
	tail -n 14 $html_files_path/html_mean_median_stddeviation.txt >> $mean_median_stddeviation_html_file_path

	if [ -f $mean_median_stddeviation_body_file_path ] && [ -f $mean_median_stddeviation_html_file_path ]; then
                echo "generate mean median std deviation html file success"
        else
                echo "generete mean median std deviation html file failed"
		exit 1
        fi	

}

generate_html_files() {

	IND17Stat_daily_files="$html_monthly_file_path/IND17Stat_daily_files"
	today_IND17Stat_file="$IND17Stat_daily_files/IND17Stat.$today.html"
	if [ ! -d $IND17Stat_monthly_data_path ]; then
		flag=1
	fi
	mkdir -p $IND17Stat_daily_files 
	mkdir -p $IND17Stat_monthly_data_path
	chown root:root $IND17Stat_html_file_path
	head -n 19 $html_files_path/html_daily.txt >> $today_IND17Stat_file

	IFS=' '
	while read -r f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 
	do
	echo "<tr><td>${f1%=>}</td><td>$f5</td><td>$f9</td><td>$f12</td></tr>" >> $today_IND17Stat_file
	done <"$log_file_name"

	tail -n 14 $html_files_path/html_daily.txt >> $today_IND17Stat_file
	
}

copy_log_file;

generate_html_files;

generate_max_loss_html_file

generate_mean_median_standard_deviation_html_file

generate_monthly_html_page

