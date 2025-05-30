import os, errno
from datetime import timedelta, datetime
import re


def get_list_of_dirs_given_pattern(folder_to_search, expression):
    list_dir = []
    reg_compile = re.compile(expression)

    for x in os.walk(folder_to_search):
        if re.search(reg_compile, os.path.basename(x[0])):
            list_dir = list_dir + [os.path.basename(x[0])]

    return list_dir


def get_list_of_files_given_pattern(folder_to_search, expression):
    list_dir = []
    reg_compile = re.compile(expression)

    for x in os.listdir(folder_to_search):
        if re.search(reg_compile, x) and not os.path.isdir(x):
            list_dir = list_dir + [x]

    return list_dir


def silent_remove(filename):
    try:
        os.remove(filename)
    except OSError as e:  # this would be "except OSError, e:" before Python 2.6
        if e.errno != errno.ENOENT:  # errno.ENOENT = no such file or directory
            raise  # re-raise exception if a different error occurred


def prev_weekday(adate):
    adate -= timedelta(days=1)
    while adate.weekday() > 4:  # Mon-Fri are 0-4
        adate -= timedelta(days=1)
    return adate


def get_list_of_dates_(end_date, num_days):
    dates_list_ = []
    num_days = int(num_days)

    dt = datetime.strptime(end_date, '%Y%m%d')
    while num_days > 0:
        dates_list_.append(dt.strftime('%Y%m%d'))
        dt = prev_weekday(dt)
        num_days -= 1
    return dates_list_


def get_list_of_dates_given_start(start_date, end_date):
    dates_list_ = []
    start_date = int(start_date)

    dt = datetime.strptime(end_date, '%Y%m%d')
    while int(dt.strftime('%Y%m%d')) >= start_date :
        dates_list_.append(dt.strftime('%Y%m%d'))
        dt = prev_weekday(dt)
    return dates_list_
