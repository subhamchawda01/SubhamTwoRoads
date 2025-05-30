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


class MultipleMATheo(StratFramework):
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
        self.nearest_big_ = [0] * len(self.product_list_)
        self.nearest_small_ = [0] * len(self.product_list_)
        self.current_vol_ = [0] * len(self.product_list_)
        self.prev_vol_ = [0] * len(self.product_list_)
        # self.sema_ = [0]*len(self.product_list_)
        # self.lema_ = [0]*len(self.product_list_)
        # self.signal_ = [0]*len(self.product_list_)
        # self.macd_= [0]*len(self.product_list_)
        self.last_bar_update_ = 0
        # self.day_rsi_ = [0]*len(self.product_list_)
        self.lt_std_ = [0] * len(self.product_list_)
        self.open_vol_avg_ = [np.NaN] * len(self.product_list_)
        self.filter_std_list_ = []
        self.day_ma_lkbk_ = [30,50,100,200]
        self.day_moving_avg_ = np.zeros(len(self.product_list_) * len(self.day_ma_lkbk_)).reshape(len(self.product_list_), len(self.day_ma_lkbk_))
        self.hour_ma_lkbk_ = [30,50,100,200]
        self.hour_moving_avg_ = np.zeros(len(self.product_list_) * len(self.hour_ma_lkbk_)).reshape(len(self.product_list_), len(self.hour_ma_lkbk_))
        self.hourly_data_  = []
        self.next_hourly_update_ =  [0] * len(self.product_list_)
        self.max_size_hour_ma_ = 200
        self.last_hourly_update_index_ =  [199] * len(self.product_list_)
        self.support_px =  np.zeros(2*len(self.product_list_) * len(self.day_ma_lkbk_)).reshape(len(self.product_list_), 2*len(self.day_ma_lkbk_))
        self.resist_px =  np.zeros(2*len(self.product_list_) * len(self.day_ma_lkbk_)).reshape(len(self.product_list_), 2*len(self.day_ma_lkbk_))
        # self.crossover_lkbk_ = np.zeros(len(self.product_list_) * len(self.day_ma_lkbk_)).reshape(len(self.product_list_), len(self.day_ma_lkbk_))
        self.crossover_lkbk_ = np.full((len(self.product_list_),2*len(self.day_ma_lkbk_)),1000)
        self.take_pos_ = np.zeros(2*len(self.product_list_) * len(self.day_ma_lkbk_)).reshape(len(self.product_list_), 2*len(self.day_ma_lkbk_))
        self.allowed_pos_ = np.zeros(2*len(self.product_list_) * len(self.day_ma_lkbk_)).reshape(len(self.product_list_), 2*len(self.day_ma_lkbk_))
        self.current_cascade_ = np.zeros(2*len(self.product_list_) * len(self.day_ma_lkbk_)).reshape(len(self.product_list_), 2*len(self.day_ma_lkbk_))
        self.exec_px_ = np.zeros(2*len(self.product_list_) * len(self.day_ma_lkbk_)).reshape(len(self.product_list_), 2*len(self.day_ma_lkbk_))
        self.current_cascade_ = self.current_cascade_.astype(int)
        self.day_ma_score_ = [0] * len(self.product_list_)
        self.first_close_ = [0] * len(self.product_list_)
        self.current_low_ = [0] * len(self.product_list_)
        self.current_high_ = [0] * len(self.product_list_)
        self.prev_low_ = [0] * len(self.product_list_)
        self.prev_high_ = [0] * len(self.product_list_)
        self.day_ma_personal_score_ = np.zeros(len(self.product_list_) * len(self.day_ma_lkbk_)).reshape(len(self.product_list_), len(self.day_ma_lkbk_))
        self.prev_day_moving_avg_ = np.zeros(len(self.product_list_) * len(self.day_ma_lkbk_)).reshape(len(self.product_list_), len(self.day_ma_lkbk_))
        self.prev_day_low_ = [0]*len(self.product_list_)
        self.prev_day_high_ = [0]*len(self.product_list_)
        self.last_di_update_ = [0]*len(self.product_list_)
        self.last_crossover_ = np.zeros(len(self.product_list_) * len(self.day_ma_lkbk_)).reshape(len(self.product_list_), len(self.day_ma_lkbk_))
        # self.bar_relative_strength_index_ = [0]*len(self.product_list_)
        # self.prev_bar_rsi_ = [0]*len(self.product_list_)
        # self.bar_avg_gain_ = [0]*len(self.product_list_)
        # self.bar_avg_loss_ = [0]*len(self.product_list_)
        # self.day_rsi_arr_ = []
        # self.old_rsi_ = [0]*len(self.product_list_)

        self.cascade_size_ = []
        for prod_ in self.product_list_:
            self.training_data_.append(pd.DataFrame(
                columns=["datetime", "open", "close", "low", "high", "vol", "c-c"]))
            self.training_data_[len(self.training_data_)-1][["datetime","open", "close", "low", "high", "vol", "c-c"]] = \
                self.training_data_[len(
                    self.training_data_)-1][["datetime","open", "close", "low", "high", "vol", "c-c"]].astype(float)
            self.hourly_data_.append(pd.DataFrame(columns=["datetime","close"]))
            self.hourly_data_[len(self.hourly_data_)-1][["close"]] = self.hourly_data_[len(self.hourly_data_)-1][["close"]].astype(float)

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
                #   self.cascade_size_[1] = float(row[2])
                # if row[0] == "THIRD_CASCADE":
                #   self.cascade_size_[2] = float(row[2])
                # if row[0] == "FOURTH_CASCADE":
                #   self.cascade_size_[3] = float(row[2])
                # if row[0] == "FIFTH_CASCADE":
                #   self.cascade_size_[4] = float(row[2])
                # if row[0] == "SIXTH_CASCADE":
                #   self.cascade_size_[5] = float(row[2])
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
                if row[0] == "GAMMA":
                    self.moment_cc_gamma_ = float(row[2])
                if row[0] == "MA_ENTRY_PT":
                    self.ma_entry_pt_ = 1 + float(row[2])/100
                if row[0] == "MA_EXIT_PT":
                    self.ma_exit_pt_ = 1 + float(row[2])/100
                if row[0] == "HMA_ENTRY_PT":
                    self.hma_entry_pt_ = 1 + float(row[2])/100
                if row[0] == "HMA_EXIT_PT":
                    self.hma_exit_pt_ = 1 + float(row[2])/100
                if row[0] == "CROSSOVER_LIMIT":
                    self.crossover_limit_ = int(row[2])
                if row[0] == "PREV_DELTA":
                    self.prev_delta_ = float(row[2])/100
                if row[0] == "ADR_DAYS":
                    self.adr_days_ = int(row[2])
                if row[0] == "STOCH_DAYS":
                    self.stoch_days_ = int(row[2])
                if row[0] == "STOCH_BARS":
                    self.stoch_bars_ = int(row[2])

        self.long_term_vol_ = np.zeros(self.lt_days_ * len(self.product_list_)).reshape(
            len(self.product_list_), self.lt_days_)
        self.opening_bar_volume_ = np.zeros(self.open_vol_days_ * len(self.product_list_)).reshape(
            len(self.product_list_), self.open_vol_days_)
        # self.long_term_vol_[:] = np.NaN
        self.opening_bar_volume_[:] = np.NaN
        self.lt_count_ = [0] * len(self.product_list_)
        self.open_vol_count_ = [0] * len(self.product_list_)

        self.plus_di_ = np.zeros(self.adr_days_ * len(self.product_list_)).reshape(len(self.product_list_), self.adr_days_)
        self.neg_di_ = np.zeros(self.adr_days_ * len(self.product_list_)).reshape(len(self.product_list_), self.adr_days_)        
        self.true_range_ = np.zeros(self.adr_days_ * len(self.product_list_)).reshape(len(self.product_list_), self.adr_days_)        
        self.smoothed_true_range_  = [0] * len(self.product_list_)
        self.smoothed_plus_di_  = [0] * len(self.product_list_)
        self.smoothed_neg_di_  = [0] * len(self.product_list_)
        self.adx_ = [0]*len(self.product_list_)
        self.stochastic_oclh_ = np.zeros(4*self.stoch_days_ * len(self.product_list_)).reshape(len(self.product_list_), self.stoch_days_,4)        
        self.stochastic_ind_ = [0]*len(self.product_list_)
        self.last_stoch_update_ = [0]*len(self.product_list_)
        self.rsi_ = [0]*len(self.product_list_)
        self.gain_ = np.zeros(self.stoch_days_ * len(self.product_list_)).reshape(len(self.product_list_), self.stoch_days_)     
        self.loss_ = np.zeros(self.stoch_days_ * len(self.product_list_)).reshape(len(self.product_list_), self.stoch_days_)     

        self.stochastic_bar_oclh_ = np.zeros(4*self.stoch_bars_ * len(self.product_list_)).reshape(len(self.product_list_), self.stoch_bars_,4)        
        self.stochastic_bar_ind_ = [0]*len(self.product_list_)
        self.last_stoch_bar_update_ = [0]*len(self.product_list_)
        self.ma_diff_ = [0]*len(self.product_list_)
        self.ma_thresh_ = 0
        self.day_min_ = [-1]*len(self.product_list_)     
        self.day_max_ = [-1]*len(self.product_list_)  
        self.volume_condn_ = np.zeros((len(self.product_list_), len(self.day_ma_lkbk_)),dtype=bool)
        
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
        special = datetime(2020,2,1,0,0)
        if end_ >= special.date() and start_ <= special.date():
            self.working_days_.append(special)
            self.working_days_.sort()

    # Called before the first trading day in date range.Initializes dataframe with values
    def getTrainingBar(self, _method):
        counter_ = 0
        for index_ in range(0, len(self.product_list_)):
            prod_ = self.product_list_[index_]
            flag_ = 0
            start_ = self.train_start_time_.split('_')
            end_ = self.train_end_time_.split('_')
            day_count_ = 0
            for date_ in self.working_days_:
                day_start_epoch_ = int(date_.replace(
                    hour=int(start_[0]), minute=int(start_[1])).strftime('%s'))
                day_end_epoch_ = int(date_.replace(
                    hour=int(end_[0]), minute=int(end_[1])).strftime('%s'))
                [o, c, l, h, v] = _method(
                    day_start_epoch_, day_end_epoch_, counter_)
                if [o, c, l, h] != [0, 0, 0, 0]:
                    flag_ = 1

                    # print("close",day_start_epoch_,c)
                    # if len(self.day_rsi_arr_[counter_]) <= self.rsi_period_:
                    # 	self.day_rsi_arr_[counter_].loc[len(self.day_rsi_arr_[counter_])] = c
                    # else:
                    # 	self.day_rsi_arr_[counter_].set_value(self.old_rsi_[counter_],"close", c)
                    # 	self.old_rsi_[counter_] = self.old_rsi_[counter_] + 1

                    # 	rsi_diff_ = self.day_rsi_arr_[counter_]["close"].diff()
                    # 	avg_gain_ = rsi_diff_[rsi_diff_> 0].mean()
                    # 	avg_loss_ = -1*rsi_diff_[rsi_diff_ < 0].mean()
                    # self.day_rsi_[counter_] = 100 - 100/(1 + (avg_gain_/avg_loss_))
                    
                    if self.prev_day_close_[index_]:
                        self.training_data_[counter_].loc[len(self.training_data_[counter_])] = [
                            day_start_epoch_, o, c, l, h, v,
                            abs(c - self.prev_day_close_[index_])
                        ]
                        if day_count_ >= self.adr_days_:
                            # print(day_count_,self.adr_days_,self.last_di_update_[index_])
                            self.smoothed_true_range_[index_] = -1*np.mean(self.true_range_[index_])
                            self.smoothed_plus_di_[index_] = -1*np.mean(self.plus_di_[index_])
                            self.smoothed_neg_di_[index_] = -1*np.mean(self.neg_di_[index_])
                        else:
                            self.smoothed_true_range_[index_] = 0
                            self.smoothed_plus_di_[index_] = 0
                            self.smoothed_neg_di_[index_] = 0
                        # print(self.last_di_update_[index_],self.true_range_[index_][self.last_di_update_[index_]],h-l,np.abs(h-self.prev_day_close_[index_]))
                        self.true_range_[index_][self.last_di_update_[index_]] = np.maximum(h-l,np.abs(h-self.prev_day_close_[index_]))
                        self.true_range_[index_][self.last_di_update_[index_]] = np.maximum(self.true_range_[index_][self.last_di_update_[index_]],np.abs(l-self.prev_day_close_[index_]))

                        self.stochastic_oclh_[index_][self.last_stoch_update_[index_]] = [o,c,l,h]
                        self.stochastic_ind_[index_] = 100*(c-self.stochastic_oclh_[index_][:,2].min())/(self.stochastic_oclh_[index_][:,3].max() - self.stochastic_oclh_[index_][:,2].min())
                        if c-o >= 0:
                            self.gain_[index_][self.last_stoch_update_[index_]] = c-o
                            self.loss_[index_][self.last_stoch_update_[index_]] = 0
                        else:
                            self.loss_[index_][self.last_stoch_update_[index_]] = o-c
                            self.gain_[index_][self.last_stoch_update_[index_]] = 0
                        
                        a = np.sum(self.gain_[index_])/len(np.nonzero(self.gain_[index_])[0])
                        b = np.sum(self.loss_[index_])/len(np.nonzero(self.loss_[index_])[0])
                        self.rsi_[index_] = 100 - 100/(1 + a/b)
                        # print(o,c,l,h)
                        # print(self.gain_[index_],self.loss_[index_],self.rsi_[index_],a,b)
                        
                        if h - self.prev_day_high_[index_] > 0 and h - self.prev_day_high_[index_] > self.prev_day_low_[index_] -l:
                            self.plus_di_[index_][self.last_di_update_[index_]] = h - self.prev_day_high_[index_]
                        else:
                            self.plus_di_[index_][self.last_di_update_[index_]] = 0

                        if self.prev_day_low_[index_] -l > 0 and h - self.prev_day_high_[index_] < self.prev_day_low_[index_] -l:
                            self.neg_di_[index_][self.last_di_update_[index_]] = self.prev_day_low_[index_] -l
                        else:
                            self.neg_di_[index_][self.last_di_update_[index_]] = 0

                        self.smoothed_true_range_[index_] = self.smoothed_true_range_[index_] + self.true_range_[index_][self.last_di_update_[index_]] + np.sum(self.true_range_[index_])
                        self.smoothed_plus_di_[index_] = self.smoothed_plus_di_[index_] + self.plus_di_[index_][self.last_di_update_[index_]] + np.sum(self.plus_di_[index_])
                        self.smoothed_neg_di_[index_] = self.smoothed_neg_di_[index_] + self.neg_di_[index_][self.last_di_update_[index_]] + np.sum(self.neg_di_[index_])

                        self.smoothed_plus_di_[index_] = 100*self.smoothed_plus_di_[index_]/self.smoothed_true_range_[index_]
                        self.smoothed_neg_di_[index_] = 100*self.smoothed_neg_di_[index_]/self.smoothed_true_range_[index_]

                        # print("adx ",self.true_range_[index_],self.plus_di_[index_],self.neg_di_[index_],self.last_di_update_[index_])
                        # print(o,c,l,h,self.prev_day_close_[index_],self.prev_day_low_[index_],self.prev_day_high_[index_])
                        self.last_di_update_[index_] = (self.last_di_update_[index_] +1)%self.adr_days_
                        self.last_stoch_update_[index_] = (self.last_stoch_update_[index_] +1)%self.stoch_days_

                        self.long_term_vol_[index_][self.lt_count_[index_] % self.lt_days_] = v*self.prev_day_close_[index_]#*self.prev_day_close_[index_]/(h-l+0.0000001)
                        self.long_term_vol_[counter_][self.lt_count_[counter_] % self.lt_days_] = v*(np.max(self.day_moving_avg_[index_]) - np.min(self.day_moving_avg_[index_]))*(np.max(self.day_moving_avg_[index_]) - np.min(self.day_moving_avg_[index_]))/(h-l)
                        self.lt_count_[counter_] = self.lt_count_[counter_] + 1

                        if self.smoothed_plus_di_[index_] and self.smoothed_neg_di_[index_]:
                            self.adx_[index_] = (self.adx_[index_]*(self.adr_days_-1) + np.abs(self.smoothed_plus_di_[index_] - self.smoothed_neg_di_[index_])\
                                                                                        /(self.smoothed_plus_di_[index_] + self.smoothed_neg_di_[index_]))/self.adr_days_
                            # print("adx",self.adx_[index_],self.smoothed_plus_di_[index_],self.smoothed_neg_di_[index_])
                        day_count_ = day_count_ + 1
                    else:
                        self.training_data_[counter_].loc[len(
                            self.training_data_[counter_])] = [day_start_epoch_, o, c, l, h, v, np.NaN]


                    for i in range(0,len(self.day_ma_lkbk_)):
                        if self.day_moving_avg_[counter_][i] !=0 and self.prev_day_close_[counter_] !=0:
                            if (self.prev_day_close_[counter_] - self.day_moving_avg_[counter_][i])*(c - self.day_moving_avg_[counter_][i]) < 0:
                                self.last_crossover_[counter_][i] = 0
                            else:
                                self.last_crossover_[counter_][i] = self.last_crossover_[counter_][i] + 1

                        self.prev_day_moving_avg_[counter_][i] = self.day_moving_avg_[counter_][i]
                        if len(self.training_data_[counter_]) == self.day_ma_lkbk_[i]:
                            self.day_moving_avg_[counter_][i] = self.training_data_[counter_]["close"].mean()
                        elif len(self.training_data_[counter_]) > self.day_ma_lkbk_[i]:
                            self.day_moving_avg_[counter_][i] = (self.day_ma_lkbk_[i]*self.day_moving_avg_[counter_][i] + c -\
                                        self.training_data_[counter_].get_value((len(self.training_data_[counter_])-1 - self.day_ma_lkbk_[i] + len(self.training_data_[counter_]))%len(self.training_data_[counter_])\
                                                                , "close") )/self.day_ma_lkbk_[i]
                            # print("hi there",i,self.day_moving_avg_[counter_][i], self.training_data_[counter_][self.training_data_[counter_]["datetime"] > self.training_data_[counter_].get_value((len(self.training_data_[counter_])-1 - self.day_ma_lkbk_[i] + len(self.training_data_[counter_]))%len(self.training_data_[counter_])\
                            #                                     , "datetime")]["close"].mean(),c,"init")
                    self.prev_day_close_[index_] = c
                    self.prev_day_low_[index_] = l
                    self.prev_day_high_[index_] = h
                    # print(len(self.training_data_[counter_]), len(self.working_days_))

            if not flag_:
                self.prod_curr_posn_[index_][Fields.start_trading.value] = True
            counter_ = counter_ + 1
            self.oldest_bar_.append(0)

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


                if len(self.working_days_) > len(self.training_data_[counter_]):
                    self.training_data_[counter_].loc[len(self.training_data_[counter_])] = [
                        self.om_.started_at_, o, c, l, h, v,
                        abs(c - self.prev_day_close_[index_])
                    ]
                    for i in range(0,len(self.day_ma_lkbk_)):
                        self.prev_day_moving_avg_[counter_][i] = self.day_moving_avg_[counter_][i]
                        if len(self.training_data_[counter_]) == self.day_ma_lkbk_[i]:
                            self.day_moving_avg_[counter_][i] = self.training_data_[counter_]["close"].mean()
                            # self.day_moving_avg_[counter_][i] = self.training_data_[counter_]["close"].ewm(span=self.day_ma_lkbk_[i]).mean().loc[len(self.training_data_[counter_]["close"]) -1]

                        elif len(self.training_data_[counter_]) > self.day_ma_lkbk_[i]:
                            self.day_moving_avg_[counter_][i] = (self.day_ma_lkbk_[i]*self.day_moving_avg_[counter_][i] + c -\
                                        self.training_data_[counter_].get_value((len(self.training_data_[counter_])-1 - self.day_ma_lkbk_[i] + len(self.training_data_[counter_]))%len(self.training_data_[counter_])\
                                                                , "close") )/self.day_ma_lkbk_[i]
                            # self.day_moving_avg_[counter_][i] = 2*c/(self.day_ma_lkbk_[i] + 1) + self.day_moving_avg_[counter_][i]*\
                            #                                     (1 - (2/(self.day_moving_avg_[counter_][i] + 1)))
                    self.day_ma_score_[counter_] = 0
                    self.day_ma_personal_score_[counter_] = [0]*len(self.day_ma_lkbk_)
                    for i in range(0,len(self.day_ma_lkbk_)):
                        for j in range(i+1,len(self.day_ma_lkbk_)):
                            if self.day_moving_avg_[counter_][i] < self.day_moving_avg_[counter_][j]:
                                self.day_ma_score_[counter_] = self.day_ma_score_[counter_] +1
                                # self.day_ma_personal_score_[counter_][i] = self.day_ma_personal_score_[counter_][i] -1
                                self.day_ma_personal_score_[counter_][j] = self.day_ma_personal_score_[counter_][j] +1
                            
                            elif self.day_moving_avg_[counter_][i] > self.day_moving_avg_[counter_][j]:
                                self.day_ma_score_[counter_] = self.day_ma_score_[counter_] -1
                                self.day_ma_personal_score_[counter_][i] = self.day_ma_personal_score_[counter_][i] +1
                                # self.day_ma_personal_score_[counter_][j] = self.day_ma_personal_score_[counter_][j] -1

                            
                else:
                    # arr = []
                    for i in range(0,len(self.day_ma_lkbk_)):
                        if self.day_moving_avg_[counter_][i] !=0 and self.prev_day_close_[counter_] !=0:
                            if (self.prev_day_close_[counter_] - self.day_moving_avg_[counter_][i])*(c - self.day_moving_avg_[counter_][i]) < 0:
                                self.last_crossover_[counter_][i] = 0
                            else:
                                self.last_crossover_[counter_][i] = self.last_crossover_[counter_][i] + 1
                        # print("values",self.day_moving_avg_[counter_][i] , c- self.training_data_[counter_].get_value((self.oldest_bar_[counter_] - self.day_ma_lkbk_[i] + \
                        #         len(self.training_data_[counter_]))%len(self.training_data_[counter_]), "close"))
                        self.prev_day_moving_avg_[counter_][i] = self.day_moving_avg_[counter_][i]
                        self.day_moving_avg_[counter_][i] = (self.day_ma_lkbk_[i]*self.day_moving_avg_[counter_][i] + c -\
                                        self.training_data_[counter_].get_value((self.oldest_bar_[counter_] - self.day_ma_lkbk_[i] + len(self.training_data_[counter_]))%len(self.training_data_[counter_])\
                                                                , "close") )/self.day_ma_lkbk_[i]
                        # self.day_moving_avg_[counter_][i] =  2*c/(self.day_ma_lkbk_[i] + 1) + self.day_moving_avg_[counter_][i]*\
                        #                                         (1 - (2/(self.day_moving_avg_[counter_][i] + 1)))
                        
                        # pd.options.display.float_format = '{:.0f}'.format
                    self.day_ma_score_[counter_] = 0
                    self.day_ma_personal_score_[counter_] = [0]*len(self.day_ma_lkbk_)
                    for i in range(0,len(self.day_ma_lkbk_)):
                        for j in range(i+1,len(self.day_ma_lkbk_)):
                            if self.day_moving_avg_[counter_][i] < self.day_moving_avg_[counter_][j]:
                                self.day_ma_score_[counter_] = self.day_ma_score_[counter_] +1
                                # self.day_ma_personal_score_[counter_][i] = self.day_ma_personal_score_[counter_][i] -1
                                self.day_ma_personal_score_[counter_][j] = self.day_ma_personal_score_[counter_][j] +1

                            elif self.day_moving_avg_[counter_][i] > self.day_moving_avg_[counter_][j]:
                                self.day_ma_score_[counter_] = self.day_ma_score_[counter_] -1       
                                self.day_ma_personal_score_[counter_][i] = self.day_ma_personal_score_[counter_][i] +1
                                # self.day_ma_personal_score_[counter_][j] = self.day_ma_personal_score_[counter_][j] -1


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
                    # print("close",self.om_.started_at_,c)
                    # for i in range(0,len(self.day_ma_lkbk_)):
                    #     print("hi there",i,self.day_moving_avg_[counter_][i], self.training_data_[counter_][self.training_data_[counter_]["datetime"] > arr[i]]["close"].mean())
                    # print("hi there",self.day_moving_avg_[counter_][3],self.training_data_[counter_]["close"].mean(),len(self.training_data_[counter_]))
                    # self.training_data_[counter_].loc[len(self.training_data_[counter_])+self.oldest_bar_[counter_]] = [self.om_.started_at_,o,c,l,h,v]
                    # self.training_data_[counter_].drop([self.oldest_bar_[counter_]],inplace=True)


                    self.oldest_bar_[counter_] = (
                        self.oldest_bar_[counter_] + 1) % len(self.training_data_[counter_])
                self.long_term_vol_[index_][self.lt_count_[index_] % self.lt_days_] = v*self.prev_day_close_[index_]#*self.prev_day_close_[index_]/(h-l+0.0000001)
                self.long_term_vol_[counter_][self.lt_count_[counter_] % self.lt_days_] = v*(np.max(self.day_moving_avg_[index_]) - np.min(self.day_moving_avg_[index_]))*(np.max(self.day_moving_avg_[index_]) - np.min(self.day_moving_avg_[index_]))/(h-l+ 0.0000005)
                # print("val",v,(np.max(self.day_moving_avg_[index_]) - np.min(self.day_moving_avg_[index_]))*(np.max(self.day_moving_avg_[index_]) - np.min(self.day_moving_avg_[index_])),(h-l+ 0.0000005))
                if h - self.prev_day_high_[index_] > 0 and h - self.prev_day_high_[index_] > self.prev_day_low_[index_] -l:
                    self.plus_di_[index_][self.last_di_update_[index_]] = h - self.prev_day_high_[index_]
                else:
                    self.plus_di_[index_][self.last_di_update_[index_]] = 0

                if self.prev_day_low_[index_] -l > 0 and h - self.prev_day_high_[index_] < self.prev_day_low_[index_] -l:
                    self.neg_di_[index_][self.last_di_update_[index_]] = self.prev_day_low_[index_] -l
                else:
                    self.neg_di_[index_][self.last_di_update_[index_]] = 0

                self.stochastic_oclh_[index_][self.last_stoch_update_[index_]] = [o,c,l,h]
                self.stochastic_ind_[index_] = 100*(c-self.stochastic_oclh_[index_][:,2].min())/(self.stochastic_oclh_[index_][:,3].max() - self.stochastic_oclh_[index_][:,2].min())

                if c-o > 0:
                    self.gain_[index_][self.last_stoch_update_[index_]] = c-o
                    self.loss_[index_][self.last_stoch_update_[index_]] = 0
                elif c-o < 0:
                    self.loss_[index_][self.last_stoch_update_[index_]] = o-c
                    self.gain_[index_][self.last_stoch_update_[index_]] = 0
                else:
                    self.gain_[index_][self.last_stoch_update_[index_]] = 0
                    self.loss_[index_][self.last_stoch_update_[index_]] = 0
                
                a = np.sum(self.gain_[index_])/len(np.nonzero(self.gain_[index_])[0])
                b = np.sum(self.loss_[index_])/len(np.nonzero(self.loss_[index_])[0])
                self.rsi_[index_] = 100 - 100/(1 + a/b)
                # print(o,c,l,h)
                # print(self.gain_[index_],self.loss_[index_],self.rsi_[index_],a,b)


                self.smoothed_true_range_[index_] = -1*np.mean(self.true_range_[index_])
                self.smoothed_plus_di_[index_] = -1*np.mean(self.plus_di_[index_])
                self.smoothed_neg_di_[index_] = -1*np.mean(self.neg_di_[index_])

                self.smoothed_true_range_[index_] = self.smoothed_true_range_[index_] + self.true_range_[index_][self.last_di_update_[index_]] + np.sum(self.true_range_[index_])
                self.smoothed_plus_di_[index_] = self.smoothed_plus_di_[index_] + self.plus_di_[index_][self.last_di_update_[index_]] + np.sum(self.plus_di_[index_])
                self.smoothed_neg_di_[index_] = self.smoothed_neg_di_[index_] + self.neg_di_[index_][self.last_di_update_[index_]] + np.sum(self.neg_di_[index_])

                self.smoothed_plus_di_[index_] = 100*self.smoothed_plus_di_[index_]/self.smoothed_true_range_[index_]
                self.smoothed_neg_di_[index_] = 100*self.smoothed_neg_di_[index_]/self.smoothed_true_range_[index_]
                self.last_di_update_[index_] = (self.last_di_update_[index_] +1)%self.adr_days_
                self.last_stoch_update_[index_] = (self.last_stoch_update_[index_] +1)%self.stoch_days_

                if self.smoothed_plus_di_[index_] and self.smoothed_neg_di_[index_]:    
                    self.adx_[index_] = (self.adx_[index_]*(self.adr_days_-1) + np.abs(self.smoothed_plus_di_[index_] - self.smoothed_neg_di_[index_])\
                                                                            /(self.smoothed_plus_di_[index_] + self.smoothed_neg_di_[index_]))/self.adr_days_
                    # print("adx",self.adx_[index_],self.smoothed_plus_di_[index_],self.smoothed_neg_di_[index_])
                self.prev_day_close_[index_] = c
                self.prev_day_low_[index_] = l
                self.prev_day_high_[index_] = h
            else:
                self.long_term_vol_[index_][self.lt_count_[index_] % self.lt_days_] = np.NaN

            self.lt_count_[index_] = self.lt_count_[index_] + 1
            # # self.open_vol_count_[index_] = self.open_vol_count_[index_] + 1

            if not flag_ and self.prod_curr_posn_[index_][Fields.start_trading.value]:
                self.prod_curr_posn_[index_][Fields.start_trading.value] = True
            self.lt_std_[index_] = np.nanstd(self.long_term_vol_[index_])
            if self.lt_std_[index_] == 0:
                self.lt_std_[index_] = np.NaN
            self.open_vol_avg_[index_] = np.nanpercentile(self.opening_bar_volume_[index_], self.open_vol_ptile_)
            self.ma_diff_[index_] = (np.max(self.day_moving_avg_[index_]) - np.min(self.day_moving_avg_[index_]))/self.prev_day_close_[index_]
            # self.ma_diff_[index_] = np.nanstd(self.day_moving_avg_[index_][self.day_moving_avg_[index_]!=0])/self.prev_day_close_[index_]
            # temp_ = self.day_moving_avg_[index_]

            # self.ma_diff_[index_] = max(np.abs(np.max(temp_) - self.prev_day_close_[index_]),np.abs(np.min(temp_) - self.prev_day_close_[index_]))/self.prev_day_close_[index_]
            counter_ = counter_ + 1
        ptl_std_ = np.nanpercentile(self.lt_std_, self.filter_ptile_)
        self.ma_thresh_ = np.nanpercentile(self.ma_diff_, self.filter_ptile_)
        crossover_thesh_ = np.nanpercentile(np.sum(self.last_crossover_,axis=1),self.filter_ptile_)
        # self.filter_std_list_.append(ptl_std_)

        for index_ in range(0, len(self.product_list_)):
            if self.lt_std_[index_] < ptl_std_ and self.lt_count_[index_] >= self.lt_days_:
                self.prod_curr_posn_[index_][Fields.start_trading.value] = False
            # print( self.long_term_vol_[index_][self.lt_count_[index_] % self.lt_days_])
        # for index_ in range(0, len(self.product_list_)):
        #     if np.sum(self.last_crossover_[index_]) < crossover_thesh_:
        #         self.prod_curr_posn_[index_][Fields.start_trading.value] = False

        # for index_ in range(0, len(self.product_list_)):
        #     if self.ma_diff_[index_] < self.ma_thresh_ :#or (self.lt_std_[index_] < ptl_std_ and self.lt_count_[index_] >= self.lt_days_:
        #         self.prod_curr_posn_[index_][Fields.start_trading.value] = False
        # print("Open vol", self.open_vol_avg_)
        # pd.options.display.float_format = '{:.0f}'.format
        # pd.options.display.float_format = '{:.4f}'.format
        # print("vec: ", self.long_term_vol_/10000,self.lt_count_)
        print("longterm",ptl_std_, self.lt_std_,self.day_ma_score_)
        # print("filter", ptl_std_, self.lt_days_)

    

    # Responsible for bar creation after receiving input from BarGenerator and sending the bar to onbarupdate
    def aggregator(self, _prod_bar_info, _time):
        if self.last_bar_update_ == 0:
            self.last_bar_update_ = self.om_.started_at_

        for index_ in range(0, len(self.product_list_)):
            if self.next_hourly_update_[index_] == 0:
                self.next_hourly_update_[index_] = self.om_.started_at_ 
            open_, close_, low_, high_, vol_ = _prod_bar_info[index_]
            # print(_time,open_, close_, low_, high_, vol_)
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

            
            if _time > self.next_hourly_update_[index_]:
                self.next_hourly_update_[index_] = self.next_hourly_update_[index_]  + 3600
                pd.options.display.float_format = '{:.4f}'.format
                # print(self.hourly_data_[index_])
                if self.max_size_hour_ma_ == len(self.hourly_data_[index_]):
                    for i in range(0,len(self.hour_ma_lkbk_)):
                        if i == len(self.hour_ma_lkbk_) -1 and self.hour_moving_avg_[index_][i] == 0:
                            self.hour_moving_avg_[index_][i] = self.hourly_data_[index_]["close"].mean()
                        else:
                            self.hour_moving_avg_[index_][i] = (self.hour_ma_lkbk_[i]*self.hour_moving_avg_[index_][i] + self.hourly_data_[index_].get_value(self.last_hourly_update_index_[index_],"close") -\
                                        self.hourly_data_[index_].get_value((self.last_hourly_update_index_[index_] - self.hour_ma_lkbk_[i] +1 + len(self.hourly_data_[index_]))%len(self.hourly_data_[index_])\
                                                                , "close") )/self.hour_ma_lkbk_[i]
                        # print("bye there",self.hourly_data_[index_].get_value(self.last_hourlyf_update_index_[index_],"close"),   self.hourly_data_[index_].get_value((self.last_hourly_update_index_[index_] - self.hour_ma_lkbk_[i] +1 + len(self.hourly_data_[index_]))%len(self.hourly_data_[index_])\
                        #                                         , "close"))
                        # print("hi there ",self.hour_moving_avg_[index_][i],self.hourly_data_[index_]["close"].mean())
                    
                    self.last_hourly_update_index_[index_] = (self.last_hourly_update_index_[index_] + 1)%self.max_size_hour_ma_
                    self.hourly_data_[index_].set_value(self.last_hourly_update_index_[index_],"datetime",_time)
                    self.hourly_data_[index_].set_value(self.last_hourly_update_index_[index_],"close",close_)

                else:
                    for i in range(0,len(self.hour_ma_lkbk_)):
                        if len(self.hourly_data_[index_]) == self.hour_ma_lkbk_[i]:
                            self.hour_moving_avg_[index_][i] = self.hourly_data_[index_]["close"].mean()
                            # print("hi there ",self.hour_moving_avg_[index_][i],self.hourly_data_[index_]["close"].mean())
                        elif len(self.hourly_data_[index_]) > self.hour_ma_lkbk_[i]:
                            # print(self.hourly_data_[index_].get_value(len(self.hourly_data_[index_])-1,"close"))
                            self.hour_moving_avg_[index_][i] = (self.hour_ma_lkbk_[i]*self.hour_moving_avg_[index_][i] + self.hourly_data_[index_].get_value(len(self.hourly_data_[index_])-1,"close") -\
                                    self.hourly_data_[index_].get_value(( len(self.hourly_data_[index_])- self.hour_ma_lkbk_[i] -1 )%len(self.hourly_data_[index_])\
                                                            , "close") )/self.hour_ma_lkbk_[i] 
                            # print("hi there ",self.hour_moving_avg_[index_][i],self.hourly_data_[index_]["close"].mean())

                    self.hourly_data_[index_].loc[len(self.hourly_data_[index_])] = [_time,close_]


            else:
                if self.max_size_hour_ma_ == len(self.hourly_data_[index_]):
                    self.hourly_data_[index_].set_value(self.last_hourly_update_index_[index_],"datetime",_time)
                    self.hourly_data_[index_].set_value(self.last_hourly_update_index_[index_],"close",close_)
                else:
                    self.hourly_data_[index_].set_value(len(self.hourly_data_[index_])-1,"datetime",_time)
                    self.hourly_data_[index_].set_value(len(self.hourly_data_[index_])-1,"close",close_)

            

        if _time == self.last_bar_update_ + self.granularity_ or _time - self.om_.ended_at_ == (datetime.strptime(
                self.strat_end_time_, "%H_%M") - datetime.strptime(self.train_end_time_, "%H_%M")).total_seconds():
            # print(_time,self.product_list_[index_],self.bar_agg_[index_][4],self.last_bar_update_)
            self.last_bar_update_ = _time
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
            self.next_hourly_update_ = [0] * len(self.product_list_)
            self.updateTrainingBar(self.day_bar_agg_)
            self.day_bar_agg_ = np.zeros(
                len(self.product_list_) * 5).reshape(len(self.product_list_), 5)
            self.bar_agg_ = np.zeros(
                len(self.product_list_) * 5).reshape(len(self.product_list_), 5)
            self.current_cascade_ = np.zeros(2*len(self.product_list_) * len(self.day_ma_lkbk_)).reshape(len(self.product_list_), 2*len(self.day_ma_lkbk_))
            self.current_cascade_ = self.current_cascade_.astype(int)
            self.prod_directions_ = [0] * len(self.product_list_)
            self.support_px =  np.zeros(2*len(self.product_list_) * len(self.day_ma_lkbk_)).reshape(len(self.product_list_), 2*len(self.day_ma_lkbk_))
            self.resist_px =  np.zeros(2*len(self.product_list_) * len(self.day_ma_lkbk_)).reshape(len(self.product_list_), 2*len(self.day_ma_lkbk_))
            self.take_pos_ = np.zeros(2*len(self.product_list_) * len(self.day_ma_lkbk_)).reshape(len(self.product_list_), 2*len(self.day_ma_lkbk_))
            self.allowed_pos_ = np.zeros(2*len(self.product_list_) * len(self.day_ma_lkbk_)).reshape(len(self.product_list_), 2*len(self.day_ma_lkbk_))
            self.exec_px_ = np.zeros(2*len(self.product_list_) * len(self.day_ma_lkbk_)).reshape(len(self.product_list_), 2*len(self.day_ma_lkbk_))
            self.crossover_lkbk_ = np.full((len(self.product_list_),2*len(self.day_ma_lkbk_)),1000)
            self.first_close_ = [0] * len(self.product_list_)
            self.prev_low_ = [0] * len(self.product_list_)
            self.prev_high_ = [0] * len(self.product_list_)
            self.current_low_ = [0] * len(self.product_list_)
            self.current_high_ = [0] * len(self.product_list_)
            self.current_vol_ = [0] * len(self.product_list_)
            self.prev_vol_ = [0] * len(self.product_list_)
            self.volume_condn_ = np.zeros((len(self.product_list_), len(self.day_ma_lkbk_)),dtype=bool)
            self.prev_bar_close_= [0] * len(self.product_list_)

    def getStochBar(self,_prod,_index, open_, close_, low_, high_):
        self.stochastic_bar_oclh_[_index][self.last_stoch_bar_update_[_index]] = [open_, close_, low_, high_]
        self.stochastic_bar_ind_[_index] = 100*(close_-self.stochastic_bar_oclh_[_index][:,2].min())/(self.stochastic_bar_oclh_[_index][:,3].max() - self.stochastic_bar_oclh_[_index][:,2].min())
        self.last_stoch_bar_update_[_index] = (self.last_stoch_bar_update_[_index] + 1)%self.stoch_bars_

    # Called from aggregator and handles main strat execution and reaction to granular bar
    def onBarUpdate(self, _prod_bar_info, _time):
        self.current_time_ = _time
        for index_ in range(0, len(self.product_list_)):
            prod_ = self.product_list_[index_]
            # pos_pnl_index_ = str(prod_+"_"+str(_time))
            self.prod_curr_posn_[index_][Fields.cascade_trigger.value] = False
            open_, close_, low_, high_, vol_ = _prod_bar_info[index_]
            # print("init bar",self.current_time_,open_,close_,low_,high_,vol_)
            self.current_bar_[index_] = [open_, close_, low_, high_]
            if self.current_time_ - self.om_.started_at_ == self.granularity_:
                if vol_:
                    self.opening_bar_volume_[index_][self.open_vol_count_[
                        index_] % self.open_vol_days_] = vol_
                    self.first_close_[index_] = close_
                    self.prev_bar_close_[index_] = self.prev_day_close_[index_]
                    
                else:
                    self.opening_bar_volume_[index_][self.open_vol_count_[
                        index_] % self.open_vol_days_] = np.NaN
                    self.first_close_[index_] = 0
                self.open_vol_count_[index_] = self.open_vol_count_[index_] + 1
            # if self.prod_curr_posn_[index_][Fields.start_trading.value]:

            if vol_:
                self.prev_vol_[index_] = self.current_vol_[index_]
                self.current_vol_[index_] = vol_
                self.getStochBar(prod_,index_,open_, close_, low_, high_)
                pnl_ = self.prod_curr_posn_[index_][Fields.pnl.value] + self.prod_curr_posn_[index_][Fields.pos.value] * (
                    close_ - self.prod_curr_posn_[index_][Fields.ltp.value])
                cascade_condition_ = False
                if pnl_ > self.prod_curr_posn_[index_][Fields.pnl.value] and pnl_ > 0:
                    self.prod_curr_posn_[index_][Fields.cascade_trigger.value] = True
                if self.prod_curr_posn_[index_][Fields.close_.value] !=0:
                    self.prev_bar_close_[index_] = self.prod_curr_posn_[index_][Fields.close_.value]
                # print("sample",close_,self.day_moving_avg_[index_][3],self.prev_bar_close_[index_])
                for i in range(0,len(self.day_ma_lkbk_)):
                    if close_ > self.day_moving_avg_[index_][i] and \
                        self.prev_bar_close_[index_] < self.day_moving_avg_[index_][i]:
                        self.crossover_lkbk_[index_][i] = 0
                    elif close_ < self.day_moving_avg_[index_][i] and \
                        self.prev_bar_close_[index_] > self.day_moving_avg_[index_][i]:
                        self.crossover_lkbk_[index_][i] = 0
                    else:
                        self.crossover_lkbk_[index_][i] = self.crossover_lkbk_[index_][i] + 1

                    if self.volume_condn_[index_][i] == False and self.crossover_lkbk_[index_][i] == 0 and\
                    not math.isnan(self.opening_bar_volume_[index_][(self.open_vol_count_[index_]+self.open_vol_days_-1)%self.open_vol_days_]) and \
                    (math.isnan(self.open_vol_avg_[index_]) or self.current_vol_[index_] > self.prev_vol_[index_]  or\
                    self.opening_bar_volume_[index_][(self.open_vol_count_[index_]+self.open_vol_days_-1)%self.open_vol_days_]> self.open_vol_avg_[index_]):
                        self.volume_condn_[index_][i] = True


                for i in range(0,len(self.hour_ma_lkbk_)):
                    if close_ > self.hour_moving_avg_[index_][i] and \
                        self.prev_bar_close_[index_] < self.hour_moving_avg_[index_][i]:
                        self.crossover_lkbk_[index_][i+4] = 0
                    elif close_ < self.hour_moving_avg_[index_][i] and \
                        self.prev_bar_close_[index_] > self.hour_moving_avg_[index_][i]:
                        self.crossover_lkbk_[index_][i+4] = 0
                    else:
                        self.crossover_lkbk_[index_][i+4] = self.crossover_lkbk_[index_][i+4] + 1



                self.prod_curr_posn_[index_][Fields.pnl.value] = pnl_
                self.prod_curr_posn_[index_][Fields.ltp.value] = close_
                self.prod_curr_posn_[index_][Fields.last_updated.value] = _time
                self.prod_curr_posn_[index_][Fields.close_.value] = close_
                self.prev_low_[index_] = self.current_low_[index_]
                self.prev_high_[index_] = self.current_high_[index_]
                self.current_low_[index_] = low_
                self.current_high_[index_] = high_
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
                self.checkSL(index_, self.prod_curr_posn_[index_][Fields.ltp.value])

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
            # if curr_macd_ > curr_signal_ and prev_macd_ < prev_signal_:

            if self.prod_curr_posn_[index_][Fields.close_.value] and not self.prod_curr_posn_[index_][Fields.sqoff_on.value]\
                    and self.prod_curr_posn_[index_][Fields.start_trading.value]:
                # if self.prod_curr_posn_[index_][Fields.pos.value] == 0 or self.prod_curr_posn_[index_][Fields.cascade_trigger.value]:


                # self.tradeBarMACDMeanRevert(prod_,index_)
                # self.tradeDayMACascade(prod_,index_)
                # self.tradeMinMaxDayMACascade(prod_,index_)
                self.tradeClosestDayMACascade(prod_,index_)
                # self.tradeDayMACapCascade(prod_,index_)
                # self.tradeHourMACascade(prod_,index_)
                # self.tradeBoundaryMACascade(prod_,index_)
                # self.tradeDayMAThresholdCascade(prod_,index_)
                # self.tradeHourMAThresholdCascade(prod_,index_)
                # self.tradeReverseDayMA(prod_,index_)

    def tradeBoundaryMACascade(self,_prod,_index):       
        pos_ = math.floor(self.max_exposure_ / self.prod_curr_posn_[_index][Fields.close_.value])

        self.ma_vec_ = self.day_moving_avg_[_index]
        self.ma_vec_= np.append(self.ma_vec_,self.hour_moving_avg_[_index])

        self.nearest_big_[_index] = self.ma_vec_.max()
        self.nearest_small_[_index] = self.ma_vec_.min()

        for i in range(0,len(self.ma_vec_)):
            if self.nearest_big_[_index] - self.prod_curr_posn_[_index][Fields.close_.value] > self.ma_vec_[i] - self.prod_curr_posn_[_index][Fields.close_.value] and\
                self.ma_vec_[i] > self.prod_curr_posn_[_index][Fields.close_.value]:
                self.nearest_big_[_index] = self.ma_vec_[i]

            if self.nearest_small_[_index] - self.prod_curr_posn_[_index][Fields.close_.value] < self.ma_vec_[i] - self.prod_curr_posn_[_index][Fields.close_.value] and\
                self.ma_vec_[i] < self.prod_curr_posn_[_index][Fields.close_.value]:
                self.nearest_small_[_index] = self.ma_vec_[i]
        # print(self.ma_vec_)
        # print(self.nearest_small_,self.prod_curr_posn_[_index][Fields.close_.value] ,self.nearest_big_)
    
    def tradeDayMAThresholdCascade(self,_prod,_index):
        pos_ = math.floor(self.max_exposure_ / self.prod_curr_posn_[_index][Fields.close_.value])
        # print(self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][3])
        for i in range(0,len(self.day_ma_lkbk_)):
            

            if self.crossover_lkbk_[_index][i] < self.crossover_limit_ and self.take_pos_[_index][i] == 0 and self.allowed_pos_[_index][i] == 0 and\
                self.day_moving_avg_[_index][i] != 0 and self.prod_curr_posn_[_index][Fields.close_.value] > self.day_moving_avg_[_index][i]*self.ma_entry_pt_ and\
                self.prod_curr_posn_[_index][Fields.high_.value] < self.nearest_big_[_index]:
                # self.prod_curr_posn_[_index][Fields.close_.value] > self.nearest_small_[_index]*self.ma_entry_pt_ and\
                # self.nearest_big_[_index] > self.nearest_small_[_index]*(2*self.ma_entry_pt_-1):
                # (2*self.prod_curr_posn_[_index][Fields.close_.value] < self.nearest_small_[_index]+  self.nearest_big_[_index]):
                # ((self.prev_bar_close_[_index] != 0 and  self.prev_bar_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] ) \
                # or self.prev_bar_close_[_index] == 0 and self.prev_day_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] )and \
                
                # self.prod_curr_posn_[_index][Fields.close_.value] < self.day_moving_avg_[_index][i]*(2*self.entry_pt_ - 1):
                for j in range(0,len(self.hour_ma_lkbk_)):
                    print(j,self.hour_moving_avg_[_index][j],self.day_moving_avg_[_index][j]," ",end='')
                print(self.crossover_lkbk_[_index][i],"")
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][i])
                self.sendOrder('B', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)
                # self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.high_.value]
                self.take_pos_[_index][i] = pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]

            elif self.crossover_lkbk_[_index][i] < self.crossover_limit_ and  self.take_pos_[_index][i] == 0 and self.allowed_pos_[_index][i] == 0 and\
                 self.day_moving_avg_[_index][i] != 0 and self.prod_curr_posn_[_index][Fields.close_.value] < self.day_moving_avg_[_index][i]/self.ma_entry_pt_ and \
                 self.prod_curr_posn_[_index][Fields.low_.value] > self.nearest_small_[_index]:
                 # self.prod_curr_posn_[_index][Fields.close_.value] < self.nearest_big_[_index]*self.ma_entry_pt_ and\
                 # self.nearest_big_[_index] > self.nearest_small_[_index]*(2*self.ma_entry_pt_-1):
                 # (2*self.prod_curr_posn_[_index][Fields.close_.value] > self.nearest_big_[_index]+  self.nearest_small_[_index] or ):
                 # ((self.prev_bar_close_[_index] != 0 and  self.prev_bar_close_[_index] > self.prod_curr_posn_[_index][Fields.close_.value] ) \
                # or self.prev_bar_close_[_index] == 0 and self.prev_day_close_[_index] > self.prod_curr_posn_[_index][Fields.close_.value] ) and \
                 # self.prod_curr_posn_[_index][Fields.close_.value] > self.day_moving_avg_[_index][i]*(2*self.entry_pt_ - 1):
                
                for j in range(0,len(self.hour_ma_lkbk_)):
                    print(j,self.hour_moving_avg_[_index][j],self.day_moving_avg_[_index][j]," ",end='')
                print(self.crossover_lkbk_[_index][i],"")
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][i])
                self.sendOrder('S', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value],self.current_time_)
                # self.resist_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.resist_px[_index][i] = self.prod_curr_posn_[_index][Fields.low_.value]
                self.take_pos_[_index][i] = -1*pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]


            elif self.current_cascade_[_index][i] == 1:
                if self.take_pos_[_index][i] > 0 and self.prod_curr_posn_[_index][Fields.close_.value] > self.exec_px_[_index][i]:
                    self.sendOrder('B', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)
                    self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                    self.take_pos_[_index][i] = self.take_pos_[_index][i] + pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                    self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                    self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]

                elif self.take_pos_[_index][i] < 0 and self.prod_curr_posn_[_index][Fields.close_.value] < self.exec_px_[_index][i]:
                    self.sendOrder('S', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value],self.current_time_)
                    self.resist_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                    self.take_pos_[_index][i] = self.take_pos_[_index][i] - pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                    self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                    self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]

    def tradeDayMACapCascade(self,_prod,_index):
        pos_ = math.floor(self.max_exposure_ / self.prod_curr_posn_[_index][Fields.close_.value])
        # print(self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][3])
        for i in range(0,len(self.day_ma_lkbk_)):
            if  self.take_pos_[_index][i] == 0 and self.allowed_pos_[_index][i] == 0 and\
                ((self.prev_bar_close_[_index] != 0 and  self.prev_bar_close_[_index] > self.prod_curr_posn_[_index][Fields.close_.value] ) \
                or self.prev_bar_close_[_index] == 0 and self.prev_day_close_[_index] > self.prod_curr_posn_[_index][Fields.close_.value] )and \
                self.day_moving_avg_[_index][i] != 0 and self.prod_curr_posn_[_index][Fields.close_.value] < self.day_moving_avg_[_index][i]*self.ma_entry_pt_ and\
                self.prod_curr_posn_[_index][Fields.close_.value] > self.day_moving_avg_[_index][i] and  self.prod_curr_posn_[_index][Fields.low_.value] < self.day_moving_avg_[_index][i]:

                for j in range(0,len(self.hour_ma_lkbk_)):
                    print(j,self.hour_moving_avg_[_index][j],self.day_moving_avg_[_index][j]," ",end='')
                print(self.crossover_lkbk_[_index][i],"")
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][i])
                self.sendOrder('B', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)
                # self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.take_pos_[_index][i] = self.cascade_size_[self.current_cascade_[_index][i]]*pos_
                self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]

            elif self.take_pos_[_index][i] == 0 and self.allowed_pos_[_index][i] == 0 and\
                ((self.prev_bar_close_[_index] != 0 and  self.prev_bar_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] ) \
                or self.prev_bar_close_[_index] == 0 and self.prev_day_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] )and \
                self.day_moving_avg_[_index][i] != 0 and self.prod_curr_posn_[_index][Fields.close_.value] > self.day_moving_avg_[_index][i]*self.ma_entry_pt_ and\
                self.prod_curr_posn_[_index][Fields.close_.value] < self.day_moving_avg_[_index][i] and  self.prod_curr_posn_[_index][Fields.high_.value] > self.day_moving_avg_[_index][i]:

                for j in range(0,len(self.hour_ma_lkbk_)):
                    print(j,self.hour_moving_avg_[_index][j],self.day_moving_avg_[_index][j]," ",end='')
                print(self.crossover_lkbk_[_index][i],"")
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][i])
                self.sendOrder('S', _index, pos_*self.cascade_size_[self.current_cascade_[_index][i]], self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)
                # self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.take_pos_[_index][i] = -1*pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]

            elif self.current_cascade_[_index][i] == 1:
                if self.take_pos_[_index][i] > 0 and self.prod_curr_posn_[_index][Fields.close_.value] > self.exec_px_[_index][i]:
                    self.sendOrder('B', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)
                    self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                    self.take_pos_[_index][i] = self.take_pos_[_index][i] + pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                    self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                    self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]

                elif self.take_pos_[_index][i] < 0 and self.prod_curr_posn_[_index][Fields.close_.value] < self.exec_px_[_index][i]:
                    self.sendOrder('S', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value],self.current_time_)
                    self.resist_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                    self.take_pos_[_index][i] = self.take_pos_[_index][i] - pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                    self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                    self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]



    def tradeDayMACascade(self,_prod,_index):
        # print(self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][3])
        for i in range(0,len(self.day_ma_lkbk_)):
            pos_ = (4-i)*math.floor(self.max_exposure_ / self.prod_curr_posn_[_index][Fields.close_.value])
            

            if self.crossover_lkbk_[_index][i] < self.crossover_limit_ and self.take_pos_[_index][i] == 0 and self.allowed_pos_[_index][i] == 0 and\
                self.day_moving_avg_[_index][i] != 0 and self.prod_curr_posn_[_index][Fields.close_.value] > self.day_moving_avg_[_index][i]*self.ma_entry_pt_:# and\
                # self.current_high_[_index] == self.prod_curr_posn_[_index][Fields.high_.value]:
                # ((self.prev_bar_close_[_index] != 0 and  self.prev_bar_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] ) \
                # or self.prev_bar_close_[_index] == 0 and self.prev_day_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] )and \
                
                # self.prod_curr_posn_[_index][Fields.close_.value] < self.day_moving_avg_[_index][i]*(2*self.entry_pt_ - 1):
                for j in range(0,len(self.hour_ma_lkbk_)):
                    print(j,self.hour_moving_avg_[_index][j],self.day_moving_avg_[_index][j]," ",end='')
                print(self.crossover_lkbk_[_index][i],"")
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][i])
                self.sendOrder('B', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)
                # self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.high_.value]
                self.take_pos_[_index][i] = pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]

            elif self.crossover_lkbk_[_index][i] < self.crossover_limit_ and  self.take_pos_[_index][i] == 0 and self.allowed_pos_[_index][i] == 0 and\
                 self.day_moving_avg_[_index][i] != 0 and self.prod_curr_posn_[_index][Fields.close_.value] < self.day_moving_avg_[_index][i]/self.ma_entry_pt_ :#and \
                 # self.current_low_[_index] == self.prod_curr_posn_[_index][Fields.low_.value]:
                 # ((self.prev_bar_close_[_index] != 0 and  self.prev_bar_close_[_index] > self.prod_curr_posn_[_index][Fields.close_.value] ) \
                # or self.prev_bar_close_[_index] == 0 and self.prev_day_close_[_index] > self.prod_curr_posn_[_index][Fields.close_.value] ) and \
                 # self.prod_curr_posn_[_index][Fields.close_.value] > self.day_moving_avg_[_index][i]*(2*self.entry_pt_ - 1):
                
                for j in range(0,len(self.hour_ma_lkbk_)):
                    print(j,self.hour_moving_avg_[_index][j],self.day_moving_avg_[_index][j]," ",end='')
                print(self.crossover_lkbk_[_index][i],"")
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][i])
                self.sendOrder('S', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value],self.current_time_)
                # self.resist_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.resist_px[_index][i] = self.prod_curr_posn_[_index][Fields.low_.value]
                self.take_pos_[_index][i] = -1*pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]


            elif self.current_cascade_[_index][i] == 1:
                if self.take_pos_[_index][i] > 0 and self.prod_curr_posn_[_index][Fields.close_.value] > self.exec_px_[_index][i]:
                    self.sendOrder('B', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)
                    self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.high_.value]
                    self.take_pos_[_index][i] = self.take_pos_[_index][i] + pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                    self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                    self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]

                elif self.take_pos_[_index][i] < 0 and self.prod_curr_posn_[_index][Fields.close_.value] < self.exec_px_[_index][i]:
                    self.sendOrder('S', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value],self.current_time_)
                    self.resist_px[_index][i] = self.prod_curr_posn_[_index][Fields.low_.value]
                    self.take_pos_[_index][i] = self.take_pos_[_index][i] - pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                    self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                    self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]

    def tradeMinMaxDayMACascade(self,_prod,_index):
        # print(self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][3])
        for i in range(0,len(self.day_ma_lkbk_)):
            # print(_prod,self.day_moving_avg_[_index])
            pos_ = (4-i)*math.floor(self.max_exposure_ / self.prod_curr_posn_[_index][Fields.close_.value])
            if len(self.day_moving_avg_[_index][self.day_moving_avg_[_index]!=0]) >0 :
                min_ = np.min(self.day_moving_avg_[_index][self.day_moving_avg_[_index]!=0])
                max_ = np.max(self.day_moving_avg_[_index][self.day_moving_avg_[_index]!=0])
            else: 
                return
            # for j in range(0,len(self.hour_ma_lkbk_)):
            #     print(j,self.day_moving_avg_[_index][j]," ",end='')
            # print(min_,max_," ",end='')
            # print("")
            if self.crossover_lkbk_[_index][i] < self.crossover_limit_ and self.take_pos_[_index][i] == 0 and self.allowed_pos_[_index][i] == 0 and\
                self.day_moving_avg_[_index][i] == max_ and self.prod_curr_posn_[_index][Fields.close_.value] > self.day_moving_avg_[_index][i]*self.ma_entry_pt_:# and\
                # ((self.prev_bar_close_[_index] != 0 and  self.prev_bar_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] ) \
                # or self.prev_bar_close_[_index] == 0 and self.prev_day_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] )and \
                
                # self.prod_curr_posn_[_index][Fields.close_.value] < self.day_moving_avg_[_index][i]*(2*self.entry_pt_ - 1):
                for j in range(0,len(self.hour_ma_lkbk_)):
                    print(j,self.day_moving_avg_[_index][j]," ",end='')
                print(min_,max_," ",end='')
                print(self.crossover_lkbk_[_index][i],"")
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][i])
                self.sendOrder('B', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)
                # self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.high_.value]
                self.take_pos_[_index][i] = pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]

            elif self.crossover_lkbk_[_index][i] < self.crossover_limit_ and  self.take_pos_[_index][i] == 0 and self.allowed_pos_[_index][i] == 0 and\
                 self.day_moving_avg_[_index][i] == min_ and self.prod_curr_posn_[_index][Fields.close_.value] < self.day_moving_avg_[_index][i]/self.ma_entry_pt_: #and \
                 # ((self.prev_bar_close_[_index] != 0 and  self.prev_bar_close_[_index] > self.prod_curr_posn_[_index][Fields.close_.value] ) \
                # or self.prev_bar_close_[_index] == 0 and self.prev_day_close_[_index] > self.prod_curr_posn_[_index][Fields.close_.value] ) and \
                 # self.prod_curr_posn_[_index][Fields.close_.value] > self.day_moving_avg_[_index][i]*(2*self.entry_pt_ - 1):
                
                for j in range(0,len(self.hour_ma_lkbk_)):
                    print(j,self.hour_moving_avg_[_index][j],self.day_moving_avg_[_index][j]," ",end='')
                print(self.crossover_lkbk_[_index][i],"")
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][i])
                self.sendOrder('S', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value],self.current_time_)
                # self.resist_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.resist_px[_index][i] = self.prod_curr_posn_[_index][Fields.low_.value]
                self.take_pos_[_index][i] = -1*pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]


            elif self.current_cascade_[_index][i] == 1:
                if self.take_pos_[_index][i] > 0 and self.prod_curr_posn_[_index][Fields.close_.value] > self.exec_px_[_index][i]:
                    self.sendOrder('B', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)
                    self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.high_.value]
                    self.take_pos_[_index][i] = self.take_pos_[_index][i] + pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                    self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                    self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]

                elif self.take_pos_[_index][i] < 0 and self.prod_curr_posn_[_index][Fields.close_.value] < self.exec_px_[_index][i]:
                    self.sendOrder('S', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value],self.current_time_)
                    self.resist_px[_index][i] = self.prod_curr_posn_[_index][Fields.low_.value]
                    self.take_pos_[_index][i] = self.take_pos_[_index][i] - pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                    self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                    self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]


    def tradeClosestDayMACascade(self,_prod,_index):
        # print(self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][3])
        self.day_min_[_index] = -1
        self.day_max_[_index] = -1
        if len(self.day_moving_avg_[_index][self.day_moving_avg_[_index]!=0]) == 0 :
            return

        # for i in range(0,len(self.day_ma_lkbk_)):
        #     if self.day_moving_avg_[_index][i] !=0:
        #         if min_ == -1 and self.day_moving_avg_[_index][i]  < self.prod_curr_posn_[_index][Fields.close_.value]:
        #             min_ = i
        #         elif min_ !=-1 and self.day_moving_avg_[_index][i]  < self.prod_curr_posn_[_index][Fields.close_.value] and \
        #             self.day_moving_avg_[_index][i] > self.day_moving_avg_[_index][min_]:
        #             min_ = i

        #         if max_ == -1 and self.day_moving_avg_[_index][i]  > self.prod_curr_posn_[_index][Fields.close_.value]:
        #             max_ = i
        #         elif max_ != -1 and self.day_moving_avg_[_index][i]  > self.prod_curr_posn_[_index][Fields.close_.value] and \
        #             self.day_moving_avg_[_index][i] < self.day_moving_avg_[_index][max_]:
        #             max_ = i     

        for i in range(0,len(self.day_ma_lkbk_)):
            if self.day_moving_avg_[_index][i] !=0:
                if self.day_min_[_index] == -1 and self.day_moving_avg_[_index][i]  < self.prod_curr_posn_[_index][Fields.close_.value]:
                    self.day_min_[_index] = i
                elif self.day_min_[_index] !=-1 and self.day_moving_avg_[_index][i]  < self.prod_curr_posn_[_index][Fields.close_.value] and \
                    self.day_moving_avg_[_index][i] > self.day_moving_avg_[_index][self.day_min_[_index]]:
                    self.day_min_[_index] = i

                if self.day_max_[_index] == -1 and self.day_moving_avg_[_index][i]  > self.prod_curr_posn_[_index][Fields.close_.value]:
                    self.day_max_[_index] = i
                elif self.day_max_[_index] != -1 and self.day_moving_avg_[_index][i]  > self.prod_curr_posn_[_index][Fields.close_.value] and \
                    self.day_moving_avg_[_index][i] < self.day_moving_avg_[_index][self.day_max_[_index]]:
                    self.day_max_[_index] = i     


        # print(self.day_moving_avg_[_index])               
        # if min_ != -1:
        #    print(min_,self.day_moving_avg_[_index][min_],self.prod_curr_posn_[_index][Fields.close_.value] ,"minmax-min") 

        # if max_ != -1:
        #     print(max_,self.day_moving_avg_[_index][max_],self.prod_curr_posn_[_index][Fields.close_.value] ,"minmax-max")

        for i in range(0,len(self.day_ma_lkbk_)):
            # print(_prod,self.day_moving_avg_[_index])
            # pos_ = (4-i)*math.floor(self.max_exposure_ / self.prod_curr_posn_[_index][Fields.close_.value])
            pos_ = math.floor(self.max_exposure_ / self.prod_curr_posn_[_index][Fields.close_.value])
            # for j in range(0,len(self.hour_ma_lkbk_)):
            #     print(j,self.day_moving_avg_[_index][j]," ",end='')
            # print(min_,max_," ",end='')
            # print("")
            # print(self.crossover_lkbk_[_index][i],self.crossover_limit_)
            if self.crossover_lkbk_[_index][i] < self.crossover_limit_ and self.take_pos_[_index][i] == 0 and self.allowed_pos_[_index][i] == 0 and self.day_min_[_index] != -1 and  i == self.day_min_[_index] and\
                self.day_moving_avg_[_index][i] != 0 and self.prod_curr_posn_[_index][Fields.close_.value] > self.day_moving_avg_[_index][i]*self.ma_entry_pt_:#  and\
                # self.stochastic_ind_[_index] < 80:# and i <= 2:
                # not math.isnan(self.opening_bar_volume_[_index][(self.open_vol_count_[_index]+self.open_vol_days_-1)%self.open_vol_days_]) and \
                # (math.isnan(self.open_vol_avg_[_index]) or  self.opening_bar_volume_[_index][(self.open_vol_count_[_index]+self.open_vol_days_-1)%self.open_vol_days_]> self.open_vol_avg_[_index] \
                # or self.current_vol_[_index] > self.prev_vol_[_index] ) and self.stochastic_ind_[_index] < 80 :
                # self.stochastic_ind_[_index] < 80 and self.volume_condn_[_index][i] == True:
                # (self.current_high_[_index] == self.prod_curr_posn_[_index][Fields.high_.value] and self.stochastic_ind_[_index] < 80):
                # self.rsi_[_index] < 70:
                # self.stochastic_ind_[_index] > 20 and self.stochastic_ind_[_index] < 80:
                # (self.smoothed_plus_di_[_index] == 0 or self.smoothed_plus_di_[_index] > self.smoothed_neg_di_[_index]):
                # (self.prev_high_[_index] == 0 or self.current_high_[_index] == self.prod_curr_posn_[_index][Fields.high_.value]):
                # (self.prev_day_moving_avg_[_index][i] == 0 or np.abs(self.prev_day_moving_avg_[_index][i] - self.day_moving_avg_[_index][i]) > self.prev_delta_*self.prev_day_moving_avg_[_index][i]): 
                # (self.prev_day_moving_avg_[_index][i] ==0 or self.day_moving_avg_[_index][i] > self.prev_day_moving_avg_[_index][i]):
                # (max_ == -1 or (self.day_moving_avg_[_index][i] + self.day_moving_avg_[_index][max_]) > 2*self.prod_curr_posn_[_index][Fields.close_.value]):
                # # ((self.prev_bar_close_[_index] != 0 and  self.prev_bar_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] ) \
                # or self.prev_bar_close_[_index] == 0 and self.prev_day_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] )and \
                
                # self.prod_curr_posn_[_index][Fields.close_.value] < self.day_moving_avg_[_index][i]*(2*self.entry_pt_ - 1):
                for j in range(0,len(self.hour_ma_lkbk_)):
                    print(j,self.day_moving_avg_[_index][j]," ",end='')
                print(self.day_min_[_index],self.day_moving_avg_[_index][self.day_min_[_index]],"minmax")
                # for j in range(0,len(self.hour_ma_lkbk_)):
                #     print(j,self.support_px[_index][j]," ",end='')
                # print("")
                print("stochastic_ind_",self.stochastic_ind_[_index], self.last_crossover_[_index])
                # print(self.crossover_lkbk_[_index][i],"")
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][i])
                self.sendOrder('B', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)
                # self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.high_.value]
                self.take_pos_[_index][i] = pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]

            elif self.crossover_lkbk_[_index][i] < self.crossover_limit_ and  self.take_pos_[_index][i] == 0 and self.allowed_pos_[_index][i] == 0 and self.day_max_[_index]!= -1 and i == self.day_max_[_index] and\
                 self.day_moving_avg_[_index][i] != 0 and self.prod_curr_posn_[_index][Fields.close_.value] < self.day_moving_avg_[_index][i]/self.ma_entry_pt_:# and \
                 # self.stochastic_ind_[_index] > 20:# and i <= 2:
                #  not math.isnan(self.opening_bar_volume_[_index][(self.open_vol_count_[_index]+self.open_vol_days_-1)%self.open_vol_days_]) and \
                # (math.isnan(self.open_vol_avg_[_index]) or  self.opening_bar_volume_[_index][(self.open_vol_count_[_index]+self.open_vol_days_-1)%self.open_vol_days_]> self.open_vol_avg_[_index] \
                # or self.current_vol_[_index] > self.prev_vol_[_index] ) and self.stochastic_ind_[_index] > 20 :
                 # self.stochastic_ind_[_index] > 20 and self.volume_condn_[_index][i] == True:
                 # (self.current_low_[_index] == self.prod_curr_posn_[_index][Fields.low_.value] and self.stochastic_ind_[_index] > 20):
                 # self.rsi_[_index] > 30:
                 # self.stochastic_ind_[_index] > 20 and self.stochastic_ind_[_index] < 80:
                 # (self.smoothed_neg_di_[_index] == 0 or self.smoothed_plus_di_[_index] < self.smoothed_neg_di_[_index]):
                 # (self.prev_low_[_index] == 0 or self.current_low_[_index] == self.prod_curr_posn_[_index][Fields.low_.value]):
                 # (self.prev_day_moving_avg_[_index][i] == 0 or np.abs(self.prev_day_moving_avg_[_index][i] - self.day_moving_avg_[_index][i]) > self.prev_delta_*self.prev_day_moving_avg_[_index][i]): 
                 # (self.prev_day_moving_avg_[_index][i] ==0 or self.day_moving_avg_[_index][i] < self.prev_day_moving_avg_[_index][i]):
                 # (min_ == -1 or (self.day_moving_avg_[_index][i] + self.day_moving_avg_[_index][min_]) < 2*self.prod_curr_posn_[_index][Fields.close_.value]):
                 # ((self.prev_bar_close_[_index] != 0 and  self.prev_bar_close_[_index] > self.prod_curr_posn_[_index][Fields.close_.value] ) \
                # or self.prev_bar_close_[_index] == 0 and self.prev_day_close_[_index] > self.prod_curr_posn_[_index][Fields.close_.value] ) and \
                 # self.prod_curr_posn_[_index][Fields.close_.value] > self.day_moving_avg_[_index][i]*(2*self.entry_pt_ - 1):
                
                for j in range(0,len(self.hour_ma_lkbk_)):
                    print(j,self.day_moving_avg_[_index][j]," ",end='')
                print(self.day_max_[_index],self.day_moving_avg_[_index][self.day_max_[_index]],"minmax")
                # for j in range(0,len(self.hour_ma_lkbk_)):
                #     print(j,self.resist_px[_index][j]," ",end='')
                # print("")
                print("stochastic_ind_",self.stochastic_ind_[_index], self.last_crossover_[_index])
                # print("adx",self.adx_[_index],self.smoothed_plus_di_[_index],self.smoothed_neg_di_[_index])
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][i])
                self.sendOrder('S', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value],self.current_time_)
                # self.resist_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.resist_px[_index][i] = self.prod_curr_posn_[_index][Fields.low_.value]
                self.take_pos_[_index][i] = -1*pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]


            elif self.current_cascade_[_index][i] >= 1 and self.prod_curr_posn_[_index][Fields.cascade_trigger.value] == True \
                and self.current_cascade_[_index][i] <  self.cascade_num_:
                if self.take_pos_[_index][i] > 0 :
                    self.sendOrder('B', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)
                    self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.high_.value]
                    self.take_pos_[_index][i] = self.take_pos_[_index][i] + pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                    self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                    self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]

                elif self.take_pos_[_index][i] < 0 :
                    self.sendOrder('S', _index, self.cascade_size_[self.current_cascade_[_index][i]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value],self.current_time_)
                    self.resist_px[_index][i] = self.prod_curr_posn_[_index][Fields.low_.value]
                    self.take_pos_[_index][i] = self.take_pos_[_index][i] - pos_*self.cascade_size_[self.current_cascade_[_index][i]]
                    self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1
                    self.exec_px_[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]


    def tradeHourMAThresholdCascade(self,_prod,_index):
        pos_ = math.floor(self.max_exposure_ / self.prod_curr_posn_[_index][Fields.close_.value])
        # print(self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][3])

        for i in range(0,len(self.hour_ma_lkbk_)):
            
            # print("hi there ",self.crossover_lkbk_[_index][i+4],self.take_pos_[_index][i+4],self.allowed_pos_[_index][i+4])
            if self.crossover_lkbk_[_index][i+4] < self.crossover_limit_ and self.take_pos_[_index][i+4] == 0 and self.allowed_pos_[_index][i+4] == 0 and\
                self.hour_moving_avg_[_index][i] != 0 and self.prod_curr_posn_[_index][Fields.close_.value] > self.hour_moving_avg_[_index][i]*self.hma_entry_pt_ and\
                2*self.prod_curr_posn_[_index][Fields.close_.value] < self.hour_moving_avg_[_index][i]+  self.nearest_big_[_index]:
                # ((self.prev_bar_close_[_index] != 0 and  self.prev_bar_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] ) \
                # or self.prev_bar_close_[_index] == 0 and self.prev_day_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] )and \
                # self.prod_curr_posn_[_index][Fields.close_.value] < self.day_moving_avg_[_index][i]*(2*self.entry_pt_ - 1):
                for j in range(0,len(self.hour_ma_lkbk_)):
                    print(j,self.hour_moving_avg_[_index][j],self.day_moving_avg_[_index][j]," ",end='')
                print(self.crossover_lkbk_[_index][i+4]," hour ")
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.hour_moving_avg_[_index][i])
                self.sendOrder('B', _index, self.cascade_size_[self.current_cascade_[_index][i+4]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)
                # self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.support_px[_index][i+4] = self.prod_curr_posn_[_index][Fields.high_.value]
                self.take_pos_[_index][i+4] = pos_*self.cascade_size_[self.current_cascade_[_index][i+4]]
                self.current_cascade_[_index][i+4] = self.current_cascade_[_index][i+4] +1
                self.exec_px_[_index][i+4] = self.prod_curr_posn_[_index][Fields.close_.value]

            elif self.crossover_lkbk_[_index][i+4] < self.crossover_limit_ and self.take_pos_[_index][i+4] == 0 and self.allowed_pos_[_index][i+4] == 0 and\
                 self.hour_moving_avg_[_index][i] != 0 and self.prod_curr_posn_[_index][Fields.close_.value] < self.hour_moving_avg_[_index][i]/self.hma_entry_pt_ and \
                 2*self.prod_curr_posn_[_index][Fields.close_.value] > self.hour_moving_avg_[_index][i]+  self.nearest_small_[_index]:
                # ((self.prev_bar_close_[_index] != 0 and  self.prev_bar_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] ) \
                # or self.prev_bar_close_[_index] == 0 and self.prev_day_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] )and \
                 # self.prod_curr_posn_[_index][Fields.close_.value] > self.day_moving_avg_[_index][i]*(2*self.entry_pt_ - 1):
                
                for j in range(0,len(self.hour_ma_lkbk_)):
                    print(j,self.hour_moving_avg_[_index][j],self.day_moving_avg_[_index][j]," ",end='')
                print(self.crossover_lkbk_[_index][i+4],"")
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.hour_moving_avg_[_index][i])
                self.sendOrder('S', _index, self.cascade_size_[self.current_cascade_[_index][i+4]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value],self.current_time_)
                # self.resist_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.resist_px[_index][i+4] = self.prod_curr_posn_[_index][Fields.low_.value]
                self.take_pos_[_index][i+4] = -1*pos_*self.cascade_size_[self.current_cascade_[_index][i+4]]
                self.current_cascade_[_index][i+4] = self.current_cascade_[_index][i+4] +1
                self.exec_px_[_index][i+4] = self.prod_curr_posn_[_index][Fields.close_.value]


            elif self.current_cascade_[_index][i+4] == 1:
                if self.take_pos_[_index][i+4] > 0 and self.prod_curr_posn_[_index][Fields.close_.value] > self.exec_px_[_index][i+4]:
                    self.sendOrder('B', _index, self.cascade_size_[self.current_cascade_[_index][i+4]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)
                    self.support_px[_index][i+4] = self.prod_curr_posn_[_index][Fields.close_.value]
                    self.take_pos_[_index][i+4] = self.take_pos_[_index][i+4] + pos_*self.cascade_size_[self.current_cascade_[_index][i+4]]
                    self.current_cascade_[_index][i+4] = self.current_cascade_[_index][i+4] +1
                    self.exec_px_[_index][i+4] = self.prod_curr_posn_[_index][Fields.close_.value]

                elif self.take_pos_[_index][i+4] < 0 and self.prod_curr_posn_[_index][Fields.close_.value] < self.exec_px_[_index][i+4]:
                    self.sendOrder('S', _index, self.cascade_size_[self.current_cascade_[_index][i+4]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value],self.current_time_)
                    self.resist_px[_index][i+4] = self.prod_curr_posn_[_index][Fields.close_.value]
                    self.take_pos_[_index][i+4] = self.take_pos_[_index][i+4] - pos_*self.cascade_size_[self.current_cascade_[_index][i+4]]
                    self.current_cascade_[_index][i+4] = self.current_cascade_[_index][i+4] +1
                    self.exec_px_[_index][i+4] = self.prod_curr_posn_[_index][Fields.close_.value]

    def tradeHourMACascade(self,_prod,_index):
        pos_ = math.floor(self.max_exposure_ / self.prod_curr_posn_[_index][Fields.close_.value])
        # print(self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][3])

        for i in range(0,len(self.hour_ma_lkbk_)):
            
            # print("hi there ",self.crossover_lkbk_[_index][i+4],self.take_pos_[_index][i+4],self.allowed_pos_[_index][i+4])
            if self.crossover_lkbk_[_index][i+4] < self.crossover_limit_ and self.take_pos_[_index][i+4] == 0 and self.allowed_pos_[_index][i+4] == 0 and\
                self.hour_moving_avg_[_index][i] != 0 and self.prod_curr_posn_[_index][Fields.close_.value] > self.hour_moving_avg_[_index][i]*self.hma_entry_pt_:# and\
                # ((self.prev_bar_close_[_index] != 0 and  self.prev_bar_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] ) \
                # or self.prev_bar_close_[_index] == 0 and self.prev_day_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] )and \
                # self.prod_curr_posn_[_index][Fields.close_.value] < self.day_moving_avg_[_index][i]*(2*self.entry_pt_ - 1):
                for j in range(0,len(self.hour_ma_lkbk_)):
                    print(j,self.hour_moving_avg_[_index][j],self.day_moving_avg_[_index][j]," ",end='')
                print(self.crossover_lkbk_[_index][i+4]," hour ")
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.hour_moving_avg_[_index][i])
                self.sendOrder('B', _index, self.cascade_size_[self.current_cascade_[_index][i+4]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)
                # self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.support_px[_index][i+4] = self.prod_curr_posn_[_index][Fields.high_.value]
                self.take_pos_[_index][i+4] = pos_*self.cascade_size_[self.current_cascade_[_index][i+4]]
                self.current_cascade_[_index][i+4] = self.current_cascade_[_index][i+4] +1
                self.exec_px_[_index][i+4] = self.prod_curr_posn_[_index][Fields.close_.value]

            elif self.crossover_lkbk_[_index][i+4] < self.crossover_limit_ and self.take_pos_[_index][i+4] == 0 and self.allowed_pos_[_index][i+4] == 0 and\
                 self.hour_moving_avg_[_index][i] != 0 and self.prod_curr_posn_[_index][Fields.close_.value] < self.hour_moving_avg_[_index][i]/self.hma_entry_pt_: #and \
                # ((self.prev_bar_close_[_index] != 0 and  self.prev_bar_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] ) \
                # or self.prev_bar_close_[_index] == 0 and self.prev_day_close_[_index] < self.prod_curr_posn_[_index][Fields.close_.value] )and \
                 # self.prod_curr_posn_[_index][Fields.close_.value] > self.day_moving_avg_[_index][i]*(2*self.entry_pt_ - 1):
                
                for j in range(0,len(self.hour_ma_lkbk_)):
                    print(j,self.hour_moving_avg_[_index][j],self.day_moving_avg_[_index][j]," ",end='')
                print(self.crossover_lkbk_[_index][i+4],"")
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.hour_moving_avg_[_index][i])
                self.sendOrder('S', _index, self.cascade_size_[self.current_cascade_[_index][i+4]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value],self.current_time_)
                # self.resist_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.resist_px[_index][i+4] = self.prod_curr_posn_[_index][Fields.low_.value]
                self.take_pos_[_index][i+4] = -1*pos_*self.cascade_size_[self.current_cascade_[_index][i+4]]
                self.current_cascade_[_index][i+4] = self.current_cascade_[_index][i+4] +1
                self.exec_px_[_index][i+4] = self.prod_curr_posn_[_index][Fields.close_.value]


            elif self.current_cascade_[_index][i+4] == 1:
                if self.take_pos_[_index][i+4] > 0 and self.prod_curr_posn_[_index][Fields.close_.value] > self.exec_px_[_index][i+4]:
                    self.sendOrder('B', _index, self.cascade_size_[self.current_cascade_[_index][i+4]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)
                    self.support_px[_index][i+4] = self.prod_curr_posn_[_index][Fields.close_.value]
                    self.take_pos_[_index][i+4] = self.take_pos_[_index][i+4] + pos_*self.cascade_size_[self.current_cascade_[_index][i+4]]
                    self.current_cascade_[_index][i+4] = self.current_cascade_[_index][i+4] +1
                    self.exec_px_[_index][i+4] = self.prod_curr_posn_[_index][Fields.close_.value]

                elif self.take_pos_[_index][i+4] < 0 and self.prod_curr_posn_[_index][Fields.close_.value] < self.exec_px_[_index][i+4]:
                    self.sendOrder('S', _index, self.cascade_size_[self.current_cascade_[_index][i+4]]*pos_, self.prod_curr_posn_[_index][Fields.close_.value],self.current_time_)
                    self.resist_px[_index][i+4] = self.prod_curr_posn_[_index][Fields.close_.value]
                    self.take_pos_[_index][i+4] = self.take_pos_[_index][i+4] - pos_*self.cascade_size_[self.current_cascade_[_index][i+4]]
                    self.current_cascade_[_index][i+4] = self.current_cascade_[_index][i+4] +1
                    self.exec_px_[_index][i+4] = self.prod_curr_posn_[_index][Fields.close_.value]


    def tradeDayMAToday(self,_prod,_index):
        pos_ = math.floor(self.max_exposure_ / self.prod_curr_posn_[_index][Fields.close_.value])
        # print(self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][3])
        for i in range(0,len(self.day_ma_lkbk_)):
            

            if self.crossover_lkbk_[_index][i] < self.crossover_limit_ and self.take_pos_[_index][i] == 0 and self.allowed_pos_[_index][i] == 0 and\
                ((self.prev_bar_close_[_index] != 0 and  self.prev_bar_close_[_index] < self.prod_curr_posn_[_index][Fields.pos.value] ) \
                or self.prev_bar_close_[_index] == 0 and self.prev_day_close_[_index] < self.prod_curr_posn_[_index][Fields.pos.value] )and \
                self.day_moving_avg_[_index][i] != 0 and self.prod_curr_posn_[_index][Fields.close_.value] > self.day_moving_avg_[_index][i]*self.ma_entry_pt_:# and\
                # self.prod_curr_posn_[_index][Fields.close_.value] < self.day_moving_avg_[_index][i]*(2*self.entry_pt_ - 1):
                for j in range(0,len(self.hour_ma_lkbk_)):
                    print(j,self.hour_moving_avg_[_index][j],self.day_moving_avg_[_index][j]," ",end='')
                print(self.crossover_lkbk_[_index][i],"")
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][i])
                self.sendOrder('B', _index, (i+1)*pos_, self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)
                self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.take_pos_[_index][i] = (i+1)*pos_

            elif self.crossover_lkbk_[_index][i] < self.crossover_limit_ and  self.take_pos_[_index][i] == 0 and self.allowed_pos_[_index][i] == 0 and\
                ((self.prev_bar_close_[_index] != 0 and  self.prev_bar_close_[_index] > self.prod_curr_posn_[_index][Fields.pos.value] ) \
                or self.prev_bar_close_[_index] == 0 and self.prev_day_close_[_index] > self.prod_curr_posn_[_index][Fields.pos.value] ) and \
                 self.day_moving_avg_[_index][i] != 0 and self.prod_curr_posn_[_index][Fields.close_.value] < self.day_moving_avg_[_index][i]/self.ma_entry_pt_: #and \
                 # self.prod_curr_posn_[_index][Fields.close_.value] > self.day_moving_avg_[_index][i]*(2*self.entry_pt_ - 1):
                
                for j in range(0,len(self.hour_ma_lkbk_)):
                    print(j,self.hour_moving_avg_[_index][j],self.day_moving_avg_[_index][j]," ",end='')
                print(self.crossover_lkbk_[_index][i],"")
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][i])
                self.sendOrder('S', _index, (i+1)*pos_, self.prod_curr_posn_[_index][Fields.close_.value],self.current_time_)
                self.resist_px[_index][i] = self.prod_curr_posn_[_index][Fields.close_.value]
                self.take_pos_[_index][i] = -1*(i+1)*pos_


    def tradeReverseDayMA(self,_prod,_index):
        pos_ = math.floor(self.max_exposure_ / self.prod_curr_posn_[_index][Fields.close_.value])
        # print(self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][3])
        # for j in range(0,len(self.hour_ma_lkbk_)):
        #     print(j,self.hour_moving_avg_[_index][j],self.day_moving_avg_[_index][j]," ",end='')
        # print("")
        # print(self.crossover_lkbk_[_index][i],"")
        for i in range(0,len(self.day_ma_lkbk_)):
            if self.take_pos_[_index][i] == 0 and self.allowed_pos_[_index][i] == 0 and\
            self.prod_curr_posn_[_index][Fields.high_.value] > self.day_moving_avg_[_index][i] and self.prod_curr_posn_[_index][Fields.close_.value] < self.day_moving_avg_[_index][i]/self.ma_entry_pt_:
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][i])
                self.sendOrder('S', _index, (i+1)*pos_, self.prod_curr_posn_[_index][Fields.close_.value],self.current_time_)
                self.resist_px[_index][i] = self.prod_curr_posn_[_index][Fields.low_.value]
                self.take_pos_[_index][i] = -1*(i+1)*pos_
                # self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1

            elif self.take_pos_[_index][i] == 0 and self.allowed_pos_[_index][i] == 0 and\
                self.prod_curr_posn_[_index][Fields.low_.value] < self.day_moving_avg_[_index][i] and self.prod_curr_posn_[_index][Fields.close_.value] > self.day_moving_avg_[_index][i]*self.ma_entry_pt_:
                # for j in range(0,len(self.hour_ma_lkbk_)):
                #     print(j,self.hour_moving_avg_[_index][j],self.day_moving_avg_[_index][j]," ",end='')
                # print(self.crossover_lkbk_[_index][i],"")
                print(i,self.prod_curr_posn_[_index][Fields.close_.value],self.prev_bar_close_[_index],  self.prev_day_close_[_index],self.day_moving_avg_[_index][i])
                self.sendOrder('B', _index, (i+1)*pos_, self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)
                self.support_px[_index][i] = self.prod_curr_posn_[_index][Fields.high_.value]
                self.take_pos_[_index][i] = (i+1)*pos_
                # self.current_cascade_[_index][i] = self.current_cascade_[_index][i] +1

    def onExec(self, _index, _size, _price, _cost, _time):
        StratFramework.onExec(self, _index, _size, _price, _cost, _time)
        prod_row_ = self.prod_curr_posn_[_index]
        prod_row_[Fields.cascade_trigger.value] = False
        # self.updateTrailingSL(_index)

    def checkSL(self,_index,_price):
        prod_ = self.product_list_[_index]
        for i in range(0,len(self.day_ma_lkbk_)):
            if self.take_pos_[_index][i] > 0 and (self.prod_curr_posn_[_index][Fields.close_.value] < self.support_px[_index][i]/self.ma_exit_pt_):# or \
                # (self.day_max_[_index] != -1 and  2*self.prod_curr_posn_[_index][Fields.high_.value] > (self.day_moving_avg_[_index][i]*self.ma_exit_pt_ + self.day_moving_avg_[_index][self.day_max_[_index]]) \
                # and  self.current_high_[_index] < self.prod_curr_posn_[_index][Fields.high_.value])):
                # (self.day_max_[_index] != -1 and self.prod_curr_posn_[_index][Fields.high_.value] >= self.day_moving_avg_[_index][self.day_max_[_index]])):
                print("SL HIT(+ve pos) at " + str(self.current_time_) + " "  + " Strat" + str(self.strat_id_) + str(_price) + " for " + prod_ + " day " + str(i))
                self.sendOrder('S', _index, self.take_pos_[_index][i], self.prod_curr_posn_[_index][Fields.close_.value],self.current_time_)
                self.take_pos_[_index][i] = 0
                self.allowed_pos_[_index][i] = -1
                self.current_cascade_[_index][i] = 0
                # self.prod_curr_posn_[_index][Fields.sqoff_on.value] =  True
                # self.prod_curr_posn_[_index][Fields.start_trading.value] =  False
                # self.squareOff(_index,self.prod_curr_posn_[_index][Fields.close_.value])

            elif  self.take_pos_[_index][i] < 0 and (self.prod_curr_posn_[_index][Fields.close_.value] > self.resist_px[_index][i]*self.ma_exit_pt_ ):#or \
                # (self.day_min_[_index] != -1 and 2*self.prod_curr_posn_[_index][Fields.low_.value] < (self.day_moving_avg_[_index][i]/self.ma_exit_pt_ + self.day_moving_avg_[_index][self.day_min_[_index]])\
                    # and self.current_low_[_index] > self.prod_curr_posn_[_index][Fields.low_.value])):
                # (self.day_min_[_index] != -1 and self.prod_curr_posn_[_index][Fields.low_.value]  <= self.day_moving_avg_[_index][self.day_min_[_index]])):
                print("SL HIT(-ve pos) at " + str(self.current_time_) + " "  + " Strat" + str(self.strat_id_) + str(_price) + " for " + prod_ + " day " + str(i))
                self.sendOrder('B', _index, -1*self.take_pos_[_index][i], self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)  
                self.take_pos_[_index][i] = 0
                self.allowed_pos_[_index][i] = -1
                self.current_cascade_[_index][i] = 0
                # self.prod_curr_posn_[_index][Fields.sqoff_on.value] =  True
                # self.prod_curr_posn_[_index][Fields.start_trading.value] =  False
                # self.squareOff(_index,self.prod_curr_posn_[_index][Fields.close_.value])


        for i in range(0,len(self.hour_ma_lkbk_)):
            if self.take_pos_[_index][i+4] > 0 and self.prod_curr_posn_[_index][Fields.close_.value] < self.support_px[_index][i+4]/self.hma_exit_pt_:
                print("SL HIT(+ve pos) at " + str(self.current_time_) + " "  + " Strat" + str(self.strat_id_) + str(_price) + " for " + prod_ + " day " + str(i+4))
                self.sendOrder('S', _index, self.take_pos_[_index][i+4], self.prod_curr_posn_[_index][Fields.close_.value],self.current_time_)
                self.take_pos_[_index][i+4] = 0
                self.allowed_pos_[_index][i+4] = -1
                # self.prod_curr_posn_[_index][Fields.sqoff_on.value] =  True
                # self.prod_curr_posn_[_index][Fields.start_trading.value] =  False
                # self.squareOff(_index,self.prod_curr_posn_[_index][Fields.close_.value])

            elif  self.take_pos_[_index][i+4] < 0 and self.prod_curr_posn_[_index][Fields.close_.value] > self.resist_px[_index][i+4]*self.hma_exit_pt_:
                print("SL HIT(-ve pos) at " + str(self.current_time_) + " "  + " Strat" + str(self.strat_id_) + str(_price) + " for " + prod_ + " day " + str(i+4))
                self.sendOrder('B', _index, -1*self.take_pos_[_index][i+4], self.prod_curr_posn_[_index][Fields.close_.value], self.current_time_)  
                self.take_pos_[_index][i+4] = 0
                self.allowed_pos_[_index][i+4] = -1
                # self.prod_curr_posn_[_index][Fields.sqoff_on.value] =  True
                # self.prod_curr_posn_[_index][Fields.start_trading.value] =  False
                # self.squareOff(_index,self.prod_curr_posn_[_index][Fields.close_.value])

    def updateTrailingSL(self, _index):
        prod_ = self.product_list_[_index]
        for i in range(0,len(self.day_ma_lkbk_)):
            # if self.take_pos_[_index][i] > 0:
            #     self.support_px[_index][i] = max(self.support_px[_index][i],self.prod_curr_posn_[_index][Fields.close_.value])
            # elif self.take_pos_[_index][i] < 0:
            #     self.resist_px[_index][i] = min(self.resist_px[_index][i],self.prod_curr_posn_[_index][Fields.close_.value])


            if self.take_pos_[_index][i] > 0:
                self.support_px[_index][i] = max(self.support_px[_index][i],self.prod_curr_posn_[_index][Fields.high_.value])
            elif self.take_pos_[_index][i] < 0:
                self.resist_px[_index][i] = min(self.resist_px[_index][i],self.prod_curr_posn_[_index][Fields.low_.value])


        for i in range(0,len(self.hour_ma_lkbk_)):
            # if self.take_pos_[_index][i] > 0:
            #     self.support_px[_index][i] = max(self.support_px[_index][i],self.prod_curr_posn_[_index][Fields.close_.value])
            # elif self.take_pos_[_index][i] < 0:
            #     self.resist_px[_index][i] = min(self.resist_px[_index][i],self.prod_curr_posn_[_index][Fields.close_.value])


            if self.take_pos_[_index][i+4] > 0:
                self.support_px[_index][i+4] = max(self.support_px[_index][i+4],self.prod_curr_posn_[_index][Fields.high_.value])
            elif self.take_pos_[_index][i+4] < 0:
                self.resist_px[_index][i+4] = min(self.resist_px[_index][i+4],self.prod_curr_posn_[_index][Fields.low_.value])
