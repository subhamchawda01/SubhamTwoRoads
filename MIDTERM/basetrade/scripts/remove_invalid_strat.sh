modelling="/home/dvctrader/modelling"
strats="strats"
pruned="pruned_strategies"
tmp_strat="/home/dvctrader/trash/tmp-strats.txt"
for shortcode in `ls $modelling/$strats`; do
	find $modelling/$strats/$shortcode -name '*core*' -type f -delete
	find $modelling/$strats/$shortcode -name '*~' -type f -delete
	find $modelling/$strats/$shortcode -type f > $tmp_strat
	while read line; do
			sed -i '/^[[:space:]]*$/d;s/[[:space:]]*$//; s/^[[:space:]]*//' $line #Remove blank lines from file
			res=`python ~/basetrade/scripts/is_strat_valid.py $line`
			exit_code=$?
			if [ $exit_code != 0 ]; then
				echo $line $res $exit_code
				file_path=${line#$modelling/$strats/}
				full_path=$modelling/$pruned/$file_path
				mkdir -p `dirname $full_path`
				mv $line $full_path
			fi
	done < $tmp_strat
done
