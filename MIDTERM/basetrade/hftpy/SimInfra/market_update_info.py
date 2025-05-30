from datetime import datetime
import time

class MarketUpdateInfo:

    def __init__(self, price_feed_list):

        self.shortcode_ = str(price_feed_list[5])
        self.time = float(price_feed_list[0])
        self.minimum_increment_price_ = float(price_feed_list[6])
        self.date = datetime.fromtimestamp(int(self.time/1000)+time.timezone).strftime("%Y%m%d")
        self.bestbid_price_ = float(price_feed_list[1])
        self.bestbid_size_ = float(price_feed_list[3])
        self.bestbid_int_price_ = int(round(self.bestbid_price_/self.minimum_increment_price_))
        self.bestask_price_ = float(price_feed_list[2])
        self.bestask_size_ = float(price_feed_list[4])
        self.bestask_int_price_ = int(round(self.bestask_price_/self.minimum_increment_price_))
        self.mkt_size_weighted_price_ = 0

    def GetBestBidPrice(self):
        return self.bestbid_price_

    def GetBestAskPrice(self):
        return self.bestask_price_

    def GetBestBidSize(self):
        return self.bestbid_size_

    def GetBestAskSize(self):
        return self.bestask_size_

    def GetBestAskIntPrice(self):
        return self.bestask_int_price_

    def GetBestBidIntPrice(self):
        return self.bestbid_int_price_

    def GetTime(self):
        return self.time



    def Dump(self):
        print('[' + str(self.bestbid_size_) + ' ' + str(self.bestbid_int_price_) + ' | ' + str(
            self.bestask_int_price_) + ' '
              + str(self.bestask_size_) + ']')
