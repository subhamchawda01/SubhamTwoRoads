/**
    \file Indicators/self_position_simple_trend.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_SELF_POSITION_SIMPLE_TREND_H
#define BASE_INDICATORS_SELF_POSITION_SIMPLE_TREND_H

#include "dvctrade/Indicators/common_indicator.hpp"

namespace HFSAT {

/// Indicator used to compute an exponentially time-decayed trend of position changes
/// Uses the same trend decaying logic as simple_trend or scaled_trend.
class SelfPositionSimpleTrend : public CommonIndicator {
 protected:
  // computational variables
  double moving_avg_position_;

  double last_position_recorded_;
  double current_indep_position_;

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static SelfPositionSimpleTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                    const std::vector<const char*>& _tokens_,
                                                    PriceType_t _basepx_pxtype_);

  static SelfPositionSimpleTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                    PromOrderManager& _indep_prom_order_manager_,
                                                    const double _fractional_seconds_, PriceType_t _basepx_pxtype_);

 protected:
  SelfPositionSimpleTrend(DebugLogger& _dbglogger_, const Watch& _watch_,
                          const std::string& concise_indicator_description_,
                          PromOrderManager& _indep_prom_order_manager_, const double _fractional_seconds_);

 public:
  ~SelfPositionSimpleTrend() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_);
  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  inline void OnMarketDataResumed(const unsigned int _security_id_) {}
  // functions

  // functions
  static std::string VarName() { return "SelfPositionSimpleTrend"; }

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_SELF_POSITION_SIMPLE_TREND_H
