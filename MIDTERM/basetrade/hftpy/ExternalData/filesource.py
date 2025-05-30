import os
import sys
import getpass
import subprocess
from datetime import datetime
import time
from random import randint
sys.path.append(os.path.join("/home", getpass.getuser(), "basetrade"))
from hftpy.ExternalData.file_source_listener import FileSourceListener
from hftpy.SimInfra.market_update_info import MarketUpdateInfo
BASE_DATA_DIR = "/home/dvctrader/HFTPY/"

class FileSource(object):
    def __init__(self, _watch_, _shortcode_, _granularity_, _trading_date_, _start_time_=None, _end_time_=None):
        '''

        :param _watch_:
        :param _shortcode_:
        :param _granularity_:
        :param _trading_date_:
        '''



        self.watch_ = _watch_
        self.shortcode_ = _shortcode_
        self.granularity_ = _granularity_
        self.trading_date_ = _trading_date_
        self.first_event_ = MarketUpdateInfo([-1, -1, -1, -1, -1, -1, -1])
        self.last_event_ = MarketUpdateInfo([-1, -1, -1, -1, -1, -1, -1])
        print BASE_DATA_DIR, self.trading_date_[:4] + "/" + self.trading_date_[4:-2] + "/" + self.trading_date_[-2:] + "/" + self.shortcode_
        self.file_name_ = os.path.join(BASE_DATA_DIR, self.trading_date_[:4] + "/" + self.trading_date_[
                                                                                     4:-2] + "/" + self.trading_date_[-2:] + "/" + self.shortcode_ + "_" + self.granularity_)

        self.minimum_increment_price = self.GetMinimumIncrementPrice()
        self.data_list = []
        self.start_parsing_logged_file = False
        self.stop_parsing_logged_file = True
        self.event_left_to_parse = True
        # initializing the next_event_ with
        self.next_event_ = MarketUpdateInfo([-1, -1, -1, -1, -1, -1, -1])
        self.event_listeners = []


    def GetFirstEvent(self):
        return self.first_event_

    def GetLastEvent(self):
        return self.last_event_

    def ProcessThisEvent(self):
        for price_level_listener in self.event_listeners:
            price_level_listener.OnMarketUpdate(self.next_event_)

    def GetMinimumIncrementPrice(self):

        '''
        Returns minimum increment price for the shortcode for the trading date
        :return: null
        '''
        user=getpass.getuser()
        path_for_min_increment = os.path.join('/home/' + user, 'basetrade_install/bin/get_min_price_increment')
        command_to_get_minimum_increment = []
        command_to_get_minimum_increment.append(path_for_min_increment)
        command_to_get_minimum_increment.append(self.shortcode_.split('/')[1])
        command_to_get_minimum_increment.append(self.trading_date_)
        min_incr_process = subprocess.Popen(command_to_get_minimum_increment, stdout=subprocess.PIPE)
        result = min_incr_process.stdout.read()
        return_code = min_incr_process.wait()
        return float(result)

    def AddExternalListener(self, price_level_listener):
        '''
        This function adds the SMV to event_listeners list which listens to market updates

        :param price_level_listener: SMV
        :return: null
        '''
        if price_level_listener not in self.event_listeners:
            self.event_listeners.append(price_level_listener)

    def SeekToFirstEventAfter(self, _start_time_):
        '''

        This function updates the n

        :param _start_time_: an instance of the time class
        :return: returns the next e
        '''
        # Gets the first event after start time, to start data logging

        for price_data in self.data_list:
            if self.start_parsing_logged_file == False:
                if price_data.GetTime() > _start_time_:
                    # self.next_event_ = self.data_list.pop(0)
                    self.start_parsing_logged_file = True
                    break
                else:
                    # if the present event is earlier than the start time then just remove it
                    self.data_list.pop(0)

    def GetTimestamps(self):
        return [(elem.GetTime(), self) for elem in self.data_list]

    def ProcessNextEvent(self):
        if len(self.data_list) == 0:
            self.event_left_to_parse = False

        if self.event_left_to_parse:
            self.next_event_ = self.data_list.pop(0)
            self.ProcessThisEvent()


    def ProcessAllEvents(self, start_time, end_time):
        '''
            Reads the entire file source in a list, reading the entire data in a list because it is easier to process
            the data once in the list.

            The read data is stored in the instance variable self.data_list
        '''
        count = 0;
        with open(self.file_name_) as file_handle:
            # reading all the file in a list, doing this because it is easier to work with data in a list
            feeds = file_handle.readlines()
            num_of_feeds = len(feeds)
            for line in feeds:
                next_event_line = line.rstrip().split()
                if int(next_event_line[0]) < start_time*1000:
                    continue

                if int(next_event_line[0]) > end_time*1000:
                    break;

                if len(next_event_line) != 5:
                    print(
                        "Currently only five columns in data file is supported time_stamp and price, the line has more than five columns hence ignoring the line")
                    print ("The content of the line are ", next_event_line)
                    continue
                else:
                    next_event_line.append(self.shortcode_)
                    next_event_line.append(float(self.minimum_increment_price))
                    price_data = MarketUpdateInfo(next_event_line)
                    if count==0:
                        self.first_event_ = price_data
                    self.last_event_ = price_data
                    self.data_list.append(price_data)
                    count+=1

    def ProcessEventsTill(self, _end_time_):
        for price_data in self.data_list:
            if self.stop_parsing_logged_file == False:
                if price_data.GetTime() < _end_time_:
                    self.next_event_ = self.data_list.pop(0)
                    self.start_parsing_logged_file = True
                else:
                    self.stop_parsing_logged_file = True
