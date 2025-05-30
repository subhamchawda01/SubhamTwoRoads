#!/usr/bin/python

import os
import sys
import re
#from subprocess import Popen, PIPE
#from datetime import datetime


def __main__():

    if len(sys.argv) < 2:
        print(("Usage: %s <input_timestamp_data>" % sys.argv[0]))
        exit(0)

    input_filename_ = sys.argv[1]

    this_time_block_ = ""
    last_timestamp_ = 0
    with open(input_filename_) as input_file_handle_:
        for input_line_ in input_file_handle_:
            #print ( "this line %s" % input_line_ )
            input_line_words_ = input_line_.strip().split()
            if (input_line_words_[0] == 'Time:') and (len(input_line_words_) >= 5):
                if (float(input_line_words_[4]) > last_timestamp_):
                    #print ( "PRINT %s CBEND" % this_time_block_ );
                    print(this_time_block_)
                    this_time_block_ = ""
                    last_timestamp_ = float(input_line_words_[4])
                else:
                    # even if it is the same timestamp forget what you read so far
                    #print ( "FORGET %s CBEND" % this_time_block_ );
                    this_time_block_ = ""
            this_time_block_ += input_line_
            #print ( "CURRENT %s CBEND" % this_time_block_ );

    print(this_time_block_)


__main__()
