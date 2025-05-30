/**
    \file ExecLogic/sgx_nk_price_pair_aggressive_trading.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_EXECLOGIC_SGX_NK_PRICE_PAIR_AGGRESSIVE_TRADING_H
#define BASE_EXECLOGIC_SGX_NK_PRICE_PAIR_AGGRESSIVE_TRADING_H

#include "dvctrade/ExecLogic/base_trading.hpp"

namespace HFSAT {

/// Class to trade with a target price that is very dependant on top level nonself marketprice
///
/// the base price moves sharply with market price and so does the target price
/// the targetprice is thus compared to bid price and ask price ( for getting an opinion to place trades )
/// and not midprice or a price that is much more stable than market weighted price
class SGXNKPricePairAggressiveTrading : public virtual BaseTrading {
 public:
  static void CollectORSShortCodes(DebugLogger& _dbglogger_, const std::string& r_dep_shortcode_,
                                   std::vector<std::string>& source_shortcode_vec_,
                                   std::vector<std::string>& ors_source_needed_vec_);

 protected:
 public:
  SGXNKPricePairAggressiveTrading(DebugLogger& _dbglogger_, const Watch& _watch_,
                                  const SecurityMarketView& _dep_market_view_,
                                  const SecurityMarketView& _nkm_market_view_,
                                  SmartOrderManager& _order_manager_,
                                  const std::string& _paramfilename_, const bool _livetrading_,
                                  MulticastSenderSocket* _p_strategy_param_sender_socket_,
                                  EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                                  const int t_trading_end_utc_mfm_, const int t_runtime_id_,
                                  const std::vector<std::string> _this_model_source_shortcode_vec_);

  ~SGXNKPricePairAggressiveTrading(){};  ///< destructor not made virtual ... please do so when making child classes

  static std::string StrategyName() {
    return "SGXNKPricePairAggressiveTrading";
  }  ///< used in trade_init to check which strategy is to be initialized

 protected:
  const SecurityMarketView& nkm_market_view_;

  void TradingLogic();  ///< All the strategy based trade execution is written here

  void OnMarketUpdate(const unsigned int security_id, const MarketUpdateInfo& market_update_info) {
    if ( security_id == dep_market_view_.security_id() ) {
      BaseTrading::OnMarketUpdate(security_id, market_update_info);
    }
    if (( security_id_ == dep_market_view_.security_id() ) || ( security_id_ == nkm_market_view_.security_id()))
    {
      CallTradingLogic();
    }
  }

  void OnTradePrint(const unsigned int security_id, const TradePrintInfo& trade_print_info,
                    const MarketUpdateInfo& market_update_info) {
    if ( security_id == dep_market_view_.security_id() ) {
      BaseTrading::OnTradePrint(security_id, trade_print_info,
                                market_update_info);
    }
    if ((security_id == dep_market_view_.security_id() ) || ( security_id == nkm_market_view_.security_id())) {
      CallTradingLogic();
    }
  }

  void CallTradingLogic() {
    if ( is_ready_ ) {
      if ((!external_freeze_trading_) && (!freeze_due_to_exchange_stage_) && (!freeze_due_to_funds_reject_)) {
        if ((!should_be_getting_flat_) && (throttle_manager_->allowed_through_throttle(watch_.msecs_from_midnight()))) {
          TradingLogic();
        }
      }
    }
  }

  void PrintFullStatus();
};
}
#endif  // BASE_EXECLOGIC_SGX_NK_PRICE_PAIR_AGGRESSIVE_TRADING_H
