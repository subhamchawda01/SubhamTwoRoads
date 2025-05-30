#!/bin/bash

USAGE="USAGE : <script> <shortcode> <ilist> <end date> <num dates> <start time> <end time> <datagen msecs=1000> <events=0> <trades=0> <print on eco=0> <pred duration/upper_threshold=100> <predalgo/lower_threshold=na_e3> <filter=fsg1> <chunk_size=100> <new_regdata = 0> <output_file=/spare/local/user/res_uniq_id/correlation_uniq_id> <work_dir=/spare/local/user/res_uniq_id/>" ;

num_arg=$#

if [ $num_arg -lt 6 ] ; then
echo $USAGE;
exit 0;
fi

if [ $num_arg -gt 6 ] ; then
datagen_msecs=$7 ;
else
datagen_msecs=1000 ;
fi

if [ $num_arg -gt 7 ] ; then
events=$8 ;
else
events=0 ;
fi

if [ $num_arg -gt 8 ] ; then
trades=$9 ;
else
trades=0 ;
fi

if [ $num_arg -gt 9 ] ; then
print_on_eco=${10} ;
else
print_on_eco=0 ;
fi

if [ $num_arg -gt 10 ] ; then
pred_duration=${11} ;
else
pred_duration=100 ;
fi


if [ $num_arg -gt 11 ] ; then
pred_algo=${12} ;
else
pred_algo="na_e3"
fi

if [ $num_arg -gt 12 ] ; then
filter=${13} ;
else
filter="fsg1"
fi

if [ $num_arg -gt 13 ] ; then
chunk_size=${14} ;
else
chunk_size=100;
fi

if [ $num_arg -gt 14 ] ; then
new_regdata=${15} ;
else
new_regdata=0;
fi

temp=`date +%s%N`;

if [ $num_arg -gt 15 ] ; then
cor_result_filename=${16} ;
else
user=`whoami`;
if [ -d "/spare/local/$user" ] ; then
cor_result_filename="/spare/local/$user/res_$temp/correlation_$temp" ;
else
mkdir /spare/local/$user ;
cor_result_filename="/spare/local/$user/res_$temp/correlation_$temp" ;
fi
fi


if [ $num_arg -gt 16 ] ; then
work_dir=${17} ;
else
user=`whoami`;
if [ -d "/spare/local/$user" ] ; then
work_dir="/spare/local/$user/res_$temp/" ;
else
mkdir /spare/local/$user ;
work_dir="/spare/local/$user/res_$temp/" ;
fi
fi

mkdir $work_dir ;

ilist=$2 ;
cp $ilist $work_dir/indicator_list ;
filename=$work_dir/indicator_list ; # all indicator list

num_indicators=`cat $filename | wc -l` ;

echo -n "" > $cor_result_filename ;

reg_data_script="/home/dvctrader/basetrade/scripts/get_regdata.py";
if [ $new_regdata -gt 0 ] ; then
reg_data_script="/home/dvctrader/basetrade/scripts/get_regdata_new.R";
else
reg_data_script="/home/dvctrader/basetrade/scripts/get_regdata.py";
fi

temp=`expr $chunk_size - 1 + $num_indicators` ;
num_loop=`expr $temp / $chunk_size` ;
shortcode=$1 ;
ilist_temp=$work_dir/ilist_temp ;

for i in `seq 1 $num_loop` ; do

echo "MODELINIT DEPBASE $shortcode OnlineMixPrice OnlineMixPrice" > $ilist_temp ;
echo "MODELMATH LINEAR CHANGE" >> $ilist_temp ;
echo "INDICATORSTART" >> $ilist_temp ;

cat $filename | head -$chunk_size >> $ilist_temp ;
echo "INDICATOREND" >> $ilist_temp ;

#cat $ilist_temp ;

cat $filename | awk -va=$chunk_size '{if(NR > a) print $0}' > $work_dir/tt ; mv $work_dir/tt $filename ;

$reg_data_script $1 $ilist_temp $3 $4 $5 $6 $datagen_msecs $events $trades $print_on_eco $pred_duration $pred_algo $filter $work_dir >/dev/null

~/basetrade_install/bin/get_dep_corr $work_dir/filtered_regdata_filename | sed 's/ /\n/g' > $work_dir/cor_$i ; rm $work_dir/catted_regdata_outfile; rm $work_dir/filtered_regdata_filename ; rm $work_dir/t_dgen_outfile ; rm $work_dir/t_regdata_outfile ;

cat $ilist_temp | awk '{if($1=="INDICATOR") print $0}' > $work_dir/tt ; mv $work_dir/tt $ilist_temp ;

paste $work_dir/cor_$i $ilist_temp >>  $cor_result_filename ;
rm $work_dir/cor_$i ;

done

#rm -r $work_dir
echo $work_dir ;
rm $ilist_temp
