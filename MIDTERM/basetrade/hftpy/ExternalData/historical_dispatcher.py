import os
import sys
from datetime import datetime
from datetime import datetime, timedelta
import time
import getpass
import subprocess
from random import randint
from abc import abstractmethod
sys.path.append(os.path.join("/home", getpass.getuser(), "basetrade"))


class HistoricalDispatcher:
    def __init__(self, watch, _start_date, _end_date, _start_time, _end_time):
        '''
            watch:
            SMV:
            _start_date: str
                         A string specifying the start date for dispactching the date
            _end_date: str
                        A string specifying the end date
            _start_time:
        '''

        self.watch = watch
        self.start_date = _start_date
        self.end_date = _end_date
        self.dates=[]
        self.GetDates()
        self.start_time = _start_time
        self.end_time = _end_time
        self.external_filesource_vec_ = []
        self.start_listener_vec = []
        self.end_listener_vec = []
        pass




    def AddStartListener(self, listener):
        '''
        Add listener to listen to the start notification of the start

        :param listener: Object which has to listen

        :return:
        '''
        self.start_listener_vec.append(listener)

    def AddEndListener(self, listener):
        '''
        Add listener to listen to the end notification of the end

        :param listener: Object which has to listen

        :return:
        '''
        self.end_listener_vec.append(listener)

    def ParseDate(self, date):
        """
        Parse for a valid date
        Args:
            date (str): Date in format : 'YYYYMMDD'
        Returns:
            Parsed date
        """
        try:
            return datetime.strptime(str(date), '%Y%m%d')
        except:
            raise AssertionError('Invalid date: %s' % str(date))


    def DateRange(self, start_date, end_date):
        """
        Args:
            start_date (datetime) : Start Date
            end_date (datetime) : End Date
        Returns:
            List of dates from start_date and end_date inclusive.
        """

        assert isinstance(start_date, datetime), 'Invalid start_date'
        assert isinstance(end_date, datetime), 'Invalid end_date'

        return list(map(lambda n: (start_date + timedelta(n)).strftime('%Y%m%d'),
                        range(int((end_date - start_date).days) + 1)))



    def GetDates(self):
        '''
            Sets the list of trading dates between start date and end date
        '''

        self.dates = self.DateRange(self.ParseDate(self.start_date), self.ParseDate(self.end_date))


    def NotifyStartListeners(self, _filesource_):
        '''
        Notify listeners listening to start Event of market
        :param _filesource_: filesource which contains the first event for the day
        :return:
        '''

        for listener in self.start_listener_vec:
            listener.notify(_filesource_.GetFirstEvent())

    def NotifyEndListeners(self, _filesource_):
        '''
        Notify listeners listening to end Event of market
        :param _filesource_: filesource which contains the last event for the day
        :return:
        '''
        for listener in self.end_listener_vec:
            listener.notify(_filesource_.GetLastEvent())

    def AddExternalFileSource(self, _filesource_):
        '''
            Add the filesource to the list of filesources to be used
            _filesource_: FileSource
                         A FileSource Object
        '''


        self.external_filesource_vec_.append(_filesource_)



    def ReadDayData(self, list_day):
        '''
            Reads and process the data for a single day for all the shortcodes

            list_day: list
                        A list with sorted object of timestamps and filesource object
        '''

        for timestamp,next_file_source in list_day:
            next_file_source.ProcessNextEvent()


    def GetUTCTimestamp(self, _date_, _time_):
        date_time = int(time.mktime(datetime.strptime(_date_,'%Y%m%d').timetuple())) - time.timezone
        date_time+= int(_time_.split('_')[1][:-2])*60*60 + int(_time_.split('_')[1][-2:])*60
        return date_time


    def RunHist(self):

        '''

        There are multiple data sources so i have to sort those source

        1. Add all the file sources for all the dates between start_date and end_date one by one in list_of_all_filesources
        2. sort the list
        3. Call the read_day_data function for the list to process all the events for each date

        '''


        # STEP 1 Combine all filesources for a date and store it in list_of_all_filesources
        #num_of_dates = len(self.dates)
        #count=0
        for date in self.dates:
            list_of_all_filesources_=[]
            start_timestamp = self.GetUTCTimestamp(date,self.start_time)
            end_timestamp = self.GetUTCTimestamp(date,self.end_time)
            for filesource_ in self.external_filesource_vec_:
                filesource_.ProcessAllEvents(start_timestamp, end_timestamp)
                list_of_all_filesources_+=filesource_.GetTimestamps()

            # STEP 2 Sort the list_of_all_filesources by timestamp

            list_of_all_filesources_.sort(key=lambda x:x[0])

            # STEP 3 Call the read_day_data function on the list_of_all_filesources to process the events
            if len(list_of_all_filesources_)>0:
                start_time,_start_event_ = list_of_all_filesources_[0]
                self.NotifyStartListeners(_start_event_)
                end_time, _end_event_ = list_of_all_filesources_[len(list_of_all_filesources_)-1]
                self.ReadDayData(list_of_all_filesources_)
                self.NotifyEndListeners(_end_event_)




    def SeekHistFileSourcesToTime(self, _start_time_):
        '''

        This method makes sure that all the file source only have messages after the _start_time_


        :param _start_time_:
        :return:
        '''
        for filesource in self.external_filesource_listener_vec_:
            filesource.SeekToFirstEventAfter(_start_time_)
