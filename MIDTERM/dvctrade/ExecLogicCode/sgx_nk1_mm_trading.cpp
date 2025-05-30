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
#include "dvctrade/ExecLogic/sgx_nk1_mm_trading.hpp"
// exec_logic_code / defines.hpp was empty
#include "dvctrade/ExecLogic/vol_utils.hpp"

#define SMALL_PX_DIFF 1e-05
#define SMALL_COOLOFF_MSECS_ 10

namespace HFSAT {

void SgxNK1MMTrading::CollectORSShortCodes(DebugLogger& dbglogger, const std::string& strategy_name,
                                           const std::string& dep_shortcode,
                                           std::vector<std::string>& source_shortcode_vec,
                                           std::vector<std::string>& ors_source_needed_vec,
                                           std::vector<std::string>& dependant_shortcode_vec) {
  if (strategy_name != StrategyName()) {
    return;
  }

  if (dep_shortcode.compare("SGX_NK_1") == 0) {
    VectorUtils::UniqueVectorAdd(source_shortcode_vec, std::string("SGX_NK_1"));
    VectorUtils::UniqueVectorAdd(ors_source_needed_vec, std::string("SGX_NK_1"));
    VectorUtils::UniqueVectorAdd(dependant_shortcode_vec, std::string("SGX_NK_1"));
    VectorUtils::UniqueVectorAdd(source_shortcode_vec, std::string("SGX_NK_0"));
    VectorUtils::UniqueVectorAdd(ors_source_needed_vec, std::string("SGX_NK_0"));
    VectorUtils::UniqueVectorAdd(dependant_shortcode_vec, std::string("SGX_NK_0"));
    VectorUtils::UniqueVectorAdd(source_shortcode_vec, std::string("SP_SGX_NK0_NK1"));
    VectorUtils::UniqueVectorAdd(ors_source_needed_vec, std::string("SP_SGX_NK0_NK1"));
    VectorUtils::UniqueVectorAdd(dependant_shortcode_vec, std::string("SP_SGX_NK0_NK1"));
    VectorUtils::UniqueVectorAdd(source_shortcode_vec, std::string("NKM_1"));
    VectorUtils::UniqueVectorAdd(ors_source_needed_vec, std::string("NKM_1"));
    VectorUtils::UniqueVectorAdd(dependant_shortcode_vec, std::string("NKM_1"));
    VectorUtils::UniqueVectorAdd(source_shortcode_vec, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(ors_source_needed_vec, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(dependant_shortcode_vec, std::string("NKM_0"));
  }
}

void SgxNK1MMTrading::GetDepShortcodes(const std::string& dep_shortcode, std::vector<std::string>& dep_shortcode_vec) {
  if (dep_shortcode.compare("SGX_NK_1") == 0) {
    VectorUtils::UniqueVectorAdd(dep_shortcode_vec, std::string("SGX_NK_1"));
    VectorUtils::UniqueVectorAdd(dep_shortcode_vec, std::string("SGX_NK_0"));
    VectorUtils::UniqueVectorAdd(dep_shortcode_vec, std::string("SP_SGX_NK0_NK1"));
  }
}

void SgxNK1MMTrading::OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                             const double _price_, const int r_int_price_, const int _security_id_) {
  if ((int32_t)sgx_nk0_market_view_.security_id() == _security_id_) {
    nk0_position_ = _new_position_;
  } else if ((int32_t)dep_market_view_.security_id() == _security_id_) {
    nk1_position_ = _new_position_;
    last_msecs_nk1_exec_ = watch_.msecs_from_midnight();
  } else if ((int32_t)spread_nk_market_view_.security_id() == _security_id_) {
    spread_nk_position_ = _new_position_;
  }
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " OnExec; position : " << _new_position_ << " price " << _price_
                           << " _buysell_ " << _buysell_ << " Security id: " << _security_id_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << "["
                           << "NKM_1 " << nkm_market_view_.market_update_info_.bestbid_size_ << " "
                           << nkm_market_view_.market_update_info_.bestbid_price_ << " "
                           << nkm_market_view_.market_update_info_.bestask_price_ << " "
                           << nkm_market_view_.market_update_info_.bestask_size_ << "]" << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << "["
                           << "SGX_NK_1 " << dep_market_view_.market_update_info_.bestbid_size_ << " "
                           << dep_market_view_.market_update_info_.bestbid_price_ << " "
                           << dep_market_view_.market_update_info_.bestask_price_ << " "
                           << dep_market_view_.market_update_info_.bestask_size_ << "]" << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << "["
                           << "SGX_NK_0 " << sgx_nk0_market_view_.market_update_info_.bestbid_size_ << " "
                           << sgx_nk0_market_view_.market_update_info_.bestbid_price_ << " "
                           << sgx_nk0_market_view_.market_update_info_.bestask_price_ << " "
                           << sgx_nk0_market_view_.market_update_info_.bestask_size_ << "]" << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << "["
                           << "SP_SGX_NK0_NK1 " << spread_nk_market_view_.market_update_info_.bestbid_size_ << " "
                           << spread_nk_market_view_.market_update_info_.bestbid_price_ << " "
                           << spread_nk_market_view_.market_update_info_.bestask_price_ << " "
                           << spread_nk_market_view_.market_update_info_.bestask_size_ << "]" << DBGLOG_ENDL_FLUSH;
  }
  if (_buysell_ == kTradeTypeBuy) {
    last_buy_msecs_ = watch_.msecs_from_midnight();
  } else if (_buysell_ == kTradeTypeSell) {
    last_sell_msecs_ = watch_.msecs_from_midnight();
  }
  nk0_1_position_ = nk0_position_ + nk1_position_;
  total_position_ = nk0_position_ + nk1_position_ + spread_nk_position_;
}

SgxNK1MMTrading::SgxNK1MMTrading(
    DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
    SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
    MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
    const std::vector<std::string> _this_model_source_shortcode_vec_, const SecurityMarketView& _sgx_nk0_market_view_,
    SmartOrderManager& _nk0_om_, const SecurityMarketView& _spread_nk_market_view_, SmartOrderManager& _spread_nk_om_,
    const SecurityMarketView& _nkm_market_view_, const SecurityMarketView& _nkm0_market_view_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_),
      nkm_market_view_(_nkm_market_view_),
      nkm0_market_view_(_nkm0_market_view_),
      sgx_nk0_market_view_(_sgx_nk0_market_view_),
      spread_nk_market_view_(_spread_nk_market_view_),
      to_quote_(false),
      total_time_to_quote_(0),
      total_time_quoted_(0),
      last_msecs_nk1_exec_(0),
      last_msecs_nkm_update_(0),
      last_msecs_stopped_quoting_(t_trading_start_utc_mfm_),
      last_msecs_started_quoting_(t_trading_start_utc_mfm_),
      rate_(0.0),
      nk0_position_(0),
      nk1_position_(0),
      spread_nk_position_(0),
      nk0_1_position_(0),
      total_position_(0),
      nk_min_price_increment_(
          SecurityDefinitions::GetContractMinPriceIncrement(sgx_nk0_market_view_.shortcode(), watch_.YYYYMMDD())),
      nk1_om_(_order_manager_),
      nk0_om_(_nk0_om_),
      spread_nk_om_(_spread_nk_om_),
      nkm_data_interrupted_(false) {
  nkm_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  sgx_nk0_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  spread_nk_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);

