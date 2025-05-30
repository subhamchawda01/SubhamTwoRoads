/**
    \file Indicators/trend_adjusted_self_position.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_TREND_ADJUSTED_SELF_POSITION_H
#define BASE_INDICATORS_TREND_ADJUSTED_SELF_POSITION_H

#include "dvctrade/Indicators/common_indicator.hpp"

namespace HFSAT {

/// Indicator used to compute the sum of
/// (i) an exponentially time-decayed trend of position changes
/// (ii) current position
/// Hence this is sort of ashort term projection of where position is heading to
/// based on current trend of position chage
/// ( Uses the same trend decaying logic as simple_trend or scaled_trend. )
/// Aim of this indicator is to catch fast trends in position changes,
///  and factor that into the total risk of a position
/// Hence indicator_value_ = current_indep_position_ + ( trend_factor_ * "position trend" )
class TrendAdjustedSelfPosition : public CommonIndicator {
 protected:
  const double trend_factor_;
  // computational variables
  double moving_avg_position_;

  double last_position_recorded_;
  double current_indep_position_;

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static TrendAdjustedSelfPosition* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                      const std::vector<const char*>& _tokens_,
                                                      PriceType_t _basepx_pxtype_);

  static TrendAdjustedSelfPosition* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                      PromOrderManager& _indep_prom_order_manager_,
                                                      const double _fractional_seconds_, const double _trend_factor_,
                                                      PriceType_t _basepx_pxtype_);

 protected:
  TrendAdjustedSelfPosition(DebugLogger& _dbglogger_, const Watch& _watch_,
                            const std::string& concise_indicator_description_,
                            PromOrderManager& _indep_prom_order_manager_, const double _fractional_seconds_,
                            const double _trend_factor_);

 public:
  ~TrendAdjustedSelfPosition() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_) {}
  void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_);
  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);
  // functions

  // functions
  static std::string VarName() { return "TrendAdjustedSelfPosition"; }

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_TREND_ADJUSTED_SELF_POSITION_H
