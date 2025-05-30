#include "dvctrade/RiskManagement/delta_hedge.hpp"
#include "dvccode/CDef/math_utils.hpp"

namespace HFSAT {

DeltaHedge::DeltaHedge(DebugLogger& _dbglogger_, const Watch& _watch_,
                       const SecurityMarketView& _underlying_market_view_, SmartOrderManager& _order_manager_,
                       OptionsParamSet _paramset_, const bool _livetrading_)
    : this_options_data_vec_(),
      underlying_market_view_(_underlying_market_view_),
      order_manager_(_order_manager_),
      watch_(_watch_),
      dbglogger_(_dbglogger_),
      paramset_(_paramset_),
      trading_start_utc_mfm_(-1),
      trading_end_utc_mfm_(-1),
      delta_hedged_(0.0),
      delta_unhedged_(0.0),
      total_delta_(0.0),
      total_gamma_(0.0),
      total_vega_(0.0),
      total_theta_(0.0),
      gamma_ATM_(0.0),
      upper_threshold_(_paramset_.delta_hedge_upper_threshold_),
      lower_threshold_(_paramset_.delta_hedge_lower_threshold_),
      interest_rate_(NSESecurityDefinitions::GetInterestRate(watch_.YYYYMMDD())),
      should_be_getting_hedge_(0),
      should_be_getting_aggressive_hedge_(0),
      security_id_(_underlying_market_view_.security_id()),
      fractional_second_implied_vol_(_paramset_.fractional_second_implied_vol_) {
  order_manager_.AddPositionChangeListener(this);
  order_manager_.AddExecutionListener(this);
  watch_.subscribe_BigTimePeriod(this);  // Required so that delta hedge logic is called repeatedly
}

void DeltaHedge::AddOptionsToHedge(OptionsExecVars* option_to_hedge_, int _trading_start_mfm_, int _trading_end_mfm_) {
  option_to_hedge_->position_ = 0;

  const double days_to_expire_ =
      difftime(DateTime::GetTimeUTC(NSESecurityDefinitions::GetExpiryFromShortCode(option_to_hedge_->smv_->shortcode()),
                                    1000),
               DateTime::GetTimeMidnightUTC(watch_.YYYYMMDD())) /
      (3600 * 24);

  const double strike_price_ = NSESecurityDefinitions::GetStrikePriceFromShortCode(option_to_hedge_->smv_->shortcode());
  const OptionType_t option_type_ =
      (OptionType_t)NSESecurityDefinitions::GetOptionType(option_to_hedge_->smv_->shortcode());

  option_to_hedge_->option_type_ = option_type_;
  option_to_hedge_->strike_ = strike_price_;

  Option* t_option = new Option(option_to_hedge_->smv_->mid_price(), option_type_, strike_price_,
                                underlying_market_view_.mid_price(), days_to_expire_, interest_rate_);
  t_option->computeGreeks();

  if (_trading_start_mfm_ >= 0) {
    option_to_hedge_->trading_start_utc_mfm_ = _trading_start_mfm_;
  } else {
    option_to_hedge_->trading_start_utc_mfm_ = trading_start_utc_mfm_;
  }

  if (_trading_end_mfm_ >= 0) {
    option_to_hedge_->trading_end_utc_mfm_ = _trading_end_mfm_;
  } else {
    option_to_hedge_->trading_end_utc_mfm_ = trading_end_utc_mfm_;
  }

  t_option->SetDaysToExpiry(days_to_expire_ - (double)option_to_hedge_->trading_start_utc_mfm_ / (3600 * 24 * 1000));

  DBGLOG_TIME_CLASS_FUNC << " trading_start_time_ " << _trading_start_mfm_ << " trading_end_time_ " << _trading_end_mfm_
                         << "common: " << trading_start_utc_mfm_ << " " << trading_end_utc_mfm_ << DBGLOG_ENDL_FLUSH;

  option_to_hedge_->option_vars = t_option;
  this_options_data_vec_.push_back(option_to_hedge_);

  // market IV for risk management ( not modelled )
  MovingAvgPriceImpliedVol* t_implied_vol_calculator_ = MovingAvgPriceImpliedVol::GetUniqueInstance(
      dbglogger_, watch_, *(option_to_hedge_->smv_), fractional_second_implied_vol_, kPriceTypeMidprice);
  t_implied_vol_calculator_->add_unweighted_indicator_listener(moving_average_implied_vol_vec_.size(), this);
  moving_average_implied_vol_vec_.push_back(t_implied_vol_calculator_);
}

void DeltaHedge::OnOptionPositionChange(int t_new_position_, int position_diff_, const unsigned int _option_index_) {
  int position_change_ = t_new_position_ - this_options_data_vec_[_option_index_]->position_;
  this_options_data_vec_[_option_index_]->position_ = t_new_position_;
  delta_unhedged_ += (this_options_data_vec_[_option_index_]->option_vars)->Delta() * position_change_;
  total_delta_ = delta_unhedged_ + delta_hedged_;
  total_gamma_ += (this_options_data_vec_[_option_index_]->option_vars)->Gamma() * position_change_;
  total_vega_ += (this_options_data_vec_[_option_index_]->option_vars)->Vega() * position_change_;
  total_theta_ += (this_options_data_vec_[_option_index_]->option_vars)->Theta() * position_change_;
  GetDeltaHedgeLogic();
  CallPlaceCancelNonBestLevels();
}

// Changed every one minute
void DeltaHedge::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  double old_delta_ = (this_options_data_vec_[_indicator_index_]->option_vars)->Delta();
  double old_gamma_ = (this_options_data_vec_[_indicator_index_]->option_vars)->Gamma();
  double old_vega_ = (this_options_data_vec_[_indicator_index_]->option_vars)->Vega();
  double old_theta_ = (this_options_data_vec_[_indicator_index_]->option_vars)->Theta();

