/**
    \file Indicators/simple_hawkes_price_process_tr_events.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#ifndef BASE_INDICATORS_SIMPLE_HAWKES_PRICE_PROCESS_TR_EVENTS_H
#define BASE_INDICATORS_SIMPLE_HAWKES_PRICE_PROCESS_TR_EVENTS_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

namespace HFSAT {

/// Class returning difference between the probability of the
// next trade at Bid/Ask based on hawkes process
class SimpleHawkesPriceProcessTREvents : public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;

  int num_moves_;
  double beta_;

  std::vector<int> bid_trade_vec_;
  std::vector<int> ask_trade_vec_;
  std::vector<double> exp_beta_;

  int bid_trade_index_;
  int bid_cycle_flag_;
  int ask_trade_index_;
  int ask_cycle_flag_;
  int last_bid_int_price_;
  int last_ask_int_price_;

  unsigned int num_trades_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static SimpleHawkesPriceProcessTREvents* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                             const std::vector<const char*>& _tokens_,
                                                             PriceType_t _basepx_pxtype_);

  static SimpleHawkesPriceProcessTREvents* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                             const SecurityMarketView& _indep_market_view_,
                                                             double _beta_, PriceType_t _price_type_);

 protected:
  SimpleHawkesPriceProcessTREvents(DebugLogger& _dbglogger_, const Watch& _watch_,
                                   const std::string& concise_indicator_description_,
                                   const SecurityMarketView& _indep_market_view_, double _beta_);

 public:
  ~SimpleHawkesPriceProcessTREvents() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
    if ((last_bid_int_price_ != _market_update_info_.bestbid_int_price_) ||
        (last_ask_int_price_ != _market_update_info_.bestask_int_price_)) {
      AdjustLevelVars();
      last_bid_int_price_ = _market_update_info_.bestbid_int_price_;
      last_ask_int_price_ = _market_update_info_.bestask_int_price_;
    }
  }

  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "SimpleHawkesPriceProcessTREvents"; }

  void InitializeValues();
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

  inline void AdjustLevelVars() {
    bid_cycle_flag_ = 0;
    bid_trade_index_ = -1;
    ask_cycle_flag_ = 0;
    ask_trade_index_ = -1;
  }
};
}

#endif  // BASE_INDICATORS_SIMPLE_HAWKES_PRICE_PROCESS_TR_EVENTS_H
