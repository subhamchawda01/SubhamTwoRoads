#!/bin/bash


USAGE="USAGE : <script> <shortcode> <ilist> <end date> <num dates> <start time> <end time> <datagen msecs=1000> <events=0> <trades=0> <print on eco=0> <pred duration=900> <predalgo=na_e3> <filter=fsg1> <num_indicator=2> <work dir=/spare/local/user/res_uniq_id/>"

num_arg=$#

reg_data_script="/home/dvctrader/basetrade/scripts/get_regdata.py";
get_num_regimes_script="/home/dvctrader/basetrade_install/GenPerlLib/get_num_regimes.pl"

if [ $num_arg -lt 6 ] ; then 
echo $USAGE;
exit 0;
fi

if [ $num_arg -gt 13 ] ; then
num_indicator=${14} ;
else
num_indicator=2
fi

if [ $num_arg -gt 14 ] ; then
work_dir=${15} ;
else
temp=`date +%s%N`;
user=`whoami`;
if [ -d "/spare/local/$user" ] ; then
work_dir="/spare/local/$user/res_$temp/" ;
else 
mkdir /spare/local/$user ;
work_dir="/spare/local/$user/res_$temp/" ;
fi
fi

if [ $num_arg -gt 12 ] ; then
filter=${13} ;
else
filter="fsg1"
fi

if [ $num_arg -gt 11 ] ; then
pred_algo=${12} ;
else
pred_algo="na_e3"
fi

if [ $num_arg -gt 10 ] ; then
pred_duration=${11} ;
else
pred_duration=900 ;
fi

if [ $num_arg -gt 9 ] ; then
print_on_eco=${10} ;
else
print_on_eco=0 ;
fi

if [ $num_arg -gt 8 ] ; then
trades=$9 ;
else
trades=0 ;
fi

if [ $num_arg -gt 7 ] ; then
events=$8 ;
else
events=0 ;
fi

if [ $num_arg -gt 6 ] ; then
datagen_msecs=$7 ;
else
datagen_msecs=1000 ;
fi


num_regime=`cat $2 | grep -v "#INDICATOR " | grep "INDICATOR " | wc -l` ;
num_regime=`expr $num_regime - $num_indicator`;

reg_indicators=();
for x in `cat $2 | grep -v "#INDICATOR " | grep "INDICATOR " | head -$num_regime | awk '{print $3}'`;do reg_indicators+=($x);done

rm -r $work_dir
echo "$reg_data_script $1 $2 $3 $4 $5 $6 $datagen_msecs $events $trades $print_on_eco $pred_duration $pred_algo $filter $work_dir"  ;
$reg_data_script $1 $2 $3 $4 $5 $6 $datagen_msecs $events $trades $print_on_eco $pred_duration $pred_algo $filter $work_dir >/dev/null
#./get_regdata.R $1 $2 $3 $4 $5 $6 1000 0 0 1 900 na_e3 fsg1 res/

cd $work_dir
total_lines=`cat filtered_regdata_filename | wc -l`
num_col=`cat filtered_regdata_filename | head -1 | awk '{print NF}'` ;
start_index=`expr $num_col - $num_indicator + 1` ;
cut -d " " -f1,"$start_index"- filtered_regdata_filename > temp
#cat filtered_regdata_filename | awk -va=$num_indicator '{print $1, $(NF-1), $NF}' > temp ;
echo -n "Original Correlation : "; ~/basetrade_install/bin/get_dep_corr temp ;


for i in `seq 1 $num_regime`
do
    echo "Regime $i"
    t=`expr $i + 1`
    echo -n "Data Split : "
    cat filtered_regdata_filename | awk -va=$t -vb=$total_lines 'BEGIN{s=0;}{if($a == 1)s = s+1} END{print s*100/b , 100-s*100/b}'
    cat filtered_regdata_filename | awk -va=$t '{print $a}' | uniq | wc -l | awk -va=$4 '{print "No. of times Regime switches : ", $1/a - 1}';
    
    ind_indx=`expr $i - 1`
    indc=${reg_indicators[$ind_indx]}
    num_regimes_for_regime=`perl -e "require \"$get_num_regimes_script\"; print GetNumRegimesFromRegimeInd(\"$indc\", 5, \"$indc\").\"\n\";"`
    for j in `seq 1 $num_regimes_for_regime`
    do
        cat filtered_regdata_filename | awk -va=$t -vb=$j '{if($a==b)print $0}' | cut -d " " -f1,"$start_index"- > temp
        #cat filtered_regdata_filename | awk -va=$t '{if($a==1)print $1, $(NF-1), $NF}' > temp
        echo -n "Regime $j corr : "
        ~/basetrade_install/bin/get_dep_corr temp
    done
done

rm temp

cd ..
rm -r $work_dir


