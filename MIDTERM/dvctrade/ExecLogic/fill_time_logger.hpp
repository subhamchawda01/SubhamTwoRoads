/**
    \file ExecLogic/fill_time_logger.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_EXECLOGIC_FILL_TIME_LOGGER_H
#define BASE_EXECLOGIC_FILL_TIME_LOGGER_H

#include "dvctrade/ExecLogic/base_trading.hpp"

namespace HFSAT {

/// Class to trade with a target price that is very dependant on top level nonself marketprice
///
/// the base price moves sharply with market price and so does the target price
/// the targetprice is thus compared to bid price and ask price ( for getting an opinion to place trades )
/// and not midprice or a price that is much more stable than market weighted price
class FillTimeLogger : virtual public BaseTrading {
 protected:
  bool to_cancel_non_best;

 public:
  static void CollectORSShortCodes(DebugLogger& _dbglogger_, const std::string& r_dep_shortcode_,
                                   std::vector<std::string>& source_shortcode_vec_,
                                   std::vector<std::string>& ors_source_needed_vec_);

  void OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_, const double _price_,
              const int r_int_price_, const int _security_id_);

  FillTimeLogger(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
                 SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
                 MulticastSenderSocket* _p_strategy_param_sender_socket_,
                 EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                 const int t_trading_end_utc_mfm_, const int t_runtime_id_,
                 const std::vector<std::string> _this_model_source_shortcode_vec_);

  ~FillTimeLogger(){};  ///< destructor not made virtual ... please do so when making child classes

  static std::string StrategyName() {
    return "FillTimeLogger";
  }  ///< used in trade_init to check which strategy is to be initialized

 protected:
  void TradingLogic();  ///< All the strategy based trade execution is written here
  bool toTrade();
  bool throttle_control();
  void PrintFullStatus() {}
  void CallPlaceCancelNonBestLevels() {}
  void PlaceCancelNonBestLevels() {}

  void OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                      const int _security_id_) {}

  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
};
}
#endif  // BASE_EXECLOGIC_FILL_TIME_LOGGER_H
