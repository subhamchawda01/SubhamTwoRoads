/**
    \file Indicators/diff_price_type.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
//#pragma once
#ifndef BASE_INDICATORS_DYNAMICPRICEL1_H
#define BASE_INDICATORS_DYNAMICPRICEL1_H

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/l1_size_trend.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "baseinfra/TradeUtils/time_decayed_trade_info_manager.hpp"

namespace HFSAT {

/// Indicator returns indep_market_view_::price_type_ - indep_market_view_::basepx_pxtype_
/// Also basepx_pxtype_ changes by set_basepx_pxtype ( ) call
class DynamicPriceL1 : public CommonIndicator {
 protected:
  const SecurityMarketView& indep_market_view_;
  TimeDecayedTradeInfoManager& time_decayed_trade_info_manager_;
  double trade_price_avg_;
  double current_price_;
  double trade_fractional_seconds_;
  double percentile_;
  double high_trade_sz_;
  PriceType_t price_type_;
  PriceType_t basepx_type_;

 public:
  // functions

  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static DynamicPriceL1* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                           const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static DynamicPriceL1* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                           SecurityMarketView& _indep_market_view_, double _trade_fractional_seconds_,
                                           double percentile_, PriceType_t _price_type_, PriceType_t _basepx_type_);

  DynamicPriceL1(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                 SecurityMarketView& _indep_market_view_, double _trade_fractional_seconds_, double percentile_,
                 PriceType_t _price_type_, PriceType_t _basepx_pxtype_);

  ~DynamicPriceL1() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_);
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "DynamicPriceL1"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
  void GetHistoricalValues();
};
}

#endif  // BASE_INDICATORS_DYNAMICPRICEL1_H
