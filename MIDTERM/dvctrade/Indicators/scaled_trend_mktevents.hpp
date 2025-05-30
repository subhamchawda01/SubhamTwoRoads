/**
    \file Indicators/scaled_trend_mktevents.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_INDICATORS_SCALED_TREND_MKTEVENTS_H
#define BASE_INDICATORS_SCALED_TREND_MKTEVENTS_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

namespace HFSAT {

/// Similar to offline_computed_pairs_port, Only difference is that the decaying is done on mktevents not time
class ScaledTrendMktEvents : public CommonIndicator {
 protected:
  // variables
  SecurityMarketView& indep_market_view_;

  unsigned int trend_history_num_events_halflife_;
  PriceType_t price_type_;

  // computational variables
  double moving_avg_price_;
  double moving_avg_squared_price_;

  double inv_decay_sum_;
  double decay_page_factor_;
  double stdev_value_;
  double current_indep_price_;
  double min_unbiased_l2_norm_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static ScaledTrendMktEvents* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static ScaledTrendMktEvents* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 SecurityMarketView& _indep_market_view_,
                                                 unsigned int _num_events_halflife_, PriceType_t _price_type_,
                                                 bool _use_time_ = false);

 protected:
  ScaledTrendMktEvents(DebugLogger& _dbglogger_, const Watch& _watch_,
                       const std::string& concise_indicator_description_, SecurityMarketView& _indep_market_view_,
                       unsigned int _num_events_halflife_, PriceType_t _price_type_);

 public:
  ~ScaledTrendMktEvents() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "ScaledTrendMktEvents"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
  void SetTimeDecayWeights();
};
}

#endif  // BASE_INDICATORS_SCALED_TREND_MKTEVENTS_H
