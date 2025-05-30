#!/usr/bin/env python

"""
Some fancy print

"""
import sys

from tabulate import tabulate


def pretty_print_dict(tdict, colname1='data', colname2='field_names'):
    """

    :param tdict:
    :param colname1:
    :param colname2:
    :return:
    """
    data = tdict[colname1]
    tlist = [tdict[colname2]]
    tmp = [list(x) for x in data]
    tlist = tlist + tmp
    print(tabulate(tlist))
