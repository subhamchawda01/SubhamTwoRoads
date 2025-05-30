rm  params1/*;
rm -r strats1/*;

for i in `cat strat1`; do strat=`ls ~/modelling/strats/*/*/$i` ; echo $strat; shc=`awk '{print $2}' $strat`; mkdir -p strats1/$shc; cp $strat strats1/$shc/ ; done

for i in strats1/*/*; do 
	shc=`awk '{print $2}' $i`; 
	param=`awk '{print $5}' $i`; 
	parambase=`basename $param`; 
	grep -v QUEUE_LONGEVITY_SUPPORT $param > params1/$parambase ; 
	awk -vnpf=$parambase '{$5="/home/archit/testing/hyster/params1/"npf; print $_;}' $i > ~/tmp_99; mv ~/tmp_99 $i; 
	keep=`~/basetrade_install/bin/print_paramset params1/$parambase 20141107 1 $shc | head -n1 | awk '{print $8}'`;
	for n in `seq 1 3`; do 
		long=`python -c "print $n * 0.25 * $keep"`;
		awk -vnpf=$parambase -vnu=$n '{$5="/home/archit/testing/hyster/params1/"npf"_"nu; print $_;}' $i >  "$i"_"$n"; 
		cp params1/$parambase params1/$parambase"_"$n; 
		echo "PARAMVALUE QUEUE_LONGEVITY_SUPPORT $long" >> params1/$parambase"_"$n;
	done; 
done

for shc in BR_DOL_0 TOPIX_0 VX_0 VX_1 VX_2 ; do echo $shc; ~/basetrade_install/bin/summarize_strategy_results $shc ~/testing/hyster/strats1/$shc/ ~/results1/ 20140101 20141201 > ~/tmp_summ; for i in `cat ~/testing/hyster/strat1 | grep -i $shc`; do str=`echo " $i | "$i"_[123] "`; egrep "$str" ~/tmp_summ | awk -vst=$i 'BEGIN{r=0} {if($2!=st && r==0){bpnl=$3; bvol=$5; r=1; bst=$2; cmd="~/a.sh "$2;}} {if($2==st){opnl=$3; ovol=$5;}} END{printf "%d %d %d %d %.2f %.2f ", opnl, bpnl, ovol, bvol, 100*(bpnl-opnl)/opnl, 100*(bvol-ovol)/ovol, bst; system(cmd)}'; done  > $shc; done
