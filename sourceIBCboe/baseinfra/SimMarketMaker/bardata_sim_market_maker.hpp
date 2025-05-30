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

/** \brief Simulating Market handling of orders. Primiarly ordained to Bardata market data
 *
 * Specific to a certain exchange_symbol_ or SMV.
 * It also listens to ORS replies for the following reasons :
 * (i) It can place ORS orders in the queue to make a better guess of when SimOrders are likely to get executed
 * (ii) Based on executions at prices which might come before market data updates,
 *      it can make a better estimate of what prices might already have become live / best level
 *      and lewtting the client cancel orders at those levels might be inaccurate.
 * In the initali implementation we are doing none of these
 */

class BardataSimMarketMaker : public BaseSimMarketMaker,
                              public SecurityMarketViewBardataListener,
                              public TimePeriodListener {
 protected:
  const SecurityMarketView& dep_market_view_;

  char bardata_buffer_[1024];

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
  BardataSimMarketMaker(DebugLogger& _dbglogger_, Watch& _watch_, MarketModel _market_model_,
                           SecurityMarketView& _dep_market_view_, HFSAT::SimTimeSeriesInfo& t_sim_time_series_info_, bool _are_we_using_bardata_ = false);

 public:
  /**
   * Needs to be implemented in derived classes since static.
   * @param _dbglogger_ logger for error reporting
   * @param _watch_ TimeKeeper
   * @param _dep_market_view_ MarketData, for finding executions, and queuesize monitoring
   * @param _market_model_index_ there are many possible market models ( SIM parameters ) for each security. One can
   * load the simtrader with the requested market_model by specifying the index of the one to be used
   */
  static BardataSimMarketMaker* GetUniqueInstance(DebugLogger& dbglogger, Watch& watch,
                                                     SecurityMarketView& dep_market_view, int market_model_index,
                                                     SimTimeSeriesInfo& sim_time_series_info, bool _are_we_using_bardata_ = false);

  static BardataSimMarketMaker* GetInstance(DebugLogger& dbglogger, Watch& watch,
                                               SecurityMarketView& dep_market_view, int market_model_index,
                                               SimTimeSeriesInfo& sim_time_series_info, bool _are_we_using_bardata_ = false);

  virtual ~BardataSimMarketMaker();

  void OnBardataUpdate(const unsigned int _security_id_, const char* _bardata_line_);

  /// from watch
  void OnTimePeriodUpdate(const int num_pages_to_add);

  void SubscribeBardataEvents(SecurityMarketView& dep_market_view);

};
}
