#!/usr/bin/env python


"""
For a given shortcode, for all possible orders it sent on given date, get the queue position
when order was placed and queue position when the order actually reached the market

"""

import os
import sys
import argparse

import subprocess

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.definitions.execs import execs

from pylib.get_mkt_trade_logger_stats import get_mkt_trade_logger_stats
from pylib.get_ors_binary_reader_stats import get_ors_binary_reader_all_data

from pylib.get_mkt_book_viewer_stats import get_mkt_book_viewer_stats

def print_stats_per_saos(saos_to_details_map):
    for saos in saos_to_details_map.keys():
        print(saos_to_details_map[saos].to_string())


def find_next_nearest_index(index, series, given_time):
    """

    :param index:
    :param series:
    :param given_time:
    :return:
    """
    new_index = index

    if len(series) <= new_index:
        return new_index

    if type(series[new_index]) == list:
        while series[new_index][0].time_ < given_time:
            # print (series[index].time_, given_time, index)
            new_index += 1

            if new_index < len(series) and series[new_index][0].time_ >= given_time:
                break
    else:
        while series[new_index].time_ < given_time:
            # print (series[index].time_, given_time, index)
            new_index += 1

            if new_index < len(series) and series[new_index].time_ >= given_time:
                break

    return new_index


def find_size_at_time(details_struct, current_time, mkt_update_index, mkt_update, trades_index, trades, use_order_size, mpi):
    """

    :param details_struct:
    :param current_time:
    :param mkt_update_index:
    :param mkt_update:
    :param trades_index:
    :param trades:
    :param use_order_size:
    :param mpi:
    :return:
    """

    # take the update index to closest timestamp
    new_mkt_update_index = find_next_nearest_index(mkt_update_index, mkt_update, current_time)
    new_trades_index = find_next_nearest_index(trades_index, trades, current_time)

    this_update_list = mkt_update[new_mkt_update_index]


    this_time_value = -1

    if details_struct.buysell_ == 'B':
        if type(this_update_list) == list:
            for update in this_update_list:
                # find the level which has same price
                if abs(details_struct.price_ - update.bid_price_) < 0.1*mpi:
                    this_update = update
                    this_time_value = this_update.bid_ordercount_ if use_order_size else this_update.bid_size_
                    break
        else:
            this_time_value = this_update_list.bid_ordercount_ if use_order_size else this_update_list.bid_size_
    elif details_struct.buysell_ == 'S':
        if type(this_update_list) == list:
            # find the level which has closest price
            for update in this_update_list:
                if abs(details_struct.price_ - update.ask_price_) < 0.1 * mpi:
                    this_update = update
                    this_time_value = this_update.ask_ordercount_ if use_order_size else this_update.ask_size_
                    break
        else:
            this_time_value = this_update_list.ask_ordercount_ if use_order_size else this_update_list.ask_size_

    return this_time_value, new_mkt_update_index, new_trades_index