  (this_options_data_vec_[_indicator_index_]->option_vars)->SetFuturePrice(underlying_market_view_.mid_price());
  (this_options_data_vec_[_indicator_index_]->option_vars)->SetImpliedVol(_new_value_);
  (this_options_data_vec_[_indicator_index_]->option_vars)->computeGreeks();

  int position_ = this_options_data_vec_[_indicator_index_]->position_;

  total_delta_ += ((this_options_data_vec_[_indicator_index_]->option_vars)->Delta() - old_delta_) * position_;
  total_gamma_ += ((this_options_data_vec_[_indicator_index_]->option_vars)->Gamma() - old_gamma_) * position_;
  total_vega_ += ((this_options_data_vec_[_indicator_index_]->option_vars)->Vega() - old_vega_) * position_;
  total_theta_ += ((this_options_data_vec_[_indicator_index_]->option_vars)->Theta() - old_theta_) * position_;
  delta_unhedged_ = total_delta_ - delta_hedged_;

  GetDeltaHedgeLogic();
  CallPlaceCancelNonBestLevels();
}

void DeltaHedge::GetDeltaHedgeLogic() {
  if (std::abs(total_delta_) > upper_threshold_)
    GetAggressiveDeltaHedge();
  else if (std::abs(total_delta_) > lower_threshold_)
    GetDeltaHedge();
  else if (std::abs(total_delta_) < lower_threshold_ / 2)
    GetDeltaHedgeFlat();
}

void DeltaHedge::GetDeltaHedge() {
  int position_ = (int)delta_unhedged_;
  if (position_ == 0) {  // nothing to be done, cancel all remaining orders
    order_manager_.CancelAllOrders();
  } else {
    if (position_ > 0) {
      // long hence cancel all bid orders
      order_manager_.CancelAllBidOrders();

      // size already placed at best or above
      int t_size_already_placed_ =
          order_manager_.SumAskSizeConfirmedEqAboveIntPrice(underlying_market_view_.bestask_int_price()) +
          order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(underlying_market_view_.bestask_int_price());

      int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(std::min(position_, paramset_.unit_trade_size_),
                                                                 underlying_market_view_.min_order_size());

      if (trade_size_to_place_ > t_size_already_placed_) {  // if the required size is not present then add more
        SendTradeAndLog(underlying_market_view_.bestask_int_price(), trade_size_to_place_ - t_size_already_placed_,
                        kTradeTypeSell, 'B');
        // done_for_this_round_ = true;
      }

      // cancelling any extra orders to avoid overfill
      order_manager_.KeepAskSizeInPriceRange(position_);

    } else {  // my_position_ < 0

      // short hence cancel all sell orders
      order_manager_.CancelAllAskOrders();
      //    bool done_for_this_round_ = false;

      // we should place at best, limit the size by some %age of mkt_size to not effect the market, by default 50%
      int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(-position_, underlying_market_view_.min_order_size());

      // size already placed at best or above
      int t_size_already_placed_ =
          order_manager_.SumBidSizeConfirmedEqAboveIntPrice(underlying_market_view_.bestbid_int_price()) +
          order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(underlying_market_view_.bestbid_int_price());

      if (trade_size_to_place_ > t_size_already_placed_) {  // if the required size is not present then add more
        SendTradeAndLog(underlying_market_view_.bestbid_int_price(), trade_size_to_place_ - t_size_already_placed_,
                        kTradeTypeBuy, 'B');
        // done_for_this_round_ = true;
      }

      // cancelling any extra orders to avoid overfill
      order_manager_.KeepBidSizeInPriceRange(-position_);
    }
  }
}

