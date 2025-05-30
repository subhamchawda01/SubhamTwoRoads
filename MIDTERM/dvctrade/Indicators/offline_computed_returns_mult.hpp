/**
    \file Indicators/offline_computed_returns_mult.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/simple_returns.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"
#include "dvctrade/Indicators/pcaport_price.hpp"
#include "dvctrade/Indicators/offline_returns_pairs_db.hpp"

namespace HFSAT {

class OfflineComputedReturnsMult : public IndicatorListener, public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  std::string portfolio_discriptor_shortcode_;
  std::vector<SecurityMarketView*> indep_market_view_vec_;

  double dep_price_trend_;
  double indep_price_trend_;
  PriceType_t price_type_;
  OfflineReturnsPairsDB& ret_pairs_db_;
  int last_lrinfo_updated_msecs_;
  ReturnsInfo current_returns_info_;
  double current_projection_multiplier_;
  double current_projected_trend_;
  SimpleTrend* p_dep_indicator_;
  std::vector<SimpleReturns*> p_indep_indicator_vec_;
  std::vector<double> weights_indep_;
  std::vector<std::string> source_shortcode_vec_;
  std::vector<double> source_weight_vec_;
  std::vector<double> prev_value_vec_;
  std::vector<bool> readiness_required_vec_;

  // functions
 protected:
  OfflineComputedReturnsMult(DebugLogger& _dbglogger_, const Watch& _watch_,
                             const std::string& concise_indicator_description_, SecurityMarketView& _dep_market_view_,
                             double _fractional_seconds_, PriceType_t _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static OfflineComputedReturnsMult* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                       const std::vector<const char*>& _tokens_,
                                                       PriceType_t _basepx_pxtype_);

  static OfflineComputedReturnsMult* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                       SecurityMarketView& _dep_market_view_,
                                                       double _fractional_seconds_, PriceType_t _price_type_);

  ~OfflineComputedReturnsMult() {}

  // listener interface

  void OnTimePeriodUpdate(const int num_pages_to_add_) {}
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_){};
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};
  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    // need to add for all independents
  }

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

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

  /// Used in ModelCreator to see which variable is in the model file
  static std::string VarName() { return "OfflineComputedReturnsMult"; }

  void WhyNotReady();

  /// market_interrupt_listener interface
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
  void Initialize();

 protected:
  void RecomputeWeights(const int update_idx_, bool t_remvove_);
  // void UpdateLRInfo ( ) ;
  void LoadSourceWeights(std::string t_portfolio_discriptor_shortcode_);
  /// function to compute current_projection_multiplier_.   This is the main difference between OfflineCorradjustedPairs
  /// and OfflineComputedReturnsMult
};
}