  nk0_om_.AddExecutionListener(this);
  nk0_om_.AddPositionChangeListener(this);
  nk0_om_.AddCancelRejectListener(this);
  nk0_om_.AddRejectDueToFundsListener(this);
  nk0_om_.AddFokFillRejectListener(this);

  spread_nk_om_.AddExecutionListener(this);
  spread_nk_om_.AddPositionChangeListener(this);
  spread_nk_om_.AddCancelRejectListener(this);
  spread_nk_om_.AddRejectDueToFundsListener(this);
  spread_nk_om_.AddFokFillRejectListener(this);

  nkm1_trades_flag_ = 0;
  nkm1_trades_duration_ = 1;
  nkm1_trades_threshold_ = 10;
  nkm1_avg_trade_size_ = MovingAvgTradeSize::GetUniqueInstance(
      _dbglogger_, _watch_, nkm_market_view_, nkm1_trades_duration_, StringToPriceType_t("MktSizeWPrice"));
  nkm1_avg_trade_size_->add_unweighted_indicator_listener(1u, this);

  nkm0_trades_flag_ = 0;
  nkm0_trades_duration_ = 1;
  nkm0_trades_threshold_ = 10;
  nkm0_avg_trade_size_ = MovingAvgTradeSize::GetUniqueInstance(
      _dbglogger_, _watch_, nkm0_market_view_, nkm0_trades_duration_, StringToPriceType_t("MktSizeWPrice"));
  nkm0_avg_trade_size_->add_unweighted_indicator_listener(2u, this);
  /*stdev_duration_ = 300u;
  stdev_flag_ = 0;
  stdev_threshold_ = 15.0;
  stdev_calculator_ =
      SlowStdevCalculator::GetUniqueInstance(_dbglogger_, _watch_, nkm_market_view_.shortcode(), stdev_duration_ *
  1000u);
  stdev_calculator_->AddSlowStdevCalculatorListener(this);*/
}

/*void SgxNK1MMTrading::OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_val_) {
  if (_new_stdev_val_ > stdev_threshold_) {
    order_manager_.CancelAllBidOrders();
    order_manager_.CancelAllAskOrders();
    GetFlatTradingLogic();
    SetQuoteToFalse();
    stdev_flag_ = 1;
  } else {
    stdev_flag_ = 0;
  }
}*/

