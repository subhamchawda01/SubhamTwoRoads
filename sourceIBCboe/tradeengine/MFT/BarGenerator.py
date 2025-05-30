import os
import time
import numpy as np


class BarGenerator:
    # content_ = []
    # curr_index_ = []
    # product_list_ = {}
    granularity_list_ = {}
    strat_list_ = []

    def __init__(self, _product_list, _start_time, _end_time):
        bar_data_path_ = "/spare/local/BarData/"
        self.content_ = [None] * len(_product_list)
        self.curr_index_ = [None] * len(_product_list)
        self.flag_ = 0
        self.product_list_ = _product_list
        for i in range(0, len(_product_list)):
            product_ = _product_list[i]
            self.content_[i] = []
            # self.product_list_[product_]=i
            product_data_ = os.path.join(bar_data_path_, product_)
            with open(product_data_, "r") as input_file:
                for line in input_file:
                    row = str.split(line)
                    # print(row[0])
                    if (row[0][0] == '#'):
                        continue
                    elif (float(row[0]) > _end_time):
                        break
                    elif (float(row[0]) >= _start_time):
                        self.content_[i].append(line)
            self.content_[i] = [x.strip() for x in self.content_[i]]
            self.curr_index_[i] = 0

    # def getTrainingData(self,_start_time,_end_time,_product):
    # 	self.flag_ = 0
    # 	[o,c,l,h] = self.readBar(_start_time,_end_time,_product)
    # 	if self.flag_ == 1:
    # 		return [o,c,l,h]
    # 	else:
    # 		return [0,0,0,0]
    def addStrat(self, _strat):
        _strat.strat_id_ = len(self.strat_list_)
        self.strat_list_.append(_strat)
        # self.granularity_list_.setdefault(300,[]).append(_strat.strat_id_)
        # self.granularity_list_.setdefault(_strat.granularity_,[]).append(_strat.strat_id_)
        # self.granularity_list_.setdefault(self.ended_at_ -self.started_at_,[]).append(_strat.strat_id_)
        #tsl check every tsl_granularity
        # self.granularity_list_.setdefault(_strat.tsl_granularity_,[]).append(_strat.strat_id_)
        # print(self.granularity_list_)

    def beginTrading(self):
        self.current_time_ = self.started_at_
        while (self.current_time_ <= self.ended_at_ - 60):
            self.onBarUpdate()
            self.current_time_ = self.current_time_ + 60
        # self.endTrading()

    def initAgg(self):
        self.bar_data_agg_ = np.zeros(len(self.product_list_) * 5).reshape(len(self.product_list_), 5)
        # print(self.bar_data_agg_)

    # def onBarUpdate(self):
    # 	t0 = time.time()
    # 	for index_ in range(0,len(self.product_list_)):
    # 		# prod_ = self.product_list_[index_]
    # 		[o,c,l,h,v] = self.readMinuteBar(self.current_time_,index_)
    # 		if [o,c,l,h] != [0,0,0,0]:
    # 			for gran_ in enumerate(self.granularity_list_):
    # 				if self.bar_data_agg_[gran_[0]][index_][0] == 0:
    # 					self.bar_data_agg_[gran_[0]][index_] = [o,c,l,h,v]
    # 				else:
    # 					self.bar_data_agg_[gran_[0]][index_][1] = c
    # 					self.bar_data_agg_[gran_[0]][index_][2] = min(self.bar_data_agg_[gran_[0]][index_][2],l)
    # 					self.bar_data_agg_[gran_[0]][index_][3] = max(self.bar_data_agg_[gran_[0]][index_][3],h)
    # 					self.bar_data_agg_[gran_[0]][index_][4] = self.bar_data_agg_[gran_[0]][index_][4] + v
    # 	t1 = time.time()
    # 	for tuple_ in enumerate(self.granularity_list_):
    # 		if (tuple_[1] != 300 and (self.current_time_+60-self.started_at_)%tuple_[1] == 0) or (tuple_[1] == 300 and self.current_time_+60-self.started_at_) == 300:
    # 			for strat_id_ in self.granularity_list_[tuple_[1]]:
    # 				strat_ = self.strat_list_[strat_id_]
    # 				strat_.onBarUpdate(self.bar_data_agg_[tuple_[0]],tuple_[1],self.current_time_+60)
    # 				self.bar_data_agg_[tuple_[0]] = np.zeros(len(self.product_list_) *5).reshape(len(self.product_list_) ,5)
    # 	t2 = time.time()
    # 	# print("bar_gen_update " +str(t1-t0) + " " + str(t2-t1))
    # 	# print(self.current_time_)
    # 	# print(self.bar_data_agg_)

    def onBarUpdate(self):
        t0 = time.time()
        for index_ in range(0, len(self.product_list_)):
            # prod_ = self.product_list_[index_]
            [o, c, l, h, v] = self.readMinuteBar(self.current_time_, index_)

            self.bar_data_agg_[index_] = [o, c, l, h, v]
            # print("num strats", len(self.strat_list_))
        for strat_ in self.strat_list_:
            strat_.aggregator(self.bar_data_agg_, self.current_time_ + 60)

    def trainingData(self, _start_time, _end_time, _index):
        train_data_ = [0, 0, 0, 0, 0]
        _time = _start_time
        while (_time < _end_time):
            [o, c, l, h, v] = self.readMinuteBar(_time, _index)
            if [o, c, l, h] != [0, 0, 0, 0]:
                if train_data_[0] == 0:
                    # print("hi")
                    train_data_ = [o, c, l, h, v]
                else:
                    # print("hi2")
                    train_data_[1] = c
                    train_data_[2] = min(train_data_[2], l)
                    train_data_[3] = max(train_data_[3], h)
                    train_data_[4] = train_data_[4] + v
            _time = _time + 60
            # print(o,c,l,h)
            # print(train_data_)
        return train_data_

    def readMinuteBar(self, _start_time, _index):
        self.flag_ = 0
        open_ = 0
        close_ = 0
        low_ = 0
        high_ = 0
        volume_ = 0
        # print(self.product_list_)
        prod_index_ = _index
        start_index_ = self.curr_index_[prod_index_]

        if self.content_[prod_index_] == []:
            return 0, 0, 0, 0, 0
        # print(_product,start_index_)
        frow = str.split(self.content_[prod_index_][start_index_])
        if frow[0][0] != '#' and float(frow[0]) > _start_time:
            start_index_ = 0
        for index in range(start_index_, len(self.content_[prod_index_])):
            row = str.split(self.content_[prod_index_][index])
            if row[0][0] != '#':
                # print(row)
                if index >= len(self.content_[prod_index_]) - 1 and float(row[0]) < _start_time:
                    open_, close_, low_, high_ = [0, 0, 0, 0]
                    break

                elif float(row[0]) <= _start_time:
                    open_ = float(row[5])
                    close_ = float(row[6])
                    low_ = float(row[7])
                    high_ = float(row[8])
                    self.curr_index_[prod_index_] = index
                    if float(row[0]) >= _start_time - 3600:
                        self.flag_ = 1
                    if float(row[0]) == _start_time:
                        volume_ = float(row[9])

                # elif float(row[0])+59 <= _end_time:
                # 	self.flag_ = 1
                # 	close_ = float(row[6])
                # 	low_ = min(low_,float(row[7]))
                # 	high_ = max(high_,float(row[8]))
                # 	self.curr_index_[prod_index_] = index

                else:
                    if float(row[0]) == _start_time + 60 and float(row[0]) == float(row[3]) and float(row[3]) + 59 > self.ended_at_:
                        if self.flag_ == 0:
                            open_ = float(row[5])
                            close_ = float(row[6])
                            low_ = float(row[7])
                            high_ = float(row[8])
                            self.flag_ = 1
                        else:
                            close_ = float(row[6])
                            low_ = min(low_, float(row[7]))
                            high_ = max(high_, float(row[8]))
                        volume_ = volume_ + float(row[9])
                    if self.flag_ == 0:
                        open_, close_, low_, high_, volume_ = [0, 0, 0, 0, 0]
                    break
        return open_, close_, low_, high_, volume_
