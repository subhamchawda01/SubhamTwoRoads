/**
   \file SimMarketMaker/order_level_sim_market_maker_2.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */

#pragma once

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/OrderRouting/market_model.hpp"
#include "baseinfra/SimMarketMaker/base_sim_market_maker.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/SimMarketMaker/ors_message_stats.hpp"
#include "baseinfra/MarketAdapter/ose_trade_time_manager.hpp"

namespace HFSAT {

class OrderLevelSimMarketMaker2 : public BaseSimMarketMaker, public OrderLevelListenerSim, public TimePeriodListener {
 private:
  std::map<int, std::vector<SimMarketOrder*>, std::greater<int>> intpx_2_bid_mkt_order_vec_;
  std::map<int, std::vector<SimMarketOrder*>, std::less<int>> intpx_2_ask_mkt_order_vec_;

  SimpleMempool<SimMarketOrder> mkt_order_mempool_;

  int buy_side_trade_level_;
  int sell_side_trade_level_;

  HFSAT::OseTradeTimeManager& ose_trade_time_manager_;

 protected:
  int GetQueueSize(std::vector<SimMarketOrder*>& t_market_order_vec_);

  void SanitizeMarketOrderMaps();
  inline void LogBidSimOrder(int t_type_, int t_size_, int t_int_price_);
  inline void LogAskSimOrder(int t_type_, int t_size_, int t_int_price_);

  void AddMarketBid(int int_price, SimMarketOrder* market_order);
  void AddMarketAsk(int int_price, SimMarketOrder* market_order);

  void ExecuteBidsAboveIntPrice(int int_price);
  void ExecuteAsksAboveIntPrice(int int_price);

  void ExecuteBidsEqAboveIntPrice(int int_price);
  void ExecuteAsksEqAboveIntPrice(int int_price);

  inline void ExecuteBidsAtIntPrice(int t_int_price_, int t_size_);
  inline void ExecuteAsksAtIntPrice(int t_int_price_, int t_size_);

  void TagSimBids(int t_int_price_, int64_t t_order_id_);
  void TagSimAsks(int t_int_price_, int64_t t_order_id_);

  void ExecuteSimBid(int t_int_price_, int64_t t_order_id_, int t_size_executed_, bool t_assign_next_order_ = false);
  void ExecuteSimAsk(int t_int_price_, int64_t t_order_id_, int t_size_executed_, bool t_assign_next_order_ = false);

  inline int64_t GetNextMarketBid(int t_int_price_, int64_t t_order_id_);
  inline int64_t GetNextMarketAsk(int t_int_price_, int64_t t_order_id_);

  inline void DeleteMarketBid(int t_int_price_, int64_t t_order_id_);
  inline void DeleteMarketAsk(int t_int_price_, int64_t t_order_id_);

  void ChangeMarketBidSize(int t_int_price_, int64_t t_order_id_, int t_size_change_);
  void ChangeMarketAskSize(int t_int_price_, int64_t t_order_id_, int t_size_change_);

  void UpdateNumEvents();

  SimMarketOrder* GetNewMarketOrder(int t_int_price_, int t_size_, double t_price_, int64_t t_order_id_);

  // Separating the order management of different exchanges to easily change the code without worrying about its effect
  // on other exchange.
  void OnOrderAddEobi(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                      const int64_t t_order_id_, const double t_price_, const int t_size_);
  void OnOrderModifyEobi(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                         const int64_t t_old_order_id_, const double t_old_price_, const int t_old_size_,
                         const int64_t t_new_order_id_, const double t_new_price_, const int t_new_size_);
  void OnOrderDeleteEobi(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                         const int64_t t_order_id_, const double t_price_, const int t_size_);
  void OnOrderExecEobi(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                       const int64_t t_order_id_, const double t_price_, const int t_size_executed_,
                       const int t_size_remaining_);
  void OnTradeEobi(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
                   const TradeType_t t_buysell_);

  void OnOrderAddOse(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                     const int64_t t_order_id_, const double t_price_, const int t_size_);
  void OnOrderModifyOse(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                        const int64_t t_old_order_id_, const double t_old_price_, const int t_old_size_,
                        const int64_t t_new_order_id_, const double t_new_price_, const int t_new_size_);
  void OnOrderDeleteOse(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                        const int64_t t_order_id_, const double t_price_, const int t_size_);
  void OnOrderExecOse(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                      const int64_t t_order_id_, const double t_price_, const int t_size_executed_,
                      const int t_size_remaining_);
  void OnTradeOse(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
                  const TradeType_t t_buysell_);

  void OnOrderAddBmf(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                     const int64_t t_order_id_, const double t_price_, const int t_size_);
  void OnOrderModifyBmf(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                        const int64_t t_old_order_id_, const double t_old_price_, const int t_old_size_,
                        const int64_t t_new_order_id_, const double t_new_price_, const int t_new_size_);
  void OnOrderDeleteBmf(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                        const int64_t t_order_id_, const double t_price_, const int t_size_);
  void OnOrderExecBmf(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                      const int64_t t_order_id_, const double t_price_, const int t_size_executed_,
                      const int t_size_remaining_);
  void OnTradeBmf(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
                  const TradeType_t t_buysell_);

  SimMarketOrder* GetBuyOrderFromLevel(int t_level_);
  SimMarketOrder* GetSellOrderFromLevel(int t_level_);

 public:
  static OrderLevelSimMarketMaker2* GetUniqueInstance(DebugLogger& dbglogger, Watch& watch,
                                                      SecurityMarketView& dep_market_view, int _market_model_index_,
                                                      HFSAT::SimTimeSeriesInfo& t_sim_time_series_info_);
  static OrderLevelSimMarketMaker2* GetInstance(DebugLogger& dbglogger, Watch& watch,
                                                SecurityMarketView& dep_market_view, int _market_model_index_,
                                                HFSAT::SimTimeSeriesInfo& t_sim_time_series_info_);

  OrderLevelSimMarketMaker2(DebugLogger& dbglogger, Watch& watch, MarketModel _market_model_,
                            SecurityMarketView& _dep_market_view_, HFSAT::SimTimeSeriesInfo& t_sim_time_series_info_);

  virtual ~OrderLevelSimMarketMaker2();

  void OnOrderAdd(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                  const int64_t t_order_id_, const double t_price_, const int t_size_);

  void OnOrderModify(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                     const int64_t t_old_order_id_, const double t_old_price_, const int t_old_size_,
                     const int64_t t_new_order_id_, const double t_new_price_, const int t_new_size_);

  void OnOrderDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                     const int64_t t_order_id_, const double t_price_, const int t_size_);

  void OnOrderExec(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                   const int64_t t_order_id_, const double t_price_, const int t_size_executed_,
                   const int t_size_remaining_);

  void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
               const TradeType_t t_buysell_);
  void ResetBook(const unsigned int t_security_id_);

  void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_);

  void OnTimePeriodUpdate(const int num_pages_to_add_);

  bool UpdateQaQb(int t_saos_, BaseOrder* t_query_order_);
  bool UpdateBidQaQb(int t_saos_, BaseOrder* t_query_order_);
  bool UpdateAskQaQb(int t_saos_, BaseOrder* t_query_order_);

  int ProcessBidSendRequest(BaseSimOrder* sim_order);
  int ProcessAskSendRequest(BaseSimOrder* sim_order);

  std::string Shortcode() { return smv_.shortcode(); }

  std::pair<int, int> GetBidQaQb(int int_price_);
  std::pair<int, int> GetAskQaQb(int int_price_);
};
}
