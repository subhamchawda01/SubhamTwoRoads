#!/bin/bash

rsync -avz 172.18.244.107:/spare/local/files/RTS/tickvalues /spare/local/files/RTS
rsync -avz 10.23.241.2:/spare/local/files/RTS/tickvalues /spare/local/files/RTS 

for machines in 10.23.74.52 10.23.74.53 10.23.74.54 10.23.74.55
do

  rsync -avz /spare/local/files/RTS/tickvalues $machines:/spare/local/files/RTS 

done 

ssh dvctrader@10.23.74.52 'rsync -ravz --timeout=60 /spare/local/files/. 54.208.92.178:/mnt/sdf/spare_local_files/' 
