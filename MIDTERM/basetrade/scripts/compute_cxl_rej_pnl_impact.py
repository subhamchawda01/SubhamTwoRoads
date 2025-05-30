#!/usr/bin/env python

"""

This exec tries to compute pnl impact of cxl-rejected orders

The detailed assumptions are written as they are added

"""
import os
import sys
import argparse
import subprocess


sys.path.append(os.path.expanduser('~/basetrade'))

from pylib.get_ors_binary_reader_stats import get_ors_binary_reader_all_data

from walkforward.definitions.execs import execs

from scripts.get_cxl_rej_stats2 import get_seq2conf_for_percentile

parser = argparse.ArgumentParser()
parser.add_argument('-shc', dest='shortcode', help="shortcode", type=str, required=True)
parser.add_argument('-date', dest='date', help="trading date", type=str, required=True)
parser.add_argument('-p', dest='percentile', help="Percentile of seqd-conf numbers below which we would ignore "
                                                  "cxl-rejects", type=int, required=False, default=0)

args = parser.parse_args()


# now for each of cxl_reject point, get the market price, position of SACI, and same details at closeout

saos_to_details_map, count_map = get_ors_binary_reader_all_data(args.shortcode, args.date)

# filter saci based data

mpi_cmd = [execs().get_contract_specs, args.shortcode, args.date, 'N2D']

mpi = float(subprocess.check_output(mpi_cmd).decode('utf-8').strip().split()[-1])

ignore_values_below = 0

if args.percentile > 0:
    ignore_values_below = get_seq2conf_for_percentile(args.percentile,args.shortcode, args.date)

saci_to_saos_to_details_map = {}

for keyval in list(saos_to_details_map.keys()):
    if saos_to_details_map[keyval].saci_ not in saci_to_saos_to_details_map.keys():
        saci_to_saos_to_details_map[saos_to_details_map[keyval].saci_] = {}
        saci_to_saos_to_details_map[saos_to_details_map[keyval].saci_][keyval] = saos_to_details_map[keyval]
    else:
        saci_to_saos_to_details_map[saos_to_details_map[keyval].saci_][keyval] = saos_to_details_map[keyval]

# essentially dummy trades file
saci_to_exec_list = {}

for saci in list(saci_to_saos_to_details_map.keys()):
    saci_to_exec_list[saci] = []
    saos_details_vec = saci_to_saos_to_details_map[saci]
    for saos in list(saos_details_vec.keys()):
        for i in range(len(saos_details_vec[saos].status_vec_)):
            if saos_details_vec[saos].status_vec_[i] == 'Exec':
                saci_to_exec_list[saci].append((saos_details_vec[saos].send_time_vec_[i],
                                                saos_details_vec[saos].pos_vec_[i],
                                                saos_details_vec[saos].price_vec_[i],
                                                saos_details_vec[saos].saos_))

    saci_to_exec_list[saci] = sorted(saci_to_exec_list[saci], key=lambda x: x[0])

# Now for each saci we have dummy trades file

# temporary vector which maintains list of cxl-reject based executions for which we haven't received flat
cxl_reject_info_vec = []

# vector containing all trades we did because of cancel reject and while closing the positions generated
all_trades_vec = []

pnl_so_far = 0

for saci in list(saci_to_exec_list.keys()):
    trades_file = saci_to_exec_list[saci]
    saos_to_details = saci_to_saos_to_details_map[saci]
    print('TRADES', trades_file, '\nTradesEnd')

    for idx in range(len(trades_file)):
        line = trades_file[idx]
        # this is already sorted
        details = saos_to_details[line[3]]
        time_id = 0
        pos_id = 1
        px_id = 2
        saos_id  = 3
        trade = (line[time_id], line[pos_id] - trades_file[idx - 1][pos_id], line[px_id], line[saos_id])

        if 'CxlSeqd' in details.status_vec_ and 'Exec' in details.status_vec_ and 'Cxld' not in details.status_vec_:
            time1 = details.send_time_vec_[details.status_vec_.index('CxlSeqd')]
            time2 = details.send_time_vec_[details.status_vec_.index('Exec')]
            if time2 - time1 > ignore_values_below:

            # this is cancel reject because of execution

                print('Cancel Reject message', trade)

                found = False
                idx = 0
                while idx < len(cxl_reject_info_vec):
                    if cxl_reject_info_vec[idx][px_id] == trade[px_id]:
                        new_update = (cxl_reject_info_vec[idx][time_id],
                                      cxl_reject_info_vec[idx][pos_id] + trade[pos_id],
                                      cxl_reject_info_vec[idx][px_id],
                                      cxl_reject_info_vec[idx][saos_id])
                        if new_update[pos_id] != 0:
                            cxl_reject_info_vec[idx] = new_update
                        else:
                            del cxl_reject_info_vec[idx]
                        found = True
                    idx += 1
                if not found:
                    cxl_reject_info_vec.append(trade)

                all_trades_vec.append(trade)
            else:
                print('Ignoring the reject with Times', time1, time2, time2 - time1, 'Thresh', ignore_values_below)


        if len(cxl_reject_info_vec) > 0 and idx > 0:
            # some data here
            last_trade_size = line[pos_id] - trades_file[idx-1][pos_id]
            cxl_idx = 0
            while cxl_idx < len(cxl_reject_info_vec):
                if last_trade_size*cxl_reject_info_vec[cxl_idx][pos_id] < 0:
#                   print('GOT Opposite Trade', cxl_reject_info_vec[cxl_idx], cxl_idx, 'ORIG: ', line, 'SIZE', last_trade_size)
                    # closing trade to last update in vector
                    trade = (line[time_id], last_trade_size, line[px_id], line[saos_id])

                    remaining_size = cxl_reject_info_vec[cxl_idx][pos_id] + last_trade_size

                    if remaining_size != 0 and abs(last_trade_size) <= abs(cxl_reject_info_vec[cxl_idx][pos_id]):
                        cxl_reject_info_vec[cxl_idx] = (line[time_id], remaining_size, cxl_reject_info_vec[cxl_idx][2], line[saos_id])
                        all_trades_vec.append((line[time_id], last_trade_size, line[px_id], line[saos_id]))
                    else:
                        all_trades_vec.append((line[time_id], -1 * cxl_reject_info_vec[cxl_idx][pos_id], line[px_id], line[saos_id]))
                        del cxl_reject_info_vec[cxl_idx]

                    # print('FOUND', cxl_reject_info_vec, 'ALL_TRADES', all_trades_vec)
                    break

                cxl_idx += 1

    all_trades = 0
    cxl_rej_trade = 0
    all_volume = 0
    cxl_vol = 0
    for trades in all_trades_vec:
        all_trades -= (trades[pos_id]*trades[px_id])
        all_volume += abs(trades[pos_id])

    for ctrades in cxl_reject_info_vec:
        cxl_rej_trade -= (ctrades[pos_id]*ctrades[px_id])
        cxl_vol += abs(ctrades[pos_id])

    print('SACI', saci, 'PNLS', all_trades, cxl_rej_trade, 'PNL_BY_REJ', (all_trades-cxl_rej_trade)*mpi,
          'VOL', all_volume - cxl_vol)

