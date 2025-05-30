#!/bin/bash

## Prequirement dvccode,infracore,baseinfra,tradeenigne,basetrader should exist in the below folder
##mkdir -p important/DAILY_BUILD_JOBS_EXEC


mkdir -p ~/important/DAILY_BUILD_JOBS_EXEC
mkdir -p ~/important/DAILY_BUILD_JOBS_EXEC/stable_execs
for i in dvccode infracore baseinfra tradeengine dvctrade basetrade; do 
	cd ~/important/DAILY_BUILD_JOBS_EXEC/$i;
	echo "------------------------------Currently Building `pwd`------------------------------"
	git pull 
	bjam -j8 release
done


cd ~/important/DAILY_BUILD_JOBS_EXEC/cvquant_install
for i in dvccode infracore baseinfra tradeengine dvctrade basetrade; do
	cd ~/important/DAILY_BUILD_JOBS_EXEC/cvquant_install/$i/bin;
	echo "------------------------------Coping Execs `pwd`------------------------------"
 	rsync -avz * ~/important/DAILY_BUILD_JOBS_EXEC/stable_execs/
done

rsync -avz ~/important/DAILY_BUILD_JOBS_EXEC 52.90.0.239:~/important/


