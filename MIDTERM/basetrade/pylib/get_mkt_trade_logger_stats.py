#!/usr/bin/env python

"""
market l1 book

"""

import subprocess

from walkforward.definitions.execs import execs

from pylib.definitions.market_data_struct import MarketDataStruct

def get_mkt_trade_logger_stats(shortcode, tradingdate):
    """

    :param shortcode:
    :param tradingdate:
    :return:
    """
    mkt_trade_logger_cmd = [execs().mkt_trade_logger, 'SIM', shortcode, str(tradingdate)]

    # We need to add of/pf feed details appropriately for different exchanges, for now using price feed
    output_val = subprocess.check_output(mkt_trade_logger_cmd).decode('utf-8').replace('[', ' ').replace(']', ' ').splitlines()
    # 1513719209.063828 OnTradePrint  B 5 @ 0.74 SP_VX2_VX3 1 60 0.73 0.74 5 1 1 60 0.73 0.74 5 1
    # 1513718940.001598 OnMarketUpdate SP_VX2_VX3 1 10 0.72 0.74 21 1 1 10 0.72 0.74 21 1

    mkt_updates = []
    trades = []

    for line in output_val:
        if 'OnMarketUpdate' in line:
            # corresponding to market update line
            update_struct = MarketDataStruct()
            words = line.split()
            update_struct.time_ = float(words[0])

            update_struct.bid_ordercount_ = int(words[3])
            update_struct.bid_size_ = int(words[4])
            update_struct.bid_price_ = float(words[5])

            update_struct.ask_price_ = float(words[6])
            update_struct.ask_size_ = int(words[7])
            update_struct.ask_ordercount_ = int(words[8])

            mkt_updates.append(update_struct)
        elif 'OnTradePrint' in line:
            # corresponding to tradeprint line
            trade_struct = MarketDataStruct()
            words = line.split()
            trade_struct.time_ = float(words[0])

            trade_struct.trade_side_ = words[2]
            trade_struct.trade_size_ = int(words[3])
            trade_struct.trade_price_ = float(words[5])

            trade_struct.bid_ordercount_ = int(words[7])
            trade_struct.bid_size_ = int(words[8])
            trade_struct.bid_price_ = float(words[9])

            trade_struct.ask_price_ = float(words[10])
            trade_struct.ask_size_ = int(words[11])
            trade_struct.ask_ordercount_ = int(words[12])
            trades.append(trade_struct)

    # print('TOP', mkt_updates[1].to_string())
    # print('BOTTOM', mkt_updates[-1].to_string())
    return mkt_updates, trades
