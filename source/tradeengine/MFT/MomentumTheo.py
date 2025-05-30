import pandas as pd
import math
from StratFramework import StratFramework, Fields
from BarGenerator import BarGenerator
from dateutil.rrule import DAILY, rrule, MO, TU, WE, TH, FR
from datetime import date,timedelta,datetime
import math
import numpy as np
# import datetime


class MomentumTheo(StratFramework):

	def __init__(self,_om,_live_file,_granularity,_train_start_day,_train_end_day,_config_file):
		#Initializations
		StratFramework.__init__(self,_om,_live_file,_granularity)
		lot_file_ = "/home/nishitbhandari/Downloads/mktlots"
		self.prod_pos_ = pd.DataFrame(columns=["prod","pos"])

		earnings_file_ = "/home/nishitbhandari/Downloads/consolidated_earnings"

		with open(earnings_file_, "r") as input_file:
		    self.earnings_list_ = (input_file.readlines())
		self.earnings_list_ = [x.strip() for x in self.earnings_list_]

		self.earnings_index_ = 0
		# self.prod_pos_["pos"] = self.prod_pos_["pos"].astype(int)
		# with open(lot_file_, "r") as input_file:
		# 	for line in input_file:
		# 		row = line.split(',')
		# 		self.prod_pos_.loc[row[1].strip()] = [row[1].strip(),row[2].strip()]
				# print(row[1].strip(),row[2].strip())
		self.prev_day_earnings_ = [False]*len(self.product_list_)
		self.train_start_ = _train_start_day
		self.train_end_ = _train_end_day
		self.training_data_ = []
		self.oldest_bar_ = []
		self.moment_oc_wt_mean_ = [0]*len(self.product_list_)
		self.moment_oc_wt_median_ = [0]*len(self.product_list_)
		self.moment_oc_wt_stdev_ = [0]*len(self.product_list_)
		self.prev_day_close_ = [0]*len(self.product_list_)
		self.moment_cc_wt_mean_ = [0]*len(self.product_list_)
		self.moment_cc_wt_median_ = [0]*len(self.product_list_)
		self.moment_cc_wt_stdev_ = [0]*len(self.product_list_)
		self.moment_cc_wt_tl_mean_ = [0]*len(self.product_list_)
		self.moment_cc_wt_tl_stdev_ = [0]*len(self.product_list_)
		# self.moment_cc_wt_capped_stdev_ = [0]*len(self.product_list_)
		self.moment_cc_wt_capped_ = [0]*len(self.product_list_)
		self.current_low_ = [0]*len(self.product_list_)
		self.current_high_ = [0]*len(self.product_list_)
		self.closing_bias_ = [0]*len(self.product_list_)
		self.current_cascade_ = [0]*len(self.product_list_)
		self.reentry_px_ = [0]*len(self.product_list_)
		self.sqoff_px_ = [0]*len(self.product_list_)
		self.bar_pos_ = [0]*len(self.product_list_)
		self.current_obv_ = [0]*len(self.product_list_)
		self.net_opp_vol_ = [0]*len(self.product_list_)
		self.entry_obv_ = [0]*len(self.product_list_)
		self.last_pos_obv_ = [0]*len(self.product_list_)
		self.last_bar_ltp_ = [0]*len(self.product_list_)
		self.vol_sqoff_ = [False]*len(self.product_list_)
		self.last_trade_ltp_ = [0]*len(self.product_list_)
		self.closing_bias_current_ = [0]*len(self.product_list_)
		self.current_time_ = 0
		self.current_date_ = 0
		self.cascade_num_ = 0
		self.cascade_size_ = []
		# self.close_lh_ = np.repeat([[10000000,-1]],len(self.product_list_),axis=0)
		# self.lh_ = np.repeat([[10000000,-1]],len(self.product_list_),axis=0)
		self.lt_std_ = [0]*len(self.product_list_)
		self.mom_obv_std_ = [0]*len(self.product_list_)
		self.open_vol_avg_ = [np.NaN]*len(self.product_list_)
		self.open_vol_mean_ = [0]*len(self.product_list_)
		self.open_vol_std_ = [0]*len(self.product_list_)
		self.filter_std_list_ = []
		for prod_ in self.product_list_:
			# if prod_ in self.prod_pos_.index:
			# 	print("Lotsize",prod_, self.prod_pos_.loc[prod_]["pos"])
			self.training_data_.append(pd.DataFrame(columns=["datetime","open","close","low","high","vol","c-o","c-c","close_bias"]))
			self.training_data_[len(self.training_data_)-1][["open","close","low","high","vol","c-o","c-c","close_bias"]] = self.training_data_[len(self.training_data_)-1][["open","close","low","high","vol","c-o","c-c","close_bias"]].astype(float)
		#Parameter Input
		with open(_config_file, "r") as main_file:
			for line in main_file:
				row = str.split(line)
				print(row)
				if row[0] == "TRAILING_STOP_LOSS_PERCENT":
					self.trailing_pt_ = float(row[2])
				if row[0] == "MAX_EXPOSURE":
					self.max_exposure_ = float(row[2])
				if row[0] == "ALPHA":
					self.moment_oc_alpha_ = float(row[2])
					self.moment_cc_alpha_ = self.moment_oc_alpha_
				if row[0] == "BETA":
					self.moment_oc_beta_ = float(row[2])
					self.moment_cc_beta_ = self.moment_oc_beta_
				if row[0] == "TIME_WT":
					self.time_wt_ = float(row[2])
				if row[0] == "POS_WT":
					self.pos_wt_ = float(row[2])
				if row[0] == "CASCADE_NUM":
					self.cascade_num_ = float(row[2]) 
					self.cascade_size_ = [1.0]*int(self.cascade_num_)
				if row[0] == "FIRST_CASCADE":
					self.cascade_size_[0] = float(row[2])
				if row[0] == "DECAY_FACTOR":
					self.decay_factor_ = float(row[2])
				if row[0] == "VOLATILITY_LOOKBACK":
					self.lt_days_ = int(row[2])
				if row[0] == "MOM_VOLATILITY_LOOKBACK":
					self.mom_lt_days_ = int(row[2])
				if row[0] == "FILTER_PTILE":
					self.filter_ptile_ = float(row[2])
				if row[0] == "MOM_FILTER_PTILE":
					self.mom_filter_ptile_ = float(row[2])
				if row[0] == "TRAIN_START_TIME":
					self.train_start_time_ = row[2]
				if row[0] == "TRAIN_END_TIME":
					self.train_end_time_ = row[2]
				if row[0] == "STRAT_START_TIME":
					self.strat_start_time_ = row[2]
				if row[0] == "STRAT_END_TIME":
					self.strat_end_time_ = row[2]
				if row[0] == "CLOSING_BIAS_THRESH":
					self.close_thresh_ = float(row[2])
				if row[0] == "OPEN_VOL_PTILE":
					self.open_vol_ptile_ = float(row[2])
				if row[0] == "OPEN_VOL_LOOKBK":
					self.open_vol_days_ = int(row[2])
				if row[0] == "HEDGE_CASCADE":
					self.hedge_cascade_ = float(row[2])
				if row[0] == "TSL_GRANULARITY":
					self.tsl_granularity_ = float(row[2])*60
				if row[0] == "VOL_SQOFF_CASCADE":
					self.vol_sqoff_cascade_ = int(row[2])


		self.long_term_vol_ = np.zeros(self.lt_days_*len(self.product_list_)).reshape(len(self.product_list_),self.lt_days_)
		self.long_term_mom_obv_ = np.zeros(self.mom_lt_days_*len(self.product_list_)).reshape(len(self.product_list_),self.mom_lt_days_)
		self.opening_bar_volume_ = np.zeros(self.open_vol_days_*len(self.product_list_)).reshape(len(self.product_list_),self.open_vol_days_)
		self.day_volume_hist_ = np.zeros(self.open_vol_days_*len(self.product_list_)).reshape(len(self.product_list_),self.open_vol_days_)
		self.long_term_vol_[:] = np.NaN
		self.long_term_mom_obv_[:] = np.NaN
		self.opening_bar_volume_[:] = np.NaN
		self.day_volume_hist_[:] = np.NaN
		self.lt_count_ = [0]*len(self.product_list_)
		self.mom_lt_count_ = [0]*len(self.product_list_)
		self.open_vol_count_ = [0]*len(self.product_list_)
		self.nifty_hedge_pos_ = 0
		self.last_bar_update_ = 0
		self.bar_agg_ = np.zeros(len(self.product_list_) *5).reshape(len(self.product_list_),5)
		self.day_bar_agg_ = np.zeros(len(self.product_list_) *5).reshape(len(self.product_list_),5)
		self.current_vol_ = [0]*len(self.product_list_)
		self.day_vol_avg_ = [np.NaN]*len(self.product_list_)
		self.vwap_px_ = [0]*len(self.product_list_)
		self.min_pnl_day_ = [10000000]*len(self.product_list_)
		self.net_vol_ = [0]*len(self.product_list_)
		self.long_wt_cc_mean_ = [0]*len(self.product_list_)
		self.long_wt_cc_std_ = [0]*len(self.product_list_)
		self.open_obv_ = [0]*len(self.product_list_)
		self.current_bar_obv_ = [0]*len(self.product_list_)
		# print(self.trailing_pt_,self.max_exposure_,self.moment_oc_alpha_,self.time_wt_,self.pos_wt_)
		# self.time_wt_ = 0.5
		# self.pos_wt_ = 0.5
		# self.moment_oc_alpha_ = 1.25
		# # self.price_delta_ = 0.1
		# self.max_exposure_ = 500000
		# self.trailing_pt_ = 0.005
	#Get Bar from BarGenerator	
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

  	#Called before the first trading day in date range.Initializes dataframe with values
	def getTrainingBar(self,_method):
		counter_ = 0
		for index_ in range(0,len(self.product_list_)):
			prod_ = self.product_list_[index_]
			flag_ = 0
			start_ = self.train_start_time_.split('_')
			end_ = self.train_end_time_.split('_')
			for date_ in self.working_days_:
				day_start_epoch_ = int(date_.replace(hour=int(start_[0]),minute=int(start_[1])).strftime('%s'))  			
				day_end_epoch_ = int(date_.replace(hour=int(end_[0]),minute=int(end_[1])).strftime('%s'))  	
				[o,c,l,h,v] = _method(day_start_epoch_,day_end_epoch_,counter_)
				if [o,c,l,h] != [0,0,0,0]:
					flag_ =1
					# index_ = str(prod_+"_"+str(date_.date()))
					if self.prev_day_close_[index_]:
						self.training_data_[counter_].loc[len(self.training_data_[counter_])] = [day_start_epoch_,o,c,l,h,v,abs(c-o),abs(c-self.prev_day_close_[index_]),abs((c-o)/(h-l + 0.00000001))]
						self.long_term_vol_[counter_][self.lt_count_[counter_]] = abs(c-self.prev_day_close_[index_])
						self.long_term_mom_obv_[counter_][self.mom_lt_count_[counter_]] = abs(self.net_vol_[index_]/v)/self.prev_day_close_[index_]

						self.lt_count_[counter_] = self.lt_count_[counter_] + 1
						self.mom_lt_count_[counter_] = self.mom_lt_count_[counter_] + 1
					else:
						self.training_data_[counter_].loc[len(self.training_data_[counter_])] = [day_start_epoch_,o,c,l,h,v,abs(c-o),np.NaN,abs((c-o)/(h-l+0.00000001))]
					self.prev_day_close_[index_] = c
					# self.closing_bias_[index_] = abs((c-o)/(h-l))
			if not flag_:
				self.prod_curr_posn_[index_][Fields.start_trading.value] =  True
			counter_ = counter_ + 1

			self.oldest_bar_.append(0)
		# print(self.training_data_)
		self.getMeanMomentCC()

	#Called eod to update dataframe with bar/day data for that day
	#Involves computing day price actions and filters to be employed for the next day
	def updateTrainingBar(self,_prod_bar_info):
		counter_ = 0
		for index_ in range(0,len(self.product_list_)):
			prod_ = self.product_list_[index_]
			flag_ = 0
			[o,c,l,h,v] = _prod_bar_info[counter_]
			if [o,c,l,h] != [0,0,0,0]:
				flag_ =1
				if len(self.working_days_) > len(self.training_data_[counter_]):
					self.training_data_[counter_].loc[len(self.training_data_[counter_])] = [self.om_.started_at_,o,c,l,h,v,abs(c-o),abs(c-self.prev_day_close_[index_]),abs((c-o)/(h-l))]
				else:
					self.training_data_[counter_].set_value(self.oldest_bar_[counter_], "datetime",self.om_.started_at_)
					self.training_data_[counter_].set_value(self.oldest_bar_[counter_], "open",o)
					self.training_data_[counter_].set_value(self.oldest_bar_[counter_], "close",c)
					self.training_data_[counter_].set_value(self.oldest_bar_[counter_], "low",l)
					self.training_data_[counter_].set_value(self.oldest_bar_[counter_], "high",h)
					self.training_data_[counter_].set_value(self.oldest_bar_[counter_], "vol",v)
					self.training_data_[counter_].set_value(self.oldest_bar_[counter_], "c-o",abs(c-o))
					self.training_data_[counter_].set_value(self.oldest_bar_[counter_], "c-c",abs(c-self.prev_day_close_[index_]))
					self.training_data_[counter_].set_value(self.oldest_bar_[counter_], "close_bias",abs((c-o)/(h-l)))
					# self.training_data_[counter_].set_value(self.oldest_bar_[counter_], "open_vol",self.pos_pnl_df_[index_][self.day_index_-1][0][10])

					self.oldest_bar_[counter_] = (self.oldest_bar_[counter_] + 1)%len(self.training_data_[counter_])
			
				self.long_term_vol_[index_][self.lt_count_[index_]%self.lt_days_] = abs(c-self.prev_day_close_[index_])
				self.long_term_mom_obv_[index_][self.mom_lt_count_[index_]%self.mom_lt_days_] = abs(self.net_vol_[index_]/v)/self.prev_day_close_[index_]
				self.prev_day_close_[index_] = c	
			else:
				self.long_term_vol_[index_][self.lt_count_[index_]%self.lt_days_] = np.NaN	
				self.long_term_mom_obv_[index_][self.mom_lt_count_[index_]%self.mom_lt_days_] = np.NaN	
				# self.opening_bar_volume_[index_][self.open_vol_count_[index_]%self.open_vol_days_] = np.NaN			

			self.lt_count_[index_] = self.lt_count_[index_] + 1
			self.mom_lt_count_[index_] = self.mom_lt_count_[index_] + 1
			# self.open_vol_count_[index_] = self.open_vol_count_[index_] + 1

			if not flag_ and self.prod_curr_posn_[index_][Fields.start_trading.value]:
				self.prod_curr_posn_[index_][Fields.start_trading.value] =  True
			# self.lt_std_[index_] = np.nanstd(self.long_term_vol_[index_])/self.prev_day_close_[index_]
			self.lt_std_[index_] = np.nanmean(self.long_term_vol_[index_])/self.prev_day_close_[index_]
			self.mom_obv_std_[index_] = np.nanstd(self.long_term_mom_obv_[index_])

			self.open_vol_avg_[index_] = np.nanpercentile(self.opening_bar_volume_[index_],self.open_vol_ptile_)
			self.day_vol_avg_[index_] = np.nanpercentile(self.day_volume_hist_[index_],1.5*self.open_vol_ptile_)

			# print("dayvol",self.day_vol_avg_,self.day_volume_hist_)
			# self.open_vol_mean_[index_] = np.nanmean(self.opening_bar_volume_[index_])
			# self.open_vol_std_[index_] = np.nanstd(self.opening_bar_volume_[index_])
			counter_ = counter_ + 1

		ptl_std_ = np.percentile(self.lt_std_,self.filter_ptile_)
		ptl_std_mom_ = np.percentile(self.mom_obv_std_,self.mom_filter_ptile_)

		self.filter_std_list_.append(ptl_std_)

		for index_ in range(0,len(self.product_list_)):
			# print("LTOBV ",self.lt_std_[index_],self.mom_obv_std_[index_])
			if self.lt_std_[index_]< ptl_std_ and self.lt_count_[index_] >=self.lt_days_:
				# print("LTV",self.product_list_[index_],self.lt_std_[index_], ptl_std_)
				self.prod_curr_posn_[index_][Fields.start_trading.value] =  False
		# for index_ in range(0,len(self.product_list_)):
			if self.mom_obv_std_[index_]< ptl_std_mom_ and self.mom_lt_count_[index_] >=self.mom_lt_days_:
				# print("OBV",self.product_list_[index_],self.mom_obv_std_[index_],ptl_std_mom_)
				self.prod_curr_posn_[index_][Fields.start_trading.value] =  False
		# print(self.training_data_)
		# print("Closing bias ", self.closing_bias_)
		self.getMeanMomentCC()
	

	#Compute Weighted mean and deviation of historic c-c
	def getMeanMomentCC(self):
		for counter_ in range(0,len(self.product_list_)):
			col = self.training_data_[counter_]["c-c"]
			self.moment_cc_wt_mean_[counter_] =  self.moment_cc_beta_*col.mean()
			self.moment_cc_wt_median_[counter_] =  self.moment_cc_beta_*col.median()
			self.moment_cc_wt_stdev_[counter_] =  self.moment_cc_alpha_*col.std()
			self.long_wt_cc_mean_[counter_] = np.nanmean(self.long_term_vol_[counter_])
			self.long_wt_cc_std_[counter_] = np.nanstd(self.long_term_vol_[counter_])


	#Responsible for bar creation after receiving input from BarGenerator and sending the bar to onbarupdate	
	def aggregator(self,_prod_bar_info,_time):
		if self.current_date_ == 0:
			self.current_date_ =  int(datetime.fromtimestamp(_time).strftime('%Y%m%d'))
			while self.earnings_index_ < len(self.earnings_list_) and int(self.earnings_list_[self.earnings_index_].split("\t")[0]) < self.current_date_:
				self.earnings_index_ = self.earnings_index_ + 1

		if self.last_bar_update_ == 0:
			self.last_bar_update_ = self.om_.started_at_
			# print("start",self.last_bar_update_)

		# print(_time,_prod_bar_info,"\n")
		for index_ in range(0,len(self.product_list_)):
			open_,close_,low_,high_,vol_ = _prod_bar_info[index_]
			if self.bar_agg_[index_][0] == 0 and vol_:
				self.bar_agg_[index_] = _prod_bar_info[index_]
				self.bar_agg_[index_][4] = vol_
			else:
				if vol_:
					self.bar_agg_[index_][1] = close_
					self.bar_agg_[index_][2] = min(self.bar_agg_[index_][2],low_)
					self.bar_agg_[index_][3] = max(self.bar_agg_[index_][3],high_)
					self.bar_agg_[index_][4] = self.bar_agg_[index_][4] + vol_

			if self.day_bar_agg_[index_][0] == 0 and vol_:
				self.day_bar_agg_[index_] = _prod_bar_info[index_]
				self.day_bar_agg_[index_][4] = vol_

			else:
				if vol_:
					self.day_bar_agg_[index_][1] = close_
					self.day_bar_agg_[index_][2] = min(self.day_bar_agg_[index_][2],low_)
					self.day_bar_agg_[index_][3] = max(self.day_bar_agg_[index_][3],high_)
					self.day_bar_agg_[index_][4] = self.day_bar_agg_[index_][4] + vol_

		if _time == self.last_bar_update_ + self.granularity_ or  _time-self.om_.ended_at_ == (datetime.strptime(self.strat_end_time_,"%H_%M")- datetime.strptime(self.train_end_time_,"%H_%M")).total_seconds():
			self.last_bar_update_ = _time
			if _time-self.om_.started_at_ >= (datetime.strptime(self.strat_start_time_,"%H_%M")- datetime.strptime(self.train_start_time_,"%H_%M")).total_seconds() \
				and _time-self.om_.ended_at_ <= (datetime.strptime(self.strat_end_time_,"%H_%M")- datetime.strptime(self.train_end_time_,"%H_%M")).total_seconds():
				self.onBarUpdate(self.bar_agg_,_time)
				# print(self.bar_agg_)
			for index_ in range(0,len(self.product_list_)):
				# if self.bar_agg_[index_][1] > self.bar_agg_[index_][0]:
				self.net_vol_[index_] += (self.bar_agg_[index_][1] - self.bar_agg_[index_][0])*self.bar_agg_[index_][4] 
			self.bar_agg_ = np.zeros(len(self.product_list_) *5).reshape(len(self.product_list_),5)
			# print(self.bar_agg_)

		#Happens at EOD resetting variables
		if _time-self.om_.started_at_ == (datetime.strptime(self.train_end_time_,"%H_%M") - datetime.strptime(self.train_start_time_,"%H_%M")).total_seconds():
			# print("the end")
			for index_ in range(0,len(self.product_list_)):
				self.day_volume_hist_[index_][(self.open_vol_count_[index_]+self.open_vol_days_-1)%self.open_vol_days_] = self.day_bar_agg_[index_][4] 

			self.current_cascade_ = [0]*len(self.product_list_)
			self.last_bar_update_ = 0
			self.reset()
			self.updateTrainingBar(self.day_bar_agg_)
			self.day_bar_agg_ = np.zeros(len(self.product_list_) *5).reshape(len(self.product_list_),5)
			self.bar_agg_ = np.zeros(len(self.product_list_) *5).reshape(len(self.product_list_),5)
			self.current_low_ = [0]*len(self.product_list_)
			self.current_high_ = [0]*len(self.product_list_)
			self.reentry_px_ = [0]*len(self.product_list_)
			self.bar_pos_ = [0]*len(self.product_list_)
			self.current_obv_ = [0]*len(self.product_list_)
			self.entry_obv_ = [0]*len(self.product_list_)
			self.last_pos_obv_ = [0]*len(self.product_list_)
			self.current_vol_ = [0]*len(self.product_list_)
			self.last_bar_ltp_ = [0]*len(self.product_list_)
			self.net_opp_vol_ = [0]*len(self.product_list_)
			self.vol_sqoff_ = [False]*len(self.product_list_)
			self.min_pnl_day_ = [10000000]*len(self.product_list_)
			self.vwap_px_ = [0]*len(self.product_list_)
			self.last_trade_ltp_ = [0]*len(self.product_list_)
			self.net_vol_ = [0]*len(self.product_list_)
			self.closing_bias_current_ = [0]*len(self.product_list_)
			self.current_date_ = 0
			self.open_obv_ = [0]*len(self.product_list_)
			self.current_bar_obv_ = [0]*len(self.product_list_)


	#Called from aggregator and handles main strat execution and reaction to granular bar
	def onBarUpdate(self,_prod_bar_info,_time):
		self.current_time_ = _time
		for index_ in range(0,len(self.product_list_)):
			prod_ = self.product_list_[index_]
			self.vol_sqoff_[index_] = False
			self.closing_bias_current_[index_] = abs(self.day_bar_agg_[index_][1] - self.day_bar_agg_[index_][0])/(self.day_bar_agg_[index_][3] - self.day_bar_agg_[index_][2] + 0.0000001)
			open_,close_,low_,high_,vol_ = _prod_bar_info[index_]
			# print("init bar",self.current_time_,open_,close_,low_,high_,vol_)
			if self.current_time_ -self.om_.started_at_ == self.granularity_ :
				if vol_:
					self.opening_bar_volume_[index_][self.open_vol_count_[index_]%self.open_vol_days_] = vol_
					self.open_obv_[index_] = np.abs(close_-open_)*vol_
				else:
					self.opening_bar_volume_[index_][self.open_vol_count_[index_]%self.open_vol_days_] = np.NaN
					self.open_obv_[index_] = np.NaN
					
				self.open_vol_count_[index_] = self.open_vol_count_[index_] +1
			if self.prod_curr_posn_[index_][Fields.start_trading.value]:
				
				if vol_:
					self.vwap_px_[index_] = ((self.day_bar_agg_[index_][4]-vol_)*self.vwap_px_[index_] + vol_*close_)/self.day_bar_agg_[index_][4]
					self.current_obv_[index_] = self.current_obv_[index_] + (close_ - open_)*vol_ 
					# print("init bar",self.current_time_,open_,close_,low_,high_,vol_, self.vwap_px_[index_])
					pnl_ = self.prod_curr_posn_[index_][Fields.pnl.value] + self.prod_curr_posn_[index_][Fields.pos.value]*(close_ - self.prod_curr_posn_[index_][Fields.ltp.value])
					if pnl_ < self.min_pnl_day_[index_]:
						self.min_pnl_day_[index_] = pnl_
					if pnl_ > self.prod_curr_posn_[index_][Fields.pnl.value] and pnl_>=0:

					#Possible cascading conditions
					# if pnl_ > self.prod_curr_posn_[index_][Fields.pnl.value] and self.prod_curr_posn_[index_][Fields.pos.value]*(close_ - self.vwap_px_[index_]) < 0:
					# and (pnl_ < self.current_cascade_[index_]*self.prod_curr_posn_[index_][Fields.pnl.value] or self.current_cascade_[index_] == 1):
					# and (abs(close_ - self.prod_curr_posn_[index_][Fields.ltp.value]) < 5*abs(self.prod_curr_posn_[index_][Fields.ltp.value] - self.last_bar_ltp_[index_]) or self.current_cascade_[index_] == 1 or self.last_bar_ltp_[index_] == 0):
					# if pnl_ > self.prod_curr_posn_[index_][Fields.pnl.value] and abs(close_ - self.prod_curr_posn_[index_][Fields.ltp.value]) > self.current_cascade_[index_]*self.moment_cc_wt_stdev_[index_]*0.5/self.cascade_num_:
					# if pnl_ > self.prod_curr_posn_[index_][Fields.pnl.value] and (self.current_obv_[index_] - self.last_pos_obv_[index_])*self.prod_curr_posn_[index_][Fields.pos.value] > 0:
						self.prod_curr_posn_[index_][Fields.cascade_trigger.value] = True
						self.net_opp_vol_[index_] = 0
					
					self.prod_curr_posn_[index_][Fields.pnl.value] = pnl_
					self.last_bar_ltp_[index_] =  self.prod_curr_posn_[index_][Fields.ltp.value]
					self.prod_curr_posn_[index_][Fields.ltp.value] = close_
					self.prod_curr_posn_[index_][Fields.last_updated.value] =  _time
					self.prod_curr_posn_[index_][Fields.close_.value] = close_
					if self.prod_curr_posn_[index_][Fields.open_.value] == 0:
						self.prod_curr_posn_[index_][Fields.open_.value] = open_
						self.prod_curr_posn_[index_][Fields.low_.value] = low_
						self.prod_curr_posn_[index_][Fields.high_.value] = high_
					else:
						if self.prod_curr_posn_[index_][Fields.low_.value] > low_:
							self.prod_curr_posn_[index_][Fields.low_.value] = low_
						if self.prod_curr_posn_[index_][Fields.high_.value] < high_:
							self.prod_curr_posn_[index_][Fields.high_.value] = high_
					self.current_low_[index_] = low_
					self.current_high_[index_] = high_
					self.current_vol_[index_] = vol_
					self.current_bar_obv_[index_] = np.abs(close_-open_)*vol_
					if self.prod_curr_posn_[index_][Fields.pos.value] > 0:
						self.bar_pos_[index_] = 1
						# if vol_ < self.opening_bar_volume_[index_][(self.open_vol_count_[index_]-1)%self.open_vol_days_] or \
						# 	high_ != self.prod_curr_posn_[index_][Fields.high_.value]:
						self.updateTrailingSL(index_,self.prod_curr_posn_[index_][Fields.high_.value])
						# else:
						# 	self.prod_curr_posn_[index_][Fields.cascade_trigger.value] = False

						# self.updateTrailingSL(index_,self.current_high_[index_])
						self.checkSL(index_,self.current_low_[index_])					
					elif self.prod_curr_posn_[index_][Fields.pos.value] < 0:
						self.bar_pos_[index_] = -1
						# if vol_ < self.opening_bar_volume_[index_][(self.open_vol_count_[index_]-1)%self.open_vol_days_] or \
						# 	low_ != self.prod_curr_posn_[index_][Fields.low_.value]:
						self.updateTrailingSL(index_,self.prod_curr_posn_[index_][Fields.low_.value])
						# else:
							# self.prod_curr_posn_[index_][Fields.cascade_trigger.value] = False
						# self.updateTrailingSL(index_,self.current_low_[index_])
						self.checkSL(index_,self.current_high_[index_])
				else:
					self.current_vol_[index_] = np.NaN
				self.pos_pnl_df_[index_][self.day_index_].append([prod_,_time,self.prod_curr_posn_[index_][Fields.pos.value],self.prod_curr_posn_[index_][Fields.pnl.value]
					,self.prod_curr_posn_[index_][Fields.ltp.value],self.prod_curr_posn_[index_][Fields.slippage_cost.value],
					open_,close_,low_,high_,vol_,0])
				# print(prod_,_time,self.prod_curr_posn_[index_][Fields.pos.value],self.prod_curr_posn_[index_][Fields.pnl.value],self.prod_curr_posn_[index_][Fields.ltp.value],self.prod_curr_posn_[index_][Fields.slippage_cost.value])
				# self.checkSL(index_,self.prod_curr_posn_[index_][Fields.ltp.value])
			else:
				self.pos_pnl_df_[index_][self.day_index_].append([prod_,_time,self.prod_curr_posn_[index_][Fields.pos.value],self.prod_curr_posn_[index_][Fields.pnl.value]
					,self.prod_curr_posn_[index_][Fields.ltp.value],self.prod_curr_posn_[index_][Fields.slippage_cost.value],
					open_,close_,low_,high_,vol_,0])				
				# print(prod_,_time,self.prod_curr_posn_[index_][Fields.pos.value],self.prod_curr_posn_[index_][Fields.pnl.value],self.prod_curr_posn_[index_][Fields.ltp.value],self.prod_curr_posn_[index_][Fields.slippage_cost.value])
	# if self.get_flat_:
	# 	return 
		

		if self.current_time_-self.om_.ended_at_ >= (datetime.strptime(self.strat_end_time_,"%H_%M")- datetime.strptime(self.train_end_time_,"%H_%M")).total_seconds():
			self.getFlat()
			return
		self.takeDecision()

	#calls relevant alpha which takes the decision to buy/sell the position
	def takeDecision(self):
		# self.nifty_hedge_pos_ = 0
		
		for index_ in range(0,len(self.product_list_)):
			prod_ = self.product_list_[index_]
			# if not self.prod_curr_posn_.loc[prod_,"sqoff_on"] and self.prod_curr_posn_.loc[prod_,"start_trading"] and not self.prod_curr_posn_.loc[prod_,"pos"]:
			if self.prod_curr_posn_[index_][Fields.close_.value] and not self.prod_curr_posn_[index_][Fields.sqoff_on.value] and self.prod_curr_posn_[index_][Fields.start_trading.value] :
				if not self.prod_curr_posn_[index_][Fields.pos.value] or self.prod_curr_posn_[index_][Fields.cascade_trigger.value]:
					#self.tradeCCMedianPrice(prod_,index_)
					# self.tradeCCTrailingMeanPrice(prod_,index_)
					#self.tradeCCFilterOCMeanPrice(prod_,index_)
					# self.tradeCCFilterOCClosingMeanPrice(prod_,index_)
					# self.tradeCCMeanPrice(prod_,index_)
					# self.tradeTodayClosingSignalPrice(prod_,index_)
					self.tradeCCFilterOCOpenVolMeanPrice(prod_,index_)
					# self.tradeCCFilterOCOpenVolMeanEntryStdPrice(prod_,index_)
					# self.tradeLongSmallCCFilterOCOpenVolMeanPrice(prod_,index_)
					# self.tradeCCFilterOCOpenVolMeanVolPrice(prod_,index_)
					# self.tradeCCFilterOCDayVolMeanPrice(prod_,index_)
					# self.tradeCCFilterOCOpenVolMedianPrice(prod_,index_)
					# self.tradeCLHFilterOCOpenVolMeanPrice(prod_,index_)
					# self.tradeCCFilterOCOpenVolMeanCappedPrice(prod_,index_)
					# self.tradeCCFilterOCOpenOrCurrentVolMeanPrice(prod_,index_)
					
					# self.tradeCCFilterOCOpenVolMeanCascadeHedgePrice(prod_,index_)
					# self.tradeCCFilterOCOpenVolClosingBiasMeanPrice(prod_,index_)
				# elif self.prod_curr_posn_[index_][Fields.cascade_trigger.value]:
				# 	pos_ = math.floor(self.max_exposure_/self.prod_curr_posn_[index_][Fields.close_.value])
				# 	if self.prod_curr_posn_[index_][Fields.pos.value]:
				# 		self.sendOrder('B',index_,pos_,self.prod_curr_posn_[index_][Fields.close_.value],self.current_time_)
				# 	else:
				# 		self.sendOrder('S',index_,pos_,self.prod_curr_posn_[index_][Fields.close_.value],self.current_time_)
			
			# elif self.prod_curr_posn_[index_][Fields.sqoff_on.value] and  self.bar_pos_[index_]:
			# 	# if not self.prod_curr_posn_[index_][Fields.pos.value] or self.prod_curr_posn_[index_][Fields.cascade_trigger.value]:
			# 	self.tradeReentry(prod_,index_)

	#After SL if want to take position in opposite direction
	def tradeReentryRev(self,_prod,_counter):
		current_cc_ = self.prod_curr_posn_[_counter][Fields.close_.value] - self.prev_day_close_[_counter]
		if _prod in self.prod_pos_.index:
			pos_ = int(self.prod_pos_.loc[_prod]["pos"])
		else:
			pos_ = math.floor(self.max_exposure_/self.prod_curr_posn_[_counter][Fields.close_.value])

		if not self.prod_curr_posn_[_counter][Fields.pos.value] and self.bar_pos_[_counter] > 0:
			if self.prod_curr_posn_[_counter][Fields.close_.value] < self.sqoff_px_[_counter]:
				self.sendOrder('S',_counter,pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.reentry_px_[_counter] = self.prod_curr_posn_[_counter][Fields.close_.value]
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1

		elif not self.prod_curr_posn_[_counter][Fields.pos.value] and self.bar_pos_[_counter] < 0:
			if self.prod_curr_posn_[_counter][Fields.close_.value] > self.sqoff_px_[_counter]:
				self.sendOrder('B',_counter,pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.reentry_px_[_counter] = self.prod_curr_posn_[_counter][Fields.close_.value]
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1

	

	#After SL if want to take position in same direction
	def tradeReentry(self,_prod,_counter):
		current_cc_ = self.prod_curr_posn_[_counter][Fields.close_.value] - self.prev_day_close_[_counter]
		if _prod in self.prod_pos_.index:
			pos_ = int(self.prod_pos_.loc[_prod]["pos"])
		else:
			pos_ = math.floor(self.max_exposure_/self.prod_curr_posn_[_counter][Fields.close_.value])

		if not self.prod_curr_posn_[_counter][Fields.pos.value] and self.prod_curr_posn_[_counter][Fields.high_.value] == self.current_high_[_counter] and self.current_cascade_[_counter] < self.cascade_num_:
			self.sendOrder('B',_counter,pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
			self.reentry_px_[_counter] = self.prod_curr_posn_[_counter][Fields.close_.value]
			self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
			# self.prod_curr_posn_[_counter][Fields.start_trading.value] = True

		elif not self.prod_curr_posn_[_counter][Fields.pos.value] and self.prod_curr_posn_[_counter][Fields.low_.value] == self.current_low_[_counter] and self.current_cascade_[_counter] < self.cascade_num_:
			self.sendOrder('S',_counter,pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
			self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
			self.reentry_px_[_counter] = self.prod_curr_posn_[_counter][Fields.close_.value]
			# self.prod_curr_posn_[_counter][Fields.start_trading.value] = True

	#Same underlying idea except if current vol higher than opening vol avg rather than just opening volume
	#This happens if there is a spurt in volume mid day
	def tradeCCFilterOCOpenOrCurrentVolMeanPrice(self,_prod,_counter):
		current_cc_ = self.prod_curr_posn_[_counter][Fields.close_.value] - self.prev_day_close_[_counter]
		if _prod in self.prod_pos_.index:
			pos_ = int(self.prod_pos_.loc[_prod]["pos"])
		else:
			pos_ = math.floor(self.max_exposure_/self.prod_curr_posn_[_counter][Fields.close_.value])

		if current_cc_*(self.prod_curr_posn_[_counter][Fields.close_.value] - self.prod_curr_posn_[_counter][Fields.open_.value]) >= 0 and \
		not math.isnan(self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]) and (math.isnan(self.open_vol_avg_[_counter]) \
			or  self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]> self.open_vol_avg_[_counter]\
			or self.prod_curr_posn_[_counter][Fields.pos.value] or self.current_vol_[_counter] > self.open_vol_avg_[_counter]):
			if current_cc_ > self.moment_cc_wt_mean_[_counter] + self.moment_cc_wt_stdev_[_counter] and self.current_cascade_[_counter] < self.cascade_num_:
				# print("B",self.prod_curr_posn_[_counter][Fields.close_.value], self.prod_curr_posn_[_counter][Fields.open_.value], self.moment_oc_wt_mean_[_counter] ,self.moment_oc_wt_stdev_[_counter])
				self.sendOrder('B',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
				self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.high_.value])
				if self.current_cascade_[_counter] == 1:
					self.entry_obv_[_counter]= self.current_obv_[_counter]
				self.last_pos_obv_[_counter]= self.current_obv_[_counter]
				# self.updateTrailingSL(_counter,self.current_high_[_counter])
			elif current_cc_ < -1 *(self.moment_cc_wt_mean_[_counter] + self.moment_cc_wt_stdev_[_counter]) and self.current_cascade_[_counter] < self.cascade_num_:
				self.sendOrder('S',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
				self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.low_.value])
				if self.current_cascade_[_counter] == 1:
					self.entry_obv_[_counter]= self.current_obv_[_counter]
				self.last_pos_obv_[_counter]= self.current_obv_[_counter]
	
	#Some tickers might move a lot and hence not get triggered in the coming days, so cap mean/std
	def tradeCCFilterOCOpenVolMeanCappedPrice(self,_prod,_counter):
		current_cc_ = self.prod_curr_posn_[_counter][Fields.close_.value] - self.prev_day_close_[_counter]
		if _prod in self.prod_pos_.index:
			pos_ = int(self.prod_pos_.loc[_prod]["pos"])
		else:
			pos_ = math.floor(self.max_exposure_/self.prod_curr_posn_[_counter][Fields.close_.value])

		if current_cc_*(self.prod_curr_posn_[_counter][Fields.close_.value] - self.prod_curr_posn_[_counter][Fields.open_.value]) >= 0 and \
		not math.isnan(self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]) and \
		(math.isnan(self.open_vol_avg_[_counter]) or  self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]> self.open_vol_avg_[_counter]):
			if current_cc_ > self.moment_cc_wt_capped_[_counter] and self.current_cascade_[_counter] < self.cascade_num_:
				# print("B",self.prod_curr_posn_[_counter][Fields.close_.value], self.prod_curr_posn_[_counter][Fields.open_.value], self.moment_oc_wt_mean_[_counter] ,self.moment_oc_wt_stdev_[_counter])
				self.sendOrder('B',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
				self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.high_.value])
				if self.current_cascade_[_counter] == 1:
					self.entry_obv_[_counter]= self.current_obv_[_counter]
				self.last_pos_obv_[_counter]= self.current_obv_[_counter]
				# self.updateTrailingSL(_counter,self.current_high_[_counter])
			elif current_cc_ < -1 *(self.moment_cc_wt_capped_[_counter]) and self.current_cascade_[_counter] < self.cascade_num_:
				self.sendOrder('S',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
				self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.low_.value])
				if self.current_cascade_[_counter] == 1:
					self.entry_obv_[_counter]= self.current_obv_[_counter]
				self.last_pos_obv_[_counter]= self.current_obv_[_counter]

	#rather than current close - prev day close, use high/low-prev day close
	def tradeCLHFilterOCOpenVolMeanPrice(self,_prod,_counter):
		current_cc_ = self.prod_curr_posn_[_counter][Fields.close_.value] - self.prev_day_close_[_counter]
		current_cl_ = self.prod_curr_posn_[_counter][Fields.close_.value] - self.prod_curr_posn_[_counter][Fields.low_.value]
		
		current_ch_ = self.prod_curr_posn_[_counter][Fields.close_.value] - self.prod_curr_posn_[_counter][Fields.high_.value]
		current_cl_ = max(current_cc_,current_cl_)
		current_ch_ = min(current_cc_,current_ch_)
		if _prod in self.prod_pos_.index:
			pos_ = int(self.prod_pos_.loc[_prod]["pos"])
		else:
			pos_ = math.floor(self.max_exposure_/self.prod_curr_posn_[_counter][Fields.close_.value])

		if not math.isnan(self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]) and \
		(math.isnan(self.open_vol_avg_[_counter]) or  self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]> self.open_vol_avg_[_counter]):
			if current_cl_ > self.moment_cc_wt_tl_mean_[_counter] + self.moment_cc_wt_tl_stdev_[_counter] and self.current_cascade_[_counter] < self.cascade_num_ \
				and current_cl_*(self.prod_curr_posn_[_counter][Fields.close_.value] - self.prod_curr_posn_[_counter][Fields.open_.value]) >= 0:
				# print("B",self.prod_curr_posn_[_counter][Fields.close_.value], self.prod_curr_posn_[_counter][Fields.open_.value], self.moment_oc_wt_mean_[_counter] ,self.moment_oc_wt_stdev_[_counter])
				self.sendOrder('B',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
				self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.high_.value])
				if self.current_cascade_[_counter] == 1:
					self.entry_obv_[_counter]= self.current_obv_[_counter]
				self.last_pos_obv_[_counter]= self.current_obv_[_counter]
				# self.updateTrailingSL(_counter,self.current_high_[_counter])
			elif current_ch_ < -1 *(self.moment_cc_wt_tl_mean_[_counter] + self.moment_cc_wt_tl_stdev_[_counter]) and self.current_cascade_[_counter] < self.cascade_num_\
				and current_ch_*(self.prod_curr_posn_[_counter][Fields.close_.value] - self.prod_curr_posn_[_counter][Fields.open_.value]) >= 0:
				self.sendOrder('S',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
				self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.low_.value])
				if self.current_cascade_[_counter] == 1:
					self.entry_obv_[_counter]= self.current_obv_[_counter]
				self.last_pos_obv_[_counter]= self.current_obv_[_counter]

	#Use Median instead of mean for historic price moves
	def tradeCCFilterOCOpenVolMedianPrice(self,_prod,_counter):
		current_cc_ = self.prod_curr_posn_[_counter][Fields.close_.value] - self.prev_day_close_[_counter]
		if _prod in self.prod_pos_.index:
			pos_ = int(self.prod_pos_.loc[_prod]["pos"])
		else:
			pos_ = math.floor(self.max_exposure_/self.prod_curr_posn_[_counter][Fields.close_.value])

		if current_cc_*(self.prod_curr_posn_[_counter][Fields.close_.value] - self.prod_curr_posn_[_counter][Fields.open_.value]) >= 0 and \
		not math.isnan(self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]) and \
		(math.isnan(self.open_vol_avg_[_counter]) or  self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]> self.open_vol_avg_[_counter]):
			if current_cc_ > self.moment_cc_wt_median_[_counter] + self.moment_cc_wt_stdev_[_counter] and self.current_cascade_[_counter] < self.cascade_num_:
				# print("B",self.prod_curr_posn_[_counter][Fields.close_.value], self.prod_curr_posn_[_counter][Fields.open_.value], self.moment_oc_wt_mean_[_counter] ,self.moment_oc_wt_stdev_[_counter])
				self.sendOrder('B',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
				self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.high_.value])
				if self.current_cascade_[_counter] == 1:
					self.entry_obv_[_counter]= self.current_obv_[_counter]
				self.last_pos_obv_[_counter]= self.current_obv_[_counter]
				# self.updateTrailingSL(_counter,self.current_high_[_counter])
			elif current_cc_ < -1 *(self.moment_cc_wt_median_[_counter] + self.moment_cc_wt_stdev_[_counter]) and self.current_cascade_[_counter] < self.cascade_num_:
				self.sendOrder('S',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
				self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.low_.value])
				if self.current_cascade_[_counter] == 1:
					self.entry_obv_[_counter]= self.current_obv_[_counter]
				self.last_pos_obv_[_counter]= self.current_obv_[_counter]

	#Rather than comparing opening bar volume, check if day volume exceeds day vol avg
	def tradeCCFilterOCDayVolMeanPrice(self,_prod,_counter):
		current_cc_ = self.prod_curr_posn_[_counter][Fields.close_.value] - self.prev_day_close_[_counter]
		if _prod in self.prod_pos_.index:
			pos_ = int(self.prod_pos_.loc[_prod]["pos"])
		else:
			pos_ = math.floor(self.max_exposure_/self.prod_curr_posn_[_counter][Fields.close_.value])

		if current_cc_*(self.prod_curr_posn_[_counter][Fields.close_.value] - self.prod_curr_posn_[_counter][Fields.open_.value]) >= 0 and \
		not math.isnan(self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]) and \
		self.day_bar_agg_[_counter][4] and (math.isnan(self.day_vol_avg_[_counter]) or  self.day_bar_agg_[_counter][4]> self.day_vol_avg_[_counter] \
			or math.isnan(self.open_vol_avg_[_counter]) or  self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]> self.open_vol_avg_[_counter]):
			if current_cc_ > self.moment_cc_wt_mean_[_counter] + self.moment_cc_wt_stdev_[_counter] and self.current_cascade_[_counter] < self.cascade_num_:
				# print("B",self.prod_curr_posn_[_counter][Fields.close_.value], self.prod_curr_posn_[_counter][Fields.open_.value], self.moment_oc_wt_mean_[_counter] ,self.moment_oc_wt_stdev_[_counter])
				self.sendOrder('B',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
				self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.high_.value])
				if self.current_cascade_[_counter] == 1:
					self.entry_obv_[_counter]= self.current_obv_[_counter]
				self.last_pos_obv_[_counter]= self.current_obv_[_counter]
				# self.updateTrailingSL(_counter,self.current_high_[_counter])
			elif current_cc_ < -1 *(self.moment_cc_wt_mean_[_counter] + self.moment_cc_wt_stdev_[_counter]) and self.current_cascade_[_counter] < self.cascade_num_:
				self.sendOrder('S',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
				self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.low_.value])
				if self.current_cascade_[_counter] == 1:
					self.entry_obv_[_counter]= self.current_obv_[_counter]
				self.last_pos_obv_[_counter]= self.current_obv_[_counter]

	#compane price move*volume rather than individual price move and vol
	def tradeCCFilterOCOpenVolMeanVolPrice(self,_prod,_counter):
		current_cc_ = self.prod_curr_posn_[_counter][Fields.close_.value] - self.prev_day_close_[_counter]
		if _prod in self.prod_pos_.index:
			pos_ = int(self.prod_pos_.loc[_prod]["pos"])
		else:
			pos_ = math.floor(self.max_exposure_/self.prod_curr_posn_[_counter][Fields.close_.value])

		if current_cc_*(self.prod_curr_posn_[_counter][Fields.close_.value] - self.prod_curr_posn_[_counter][Fields.open_.value]) >= 0 and \
		not math.isnan(self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]) and self.current_cascade_[_counter] < self.cascade_num_:
		
			if (math.isnan(self.open_vol_avg_[_counter]) and current_cc_ > self.moment_cc_wt_mean_[_counter] + self.moment_cc_wt_stdev_[_counter]) \
			 or  (self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]*current_cc_ \
			 	> self.open_vol_avg_[_counter] *(self.moment_cc_wt_mean_[_counter] + self.moment_cc_wt_stdev_[_counter]) and self.closing_bias_current_[_counter] >=0.5):
				# print("B",self.prod_curr_posn_[_counter][Fields.close_.value], self.prod_curr_posn_[_counter][Fields.open_.value], self.moment_oc_wt_mean_[_counter] ,self.moment_oc_wt_stdev_[_counter])
				self.sendOrder('B',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
				self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.high_.value])
				if self.current_cascade_[_counter] == 1:
					self.entry_obv_[_counter]= self.current_obv_[_counter]
				self.last_pos_obv_[_counter]= self.current_obv_[_counter]
				# self.updateTrailingSL(_counter,self.current_high_[_counter])
			elif (math.isnan(self.open_vol_avg_[_counter]) and current_cc_ < -1*(self.moment_cc_wt_mean_[_counter] + self.moment_cc_wt_stdev_[_counter])) \
			or (self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]*current_cc_ \
				< -1*self.open_vol_avg_[_counter] *(self.moment_cc_wt_mean_[_counter] + self.moment_cc_wt_stdev_[_counter]) and self.closing_bias_current_[_counter] >=0.5):

				self.sendOrder('S',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
				self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.low_.value])
				if self.current_cascade_[_counter] == 1:
					self.entry_obv_[_counter]= self.current_obv_[_counter]
				self.last_pos_obv_[_counter]= self.current_obv_[_counter]

	#Longer lookback in combination with smaller
	def tradeLongSmallCCFilterOCOpenVolMeanPrice(self,_prod,_counter):
		current_cc_ = self.prod_curr_posn_[_counter][Fields.close_.value] - self.prev_day_close_[_counter]
		if _prod in self.prod_pos_.index:
			pos_ = int(self.prod_pos_.loc[_prod]["pos"])
		else:
			pos_ = math.floor(self.max_exposure_/self.prod_curr_posn_[_counter][Fields.close_.value])

		if current_cc_*(self.prod_curr_posn_[_counter][Fields.close_.value] - self.prod_curr_posn_[_counter][Fields.open_.value]) >= 0 and \
		not math.isnan(self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]) and \
		(math.isnan(self.open_vol_avg_[_counter]) or  self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]> self.open_vol_avg_[_counter]):
			# if (current_cc_ > self.moment_cc_wt_mean_[_counter] + self.moment_cc_wt_stdev_[_counter] and self.current_cascade_[_counter] < self.cascade_num_) or\
			if (current_cc_ > self.long_wt_cc_mean_[_counter] + self.long_wt_cc_std_[_counter] and self.current_cascade_[_counter] < self.cascade_num_) :
				# print("B",self.prod_curr_posn_[_counter][Fields.close_.value], self.prod_curr_posn_[_counter][Fields.open_.value], self.moment_oc_wt_mean_[_counter] ,self.moment_oc_wt_stdev_[_counter])
				self.sendOrder('B',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
				self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.high_.value])
				if self.current_cascade_[_counter] == 1:
					self.entry_obv_[_counter]= self.current_obv_[_counter]
				self.last_pos_obv_[_counter]= self.current_obv_[_counter]
				# self.updateTrailingSL(_counter,self.current_high_[_counter])
			# elif (current_cc_ < -1 *(self.moment_cc_wt_mean_[_counter] + self.moment_cc_wt_stdev_[_counter]) and self.current_cascade_[_counter] < self.cascade_num_) or \
			elif (current_cc_ < -1 *(self.long_wt_cc_mean_[_counter] + self.long_wt_cc_std_[_counter]) and self.current_cascade_[_counter] < self.cascade_num_):
				self.sendOrder('S',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
				self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.low_.value])
				if self.current_cascade_[_counter] == 1:
					self.entry_obv_[_counter]= self.current_obv_[_counter]
				self.last_pos_obv_[_counter]= self.current_obv_[_counter]

	#longer lookback for price move
	def tradeCCFilterOCOpenVolMeanEntryStdPrice(self,_prod,_counter):
		current_cc_ = self.prod_curr_posn_[_counter][Fields.close_.value] - self.prev_day_close_[_counter]
		if _prod in self.prod_pos_.index:
			pos_ = int(self.prod_pos_.loc[_prod]["pos"])
		else:
			pos_ = math.floor(self.max_exposure_/self.prod_curr_posn_[_counter][Fields.close_.value])

		if current_cc_*(self.prod_curr_posn_[_counter][Fields.close_.value] - self.prod_curr_posn_[_counter][Fields.open_.value]) >= 0 and \
		not math.isnan(self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]) and \
		(math.isnan(self.open_vol_avg_[_counter]) or  self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]> self.open_vol_avg_[_counter]):
			if current_cc_ > self.moment_cc_wt_mean_[_counter] + self.moment_cc_wt_stdev_[_counter] and self.current_cascade_[_counter] < self.cascade_num_:
				if self.current_cascade_[_counter] > 0 or current_cc_ < self.long_wt_cc_mean_[_counter] + 3*self.long_wt_cc_std_[_counter]:
					# print("B",self.prod_curr_posn_[_counter][Fields.close_.value], self.prod_curr_posn_[_counter][Fields.open_.value], self.moment_oc_wt_mean_[_counter] ,self.moment_oc_wt_stdev_[_counter])
					self.sendOrder('B',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
					self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
					self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.high_.value])
					if self.current_cascade_[_counter] == 1:
						self.entry_obv_[_counter]= self.current_obv_[_counter]
					self.last_pos_obv_[_counter]= self.current_obv_[_counter]
				# self.updateTrailingSL(_counter,self.current_high_[_counter])
			elif current_cc_ < -1 *(self.moment_cc_wt_mean_[_counter] + self.moment_cc_wt_stdev_[_counter]) and self.current_cascade_[_counter] < self.cascade_num_:
				if self.current_cascade_[_counter] > 0 or current_cc_ > -1*(self.long_wt_cc_mean_[_counter] + 3*self.long_wt_cc_std_[_counter]):
					self.sendOrder('S',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
					self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
					self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.low_.value])
					if self.current_cascade_[_counter] == 1:
						self.entry_obv_[_counter]= self.current_obv_[_counter]
					self.last_pos_obv_[_counter]= self.current_obv_[_counter]
				# self.updateTrailingSL(_counter,self.current_low_[_counter])

	#Prod alpha- final
	def tradeCCFilterOCOpenVolMeanPrice(self,_prod,_counter):
		current_cc_ = self.prod_curr_posn_[_counter][Fields.close_.value] - self.prev_day_close_[_counter]
		if _prod in self.prod_pos_.index:
			pos_ = int(self.prod_pos_.loc[_prod]["pos"])
		else:
			pos_ = math.floor(self.max_exposure_/self.prod_curr_posn_[_counter][Fields.close_.value])

		#see earnings response
		# if self.prev_day_earnings_[_counter] == True:
		# 	self.prod_curr_posn_[_counter][Fields.start_trading.value] = False
		# 	return

		# dummy_index_ = self.earnings_index_
		# while dummy_index_ < len(self.earnings_list_) and int(self.earnings_list_[dummy_index_].split("\t")[0]) <= self.current_date_:
		# 	if self.earnings_list_[dummy_index_].split("\t")[2] == _prod:
		# 		print("Earnings",self.earnings_list_[dummy_index_],_prod)
		# 		self.prod_curr_posn_[_counter][Fields.start_trading.value] = False
		# 		return
		# 	dummy_index_ = dummy_index_ + 1
		if current_cc_*(self.prod_curr_posn_[_counter][Fields.close_.value] - self.prod_curr_posn_[_counter][Fields.open_.value]) >= 0 and \
		not math.isnan(self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]) and \
		(math.isnan(self.open_vol_avg_[_counter]) or  self.opening_bar_volume_[_counter][(self.open_vol_count_[_counter]+self.open_vol_days_-1)%self.open_vol_days_]> self.open_vol_avg_[_counter]):
			if current_cc_ > self.moment_cc_wt_mean_[_counter] + self.moment_cc_wt_stdev_[_counter] and self.current_cascade_[_counter] < self.cascade_num_:
				# print("B",self.prod_curr_posn_[_counter][Fields.close_.value], self.prod_curr_posn_[_counter][Fields.open_.value], self.moment_oc_wt_mean_[_counter] ,self.moment_oc_wt_stdev_[_counter])
				self.sendOrder('B',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
				self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.high_.value])
				if self.current_cascade_[_counter] == 1:
					self.entry_obv_[_counter]= self.current_obv_[_counter]
				self.last_pos_obv_[_counter]= self.current_obv_[_counter]
				# self.updateTrailingSL(_counter,self.current_high_[_counter])
			elif current_cc_ < -1 *(self.moment_cc_wt_mean_[_counter] + self.moment_cc_wt_stdev_[_counter]) and self.current_cascade_[_counter] < self.cascade_num_:
				# print(datetime.fromtimestamp(self.current_time_).strftime('%c'))
				self.sendOrder('S',_counter,self.cascade_size_[self.current_cascade_[_counter]] *pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_cascade_[_counter] = self.current_cascade_[_counter] + 1
				self.updateTrailingSL(_counter,self.prod_curr_posn_[_counter][Fields.low_.value])
				if self.current_cascade_[_counter] == 1:
					self.entry_obv_[_counter]= self.current_obv_[_counter]
				self.last_pos_obv_[_counter]= self.current_obv_[_counter]
		
	#trailing SL based on price move, historical std and position
	def updateTrailingSL(self,_index,_px):
		# index_ = str(_prod+"_"+str(self.current_time_))
		prod_ = self.product_list_[_index]
		time_factor_ = self.time_wt_ *(self.current_time_ - self.om_.started_at_)/(self.om_.ended_at_ - self.om_.started_at_)
		pos_factor_ = (self.pos_wt_* _px/self.max_exposure_)* abs(self.prod_curr_posn_[_index][Fields.pos.value])
		# self.decay_factor_ = -0.0693
		if self.prod_curr_posn_[_index][Fields.pos.value] > 0:
			# desired_price_ = self.prod_curr_posn_[_index][Fields.ltp.value]*(1 -self.price_delta_*pow(math.e,-1*(time_factor_+pos_factor_)))
			# desired_price_ = self.prod_curr_posn_[_index][Fields.ltp.value]*(1-self.trailing_pt_)
			# desired_price_ = self.prod_curr_posn_[_index][Fields.ltp.value]-self.trailing_pt_*self.moment_cc_wt_stdev_[_index]*(self.decay_factor_*self.cascade_num_+1-self.current_cascade_[_index])/(self.decay_factor_*self.cascade_num_)#*pow(math.e,-1*(self.current_cascade_[_index]-1))
			desired_price_ = _px-self.trailing_pt_*self.moment_cc_wt_stdev_[_index]*pow(math.e,self.decay_factor_*(self.current_cascade_[_index]-1))
			# print(prod_,_px,desired_price_,self.prod_curr_posn_[_index][Fields.support_px.value],self.moment_cc_wt_stdev_[_index],pow(math.e,self.decay_factor_*(self.current_cascade_[_index])))
			self.prod_curr_posn_[_index][Fields.support_px.value] =  max(self.prod_curr_posn_[_index][Fields.support_px.value],desired_price_)
		elif self.prod_curr_posn_[_index][Fields.pos.value] < 0:
			# desired_price_ = self.prod_curr_posn_[_index][Fields.ltp.value]*(1 +self.price_delta_*pow(math.e,-1*(time_factor_+pos_factor_)))
			# desired_price_ = self.prod_curr_posn_[_index][Fields.ltp.value]*(1+self.trailing_pt_)
			# desired_price_ = self.prod_curr_posn_[_index][Fields.ltp.value]+self.trailing_pt_*self.moment_cc_wt_stdev_[_index]*(self.decay_factor_*self.cascade_num_+1-self.current_cascade_[_index])/(self.decay_factor_*self.cascade_num_)#*pow(math.e,-1*(self.current_cascade_[_index]-1))
			desired_price_ = _px+self.trailing_pt_*self.moment_cc_wt_stdev_[_index]*pow(math.e,self.decay_factor_*(self.current_cascade_[_index]-1))
			# print(prod_,_px,desired_price_,self.prod_curr_posn_[_index][Fields.resist_px.value],self.moment_cc_wt_stdev_[_index],pow(math.e,self.decay_factor_*(self.current_cascade_[_index])))
			self.prod_curr_posn_[_index][Fields.resist_px.value] =  min(self.prod_curr_posn_[_index][Fields.resist_px.value],desired_price_)
		# self.prod_curr_posn_.loc[_prod,"SL"] = 0.5*self.prod_curr_posn_.loc[_prod]["Hard_SL"]*pow(math.e,-1*(time_factor_+pos_factor_))
		# print(self.prod_curr_posn_[_index][Fields.support_px.value],self.prod_curr_posn_[_index][Fields.resist_px.value])

	#Exec
	def onExec(self,_index,_size,_price,_cost,_time):
		StratFramework.onExec(self,_index,_size,_price,_cost,_time)
		prod_row_ = self.prod_curr_posn_[_index]
		prod_row_[Fields.cascade_trigger.value] = False
		self.last_trade_ltp_[_index] = _price
		


	def checkSL(self,_ind,_px):
		StratFramework.checkSL(self,_ind,_px)
		prod_ = self.product_list_[_ind]
		if self.prod_curr_posn_[_ind][Fields.pos.value] > 0 and self.current_vol_[_ind] > self.opening_bar_volume_[_ind][(self.open_vol_count_[_ind]+self.open_vol_days_-1)%self.open_vol_days_]:#\
			#and self.current_cascade_[_ind] < self.vol_sqoff_cascade_:
		# if self.prod_curr_posn_[_ind][Fields.pos.value] > 0 and self.current_vol_[_ind] > self.opening_bar_volume_[_ind][(self.open_vol_count_[_ind]+self.open_vol_days_-1)%self.open_vol_days_]:# or self.current_obv_[_ind] < 0.5*self.entry_obv_[_ind]):
			print("SL HIT(support_px) at " + str(self.current_time_) + " "  + str(self.prod_curr_posn_[_ind][Fields.support_px.value]) + " Strat" + str(self.strat_id_) + str(_px) + " for " + prod_ + " " + str(pow(math.e,self.decay_factor_*(self.current_cascade_[_ind]-1))))
			self.prod_curr_posn_[_ind][Fields.sqoff_on.value] =  True
			# self.prod_curr_posn_[_ind][Fields.start_trading.value] =  False
			self.squareOff(_ind,self.prod_curr_posn_[_ind][Fields.close_.value])
			self.current_cascade_[_ind] = 0
			self.prod_curr_posn_[_ind][Fields.support_px.value] = 0
			# self.reentry_px_[_ind] = 0
			# self.sqoff_px_[_ind] = self.prod_curr_posn_[_ind][Fields.close_.value]
		elif self.prod_curr_posn_[_ind][Fields.pos.value] < 0 and self.current_vol_[_ind] > self.opening_bar_volume_[_ind][(self.open_vol_count_[_ind]+self.open_vol_days_-1)%self.open_vol_days_]:#\
			# and self.current_cascade_[_ind] < self.vol_sqoff_cascade_:
		# elif self.prod_curr_posn_[_ind][Fields.pos.value] < 0 and self.current_vol_[_ind] > self.opening_bar_volume_[_ind][(self.open_vol_count_[_ind]+self.open_vol_days_-1)%self.open_vol_days_]:# or self.current_obv_[_ind] > 0.5*self.entry_obv_[_ind]): # and prod_ != "NIFTY":
			print("SL HIT(resist_px) at " + str(self.current_time_) + " "  + str(self.prod_curr_posn_[_ind][Fields.resist_px.value]) + " Strat" + str(self.strat_id_) +  str(_px) + " for " + prod_+ " " + str(pow(math.e,self.decay_factor_*(self.current_cascade_[_ind]-1))))
			self.prod_curr_posn_[_ind][Fields.sqoff_on.value] =  True
			# self.prod_curr_posn_[_ind][Fields.start_trading.value] =  False
			self.squareOff(_ind,self.prod_curr_posn_[_ind][Fields.close_.value])
			self.current_cascade_[_ind] = 0
			self.prod_curr_posn_[_ind][Fields.resist_px.value] = 0
			# self.reentry_px_[_ind] = 0
			# self.sqoff_px_[_ind] = self.prod_curr_posn_[_ind][Fields.close_.value]
