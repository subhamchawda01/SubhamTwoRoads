#!/usr/bin/env python

import os
import sys

fac1 = 4
fac2 = 3
fac3 = 2
fac = 9


def get_median(x):
    if x[0] >= x[1] >= x[2] or x[0] <= x[1] <= x[2]:
        return x[1]
    elif x[1] >= x[0] >= x[2] or x[1] <= x[0] <= x[2]:
        return x[0]
    elif x[1] >= x[2] >= x[0] or x[1] <= x[2] <= x[0]:
        return x[2]
    return x[1]


def combine_outputs(l1, l2, l3):
    return (fac1 * l1 + fac2 * l2 + fac3 * l3) / fac


def print_output(l, l1, l2, l3, num_cols):
    for i in xrange(num_cols - 2):
        print l[i + 2],
    for i in xrange(num_cols - 2):
        print l1[i + 2],
    for i in xrange(num_cols - 2):
        print l2[i + 2],
    for i in xrange(num_cols - 2):
        print l3[i + 2],
    print


def __main__():
    if (len(sys.argv) < 3):
        print("USAGE : <exec> <datagen_output_file> <pred_num_events>")
        exit()
    datagen_file = sys.argv[1]
    event_col = 2
    num_event = (int)(sys.argv[2])
    # na_e3
    la1_event_bias = num_event / 3
    la2_event_bias = 2 * num_event / 3
    la3_event_bias = num_event

    f = open(datagen_file)
    la1 = open(datagen_file)
    la2 = open(datagen_file)
    la3 = open(datagen_file)

    l = f.readline()
    l1 = la1.readline()
    l2 = la2.readline()
    l3 = la3.readline()

    num_col = len(l.split())

    end_reached = False

    while l is not None and l != "":
        base_event = (int)(l.split()[event_col - 1])
        la1_event = base_event + la1_event_bias
        la2_event = base_event + la2_event_bias
        la3_event = base_event + la3_event_bias
        l1_event = (int)(l1.split()[event_col - 1])
        l2_event = (int)(l2.split()[event_col - 1])
        l3_event = (int)(l3.split()[event_col - 1])

        while l1 is not None and l1 != "":
            if l1_event >= la1_event or end_reached:
                break
            else:
                l1 = la1.readline()
                if l1 is None or l1 == "":
                    end_reached = True
                else:
                    l1_event = (int)(l1.split()[event_col - 1])

        while l2 is not None and l2 != "":
            if l2_event >= la2_event or end_reached:
                break
            else:
                l2 = la2.readline()
                if l2 is None or l2 == "":
                    end_reached = True
                else:
                    l2_event = (int)(l2.split()[event_col - 1])

        while l3 is not None and l3 != "":
            if l3_event >= la3_event or end_reached:
                break
            else:
                l3 = la3.readline()
                if l3 is None or l3 == "":
                    end_reached = True
                else:
                    l3_event = (int)(l3.split()[event_col - 1])
        if end_reached:
            break
        print_output(l.split(), l1.split(), l2.split(), l3.split(), num_col)
        l = f.readline()


__main__()