void SgxNK1MMTrading::TradingLogic() {
  if (!toTrade()) {
    nk1_om_.CancelAllOrders();
    FlatLogic();
    return;
  }

  if (getflat_due_to_economic_times_) {
    SetQuoteToFalse();
    nk1_om_.CancelAllOrders();

    if (nk0_1_position_ != 0) {
      FlatLogic();
    } else {
      nk0_om_.CancelAllOrders();
    }
    if (nk0_1_position_ == 0 && nk1_position_ == -nk0_position_ && nk1_position_ != 0) GetFlatInSpread();
    return;
  }

  int position_bias_ = 0;

  // Trying to make sure that we don't accumulate a lot of spread position in one direction.
  // Hardcoding the threshold now
  // TODO change the threshold to a param
  if (nk1_position_ > 30) {
    position_bias_ = -1;
  } else if (nk1_position_ < -30) {
    position_bias_ = 1;
  }

  int bid_int_price_to_quote_ = best_nonself_bid_int_price_ - 1;
  int ask_int_price_to_quote_ = best_nonself_ask_int_price_ + 1;
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "**********************************************************************************"
                           << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << "["
                           << "NKM_1 " << nkm_market_view_.market_update_info_.bestbid_size_ << " "
                           << nkm_market_view_.market_update_info_.bestbid_int_price_ << " "
                           << nkm_market_view_.market_update_info_.bestask_int_price_ << " "
                           << nkm_market_view_.market_update_info_.bestask_size_ << "]" << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << "["
                           << "SGX_NK_1 " << dep_market_view_.market_update_info_.bestbid_size_ << " "
                           << dep_market_view_.market_update_info_.bestbid_int_price_ << "("
                           << best_nonself_bid_int_price_ << ")"
                           << dep_market_view_.market_update_info_.bestask_int_price_ << "("
                           << best_nonself_ask_int_price_ << ") " << dep_market_view_.market_update_info_.bestask_size_
                           << "]" << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << "["
                           << "SGX_NK_0 " << sgx_nk0_market_view_.market_update_info_.bestbid_size_ << " "
                           << sgx_nk0_market_view_.market_update_info_.bestbid_int_price_ << " "
                           << sgx_nk0_market_view_.market_update_info_.bestask_int_price_ << " "
                           << sgx_nk0_market_view_.market_update_info_.bestask_size_ << "]" << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << "["
                           << "SP_SGX_NK0_NK1 " << spread_nk_market_view_.market_update_info_.bestbid_size_ << " "
                           << spread_nk_market_view_.market_update_info_.bestbid_int_price_ << " "
                           << spread_nk_market_view_.market_update_info_.bestask_int_price_ << " "
                           << spread_nk_market_view_.market_update_info_.bestask_size_ << "]" << DBGLOG_ENDL_FLUSH;
  }
  int best_bid_int_price = dep_market_view_.market_update_info_.bestbid_int_price_;
  int best_ask_int_price = dep_market_view_.market_update_info_.bestask_int_price_;
  int spread_nk = best_ask_int_price - best_bid_int_price;
  int nkm_best_ask_int = nkm_market_view_.market_update_info_.bestask_int_price_;
  int nkm_best_bid_int = nkm_market_view_.market_update_info_.bestbid_int_price_;
  int spread_nkm = nkm_best_ask_int - nkm_best_bid_int;
  double nk1_synthetic_bid_price = sgx_nk0_market_view_.bestbid_price() + spread_nk_market_view_.bestbid_price();
  double nk1_synthetic_ask_price = sgx_nk0_market_view_.bestask_price() + spread_nk_market_view_.bestask_price();

  double nk1_synthetic_closest_bid_price =
      ((int)(nk1_synthetic_bid_price) / nk_min_price_increment_) * nk_min_price_increment_;
  double nk1_synthetic_closest_ask_price = 0;
  if ((int)nk1_synthetic_ask_price % nk_min_price_increment_ == 0) {
    nk1_synthetic_closest_ask_price = nk1_synthetic_ask_price;
  } else {
    nk1_synthetic_closest_ask_price =
        ((int)(nk1_synthetic_ask_price) / nk_min_price_increment_ + 1) * nk_min_price_increment_;
  }

  // double nk1_synthetic_mid_price = 1.0 * (nk1_synthetic_bid_price + nk1_synthetic_ask_price) / 2.0;

  if (nk0_1_position_ != 0) {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
      DBGLOG_TIME_CLASS_FUNC << "Positions: SGX_NK_1 = " << nk1_position_ << " SGX_NK_0 = " << nk0_position_
                             << " Total position = " << nk0_1_position_ << " Spread positin = " << spread_nk_position_
                             << DBGLOG_ENDL_FLUSH;
    if (abs(nk0_1_position_) > param_set_.max_position_) {
      SetQuoteToFalse();
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
        DBGLOG_TIME_CLASS_FUNC << "Cancelling Quotes because max position breached" << DBGLOG_ENDL_FLUSH;
      nk1_om_.CancelAllBidOrders();
      nk1_om_.CancelAllAskOrders();
      FlatLogic();
      return;
    } else if (nkm1_trades_flag_ == 1 || nkm0_trades_flag_ == 1) {
      //      SetQuoteToFalse();
      //      nk1_om_.CancelAllOrders();
      //      FlatLogic();
      //      return;
    } else if (watch_.msecs_from_midnight() - last_msecs_nk1_exec_ < param_set_.cooloff_interval_) {
      SetQuoteToFalse();
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
        DBGLOG_TIME_CLASS_FUNC << "Cancelling Quotes because cooling off" << DBGLOG_ENDL_FLUSH;
      nk1_om_.CancelAllOrders();
      FlatLogic();
      return;
    } else if (nkm_data_interrupted_) {
      SetQuoteToFalse();
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
        DBGLOG_TIME_CLASS_FUNC << "Cancelling Quotes because market data interrupt" << DBGLOG_ENDL_FLUSH;
      nk1_om_.CancelAllOrders();
      FlatLogic();
      return;
    } else if (watch_.msecs_from_midnight() - last_msecs_nkm_update_ > 100000) {
      nk1_om_.CancelAllOrders();
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
        DBGLOG_TIME_CLASS_FUNC << "Cancelling Quotes because no market updates in NKM_1" << DBGLOG_ENDL_FLUSH;
      FlatLogic();
      return;
    } else {
      FlatLogic();
      return;
    }
  } else {
    nk0_om_.CancelAllOrders();
    if (nk1_position_ == 0 && nk0_position_ == 0) {
      spread_nk_om_.CancelAllOrders();
    }
    if (nkm1_trades_flag_ == 1 || nkm0_trades_flag_ == 1) {
      //      SetQuoteToFalse();
      //      nk1_om_.CancelAllOrders();
      //      return;
    } else if (watch_.msecs_from_midnight() - last_msecs_nk1_exec_ < param_set_.cooloff_interval_) {
      SetQuoteToFalse();
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
        DBGLOG_TIME_CLASS_FUNC << "Cancelling Quotes because cooling off" << DBGLOG_ENDL_FLUSH;
      nk1_om_.CancelAllOrders();
      return;
    } else if (watch_.msecs_from_midnight() - last_msecs_nkm_update_ > 100000) {
      nk1_om_.CancelAllOrders();
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
        DBGLOG_TIME_CLASS_FUNC << "Cancelling Quotes because no market updates in NKM_1" << DBGLOG_ENDL_FLUSH;
      FlatLogic();
      return;
    }
  }

  if (nk0_1_position_ == 0 && nk0_position_ == -nk1_position_ && nk0_position_ != 0) {
    GetFlatInSpread();
  }

  if (spread_nk == 1) {
    bid_int_price_to_quote_ = best_bid_int_price - 1;
    ask_int_price_to_quote_ = best_ask_int_price + 1;
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
      DBGLOG_TIME_CLASS_FUNC << "Spread of SGX_NK_1 = 1. Placing orders. " << bid_int_price_to_quote_ << " "
                             << ask_int_price_to_quote_ << DBGLOG_ENDL_FLUSH;
    SetQuoteToTrue();
  } else if (spread_nkm == 1) {
    bid_int_price_to_quote_ = nkm_best_bid_int - 1;
    ask_int_price_to_quote_ = nkm_best_ask_int + 1;

    bid_int_price_to_quote_ += position_bias_;
    ask_int_price_to_quote_ += position_bias_;

    if (bid_int_price_to_quote_ >= best_ask_int_price || ask_int_price_to_quote_ <= best_bid_int_price) {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
        DBGLOG_TIME_CLASS_FUNC << "Spread of NKM_1 = 1. " << bid_int_price_to_quote_ << "(" << best_bid_int_price
                               << ") " << ask_int_price_to_quote_ << "(" << best_ask_int_price
                               << ") Could be AGGRESSIVE, hence quoting False" << DBGLOG_ENDL_FLUSH;

      nk1_om_.CancelAllOrders();
      SetQuoteToFalse();
      return;
    } else {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
        DBGLOG_TIME_CLASS_FUNC << "Spread of NKM_1 = 1. Placing orders. " << bid_int_price_to_quote_ << " "
                               << ask_int_price_to_quote_ << DBGLOG_ENDL_FLUSH;
      SetQuoteToTrue();
    }

  } else if (nk1_synthetic_closest_ask_price - nk1_synthetic_closest_bid_price <= 3 * nk_min_price_increment_) {
    if (nk1_synthetic_closest_ask_price - nk1_synthetic_closest_bid_price == 2 * nk_min_price_increment_) {
      int second_closest_bid_price = nk1_synthetic_closest_bid_price - nk_min_price_increment_;
      int second_closest_ask_price = nk1_synthetic_closest_ask_price + nk_min_price_increment_;
      int case_1_distance = std::min(nk1_synthetic_bid_price - second_closest_bid_price,
                                     nk1_synthetic_closest_ask_price - nk1_synthetic_ask_price);
      int case_2_distance = std::min(nk1_synthetic_bid_price - nk1_synthetic_closest_bid_price,
                                     second_closest_ask_price - nk1_synthetic_ask_price);
      if (case_1_distance > case_2_distance) {
        bid_int_price_to_quote_ = second_closest_bid_price / nk_min_price_increment_;
        ask_int_price_to_quote_ = nk1_synthetic_closest_ask_price / nk_min_price_increment_;
      } else {
        bid_int_price_to_quote_ = nk1_synthetic_closest_bid_price / nk_min_price_increment_;
        ask_int_price_to_quote_ = second_closest_ask_price / nk_min_price_increment_;
      }
    } else if (nk1_synthetic_closest_ask_price - nk1_synthetic_closest_bid_price == 3 * nk_min_price_increment_) {
      ask_int_price_to_quote_ = nk1_synthetic_closest_ask_price / nk_min_price_increment_;
      bid_int_price_to_quote_ = nk1_synthetic_closest_bid_price / nk_min_price_increment_;
    }

    bid_int_price_to_quote_ += position_bias_;
    ask_int_price_to_quote_ += position_bias_;

    if (bid_int_price_to_quote_ >= best_ask_int_price || ask_int_price_to_quote_ <= best_bid_int_price ||
        bid_int_price_to_quote_ > nkm_best_bid_int || ask_int_price_to_quote_ < nkm_best_ask_int) {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
        DBGLOG_TIME_CLASS_FUNC << "Spread of Synthetic NK1 <= 2. Placing orders " << bid_int_price_to_quote_ << "("
                               << best_bid_int_price << ") " << ask_int_price_to_quote_ << "(" << best_ask_int_price
                               << ") Could be AGGRESSIVE, hence quoting False" << DBGLOG_ENDL_FLUSH;
      nk1_om_.CancelAllOrders();
      SetQuoteToFalse();
      return;
    } else {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
        DBGLOG_TIME_CLASS_FUNC << "Spread of Synthetic NK1 <= 2. Placing orders " << bid_int_price_to_quote_ << " "
                               << ask_int_price_to_quote_ << DBGLOG_ENDL_FLUSH;
      SetQuoteToTrue();
    }
  } else if (spread_nkm == 2) {
    double nkm_mid_price = 1.0 * (nkm_best_ask_int + nkm_best_bid_int) / 2.0;
    if (nkm_mid_price >= best_bid_int_price && nkm_mid_price <= best_ask_int_price) {
      if (nkm_mid_price - best_bid_int_price > best_ask_int_price - nkm_mid_price) {
        ask_int_price_to_quote_ = nkm_best_ask_int + 1;
        bid_int_price_to_quote_ = nkm_best_bid_int;
      } else if (nkm_mid_price - best_bid_int_price < best_ask_int_price - nkm_mid_price) {
        bid_int_price_to_quote_ = nkm_best_bid_int - 1;
        ask_int_price_to_quote_ = nkm_best_ask_int;
      } else {
        bid_int_price_to_quote_ = nkm_best_bid_int - 1;
        ask_int_price_to_quote_ = nkm_best_ask_int;
      }

      bid_int_price_to_quote_ += position_bias_;
      ask_int_price_to_quote_ += position_bias_;

      if (bid_int_price_to_quote_ >= best_ask_int_price || ask_int_price_to_quote_ <= best_bid_int_price) {
        nk1_om_.CancelAllOrders();
        SetQuoteToFalse();
        return;
      } else {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
          DBGLOG_TIME_CLASS_FUNC << "Spread of NKM_1 = 2. Placing orders. " << bid_int_price_to_quote_ << " "
                                 << ask_int_price_to_quote_ << DBGLOG_ENDL_FLUSH;
        SetQuoteToTrue();
      }
    } else {
      nk1_om_.CancelAllOrders();
      SetQuoteToFalse();
      return;
    }
  } else if (spread_nkm == 3) {
    int nkm_best_ask = nkm_market_view_.market_update_info_.bestask_int_price_;
    int nkm_best_bid = nkm_market_view_.market_update_info_.bestbid_int_price_;

    bid_int_price_to_quote_ = nkm_best_bid;
    ask_int_price_to_quote_ = nkm_best_ask;

    bid_int_price_to_quote_ += position_bias_;
    ask_int_price_to_quote_ += position_bias_;

    if (bid_int_price_to_quote_ >= best_ask_int_price || ask_int_price_to_quote_ <= best_bid_int_price) {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
        DBGLOG_TIME_CLASS_FUNC << "Spread of NKM_1 = 3. " << bid_int_price_to_quote_ << "(" << best_bid_int_price
                               << ") " << ask_int_price_to_quote_ << "(" << best_ask_int_price
                               << ") Could be AGGRESSIVE, hence quoting False" << DBGLOG_ENDL_FLUSH;

      nk1_om_.CancelAllOrders();
      SetQuoteToFalse();
      return;
    } else {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
        DBGLOG_TIME_CLASS_FUNC << "Spread of NKM_1 = 3. Placing orders. " << bid_int_price_to_quote_ << " "
                               << ask_int_price_to_quote_ << DBGLOG_ENDL_FLUSH;

      SetQuoteToTrue();
    }

  } else {
    bid_int_price_to_quote_ = best_bid_int_price - 5;
    ask_int_price_to_quote_ = best_ask_int_price + 5;
    nk1_om_.CancelAllOrders();
    SetQuoteToFalse();
    return;
  }

  if (to_quote_) {
    if (ask_int_price_to_quote_ - bid_int_price_to_quote_ <= 3) {
      if (throttle_control()) nk1_om_.CancelAsksAboveIntPrice(ask_int_price_to_quote_);
      if (throttle_control()) nk1_om_.CancelAsksBelowIntPrice(ask_int_price_to_quote_ + 2);

      if (throttle_control()) nk1_om_.CancelBidsAboveIntPrice(bid_int_price_to_quote_);
      if (throttle_control()) nk1_om_.CancelBidsBelowIntPrice(bid_int_price_to_quote_ - 2);

      PlaceAskOrdersAtPrice(ask_int_price_to_quote_, param_set_.unit_trade_size_);
      PlaceAskOrdersAtPrice(ask_int_price_to_quote_ + 1, param_set_.unit_trade_size_);
      PlaceAskOrdersAtPrice(ask_int_price_to_quote_ + 2, param_set_.unit_trade_size_);
      PlaceBidOrdersAtPrice(bid_int_price_to_quote_, param_set_.unit_trade_size_);
      PlaceBidOrdersAtPrice(bid_int_price_to_quote_ - 1, param_set_.unit_trade_size_);
      PlaceBidOrdersAtPrice(bid_int_price_to_quote_ - 2, param_set_.unit_trade_size_);
    } else {
      // DBGLOG_TIME_CLASS_FUNC << "Should never be here\n";
      if (throttle_control()) nk1_om_.CancelAllBidOrders();
      if (throttle_control()) nk1_om_.CancelAllAskOrders();
    }
  } else {
    if (throttle_control()) nk1_om_.CancelAllBidOrders();
    if (throttle_control()) nk1_om_.CancelAllAskOrders();
  }
}

