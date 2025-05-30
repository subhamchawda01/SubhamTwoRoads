#!/bin/bash


years=(2017 2018 2019 2020 2021);
months=(01 02 03 04 05 06 07 08 09 10 11 12)
dir_path="/home/hardik/PNLProject"
# Monthly
 echo "Script Monthly"
 for year in "${years[@]}"; do
	 for month in "${months[@]}"; do 
	 $dir_path/get_top5_gainer_loser_product.sh MONTHLY $year$month
	echo $year$month
 	done 
 done
 echo "Script Yearly"
# yearly
 for year in "${years[@]}"; do 
 $dir_path/get_top5_gainer_loser_product.sh YEARLY $year
 echo $year
 done

echo "Script Ended"


