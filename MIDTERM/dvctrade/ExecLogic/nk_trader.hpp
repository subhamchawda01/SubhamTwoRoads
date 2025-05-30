/**
    \file ExecLogic/nk_trader.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include "dvctrade/ExecLogic/base_trading.hpp"

namespace HFSAT {

enum PriceType { kPriceMethodMktLinear, kPriceMethodMktSin, kPriceMethodTrdBookLinear };

class NKTrader : public virtual BaseTrading {
 protected:
 public:
  NKTrader(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
           SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
           MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
           const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int _runtime_id_,
           const std::vector<std::string> _this_model_source_shortcode_vec_);

  ~NKTrader() {}

  virtual void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  virtual void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                            const MarketUpdateInfo& _market_update_info_){};

  void NonSelfMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& cr_market_update_info_);

  static std::string StrategyName() { return "NKTrader"; }

 protected:
  void TradingLogic();
  void PrintFullStatus();

  SecurityMarketView* nk_smv_;
  SecurityMarketView* nkm_smv_;

  double nk_bid_price_;
  double nk_ask_price_;
  double nk_bid_size_;
  double nk_ask_size_;
  double nk_bid_orders_;
  double nk_ask_orders_;

  double nkm_bid_price_;
  double nkm_ask_price_;
  double nkm_bid_size_;
  double nkm_ask_size_;
  double nkm_bid_orders_;
  double nkm_ask_orders_;

  double bid_price_;
  double ask_price_;
  double bid_size_;
  double ask_size_;
  double mid_price_;
  double price_tilt_;
  double size_tilt_;

  PriceType pmethod_;

  double fair_price_;
};
}
