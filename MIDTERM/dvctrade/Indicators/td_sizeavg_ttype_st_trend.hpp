/**
    \file Indicators/td_sizeavg_ttype_st_trend.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/TradeUtils/time_decayed_trade_info_manager.hpp"
#include "dvctrade/Indicators/recent_simple_range_measure.hpp"
#include "dvctrade/Indicators/trade_adjusted_simple_trend.hpp"

namespace HFSAT {

/// Indicator returning time decayed EW Size avg of trade type
/// LT indicator = ( Sum of trade type and trade size divided by sum of trade size ) * ( ( lt_fractional_seconds_ /
/// recent_simple_range_measure.range_calc_seconds() ) * recent_simple_range_measure.recent_range() )
/// ST indicator = TradeAdjustedSimpleTrend ( st_fractional_seconds_, st_trade_seconds_ )
class TDSizeAvgTTypeStTrend : public CommonIndicator,
                              public RecentSimpleRangeMeasureListener,
                              public IndicatorListener {
 protected:
  const SecurityMarketView& indep_market_view_;

  double lt_fractional_seconds_;
  double st_factor_;
  double st_fractional_seconds_;
  double st_trade_seconds_;

  RecentSimpleRangeMeasure& recent_simple_range_measure_;
  TimeDecayedTradeInfoManager& lt_time_decayed_trade_info_manager_;
  TradeAdjustedSimpleTrend& st_trade_adjusted_simple_trend_;

  double lt_range_;
  double lt_bias_;
  double st_trend_;

 protected:
  TDSizeAvgTTypeStTrend(DebugLogger& _dbglogger_, const Watch& _watch_,
                        const std::string& concise_indicator_description_, SecurityMarketView& _indep_market_view_,
                        double lt_fractional_seconds_, double st_factor_, double st_fractional_seconds_,
                        double st_trade_seconds_, PriceType_t _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static TDSizeAvgTTypeStTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                  const std::vector<const char*>& _tokens_,
                                                  PriceType_t _basepx_pxtype_);

  static TDSizeAvgTTypeStTrend* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  SecurityMarketView& _indep_market_view_,
                                                  double lt_fractional_seconds_, double st_factor_,
                                                  double st_fractional_seconds_, double st_trade_seconds_,
                                                  PriceType_t _price_type_);

  ~TDSizeAvgTTypeStTrend() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  void OnRangeUpdate(const unsigned int r_security_id_, const double& _new_range_value_);

  // functions
  static std::string VarName() { return "TDSizeAvgTTypeStTrend"; }

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);
};
}