void SgxNK1MMTrading::OnTimePeriodUpdate(const int num_pages_to_add_) { ProcessTimePeriodUpdate(num_pages_to_add_); }

void SgxNK1MMTrading::GetFlatInSpread() {
  if (nk1_position_ + spread_nk_position_ > 0) {
    int sell_size = nk1_position_ + spread_nk_position_;
    int spread_best_ask_int_price = spread_nk_market_view_.market_update_info_.bestask_int_price_;
    int orders_at_best_level = spread_nk_om_.GetTotalAskSizeOrderedAtIntPx(spread_best_ask_int_price);
    sell_size -= orders_at_best_level;
    sell_size = std::min(sell_size, param_set_.unit_trade_size_);
    spread_nk_om_.CancelAllBidOrders();
    spread_nk_om_.CancelAsksBelowIntPrice(spread_best_ask_int_price);
    if (sell_size <= 0) return;
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
      DBGLOG_TIME_CLASS_FUNC << "Closing in spread Best level Sell: Size = " << sell_size
                             << " Price = " << spread_best_ask_int_price << DBGLOG_ENDL_FLUSH;
    spread_nk_om_.SendTradeIntPx(spread_best_ask_int_price, sell_size, kTradeTypeSell, 'B');

  } else {
    int buy_size = abs(nk1_position_ + spread_nk_position_);
    int spread_best_bid_int_price = spread_nk_market_view_.market_update_info_.bestbid_int_price_;
    int orders_at_best_level = spread_nk_om_.GetTotalBidSizeOrderedAtIntPx(spread_best_bid_int_price);
    buy_size -= orders_at_best_level;
    buy_size = std::min(buy_size, param_set_.unit_trade_size_);
    spread_nk_om_.CancelAllAskOrders();
    spread_nk_om_.CancelBidsBelowIntPrice(spread_best_bid_int_price);
    if (buy_size <= 0) return;
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
      DBGLOG_TIME_CLASS_FUNC << "Closing in spread Best level Buy: Size = " << buy_size
                             << " Price = " << spread_best_bid_int_price << DBGLOG_ENDL_FLUSH;
    spread_nk_om_.SendTradeIntPx(spread_best_bid_int_price, buy_size, kTradeTypeBuy, 'B');
  }
}

