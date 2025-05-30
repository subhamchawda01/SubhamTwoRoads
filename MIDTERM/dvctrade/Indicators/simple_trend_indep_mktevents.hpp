/**
    \file Indicators/simple_trend_indep_mktevents.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_SIMPLE_TREND_INDEP_MKTEVENTS_H
#define BASE_INDICATORS_SIMPLE_TREND_INDEP_MKTEVENTS_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/recent_simple_events_measure.hpp"

namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially event-decaying moving average
/// probably a better fit for ES type products. Need to see correlations
class SimpleTrendIndepMktEvents : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& indep_market_view_;

  RecentSimpleEventsMeasure* p_dep_indicator_;
  RecentSimpleEventsMeasure* p_indep_indicator_;

  unsigned int trend_history_num_events_halflife_;
  PriceType_t price_type_;

  // computational variables
  double offline_events_ratio_;
  double scaled_event_ratio_;

  int kRatioFactor_;
  std::vector<double> decay_vector_;
  double decay_page_factor_;
  double inv_decay_sum_;

  double moving_avg_price_;
  double current_dep_price_;

  bool indc_ready_;
  bool indep_interrupted_;
  bool dep_interrupted_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static SimpleTrendIndepMktEvents* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                      const std::vector<const char*>& _tokens_,
                                                      PriceType_t _basepx_pxtype_);

  static SimpleTrendIndepMktEvents* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                      const SecurityMarketView& _dep_market_view_,
                                                      const SecurityMarketView& _indep_market_view_,
                                                      unsigned int _num_events_halflife_, PriceType_t _price_type_,
                                                      bool _use_time_ = false);

 protected:
  SimpleTrendIndepMktEvents(DebugLogger& _dbglogger_, const Watch& _watch_,
                            const std::string& concise_indicator_description_,
                            const SecurityMarketView& _dep_market_view_, const SecurityMarketView& _indep_market_view_,
                            unsigned int _num_events_halflife_, PriceType_t _price_type_);

  void SetTimeDecayWeights();
  void InitializeValues();

 public:
  ~SimpleTrendIndepMktEvents() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  // functions
  static std::string VarName() { return "SimpleTrendIndepMktEvents"; }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
};
}

#endif  // BASE_INDICATORS_SIMPLE_TREND_INDEP_MKTEVENTS_H
