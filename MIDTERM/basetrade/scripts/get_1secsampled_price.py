#!/usr/bin/python


import os
import sys
import re


data = open(sys.argv[1]).readlines()


L1event = []
trade = []


def getTime(line):
    return float(line[1])


class L1data:
    def __init__(self, line):
        self.bo = int(line[3])
        self.bs = int(line[5])
        self.bp = float(line[6])

        self.ao = int(line[7])
        self.as = int(line[8])
        self.ap = float(line[9])

    def +(self, l2):
        self


for l in data:
    l = l.strip().split()
    if getTime(l) ==
