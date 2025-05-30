import pandas as pd
import math
from StratFramework import StratFramework, Fields
from BarGenerator import BarGenerator
from dateutil.rrule import DAILY, rrule, MO, TU, WE, TH, FR
from datetime import date,timedelta,datetime
import math
import numpy as np
# import datetime


class HFTTheo(StratFramework):

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


		self.train_start_ = _train_start_day
		self.train_end_ = _train_end_day
		self.training_data_ = []
		self.oldest_bar_ = []
		self.last_bar_update_ = 0
		self.open_vol_count_ = [0]*len(self.product_list_)
		self.open_bar_px_ = [0]*len(self.product_list_)

		self.prev_day_close_ = [0]*len(self.product_list_)
		self.prev_day_high_ = [0]*len(self.product_list_)
		self.prev_day_low_ = [0]*len(self.product_list_)
		self.last_execution_px_ = [0]*len(self.product_list_)
		self.current_low_ = [0]*len(self.product_list_)
		self.current_high_ = [0]*len(self.product_list_)
		self.current_open_ = [0]*len(self.product_list_)
		self.prev_bar_open_ = [0]*len(self.product_list_)
		self.current_cascade_ = [0]*len(self.product_list_)
		self.prev_day_bar_vol_ = [0]*len(self.product_list_)
		self.active_days_count_ = [0]*len(self.product_list_)
		self.open_vol_avg_ = [np.NaN]*len(self.product_list_)
		self.current_time_ = 0
		self.current_date_ = 0
		self.cascade_num_ = 0
		self.cascade_size_ = []
		self.current_obv_ = [0]*len(self.product_list_)
		self.max_obv_ = [0]*len(self.product_list_)


		for prod_ in self.product_list_:
		# 	# if prod_ in self.prod_pos_.index:
		# 	# 	print("Lotsize",prod_, self.prod_pos_.loc[prod_]["pos"])
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
				if row[0] == "TRAIN_START_TIME":
					self.train_start_time_ = row[2]
				if row[0] == "TRAIN_END_TIME":
					self.train_end_time_ = row[2]
				if row[0] == "STRAT_START_TIME":
					self.strat_start_time_ = row[2]
				if row[0] == "STRAT_END_TIME":
					self.strat_end_time_ = row[2]
				if row[0] == "OPEN_VOL_LOOKBK":
					self.open_vol_days_ = int(row[2])
				if row[0] == "OPEN_VOL_PTILE":
					self.open_vol_ptile_ = float(row[2])
				if row[0] == "BETA":
					self.moment_cc_beta_ = float(row[2])
					# self.moment_cc_beta_ = self.moment_oc_beta_





		self.opening_bar_volume_ = np.zeros(self.open_vol_days_*len(self.product_list_)).reshape(len(self.product_list_),self.open_vol_days_)
		self.opening_bar_volume_[:] = np.NaN

		self.bar_agg_ = np.zeros(len(self.product_list_) *5).reshape(len(self.product_list_),5)
		self.day_bar_agg_ = np.zeros(len(self.product_list_) *5).reshape(len(self.product_list_),5)
		self.current_vol_ = [0]*len(self.product_list_)
		self.day_vol_avg_ = [np.NaN]*len(self.product_list_)

		self.prev_bar_close_ = [0]*len(self.product_list_)
		self.prev_bar_vol_= [0]*len(self.product_list_)
		self.max_close_ = [0]*len(self.product_list_)
		self.min_close_ = [0]*len(self.product_list_)
		self.threshold_ = [0.015]*len(self.product_list_)
		self.neg_casc_ = [0]*len(self.product_list_)
		# self.threshold_[len(self.product_list_)-1] = 0.015

	#Get Bar from BarGenerator	
	def loadBar(self, _method):
		self.dateRange()
		self.getTrainingBar(_method)
		
	#dates for which to run simulations
	def dateRange(self):
		start_ = self.train_start_.split('_')
		end_ = self.train_end_.split('_')
		start_ = date(int(start_[0]),int(start_[1]),int(start_[2]))
		end_ = date(int(end_[0]),int(end_[1]),int(end_[2])) -timedelta(1)
		self.working_days_ =  list(rrule(DAILY, dtstart=start_, until=end_, byweekday=(MO,TU,WE,TH,FR)))



	#Called before the first trading day in date range.Initializes dataframe with values
	def getTrainingBar(self,_method):
		counter_ = 0
		for index_ in range(0,len(self.product_list_)):
			prod_ = self.product_list_[index_]
			flag_ = 0
			start_ = self.train_start_time_.split('_')
			end_ = self.train_end_time_.split('_')
			date_counter_ = 0
			for date_ in self.working_days_:
				day_start_epoch_ = int(date_.replace(hour=int(start_[0]),minute=int(start_[1])).strftime('%s'))  			
				day_end_epoch_ = int(date_.replace(hour=int(end_[0]),minute=int(end_[1])).strftime('%s'))  	
				[o,c,l,h,v] = _method(day_start_epoch_,day_end_epoch_,counter_)
				if [o,c,l,h] != [0,0,0,0]:
					flag_ =1
					if self.prev_day_close_[index_]:
						self.training_data_[counter_].loc[len(self.training_data_[counter_])] = [day_start_epoch_,o,c,l,h,v,abs(c-o),abs(c-self.prev_day_close_[index_]),abs((c-o)/(h-l + 0.00000001))]
					else:
						self.training_data_[counter_].loc[len(self.training_data_[counter_])] = [day_start_epoch_,o,c,l,h,v,abs(c-o),np.NaN,abs((c-o)/(h-l+0.00000001))]
					self.prev_day_close_[index_] = c
					self.prev_day_high_[index_] = h
					self.prev_day_low_[index_] = l
					date_counter_ = date_counter_ +1

			if not flag_:
				self.prod_curr_posn_[index_][Fields.start_trading.value] =  True
			self.active_days_count_[counter_] = date_counter_
			counter_ = counter_ + 1

			self.oldest_bar_.append(0)
		# self.getMeanMomentCC()

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
				if len(self.working_days_) > self.active_days_count_[counter_]:
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

				self.prev_day_close_[index_] = c
				self.prev_day_high_[index_] = h
				self.prev_day_low_[index_] = l	


			if not flag_ and self.prod_curr_posn_[index_][Fields.start_trading.value]:
				self.prod_curr_posn_[index_][Fields.start_trading.value] =  True


			self.open_vol_avg_[index_] = np.nanpercentile(self.opening_bar_volume_[index_],self.open_vol_ptile_)
			# self.day_vol_avg_[index_] = np.nanpercentile(self.day_volume_hist_[index_],1.5*self.open_vol_ptile_)

			counter_ = counter_ + 1

		# self.getMeanMomentCC()

	def getMeanMomentCC(self):
		for counter_ in range(0,len(self.product_list_)):
			col = self.training_data_[counter_]["c-c"]
			self.threshold_[counter_] =  self.moment_cc_beta_*col.mean()
		print(self.threshold_)
			
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
				self.prev_bar_close_[index_] = self.bar_agg_[index_][1]
				self.prev_bar_open_[index_] = self.bar_agg_[index_][0]
				self.prev_bar_vol_[index_] = self.bar_agg_[index_][4]
				# if self.bar_agg_[index_][1] > self.bar_agg_[index_][0]:
				# self.net_vol_[index_] += (self.bar_agg_[index_][1] - self.bar_agg_[index_][0])*self.bar_agg_[index_][4] 
			self.bar_agg_ = np.zeros(len(self.product_list_) *5).reshape(len(self.product_list_),5)
			# print(self.bar_agg_)

		# elif _time == self.last_bar_update_ + self.granularity_/3 or _time == self.last_bar_update_ + 2*self.granularity_/3:
		# 	self.checkDayOpenRevert(self.bar_agg_,_time)

		#Happens at EOD resetting variables
		if _time-self.om_.started_at_ == (datetime.strptime(self.train_end_time_,"%H_%M") - datetime.strptime(self.train_start_time_,"%H_%M")).total_seconds():
			# print("the end")
			for index_ in range(0,len(self.product_list_)):
				# self.day_volume_hist_[index_][(self.open_vol_count_[index_]+self.open_vol_days_-1)%self.open_vol_days_] = self.day_bar_agg_[index_][4] 
				if not np.isnan(self.current_vol_[index_]):
					self.prev_day_bar_vol_[index_] = self.current_vol_[index_]

			self.current_cascade_ = [0]*len(self.product_list_)
			self.last_bar_update_ = 0
			self.reset()
			self.updateTrainingBar(self.day_bar_agg_)
			self.day_bar_agg_ = np.zeros(len(self.product_list_) *5).reshape(len(self.product_list_),5)
			self.bar_agg_ = np.zeros(len(self.product_list_) *5).reshape(len(self.product_list_),5)
			self.current_low_ = [0]*len(self.product_list_)
			self.current_high_ = [0]*len(self.product_list_)
			self.current_vol_ = [0]*len(self.product_list_)
			self.last_execution_px_ = [0]*len(self.product_list_)
			self.current_date_ = 0
			self.max_close_ = [0]*len(self.product_list_)
			self.min_close_ = [0]*len(self.product_list_)
			self.current_open_ = [0]*len(self.product_list_)
			self.prev_bar_open_ = [0]*len(self.product_list_)
			self.prev_bar_vol_ = [0]*len(self.product_list_)
			self.current_obv_ = [0]*len(self.product_list_)
			self.max_obv_ = [0]*len(self.product_list_)
			self.neg_casc_ = [0]*len(self.product_list_)

	def checkDayOpenRevert(self,_prod_bar_info,_time):
		self.current_time_ = _time
		for index_ in range(0,len(self.product_list_)):
			open_,close_,low_,high_,vol_ = _prod_bar_info[index_]
			if close_ == 0:
				close_ = self.prod_curr_posn_[index_][Fields.close_.value]
			if self.prod_curr_posn_[index_][Fields.pos.value] > 0:
				obv_ = self.current_obv_[index_] + (close_- open_)*vol_
				if obv_ < 0.75*self.max_obv_[index_]:
					self.sendOrder('S',index_,self.prod_curr_posn_[index_][Fields.pos.value],close_,self.current_time_)
					self.current_obv_[index_] = 0

			elif self.prod_curr_posn_[index_][Fields.pos.value] < 0:
				obv_ = self.current_obv_[index_] + (close_- open_)*vol_
				if obv_ > -0.75*self.max_obv_[index_]:
					self.sendOrder('B',index_,-1*self.prod_curr_posn_[index_][Fields.pos.value],close_,self.current_time_)
					self.current_obv_[index_] = 0

			

	#Called from aggregator and handles main strat execution and reaction to granular bar
	def onBarUpdate(self,_prod_bar_info,_time):
		self.current_time_ = _time
		for index_ in range(0,len(self.product_list_)):
			prod_ = self.product_list_[index_]
			open_,close_,low_,high_,vol_ = _prod_bar_info[index_]
			# print(prod_,open_,close_,low_,high_,vol_,_time,np.abs((self.prod_curr_posn_[index_][Fields.close_.value] - self.current_open_[index_])*self.current_vol_[index_]))
			if self.current_time_ -self.om_.started_at_ == self.granularity_ :
				if vol_:
					self.opening_bar_volume_[index_][self.open_vol_count_[index_]%self.open_vol_days_] = vol_
					self.open_bar_px_[index_] = close_
					self.max_close_[index_] = close_
					self.min_close_[index_] = close_

				else:
					self.opening_bar_volume_[index_][self.open_vol_count_[index_]%self.open_vol_days_] = np.NaN
					self.open_bar_px_[index_] = np.NaN

				self.open_vol_count_[index_] = self.open_vol_count_[index_] +1
			if self.prod_curr_posn_[index_][Fields.start_trading.value]:
				if vol_:
					if self.max_close_[index_] < close_:
						self.max_close_[index_] = close_
					if self.min_close_[index_] > close_:
						self.min_close_[index_] = close_
					# print("init bar",self.current_time_,open_,close_,low_,high_,vol_, self.px_move_[index_],np.max(np.abs(self.biggest_move_arr_[index_])))
					pnl_ = self.prod_curr_posn_[index_][Fields.pnl.value] + self.prod_curr_posn_[index_][Fields.pos.value]*(close_ - self.prod_curr_posn_[index_][Fields.ltp.value])
					# print("PNL",prod_,open_,close_,low_,high_,vol_,_time,pnl_)
					# if pnl_ < self.min_pnl_day_[index_]:
					# 	self.min_pnl_day_[index_] = pnl_
					if pnl_ >= self.prod_curr_posn_[index_][Fields.pnl.value] :
						self.prod_curr_posn_[index_][Fields.cascade_trigger.value] = True
						self.neg_casc_[index_] = 0
					else:
						self.prod_curr_posn_[index_][Fields.cascade_trigger.value] = False
						self.neg_casc_[index_] = self.neg_casc_[index_] + 1

					self.prod_curr_posn_[index_][Fields.pnl.value] = pnl_
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
					self.current_open_[index_] = open_
				
				else:
					self.current_vol_[index_] = np.NaN

			else:
				self.pos_pnl_df_[index_][self.day_index_].append([prod_,_time,self.prod_curr_posn_[index_][Fields.pos.value],self.prod_curr_posn_[index_][Fields.pnl.value]
					,self.prod_curr_posn_[index_][Fields.ltp.value],self.prod_curr_posn_[index_][Fields.slippage_cost.value],
					open_,close_,low_,high_,vol_,0])

		if self.current_time_-self.om_.ended_at_ >= (datetime.strptime(self.strat_end_time_,"%H_%M")- datetime.strptime(self.train_end_time_,"%H_%M")).total_seconds():
			self.getFlat()
			return
		if self.current_time_ -self.om_.started_at_ != self.granularity_:
			self.takeDecision()



	def takeDecision(self):
		for index_ in range(0,len(self.product_list_)):
			prod_ = self.product_list_[index_]
			# print(prod_, self.px_move_[index_]," ", end = '' )
			# if not self.prod_curr_posn_.loc[prod_,"sqoff_on"] and self.prod_curr_posn_.loc[prod_,"start_trading"] and not self.prod_curr_posn_.loc[prod_,"pos"]:
			if self.prod_curr_posn_[index_][Fields.close_.value] and not self.prod_curr_posn_[index_][Fields.sqoff_on.value] and self.prod_curr_posn_[index_][Fields.start_trading.value] and \
			self.prod_curr_posn_[index_][Fields.high_.value]- self.prod_curr_posn_[index_][Fields.low_.value] > self.threshold_[index_]*self.prod_curr_posn_[index_][Fields.open_.value]:
			# np.abs((self.prod_curr_posn_[index_][Fields.close_.value] - self.current_open_[index_])*self.current_vol_[index_]) > \
			# np.abs((self.prev_bar_close_[index_] - self.prev_bar_open_[index_])*self.prev_bar_vol_[index_]):
				# self.tradeTrend(prod_,index_,klarge,ksmall)
				self.tradeMin(prod_,index_)


	def tradeMin(self,_prod,_counter):
		pos_ = math.floor(self.max_exposure_/self.prod_curr_posn_[_counter][Fields.close_.value])
		# print(_prod,np.abs((self.prod_curr_posn_[_counter][Fields.close_.value] - self.current_open_[_counter])*self.current_vol_[_counter]), np.abs((self.prev_bar_close_[_counter] - self.prev_bar_open_[_counter])*self.prev_bar_vol_[_counter]) )
		if self.prod_curr_posn_[_counter][Fields.pos.value] > 0:
			self.current_obv_[_counter] = self.current_obv_[_counter] + (self.prod_curr_posn_[_counter][Fields.close_.value] - self.current_open_[_counter])*self.current_vol_[_counter]
			if self.max_obv_[_counter] < self.current_obv_[_counter]:
				self.max_obv_[_counter] = self.current_obv_[_counter]
			# print(_prod,"+" ,self.current_obv_[_counter] )
			if self.current_obv_[_counter] < 0.75*self.max_obv_[_counter] or self.neg_casc_[_counter] > 1:
			# if self.prod_curr_posn_[_counter][Fields.close_.value] < self.prev_bar_open_[_counter] and \
			# 	self.prod_curr_posn_[_counter][Fields.close_.value] < self.current_open_[_counter]:
				self.sendOrder('S',_counter,self.prod_curr_posn_[_counter][Fields.pos.value],self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_obv_[_counter] = 0
				self.neg_casc_[_counter] = 0
				# print(_prod,self.prod_curr_posn_[_counter][Fields.pnl.value])
				
		elif self.prod_curr_posn_[_counter][Fields.pos.value] < 0:
			self.current_obv_[_counter] = self.current_obv_[_counter] + (self.prod_curr_posn_[_counter][Fields.close_.value] - self.current_open_[_counter])*self.current_vol_[_counter]
			if self.max_obv_[_counter] < -1*self.current_obv_[_counter]:
				self.max_obv_[_counter] = -1*self.current_obv_[_counter]
			# print(_prod,"-" ,self.current_obv_[_counter] )
			if self.current_obv_[_counter] > -0.75*self.max_obv_[_counter] or self.neg_casc_[_counter] > 1:
			# if self.prod_curr_posn_[_counter][Fields.close_.value] > self.prev_bar_open_[_counter] and\
			# 	self.prod_curr_posn_[_counter][Fields.close_.value] > self.current_open_[_counter]:
				self.sendOrder('B',_counter,-1*self.prod_curr_posn_[_counter][Fields.pos.value],self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_obv_[_counter] = 0
				self.neg_casc_[_counter] = 0
				# print(_prod,self.prod_curr_posn_[_counter][Fields.pnl.value])
		elif np.abs((self.prod_curr_posn_[_counter][Fields.close_.value] - self.current_open_[_counter])*self.current_vol_[_counter]) > \
			np.abs((self.prev_bar_close_[_counter] - self.prev_bar_open_[_counter])*self.prev_bar_vol_[_counter]):
			if (self.prev_bar_open_[_counter] == 0 or self.prod_curr_posn_[_counter][Fields.close_.value] > self.prev_bar_open_[_counter] )and \
				self.prod_curr_posn_[_counter][Fields.close_.value]  > self.current_open_[_counter]:
				self.sendOrder('B',_counter,pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_obv_[_counter] = self.current_obv_[_counter] + (self.prod_curr_posn_[_counter][Fields.close_.value] - self.current_open_[_counter])*self.current_vol_[_counter]
				self.max_obv_[_counter] = self.current_obv_[_counter]
				# print(_prod,"+" ,self.current_obv_[_counter] )
				# print(_prod,self.prod_curr_posn_[_counter][Fields.pnl.value])
			elif (self.prev_bar_open_[_counter] == 0 or self.prod_curr_posn_[_counter][Fields.close_.value] < self.prev_bar_open_[_counter]) and \
				self.prod_curr_posn_[_counter][Fields.close_.value]  < self.current_open_[_counter]:
				self.sendOrder('S',_counter,pos_,self.prod_curr_posn_[_counter][Fields.close_.value],self.current_time_)
				self.current_obv_[_counter] = self.current_obv_[_counter] + (self.prod_curr_posn_[_counter][Fields.close_.value] - self.current_open_[_counter])*self.current_vol_[_counter]
				self.max_obv_[_counter] = -1*self.current_obv_[_counter]
				# print(_prod,"-" ,self.current_obv_[_counter] )
				# print(_prod,self.prod_curr_posn_[_counter][Fields.pnl.value])
