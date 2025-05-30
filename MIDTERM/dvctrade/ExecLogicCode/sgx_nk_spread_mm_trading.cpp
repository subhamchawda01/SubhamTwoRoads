/**
    \file ExecLogicCode/sgx_mm_trading.cpp
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/ExecLogic/sgx_nk_spread_mm_trading.hpp"
// exec_logic_code / defines.hpp was empty
#include "dvctrade/ExecLogic/vol_utils.hpp"

#define SMALL_PX_DIFF 1e-05
#define SMALL_COOLOFF_MSECS_ 10

namespace HFSAT {

void SgxNKSpreadMMTrading::CollectORSShortCodes(DebugLogger& _dbglogger_, const std::string& r_dep_shortcode_,
                                                std::vector<std::string>& source_shortcode_vec_,
                                                std::vector<std::string>& ors_source_needed_vec_) {
  HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, r_dep_shortcode_);
  HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, r_dep_shortcode_);

  if (r_dep_shortcode_.compare("SP_SGX_NK0_NK1") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("NKM_1"));
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("NKM_0"));
  }
}

void SgxNKSpreadMMTrading::OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
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

SgxNKSpreadMMTrading::SgxNKSpreadMMTrading(
    DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
    SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
    MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
    const std::vector<std::string> _this_model_source_shortcode_vec_, const SecurityMarketView& _nkm0_market_view_,
    const SecurityMarketView& _nkm1_market_view_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_),
      to_quote_(false),
      total_time_to_quote_(0),
      total_time_quoted_(0),
      last_msecs_stopped_quoting_(t_trading_start_utc_mfm_),
      last_msecs_started_quoting_(t_trading_start_utc_mfm_),
      rate_(0.0),
      position_get_flat_(0),
      nkm0_market_view_(_nkm0_market_view_),
      nkm1_market_view_(_nkm1_market_view_) {
  nkm0_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  nkm1_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
}

bool SgxNKSpreadMMTrading::toTrade() {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " SP_SGX_NK0_NK1 ready: " << dep_market_view_.is_ready_
                           << " NKM_0 ready: " << nkm0_market_view_.is_ready_
                           << " NKM_1 ready: " << nkm1_market_view_.is_ready_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << "Trading Start: " << trading_start_utc_mfm_
                           << " Trading end: " << trading_end_utc_mfm_
                           << " Current msecs: " << watch_.msecs_from_midnight() << DBGLOG_ENDL_FLUSH;
  }
  if (!nkm0_market_view_.is_ready_ || !nkm1_market_view_.is_ready_ || !dep_market_view_.is_ready_ ||
      external_getflat_ || watch_.msecs_from_midnight() < trading_start_utc_mfm_ ||
      watch_.msecs_from_midnight() > trading_end_utc_mfm_) {
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " Not Trading" << DBGLOG_ENDL_FLUSH;
    return false;
  }
  return true;
}
void SgxNKSpreadMMTrading::TradingLogic() {
  if (!toTrade()) {
    order_manager_.CancelAllOrders();
    return;
  }

  if (getflat_due_to_economic_times_) {
    SetQuoteToFalse();
    if (throttle_control()) order_manager_.CancelAllOrders();
    return;
  }
  int bid_int_price_to_quote_ = best_nonself_bid_int_price_ - 1;
  int ask_int_price_to_quote_ = best_nonself_ask_int_price_ + 1;
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "**********************************************************************************\n";
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << "["
                           << "SP_SGX_NK0_NK1 " << dep_market_view_.market_update_info_.bestbid_size_ << " "
                           << dep_market_view_.market_update_info_.bestbid_int_price_ << "("
                           << best_nonself_bid_int_price_ << ")"
                           << dep_market_view_.market_update_info_.bestask_int_price_ << "("
                           << best_nonself_ask_int_price_ << ") " << dep_market_view_.market_update_info_.bestask_size_
                           << "]" << DBGLOG_ENDL_FLUSH;
  }
  int best_bid_int_price = dep_market_view_.market_update_info_.bestbid_int_price_;
  int best_ask_int_price = dep_market_view_.market_update_info_.bestask_int_price_;
  double best_bid_price = dep_market_view_.market_update_info_.bestbid_price_;
  double best_ask_price = dep_market_view_.market_update_info_.bestask_price_;
  int spread_nk0_nk1 = best_ask_int_price - best_bid_int_price;

  if (spread_nk0_nk1 == 1) {
    double mkt_price = dep_market_view_.market_update_info_.mkt_size_weighted_price_;
    double trade_int_price = dep_market_view_.trade_print_info_.int_trade_price_;
    double nkm0_mkt_price = nkm0_market_view_.market_update_info_.mkt_size_weighted_price_;
    double nkm1_mkt_price = nkm1_market_view_.market_update_info_.mkt_size_weighted_price_;
    double spread = nkm1_mkt_price - nkm0_mkt_price;

    if (trade_int_price < best_bid_int_price || trade_int_price > best_ask_int_price) {
      trade_int_price = mkt_price;
    }
    //     trade_int_price = std::min(best_ask_int_price, std::max(best_bid_int_price, trade_int_price));
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
      DBGLOG_TIME_CLASS_FUNC << "Market Price " << mkt_price << " Trade Int Price " << trade_int_price << " Spread "
                             << spread << DBGLOG_ENDL_FLUSH;
    if (mkt_price - best_bid_price < best_ask_price - mkt_price) {
      bid_int_price_to_quote_ = best_bid_int_price - 1;
      ask_int_price_to_quote_ = best_ask_int_price;
    } else {
      bid_int_price_to_quote_ = best_bid_int_price;
      ask_int_price_to_quote_ = best_ask_int_price + 1;
    }
    /*if (trade_int_price - best_bid_int_price < best_ask_int_price - trade_int_price) {
      bid_int_price_to_quote_ = best_bid_int_price - 1;
      ask_int_price_to_quote_ = best_ask_int_price;
    } else {
      bid_int_price_to_quote_ = best_bid_int_price;
      ask_int_price_to_quote_ = best_ask_int_price + 1;
    }*/

    if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
      DBGLOG_TIME_CLASS_FUNC << "Spread of SP_SGX_NK0_NK1 = 1. Placing orders. " << bid_int_price_to_quote_ << " "
                             << ask_int_price_to_quote_ << DBGLOG_ENDL_FLUSH;
    SetQuoteToTrue();
  } else if (spread_nk0_nk1 == 2) {
    bid_int_price_to_quote_ = best_bid_int_price;
    ask_int_price_to_quote_ = best_ask_int_price;
    SetQuoteToTrue();
  } else {
    bid_int_price_to_quote_ = best_bid_int_price - 5;
    ask_int_price_to_quote_ = best_ask_int_price + 5;
    SetQuoteToFalse();
  }

  if (to_quote_) {
    if (ask_int_price_to_quote_ - bid_int_price_to_quote_ <= 2) {
      if (throttle_control()) order_manager_.CancelAsksAboveIntPrice(ask_int_price_to_quote_);
      if (throttle_control()) order_manager_.CancelAsksBelowIntPrice(ask_int_price_to_quote_ + 2);

      if (throttle_control()) order_manager_.CancelBidsAboveIntPrice(bid_int_price_to_quote_);
      if (throttle_control()) order_manager_.CancelBidsBelowIntPrice(bid_int_price_to_quote_ - 2);

      PlaceAskOrdersAtPrice(ask_int_price_to_quote_, param_set_.unit_trade_size_);
      PlaceAskOrdersAtPrice(ask_int_price_to_quote_ + 1, param_set_.unit_trade_size_);
      PlaceAskOrdersAtPrice(ask_int_price_to_quote_ + 2, param_set_.unit_trade_size_);
      PlaceBidOrdersAtPrice(bid_int_price_to_quote_, param_set_.unit_trade_size_);
      PlaceBidOrdersAtPrice(bid_int_price_to_quote_ - 1, param_set_.unit_trade_size_);
      PlaceBidOrdersAtPrice(bid_int_price_to_quote_ - 2, param_set_.unit_trade_size_);
    } else {
      if (throttle_control()) order_manager_.CancelAllBidOrders();
      if (throttle_control()) order_manager_.CancelAllAskOrders();
    }
  } else {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) DBGLOG_TIME_CLASS_FUNC << "CANCELLING\n";
    if (throttle_control()) order_manager_.CancelAllBidOrders();
    if (throttle_control()) order_manager_.CancelAllAskOrders();
  }
}

