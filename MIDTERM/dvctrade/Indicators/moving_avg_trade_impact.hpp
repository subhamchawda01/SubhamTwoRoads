/*i*
    \file Indicators/moving_avg_trade_size.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include "dvccode/CommonDataStructures/simple_mempool.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include <deque>

namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially time-decaying moving average
class MovingAvgTradeImpact : public CommonIndicator {
 protected:
  // variables
  SecurityMarketView& indep_market_view_;

  int page_width_msecs_;
  int buysell_;  // 0:buy side,1:sell side
  double decay_price_factor_;

  std::vector<double> decay_price_factor_vec_;

  int last_int_trade_price_;
  double current_trade_impact_;
  int current_int_trade_price_;
  int sum_trades_;
  int current_book_size_;
  int current_trade_size_;

  struct TradeElem {
    int time_msecs_;
    double impact_;
    int size_;
    TradeElem(int _time_msecs_, double _impact_, int _size_)
        : time_msecs_(_time_msecs_), impact_(_impact_), size_(_size_) {}
  };

  std::deque<TradeElem*> trades_queue_;
  SimpleMempool<TradeElem> trades_queue_mempool_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static MovingAvgTradeImpact* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static MovingAvgTradeImpact* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 SecurityMarketView& _indep_market_view_, int _window_msecs_,
                                                 int buysell_, double decay_factor_);

 protected:
  MovingAvgTradeImpact(DebugLogger& _dbglogger_, const Watch& _watch_,
                       const std::string& concise_indicator_description_, SecurityMarketView& _indep_market_view_,
                       int _window_msecs_, int buysell_, double decay_factor_);

  void RescaleFactorVec(int till_value);

 public:
  ~MovingAvgTradeImpact() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_);

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}
  inline double GetTradeSizeFactor() { return indicator_value_; }
  // functions
  static std::string VarName() { return "MovingAvgTradeImpact"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
};
}
