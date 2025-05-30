#!/usr/bin/env python

import os
import sys


def __main__():
    if len(sys.argv) < 2:
        print("Need globres filename!")
        sys.exit(0)

    strat_to_res_map_ = {}
    input_file_ = open(sys.argv[1])
    for input_line_ in input_file_:
        input_words_ = input_line_.strip().split()
        if (len(input_words_) > 1):
            strat_to_res_map_[input_words_[0]] = input_line_.strip()

    for strat_line_ in strat_to_res_map_.values():
        print(strat_line_)


__main__()
