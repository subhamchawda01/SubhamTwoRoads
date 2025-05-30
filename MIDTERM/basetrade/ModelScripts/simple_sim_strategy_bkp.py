#!/usr/bin/env python

import sys
import subprocess
from datetime import datetime
import numpy
import pandas

MODELSCRIPTS_DIR = "~/basetrade_intall/ModelScripts/"
BIN_DIR = "~/basetrade_install/bin/"


class Order():
    def __init__(self, size_, price_, side_, qA, qB):
        self.size_ = size_
        self.price_ = price_
        self.side_ = side_
        self.qA = qA
        self.qB = qB


class Param ():
    def __init__(self, bid_keep_, bid_place_, bid_aggress_, ask_keep_, ask_place_, ask_aggress_, bs_, as_):
        self.bid_keep_ = bid_keep_
        self.bid_place_ = bid_place_
        self.bid_aggress_ = bid_aggress_
        self.ask_keep_ = ask_keep_
        self.ask_place_ = ask_place_
        self.ask_aggress_ = ask_aggress_
        self.bs_ = bs_
        self.as_ = as_


class Info ():
    def __init__(self):
        self.shortode = "EMPTY"
        self.time_ = " "
        self.tradingdate_ = 20120101
        self.volume_ = 0

        self.pnl_ = 0
        self.total_pnl_ = 0
        self.total_volume_ = 0

        self.pos_ = 0
        self.abs_pos_ = 0
        self.map_pos_increment_ = 0
        self.max_pos_ = 0

        self.bid_price_ = 0
        self.bid_size_ = 0
        self.bid_order_ = 0
        self.ask_price_ = 10000
        self.ask_order_ = 0
        self.ask_size_ = 0
        self.trade_side_ = 0
        self.trade_size_ = 0
        self.trade_price_ = 0

        self.bid_keep_ = 0.0
        self.ask_keep_ = 0.0
        self.bid_place_ = 0.0
        self.ask_place_ = 0.0
        self.uts_ = 3
        self.dbg_ = False
        self.bid_price_to_order_ = {}
        self.ask_price_to_order_ = {}
        self.tradevarsetvec_ = {}


info_ = Info()


def ExecuteOrder(size_, price_, side_):
    global info_
    info_.total_pnl_ = info_.total_pnl_ + size_ * price_ * side_
    info_.pnl_ = info_.pnl_ + size_ * price_ * side_
    info_.volume_ = info_.volume_ + size_
    info_.pos_ = info_.pos_ + size_ * side_


def SendOrder(price_, size_, side_):
    global info_
    if info_.dbg_:
        print info_.time_ + "Sending Order: " + str(price_) + " " + str(size_) + " " + str(side_)
    if (side_ == 1):
        ord_ = Order(size_, price_, side_, info_.bid_size_, 0)
        info_.bid_price_to_order_[price_] = ord_
    elif (side_ == -1):
        ord_ = Order(size_, price_, side_, info_.ask_size_, 0)
        info_.ask_price_to_order_[price_] = ord_


def ExecLogic(sumvars_, paramset_):
    global info_
    if info_.dbg_:
        print info_.time_ + " sv: " + str(sumvars_) + " bk: " + str(paramset_.bid_keep_) + " bp: " + str(paramset_.bid_place_) + " ak: " + str(paramset_.ask_keep_) + " ap: " + str(paramset_.ask_place_)
    if (sumvars_ < paramset_.bid_keep_ and (info_.bid_price_ in info_.bid_price_to_order_.keys())):
        del info_.bid_price_to_order_[info_.bid_price_]
    elif (sumvars_ >= paramset_.bid_place_ and (info_.bid_price_ not in info_.bid_price_to_order_.keys()) and paramset_.bs_ > 0):
        if info_.dbg_:
            print "SendBidOrder: " + str(info_.bid_price_) + " " + str(info_.uts_)
        SendOrder(info_.bid_price_, info_.uts_, 1)
    if (sumvars_ > -1 * paramset_.ask_keep_ and (info_.ask_price_ in info_.ask_price_to_order_.keys())):
        del info_.ask_price_to_order_[info_.ask_price_]
    elif (sumvars_ <= -1 * paramset_.ask_place_ and (info_.ask_price_ not in info_.ask_price_to_order_.keys()) and paramset_.as_ > 0):
        if info_.dbg_:
            print "SendAskOrder: " + str(info_.ask_price_) + " " + str(info_.uts_) + " sv: " + str(sumvars_) + " " + "thres:" + str(-1 * paramset_.ask_place_)
        SendOrder(info_.ask_price_, info_.uts_, -1)


