#!/bin/bash

today=`date +%Y%m%d`;

rsync -avz --progress dvctrader@52.3.22.99:/spare/local/NseHftFiles /home/dvctrader/usarraf

echo "STARTING SYNC TO SERVERS" ;

for i in 64 65 69 81 82 83 84; 
do 

echo "rsync -avz --progress /home/dvctrader/usarraf/NseHftFiles dvctrader@10.23.227.${i}:/spare/local" 

rsync -avz --progress /home/dvctrader/usarraf/NseHftFiles dvctrader@10.23.227.${i}:/spare/local 

done

echo "SERVER SYNC DONE" ;

rsync -avz dvctrader@52.3.22.99:/spare/local/BarData /home/dvctrader/usarraf
rsync -avz /home/dvctrader/usarraf/BarData dvctrader@10.23.227.69:/spare/local
rsync -avz /home/dvctrader/usarraf/BarData dvctrader@10.23.227.84:/spare/local

