#!/bin/bash

echo "Syncing Fut Bar data from 66"
######                  rsync -avz 10.23.5.66:/NAS1/data/NSEBarData/FUT_BarData_Adjusted /spare/local/MDSlogs
rsync -avz --progress dvctrader@44.202.186.243:/spare/local/BarData/ /spare/local/MDSlogs/FUT_BarData_Adjusted/ --delete-after
echo "Syncing Bar data from 42"
rsync -avz 10.23.5.42:/run/media/dvcinfra/BACKUP2/MediumTerm /spare/local/MDSlogs
rsync -avz 10.23.5.42:/run/media/dvcinfra/BACKUP2/MediumTermOptions /spare/local/MDSlogs
rsync -avz 10.23.5.42:/run/media/dvcinfra/BACKUP2/MediumTermWeeklyOptions /spare/local/MDSlogs

echo "Syncing Bar data from corporate action"
rsync -avz --progress 10.23.5.66:/NAS1/data/NSEMidTerm/MachineReadableCorpAdjustmentFiles /spare/local/files/NSE/MidTermData


ssh dvctrader@10.23.5.69 "/home/pengine/prod/live_scripts/sync_all_midterm_simulation_data_dvctrader.sh"