def GetParmsOriginal(shortcode_, tradingdate_, paramfilename_):
    global info_
    exec_cmd_ = BIN_DIR + "get_contract_specs " + shortcode_ + " " + str(tradingdate_) + " " + "LOTSIZE"
    out = subprocess.Popen(exec_cmd_, shell=True, stdout=subprocess.PIPE)
    min_order_size_ = out.communicate()[0].strip().split()[1]
    exec_cmd_ = BIN_DIR + "get_contract_specs " + shortcode_ + " " + str(tradingdate_) + " " + "TICKSIZE"
    out = subprocess.Popen(exec_cmd_, shell=True, stdout=subprocess.PIPE)
    min_price_increment_ = out.communicate()[0].strip().split()[1]
    exec_cmd_ = BIN_DIR + "print_paramset " + paramfilename_ + " " + \
        str(tradingdate_) + " " + str(min_price_increment_) + " " + shortcode_ + " " + str(min_order_size_)
    out = subprocess.Popen(exec_cmd_, shell=True, stdout=subprocess.PIPE)
    paramset_out_ = out.communicate()
    position_vec_ = []
    paramset_out_ = paramset_out_[0]
    paramset_out_ = paramset_out_.split("\n")
    for line in paramset_out_:
        line = line.split()
        if (len(line) < 5):
            continue
        if (line[0] == "Position"):
            # Position 0 mapidx 32  BP: 0.2 BK: 0.1 AP: 0.2 AK: 0.1 BI: 100.2 BIK: 100.1 BA: 100.2 AI: 100.2 AIK: 100.1 AA: 100.2 bidsz: 3 asksz: 3
            param_ = Param(float(line[7]), float(line[5]), float(line[17]), float(line[11]),
                           float(line[9]), float(line[23]), float(line[25]), float(line[27]))
            info_.tradevarsetvec_[int(line[1])] = param_
            position_vec_.append(int(line[1]))
    i = 0
    while (i < len(position_vec_) - 1):
        map_pos_increment_ = abs(position_vec_[i + 1] - position_vec_[i])
        if info_.dbg_:
            print str(map_pos_increment_) + " " + str(position_vec_[i + 1]) + " " + str(position_vec_[i]) + " "
        if (map_pos_increment_ > info_.map_pos_increment_ and (position_vec_[i + 1] * position_vec_[i] >= 0)):
            info_.map_pos_increment_ = map_pos_increment_
        i = i + 1

    info_.max_position_ = max(position_vec_)


