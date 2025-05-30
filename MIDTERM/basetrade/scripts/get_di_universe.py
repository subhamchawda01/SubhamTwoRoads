#!/usr/bin/python

# def GetUniverse():
#    return options
from datetime import date
import sys
import os
import subprocess
import random

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.definitions import execs


def GetDaystoExpiry(dateNow, shortcode):
    expiry_year = 2000 + int(shortcode[4:6])
    expiry_month = 1
    if shortcode[3] == 'F':
        expiry_month = 1
    if shortcode[3] == 'J':
        expiry_month = 4
    if shortcode[3] == 'N':
        expiry_month = 7
    if shortcode[3] == 'V':
        expiry_month = 10
    expiry_date = 1
    current_year = int(dateNow[0:4])
    curent_month = int(dateNow[4:6])
    curent_date = int(dateNow[6:8])
    currDate = date(current_year, curent_month, curent_date)
    expDate = date(expiry_year, expiry_month, expiry_date)
    numDaysToExpiry = (expDate - currDate).days
    return numDaysToExpiry


def GetDV01Volume(dateNow, shortCode):
        #print(shortCode , dateNow)
    avg_sample_cmd = [execs.execs().avg_samples, shortCode, dateNow, "60", "BRT_905", "BRT_1540", "0", "VOL"]
    # print(avg_sample_cmd)
    process = subprocess.Popen(' '.join(avg_sample_cmd), shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    # print(err)
    if err is not None:
        err = err.decode('utf-8')
    errcode = process.returncode
    if len(err) > 0:
        raise ValueError("Error in fetching sample data")
    volume = str(out.strip().split()[2])
    dv01_cmd = [execs.execs().di1_dv01_val, shortCode, dateNow]
    # print(dv01_cmd)
    process = subprocess.Popen(' '.join(dv01_cmd), shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    out, err = process.communicate()
    # print(err)
    if out is not None:
        out = out.decode('utf-8')
    # if err is not None:
    #    err = err.decode('utf-8')
    #errcode = process.returncode
    # if len(err) > 0:
    #    raise ValueError("Error in fetching dv01 info "+err)
    dv01 = out.strip()
    dv01_volume = float(dv01) * float(volume)
    return dv01_volume


def GetDV01Stdev(dateNow, shortCode):
    avg_sample_cmd = [execs.execs().avg_samples, shortCode, dateNow, "30", "BRT_905", "BRT_1540", "0", "STDEV"]
    process = subprocess.Popen(' '.join(avg_sample_cmd), shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')
    errcode = process.returncode
    if len(err) > 0:
        raise ValueError("Error in fetching sample data")
    stdev = str(out.strip().split()[2])
    dv01_cmd = [execs.execs().di1_dv01_val, shortCode, dateNow]
    process = subprocess.Popen(' '.join(dv01_cmd), shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    # if err is not None:
    #    err = err.decode('utf-8')
    #errcode = process.returncode
    # if len(err) > 0:
    #    raise ValueError("Error in fetching dv01 info "+err)
    dv01 = out.strip()
    dv01_stdev = float(dv01) * float(stdev)
    return dv01_stdev


def GetStdev(dateNow, shortCode):
    avg_sample_cmd = [execs.execs().avg_samples, shortCode, dateNow, "30", "BRT_905", "BRT_1540", "0", "STDEV"]
    process = subprocess.Popen(' '.join(avg_sample_cmd), shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')
    errcode = process.returncode
    if len(err) > 0:
        raise ValueError("Error in fetching sample data")
    stdev = str(out.strip().split()[2])
    dv01_cmd = [execs.execs().di1_dv01_val, shortCode, dateNow]
    process = subprocess.Popen(' '.join(dv01_cmd), shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    # if err is not None:
    #    err = err.decode('utf-8')
    #errcode = process.returncode
    # if len(err) > 0:
    #    raise ValueError("Error in fetching dv01 info "+err)
    dv01 = out.strip()
    stdev1 = float(stdev)
    return stdev1


def GetDV01(dateNow, shortCode):
    avg_sample_cmd = [execs.execs().avg_samples, shortCode, dateNow, "30", "BRT_905", "BRT_1540", "0", "STDEV"]
    process = subprocess.Popen(' '.join(avg_sample_cmd), shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')
    errcode = process.returncode
    if len(err) > 0:
        raise ValueError("Error in fetching sample data")
    stdev = str(out.strip().split()[2])
    dv01_cmd = [execs.execs().di1_dv01_val, shortCode, dateNow]
    process = subprocess.Popen(' '.join(dv01_cmd), shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    
    dv01 = out.strip()
    dv1 = float(dv01)
    return dv1


#In GetShortCodeList is to decide universe of shortCode to trade on a particular date . We decide it by cutoff on DV01Volume of the product and check that the shortcode is not very near to expiry

def GetShortCodeList(dateNow):
    print("Error in DateNow is in : ", dateNow)
    print(dateNow)
    current_year = int(dateNow[0:4])
    end_year = current_year + 12
    shortCodeList = []
    for year in range(current_year, end_year):
        for quarter in range(1, 4):
            m = "F"
            if quarter == 1:
                m = "F"
            if quarter == 2:
                m = "J"
            if quarter == 3:
                m = "N"
            if quarter == 4:
                m = "V"
            shortCode = "DI1" + m + str(year - 2000)
            numDaysToExpiry = GetDaystoExpiry(dateNow, shortCode)
            if numDaysToExpiry > 270:
                dv01Vol = GetDV01Volume(dateNow, shortCode)
                if dv01Vol > 2000:
                    shortCodeList.append([shortCode, dv01Vol])
    return shortCodeList


def WriteShortCodeListtoFile(dateNow, configId):
    shortCodeList = GetShortCodeList(dateNow)
    work_dir = execs.get_temp_location()

    if not os.path.isdir(work_dir):
        os.system("mkdir " + work_dir)
    work_dir_loc = work_dir + "/temp_models/"
    if not (os.path.isdir(work_dir_loc)):
        os.system("mkdir " + work_dir_loc)
    shcString = ""
    for shc in shortCodeList:
        shcString += ""
        shcString += "INDICATOR 1 " + shc[0] + " " + str(shc[1]) + "\n"

    model_file = work_dir_loc + "model_file_date_" + str(dateNow) + "_configid_" + str(configId)
    f = open(model_file, 'w')
    f.write(shcString)
    f.close()

    return model_file


'''shortCode=sys.argv[1]
mode=sys.argv[2]
dateNow="20160101"
currDate="20171013"
while dateNow<currDate:
    if mode=="Stdev":
        dv01Stdev=GetStdev(dateNow,shortCode)
    if mode=="DV01":
        dv01Stdev=GetDV01(dateNow,shortCode)
    if mode=="DV01Stdev":
        dv01Stdev=GetDV01Stdev(dateNow,shortCode)
    
    print(dateNow,"\t",dv01Stdev)
    dateNow=os.popen("/home/dvctrader/basetrade_install/bin/calc_next_week_day "+dateNow+" 1").read()'''
