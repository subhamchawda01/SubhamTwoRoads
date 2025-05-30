/**
    \file Indicators/volatility_regime.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/core_shortcodes.hpp"
#include "dvctrade/OfflineUtils/periodic_l1norm_utils.hpp"

#define NUM_DAYS_HISTORY 20

namespace HFSAT {

class VolatilityRegime : public IndicatorListener, public CommonIndicator {
 protected:
  // variables
  SecurityMarketView &indep_market_view_;

  int trend_history_msecs_;
  const PriceType_t price_type_;

  double _switch_threshold_;

  std::vector<SecurityMarketView *> smv;
  std::vector<double> trend;
  std::vector<double> l1_norm;
  std::vector<SimpleTrend *> trend_instance;
  std::vector<std::string> core_shortcode_vec_;
  unsigned int length;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                std::vector<std::string> &_ors_source_needed_vec_,
                                const std::vector<const char *> &_tokens_);

  static VolatilityRegime *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                             const std::vector<const char *> &_tokens_, PriceType_t _basepx_pxtype_);

  static VolatilityRegime *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                             SecurityMarketView &_indep_market_view_, double _fractional_seconds_,
                                             PriceType_t _price_type_, double switch_threshold);

 protected:
  VolatilityRegime(DebugLogger &_dbglogger_, const Watch &_watch_, const std::string &concise_indicator_description_,
                   SecurityMarketView &_indep_market_view_, double _fractional_seconds_, PriceType_t _price_type_,
                   double switch_threshold);

 public:
  ~VolatilityRegime() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                           const MarketUpdateInfo &_market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  void OnStdevUpdate(const unsigned int _security_id_, const double &_new_stdev_value_){};

  void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &_new_value_);

  inline void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &new_value_decrease_,
                                const double &new_value_nochange_, const double &new_value_increase_) {
    return;
  }

  // functions
  static std::string VarName() { return "VolatilityRegime"; }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues(){};
};
}
