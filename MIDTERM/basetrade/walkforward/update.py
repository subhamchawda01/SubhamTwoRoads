#!/usr/bin/env python

#!/home/psarthy/anaconda2/bin/python
#!/home/dvctrader/anaconda2/bin/python


import os


import pandas as pd
import numpy as np

from functools import partial
import datetime as dt
from string import split
import subprocess
import json


def clean_(x):
    # x=x.strip()
    # return x.replace(" ","")
    x = x.rstrip('\n')
    x = x.strip()
    return x + "\n"


def validline_(x):
    if x[0] == "#":
        return False
    else:
        return True


if 1:
    home_directory = os.getenv("HOME")
else:
    home_directory = "/home/dvctrader/"


class update(object):
    """
    base class to define implement basic idea of walk forward trading:
    0. A config file is created which has parameters for the following
    1. Use recent data (T-8 to T-1) to define model
    2. Use yesterdays (T-1) stdev as param
    3. Test at T
    """

    def __init__(self, config_path):
        """initialization of config from json"""
        print("Initializing...")
        self.config_path = config_path
        self.config_name = self.config_path.split("/")[-1]
        with open(config_path) as json_data:
            self.config = json.load(json_data)
            # print(d)

        print(self.config['shortcode'])

    def show(self):
        """print config contents"""
        for key in self.config:
            print(key, self.config[key])

    def add_attribute(self, attribute_header, attribute_values_list):
        """print config contents"""
        self.config.append(attribute_header + "\n")
        for attribute in attribute_values_list:
            self.config.append(attribute + "\n")

    def remove_attribute(self, attribute_header):
        """print config contents"""
        self.config.pop(self.config.index(attribute_header + "\n") + 1)
        self.config.pop(self.config.index(attribute_header + "\n"))

    def show_ilist(self):
        """show ilist header"""
        print("Hi")

    def update(self, trade_date):
        cmd_ = [home_directory + "/basetrade/scripts/get_list_of_dates_for_shortcode.pl",
                self.get_attribute("SHORTCODE"), str(trade_date), self.get_attribute("MODEL_TRAINING_LOOKBACK")]
        training_dates = subprocess.check_output(cmd_)
        training_dates = training_dates.split(" ")
        self.add_attribute("\nDATAGEN_DAYS", training_dates)
        self.remove_attribute("MODEL_TRAINING_LOOKBACK")
        filename = "/spare/local/walkforward/genstrat_configs/genstratconfig_" + \
            self.config_name + "_" + str(trade_date)
        print(filename)
        f = open(filename, 'w')
        f.write("".join(self.config))
        f.close()
