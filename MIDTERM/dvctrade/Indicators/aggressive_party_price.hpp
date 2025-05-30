/**
    \file Indicators/aggressive_party_price.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_AGGRESSIVE_PARTY_PRICE_H
#define BASE_INDICATORS_AGGRESSIVE_PARTY_PRICE_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

namespace HFSAT {

/// Assuming that for every trade that has happened the passive party will be forced to trade with
/// porbability 0.50, which translates to a full effect of same size as the aggressive trade.
/// When we see an aggressive sell of size 100 at price 10000, we deduct a further 100 from the bid size at 10000
/// if available.
/// We slowly remove the effect of this with time.
/// The projected price here is the weighted-price-of-market after removing effect of these trades
/// The indicator returns the projected mktwpx - current mktwpx
class AggressivePartyPrice : public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;

  double projected_mktpx_;  ///< weighted-price-of-market after removing effect of recent trades
  int projected_bidsz_;
  int projected_asksz_;
  int projected_bid_int_price_;
  int projected_ask_int_price_;
  double projected_bid_price_;
  double projected_ask_price_;
  int projected_bidsz_depletion_;
  int projected_asksz_depletion_;
  int last_decayed_msecs_;
  int trade_history_halflife_msecs_;
  int prev_best_bid_size_;
  int prev_best_ask_size_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static AggressivePartyPrice* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static AggressivePartyPrice* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 SecurityMarketView& _indep_market_view_, double _trade_seconds_,
                                                 PriceType_t _basepx_pxtype_);

 protected:
  AggressivePartyPrice(DebugLogger& _dbglogger_, const Watch& _watch_,
                       const std::string& concise_indicator_description_, SecurityMarketView& _indep_market_view_,
                       double _trade_seconds_);

 public:
  ~AggressivePartyPrice() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_);
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);
  // functions
  // functions
  static std::string VarName() { return "AggressivePartyPrice"; }

  void WhyNotReady();

 protected:
  void ReduceTradeEffect();
  void TestReduceTradeEffect();
};
}

#endif  // BASE_INDICATORS_AGGRESSIVE_PARTY_PRICE_H
