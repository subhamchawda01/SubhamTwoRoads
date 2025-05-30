#!/usr/bin/env python

import datetime
import sys
import shutil

BACKUP_DIR_MYSQL='/media/shared/ephemeral16/db_backup/regular'
BACKUP_DIR_PSQL=''
BACKUP_DIR_MONGO=''

def remove_folders(folders_to_be_kept):
    dirs = os.listdir(BACKUP_DIR_MYSQL)
    for dir in dirs:
        if os.path.isdir(os.path.join(BACKUP_DIR_MYSQL, dir)):
            if dir not in folders_to_be_kept:
                shutil.rmtree(os.path.join(BACKUP_DIR_MYSQL, dir))


def clear_backups():
    curr_time = datetime.datetime.utcnow()
    curr_date = curr_time.date().strftime('%Y%m%d')
    folders_to_be_kept = []
    folders_to_be_kept.append(curr_date)
    for i in range(1,4):
        folders_to_be_kept.append((curr_time - datetime.timedelta(days=i)).date().strftime('%Y%m%d'))
    delta_days = 4
    month_weekend_start = curr_time - datetime.timedelta(days=delta_days)
    while True:
        if month_weekend_start.weekday() == 6:
            folders_to_be_kept.append(month_weekend_start.date().strftime('%Y%m%d'))
            month_weekend_start = month_weekend_start - timedelta(days=7)
        else :
            month_weekend_start = month_weekend_start - datetime.timedelta(days=month_weekend_start.weekday()+1)

        if len(folders_to_be_kept) == 12:
            break;
    remove_folders(folders_to_be_kept)



clear_backups()
