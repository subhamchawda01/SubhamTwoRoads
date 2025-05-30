/**
    \file Indicators/online_computed_negatively_correlated_pairs_mktevents_port.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_ONLINE_COMPUTED_NEGATIVELY_CORRELATED_PAIRS_MKTEVENTS_PORT_H
#define BASE_INDICATORS_ONLINE_COMPUTED_NEGATIVELY_CORRELATED_PAIRS_MKTEVENTS_PORT_H

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/pcaport_price.hpp"

namespace HFSAT {

/// Indicator that is a mktevents decayed version of OnlineComputedNegativelyCorrelatedPairsPort
class OnlineComputedNegativelyCorrelatedPairsMktEventsPort : public CommonIndicator {
 protected:
  // variables
  SecurityMarketView& dep_market_view_;
  PCAPortPrice* indep_portfolio_price__;
  unsigned int trend_history_num_events_halflife_;
  PriceType_t price_type_;

  double twice_initial_indep_price_;

  // computational variables
  double moving_avg_dep_price_;
  double moving_avg_indep_price_;
  double moving_avg_dep_indep_price_;
  double moving_avg_indep_indep_price_;

  double decay_page_factor_;
  double inv_decay_sum_;

  double current_dep_price_;
  double current_indep_price_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static OnlineComputedNegativelyCorrelatedPairsMktEventsPort* GetUniqueInstance(
      DebugLogger& _dbglogger_, const Watch& _watch_, const std::vector<const char*>& _tokens_,
      PriceType_t _basepx_pxtype_);

  static OnlineComputedNegativelyCorrelatedPairsMktEventsPort* GetUniqueInstance(
      DebugLogger& _dbglogger_, const Watch& _watch_, SecurityMarketView& _dep_market_view_,
      std::string _portfolio_descriptor_shortcode_, unsigned int _num_events_halflife_, PriceType_t _price_type_);

 protected:
  OnlineComputedNegativelyCorrelatedPairsMktEventsPort(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                       const std::string& concise_indicator_description_,
                                                       SecurityMarketView& _dep_market_view_,
                                                       std::string _portfolio_descriptor_shortcode_,
                                                       unsigned int _num_events_halflife_, PriceType_t _price_type_);

 public:
  ~OnlineComputedNegativelyCorrelatedPairsMktEventsPort() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  void OnPortfolioPriceChange(double _new_price_);
  void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_);

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    market_update_manager_.AddMarketDataInterruptedListener(indep_portfolio_price__);
  }
  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);
  // functions
  // functions
  static std::string VarName() { return "OnlineComputedNegativelyCorrelatedPairsMktEventsPort"; }

 protected:
  void InitializeValues();
  void UpdateComputedVariables();
};
}

#endif  // BASE_INDICATORS_ONLINE_COMPUTED_NEGATIVELY_CORRELATED_PAIRS_MKTEVENTS_PORT_H