void SgxNKSpreadMMTrading::OnTimePeriodUpdate(const int num_pages_to_add_) {
  ProcessTimePeriodUpdate(num_pages_to_add_);
}

void SgxNKSpreadMMTrading::OnMarketUpdate(const unsigned int _security_id_,
                                          const MarketUpdateInfo& _market_update_info_) {
  BaseTrading::OnMarketUpdate(_security_id_, _market_update_info_);
  TradingLogic();
}

bool SgxNKSpreadMMTrading::throttle_control() {
  return (throttle_manager_->allowed_through_throttle(watch_.msecs_from_midnight()));
}
void SgxNKSpreadMMTrading::SetQuoteToFalse() {
  if (to_quote_) {
    if (watch_.msecs_from_midnight() > last_msecs_started_quoting_ &&
        watch_.msecs_from_midnight() < trading_end_utc_mfm_) {
      total_time_quoted_ += (watch_.msecs_from_midnight() - last_msecs_started_quoting_);
      last_msecs_stopped_quoting_ = watch_.msecs_from_midnight();
      to_quote_ = false;
    } else if (watch_.msecs_from_midnight() > last_msecs_started_quoting_ &&
               watch_.msecs_from_midnight() >= trading_end_utc_mfm_) {
      total_time_quoted_ += (trading_end_utc_mfm_ - last_msecs_started_quoting_);
      last_msecs_stopped_quoting_ = trading_end_utc_mfm_;
      to_quote_ = false;
    }
  }
}

