import os
import sys
import getpass
sys.path.append(os.path.join("/home", getpass.getuser(), "basetrade", "hftpy"))
from hftpy.TradingLogic.base_trading import BaseTrading
class DirectionalAggressiveTrading(BaseTrading):
    def __init__(self, _smm_,
                 _base_model_math_,
                 _sim_pnl_,
                 dep_smv,
                 watch,
                 trading_start_time,
                 trading_end_time,
                 use_param,
                 param_file_name=None):

        BaseTrading.__init__(self, _smm_,
                             _base_model_math_,
                             _sim_pnl_,
                             dep_smv,
                             watch,
                             trading_start_time,
                             trading_end_time,
                             use_param,
                             param_file_name)

        self.threshold = 0.3
        self.max_position = 2

    def OnModelUpdate(self, new_model_value_):
        self.sumvars = new_model_value_
        self.TradingLogic()

    def TradingLogic(self):

        if self.ShouldGetFlat():
            self.GetFlatTradingLogic()
            return
        else:
            if self.sumvars < - self.threshold and abs(self.my_position) < self.max_position:
                self.SendTrade(self.dep_smv.BestAskPrice(), self.dep_smv.BestAskIntPrice(), 1, "S")
            elif self.sumvars >= self.threshold and abs(self.my_position) < self.max_position:
                self.SendTrade(self.dep_smv.BestBidPrice(), self.dep_smv.BestBidIntPrice(), 1, "B")
            elif abs(self.my_position) >= self.max_position:
                if self.my_position > 0:
                    self.SendTrade(self.dep_smv.BestAskPrice(), self.dep_smv.BestAskIntPrice(), 1, "S")
                elif self.my_position < 0:
                    self.SendTrade(self.dep_smv.BestBidPrice(), self.dep_smv.BestBidIntPrice(), 1, "B")
