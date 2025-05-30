/**
   \file Indicators/time_based_regime_binary.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 351, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

namespace HFSAT {

class TimeBasedRegimeBinary : public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  unsigned int* regime_times_;
  int current_time_slot_;
  int next_time_;

 protected:
  TimeBasedRegimeBinary(DebugLogger& _dbglogger_, const Watch& _watch_,
                        const std::string& concise_indicator_description_, unsigned int* regime_times_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static TimeBasedRegimeBinary* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                  const std::vector<const char*>& _tokens_,
                                                  PriceType_t _basepx_pxtype_);

  static TimeBasedRegimeBinary* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                  unsigned int* regime_times_, const int _num_time_tokens_);

  ~TimeBasedRegimeBinary() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_);

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  // functions
  /// Used in ModelCreator to see which shortcodes are core
  bool GetReadinessRequired(const std::string& r_dep_shortcode_, const std::vector<const char*>& tokens_) const {
    return false;
  }

  static std::string VarName() { return "TimeBasedRegimeBinary"; }

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    market_update_manager_.AddMarketDataInterruptedListener(this);
  }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
