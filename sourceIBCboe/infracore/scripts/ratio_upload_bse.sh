#!/bin/bash

today=`date +%Y%m%d`;

rsync -avz --progress dvctrader@44.202.186.243:/spare/local/BseHftFiles /home/dvctrader/usarraf

echo "STARTING SYNC TO SERVERS" ;

for i in 64 65 69 81 82 83 84 71 72 66 74; 
do 

echo "rsync -avz --progress /home/dvctrader/usarraf/BseHftFiles dvctrader@10.23.227.${i}:/spare/local" 

rsync -avz --progress /home/dvctrader/usarraf/BseHftFiles dvctrader@10.23.227.${i}:/spare/local 

done

for i in 11 12;
do
echo "rsync -avz --progress /home/dvctrader/usarraf/BseHftFiles dvctrader@192.168.132.${i}:/spare/local" 
rsync -avz --progress /home/dvctrader/usarraf/BseHftFiles dvctrader@192.168.132.${i}:/spare/local
done


rsync -avz --progress /home/dvctrader/usarraf/BseHftFiles dvctrader@10.23.5.62:/spare/local
rsync -avz --progress /home/dvctrader/usarraf/BseHftFiles dvctrader@10.23.5.43:/spare/local
rsync -avz --progress /home/dvctrader/usarraf/BseHftFiles dvctrader@10.23.5.66:/spare/local
echo "SERVER SYNC DONE" ;


