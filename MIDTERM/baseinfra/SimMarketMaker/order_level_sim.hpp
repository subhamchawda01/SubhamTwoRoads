/**
   \file SimMarketMaker/order_level_sim.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */

#pragma once
#include <math.h>
#include "dvccode/CommonDataStructures/simple_mempool.hpp"
#include "baseinfra/MarketAdapter/market_orders_view.hpp"
#include "baseinfra/SimMarketMaker/base_sim_market_maker.hpp"
#include "baseinfra/OrderRouting/market_model.hpp"
#include "baseinfra/OrderRouting/market_model_manager.hpp"

namespace HFSAT {

/*
 * This order level sim manages self order.
 * The sim subscribes to order level queue position updates
 * and updates queue size ahead and queue size behind.
 */
class OrderLevelSim : public BaseSimMarketMaker,
                      public QueuePositionChangeListener,
                      public TimePeriodListener,
                      public TradeGlobalListener {
 protected:
  MarketOrdersView* mov_;

  void BidQueuePos(QueuePositionUpdate pos_update);
  void AskQueuePos(QueuePositionUpdate pos_update);

  void UpdateBidsOrderAdd(int int_price, int position, int size);
  void UpdateAsksOrderAdd(int int_price, int position, int size);

  void UpdateBidsOrderRemove(int int_price, int position, int size);
  void UpdateAsksOrderRemove(int int_price, int position, int size);

  void UpdateBidsOrderExec(int int_price, int position, int size, bool agg_order_update);
  void UpdateAsksOrderExec(int int_price, int position, int size, bool agg_order_update);

  void AggBuyTrade(int int_price_, int size_);
  void AggSellTrade(int int_price_, int size_);

  void ProRataBidExec(int int_price_, int size_);
  void ProRataAskExec(int int_price_, int size_);

  void ProRataExec(int int_price_, int size_, std::vector<ExchMarketOrder*>& mkt_vec_);

  void LogUpdate(TradeType_t buysell, std::string update, int int_price, int position, int size);

  void LogOrderStatus(BaseSimOrder* sim_order, std::string update);

  void PosAdd(BaseSimOrder* sim_order, int size, int position);
  void PosDel(BaseSimOrder* sim_order, int size, int position);
  void PosExe(BaseSimOrder* sim_order, int size, int position);

  /*
     * Provides fills based on a 4 step logic
     * In first step fills are provided based on Top Order Priority
     * 2 step> fills are provided based on fifo using 40% of the remaining size of order
     * 3 step> fills are provided based on prorata using the remaining 60% in previous step
     * 4 step> if some orders do not get fills in pro rata and size remaining>0 one lot is given to unfilled orders in
     * previous steps called (pro rata levelling)
     * 5th step> if order remaining size is still >0 fills are again provided based on FIFO logic
     */
  void SplitFIFOProRata(BaseSimOrder* sim_order, int size);
  // Provides fills based on a three step logic(used by CME)
  // first the top order i.e. the order that creates the price level gets the fill till its order size is not satisfied
  // second the orders get the fills in proportion of their sizes such that fill is greater than 2
  // third the orders are fulfilled based on a simple FIFO logic

  void PosExeProRataResidualFIFO(BaseSimOrder* sim_order, int size);

  // Give aggressive fills in case of cross
  // situation of sim orders in OrderExec event
  // or OrderAdd event
  void FillAggAsks(int int_price, int size);
  void FillAggBids(int int_price, int size);

  void UpdateNumEvents();

 public:
  static OrderLevelSim* GetUniqueInstance(DebugLogger& dbglogger, Watch& watch, SecurityMarketView& dep_market_view,
                                          int market_model_index, SimTimeSeriesInfo& sim_time_series_info);

  static OrderLevelSim* GetInstance(DebugLogger& dbglogger, Watch& watch, SecurityMarketView& dep_market_view,
                                    int market_model_index, SimTimeSeriesInfo& sim_time_series_info);

  OrderLevelSim(DebugLogger& dbglogger, Watch& watch, SecurityMarketView& dep_market_view, MarketModel market_model,
                SimTimeSeriesInfo& sim_time_series_info);

  ~OrderLevelSim() {}

  // Time period update callback handler
  void OnTimePeriodUpdate(const int num_pages_to_add);

  void QueuePosChange(QueuePositionUpdate position_update);

  int ICEOrderSizeMatch(int const size_total_resting_order_, int const size_preceding_order_,
                        int const size_current_order_, int const market_order_size_to_fill_, int base_power);
  void ICEProRata(BaseSimOrder* sim_order, int size, int base_power);

  // void ProcessRequestQueue();
  int ProcessBidSendRequest(BaseSimOrder* sim_order);
  int ProcessAskSendRequest(BaseSimOrder* sim_order);

  void OnTrade(const unsigned int security_id, const double trade_price, const int trade_size,
               const TradeType_t buysell);

  int GetSizeAtIntPrice(TradeType_t buysell, int int_price) override;
};
}
