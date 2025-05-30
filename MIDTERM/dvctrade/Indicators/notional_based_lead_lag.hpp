/**
    \file Indicators/notional_based_lead_lag.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/TradeUtils/time_decayed_trade_info_manager.hpp"

namespace HFSAT {

/**
 * This indicator takes into account the lead-lag. Idea is that If we have more confidence in lead-lag, then we want to
 * give offline indicator value multiflied by some high quantity.
 * On the contrary, if we have less confidence then we want to multiply offline indicator by some low quantity.
 *
 * Formulation :
 * indicator_value = lead_lag_factor * offline_indicator_value
 *
 * This indicator only computes lead_lag_factor whcih wil be further used in offline indicators
 *
 * lead_lag_factor is computed in following way:
 * lead_lag_factor =  notional amount of indep traded/ notional amount of dep traded
 *
 * notional amount traded = exponentially decayed summation (trade px * trade size * n2d)
 *
 * Note: lead_lag_factor >=0 ; It is not necessarily to be less than 1. When we learn weight to indicator, It will get
 * adjusted there.
 *
 */
class NotionalBasedLeadLag : public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& indep_market_view_;

  TimeDecayedTradeInfoManager& dep_time_decayed_trade_info_manager_;
  TimeDecayedTradeInfoManager& indep_time_decayed_trade_info_manager_;

  double dep_n2d_;
  double indep_n2d_;

  double dep_prev_sumpxsz_;
  double indep_prev_sumpxsz_;

  // functions
 protected:
  NotionalBasedLeadLag(DebugLogger& _dbglogger_, const Watch& _watch_,
                       const std::string& concise_indicator_description_, SecurityMarketView& _dep_market_view_,
                       SecurityMarketView& _indep_market_view_, double _trade_duration_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static NotionalBasedLeadLag* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static NotionalBasedLeadLag* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 SecurityMarketView& _dep_market_view_,
                                                 SecurityMarketView& _indep_market_view_, double _trade_duration_);

  ~NotionalBasedLeadLag() {}

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_);
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};
  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {}

  static std::string VarName() { return "NotionalBasedLeadLag"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
