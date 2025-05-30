#!/usr/bin/env python

import heapq
import sys
import numpy as np
from scipy.spatial import distance
import subprocess
import os
import shlex

if len(sys.argv) < 2:
    print "Usage:<script><input file>"
    exit(1)

dates = []
data = []
input_file = sys.argv[1]
k = 20

with open(input_file) as fp:
    for line in fp:
        line = line.strip().split()
        dates.append(line[0])
        data.append(map(float, line[1:]))

data = np.array(data)

FORECAST_SCRIPT = os.environ["HOME"] + "/basetrade/ModelScripts/forecast_arima.R"

processes = [subprocess.Popen(shlex.split(FORECAST_SCRIPT + " " + input_file + " " +
                                          str(column + 2)), stdout=subprocess.PIPE) for column in xrange(0, data.shape[1])]
forecast = []
for process in processes:
    try:
        forecast.append(float(process.communicate()[0]))
    except:
        forecast.append(1)

input_mean = [np.mean(data[:, i]) for i in xrange(data.shape[1])]
input_stdev = [np.std(data[:, i]) for i in xrange(data.shape[1])]
forecast = np.array(forecast)
for i in xrange(forecast.shape[0]):
    forecast[i] = (forecast[i] - input_mean[i]) / input_stdev[i] if input_stdev[i] != 0 else 0
    data[:, i] = (data[:, i] - input_mean[i]) / input_stdev[i] if input_stdev[i] != 0 else data[:, i]

queue = []

cov_mat = np.linalg.inv(np.cov(np.transpose(data)))

for i in xrange(min(data.shape[0], k)):
    queue.append((1 / distance.mahalanobis(forecast, data[i], cov_mat), i))
heapq.heapify(queue)

for i in xrange(k, data.shape[0]):
    current_pair = (1 / distance.mahalanobis(forecast, data[i], cov_mat), i)
    if current_pair[0] > queue[0][0]:
        top = heapq.heappop(queue)
        heapq.heappush(queue, current_pair)

print "Dates to pick on\tweights:"
for pairs in queue:
    print dates[pairs[1]], '\t', pairs[0]