void SgxNKSpreadMMTrading::SetQuoteToTrue() {
  if (!to_quote_) {
    if (last_msecs_started_quoting_ < watch_.msecs_from_midnight() &&
        watch_.msecs_from_midnight() < trading_end_utc_mfm_ && watch_.msecs_from_midnight() > trading_start_utc_mfm_) {
      last_msecs_started_quoting_ = watch_.msecs_from_midnight();
      to_quote_ = true;
    }
  }
}

void SgxNKSpreadMMTrading::PlaceAskOrdersAtPrice(const int ask_int_price_to_quote_, const int size_to_place_) {
  int sum_ask_size_at_price_to_quote_ = order_manager_.GetTotalAskSizeOrderedAtIntPx(ask_int_price_to_quote_);
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
    DBGLOG_TIME_CLASS_FUNC << sum_ask_size_at_price_to_quote_ << " ask orders already at " << ask_int_price_to_quote_
                           << DBGLOG_ENDL_FLUSH;
  if (sum_ask_size_at_price_to_quote_ < size_to_place_) {
    unsigned int ask_size_to_place_ = size_to_place_ - sum_ask_size_at_price_to_quote_;
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
      DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " Placing Ask Orders at " << ask_int_price_to_quote_
                             << " Size: " << ask_size_to_place_ << DBGLOG_ENDL_FLUSH;
    if (throttle_control())
      order_manager_.SendTradeIntPx(ask_int_price_to_quote_, ask_size_to_place_, kTradeTypeSell, 'S');
  }
}

void SgxNKSpreadMMTrading::PlaceBidOrdersAtPrice(const int bid_int_price_to_quote_, const int size_to_place_) {
  int sum_bid_size_at_price_to_quote_ = order_manager_.GetTotalBidSizeOrderedAtIntPx(bid_int_price_to_quote_);
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
    DBGLOG_TIME_CLASS_FUNC << sum_bid_size_at_price_to_quote_ << " bid orders already at " << bid_int_price_to_quote_
                           << DBGLOG_ENDL_FLUSH;
  if (sum_bid_size_at_price_to_quote_ < size_to_place_) {
    unsigned int bid_size_to_place_ = size_to_place_ - sum_bid_size_at_price_to_quote_;
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
      DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " Placing Bid Orders at " << bid_int_price_to_quote_
                             << " Size: " << bid_size_to_place_ << DBGLOG_ENDL_FLUSH;
    if (throttle_control())
      order_manager_.SendTradeIntPx(bid_int_price_to_quote_, bid_size_to_place_, kTradeTypeBuy, 'S');
  }
}

