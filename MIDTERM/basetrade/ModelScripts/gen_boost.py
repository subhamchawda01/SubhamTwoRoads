#!/media/shared/ephemeral14/anaconda2/bin/python

import os
import sys
import sklearn
import random
import subprocess
import math
import numpy as np
from sklearn.metrics import precision_score, confusion_matrix
from sklearn.metrics import recall_score
from sklearn.ensemble import GradientBoostingClassifier
from sklearn.linear_model import LogisticRegression

# Generate regdata


def gen_modified_regdata(shortcode, ilist, date, num_days, start_time, end_time,
                         datagen_args, lag, obv, work_dir):
    os.system("rm -rf " + work_dir)
    os.system("mkdir -p " + work_dir)
    dates_cmd = ["/home/dvctrader/basetrade/scripts/get_list_of_dates_for_shortcode.pl", shortcode, date, num_days]
    dates = subprocess.check_output(dates_cmd)
    out_file = work_dir + "t_dgen_outfile"
    catted_bid_file = work_dir + "catted_regdata_bid_filename"
    catted_ask_file = work_dir + "catted_regdata_ask_filename"
    prog_id = "12398"
    regdata = []
    dates = dates.split()
    count_0 = 0
    count_1 = 0
    count_2 = 0
    for date in dates:
        min_price_cmd = ["/home/dvctrader/basetrade_install/bin/get_contract_specs", shortcode, date, "ALL"]
        mpi = subprocess.check_output(min_price_cmd)
        mpi = float(mpi.split()[7])
        date = str(date)
        datagen_cmd = ["/home/dvctrader/LiveExec/bin/datagen", ilist,
                       date, start_time, end_time, prog_id, out_file] + datagen_args
        # print " ".join(datagen_cmd)
        out = subprocess.check_output(datagen_cmd)
        data = np.loadtxt(out_file)
        if data.shape[0] == 0:
            continue
        labels1 = []
        labels2 = []
        f1 = open(catted_bid_file + "_" + str(date), 'ab')
        f2 = open(catted_ask_file + "_" + str(date), 'ab')
        for r in range(data.shape[0]):
            row = data[r]
            current_time = row[0]
            next_obv = r + 1
            if next_obv >= data.shape[0]:
                break
            '''if data[next_obv][0] > current_time + lag+obv:
                label1 = 0
                label2 = 0
                labels1.append(label1)
                labels2.append(label2)
                continue'''
            while data[next_obv][0] - current_time <= lag:
                next_obv += 1
                if next_obv >= data.shape[0]:
                    break
            label1 = 0
            label2 = 0
            '''start1 = data[next_obv-1][2]
            start2 = data[next_obv-1][3]'''
            start1 = data[next_obv - 1][4]
            start2 = data[next_obv - 1][5]
            if next_obv >= data.shape[0]:
                break
            '''if data[next_obv][0] > current_time+lag+obv:
                label1 = 0
                label2 = 0
                labels1.append(label1)
                labels2.append(label2)
                continue'''
            while data[next_obv][0] - current_time <= lag + obv:
                next_obv += 1
                if next_obv >= data.shape[0]:
                    break
            if next_obv >= data.shape[0]:
                break
            '''last1 = data[next_obv-1][2]
            last2 = data[next_obv-1][3]
            if (last1-start1 >=mpi):
                label1 = 1
                #count_1 +=1
            elif (last1-start1 <= -mpi):
                label1 = -1
                #count_2 +=1
            #else:
                #count_0 +=1
            labels1.append(label1)
            
            if (last2-start2 >=mpi):
                label2 = 1
                count_1+=1
            elif (last2-start2 <= -mpi):
                label2 = -1
                count_2 +=2
            else:
                count_0 +=1
            labels2.append(label2)'''
            last1 = data[next_obv - 1][4]
            last2 = data[next_obv - 1][5]
            if(last1 < start1):
                label1 = 1
            else:
                label1 = 0
            if last2 > start2:
                label2 = 1
            else:
                label2 = 0
            labels1.append(label1)
            labels2.append(label2)
        data = data[0:len(labels1), :]
        labels1 = np.array(labels1)
        labels2 = np.array(labels2)
        regdata = np.column_stack((labels1, labels2, data))

        c_0 = regdata[regdata[:, 0] == 0, :]
        c_1 = regdata[regdata[:, 0] == 1, :]
        c_2 = regdata[regdata[:, 1] == 0, :]
        c_3 = regdata[regdata[:, 1] == 1, :]
        if c_0.shape[0] == 0 or c_1.shape[0] == 0:
            continue
        size = max(10, min(5000, 20 * c_1.shape[0]))
        sample0 = np.random.choice(c_0.shape[0], size, replace=False)
        c_0 = c_0[sample0, :]
        sample1 = np.random.choice(c_1.shape[0], size, replace=True)
        c_1 = c_1[sample1, :]

        size = max(10, min(5000, 20 * c_3.shape[0]))
        sample2 = np.random.choice(c_2.shape[0], size, replace=False)
        c_2 = c_2[sample2, :]
        sample3 = np.random.choice(c_3.shape[0], size, replace=True)
        c_3 = c_3[sample3, :]
        regdata1 = np.vstack((c_0, c_1))
        regdata2 = np.vstack((c_2, c_3))

        np.savetxt(f1, regdata1, fmt='%8.7f')
        np.savetxt(f2, regdata2, fmt='%8.7f')
        f1.close()
        f2.close()
    # regdata1=np.loadtxt(catted_bid_file)
    # regdata2=np.loadtxt(catted_ask_file)
    # return (regdata1,regdata2)


