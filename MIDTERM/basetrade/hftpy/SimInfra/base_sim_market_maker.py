
import sys
import os
import getpass
sys.path.append(os.path.join("/home", getpass.getuser(), "basetrade"))


'''
This is an interface between the market model and the exec logic that sends the order
Think of this as the exchange that i send the order to

The base_market_model decides how would i give fills to the order sent to the exchange
'''


class BaseSimMarketMaker(object):
    # defining CAOS as a class variable becuase , i can happend that i am trading multiple shortcodes
    # at the same time, in that case the sim_strategy would have multi sim_market_maker
    # hence this is a class variable
    client_assigned_order_sequence_ = 1

    def __init__(self, _dep_smv_, _security_name_):
        self.dep_smv_ = _dep_smv_
        # make this object listen to changes in smv
        self.dep_smv_.AddSmvListener(self)
        self.pnl_update_map_ = {}
        self.security_name_ = _security_name_
        self.order_exec_listeners = []
        self.client_position_map_ = {}
        self.caos_pnl_map_ = {}
        # a map that stores the buy orders at every price,
        # the key to the map is the int_px and the value is a list of order structs
        self.intpx_to_bid_order_ = {}
        # a map that stores the sell orders at every price
        # the key to the map is the int_px and the value is a list of order structs  key:int_px    value:[order_structs]
        self.intpx_to_ask_order_ = {}

        # a counter that assigns a unique id to each orders
        # self.client_assigned_order_sequence_ = 0
        self.global_position_ = 0
        # a map that has the key as the SACI , and the value is the sum of the bid size requested by the SACI for orders on all prices
        self.client_sum_bid_size_map_ = {}
        # a map that has the key as the SACI , and the value is the sum of the ask size requested by the SACI for orders on all prices
        self.client_sum_ask_size_map_ = {}
        # a dictionary that has key as the SACI and the value is another dictionary
        # the key to the other dictionary is the price and the value is the size requested at that price
        self.client_price_bid_size_map_ = {}
        # a dictionary that has key as the SACI and the value is another dictionary
        # the key to the other dictionary is the price and the value is the size requested at that price
        self.client_price_ask_size_map_ = {}

    def AddBaseMarketModel(self, _base_market_model_):
        self.base_market_model_ = _base_market_model_

    def OnMarketUpdate(self, _market_update_info_):
        #         print("SMM:OnMarketUpdate")
        self.base_market_model_.OnMarketUpdate(_market_update_info_)

    # def on_trade_print(self, _trade_print_info_):
    #     print("SMM:OnTradePrint")
    #     self.base_market_model_.on_trade_print(_trade_print_info_)

    def UpdateOnOrderExec(self, _order_):
        if _order_.buysell_ == 'B':
            self.client_sum_bid_size_map_[_order_.server_assigned_client_id_] -= _order_.size_executed()
            self.client_price_bid_size_map_[_order_.server_assigned_client_id_][
                _order_.int_price()] -= _order_.size_executed()
        else:
            self.client_sum_ask_size_map_[_order_.server_assigned_client_id_] -= _order_.size_executed()
            self.client_price_ask_size_map_[_order_.server_assigned_client_id_][
                _order_.int_price()] -= _order_.size_executed()

        for exec_listener in self.order_exec_listeners:
            # the listeners to the on_exec function must parse price_executed,size_executed,is_aggressive,
            exec_listener.OnExec(_order_.int_price(), _order_.size_executed(), _order_.buysell_,
                                 _order_.client_assigned_order_sequence_)

    def OnTradePrint(self, _trade_print_info_, _market_update_info_):
        pass

    def NotifyOnExecListener(self, order):

        # order struct , extracting the relevant information from these
        for order_exec_listener in self.order_exec_listeners:
            order_exec_listener.OnExec(order)

    def AddOrderExecListener(self, _new_listener_):
        self.order_exec_listeners.append(_new_listener_)

    def SendTrade(self, _price_, _int_price_, _size_requested_, _buysell_, _server_assigned_client_id_):
        # print "Int price in SMM: ", _int_price_

        #         print _price_, _int_price_, _size_requested_, _buysell_, _server_assigned_client_id_

        if _size_requested_ <= 0:
            return
            # incrementing the CAOS

        #         print "A trade received ",_price_, _int_price_, _size_requested_, _buysell_, _server_assigned_client_id_
        if _server_assigned_client_id_ not in self.client_position_map_.keys():
            self.client_position_map_[_server_assigned_client_id_] = 0

        BaseSimMarketMaker.client_assigned_order_sequence_ += 1
        if _server_assigned_client_id_ not in self.client_price_bid_size_map_:
            self.client_price_bid_size_map_[_server_assigned_client_id_] = {}
        if _server_assigned_client_id_ not in self.client_price_ask_size_map_:
            self.client_price_ask_size_map_[_server_assigned_client_id_] = {}
        if _server_assigned_client_id_ not in self.client_sum_bid_size_map_:
            self.client_sum_bid_size_map_[_server_assigned_client_id_] = 0
        if _server_assigned_client_id_ not in self.client_sum_ask_size_map_:
            self.client_sum_ask_size_map_[_server_assigned_client_id_] = 0

        # print "map initialized"


        if _buysell_ == 'B':
            self.client_sum_bid_size_map_[_server_assigned_client_id_] += _size_requested_
            if _int_price_ not in self.client_price_bid_size_map_[_server_assigned_client_id_].keys():
                #                 print "INt price added bid map is: ", _int_price_
                self.client_price_bid_size_map_[_server_assigned_client_id_][_int_price_] = 0
            self.client_price_bid_size_map_[_server_assigned_client_id_][_int_price_] += _size_requested_
        # print "bid map updated"
        else:
            self.client_sum_ask_size_map_[_server_assigned_client_id_] += _size_requested_
            if _int_price_ not in self.client_price_ask_size_map_[_server_assigned_client_id_].keys():
                #                 print "INt price added in ask map is ", _int_price_
                self.client_price_ask_size_map_[_server_assigned_client_id_][_int_price_] = 0
            self.client_price_ask_size_map_[_server_assigned_client_id_][_int_price_] += _size_requested_
            # print "ask map updated"
            #         print self.client_price_bid_size_map_
            #         print self.client_price_ask_size_map_
            #         print self.client_sum_bid_size_map_
            #         print self.client_sum_ask_size_map_
            #         print "sending the order to the market model"

            # print "The keys of the client size map: ", self.client_price_bid_size_map_.keys()
        #         print "Keys in the client_price_ask_size_map",self.client_price_ask_size_map_[_server_assigned_client_id_].keys()
        #         print "Keys in the client_price_bid_size_map",self.client_price_bid_size_map_[_server_assigned_client_id_].keys()
        self.base_market_model_.SendOrder(_server_assigned_client_id_, self.security_name_, _buysell_, _price_,
                                          _size_requested_, _int_price_,
                                          self.client_assigned_order_sequence_)

    # all orders at this price is cancelled for this saci
    def Cancel(self, _int_price_, _buysell_, _server_assigned_client_id_):
        if _buysell_ == 'B':
            if _int_price_ in self.intpx_to_bid_order_:
                for order in self.intpx_to_bid_order_[_int_price_]:
                    self.base_market_model_.CancelOrder(_server_assigned_client_id_, order.buysell(), order.int_price())
        else:
            if _int_price_ in self.intpx_to_ask_order_:
                for order in self.intpx_to_ask_order_[_int_price_]:
                    self.base_market_model_.CancelOrder(_server_assigned_client_id_, order.buysell(), order.int_price())

    def AddPnlUpdateListener(self, _pnl_update_listener_, _server_assigned_client_id_):
        self.pnl_update_map_[_server_assigned_client_id_] = _pnl_update_listener_

    def NotifyPnlUpdateListener(self, _server_assigned_client_id_, _new_pnl_):
        self.pnl_update_map_[_server_assigned_client_id_].OnPnlUpdate(_new_pnl_)

    def AddOrderExecListener(self, _new_listener_):
        self.order_exec_listeners.append(_new_listener_)

    def NotifyOrderExecListener(self):
        pass

    def ComputePnl(self, order):
        pnl_ = 0
        # if the order caos is not in the order_map then return
        if order.client_assigned_order_sequence() not in self.caos_pnl_map_:
            print "order not found in the order pnl map "
            return
        else:
            if order.buysell() == "B":
                if self.caos_pnl_map_[order.client_assigned_order_sequence_].buysell() == "S":
                    pnl_ = order.price_ - self.caos_pnl_map_[order.client_assigned_order_sequence_].price_
                else:
                    pass

            elif order.buysell() == "S":
                if self.caos_pnl_map_[order.client_assigned_order_sequence_].buysell() == "B":
                    pnl_ = self.caos_pnl_map_[order.client_assigned_order_sequence_].price_ - order.price_
                else:
                    pass
            else:
                print "the order type has to be B or S"
                sys.exit(0)

        if pnl_ != 0:
            print "The pnl in compute pnl in base market model: ", pnl_
            self.NotifyPnlUpdateListener(order.server_assigned_client_id(), pnl_)