void SgxNK1MMTrading::FlatLogic() {
  if (nk0_1_position_ > 0) {
    nk0_om_.CancelAllBidOrders();
    int sell_size = abs(nk1_position_ + nk0_position_);
    int nk0_best_ask_int_price = sgx_nk0_market_view_.market_update_info_.bestask_int_price_;
    int orders_at_best_level = nk0_om_.GetTotalAskSizeOrderedAtIntPx(nk0_best_ask_int_price);
    nk0_om_.CancelAsksBelowIntPrice(nk0_best_ask_int_price);
    sell_size -= orders_at_best_level;
    sell_size = std::min(sell_size, param_set_.unit_trade_size_ - orders_at_best_level);
    nk0_om_.CancelAsksBelowIntPrice(nk0_best_ask_int_price);

    if (sell_size <= 0) return;
    nk0_om_.SendTradeIntPx(nk0_best_ask_int_price, sell_size, kTradeTypeSell, 'B');
  } else if (nk0_1_position_ < 0) {
    nk0_om_.CancelAllAskOrders();
    int buy_size = abs(nk1_position_ + nk0_position_);
    int nk0_best_bid_int_price = sgx_nk0_market_view_.market_update_info_.bestbid_int_price_;
    int orders_at_best_level = nk0_om_.GetTotalBidSizeOrderedAtIntPx(nk0_best_bid_int_price);
    nk0_om_.CancelBidsBelowIntPrice(nk0_best_bid_int_price);
    buy_size -= orders_at_best_level;
    buy_size = std::min(buy_size, param_set_.unit_trade_size_ - orders_at_best_level);
    nk0_om_.CancelBidsBelowIntPrice(nk0_best_bid_int_price);
    if (buy_size <= 0) return;
    nk0_om_.SendTradeIntPx(nk0_best_bid_int_price, buy_size, kTradeTypeBuy, 'B');
  }
}
/*
void SgxNK1MMTrading::FlatLogic() {
  if (nk0_1_position_ > 0) {
    int sell_size = abs(nk1_position_ + nk0_position_);
    nk0_om_.CancelAllAskOrders();
    // DBGLOG_TIME_CLASS_FUNC << "Positions: Cancelling Ask orders" << DBGLOG_ENDL_FLUSH;
    int nk0_ask_orders = nk0_om_.SumAskSizes();
    if (sell_size - nk0_ask_orders > 0 && nk0_ask_orders == 0) {
      sell_size = std::min(param_set_.unit_trade_size_, sell_size);
      int nk0_best_bid_price = sgx_nk0_market_view_.market_update_info_.bestbid_int_price_;
      // DBGLOG_TIME_CLASS_FUNC << "Positions: Sell size = " << sell_size << " at price =" << nk0_best_bid_price <<
DBGLOG_ENDL_FLUSH;
      nk0_om_.SendTradeIntPx(nk0_best_bid_price, sell_size, kTradeTypeSell, 'A');
    }
  } else if (nk0_1_position_ < 0) {
    int buy_size = abs(nk0_position_ + nk1_position_);
    nk0_om_.CancelAllBidOrders();
    // DBGLOG_TIME_CLASS_FUNC << "Positions: Cancelling Bid orders" << DBGLOG_ENDL_FLUSH;
    int nk0_bid_orders = nk0_om_.SumBidSizes();
    if (buy_size - nk0_bid_orders > 0 && nk0_bid_orders == 0) {
      buy_size = std::min(param_set_.unit_trade_size_, abs(buy_size));
      int nk0_best_ask_price = sgx_nk0_market_view_.market_update_info_.bestask_int_price_;
      // DBGLOG_TIME_CLASS_FUNC << "Positions: Buy size = " << buy_size << " at price =" << nk0_best_ask_price <<
DBGLOG_ENDL_FLUSH;
      nk0_om_.SendTradeIntPx(nk0_best_ask_price, buy_size, kTradeTypeBuy, 'A');
    }
  }
}
*/
void SgxNK1MMTrading::GetFlatTradingLogic() {}