def compute_queue_pos_stats_for_prod(shortcode, tradingdate, use_order_size, use_complete_book):
    """
    Currently we look at the timestamp and queue size when an order was placed and timestamp
    and queue size when order was confirmed and use the two queue-pos values as difference

    improvements:
    a) Account for queue position at half of seqd/conf values ( i.e when the order was sent)
    b) Use order-feed data to match order id's and get more accurate match

    :param shortcode:
    :param tradingdate:
    :param use_order_size
    :param use_complete_book whether to use mkt-book-viewer or mkt-trade-logger
    :return:

    """

    mkt_update = []
    trades = []
    if use_complete_book:
        lvl = 5 if use_complete_book else 20
        mkt_update = get_mkt_book_viewer_stats(shortcode, tradingdate, lvl)
    else:
        mkt_update, trades = get_mkt_trade_logger_stats(shortcode, tradingdate)


    saos_to_details_map , summary_map = get_ors_binary_reader_all_data(shortcode, tradingdate)

    mpi_cmd = [execs().get_contract_specs, shortcode, str(tradingdate), 'TICKSIZE']

    mpi = float(subprocess.check_output(mpi_cmd).decode('utf-8').strip().split()[-1])
    mkt_update_index = 0
    trades_index = 0

    # to make it run fast, the purpose is to find the
    for saos in sorted(saos_to_details_map.keys()):
        details_struct = saos_to_details_map[saos]
        # print(saos, details_struct.to_string())
        seqd_index = details_struct.status_vec_.index('Seqd')
        send_time = details_struct.send_time_vec_[seqd_index]

        details_struct.queue_at_send_, mkt_update_index, trades_index \
            = find_size_at_time(details_struct, send_time, mkt_update_index, mkt_update,
                                trades_index, trades, use_order_size, mpi)

        # if details_struct.queue_at_send_ == -1:
        #    print('NO MATCH SEQD', send_time, seqd_index, mkt_update_index, trades_index)

        if 'Conf' in details_struct.status_vec_:
            conf_index = details_struct.status_vec_.index('Conf')
        else:
            # would happen in case of internal execs
            continue

        conf_time = details_struct.send_time_vec_[conf_index]

        details_struct.queue_at_conf_, mui, ti \
            = find_size_at_time(details_struct, conf_time, mkt_update_index, mkt_update,
                                trades_index, trades, use_order_size, mpi)

        # if details_struct.queue_at_conf_ == -1:
        #    print('NO MATCH CONF', conf_time, conf_index, mui, ti, mkt_update_index, trades_index)

        # For now in case the level has changed, we are not considering that point as interesting.
        # ideally we would want to use book-viewer in order to get the correct size for that level
        if details_struct.queue_at_conf_ == -1 :
            details_struct.queue_at_send_ = -1

        # get queue size at cancel seqd
        cxl_seqd_index = -1
        if 'CxlSeqd' in details_struct.status_vec_:
            cxl_seqd_index = details_struct.status_vec_.index('CxlSeqd')

        cxl_seqd_time = details_struct.send_time_vec_[cxl_seqd_index]
        if cxl_seqd_index != -1:
            details_struct.queue_at_cxl_seq_ , mui, ti \
                = find_size_at_time(details_struct, cxl_seqd_time, mkt_update_index, mkt_update, trades_index,
                                    trades, use_order_size, mpi)

        # get queue size at canceled
        cxld_index = -1
        if 'Cxld' in details_struct.status_vec_:
            cxld_index = details_struct.status_vec_.index('Cxld')

        cxld_time = details_struct.send_time_vec_[cxld_index]
        if cxld_index != -1:
            details_struct.queue_at_cxld_, mui, ti \
                = find_size_at_time(details_struct, cxld_time, mkt_update_index, mkt_update, trades_index, trades,
                                    use_order_size, mpi)

        # get queue size at exec
        exec_index = -1
        if 'Exec' in details_struct.status_vec_:
            exec_index = details_struct.status_vec_.index('Exec')

        exec_time = details_struct.send_time_vec_[exec_index]

        if exec_index != -1:
            details_struct.queue_at_exec_, mui, ti \
                = find_size_at_time(details_struct, exec_time, mkt_update_index, mkt_update, trades_index, trades,
                                    use_order_size, mpi)

        # print(details_struct.to_string())
        print(mkt_update_index, mui, details_struct.to_string())
    # print_stats_per_saos(saos_to_details_map)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', dest='shortcode', help="shortcode string", type=str, required=True)
    parser.add_argument('-d', dest='tradingdate', help=' tradingdate ', type=int, required=True)
    parser.add_argument('-osz', dest='use_order_size', help='whether to use total size or order count at px',
                        type=int, default=0)
    parser.add_argument('-b', dest='complete_book', help='Whether to use complete book or not', type=int, required=False, default=0)
    args = parser.parse_args()

    compute_queue_pos_stats_for_prod(args.shortcode, args.tradingdate, args.use_order_size != 0, args.complete_book != 0)
