/**
     \file ExecLogicCode/nik_price_pair_based_aggressive_trading.cpp

     \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
     Suite No 353, Evoma, #14, Bhattarhalli,
     Old Madras Road, Near Garden City College,
     KR Puram, Bangalore 560049, India
     +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/ExecLogic/nik_price_pair_based_aggressive_trading.hpp"
// exec_logic_code / defines.hpp was empty

namespace HFSAT {

NikPricePairBasedAggressiveTrading::NikPricePairBasedAggressiveTrading(
    DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
    SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
    MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
    const std::vector<std::string> _this_model_source_shortcode_vec_, NikTradingManager* _niktm_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_),
      niktm_(_niktm_),
      last_pair_bid_cancel_msecs_(0),
      last_pair_ask_cancel_msecs_(0) {
  is_pair_strategy_ = true;

  if (!niktm_->is_max_opentrade_loss_set_) {
    niktm_->max_opentrade_loss_ = param_set_.max_opentrade_loss_;
    niktm_->is_max_opentrade_loss_set_ = true;
  }

  if (!niktm_->is_max_loss_set_) {
    niktm_->max_loss_ = param_set_.max_loss_;
    niktm_->is_max_loss_set_ = true;
  }

  if (!niktm_->is_break_msecs_on_max_opentrade_loss_set_) {
    niktm_->break_msecs_on_max_opentrade_loss_ = param_set_.break_msecs_on_max_opentrade_loss_;
    niktm_->is_break_msecs_on_max_opentrade_loss_set_ = true;
  }

  if (security_id_ == 1) {
    for (auto i = 0u; i < param_set_vec_.size(); i++) {
      UpdatePairParams(param_set_vec_[i]);
    }

    UpdatePairParams(param_set_);

    BuildTradeVarSets();
  }
}

void NikPricePairBasedAggressiveTrading::UpdatePairParams(ParamSet& _param_set_) {
  if (!_param_set_.read_pair_unit_trade_size_factor_) {
    _param_set_.pair_unit_trade_size_factor_ = 10;
  }

  _param_set_.unit_trade_size_ *= _param_set_.pair_unit_trade_size_factor_;
  int pos_factor_ = (10 / _param_set_.pair_unit_trade_size_factor_);

  _param_set_.max_position_ *= (10 * pos_factor_);
  _param_set_.zeropos_limits_ *= (10 * pos_factor_);
  _param_set_.highpos_limits_ *= (10 * pos_factor_);
  _param_set_.worst_case_position_ *= (10 * pos_factor_);

  if (_param_set_.read_pair_place_diff_ && !_param_set_.read_pair_keep_diff_) {
    _param_set_.pair_keep_diff_ = _param_set_.pair_place_diff_;
  }

  if (_param_set_.read_pair_keep_diff_ && !_param_set_.read_pair_place_diff_) {
    _param_set_.pair_place_diff_ = _param_set_.pair_keep_diff_;
  }

  _param_set_.zeropos_place_ -= _param_set_.pair_place_diff_;
  _param_set_.zeropos_keep_ -= _param_set_.pair_keep_diff_;
  _param_set_.increase_place_ -= _param_set_.pair_place_diff_;
  _param_set_.increase_keep_ -= _param_set_.pair_keep_diff_;
  _param_set_.decrease_place_ -= _param_set_.pair_place_diff_;
  _param_set_.decrease_keep_ -= _param_set_.pair_keep_diff_;

  _param_set_.zeropos_place_ = _param_set_.zeropos_place_ > 0 ? _param_set_.zeropos_place_ : 0;
  _param_set_.zeropos_keep_ = _param_set_.zeropos_keep_ > 0 ? _param_set_.zeropos_keep_ : 0;
  _param_set_.increase_place_ = _param_set_.increase_place_ > 0 ? _param_set_.increase_place_ : 0;
  _param_set_.increase_keep_ = _param_set_.increase_keep_ > 0 ? _param_set_.increase_keep_ : 0;
  _param_set_.decrease_place_ = _param_set_.decrease_place_ > 0 ? _param_set_.decrease_place_ : 0;
  _param_set_.decrease_keep_ = _param_set_.decrease_keep_ > 0 ? _param_set_.decrease_keep_ : 0;

  if (_param_set_.read_pair_zeropos_keep_ && _param_set_.read_pair_zeropos_decrease_diff_ &&
      _param_set_.read_pair_increase_zeropos_diff_ && _param_set_.read_pair_place_keep_diff_) {
    _param_set_.zeropos_keep_ = _param_set_.pair_zeropos_keep_;
    _param_set_.read_zeropos_keep_ = true;
    _param_set_.zeropos_decrease_diff_ = _param_set_.pair_zeropos_decrease_diff_;
    _param_set_.read_zeropos_decrease_diff_ = true;
    _param_set_.increase_zeropos_diff_ = _param_set_.pair_increase_zeropos_diff_;
    _param_set_.read_increase_zeropos_diff_ = true;
    _param_set_.place_keep_diff_ = _param_set_.pair_place_keep_diff_;
    _param_set_.read_place_keep_diff_ = true;
    _param_set_.read_zeropos_place_ = false;
    _param_set_.read_increase_place_ = false;
    _param_set_.read_increase_keep_ = false;
    _param_set_.decrease_place_ = false;
    _param_set_.decrease_keep_ = false;
    _param_set_.ReconcileParams(dep_market_view_.shortcode());
  }

  if (_param_set_.read_pair_aggressive_) {
    _param_set_.aggressive_ = _param_set_.pair_aggressive_;
    _param_set_.read_aggressive_ = true;
  }
  if (_param_set_.read_pair_improve_) {
    _param_set_.improve_ = _param_set_.pair_improve_;
    _param_set_.read_improve_ = true;
  }
}

bool NikPricePairBasedAggressiveTrading::ShouldBeGettingFlat() {
  /*
          should_be_getting_flat_ = getflat_due_to_external_getflat_ ||
          getflat_due_to_close_ ||
          getflat_due_to_max_position_ ||
          getflat_due_to_max_loss_ ||
          getflat_due_to_max_opentrade_loss_ ||
          // getflat_due_to_max_pertrade_loss_ ||
          getflat_due_to_economic_times_ ;
   */

  if (external_getflat_) {
    if (!getflat_due_to_external_getflat_) {  // first time that getflat_due_to_external_getflat_ is false and
                                              // external_getflat_ is true ..
      // which means the user message was received right now
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_external_getflat_ " << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      getflat_due_to_external_getflat_ = true;
    }
    return true;
  } else {
    getflat_due_to_external_getflat_ = false;
  }

  if (getflat_due_to_close_ ||
      (watch_.msecs_from_midnight() >
       trading_end_utc_mfm_)) {  // if getflat_due_to_close_ is once activated then it can't be reset  ...
    // perhaps unless a usermessage is sent ?
    if (!getflat_due_to_close_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_close_ current: " << watch_.msecs_from_midnight() << " " << trading_end_utc_mfm_
                    << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      getflat_due_to_close_ = true;
    }
    return true;
  } else {
    getflat_due_to_close_ = false;
  }

  if (livetrading_ && niktm_->IsHittingMaxLoss()) {
    if (!getflat_due_to_max_loss_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_max_loss_ " << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }

        if (livetrading_ &&  // live-trading and within trading window
            (watch_.msecs_from_midnight() > trading_start_utc_mfm_) &&
            (watch_.msecs_from_midnight() < trading_end_utc_mfm_)) {  // Send out an email / alert
          char hostname_[128];
          hostname_[127] = '\0';
          gethostname(hostname_, 127);

          std::string getflat_email_string_ = "";
          {
            std::ostringstream t_oss_;
            t_oss_ << "Strategy: " << runtime_id_
                   << " getflat_due_to_max_loss_: " << order_manager_.base_pnl().total_pnl() << " product "
                   << dep_market_view_.shortcode() << " = " << dep_market_view_.secname() << " on " << hostname_
                   << "\n";

            getflat_email_string_ = t_oss_.str();
          }

          HFSAT::Email email_;
          email_.setSubject(getflat_email_string_);
          email_.addRecepient("nseall@tworoads.co.in");
          email_.addSender("nseall@tworoads.co.in");
          email_.content_stream << getflat_email_string_ << "<br/>";
          email_.sendMail();
        }
      }
      getflat_due_to_max_loss_ = true;
    }
    return true;
  } else {
    getflat_due_to_max_loss_ = false;
  }

  // TODO .. add support for different Economic Zones, based on basename
  // and possibly override by paramfile

  // Another TODO ... change the threshold of severity based on the strategy parameters

  if (getflat_due_to_market_data_interrupt_ && enable_market_data_interrupt_) {
    return true;
  }

  if (getflat_due_to_max_opentrade_loss_) {
    return true;
  }

  if (getflat_due_to_economic_times_) {
    return true;
  }

  if (getflat_due_to_non_tradable_events_ && is_event_based_) {
    return true;
  }

  if (should_be_getting_flat_) {  // currently it is set  ... so after this call we will start normal trading
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME << "resume_normal_trading @"
                  << " NYC: " << watch_.NYTimeString() << " UTC: " << watch_.UTCTimeString()
                  << " IND: " << watch_.IndTimeString() << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
  }

  return false;
}

