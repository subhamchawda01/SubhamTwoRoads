import argparse
import sys
import os
import time
import pandas as pd
import threading
import math
# from BarGenerator import BarGenerator
from enum import Enum
import pickle
import numpy as np

class Fields(Enum):
	support_px = 0
	resist_px = 1
	HARD_SL = 2
	SL = 3
	sqoff_on = 4
	start_trading = 5
	pos = 6
	pnl = 7
	ltp = 8
	open_ = 9
	close_ =10
	low_ = 11
	high_ = 12
	last_updated = 13
	cascade_trigger = 14
	slippage_cost = 15


class StratFramework:

	 

	def __init__(self,_om,_live_file,_granularity):
		# self.bar_data_df_ = pd.DataFrame(columns=["open","close","low","high"])
		# self.pos_pnl_df_ = pd.DataFrame(columns=["prod","time","pos","pnl","ltp"])
		self.pos_pnl_df_ = []
		self.granularity_ = _granularity
		with open(_live_file, "r") as input_file:
			self.product_list_ = (input_file.readlines())
		self.product_list_ = [x.strip() for x in self.product_list_]
		self.prod_curr_posn_ = []
		# self.prod_curr_posn_ = pd.DataFrame(columns=["support_px","resist_px","Hard_SL","SL","sqoff_on","start_trading","pos","pnl","ltp","open","close","low","high","last_updated"])
		for prod_ in  self.product_list_:
			self.prod_curr_posn_.append([0,1000000,-2000000,-1000000,False,True,0,0,0,0,0,0,0,0,False,0])
			self.pos_pnl_df_.append([])
			self.pos_pnl_df_[len(self.pos_pnl_df_)-1].append([])
			
		self.om_ = _om
		self.strat_id_ = 0
		self.get_flat_ = False
		self.current_time_ = 0
		self.day_index_ = 0


	# Redundant function- overloaded in core strats

	# def onBarUpdate(self,_prod_bar_info,_gran_update,_time):
	# 	t0 = time.time()
	# 	# print(_prod_bar_info)
	# 	if _gran_update != self.om_.ended_at_ - self.om_.started_at_ and   _time == self.om_.ended_at_:
	# 		return
	# 	self.current_time_ = _time

	# 	for index_ in range(0,len(self.product_list_)):
	# 		prod_ = self.product_list_[index_]
	# 		pos_pnl_index_ = str(prod_+"_"+str(_time))
	# 		open_,close_,low_,high_,vol_ = _prod_bar_info[index_]
	# 		if self.prod_curr_posn_[index_][Fields.start_trading.value]:
	# 			# if _time == self.prod_curr_posn_[index_][Fields.last_updated.value]:
	# 			# 	continue
		
	# 			# if [open_,close_,high_,low_] == [0,0,0,100000000]:
	# 				# print("Trading disabled for " + prod_)
	# 				# self.prod_curr_posn_.set_value(prod_,"start_trading", False)
	# 			if [open_,close_,high_,low_] != [0,0,0,0]:
	# 				pnl_ = self.prod_curr_posn_[index_][Fields.pnl.value] + self.prod_curr_posn_[index_][Fields.pos.value]*(close_ - self.prod_curr_posn_[index_][Fields.ltp.value])
	# 				self.prod_curr_posn_[index_][Fields.pnl.value] = pnl_
	# 				self.prod_curr_posn_[index_][Fields.ltp.value] = close_
	# 				self.prod_curr_posn_[index_][Fields.last_updated.value] =  _time
	# 				self.prod_curr_posn_[index_][Fields.close_.value] = close_
	# 				if not self.prod_curr_posn_[index_][Fields.open_.value]:
	# 					self.prod_curr_posn_[index_][Fields.open_.value] = open_
	# 					self.prod_curr_posn_[index_][Fields.low_.value] = low_
	# 					self.prod_curr_posn_[index_][Fields.high_.value] = high_
	# 				else:
	# 					if self.prod_curr_posn_[index_][Fields.low_.value] > low_:
	# 						self.prod_curr_posn_[index_][Fields.low_.value] = low_
	# 					if self.prod_curr_posn_[index_][Fields.high_.value] > high_:
	# 						self.prod_curr_posn_[index_][Fields.high_.value] = high_
	# 				# self.pos_pnl_df_.loc[pos_pnl_index_] = [prod_,_time,self.prod_curr_posn_[index_][Fields.pos.value],self.prod_curr_posn_[index_][Fields.pnl.value],self.prod_curr_posn_[index_][Fields.ltp.value]]
	# 			self.pos_pnl_df_[index_][self.day_index_].append([prod_,_time,self.prod_curr_posn_[index_][Fields.pos.value],self.prod_curr_posn_[index_][Fields.pnl.value]
	# 				,self.prod_curr_posn_[index_][Fields.ltp.value],self.prod_curr_posn_[index_][Fields.slippage_cost.value],
	# 				open_,close_,low_,high_,vol_,0])

	# 			self.checkSL(index_,self.prod_curr_posn_[index_][Fields.ltp.value])
	# 		else:
	# 			self.pos_pnl_df_[index_][self.day_index_].append([prod_,_time,self.prod_curr_posn_[index_][Fields.pos.value],self.prod_curr_posn_[index_][Fields.pnl.value]
	# 				,self.prod_curr_posn_[index_][Fields.ltp.value],self.prod_curr_posn_[index_][Fields.slippage_cost.value],
	# 				open_,close_,low_,high_,vol_,0])			
	# 	t1 = time.time()
	# 	# print("bar_update " + str(t1-t0))
	# 	if self.get_flat_:
	# 		return 

	# 	if _gran_update == self.om_.ended_at_ - self.om_.started_at_ :
	# 		# print(_prod_barq_info)
	# 		self.getFlat()
	# 		self.reset()
	# 		return
	# 	self.takeDecision()

	def takeDecision(self):
		t0 = time.time()
		for index_ in range(0,len(self.product_list_)):
			prod_ = self.product_list_[index_]
			if self.prod_curr_posn_[index_][Fields.last_updated.value] == self.current_time_ and not self.prod_curr_posn_[index_][Fields.sqoff_on.value] and self.prod_curr_posn_[index_][Fields.start_trading.value] and not self.prod_curr_posn_[index_][Fields.pos.value]:
				self.sendOrder('B',prod_,1,self.bar_data_df_.get_value(str(prod_+"_"+str(self.current_time_)),"close"),self.granularity_ + self.current_time_)
				self.sendOrder('B',index_,1,self.prod_curr_posn_[index_][Fields.ltp.value],self.current_time_)
		t1 = time.time()
		# print("takeDecision " +str(t1-t0))

	def getFlat(self):
		t0 = time.time()
		if self.get_flat_:
			# self.printPnl()
			return
		self.get_flat_ = True
		# threads = []
		# for prod_ in self.product_list_:
		# 	if self.prod_curr_posn_.get_value(prod_,"start_trading"):
		# 		self.updatePosPnl(prod_)
			# t = threading.Thread(target = self.getProdBar,args=(prod_,))
			# threads.append(t)
			# t.start()
			# t.join()
		t1 = time.time()
		# self.updatePosPnl()
		t2 = time.time()
		print("Get Flat called for Strat"+ str(self.strat_id_))
		for index_ in range(0,len(self.product_list_)):
			prod_ = self.product_list_[index_]
			# index_ = str(prod_+"_"+str(self.current_time_))
			if self.prod_curr_posn_[index_][Fields.pos.value] > 0:
				self.sendOrder('S',index_,self.prod_curr_posn_[index_][Fields.pos.value],self.prod_curr_posn_[index_][Fields.ltp.value],self.current_time_)
				ttv_ = self.pos_pnl_df_[index_][self.day_index_][len(self.pos_pnl_df_[index_][self.day_index_])-1][11]
				self.pos_pnl_df_[index_][self.day_index_][len(self.pos_pnl_df_[index_][self.day_index_])-1] =[prod_,self.current_time_ ,self.prod_curr_posn_[index_][Fields.pos.value],self.prod_curr_posn_[index_][Fields.pnl.value],self.prod_curr_posn_[index_][Fields.ltp.value],self.prod_curr_posn_[index_][Fields.slippage_cost.value],
																								self.prod_curr_posn_[index_][Fields.open_.value],self.prod_curr_posn_[index_][Fields.close_.value],
																								self.prod_curr_posn_[index_][Fields.low_.value],self.prod_curr_posn_[index_][Fields.high_.value],0,ttv_]
				# print(prod_,self.current_time_,self.prod_curr_posn_[index_][Fields.pos.value],self.prod_curr_posn_[index_][Fields.pnl.value],self.prod_curr_posn_[index_][Fields.ltp.value])
			elif self.prod_curr_posn_[index_][Fields.pos.value] < 0:
				self.sendOrder('B',index_,-1*self.prod_curr_posn_[index_][Fields.pos.value],self.prod_curr_posn_[index_][Fields.ltp.value],self.current_time_)
				ttv_ = self.pos_pnl_df_[index_][self.day_index_][len(self.pos_pnl_df_[index_][self.day_index_])-1][11]
				self.pos_pnl_df_[index_][self.day_index_][len(self.pos_pnl_df_[index_][self.day_index_])-1] =[prod_,self.current_time_ ,self.prod_curr_posn_[index_][Fields.pos.value],self.prod_curr_posn_[index_][Fields.pnl.value],self.prod_curr_posn_[index_][Fields.ltp.value],self.prod_curr_posn_[index_][Fields.slippage_cost.value],
																								self.prod_curr_posn_[index_][Fields.open_.value],self.prod_curr_posn_[index_][Fields.close_.value],
																								self.prod_curr_posn_[index_][Fields.low_.value],self.prod_curr_posn_[index_][Fields.high_.value],0,ttv_]
				# print(prod_,self.current_time_,self.prod_curr_posn_[index_][Fields.pos.value],self.prod_curr_posn_[index_][Fields.pnl.value],self.prod_curr_posn_[index_][Fields.ltp.value])
			self.prod_curr_posn_[index_][Fields.start_trading.value] =  False
		self.printPnl()

		t3 = time.time()
		# print("getflat ",t1-t0,t2-t1, t3-t2)
		

	def disableFlat(self):
		self.get_flat_ = False

	def sendOrder(self,_buy_sell,_index,_size,_price,_time):
		# print("Sending order Strat" + str(self.strat_id_) +" " + str(_buy_sell) + " " +str(self.product_list_[_index]) + " " + str(_size) + " " + str(_price) + " " + str(self.current_time_))
		size_,price_,cost_,time_ = self.om_.sendTrade(_buy_sell,self.product_list_[_index],_index,_size,_price,_time,self.strat_id_)
		self.onExec(_index,size_,price_,cost_,time_)
		
			
		# print(self.nifty_hedge_pos_,"nifty")

	def squareOff(self,_ind,_px):
		prod_ = self.product_list_[_ind]
		print("Squaring off Strat" + str(self.strat_id_) + " for " + prod_)
		
		self.prod_curr_posn_[_ind][Fields.sqoff_on.value] =  True
		# self.prod_curr_posn_[_ind][Fields.start_trading.value] =  False
		if self.prod_curr_posn_[_ind][Fields.pos.value] > 0:
			self.sendOrder('S',_ind,self.prod_curr_posn_[_ind][Fields.pos.value],_px,self.current_time_)
		elif  self.prod_curr_posn_[_ind][Fields.pos.value] < 0:
			self.sendOrder('B',_ind,-1*self.prod_curr_posn_[_ind][Fields.pos.value],_px,self.current_time_)
		# self.printPnl()
		

	# 	print("updatePosPnl " + str(t1-t0))
	def reset(self):
		
		t0 = time.time()
		for index_ in range(0,len(self.product_list_)):
			self.prod_curr_posn_[index_] = [0,1000000,-20000000,-1000000,False,True,0,0,0,0,0,0,0,0,False,0]
			self.pos_pnl_df_[index_].append([])
		self.disableFlat()
		self.nifty_hedge_pos_ = 0
		self.day_index_ = self.day_index_ + 1
		self.current_time_ = 0
		t1 = time.time()
		# print(self.day_index_,"reset")
		# print("reset " + str(t1-t0))

	def checkSL(self,_ind,_px):
		prod_ = self.product_list_[_ind]
		if self.prod_curr_posn_[_ind][Fields.pos.value] > 0 and (_px <= self.prod_curr_posn_[_ind][Fields.support_px.value]):# or self.current_obv_[_ind] < 0.5*self.entry_obv_[_ind]):
			print("SL HIT(support_px) at " + str(self.current_time_) + " "  + str(self.prod_curr_posn_[_ind][Fields.support_px.value]) + " Strat" + str(self.strat_id_) + str(_px) + " for " + prod_ + " " + str(pow(math.e,self.decay_factor_*(self.current_cascade_[_ind]-1))))
			self.prod_curr_posn_[_ind][Fields.sqoff_on.value] =  True
			# self.prod_curr_posn_[_ind][Fields.start_trading.value] =  False
			self.squareOff(_ind,self.prod_curr_posn_[_ind][Fields.close_.value])
			self.current_cascade_[_ind] = 0
			self.prod_curr_posn_[_ind][Fields.support_px.value] = 0
			# self.reentry_px_[_ind] = 0
			# self.sqoff_px_[_ind] = self.prod_curr_posn_[_ind][Fields.close_.value]

		elif self.prod_curr_posn_[_ind][Fields.pos.value] < 0 and (_px >= self.prod_curr_posn_[_ind][Fields.resist_px.value]):# or self.current_obv_[_ind] > 0.5*self.entry_obv_[_ind]): # and prod_ != "NIFTY":
			print("SL HIT(resist_px) at " + str(self.current_time_) + " "  + str(self.prod_curr_posn_[_ind][Fields.resist_px.value]) + " Strat" + str(self.strat_id_) +  str(_px) + " for " + prod_+ " " + str(pow(math.e,self.decay_factor_*(self.current_cascade_[_ind]-1))))
			self.prod_curr_posn_[_ind][Fields.sqoff_on.value] =  True
			# self.prod_curr_posn_[_ind][Fields.start_trading.value] =  False
			self.squareOff(_ind,self.prod_curr_posn_[_ind][Fields.close_.value])
			self.current_cascade_[_ind] = 0
			self.prod_curr_posn_[_ind][Fields.resist_px.value] = 0
			# self.reentry_px_[_ind] = 0
			# self.sqoff_px_[_ind] = self.prod_curr_posn_[_ind][Fields.close_.value]

		elif self.prod_curr_posn_[_ind][Fields.pnl.value] -self.prod_curr_posn_[_ind][Fields.slippage_cost.value] <=  self.prod_curr_posn_[_ind][Fields.SL.value] and self.prod_curr_posn_[_ind][Fields.pos.value] != 0: # and prod_ != "NIFTY":
			print("SL HIT Strat at " + str(self.current_time_) + " "  + str(self.strat_id_) + " for " + prod_)
			self.om_.sl_hit_counter_ = self.om_.sl_hit_counter_ + 1
			self.prod_curr_posn_[_ind][Fields.sqoff_on.value] =  True
			# self.prod_curr_posn_[_ind][Fields.start_trading.value] =  False
			self.squareOff(_ind,self.prod_curr_posn_[_ind][Fields.close_.value])
			self.current_cascade_[_ind] = 0
			# self.reentry_px_[_ind] = 0
			# self.sqoff_px_[_ind] = self.prod_curr_posn_[_ind][Fields.close_.value]
		

	#redundant function, used to emulate real SL in case of no positions taken
	def checkFakeSL(self,_ind,_px):
		prod_ = self.product_list_[_ind]
		if self.current_cascade_[_ind] > 0 and  _px <= self.prod_curr_posn_[_ind][Fields.support_px.value]:
			print("SL HIT(support_px) at " + str(self.current_time_) + " "  + str(self.prod_curr_posn_[_ind][Fields.support_px.value]) + " Strat" + str(self.strat_id_) + str(_px) + " for " + prod_ + " " + str(pow(math.e,self.decay_factor_*(self.current_cascade_[_ind]-1))))
			self.prod_curr_posn_[_ind][Fields.sqoff_on.value] =  True
			self.prod_curr_posn_[_ind][Fields.start_trading.value] =  False
			self.current_cascade_[_ind] = 0
			if self.prod_curr_posn_[_ind][Fields.pos.value] != 0:
				self.squareOff(_ind,_px)

		elif self.current_cascade_[_ind] > 0 and _px >= self.prod_curr_posn_[_ind][Fields.resist_px.value] :
			print("SL HIT(resist_px) at " + str(self.current_time_) + " "  + str(self.prod_curr_posn_[_ind][Fields.resist_px.value]) + " Strat" + str(self.strat_id_) +  str(_px) + " for " + prod_+ " " + str(pow(math.e,self.decay_factor_*(self.current_cascade_[_ind]-1))))
			self.prod_curr_posn_[_ind][Fields.sqoff_on.value] =  True
			self.prod_curr_posn_[_ind][Fields.start_trading.value] =  False
			self.current_cascade_[_ind] = 0
			if self.prod_curr_posn_[_ind][Fields.pos.value] != 0:
				self.squareOff(_ind,_px)

		elif self.prod_curr_posn_[_ind][Fields.pnl.value] -self.prod_curr_posn_[_ind][Fields.slippage_cost.value] <=  self.prod_curr_posn_[_ind][Fields.SL.value] and self.prod_curr_posn_[_ind][Fields.pos.value] != 0 and prod_ != "NIFTY":
			print("SL HIT Strat at " + str(self.current_time_) + " "  + str(self.strat_id_) + " for " + prod_)
			self.om_.sl_hit_counter_ = self.om_.sl_hit_counter_ + 1
			self.prod_curr_posn_[_ind][Fields.sqoff_on.value] =  True
			self.prod_curr_posn_[_ind][Fields.start_trading.value] =  False
			self.squareOff(_ind,_px)

	

	def convertDateTimeToEpoch(self,_date,_time):
		datetime_ = _date + " " + _time
		pattern = '%Y_%m_%d %H_%M'
		epoch = int(time.mktime(time.strptime(datetime_, pattern)))
		return epoch

	def onExec(self,_index,_size,_price,_cost,_time):
		prod_ = self.product_list_[_index]
		prod_row_ = self.prod_curr_posn_[_index]
		prod_row_[Fields.pnl.value] = prod_row_[Fields.pnl.value] + prod_row_[Fields.pos.value]*(_price - prod_row_[Fields.ltp.value])
		prod_row_[Fields.pos.value] = prod_row_[Fields.pos.value] + _size
		prod_row_[Fields.ltp.value] = _price
		prod_row_[Fields.last_updated.value] = self.current_time_
		prod_row_[Fields.slippage_cost.value] = prod_row_[Fields.slippage_cost.value] + _cost
		self.pos_pnl_df_[_index][self.day_index_].append([prod_,_time,self.prod_curr_posn_[_index][Fields.pos.value],self.prod_curr_posn_[_index][Fields.pnl.value],self.prod_curr_posn_[_index][Fields.ltp.value],self.prod_curr_posn_[_index][Fields.slippage_cost.value],
															0,0,0,0,0,_size*_price])
		# print(prod_,_time,self.prod_curr_posn_[_index][Fields.pos.value],self.prod_curr_posn_[_index][Fields.pnl.value],self.prod_curr_posn_[_index][Fields.ltp.value],self.prod_curr_posn_[_index][Fields.slippage_cost.value])

	def printPnl(self):	
		pnl_ = 0
		for index_ in range(0,len(self.prod_curr_posn_)):
			pnl_tuple_ = [row[3] for row in self.pos_pnl_df_[index_][self.day_index_]]
			if pnl_tuple_:
				max_pnl_ = np.max(pnl_tuple_)
				min_pnl_ = np.min(pnl_tuple_)
			else:
				max_pnl_ = 0
				min_pnl_ = 0
			row_ = self.prod_curr_posn_[index_]
			print(self.product_list_[index_], self.prod_curr_posn_[index_][Fields.pnl.value] -self.prod_curr_posn_[index_][Fields.slippage_cost.value], max_pnl_, min_pnl_)
					# abs((row_[Fields.high_.value] - row_[Fields.low_.value])/(row_[Fields.close_.value] - row_[Fields.open_.value] +0.000000001)),self.moment_cc_wt_tl_mean_[index_])
			pnl_ = pnl_ + self.prod_curr_posn_[index_][Fields.pnl.value] -self.prod_curr_posn_[index_][Fields.slippage_cost.value]
		print("Strat" + str(self.strat_id_),pnl_, time.strftime('%Y-%m-%d', time.localtime(self.om_.started_at_)))

		
