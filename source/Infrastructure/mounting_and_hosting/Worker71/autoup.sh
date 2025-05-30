#!/bin/bash

# Sleep for 30 seconds so that drives can get mounted.
sleep 30

# Jupyter Notebook
## Script is present in pengine
## Log File: /home/dvctrader/Jupyter_NoteBooks/jupyter_notebook_daemon_logs_hub.txt
## Before this drives need to be mounted
## Check cron for automount script
/home/pengine/prod/live_scripts/dvc_jupyter_daemon_localworker_hub_570.sh

# Metabase
## Script is present in /root/
## Log File: /tmp/metabase_2
## Before this drives need to be mounted
## Check cron for automount script
/root/restart_metabase.sh
