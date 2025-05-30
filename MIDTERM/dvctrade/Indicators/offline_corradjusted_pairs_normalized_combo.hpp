/**
    \file Indicators/offline_corradjusted_pairs_normalized_combo.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_OFFLINE_CORRADJUSTED_PAIRS_NORMALIZED_COMBO_H
#define BASE_INDICATORS_OFFLINE_CORRADJUSTED_PAIRS_NORMALIZED_COMBO_H

#include <map>
#include <vector>
#include <string>

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"

namespace HFSAT {

/// Creates SimpleTrend of dependant and sources in port
/// fetches lr_coeff_ and lr_correlation_ from OfflineReturnsLRDB
/// Becomes a listener to the indicators only.
/// Math: IV = current price of dependant + Sum ( weight_i * Trend_i ) - Trend_D
/// where weight_i = numer_i / Sum ( numer_i )
/// where numer_i = lr_coeff_/fabs(lr_correlation_) of Dependant and Source_i
class OfflineCorradjustedPairsNormalizedCombo : public IndicatorListener,
                                                public CommonIndicator,
                                                public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;  ///< needed for current price
  OfflineReturnsLRDB& lrdb_;
  int last_lrinfo_updated_msecs_;
  SimpleTrend* p_dep_price_change_indicator_;
  double prev_dep_price_change_;
  std::vector<std::string> shortcode_vec_;
  std::vector<double> normalized_weight_vec_;
  std::vector<bool> is_ready_vec_;                                 // 0 to size-1 of portfolio
  std::vector<double> prev_value_vec_;                             ///< to make computation faster
  std::vector<SimpleTrend*> p_source_price_change_indicator_vec_;  ///< store of the created source SimpleTrend since we
                                                                   /// can't add this as indicator listener till we have
                                                                   /// all the weights

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static OfflineCorradjustedPairsNormalizedCombo* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                                    const std::vector<const char*>& _tokens_,
                                                                    PriceType_t _basepx_pxtype_);

  static OfflineCorradjustedPairsNormalizedCombo* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                                    const SecurityMarketView& _dep_market_view_,
                                                                    std::string _portfolio_descriptor_shortcode_,
                                                                    double _fractional_seconds_,
                                                                    PriceType_t _price_type_);

  OfflineCorradjustedPairsNormalizedCombo(DebugLogger& _dbglogger_, const Watch& _watch_,
                                          const std::string& concise_indicator_description_,
                                          const SecurityMarketView& _dep_market_view_,
                                          std::string _portfolio_descriptor_shortcode_,
                                          const std::vector<std::string>& t_shortcode_vec_, double _fractional_seconds_,
                                          PriceType_t _price_type_);

  ~OfflineCorradjustedPairsNormalizedCombo() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_) { UpdateLRInfo(); }

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  }  // ideally this is important but we will probably get a SimpleTrend OnIndicatorUpdate for the dependant
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    for (auto i = 0u; i < p_source_price_change_indicator_vec_.size(); i++) {
      if (p_source_price_change_indicator_vec_[i] != NULL) {
        market_update_manager_.AddMarketDataInterruptedListener(p_source_price_change_indicator_vec_[i]);
      }
    }

    if (p_dep_price_change_indicator_ != NULL) {
      market_update_manager_.AddMarketDataInterruptedListener(p_dep_price_change_indicator_);
    }
  }

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  // functions
  static std::string VarName() { return "OfflineCorradjustedPairsNormalizedCombo"; }

  void WhyNotReady();

  /// market_interrupt_listener interface
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

  void set_basepx_pxtype(SecurityMarketView& _dep_market_view_, PriceType_t _basepx_pxtype_) {  // TODO
  }

 protected:
  bool AreAllReady();
  void UpdateLRInfo();
  void UpdateWeights();
};
}

#endif  // BASE_INDICATORS_OFFLINE_CORRADJUSTED_PAIRS_COMBO_H
