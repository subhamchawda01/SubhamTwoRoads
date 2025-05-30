# from Indicators.simple_trend import SimpleTrend
import sys
import os
import getpass
user = getpass.getuser()
sys.path.append(os.path.join('/home', user, 'basetrade'))
from hftpy.SimInfra.base_order import BaseOrder
from hftpy.Indicators.common_indicator import CommonIndicator
import math


class SimpleTrend(CommonIndicator):
    def __init__(self, shortcode, lookback_seconds,shortcode_smv_map_,print_indicator_value_):
        CommonIndicator.__init__(self)
        dep_smv_ = shortcode_smv_map_[shortcode]
        self.dep_smv_ = dep_smv_
        # add self object as listener to the smv
        self.dep_smv_.AddSmvListener(self)
        self.moving_avg_price_ = 0
        self.last_price_recorded_ = 0
        self.current_indep_price_ = 0
        self.indicator_value_ = 0
        self.lookback_seconds = lookback_seconds
        self.decay_val_ = 1
        self.decay_vector_price_list_ = []
        self.print_indicator_value_ = print_indicator_value_

    def initialize_value(self):
        self.moving_avg_price_ = self.current_indep_price_
        self.last_price_recorded_ = self.current_indep_price_
        self.indicator_value_ = 0
        self.last_price_recorded_ = 0
        self.current_indep_price_ = 0

    def OnMarketUpdate(self, market_update_info_):
        self.current_indep_price_ = self.dep_smv_.MktWtPrice()
        #         print "SMV market price: ",self.dep_smv_.MktWtPrice()
        # Compute the Moving avg price
        self.ComputeMovingAvgPrice()
        # Compute the indicator value
        self.ComputeIndicatorValue()
        # Notify Indicator Listeners
        self.NotifyIndicatorListeners(self.indicator_value_)

    def ComputeMovingAvgPrice(self):
        # whether to initialize the list on mkt update or not, because right now it will take 30 sec for full update

        assert (len(self.decay_vector_price_list_) < self.lookback_seconds), "The length of the price list has to be less than 30"
        #         print self.decay_vector_price_list_
        if len(self.decay_vector_price_list_) < self.lookback_seconds - 1:
            self.decay_vector_price_list_.insert(0, self.decay_val_ * self.current_indep_price_)
        else:
            self.decay_vector_price_list_.pop()
            # add the latest price to the list
            self.decay_vector_price_list_.insert(0, self.decay_val_ * self.current_indep_price_)

        self.moving_avg_price_ = (sum(self.decay_vector_price_list_) / (len(self.decay_vector_price_list_)))

    def ComputeIndicatorValue(self):
        self.indicator_value_ = self.current_indep_price_ - self.moving_avg_price_
        if self.print_indicator_value_:
            print "The name of the indicator: ",self.lookback_seconds
            print "The indicator value: ",self.indicator_value_
            print



