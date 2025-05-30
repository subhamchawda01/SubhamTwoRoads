#!/bin/bash

# Sleep for 30 seconds so that drives can get mounted.
sleep 30

# Jupyter Notebook
## Script is present in pengine
## Log File: /home/dvctrader/Jupyter_NoteBooks/jupyter_notebook_daemon_logs_hub.txt
## Before this drives need to be mounted
## Check cron for automount script
ssh dvctrader@10.23.5.69 "/home/dvctrader/juphub.sh"

# Metabase
## Script is present in /home/dvctrader/metabase
## Log File: /home/dvctrader/metabase/log_metabase_start_$(date +%F_%T).log
## Before this drives need to be mounted
## Check cron for automount script
ssh dvctrader@10.23.5.69 "/home/dvctrader/metabase/start_metabase.sh"
