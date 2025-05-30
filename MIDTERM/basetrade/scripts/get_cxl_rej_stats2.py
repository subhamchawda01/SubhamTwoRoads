#!/usr/bin/env python

"""
"""

import os
import sys
import argparse
import subprocess

sys.path.append(os.path.expanduser('~/basetrade'))

from walkforward.definitions.execs import execs

from pylib.get_ors_binary_reader_stats import get_ors_binary_reader_all_data

def get_seq2conf_for_percentile(percentile, shortcode, date):
    # Produces the summary and then parsing it
    cmd = ('%s %s %s SUMMARY' % (execs().ors_binary_reader, shortcode, date)).split()
    cout = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    text = cout.decode('utf-8').split()
    percentile_dict = {}
    units = 10**6
    percentile_dict[50] = float(int(text[9].split(':')[1])) / units
    percentile_dict[75] = float(int(text[10].split(':')[1])) / units
    percentile_dict[90] = float(int(text[11].split(':')[1])) / units
    percentile_dict[95] = float(int(text[12].split(':')[1])) / units
    percentile_dict[99] = float(int(text[13].split(':')[1])) / units

    if percentile in percentile_dict:
        return percentile_dict[percentile]
    else:
        # Throw invalid percentile error here.
        sys.stderr.write("Invalid percentile mentioned. Only available options are: 50, 75, 90, 95, 99")
        exit()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-shc', dest='shortcode', help="shortcode", type=str, required=True)
    parser.add_argument('-date', dest='date', help="trading date", type=str, required=True)
    parser.add_argument('-o', dest='output_filename', help="output_filename", type=str, required=True)
    parser.add_argument('-p', dest='percentile', help="SeqdToConf Percentile. The output is Cxl_rejects where time "
                                                      "difference is more than given SeqdConf Percentile [50, 75, 90, 95]", type=int, required=False)
    parser.add_argument('-c', dest='counts', help='Count of CxlRejects and CxlSeqd, Cxld Messages per SACI', type=int, required=False)

    args = parser.parse_args()

    saos_details_map, summary_data = get_ors_binary_reader_all_data(args.shortcode, args.date)

    if args.counts and args.counts > 0:
        of = open(args.output_filename, 'w')
        # #cxl_seqd #cxld #cxl_rej #saci
        for key in summary_data.keys():
            of.write('%d %d %d %d\n' % (summary_data[key]['cxlseqd'], summary_data[key]['cxld'], summary_data[key]['cxl_rejc'], key))

        of.close()
    else:
        of = open(args.output_filename, 'w')

        threshold = 0
        if args.percentile:
            threshold = get_seq2conf_for_percentile(args.percentile, args.shortcode, args.date)

        of.write("CxldSeq-TS    Exec-TS SACI\n")

        for saos in saos_details_map.keys():
            if 'CxlSeqd' in saos_details_map[saos].status_vec_ and 'Exec' in saos_details_map[saos].status_vec_ \
                    and 'Cxld' not in saos_details_map[saos].status_vec_:
                # orders for which we sent cancellations but received only execs
                cxlseq_time = saos_details_map[saos].send_time_vec_[saos_details_map[saos].status_vec_.index('CxlSeqd')]
                exec_time = saos_details_map[saos].send_time_vec_[saos_details_map[saos].status_vec_.index('Exec')]
                if not args.percentile or (args.percentile and exec_time - cxlseq_time < threshold):
                    of.write('%0.6f %0.6f %d\n' % (float(cxlseq_time), float(exec_time), saos_details_map[saos].saci_))

        # outside
        of.close()