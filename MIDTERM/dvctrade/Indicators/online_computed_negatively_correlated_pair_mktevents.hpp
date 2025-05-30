/**
    \file Indicators/online_computed_negatively_correlated_pair_mktevents.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_ONLINE_COMPUTED_NEGATIVELY_CORRELATED_PAIR_MKTEVENTS_H
#define BASE_INDICATORS_ONLINE_COMPUTED_NEGATIVELY_CORRELATED_PAIR_MKTEVENTS_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"

namespace HFSAT {

class OnlineComputedNegativelyCorrelatedPairMktEvents : public CommonIndicator {
 protected:
  // variables
  SecurityMarketView& dep_market_view_;
  SecurityMarketView& indep_market_view_;

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

  static OnlineComputedNegativelyCorrelatedPairMktEvents* GetUniqueInstance(DebugLogger& _dbglogger_,
                                                                            const Watch& _watch_,
                                                                            const std::vector<const char*>& _tokens_,
                                                                            PriceType_t _basepx_pxtype_);

  static OnlineComputedNegativelyCorrelatedPairMktEvents* GetUniqueInstance(
      DebugLogger& _dbglogger_, const Watch& _watch_, SecurityMarketView& _dep_market_view_,
      SecurityMarketView& _indep_market_view_, unsigned int _num_events_halflife_, PriceType_t _price_type_);

 protected:
  OnlineComputedNegativelyCorrelatedPairMktEvents(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                  const std::string& concise_indicator_description_,
                                                  SecurityMarketView& _dep_market_view_,
                                                  SecurityMarketView& _indep_market_view_,
                                                  unsigned int _num_events_halflife_, PriceType_t _price_type_);

 public:
  ~OnlineComputedNegativelyCorrelatedPairMktEvents() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

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

  static std::string VarName() { return "OnlineComputedNegativelyCorrelatedPairMktEvents"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_ONLINE_COMPUTED_NEGATIVELY_CORRELATED_PAIR_MKTEVENTS_H
