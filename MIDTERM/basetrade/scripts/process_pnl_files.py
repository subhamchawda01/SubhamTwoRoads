#!/usr/bin/env python

import os
import os.path
import sys
import time

import unicodedata
import re
import string

all_chars = (chr(i) for i in range(0x110000))
control_chars = ''.join(c for c in all_chars if unicodedata.category(c) == 'Cc')
# or equivalently and much more efficiently
control_chars = ''.join(map(chr, list(range(0, 32)) + list(range(127, 160))))

control_char_re = re.compile('[%s]' % re.escape(control_chars))

ansi_escape = re.compile(r'\x1b[^m]*m')


def remove_control_chars(s):
    return ansi_escape.sub('', s)
#    return filter(lambda x: x in string.printable, s)
#    return control_char_re.sub('', s)


def process_input_data_line(t_shc_, t_pnl_, t_output_target_file_handle_, t_input_data_line_):
    input_data_line_ = remove_control_chars(t_input_data_line_)
    if (input_data_line_.find("GMT") >= 10):
        # matched with a line which specifies the time like:
        # Wed Jun 10 19:52:10 GMT 2015
        t_datetime_ = time.strptime(input_data_line_, "%a %b %d %H:%M:%S GMT %Y")
        unixtimestr_ = time.strftime("%s", t_datetime_)
        if (t_pnl_ >= -10000000):
            t_output_target_file_handle_.write(unixtimestr_ + " " + str(t_pnl_) + "\n")
            t_pnl_ = 0
    if (input_data_line_.find(t_shc_) >= 0) and (input_data_line_.find("PNL :") >= 0):
        # TOTAL | PNL :    -58.642 | VOLUME :        0 |
        #| I   FMZ0016! | PNL :     -1.239 | POS:    3 | VOL:     0 | v/V: -0.0 | LPX: 99.835000 |

        input_data_line_words_ = input_data_line_.split()
        for i in range(2, (len(input_data_line_words_) - 2)):
            if (input_data_line_words_[i] == "PNL") and (input_data_line_words_[i + 1] == ":"):
                t_pnl_text_ = input_data_line_words_[i + 2]
                if (t_pnl_ < -10000000):
                    # since initial value is -10000001
                    # testing for being less than -10000000 means that it has not been set yet.
                    t_pnl_ = 0
                t_pnl_ += (float)(t_pnl_text_)
                # Question: Why are we adding and not setting equality ?
                # Answer : Suppose you want to see PNL of all BAX contracts together, then we need to see all the lines we find
                # BAX in and probably twice a line if there are twoi BAX contracts in the same line.
    return (t_pnl_)


def __main__():

    if len(sys.argv) < 2:
        print("Usage: shc")
        sys.exit()

    shc_ = sys.argv[1]

#    unixtimestr_="0";
    pnl_ = -10000001
    output_target_file_handle_ = open("/tmp/proc_pnl_file.txt", 'w')  # open file for writing
    for input_data_line_ in sys.stdin:
        final_pnl_ = process_input_data_line(shc_, pnl_, output_target_file_handle_, input_data_line_.strip())
        pnl_ = final_pnl_
    output_target_file_handle_.close()
    os.system("~/basetrade/scripts/plotgraphunix.pl /tmp/proc_pnl_file.txt")
    os.remove("/tmp/proc_pnl_file.txt")


__main__()
