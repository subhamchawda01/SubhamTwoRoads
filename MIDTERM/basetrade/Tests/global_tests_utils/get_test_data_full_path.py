#!/usr/bin/env python

""""
Get the full path for test-data

"""

import os


def get_test_data_full_path(test_data, subdir=""):
    """
    Returns the full path of the data file given environment variables
    :param test_data: 
    :param subdir: 
    :return: 
    """
    work_dir = '/home/dvctrader/basetrade'
    work_dir_env = os.getenv('WORKDIR')

    if work_dir_env:
        work_dir = work_dir_env
    else:
        home_env = os.getenv('HOME')
        if home_env:
            work_dir = home_env + '/basetrade'

    test_data_filename = work_dir + '/Tests/data/' + test_data
    if len(subdir) > 0:
        test_data_filename = work_dir + '/Tests/data/' + subdir + '/' + test_data

    return test_data_filename
