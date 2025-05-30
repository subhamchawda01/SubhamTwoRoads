#!/bin/bash

echo "SYNC IND18"
rsync -avz /NAS1/data/NSEBarData/CASH_BarData/* dvctrader@10.23.227.83:/spare/local/CASH_BarData/
ssh dvctrader@10.23.227.83 "rsync -avz /spare/local/CASH_BarData/ dvctrader@10.23.227.82:/spare/local/CASH_BarData"
ssh dvctrader@10.23.227.83 "rsync -avz /spare/local/CASH_BarData/ dvctrader@10.23.227.81:/spare/local/CASH_BarData"
ssh dvctrader@10.23.227.83 "rsync -avz /spare/local/CASH_BarData/ dvctrader@10.23.227.72:/spare/local/CASH_BarData"
echo "SYNC IND17"
rsync -avz /NAS1/data/NSEBarData/CASH_BarData/* dvctrader@10.23.227.82:/spare/local/CASH_BarData/ 
echo "SYNC IND16"
rsync -avz /NAS1/data/NSEBarData/CASH_BarData/* dvctrader@10.23.227.81:/spare/local/CASH_BarData/
echo "SYNC IND23"
rsync -avz /NAS1/data/NSEBarData/CASH_BarData/* dvctrader@10.23.227.72:/spare/local/CASH_BarData/
#echo "SYNC IND24"
#rsync -avz /NAS1/data/NSEBarData/CASH_BarData/* dvctrader@10.23.227.74:/spare/local/CASH_BarData/
echo "SYNC WORKER"
rsync -avz /NAS1/data/NSEBarData/CASH_BarData/* dvctrader@54.90.155.232:/NAS4/CASH_BarData/
rsync -avz /NAS1/data/NSEBarData/CASH_BarData/* dvctrader@54.90.155.232:/NAS4/CASH_BarData/
rsync -avz /NAS1/data/NSEBarData/CASH_BarData/* dvctrader@44.202.186.243:/spare/local/CASH_BarData/
