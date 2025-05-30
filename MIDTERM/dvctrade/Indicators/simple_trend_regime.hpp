/**
   \file Indicators/offline_breakout_adjusted_pairs.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvctrade/OfflineUtils/periodic_l1norm_utils.hpp"

#define NUM_DAYS_HISTORY 20
#define BREAKOUT_TREND_SECONDS 900

namespace HFSAT {

/// Indicates the regime ( 1 or 2 ) based on current simple trend and historical average
class SimpleTrendRegime : public IndicatorListener, public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;

  double trend_ratio_thresh_;
  double dep_price_trend_;

  double dep_l1_norm_ratio_;
  double indep_l1_norm_ratio_;

  double dep_mean_l1_norm_;
  unsigned int pred_mode_;

  SimpleTrend* p_dep_indicator_;

 protected:
  SimpleTrendRegime(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                    const SecurityMarketView& _dep_market_view_, double _fractional_seconds_,
                    double _trend_ratio_thresh_, PriceType_t _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static SimpleTrendRegime* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                              const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static SimpleTrendRegime* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                              const SecurityMarketView& _dep_market_view_, double _fractional_seconds_,
                                              double _trend_ratio_thresh_, PriceType_t _price_type_);

  ~SimpleTrendRegime() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_);

  void OnVolumeRatioUpdate(const unsigned int r_security_id_, const double& r_new_volume_ratio_);

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_){};
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
    std::vector<std::string> core_shortcodes_;
    GetCoreShortcodes(r_dep_shortcode_, core_shortcodes_);
    // here tokes 3 and 4 are important
    // if ( ( tokens_.size() > 3u ) &&
    // 	   ( VectorUtils::LinearSearchValue ( core_shortcodes_, std::string(tokens_[3]) ) ) )
    // 	{ return true ; }
    if ((tokens_.size() > 4u) && (VectorUtils::LinearSearchValue(core_shortcodes_, std::string(tokens_[4])))) {
      return true;
    }
    return false;
  }

  static std::string VarName() { return "SimpleTrendRegime"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();

  /// Multiplier :
  /// From price changes, we compute the coefficient and corerlation through Single Linear Regression
  /// < lr_coeff_, lr_correlation_ > = GetSLRCoeffCorrelation ( dep_price_change_vector_, indep_price_change_vector_ )
  /// For SimpleTrendRegime lr_coeff_/fabs(lr_correlation_) is sort of the lr_coeff_ as it would be if indep and dep are
  /// either perfectly correlated or
  /// perfectly anti-correlated ( correation = -1 )
};
}
