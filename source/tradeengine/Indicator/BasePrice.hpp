#ifndef _INDICATOR_BASE_PRICE_H
#define _INDICATOR_BASE_PRICE_H

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "tradeengine/CommonInitializer/defines.hpp"

class BasePrice : public HFSAT::SecurityMarketViewChangeListener {
 protected:
  HFSAT::SecurityMarketView* smv_;
  HFSAT::Watch& watch_;
  HFSAT::DebugLogger& dbglogger_;
  double base_bid_price_;
  double base_ask_price_;
  double weight_;
  bool is_ready_;

 public:
  BasePrice(HFSAT::SecurityMarketView* smv, HFSAT::Watch& _watch_, HFSAT::DebugLogger& _dbglogger_, double weight_);
  virtual ~BasePrice();

  virtual void GetBasePrice(double& bid_price, double& ask_price) {
    bid_price = base_bid_price_;
    ask_price = base_ask_price_;
  }

  void InitializeDataSubscription() { smv_->subscribe_L2(this); }

  double GetWeight() { return weight_; }

  virtual void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {}

  virtual void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                            const HFSAT::MarketUpdateInfo& _market_update_info_) {}
};

#endif  //_INDICATOR_BASE_PRICE_H
