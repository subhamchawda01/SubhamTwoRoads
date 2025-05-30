#!/bin/bash
#args=("$@")

#1. Copies data to Temp
#2. Appends it to FUT_bkp/ OPT_bkp/ WEEKLY_OPT_bkp
#3. Sorts it datewise to Temp
#4. Sorts it uniquely to FUT_bkp/ OPT_bkp/ WEEKLY_OPT_bkp
#5. Copies data to FUT_BarData/ OPT_BarData/  Weekly_Options_Data/ Cash_Data/


#Update MIN_MAX prices first
rm /NAS1/data/NSEBarData/Archive/Temp/*

cd /home/dvctrader/midterm/Data_Generator
cp Sorted_Data/* /NAS1/data/NSEBarData/Archive/Temp/
cd /NAS1/data/NSEBarData/Archive/Temp
for i in *; do cat $i >> /NAS1/data/NSEBarData/Archive/FUT_bkp/$i; done
rm /NAS1/data/NSEBarData/Archive/Temp/*
cd /NAS1/data/NSEBarData/Archive/FUT_bkp
for i in *; do sort -n -k 1 $i > /NAS1/data/NSEBarData/Archive/Temp/$i; done
cd /NAS1/data/NSEBarData/Archive/Temp
for i in *; do sort -u $i > /NAS1/data/NSEBarData/Archive/FUT_bkp/$i; done
rm /NAS1/data/NSEBarData/Archive/Temp/*
cd /NAS1/data/NSEBarData/Archive/FUT_bkp
cp /NAS1/data/NSEBarData/Archive/FUT_bkp/* /NAS1/data/NSEBarData/FUT_BarData/
chown dvcinfra:infra /NAS1/data/NSEBarData/FUT_BarData/*


cd /home/dvctrader/midterm/Data_Generator
cp Sorted_Options_Data/* /NAS1/data/NSEBarData/Archive/Temp/
cd /NAS1/data/NSEBarData/Archive/Temp
for i in *; do cat $i >> /NAS1/data/NSEBarData/Archive/OPT_bkp/$i; done
rm /NAS1/data/NSEBarData/Archive/Temp/*
cd /NAS1/data/NSEBarData/Archive/OPT_bkp
for i in *; do sort -n -k 1 $i > /NAS1/data/NSEBarData/Archive/Temp/$i; done
cd /NAS1/data/NSEBarData/Archive/Temp
for i in *; do sort -u $i > /NAS1/data/NSEBarData/Archive/OPT_bkp/$i; done
rm /NAS1/data/NSEBarData/Archive/Temp/*
cd /NAS1/data/NSEBarData/Archive/OPT_bkp
cp /NAS1/data/NSEBarData/Archive/OPT_bkp/* /NAS1/data/NSEBarData/OPT_BarData/
chown dvcinfra:infra /NAS1/data/NSEBarData/OPT_BarData/*


cd /home/dvctrader/midterm/Data_Generator
cp Sorted_Weekly_Options_Data/* /NAS1/data/NSEBarData/Archive/Temp/
cd /NAS1/data/NSEBarData/Archive/Temp
for i in *; do cat $i >> /NAS1/data/NSEBarData/Archive/WEEKLY_OPT_bkp/$i; done
rm /NAS1/data/NSEBarData/Archive/Temp/*
cd /NAS1/data/NSEBarData/Archive/WEEKLY_OPT_bkp
for i in *; do sort -n -k 1 $i > /NAS1/data/NSEBarData/Archive/Temp/$i; done
cd /NAS1/data/NSEBarData/Archive/Temp
for i in *; do sort -u $i > /NAS1/data/NSEBarData/Archive/WEEKLY_OPT_bkp/$i; done
rm /NAS1/data/NSEBarData/Archive/Temp/*
cd /NAS1/data/NSEBarData/Archive/WEEKLY_OPT_bkp
cp /NAS1/data/NSEBarData/Archive/WEEKLY_OPT_bkp/* /NAS1/data/NSEBarData/Weekly_Options_Data/
chown dvcinfra:infra /NAS1/data/NSEBarData/Weekly_Options_Data/*

#cd /home/dvctrader/midterm/Data_Generator
#rsync Sorted_Cash_Data/* /NAS1/data/NSEBarData/Archive/Temp/
#cd /NAS1/data/NSEBarData/Archive/Temp
#for i in *; do cat $i >> /NAS1/data/NSEBarData/Archive/Cash_BarData_bkp/$i; done
#rm /NAS1/data/NSEBarData/Archive/Temp/*
#cd /NAS1/data/NSEBarData/Archive/Cash_BarData_bkp
#for i in *; do sort -n -k 1 $i > /NAS1/data/NSEBarData/Archive/Temp/$i; done
#cd /NAS1/data/NSEBarData/Archive/Temp
#for i in *; do sort -u $i > /NAS1/data/NSEBarData/Archive/Cash_BarData_bkp/$i; done
#rm /NAS1/data/NSEBarData/Archive/Temp/*
#cd /NAS1/data/NSEBarData/Archive/Cash_BarData_bkp
#cp /NAS1/data/NSEBarData/Archive/Cash_BarData_bkp/* /NAS1/data/NSEBarData/Cash_BarData/
#chown dvcinfra:infra /NAS1/data/NSEBarData/Cash_BarData/*

#cp /NAS1/data/NSEBarData/Archive/FUT_bkp/* /NAS1/data/NSEBarData/FUT_BarData/
#cp /NAS1/data/NSEBarData/Archive/OPT_bkp/* /NAS1/data/NSEBarData/OPT_BarData/
#cp /NAS1/data/NSEBarData/Archive/WEEKLY_OPT_bkp/* /NAS1/data/NSEBarData/Weekly_Options_Data/
