if [ $# -eq 0 ];then
	echo "Expect date arguement IN yyyymmdd or TODAY";
	exit;
fi 
if [ "$1" == "TODAY" ];then
	Date=`date +"%Y%m%d"`;
else
	Date=$1;
fi

TIMESTAMP=`date -d "$Date" "+%s"`;
LatencyReportsDir='/var/www/html/LatencyReports/'
LatencyReportsAnalysisDir='/var/www/html/LatencyReportsAnalysis/'
Servers="IND14 IND15 IND16 IND17 IND18 IND19 IND20";
#get last 120 days get for each machine
cd ${LatencyReportsDir};
for server in ${Servers};
do
	cat ${server}.index.html | grep "<tr><td>202" | awk -F"<td>" '{print $2" "$5" "$6" "$7" "$8" "$9" "$10" "$12}' | sed -r '/^\s*$/d' \
	| sed 's/<\/td>//g' | awk '{if (NF>7) {print $0}}' | sort -nrk 1 | grep -A120 ${Date} > ${LatencyReportsAnalysisDir}/${server}.datapoints.txt.tmp;
	for lines in `cat  ${LatencyReportsAnalysisDir}/${server}.datapoints.txt.tmp| tr ' ' '~'`; do t=`echo $lines | awk -F"~" '{print $1}'`; stime=`date -d $t "+%s"`; echo $stime" "$lines; done\
	| tr '~' ' '  | awk '{if($6 < 5) {print $1" "$3" "$4" "$5" "$6" " $7" "$8" "$9" "$10}}' > ${LatencyReportsAnalysisDir}/${server}.datapoints.txt; 
        rm -rf ${LatencyReportsAnalysisDir}/${server}.datapoints.txt.tmp;
done
#now generate the graph for the datapoints
mkdir -p ${LatencyReportsAnalysisDir}/${Date};
cd ${LatencyReportsAnalysisDir};
for server in ${Servers};
do
	GNUPLOT='set terminal jpeg size 1366,768; set title "MIN";set output "'$LatencyReportsAnalysisDir'/'$Date'/'$server'.min.jpeg";set xdata time;set timefmt "%s";plot "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:2 with lines notitle, "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:2 with lines smooth bezier notitle;'
	echo $GNUPLOT | gnuplot;
	GNUPLOT='set terminal jpeg size 1366,768; set title "1TH PERCENTILE";set output "'$LatencyReportsAnalysisDir'/'$Date'/'$server'.1th.jpeg";set xdata time;set timefmt "%s";plot "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:3 with lines notitle, "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:3 with lines smooth bezier notitle;'
	echo $GNUPLOT | gnuplot;
	GNUPLOT='set terminal jpeg size 1366,768; set title "10TH PERCENTILE";set output "'$LatencyReportsAnalysisDir'/'$Date'/'$server'.10th.jpeg";set xdata time;set timefmt "%s";plot "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:4 with lines notitle, "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:4 with lines smooth bezier notitle;'
	echo $GNUPLOT | gnuplot;
	GNUPLOT='set terminal jpeg size 1366,768; set title "25TH PERCENTILE";set output "'$LatencyReportsAnalysisDir'/'$Date'/'$server'.25th.jpeg";set xdata time;set timefmt "%s";plot "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:5 with lines notitle, "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:5 with lines smooth bezier notitle;'
	echo $GNUPLOT | gnuplot;
	GNUPLOT='set terminal jpeg size 1366,768; set title "MEADIAN";set output "'$LatencyReportsAnalysisDir'/'$Date'/'$server'.median.jpeg";set xdata time;set timefmt "%s";plot "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:6 with lines notitle, "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:6 with lines smooth bezier notitle;'
	echo $GNUPLOT | gnuplot;
	GNUPLOT='set terminal jpeg size 1366,768; set title "75TH PERCENTILE";set output "'$LatencyReportsAnalysisDir'/'$Date'/'$server'.75th.jpeg";set xdata time;set timefmt "%s";plot "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:7 with lines notitle, "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:7 with lines smooth bezier notitle;'
	echo $GNUPLOT | gnuplot;
	GNUPLOT='set terminal jpeg size 1366,768; set title "90TH PERCENTILE";set output "'$LatencyReportsAnalysisDir'/'$Date'/'$server'.90th.jpeg";set xdata time;set timefmt "%s";plot "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:8 with lines notitle, "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:8 with lines smooth bezier notitle;'
	echo $GNUPLOT | gnuplot;
	GNUPLOT='set terminal jpeg size 1366,768; set title "ALL";set output "'$LatencyReportsAnalysisDir'/'$Date'/'$server'.ALL.jpeg";set xdata time;set timefmt "%s";plot "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:2 title "MIN" with lines,  "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:2 with lines smooth bezier notitle, "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:3 title "1%" with lines,  "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:3 with lines smooth bezier notitle, "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:4 title "10%" with lines,  "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:4 with lines smooth bezier notitle, "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:5 title "25%" with lines,  "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:5 with lines smooth bezier notitle, "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:6 title "50%" with lines,  "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:6 with lines smooth bezier notitle, "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:7 title "75%" with lines,  "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:7 with lines smooth bezier notitle, "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:8 title "90%" with lines, "'$LatencyReportsAnalysisDir'/'$server'.datapoints.txt" using 1:8 with lines smooth bezier notitle;'
	echo $GNUPLOT | gnuplot;
	
done

#generate graph for only cash
GNUPLOT='set terminal jpeg size 1366,768; set title "CAPITAL MARKET";set output "'$LatencyReportsAnalysisDir'/'$Date'/cash.median.jpeg";set xdata time;set timefmt "%s";plot "'$LatencyReportsAnalysisDir'/IND16.datapoints.txt" using 1:6 title "IND16" with lines, "'$LatencyReportsAnalysisDir'/IND16.datapoints.txt" using 1:6 with lines smooth bezier notitle, "'$LatencyReportsAnalysisDir'/IND17.datapoints.txt" using 1:6 title "IND17" with lines, "'$LatencyReportsAnalysisDir'/IND17.datapoints.txt" using 1:6 with lines smooth bezier notitle, "'$LatencyReportsAnalysisDir'/IND18.datapoints.txt" using 1:6 title "IND18" with lines, "'$LatencyReportsAnalysisDir'/IND18.datapoints.txt" using 1:6 with lines smooth bezier notitle;'
echo $GNUPLOT | gnuplot;

#generate graph for fut
GNUPLOT='set terminal jpeg size 1366,768; set title "DERIVATIVES";set output "'$LatencyReportsAnalysisDir'/'$Date'/fut.median.jpeg";set xdata time;set timefmt "%s";plot "'$LatencyReportsAnalysisDir'/IND14.datapoints.txt" using 1:6 title "IND14" with lines, "'$LatencyReportsAnalysisDir'/IND14.datapoints.txt" using 1:6 with lines smooth bezier notitle, "'$LatencyReportsAnalysisDir'/IND15.datapoints.txt" using 1:6 title "IND15" with lines, "'$LatencyReportsAnalysisDir'/IND15.datapoints.txt" using 1:6 with lines smooth bezier notitle, "'$LatencyReportsAnalysisDir'/IND19.datapoints.txt" using 1:6 title "IND19" with lines, "'$LatencyReportsAnalysisDir'/IND19.datapoints.txt" using 1:6 with lines smooth bezier notitle, "'$LatencyReportsAnalysisDir'/IND20.datapoints.txt" using 1:6 title "IND20" with lines, "'$LatencyReportsAnalysisDir'/IND20.datapoints.txt" using 1:6 with lines smooth bezier notitle;'
echo $GNUPLOT | gnuplot;


cd ${LatencyReportsAnalysisDir}/${Date};
graphs_count=`ls  | wc -l`;
if [ $graphs_count -eq 0 ];then
	exit 0;
fi

#append todays generated data to historicalfile;
grep -v $Date ${LatencyReportsAnalysisDir}/historical_data.txt > ${LatencyReportsAnalysisDir}/historical_data.txt_tmp;
mv ${LatencyReportsAnalysisDir}/historical_data.txt_tmp ${LatencyReportsAnalysisDir}/historical_data.txt;
#add generated
data_to_append="<tr><td>$Date</td><td><a href=\"$Date/IND14.median.jpeg\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td><td><a href=\"$Date/IND15.median.jpeg\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td><td><a href=\"$Date/IND16.median.jpeg\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td><td><a href=\"$Date/IND17.median.jpeg\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td><td><a href=\"$Date/IND18.median.jpeg\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td><td><a href=\"$Date/IND19.median.jpeg\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td><td><a href=\"$Date/IND20.median.jpeg\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td><td><a href=\"$Date/cash.median.jpeg\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td><td><a href=\"$Date/fut.median.jpeg\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td></tr>"
echo $data_to_append >> ${LatencyReportsAnalysisDir}/historical_data.txt;

for server in $Servers;
do
	grep -v $Date ${LatencyReportsAnalysisDir}$server.historical_data.txt > ${LatencyReportsAnalysisDir}historical_data.txt_tmp;
	mv ${LatencyReportsAnalysisDir}/historical_data.txt_tmp ${LatencyReportsAnalysisDir}$server.historical_data.txt
	data_to_append="<tr>
	<td>$Date</td>
	<td><a href=\"$Date/$server.min.jpeg\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td>
	<td><a href=\"$Date/$server.1th.jpeg\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td>
	<td><a href=\"$Date/$server.10th.jpeg\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td>
	<td><a href=\"$Date/$server.25th.jpeg\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td>
	<td><a href=\"$Date/$server.median.jpeg\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td>
	<td><a href=\"$Date/$server.75th.jpeg\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td>
	<td><a href=\"$Date/$server.90th.jpeg\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td>
	<td><a href=\"$Date/$server.ALL.jpeg\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td>
	</tr>"
	echo $data_to_append >> ${LatencyReportsAnalysisDir}/$server.historical_data.txt;
done

#now generate html pages
cat ${LatencyReportsAnalysisDir}/main_page_header.txt > ${LatencyReportsAnalysisDir}index.html
cat ${LatencyReportsAnalysisDir}/historical_data.txt >> ${LatencyReportsAnalysisDir}index.html
cat ${LatencyReportsAnalysisDir}/main_page_footer.txt >> ${LatencyReportsAnalysisDir}index.html

for server in $Servers;
do 
	cat $LatencyReportsAnalysisDir/machines_pages_header.txt > $LatencyReportsAnalysisDir/$server.index.html
	cat $LatencyReportsAnalysisDir/$server.historical_data.txt >> $LatencyReportsAnalysisDir/$server.index.html
	cat $LatencyReportsAnalysisDir/machines_pages_footer.txt >>  $LatencyReportsAnalysisDir/$server.index.html;
done

