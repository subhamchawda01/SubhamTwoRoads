/**
    \file Indicators/simple_trend.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_CORRBASED_SIMPLE_TREND_H
#define BASE_INDICATORS_CORRBASED_SIMPLE_TREND_H
#include <fstream>
#include <cstring>
#include <iostream>

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#define CORR_INFO_FILE_PREFIX "/spare/local/tradeinfo/datageninfo/timed_corr_results/timed_corr_"
#define MAX_NUM_DAYS 10

namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially time-decaying moving average
class CorrBasedSimpleTrend : public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;
  const SecurityMarketView& dep_market_view_;

  const PriceType_t price_type_;

  // computational variables
  double moving_avg_price_;

  std::vector<int> start_times_;
  std::vector<int> end_times_;
  std::vector<double> corrs_;
  double max_corr_;
  bool use_corrs_;
  unsigned int index_;
  double mean_, stdev_;
  int date_today_;

  double last_price_recorded_;
  double current_indep_price_;
  unsigned int num_intervals_;
  int daycount_;
  double decay_factor_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static CorrBasedSimpleTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static CorrBasedSimpleTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 const SecurityMarketView& _indep_market_view_,
                                                 const SecurityMarketView& _dep_market_view_,
                                                 double _fractional_seconds_, PriceType_t _price_type_);

 protected:
  CorrBasedSimpleTrend(DebugLogger& _dbglogger_, const Watch& _watch_,
                       const std::string& concise_indicator_description_, const SecurityMarketView& _indep_market_view_,
                       const SecurityMarketView& _dep_market_view_, double _fractional_seconds_,
                       PriceType_t _price_type_);

 public:
  ~CorrBasedSimpleTrend() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "CorrBasedSimpleTrend"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
  double GetCorratTime(const Watch& _watch_);
  void SmoothenCorrs();

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_SIMPLE_TREND_H
