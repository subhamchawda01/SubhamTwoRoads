
class ComputePnl(object):
    def __init__(self,sim_market_maker):
        # key = order_id      value = [_price_,_size_executed_,_buy_sell_]
        self.order_map={}
        self.order_pnl = {}
        sim_market_maker.add_order_exec_listener(self)
        self.total_pnl_ = 0
        self.max_draw_down_ = 100000000
        self.max_pnl_ = -100000
    def _add_order_(self,order_id_,_price_,_size_executed_,_buy_sell_):
        if order_id_ not in self.order_map.keys():
            self.order_map[order_id_] = [_price_,_size_executed_,_buy_sell_]
    def on_exec_(self,order_id_,_price_,_size_executed_,_buy_sell_):
        if _buy_sell_ == "B":
            #if order exists in the map then this trade has to be a sell trade and hence compute the pnl
            if order_id_ in self.order_map.keys():
                order_buy_price_,order_buy_executed_,order_buy_sell_ = self.order_map[order_id_]
                if order_buy_sell_ == "S":
                    #compute the pnl of the order
                    self.order_pnl[order_id_] = (_price_ - order_buy_price_) * _size_executed_
                    #update the order map
                    self.order_map[order_id_][1] -=  _size_executed_
            #add the order in self.order_map
            else:
                self.order_map[order_id_] = [_price_,_size_executed_,_buy_sell_]
        elif _buy_sell_=="S":
            #if the order exists in the map then this trade has to be a buy trade and hence compute the pnl
            if order_id_ in self.order_map.keys():
                order_sell_price_,order_sell_size_executed_,order_buy_sell_ = self.order_map[order_id_]
                if order_buy_sell_ == "B":
                    self.order_pnl[order_id_] = 1

