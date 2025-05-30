

class CommonIndicator(object):

    def __init__(self):
        self.indicator_listeners_ = []

    def OnMarketUpdate(self, _market_update_info_):
        raise NotImplemented()

    def on_trade_print(self, _trade_print_info_, _market_update_info_):
        pass


    def AddIndicatorListener(self,_indicator_index_,_new_listener_):
        self.indicator_listeners_.append((_indicator_index_,_new_listener_))

    def NotifyIndicatorListeners(self,_indicator_value_):
        for ind_index_,ind_listener_ in self.indicator_listeners_:
            ind_listener_.OnIndicatorUpdate(ind_index_,_indicator_value_)
