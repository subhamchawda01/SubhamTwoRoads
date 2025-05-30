#!/usr/bin/env python

"""
The purpose of the script is to make an estimate of how many orders where executed before us

"""


import os
import sys
import argparse


def compute_qpos(shortcode, tradingdate):
    return

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', dest='shortcode', help="shortcode string", type=str, required=True)
    parser.add_argument('-d', dest='tradingdate', help=' tradingdate ', type=int, required=True)

    args = parser.parse_args()

    compute_qpos(args.shortcode, args.tradingdate)
