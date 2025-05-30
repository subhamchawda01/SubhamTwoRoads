/**
    \file ExecLogic/feu3_mm.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_FEU3_MM_H
#define BASE_FEU3_MM_H

#include "dvctrade/ExecLogic/base_trading.hpp"

namespace HFSAT {

// temporaray market making exec to fullfill quote obligations
class FEU3MM : public virtual BaseTrading {
 protected:
 public:
  static void CollectORSShortCodes(DebugLogger& _dbglogger_, const std::string& r_dep_shortcode_,
                                   std::vector<std::string>& source_shortcode_vec_,
                                   std::vector<std::string>& ors_source_needed_vec_);

  FEU3MM(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
         SmartOrderManager& _order_manager_, const SecurityMarketView& _indep_market_view_,
         const std::string& _paramfilename_, const bool _livetrading_,
         MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
         const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int _runtime_id_,
         const std::vector<std::string> _this_model_source_shortcode_vec_);

  ~FEU3MM() {}  ///< destructor not made virtual ... please do so when making child classes

  static std::string StrategyName() {
    return "FEU3MM";
  }  ///< used in trade_init to check which strategy is to be initialized

 protected:
  virtual void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  virtual void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                            const MarketUpdateInfo& _market_update_info_);
  void OnTimePeriodUpdate(const int num_pages_to_add_);
  inline void CallPlaceCancelNonBestLevels();

  void TradingLogic();  ///< All the strategy based trade execution is written here
  void PrintFullStatus();

  const SecurityMarketView& indep_market_view_;  // LFI
  double best_indep_bid_price_;
  int best_indep_bid_int_price_;
  int best_indep_bid_size_;

  double best_indep_ask_price_;
  int best_indep_ask_int_price_;
  int best_indep_ask_size_;

  bool status_;
};
}
#endif  // BASE_EXECLOGIC_PRICE_BASED_TRADING_H
