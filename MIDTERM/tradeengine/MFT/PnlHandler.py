import pandas as pd
from BarGenerator import BarGenerator
import cPickle as pickle
import time
import matplotlib.pyplot as plt
import datetime as dt
import numpy as np
import math

class PnlHandler:
	def __init__(self,_prod_file,_pickle_file,_strat_dates):
		t0 = time.time()
		with open(_prod_file, "r") as input_file:
			self.product_list_ = (input_file.readlines())
		self.product_list_ = [x.strip() for x in self.product_list_]
		with open(_pickle_file,"rb") as f:
			self.pos_pnl_df_ = pickle.load(f)
		# self.pos_pnl_df_ = []
		self.stats_df_ = pd.DataFrame(columns=["prod","date","maxpnl","pnl","sharpe","maxdraw","maxpos","minpos","max_exp","return_on_draw"])
		self.stats_df_[["maxpnl","pnl","sharpe","maxdraw","maxpos","minpos","max_exp","return_on_draw"]] = self.stats_df_[["maxpnl","pnl","sharpe","maxdraw","maxpos","minpos","max_exp","return_on_draw"]].astype(float)
		t1 = time.time()
		# print("Dataframe " + str(t1-t0))
		# print(self.pos_pnl_df_)
		self.lt_days_ = 30
		self.strat_start_time = '09_15'
		self.strat_end_time = '15_30'
		self.strat_dates_ = _strat_dates
		self.cascade_pnl = [0]*25
		# self.ltvol_ = [np.NaN]*self.lt_days_
		# self.ltvol_ = [self.ltvol_]*len(self.product_list_)
		self.ltvol_ = np.zeros(self.lt_days_*len(self.product_list_)).reshape(len(self.product_list_),self.lt_days_)
		self.ltvol_[:] = np.NaN
		self.ltstdlist_ = []
		self.pnllist_ = []
		self.datelist_=[]
		self.slippage_factor_ = 0.0005
		print("")
		self.computeStats()


	def convertDateTimeToEpoch(self,_date,_time):
		datetime_ = _date + " " + _time
		pattern = '%Y_%m_%d %H_%M'
		epoch = int(time.mktime(time.strptime(datetime_, pattern)))
		return epoch

	def computeStats(self):
		counter_ = 0
		self.total_slippage_= 0
		self.total_traded_value_ = 0
		for date_ in self.strat_dates_:
			epoch_start_ = self.convertDateTimeToEpoch(date_.strftime('%Y_%m_%d'),self.strat_start_time)
			epoch_end_ = self.convertDateTimeToEpoch(date_.strftime('%Y_%m_%d'),self.strat_end_time)
			gross_exp_ = 0
			net_exp_ = 0
			for index_ in xrange(0,len(self.product_list_)):
				t0 = time.time()
				prod_ = self.product_list_[index_]
				useful_rows_ = np.asarray(self.pos_pnl_df_[index_][counter_])
				# useful_rows_ = pd.DataFrame(self.pos_pnl_df_[index_][counter_])
				# useful_rows_.columns = ["prod","time","pos","pnl","ltp"]
				# print(useful_rows_[:,11])
				prod_info_ = self.pos_pnl_df_[index_]
				# if counter_ == self.lt_days_:
				# 	for i in range(1,self.lt_days_+1):
				# 		if float(prod_info_[i][len(prod_info_[i])-1][7]) and float(prod_info_[i-1][len(prod_info_[i-1])-1][7]):
				# 			self.ltvol_[index_][i%self.lt_days_] = abs(float(prod_info_[i][len(prod_info_[i])-1][7]) - float(prod_info_[i-1][len(prod_info_[i-1])-1][7]))/float(prod_info_[i][len(prod_info_[i])-1][7])
				# 		else:
				# 			self.ltvol_[index_][i%self.lt_days_] = np.NaN
				# 	self.ltstdlist_.append([])
				# 	self.pnllist_.append([])
				# 	self.datelist_.append([])
				# if counter_ > self.lt_days_:
				# 	# print(prod_,self.ltvol_[index_],np.nanstd(self.ltvol_[index_]))
				# 	if useful_rows_[len(useful_rows_)-1][3].astype(float):
				# 		# print(date_,prod_)
				# 		self.pnllist_[index_].append(useful_rows_[len(useful_rows_)-1][3].astype(float)-useful_rows_[len(useful_rows_)-1][5].astype(float))
				# 		self.ltstdlist_[index_].append(np.nanstd(self.ltvol_[index_]))
				# 		self.datelist_[index_].append(date_)
				# 	if float(prod_info_[counter_][len(prod_info_[counter_])-1][7]) and float(prod_info_[counter_-1][len(prod_info_[counter_-1])-1][7]):
				# 		self.ltvol_[index_][counter_%self.lt_days_] = abs(float(prod_info_[counter_][len(prod_info_[counter_])-1][7]) - float(prod_info_[counter_-1][len(prod_info_[counter_-1])-1][7]))/float(prod_info_[counter_][len(prod_info_[counter_])-1][7])
				# 	else:
				# 		self.ltvol_[index_][counter_%self.lt_days_] = np.NaN
				# print(prod_,useful_rows_[len(useful_rows_)-1][7])
					# print(self.ltstdlist_,self.pnllist_)
				# print(self.ltstdlist_[index_][len(self.ltstdlist_[index_])-1],self.pnllist_[index_][len(self.pnllist_[index_])-1])
				t1 = time.time()
				if useful_rows_ != [] and abs(useful_rows_[:,2].astype(float)).max():
					useful_rows_[:,3] = useful_rows_[:,3].astype(float) - useful_rows_[:,5].astype(float) 
					last_row_ = useful_rows_[len(useful_rows_)-1]
					self.total_slippage_ = self.total_slippage_ + float(last_row_[5])
					self.total_traded_value_ = self.total_traded_value_ + abs(useful_rows_[:,11].astype(float)).sum()
					max_pnl_ = useful_rows_[:,3].astype(float).max()
					# eod_pnl_ = float(last_row_[3]) 
					eod_pnl_ = float(last_row_[3])
					# sharpe_ = useful_rows_[:,3].astype(float).mean()/useful_rows_[:,3].astype(float).std()
					max_draw_ = (useful_rows_[:,3].astype(float) - np.maximum.accumulate(useful_rows_[:,3].astype(float))).min()
					max_pos_ = useful_rows_[:,2].astype(float).max()
					min_pos_ = useful_rows_[:,2].astype(float).min()
					start_pos_ = 0
					index_ = 0
					slippage_ = 0
					if min_pos_:
						while start_pos_ != min_pos_:
							ind_list_ = np.where(useful_rows_[:,2].astype(float)<start_pos_)[0]
							if not len(ind_list_):
								break
							self.cascade_pnl[index_] = self.cascade_pnl[index_]+ (last_row_[4].astype(float)-useful_rows_[ind_list_[0]][4].astype(float))*(useful_rows_[ind_list_[0]][2].astype(float) - start_pos_)
							self.cascade_pnl[index_] = self.cascade_pnl[index_] -2*(useful_rows_[ind_list_[0]][5].astype(float) - slippage_)
							start_pos_ = useful_rows_[ind_list_[0]][2].astype(float)
							# print("hi",ind_list_[0],start_pos_,min_pos_,self.cascade_pnl[index_])
							index_ = index_+1 
							slippage_ = useful_rows_[ind_list_[0]][5].astype(float)

					elif max_pos_:
						while start_pos_ != max_pos_:
							ind_list_ = np.where(useful_rows_[:,2].astype(float)>start_pos_)[0]
							if not len(ind_list_):
								break
							self.cascade_pnl[index_] = self.cascade_pnl[index_]+ (last_row_[4].astype(float)-useful_rows_[ind_list_[0]][4].astype(float))*(useful_rows_[ind_list_[0]][2].astype(float) - start_pos_)
							self.cascade_pnl[index_] = self.cascade_pnl[index_] -2*(useful_rows_[ind_list_[0]][5].astype(float) - slippage_)
							start_pos_ = useful_rows_[ind_list_[0]][2].astype(float)
							# print("hi",ind_list_[0],start_pos_,max_pos_,self.cascade_pnl[index_])
							index_ = index_+1 
							slippage_ = useful_rows_[ind_list_[0]][5].astype(float)

					max_exp_ = abs(useful_rows_[:,2].astype(float)*useful_rows_[:,4].astype(float)).max()
					return_on_draw_ =  float(useful_rows_[:,3].astype(float).mean()/max_draw_)
					self.stats_df_.loc[len(self.stats_df_)] = [prod_,str(date_.date()),max_pnl_,eod_pnl_,0,max_draw_,max_pos_,min_pos_,max_exp_,return_on_draw_]
					gross_exp_ = gross_exp_ + max_exp_
					if not net_exp_:
						net_exp_ = max_exp_
					else:
						net_exp_ = max(net_exp_,max_exp_)
					# net_exp_ = net_exp_ + float(last_row_[2])*float(last_row_[4])
				t2  = time.time()
				# print("computeStats " + str(t1-t0) + " " + str(t2-t1))
			useful_rows_ = self.stats_df_[(self.stats_df_["date"] == str(date_.date()))]
			if len(useful_rows_):
				total_pnl_ = useful_rows_["pnl"].sum()
				mean_pnl_ = useful_rows_["pnl"].mean()
				# sharpe_ = mean_pnl_/useful_rows_["pnl"].std()
				max_pos_ = useful_rows_["maxpos"].max()
				min_pos_ = useful_rows_["minpos"].min()
				self.stats_df_.loc[len(self.stats_df_)] = ["ALL",str(date_.date()),total_pnl_,mean_pnl_,0,0,max_pos_,min_pos_,gross_exp_,net_exp_]
				# print(net_exp_,gross_exp_,str(date_.date()))
				# print(epoch_start_,epoch_end_,useful_rows_)
			counter_ = counter_ + 1

		for index_ in xrange(0,len(self.product_list_)):
			prod_ = self.product_list_[index_]
			useful_rows_ = self.stats_df_[(self.stats_df_["prod"] == prod_)]
			if len(useful_rows_):
				max_pnl_ = useful_rows_["pnl"].sum()
				mean_pnl_ = useful_rows_["pnl"].mean()
				sharpe_ = useful_rows_["pnl"].mean()/useful_rows_["pnl"].std()
				max_draw_ = useful_rows_["maxdraw"].min()
				max_pos_ = useful_rows_["maxpos"].max()
				min_pos_ = useful_rows_["minpos"].min()
				gross_exp_ = float(useful_rows_["max_exp"].sum())
				self.stats_df_.loc[len(self.stats_df_)] = [prod_,"ALL",max_pnl_,mean_pnl_,sharpe_,max_draw_,max_pos_,min_pos_,gross_exp_,mean_pnl_/max_draw_]
			# self.stats_df_.loc[len(self.stats_df_)] = [prod_,,0,0,0,0,0,0,0]
		self.all_prod_ = self.stats_df_[self.stats_df_["prod"] == "ALL"]
		self.all_prod_["cumpnl"] = self.all_prod_["maxpnl"].cumsum()
		total_pnl_ = self.all_prod_["maxpnl"].sum()
		mean_pnl_ = self.all_prod_["maxpnl"].mean()
		num_days_traded_ = self.all_prod_["maxpnl"].count()
		avg_yearly_pnl_ = mean_pnl_*250
		sharpe_ = math.sqrt(252)*mean_pnl_/self.all_prod_["maxpnl"].std()
		temp_ = (self.all_prod_[self.all_prod_["maxpnl"] < 0]["maxpnl"]-mean_pnl_)**2
		# print(temp_)
		neg_sortino_ = math.sqrt(252)*mean_pnl_/np.sqrt(temp_.mean())
		# print(neg_sortino_)
		temp_ = (self.all_prod_[self.all_prod_["maxpnl"] < mean_pnl_]["maxpnl"]-mean_pnl_)**2
		sortino_ = math.sqrt(252)*mean_pnl_/np.sqrt(temp_.mean())
		max_draw_ = (self.all_prod_["cumpnl"] - self.all_prod_["cumpnl"].cummax()).min()
		max_pos_ = self.all_prod_["maxpos"].max()
		min_pos_ = self.all_prod_["minpos"].min()
		max_exp_ = self.all_prod_["max_exp"].max()
		max_prod_exp_ = self.all_prod_["return_on_draw"].max()
		print("")
		print(total_pnl_,mean_pnl_,avg_yearly_pnl_,num_days_traded_,sharpe_,neg_sortino_,sortino_,max_draw_,max_exp_,avg_yearly_pnl_/abs(max_draw_),self.total_slippage_, total_pnl_/self.total_traded_value_, self.total_traded_value_)
		self.stats_df_.loc[len(self.stats_df_)] = ["ALL","ALL",total_pnl_,mean_pnl_,sharpe_,max_draw_,max_pos_,min_pos_,max_exp_,total_pnl_/max_draw_]
		print("")
		print("90%tile portfolio " + str(self.all_prod_["max_exp"].quantile(0.9)))
		print("max conc " + str(max_prod_exp_)+" 90%tile conc " + str(self.all_prod_["return_on_draw"].quantile(0.9)))
		print(self.cascade_pnl/np.sum(self.cascade_pnl),np.sum(self.cascade_pnl))
		# print(self.stats_df_)
		# print(self.all_prod_)
		# print(self.all_prod_)
		# self.all_prod_.plot(x="date",y="pnl")
		# self.all_prod_.set_index("date",inplace=True)
		dates_list_ = [dt.datetime.strptime(date, '%Y-%m-%d').date() for date in self.all_prod_["date"]]
		# plt.plot(dates_list_,self.all_prod_["cumpnl"].tolist())
		# plt.save()
		# plt.show()
		# for index_ in xrange(0,len(self.product_list_)):
		# 	prod_ = self.product_list_[index_]
		# 	# plt.scatter(self.datelist_[index_],self.ltstdlist_[index_])
		# 	# plt.scatter(self.ltstdlist_[index_],self.pnllist_[index_])
		# 	print(prod_,np.mean(self.ltstdlist_[index_]),np.sum(self.pnllist_[index_]))
		# 	# plt.show()
