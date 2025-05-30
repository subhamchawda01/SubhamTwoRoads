/**
    \file Indicators/curve_adjusted_simple_trend.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_CURVE_ADJUSTED_PORTS_H
#define BASE_INDICATORS_CURVE_ADJUSTED_PORTS_H

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "baseinfra/BaseUtils/curve_utils.hpp"

namespace HFSAT {

class CurveAdjustedPorts : public IndicatorListener, public CommonIndicator {
 protected:
  const SecurityMarketView* dep_market_view_;
  SecurityMarketViewPtrVec indep_smv_vec_;

  SimpleTrend* dep_trend_;
  std::vector<SimpleTrend*> indep_trend_vec_;

  double dep_term_;
  std::vector<double> term_vec_;

  // readiness && interrupts handlers
  std::vector<bool> is_ready_vec_;
  std::vector<bool> indep_interrupted_vec_;

  std::vector<double> prev_value_vec_;

  bool dep_interrupted_;

  CurveAdjustedPorts(DebugLogger& r_dbglogger, const Watch& r_watch, const std::string& r_concise_indicator_description,
                     const SecurityMarketView* r_dep_market_view_, const std::vector<std::string> indep_shortcodes_vec,
                     double t_trend_history_secs, PriceType_t t_price_type);

 public:
  static void CollectShortCodes(std::vector<std::string>& r_shortcodes_affecting_this_indicator,
                                std::vector<std::string>& r_ors_source_needed_vec,
                                const std::vector<const char*>& t_tokens);

  static CurveAdjustedPorts* GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                               const std::vector<const char*>& r_tokens_,
                                               PriceType_t t_base_price_type);

  static CurveAdjustedPorts* GetUniqueInstance(DebugLogger& r_dbglogger, const Watch& r_watch,
                                               const SecurityMarketView* r_dep_market_view,
                                               const std::vector<std::string> indep_shortcodes_vec,
                                               double t_trend_history_secs_, PriceType_t t_price_type);

  ~CurveAdjustedPorts() {}

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
  static std::string VarName() { return "CurveAdjustedPorts"; }

 protected:
  bool AreAllReady();
  void InitializeValues();
};
}

#endif
