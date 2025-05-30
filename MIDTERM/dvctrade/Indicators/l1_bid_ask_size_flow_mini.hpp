/**
    \file Indicators/l1_bid_ask_size_flow.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_L1_BID_ASK_SIZE_FLOW__MINI_H
#define BASE_INDICATORS_L1_BID_ASK_SIZE_FLOW_MINI_H

#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"

namespace HFSAT {

/// Event Decayed L1 Bid Ask Bias Trend
/// Bid Ask Bias = ( BidSize - AskSize ) / ( BidSize + AskSize )
/// Essentially we notice that before a trade, the size on the respective depletes quickly
/// and this can be used cancellation
//
/// this indicator is specific to the purpose of shortcodes
/// which have min / major pair traded at the exchange
/// with exactly same min_price increment like WDO/DOL, WIN/IND
///
/// In this indicator we'll use the above formulation of leading security
/// for the lagging security with a condition that book levels are exactly same
/// between mini and major security

class L1BidAskSizeFlowMini : public CommonIndicator {
 protected:
  // variables
  SecurityMarketView& dep_market_view_;
  SecurityMarketView& indep_market_view_;
  unsigned int dep_sec_id_;
  unsigned int indep_sec_id_;

  double decay_factor_;  // decaying on l1 updates
  double decay_value_;   // maintains the decay^event_count to avoid std::pow callback

  double last_dep_bid_int_price_;
  double last_dep_ask_int_price_;

  double last_indep_bid_int_price_;
  double last_indep_ask_int_price_;

  double current_bid_ask_bias_;
  double moving_avg_bid_ask_bias_;
  int event_count_;

  // functions
 protected:
  L1BidAskSizeFlowMini(DebugLogger& _dbglogger_, const Watch& _watch_,
                       const std::string& concise_indicator_description_, SecurityMarketView& _dep_market_view_,
                       SecurityMarketView& _indep_market_view_, unsigned int _num_levels_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static L1BidAskSizeFlowMini* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static L1BidAskSizeFlowMini* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 SecurityMarketView& _dep_market_view_,
                                                 SecurityMarketView& _indep_market_view_, unsigned int _num_events_);

 public:
  ~L1BidAskSizeFlowMini() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  void OnPortfolioPriceChange(double _new_price_) {}
  void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_) {}
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

  // functions
  static std::string VarName() { return "L1BidAskSizeFlowMini"; }
};
}

#endif  // BASE_INDICATORS_L1_BID_ASK_SIZE_FLOW_MINI_H
