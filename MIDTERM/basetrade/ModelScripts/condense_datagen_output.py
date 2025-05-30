#!/usr/bin/env python

import numpy as np
import sys
from math import factorial


def savitzky_golay(y, window_size, order, deriv=0, rate=1):
    try:
        window_size = np.abs(np.int(window_size))
        order = np.abs(np.int(order))
    except ValueError, msg:
        raise ValueError("window_size and order have to be of type int")
        if window_size % 2 != 1 or window_size < 1:
            raise TypeError("window_size size must be a positive odd number")
        if window_size < order + 2:
            raise TypeError("window_size is too small for the polynomials order")
    order_range = range(order + 1)
    half_window = (window_size - 1) // 2
    b = np.mat([[k**i for i in order_range] for k in range(-half_window, half_window + 1)])
    m = np.linalg.pinv(b).A[deriv] * rate**deriv * factorial(deriv)
    firstvals = y[0] - np.abs(y[1:half_window + 1][::-1] - y[0])
    lastvals = y[-1] + np.abs(y[-half_window - 1:-1][::-1] - y[-1])
    y = np.concatenate((firstvals, y, lastvals))
    return np.convolve(m[::-1], y, mode='valid')


if len(sys.argv) < 3:
    print "Usage : <script> <input> <output>"
    exit(0)

inp_file = sys.argv[1]
out_file = sys.argv[2]

inp_data = np.loadtxt(inp_file)

write_data = []

current_time = inp_data[0, 0]
current_idx = 0

# for i in xrange(4,inp_data.shape[1]):
#	write_data.append(savitzky_golay(inp_data[:,i],100,3))

#write_data = np.transpose(np.array(write_data))

for i in xrange(1, inp_data.shape[0]):
    if inp_data[i, 0] >= current_time + 900000:
        write_data.append(np.average(inp_data[current_idx:i + 1, 4:], axis=0,
                                     weights=[0.95**(i - j) for j in xrange(current_idx, i + 1)]))
        current_idx = i + 1
        current_time = inp_data[i + 1, 0]

np.savetxt(out_file, write_data, fmt='%.5f')