void DeltaHedge::GetAggressiveDeltaHedge() {
  int position_ = (int)delta_unhedged_;
  if (position_ == 0) {
    order_manager_.CancelAllOrders();
  } else {
    if (position_ > 0) {
      order_manager_.CancelAllBidOrders();
      order_manager_.CancelAsksBelowIntPrice(underlying_market_view_.bestbid_int_price());
      int t_size_ordered_ =
          order_manager_.SumAskSizeConfirmedEqAboveIntPrice(underlying_market_view_.bestbid_int_price()) +
          order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(underlying_market_view_.bestbid_int_price());

      int trade_size_required_ = MathUtils::GetFlooredMultipleOf(std::min(position_, paramset_.unit_trade_size_),
                                                                 underlying_market_view_.min_order_size());

      if (t_size_ordered_ < trade_size_required_) {
        SendTradeAndLog(underlying_market_view_.bestbid_int_price(), trade_size_required_ - t_size_ordered_,
                        kTradeTypeSell, 'B');
      }
    } else {
      order_manager_.CancelAllAskOrders();  // short hence cancel all sell orders
      order_manager_.CancelBidsBelowIntPrice(
          underlying_market_view_.bestask_int_price());  // cancel all non bestlevel bid orders
      // ideal size to place ask for is what the position is, except limited to unit_trade_size_ ( for sanity and market
      // effect reasons )
      int t_size_ordered_ =
          order_manager_.SumBidSizeConfirmedEqAboveIntPrice(underlying_market_view_.bestask_int_price()) +
          order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(underlying_market_view_.bestask_int_price());
      int trade_size_required_ = MathUtils::GetFlooredMultipleOf(-position_, underlying_market_view_.min_order_size());
      if (t_size_ordered_ < trade_size_required_) {  // if the required size is not present then add more
        SendTradeAndLog(underlying_market_view_.bestask_int_price(), trade_size_required_ - t_size_ordered_,
                        kTradeTypeBuy, 'A');
      }
    }
  }
}

void DeltaHedge::GetDeltaHedgeFlat() {
  int position_ = -(int)delta_hedged_;
  if (position_ == 0) {  // nothing to be done, cancel all remaining orders
    order_manager_.CancelAllOrders();
  } else {
    if (position_ > 0) {
      // long hence cancel all bid orders
      order_manager_.CancelAllBidOrders();

      // size already placed at best or above
      int t_size_already_placed_ =
          order_manager_.SumAskSizeConfirmedEqAboveIntPrice(underlying_market_view_.bestask_int_price()) +
          order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(underlying_market_view_.bestask_int_price());

      int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(std::min(position_, paramset_.unit_trade_size_),
                                                                 underlying_market_view_.min_order_size());

      if (trade_size_to_place_ > t_size_already_placed_) {  // if the required size is not present then add more
        SendTradeAndLog(underlying_market_view_.bestask_int_price(), trade_size_to_place_ - t_size_already_placed_,
                        kTradeTypeSell, 'B');
        // done_for_this_round_ = true;
      }

      // cancelling any extra orders to avoid overfill
      order_manager_.KeepAskSizeInPriceRange(position_);

    } else {  // my_position_ < 0

      // short hence cancel all sell orders
      order_manager_.CancelAllAskOrders();
      //    bool done_for_this_round_ = false;

      // we should place at best, limit the size by some %age of mkt_size to not effect the market, by default 50%
      int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(-position_, underlying_market_view_.min_order_size());

      // size already placed at best or above
      int t_size_already_placed_ =
          order_manager_.SumBidSizeConfirmedEqAboveIntPrice(underlying_market_view_.bestbid_int_price()) +
          order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(underlying_market_view_.bestbid_int_price());

      if (trade_size_to_place_ > t_size_already_placed_) {  // if the required size is not present then add more
        SendTradeAndLog(underlying_market_view_.bestbid_int_price(), trade_size_to_place_ - t_size_already_placed_,
                        kTradeTypeBuy, 'B');
        // done_for_this_round_ = true;
      }

      // cancelling any extra orders to avoid overfill
      order_manager_.KeepBidSizeInPriceRange(-position_);
    }
  }
}

