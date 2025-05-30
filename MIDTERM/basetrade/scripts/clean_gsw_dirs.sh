#!/bin/bash

find /spare/local/$USER/GSW/*/* -type d -mtime +5 -exec rm -rf {} \;

find /spare/local/DailyTimedDataDir/ /spare/local/DailyRegDataDir /spare/local/RegDataDir -type f -atime +1 -exec rm -f {} \;
