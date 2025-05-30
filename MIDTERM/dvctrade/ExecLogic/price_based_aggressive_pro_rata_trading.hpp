/**
    \file ExecLogic/price_based_aggressive_trading.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_EXECLOGIC_PRICE_BASED_AGGRESSIVE_PRO_RATA_TRADING_H
#define BASE_EXECLOGIC_PRICE_BASED_AGGRESSIVE_PRO_RATA_TRADING_H

#include "dvctrade/ExecLogic/base_trading.hpp"

namespace HFSAT {

/// Class to trade with a target price that is very dependant on top level nonself marketprice
///
/// the base price moves sharply with market price and so does the target price
/// the targetprice is thus compared to bid price and ask price ( for getting an opinion to place trades )
/// and not midprice or a price that is much more stable than market weighted price
class PriceBasedAggressiveProRataTrading : virtual public BaseTrading {
 protected:
 public:
  PriceBasedAggressiveProRataTrading(DebugLogger& _dbglogger_, const Watch& _watch_,
                                     const SecurityMarketView& _dep_market_view_, SmartOrderManager& _order_manager_,
                                     const std::string& _paramfilename_, const bool _livetrading_,
                                     MulticastSenderSocket* _p_strategy_param_sender_socket_,
                                     EconomicEventsManager& t_economic_events_manager_,
                                     const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_,
                                     const int t_runtime_id_,
                                     const std::vector<std::string> _this_model_source_shortcode_vec_, bool allow_exit);

  ~PriceBasedAggressiveProRataTrading(){};  ///< destructor not made virtual ... please do so when making child classes

  static std::string StrategyName() {
    return "PriceBasedAggressiveProRataTrading";
  }  ///< used in trade_init to check which strategy is to be initialized

  void CheckForExit();

 protected:
  void TradingLogic();  ///< All the strategy based trade execution is written here
  void PrintFullStatus();
  void TradeVarSetLogic(int t_position);

  inline void SetBidImproveAggressFlags();
  inline void SetAskImproveAggressFlags();
  inline void SetBidImproveKeepFlag();
  inline void SetAskImproveKeepFlag();
  inline void SetBidKeepFlag();
  inline void SetAskKeepFlag();
  inline bool CanPlaceBidsThisRound();
  inline bool CanPlaceAsksThisRound();
  inline bool CanPlaceBestBidThisRound();
  inline bool CanPlaceBestAskThisRound();
  inline bool CanBidAggressThisRound();
  inline bool CanAskAggressThisRound();
  inline bool CanBidImproveThisRound();
  inline bool CanAskImproveThisRound();

  inline void PlaceBestBid();
  inline void PlaceBestAsk();
  inline void ModifyOrder(double _old_price_, int _old_int_price_, int _old_size_, TradeType_t _old_buysell_,
                          char _old_order_level_indicator_, double _new_price_, int _new_int_price_, int _new_size_,
                          TradeType_t _new_buysell_, char _new_order_level_indicator_);
  inline void PlaceOrderAndLog(double _price_, int _int_price_, int _size_, TradeType_t _buysell_,
                               char _order_level_indicator_);
  inline void PlaceIntOrderAndLog(int _int_price_, int _size_, TradeType_t _buysell_, char _order_level_indicator_);
  inline void LogCancelOrder(double _price_, int canceled_size_, const std::string& _cancel_string_);
  int ComputeSizeToPlace(int total_size, TradeType_t buysell, int current_position);

  void PlaceCancelNonBestLevels();
  void PlaceNonBestLevelBids();
  void PlaceNonBestLevelAsks();

  void CancelNonBestLevelBids();
  void CancelNonBestLevelAsks();

 protected:
  int avg_l1_size_;
  double pro_rata_factor_;
  double pro_rata_adjusted_max_trade_size_;
  int current_bid_size_to_place_;
  int current_ask_size_to_place_;
};
}
#endif  // BASE_EXECLOGIC_PRICE_BASED_AGGRESSIVE_TRADING_H
