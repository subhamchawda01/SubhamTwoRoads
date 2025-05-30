/**
   \file Indicators/online_beta_trend.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 351, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_ONLINE_BETA_TREND_H
#define BASE_INDICATORS_ONLINE_BETA_TREND_H

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvctrade/Indicators/simple_trend_port.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"

namespace HFSAT {

/// Common interface extended by all classes listening to OnlineBetaTrend
/// to listen to changes in online computed correlation of the products
class OnlineBetaTrend : public IndicatorListener, public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  const unsigned int beta_history_msecs_;
  const SecurityMarketView &dep_market_view_;

  // computational variables
  double moving_covariance_;

  SimpleTrend &dep_trend_indicator_;
  CommonIndicator *indep_trend_indicator_;

  OfflineReturnsLRDB &lrdb_;
  int last_lrinfo_updated_msecs_;
  LRInfo current_lrinfo_;
  double current_projection_multiplier_;

  bool dep_updated_;
  bool indep_updated_;

  double last_dep_trend_recorded_;
  double last_indep_trend_recorded_;

  double current_dep_trend_;
  double current_indep_trend_;

  double dep_moving_avg_trend_;
  double indep_moving_avg_trend_;
  double moving_avg_squared_trend_indep_;
  double dep_indep_moving_avg_trend_;
  double min_unbiased_l2_norm_;
  std::string source_shortcode_;
  double current_correlation_ = 0;
  double upper_bound_;
  double lower_bound_;

  // functions
 public:
  static OnlineBetaTrend *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                            const SecurityMarketView &_dep_market_view_, std::string _source_shortcode_,
                                            const unsigned int t_trend_history_msecs_, const unsigned int _beta_secs_,
                                            PriceType_t _t_price_type_);

 protected:
  OnlineBetaTrend(DebugLogger &_dbglogger_, const Watch &_watch_, const std::string &concise_indicator_description_,
                  const SecurityMarketView &_dep_market_view_, std::string _source_shortcode_,
                  const unsigned int t_trend_history_msecs_, const unsigned int _beta_secs_,
                  PriceType_t _t_price_type_);

 public:
  ~OnlineBetaTrend() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                           const MarketUpdateInfo &_market_update_info_) {}
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_) {}

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);

  void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &_new_value_);

  inline void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &new_value_decrease_,
                                const double &new_value_nochange_, const double &new_value_increase_) {
    return;
  }

  void OnTimePeriodUpdate(const int num_pages_to_add_) { UpdateLRInfo(); }

  // functions
  static std::string VarName() { return "OnlineBetaTrend"; }

  static OnlineBetaTrend *GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                            const std::vector<const char *> &r_tokens_, PriceType_t _basepx_pxtype_);
  static void CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                std::vector<std::string> &_ors_source_needed_vec_,
                                const std::vector<const char *> &r_tokens_);

 protected:
  void InitializeValues();
  void UpdateLRInfo();
  inline void ComputeMultiplier() {
    current_projection_multiplier_ = current_lrinfo_.lr_coeff_;
    current_correlation_ = current_lrinfo_.lr_correlation_;
    if (current_projection_multiplier_ > 0 && current_correlation_ > 0) {
      if (fabs(current_correlation_) > 0.001) {
        upper_bound_ = current_projection_multiplier_ / current_correlation_;
        lower_bound_ = current_projection_multiplier_ * current_correlation_;
      }
    } else if (current_projection_multiplier_ < 0 && current_correlation_ < 0) {
      if (fabs(current_correlation_) > 0.001) {
        upper_bound_ = current_projection_multiplier_ * fabs(current_correlation_);
        lower_bound_ = current_projection_multiplier_ / fabs(current_correlation_);
      }
    }
  }
};
}

#endif  // BASE_INDICATORS_ONLINE_BETA_TREND_H
