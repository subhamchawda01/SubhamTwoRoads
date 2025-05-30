#!/bin/bash

echo "From Worker"
rsync -avz 52.90.0.239:/home/dvctrader/rahul/simr_tradeengine/CONFIG_FUT0/SIMR_CONFIG /home/dvctrader/ATHENA
echo "To Ind"
rsync -avz /home/dvctrader/ATHENA/SIMR_CONFIG 10.23.227.69:/home/dvctrader/ATHENA/SIMR_LIVE/CONFIG_FUT0/
rsync -avz /spare/local/MidtermStrat/ 52.90.0.239:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.64:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.65:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.69:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.84:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.72:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.63:/spare/local/MidtermStrat

