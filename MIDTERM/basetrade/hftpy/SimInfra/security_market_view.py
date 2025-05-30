import sys
import os
import getpass
sys.path.append(os.path.join("/home", getpass.getuser(), "basetrade"))
from hftpy.SimInfra.market_update_info import MarketUpdateInfo
from hftpy.SimInfra.trade_print_info import TradePrintInfo

class SecurityMarketView:
    def __init__(self, _shortcode_, file_source_object_, _watch_=None):
        self.shortcode_ = _shortcode_
        self.watch_ = _watch_
        self.file_source_object_ = file_source_object_
        self.market_update_info_ = None
        # add the present object as listener to the filesource updates
        self.file_source_object_.AddExternalListener(self)
        self.is_ready_ = False
        self.smv_listeners = []
        self.smm_ = None
        self.count_ = 0
        self.bestask_price_ = 0
        self.bestask_size_ = 0
        self.bestask_int_price_ = 0
        self.bestbid_price_ = 0
        self.bestbid_size_ = 0
        self.bestbid_int_price_ = 0
        self.spread_increments_ = 0

    def AddSmvListener(self, _new_listener_):
        self.smv_listeners.append(_new_listener_)

    def NotifySmvListener(self):
        for listener in self.smv_listeners:
            listener.OnMarketUpdate(self.market_update_info_)

    def SetSimMarketMaker(self, _sim_market_maker_):
        self.smm_ = _sim_market_maker_

    def Shortcode(self):
        return self.shortcode_

    def BestBidPrice(self):
        return self.bestbid_price_

    def BestAskPrice(self):
        return self.bestask_price_

    def BestBidIntPrice(self):
        return self.bestbid_int_price_

    def BestAskIntPrice(self):
        return self.bestask_int_price_

    def BestBidSize(self):
        return self.bestbid_size_

    def BestAskSize(self):
        return self.bestask_size_

    def MinPriceIncrement(self):
        return self.min_price_increment_

    def UpdateL1Price(self):
        pass

    def OnMarketUpdate(self, mkt_update_info):
        self.bestask_price_ = mkt_update_info.GetBestAskPrice()
        self.bestask_size_ = mkt_update_info.GetBestAskSize()
        self.bestask_int_price_ = mkt_update_info.GetBestAskIntPrice()
        self.bestbid_price_ = mkt_update_info.GetBestBidPrice()
        self.bestbid_size_ = mkt_update_info.GetBestBidSize()
        self.bestbid_int_price_ = mkt_update_info.GetBestBidIntPrice()
        self.spread_increments_ = self.bestask_int_price_ - self.bestbid_int_price_
        self.mkt_update_info = mkt_update_info
        self.count_ += 1
        self.is_ready_ = True
        self.market_update_info_ = mkt_update_info
        self.NotifySmvListener()

    def MktWtPrice(self):
        return (self.BestAskSize() * self.BestBidPrice() + self.BestAskPrice() * self.BestBidSize()) / (self.BestAskSize() + self.BestBidSize())

    def MidPrice(self):
        return 0.5 * (self.BestBidPrice() + self.BestAskPrice())

    def OnTradePrint(self):
        # update trade_print_info
        # notify smv listeners
        pass