void NikPricePairBasedAggressiveTrading::OnTimePeriodUpdate(const int num_pages_to_add_) {
  BaseTrading::OnTimePeriodUpdate(num_pages_to_add_);

  if (!getflat_due_to_max_opentrade_loss_) {
    // check opentrade loss
    if (niktm_->IsHittingOpentradeLoss()) {
      getflat_due_to_max_opentrade_loss_ = true;
    }
  } else {
    getflat_due_to_max_opentrade_loss_ = false;
  }

  if (applicable_severity_ >= severity_to_getflat_on_) {
    if (!getflat_due_to_economic_times_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << "getflat_due_to_economic_times_ @"
                    << " NYC: " << watch_.NYTimeString() << " UTC: " << watch_.UTCTimeString()
                    << " IND: " << watch_.IndTimeString() << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      getflat_due_to_economic_times_ = true;
    }
  } else {
    getflat_due_to_economic_times_ = false;
  }

  if (watch_.msecs_from_midnight() < trading_start_utc_mfm_) {
    return;
  }

  if (ShouldBeGettingFlat()) {
    niktm_->GetFlat();
    GetFlatTradingLogic();
  } else {
    niktm_->ResumeAfterGetFlat();
  }
}

void NikPricePairBasedAggressiveTrading::TradeVarSetLogic(int t_position) {
  current_position_tradevarset_map_index_ = P2TV_zero_idx_;
  if (map_pos_increment_ > 1)  // only when param_set_.max_position_ > MAX_POS_MAP_SIZE
  {
    if (t_position > 0) {
      current_position_tradevarset_map_index_ += std::min(MAX_POS_MAP_SIZE, (abs(t_position) / map_pos_increment_));
      current_tradevarset_ = position_tradevarset_map_[current_position_tradevarset_map_index_];

      if ((current_tradevarset_.l1bid_trade_size_ + t_position) > param_set_.max_position_) {
        current_tradevarset_.l1bid_trade_size_ = param_set_.max_position_ - t_position;
      }
    } else {
      current_position_tradevarset_map_index_ -= std::min(MAX_POS_MAP_SIZE, (abs(t_position) / map_pos_increment_));
      current_tradevarset_ = position_tradevarset_map_[current_position_tradevarset_map_index_];

      if ((current_tradevarset_.l1ask_trade_size_ - t_position) > param_set_.max_position_) {
        current_tradevarset_.l1ask_trade_size_ = param_set_.max_position_ + t_position;
      }
    }
  } else {
    if (t_position > 0) {
      current_position_tradevarset_map_index_ += std::min(MAX_POS_MAP_SIZE, abs(t_position));
    } else {
      current_position_tradevarset_map_index_ -= std::min(MAX_POS_MAP_SIZE, abs(t_position));
    }
    current_tradevarset_ = position_tradevarset_map_[current_position_tradevarset_map_index_];
  }

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << " newpos: " << my_position_ << " newcombinedposition: " << t_position
                           << " gpos: " << my_global_position_ << " stddpfac " << stdev_scaled_capped_in_ticks_
                           << " mapidx " << current_position_tradevarset_map_index_ << ' '
                           << ToString(current_tradevarset_).c_str() << DBGLOG_ENDL_FLUSH;
  }
}

