/**
   \file SimMarketMaker/price_level_sim_market_maker.hpp

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

// For target price sim
#include "baseinfra/TradeUtils/time_decayed_trade_info_manager.hpp"
#include "baseinfra/SimMarketMaker/ors_message_stats.hpp"

#include "dvccode/CDef/random_number_generator.hpp"
#include "baseinfra/TradeUtils/moving_average.hpp"

namespace HFSAT {

/** \brief Simulating Market handling of orders. Primiarly ordained to PriceLevel market data
 *
 * Specific to a certain exchange_symbol_ or SMV.
 * It also listens to ORS replies for the following reasons :
 * (i) It can place ORS orders in the queue to make a better guess of when SimOrders are likely to get executed
 * (ii) Based on executions at prices which might come before market data updates,
 *      it can make a better estimate of what prices might already have become live / best level
 *      and lewtting the client cancel orders at those levels might be inaccurate.
 * In the initali implementation we are doing none of these
 */

class PriceLevelSimMarketMaker : public BaseSimMarketMaker,
                                 public SecurityMarketViewChangeListener,
                                 public TimePeriodListener {
 protected:
  const SecurityMarketView& dep_market_view_;

  bool masked_bids_;  ///< flag set to false normally and only set to true if any bid has been masked
  std::vector<int> masked_from_market_data_bids_map_;  ///< map from unique_client_id_ to bidsize at this price that
  /// this client has HIT already. Used to make sure that SMM does
  /// not give more than 1 aggressive executions for one unit size
  /// present in market
  bool masked_asks_;  ///< flag set to false normally and only set to true if any offer has been masked
  std::vector<int> masked_from_market_data_asks_map_;  ///< map from unique_client_id_ to asksize at this price that
  /// this client has LIFT already. Used to make sure that SMM does
  /// not give more than 1 aggressive executions for one unit size
  /// present in market

  int bestbid_int_price_;
  int bestask_int_price_;
  int bestbid_size_;
  int bestask_size_;

  int last_all_levels_queued_;

  std::map<int, int> saos_to_size_exec_real_;

  std::map<int, ttime_t> current_ask_trade_time_;
  std::map<int, ttime_t> current_bid_trade_time_;

  int last_bestbid_int_price_[2];
  int last_bestask_int_price_[2];

  double bid_tr_ratio_;
  double ask_tr_ratio_;
  double current_ask_trade_ratio_;
  double current_bid_trade_ratio_;

  int askside_trade_size_;
  int bidside_trade_size_;

  /* Trade based sim variables */
  std::map<int, double> current_ask_trade_ratio_vec_;
  std::map<int, double> current_bid_trade_ratio_vec_;
  /* Target price based variables */

  TimeDecayedTradeInfoManager& time_decayed_trade_info_manager_;
  double w1;
  double w2;
  double prev_dep_price_change_;
  double low_likelihood_thresh_;
  MovingAverage* moving_avg_util_;

  /* Variables used for matching market quotes with trades (only for CFE) */
  std::deque<CSM_MDS::CSMCommonStruct> upcoming_trades_;  // a lookahead buffer of upcoming trades
  HFSAT::BulkFileReader* lookahead_reader_ = NULL;        // Lookahead reader
  ttime_t last_market_quote_time_;                        // Time at which last market quote was seen
  bool is_dummy_trade_print_;
  int lookahead_msecs_;

  int last_ask_size_change_msecs_;
  int last_bid_size_change_msecs_;

  std::vector<bool> bid_side_priority_order_exists_map_;
  std::vector<int> bid_side_priority_order_size_map_;

  std::vector<bool> ask_side_priority_order_exists_map_;
  std::vector<int> ask_side_priority_order_size_map_;

  bool is_mov_avg_ready_;  // To keep track of moving average readiness

 protected:
  /** \brief constructor, Takes market model so that we can change the simmarketmaker and see results with different
   * marketmodels
   *
   * @param _dbglogger_ logger for error reporting
   * @param _watch_ TimeKeeper
   * @param _dep_market_view_ for finding executions, and queuesize monitoring
   * @param _market_model_ there are many possible market models ( SIM parameters ) for each security. One can load the
   * simtrader with the requested market_model by specifying the index of the one to be used
   */
  PriceLevelSimMarketMaker(DebugLogger& _dbglogger_, Watch& _watch_, MarketModel _market_model_,
                           SecurityMarketView& _dep_market_view_, HFSAT::SimTimeSeriesInfo& t_sim_time_series_info_);

  // int RestoreQueueSizes ( BaseSimOrder * p_sim_order_, const int t_posttrade_asksize_at_trade_price_, const int
  // trd_size_);
  // void BackupQueueSizes ( BaseSimOrder * p_sim_order_);
  void UpdateQueueSizesTargetPrice(int new_size_, int prev_size_, BaseSimOrder* p_sim_order_);
  void UpdateQueueSizesBasePriceBasedTargetPrice(int new_size_, int prev_size_, BaseSimOrder* p_sim_order_);
  void UpdateQueueSizesTradeBased(int new_size_, int prev_size_, BaseSimOrder* p_sim_order_);
  void UpdateQueueSizes(int new_size_, int prev_size_, BaseSimOrder* p_sim_order_);
  inline double func(double trdsz_) { return trdsz_ / sqrt(1 + pow(trdsz_, 2)); }

 public:
  /**
   * Needs to be implemented in derived classes since static.
   * @param _dbglogger_ logger for error reporting
   * @param _watch_ TimeKeeper
   * @param _dep_market_view_ MarketData, for finding executions, and queuesize monitoring
   * @param _market_model_index_ there are many possible market models ( SIM parameters ) for each security. One can
   * load the simtrader with the requested market_model by specifying the index of the one to be used
   */
  static PriceLevelSimMarketMaker* GetUniqueInstance(DebugLogger& dbglogger, Watch& watch,
                                                     SecurityMarketView& dep_market_view, int market_model_index,
                                                     SimTimeSeriesInfo& sim_time_series_info);

  static PriceLevelSimMarketMaker* GetInstance(DebugLogger& dbglogger, Watch& watch,
                                               SecurityMarketView& dep_market_view, int market_model_index,
                                               SimTimeSeriesInfo& sim_time_series_info);

  virtual ~PriceLevelSimMarketMaker();

  /** \brief SecurityMarketViewChangeListener::OnMarketUpdate */
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);

  /** \brief SecurityMarketViewChangeListener::OnTradePrint */
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);

  int Connect();

  /// from watch
  void OnTimePeriodUpdate(const int num_pages_to_add);

  void SubscribeL2Events(SecurityMarketView& dep_market_view);

  // For matching trade with market quote (only for CFE)
  TradeType_t MatchTradeAndQuote(int old_bestask_int_price_, int old_bestask_size_, int old_bestbid_int_price_,
                                 int old_bestbid_size_, int bestask_int_price_, int bestask_size_,
                                 int bestbid_int_price_, int bestbid_size_, CSM_MDS::CSMCommonStruct event_);

 protected:
  void CxlOrdersAboveBestLevel(const int max_conf_orders_above_best_level);
  void InitializeOnMktUpdate(const MarketUpdateInfo& market_update_info);
  void HKOSENonBestHandling();
  void BidUpdateBest(const MarketUpdateInfo& market_update_info, int bid_int_price,
                     std::vector<BaseSimOrder*>& bid_vector);
  void AskUpdateBest(const MarketUpdateInfo& market_update_info, int ask_int_price,
                     std::vector<BaseSimOrder*>& ask_vector);
  void BidUpdateImprove(std::vector<BaseSimOrder*>& bid_vector);
  void AskUpdateImprove(std::vector<BaseSimOrder*>& ask_vector);
  void BidUpdateAggress(int bid_int_price, std::vector<BaseSimOrder*>& bid_vector);
  void AskUpdateAggress(int ask_int_price, std::vector<BaseSimOrder*>& ask_vector);
};
}
