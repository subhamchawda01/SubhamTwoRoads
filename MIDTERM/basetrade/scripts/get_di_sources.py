#!/usr/bin/python

from datetime import date
import sys
import os
import subprocess
import math
import operator
import heapq
import functools
import time

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.definitions import execs
from scripts.get_di_universe import GetDV01Volume
from scripts.get_di_universe import GetShortCodeList


def NumMonthsToExpiryFrom2000(IndepShortCode):
    if IndepShortCode[3] == 'F':
        mValInDep = 1
    if IndepShortCode[3] == 'J':
        mValInDep = 4
    if IndepShortCode[3] == 'N':
        mValInDep = 7
    if IndepShortCode[3] == 'V':
        mValInDep = 10
    indep_year = int(IndepShortCode[4:6])
    return 12 * indep_year + mValInDep


def CloseNessScore(DepShortCode, IndepShortCode):
    closeness = abs(NumMonthsToExpiryFrom2000(DepShortCode) - NumMonthsToExpiryFrom2000(IndepShortCode))
    return 100 / math.sqrt(closeness)


def NotionalScore(dateNow, IndepShortCode):
    score = GetDV01Volume(dateNow, IndepShortCode) / 1000
    return score


def CompareExpiry(shc1, shc2):
    if NumMonthsToExpiryFrom2000(shc1) > NumMonthsToExpiryFrom2000(shc2):
        return 1
    return -1


def TotalScore(dateNow, depShortCode, indepShortCode):
    return CloseNessScore(depShortCode, indepShortCode) + NotionalScore(dateNow, indepShortCode)


def IsDepExpiryEarly(dep, indep):
    return -CompareExpiry(dep, indep)


def IsDepExpiryLate(dep, indep):
    return CompareExpiry(dep, indep)


def IsDepExpiryMiddle(dep, indep1, indep2):
    netVal = CompareExpiry(dep, indep1) + CompareExpiry(dep, indep2)
    if netVal == 0:
        return 1
    return 0


#GetSources function mainly output source for the shortcode amongst the universe of shortcodes to  be traded on that which are always passed in this as parameter . Here we choose number of leading Shortcodes which is also parameter and we decide leading shortcodes by DV01Volume . Out of these leading shortcodes sources are chosen by nearness propert to dep Shortcode. Here nearness is defined by closeness of  expiry between shortcodes.

def GetSources(shortCode, numSourcesLeading, dateNow, universe):
    t0 = time.time()
    shortCodeUniverse = universe
    if universe == []:
        shortCodeUniverse = GetShortCodeList(dateNow)
    allShortCodeDict = dict()
    shortCodeDict = dict()
    for shc in shortCodeUniverse:
        print(shc)
        allShortCodeDict[shc[0]] = float(shc[1]) / 1000
        if shc[0] != shortCode:
            # shortCodeDict[shc]=NotionalScore(dateNow,shc)
            shortCodeDict[shc[0]] = float(shc[1]) / 1000
    leadingShortCodeList = heapq.nlargest(numSourcesLeading, shortCodeDict, key=shortCodeDict.get)
    leadingShortCodeList2 = sorted(leadingShortCodeList, key=functools.cmp_to_key(CompareExpiry))

    entryFlag = 0
    exitFlag = 0
    finalShortCodeList = []
    shc_prev = ""
    volumeCutoff = 1.5
    for shc in leadingShortCodeList2:
        if entryFlag == 0:
            entryFlag = 1
            # print(IsDepExpiryEarly(shortCode,shc))
            if IsDepExpiryEarly(shortCode, shc) == 1:
                exitFlag = 1
                if allShortCodeDict[shortCode] < volumeCutoff * allShortCodeDict[shc]:
                    finalShortCodeList.append(shc)
                    break
        else:
            if IsDepExpiryMiddle(shortCode, shc_prev, shc) == 1:
                exitFlag = 1
                if allShortCodeDict[shortCode] < volumeCutoff * allShortCodeDict[shc_prev]:
                    finalShortCodeList.append(shc_prev)
                if allShortCodeDict[shortCode] < volumeCutoff * allShortCodeDict[shc]:
                    finalShortCodeList.append(shc)
                break
        shc_prev = shc
    if exitFlag == 0:
        if IsDepExpiryLate(shortCode, shc_prev):
            if allShortCodeDict[shortCode] < volumeCutoff * allShortCodeDict[shc_prev]:
                finalShortCodeList.append(shc_prev)
    t1 = time.time()
    print("Time required for GetSources : ", (t1 - t0))
    return finalShortCodeList, allShortCodeDict


# depShortCode=sys.argv[1]
# numSourcesLeading=int(sys.argv[2])
# dateNow=sys.argv[3]


# GetSources(depShortCode,numSourcesLeading,dateNow)
