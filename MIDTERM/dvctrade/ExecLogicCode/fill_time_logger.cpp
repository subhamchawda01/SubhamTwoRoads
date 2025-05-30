/**
   \file ExecLogicCode/price_based_aggressive_trading.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/ExecLogic/fill_time_logger.hpp"
// exec_logic_code / defines.hpp was empty

namespace HFSAT {

void FillTimeLogger::CollectORSShortCodes(DebugLogger& _dbglogger_, const std::string& r_dep_shortcode_,
                                          std::vector<std::string>& source_shortcode_vec_,
                                          std::vector<std::string>& ors_source_needed_vec_) {
  HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, r_dep_shortcode_);
  HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, r_dep_shortcode_);
}

void FillTimeLogger::OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                            const double _price_, const int r_int_price_, const int _security_id_) {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " OnExec; position : " << _new_position_ << " price " << _price_
                           << " _buysell_ " << _buysell_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << "["
                           << "SP_SGX_NK0_NK1 " << dep_market_view_.market_update_info_.bestbid_size_ << " "
                           << dep_market_view_.market_update_info_.bestbid_price_ << " "
                           << dep_market_view_.market_update_info_.bestask_price_ << " "
                           << dep_market_view_.market_update_info_.bestask_size_ << "]" << DBGLOG_ENDL_FLUSH;
  }
  if (_buysell_ == kTradeTypeBuy) {
    last_buy_msecs_ = watch_.msecs_from_midnight();
  } else if (_buysell_ == kTradeTypeSell) {
    last_sell_msecs_ = watch_.msecs_from_midnight();
  }
}

FillTimeLogger::FillTimeLogger(DebugLogger& _dbglogger_, const Watch& _watch_,
                               const SecurityMarketView& _dep_market_view_, SmartOrderManager& _order_manager_,
                               const std::string& _paramfilename_, const bool _livetrading_,
                               MulticastSenderSocket* _p_strategy_param_sender_socket_,
                               EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                               const int t_trading_end_utc_mfm_, const int t_runtime_id_,
                               const std::vector<std::string> _this_model_source_shortcode_vec_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_),
      to_cancel_non_best(false) {}

void FillTimeLogger::TradingLogic() {
  if (!toTrade()) {
    order_manager_.CancelAllOrders();
    return;
  }

  if (getflat_due_to_economic_times_) {
    if (throttle_control()) order_manager_.CancelAllOrders();
    return;
  }
  int bid_int_price_to_quote_ = best_nonself_bid_int_price_;
  int ask_int_price_to_quote_ = best_nonself_ask_int_price_;

  if (to_cancel_non_best) {
    order_manager_.CancelBidsBelowIntPrice(best_nonself_bid_int_price_);
    order_manager_.CancelAsksBelowIntPrice(best_nonself_ask_int_price_);
  }

  order_manager_.SendTradeIntPx(bid_int_price_to_quote_, 1, kTradeTypeBuy, 'B');
  order_manager_.SendTradeIntPx(ask_int_price_to_quote_, 1, kTradeTypeSell, 'B');
}

bool FillTimeLogger::toTrade() {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " Dep market view " << dep_market_view_.shortcode()
                           << " ready: " << dep_market_view_.is_ready_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << "Trading Start: " << trading_start_utc_mfm_
                           << " Trading end: " << trading_end_utc_mfm_
                           << " Current msecs: " << watch_.msecs_from_midnight() << DBGLOG_ENDL_FLUSH;
  }
  if (!dep_market_view_.is_ready_ || external_getflat_ || watch_.msecs_from_midnight() < trading_start_utc_mfm_ ||
      watch_.msecs_from_midnight() > trading_end_utc_mfm_) {
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " Not Trading" << DBGLOG_ENDL_FLUSH;
    return false;
  }
  return true;
}

bool FillTimeLogger::throttle_control() {
  return (throttle_manager_->allowed_through_throttle(watch_.msecs_from_midnight()));
}
}
