#!/usr/bin/env python

"""
Get the stats of seqd-conf numbers

"""


import subprocess
import copy

from walkforward.definitions.execs import execs

from pylib.definitions.ors_struct import ORSStruct


def get_ors_binary_reader_all_data(shortcode, tradingdate):
    """

    Takes input two things shortcode and tradingdate,
    For each SAOS in for shortcode for that day, it creates it's details. The details include it's order-status in
    given saos's lifetime

    :param shortcode:
    :param tradingdate:
    :return:
    """

    ors_binary_reader_cmd = [execs().ors_binary_reader, shortcode, str(tradingdate)]
    output_val = subprocess.check_output(ors_binary_reader_cmd).decode('utf-8').strip().splitlines()

    summary_map = {'new_order': 0, 'cxlseqd': 0, 'cxld': 0, 'exec': 0, 'uniq_exec': 0, 'conf': 0,
                   'cxl_rejc': 0, 'modify': 0, 'rejc': 0}

    saci_to_summary_map = {}

    saos_to_details = {}
    for line in output_val:
        line_words = line.split()
        # in case of two words shortcodes this code needs separate handling

        saos_index = line_words.index('SAOS:') + 1
        saos = int(line_words[saos_index])

        saci_index = line_words.index('SACI:') + 1
        saci = int(line_words[saci_index])

        if saos == 0:
            continue

        if saci not in saci_to_summary_map.keys():
            saci_to_summary_map[saci] = copy.copy(summary_map)

        details_struct = None
        if saos in saos_to_details.keys():
            details_struct = saos_to_details[saos]
        else:
            details_struct = ORSStruct()
        send_time_index = line_words.index('ST:') + 1
        data_time_index = line_words.index('DT:') + 1
        if data_time_index == 0:
            data_time_index = line_words.index('CT:') + 1
        order_status_index = line_words.index('ORR:') + 1
        position_index = line_words.index('CLTPOS:') + 1

        price_index = line_words.index('Px:') + 1

        details_struct.send_time_vec_.append(float(line_words[send_time_index]))
        details_struct.data_time_vec_.append(float(line_words[data_time_index]))
        details_struct.status_vec_.append(line_words[order_status_index])
        details_struct.pos_vec_.append(int(line_words[position_index]))
        details_struct.price_vec_.append(float(line_words[price_index]))

        if line_words[order_status_index] == 'Seqd':
            # get the index for each of the tags
            # To support the case of ICE shortcodes, we are getting the index for different tags

            caos_index = line_words.index('CAOS:') + 1

            int_price_index = line_words.index('INTPX:') + 1
            buysell_index = line_words.index('BS:') + 1

            size_remaining_index = line_words.index('SIZE:') + 1
            size_executed_index = line_words.index('SE:') + 1
            order_id_index = line_words.index('Seq:') + 1

            # information which would mostly remain same
            details_struct.saos_ = saos
            details_struct.caos_ = int(line_words[caos_index])
            details_struct.price_ = float(line_words[price_index])
            details_struct.int_price_ = float(line_words[int_price_index])
            details_struct.buysell_ = line_words[buysell_index]
            details_struct.saci_ = int(line_words[saci_index])

            details_struct.size_remaining_ = int(line_words[size_remaining_index])
            details_struct.size_executed_ = int(line_words[size_executed_index])
            details_struct.order_id_ = int(line_words[order_id_index])

            saci_to_summary_map[saci]['new_order'] += 1

        elif line_words[order_status_index] == 'Conf':
            # order id changes to exchange provided one
            order_id_index = line_words.index('Seq:') + 1
            details_struct.order_id_ = int(line_words[order_id_index])
            saci_to_summary_map[saci]['conf'] += 1
        elif line_words[order_status_index] == 'CxlSeqd':
            l = ''  # print(line)
            saci_to_summary_map[saci]['cxlseqd'] += 1
        elif line_words[order_status_index] == 'Cxld':
            saci_to_summary_map[saci]['cxld'] += 1
            l = ''  # print(line)
        elif line_words[order_status_index] == 'Exec':
            size_executed_index = line_words.index('SE:') + 1
            details_struct.size_executed_ = int(line_words[size_executed_index])

            saci_to_summary_map[saci]['exec'] += 1
            # if there was no previous exec message of the saos, record that
            if 'Exec' not in details_struct.status_vec_[:-1]:
                saci_to_summary_map[saci]['uniq_exec'] += 1

                # if we had cxl_seqd, it would be recorded in first exec only
                if 'CxlSeqd' in details_struct.status_vec_:
                    # print('REJ', details_struct.to_string())
                    saci_to_summary_map[saci]['cxl_rejc'] += 1

        elif line_words[order_status_index] == 'CxlRe':
            # price and size could change
            price_index = line_words.index('Px:') + 1
            int_price_index = line_words.index('INTPX:') + 1
            size_remaining_index = line_words.index('SIZE:') + 1

            details_struct.price_ = float(line_words[price_index])
            details_struct.int_price_ = int(line_words[int_price_index])
            details_struct.size_remaining_ = int(line_words[size_remaining_index])

            saci_to_summary_map[saci]['modify'] += 1
        elif line_words[order_status_index] == 'Rej':
            size_executed_index = line_words.index('SE:') + 1
            details_struct.reject_reason_ = line_words[size_executed_index]
            saci_to_summary_map[saci]['rejc'] += 1
        elif line_words[order_status_index] == 'CxlReject':
            size_executed_index = line_words.index('SE:') + 1
            details_struct.cancel_reject_reason_ = line_words[size_executed_index]

            # saci_to_summary_map[saci]['cxl_rejc'] += 1
        # print('OS', saos, details_struct.to_string(), 'LN', line )
        saos_to_details[saos] = details_struct

    return saos_to_details, saci_to_summary_map
