import math
import time
from datetime import date, datetime, timedelta
from enum import Enum

import numpy as np
import pandas as pd
from dateutil.rrule import DAILY, FR, MO, TH, TU, WE, rrule

from StratFramework import Fields, StratFramework


class MACDFields(Enum):
    time = 0
    open_ = 1
    close_ = 2
    low_ = 3
    high_ = 4
    vol_ = 5
    sema = 6
    lema = 7
    macd = 8
    signal = 9
    sema_filter = 10
    lema_filter = 11
    macd_filter = 12
    signal_filter = 13


class MACDTheo(StratFramework):
        # Initializations
    def __init__(self, _om, _live_file, _granularity, _train_start_day, _train_end_day, _config_file):
        StratFramework.__init__(self, _om, _live_file, _granularity)
        # lot_file_ = "/home/nishitbhandari/Downloads/mktlots"
        # self.prod_pos_ = pd.DataFrame(columns=["prod", "pos"])
        # self.prod_pos_["pos"] = self.prod_pos_["pos"].astype(int)
        # with open(lot_file_, "r") as input_file:
        #	 for line in input_file:
        #		row = line.split(',')
        #		self.prod_pos_.loc[row[1].strip()] = [row[1].strip(),row[2].strip()]

        self.train_start_ = _train_start_day
        self.train_end_ = _train_end_day
        self.training_data_ = []
        self.bar_training_data_ = []
        self.bollinger_data_ = []
        self.current_val_vec_ = []
        self.oldest_bar_ = []
        # self.num_bars_ = []
        self.current_bar_ = np.zeros(
            len(self.product_list_) * 4).reshape(len(self.product_list_), 4)
        self.prev_day_close_ = [0] * len(self.product_list_)
        self.prev_bar_close_ = [0] * len(self.product_list_)
        self.bar_agg_ = np.zeros(
            len(self.product_list_) * 5).reshape(len(self.product_list_), 5)
        self.day_bar_agg_ = np.zeros(
            len(self.product_list_) * 5).reshape(len(self.product_list_), 5)
        self.moment_cc_wt_mean_ = [0] * len(self.product_list_)
        self.moment_cc_wt_median_ = [0] * len(self.product_list_)
        self.moment_cc_wt_stdev_ = [0] * len(self.product_list_)
        self.momentum_cc_wt_mean_ = [0] * len(self.product_list_)
        self.momentum_cc_wt_stdev_ = [0] * len(self.product_list_)
        self.moment_cc_wt_hl_ = [0] * len(self.product_list_)
        self.prod_directions_ = [0] * len(self.product_list_)
        # self.sema_ = [0]*len(self.product_list_)
        # self.lema_ = [0]*len(self.product_list_)
        # self.signal_ = [0]*len(self.product_list_)
        # self.macd_= [0]*len(self.product_list_)
        self.last_bar_update_ = 0
        # self.day_rsi_ = [0]*len(self.product_list_)
        self.crossover_ = [0] * len(self.product_list_)
        self.bar_sema_ = [0] * len(self.product_list_)
        self.bar_lema_ = [0] * len(self.product_list_)
        self.bar_signal_ = [0] * len(self.product_list_)
        self.bar_macd_ = [0] * len(self.product_list_)
        self.macd_initialize_bar_ = [0] * len(self.product_list_)
        self.macd_initialize_day_ = [0] * len(self.product_list_)
        self.macd_data_ = []
        self.bollinger_mean_ = [0] * len(self.product_list_)
        self.bollinger_uband_ = [0] * len(self.product_list_)
        self.bollinger_lband_ = [0] * len(self.product_list_)
        self.oldest_bollinger_ = [0] * len(self.product_list_)
        self.bollinger_move_ = [0] * len(self.product_list_)

        self.day_bollinger_mean_ = [0] * len(self.product_list_)
        self.day_bollinger_uband_ = [0] * len(self.product_list_)
        self.day_bollinger_lband_ = [0] * len(self.product_list_)
        self.day_bollinger_move_ = [0] * len(self.product_list_)

        self.prev_day_bollinger_crossover_ = [0] * len(self.product_list_)
        self.macd_crossover_ = [0] * len(self.product_list_)
        self.current_cascade_ = [0] * len(self.product_list_)
        self.current_slope_ = [0] * len(self.product_list_)
        self.last_meanstd_ = [0] * len(self.product_list_)

        self.lt_std_ = [0] * len(self.product_list_)
        self.open_vol_avg_ = [np.NaN] * len(self.product_list_)
        self.filter_std_list_ = []
        self.current_volume_ = [0] * len(self.product_list_)
        # self.bar_relative_strength_index_ = [0]*len(self.product_list_)
        # self.prev_bar_rsi_ = [0]*len(self.product_list_)
        # self.bar_avg_gain_ = [0]*len(self.product_list_)
        # self.bar_avg_loss_ = [0]*len(self.product_list_)
        # self.day_rsi_arr_ = []
        # self.old_rsi_ = [0]*len(self.product_list_)

        self.macd_px_ = []
        self.cascade_size_ = []
        self.bar_macd_px_ = []
        self.meanstd_lkbk_ = []
        for prod_ in self.product_list_:
            self.training_data_.append(pd.DataFrame(
                columns=["datetime", "open", "close", "low", "high", "vol", "c-c"]))
            self.training_data_[len(self.training_data_)-1][["open", "close", "low", "high", "vol", "c-c"]] = \
                self.training_data_[len(
                    self.training_data_)-1][["open", "close", "low", "high", "vol", "c-c"]].astype(float)

            self.macd_data_.append(pd.DataFrame(
                columns=["datetime", "open", "close", "low", "high", "vol"]))
            self.macd_data_[len(self.macd_data_) - 1][["open", "close", "low", "high",
                                                       "vol"]] = self.macd_data_[len(self.macd_data_) - 1][[
                                                           "open", "close", "low", "high", "vol"
                                                       ]].astype(float)

            self.bar_training_data_.append(
                pd.DataFrame(columns=[
                    "datetime", "open", "close", "low", "high", "vol", "sema", "lema", "macd", "signal", "sema_filter",
                    "lema_filter", "macd_filter", "signal_filter"
                ]))
            self.bar_training_data_[len(self.bar_training_data_)-1][["open", "close", "low", "high", "vol", "sema", "lema", "macd", "signal", "sema_filter", "lema_filter", "macd_filter", "signal_filter"]] = \
                self.bar_training_data_[len(self.bar_training_data_)-1][["open", "close", "low", "high", "vol", "sema",
                                                                         "lema", "macd", "signal", "sema_filter", "lema_filter", "macd_filter", "signal_filter"]].astype(float)

            self.bollinger_data_.append(
                pd.DataFrame(columns=["datetime", "close"]))
            self.bollinger_data_[len(self.bollinger_data_) - 1][[
                "close"
            ]] = self.bollinger_data_[len(self.bollinger_data_) - 1][["close"]].astype(float)

            self.meanstd_lkbk_.append(pd.DataFrame(
                columns=["datetime", "close", "c-c"]))
            self.meanstd_lkbk_[len(self.meanstd_lkbk_) - 1][["close",
                                                             "c-c"]] = self.meanstd_lkbk_[len(self.meanstd_lkbk_) - 1][[
                                                                 "close", "c-c"
                                                             ]].astype(float)
            self.macd_px_.append([[0, 0, 0, 0, 0, 0, 0, 0, 0, 0], [
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]])
            self.bar_macd_px_.append([])
            # self.day_rsi_arr_.append(pd.DataFrame(columns=["close"]))
            self.bar_macd_px_[len(self.bar_macd_px_) -
                              1] = np.empty((0, 6), float)
            self.current_val_vec_.append([[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                                          [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]])

        # Parameter Input
        with open(_config_file, "r") as main_file:
            for line in main_file:
                row = str.split(line)
                print(row)
                if row[0] == "TRAILING_STOP_LOSS_PERCENT":
                    self.trailing_pt_ = float(row[2])
                if row[0] == "SL_LOOKBACK":
                    self.sl_lkbk_ = int(row[2])
                if row[0] == "TIME_WT":
                    self.time_wt_ = float(row[2])
                if row[0] == "POS_WT":
                    self.pos_wt_ = float(row[2])
                if row[0] == "DECAY_FACTOR":
                    self.decay_factor_ = float(row[2])
                if row[0] == "SEMA":
                    self.sema_param_ = float(row[2])
                if row[0] == "LEMA":
                    self.lema_param_ = float(row[2])
                if row[0] == "SIGNAL":
                    self.signal_param_ = float(row[2])
                if row[0] == "CASCADE_NUM":
                    self.cascade_num_ = float(row[2])
                    self.cascade_size_ = [1] * int(self.cascade_num_)
                if row[0] == "FIRST_CASCADE":
                    self.cascade_size_[0] = float(row[2])
                # if row[0] == "SECOND_CASCADE":
                # 	self.cascade_size_[1] = float(row[2])
                # if row[0] == "THIRD_CASCADE":
                # 	self.cascade_size_[2] = float(row[2])
                # if row[0] == "FOURTH_CASCADE":
                # 	self.cascade_size_[3] = float(row[2])
                # if row[0] == "FIFTH_CASCADE":
                # 	self.cascade_size_[4] = float(row[2])
                # if row[0] == "SIXTH_CASCADE":
                #	self.cascade_size_[5] = float(row[2])
                if row[0] == "SEMA_FILTER":
                    self.sema_filter_param_ = float(row[2])
                    self.day_sema_param_ = self.sema_filter_param_
                if row[0] == "LEMA_FILTER":
                    self.lema_filter_param_ = float(row[2])
                    self.day_lema_param_ = self.lema_filter_param_
                if row[0] == "SIGNAL_FILTER":
                    self.signal_filter_param_ = float(row[2])
                    self.day_signal_param_ = self.signal_filter_param_
                if row[0] == "TRAIN_START_TIME":
                    self.train_start_time_ = row[2]
                if row[0] == "TRAIN_END_TIME":
                    self.train_end_time_ = row[2]
                if row[0] == "STRAT_START_TIME":
                    self.strat_start_time_ = row[2]
                if row[0] == "STRAT_END_TIME":
                    self.strat_end_time_ = row[2]
                # if row[0] == "GRANULARITY":
                #     self.granularity_ = 60 * int(row[2])
                if row[0] == "MAX_EXPOSURE":
                    self.max_exposure_ = float(row[2])
                if row[0] == "NUM_BARS":
                    self.total_bar_ = float(row[2])
                if row[0] == "ALPHA":
                    self.moment_oc_alpha_ = float(row[2])
                    self.moment_cc_alpha_ = self.moment_oc_alpha_
                if row[0] == "BETA":
                    self.moment_oc_beta_ = float(row[2])
                    self.moment_cc_beta_ = self.moment_oc_beta_
                if row[0] == "BAR_LKBK":
                    self.bar_lkbk_ = int(row[2])
                if row[0] == "RSI_PERIOD":
                    self.rsi_period_ = float(row[2])
                if row[0] == "UPPER_RSI":
                    self.upper_rsi_ = float(row[2])
                if row[0] == "LOWER_RSI":
                    self.lower_rsi_ = float(row[2])
                if row[0] == "OPEN_VOL_PTILE":
                    self.open_vol_ptile_ = float(row[2])
                if row[0] == "OPEN_VOL_LOOKBK":
                    self.open_vol_days_ = int(row[2])
                if row[0] == "VOLATILITY_LOOKBACK":
                    self.lt_days_ = int(row[2])
                if row[0] == "FILTER_PTILE":
                    self.filter_ptile_ = float(row[2])
                # if row[0] == "BOLLINGER_PERIOD":
                #	self.bollinger_period_ = int(row[2])
                if row[0] == "BOLLINGER_BAND":
                    self.bollinger_std_band_ = float(row[2])
                if row[0] == "CROSSOVER_HIST":
                    self.macd_crossover_hist_ = float(row[2])
                if row[0] == "GAMMA":
                    self.moment_cc_gamma_ = float(row[2])

        self.long_term_vol_ = np.zeros(self.lt_days_ * len(self.product_list_)).reshape(
            len(self.product_list_), self.lt_days_)
        self.opening_bar_volume_ = np.zeros(self.open_vol_days_ * len(self.product_list_)).reshape(
            len(self.product_list_), self.open_vol_days_)
        self.long_term_vol_[:] = np.NaN
        self.opening_bar_volume_[:] = np.NaN
        self.lt_count_ = [0] * len(self.product_list_)
        self.open_vol_count_ = [0] * len(self.product_list_)
        #print(self.moment_cc_alpha_, self.moment_cc_beta_)

    # Get Bar from BarGenerator
    def loadBar(self, _method):
        self.dateRange()
        self.getTrainingBar(_method)

    # dates for which to run simulations
    def dateRange(self):
        start_ = self.train_start_.split('_')
        end_ = self.train_end_.split('_')
        start_ = date(int(start_[0]), int(start_[1]), int(start_[2]))
        end_ = date(int(end_[0]), int(end_[1]), int(end_[2])) - timedelta(1)
        self.working_days_ = list(
            rrule(DAILY, dtstart=start_, until=end_, byweekday=(MO, TU, WE, TH, FR)))
        self.bollinger_period_ = len(self.working_days_)

    # Compute Weighted mean and deviation of historic c-c
    def getMeanMomentCC(self):
        for counter_ in range(0, len(self.product_list_)):
            if len(self.meanstd_lkbk_[counter_]) == 0:
                continue
            a = pd.DataFrame(columns=['a'])
            a['a'] = a['a'].astype(float)
            for i in range(0, 5):
                a.loc[i] = self.meanstd_lkbk_[counter_].loc[(self.last_meanstd_[counter_] - i - 1) % len(
                    self.meanstd_lkbk_[counter_])]["c-c"]
                # print(self.meanstd_lkbk_[counter_].loc[(self.last_meanstd_[counter_]-i-1)%len(self.meanstd_lkbk_[counter_])])
            col = self.meanstd_lkbk_[counter_]["c-c"]
            hl_ = self.training_data_[counter_]["high"] - \
                self.training_data_[counter_]["low"]
            # print(a)
            self.momentum_cc_wt_mean_[counter_] = 0.4 * a['a'].mean()
            self.momentum_cc_wt_stdev_[counter_] = 1.8 * a['a'].std()
            self.moment_cc_wt_mean_[
                counter_] = self.moment_cc_beta_ * col.mean()
            self.moment_cc_wt_stdev_[
                counter_] = self.moment_cc_alpha_ * col.std()

    # Called before the first trading day in date range.Initializes dataframe with values
    def getTrainingBar(self, _method):
        counter_ = 0
        for index_ in range(0, len(self.product_list_)):
            prod_ = self.product_list_[index_]
            flag_ = 0
            start_ = self.train_start_time_.split('_')
            end_ = self.train_end_time_.split('_')
            for date_ in self.working_days_:
                day_start_epoch_ = int(date_.replace(
                    hour=int(start_[0]), minute=int(start_[1])).strftime('%s'))
                day_end_epoch_ = int(date_.replace(
                    hour=int(end_[0]), minute=int(end_[1])).strftime('%s'))
                [o, c, l, h, v] = _method(
                    day_start_epoch_, day_end_epoch_, counter_)
                if [o, c, l, h] != [0, 0, 0, 0]:
                    flag_ = 1
                    # if len(self.day_rsi_arr_[counter_]) <= self.rsi_period_:
                    # 	self.day_rsi_arr_[counter_].loc[len(self.day_rsi_arr_[counter_])] = c
                    # else:
                    # 	self.day_rsi_arr_[counter_].set_value(self.old_rsi_[counter_],"close", c)
                    # 	self.old_rsi_[counter_] = self.old_rsi_[counter_] + 1

                    # 	rsi_diff_ = self.day_rsi_arr_[counter_]["close"].diff()
                    # 	avg_gain_ = rsi_diff_[rsi_diff_> 0].mean()
                    # 	avg_loss_ = -1*rsi_diff_[rsi_diff_ < 0].mean()
                    # self.day_rsi_[counter_] = 100 - 100/(1 + (avg_gain_/avg_loss_))
                    if len(self.meanstd_lkbk_[counter_]) < self.sl_lkbk_:
                        if self.prev_day_close_[index_]:
                            self.meanstd_lkbk_[counter_].loc[len(self.meanstd_lkbk_[counter_])] = [
                                day_start_epoch_, c, abs(
                                    c - self.prev_day_close_[index_])
                            ]
                        else:
                            self.meanstd_lkbk_[counter_].loc[len(
                                self.meanstd_lkbk_[counter_])] = [day_start_epoch_, c, np.NaN]
                    else:
                        self.meanstd_lkbk_[counter_].set_value(self.last_meanstd_[counter_], "datetime",
                                                               day_start_epoch_)
                        self.meanstd_lkbk_[counter_].set_value(
                            self.last_meanstd_[counter_], "close", c)
                        if self.prev_day_close_[index_]:
                            self.meanstd_lkbk_[counter_].set_value(self.last_meanstd_[counter_], "c-c",
                                                                   abs(c - self.prev_day_close_[index_]))
                        else:
                            self.meanstd_lkbk_[counter_].set_value(
                                self.last_meanstd_[counter_], "c-c", np.NaN)
                        self.last_meanstd_[counter_] = (self.last_meanstd_[counter_] + 1) % len(
                            self.meanstd_lkbk_[counter_])

                    self.macd_data_[counter_].loc[len(self.macd_data_[counter_])] = [
                        day_start_epoch_, o, c, l, h, v]
                    if self.prev_day_close_[index_]:
                        self.training_data_[counter_].loc[len(self.training_data_[counter_])] = [
                            day_start_epoch_, o, c, l, h, v,
                            abs(c - self.prev_day_close_[index_])
                        ]
                        self.long_term_vol_[counter_][self.lt_count_[counter_]] = abs(
                            c - self.prev_day_close_[index_])
                        self.lt_count_[counter_] = self.lt_count_[counter_] + 1
                    else:
                        self.training_data_[counter_].loc[len(
                            self.training_data_[counter_])] = [day_start_epoch_, o, c, l, h, v, np.NaN]
                    self.getDayBollinger(index_, 0, 0)
                    self.prev_day_close_[index_] = c
            if not flag_:
                self.prod_curr_posn_[index_][Fields.start_trading.value] = True
            counter_ = counter_ + 1
            self.oldest_bar_.append(0)
        self.getMeanMomentCC()
        # self.getMACD()

    # Called eod to update dataframe with bar/day data for that day
        # Involves computing day macd day bollinger and filters to be employed for the next day
    def updateTrainingBar(self, _prod_bar_info):
        counter_ = 0
        for index_ in range(0, len(self.product_list_)):
            prod_ = self.product_list_[index_]
            flag_ = 0
            [o, c, l, h, v] = _prod_bar_info[counter_]
            if [o, c, l, h] != [0, 0, 0, 0]:
                flag_ = 1

                # if len(self.macd_px_[counter_]):
                # 	self.macd_px_[counter_].set_value(len(self.macd_px_[counter_])-1,"c-c",c-self.prev_day_close_[index_])
                if self.macd_initialize_day_[counter_] == 0:
                    self.macd_data_[counter_].loc[len(
                        self.macd_data_[counter_])] = [self.om_.started_at_, o, c, l, h, v]
                else:
                    self.macd_px_[counter_][0] = self.macd_px_[counter_][1]
                    self.macd_px_[counter_][1] = [
                        self.om_.started_at_, o, c, l, h, v, 0, 0, 0, 0]

                if len(self.meanstd_lkbk_[counter_]) < self.sl_lkbk_:
                    if self.prev_day_close_[index_]:
                        self.meanstd_lkbk_[counter_].loc[len(self.meanstd_lkbk_[counter_])] = [
                            self.om_.started_at_, c, abs(
                                c - self.prev_day_close_[index_])
                        ]
                    else:
                        self.meanstd_lkbk_[counter_].loc[len(
                            self.meanstd_lkbk_[counter_])] = [self.om_.started_at_, c, np.NaN]
                else:
                    self.meanstd_lkbk_[counter_].set_value(self.last_meanstd_[counter_], "datetime",
                                                           self.om_.started_at_)
                    self.meanstd_lkbk_[counter_].set_value(
                        self.last_meanstd_[counter_], "close", c)
                    if self.prev_day_close_[index_]:
                        self.meanstd_lkbk_[counter_].set_value(self.last_meanstd_[counter_], "c-c",
                                                               abs(c - self.prev_day_close_[index_]))
                    else:
                        self.meanstd_lkbk_[counter_].set_value(
                            self.last_meanstd_[counter_], "c-c", np.NaN)
                    self.last_meanstd_[counter_] = (self.last_meanstd_[counter_] + 1) % len(
                        self.meanstd_lkbk_[counter_])

                if len(self.working_days_) > len(self.training_data_[counter_]):
                    self.training_data_[counter_].loc[len(self.training_data_[counter_])] = [
                        self.om_.started_at_, o, c, l, h, v,
                        abs(c - self.prev_day_close_[index_])
                    ]
                    self.getDayBollinger(index_, 0, 0)
                else:
                    if c > self.day_bollinger_mean_[counter_] and self.prev_day_close_[
                            counter_] < self.day_bollinger_mean_[counter_]:
                        self.prev_day_bollinger_crossover_[counter_] = 1
                        # print("crossover+ve",counter_,self.om_.started_at_)
                    elif c < self.day_bollinger_mean_[counter_] and self.prev_day_close_[
                            counter_] > self.day_bollinger_mean_[counter_]:
                        self.prev_day_bollinger_crossover_[counter_] = -1
                        # print("crossover-ve",counter_,self.om_.started_at_)
                    else:
                        self.prev_day_bollinger_crossover_[counter_] = 0
                    old_val_ = self.training_data_[counter_].get_value(
                        self.oldest_bar_[counter_], "close")
                    self.training_data_[counter_].set_value(self.oldest_bar_[counter_], "datetime",
                                                            self.om_.started_at_)
                    self.training_data_[counter_].set_value(
                        self.oldest_bar_[counter_], "open", o)
                    self.training_data_[counter_].set_value(
                        self.oldest_bar_[counter_], "close", c)
                    self.training_data_[counter_].set_value(
                        self.oldest_bar_[counter_], "low", l)
                    self.training_data_[counter_].set_value(
                        self.oldest_bar_[counter_], "high", h)
                    self.training_data_[counter_].set_value(
                        self.oldest_bar_[counter_], "vol", v)
                    self.training_data_[counter_].set_value(self.oldest_bar_[counter_], "c-c",
                                                            abs(c - self.prev_day_close_[index_]))
                    self.getDayBollinger(index_, c, old_val_)
                    # self.training_data_[counter_].loc[len(self.training_data_[counter_])+self.oldest_bar_[counter_]] = [self.om_.started_at_,o,c,l,h,v]
                    # self.training_data_[counter_].drop([self.oldest_bar_[counter_]],inplace=True)

                    self.oldest_bar_[counter_] = (
                        self.oldest_bar_[counter_] + 1) % len(self.training_data_[counter_])
                self.long_term_vol_[index_][self.lt_count_[index_] % self.lt_days_] = abs(c -
                                                                                          self.prev_day_close_[index_])
                self.prev_day_close_[index_] = c
            else:
                self.long_term_vol_[index_][self.lt_count_[
                    index_] % self.lt_days_] = np.NaN

            self.lt_count_[index_] = self.lt_count_[index_] + 1
            # self.open_vol_count_[index_] = self.open_vol_count_[index_] + 1

            if not flag_ and self.prod_curr_posn_[index_][Fields.start_trading.value]:
                self.prod_curr_posn_[index_][Fields.start_trading.value] = True
            self.lt_std_[index_] = np.nanstd(
                self.long_term_vol_[index_]) / self.prev_day_close_[index_]
            self.open_vol_avg_[index_] = np.nanpercentile(
                self.opening_bar_volume_[index_], self.open_vol_ptile_)

            counter_ = counter_ + 1
        ptl_std_ = np.percentile(self.lt_std_, self.filter_ptile_)
        self.filter_std_list_.append(ptl_std_)

        for index_ in range(0, len(self.product_list_)):
            if self.lt_std_[index_] < ptl_std_ and self.lt_count_[index_] >= self.lt_days_:
                self.prod_curr_posn_[index_][Fields.start_trading.value] = False
        self.getMeanMomentCC()
        print("Open vol", self.open_vol_avg_)
        print("longterm", self.lt_std_)
        print("filter", ptl_std_, self.lt_days_)
        # self.getMACD(self.om_.started_at_)

    # Computes day level MACD using Small EMA, long EMA, MACD and signal
    def getMACD(self, _time):
        for counter_ in range(0, len(self.product_list_)):
            # self.training_data_[counter_].set_index("datetime")
            if len(self.macd_data_[counter_]) == self.total_bar_:
                # print(col,"hi")
                if self.macd_initialize_day_[counter_] == 0:

                    col = self.macd_data_[counter_]["close"]
                    # self.macd_data_[counter_]["sema"] = pd.ewma(col, span=self.day_sema_param_)
                    # self.macd_data_[counter_]["lema"] = pd.ewma(col, span=self.day_lema_param_)
                    self.macd_data_[counter_]["sema"] = col.ewm(span=self.day_sema_param_).mean()
                    self.macd_data_[counter_]["lema"] = col.ewm(span=self.day_lema_param_).mean()
                    
                    # # self.training_data_[counter_]["sema"] = pd.rolling_mean(col,window=self.sema_param_)
                    # self.training_data_[counter_]["lema"] = pd.rolling_mean(col,window=self.lema_param_)
                    self.macd_data_[counter_]["macd"] = self.macd_data_[counter_]["sema"] - self.macd_data_[counter_]["lema"]
                    # self.macd_data_[counter_]["signal"] = pd.ewma(self.macd_data_[counter_]["macd"], span=self.day_signal_param_)
                    self.macd_data_[counter_]["signal"] = self.macd_data_[counter_]["macd"].ewm(span=self.day_signal_param_).mean()
                    self.macd_initialize_day_[counter_] = 1
                    # self.training_data_[counter_]["signal"] = pd.rolling_mean(self.training_data_[counter_]["macd"],window=self.signal_param_)
                    # print(self.oldest_bar_[counter_],self.sema_param_,self.lema_param_,self.signal_param_)
                    if _time == self.macd_data_[counter_].get_value(len(self.macd_data_[counter_]) - 1, "datetime"):
                        self.macd_px_[counter_][0] = ( self.macd_data_[counter_].loc[len(self.macd_data_[counter_]) - 1])

                else:
                    if _time == self.macd_px_[counter_][1][MACDFields.time.value]:
                        prev_sema_ = self.macd_px_[
                            counter_][0][MACDFields.sema.value]
                        prev_lema_ = self.macd_px_[
                            counter_][0][MACDFields.lema.value]
                        prev_macd_ = prev_sema_ - prev_lema_
                        prev_signal_ = self.macd_px_[
                            counter_][0][MACDFields.signal.value]

                        curr_close_ = self.macd_px_[
                            counter_][1][MACDFields.close_.value]
                        sema_ = 2 * curr_close_ / (self.day_sema_param_ + 1) + prev_sema_ * (
                            1 - (2 / (self.day_sema_param_ + 1)))
                        lema_ = 2 * curr_close_ / (self.day_lema_param_ + 1) + prev_lema_ * (
                            1 - (2 / (self.day_lema_param_ + 1)))
                        macd_ = sema_ - lema_
                        signal_ = 2 * macd_ / (self.day_signal_param_ + 1) + prev_signal_ * (
                            1 - (2 / (self.day_signal_param_ + 1)))

                        self.macd_px_[counter_][1][MACDFields.sema.value] = sema_
                        self.macd_px_[counter_][1][MACDFields.lema.value] = lema_
                        self.macd_px_[counter_][1][MACDFields.macd.value] = macd_
                        self.macd_px_[counter_][1][MACDFields.signal.value] = signal_
            # print(self.macd_px_[counter_])

    # Computes Bar level MACD
    def getBarMACD(self, _time):
        t1 = time.time()
        # print(self.bar_training_data_)
        for counter_ in range(0, len(self.product_list_)):
            # self.training_data_[counter_].set_index("datetime")
            if len(self.bar_training_data_[counter_]) == self.total_bar_:
                # print(col,"hi")
                if self.macd_initialize_bar_[counter_] == 0:
                    col = self.bar_training_data_[counter_]["close"]
                    # self.bar_training_data_[counter_]["sema"] = pd.ewma(col, span=self.sema_param_)
                    # self.bar_training_data_[counter_]["lema"] = pd.ewma(col, span=self.lema_param_)
                    # self.bar_training_data_[counter_]["macd"] = self.bar_training_data_[counter_]["sema"] - self.bar_training_data_[counter_]["lema"]
                    # self.bar_training_data_[counter_]["signal"] = pd.ewma(self.bar_training_data_[counter_]["macd"], span=self.signal_param_)

                    # self.bar_training_data_[counter_]["sema_filter"] = pd.ewma(col, span=self.sema_filter_param_)
                    # self.bar_training_data_[counter_]["lema_filter"] = pd.ewma(col, span=self.lema_filter_param_)
                    # self.bar_training_data_[counter_]["macd_filter"] = self.bar_training_data_[counter_]["sema_filter"] - self.bar_training_data_[counter_]["lema_filter"]
                    # self.bar_training_data_[counter_]["signal_filter"] = pd.ewma(self.bar_training_data_[counter_]["macd_filter"], span=self.signal_filter_param_)

                    self.bar_training_data_[counter_]["sema"] = col.ewm(span=self.sema_param_).mean()
                    self.bar_training_data_[counter_]["lema"] = col.ewm(span=self.lema_param_).mean()
                    self.bar_training_data_[counter_]["macd"] = self.bar_training_data_[counter_]["sema"] - self.bar_training_data_[counter_]["lema"]
                    self.bar_training_data_[counter_]["signal"] = self.bar_training_data_[counter_]["macd"].ewm(span=self.signal_param_).mean()

                    self.bar_training_data_[counter_]["sema_filter"] = col.ewm(span=self.sema_filter_param_).mean()
                    self.bar_training_data_[counter_]["lema_filter"] = col.ewm(span=self.lema_filter_param_).mean()
                    self.bar_training_data_[counter_]["macd_filter"] = self.bar_training_data_[counter_]["sema_filter"] - self.bar_training_data_[counter_]["lema_filter"]
                    self.bar_training_data_[counter_]["signal_filter"] = self.bar_training_data_[counter_]["macd_filter"].ewm(span=self.signal_filter_param_).mean()

                    # self.bar_training_data_[counter_]["signal_filter"] = pd.ewma(self.bar_training_data_[counter_]["macd"],span=self.signal_filter_param_)
                    self.macd_initialize_bar_[counter_] = 1

                    if _time == self.bar_training_data_[counter_].get_value(
                            len(self.bar_training_data_[counter_]) - 1, "datetime"):
                        # macd_ = self.bar_training_data_[counter_].get_value(len(self.bar_training_data_[counter_]) + self.num_bars_[counter_]-1,"macd")
                        # signal_ = self.bar_training_data_[counter_].get_value(len(self.bar_training_data_[counter_]) + self.num_bars_[counter_]-1,"signal")
                        self.current_val_vec_[counter_][0] = (
                            self.bar_training_data_[counter_].loc[len(self.bar_training_data_[counter_]) - 1])
                        # self.bar_macd_px_[counter_] = np.append(self.bar_macd_px_[counter_], np.array([[_time,self.current_val_vec_[counter_][0][MACDFields.macd.value], self.current_val_vec_[counter_][0][MACDFields.signal.value]\
                        # ,self.current_val_vec_[counter_][0][MACDFields.macd_filter.value],self.current_val_vec_[counter_][0][MACDFields.signal_filter.value],0]]),axis=0)

                else:
                    # self.getMeanMomentCC()
                    if _time == self.current_val_vec_[counter_][1][MACDFields.time.value]:
                        prev_sema_ = self.current_val_vec_[
                            counter_][0][MACDFields.sema.value]
                        prev_lema_ = self.current_val_vec_[
                            counter_][0][MACDFields.lema.value]
                        prev_macd_ = prev_sema_ - prev_lema_
                        prev_signal_ = self.current_val_vec_[
                            counter_][0][MACDFields.signal.value]

                        prev_sema_filter_ = self.current_val_vec_[
                            counter_][0][MACDFields.sema_filter.value]
                        prev_lema_filter_ = self.current_val_vec_[
                            counter_][0][MACDFields.lema_filter.value]
                        prev_macd_filter_ = prev_sema_filter_ - prev_lema_filter_
                        prev_signal_filter_ = self.current_val_vec_[
                            counter_][0][MACDFields.signal_filter.value]

                        curr_close_ = self.current_val_vec_[
                            counter_][1][MACDFields.close_.value]
                        sema_ = 2 * curr_close_ / (self.sema_param_ + 1) + prev_sema_ * (1 -
                                                                                         (2 / (self.sema_param_ + 1)))
                        lema_ = 2 * curr_close_ / (self.lema_param_ + 1) + prev_lema_ * (1 -
                                                                                         (2 / (self.lema_param_ + 1)))
                        macd_ = sema_ - lema_
                        signal_ = 2 * macd_ / (self.signal_param_ + 1) + prev_signal_ * (1 -
                                                                                         (2 / (self.signal_param_ + 1)))

                        sema_filter_ = 2 * curr_close_ / (self.sema_filter_param_ + 1) + prev_sema_filter_ * (
                            1 - (2 / (self.sema_filter_param_ + 1)))
                        lema_filter_ = 2 * curr_close_ / (self.lema_filter_param_ + 1) + prev_lema_filter_ * (
                            1 - (2 / (self.lema_filter_param_ + 1)))
                        macd_filter_ = sema_filter_ - lema_filter_
                        signal_filter_ = 2 * macd_filter_ / (self.signal_filter_param_ + 1) + prev_signal_filter_ * (
                            1 - (2 / (self.signal_filter_param_ + 1)))
                        # signal_filter_ = 2*macd_/(self.signal_filter_param_+1) + prev_signal_filter_*(1- (2/(self.signal_filter_param_+1)))

                        self.current_val_vec_[
                            counter_][1][MACDFields.sema.value] = sema_
                        self.current_val_vec_[
                            counter_][1][MACDFields.lema.value] = lema_
                        self.current_val_vec_[
                            counter_][1][MACDFields.macd.value] = macd_
                        self.current_val_vec_[
                            counter_][1][MACDFields.signal.value] = signal_
                        self.current_val_vec_[
                            counter_][1][MACDFields.sema_filter.value] = sema_filter_
                        self.current_val_vec_[
                            counter_][1][MACDFields.lema_filter.value] = lema_filter_
                        self.current_val_vec_[
                            counter_][1][MACDFields.macd_filter.value] = macd_filter_
                        self.current_val_vec_[
                            counter_][1][MACDFields.signal_filter.value] = signal_filter_
                        # self.bar_macd_px_[counter_] = np.append(self.bar_macd_px_[counter_], np.array([[_time,macd_,signal_,macd_filter_,signal_filter_,0]]),axis=0)
                        # print(self.current_val_vec_[counter_][0][MACDFields.time.value],self.current_val_vec_[counter_][0][MACDFields.close_.value],self.current_val_vec_[counter_][0][MACDFields.macd.value],self.current_val_vec_[counter_][0][MACDFields.signal.value],self.current_val_vec_[counter_][0][MACDFields.macd_filter.value],self.current_val_vec_[counter_][0][MACDFields.signal_filter.value])
                        # self.current_val_vec_[counter_][0] = self.current_val_vec_[counter_][1]

        t2 = time.time()

    # Computes Day Bollinger and the bands associated with it
    def getDayBollinger(self, _counter, _new_val_, _old_val_):
        if _old_val_ == 0:
            self.day_bollinger_mean_[_counter] = self.training_data_[
                _counter]["close"].mean()
            std_ = self.training_data_[_counter]["close"].std()
            self.day_bollinger_uband_[_counter] = self.day_bollinger_mean_[
                _counter] + self.bollinger_std_band_ * std_
            self.day_bollinger_lband_[_counter] = self.day_bollinger_mean_[
                _counter] - self.bollinger_std_band_ * std_
        else:
            old_mean_ = self.day_bollinger_mean_[_counter]
            self.day_bollinger_mean_[_counter] = self.day_bollinger_mean_[_counter] + (
                _new_val_ - _old_val_) / self.bollinger_period_
            old_std_ = (self.day_bollinger_uband_[_counter] - self.day_bollinger_lband_[_counter]) / (
                2 * self.bollinger_std_band_)
            new_std_ = math.sqrt(old_std_ * old_std_ + (_new_val_ - _old_val_) *
                                 (_new_val_ - self.day_bollinger_mean_[_counter] + _old_val_ - old_mean_) /
                                 (self.bollinger_period_ - 1))
            self.day_bollinger_uband_[
                _counter] = self.day_bollinger_mean_[_counter] + self.bollinger_std_band_ * new_std_
            self.day_bollinger_lband_[
                _counter] = self.day_bollinger_mean_[_counter] - self.bollinger_std_band_ * new_std_

    # Computes Bar Bollinger and bands
        def getBarBollinger(self, _counter, _new_val_, _old_val_):
            if _old_val_ == 0:
                self.bollinger_mean_[_counter] = self.bollinger_data_[
                    _counter]["close"].mean()
                std_ = self.bollinger_data_[_counter]["close"].std()
                self.bollinger_uband_[_counter] = self.bollinger_mean_[
                    _counter] + self.bollinger_std_band_*std_
                self.bollinger_lband_[_counter] = self.bollinger_mean_[
                    _counter] - self.bollinger_std_band_*std_
            else:
                old_mean_ = self.bollinger_mean_[_counter]
                self.bollinger_mean_[_counter] = self.bollinger_mean_[
                    _counter] + (_new_val_ - _old_val_)/self.bollinger_period_
                old_std_ = (self.bollinger_uband_[
                            _counter] - self.bollinger_lband_[_counter])/(2*self.bollinger_std_band_)
                new_std_ = math.sqrt(old_std_*old_std_ + (_new_val_ - _old_val_)*(
                    _new_val_ - self.bollinger_mean_[_counter] + _old_val_ - old_mean_) / (self.bollinger_period_-1))
                self.bollinger_uband_[_counter] = self.bollinger_mean_[
                    _counter] + self.bollinger_std_band_*new_std_
                self.bollinger_lband_[_counter] = self.bollinger_mean_[
                    _counter] - self.bollinger_std_band_*new_std_

    # Responsible for bar creation after receiving input from BarGenerator and sending the bar to onbarupdate
    def aggregator(self, _prod_bar_info, _time):
        if self.last_bar_update_ == 0:
            self.last_bar_update_ = self.om_.started_at_

        for index_ in range(0, len(self.product_list_)):
            open_, close_, low_, high_, vol_ = _prod_bar_info[index_]
            if self.bar_agg_[index_][0] == 0 and vol_:
                self.bar_agg_[index_] = _prod_bar_info[index_]
                self.bar_agg_[index_][4] = vol_
            else:
                if vol_:
                    self.bar_agg_[index_][1] = close_
                    self.bar_agg_[index_][2] = min(
                        self.bar_agg_[index_][2], low_)
                    self.bar_agg_[index_][3] = max(
                        self.bar_agg_[index_][3], high_)
                    self.bar_agg_[index_][4] = self.bar_agg_[index_][4] + vol_

            if self.day_bar_agg_[index_][0] == 0 and vol_:
                self.day_bar_agg_[index_] = _prod_bar_info[index_]
                self.day_bar_agg_[index_][4] = vol_

            else:
                if vol_:
                    self.day_bar_agg_[index_][1] = close_
                    self.day_bar_agg_[index_][2] = min(
                        self.day_bar_agg_[index_][2], low_)
                    self.day_bar_agg_[index_][3] = max(
                        self.day_bar_agg_[index_][3], high_)
                    self.day_bar_agg_[index_][4] = self.day_bar_agg_[
                        index_][4] + vol_
        if _time == self.last_bar_update_ + self.granularity_ or _time - self.om_.ended_at_ == (datetime.strptime(
                self.strat_end_time_, "%H_%M") - datetime.strptime(self.train_end_time_, "%H_%M")).total_seconds():
            # print(_time,self.product_list_[index_],self.bar_agg_[index_][4],self.last_bar_update_)
            self.last_bar_update_ = _time
            t0 = time.time()
            t2 = t0
            for index_ in range(0, len(self.product_list_)):
                if self.bar_agg_[index_][4]:
                    if self.macd_initialize_bar_[index_] == 0:
                        self.bar_training_data_[index_].loc[len(self.bar_training_data_[index_])] \
                            = [_time-self.granularity_, self.bar_agg_[index_][0], self.bar_agg_[index_][1], self.bar_agg_[index_][2], self.bar_agg_[index_][3], self.bar_agg_[index_][4], 0, 0, 0, 0, 0, 0, 0, 0]
                    else:
                        self.current_val_vec_[
                            index_][0] = self.current_val_vec_[index_][1]
                        self.current_val_vec_[index_][1] = [
                            _time -
                            self.granularity_, self.bar_agg_[
                                index_][0], self.bar_agg_[index_][1],
                            self.bar_agg_[index_][2], self.bar_agg_[
                                index_][3], self.bar_agg_[index_][4], 0, 0, 0, 0, 0,
                            0, 0, 0
                        ]

                    if self.bollinger_period_ > len(self.bollinger_data_[index_]):
                        self.bollinger_data_[index_].loc[len(
                            self.bollinger_data_[index_])] = [_time - self.granularity_, self.bar_agg_[index_][1]]
                        # if self.bollinger_period_ == len(self.bollinger_data_[index_]):
                        #	self.getBarBollinger(index_,0,0)
                    else:
                        old_val_ = self.bollinger_data_[index_].get_value(
                            self.oldest_bollinger_[index_], "close")
                        self.bollinger_data_[index_].set_value(self.oldest_bollinger_[index_], "datetime",
                                                               _time - self.granularity_)
                        new_val_ = self.bar_agg_[index_][1]
                        self.bollinger_data_[index_].set_value(
                            self.oldest_bollinger_[index_], "close", new_val_)
                        # self.getBarBollinger(index_,new_val_,old_val_)
                        self.oldest_bollinger_[index_] = (self.oldest_bollinger_[index_] + 1) % len(
                            self.bollinger_data_[index_])

            t3 = time.time()
            # print("aggregator3 " + str(t3-t2))
            self.getBarMACD(_time - self.granularity_)
            for index_ in range(0, len(self.product_list_)):
                prod_ = self.product_list_[index_]
                curr_macd_ = self.current_val_vec_[index_][1][MACDFields.macd.value]
                curr_signal_ = self.current_val_vec_[
                    index_][1][MACDFields.signal.value]
                prev_macd_ = self.current_val_vec_[index_][0][MACDFields.macd.value]
                prev_signal_ = self.current_val_vec_[
                    index_][0][MACDFields.signal.value]
                if curr_macd_ > curr_signal_ and prev_macd_ < prev_signal_:
                    self.macd_crossover_[index_] = self.macd_crossover_hist_
                    # print("macd crossover_+",_time-self.granularity_)
                elif curr_macd_ < curr_signal_ and prev_macd_ > prev_signal_:
                    self.macd_crossover_[index_] = - \
                        1 * self.macd_crossover_hist_
                    # print("macd crossover_-",_time-self.granularity_)
                else:
                    if self.macd_crossover_[index_] > 0:
                        self.macd_crossover_[
                            index_] = self.macd_crossover_[index_] - 1
                    elif self.macd_crossover_[index_] < 0:
                        self.macd_crossover_[
                            index_] = self.macd_crossover_[index_] + 1
                    # print(self.macd_crossover_[index_],_time-self.granularity_)
            if _time-self.om_.started_at_ >= (datetime.strptime(self.strat_start_time_, "%H_%M") - datetime.strptime(self.train_start_time_, "%H_%M")).total_seconds() \
                    and _time-self.om_.ended_at_ <= (datetime.strptime(self.strat_end_time_, "%H_%M") - datetime.strptime(self.train_end_time_, "%H_%M")).total_seconds():

                self.onBarUpdate(self.bar_agg_, _time)
            self.bar_agg_ = np.zeros(
                len(self.product_list_) * 5).reshape(len(self.product_list_), 5)

        # Happens at EOD resetting variables
        if (_time - self.om_.started_at_) == (datetime.strptime(self.train_end_time_, "%H_%M") - datetime.strptime(
                self.train_start_time_, "%H_%M")).total_seconds():
            self.reset()
            self.last_bar_update_ = 0
            self.updateTrainingBar(self.day_bar_agg_)
            self.day_bar_agg_ = np.zeros(
                len(self.product_list_) * 5).reshape(len(self.product_list_), 5)
            self.bar_agg_ = np.zeros(
                len(self.product_list_) * 5).reshape(len(self.product_list_), 5)
            self.current_cascade_ = [0] * len(self.product_list_)
            self.prod_directions_ = [0] * len(self.product_list_)
            # self.sema_ = [0]*len(self.product_list_)
            # self.lema_ = [0]*len(self.product_list_)
            # self.signal_ = [0]*len(self.product_list_)
            self.macd_ = [0] * len(self.product_list_)
            self.crossover_ = [0] * len(self.product_list_)
            self.bollinger_move_ = [0] * len(self.product_list_)
            self.macd_crossover_ = [0] * len(self.product_list_)
            self.current_volume_ = [0] * len(self.product_list_)

    # Called from aggregator and handles main strat execution and reaction to granular bar
    def onBarUpdate(self, _prod_bar_info, _time):
        self.current_time_ = _time
        for index_ in range(0, len(self.product_list_)):
            prod_ = self.product_list_[index_]
            # pos_pnl_index_ = str(prod_+"_"+str(_time))
            self.prod_curr_posn_[index_][Fields.cascade_trigger.value] = False
            open_, close_, low_, high_, vol_ = _prod_bar_info[index_]
            self.current_bar_[index_] = [open_, close_, low_, high_]
            if self.current_time_ - self.om_.started_at_ == self.granularity_:
                if vol_:
                    self.opening_bar_volume_[index_][self.open_vol_count_[
                        index_] % self.open_vol_days_] = vol_
                else:
                    self.opening_bar_volume_[index_][self.open_vol_count_[
                        index_] % self.open_vol_days_] = np.NaN
                self.open_vol_count_[index_] = self.open_vol_count_[index_] + 1
            if self.prod_curr_posn_[index_][Fields.start_trading.value]:

                if vol_:
                    self.current_volume_[index_] = vol_
                    pnl_ = self.prod_curr_posn_[index_][Fields.pnl.value] + self.prod_curr_posn_[index_][Fields.pos.value] * (
                        close_ - self.prod_curr_posn_[index_][Fields.ltp.value])
                    cascade_condition_ = False
                    if pnl_ > self.prod_curr_posn_[index_][Fields.pnl.value] and pnl_ > 0:
                        self.prod_curr_posn_[
                            index_][Fields.cascade_trigger.value] = True
                    self.prod_curr_posn_[index_][Fields.pnl.value] = pnl_
                    self.prod_curr_posn_[index_][Fields.ltp.value] = close_
                    self.prod_curr_posn_[index_][Fields.last_updated.value] = _time
                    self.prod_curr_posn_[index_][Fields.close_.value] = close_
                    if not self.prod_curr_posn_[index_][Fields.open_.value]:
                        self.prod_curr_posn_[index_][Fields.open_.value] = open_
                        self.prod_curr_posn_[index_][Fields.low_.value] = low_
                        self.prod_curr_posn_[index_][Fields.high_.value] = high_
                    else:
                        if self.prod_curr_posn_[index_][Fields.low_.value] > low_:
                            self.prod_curr_posn_[index_][Fields.low_.value] = low_
                        if self.prod_curr_posn_[index_][Fields.high_.value] < high_:
                            self.prod_curr_posn_[index_][Fields.high_.value] = high_
                    self.pos_pnl_df_[index_][self.day_index_].append([
                        prod_, _time, self.prod_curr_posn_[index_][Fields.pos.value],
                        self.prod_curr_posn_[index_][Fields.pnl.value], self.prod_curr_posn_[
                            index_][Fields.ltp.value],
                        self.prod_curr_posn_[
                            index_][Fields.slippage_cost.value], open_, close_, low_, high_, vol_, 0
                    ])
                    self.updateTrailingSL(index_)
                    self.checkSL(index_, self.prod_curr_posn_[
                                 index_][Fields.ltp.value])

                else:
                    self.pos_pnl_df_[index_][self.day_index_].append([
                        prod_, _time, self.prod_curr_posn_[index_][Fields.pos.value],
                        self.prod_curr_posn_[index_][Fields.pnl.value], self.prod_curr_posn_[
                            index_][Fields.ltp.value],
                        self.prod_curr_posn_[
                            index_][Fields.slippage_cost.value], open_, close_, low_, high_, vol_, 0
                    ])

        if self.current_time_ - self.om_.ended_at_ >= (datetime.strptime(
                self.strat_end_time_, "%H_%M") - datetime.strptime(self.train_end_time_, "%H_%M")).total_seconds():
            self.getFlat()

            return
        self.takeDecision()

    # calls relevant alpha which takes the decision to buy/sell the position
    def takeDecision(self):

        for index_ in range(0, len(self.product_list_)):
            prod_ = self.product_list_[index_]
            curr_macd_ = self.current_val_vec_[index_][1][MACDFields.macd.value]
            curr_signal_ = self.current_val_vec_[index_][1][MACDFields.signal.value]
            prev_macd_ = self.current_val_vec_[index_][0][MACDFields.macd.value]
            prev_signal_ = self.current_val_vec_[index_][0][MACDFields.signal.value]
            # if curr_macd_ > curr_signal_ and prev_macd_ < prev_signal_:

            if self.prod_curr_posn_[index_][Fields.close_.value] and not self.prod_curr_posn_[index_][Fields.sqoff_on.value]\
                    and self.prod_curr_posn_[index_][Fields.start_trading.value] and self.macd_initialize_bar_[index_] != 0:
                # if self.prod_curr_posn_[index_][Fields.pos.value] == 0 or self.prod_curr_posn_[index_][Fields.cascade_trigger.value]:

                # self.tradeMomentumMeanStdBarMACDDayBollingerLKBKOpenVolFilter(prod_,index_)
                # self.tradeBarMACDDayBollingerLKBKOpenVolFilter(prod_,index_)
                self.tradeBarMACDDayBollingerLKBKDayLHOpenVolFilter(prod_, index_)
                # self.tradeBarMACDDayBollingerLKBKBandDayLHOpenVolFilter(prod_,index_)

                # self.tradeBarMACDMeanRevert(prod_,index_)

            self.prev_bar_close_[index_] = self.prod_curr_posn_[
                index_][Fields.close_.value]
            if prev_macd_ == prev_signal_:
                self.current_slope_[index_] = 0
            else:
                self.current_slope_[index_] = (
                    curr_macd_ - curr_signal_) / (prev_macd_ - prev_signal_)

    # Trade when day macd crosses day signal line
    def tradeDayMACD(self, _prod, _counter):
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])

        curr_sema_ = self.macd_px_[_counter][1][MACDFields.sema.value]
        curr_lema_ = self.macd_px_[_counter][1][MACDFields.lema.value]
        curr_macd_ = self.macd_px_[_counter][1][MACDFields.macd.value]
        curr_signal_ = self.macd_px_[_counter][1][MACDFields.signal.value]

        prev_macd_ = self.macd_px_[_counter][0][MACDFields.macd.value]
        prev_signal_ = self.macd_px_[_counter][0][MACDFields.signal.value]

        if curr_macd_ != 0 and curr_signal_ != 0:
            curr_close_ = self.prod_curr_posn_[_counter][Fields.close_.value]
            sema_ = 2 * curr_close_ / \
                (self.day_sema_param_ + 1) + curr_sema_ * \
                (1 - (2 / (self.day_sema_param_ + 1)))
            lema_ = 2 * curr_close_ / \
                (self.day_lema_param_ + 1) + curr_lema_ * \
                (1 - (2 / (self.day_lema_param_ + 1)))
            macd_ = sema_ - lema_
            signal_ = 2 * macd_ / (self.day_signal_param_ + 1) + \
                curr_signal_ * (1 - (2 / (self.day_signal_param_ + 1)))

            # Positive crossover of macd, and hence this is a time to buy.
            if macd_ > signal_ and curr_macd_ < curr_signal_:
                if self.prod_curr_posn_[_counter][Fields.pos.value] == 0 and self.prod_curr_posn_[_counter][Fields.
                                                                                                      start_trading]:
                    self.sendOrder('B', _counter, pos_, self.prod_curr_posn_[_counter][Fields.close_.value],
                                   self.current_time_)
            elif macd_ < signal_ and curr_macd_ > curr_signal_:  # Negative crossover and a sell signal
                if self.prod_curr_posn_[_counter][Fields.pos.value] == 0 and self.prod_curr_posn_[_counter][Fields.
                                                                                                      start_trading]:
                    self.sendOrder('S', _counter, pos_, self.prod_curr_posn_[_counter][Fields.close_.value],
                                   self.current_time_)
                    # print("-ve crossover")

    # Trade a combination of day macd cross and day bollinger cross
    def tradeDayMACDDayBollinger(self, _prod, _counter):
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])

        curr_sema_ = self.macd_px_[_counter][1][MACDFields.sema.value]
        curr_lema_ = self.macd_px_[_counter][1][MACDFields.lema.value]
        curr_macd_ = self.macd_px_[_counter][1][MACDFields.macd.value]
        curr_signal_ = self.macd_px_[_counter][1][MACDFields.signal.value]

        prev_macd_ = self.macd_px_[_counter][0][MACDFields.macd.value]
        prev_signal_ = self.macd_px_[_counter][0][MACDFields.signal.value]

        if curr_macd_ != 0 and curr_signal_ != 0:
            curr_close_ = self.prod_curr_posn_[_counter][Fields.close_.value]
            sema_ = 2 * curr_close_ / \
                (self.day_sema_param_ + 1) + curr_sema_ * \
                (1 - (2 / (self.day_sema_param_ + 1)))
            lema_ = 2 * curr_close_ / \
                (self.day_lema_param_ + 1) + curr_lema_ * \
                (1 - (2 / (self.day_lema_param_ + 1)))
            macd_ = sema_ - lema_
            signal_ = 2 * macd_ / (self.day_signal_param_ + 1) + \
                curr_signal_ * (1 - (2 / (self.day_signal_param_ + 1)))
            if macd_ > signal_ and curr_macd_ < curr_signal_:  # +ve crossover. Hence sending a buy
                if self.prod_curr_posn_[_counter][Fields.pos.value] == 0 and self.prod_curr_posn_[_counter][Fields.start_trading.value]\
                        and self.prod_curr_posn_[_counter][Fields.close_.value] > self.day_bollinger_mean_[_counter]\
                        and self.prev_bar_close_[_counter] < self.day_bollinger_mean_[_counter]:
                    self.sendOrder('B', _counter, pos_, self.prod_curr_posn_[_counter][Fields.close_.value],
                                   self.current_time_)
            elif macd_ < signal_ and curr_macd_ > curr_signal_:  # Negative crossover. Hence sending a sell
                if self.prod_curr_posn_[_counter][Fields.pos.value] == 0 and self.prod_curr_posn_[_counter][Fields.start_trading.value]\
                        and self.prod_curr_posn_[_counter][Fields.close_.value] < self.day_bollinger_mean_[_counter] \
                        and self.prev_bar_close_[_counter] > self.day_bollinger_mean_[_counter]:
                    self.sendOrder('S', _counter, pos_, self.prod_curr_posn_[_counter][Fields.close_.value],
                                   self.current_time_)

    # Trade bar macd
    def tradeBarMACD(self, _prod, _counter):
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])
        # curr_macd_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][1]
        # curr_signal_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][2]
        curr_macd_ = self.current_val_vec_[_counter][1][MACDFields.macd.value]
        curr_signal_ = self.current_val_vec_[_counter][1][MACDFields.signal.value]
        prev_macd_ = self.current_val_vec_[_counter][0][MACDFields.macd.value]
        prev_signal_ = self.current_val_vec_[_counter][0][MACDFields.signal.value]

        if curr_macd_ > curr_signal_ and prev_macd_ < prev_signal_:
            if self.prod_curr_posn_[_counter][Fields.pos.value] < 0:
                self.squareOff(_counter, self.prod_curr_posn_[
                               _counter][Fields.close_.value])
            elif self.prod_curr_posn_[_counter][Fields.pos.value] == 0 and self.prod_curr_posn_[_counter][Fields.
                                                                                                    start_trading]:
                self.sendOrder('B', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)
        elif curr_macd_ < curr_signal_ and prev_macd_ > prev_signal_:
            if self.prod_curr_posn_[_counter][Fields.pos.value] > 0:
                self.squareOff(_counter, self.prod_curr_posn_[
                               _counter][Fields.close_.value])
            elif self.prod_curr_posn_[_counter][Fields.pos.value] == 0 and self.prod_curr_posn_[_counter][Fields.
                                                                                                    start_trading]:
                self.sendOrder('S', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)

    # trade bar macd and day bollinger mean
    def tradeBarMACDDayBollinger(self, _prod, _counter):
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])
        # curr_macd_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][1]
        # curr_signal_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][2]
        curr_macd_ = self.current_val_vec_[_counter][1][MACDFields.macd.value]
        curr_signal_ = self.current_val_vec_[_counter][1][MACDFields.signal.value]
        prev_macd_ = self.current_val_vec_[_counter][0][MACDFields.macd.value]
        prev_signal_ = self.current_val_vec_[_counter][0][MACDFields.signal.value]

        if curr_macd_ > curr_signal_ and prev_macd_ < prev_signal_:
            if self.prod_curr_posn_[_counter][Fields.pos.value] < 0:
                self.squareOff(_counter, self.prod_curr_posn_[
                               _counter][Fields.close_.value])
            elif self.prod_curr_posn_[_counter][Fields.pos.value] == 0 and self.prod_curr_posn_[_counter][Fields.start_trading.value]\
                    and self.prod_curr_posn_[_counter][Fields.close_.value] > self.day_bollinger_mean_[_counter]\
                    and self.prev_day_close_[_counter] < self.day_bollinger_mean_[_counter]:
                self.sendOrder('B', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)
        elif curr_macd_ < curr_signal_ and prev_macd_ > prev_signal_:
            if self.prod_curr_posn_[_counter][Fields.pos.value] > 0:
                self.squareOff(_counter, self.prod_curr_posn_[
                               _counter][Fields.close_.value])
            elif self.prod_curr_posn_[_counter][Fields.pos.value] == 0 and self.prod_curr_posn_[_counter][Fields.start_trading.value]\
                    and self.prod_curr_posn_[_counter][Fields.close_.value] < self.day_bollinger_mean_[_counter] \
                    and self.prev_day_close_[_counter] > self.day_bollinger_mean_[_counter]:
                self.sendOrder('S', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)

    # Trade bar macd and day bollinger
    def tradeBarMACDDayBollingerLKBK(self, _prod, _counter):
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])
        # curr_macd_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][1]
        # curr_signal_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][2]
        curr_macd_ = self.current_val_vec_[_counter][1][MACDFields.macd.value]
        curr_signal_ = self.current_val_vec_[_counter][1][MACDFields.signal.value]
        prev_macd_ = self.current_val_vec_[_counter][0][MACDFields.macd.value]
        prev_signal_ = self.current_val_vec_[_counter][0][MACDFields.signal.value]

        if self.current_cascade_[_counter] == 0 and self.prod_curr_posn_[_counter][Fields.start_trading.value]:
            if self.prod_curr_posn_[_counter][Fields.close_.value] > self.day_bollinger_mean_[_counter]\
                    and self.prev_day_close_[_counter] < self.day_bollinger_mean_[_counter]:
                if (curr_macd_ > curr_signal_ and prev_macd_ < prev_signal_) or self.macd_crossover_[_counter] > 0:
                    self.sendOrder('B', _counter, self.cascade_size_[self.current_cascade_[_counter]] * pos_,
                                   self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                    self.prod_directions_[_counter] = 1
                    self.current_cascade_[
                        _counter] = self.current_cascade_[_counter] + 1
            elif self.prod_curr_posn_[_counter][Fields.close_.value] < self.day_bollinger_mean_[_counter] \
                    and self.prev_day_close_[_counter] > self.day_bollinger_mean_[_counter]:
                if (curr_macd_ < curr_signal_ and prev_macd_ > prev_signal_) or self.macd_crossover_[_counter] < 0:
                    self.sendOrder('S', _counter, self.cascade_size_[self.current_cascade_[_counter]] * pos_,
                                   self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                    self.prod_directions_[_counter] = -1
                    self.current_cascade_[
                        _counter] = self.current_cascade_[_counter] + 1

        elif self.prod_curr_posn_[_counter][Fields.start_trading.value] and self.prod_curr_posn_[_counter][Fields.cascade_trigger.value] == True \
                and self.current_cascade_[_counter] < self.cascade_num_:

            if self.prod_directions_[_counter] > 0:
                self.sendOrder('B', _counter, self.cascade_size_[self.current_cascade_[_counter]] * pos_,
                               self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                self.current_cascade_[
                    _counter] = self.current_cascade_[_counter] + 1
            # elif curr_macd_ - curr_signal_ < 0 and curr_macd_ - curr_signal_ < prev_macd_ -prev_signal_\
            # 	and self.current_cascade_[_counter]  != 0 and self.prod_directions_[_counter] < 0:
            elif self.prod_directions_[_counter] < 0:
                self.sendOrder('S', _counter, self.cascade_size_[self.current_cascade_[_counter]] * pos_,
                               self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                self.current_cascade_[
                    _counter] = self.current_cascade_[_counter] + 1

    # Use momentum price filter and opening volume filter  along with bar macd and bollinger day

    def tradeMomentumMeanStdBarMACDDayBollingerLKBKOpenVolFilter(self, _prod, _counter):
        current_cc_ = self.prod_curr_posn_[
            _counter][Fields.close_.value] - self.prev_day_close_[_counter]
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])
        # curr_macd_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][1]
        # curr_signal_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][2]
        curr_macd_ = self.current_val_vec_[_counter][1][MACDFields.macd.value]
        curr_signal_ = self.current_val_vec_[_counter][1][MACDFields.signal.value]
        prev_macd_ = self.current_val_vec_[_counter][0][MACDFields.macd.value]
        prev_signal_ = self.current_val_vec_[_counter][0][MACDFields.signal.value]

        if current_cc_*(self.prod_curr_posn_[_counter][Fields.close_.value] - self.prod_curr_posn_[_counter][Fields.open_.value]) >= 0 and \
            self.current_cascade_[_counter] == 0 and self.prod_curr_posn_[_counter][Fields.start_trading.value]\
            and (not math.isnan(self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1) % self.open_vol_days_]) and
                 (math.isnan(self.open_vol_avg_[_counter]) or
                  self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1) % self.open_vol_days_] > self.open_vol_avg_[_counter])):
            if self.prod_curr_posn_[_counter][Fields.close_.value] > self.day_bollinger_mean_[_counter]\
                    and current_cc_ < self.momentum_cc_wt_mean_[_counter] + self.momentum_cc_wt_stdev_[_counter] \
                    and self.prev_day_close_[_counter] < self.day_bollinger_mean_[_counter]:
                if (curr_macd_ > curr_signal_ and prev_macd_ < prev_signal_) or self.macd_crossover_[_counter] > 0:
                    self.current_cascade_[
                        _counter] = self.current_cascade_[_counter] + 1
                    print(self.prev_day_close_[_counter], self.prod_curr_posn_[
                          _counter][Fields.open_.value])
                    self.sendOrder('B', _counter, self.cascade_size_[self.current_cascade_[_counter] - 1] * pos_,
                                   self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                    self.prod_directions_[_counter] = 1
            elif self.prod_curr_posn_[_counter][Fields.close_.value] < self.day_bollinger_mean_[_counter] \
                    and current_cc_ > -1*(self.momentum_cc_wt_mean_[_counter] + self.momentum_cc_wt_stdev_[_counter]) \
                    and self.prev_day_close_[_counter] > self.day_bollinger_mean_[_counter]:
                if (curr_macd_ < curr_signal_ and prev_macd_ > prev_signal_) or self.macd_crossover_[_counter] < 0:
                    self.current_cascade_[
                        _counter] = self.current_cascade_[_counter] + 1
                    print(self.prev_day_close_[_counter], self.prod_curr_posn_[
                          _counter][Fields.open_.value])
                    self.sendOrder('S', _counter, self.cascade_size_[self.current_cascade_[_counter] - 1] * pos_,
                                   self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                    self.prod_directions_[_counter] = -1

        elif self.prod_curr_posn_[_counter][Fields.start_trading.value] and self.prod_curr_posn_[_counter][Fields.cascade_trigger.value] == True \
                and self.current_cascade_[_counter] < self.cascade_num_:

            if self.prod_directions_[_counter] > 0:
                self.sendOrder('B', _counter, self.cascade_size_[self.current_cascade_[_counter]] * pos_,
                               self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                self.current_cascade_[
                    _counter] = self.current_cascade_[_counter] + 1
            # elif curr_macd_ - curr_signal_ < 0 and curr_macd_ - curr_signal_ < prev_macd_ -prev_signal_\
            # 	and self.current_cascade_[_counter]  != 0 and self.prod_directions_[_counter] < 0:
            elif self.prod_directions_[_counter] < 0:
                self.sendOrder('S', _counter, self.cascade_size_[self.current_cascade_[_counter]] * pos_,
                               self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                self.current_cascade_[
                    _counter] = self.current_cascade_[_counter] + 1

    # add opening volume filter to previous trading alphas
    def tradeBarMACDDayBollingerLKBKOpenVolFilter(self, _prod, _counter):
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])
        # curr_macd_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][1]
        # curr_signal_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][2]
        curr_macd_ = self.current_val_vec_[_counter][1][MACDFields.macd.value]
        curr_signal_ = self.current_val_vec_[_counter][1][MACDFields.signal.value]
        prev_macd_ = self.current_val_vec_[_counter][0][MACDFields.macd.value]
        prev_signal_ = self.current_val_vec_[_counter][0][MACDFields.signal.value]

        if self.current_cascade_[_counter] == 0 and self.prod_curr_posn_[_counter][Fields.start_trading.value]\
            and (not math.isnan(self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1) % self.open_vol_days_]) and
                 (math.isnan(self.open_vol_avg_[_counter]) or
                  self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1) % self.open_vol_days_] > self.open_vol_avg_[_counter])):
            if self.prod_curr_posn_[_counter][Fields.close_.value] > self.day_bollinger_mean_[_counter]\
                    and self.prev_day_close_[_counter] < self.day_bollinger_mean_[_counter]:
                if (curr_macd_ > curr_signal_ and prev_macd_ < prev_signal_) or self.macd_crossover_[_counter] > 0:
                    self.current_cascade_[
                        _counter] = self.current_cascade_[_counter] + 1
                    self.sendOrder('B', _counter, self.cascade_size_[self.current_cascade_[_counter] - 1] * pos_,
                                   self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                    self.prod_directions_[_counter] = 1
            elif self.prod_curr_posn_[_counter][Fields.close_.value] < self.day_bollinger_mean_[_counter] \
                    and self.prev_day_close_[_counter] > self.day_bollinger_mean_[_counter]:
                if (curr_macd_ < curr_signal_ and prev_macd_ > prev_signal_) or self.macd_crossover_[_counter] < 0:
                    self.current_cascade_[
                        _counter] = self.current_cascade_[_counter] + 1
                    self.sendOrder('S', _counter, self.cascade_size_[self.current_cascade_[_counter] - 1] * pos_,
                                   self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                    self.prod_directions_[_counter] = -1

        elif self.prod_curr_posn_[_counter][Fields.start_trading.value] and self.prod_curr_posn_[_counter][Fields.cascade_trigger.value] == True \
                and self.current_cascade_[_counter] < self.cascade_num_:

            if self.prod_directions_[_counter] > 0 and curr_macd_ > curr_signal_:
                self.sendOrder('B', _counter, self.cascade_size_[self.current_cascade_[_counter]] * pos_,
                               self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                self.current_cascade_[
                    _counter] = self.current_cascade_[_counter] + 1
            # elif curr_macd_ - curr_signal_ < 0 and curr_macd_ - curr_signal_ < prev_macd_ -prev_signal_\
            # 	and self.current_cascade_[_counter]  != 0 and self.prod_directions_[_counter] < 0:
            elif self.prod_directions_[_counter] < 0 and curr_macd_ < curr_signal_:
                self.sendOrder('S', _counter, self.cascade_size_[self.current_cascade_[_counter]] * pos_,
                               self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                self.current_cascade_[
                    _counter] = self.current_cascade_[_counter] + 1

    # trade bar macd day bollinger and use previous day high low lookback and volume filter
    def tradeBarMACDDayBollingerLKBKDayLHOpenVolFilter(self, _prod, _counter):
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])
        # curr_macd_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][1]
        # curr_signal_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][2]
        curr_macd_ = self.current_val_vec_[_counter][1][MACDFields.macd.value]
        curr_signal_ = self.current_val_vec_[_counter][1][MACDFields.signal.value]
        prev_macd_ = self.current_val_vec_[_counter][0][MACDFields.macd.value]
        prev_signal_ = self.current_val_vec_[_counter][0][MACDFields.signal.value]

        if self.current_cascade_[_counter] == 0 and self.prod_curr_posn_[_counter][Fields.start_trading.value]\
            and (not math.isnan(self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1) % self.open_vol_days_]) and
                 (math.isnan(self.open_vol_avg_[_counter]) or
                  self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1) % self.open_vol_days_] > self.open_vol_avg_[_counter])):
            if self.prod_curr_posn_[_counter][Fields.close_.value] > self.day_bollinger_mean_[_counter]\
                    and self.current_bar_[_counter][3] == self.prod_curr_posn_[_counter][Fields.high_.value]\
                    and self.current_bar_[_counter][1] > self.current_bar_[_counter][0]\
                    and self.prev_day_close_[_counter] < self.day_bollinger_mean_[_counter]:
                if (curr_macd_ > curr_signal_ and prev_macd_ < prev_signal_) or self.macd_crossover_[_counter] > 0:
                    self.current_cascade_[
                        _counter] = self.current_cascade_[_counter] + 1
                    self.sendOrder('B', _counter, self.cascade_size_[self.current_cascade_[_counter] - 1] * pos_,
                                   self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                    self.prod_directions_[_counter] = 1
            elif self.prod_curr_posn_[_counter][Fields.close_.value] < self.day_bollinger_mean_[_counter] \
                    and self.current_bar_[_counter][2] == self.prod_curr_posn_[_counter][Fields.low_.value]\
                    and self.current_bar_[_counter][1] < self.current_bar_[_counter][0]\
                    and self.prev_day_close_[_counter] > self.day_bollinger_mean_[_counter]:
                if (curr_macd_ < curr_signal_ and prev_macd_ > prev_signal_) or self.macd_crossover_[_counter] < 0:
                    self.current_cascade_[
                        _counter] = self.current_cascade_[_counter] + 1
                    self.sendOrder('S', _counter, self.cascade_size_[self.current_cascade_[_counter] - 1] * pos_,
                                   self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                    self.prod_directions_[_counter] = -1

        elif self.prod_curr_posn_[_counter][Fields.start_trading.value] and self.prod_curr_posn_[_counter][Fields.cascade_trigger.value] == True \
                and self.current_cascade_[_counter] < self.cascade_num_:

            if self.prod_directions_[_counter] > 0:
                self.sendOrder('B', _counter, self.cascade_size_[self.current_cascade_[_counter]] * pos_,
                               self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                self.current_cascade_[
                    _counter] = self.current_cascade_[_counter] + 1
            # elif curr_macd_ - curr_signal_ < 0 and curr_macd_ - curr_signal_ < prev_macd_ -prev_signal_\
            # 	and self.current_cascade_[_counter]  != 0 and self.prod_directions_[_counter] < 0:
            elif self.prod_directions_[_counter] < 0:
                self.sendOrder('S', _counter, self.cascade_size_[self.current_cascade_[_counter]] * pos_,
                               self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                self.current_cascade_[
                    _counter] = self.current_cascade_[_counter] + 1

    # trade bar macd day bollinger and use previous day high low lookback and volume filter
    def tradeBarMACDDayBollingerLKBKBandDayLHOpenVolFilter(self, _prod, _counter):
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])
        # curr_macd_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][1]
        # curr_signal_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][2]
        curr_macd_ = self.current_val_vec_[_counter][1][MACDFields.macd.value]
        curr_signal_ = self.current_val_vec_[_counter][1][MACDFields.signal.value]
        prev_macd_ = self.current_val_vec_[_counter][0][MACDFields.macd.value]
        prev_signal_ = self.current_val_vec_[_counter][0][MACDFields.signal.value]

        if self.current_cascade_[_counter] == 0 and self.prod_curr_posn_[_counter][Fields.start_trading.value]\
            and (not math.isnan(self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1) % self.open_vol_days_]) and
                 (math.isnan(self.open_vol_avg_[_counter]) or
                  self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1) % self.open_vol_days_] > self.open_vol_avg_[_counter])):    
            if self.current_bar_[_counter][3] == self.prod_curr_posn_[_counter][Fields.high_.value]\
                    and self.current_bar_[_counter][1] > self.current_bar_[_counter][0]\
                    and ((self.prod_curr_posn_[_counter][Fields.close_.value] > self.day_bollinger_mean_[_counter]\
                    and self.prev_day_close_[_counter] < self.day_bollinger_mean_[_counter])or \
                    (self.prod_curr_posn_[_counter][Fields.close_.value] > self.day_bollinger_uband_[_counter]\
                    and self.prev_day_close_[_counter] < self.day_bollinger_uband_[_counter])):
                if (curr_macd_ > curr_signal_ and prev_macd_ < prev_signal_) or self.macd_crossover_[_counter] > 0:
                    self.current_cascade_[
                        _counter] = self.current_cascade_[_counter] + 1
                    self.sendOrder('B', _counter, self.cascade_size_[self.current_cascade_[_counter] - 1] * pos_,
                                   self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                    self.prod_directions_[_counter] = 1
            elif self.current_bar_[_counter][2] == self.prod_curr_posn_[_counter][Fields.low_.value]\
                    and self.current_bar_[_counter][1] < self.current_bar_[_counter][0]\
                    and ((self.prod_curr_posn_[_counter][Fields.close_.value] < self.day_bollinger_mean_[_counter] \
                    and self.prev_day_close_[_counter] > self.day_bollinger_mean_[_counter])or \
                    (self.prod_curr_posn_[_counter][Fields.close_.value] < self.day_bollinger_lband_[_counter] \
                    and self.prev_day_close_[_counter] > self.day_bollinger_lband_[_counter])):
                if (curr_macd_ < curr_signal_ and prev_macd_ > prev_signal_) or self.macd_crossover_[_counter] < 0:
                    self.current_cascade_[
                        _counter] = self.current_cascade_[_counter] + 1
                    self.sendOrder('S', _counter, self.cascade_size_[self.current_cascade_[_counter] - 1] * pos_,
                                   self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                    self.prod_directions_[_counter] = -1

        elif self.prod_curr_posn_[_counter][Fields.start_trading.value] and self.prod_curr_posn_[_counter][Fields.cascade_trigger.value] == True \
                and self.current_cascade_[_counter] < self.cascade_num_:

            if self.prod_directions_[_counter] > 0:
                self.sendOrder('B', _counter, self.cascade_size_[self.current_cascade_[_counter]] * pos_,
                               self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                self.current_cascade_[
                    _counter] = self.current_cascade_[_counter] + 1
            # elif curr_macd_ - curr_signal_ < 0 and curr_macd_ - curr_signal_ < prev_macd_ -prev_signal_\
            #   and self.current_cascade_[_counter]  != 0 and self.prod_directions_[_counter] < 0:
            elif self.prod_directions_[_counter] < 0:
                self.sendOrder('S', _counter, self.cascade_size_[self.current_cascade_[_counter]] * pos_,
                               self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                self.current_cascade_[
                    _counter] = self.current_cascade_[_counter] + 1

    # if ticker off day low/high and bar macd crossover then buy/sell
    def tradeBarMACDMeanRevert(self, _prod, _counter):
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])
        # curr_macd_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][1]
        # curr_signal_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][2]
        curr_macd_ = self.current_val_vec_[_counter][1][MACDFields.macd.value]
        curr_signal_ = self.current_val_vec_[_counter][1][MACDFields.signal.value]
        prev_macd_ = self.current_val_vec_[_counter][0][MACDFields.macd.value]
        prev_signal_ = self.current_val_vec_[_counter][0][MACDFields.signal.value]
        # print(curr_macd_,curr_signal_,prev_macd_,prev_signal_)
        if self.current_cascade_[_counter] == 0 and self.prod_curr_posn_[_counter][
                Fields.start_trading.value] and self.current_bar_[_counter][1] != 0:
            # and (not math.isnan(self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]) and \
            # self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]> self.open_vol_avg_[_counter]):
            # print(curr_macd_-curr_signal_,prev_macd_-prev_signal_, self.moment_cc_wt_hl_[_counter],self.prod_curr_posn_[_counter][Fields.low_.value],self.prod_curr_posn_[_counter][Fields.high_.value])
            if curr_macd_ - curr_signal_ < 0 and curr_macd_ - curr_signal_ > prev_macd_ - prev_signal_ and\
                    self.current_bar_[_counter][1] > self.current_bar_[_counter][0] and\
                    self.prod_curr_posn_[_counter][Fields.low_.value] < self.day_bollinger_lband_[_counter] and\
                    self.prod_curr_posn_[_counter][Fields.close_.value] > self.moment_cc_wt_hl_[_counter] + self.prod_curr_posn_[_counter][Fields.low_.value]:
                self.current_cascade_[
                    _counter] = self.current_cascade_[_counter] + 1
                self.sendOrder('B', _counter, self.cascade_size_[self.current_cascade_[_counter] - 1] * pos_,
                               self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                self.prod_directions_[_counter] = 1

            elif curr_macd_ - curr_signal_ > 0 and curr_macd_ - curr_signal_ < prev_macd_ - prev_signal_ and\
                    self.current_bar_[_counter][1] < self.current_bar_[_counter][0] and\
                    self.prod_curr_posn_[_counter][Fields.high_.value] > self.day_bollinger_uband_[_counter] and\
                    self.prod_curr_posn_[_counter][Fields.close_.value] < self.prod_curr_posn_[_counter][Fields.high_.value] - self.moment_cc_wt_hl_[_counter]:
                self.current_cascade_[
                    _counter] = self.current_cascade_[_counter] + 1
                self.sendOrder('S', _counter, self.cascade_size_[self.current_cascade_[_counter] - 1] * pos_,
                               self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                self.prod_directions_[_counter] = -1

        elif self.prod_curr_posn_[_counter][Fields.start_trading.value] and self.prod_curr_posn_[_counter][Fields.cascade_trigger.value] == True \
                and self.current_cascade_[_counter] < self.cascade_num_:

            if self.prod_directions_[_counter] > 0:
                self.sendOrder('B', _counter, self.cascade_size_[self.current_cascade_[_counter]] * pos_,
                               self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                self.current_cascade_[
                    _counter] = self.current_cascade_[_counter] + 1
            # elif curr_macd_ - curr_signal_ < 0 and curr_macd_ - curr_signal_ < prev_macd_ -prev_signal_\
            # 	and self.current_cascade_[_counter]  != 0 and self.prod_directions_[_counter] < 0:
            elif self.prod_directions_[_counter] < 0:
                self.sendOrder('S', _counter, self.cascade_size_[self.current_cascade_[_counter]] * pos_,
                               self.prod_curr_posn_[_counter][Fields.close_.value], self.current_time_)
                self.current_cascade_[
                    _counter] = self.current_cascade_[_counter] + 1

    # trade on bollinger mean crossover
    def tradeBollinger(self, _prod, _counter):
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])

        if self.prod_curr_posn_[_counter][Fields.pos.value] == 0:
            if self.prod_curr_posn_[_counter][Fields.close_.value] > self.day_bollinger_mean_[_counter]\
                    and self.prev_day_close_[_counter] < self.day_bollinger_mean_[_counter]:
                self.sendOrder('B', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)
            elif self.prod_curr_posn_[_counter][Fields.close_.value] < self.day_bollinger_mean_[_counter] \
                    and self.prev_day_close_[_counter] > self.day_bollinger_mean_[_counter]:
                self.sendOrder('S', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)

    # trade on bar bollinger mean crossover
    def tradeBarBollinger(self, _prod, _counter):
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])

        if self.prod_curr_posn_[_counter][Fields.pos.value] == 0:
            if self.prod_curr_posn_[_counter][Fields.close_.value] > self.bollinger_uband_[_counter]:
                self.sendOrder('B', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)
            elif self.prod_curr_posn_[_counter][Fields.close_.value] < self.bollinger_lband_[_counter]:
                self.sendOrder('S', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)

    # Bigger macd signal
    def tradeIncreasingBarMACD(self, _prod, _counter):
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])
        # curr_macd_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][1]
        # curr_signal_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][2]
        curr_macd_ = self.current_val_vec_[_counter][1][MACDFields.macd.value]
        curr_signal_ = self.current_val_vec_[_counter][1][MACDFields.signal.value]
        prev_macd_ = self.current_val_vec_[_counter][0][MACDFields.macd.value]
        prev_signal_ = self.current_val_vec_[_counter][0][MACDFields.signal.value]

        if curr_macd_ > curr_signal_:
            if prev_macd_ < prev_signal_ and self.prod_curr_posn_[_counter][Fields.pos.value] < 0:
                self.squareOff(_counter, self.prod_curr_posn_[
                               _counter][Fields.close_.value])
            elif self.prod_curr_posn_[_counter][Fields.pos.value] == 0 and self.prod_curr_posn_[_counter][Fields.start_trading.value] \
                    and self.bar_relative_strength_index_[_counter] > self.lower_rsi_ and self.prev_bar_rsi_[_counter] < self.lower_rsi_:
                self.sendOrder('B', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)
        elif curr_macd_ < curr_signal_:
            if prev_macd_ > prev_signal_ and self.prod_curr_posn_[_counter][Fields.pos.value] > 0:
                self.squareOff(_counter, self.prod_curr_posn_[
                               _counter][Fields.close_.value])
            elif self.prod_curr_posn_[_counter][Fields.pos.value] == 0 and self.prod_curr_posn_[_counter][Fields.start_trading.value] \
                    and self.bar_relative_strength_index_[_counter] < self.upper_rsi_ and self.prev_bar_rsi_[_counter] > self.upper_rsi_:
                self.sendOrder('S', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)

    def tradeNextBarMACD(self, _prod, _counter):
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])
        # curr_macd_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][1]
        # curr_signal_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][2]
        curr_macd_ = self.current_val_vec_[_counter][1][MACDFields.macd.value]
        curr_signal_ = self.current_val_vec_[_counter][1][MACDFields.signal.value]
        prev_macd_ = self.current_val_vec_[_counter][0][MACDFields.macd.value]
        prev_signal_ = self.current_val_vec_[_counter][0][MACDFields.signal.value]
        # print(curr_macd_,curr_signal_,prev_macd_,prev_signal_,self.prod_curr_posn_[_counter][Fields.close_.value],curr_macd_-curr_signal_)
        if self.prod_curr_posn_[_counter][Fields.pos.value] == 0 and self.prod_curr_posn_[_counter][Fields.start_trading.value]:
            if self.crossover_[
                    _counter] == 1 and curr_macd_ > curr_signal_ and curr_macd_ - curr_signal_ > prev_macd_ - prev_signal_:
                self.sendOrder('B', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)
            elif self.crossover_[
                    _counter] == -1 and curr_macd_ < curr_signal_ and curr_macd_ - curr_signal_ < prev_macd_ - prev_signal_:
                self.sendOrder('S', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)

        if curr_macd_ > curr_signal_ and prev_macd_ < prev_signal_:
            if self.prod_curr_posn_[_counter][Fields.pos.value] < 0:
                self.squareOff(_counter, self.prod_curr_posn_[
                               _counter][Fields.close_.value])
                self.crossover_[_counter] = 0
            elif self.prod_curr_posn_[_counter][Fields.pos.value] == 0 and self.prod_curr_posn_[_counter][Fields.
                                                                                                    start_trading]:
                self.crossover_[_counter] = 1
                # self.sendOrder('B',_counter,pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
        elif curr_macd_ < curr_signal_ and prev_macd_ > prev_signal_:
            if self.prod_curr_posn_[_counter][Fields.pos.value] > 0:
                self.squareOff(_counter, self.prod_curr_posn_[
                               _counter][Fields.close_.value])
                self.crossover_[_counter] = 0
            elif self.prod_curr_posn_[_counter][Fields.pos.value] == 0 and self.prod_curr_posn_[_counter][Fields.
                                                                                                    start_trading]:
                self.crossover_[_counter] = -1
                # self.sendOrder('S',_counter,pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
        else:
            self.crossover_[_counter] = 0

    def tradeBarCCMACD(self, _prod, _counter):
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])
        # curr_macd_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][1]
        # curr_signal_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][2]
        curr_macd_ = self.current_val_vec_[_counter][1][MACDFields.macd.value]
        curr_signal_ = self.current_val_vec_[_counter][1][MACDFields.signal.value]
        prev_macd_ = self.current_val_vec_[_counter][0][MACDFields.macd.value]
        prev_signal_ = self.current_val_vec_[_counter][0][MACDFields.signal.value]

        if curr_macd_ > curr_signal_ and prev_macd_ < prev_signal_:
            if self.prod_curr_posn_[_counter][Fields.pos.value] < 0:
                self.squareOff(_counter, self.prod_curr_posn_[
                               _counter][Fields.close_.value])
            elif self.prod_curr_posn_[_counter][Fields.pos.value] == 0 and \
                    curr_macd_ - curr_signal_ > self.moment_cc_wt_mean_[_counter] + self.moment_cc_wt_stdev_[_counter] \
                    and self.prod_curr_posn_[_counter][Fields.start_trading.value]:
                self.sendOrder('B', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)
        elif curr_macd_ < curr_signal_ and prev_macd_ > prev_signal_:
            if self.prod_curr_posn_[_counter][Fields.pos.value] > 0:
                self.squareOff(_counter, self.prod_curr_posn_[
                               _counter][Fields.close_.value])
            elif self.prod_curr_posn_[_counter][Fields.pos.value] == 0 and \
                    curr_macd_ - curr_signal_ < -1*(self.moment_cc_wt_mean_[_counter] + self.moment_cc_wt_stdev_[_counter]) \
                    and self.prod_curr_posn_[_counter][Fields.start_trading.value]:
                self.sendOrder('S', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)

    def tradeBarMACDSignalFilter(self, _prod, _counter):
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])
        # curr_macd_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][1]
        # curr_signal_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][2]
        # curr_macd_filter_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][3]
        # curr_signal_filter_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][4]
        curr_macd_ = self.current_val_vec_[_counter][1][MACDFields.macd.value]
        curr_signal_ = self.current_val_vec_[_counter][1][MACDFields.signal.value]
        curr_macd_filter_ = self.current_val_vec_[
            _counter][1][MACDFields.macd_filter.value]
        curr_signal_filter_ = self.current_val_vec_[
            _counter][1][MACDFields.signal_filter.value]
        prev_macd_ = self.current_val_vec_[_counter][0][MACDFields.macd.value]
        prev_signal_ = self.current_val_vec_[_counter][0][MACDFields.signal.value]
        prev_macd_filter_ = self.current_val_vec_[
            _counter][0][MACDFields.macd_filter.value]
        prev_signal_filter_ = self.current_val_vec_[
            _counter][0][MACDFields.signal_filter.value]

        if curr_macd_filter_ > curr_signal_filter_ and prev_macd_filter_ < prev_signal_filter_ and self.prod_curr_posn_[
                _counter][Fields.pos.value] < 0:
            self.squareOff(_counter, self.prod_curr_posn_[
                           _counter][Fields.close_.value])
        elif curr_macd_filter_ < curr_signal_filter_ and prev_macd_filter_ > prev_signal_filter_ and self.prod_curr_posn_[
                _counter][Fields.pos.value] > 0:
            self.squareOff(_counter, self.prod_curr_posn_[
                           _counter][Fields.close_.value])

        if curr_macd_ > curr_signal_ and prev_macd_ < prev_signal_:
            if self.prod_curr_posn_[_counter][Fields.pos.value] < 0:
                self.squareOff(_counter, self.prod_curr_posn_[
                               _counter][Fields.close_.value])
            elif self.prod_curr_posn_[_counter][Fields.pos.value] == 0 and \
                    curr_macd_filter_ > curr_signal_filter_ and self.prod_curr_posn_[_counter][Fields.start_trading.value] and curr_macd_filter_ - curr_signal_filter_ > prev_macd_filter_ - prev_signal_filter_:
                self.sendOrder('B', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)
        elif curr_macd_ < curr_signal_ and prev_macd_ > prev_signal_:
            if self.prod_curr_posn_[_counter][Fields.pos.value] > 0:
                self.squareOff(_counter, self.prod_curr_posn_[
                               _counter][Fields.close_.value])
            elif self.prod_curr_posn_[_counter][Fields.pos.value] == 0 and \
                    curr_macd_filter_ < curr_signal_filter_ and self.prod_curr_posn_[_counter][Fields.start_trading.value] and curr_macd_filter_ - curr_signal_filter_ < prev_macd_filter_ - prev_signal_filter_:
                self.sendOrder('S', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)

    def tradeBarMACDReverseSignalFilter(self, _prod, _counter):
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])
        # curr_macd_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][1]
        # curr_signal_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][2]
        # curr_macd_filter_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][3]
        # curr_signal_filter_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][4]
        curr_macd_ = self.current_val_vec_[_counter][1][MACDFields.macd.value]
        curr_signal_ = self.current_val_vec_[_counter][1][MACDFields.signal.value]
        curr_macd_filter_ = self.current_val_vec_[
            _counter][1][MACDFields.macd_filter.value]
        curr_signal_filter_ = self.current_val_vec_[
            _counter][1][MACDFields.signal_filter.value]
        prev_macd_ = self.current_val_vec_[_counter][0][MACDFields.macd.value]
        prev_signal_ = self.current_val_vec_[_counter][0][MACDFields.signal.value]
        prev_macd_filter_ = self.current_val_vec_[
            _counter][0][MACDFields.macd_filter.value]
        prev_signal_filter_ = self.current_val_vec_[
            _counter][0][MACDFields.signal_filter.value]

        if curr_macd_ > curr_signal_ and prev_macd_ < prev_signal_ and self.prod_curr_posn_[_counter][Fields.pos.value] < 0:
            self.squareOff(_counter, self.prod_curr_posn_[
                           _counter][Fields.close_.value])
        elif curr_macd_ < curr_signal_ and prev_macd_ > prev_signal_ and self.prod_curr_posn_[_counter][Fields.pos.value] > 0:
            self.squareOff(_counter, self.prod_curr_posn_[
                           _counter][Fields.close_.value])

        if curr_macd_filter_ > curr_signal_filter_ and prev_macd_filter_ < prev_signal_filter_:
            if self.prod_curr_posn_[_counter][Fields.pos.value] < 0:
                self.squareOff(_counter, self.prod_curr_posn_[
                               _counter][Fields.close_.value])
            elif curr_macd_ - curr_signal_ > prev_macd_ - prev_signal_ and self.prod_curr_posn_[_counter][
                    Fields.start_trading.value]:
                self.sendOrder('B', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)
        elif curr_macd_filter_ < curr_signal_filter_ and prev_macd_filter_ > prev_signal_filter_:
            if self.prod_curr_posn_[_counter][Fields.pos.value] > 0:
                self.squareOff(_counter, self.prod_curr_posn_[
                               _counter][Fields.close_.value])
            elif curr_macd_ - curr_signal_ < prev_macd_ - prev_signal_ and self.prod_curr_posn_[_counter][
                    Fields.start_trading.value]:
                self.sendOrder('S', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)

    def tradeBarMACDCascade(self, _prod, _counter):
        pos_ = math.floor(self.max_exposure_ /
                          self.prod_curr_posn_[_counter][Fields.close_.value])
        # curr_macd_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][1]
        # curr_signal_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][2]
        curr_macd_ = self.current_val_vec_[_counter][1][MACDFields.macd.value]
        curr_signal_ = self.current_val_vec_[_counter][1][MACDFields.signal.value]
        prev_macd_ = self.current_val_vec_[_counter][0][MACDFields.macd.value]
        prev_signal_ = self.current_val_vec_[_counter][0][MACDFields.signal.value]
        net_alpha_ = curr_macd_ - curr_signal_
        net_prev_alpha_ = prev_macd_ - prev_signal_

        if curr_macd_ > curr_signal_ and prev_macd_ < prev_signal_:
            if self.prod_curr_posn_[_counter][Fields.pos.value] < 0:
                self.squareOff(_counter, self.prod_curr_posn_[
                               _counter][Fields.close_.value])
            else:
                self.sendOrder('B', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)
        elif curr_macd_ < curr_signal_ and prev_macd_ > prev_signal_:
            if self.prod_curr_posn_[_counter][Fields.pos.value] > 0:
                self.squareOff(_counter, self.prod_curr_posn_[
                               _counter][Fields.close_.value])
            else:
                self.sendOrder('S', _counter, pos_, self.prod_curr_posn_[
                               _counter][Fields.close_.value], self.current_time_)

        # CASCADE
        if curr_macd_ > curr_signal_ and prev_macd_ > prev_signal_ and curr_macd_ - curr_signal_ > prev_macd_ - prev_signal_\
                and self.prod_curr_posn_[_counter][Fields.pos.value] > 0:
            print("cascade")
            self.sendOrder('B', _counter, pos_, self.prod_curr_posn_[
                           _counter][Fields.close_.value], self.current_time_)

        elif curr_macd_ < curr_signal_ and prev_macd_ < prev_signal_ and curr_macd_ - curr_signal_ < prev_macd_ - prev_signal_\
                and self.prod_curr_posn_[_counter][Fields.pos.value] < 0:
            print("cascade")
            self.sendOrder('S', _counter, pos_, self.prod_curr_posn_[
                           _counter][Fields.close_.value], self.current_time_)

    # def tradeBarMACDCloseFilter(self,_prod,_counter):
    # 	pos_ = math.floor(self.max_exposure_/self.prod_curr_posn_[_counter][Fields.close_.value])
    # 	curr_macd_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][1]
    # 	curr_signal_ = self.bar_macd_px_[_counter][len(self.bar_macd_px_[_counter])-1][2]
    # 	prev_macd_ = self.current_val_vec_[_counter][0][MACDFields.macd.value]
    # 	prev_signal_ = self.current_val_vec_[_counter][0][MACDFields.signal.value]
    # 	net_alpha_ = curr_macd_ - curr_signal_
    # 	net_prev_alpha_ = prev_macd_ - prev_signal_

    # 	if curr_macd_ > curr_signal_ and prev_macd_ < prev_signal_:
    # 		if self.prod_curr_posn_[_counter][Fields.pos.value] < 0:
    # 			self.squareOff(_counter,self.prod_curr_posn_[_counter][Fields.close_.value])
    # 		elif self.prod_curr_posn_[_counter][Fields.close_.value] >= self.prev_bar_close_[_counter]:
    # 			self.sendOrder('B',_counter,pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
    # 	elif curr_macd_ < curr_signal_ and prev_macd_ > prev_signal_:
    # 		if self.prod_curr_posn_[_counter][Fields.pos.value] > 0:
    # 			self.squareOff(_counter,self.prod_curr_posn_[_counter][Fields.close_.value])
    # 		elif self.prod_curr_posn_[_counter][Fields.close_.value] <= self.prev_bar_close_[_counter]:
    # 			self.sendOrder('S',_counter,pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)

    # print(prev_macd_,prev_signal_,curr_macd_,curr_signal_,self.current_val_vec_[_counter][1][MACDFields.macd.value],self.current_val_vec_[_counter][1][MACDFields.signal.value])

    # def tradeMACD(self,_prod,_counter):
    # 	pos_ = math.floor(self.max_exposure_/self.prod_curr_posn_[_counter][Fields.close_.value])

    # 	current_cc_ = self.prod_curr_posn_[_counter][Fields.close_.value] - self.prev_day_close_[_counter]
    # 	current_row_ = self.macd_px_[_counter].loc[len(self.macd_px_[_counter])-1]
    # 	if len(self.macd_px_[_counter]) >=2:
    # 		prev_row_ = self.macd_px_[_counter].loc[len(self.macd_px_[_counter])-2]
    # 		prev_macd_ = prev_row_["macd"]
    # 		prev_signal_ = prev_row_["signal"]
    # 	else:
    # 		prev_macd_ = 0
    # 		prev_signal_ = 0
    # 	macd_ = current_row_["macd"]
    # 	signal_ = current_row_["signal"]
    # 	# print(_prod,current_cc_,macd_,signal_,prev_macd_,prev_signal_)
    # 	# if macd_ > 0 and prev_macd_ < 0:
    # 	# if current_cc_ > 0 and macd_ > signal_ and prev_macd_ < prev_signal_ and macd_ > 0:
    # 	if macd_ > signal_ and prev_macd_ < prev_signal_:
    # 		self.sendOrder('B',_counter,pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
    # 	# elif macd_ < 0 and prev_macd_ > 0:
    # 	# elif current_cc_ < 0 and macd_ < signal_ and prev_macd_ > prev_signal_ and macd_ < 0:
    # 	elif macd_ < signal_ and prev_macd_ > prev_signal_:
    # 		self.sendOrder('S',_counter,pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)

    def updateTrailingSL(self, _index):
        # index_ = str(_prod+"_"+str(self.current_time_))
        prod_ = self.product_list_[_index]
        time_factor_ = self.time_wt_ * (self.current_time_ - self.om_.started_at_) / (
            self.om_.ended_at_ - self.om_.started_at_)
        pos_factor_ = (self.pos_wt_ * self.prod_curr_posn_[_index][Fields.ltp.value] / self.max_exposure_) * abs(
            self.prod_curr_posn_[_index][Fields.pos.value])
        # self.decay_factor_ = -0.0693
        # print(self.trailing_pt_,self.moment_cc_wt_stdev_[_index])
        if self.prod_curr_posn_[_index][Fields.pos.value] > 0:
            # desired_price_ = self.prod_curr_posn_[_index][Fields.ltp.value]*(1 -self.price_delta_*pow(math.e,-1*(time_factor_+pos_factor_)))
            # desired_price_ = self.prod_curr_posn_[_index][Fields.ltp.value]*(1-self.trailing_pt_)
            # desired_price_ = self.prod_curr_posn_[_index][Fields.ltp.value]-self.trailing_pt_*self.moment_cc_wt_stdev_[_index]*(self.decay_factor_*self.cascade_num_+1-self.current_cascade_[_index])/(self.decay_factor_*self.cascade_num_)#*pow(math.e,-1*(self.current_cascade_[_index]-1))
            desired_price_ = self.prod_curr_posn_[_index][
                Fields.ltp.value] - self.trailing_pt_ * self.moment_cc_wt_stdev_[_index] * pow(
                    math.e, self.decay_factor_ * (self.current_cascade_[_index] - 1))
            # print(self.prod_curr_posn_[_index][Fields.ltp.value],desired_price_,self.prod_curr_posn_[_index][Fields.support_px.value],time_factor_,pos_factor_)
            self.prod_curr_posn_[_index][Fields.support_px.value] = max(self.prod_curr_posn_[_index][Fields.support_px.value],
                                                                  desired_price_)
        elif self.prod_curr_posn_[_index][Fields.pos.value] < 0:
            # desired_price_ = self.prod_curr_posn_[_index][Fields.ltp.value]*(1 +self.price_delta_*pow(math.e,-1*(time_factor_+pos_factor_)))
            # desired_price_ = self.prod_curr_posn_[_index][Fields.ltp.value]*(1+self.trailing_pt_)
            # desired_price_ = self.prod_curr_posn_[_index][Fields.ltp.value]+self.trailing_pt_*self.moment_cc_wt_stdev_[_index]*(self.decay_factor_*self.cascade_num_+1-self.current_cascade_[_index])/(self.decay_factor_*self.cascade_num_)#*pow(math.e,-1*(self.current_cascade_[_index]-1))
            desired_price_ = self.prod_curr_posn_[_index][
                Fields.ltp.value] + self.trailing_pt_ * self.moment_cc_wt_stdev_[_index] * pow(
                    math.e, self.decay_factor_ * (self.current_cascade_[_index] - 1))
            # print(self.prod_curr_posn_[_index][Fields.ltp.value],desired_price_,self.prod_curr_posn_[_index][Fields.resist_px.value],time_factor_,pos_factor_)
            self.prod_curr_posn_[_index][Fields.resist_px.value] = min(self.prod_curr_posn_[_index][Fields.resist_px.value],
                                                                 desired_price_)
        # self.prod_curr_posn_.loc[_prod,"SL"] = 0.5*self.prod_curr_posn_.loc[_prod]["Hard_SL"]*pow(math.e,-1*(time_factor_+pos_factor_))
        # print(self.prod_curr_posn_[_index][Fields.support_px.value],self.prod_curr_posn_[_index][Fields.resist_px.value])

    def onExec(self, _index, _size, _price, _cost, _time):
        StratFramework.onExec(self, _index, _size, _price, _cost, _time)
        prod_row_ = self.prod_curr_posn_[_index]
        prod_row_[Fields.cascade_trigger.value] = False
        self.updateTrailingSL(_index)

    # def checkSL(self,_ind,_px):
    #     StratFramework.checkSL(self,_ind,_px)
    #     prod_ = self.product_list_[_ind]
    #     if self.prod_curr_posn_[_ind][Fields.pos.value] > 0 and self.current_volume_[_ind] > self.opening_bar_volume_[_ind][(self.open_vol_count_[_ind]+self.open_vol_days_-1)%self.open_vol_days_]:#\
    #         #and self.current_cascade_[_ind] < self.vol_sqoff_cascade_:
    #     # if self.prod_curr_posn_[_ind][Fields.pos.value] > 0 and self.current_volume_[_ind] > self.opening_bar_volume_[_ind][(self.open_vol_count_[_ind]+self.open_vol_days_-1)%self.open_vol_days_]:# or self.current_obv_[_ind] < 0.5*self.entry_obv_[_ind]):
    #         print("SL HIT(support_px) at " + str(self.current_time_) + " "  + str(self.prod_curr_posn_[_ind][Fields.support_px.value]) + " Strat" + str(self.strat_id_) + str(_px) + " for " + prod_ + " " + str(pow(math.e,self.decay_factor_*(self.current_cascade_[_ind]-1))))
    #         self.prod_curr_posn_[_ind][Fields.sqoff_on.value] =  True
    #         # self.prod_curr_posn_[_ind][Fields.start_trading.value] =  False
    #         self.squareOff(_ind,self.prod_curr_posn_[_ind][Fields.close_.value])
    #         self.current_cascade_[_ind] = 0
    #         self.prod_curr_posn_[_ind][Fields.support_px.value] = 0
    #         # self.reentry_px_[_ind] = 0
    #         # self.sqoff_px_[_ind] = self.prod_curr_posn_[_ind][Fields.close_.value]
    #     elif self.prod_curr_posn_[_ind][Fields.pos.value] < 0 and self.current_volume_[_ind] > self.opening_bar_volume_[_ind][(self.open_vol_count_[_ind]+self.open_vol_days_-1)%self.open_vol_days_]:#\
    #         # and self.current_cascade_[_ind] < self.vol_sqoff_cascade_:
    #     # elif self.prod_curr_posn_[_ind][Fields.pos.value] < 0 and self.current_volume_[_ind] > self.opening_bar_volume_[_ind][(self.open_vol_count_[_ind]+self.open_vol_days_-1)%self.open_vol_days_]:# or self.current_obv_[_ind] > 0.5*self.entry_obv_[_ind]): # and prod_ != "NIFTY":
    #         print("SL HIT(resist_px) at " + str(self.current_time_) + " "  + str(self.prod_curr_posn_[_ind][Fields.resist_px.value]) + " Strat" + str(self.strat_id_) +  str(_px) + " for " + prod_+ " " + str(pow(math.e,self.decay_factor_*(self.current_cascade_[_ind]-1))))
    #         self.prod_curr_posn_[_ind][Fields.sqoff_on.value] =  True
    #         # self.prod_curr_posn_[_ind][Fields.start_trading.value] =  False
    #         self.squareOff(_ind,self.prod_curr_posn_[_ind][Fields.close_.value])
    #         self.current_cascade_[_ind] = 0
    #         self.prod_curr_posn_[_ind][Fields.resist_px.value] = 0
            # self.reentry_px_[_ind] = 0
            # self.sqoff_px_[_ind] = self.prod_curr_posn_[_ind][Fields.close_.value]