void SgxNK1MMTrading::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  BaseTrading::OnMarketUpdate(_security_id_, _market_update_info_);
  if (_security_id_ == nkm_market_view_.security_id()) {
    last_msecs_nkm_update_ = watch_.msecs_from_midnight();
  }
  TradingLogic();
}

bool SgxNK1MMTrading::toTrade() {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " SGX_NK_1 ready: " << dep_market_view_.is_ready_
                           << " SGX_NK_0 ready: " << sgx_nk0_market_view_.is_ready_
                           << " SP_SGX_NK0_NK1 ready: " << spread_nk_market_view_.is_ready_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << "Trading Start: " << trading_start_utc_mfm_
                           << " Trading end: " << trading_end_utc_mfm_
                           << " Current msecs: " << watch_.msecs_from_midnight() << DBGLOG_ENDL_FLUSH;
  }
  if (!dep_market_view_.is_ready_ || !sgx_nk0_market_view_.is_ready_ || !spread_nk_market_view_.is_ready_ ||
      external_getflat_ || watch_.msecs_from_midnight() < trading_start_utc_mfm_ ||
      watch_.msecs_from_midnight() > trading_end_utc_mfm_) {
    DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " Not Trading" << DBGLOG_ENDL_FLUSH;
    return false;
  }
  return true;
}

