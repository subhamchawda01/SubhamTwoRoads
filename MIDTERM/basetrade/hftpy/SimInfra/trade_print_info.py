
class TradePrintInfo:

    def __init__(self, _shortcode_):
        self.shortcode_ = _shortcode_
        self.buysell_ = ''
        self.trade_price_ = 0.0
        self.size_traded_ = 0
        self.int_trade_price_ = 0

    def dump(self):
        print('[' + self.buysell_ + ' ' + str(self.size_traded_) + ' ' + str(self.int_trade_price_) + ']')