void NikPricePairBasedAggressiveTrading::OnPositionChange(int t_new_position_, int position_diff_,
                                                          const unsigned int _security_id_) {
  BaseTrading::OnPositionChange(t_new_position_, position_diff_, _security_id_);
  niktm_->OnPositionUpdate(_security_id_, t_new_position_);
}

void NikPricePairBasedAggressiveTrading::OnExec(const int t_new_position_, const int _exec_quantity_,
                                                const TradeType_t _buysell_, const double _price_,
                                                const int r_int_price_, const int _security_id_) {
  BaseTrading::OnExec(t_new_position_, _exec_quantity_, _buysell_, _price_, r_int_price_, _security_id_);
  niktm_->OnExec(_security_id_, t_new_position_, _exec_quantity_, _buysell_, _price_, r_int_price_);
}

void NikPricePairBasedAggressiveTrading::TradingLogic() {
  // setting top level directives
  if (dbglogger_.CheckLoggingLevel(MUM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << " Last Buy: " << last_buy_msecs_ << " Last Sell: " << last_sell_msecs_
                           << " Last agg buy: " << last_agg_buy_msecs_ << " last agg sell: " << last_agg_sell_msecs_
                           << " My position: " << my_position_ << " target price: " << target_price_
                           << " best nonself bid" << best_nonself_bid_price_ << " best nonself ask "
                           << best_nonself_ask_price_ << " " << DBGLOG_ENDL_FLUSH;
    ShowParams();
  }

  top_bid_place_ = false;
  top_bid_keep_ = false;
  top_bid_improve_ = false;
  top_ask_lift_ = false;
  bid_improve_keep_ = false;

  if (  // check if we have any allowance to place orders at top level
      (my_position_ <= (param_set_.max_position_ * param_set_.abs_max_pos_factor_ - param_set_.unit_trade_size_)) &&
      (current_tradevarset_.l1bid_trade_size_ > 0) &&
      // first check for cooloff_interval
      ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
       ((best_nonself_bid_int_price_ - last_buy_int_price_) <
        (param_set_.px_band_ - 1)))  // later change setting of last_buy_int_price_ to include px_band_
      ) {
    // check if the margin of buying at the price ( best_nonself_bid_price_ )
    // i.e. ( target_price_ - best_nonself_bid_price_ )
    // exceeds the threshold current_tradevarset_.l1bid_place_
    if ((best_nonself_bid_size_ > param_set_.safe_distance_) ||
        ((security_id_ == 0 && niktm_->nk_bestbid_price_ < niktm_->nkm_bestbid_price_) ||
         (security_id_ == 1 && niktm_->nkm_bestbid_price_ < niktm_->nk_bestbid_price_)) ||
        ((target_price_ - best_nonself_bid_price_ >=
          current_tradevarset_.l1bid_place_ - l1_bias_ - l1_order_bias_ - short_positioning_bias_ +
              param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1)) &&
         (best_nonself_bid_size_ >
          param_set_.min_size_to_join_) &&  // Don't place any orders at a level if less than X size there
         (watch_.msecs_from_midnight() - last_pair_bid_cancel_msecs_ > param_set_.pair_exec_cancel_cooloff_msecs_) &&
         (param_set_.place_on_trade_update_implied_quote_ || !dep_market_view_.trade_update_implied_quote()))) {
      top_bid_place_ = true;
      top_bid_keep_ = true;

      if (watch_.msecs_from_midnight() - last_agg_buy_msecs_ > param_set_.agg_cooloff_interval_) {
        // conditions to place aggressive orders:
        // ALLOWED_TO_AGGRESS
        // position is not too long already
        // spread narrow
        // signal strong
        if ((param_set_.allowed_to_aggress_) &&  // external control on aggressing
            (target_price_ - best_nonself_ask_price_ >=
             current_tradevarset_.l1bid_aggressive_ +
                 param_set_.spread_increase_ * (dep_market_view_.spread_increments() -
                                                1)) &&  // only LIFT offer if the margin of buying here i.e. (
                                                        // target_price_ - best_nonself_ask_price_ ) exceeds the
                                                        // threshold current_tradevarset_.l1bid_aggressive_
            (my_position_ <=
             param_set_.max_position_to_lift_) &&  // Don't LIFT offer when my_position_ is already decently long
            (dep_market_view_.spread_increments() <=
             param_set_.max_int_spread_to_cross_) &&  // Don't LIFT when effective spread is to much
            ((last_buy_msecs_ <= 0) ||
             (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
             ((best_nonself_ask_int_price_ - last_buy_int_price_) <
              (param_set_.px_band_ - 1)))  // TODO_OPT : later change setting of last_buy_int_price_ to include px_band_
            ) {
          last_agg_buy_msecs_ = watch_.msecs_from_midnight();
          if (security_id_ == 0 && niktm_->nkm_bestask_price_ < niktm_->nk_bestask_price_) {
            niktm_->BuyNKM();
          } else if (security_id_ == 1 && niktm_->nk_bestask_price_ < niktm_->nkm_bestask_price_) {
            niktm_->BuyNK();
          } else {
            top_ask_lift_ = true;

            // when we are already long and ask_lift is set to true,
            // place and keep at top bid are set false so that
            // we cancel top level orders before we place new ones at aggressive prices
            if (my_position_ >= param_set_.max_position_to_cancel_on_lift_) {
              top_bid_place_ = false;
              top_bid_keep_ = false;
            }
          }
        } else {
          // Even of the above condition doesn't allow aggressive order, place aggressive in NKM in exetreme cases
          if (security_id_ == 1 && niktm_->nkm_bestask_price_ < niktm_->nk_bestbid_price_ &&
              watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) {
            //		        std::cout << "Hey, this is special aggres buy case for NKM. nkm_mkt: [ " <<
            // niktm_->nkm_bestbid_size_ << " "
            //		            << niktm_->nkm_bestbid_price_ << " * " << niktm_->nkm_bestask_price_ << " " <<
            // niktm_->nkm_bestask_size_ << " ] nk_mkt: [ "
            //		            << niktm_->nk_bestbid_size_ << " " << niktm_->nk_bestbid_price_ << " * " <<
            // niktm_->nk_bestask_price_ << " "
            //		            << niktm_->nk_bestask_size_ << std::endl;
            top_ask_lift_ = true;
          } else {
            top_ask_lift_ = false;

            // conditions to place market improving bid orders:
            // ALLOWED_TO_IMPROVE
            // position is not too long already
            // spread wide
            // signal strong
            if ((param_set_.allowed_to_improve_) &&
                (my_position_ <=
                 param_set_
                     .max_position_to_bidimprove_) &&  // Don't improve bid when my_position_ is already decently long
                (dep_market_view_.spread_increments() >= param_set_.min_int_spread_to_improve_) &&
                (target_price_ - best_nonself_bid_price_ - dep_market_view_.min_price_increment() >=
                 current_tradevarset_.l1bid_improve_) &&
                ((last_buy_msecs_ <= 0) ||
                 (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
                 (((best_nonself_bid_int_price_ + 1) - last_buy_int_price_) <
                  (param_set_.px_band_ -
                   1)))  // TODO_OPT : later change setting of last_buy_int_price_ to include px_band_
                ) {
              top_bid_improve_ = true;
            } else {
              top_bid_improve_ = false;
            }
          }
        }
      }

    } else {
      // either size less than min_size_to_join_
      // or signal is not strong enough for placing bids at best_nonself_bid_price_
      // check if we should retain existing bids due to place_in_line

      // TODO : perhaps we should make place_in_line effect bestbid_queue_hysterisis_ smarter
      //        for instance when short term volatility in the market is very high
      //        then being high in the queue should count for less.
      if ((watch_.msecs_from_midnight() - last_pair_bid_cancel_msecs_ > param_set_.pair_exec_cancel_cooloff_msecs_) &&
          (security_id_ == 0 && niktm_->nk_bestbid_price_ <= niktm_->nkm_bestbid_price_) &&
          ((target_price_ - best_nonself_bid_price_ + bestbid_queue_hysterisis_) >=
           current_tradevarset_.l1bid_keep_ - l1_bias_ - l1_order_bias_ - short_positioning_bias_ +
               param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1))) {
        top_bid_keep_ = true;
      } else {
        top_bid_keep_ = false;
      }
    }

    if (((dep_market_view_.spread_increments() > 1) &&
         (target_price_ - best_nonself_bid_price_) >=
             current_tradevarset_.l1bid_improve_keep_ - l1_bias_ - l1_order_bias_ - short_positioning_bias_ +
                 param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1))) {
      bid_improve_keep_ = true;
    } else {
      bid_improve_keep_ = false;
    }
  }

  top_ask_place_ = false;
  top_ask_keep_ = false;
  top_ask_improve_ = false;
  top_bid_hit_ = false;
  ask_improve_keep_ = false;

  if (  // check if we have any allowance to place orders at top level
      (my_position_ >= -(param_set_.max_position_ * param_set_.abs_max_pos_factor_ - param_set_.unit_trade_size_)) &&
      (current_tradevarset_.l1ask_trade_size_ > 0) &&
      // first check for cooloff_interval
      ((last_sell_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
       (last_sell_int_price_ - best_nonself_ask_int_price_ <
        (param_set_.px_band_ - 1)))  // later change setting of last_sell_int_price_ to include px_band_
      ) {
    // check if the margin of placing limit ask orders at the price ( best_nonself_ask_price_ )
    // i.e. ( best_nonself_ask_price_ - target_price_ )
    //      exceeds the threshold current_tradevarset_.l1ask_place_
    if ((best_nonself_ask_size_ > param_set_.safe_distance_) ||
        ((security_id_ == 0 && niktm_->nk_bestask_price_ > niktm_->nkm_bestask_price_) ||
         (security_id_ == 1 && niktm_->nkm_bestask_price_ > niktm_->nk_bestask_price_)) ||
        ((best_nonself_ask_price_ - target_price_ >=
          current_tradevarset_.l1ask_place_ - l1_bias_ - l1_order_bias_ - long_positioning_bias_ +
              param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1)) &&
         (best_nonself_ask_size_ > param_set_.min_size_to_join_) &&
         (watch_.msecs_from_midnight() - last_pair_ask_cancel_msecs_ > param_set_.pair_exec_cancel_cooloff_msecs_) &&
         (param_set_.place_on_trade_update_implied_quote_ || !dep_market_view_.trade_update_implied_quote()))) {
      top_ask_place_ = true;
      top_ask_keep_ = true;

      if (watch_.msecs_from_midnight() - last_agg_sell_msecs_ > param_set_.agg_cooloff_interval_) {
        // conditions to place aggressive orders:
        // ALLOWED_TO_AGGRESS
        // position is not too short already
        // spread narrow
        // signal strong
        if ((param_set_.allowed_to_aggress_) &&  // external control on aggressing
            (my_position_ >=
             param_set_.min_position_to_hit_) &&  // Don't HIT bid when my_position_ is already decently short
            (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ <=
             param_set_.max_int_spread_to_cross_) &&  // Don't HIT ( cross ) when effective spread is to much
            (best_nonself_bid_price_ - target_price_ >=
             current_tradevarset_.l1ask_aggressive_ +
                 param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1)) &&
            ((last_sell_msecs_ <= 0) ||
             (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
             ((last_sell_int_price_ - best_nonself_bid_int_price_) <
              (param_set_.px_band_ -
               1)))  // TODO_OPT : later change setting of last_sell_int_price_ to include px_band_
            )

        {
          last_agg_sell_msecs_ = watch_.msecs_from_midnight();

          if (security_id_ == 0 && niktm_->nkm_bestask_price_ > niktm_->nk_bestask_price_) {
            niktm_->SellNKM();
          } else if (security_id_ == 1 && niktm_->nk_bestask_price_ > niktm_->nkm_bestask_price_) {
            niktm_->SellNK();
          } else {
            top_bid_hit_ = true;

            // when we are already short and bit_hit is set to true,
            // place and keep at top ask are set false so that
            // we cancel top level orders before we place new ones at aggressive prices
            if (my_position_ <= param_set_.min_position_to_cancel_on_hit_) {
              top_ask_place_ = false;
              top_ask_keep_ = false;
            }
          }
        } else {
          // Even of the above condition doesn't allow aggressive order, place aggressive in NKM in exetreme cases
          if (security_id_ == 1 && niktm_->nkm_bestbid_price_ > niktm_->nk_bestask_price_ &&
              watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) {
            //                        std::cout << "Hey, this is special aggres sell case for NKM. nkm_mkt: [ " <<
            //                        niktm_->nkm_bestbid_size_ << " "
            //                            << niktm_->nkm_bestbid_price_ << " * " << niktm_->nkm_bestask_price_ << " " <<
            //                            niktm_->nkm_bestask_size_ << " ] nk_mkt: [ "
            //                            << niktm_->nk_bestbid_size_ << " " << niktm_->nk_bestbid_price_ << " * " <<
            //                            niktm_->nk_bestask_price_ << " "
            //                            << niktm_->nk_bestask_size_ << std::endl;
            top_bid_hit_ = true;
          } else {
            top_bid_hit_ = false;
            if ((param_set_.allowed_to_improve_) &&
                (my_position_ >=
                 param_set_
                     .min_position_to_askimprove_) &&  // Don't improve ask when my_position_ is already decently short
                (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ >= param_set_.min_int_spread_to_improve_) &&
                (best_nonself_ask_price_ - target_price_ - dep_market_view_.min_price_increment() >=
                 current_tradevarset_.l1ask_improve_) &&
                ((last_sell_msecs_ <= 0) ||
                 (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
                 (((last_sell_int_price_ - 1) - best_nonself_ask_int_price_) <
                  (param_set_.px_band_ -
                   1)))  // TODO_OPT : later change setting of last_sell_int_price_ to include px_band_
                ) {
              top_ask_improve_ = true;
            } else {
              top_ask_improve_ = false;
            }
          }
        }
      }
    } else {
      // signal not strog enough to place limit orders at the best ask level
      if ((watch_.msecs_from_midnight() - last_pair_ask_cancel_msecs_ > param_set_.pair_exec_cancel_cooloff_msecs_) &&
          (security_id_ == 0 && niktm_->nk_bestask_price_ >= niktm_->nkm_bestask_price_) &&
          ((best_nonself_ask_price_ - target_price_ + bestask_queue_hysterisis_) >=
           current_tradevarset_.l1ask_keep_ - l1_bias_ - l1_order_bias_ - long_positioning_bias_ +
               param_set_.spread_increase_ *
                   (dep_market_view_.spread_increments() -
                    1))) {  // but with place in line effect enough to keep the live order there
        top_ask_keep_ = true;
      } else {
        top_ask_keep_ = false;
      }
    }
    if ((dep_market_view_.spread_increments() > 1) &&
        ((best_nonself_ask_price_ - target_price_) >=
         current_tradevarset_.l1ask_improve_keep_ - l1_bias_ - l1_order_bias_ - long_positioning_bias_ +
             param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1))) {
      ask_improve_keep_ = true;
    } else {
      ask_improve_keep_ = false;
    }
  }

  OrderPlacingLogic();
}

