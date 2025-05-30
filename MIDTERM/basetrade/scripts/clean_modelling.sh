#!/bin/bash

#creating backup of modelling directory

#rsync -az /spare1/shared/modelling dvcinfra@10.23.74.41:/logs2/apps/logs/ 

work_dir="/tmp/work_dir"
mkdir -p $work_dir;
for prod in `ls /spare1/shared/modelling/models | grep -v NSE`; 
do 
	echo "Getting normal strats for $prod...";
	ls /spare1/shared/modelling/strats/$prod/*/* > $work_dir/strats_$prod;
	ls /spare1/shared/modelling/strats/$prod/EBT/*/* >> $work_dir/strats_$prod;
	ls /spare1/shared/modelling/staged_strats/$prod/*/* >> $work_dir/strats_$prod;

	echo "Getting all in use models for $prod...";
	#get model for all strategies
	>$work_dir/model_$prod;
	for strat in `cat $work_dir/strats_$prod`;
	do
		cat $strat | awk '{ print $4}' >> $work_dir/model_$prod;
		cat $strat | awk '{ print $5}' >> $work_dir/param_$prod;
	done
	
	echo "Getting all stir_strats and models for product .."
	#models corresponding to stir strats
	ls /spare1/shared/modelling/stir_strats/$prod/*/* > $work_dir/init_strat_$prod;
	for strat in `cat $work_dir/init_strat_$prod`; 
	do
		im_strat=`cat $strat`;
		grep STRATEGYLINE $im_strat | awk '{ print $3}' >> $work_dir/model_$prod;
		grep STRATEGYLINE $im_strat | awk '{ print $4}' >> $work_dir/param_$prod;
	done

	echo "Adding params from regime $params"
	for reg_param in `grep regm_ $work_dir/param_$prod`;
	do
		grep PARAMFILELIST $reg_param | awk '{ print $2}' >> $work_dir/param_$prod;
	done

	cat $work_dir/param_$prod | sort | uniq | sed 's/\/home\/dvctrader/\/spare1\/shared/g' > $work_dir/tmp; mv $work_dir/tmp $work_dir/param_$prod;
	cat $work_dir/model_$prod | sort | uniq | sed 's/\/home\/dvctrader/\/spare1\/shared/g' > $work_dir/tmp; mv $work_dir/tmp $work_dir/model_$prod;

	echo "Getting all existing models .. "
	#all existing models
	ls /spare1/shared/modelling/models/$prod/*/* > $work_dir/existing_models_$prod;
	ls /spare1/shared/modelling/models/$prod/*/*/* >> $work_dir/existing_models_$prod;
	
	echo "Getting all existing params ..."
	ls /spare1/shared/modelling/params/$prod/* > $work_dir/existing_params_$prod;

	grep -vf $work_dir/model_$prod $work_dir/existing_models_$prod > $work_dir/unused_model_$prod;
		
	echo "Removing models";
	for file in `cat $work_dir/unused_model_$prod`;
	do
		rm $file;
	done

	grep -vf $work_dir/param_$prod $work_dir/existing_params_$prod > $work_dir/unused_param_$prod;
		
	echo "Removing $i";
	for file in `cat $work_dir/unused_param_$prod`;
	do
		rm $file;
	done
	
	#break
	
	echo "Removing Temporary files for $prod ...."
	rm $work_dir/*$prod
	
	echo "Removing Pruned Strategies for $prod ..."
	rm -r /spare1/shared/modelling/pruned_strategies/$prod;
	rm -r /spare1/shared/modelling/staged_pruned_strategies/$prod;
	rm -r /spare1/shared/modelling/pruned_params/$prod;
	rm -r /spare1/shared/modelling/pruned_stratwork/$prod;
done

