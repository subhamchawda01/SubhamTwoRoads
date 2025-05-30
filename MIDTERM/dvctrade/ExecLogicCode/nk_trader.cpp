#include "dvctrade/ExecLogic/nk_trader.hpp"

namespace HFSAT {

NKTrader::NKTrader(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
                   SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
                   MulticastSenderSocket* _p_strategy_param_sender_socket_,
                   EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                   const int t_trading_end_utc_mfm_, const int t_runtime_id_,
                   const std::vector<std::string> _this_model_source_shortcode_vec_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_) {
  nk_smv_ = (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView("NK_0"));
  nkm_smv_ = (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView("NKM_0"));

  nk_bid_price_ = 0.0;
  nk_ask_price_ = 0.0;
  nk_bid_size_ = 0.0;
  nk_ask_size_ = 0.0;
  nk_bid_orders_ = 0.0;
  nk_ask_orders_ = 0.0;

  nkm_bid_price_ = 0.0;
  nkm_ask_price_ = 0.0;
  nkm_bid_size_ = 0.0;
  nkm_ask_size_ = 0.0;
  nkm_bid_orders_ = 0.0;
  nkm_ask_orders_ = 0.0;

  bid_price_ = 0.0;
  ask_price_ = 0.0;
  bid_size_ = 0.0;
  ask_size_ = 0.0;
  mid_price_ = 0.0;
  price_tilt_ = 0.0;
  size_tilt_ = 0.0;

  fair_price_ = 0.0;
  pmethod_ = PriceType::kPriceMethodMktLinear;

  nk_smv_->subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  nkm_smv_->subscribe_price_type(this, kPriceTypeMktSizeWPrice);
}

void NKTrader::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& cr_market_update_info_) {
  // kMethod = mkt, ord, mktsin, ordsin, offline, offlinesin
  NonSelfMarketUpdate(_security_id_, cr_market_update_info_);

  if (pmethod_ == PriceType::kPriceMethodMktLinear) {
    fair_price_ = mid_price_ + price_tilt_ * size_tilt_;
  } else if (pmethod_ == PriceType::kPriceMethodMktSin) {
    fair_price_ = mid_price_ + price_tilt_ * size_tilt_ * size_tilt_ * size_tilt_;
  } else if (pmethod_ == PriceType::kPriceMethodTrdBookLinear) {
    fair_price_ = mid_price_ + price_tilt_ * size_tilt_ * size_tilt_ * size_tilt_;
  }
}

void NKTrader::NonSelfMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& cr_market_update_info_) {
  if (_security_id_ == nk_smv_->security_id()) {
    nk_bid_price_ = cr_market_update_info_.bestbid_price_;
    nk_ask_price_ = cr_market_update_info_.bestask_price_;
    nk_bid_size_ = cr_market_update_info_.bestbid_size_;
    nk_ask_size_ = cr_market_update_info_.bestask_size_;
    nk_bid_orders_ = cr_market_update_info_.bestbid_ordercount_;
    nk_ask_orders_ = cr_market_update_info_.bestask_ordercount_;
  } else if (_security_id_ == nkm_smv_->security_id()) {
    nkm_bid_price_ = cr_market_update_info_.bestbid_price_;
    nkm_ask_price_ = cr_market_update_info_.bestask_price_;
    nkm_bid_size_ = cr_market_update_info_.bestbid_size_;
    nkm_ask_size_ = cr_market_update_info_.bestask_size_;
    nkm_bid_orders_ = cr_market_update_info_.bestbid_ordercount_;
    nkm_ask_orders_ = cr_market_update_info_.bestask_ordercount_;
  }
  bid_price_ =
      (nk_bid_price_ * nk_bid_size_ + nkm_bid_price_ * 0.1 * nkm_bid_size_) / (nk_bid_size_ + nkm_bid_size_ * 0.1);
  ask_price_ =
      (nk_ask_price_ * nk_ask_size_ + nkm_ask_price_ * 0.1 * nkm_ask_size_) / (nk_ask_size_ + nkm_ask_size_ * 0.1);
  bid_size_ = nk_bid_size_ + nkm_bid_size_ * 0.1;
  ask_size_ = nk_ask_size_ + nkm_ask_size_ * 0.1;
  mid_price_ = (bid_price_ + ask_price_) / 2;
  price_tilt_ = (bid_price_ - ask_price_) / 2;
  size_tilt_ = (bid_size_ - ask_size_) / (bid_size_ + ask_size_);

  DBGLOG_TIME_CLASS_FUNC << bid_price_ << " " << ask_price_ << " " << bid_size_ << " " << ask_size_ << " "
                         << price_tilt_ << " " << size_tilt_ << DBGLOG_ENDL_FLUSH;
  // BaseTrading::NonSelfMarketUpdate();
}

void NKTrader::TradingLogic() {
  top_bid_place_ = false;
  top_bid_keep_ = false;
  top_ask_place_ = false;
  top_ask_keep_ = false;

  DBGLOG_TIME_CLASS_FUNC << "NKM: [ " << nkm_bid_size_ << " @ " << nkm_bid_price_ << " X " << nkm_ask_price_ << " @ "
                         << nkm_ask_size_ << " ]"
                         << "NK: [ " << nk_bid_size_ << " @ " << nk_bid_price_ << " X " << nk_ask_price_ << " @ "
                         << nk_ask_size_ << " ]"
                         << " " << fair_price_ << DBGLOG_ENDL_FLUSH;
}

void NKTrader::PrintFullStatus() {}
}
