#!/usr/bin/python

from datetime import date
import sys
import os
import subprocess
import math
import operator
import heapq
import time

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.definitions import execs
from scripts.get_di_universe import GetDV01Volume
from scripts.get_di_universe import GetShortCodeList
from scripts.get_di_sources import GetSources


def IsLiquidDI(dateNow, DepShortCode, allShortCodeDict):
    if allShortCodeDict[DepShortCode] > 20:
        return 1
    return 0

#Here we fixed some indicators which are good performing over long term . There are some source based indicators , where sources are retrieved from GetSources module . Also some indicator to use are conditional on Volume feature for recent days of the product

def MakeIlist(dateNow, DepShortCode, universe):
    t0 = time.time()

    ilistString = ""
    ilistString += "MODELINIT DEPBASE " + DepShortCode + " MktSizeWPrice MktSizeWPrice\n"
    ilistString += "MODELMATH LINEAR CHANGE\n"
    ilistString += "INDICATORSTART\n"
    ilistString += "INDICATOR 1.00 ScaledTrendMktEvents BR_DOL_0 30 T OfflineMixMMS\n"
    ilistString += "INDICATOR 1.00 OnlineComputedPair " + DepShortCode + " BR_DOL_0 300 OfflineMixMMS\n"

    sourceShortCodeList, allShortCodeDict = GetSources(DepShortCode, 3, dateNow, universe)

    if IsLiquidDI(dateNow, DepShortCode, allShortCodeDict):
        ilistString += "INDICATOR 1.00 MultMktOrderPrice " + DepShortCode + " 3 0.75 OfflineMixMMS\n"
        ilistString += "INDICATOR 1.00 ScaledTrendMktEvents " + DepShortCode + " 10 T OfflineMixMMS\n"

    elif len(sourceShortCodeList) == 2:
        ilistString += "INDICATOR 1.00 CurveAdjustedSimpleTrendMktEvents " + DepShortCode + \
            " " + sourceShortCodeList[0] + " " + sourceShortCodeList[1] + " 300 T OfflineMixMMS\n"

    for shc in sourceShortCodeList:
        ilistString += "INDICATOR 1.00 ScaledTrendMktEvents " + shc + " 30 T OfflineMixMMS\n"
        ilistString += "INDICATOR 1.00 OnlineComputedPair " + DepShortCode + " " + shc + " 300 OfflineMixMMS\n"

    ilistString += "INDICATOREND"

    t1 = time.time()
    print("Time Required for MakeIlist", t1 - t0)

    return ilistString
