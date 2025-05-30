import sys
import os
import getpass
sys.path.append(os.path.join("/home", getpass.getuser(), "basetrade", "hftpy"))
from random import randint
from hftpy.SimInfra.base_order import BaseOrder
import math


class BaseMarketModel:
    def __init__(self, _dep_smv_, _smm_, _server_assigned_client_id_, slippage_model_object):
        self.dep_smv_ = _dep_smv_
        self.server_assigned_client_id_ = _server_assigned_client_id_
        self.server_assigned_order_sequence_ = 0
        self.smm_ = _smm_
        self.slippage_model = slippage_model_object

        # variable to keep track of the present condition of the book
        # self.best_bid_price_ = self.dep_smv_.bestbid_price()
        # self.best_ask_price_ = self.dep_smv_.bestask_price()
        # self.best_bid_size_ = self.dep_smv_.BestBidSize()
        # self.best_ask_size_ = self.dep_smv_.BestAskSize()

    def SendOrder(self, _server_assigned_client_id_, _security_name_, _buysell_, _price_, _size_requested_,
                  _int_price_, _client_assigned_order_sequence_):

        '''

        This method udpates the book that we are maintaining on our end. So the order placed at different 
        prices are maintained in two dictionaries. 
        1. intpx_to_bid_order_ 
        2. intpx_to_ask_order_

        :param _server_assigned_client_id_: 
        :param _security_name_: 
        :param _buysell_: 
        :param _price_: 
        :param _size_requested_: 
        :param _int_price_: 
        :param _client_assigned_order_sequence_: 
        :return: 
        '''

        #         print "base market model received the order ",_server_assigned_client_id_, _security_name_, _buysell_, _price_, _size_requested_,
        #                    _int_price_, _client_assigned_order_sequence_
        #         print "Base market model"
        order_ = BaseOrder()
        order_.security_name_ = _security_name_
        order_.buysell_ = _buysell_
        order_.price_ = _price_
        order_.size_remaining_ = _size_requested_
        order_.int_price_ = _int_price_
        order_.order_status_ = 'None'
        order_.size_requested_ = _size_requested_
        order_.num_events_seen_ = 0
        order_.client_assigned_order_sequence_ = _client_assigned_order_sequence_
        order_.server_assigned_client_id_ = _server_assigned_client_id_
        order_.server_assigned_order_sequence_ = self.server_assigned_order_sequence_
        self.server_assigned_order_sequence_ += 1

        #         print "The order price: ",order_.int_price_
        #         print "The current bid price: ",self.dep_smv_.BestBidIntPrice()
        #         print "The current ask price: ",self.dep_smv_.BestAskIntPrice()
        #         print "The order type is: ",order_.buysell()
        if order_.buysell() == 'B':
            # if aggressive
            if order_.int_price_ >= self.dep_smv_.BestAskIntPrice():
                #                 print "in the if block"
                order_.int_price_ = self.dep_smv_.BestAskIntPrice()
                order_.price_ = self.dep_smv_.BestAskPrice()
            # print "in the else block"
            #                 print "the int price of the order",order_.int_price()
            #                 print "the keys of the smm map",self.smm_.intpx_to_bid_order_.keys()
            if order_.int_price() not in self.smm_.intpx_to_bid_order_.keys():
                # price not added in the bid order map hence add the price
                # print 123
                self.smm_.intpx_to_bid_order_[order_.int_price()] = []
            # add the order to the int_px_order map
            self.smm_.intpx_to_bid_order_[order_.int_price()].append(order_)
            self.smm_.caos_pnl_map_[order_.client_assigned_order_sequence_] = order_

        # print "Bid order map: ",self.smm_.intpx_to_bid_order_.keys()
        elif order_.buysell() == 'S':
            # if aggressive order then give full fills
            if order_.int_price_ >= self.dep_smv_.BestBidIntPrice():
                order_.int_price_ = self.dep_smv_.BestBidIntPrice()
                order_.price_ = self.dep_smv_.BestBidPrice()
                # give full execution to the aggressive order
            if order_.int_price() not in self.smm_.intpx_to_ask_order_.keys():
                # price not added in the ask order map hence add the price
                self.smm_.intpx_to_ask_order_[order_.int_price()] = []
            self.smm_.intpx_to_ask_order_[order_.int_price()].append(order_)
            self.smm_.caos_pnl_map_[order_.client_assigned_order_sequence_] = order_
            #             print "Ask order map: ",self.smm_.intpx_to_ask_order_.keys()
            # print self.smm_.intpx_to_bid_order_

    #         sys.exit(0)
    def CancelOrder(self, _server_assigned_client_id_, _buysell_, _int_price_):

        if _buysell_ == 'B':
            if _int_price_ in self.smm_.intpx_to_bid_order_:
                # all orders are cancelled at a particular price
                for order in self.smm_.intpx_to_bid_order_[_int_price_]:
                    if order.server_assigned_client_id() == _server_assigned_client_id_:
                        if order.can_be_cancelled():
                            order.canceled_ = True
                            self.smm_.intpx_to_bid_order_[_int_price_].remove(order)
                            self.smm_.client_sum_bid_size_map_[_int_price_] -= order.size_remaining()
                            self.smm_.client_price_bid_size_map_[_server_assigned_client_id_][
                                _int_price_] -= order.size_remaining()

        else:
            if _int_price_ in self.smm_.intpx_to_ask_order_[_int_price_]:
                for order in self.smm_.intpx_to_ask_order_[_int_price_]:
                    if order.server_assigned_client_id() == _server_assigned_client_id_:
                        if order.can_be_cancelled():
                            order.canceled_ = True
                            self.smm_.intpx_to_ask_order_[_int_price_].remove(order)
                            self.smm_.client_sum_ask_size_map_[_int_price_] -= order.size_remaining()
                            self.smm_.client_price_ask_size_map_[_server_assigned_client_id_][
                                _int_price_] -= order.size_remaining()

    def OnMarketUpdate(self, _market_update_info_):
        self.best_bid_price_ = self.dep_smv_.BestBidPrice()
        self.best_ask_price_ = self.dep_smv_.BestAskPrice()
        self.best_bid_size_ = self.dep_smv_.BestBidSize()
        self.best_ask_size_ = self.dep_smv_.BestAskSize()
        total_size_executed_ = 0
        for price in sorted(self.smm_.intpx_to_bid_order_.keys(), reverse=True):
            if self.best_bid_size_ <= 0:
                break
            # do nothing for orders at prices on non best level
            if price < self.dep_smv_.BestBidIntPrice():
                continue

            if price > self.dep_smv_.BestAskIntPrice():
                for order in self.smm_.intpx_to_bid_order_[price]:
                    this_size_executed_ = 0
                    # give the full aggressive fill to the buy order at ask price, and check for the book to give the execution size
                    this_size_executed_ = order.execute_remaining()
                    self.smm_.client_position_map_[order.server_assigned_client_id_] += this_size_executed_
                    self.smm_.client_price_bid_size_map_[order.server_assigned_client_id_][
                        order.int_price()] -= this_size_executed_
                    self.smm_.client_sum_bid_size_map_[order.server_assigned_client_id_] -= this_size_executed_

                    if this_size_executed_ > 0:
                        self.smm_.NotifyOnExecListener(order)
                        self.smm_.ComputePnl(order)

                    if order.size_remaining() == 0:
                        self.smm_.intpx_to_bid_order_[price].remove(order)
                    else:
                        print "The aggressive order got partial fill , this should not happen INVESTIGATE!"
                        print "Printing the order information"
                        print "The order price: ", order.price_
                        print "The current best ask price:", self.dep_smv_.BestAskIntPrice()

            # these orders are going to be executed at the mkt_wt_price
            elif price >= self.dep_smv_.BestBidIntPrice() and price <= self.dep_smv_.BestAskIntPrice():
                for order in self.smm_.intpx_to_bid_order_[price]:
                    # give execution at mkt price with some slippage
                    current_market_wt_price_ = self.dep_smv_.MktWtPrice()
                    # whether_to_give_execution_or_not_ = self.slippage_model.is_order_slip(self.dep_smv_,_market_update_info_)
                    whether_to_give_execution_or_not_ = True if randint(0, 1000) < 200 else False
                    if whether_to_give_execution_or_not_ == True:

                        # execute the all the order at mkt_wt_price
                        this_size_executed_ = order.execute_remaining()
                        # update the client position map
                        self.smm_.client_position_map_[order.server_assigned_client_id_] += this_size_executed_
                        # update the client_price_map
                        self.smm_.client_price_bid_size_map_[order.server_assigned_client_id_][
                            order.int_price()] -= this_size_executed_
                        # update the client_saci_sum_map
                        self.smm_.client_sum_bid_size_map_[order.server_assigned_client_id_] -= this_size_executed_
                        # change the order price to mkt_wt_price, this is needed for pnl computation
                        order.price_ = current_market_wt_price_

                        if this_size_executed_ > 0:
                            self.smm_.NotifyOnExecListener(order)
                            self.smm_.ComputePnl(order)

                        if order.size_remaining() == 0:
                            self.smm_.intpx_to_bid_order_[price].remove(order)
                        else:
                            # print "The aggressive buy order got partial fill , this should not happen INVESTIGATE!"
                            pass

        for price in sorted(self.smm_.intpx_to_ask_order_.keys()):
            if self.best_ask_size_ <= 0:
                break
            # orders at price higher than the best ask price are non best hence ignore
            if price > self.dep_smv_.BestAskIntPrice():
                continue
            # this means that this order now is an aggressive sell order
            if price < self.dep_smv_.BestBidIntPrice():
                # giving aggressive execution to all orders at this price
                for order in self.smm_.intpx_to_ask_order_[price]:

                    if order.int_price_ != price:
                        print "Mismatched order, the order price is not equal to the level it was added to in market model math"
                        continue

                    this_size_executed_ = 0
                    # give the full aggressive fill to the buy order at ask price, and check for the book to give the execution size
                    this_size_executed_ = order.execute_remaining()
                    self.smm_.client_position_map_[order.server_assigned_client_id_] += this_size_executed_
                    #                     print self.smm_.client_price_ask_size_map_[order.server_assigned_client_id_]
                    #                     print self.smm_.client_price_ask_size_map_[order.server_assigned_client_id_][order.int_price()]
                    if price in self.smm_.client_price_ask_size_map_[order.server_assigned_client_id_].keys():
                        self.smm_.client_price_ask_size_map_[order.server_assigned_client_id_][
                            price] -= this_size_executed_
                        self.smm_.client_sum_ask_size_map_[order.server_assigned_client_id_] -= this_size_executed_
                    else:
                        continue
                    # Notify the listeners
                    if this_size_executed_ > 0:
                        self.smm_.NotifyOnExecListener(order)
                        self.smm_.ComputePnl(order)

                    if order.size_remaining() == 0:
                        self.smm_.intpx_to_ask_order_[price].remove(order)
                    else:
                        print "The aggressive order got partial fill , this should not happen INVESTIGATE!"
                        print "Printing the order information"
                        print "The order price: ", order.price_
                        print "The current best ask price:", self.dep_smv_.BestBidIntPrice()
            # if the order is at a price between the bid and the ask price , then the execution is at mkt_wt_price
            elif price <= self.dep_smv_.BestAskIntPrice() and price >= self.dep_smv_.BestBidIntPrice():
                for order in self.smm_.intpx_to_ask_order_[price]:
                    # give execution to these orders at mkt price with some slippage
                    current_market_wt_price_ = self.dep_smv_.MktWtPrice()
                    # whether_to_give_execution_or_not_ = self.slippage_model.is_order_slip(self.dep_smv_,_market_update_info_)
                    whether_to_give_execution_or_not_ = True if randint(0, 1000) < 200 else False
                    if whether_to_give_execution_or_not_ == True:
                        # call the order exec function
                        this_size_executed_ = order.execute_remaining()
                        # update the client position map
                        self.smm_.client_position_map_[order.server_assigned_client_id_] += this_size_executed_
                        # update the client_price_map
                        # here since i am giving fill at mkt price I have to check the order at bid or ask side

                        # check for buy order
                        if order.int_price() in self.smm_.client_price_bid_size_map_[
                            order.server_assigned_client_id_].keys():
                            self.smm_.client_price_bid_size_map_[order.server_assigned_client_id_][
                                order.int_price()] -= this_size_executed_
                            # update the client_saci_sum_map
                            self.smm_.client_sum_bid_size_map_[order.server_assigned_client_id_] -= this_size_executed_
                            # change the order price to mkt_wt_price, this is needed for pnl computation
                            order.price_ = current_market_wt_price_
                            # remove the order from the bid map as full execution given
                            #                             self.smm_.client_price_bid_size_map_[order.server_assigned_client_id_][order.int_price()].remove(order)
                            self.smm_.intpx_to_ask_order_[price].remove(order)
                        elif order.int_price() in self.smm_.client_price_ask_size_map_[
                            order.server_assigned_client_id_].keys():
                            self.smm_.client_price_ask_size_map_[order.server_assigned_client_id_][
                                order.int_price()] -= this_size_executed_
                            # update the client_saci_sum_map
                            self.smm_.client_sum_ask_size_map_[order.server_assigned_client_id_] -= this_size_executed_
                            # change the order price to mkt_wt_price, this is needed for pnl computation
                            order.price_ = current_market_wt_price_
                            # removing the order from the ask map as full execution given
                            #                             self.smm_.client_price_ask_size_map_[order.server_assigned_client_id_][order.int_price()].remove(order)
                            self.smm_.intpx_to_ask_order_[price].remove(order)
                        else:
                            print "The order not present hence removing that order"
                            self.smm_.intpx_to_ask_order_[price].remove(order)

                        if this_size_executed_ > 0:
                            self.smm_.NotifyOnExecListener(order)
                            # self.smm_.ComputePnl(order)


    def FetchOrder(self, _buysell_, _int_price_, _server_assigned_order_sequence_):
        pass

