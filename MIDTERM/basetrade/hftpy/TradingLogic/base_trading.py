import sys
import os
import getpass
sys.path.append(os.path.join("/home", getpass.getuser(), "basetrade", "hftpy"))
from hftpy.SimInfra.order_exec_listener import OrderExecListener
from hftpy.SimInfra.security_market_view_listener import SecurityMarketViewListener
# from TradingLogic.paramset import ParamSet
n2d_file_ = os.path.join("/home", getpass.getuser(), "basetrade", "hftpy","shc_n2d_file")



class BaseTrading(object):
    _server_assigned_client_id_ = 1

    def __init__(self, _smm_,
                 _base_model_math_,
                 _sim_pnl_,
                 dep_smv,
                 watch,
                 trading_start_time,
                 trading_end_time,
                 use_param,
                 param_file_name=None):
        '''




        :param _smm_: 
        :param _base_model_math_: this is an object of the base model math class. So this has been initialized earlier and passed
        :param _base_model_action_: this is an object of the base action class. Not Implemented as of now
        :param _sim_pnl_: 
        :param dep_smv: 
        :param watch: 
        :param trading_start_time: 
        :param trading_end_time: 
        :param use_param: 
        :param param_file_name: 
        '''

        # assign the SACI
        self._server_assigned_client_id_ = BaseTrading._server_assigned_client_id_
        # Update the SACI
        self.UpdateSACI()

        self.smm_ = _smm_
        # add this object as listener to smm_ order execution
        self.smm_.AddOrderExecListener(self)
        # add this object to listen to pnl update
        self.smm_.AddPnlUpdateListener(self, self._server_assigned_client_id_)
        self.base_model_math_ = _base_model_math_
        # add the basetrade as listener to the model output
        self.base_model_math_.AddModelListener(self)
        self.count = 0

        # add the pnl map
        self.pnl_map_ = {}
        self.pnl_map_["pnl"] = 0
        self.pnl_map_["position_list"] = []
        self.pnl_map_["price_list"] = []

        # Initialize the caos
        self._server_assigned_client_id_ = 1
        self.sim_pnl_ = _sim_pnl_
        self.cycle_pnl_ = 0
        self.total_pnl_list_ = []
        self.first_order_ = True
        self.my_position = 0
        self.watch = watch
        self.base_price_ = 0
        # add this object as listener to the watch
        # this is pointless now as there is no watch
        if self.watch is not None:
            self.watch.AddTimePeriodListener(self)
        self.is_ready = False
        self.trading_start_time = trading_start_time
        self.trading_end_time = trading_end_time

        # only listen to the self book
        self.dep_smv = dep_smv
        # add this object as listener to SMV
        self.dep_smv.AddSmvListener(self)
        self.use_param = use_param
        self.model_prediction_ = 0
        self.n2d_ = self.GetN2DShortcode()
        print "My n2D is: ", self.n2d_
        if self.use_param:
            self.param_file_name = param_file_name
            self.LoadParamSet()

        # conditions for gettting flat
        self.get_flat = False
        self.get_flat_due_to_max_loss = False
        self.get_flat_due_to_max_position = False
        self.get_flat_due_to_exchange_close = False

        # variables updated by the model_math
        self.target_price = 0
        self.sumvars = 0

        # variables to keep track whether to place buy an order or not
        self.top_bid_place = False
        self.top_bid_keep = False
        self.top_bid_improve = False

        # variables to keep track whether to place sell order or not
        self.top_ask_place = False
        self.top_ask_keep = False
        self.top_ask_improve = False

        # variables for aggressing
        self.top_ask_lift = False
        self.top_bid_lift = False

        # variables for keeping track of last trade event
        self.last_trade_price = 0
        self.last_buy_msecs = 0
        self.last_sell_msecs = 0
        self.last_aggress_buy_msec = 0
        self.last_aggress_sell_msecs = 0

    def OnExec(self, order):
        print "MY ORDER GOT EXECUTED!!!!!"
        print order.dump()
        if self.first_order_ == True:

            self.last_traded_price_ = order.price_
            self.first_order_ = False
            self.my_position = 0
            self.cycle_pnl_ = 0
        else:
            if self.my_position > 0:
                current_pnl_ = abs(float(self.my_position)) * (order.price_ - self.last_traded_price_)
                self.cycle_pnl_ += current_pnl_
                self.last_trade_price = order.price_
                print "My cycle pnl: ", self.cycle_pnl_ * float(self.n2d_)
                self.count += 1
            elif self.my_position < 0:
                current_pnl_ = abs(float(self.my_position)) * (self.last_traded_price_ - order.price_)
                self.cycle_pnl_ += current_pnl_
                self.last_trade_price = order.price_
                print "My cycle pnl: ", self.cycle_pnl_ * float(self.n2d_)
                self.count += 1

            elif self.my_position == 0:
                self.current_pnl_ = 0
                print "My cycle pnl: ", 0



        security_name_ = order.security_name_
        buysell_ = order.buysell_
        price_ = order.price_
        size_remaining_ = order.size_remaining_
        size_executed_ = order.size_executed_
        int_price_ = order.int_price_

        if order.buysell() == "B":
            new_position = self.my_position + order.size_requested_
        else:
            new_position = self.my_position - order.size_requested_

        self.my_position = new_position

        print "My new position is: ", self.my_position

        if self.my_position == 0:
            self.first_order_ = True
            self.total_pnl_list_.append(self.cycle_pnl_ * float(self.n2d_))
            print "My total pnl: ", sum(self.total_pnl_list_)

            #         new_position = self.my_position+size_executed_
            #         if (new_position>self.my_position):
            #             if (new_position>=0):
            # #                 self.last_buy_msecs=self.watch.msec_from_midnight()
            # #                 self.last_buy_int_price=int_price
            #                 pass
            #             self.last_sell_msecs=0
            #         elif (new_position<self.my_position):
            #             if (new_position<=0):
            # #                 self.last_sell_msecs=self.watch.msec_from_midnight()
            # #                 self.last_sell_int_price=int_price
            #                 pass
            #             self.last_buy_msecs=0

            # Print the pnl due to exchange close

    def OnMarketUpdate(self, _market_update_info_):
        self.base_price_ = self.dep_smv.MktWtPrice()
        self.TradingLogic()

    def OnModelUpdate(self, new_model_value_):
        raise NotImplementedError()

    def OnTradePrint(self, _trade_print_info_, _market_update_info_):
        pass

    def OnTimePeriodUpdate(self):
        pass

    def OnModelPrediction(self, new_target_price, new_sum_vars):
        if (not self.is_ready):
            if (self.watch.msec_from_midnight() > self.trading_start_time and
                        self.watch.msec_from_midnight() < self.trading_end_time and
                    self.dep_smv.is_ready() and
                        new_target_price > self.dep_smv.GetBestBidPrice() and
                        new_target_price < self.dep_smv.GetBestAskPrice()):
                self.is_ready = True
        else:
            self.target_price = new_target_price
            self.sumvars = new_sum_vars
            # self.sumvars=new_sum_vars
            if (self.ShouldGetFlat()):
                self.GetFlatTradingLogic()
            else:
                self.TradingLogic()

    def GetFlatTradingLogic(self):
        '''this function would interact with the sim market maker for sending passive get flat'''
        pass

    def GetFlatAggressive(self):
        '''
        This function would interact with the sim market maker for sending aggressive order.
        '''
        pass

    def SendTrade(self, _price_, _int_price_, _size_requested_, _buy_sell_):
        '''
        This function is used to send trade to the sim_market maker in 

        _price_, _int_price_, _size_requested_, _buysell_, _server_assigned_client_id_

        '''
        self.smm_.SendTrade(_price_, _int_price_, _size_requested_, _buy_sell_, self._server_assigned_client_id_)

    def CancelOrder(self):
        '''
        This function is used to cancel trade

        '''
        pass

    def TradingLogic(self):
        raise NotImplementedError()

    def ShouldGetFlat(self):
        return False
        # if self.use_param:
        #     if self.my_position >= self.paramset.max_position:
        #         self.get_flat_due_to_max_position = True
        #     elif self.watch.msec_from_midnight() > self.trading_end_time:
        #         self.get_flat_due_to_exchange_close = True
        #     elif -self.paramset.max_loss > self.sim_pnl_:
        #         self.get_flat_due_to_max_loss = True
        #     self.get_flat = self.get_flat_due_to_max_position or self.get_flat_due_to_exchange_close or self.get_flat_due_to_max_loss
        #     return self.get_flat
        # else:
        #     # if the user is not using the existing setup of param based get_flat then he has to provide his own
        #     # get_flat logic
        #     if self.watch.msec_from_midnight() > self.trading_end_time:
        #         self.get_flat_due_to_exchange_close = True
        #     elif -self.custom_max_loss > self.sim_pnl_:
        #         self.get_flat_due_to_max_loss = True
        #     return self.GetFlatLogic() or self.get_flat_due_to_exchange_close or self.get_flat_due_to_max_loss

    def OnGlobalPnlChange(self, new_pnl):
        self.sim_pnl_ = new_pnl
        # check if i need to get flat due to max loss
        self.ShouldGetFlat()

    def UpdateTradeVarSet(self):
        pass

    def OnModelUpdate(self, _new_val_):
        self.model_prediction_ = _new_val_

    def OnPnlUpdate(self, order):
        pass

    def UpdateSACI(self):
        BaseTrading._server_assigned_client_id_ += 1

    def GetN2DShortcode(self):
        with open(n2d_file_) as f:
            lines = f.read().splitlines()
        for elem in lines:
            temp_data = elem.split()
            if temp_data[0] == self.dep_smv.shortcode_:
                return float(temp_data[1])

    def LoadParamSet(self):
        # check if the param file exists or not
        if not os.path.exist(self.param_file_name):
            print("param file doesnot exist")
            sys.exit(1)
        else:
            # load the param in paramset class
            # self.paramset = ParamSet(self.param_file_name)
            pass
