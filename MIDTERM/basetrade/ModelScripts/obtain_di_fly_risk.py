#!/usr/bin/env python

import numpy as np
import sys
import subprocess


if len(sys.argv) < 6:
    print "USAGE : <start date> <end date> <chunk divisor> <list of outrights(shorter expiry first)"
    exit(0)

legs = []
start_date_ = sys.argv[1]
end_date_ = sys.argv[2]
percentile_ = int(sys.argv[3])
for i in range(4, len(sys.argv)):
    legs.append(sys.argv[i])
out_series = []
sp_series = []

# obtain the price series with a leap of 9000 points (appx an hour)
while end_date_ >= start_date_:
    price_series_ = subprocess.Popen("~/basetrade_install/bin/mult_price_printer " + end_date_ +
                                     " BRT_905 BRT_1540 MktSizeWPrice " + " ".join(legs), shell=True, stdout=subprocess.PIPE)
    output_ = price_series_.communicate()[0].rstrip('\n').split('\n')
    if (len(output_) == 0):
        prev_date_process_ = subprocess.Popen(
            "~/basetrade_install/bin/calc_prev_week_day " + end_date_, shell=True, stdout=subprocess.PIPE)
        end_date_ = prev_date_process_.communicate()[0].rstrip('\n')
        continue
    series_output_all_ = [map(float, element.split()[1:]) for element in output_]
    open_indices_ = range(0, len(series_output_all_))[900::9000]  # trades open here
    closed_indices_ = range(0, len(series_output_all_))[9000::9000]  # trades close here
    all_indices_ = sorted(open_indices_ + closed_indices_)
    series_output_ = [np.average(series_output_all_[index_ - 900:index_], axis=0) for index_ in all_indices_]
    dv01_process_ = [subprocess.Popen("~/basetrade_install/bin/get_dv01_for_shortcode " +
                                      leg + " " + end_date_, shell=True, stdout=subprocess.PIPE) for leg in legs]
    dv01_ = [float(process.communicate()[0].strip('\n')) for process in dv01_process_]
    open_position_ = 0
    for element in series_output_:
        n2d_processes_ = [subprocess.Popen("~/basetrade_install/bin/get_di_numbers_to_dollars " + legs[i] + " " +
                                           end_date_ + " " + str(element[i]), shell=True, stdout=subprocess.PIPE) for i in range(len(element))]
        n2d_ = [float(n2d_process_.communicate()[0].strip('\n')) for n2d_process_ in n2d_processes_]
        if open_position_ == 0:
            last_prices_ = [element[i] * n2d_[i] for i in range(len(legs))]
            open_position_ = 1
        else:
            out_pnl_ = [abs(element[i] * n2d_[i] - last_prices_[i]) for i in range(len(legs))]
            out_series.append(out_pnl_)
            sp_pnl_ = []
            for i in range(len(legs)):  # shortest wing
                for k in range(i + 1, len(legs)):  # belly
                    for j in range(k + 1, len(legs)):  # long wing
                        sp_pnl_.append(abs(element[j] * n2d_[j] - last_prices_[j] + dv01_[j] / dv01_[i] * (element[i] * n2d_[
                                       i] - last_prices_[i]) - 2 * dv01_[j] / dv01_[k] * (element[k] * n2d_[k] - last_prices_[k])))
            sp_series.append(sp_pnl_)
            open_position_ = 0
    prev_date_process_ = subprocess.Popen(
        "~/basetrade_install/bin/calc_prev_week_day " + end_date_, shell=True, stdout=subprocess.PIPE)
    end_date_ = prev_date_process_.communicate()[0].rstrip('\n')

# analyse
out_series = np.array(out_series)
sp_series = np.array(sp_series)
last_element_ = max(10, out_series.shape[0] / percentile_)
if last_element_ == 10:
    end_element_ = 1
else:
    end_element_ = 10
out_series_for_analysis = np.array([np.sort(out_series[:, i])[-last_element_:-end_element_] for i in range(len(legs))])
sp_series_for_analysis = np.array([np.sort(sp_series[:, i])[-last_element_:-end_element_]
                                   for i in range(sp_series.shape[1])])
for i in range(len(legs)):
    print legs[i], np.mean(out_series_for_analysis[i])

idx_ = 0
for i in range(len(legs)):
    for k in range(i + 1, len(legs)):
        for j in range(k + 1, len(legs)):
            print legs[j] + "-" + legs[k] + "-" + legs[i], np.mean(sp_series_for_analysis[idx_])
            idx_ = idx_ + 1