def SimTrader(isTrade):
    global info_
    if isTrade:
        if info_.trade_side_ == -1:
            for key in info_.bid_price_to_order_.keys():
                if (key > info_.trade_price_):
                    ord_ = info_.bid_price_to_order_[key]
                    ExecuteOrder(ord_.size_, ord_.price_, ord_.side_)
                    del info_.bid_price_to_order_[key]
                elif (key == info_.trade_price_):
                    ord_ = info_.bid_price_to_order_[key]
                    if (ord_.qA > info_.trade_size_):
                        ord_.qA = ord_.qA - info_.trade_size_
                    elif (ord_.qA <= info_.trade_size_):
                        ord_.qA = ord_.qA - info_.trade_size_
                        if (ord_.qA < 0 and abs(ord_.qA) >= ord_.size_):
                            ExecuteOrder(ord_.size_, ord_.price_, ord_.side_)
                            del info_.bid_price_to_order_[key]
                        elif (ord_.qA < 0):
                            ExecuteOrder(abs(ord_.qA), ord_.price_, ord_.side_)
                            ord_.size_ = ord_.size_ - abs(ord_.qA)

        elif info_.trade_side_ == 1:
            for key in info_.ask_price_to_order_.keys():
                if (key < info_.trade_price_):
                    ord_ = info_.ask_price_to_order_[key]
                    ExecuteOrder(ord_.size_, ord_.price_, ord_.side_)
                    del info_.ask_price_to_order_[key]
                elif (key == info_.trade_price_):
                    ord_ = info_.ask_price_to_order_[key]
                    if (ord_.qA > info_.trade_size_):
                        ord_.qA = ord_.qA - info_.trade_size_
                    elif (ord_.qA <= info_.trade_size_):
                        ord_.qA = ord_.qA - info_.trade_size_
                        if (ord_.qA < 0 and abs(ord_.qA) >= ord_.size_):
                            ExecuteOrder(ord_.size_, ord_.price_, ord_.side_)
                            del info_.ask_price_to_order_[key]
                        elif (ord_.qA < 0):
                            ExecuteOrder(abs(ord_.qA), ord_.price_, ord_.side_)
                            ord_.size_ = ord_.size_ - abs(ord_.qA)
    else:
        for key in info_.bid_price_to_order_.keys():
            if (key >= info_.ask_price_):
                ord_ = info_.bid_price_to_order_[key]
                ExecuteOrder(ord_.size_, ord_.price_, ord_.side_)
                del info_.bid_price_to_order_[key]
            elif (key > info_.bid_price_):
                ord_ = info_.bid_price_to_order_[key]
                ord_.qA = 0
                ord_.qB = 0
            elif (key == info_.bid_price_):
                ord_ = info_.bid_price_to_order_[key]
                prev_ = (ord_.qA + ord_.qB)
                if (prev_ <= info_.bid_size_):
                    ord_.qB = ord_.qB + (info_.bid_size_ + prev_)
                elif (prev_ > info_.bid_size_):
                    ord_.qA = ord_.qA - int((prev_ - info_.bid_size_) * float(ord_.qA) / (prev_))
                    ord_.qB = info_.bid_size_ - ord_.qA

        for key in info_.ask_price_to_order_.keys():
            if (key <= info_.bid_price_):
                ord_ = info_.ask_price_to_order_[key]
                ExecuteOrder(ord_.size_, ord_.price_, ord_.side_)
                del info_.ask_price_to_order_[key]
            elif (key < info_.ask_price_):
                ord_ = info_.ask_price_to_order_[key]
                ord_.qA = 0
                ord_.qB = 0
            elif (key == info_.ask_price_):
                ord_ = info_.ask_price_to_order_[key]
                prev_ = (ord_.qA + ord_.qB)
                if (prev_ <= info_.ask_size_):
                    ord_.qB = ord_.qB + (info_.ask_size_ + prev_)
                elif (prev_ > info_.ask_size_):
                    ord_.qA = ord_.qA - int((prev_ - info_.ask_size_) * float(ord_.qA) / (prev_))
                    ord_.qB = info_.bid_size_ - ord_.qA


