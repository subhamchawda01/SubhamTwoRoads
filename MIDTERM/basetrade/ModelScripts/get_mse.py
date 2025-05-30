#!/usr/bin/python

import sys


def __main__():

    if len(sys.argv) < 2:
        print("Usage: yhat_file")
        sys.exit()

    cumulative_squared_error_ = 0
    cumulative_variance_dep_ = 0
    count_ = 0
    with open(sys.argv[1]) as input_yhat_file_handle_:
        for input_yhat_line_ in input_yhat_file_handle_:
            input_yhat_line_words_ = input_yhat_line_.strip().split()
            if len(input_yhat_line_words_) >= 2:
                dep_value_ = (float)(input_yhat_line_words_[0])
                this_error_ = (float)(input_yhat_line_words_[1]) - (float)(input_yhat_line_words_[0])
                cumulative_squared_error_ += (this_error_ * this_error_)
                cumulative_variance_dep_ += dep_value_ * dep_value_
                count_ += 1

    mean_squared_error_ = 0
    if count_ > 0:
        mean_squared_error_ = cumulative_squared_error_ / count_

    variance_dep_ = (cumulative_variance_dep_ / count_)
    print mean_squared_error_, (1 - (mean_squared_error_ / variance_dep_))


__main__()
