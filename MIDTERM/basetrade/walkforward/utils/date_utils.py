#!/usr/bin/env python

"""
Utility to get next, prev [week] day
"""

import time
import datetime
from datetime import date
from datetime import timedelta
from dateutil.relativedelta import relativedelta


def calc_next_day(t_date, lookforward_days=1):
    """
    #calculates the next day
    :param t_date: 
    :param lookforward_days: 
    :return: next_day
    """
    t = time.strptime(str(t_date), '%Y%m%d')
    newdate = date(t.tm_year, t.tm_mon, t.tm_mday) + timedelta(lookforward_days)
    return int(newdate.strftime('%Y%m%d'))


def calc_prev_day(t_date, lookback_days=1):
    """
    #calculates teh prev day

    :param t_date: 
    :param lookback_days: 
    :return: 
    """

    t = time.strptime(str(t_date), '%Y%m%d')
    newdate = date(t.tm_year, t.tm_mon, t.tm_mday) - timedelta(lookback_days)
    return int(newdate.strftime('%Y%m%d'))


def calc_next_week_day(t_date, lookforward_days=1):
    """

    :param t_date: 
    :param lookforward_days: 
    :return: 
    """
    current_date = time.strptime(str(t_date), '%Y%m%d')
    newdate = date(current_date.tm_year, current_date.tm_mon, current_date.tm_mday)
    num_times = lookforward_days
    while num_times > 0:
        newdate += timedelta(1)
        while newdate.isoweekday() > 5:  # gives numbers from 1-7
            newdate += timedelta(1)
        num_times -= 1

    return int(newdate.strftime('%Y%m%d'))


def calc_prev_week_day(t_date, lookback_days=1):
    """

    :param t_date: 
    :param lookback_days: 
    :return:

    """
    current_date = time.strptime(str(t_date), '%Y%m%d')
    newdate = date(current_date.tm_year, current_date.tm_mon, current_date.tm_mday)
    num_times = lookback_days
    while num_times > 0:
        newdate -= timedelta(1)
        while newdate.isoweekday() > 5:  # gives numbers from 1-7
            newdate -= timedelta(1)
        num_times -= 1

    return int(newdate.strftime('%Y%m%d'))


def calc_this_prev_weekday_date(t_date, t_which_weekday):
    """

    # this one returns closeset particular weekday looking back including given date
    # weekday identification:  Monday is 0 and Sunday is 6

    :param t_date: 
    :param t_which_weekday: 
    :return: 

    """

    t_date_format = datetime.datetime.strptime(str(t_date), '%Y%m%d')
    if int(t_which_weekday) not in list(range(0, 6)):
        raise ValueError(t_which_weekday + " doesnot belong to this world weekday, our calendar range is [0,6]")

    while t_date_format.weekday() != t_which_weekday:
        t_date_format = date(t_date_format.year, t_date_format.month, t_date_format.day) - timedelta(1)

    return int(t_date_format.strftime('%Y%m%d'))


def calc_this_next_weekday_date(t_date, t_which_weekday):
    """
    # this one returns closeset particular weekday looking forward including given date
    # weekday identification:  Monday is 0 and Sunday is 6    
    :param t_date: 
    :param t_which_weekday: 
    :return: 
    """
    t_date_format = datetime.datetime.strptime(str(t_date), '%Y%m%d')
    if int(t_which_weekday) not in list(range(0, 6)):
        raise ValueError(t_which_weekday + " doesnot belong to this world weekday, our calendar range is [0,6]")

    while t_date_format.weekday() != t_which_weekday:
        t_date_format = date(t_date_format.year, t_date_format.month, t_date_format.day) + timedelta(1)

    return int(t_date_format.strftime('%Y%m%d'))


# this one returns first weekday date of a given month
def calc_first_weekday_date_of_month(t_date):
    """

    :param t_date: 
    :return: 
    """
    t_date_format = datetime.datetime.strptime(str(t_date), '%Y%m%d')
    this_month_first_day_date = date(t_date_format.year, t_date_format.month, 1)
    if this_month_first_day_date.weekday() == 5:  # saturaday
        this_month_first_day_date = this_month_first_day_date + timedelta(2)
    if this_month_first_day_date.weekday() == 6:  # sunday
        this_month_first_day_date = this_month_first_day_date + timedelta(1)

    return int(this_month_first_day_date.strftime('%Y%m%d'))


# this one returns last weekday date of a given month
def calc_last_weekday_date_of_month(t_date):
    """

    :param t_date: 
    :return: 
    """
    t_date_format = datetime.datetime.strptime(str(t_date), '%Y%m%d')
    next_month_first_day_date = date(t_date_format.year, t_date_format.month, 1) + relativedelta(months=1)
    this_month_last_day_date = calc_prev_week_day(int(next_month_first_day_date.strftime('%Y%m%d')), 1)
    return int(this_month_last_day_date)


def week_days_between_dates(start_date, end_date):
    """

    :param start_date:
    :param end_date:
    :return: (int) number of week days in between

    """

    current_date = datetime.datetime.strptime(str(start_date), '%Y%m%d')
    last_date = datetime.datetime.strptime(str(end_date), '%Y%m%d')
    daydiff = last_date.weekday() - current_date.weekday()

    days = ((last_date - current_date).days - daydiff) / 7 * 5 + min(daydiff, 5) - (max(last_date.weekday() - 4, 0) % 5)
    return days    

def days_between_dates(start_date, end_date):
    """

    :param start_date:
    :param end_date:
    :return:

    """
    current_date = datetime.datetime.strptime(str(start_date), '%Y%m%d')
    last_date = datetime.datetime.strptime(str(end_date), '%Y%m%d')
    return (last_date - current_date).days

def get_week_day_list_from_day(end_date, ldays):
    """

    :param end_date: 
    :param ldays: Number of past days to look back
    :return: list of past ldays from end_date (including)

    """
    list_of_days = []
    for i in range(0,ldays):
        list_of_days.append(calc_prev_week_day(end_date, i))

    return list_of_days


def get_dates_between(start_date, end_date):
    """

    :param start_date: 
    :param end_date: 
    :return: list of weekdays between start and end (both including)

    """
    return get_week_day_list_from_day(end_date, int(week_days_between_dates(start_date, end_date))+1)

def calc_iso_date_from_str_min1(date_str):
    substr = "TODAY"
    today = datetime.datetime.strftime(datetime.date.today(),"%Y%m%d")
    if substr in date_str:
        try:
            num_prev = -1*int(date_str[date_str.find(substr)+len(substr):])
        except:
            num_prev = 1
        return calc_prev_week_day(today, num_prev)
    else:
        return int(date_str)
