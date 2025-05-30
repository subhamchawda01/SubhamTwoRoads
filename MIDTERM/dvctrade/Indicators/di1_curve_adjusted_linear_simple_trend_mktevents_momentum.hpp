/**
    \file Indicators/di1_curve_adjusted_linear_simple_trend_mktevents.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_DI1_CURVE_ADJUSTED_LINEAR_SIMPLE_TREND_MKTEVENTS_MOMENTUM_H
#define BASE_INDICATORS_DI1_CURVE_ADJUSTED_LINEAR_SIMPLE_TREND_MKTEVENTS_MOMENTUM_H

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/BaseUtils/curve_utils.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvctrade/Indicators/simple_trend_mktevents.hpp"

namespace HFSAT {

class DI1CurveAdjustedLinearSimpleTrendMktEventsMomentum : public IndicatorListener, public CommonIndicator {
 protected:
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& indep1_market_view_;
  const SecurityMarketView& indep2_market_view_;

  double dep_term_;
  double indep1_term_;
  double indep2_term_;

  SimpleTrendMktEvents* dep_trendI_;
  SimpleTrendMktEvents* indep1_trendI_;
  SimpleTrendMktEvents* indep2_trendI_;

  // readiness && interrupts handlers
  std::vector<bool> is_ready_vec_;
  std::vector<double> prev_value_vec_;
  bool indep1_interrupted_;
  bool indep2_interrupted_;
  bool dep_interrupted_;

  DI1CurveAdjustedLinearSimpleTrendMktEventsMomentum(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                     const std::string& _concise_indicator_description_,
                                                     const SecurityMarketView& _dep_market_view_,
                                                     const SecurityMarketView& _indep1_market_view_,
                                                     const SecurityMarketView& _indep2_market_view_,
                                                     double _trend_history_secs_, PriceType_t _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static DI1CurveAdjustedLinearSimpleTrendMktEventsMomentum* GetUniqueInstance(DebugLogger& _dbglogger_,
                                                                               const Watch& _watch_,
                                                                               const std::vector<const char*>& _tokens_,
                                                                               PriceType_t _base_price_type_);

  static DI1CurveAdjustedLinearSimpleTrendMktEventsMomentum* GetUniqueInstance(
      DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
      const SecurityMarketView& _indep1_market_view_, const SecurityMarketView& _indep2_market_view_,
      double _trend_history_secs_, PriceType_t t_price_type_);

  ~DI1CurveAdjustedLinearSimpleTrendMktEventsMomentum() {}

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_){};

  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // subscribing at this indicators level ( OnMarketDataInterrupted )
  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_){};

  void OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_);
  inline void OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);
  // functions

  // functions
  static std::string VarName() { return "DI1CurveAdjustedLinearSimpleTrendMktEventsMomentum"; }

 protected:
  bool AreAllReady();
  void InitializeValues();
};
}

#endif
