#!/usr/bin/env python

"""

"""


market_data_print_cols = ['time', 'bid_px', 'bid_int_px', 'bid_ord', 'bid_sz', 'ask_px', 'ask_int_px', 'ask_ord',
                          'ask_sz', 'trd_size', 'trd_price', 'trd_side']

class MarketDataStruct():

    def __init__(self):
        self.time_ = 0.0
        self.bid_price_ = 0.0
        self.bid_int_price_ = 0
        self.bid_ordercount_ = 0
        self.bid_size_ = 0

        self.ask_price_ = 0.0
        self.ask_int_price_ = 0
        self.ask_ordercount_ = 0
        self.ask_size_ = 0

        self.trade_size_ = 0
        self.trade_price_ = 0.0
        self.trade_side_ = 'O'

    def to_string(self):
        print_string = '%0.6f %0.5f %5d %4d %d - ' \
                       '%0.5f %5d %4d %d %5d %0.5f %s' % (self.time_, self.bid_price_, self.bid_int_price_,
                                                          self.bid_ordercount_, self.bid_size_, self.ask_price_,
                                                          self.ask_int_price_, self.ask_ordercount_, self.ask_size_,
                                                          self.trade_size_, self.trade_price_, self.trade_side_)
        return print_string