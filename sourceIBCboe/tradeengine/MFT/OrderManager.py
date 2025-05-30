import pandas as pd
import time
import matplotlib.pyplot as plt
from BarGenerator import BarGenerator
from PnlHandler import PnlHandler


#Handles order executions and slippages associated
class OrderManager:
	# product_list_=""
	# bar_gen_ = ""
	

	def __init__(self,_prod_file,_slippage_file,_start_time,_end_time):
		with open(_prod_file, "r") as input_file:
			self.product_list_ = (input_file.readlines())
		with open(_slippage_file, "r") as slip_file:
			self.slippage_list_ = (slip_file.readlines())
		self.product_list_ = [x.strip() for x in self.product_list_]
		self.slippage_list_ = [x.strip() for x in self.slippage_list_]
		self.prod_slip_pair_ = []
		#Constant slippage assumptions
		self.slippage_factor_ = 0.0005
		for prod in self.product_list_:
			flag_ = 0
			#Slippage costs
			# for line in self.slippage_list_:
			# 	sample_ = line.split(',')[0].split('_')[1]
			# 	if prod == sample_:
			# 		flag_ = 1
			# 		self.prod_slip_pair_.append([prod,float(line.split(',')[1])/100])
			if not flag_:
				self.prod_slip_pair_.append([prod,self.slippage_factor_])
				print(prod)

		self.last_update_ = []
		self.started_at_ = _start_time
		self.ended_at_ = _end_time
		self.sl_hit_counter_ = 0
		print(self.prod_slip_pair_)

	def sendTrade(self,_buy_sell,_product,_index,_size,_price,_time,_strat_id_):
			execution_price_ = _price
			if _buy_sell == 'B':
				# execution_price_ = _price*(1+self.slippage_factor_)
				return self.orderExecuted(_product,_index,_size,execution_price_,_time,_strat_id_)
			else:
				# execution_price_ = _price*(1-self.slippage_factor_)
				return self.orderExecuted(_product,_index,-1*_size,execution_price_,_time,_strat_id_)

	def orderExecuted(self,_product,_index,_size,_price,_time,_strat_id_):
		print("Executed order " +str(_product) + " " + str(_size) + " " + str(_price) + " " + str(abs(float(_size*_price))) + " " + str(_time))
		# print(_product,abs(float(_size*_price*self.prod_slip_pair_[_index][1])),_time)
		return _size,_price,abs(float(_size*_price*self.prod_slip_pair_[_index][1])),_time
		# return _size,_price,0,_time


	