void SgxNK1MMTrading::SetQuoteToFalse() {
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

void SgxNK1MMTrading::SetQuoteToTrue() {
  if (!to_quote_) {
    if (last_msecs_started_quoting_ < watch_.msecs_from_midnight() &&
        watch_.msecs_from_midnight() < trading_end_utc_mfm_ && watch_.msecs_from_midnight() > trading_start_utc_mfm_) {
      last_msecs_started_quoting_ = watch_.msecs_from_midnight();
      to_quote_ = true;
    }
  }
}

bool SgxNK1MMTrading::throttle_control() {
  return (throttle_manager_->allowed_through_throttle(watch_.msecs_from_midnight()));
}

void SgxNK1MMTrading::PlaceAskOrdersAtPrice(const int ask_int_price_to_quote_, const int size_to_place_) {
  int sum_ask_size_at_price_to_quote_ = nk1_om_.GetTotalAskSizeOrderedAtIntPx(ask_int_price_to_quote_);
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
    DBGLOG_TIME_CLASS_FUNC << sum_ask_size_at_price_to_quote_ << " ask orders already at " << ask_int_price_to_quote_
                           << DBGLOG_ENDL_FLUSH;
  if (sum_ask_size_at_price_to_quote_ < size_to_place_) {
    unsigned int ask_size_to_place_ = size_to_place_ - sum_ask_size_at_price_to_quote_;
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
      DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " Placing Ask Orders at " << ask_int_price_to_quote_
                             << " Size: " << ask_size_to_place_ << DBGLOG_ENDL_FLUSH;
    if (throttle_control()) nk1_om_.SendTradeIntPx(ask_int_price_to_quote_, ask_size_to_place_, kTradeTypeSell, 'S');
  }
}

void SgxNK1MMTrading::PlaceBidOrdersAtPrice(const int bid_int_price_to_quote_, const int size_to_place_) {
  int sum_bid_size_at_price_to_quote_ = nk1_om_.GetTotalBidSizeOrderedAtIntPx(bid_int_price_to_quote_);
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
    DBGLOG_TIME_CLASS_FUNC << sum_bid_size_at_price_to_quote_ << " bid orders already at " << bid_int_price_to_quote_
                           << DBGLOG_ENDL_FLUSH;
  if (sum_bid_size_at_price_to_quote_ < size_to_place_) {
    unsigned int bid_size_to_place_ = size_to_place_ - sum_bid_size_at_price_to_quote_;
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO))
      DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " Placing Bid Orders at " << bid_int_price_to_quote_
                             << " Size: " << bid_size_to_place_ << DBGLOG_ENDL_FLUSH;
    if (throttle_control()) nk1_om_.SendTradeIntPx(bid_int_price_to_quote_, bid_size_to_place_, kTradeTypeBuy, 'S');
  }
}

