/**
    \file Indicators/offline_computed_returns_pairs_mktevents_port.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_OFFLINE_COMPUTED_RETURNS_PAIRS_MKTEVENTS_PORT_H
#define BASE_INDICATORS_OFFLINE_COMPUTED_RETURNS_PAIRS_MKTEVENTS_PORT_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/pcaport_price.hpp"
#include "dvctrade/Indicators/offline_returns_rlrdb.hpp"
#include "dvctrade/Indicators/simple_returns_mktevents.hpp"
#include "dvctrade/Indicators/simple_returns_mktevents_port.hpp"

namespace HFSAT {

/// Indicator : dep is a SecurityMarketView, indep is PCAPortPrice
/// rest is smilar to OfflineComputedPairsMktEvents
class OfflineComputedReturnsPairsMktEventsPort : public IndicatorListener,
                                                 public CommonIndicator,
                                                 public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;

  PCAPortPrice& indep_portfolio_price_;
  PriceType_t price_type_;

  double dep_price_trend_;
  double indep_price_trend_;
  SimpleReturnsMktEvents* p_dep_indicator_;
  SimpleReturnsMktEventsPort* p_indep_indicator_;

  OfflineReturnsRetLRDB& rlrdb_;
  int last_lrinfo_updated_msecs_;
  LRInfo current_lrinfo_;
  double current_projection_multiplier_;
  double current_projected_trend_;

  // functions
 protected:
  OfflineComputedReturnsPairsMktEventsPort(DebugLogger& _dbglogger_, const Watch& _watch_,
                                           const std::string& concise_indicator_description_,
                                           SecurityMarketView& _dep_market_view_,
                                           std::string _portfolio_descriptor_shortcode_,
                                           unsigned int _num_events_halflife_, PriceType_t _price_type_,
                                           bool _use_time_ = false);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static OfflineComputedReturnsPairsMktEventsPort* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                                     const std::vector<const char*>& _tokens_,
                                                                     PriceType_t _basepx_pxtype_);

  static OfflineComputedReturnsPairsMktEventsPort* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                                     SecurityMarketView& _dep_market_view_,
                                                                     std::string _portfolio_descriptor_shortcode_,
                                                                     unsigned int _num_events_halflife_,
                                                                     PriceType_t _price_type_, bool _use_time_ = false);

  ~OfflineComputedReturnsPairsMktEventsPort() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_) { UpdateLRInfo(); }

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};
  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    market_update_manager_.AddMarketDataInterruptedListener(&indep_portfolio_price_);
    if (p_indep_indicator_ != NULL) {
      market_update_manager_.AddMarketDataInterruptedListener(p_indep_indicator_);
    }
  }

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);
  // functions

  // functions
  static std::string VarName() { return "OfflineComputedReturnsPairsMktEventsPort"; }

 protected:
  void InitializeValues();
  void UpdateLRInfo();

  inline void ComputeMultiplier() { current_projection_multiplier_ = current_lrinfo_.lr_coeff_; }
};
}

#endif  // BASE_INDICATORS_OFFLINE_COMPUTED_RETURNS_PAIRS_MKTEVENTS_PORT_H