// After setting top-level directives ...
// get to order placing or canceling part

void NikPricePairBasedAggressiveTrading::OrderPlacingLogic() {
  // Active BID order management
  placed_bids_this_round_ = false;
  canceled_bids_this_round_ = false;
  if (top_ask_lift_) {
    // only place aggressive bid orders when there is no active unconfirmed order at or above best_nonself_bid_price_
    // and no confirmed orders above the best_nonself_bid_price_
    if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_ + 1) == 0) &&
        (order_manager_.SumBidSizeConfirmedAboveIntPrice(best_nonself_bid_int_price_) == 0)) {
      int _canceled_size_ = 0;
      if (!top_bid_keep_) {  // if because of my_position_ being greater than max_position_to_cancel_on_lift_
        // placing aggressive LIFT order requires us to cancel bids from all active levels
        // where an actve level is >= best_nonself_bid_price_
        _canceled_size_ += order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
        if (_canceled_size_ > 0) canceled_bids_this_round_ = true;
      }

      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the bid side
      /*	    if ( my_position_ + order_manager_.SumBidSizes ( ) + current_tradevarset_.l1bid_trade_size_ >=
         param_set_.worst_case_position_ )
          {
          // and if size canceled already is less than l1bid_trade_size_
          if ( my_position_ + order_manager_.SumBidSizes ( ) + current_tradevarset_.l1bid_trade_size_ - _canceled_size_
         > param_set_.worst_case_position_ )
          {
          // then cancel Bids from bottom levels for the required size
          _canceled_size_ += order_manager_.CancelBidsFromFar ( current_tradevarset_.l1bid_trade_size_ );
          }

          // // now total size canceled is more than l1bid_trade_size_
          // // then go ahead and place the aggressive lift order you have been wanting to place
          // if ( _canceled_size_ >= current_tradevarset_.l1bid_trade_size_ )
          //   {
          //     order_manager_.SendTrade ( best_nonself_ask_price_, best_nonself_ask_int_price_,
         current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'A' ) ;
          //     placed_bids_this_round_ = true;
          //     last_agg_buy_msecs_ = watch_.msecs_from_midnight();
          //     if ( dbglogger_.CheckLoggingLevel ( TRADING_ERROR ) )
          //       {
          // 	DBGLOG_TIME_CLASS_FUNC
          // 	  << "Sending aggressive B at px " << best_nonself_ask_price_
          // 	  << " position " << my_position_ << " cancelled size " << _canceled_size_
          // 	  << " high bid sizes "
          // 	  << DBGLOG_ENDL_FLUSH ;
          //       }
          //   }
          }
        */

      int allowance_for_aggressive_buy_ = my_position_ + order_manager_.SumBidSizes() +
                                          current_tradevarset_.l1bid_trade_size_ - param_set_.worst_case_position_;

      if (param_set_.use_new_aggress_logic_) {
        allowance_for_aggressive_buy_ = my_position_ + order_manager_.SumBidSizes() +
                                        current_tradevarset_.l1bid_trade_size_ -
                                        std::max(param_set_.worst_case_position_, param_set_.max_position_);
      }

      if (allowance_for_aggressive_buy_ >= 0) {
        if (allowance_for_aggressive_buy_ > _canceled_size_) {
          _canceled_size_ += order_manager_.CancelBidsFromFar(current_tradevarset_.l1bid_trade_size_);
        }
      }

      else {
        // Place new order
        order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                 current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'A');
        placed_bids_this_round_ = true;
        last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending aggressive B at px " << best_nonself_ask_price_ << " position "
                                 << my_position_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      // only after canceling them can we be allowed to place aggressive orders
      order_manager_.CancelBidsAboveIntPrice(best_nonself_bid_int_price_);
    }
  }

  if ((!placed_bids_this_round_) && (top_bid_improve_)) {
    // only place Bid Improve orders when there is no active unconfirmed order at or above the
    // best_nonself_bid_int_price_
    //    and no confirmed orders above the best price
    if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_ + 1) == 0) &&
        (order_manager_.SumBidSizeConfirmedAboveIntPrice(best_nonself_bid_int_price_) == 0)) {
      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the bid side
      if (my_position_ + order_manager_.SumBidSizes() + current_tradevarset_.l1bid_trade_size_ >=
          param_set_.worst_case_position_) {
        int _canceled_size_ = 0;
        // then cancel Bids from bottom levels for the required size
        _canceled_size_ += order_manager_.CancelBidsFromFar(current_tradevarset_.l1bid_trade_size_);

        // if ( _canceled_size_ >= current_tradevarset_.l1bid_trade_size_ )
        //   {
        //     // if managed to cancel enough then place a new improve order as we wanted
        //     order_manager_.SendTradeIntPx ( (best_nonself_bid_int_price_ + 1),
        //     current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'I' ) ;
        //     placed_bids_this_round_ = true;
        //     last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        //     if ( dbglogger_.CheckLoggingLevel ( TRADING_ERROR ) )
        //       {
        // 	DBGLOG_TIME_CLASS_FUNC
        // 	  << "Sending improve B at px " << best_nonself_bid_int_price_ + 1
        // 	  << " position " << my_position_ << " cancelled size " << _canceled_size_
        // 	  << " high bid sizes "
        // 	  << DBGLOG_ENDL_FLUSH ;
        //       }
        //   }
      } else {
        // Place new order
        order_manager_.SendTradeIntPx((best_nonself_bid_int_price_ + 1), current_tradevarset_.l1bid_trade_size_,
                                      kTradeTypeBuy, 'I');
        placed_bids_this_round_ = true;
        last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending improve B at px " << best_nonself_bid_int_price_ + 1 << " position "
                                 << my_position_ << DBGLOG_ENDL_FLUSH;
        }
      }
    }

  } else {
    if ((dep_market_view_.spread_increments() > 1) && (!bid_improve_keep_)) {
      int cancelled_size_ = order_manager_.CancelBidsAboveIntPrice(best_nonself_bid_int_price_);
      if (cancelled_size_ > 0) {
        canceled_bids_this_round_ = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Cancelled Improve Bid orders Above: " << best_nonself_bid_int_price_
                                 << " Position: " << my_position_ << " ebp_t: "
                                 << (target_price_ - best_nonself_bid_price_ + bestbid_queue_hysterisis_) /
                                        dep_market_view_.min_price_increment() << " thresh_t: "
                                 << current_tradevarset_.l1bid_improve_keep_ / dep_market_view_.min_price_increment()
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (!placed_bids_this_round_) {  // only come here if no aggressive or improve bid orders sent this cycle
    if (top_bid_place_) {
      // only place best level Bid orders when there is no active unconfirmed order at or above
      // best_nonself_bid_int_price_
      // and no confirmed order at or above the best price
      if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) == 0) &&
          (order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) == 0) &&
          (stdev_ <= param_set_.low_stdev_lvl_ ||
           (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_place_)))  // Don't place any new
                                                                                            // orders in inside market
                                                                                            // if the spread is too wide
      {
        order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                 current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'B');
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "SendTrade B of " << current_tradevarset_.l1bid_trade_size_ << " @ "
                                 << best_nonself_bid_price_ << " ebp_t: "
                                 << (target_price_ - best_nonself_bid_price_) / dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_tradevarset_.l1bid_place_ / dep_market_view_.min_price_increment()
                                 << " IntPx: " << best_nonself_bid_int_price_ << " tMktSz: " << best_nonself_bid_size_
                                 << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_
                                 << " StdevSpreadFactor: " << stdev_scaled_capped_in_ticks_ << DBGLOG_ENDL_FLUSH;
        }

        placed_bids_this_round_ = true;
      }
    } else {
      if (!top_bid_keep_) {  // cancel all orders at best_nonself_bid_price_
        int canceled_size_ = order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
        if (canceled_size_ > 0) {
          canceled_bids_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "Canceled B of " << canceled_size_ << " eqAbove " << best_nonself_bid_price_
                                   << " ebp_t: "
                                   << (target_price_ - best_nonself_bid_price_ + bestbid_queue_hysterisis_) /
                                          dep_market_view_.min_price_increment() << " thresh_t: "
                                   << current_tradevarset_.l1bid_keep_ / dep_market_view_.min_price_increment()
                                   << " tMktSz: " << best_nonself_bid_size_ << " mkt: " << best_nonself_bid_size_
                                   << " @ " << best_nonself_bid_price_ << " X " << best_nonself_ask_price_ << " @ "
                                   << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }

  // Active ASK order management
  placed_asks_this_round_ = false;
  canceled_asks_this_round_ = false;
  if (top_bid_hit_) {
    // only place aggressive orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    if ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_ - 1) == 0) &&
        (order_manager_.SumAskSizeConfirmedAboveIntPrice(best_nonself_ask_int_price_) == 0)) {
      int _canceled_size_ = 0;
      if (!top_ask_keep_) {  // if due to position placing aggressive LIFT order requires us to cancel asks from all
                             // active levels
        _canceled_size_ += order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
      }

      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the ask side
      /*	    if ( -my_position_ + order_manager_.SumAskSizes ( ) + current_tradevarset_.l1ask_trade_size_ >=
         param_set_.worst_case_position_ )
          {

          // and if size canceled already is less than l1ask_trade_size_
          if ( -my_position_ + order_manager_.SumAskSizes ( ) + current_tradevarset_.l1ask_trade_size_ - _canceled_size_
         > param_set_.worst_case_position_ )
          {
          _canceled_size_ += order_manager_.CancelAsksFromFar ( current_tradevarset_.l1ask_trade_size_ ); ///< then
         cancel Asks from bottom levels for the required size
          }

          // if ( _canceled_size_ >= current_tradevarset_.l1ask_trade_size_ )
          //   {
          //     order_manager_.SendTrade ( best_nonself_bid_price_, best_nonself_bid_int_price_,
         current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'A' ) ;
          //     placed_asks_this_round_ = true;
          //     last_agg_sell_msecs_ = watch_.msecs_from_midnight();
          //     if ( dbglogger_.CheckLoggingLevel ( TRADING_ERROR ) )
          //       {
          // 	DBGLOG_TIME_CLASS_FUNC
          // 	  << "Sending aggressive S at px " << best_nonself_bid_price_
          // 	  << " position " << my_position_ << " cancelled size " << _canceled_size_
          // 	  << " high ask sizes "
          // 	  << DBGLOG_ENDL_FLUSH ;
          //       }
          //   }
          }
       */
      int allowance_for_aggressive_sell_ = -my_position_ + order_manager_.SumAskSizes() +
                                           current_tradevarset_.l1ask_trade_size_ - param_set_.worst_case_position_;

      if (param_set_.use_new_aggress_logic_) {
        allowance_for_aggressive_sell_ = -my_position_ + order_manager_.SumAskSizes() +
                                         current_tradevarset_.l1ask_trade_size_ -
                                         std::max(param_set_.worst_case_position_, param_set_.max_position_);
      }

      if (allowance_for_aggressive_sell_ >= 0) {
        if (allowance_for_aggressive_sell_ > _canceled_size_) {
          _canceled_size_ += order_manager_.CancelAsksFromFar(
              current_tradevarset_.l1ask_trade_size_);  ///< then cancel Asks from bottom levels for the required size
        }
      } else {
        // Place new order
        order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                 current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'A');
        placed_asks_this_round_ = true;
        last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "Sending aggressive S at px " << best_nonself_bid_price_ << " position "
                                 << my_position_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      order_manager_.CancelAsksAboveIntPrice(
          best_nonself_ask_int_price_);  // only after canceling them can we be allowed to place aggressive orders
    }
  }

  if ((!placed_asks_this_round_) && (top_ask_improve_)) {
    // only place Ask Improve orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    if ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_ - 1) == 0) &&
        (order_manager_.SumAskSizeConfirmedAboveIntPrice(best_nonself_ask_int_price_) == 0)) {
      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the ask side
      if (-my_position_ + order_manager_.SumAskSizes() + current_tradevarset_.l1ask_trade_size_ +
              current_tradevarset_.l1ask_trade_size_ >
          param_set_.worst_case_position_) {
        int _canceled_size_ = 0;
        _canceled_size_ += order_manager_.CancelAsksFromFar(
            current_tradevarset_.l1ask_trade_size_);  ///< then cancel Asks from bottom levels for the required size

        // if ( _canceled_size_ >= current_tradevarset_.l1ask_trade_size_ )
        //   {
        //     order_manager_.SendTradeIntPx ( (best_nonself_ask_int_price_ - 1),
        //     current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'I' ) ;
        //     placed_asks_this_round_ = true;
        //     last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        //     if ( dbglogger_.CheckLoggingLevel ( TRADING_ERROR ) )
        //       {
        // 	DBGLOG_TIME_CLASS_FUNC
        // 	  << "Sending improve S at px " << best_nonself_ask_int_price_ - 1
        // 	  << " position " << my_position_ << " cancelled size " << _canceled_size_
        // 	  << " high ask sizes "
        // 	  << DBGLOG_ENDL_FLUSH ;
        //       }
        //   }
      } else {
        // Place new order
        order_manager_.SendTradeIntPx((best_nonself_ask_int_price_ - 1), current_tradevarset_.l1ask_trade_size_,
                                      kTradeTypeSell, 'I');
        placed_asks_this_round_ = true;
        last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "Sending improve S at px " << best_nonself_ask_int_price_ - 1 << " position "
                                 << my_position_ << DBGLOG_ENDL_FLUSH;
        }
      }
    }

  } else {
    if (dep_market_view_.spread_increments() > 1 && !ask_improve_keep_) {
      int cancelled_size_ = order_manager_.CancelAsksAboveIntPrice(best_nonself_ask_int_price_);
      if (cancelled_size_ > 0) {
        canceled_asks_this_round_ = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Cancelling Improve Ask orders Above: " << best_nonself_ask_int_price_
                                 << " position: " << my_position_ << " eap_t: "
                                 << (best_nonself_ask_price_ - target_price_ + bestask_queue_hysterisis_) /
                                        dep_market_view_.min_price_increment() << " thresh_t: "
                                 << current_tradevarset_.l1ask_improve_keep_ / dep_market_view_.min_price_increment()
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (!placed_asks_this_round_) {
    // get to this location if no aggressive or improve orders placed in this cycle
    if (top_ask_place_) {
      // only place Ask orders when there is no active unconfirmed order at or above best_nonself_ask_int_price_
      //      and no confirmed order at or above the best price
      if ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_) == 0) &&
          (order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) == 0) &&
          (stdev_ <= param_set_.low_stdev_lvl_ ||
           (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_place_)))  // Don't place any new
                                                                                            // orders in inside market
                                                                                            // if the spread is too wide
      {
        order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                 current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'B');
        placed_asks_this_round_ = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "SendTrade S of " << current_tradevarset_.l1ask_trade_size_ << " @ "
                                 << best_nonself_ask_price_ << " eap_t: "
                                 << (best_nonself_ask_price_ - target_price_) / dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_tradevarset_.l1ask_place_ / dep_market_view_.min_price_increment()
                                 << " IntPx: " << best_nonself_ask_int_price_ << " tMktSz: " << best_nonself_ask_size_
                                 << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_
                                 << " StdevSpreadFactor: " << stdev_scaled_capped_in_ticks_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      if (!top_ask_keep_) {
        // cancel all orders at best_nonself_ask_price_
        int canceled_size_ = order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
        if (canceled_size_ > 0) {
          canceled_asks_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "Canceled S of " << canceled_size_ << " EqAbove " << best_nonself_ask_price_
                                   << " eap_t: "
                                   << (best_nonself_ask_price_ - target_price_ + bestask_queue_hysterisis_) /
                                          dep_market_view_.min_price_increment() << " thresh_t: "
                                   << current_tradevarset_.l1ask_keep_ / dep_market_view_.min_price_increment()
                                   << " tMktSz: " << best_nonself_ask_size_ << " mkt: " << best_nonself_bid_size_
                                   << " @ " << best_nonself_bid_price_ << " X " << best_nonself_ask_price_ << " @ "
                                   << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }

  // for zero loging mode have moved to a higher dbg log level
  if ((dbglogger_.CheckLoggingLevel(TRADING_INFO) && (placed_bids_this_round_ || placed_asks_this_round_))) {
    PrintFullStatus();
  }

  // temporarily printing indicators every time
  if ((livetrading_ || dbglogger_.CheckLoggingLevel(TRADING_INFO)) &&
      (placed_bids_this_round_ || placed_asks_this_round_ || canceled_bids_this_round_ || canceled_asks_this_round_)) {
    dump_inds = true;
  }
}

void NikPricePairBasedAggressiveTrading::GetFlatTradingLogic() {
  // only passive execution in getting out
  int t_position_ = my_combined_position_;

  if (((my_position_ > param_set_.max_position_ * param_set_.abs_max_pos_factor_ - param_set_.unit_trade_size_) &&
       t_position_ <= 0) ||
      ((my_position_ < -param_set_.max_position_ * param_set_.abs_max_pos_factor_ + param_set_.unit_trade_size_) &&
       t_position_ > 0)) {
    return;
  }

  if (t_position_ == 0) {  // nothing to be done, cancel all remaining orders
    order_manager_.CancelAllOrders();
  } else if (t_position_ > 0) {  // long hence cancel all bid orders
    order_manager_.CancelAllBidOrders();
    // cancel all non active (best_level) sell orders
    order_manager_.CancelAsksBelowIntPrice(best_nonself_ask_int_price_);

    // if given agg closeout time
    // and past closeout time
    // if spread is 1 tick, then cancel all passive ask orders and place the agg sell order of size min ( position,
    // unit_trade_size_ ) at bidpx
    // else if given agg_closeout_max_size_
    // and bidsz < agg_closeout_max_size_
    // place agg sell order of size min ( position, unit_trade_size_ ) at bidpx

    // checking for stable markets
    if ((dep_market_view_.ask_int_price(1) - dep_market_view_.ask_int_price(0) <=
         param_set_.max_int_level_diff_to_place_)) {
      // ideal size to place ask for is what the position is, except limited to unit_trade_size_ ( for sanity and market
      // effect reasons )
      int trade_size_required_ = MathUtils::GetFlooredMultipleOf(std::min(t_position_, param_set_.unit_trade_size_),
                                                                 dep_market_view_.min_order_size());
      int t_size_ordered_ = order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) +
                            order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_);
      if (-trade_size_required_ + t_position_ < -param_set_.max_position_) {
        return;
      }

      if (t_size_ordered_ < trade_size_required_) {  // if the required size is not present then add more
        order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                 trade_size_required_ - t_size_ordered_, kTradeTypeSell, 'B');

        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "GetFlatSendTrade S of " << trade_size_required_ - t_size_ordered_ << " @ "
                                 << best_nonself_ask_price_ << " IntPx: " << best_nonself_ask_int_price_
                                 << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
        }
      }
      // not doing anything if the placed size is more than required
    }
  } else {  // my_position_ < 0
    // short hence cancel all sell orders
    order_manager_.CancelAllAskOrders();
    // cancel all non bestlevel bid orders
    order_manager_.CancelBidsBelowIntPrice(best_nonself_bid_int_price_);

    // if given agg closeout time
    // and past closeout time
    // if spread is 1 tick, then cancel all passive ask orders and place the agg sell order of size min ( position,
    // unit_trade_size_ ) at bidpx
    // else if given agg_closeout_max_size_
    // and bidsz < agg_closeout_max_size_
    // place agg sell order of size min ( position, unit_trade_size_ ) at bidpx

    // checking for stable markets
    if ((dep_market_view_.bid_int_price(0) - dep_market_view_.bid_int_price(1) <=
         param_set_.max_int_level_diff_to_place_)) {
      // ideal size to place ask for is what the position is, except limited to unit_trade_size_ ( for sanity and market
      // effect reasons )
      int trade_size_required_ = MathUtils::GetFlooredMultipleOf(std::min(-t_position_, param_set_.unit_trade_size_),
                                                                 dep_market_view_.min_order_size());
      int t_size_ordered_ = order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) +
                            order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_);

      if (trade_size_required_ + t_position_ > param_set_.max_position_) {
        return;
      }

      if (t_size_ordered_ < trade_size_required_) {  // if the required size is not present then add more
        order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                 trade_size_required_ - t_size_ordered_, kTradeTypeBuy, 'B');

        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "GetFlatSendTrade B of " << trade_size_required_ - t_size_ordered_ << " @ "
                                 << best_nonself_bid_price_ << " IntPx: " << best_nonself_bid_int_price_
                                 << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
        }
      }
      // not doing anything if the placed size is more than required
    }
  }
}

void NikPricePairBasedAggressiveTrading::PrintFullStatus() {
  DBGLOG_TIME << "tgt: " << target_price_ << " mkt " << best_nonself_bid_price_ << " X " << best_nonself_bid_size_
              << " pft: " << (target_price_ - best_nonself_bid_price_) << " -- " << best_nonself_ask_price_ << " X "
              << best_nonself_ask_size_ << " pft: " << (best_nonself_ask_price_ - target_price_) << " bias: "
              << ((target_price_ - dep_market_view_.mkt_size_weighted_price()) / dep_market_view_.min_price_increment())
              << " pos: " << my_position_ << " smaps "
              << order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) << " "
              << order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) << " "
              << order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) << " "
              << order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_)
              << " opnl: " << order_manager_.base_pnl().opentrade_unrealized_pnl() << " gpos: " << my_global_position_
              << DBGLOG_ENDL_FLUSH;
}
}
