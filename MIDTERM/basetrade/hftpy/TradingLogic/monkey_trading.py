from random import randint
import os
import sys
import getpass
sys.path.append(os.path.join("/home", getpass.getuser(), "basetrade", "hftpy"))
from hftpy.TradingLogic.base_trading import BaseTrading

class RandomMonkey(BaseTrading):
    def _init__(self,_smm_,
                _base_model_math_,
                dep_smv_,
                watch,
                trading_start_time_,
                trading_end_time_,
                use_param,
                param_file_name=None):

        BaseTrading.__init__(self,_smm_,_base_model_math_,dep_smv_,watch,trading_start_time_,trading_end_time_,use_param,param_file_name)


    def OnModelUpdate(self, new_model_value_):
        self.sumvars = new_model_value_
        self.TradingLogic()


    def TradingLogic(self):
        #check if I should get flat or not
        if self.ShouldGetFlat():
            self.GetFlatTradingLogic()
            return
        else:
            #if my current position is +1 then place a sell trade with some probablity else send a buy trade with some probab
            if self.my_position==1:
                if randint(0,1000)<10:
                    #Send a sell trade
                    self.SendTrade(self.dep_smv.BestAskPrice(), self.dep_smvBestAskIntPrice(), 1, "S",
                                   self._server_assigned_client_id_)
                else:
                    return
            elif self.my_position==-1:
                if randint(0,1000)<10:
                    #Send a buy trade
                    self.SendTrade(self.dep_smv.BestBidPrice(),self.dep_smvBestBidIntPrice(),1,"B", self._server_assigned_client_id_)
                else:
                    return
