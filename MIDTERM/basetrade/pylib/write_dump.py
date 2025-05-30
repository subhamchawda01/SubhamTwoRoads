#!/usr/bin/env python

import sys
import time
import os
from single_process import single_process

import MySQLdb


@single_process
def take_snapshot():
    args = sys.argv

    MIN_ARG_LEN = 4

    if len(args) < MIN_ARG_LEN:
        sys.stderr.write("Insufficient arguments - <host> <dbname> <type> required\n")
        sys.exit()

    host = args[1]
    db_name = args[2]
    dump_type = args[3]

    BASE_PATH = ''

    if dump_type == 'adhoc':
        BASE_PATH = '/media/ephemeral16/db_backup/adhoc/'
    elif dump_type == 'regular':
        BASE_PATH = '/media/ephemeral16/db_backup/regular/'
    else:
        sys.stderr.write("Backup type should be adhoc or regular\n")
        sys.exit()

    tables = []

    user = 'dvcwriter'
    passwd = 'f33du5rB'

    # if list of tables provided in args, then dump only these else dump all tables for the db
    """    if len(args) > MIN_ARG_LEN:
        tables = args[MIN_ARG_LEN:]
    else:
        db = MySQLdb.connect(host, user, passwd, db_name)

        cursor = db.cursor()
        cursor.execute('SHOW TABLES')
        rows = cursor.fetchall()

        for row in rows:
            tables.append(row[0])

        db.close()
    """

    date_str = time.strftime('%Y%m%d')
    dirpath = BASE_PATH + '/' + date_str + '/'

    try:
        if not os.path.exists(dirpath):
            os.makedirs(dirpath)
    except:
        pass

    filename = dirpath +  'db_backup.sql'

    command = "mysqldump --skip-lock-tables -h " + host + " -u " + user + \
            " -p" + passwd + " --all-databases > " + filename

    os.system(command)


take_snapshot()
