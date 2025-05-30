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
from scripts.get_di_universe import GetDV01Stdev
from scripts.get_di_universe import GetShortCodeList
from scripts.get_avg_trsz import GetAvgTradeSize


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


def GetTickSize(shortCode, dateNow):
    out = os.popen("/home/pengine/prod/live_execs/get_contract_specs " + shortCode+" " + dateNow + " ALL | grep TICKSIZE | awk '{print $2}' ").read()


    #  Couldn't  find execs  which was working ,it is in Todo List here to add execs.

    '''get_ticksz_cmd = [execs.execs().get_contract_specs, shortCode,dateNow, "TICKSIZE"]
    process = subprocess.Popen(' '.join(get_ticksz_cmd), shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    # print(err)
    if err is not None:
        err = err.decode('utf-8')
    errcode = process.returncode
    if len(err) > 0:
        raise ValueError("Error in running exec")
    outputArray = out.split()'''



    
    return float(out.strip())


def GetParamsForShortCode(shortCode, dateNow, baseParam, leaderShc, baseDate, baseUTS, workDir, configId):
    t0 = time.time()

    if not os.path.isdir(workDir):
        os.system("mkdir " + workDir)
    if not (os.path.isdir(workDir + shortCode + "/")):
        os.system("mkdir " + workDir + shortCode)

    utsLeader = GetDV01Stdev(baseDate, leaderShc) * baseUTS / GetDV01Stdev(dateNow, leaderShc)
    uts = round(utsLeader / 5.0) * 5
    if shortCode != leaderShc and shortCode[3] == 'N':
        outLeaderTRSize = str(GetAvgTradeSize(leaderShc,dateNow,10))
        print("LeaderShortCode UTS:", utsLeader)
        print("LeaderShtCode Trade Size:", outLeaderTRSize)
        outMyTRSize = str(GetAvgTradeSize(shortCode,dateNow,10))
        print("DepShortCode Trade Size :", outMyTRSize)
        print("UTS :", uts, " IndepTradeSize :", outLeaderTRSize, "DepTradeSize: ", outMyTRSize)
        uts = round(float(utsLeader) * float(outMyTRSize) / (float(outLeaderTRSize) * 5.0)) * 5
    else:
        DV01StdevLeaderBaseDate = GetDV01Stdev(baseDate, leaderShc)
        DV01StdevDepCurrentDate = GetDV01Stdev(dateNow, shortCode)
        print("DV01sTDEV basedate leader :", DV01StdevLeaderBaseDate)
        print("DV01sTDEV CurrentDate Dep :", DV01StdevDepCurrentDate)
        uts = GetDV01Stdev(baseDate, leaderShc) * baseUTS / GetDV01Stdev(dateNow, shortCode)
        print("UTS:", uts)
        uts = round(uts / 5.0) * 5

    mur = 4
    if GetTickSize(shortCode, dateNow) == 0.01:
        mur = 4
    elif GetTickSize(shortCode, dateNow) == 0.005:
        mur = 5
    paramArray = baseParam.split("\n")
    for index in range(0, len(paramArray)):
        line = paramArray[index]
        lineArray = line.split()
        if len(lineArray) < 3:
            continue
        if lineArray[1] == "MAX_UNIT_RATIO":
            lineArray[2] = str(mur)
        if lineArray[1] == "UNIT_TRADE_SIZE":
            lineArray[2] = str(uts)
        if lineArray[1] == "MAX_OPENTRADE_LOSS":
            openLoss = int(lineArray[2]) * uts / 30
            lineArray[2] = str(openLoss)

        line = ' '.join(lineArray)
        paramArray[index] = line
    newParam = '\n'.join(paramArray)
    newParamfile = workDir + shortCode + "/" + "param_file_date_" + str(dateNow) + "_configid_" + str(configId)
    f = open(newParamfile, 'w')
    f.write(newParam)
    f.close()
    t1 = time.time()
    print("Time Required for GetParamsForShortCode: ", t1 - t0)
    return newParamfile


def GetPreviousDays(dateNow, NumDays):
    calc_cmd = [execs.execs().calc_prev_week_day, dateNow, NumDays]
    process = subprocess.Popen(' '.join(calc_cmd), shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    # print(err)
    if err is not None:
        err = err.decode('utf-8')
    errcode = process.returncode
    if len(err) > 0:
        raise ValueError("Error in running exec")
    date = out
    return date


def GetHighestTradedDI(shortCode, dateNow, universe):
    t0 = time.time()
    shortCodeUniverse = universe
    if universe == []:
        shortCodeUniverse = GetShortCodeList(dateNow)
    allShortCodeDict = dict()
    shortCodeDict = dict()
    for shc in shortCodeUniverse:
        allShortCodeDict[shc[0]] = shc[1] / 1000
        shortCodeDict[shc[0]] = shc[1] / 1000
    leadingShortCodeList = heapq.nlargest(1, shortCodeDict, key=shortCodeDict.get)
    t1 = time.time()
    print("Time required for finding highest traded DI the " + leadingShortCodeList[0] + "is  : ", t1 - t0)
    return leadingShortCodeList[0]


if __name__ == '__main__':
    print(GetTickSize("DI1F21", "20171103"))