def getWindowData(date, num_days, work_dir):
    dates_cmd = ["/home/dvctrader/basetrade/scripts/get_list_of_dates_for_shortcode.pl", shortcode, date, num_days]
    dates = subprocess.check_output(dates_cmd)
    dates = dates.split()
    bid_regdata = None
    ask_regdata = None
    for date in dates:
        date = str(date)
        bid_regdata_file = work_dir + "catted_regdata_bid_filename_" + date
        ask_regdata_file = work_dir + "catted_regdata_ask_filename_" + date
        bid_data = None
        ask_data = None
        if os.path.exists(bid_regdata_file):
            bid_data = np.loadtxt(bid_regdata_file)
        if os.path.exists(ask_regdata_file):
            ask_data = np.loadtxt(ask_regdata_file)
        if bid_regdata is None:
            if bid_data is None:
                continue
            else:
                bid_regdata = bid_data
        else:
            if bid_data is None:
                continue
            else:
                print "Bid concat"
                print bid_regdata.shape, bid_data.shape
                bid_regdata = np.vstack((bid_regdata, bid_data))
        if ask_regdata is None:
            if ask_data is None:
                continue
            else:
                ask_regdata = ask_data
        else:
            if ask_data is None:
                continue
            else:
                print "Ask concat"
                print ask_regdata.shape, ask_data.shape
                ask_regdata = np.vstack((ask_regdata, ask_data))
    print bid_regdata.shape, ask_regdata.shape
    return (bid_regdata, ask_regdata)


def write_model_to_file(clf, writer):
    for i in range(clf.estimators_.shape[0]):
        for j in range(clf.estimators_.shape[1]):
            writer.write("TREE " + str(i) + " " + str(j))
            writer.write("\n")
            left = map(str, clf.estimators_[i][j].tree_.children_left)
            right = map(str, clf.estimators_[i][j].tree_.children_right)
            feature = map(str, clf.estimators_[i][j].tree_.feature)
            value = [str("%.7f" % x) for x in clf.estimators_[i][j].tree_.value.ravel()]
            threshold = map(str, clf.estimators_[i][j].tree_.threshold)
            writer.write("LEFTCHILD " + " ".join(left))
            writer.write("\n")
            writer.write("RIGHTCHILD " + " ".join(right))
            writer.write("\n")
            writer.write("FEATURE " + " ".join(feature))
            writer.write("\n")
            writer.write("VALUE " + " ".join(value))
            writer.write("\n")
            writer.write("THRESHOLD " + " ".join(threshold))
            writer.write("\n")