void SgxNKSpreadMMTrading::ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_) {
  if (to_quote_) {
    printf("%u %u %u\n", total_time_quoted_, last_msecs_started_quoting_, watch_.msecs_from_midnight());
    total_time_quoted_ += (trading_end_utc_mfm_ - last_msecs_started_quoting_);
  }
  if (livetrading_) {
    printf("SIMRESULT %d %d %d %d %d %d\n", (int)order_manager_.base_pnl().ReportConservativeTotalPNL(true),
           order_manager_.trade_volume(), order_manager_.SupportingOrderFilledPercent(),
           order_manager_.BestLevelOrderFilledPercent(), order_manager_.AggressiveOrderFilledPercent(),
           order_manager_.ImproveOrderFilledPercent());
  } else {
    int t_pnl_ = (int)(order_manager_.base_pnl().ReportConservativeTotalPNL(true));
    int total_time_to_quote_ = trading_end_utc_mfm_ - trading_start_utc_mfm_;
    printf("Total time = %d\n", total_time_to_quote_);
    printf("Quoted time = %u\n", total_time_quoted_);
    double fraction_quoted_ = total_time_quoted_ * 100.0 / (1.0 * total_time_to_quote_);
    printf("SIMRESULT %d %d %d %d %d %d %1.6f\n", t_pnl_, order_manager_.trade_volume(),
           order_manager_.SupportingOrderFilledPercent(), order_manager_.BestLevelOrderFilledPercent(),
           order_manager_.AggressiveOrderFilledPercent(), order_manager_.ImproveOrderFilledPercent(), fraction_quoted_);

    trades_writer_ << "SIMRESULT " << runtime_id_ << " " << t_pnl_ << " " << order_manager_.trade_volume() << " "
                   << order_manager_.SupportingOrderFilledPercent() << " "
                   << order_manager_.BestLevelOrderFilledPercent() << " "
                   << order_manager_.AggressiveOrderFilledPercent() << " " << order_manager_.ImproveOrderFilledPercent()
                   << " " << improve_cancel_counter_ << " "
                   << "\n";

    trades_writer_ << "EOD_MIN_PNL: " << runtime_id_ << " " << order_manager_.base_pnl().min_pnl_till_now() << " "
                   << num_opentrade_loss_hits_ << "\n";
    trades_writer_ << "EOD_MSG_COUNT: " << runtime_id_ << " "
                   << (order_manager_.SendOrderCount() + order_manager_.CxlOrderCount() +
                       order_manager_.ModifyOrderCount()) << "\n";
    trades_writer_ << "NUM_OPENTRADE_HITS: " << runtime_id_ << " " << num_opentrade_loss_hits_ << "\n";
    trades_writer_ << "UNIT_TRADE_SIZE: " << runtime_id_ << " " << param_set_.unit_trade_size_ << "\n";
    trades_writer_ << "PNLSAMPLES " << runtime_id_ << " ";
    if (pnl_samples_.size() > 0) {
      for (auto i = 0u; i < pnl_samples_.size(); i++) {
        trades_writer_ << pnl_sampling_timestamps_[i] << " " << pnl_samples_[i] << " ";
      }
      trades_writer_ << "\n";
    } else {
      trades_writer_ << trading_end_utc_mfm_ << " " << t_pnl_ << "\n";
    }

    trades_writer_.CheckToFlushBuffer();

    dbglogger_ << "SIMRESULT " << runtime_id_ << " " << t_pnl_ << " " << order_manager_.trade_volume() << " "
               << order_manager_.SupportingOrderFilledPercent() << " " << order_manager_.BestLevelOrderFilledPercent()
               << " " << order_manager_.AggressiveOrderFilledPercent() << " "
               << order_manager_.ImproveOrderFilledPercent() << " " << improve_cancel_counter_ << " "
               << "\n";

    dbglogger_ << "EOD_MIN_PNL: " << runtime_id_ << " " << order_manager_.base_pnl().min_pnl_till_now() << " "
               << num_opentrade_loss_hits_ << "\n";
    dbglogger_ << "EOD_MSG_COUNT: " << runtime_id_ << " "
               << (order_manager_.SendOrderCount() + order_manager_.CxlOrderCount() + order_manager_.ModifyOrderCount())
               << "\n";
    dbglogger_ << "NUM_OPENTRADE_HITS: " << runtime_id_ << " " << num_opentrade_loss_hits_ << "\n";
    dbglogger_ << "UNIT_TRADE_SIZE: " << runtime_id_ << " " << param_set_.unit_trade_size_ << "\n";
    dbglogger_ << "PNLSAMPLES " << runtime_id_ << " ";
    if (pnl_samples_.size() > 0) {
      for (auto i = 0u; i < pnl_samples_.size(); i++) {
        dbglogger_ << pnl_sampling_timestamps_[i] << " " << pnl_samples_[i] << " ";
      }
      dbglogger_ << "\n";
    } else {
      dbglogger_ << trading_end_utc_mfm_ << " " << t_pnl_ << "\n";
    }

    dbglogger_.CheckToFlushBuffer();
  }
}
}
