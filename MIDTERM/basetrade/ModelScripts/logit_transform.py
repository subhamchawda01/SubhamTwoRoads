#!/usr/bin/python

import os
import os.path
import sys
import math


def sigmoid(x):
    if (x < -5):
        return 0
    if (x > 5):
        return 1
    try:
        retval = 1 / (1 + math.exp(-x))
        return retval
    except OverflowError:
        if (x < 0):
            return 0
        else:
            return 1
    return 0.5


class ColumnTransform:
    def __init__(self, t_multiplier_):
        self.count_seen_ = 0
        self.mean_value_ = 0
        self.stdev_value_ = 0
        self.boundary_value_ = 0
        self.sum_values_ = 0
        self.sum_square_values_ = 0
        self.multiplier_ = t_multiplier_

    def AddSample(self, t_value_):
        self.count_seen_ += 1
        self.sum_values_ += t_value_
        self.sum_square_values_ += t_value_ * t_value_

    def CalcBoundary(self):
        self.mean_value_ = self.sum_values_ / self.count_seen_
        if (self.count_seen_ > 2):
            self.stdev_value_ = math.sqrt(
                (self.sum_square_values_ - (self.sum_values_ * self.mean_value_)) / (self.count_seen_ - 1))
        else:
            self.stdev_value_ = math.fabs(self.mean_value_)
        self.boundary_value_ = self.multiplier_ * self.stdev_value_

    def Transform(self, t_value_):
        if (self.stdev_value_ > 0):
            return ((2 * sigmoid((t_value_ - self.mean_value_) / self.stdev_value_) - 1) * self.boundary_value_)
        else:
            return t_value_


def __main__():

    if len(sys.argv) < 3:
        print("Usage: input_filename stdev_multiplier output_filename")
        sys.exit()

    input_filename_ = sys.argv[1]
    stdev_multiplier_ = float(sys.argv[2])
    output_file_handle_ = open(sys.argv[3], 'w')

    column_transform_list_ = []
    if os.path.isfile(input_filename_):
        with open(input_filename_) as input_file_handle_calc_:
            for input_line_ in input_file_handle_calc_:
                input_line_words_ = input_line_.strip().split()
                if len(column_transform_list_) == 0:
                    if len(input_line_words_) > 0:
                        for i in range(len(input_line_words_)):
                            column_transform_list_.append(ColumnTransform(stdev_multiplier_))
                else:
                    if len(input_line_words_) == len(column_transform_list_):
                        for i in range(len(input_line_words_)):
                            t_ret_value_ = column_transform_list_[i]. AddSample(float(input_line_words_[i]))

        for t_column_transform_ in column_transform_list_:
            t_column_transform_.CalcBoundary()

        with open(input_filename_) as input_file_handle_:
            for input_line_ in input_file_handle_:
                input_line_words_ = input_line_.strip().split()
                if len(input_line_words_) == len(column_transform_list_):
                    for i in range(len(input_line_words_)):
                        t_ret_value_ = column_transform_list_[i]. Transform(float(input_line_words_[i]))
                        output_file_handle_.write("" + str(t_ret_value_) + " ")
                    if len(input_line_words_) > 0:
                        output_file_handle_.write("\n")


__main__()