void DeltaHedge::GetAggressiveDeltaHedgeFlat() {
  int position_ = -(int)delta_hedged_;
  if (position_ == 0) {
    order_manager_.CancelAllOrders();
  } else {
    if (position_ > 0) {
      order_manager_.CancelAllBidOrders();
      order_manager_.CancelAsksBelowIntPrice(underlying_market_view_.bestbid_int_price());
      int t_size_ordered_ =
          order_manager_.SumAskSizeConfirmedEqAboveIntPrice(underlying_market_view_.bestbid_int_price()) +
          order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(underlying_market_view_.bestbid_int_price());

      int trade_size_required_ = MathUtils::GetFlooredMultipleOf(std::min(position_, paramset_.unit_trade_size_),
                                                                 underlying_market_view_.min_order_size());

      if (t_size_ordered_ < trade_size_required_) {
        SendTradeAndLog(underlying_market_view_.bestbid_int_price(), trade_size_required_ - t_size_ordered_,
                        kTradeTypeSell, 'B');
      }
    } else {
      order_manager_.CancelAllAskOrders();  // short hence cancel all sell orders
      order_manager_.CancelBidsBelowIntPrice(
          underlying_market_view_.bestask_int_price());  // cancel all non bestlevel bid orders
      // ideal size to place ask for is what the position is, except limited to unit_trade_size_ ( for sanity and market
      // effect reasons )
      int t_size_ordered_ =
          order_manager_.SumBidSizeConfirmedEqAboveIntPrice(underlying_market_view_.bestask_int_price()) +
          order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(underlying_market_view_.bestask_int_price());
      int trade_size_required_ = MathUtils::GetFlooredMultipleOf(-position_, underlying_market_view_.min_order_size());
      if (t_size_ordered_ < trade_size_required_) {  // if the required size is not present then add more
        SendTradeAndLog(underlying_market_view_.bestask_int_price(), trade_size_required_ - t_size_ordered_,
                        kTradeTypeBuy, 'A');
      }
    }
  }
}

void DeltaHedge::SendTradeAndLog(int _int_px_, int _size_, TradeType_t _buysell_, char _level_indicator_) {
  order_manager_.SendTradeIntPx(_int_px_, _size_, _buysell_, _level_indicator_);
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "SendTrade " << GetTradeTypeChar(_buysell_) << " " << underlying_market_view_.shortcode()
                           << " of " << _size_ << " @ IntPx: " << _int_px_
                           << " mkt: " << underlying_market_view_.bestbid_size() << " @ "
                           << underlying_market_view_.bestbid_price() << " X "
                           << underlying_market_view_.bestask_price() << " @ " << underlying_market_view_.bestask_size()
                           << DBGLOG_ENDL_FLUSH;
  }
}

inline void DeltaHedge::CallPlaceCancelNonBestLevels() {
  order_manager_.CancelBidsBelowIntPrice(underlying_market_view_.bestbid_int_price());
  order_manager_.CancelAsksBelowIntPrice(underlying_market_view_.bestask_int_price());
}

void DeltaHedge::OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_) {
  delta_hedged_ = -t_new_position_;
  delta_unhedged_ = total_delta_ - delta_hedged_;
}

void DeltaHedge::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if ((watch_.msecs_from_midnight() - watch_.last_fifteen_min_page_msecs()) > 900000) {
    for (auto i = 0u; i < this_options_data_vec_.size(); i++) {
      this_options_data_vec_[i]->option_vars->SetDaysToExpiry(this_options_data_vec_[i]->option_vars->Days() -
                                                              15 / (60 * 24));
    }
  }
  GetDeltaHedgeLogic();
  CallPlaceCancelNonBestLevels();
}
}
