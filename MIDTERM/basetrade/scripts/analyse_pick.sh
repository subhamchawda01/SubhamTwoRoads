if [ $# -lt 7 ]; then echo "USAGE : $0 shc sd ed num_strats dir sort_alg sfreq hfreq=100 minvol=0"; exit; fi 

shc=$1; 
given_sd=$2; 
ed=$3; 
num_strats=$4;
dir=$5;
sort_algo=$6;
sfreq=$7;
hfreq=100;
mvol=0
if [ $# -gt 8 ]; then hfreq=$8; fi;
if [ $# -gt 9 ]; then mvol=$9; fi;
echo "shc=$shc given_sd=$given_sd ed=$ed num_strats=$num_strats dir=$dir sort_algo=$sort_algo sfreq=$sfreq hfreq=$hfreq mvol=$mvol";
uid=`date +%N`
tfile=tmp_"$uid";
ofile=out_"$uid";
sd=$ed;
summ_ed=`$HOME/basetrade_install/bin/calc_prev_week_day $sd 1`; 
summ_sd=`$HOME/basetrade_install/bin/calc_prev_week_day $sd $sfreq`; 
days=0;

rm -f $tfile;
rm -f $ofile;

while [ $sd -ge $given_sd ]; do
    days=`expr $days + 1`;
    $HOME/basetrade_install/bin/summarize_strategy_results $shc $dir $HOME/ec2_globalresults $summ_sd $summ_ed INVALIDFILE $sort_algo | grep STRAT | awk -vvol=$mvol '{if($5>vol){print $2}}' | head -n"$num_strats" > $tfile ; 
    #echo $sd;
    #cat $tfile;
    for i in `cat $tfile`; do 
	$HOME/basetrade_install/bin/summarize_strategy_results $shc $dir $HOME/ec2_globalresults $sd $sd | awk -vstrat=$i '{if($2==strat){print NR, $_}}' >> $ofile; 
    done; 
    sd=$summ_ed; 
    summ_ed=`$HOME/basetrade_install/bin/calc_prev_week_day $sd 1`; 
    summ_sd=`$HOME/basetrade_install/bin/calc_prev_week_day $sd $sfreq`;  
done 
rm -f $tfile;

echo "TDAYS:" $days;
awk -vnum=$num_strats 'BEGIN{pnl=0; vol=0} {pnl+=$4; vol+=$6} END{print "ST AVGPNL,VOL:", num*(pnl/NR), num*(vol/NR)}' $ofile
rm -f $ofile;

summ_ed=`$HOME/basetrade_install/bin/calc_prev_week_day $given_sd 1`; 
summ_sd=`$HOME/basetrade_install/bin/calc_prev_week_day $given_sd $hfreq`; 

$HOME/basetrade_install/bin/summarize_strategy_results $shc $dir $HOME/ec2_globalresults $summ_sd $summ_ed INVALIDFILE $sort_algo | grep STRAT | awk -vvol=$mvol '{if($5>vol){print $2}}' | head -n"$num_strats" > $tfile;
#echo "LT"
#cat $tfile;
for i in `cat $tfile`; do 
    $HOME/basetrade_install/bin/summarize_strategy_results $shc $dir $HOME/ec2_globalresults $given_sd $ed | awk -vstrat=$i '{if($2==strat){print NR, $_}}' ; 
done > $ofile"1"
rm -f $tfile;

awk -vnum=$num_strats 'BEGIN{pnl=0; vol=0} {pnl+=$4; vol+=$6} END{print "LT AVGPNL,VOL:", num*(pnl/NR), num*(vol/NR)}' $ofile"1"
rm -f $ofile"1";
