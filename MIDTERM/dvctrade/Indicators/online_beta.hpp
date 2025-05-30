/**
   \file Indicators/moving_correlation_cutoff.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 351, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_ONLINE_BETA_H
#define BASE_INDICATORS_ONLINE_BETA_H

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"

namespace HFSAT {

/// Common interface extended by all classes listening to OnlineBeta
/// to listen to changes in online computed correlation of the products
class OnlineBeta : public CommonIndicator, public SlowStdevCalculatorListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& indep_market_view_;

  PriceType_t price_type_;

  // computational variables
  double moving_covariance_;

  SlowStdevCalculator& dep_slow_stdev_calculator_;
  SlowStdevCalculator& indep_slow_stdev_calculator_;

  double dep_std_dev_;
  double indep_std_dev_;

  bool dep_updated_;
  bool indep_updated_;

  double last_dep_price_recorded_;
  double last_indep_price_recorded_;

  double current_dep_price_;
  double current_indep_price_;

  double dep_moving_avg_price_;
  double indep_moving_avg_price_;
  double dep_indep_moving_avg_price_;

  // functions
 public:
  static OnlineBeta* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                       const SecurityMarketView& _dep_market_view_,
                                       const SecurityMarketView& _indep_market_view_,
                                       const unsigned int t_trend_history_msecs_, PriceType_t _t_price_type_);

 protected:
  OnlineBeta(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
             const SecurityMarketView& _dep_market_view_, const SecurityMarketView& _indep_market_view_,
             const unsigned int t_trend_history_msecs_, PriceType_t _t_price_type_);

 public:
  ~OnlineBeta() {}

  // listener interface
  void OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_);
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_) {}

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);

  // functions
  static std::string VarName() { return "OnlineBeta"; }

  static OnlineBeta* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                       const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_);
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_ONLINE_BETA_H
