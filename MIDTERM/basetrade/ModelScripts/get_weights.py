import time
import os
import re
import sys
import copy

modelHeaders = ""
modelFooters = ""


def writeModel(Indicators, write_directory, name):
    file_name = write_directory + name
    f = open(file_name, 'w')
    f.write(modelHeaders)
    # sys.stdout.write(modelHeaders)
    for i in range(len(Indicators)):
        line = ""
        for j in range(len(Indicators[i])):
            line += Indicators[i][j] + " "
        f.write(line[:-1])
        # sys.stdout.write(line[:-1])
    # sys.stdout.write(modelFooters)
    f.write(modelFooters)
    f.close()


def readModel(file_location):
    Inds = []
    modelData = open(file_location).readlines()
    global modelHeaders, modelFooters
    modelHeaders = modelHeaders + (modelData[0] + modelData[1] + modelData[2])
    modelFooters = modelFooters + modelData[-1]
    for line in modelData[3:-1]:
        if line.find("INDICATOR ") == -1:
            continue
        Inds.append(line.split(" "))
    return Inds


def getWeights(Indicators):
    weights = []
    sigmoidParam = []
    for i in range(len(Indicators)):
        param = Indicators[i][1].split(":")
        sigmoidParam.append(param[0])
        weights.append(param[1])
    return sigmoidParam, weights


def getNewInds(Indicators, sigmoidParam, weights):
    newInds = copy.deepcopy(Indicators)
    for i in range(len(Indicators)):
        newInds[i][1] = sigmoidParam[i] + ":" + weights[i]
    return newInds

# f.write('hi there\n') # python will convert \n to os.linesep
#f = open('myfile','w')
# f.close()


Inds = readModel("/home/vdarda/regdata/models/model_temp")
sig, wt = getWeights(Inds)

for i1 in xrange(0, 6):
    for i2 in xrange(0, 6 - i1):
        for i3 in xrange(0, 6 - i1 - i2):
            for i4 in xrange(0, 6 - i1 - i2 - i3):
                for i5 in xrange(0, 6 - i1 - i2 - i3 - i4):
                    if i1 + i2 + i3 + i4 + i5 == 5:
                        new_wt = [str((i1 + 1) * 10), str((i2 + 1) * 10), str((i3 + 1) * 10),
                                  str((i4 + 1) * 10), str((i5 + 1) * 10)]
                        newInds = getNewInds(Inds, sig, new_wt)
                        writeModel(newInds, "/home/vdarda/regdata/models/", "model_temp")
                        #os.system("python /home/vdarda/regdata/run_simu.py")
                        os.system(
                            "python /home/vdarda/basetrade/ModelScripts/run_simu.py SIM /home/vdarda/regdata/models/strat1 2222 20151210 ADD_DBG_CODE -1 /home/vdarda/regdata/ USD000UTSTOM 10")
                        f = open("/home/vdarda/regdata/complete_result", "a")
                        f.write(str(new_wt))
                        f.write("\t")
                        with open("/home/vdarda/regdata/output_tmp", "r") as ins:
                            for line in ins:
                                f.write(line)
                            f.close()


# permute weights
'''
for x in range(1, 6):
    wt1 = [".1",".5",".5",".5",".5"]
    wt1[0] = str(x/10.0)
    newInds = getNewInds(Inds, sig, wt1)
    writeModel(newInds, "/home/vdarda/regdata/models/", "model_temp")
'''

# Strat file change
'''
f = open("/home/vdarda/regdata/models/strat", "a")
stratFile = "/home/vdarda/regdata/models/strat"
underlyer = "USD000UTSTOM"
execLogic = "PriceBasedAggressiveTrading"
modelFile = "/home/vdarda/regdata/models/" + "model_"+str(x)
paramFile = "/home/vdarda/regdata/params/param_4s"
startTime = "EST_300"
endTime   = "EST_915"
handle    = "989398" + str(x)
f.write("STRATEGYLINE " + underlyer + " " + execLogic + " " + modelFile + " " + paramFile + " " + startTime + " " + endTime + " " + handle + "\n")
f.close()
'''
