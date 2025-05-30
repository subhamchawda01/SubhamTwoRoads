#!/usr/bin/env python

""" 
Utility script to get the details of the database

Reads the file from /spare/local/files directory :
The details read are :
    Network ip of database :
        The ip's would be different in case of ec2 workers and other machines ( public ip would be used )
    Name of the mysql-database
    User details to be logged into. Currently following users are there :
       dvcreader : supports only reading from the database
       dvcwriter : apart from all dvcreader access, it can modify and create tables as well
                 has access to each operation from hs1(52.87.81.158)
    
It maintains an object with all these fields which further gets used in creating the connection
       

"""

import os
import json
import socket


class wf_db_config:
    """
    # Maintains all the fields
    """
    wf_db_name = ""
    wf_db_reader = ""
    wf_db_writer = ""
    wf_db_pass = ""
    wf_db_ip = ""
    backtest = False
    hostname = ""
    is_ec2 = True

    """

    Setup hostname,
    Currently by default we load production database

    """

    def __init__(self):
        self.hostname = socket.gethostname()
        self.is_ec2 = self.hostname.find('ip-10-0') != -1
        wf_db_config_path = '/spare/local/files/DBCONFIG_results_json'
        self.load_config(wf_db_config_path)

    def print_db_config(self):
        """
        Prints all the values read from config.
        Used primarily in debugging
        # no input/output variables

        :return: 
        """
        print(("DBNAME  ", self.wf_db_name))
        print(("HOSTNAME", self.wf_db_ip))
        print(("READER  ", self.wf_db_reader))
        print(("WRITER  ", self.wf_db_writer))
        print(("PASSWORD", self.wf_db_pass))

    """

    To be called from outside, used to change the database dynamically

    """

    def is_backtest(self):
        """
        Return the flag of normal or backtest database
        :return: 
        """
        return self.backtest

    def set_is_backtest(self, flag):
        """
        Allows external control over setting the database type.
        Once we change the type of database, it reloads all the fields from config depending on the flag set

        :param flag: type of database true for backtest false for prod
        :return: 

        """
        self.backtest = flag
        self.reload_config()

    def reload_config(self):
        """
        Change the appropriate path of configs depending on backtest flag.
        It also reloads the fields
        :return: 
        """
        if self.backtest:
            wf_db_config_path = '/spare/local/files/DBCONFIG_backtest_results_json'
        else:
            wf_db_config_path = '/spare/local/files/DBCONFIG_results_json'
        self.load_config(wf_db_config_path)

    def load_config(self, wf_db_config_path):
        """
       Read the content of the file provided and setup the appropriate fields in config fields

        :param wf_db_config_path: path of the json file containing database details 
        :return: 

        """
        if os.path.exists(wf_db_config_path):
            with open(wf_db_config_path) as wf_db_config_file:
                wf_db_config_values = json.load(wf_db_config_file)

            # fetch the dbname from the config
            if 'DBNAME' in wf_db_config_values:
                self.wf_db_name = wf_db_config_values['DBNAME']
            else:
                raise ValueError("DBNAME not found in config")

            # fetch the hostname from the config
            if 'HOSTNAME' in wf_db_config_values and not self.is_ec2:
                self.wf_db_ip = wf_db_config_values['HOSTNAME']
            elif 'EC2HOSTNAME' in wf_db_config_values and self.is_ec2:
                self.wf_db_ip = wf_db_config_values['EC2HOSTNAME']
            else:
                raise ValueError("Neither HOSTNAME nor EC2HOSTNAME was found")

            # fetch the reader/writer from the config
            if 'USERNAME' in wf_db_config_values:
                self.wf_db_writer = wf_db_config_values['USERNAME']
                self.wf_db_reader = self.wf_db_writer
            else:
                raise ValueError("USERNAME not found")

            # fetch the password

            if 'PASSWORD' in wf_db_config_values:
                self.wf_db_pass = wf_db_config_values['PASSWORD']
            else:
                raise ValueError('PASSWORD not found')
        else:
            print(("DB Configfile: " + wf_db_config_path + " does not exist"))
