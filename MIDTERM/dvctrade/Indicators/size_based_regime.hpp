/**
   \file Indicators/size_based_regime.hpp

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
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"
#include "dvctrade/OfflineUtils/periodic_l1norm_utils.hpp"
#include "dvctrade/Indicators/volume_ratio_calculator.hpp"
#include "dvctrade/Indicators/book_info_manager.hpp"

#define NUM_DAYS_HISTORY 20
#define BREAKOUT_TREND_SECONDS 900

namespace HFSAT {

class SizeBasedRegime : public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  SecurityMarketView& dep_market_view_;

  double current_projection_multiplier_;
  double current_projected_trend_;

  unsigned int pred_mode_;
  int avg_bid_level_size_;
  int avg_ask_level_size_;

  int moving_average_bid_size_;
  int moving_average_ask_size_;

  int cumulative_bid_size_;
  int cumulative_ask_size_;

  int num_event_count_;
  double stdev_duration_;

  double fractional_seconds_;
  int last_new_page_msecs_;
  int page_width_msecs_;
  int last_recorded_ask_size_;
  int last_recorded_bid_size_;
  bool first_time_;
  double alpha_;
  BookInfoManager& book_info_manager_;
  BookInfoManager::BookInfoStruct* book_info_struct_;

 protected:
  SizeBasedRegime(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                  SecurityMarketView& _dep_market_view_, double _fractional_seconds_, int t_volume_measure_seconds_,
                  PriceType_t _price_type_, double _stdev_duration_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static SizeBasedRegime* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                            const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static SizeBasedRegime* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                            SecurityMarketView& _dep_market_view_, double _fractional_seconds_,
                                            int t_volume_measure_seconds_, PriceType_t _price_type_,
                                            double _stdev_duration_);

  ~SizeBasedRegime() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_);

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_){};
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  /// Used in ModelCreator to see which shortcodes are core
  bool GetReadinessRequired(const std::string& r_dep_shortcode_, const std::vector<const char*>& tokens_) const {
    return true;
  }

  static std::string VarName() { return "SizeBasedRegime"; }

  void WhyNotReady();

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {}

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();

  /// Multiplier :
  /// From price changes, we compute the coefficient and corerlation through Single Linear Regression
  /// < lr_coeff_, lr_correlation_ > = GetSLRCoeffCorrelation ( dep_price_change_vector_, indep_price_change_vector_ )
  /// For SizeBasedRegime lr_coeff_/fabs(lr_correlation_) is sort of the lr_coeff_ as it would be if indep and dep are
  /// either perfectly correlated or
  /// perfectly anti-correlated ( correation = -1 )
};
}
