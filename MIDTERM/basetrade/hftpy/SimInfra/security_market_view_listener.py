from abc import abstractmethod


class SecurityMarketViewListener():

    @abstractmethod
    def on_market_update(self, _market_update_info_):
        return

    @abstractmethod
    def on_trade_print(self, _trade_print_info_, _market_update_info_):
        return