void SgxNK1MMTrading::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  std::string shc_not_ready = dep_market_view_.sec_name_indexer_.GetShortcodeFromId(_security_id_);
  DBGLOG_TIME << " market_data_interrupt_ for " << shc_not_ready << DBGLOG_ENDL_FLUSH;

  if (_security_id_ == nkm_market_view_.security_id()) {
    if (!nkm_data_interrupted_) {
      nkm_data_interrupted_ = true;
    }
  }

  if (dbglogger_.CheckLoggingLevel(TRADING_ERROR) && enable_market_data_interrupt_) {
    DBGLOG_TIME << " getflat_due_to_market_data_interrupt_ of " << shc_not_ready << " for QueryID: " << runtime_id_
                << DBGLOG_ENDL_FLUSH;
    if (livetrading_) {
      DBGLOG_DUMP;
    }

    bool sending_md_interrupt_email_ = true;
    if (sending_md_interrupt_email_ && livetrading_ &&  // live-trading and within trading window
        (watch_.msecs_from_midnight() > trading_start_utc_mfm_) &&
        (watch_.msecs_from_midnight() < trading_end_utc_mfm_)) {  // Send out an email / alert
      char hostname_[128];
      hostname_[127] = '\0';
      gethostname(hostname_, 127);

      std::string getflat_email_string_ = "";
      {
        std::ostringstream t_oss_;
        t_oss_ << "Strategy: " << runtime_id_ << " getflat_due_to_market_data_interrupt_ of " << shc_not_ready << " for"
               << " product " << dep_market_view_.shortcode() << " = " << dep_market_view_.secname() << " on "
               << hostname_ << "\n";

        getflat_email_string_ = t_oss_.str();
      }

      SendMail(getflat_email_string_, getflat_email_string_);
    }
  }
}

void SgxNK1MMTrading::OnMarketDataResumed(const unsigned int _security_id_) {
  std::string shc_ready =
      dep_market_view_.sec_name_indexer_.GetShortcodeFromId(_security_id_);  // data resumed for this shortcode

  if (_security_id_ == nkm_market_view_.security_id()) {
    if (nkm_data_interrupted_) {
      nkm_data_interrupted_ = false;
    }
  }
  if (dbglogger_.CheckLoggingLevel(TRADING_ERROR) && enable_market_data_interrupt_) {
    DBGLOG_TIME << " getflat_due_to_market_data_interrupt_ of " << shc_ready << " for QueryID: " << runtime_id_
                << DBGLOG_ENDL_FLUSH;
    if (livetrading_) {
      DBGLOG_DUMP;
    }

    bool sending_md_interrupt_email_ = true;
    if (sending_md_interrupt_email_ && livetrading_ &&  // live-trading and within trading window
        (watch_.msecs_from_midnight() > trading_start_utc_mfm_) &&
        (watch_.msecs_from_midnight() < trading_end_utc_mfm_)) {  // Send out an email / alert
      char hostname_[128];
      hostname_[127] = '\0';
      gethostname(hostname_, 127);

      std::string getflat_email_string_ = "";
      {
        std::ostringstream t_oss_;
        t_oss_ << "Strategy: " << runtime_id_ << " getflat_due_to_market_data_interrupt_ of " << shc_ready << " for"
               << " product " << dep_market_view_.shortcode() << " = " << dep_market_view_.secname() << " on "
               << hostname_ << "\n";

        getflat_email_string_ = t_oss_.str();
      }

      SendMail(getflat_email_string_, getflat_email_string_);
    }
  }
}

void SgxNK1MMTrading::ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_) {
  if (to_quote_) {
    total_time_quoted_ += (trading_end_utc_mfm_ - last_msecs_started_quoting_);
  }
  if (livetrading_) {
    printf("SIMRESULT %d %d %d %d %d %d\n", (int)nk1_om_.base_pnl().ReportConservativeTotalPNL(true),
           nk1_om_.trade_volume(), nk1_om_.SupportingOrderFilledPercent(), nk1_om_.BestLevelOrderFilledPercent(),
           nk1_om_.AggressiveOrderFilledPercent(), nk1_om_.ImproveOrderFilledPercent());
  } else {
    {
      int t_pnl_ = (int)(nk1_om_.base_pnl().ReportConservativeTotalPNL(true));
      int total_time_to_quote_ = trading_end_utc_mfm_ - trading_start_utc_mfm_;
      printf("Total time = %d\n", total_time_to_quote_);
      printf("Quoted time = %u\n", total_time_quoted_);
      double fraction_quoted_ = total_time_quoted_ * 100.0 / (1.0 * total_time_to_quote_);
      printf("SIMRESULT %d SGX_NK_1 %d %d %d %d %d %d %1.6f\n", tradingdate_, t_pnl_, nk1_om_.trade_volume(),
             nk1_om_.SupportingOrderFilledPercent(), nk1_om_.BestLevelOrderFilledPercent(),
             nk1_om_.AggressiveOrderFilledPercent(), nk1_om_.ImproveOrderFilledPercent(), fraction_quoted_);
      printf("SIMRESULT %d SGX_NK_0 %d %d %d %d %d %d\n", tradingdate_,
             (int)nk0_om_.base_pnl().ReportConservativeTotalPNL(true), nk0_om_.trade_volume(),
             nk0_om_.SupportingOrderFilledPercent(), nk0_om_.BestLevelOrderFilledPercent(),
             nk0_om_.AggressiveOrderFilledPercent(), nk0_om_.ImproveOrderFilledPercent());
      printf("SIMRESULT %d SP_SGX_NK0_NK1 %d %d %d %d %d %d\n", tradingdate_,
             (int)spread_nk_om_.base_pnl().ReportConservativeTotalPNL(true), spread_nk_om_.trade_volume(),
             spread_nk_om_.SupportingOrderFilledPercent(), spread_nk_om_.BestLevelOrderFilledPercent(),
             spread_nk_om_.AggressiveOrderFilledPercent(), spread_nk_om_.ImproveOrderFilledPercent());

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
                 << (order_manager_.SendOrderCount() + order_manager_.CxlOrderCount() +
                     order_manager_.ModifyOrderCount()) << "\n";
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
}
