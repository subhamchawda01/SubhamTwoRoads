#!/usr/bin/python

# this program takes 3+ arguments.
# First argument ( sys.argv[1] ) is an input data file
# which is of the format <depvalue> <indep_1> <indep_2> .... <indep_n>
# 2nd argument ( sys.argv[2] ) is the output of linear regression which has lines like :
# OutCoeff 3 0.5
# From this we build a map coeff_wt_map_ in the first block
# Next argument ( sys.argv[3] ) is the output file, where we want to write the target price
# Optional fourth argument is a configurationof whether the model type is LASSo or not since in that case the line format has just "3 0.5" instead of "OutCoeff 3 0.5"

import os
import os.path
import sys


def __main__():

    if len(sys.argv) < 4:
        print("Usage: input_datafile regress_results_file output_yhat_file [output_type=0]")
        sys.exit()

    coeff_wt_map_ = {}  # initialize map
    regress_results_filename_ = sys.argv[2]
    output_type_ = 0
    if len(sys.argv) >= 5:  # in case optional argument provided
        output_type_ = int(sys.argv[4])

    if os.path.isfile(regress_results_filename_):  # if file exists
        with open(regress_results_filename_) as regress_results_file_handle_:  # decaring a local variable by 'with'
            # this is one of the most efficient ways of reading files from a big data file, line by line
            for regress_results_output_line_ in regress_results_file_handle_:
                regress_results_output_line_words_ = regress_results_output_line_.strip().split()  # returns a list of words in the line
                if output_type_ == 0:
                    if (len(regress_results_output_line_words_) >= 3) and (regress_results_output_line_words_[0] == "OutCoeff"):
                        coeff_wt_map_[(int)(regress_results_output_line_words_[1])] = (
                            float)(regress_results_output_line_words_[2])  # build a map
                elif output_type_ == 1:  # LASSO
                    if (len(regress_results_output_line_words_) >= 2):
                        coeff_wt_map_[(int)(regress_results_output_line_words_[0])] = (
                            float)(regress_results_output_line_words_[1])

        output_yhat_file_handle_ = open(sys.argv[3], 'w')  # open file for writing

        with open(sys.argv[1]) as reg_data_file_handle_:
            for reg_data_line_ in reg_data_file_handle_:  # this is one of the most efficient ways of reading files from a big data file, line by line
                reg_data_line_words_ = reg_data_line_.strip().split()  # returns a list of words in the line
                this_yhat_value_ = 0.0
                for key, value in coeff_wt_map_.iteritems():
                    # key+1 should be index in the list. Ideally we should check if such words exist. This code is not written well.
                    this_yhat_value_ = this_yhat_value_ + ((float)(reg_data_line_words_[key + 1]) * value)

                # write used to output to the file
                output_yhat_file_handle_.write("" + str(reg_data_line_words_[0]) + " " + str(this_yhat_value_) + "\n")


__main__()
