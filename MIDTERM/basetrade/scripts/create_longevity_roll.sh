for shc in `cat $1`; do 
	for strat in `ls /home/dvctrader/modelling/strats/$shc/*/* | egrep -v 'w_r_|reg'`; do 
		param=`awk '{print $5}' $strat`; 
		if [ `grep UNIT_TRADE_SIZE $param | wc -l` -gt 0 ] ; then 
			parambase=`basename $param`
			paramlist="/home/dvctrader/modelling/stratwork/$shc/longevity_paramlist_"$parambase;
			echo $param > $paramlist;
			grep -v QUEUE_LONGEVITY_SUPPORT $param > "$param"_L0;
            long=`grep QUEUE_LONGEVITY_SUPPORT $param | awk '{print $NF}'`;
            if [ -z $long ] ; then long=0 ; fi;
			for val in `cat $2`; do
				newparam="$param"_L"$val";
				cp "$param"_L0 $newparam; 
                newlong="0."$val;
				echo "PARAMVALUE QUEUE_LONGEVITY_SUPPORT $val" >> $newparam;

				if [ `expr $long '=' $newlong` -eq 0 ]; then
					echo $newparam >> $paramlist;
				else
					rm $newparam;
				fi;
			done;
			newparam=$param"_L0"
			if [ `expr $long '=' 0` -eq 0 ]; then
					echo $newparam >> $paramlist;
				else
					rm $newparam
			fi;
			stratbase=`basename $strat`;
			echo "/home/dvctrader/basetrade_install/ModelScripts/install_rolling_strat.pl $stratbase TODAY-1 100 0 $paramlist 2"
		fi;
	done; 
done
