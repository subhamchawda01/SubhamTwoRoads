#!/bin/bash

dt=`date +"%Y%m%d"`
htmlprojectpath="/home/subham/subham/htmlProject"
htmlfilepath="/var/www/html/IND17_OEBU_PNL_STAT"

copy_log_file() {
	mkdir -p $htmlprojectpath/log
	scp dvcinfra@10.23.227.63:/home/dvcinfra/important/IND17_OEBU_STAT/log_$dt.txt $htmlprojectpath/log/

	if [ -f $htmlprojectpath/log/log_$dt.txt ]; then
		echo "copy log_$dt file success"
	else
		echo "copy log_$dt file failed"
		exit 1
	fi
}

generate_max_loss_html_file() {
	echo "<tr><td><a href=IND17Stat/IND17Stat.$dt.html style=color:blue>$dt</a></td><td>$1</td><td>$2</td><td>$3</td><td>$4</td></tr>" >> $htmlprojectpath/htmlFiles/html_body_max_loss.txt
	cat $htmlprojectpath/htmlFiles/html_head_max_loss.txt > $htmlfilepath/IND17_max_loss.html
	cat $htmlprojectpath/htmlFiles/html_body_max_loss.txt >> $htmlfilepath/IND17_max_loss.html
	cat $htmlprojectpath/htmlFiles/html_tail_max_loss.txt >> $htmlfilepath/IND17_max_loss.html

	if [ -f $htmlprojectpath/htmlFiles/html_body_max_loss.txt ] && [ -f $htmlfilepath/IND17_max_loss.html ]; then
		echo "generate max loss html file success"
	else
		echo "generete max loss html file failed"
	fi
}	

generate_html_file() {
	mkdir -p $htmlfilepath/IND17Stat
	chown root:root $htmlfilepath/IND17Stat
	cat $htmlprojectpath/htmlFiles/html_head_daily.txt >> $htmlfilepath/IND17Stat/IND17Stat.$dt.html
	maxloss=0
	product=""
	tradedvalue=0
	vv=0
	pnl=0
	flag=0

	filename="$htmlprojectpath/log/log_$dt.txt"
	IFS=' '
	while read -r f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 
	do
	if [ $flag -eq 0 ]; then
		maxloss=$f12
		product=${f1%=>}
		tradedvalue=$f5
		vv=$f9
		pnl=$f12
		flag=1
	fi

	echo "<tr><td>${f1%=>}</td><td>$f5</td><td>$f9</td><td>$f12</td></tr>" >> $htmlfilepath/IND17Stat/IND17Stat.$dt.html
	echo "<tr><td>${f1%=>}</td><td>$f5</td><td>$f9</td><td>$f12</td></tr>"

	if [ $f12 -lt $maxloss ]; then
		maxloss=$f12
		product=${f1%=>}
		tradedvalue=$f5
		vv=$f9
		pnl=$f12
	fi

	done <"$filename"

	cat $htmlprojectpath/htmlFiles/html_tail_daily.txt >> $htmlfilepath/IND17Stat/IND17Stat.$dt.html
	
	generate_max_loss_html_file $product $tradedvalue $vv $pnl
}

copy_log_file;

generate_html_file;

