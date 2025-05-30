import datetime as dt
import subprocess
from .pengineexecs import *

def BashExec(command):
        return subprocess.check_output(['bash', '-c',
                command]).decode('UTF-8').strip()

def IsHoliday(date, exchange):
        is_holiday = BashExec(holiday_manager + ' EXCHANGE ' + exchange + ' ' + date + ' T')
        return True if is_holiday == '1' else False

def GetPreviousWorkingDay(date, exchange):
        prev_day = BashExec(update_date + ' ' + date + ' P W');
        while IsHoliday(prev_day, exchange):
                prev_day = BashExec(update_date + ' ' + prev_day + ' P W');
        return prev_day

def GetNextWorkingDay(date, exchange):
    next_day = BashExec(update_date + ' ' + date + ' N W')
    is_holiday = BashExec(holiday_manager + ' EXCHANGE ' +  exchange + ' ' + date
                          + ' T')
    while is_holiday == '1':
        next_day = BashExec(update_date + ' ' + next_day + ' N W')
        is_holiday = BashExec(holiday_manager + ' EXCHANGE ' + exchange + ' '
                              + next_day + ' T')
    return next_day

