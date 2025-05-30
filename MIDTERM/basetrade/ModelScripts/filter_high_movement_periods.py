#!/usr/bin/env python

import os
import sys


def __main__():
    if len(sys.argv) < 4:
        print "inputfile windowlength fraction"
        sys.exit(0)
    # open file
    input_file_ = open(sys.argv[1])
    _window_length = int(sys.argv[2])
    _fraction = float(sys.argv[3])

    # Logic:
    # read each line and try to calculate the average value of abs(column 1) for the file.
    # calculate the number of lines in the file.
    # seek to beginning of file
    # for sets of say 100 lines, see the total sum of the values of column1 and
    # if absolute value of it exceeds 0.2 * 100 * average ( abs ( column 1 ) ) then print that set out to the file.
    sum_ = 0
    count_ = 0
    for input_line_ in input_file_:
        input_words_ = input_line_.strip().split()
        if len(input_words_) >= 2:
            sum_ += abs(float(input_words_[0]))
            count_ += 1.0

    average_abs_value_ = sum_ / count_
    _significant_sum = _fraction * _window_length * average_abs_value_

    sum_ = 0
    count_ = 0
    _current_line_set = []
    input_file_.seek(0, 0)  # seek to beginning of file
    for input_line_ in input_file_:
        input_words_ = input_line_.strip().split()
        if len(input_words_) >= 2:
            sum_ += float(input_words_[0])
            count_ += 1.0
            _current_line_set.append(input_line_.strip())
            if (count_ >= _window_length):
                if (abs(sum_) >= _significant_sum):
                    for _saved_line in _current_line_set:
                        print _saved_line
                _current_line_set = []
                count_ = 0
                sum_ = 0

    input_file_.close()


__main__()