def train(shortcode, ilist, date, start_time, end_time, obv, training_dir, model_file, lookback, stages):
    (bid_regdata, ask_regdata) = getWindowData(date, lookback, training_dir)
    if bid_regdata.shape[0] == 0 or ask_regdata.shape[0] == 0:
        return
    f = open(ilist, 'r')
    s = f.readlines()
    os.system("rm -f " + model_file + "_" + shortcode + "_" + str(date))

    writer = open(model_file + "_" + shortcode + "_" + str(date), 'w')
    writer.write(s[0])
    writer.write("MODELMATH GRADBOOSTINGCLASSIFIER CHANGE\n")
    writer.write('MODELARGS ' + str(stages) + ' ' + str(2) + "\n")
    writer.write("INDICATORSTART 0.1")
    writer.write("\n")
    writer.write("".join(s[5:]).strip())
    writer.write("\n")
    print "TRAINING " + str(date)
    for j in range(2):
        '''c_0 = regdata[regdata[:,j]==0,:]
        c_1 = regdata[regdata[:,j]==1,:]
        #c_2 = regdata[regdata[:,j]==-1,:]
        if c_0.shape[0]==0 or c_1.shape[0]==0: #or c_2.shape[0]==0:
            continue
        size = max(10,min(50000,10*c_1.shape[0]))
        sample0 = np.random.choice(c_0.shape[0], size, replace=False)
        c_0 = c_0[sample0,:]
        sample1 = np.random.choice(c_1.shape[0], size, replace=True)
        c_1 = c_1[sample1,:]
        data = np.vstack((c_0,c_1))'''
        if j == 0:
            data = bid_regdata
        else:
            data = ask_regdata
        X = data[:, 8:]
        Y = data[:, j]
        X = X.tolist()
        Y = Y.tolist()
        Y = [int(x) for x in Y]
        clf = GradientBoostingClassifier(n_estimators=stages)
        #clf = LogisticRegression()
        clf.fit(X, Y)
        if (j == 0):
            writer.write("SIDE BID\n")
        else:
            writer.write("SIDE ASK\n")
        write_model_to_file(clf, writer)


if len(sys.argv) < 11:
    print "script shortcode ilist date num_days start_time end_time obv_time stages lookback_days step_days [model_directory]"
    sys.exit(0)

shortcode = sys.argv[1]
ilist = sys.argv[2]
date = sys.argv[3]
num_days = sys.argv[4]
start_time = sys.argv[5]
end_time = sys.argv[6]
model_dir = "/spare/local/tradeinfo/CancellationModel/" + shortcode + "/"
obv = (int)(sys.argv[7])
stages = (int)(sys.argv[8])
lookback = (sys.argv[9])
step_days = (int)(sys.argv[10])
work_dir = "/spare/local/dvctrader/classfication_" + str(random.getrandbits(32)) + "/"
if len(sys.argv) > 11:
    model_dir = sys.argv[11]
model_file_prefix = model_dir + "/model"

print "Working directory: " + work_dir
gen_modified_regdata(shortcode, ilist, date, num_days, start_time, end_time, "0 1 0 0".split(), 2, obv, work_dir)

dates_cmd = ["/home/dvctrader/basetrade/scripts/get_list_of_dates_for_shortcode.pl", shortcode, date, num_days]
dates = subprocess.check_output(dates_cmd)
dates = dates.split()
dates.reverse()

os.system("mkdir -p " + model_dir)

for date_index in range(0, len(dates) - 1, step_days):
    date = dates[date_index]
    print date

    train(shortcode, ilist, date, start_time, end_time, obv, work_dir, model_file_prefix, lookback, stages)

os.system("rm -rf " + work_dir)