def main(mkt_data_filename_, reg_data_filename_):
    global info_
    mfm_ = 0
    last_mfm_ = 0
    last_sumvars_ = 0.0
    bid_ask_read_ = False
    getflat_ = False

    # first do preprocessing to find the index where day change occurs
    cols = ['1', '2', '3', '4', '5', '6', '7', '8', '9', '10', '11', '12', '13', '14']
    lines_ = pandas.read_csv(mkt_data_filename_, sep=' ', header=None, names=cols).values
    reg_data_ = pandas.read_csv(reg_data_filename_, sep=' ', header=None).values
    min_reg_data_timestamp_ = reg_data_[0][0]

    index_to_begin_from_ = numpy.searchsorted(lines_[:, 0], min_reg_data_timestamp_)
    reg_data_idx_ = 0

    for idx in xrange(index_to_begin_from_, lines_.shape[0]):
        # if info_.dbg_:  print "LINE " +str ( lines_[idx] )
        line_words_ = lines_[idx]
        mkt_mfm_ = line_words_[0]
        info_.time_ = str(mkt_mfm_)
        isTrade = False
        if reg_data_idx_ >= reg_data_.shape[0]:
            getflat_ = True
            break
        if info_.dbg_:
            print "mkt_mfm_" + str(mkt_mfm_)
        while reg_data_idx_ < reg_data_.shape[0] and mkt_mfm_ > mfm_:
            reg_line_ = reg_data_[reg_data_idx_]
            if reg_line_[0] > mkt_mfm_:
                reg_data_idx_ = reg_data_idx_ - 1
                break
            mfm_ = reg_line_[0]
            last_mfm_ = mfm_
            last_sumvars_ = reg_line_[1]
            reg_data_idx_ = reg_data_idx_ + 1
            if info_.dbg_:
                print "this mfm_" + str(mfm_)

        if info_.dbg_:
            print "LINE " + str(lines_[idx])
        if (line_words_[1] == "TRADE"):
            # time TRADE 10 B @ 10001 [ 1 10 9999 10001 10 1 ]
            isTrade = True
            info_.bid_order_ = int(line_words_[7])
            info_.bid_size_ = int(line_words_[8])
            info_.bid_price_ = float(line_words_[9])
            info_.ask_price_ = float(line_words_[10])
            info_.ask_size_ = int(line_words_[11])
            info_.ask_order_ = int(line_words_[12])
            if line_words_[3] == 'B':
                info_.trade_side_ = 1
            else:
                info_.trade_side_ = -1
            info_.trade_size_ = int(line_words_[2])
            info_.trade_price_ = float(line_words_[5])
            bid_ask_read_ = True

        elif (line_words_[1] == "UPDATE"):
            # time UPDATE [ 1 10 9999 10001 10 1 ]
            info_.bid_order_ = int(line_words_[3])
            info_.bid_size_ = int(line_words_[4])
            info_.bid_price_ = float(line_words_[5])
            info_.ask_price_ = float(line_words_[6])
            info_.ask_size_ = int(line_words_[7])
            info_.ask_order_ = int(line_words_[8])
            bid_ask_read_ = True

        #line_words_ = map ( float, line_words_ )

        if (bid_ask_read_ and not getflat_):
            sumvars_ = 1  # sum ( line_words_[1:len(line_words_)-1] )
            t_position_ = info_.pos_
            if (info_.pos_ > info_.max_position_):
                t_position_ = info_.max_position_
            elif (info_.pos_ < -1 * info_.max_position_):
                t_position_ = -1 * info_.max_position_
            param_ = info_.tradevarsetvec_[int(t_position_ / info_.map_pos_increment_) * info_.map_pos_increment_]
            ExecLogic(last_sumvars_, param_)
            SimTrader(isTrade)


if __name__ == "__main__":
    if (len(sys.argv) < 6):
        print "USAGE: <shortcode> <tradingdate> <mkt_data> <reg_data> <param>"
        exit(0)

    info_.shortcode_ = sys.argv[1]
    info_.tradingdate_ = int(sys.argv[2])
    mkt_data_filename_ = sys.argv[3]
    reg_data_filename_ = sys.argv[4]
    paramfilename_ = sys.argv[5]

    #GetParams ( paramfilename_ )
    GetParmsOriginal(info_.shortcode_, info_.tradingdate_, paramfilename_)

    main(mkt_data_filename_, reg_data_filename_)

    info_.total_pnl_ = info_.pnl_
    #info_ = Info()
    print "SUM: " + str(-info_.pnl_) + " " + str(info_.volume_) + " " + str(info_.pos_)
    print "TOTAL: " + str(-1 * info_.total_pnl_)
