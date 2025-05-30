#!/usr/bin/env python

"""
same as mkt-trade logger,
just mkt-book viewer

"""


import subprocess

from walkforward.definitions.execs import execs
from pylib.definitions.market_data_struct import MarketDataStruct


def get_mkt_book_viewer_stats(shortcode, tradingdate, num_levels=20):
    """

    :param shortcode:
    :param tradingdate:
    :return:
    """

    mkt_book_viewer_cmd = [execs().mkt_book_viewer, 'SIM', shortcode, str(tradingdate), str(num_levels)]

    # replace the special characters to whitespaces
    output_val = subprocess.check_output(mkt_book_viewer_cmd).decode('utf-8').replace('\x1b[4;1H\x1b[2K', ' ').replace('\x1b[2K', ' ').splitlines()

    current_time = 0
    index_to_log = 5
    count_so_far = 0

    mkt_updates_vec = []

    for line in output_val:
        line_words = line.split()
        restart = False

        if 'Time' in line:
            restart = True
            count_so_far = 0
            current_time = float(line_words[-1])
            mkt_updates_vec.append([])
            continue

        if 'Traded Volume' in line:
            continue

        if len(mkt_updates_vec) > 0 and len(mkt_updates_vec[-1]) < index_to_log:
            update_struct = MarketDataStruct()
            update_struct.time_ = current_time

            indx = 1
            update_struct.bid_size_ = int(line_words[indx])
            indx += 1
            update_struct.bid_ordercount_ = int(line_words[indx])
            indx += 1
            update_struct.bid_price_ = float(line_words[indx])

            indx += 4
            update_struct.ask_price_ = float(line_words[indx])
            indx += 1
            update_struct.ask_ordercount_ = int(line_words[indx])
            indx += 1
            update_struct.ask_size_ = int(line_words[indx])

            mkt_updates_vec[-1].append(update_struct)

    return mkt_updates_vec