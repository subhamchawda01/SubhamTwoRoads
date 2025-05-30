#/usr/bin/env python

"""
Maintains global_connection instance

"""
from __future__ import print_function

import os
import sys

import MySQLdb


from walkforward.wf_db_utils.get_db_config import wf_db_config

db_config = wf_db_config()


def create_connection(host, user, passwd, db):
    """
    create the connection with given paramters

    :return: 
    """
    global global_connection
    global db_config
    global cursor

    # open a database global_connection
    # be sure to change the host IP address, username, password and database name to match your own
    global_connection = MySQLdb.connect(host=host, user=user, passwd=passwd,
                                        db=db)
    # setting autocommit to true,
    # this will make sure each database query execution is written to database right aways
    # not considering any performance issue because of autocommit
    global_connection.autocommit(True)

    # prepare a cursor object using cursor() method
    cursor = global_connection.cursor()


def connection():
    global global_connection
    return global_connection


def is_backtest():
    """

    :return: bool: is db in backtest mode
    """
    global db_config
    return db_config.is_backtest()


def set_backtest(flag):
    """
    Set backtesting database depending on flag
    :param flag: 
    :return: 
    """
    global global_connection
    global db_config
    global cursor

    if flag != db_config.is_backtest():
        # print >>sys.stderr, ("Settting " + str(flag) + " database. ")
        # if global_connection is open the close it
        if global_connection:
            cursor.close()
            global_connection.close()

        # set the backtesting to appropriate flags
        db_config.set_is_backtest(flag)
        # db_config.print_db_config()
        # connect to mysql database with updated files
        create_connection(host=db_config.wf_db_ip, user=db_config.wf_db_reader, passwd=db_config.wf_db_pass,
                          db=db_config.wf_db_name)


def change_db(database_name):
    global global_connection
    global db_config
    global cursor
    if global_connection:
        cursor.close()
        global_connection.close()

    create_connection(host=db_config.wf_db_ip, user=db_config.wf_db_reader, passwd=db_config.wf_db_pass,
                      db=database_name)


# global call to functions
create_connection(host=db_config.wf_db_ip, user=db_config.wf_db_reader, passwd=db_config.wf_db_pass,
                  db=db_config.wf_db_name)

env_var = os.getenv('USE_BACKTEST')
username = os.getenv('USER')

if env_var and (int(env_var) > 0):
    # in case in env is not defined then by default use backtest for local user
    # if env is defined then override with env value
    set_backtest(True)
