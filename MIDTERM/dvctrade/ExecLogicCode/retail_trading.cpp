/**
   \file ExecLogicCode/retail_trading.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "dvctrade/ExecLogic/retail_trading.hpp"

namespace HFSAT {

RetailTrading::RetailTrading(DebugLogger& _dbglogger_, const Watch& _watch_,
                             const SecurityMarketView& _dep_market_view_, SmartOrderManager& _order_manager_,
                             const std::string& _paramfilename_, const bool _livetrading_,
                             MulticastSenderSocket* _p_strategy_param_sender_socket_,
                             EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                             const int t_trading_end_utc_mfm_, const int t_runtime_id_,
                             const std::vector<std::string> _this_model_source_shortcode_vec_)
    : ExecInterface(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_),
      security_id_(_dep_market_view_.security_id()),
      last_retail_offer_(),
      retail_offer_listeners_(),
      avg_stdev_(PcaWeightsManager::GetUniqueInstance().GetShortcodeStdevs(dep_market_view_.shortcode())),
      bom_position_(0),
      fp_position_(0),
      my_position_(0),
      my_global_position_(0),
      p_prom_order_manager_(PromOrderManager::GetCreatedInstance(dep_market_view_.shortcode())),
      last_retail_update_msecs_(0),
      last_full_logging_msecs_(0),
      is_ready_(false),
      trading_start_utc_mfm_(t_trading_start_utc_mfm_),
      trading_end_utc_mfm_(t_trading_end_utc_mfm_),
      runtime_id_(t_runtime_id_),
      should_be_getting_flat_(false),
      start_not_given_(livetrading_),
      getflat_due_to_external_getflat_(livetrading_),
      getflat_due_to_close_(false),
      getflat_due_to_max_loss_(false),
      getflat_due_to_economic_times_(false),
      getflat_due_to_market_data_interrupt_(false),
      getflat_due_to_allowed_economic_event_(false),
      last_allowed_event_index_(0u),
      control_reply_struct_(),
      economic_events_manager_(t_economic_events_manager_),
      ezone_vec_(),
      severity_to_getflat_on_base_(1.00),
      severity_to_getflat_on_(1.00),
      severity_change_end_msecs_(t_trading_end_utc_mfm_),
      applicable_severity_(0.00),
      allowed_events_present_(false),
      getflat_retail_update_type_(HFSAT::CDef::knormal),
      last_fp_buy_order_msecs_(0),
      last_fp_sell_order_msecs_(0),
      target_price_(0),
      targetbias_numbers_(0),
      param_index_to_use_(param_set_vec_.size() - 1) {
  dep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  dep_market_view_.subscribe_price_type(this, kPriceTypeOrderWPrice);
  watch_.subscribe_BigTimePeriod(this);

  memcpy(control_reply_struct_.symbol_, dep_market_view_.secname(), kSecNameLen);
  control_reply_struct_.trader_id_ = runtime_id_;

  // economic events initializations
  economic_events_manager_.AdjustSeverity(dep_market_view_.shortcode(), dep_market_view_.exch_source());
  economic_events_manager_.AllowEconomicEventsFromList(dep_market_view_.shortcode());
  allowed_events_present_ = economic_events_manager_.AllowedEventsPresent();

  GetEZVecForShortcode(dep_market_view_.shortcode(), t_trading_start_utc_mfm_, ezone_vec_);
  DBGLOG_CLASS_FUNC << "After checking at mfm " << t_trading_start_utc_mfm_ << " Stopping for EZones:";
  for (auto i = 0u; i < ezone_vec_.size(); i++) {
    dbglogger_ << ' ' << GetStrFromEconomicZone(ezone_vec_[i]);
  }
  dbglogger_ << DBGLOG_ENDL_FLUSH;

  //( t_trading_end_utc_mfm_ - 1 ) to avoid events overlapping period for EU and US
  GetEZVecForShortcode(dep_market_view_.shortcode(), t_trading_end_utc_mfm_ - 1, ezone_vec_);
  DBGLOG_CLASS_FUNC << "After checking at mfm " << (t_trading_end_utc_mfm_ - 1) << " Stopping for EZones:";
  for (auto i = 0u; i < ezone_vec_.size(); i++) {
    dbglogger_ << ' ' << GetStrFromEconomicZone(ezone_vec_[i]);
  }
  dbglogger_ << DBGLOG_ENDL_FLUSH;

  if (p_prom_order_manager_) {
    p_prom_order_manager_->ManageOrdersAlso();
  }

  // paramset initializations
  CheckParamSet();
  ReconcileParams();

  bidsize_to_show_maxpos_limit = param_set_.max_position_;
  asksize_to_show_maxpos_limit = param_set_.max_position_;
  bidsize_to_show_global_maxpos_limit = param_set_.max_global_position_;
  asksize_to_show_global_maxpos_limit = param_set_.max_global_position_;

  SlowStdevCalculator* t_stdev_ =
      SlowStdevCalculator::GetUniqueInstance(_dbglogger_, _watch_, _dep_market_view_.shortcode(), 100u * 1000u);
  t_stdev_->AddSlowStdevCalculatorListener(this);

  // just to prevent comparisons with non-initialized values, may not be needed
  last_retail_offer_.retail_update_type_ = HFSAT::CDef::knormal;
  last_retail_offer_.offered_bid_price_ = MIN_INVALID_PRICE;
  last_retail_offer_.offered_ask_price_ = MAX_INVALID_PRICE;
  last_retail_offer_.offered_bid_size_ = 0;
  last_retail_offer_.offered_ask_size_ = 0;

  p_exp_mov_fok_size_ =
      new ExpMovingSumGeneric(10 * 60 * 1000);  // this will keep exp moving sum of fok_size in last 10 min

  order_manager_.RemoveExecutionLister(&(order_manager_.base_pnl()));
  DBGLOG_CLASS_FUNC << "Removing BasePnl from ExecutionListeners of OM for " << dep_market_view_.shortcode()
                    << DBGLOG_ENDL_FLUSH;
}

void RetailTrading::NotifyListeners(const HFSAT::CDef::RetailOffer& _retail_offer_) {
  if (((last_retail_update_msecs_ <= 0) ||
       (watch_.msecs_from_midnight() - last_retail_update_msecs_ >= RETAIL_UPDATE_TIMEOUT_MSECS)) ||
      (last_retail_offer_ != _retail_offer_)  // offer has changed
      ) {
    for (auto i = 0u; i < this->retail_offer_listeners_.size(); i++) {
      retail_offer_listeners_[i]->OnRetailOfferUpdate(dep_market_view_.security_id(), dep_market_view_.shortcode(),
                                                      dep_market_view_.secname(),
                                                      order_manager_.server_assigned_client_id_, _retail_offer_);
    }
    last_retail_offer_ = _retail_offer_;
    last_retail_update_msecs_ = watch_.msecs_from_midnight();

    if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
      LogFullStatus();
    }
  }
}

void RetailTrading::SetOfferVars() {
  HFSAT::CDef::RetailOffer this_retail_offer_;
  this_retail_offer_.retail_update_type_ = HFSAT::CDef::knormal;
  this_retail_offer_.offered_bid_price_ = MIN_INVALID_PRICE;
  this_retail_offer_.offered_ask_price_ = MAX_INVALID_PRICE;
  this_retail_offer_.offered_bid_size_ = 0;
  this_retail_offer_.offered_ask_size_ = 0;

  // It is a one leg contract. 
  this_retail_offer_.product_split_.product_type_ = HFSAT::CDef::normal;
  strcpy(this_retail_offer_.product_split_.sub_product_bid_[0].shortcode_, dep_market_view_.shortcode().c_str());
  strcpy(this_retail_offer_.product_split_.sub_product_bid_[0].security_name_, dep_market_view_.secname());
  this_retail_offer_.product_split_.sub_product_bid_[0].product_price_ = MAX_INVALID_PRICE;
  this_retail_offer_.product_split_.sub_product_bid_[0].product_size_ = 0;
  this_retail_offer_.product_split_.sub_product_bid_[0].buysell_ = kTradeTypeSell;

  strcpy(this_retail_offer_.product_split_.sub_product_ask_[0].shortcode_, dep_market_view_.shortcode().c_str());
  strcpy(this_retail_offer_.product_split_.sub_product_ask_[0].security_name_, dep_market_view_.secname());
  this_retail_offer_.product_split_.sub_product_ask_[0].product_price_ = MAX_INVALID_PRICE;
  this_retail_offer_.product_split_.sub_product_ask_[0].product_size_ = 0;
  this_retail_offer_.product_split_.sub_product_ask_[0].buysell_ = kTradeTypeBuy;


  if (should_be_getting_flat_) {
    // control wont come out of this if block without returning
    // send invalid prices in getflat situations
    this_retail_offer_.retail_update_type_ = getflat_retail_update_type_;
    NotifyListeners(this_retail_offer_);
    return;
  }

  int t_bid_size_to_offer_ = 0;
  double t_mkt_bidbias_ = dep_market_view_.mkt_size_weighted_price() - dep_market_view_.bestbid_price();
  double t_owp_bidbias_ = dep_market_view_.order_weighted_price() - dep_market_view_.bestbid_price();
  bool last_bidoffer_was_at_same_price_ =
      MathUtils::DblPxCompare(last_retail_offer_.offered_bid_price_, dep_market_view_.bestbid_price(),
                              dep_market_view_.min_price_increment() / 2.0);
  if ((t_mkt_bidbias_ > param_set_.zeropos_place_ && t_owp_bidbias_ > param_set_.owp_retail_offer_thresh_) ||
      (last_bidoffer_was_at_same_price_ &&
       t_mkt_bidbias_ > (1 - param_set_.retail_price_threshold_tolerance_) * param_set_.zeropos_place_ &&
       t_owp_bidbias_ > (1 - param_set_.retail_price_threshold_tolerance_) *
                            param_set_.owp_retail_offer_thresh_)  // to reduce rapid flunctuations
      ) {
    t_bid_size_to_offer_ = std::min(
        std::min(bidsize_to_show_maxpos_limit, bidsize_to_show_global_maxpos_limit),
        std::max(0, int(param_set_.retail_size_factor_to_offer_ * dep_market_view_.bestbid_size()) - my_position_));
    t_bid_size_to_offer_ = MathUtils::GetFlooredMultipleOf(t_bid_size_to_offer_, dep_market_view_.min_order_size());

    if (t_bid_size_to_offer_ > 0 && last_bidoffer_was_at_same_price_ &&
        abs(last_retail_offer_.offered_bid_size_ - t_bid_size_to_offer_) <
            param_set_.retail_size_tolerance_ * last_retail_offer_.offered_bid_size_) {
      // again to avoid rapid fluctuations
      t_bid_size_to_offer_ = last_retail_offer_.offered_bid_size_;
    }
  }

  int t_ask_size_to_offer_ = 0;
  double t_mkt_askbias_ = dep_market_view_.bestask_price() - dep_market_view_.mkt_size_weighted_price();
  double t_owp_askbias_ = dep_market_view_.bestask_price() - dep_market_view_.order_weighted_price();
  bool last_askoffer_was_at_same_price_ =
      MathUtils::DblPxCompare(last_retail_offer_.offered_ask_price_, dep_market_view_.bestask_price(),
                              dep_market_view_.min_price_increment() / 2.0);
  if ((t_mkt_askbias_ > param_set_.zeropos_place_ && t_owp_askbias_ > param_set_.owp_retail_offer_thresh_) ||
      (last_askoffer_was_at_same_price_ &&
       t_mkt_askbias_ > (1 - param_set_.retail_price_threshold_tolerance_) * param_set_.zeropos_place_ &&
       t_owp_askbias_ > (1 - param_set_.retail_price_threshold_tolerance_) *
                            param_set_.owp_retail_offer_thresh_)  // to reduce rapid flunctuations
      ) {
    t_ask_size_to_offer_ = std::min(
        std::min(asksize_to_show_maxpos_limit, asksize_to_show_global_maxpos_limit),
        std::max(0, int(param_set_.retail_size_factor_to_offer_ * dep_market_view_.bestask_size()) + my_position_));
    t_ask_size_to_offer_ = MathUtils::GetFlooredMultipleOf(t_ask_size_to_offer_, dep_market_view_.min_order_size());

    if (t_ask_size_to_offer_ > 0 && last_askoffer_was_at_same_price_ &&
        abs(last_retail_offer_.offered_ask_size_ - t_ask_size_to_offer_) <
            param_set_.retail_size_tolerance_ * last_retail_offer_.offered_ask_size_) {
      // again to avoid rapid fluctuations
      t_ask_size_to_offer_ = last_retail_offer_.offered_ask_size_;
    }
  }

  if (t_bid_size_to_offer_ <= 0 && t_ask_size_to_offer_ <= 0) {
    this_retail_offer_.retail_update_type_ = HFSAT::CDef::kSubBestBidAsk;
  } else if (t_bid_size_to_offer_ <= 0) {
    this_retail_offer_.retail_update_type_ = HFSAT::CDef::kSubBestBid;
  } else if (t_ask_size_to_offer_ <= 0) {
    this_retail_offer_.retail_update_type_ = HFSAT::CDef::kSubBestAsk;
  }

  if (t_bid_size_to_offer_ > 0) {
    this_retail_offer_.offered_bid_size_ = t_bid_size_to_offer_;
    this_retail_offer_.offered_bid_price_ = dep_market_view_.bestbid_price();
  } else {
    // Offering min size at second level
    this_retail_offer_.offered_bid_size_ = dep_market_view_.min_order_size();
    this_retail_offer_.offered_bid_price_ = dep_market_view_.bid_price(1);
  }

  if (t_ask_size_to_offer_ > 0) {
    this_retail_offer_.offered_ask_size_ = t_ask_size_to_offer_;
    this_retail_offer_.offered_ask_price_ = dep_market_view_.bestask_price();
  } else {
    // Offering min size at second level
    this_retail_offer_.offered_ask_size_ = dep_market_view_.min_order_size();
    this_retail_offer_.offered_ask_price_ = dep_market_view_.ask_price(1);
  }

  this_retail_offer_.product_split_.sub_product_bid_[0].product_price_ = this_retail_offer_.offered_bid_price_;
  this_retail_offer_.product_split_.sub_product_bid_[0].product_size_ = this_retail_offer_.offered_bid_size_;
  this_retail_offer_.product_split_.sub_product_bid_[0].buysell_ = kTradeTypeBuy;

  this_retail_offer_.product_split_.sub_product_ask_[0].product_price_ = this_retail_offer_.offered_ask_price_;
  this_retail_offer_.product_split_.sub_product_ask_[0].product_size_ = this_retail_offer_.offered_ask_size_;
  this_retail_offer_.product_split_.sub_product_ask_[0].buysell_ = kTradeTypeSell;

  NotifyListeners(this_retail_offer_);
}

void RetailTrading::TradingLogic() {
  if (my_position_ == 0) {  // nothing to be done, cancel all remaining orders
    order_manager_.CancelAllOrders();
  } else {
    if (my_position_ > 0) {
      // long hence cancel all bid orders
      order_manager_.CancelAllBidOrders();

      bool done_for_this_round_ = false;
      if (param_set_.retail_place_fok_) {
        done_for_this_round_ = TrySendingFOKOrders();
      }

      // using this instead of mkt_price to avoid problems when spd>1
      double t_ask_placing_bias_ =
          dep_market_view_.bestask_size() / double(dep_market_view_.bestbid_size() + dep_market_view_.bestask_size());

      // agg after proper checks
      if (!done_for_this_round_ && param_set_.allowed_to_aggress_ &&
          (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_cross_) &&
          (dep_market_view_.bestbid_size() <
           param_set_.max_size_to_aggress_) &&            // if not too high size available on bid size
          (t_ask_placing_bias_ > param_set_.aggressive_)  // market is going away towards bid
          ) {
        int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(std::min(my_position_, param_set_.unit_trade_size_),
                                                                   dep_market_view_.min_order_size());

        // size already agressed
        int t_size_already_placed_ =
            order_manager_.SumAskSizeConfirmedEqAboveIntPrice(dep_market_view_.bestbid_int_price()) +
            order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(dep_market_view_.bestbid_int_price());

        if (trade_size_to_place_ > t_size_already_placed_) {
          SendTradeAndLog(dep_market_view_.bestbid_int_price(), trade_size_to_place_ - t_size_already_placed_,
                          kTradeTypeSell, 'A');
          done_for_this_round_ = true;
        }
      }

      if (!done_for_this_round_ && param_set_.allowed_to_improve_ &&
          dep_market_view_.spread_increments() >= param_set_.min_int_spread_to_improve_ &&
          t_ask_placing_bias_ > param_set_.improve_) {
        int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(std::min(my_position_, param_set_.unit_trade_size_),
                                                                   dep_market_view_.min_order_size());

        // size already improved/aggressed
        int t_size_already_placed_ =
            order_manager_.SumAskSizeConfirmedEqAboveIntPrice(dep_market_view_.bestask_int_price() - 1) +
            order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(dep_market_view_.bestask_int_price() - 1);

        if (trade_size_to_place_ > t_size_already_placed_) {
          SendTradeAndLog(dep_market_view_.bestask_int_price() - 1, trade_size_to_place_ - t_size_already_placed_,
                          kTradeTypeSell, 'I');
          done_for_this_round_ = true;
        }
      }

      if (!done_for_this_round_) {
        // we should place at best, limit the size by some %age of mkt_size to not effect the market, by default 50%
        int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(
            std::min(double(std::min(my_position_, param_set_.unit_trade_size_)),
                     param_set_.retail_size_factor_to_place_ * dep_market_view_.bestask_size()),
            dep_market_view_.min_order_size());

        // size already placed at best or above
        int t_size_already_placed_ =
            order_manager_.SumAskSizeConfirmedEqAboveIntPrice(dep_market_view_.bestask_int_price()) +
            order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(dep_market_view_.bestask_int_price());

        if (trade_size_to_place_ > t_size_already_placed_) {  // if the required size is not present then add more
          SendTradeAndLog(dep_market_view_.bestask_int_price(), trade_size_to_place_ - t_size_already_placed_,
                          kTradeTypeSell, 'B');
          done_for_this_round_ = true;
        }
      }
      // cancelling any extra orders to avoid overfill
      order_manager_.KeepAskSizeInPriceRange(my_position_);
    } else {  // my_position_ < 0

      // short hence cancel all sell orders
      order_manager_.CancelAllAskOrders();
      bool done_for_this_round_ = false;
      if (param_set_.retail_place_fok_) {
        done_for_this_round_ = TrySendingFOKOrders();
      }

      // using this instead of mkt_price to avoid problems when spd>1
      double t_bid_placing_bias_ =
          dep_market_view_.bestbid_size() / double(dep_market_view_.bestbid_size() + dep_market_view_.bestask_size());

      // agg after proper checks
      if (!done_for_this_round_ && param_set_.allowed_to_aggress_ &&
          (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_cross_) &&
          (dep_market_view_.bestask_size() <
           param_set_.max_size_to_aggress_) &&            // if not too high size available on ask size
          (t_bid_placing_bias_ > param_set_.aggressive_)  // market is going away towards ask
          ) {
        int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(std::min(-my_position_, param_set_.unit_trade_size_),
                                                                   dep_market_view_.min_order_size());

        // size already agressed
        int t_size_already_placed_ =
            order_manager_.SumBidSizeConfirmedEqAboveIntPrice(dep_market_view_.bestask_int_price()) +
            order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(dep_market_view_.bestask_int_price());

        if (trade_size_to_place_ > t_size_already_placed_) {
          SendTradeAndLog(dep_market_view_.bestask_int_price(), trade_size_to_place_ - t_size_already_placed_,
                          kTradeTypeBuy, 'A');
          done_for_this_round_ = true;
        }
      }

      if (!done_for_this_round_ && param_set_.allowed_to_improve_ &&
          (dep_market_view_.spread_increments() >= param_set_.min_int_spread_to_improve_) &&
          (t_bid_placing_bias_ > param_set_.improve_)  // market is going away towards ask
          ) {
        int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(std::min(-my_position_, param_set_.unit_trade_size_),
                                                                   dep_market_view_.min_order_size());

        // size already agressed
        int t_size_already_placed_ =
            order_manager_.SumBidSizeConfirmedEqAboveIntPrice(dep_market_view_.bestbid_int_price() + 1) +
            order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(dep_market_view_.bestbid_int_price() + 1);

        if (trade_size_to_place_ > t_size_already_placed_) {
          SendTradeAndLog(dep_market_view_.bestbid_int_price() + 1, trade_size_to_place_ - t_size_already_placed_,
                          kTradeTypeBuy, 'I');
          done_for_this_round_ = true;
        }
      }

      if (!done_for_this_round_) {
        // we should place at best, limit the size by some %age of mkt_size to not effect the market, by default 50%
        int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(
            std::min(double(std::min(-my_position_, param_set_.unit_trade_size_)),
                     param_set_.retail_size_factor_to_place_ * dep_market_view_.bestbid_size()),
            dep_market_view_.min_order_size());

        // size already placed at best or above
        int t_size_already_placed_ =
            order_manager_.SumBidSizeConfirmedEqAboveIntPrice(dep_market_view_.bestbid_int_price()) +
            order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(dep_market_view_.bestbid_int_price());

        if (trade_size_to_place_ > t_size_already_placed_) {  // if the required size is not present then add more
          SendTradeAndLog(dep_market_view_.bestbid_int_price(), trade_size_to_place_ - t_size_already_placed_,
                          kTradeTypeBuy, 'B');
          done_for_this_round_ = true;
        }
      }
      // cancelling any extra orders to avoid overfill
      order_manager_.KeepBidSizeInPriceRange(-my_position_);
    }
  }
}

bool RetailTrading::TrySendingFOKOrders() {
  if (my_position_ == 0 || !livetrading_) {
    return false;
  }

  bool order_send_ = false;

  if (my_position_ > 0) {
    int bestbidsize_placed_ = 0;

    if (p_prom_order_manager_ != NULL) {
      bestbidsize_placed_ = p_prom_order_manager_->GetBidSizePlacedAboveEqIntPx(dep_market_view_.bestbid_int_price(),
                                                                                dep_market_view_.bestask_int_price());
    } else {
      DBGLOG_TIME_CLASS_FUNC << "p_prom_order_manager_==NULL, can't place FOK orders" << DBGLOG_ENDL_FLUSH;
    }

    if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "bestbidsize_placed_: " << bestbidsize_placed_ << DBGLOG_ENDL_FLUSH;
    }

    if (bestbidsize_placed_ > 0) {
      // not capping trade_size_to_place_ by uts for FOK
      int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(my_position_, dep_market_view_.min_order_size());

      // size already agressed
      int t_size_already_placed_ =
          order_manager_.SumAskSizeConfirmedEqAboveIntPrice(dep_market_view_.bestbid_int_price()) +
          order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(dep_market_view_.bestbid_int_price());

      if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "trade_size_to_place_: " << trade_size_to_place_
                               << " t_size_already_placed_: " << t_size_already_placed_ << DBGLOG_ENDL_FLUSH;
      }

      if (trade_size_to_place_ > t_size_already_placed_) {
        int t_size_ = std::min(bestbidsize_placed_, trade_size_to_place_ - t_size_already_placed_);
        order_manager_.SendTrade(dep_market_view_.bestbid_price(), dep_market_view_.bestbid_int_price(), t_size_,
                                 kTradeTypeSell, 'A', true);
        order_send_ = true;

        if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "FOKSendTrade S of " << t_size_ << " @ " << dep_market_view_.bestbid_price()
                                 << " IntPx: " << dep_market_view_.bestbid_int_price()
                                 << " mkt: " << dep_market_view_.bestbid_size() << " @ "
                                 << dep_market_view_.bestbid_price() << " X " << dep_market_view_.bestask_price()
                                 << " @ " << dep_market_view_.bestask_size()
                                 << " bestbidsize_placed_: " << bestbidsize_placed_ << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  } else {
    int bestasksize_placed_ = 0;

    if (p_prom_order_manager_ != NULL) {
      bestasksize_placed_ = p_prom_order_manager_->GetAskSizePlacedAboveEqIntPx(dep_market_view_.bestask_int_price(),
                                                                                dep_market_view_.bestbid_int_price());
    } else {
      DBGLOG_TIME_CLASS_FUNC << "p_prom_order_manager_==NULL, can't place FOK orders" << DBGLOG_ENDL_FLUSH;
    }

    if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "bestasksize_placed_:" << bestasksize_placed_ << DBGLOG_ENDL_FLUSH;
    }

    if (bestasksize_placed_ > 0) {
      // not capping trade_size_to_place_ by uts for FOK
      int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(-my_position_, dep_market_view_.min_order_size());

      // size already agressed
      int t_size_already_placed_ =
          order_manager_.SumBidSizeConfirmedEqAboveIntPrice(dep_market_view_.bestask_int_price()) +
          order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(dep_market_view_.bestask_int_price());

      if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "trade_size_to_place_: " << trade_size_to_place_
                               << " t_size_already_placed_: " << t_size_already_placed_ << DBGLOG_ENDL_FLUSH;
      }

      if (trade_size_to_place_ > t_size_already_placed_) {
        int t_size_ = std::min(bestasksize_placed_, trade_size_to_place_ - t_size_already_placed_);
        order_manager_.SendTrade(dep_market_view_.bestask_price(), dep_market_view_.bestask_int_price(), t_size_,
                                 kTradeTypeBuy, 'A', true);

        order_send_ = true;

        if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "FOKSendTrade B of " << t_size_ << " @ " << dep_market_view_.bestask_price()
                                 << " IntPx: " << dep_market_view_.bestask_int_price()
                                 << " mkt: " << dep_market_view_.bestbid_size() << " @ "
                                 << dep_market_view_.bestbid_price() << " X " << dep_market_view_.bestask_price()
                                 << " @ " << dep_market_view_.bestask_size()
                                 << " bestasksize_placed_: " << bestasksize_placed_ << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  return order_send_;
}

void RetailTrading::OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) {
  if (_security_id_ == security_id_) {
    my_global_position_ = _new_global_position_;
  }

  AdjustGlobalMaxPosLimitOfferSizes();
  // not calling setoffervars here to save some calls, it would get called on next market update
}

bool RetailTrading::UpdateTarget(double _target_price_, double _targetbias_numbers_, int _modelmath_index_) {
  if (!is_ready_) {  // checking whether we are post start time
    // if dependant is ready
    if (watch_.msecs_from_midnight() > trading_start_utc_mfm_ && dep_market_view_.is_ready() &&
        (_target_price_ >= dep_market_view_.bestbid_price() && _target_price_ <= dep_market_view_.bestask_price())) {
      // Now it's the right time to get CPU
      if (livetrading_) {
        AllocateCPU();
      }

      is_ready_ = true;
      DBGLOG_TIME_CLASS_FUNC << "SACI: " << order_manager_.server_assigned_client_id_
                             << " got ready! shc: " << dep_market_view_.shortcode() << " queryid: " << runtime_id_
                             << DBGLOG_ENDL_FLUSH;
      p_base_model_math_->ShowIndicatorValues();
    }
  } else {  // is_ready amd tradevars are already set
    target_price_ = _target_price_;
    targetbias_numbers_ = _targetbias_numbers_;
    ProcessGetFlat();  // can avoid this , check and set all getflats in OnTimePeriodUpdate
    SetOfferVars();
    TradingLogic();
  }
  return false;
}

void RetailTrading::LogFullStatus() {
  if ((last_full_logging_msecs_ == 0) || (watch_.msecs_from_midnight() - last_full_logging_msecs_ > 5000)) {
    last_full_logging_msecs_ = watch_.msecs_from_midnight();
    PrintFullStatus();
  }
}

void RetailTrading::PrintFullStatus() {
  DBGLOG_TIME_CLASS_FUNC << dep_market_view_.shortcode() << " Retail Sending: " << last_retail_offer_.ToString()
                         << "\nmkt: [ " << dep_market_view_.bestbid_size() << " " << dep_market_view_.bestbid_price()
                         << " * " << dep_market_view_.bestask_price() << " " << dep_market_view_.bestask_size() << " ] "
                         << " bept_: "
                         << (dep_market_view_.mkt_size_weighted_price() - dep_market_view_.bestbid_price())
                         << " aept_: "
                         << (dep_market_view_.bestask_price() - dep_market_view_.mkt_size_weighted_price())
                         << " thresh_: " << param_set_.zeropos_place_ << " " << param_set_.aggressive_
                         << " blimits: " << bidsize_to_show_maxpos_limit << " " << bidsize_to_show_global_maxpos_limit
                         << " alimits: " << asksize_to_show_maxpos_limit << " " << asksize_to_show_global_maxpos_limit
                         << DBGLOG_ENDL_FLUSH;
}

void RetailTrading::OnControlUpdate(const ControlMessage& _control_message_, const char* symbol, const int trader_id) {
  switch (_control_message_.message_code_) {
    case kControlMessageCodeGetflat: {
      if (!getflat_due_to_external_getflat_) {
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME << "getflat_due_to_external_getflat_ " << runtime_id_ << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
        getflat_due_to_external_getflat_ = true;
      }
    } break;
    case kControlMessageCodeStartTrading: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "StartTrading Called " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      getflat_due_to_external_getflat_ = false;
      start_not_given_ = false;
    } break;
    case kControlMessageCodeSetUnitTradeSize: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (_control_message_.intval_1_ > param_set_vec_[i].unit_trade_size_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < param_set_vec_[i].unit_trade_size_ * FAT_FINGER_FACTOR) {
          param_set_vec_[i].unit_trade_size_ = std::max(1, _control_message_.intval_1_);
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetUnitTradeSize " << runtime_id_ << " called with " << _control_message_.intval_1_
                            << " and UnitTradeSize for param " << i << " set to " << param_set_vec_[i].unit_trade_size_
                            << "" << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }

      param_set_.unit_trade_size_ = param_set_vec_[param_index_to_use_].unit_trade_size_;
    } break;
    case kControlMessageCodeAddPosition: {
      DBGLOG_TIME_CLASS << "kControlMessageCodeAddPosition called for " << dep_market_view_.shortcode() << " with "
                        << _control_message_.intval_1_ << DBGLOG_ENDL_FLUSH;
      if (abs(my_position_ + _control_message_.intval_1_) > param_set_.max_position_) {
        // just for sanity
        DBGLOG_TIME_CLASS << "Failed to add position. Attempt to add position " << _control_message_.intval_1_
                          << " > Max position " << param_set_.max_position_ << DBGLOG_ENDL_FLUSH;
      } else if (!dep_market_view_.is_ready()) {
        DBGLOG_TIME_CLASS << "Failed to add position. SMV not ready for " << dep_market_view_.shortcode()
                          << DBGLOG_ENDL_FLUSH;
      } else {
        int t_position_offset_ =
            MathUtils::GetFlooredMultipleOf(_control_message_.intval_1_, dep_market_view_.min_order_size());
        if (t_position_offset_ != 0) {
          DBGLOG_TIME_CLASS << "Adding position " << t_position_offset_ << DBGLOG_ENDL_FLUSH;
          TradeType_t t_buysell_ = t_position_offset_ > 0 ? kTradeTypeBuy : kTradeTypeSell;
          // assuming a trade at best level
          double t_exec_price_ =
              t_position_offset_ > 0 ? dep_market_view_.bestbid_price() : dep_market_view_.bestask_price();
          // simulate a GUI Trade
          FPOrderExecuted(dep_market_view_.secname(), t_exec_price_, t_buysell_, abs(t_position_offset_));
        }
      }
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    } break;
    case kControlMessageCodeSetMaxUnitRatio: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (int(_control_message_.doubleval_1_ * param_set_vec_[i].unit_trade_size_ + 0.5) >
                param_set_vec_[i].max_position_ / FAT_FINGER_FACTOR &&
            int(_control_message_.doubleval_1_ * param_set_vec_[i].unit_trade_size_ + 0.5) <
                param_set_vec_[i].max_position_ * FAT_FINGER_FACTOR) {
          param_set_vec_[i].max_position_ =
              (int)(std::max(1.0, _control_message_.doubleval_1_) * param_set_vec_[i].unit_trade_size_ + 0.5);
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxUnitRatio " << runtime_id_ << " called with " << _control_message_.doubleval_1_
                            << " and MaxPosition for param " << i << " set to " << param_set_vec_[i].max_position_ << ""
                            << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }

      param_set_.max_position_ = param_set_vec_[param_index_to_use_].max_position_;
      AdjustMaxPosLimitOfferSizes();
    } break;
    case kControlMessageCodeSetMaxGlobalRisk: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (_control_message_.intval_1_ > param_set_vec_[i].max_global_risk_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < param_set_vec_[i].max_global_risk_ / FAT_FINGER_FACTOR) {
          param_set_vec_[i].max_global_risk_ = _control_message_.intval_1_;
        }
      }
      param_set_.max_global_risk_ = param_set_vec_[param_index_to_use_].max_global_risk_;
    } break;
    case kControlMessageCodeSetMaxPosition: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (_control_message_.intval_1_ > param_set_vec_[i].max_position_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < param_set_vec_[i].max_position_ * FAT_FINGER_FACTOR) {
          param_set_vec_[i].max_position_ = std::max(1, _control_message_.intval_1_);
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxPosition " << runtime_id_ << " called with " << _control_message_.intval_1_
                            << " and MaxPosition for param " << i << " set to " << param_set_vec_[i].max_position_ << ""
                            << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }

      param_set_.max_position_ = param_set_vec_[param_index_to_use_].max_position_;
      AdjustMaxPosLimitOfferSizes();
    } break;
    case kControlMessageCodeShowParams: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "ShowParams " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      ShowParams();
    } break;
    case kControlMessageCodeSetEcoSeverity: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "IgnoreEconomicNumbers " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      severity_to_getflat_on_ = std::max(1.00, _control_message_.doubleval_1_);
      int t_severity_change_end_msecs_ = GetMsecsFromMidnightFromHHMMSS(_control_message_.intval_1_);
      severity_change_end_msecs_ = std::min(trading_end_utc_mfm_, t_severity_change_end_msecs_);
      DBGLOG_TIME << "Seteco set to " << severity_to_getflat_on_ << " at time " << watch_.msecs_from_midnight()
                  << " with end time as " << severity_change_end_msecs_ << DBGLOG_ENDL_FLUSH;
    } break;
    case kControlMessageCodeForceIndicatorReady: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "ForceIndicatorReady " << runtime_id_
                          << " called for indicator_index_ = " << _control_message_.intval_1_ << DBGLOG_ENDL_FLUSH;

        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }

      p_base_model_math_->ForceIndicatorReady(_control_message_.intval_1_);
    } break;
    case kControlMessageCodeForceAllIndicatorReady: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "ForceAllIndicatorReady " << runtime_id_ << DBGLOG_ENDL_FLUSH;

        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }

      p_base_model_math_->ForceAllIndicatorReady();
    } break;

    case kControlMessageCodeSetMaxLoss: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (_control_message_.intval_1_ > param_set_vec_[i].max_loss_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < param_set_vec_[i].max_loss_ * FAT_FINGER_FACTOR) {
          param_set_vec_[i].max_loss_ = _control_message_.intval_1_;
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxLoss " << runtime_id_
                            << " called for abs_max_loss_ = " << _control_message_.intval_1_
                            << " and MaxLoss for param " << i << " set to " << param_set_vec_[i].max_loss_
                            << DBGLOG_ENDL_FLUSH;

          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }
      param_set_.max_loss_ = param_set_vec_[param_index_to_use_].max_loss_;
    } break;

    case kControlMessageCodeShowOrders: {
      DBGLOG_TIME_CLASS << "ShowOrders " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
      PrintFullStatus();
      order_manager_.LogFullStatus();
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    } break;

    case kControlMessageCodeSetMaxGlobalPosition: {
      for (auto i = 0u; i < param_set_vec_.size(); i++) {
        if (param_set_vec_[i].read_max_global_position_) {
          if (_control_message_.intval_1_ >= 0 &&
              _control_message_.intval_1_ < param_set_vec_[i].max_global_position_ * FAT_FINGER_FACTOR) {
            param_set_vec_[i].max_global_position_ = std::max(
                0, _control_message_
                       .intval_1_);  // Allow 0 to disable trading when our net position across all queries = 0 ???
          }
        } else {  // Param file did not specify a max-global-position , set it to provided value.
          if (_control_message_.intval_1_ >= 0) {
            param_set_vec_[i].max_global_position_ = std::max(0, _control_message_.intval_1_);
            param_set_vec_[i].read_max_global_position_ =
                true;  // Enable this for the max-global-position to take effect.
          }
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxGlobalPosition " << runtime_id_ << " called with " << _control_message_.intval_1_
                            << " and MaxGlobalPosition for param " << i << " set to "
                            << param_set_vec_[i].max_global_position_ << "" << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }
      param_set_.max_global_position_ = param_set_vec_[param_index_to_use_].max_global_position_;
      param_set_.read_max_global_position_ = param_set_vec_[param_index_to_use_].read_max_global_position_;
      AdjustGlobalMaxPosLimitOfferSizes();
    } break;

    case kControlMessageCodeSetStartTime: {
      int old_trading_start_utc_mfm_ = -1;

      if (_control_message_.intval_1_ >= 0 && _control_message_.intval_1_ <= 2359) {  // utc hhmm time sanity check.
        old_trading_start_utc_mfm_ = trading_start_utc_mfm_;
        trading_start_utc_mfm_ = GetMsecsFromMidnightFromHHMM(_control_message_.intval_1_) + watch_.day_offset();
      }

      DBGLOG_TIME_CLASS << "SetStartTime " << runtime_id_ << " called with " << _control_message_.intval_1_
                        << " and trading_start_utc_mfm_ set to " << trading_start_utc_mfm_ << " from "
                        << old_trading_start_utc_mfm_ << DBGLOG_ENDL_FLUSH;

      if (livetrading_) {
        DBGLOG_DUMP;
      }
    } break;

    case kControlMessageCodeSetEndTime: {
      int old_trading_end_utc_mfm_ = -1;

      if (_control_message_.intval_1_ >= 0 && _control_message_.intval_1_ <= 2359) {  // utc hhmm time sanity check.
        old_trading_end_utc_mfm_ = trading_end_utc_mfm_;
        trading_end_utc_mfm_ = GetMsecsFromMidnightFromHHMM(_control_message_.intval_1_) +
                               watch_.day_offset();  // no solution if someone calls it during the previous day UTC
                                                     // itself.Trading will stop immediately in this pathological case
      }

      DBGLOG_TIME_CLASS << "SetEndTime " << runtime_id_ << " called with " << _control_message_.intval_1_
                        << " and trading_end_utc_mfm_ set to " << trading_end_utc_mfm_ << " from "
                        << old_trading_end_utc_mfm_ << DBGLOG_ENDL_FLUSH;

      if (livetrading_) {
        DBGLOG_DUMP;
      }
    } break;

    case kControlMessageReloadEconomicEvents: {
      DBGLOG_TIME_CLASS << "Refreshecoevents user_msg " << DBGLOG_ENDL_FLUSH;
      economic_events_manager_.ReloadDB();
      economic_events_manager_.AllowEconomicEventsFromList(dep_market_view_.shortcode());
      allowed_events_present_ = economic_events_manager_.AllowedEventsPresent();
      economic_events_manager_.AdjustSeverity(dep_market_view_.shortcode(), dep_market_view_.exch_source());
    } break;

    case kControlMessageShowEconomicEvents: {
      DBGLOG_TIME_CLASS << "Showecoevents user_msg " << DBGLOG_ENDL_FLUSH;
      economic_events_manager_.ShowDB();
    } break;

    default:
      break;
  }
}

void RetailTrading::ProcessGetFlat() {
  bool t_should_be_getting_flat_ = RetailShouldbeGettingFlat(getflat_retail_update_type_);

  if (!should_be_getting_flat_ && t_should_be_getting_flat_) {
    // going to getflat
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME << "getting_flat because " << HFSAT::CDef::RetailUpdateTypeToString(getflat_retail_update_type_)
                  << " @"
                  << " NYC: " << watch_.NYTimeString() << " UTC: " << watch_.UTCTimeString()
                  << " IND: " << watch_.IndTimeString() << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
  }

  if (should_be_getting_flat_ &&
      !t_should_be_getting_flat_) {  // currently it is set  ... so after this call we will start normal trading
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME << "resume_normal_trading @"
                  << " NYC: " << watch_.NYTimeString() << " UTC: " << watch_.UTCTimeString()
                  << " IND: " << watch_.IndTimeString() << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
  }

  should_be_getting_flat_ = t_should_be_getting_flat_;
}

void RetailTrading::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}

void RetailTrading::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                 const MarketUpdateInfo& _market_update_info_) {}

void RetailTrading::CheckParamSet() {
  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    if (!param_set_vec_[i].read_max_position_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "max_position_");
    }
    if (!param_set_vec_[i].read_max_global_position_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "max_global_position_");
    }
    if (!param_set_vec_[i].read_unit_trade_size_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "unit_trade_size_");
    }
    if (!param_set_vec_[i].read_zeropos_place_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "zeropos_place_");
    }
    if (param_set_vec_[i].allowed_to_aggress_ && !param_set_vec_[i].read_aggressive_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "aggressive_");
    }
    if (param_set_vec_[i].allowed_to_improve_ && !param_set_vec_[i].read_improve_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "improve_");
    }
    if (!param_set_vec_[i].read_max_loss_ && livetrading_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "max_loss_");
    }
  }
}

bool RetailTrading::RetailShouldbeGettingFlat(HFSAT::CDef::RetailUpdateType& _retail_update_type_) {
  if (!is_ready_) {
    // not necessary as is_ready has to be true to come here
    _retail_update_type_ = HFSAT::CDef::kquerynotready;
    return true;
  } else if (getflat_due_to_external_getflat_) {
    if (start_not_given_) {
      _retail_update_type_ = HFSAT::CDef::kstartnotgiven;
    } else {
      _retail_update_type_ = HFSAT::CDef::kexternalgetflat;
    }
    return true;
  } else if (getflat_due_to_economic_times_ || getflat_due_to_allowed_economic_event_) {
    _retail_update_type_ = HFSAT::CDef::kecotime;
    return true;
  } else if (getflat_due_to_close_) {
    _retail_update_type_ = HFSAT::CDef::kafterendtime;
    return true;
  } else if (getflat_due_to_max_loss_) {
    _retail_update_type_ = HFSAT::CDef::kother;
    return true;
  } else if (getflat_due_to_market_data_interrupt_ || !dep_market_view_.is_ready()) {
    _retail_update_type_ = HFSAT::CDef::kunstablemarket;
    return true;
  }

  _retail_update_type_ = HFSAT::CDef::knormal;
  return false;
}

void RetailTrading::OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_val_) {
  retail_stdev_ = std::min(param_set_.stdev_cap_, _new_stdev_val_ / avg_stdev_);
}

void RetailTrading::FPOrderExecuted(const char* _secname_, double _price_, TradeType_t r_buysell_,
                                    int _size_executed_) {
  if (_size_executed_ <= 0 || strcmp(_secname_, dep_market_view_.secname()) != 0) {
    return;
  }

  DBGLOG_TIME_CLASS_FUNC << "BEFORE: " << dep_market_view_.shortcode() << " FPPOS: " << fp_position_
                         << " BOMPOS: " << bom_position_ << " MYPOS: " << my_position_ << " TRADE: " << _size_executed_
                         << " " << GetTradeTypeChar(r_buysell_) << " @ " << _price_ << DBGLOG_ENDL_FLUSH;

  if (r_buysell_ == kTradeTypeBuy) {
    last_fp_buy_order_msecs_ = watch_.msecs_from_midnight();
  } else {
    last_fp_sell_order_msecs_ = watch_.msecs_from_midnight();
  }

  fp_position_ += (r_buysell_ == kTradeTypeBuy) ? _size_executed_ : -_size_executed_;
  my_position_ = bom_position_ + fp_position_;

  AdjustMaxPosLimitOfferSizes();
  UpdateTarget(target_price_, targetbias_numbers_);

  order_manager_.AddToTradeVolume(_size_executed_);
  order_manager_.base_pnl().OnRetailExec(my_position_, _size_executed_, r_buysell_, _price_,
                                         dep_market_view_.GetIntPx(_price_), dep_market_view_.security_id());

  DBGLOG_TIME_CLASS_FUNC << "After: FPPOS: " << fp_position_ << " BOMPOS: " << bom_position_
                         << " MYPOS: " << my_position_ << DBGLOG_ENDL_FLUSH;

  if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
    PrintFullStatus();
    order_manager_.LogFullStatus();
  }
}

void RetailTrading::OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                           const double _price_, const int r_int_price_, const int _security_id_) {
  if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "BEFORE: " << dep_market_view_.shortcode() << " FPPOS: " << fp_position_
                           << " BOMPOS: " << bom_position_ << " MYPOS: " << my_position_
                           << " TRADE: " << _exec_quantity_ << " " << GetTradeTypeChar(_buysell_) << " @ " << _price_
                           << DBGLOG_ENDL_FLUSH;
  }

  bom_position_ = _new_position_;
  my_position_ = bom_position_ + fp_position_;

  AdjustMaxPosLimitOfferSizes();
  UpdateTarget(target_price_, targetbias_numbers_);

  order_manager_.base_pnl().OnExec(my_position_, _exec_quantity_, _buysell_, _price_, r_int_price_, _security_id_);

  if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "After: FPPOS: " << fp_position_ << " BOMPOS: " << bom_position_
                           << " MYPOS: " << my_position_ << DBGLOG_ENDL_FLUSH;
  }
}

void RetailTrading::AdjustMaxPosLimitOfferSizes() {
  // make this a step function of my_position_, to reduce fluctuation and giving time to retail query to reduce position
  int t_exp_mov_avg_fok_size_ = int(p_exp_mov_fok_size_->GetSum());

  // increase effective position if we have transferred any long positions to HFT queries via FOK, this will further
  // reduce bidsizes
  int t_effective_position_ = my_position_ + std::max(0, t_exp_mov_avg_fok_size_);
  bidsize_to_show_maxpos_limit = param_set_.max_position_ -
                                 int(ceil(t_effective_position_ / double(param_set_.retail_max_position_step_size_))) *
                                     param_set_.retail_max_position_step_size_;

  // decrease effective position if we have transferred any short positions to HFT queries via FOK, this will further
  // reduce asksizes
  t_effective_position_ = my_position_ + std::min(0, t_exp_mov_avg_fok_size_);
  asksize_to_show_maxpos_limit = param_set_.max_position_ -
                                 int(ceil(-t_effective_position_ / double(param_set_.retail_max_position_step_size_))) *
                                     param_set_.retail_max_position_step_size_;
}

void RetailTrading::AdjustGlobalMaxPosLimitOfferSizes() {
  // make this a step function of my_position_, to reduce fluctuation and giving time to retail query to reduce position
  bidsize_to_show_global_maxpos_limit =
      param_set_.max_global_position_ -
      int(ceil(my_global_position_ / double(param_set_.retail_max_global_position_step_size_))) *
          param_set_.retail_max_global_position_step_size_;
  asksize_to_show_global_maxpos_limit =
      param_set_.max_global_position_ -
      int(ceil(-my_global_position_ / double(param_set_.retail_max_global_position_step_size_))) *
          param_set_.retail_max_global_position_step_size_;
}

void RetailTrading::OnTimePeriodUpdate(const int num_pages_to_add_) {
  // computing applicable_severity_
  applicable_severity_ = 0;
  for (auto i = 0u; i < ezone_vec_.size(); i++) {
    applicable_severity_ += economic_events_manager_.GetCurrentSeverity(ezone_vec_[i]);
  }

  // setting/resetting severity to getlfat on
  if (watch_.msecs_from_midnight() > severity_change_end_msecs_ &&
      severity_to_getflat_on_ != severity_to_getflat_on_base_) {
    severity_to_getflat_on_ = severity_to_getflat_on_base_;
    DBGLOG_TIME << "Seteco set to " << severity_to_getflat_on_ << " at time " << watch_.msecs_from_midnight()
                << DBGLOG_ENDL_FLUSH;
    severity_change_end_msecs_ = trading_end_utc_mfm_;
  }

  // setting getflats
  if (applicable_severity_ >= severity_to_getflat_on_) {
    if (!getflat_due_to_economic_times_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << "getflat_due_to_economic_times_ @"
                    << " NYC: " << watch_.NYTimeString() << " UTC: " << watch_.UTCTimeString()
                    << " IND: " << watch_.IndTimeString() << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      getflat_due_to_economic_times_ = true;
    }
  } else {
    getflat_due_to_economic_times_ = false;
  }

  if (allowed_events_present_) {
    const std::vector<EventLine>& allowed_events_of_the_day_ = economic_events_manager_.allowed_events_of_the_day();
    if (getflat_due_to_allowed_economic_event_) {  // if currently in getflat see if we are getting out of it
      if ((last_allowed_event_index_ <= allowed_events_of_the_day_.size()) &&
          (watch_.msecs_from_midnight() >= allowed_events_of_the_day_[last_allowed_event_index_].end_mfm_)) {
        getflat_due_to_allowed_economic_event_ = false;
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME << "getflat_due_to_allowed_economic_event_ set to false" << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }
    } else {
      // TODO optimize by searching in only nearby events by time ... not all events
      for (unsigned int i = last_allowed_event_index_; i < allowed_events_of_the_day_.size(); i++) {
        if (watch_.msecs_from_midnight() < allowed_events_of_the_day_[i].start_mfm_) {
          break;
        } else {  // >= start
          if (watch_.msecs_from_midnight() < allowed_events_of_the_day_[i].end_mfm_) {
            if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
              DBGLOG_TIME << "getflat_due_to_allowed_economic_event_ set to true " << DBGLOG_ENDL_FLUSH;
              if (livetrading_) {
                DBGLOG_DUMP;
              }
            }
            getflat_due_to_allowed_economic_event_ = true;
            last_allowed_event_index_ = i;
            break;
          }
        }
      }
    }
  }

  if (getflat_due_to_close_ || watch_.msecs_from_midnight() > trading_end_utc_mfm_) {
    if (!getflat_due_to_close_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_close_ current: " << watch_.msecs_from_midnight() << " " << trading_end_utc_mfm_
                    << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      getflat_due_to_close_ = true;
      if (livetrading_) {
        CPUManager::AffinToInitCores(getpid());
      }
    }
  } else {
    getflat_due_to_close_ = false;
  }

  if (livetrading_ && (order_manager_.base_pnl().total_pnl() < -param_set_.max_loss_)) {
    if (!getflat_due_to_max_loss_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << " getflat_due_to_max_loss_ " << runtime_id_ << DBGLOG_ENDL_FLUSH;
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
  } else {
    getflat_due_to_max_loss_ = false;
  }

  if (abs(p_exp_mov_fok_size_->GetSum()) > 0) {
    p_exp_mov_fok_size_->NewSample(watch_.msecs_from_midnight(), 0);  // this will decay to 0 as FOKs become old
  }

  if (watch_.msecs_from_midnight() - last_retail_update_msecs_ > RETAIL_UPDATE_TIMEOUT_MSECS) {
    // just to send an update to GUI if we are idle for too long
    UpdateTarget(target_price_, targetbias_numbers_);
  }
}

void RetailTrading::TargetNotReady() {
  if (!getflat_due_to_market_data_interrupt_) {
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "Setting getflat_due_to_external_getflat_ = true because TargetNotReady called. Give "
                                "Start if things seem fine." << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
    getflat_due_to_external_getflat_ = true;
    UpdateTarget(target_price_, targetbias_numbers_);
  }
}

void RetailTrading::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    if (!getflat_due_to_market_data_interrupt_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC << "Setting getflat_due_to_market_data_interrupt_ = true in querid: " << runtime_id_
                               << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      getflat_due_to_market_data_interrupt_ = true;
      UpdateTarget(target_price_, targetbias_numbers_);
    }
  }
}

void RetailTrading::OnMarketDataResumed(const unsigned int _security_id_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    if (getflat_due_to_market_data_interrupt_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC << "Setting getflat_due_to_market_data_interrupt_ = false in querid: " << runtime_id_
                               << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      getflat_due_to_market_data_interrupt_ = false;
      // UpdateTarget(target_price_, targetbias_numbers_); this will get called on smv ready
    }
  }
}

void RetailTrading::AllocateCPU() {
  std::vector<std::string> affinity_process_list_vec;
  process_type_map process_and_type = AffinityAllocator::parseProcessListFile(affinity_process_list_vec);

  int32_t core_assigned = CPUManager::allocateFirstBestAvailableCore(process_and_type, affinity_process_list_vec,
                                                                     getpid(), "retail-tradeinit", false);
  DBGLOG_CLASS_FUNC_LINE_INFO << " AFFINED TO : " << core_assigned << " PID : " << getpid() << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;
}

void RetailTrading::ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool conservative_close_) {
  if (livetrading_) {
    printf("SIMRESULT %d %d %d %d %d %d\n",
           (int)order_manager_.base_pnl().ReportConservativeTotalPNL(conservative_close_),
           order_manager_.trade_volume(), order_manager_.SupportingOrderFilledPercent(),
           order_manager_.BestLevelOrderFilledPercent(), order_manager_.AggressiveOrderFilledPercent(),
           order_manager_.ImproveOrderFilledPercent());
  } else {
    int t_pnl_ = (int)(order_manager_.base_pnl().ReportConservativeTotalPNL(conservative_close_));
    printf("SIMRESULT %d %d %d %d %d %d\n", t_pnl_, order_manager_.trade_volume(),
           order_manager_.SupportingOrderFilledPercent(), order_manager_.BestLevelOrderFilledPercent(),
           order_manager_.AggressiveOrderFilledPercent(), order_manager_.ImproveOrderFilledPercent());

    trades_writer_ << "EOD_MIN_PNL: " << runtime_id_ << " " << order_manager_.base_pnl().min_pnl_till_now() << "\n";
    trades_writer_ << "EOD_MSG_COUNT: " << runtime_id_ << " "
                   << (order_manager_.SendOrderCount() + order_manager_.CxlOrderCount() +
                       order_manager_.ModifyOrderCount()) << "\n";
    trades_writer_ << "UNIT_TRADE_SIZE: " << runtime_id_ << " " << param_set_.unit_trade_size_ << "\n";
    trades_writer_.CheckToFlushBuffer();
  }
}

void RetailTrading::OnFokFill(const TradeType_t _buysell_, const double _price_, const int intpx_,
                              const int _size_exec_) {
  if (_buysell_ == kTradeTypeSell) {
    // we transfered long position to HFT queries
    p_exp_mov_fok_size_->NewSample(watch_.msecs_from_midnight(), _size_exec_);
  } else {
    // we transfered short position to HFT queries
    p_exp_mov_fok_size_->NewSample(watch_.msecs_from_midnight(), -_size_exec_);
  }
}

void RetailTrading::SendTradeAndLog(int _int_px_, int _size_, TradeType_t _buysell_, char _level_indicator_) {
  unsigned int t_num_ords_to_send_ = _size_ / param_set_.retail_max_ord_size_;
  int t_last_ord_size_ = _size_ % param_set_.retail_max_ord_size_;

  for (auto i = 0u; i < t_num_ords_to_send_; i++) {
    order_manager_.SendTradeIntPx(_int_px_, param_set_.retail_max_ord_size_, _buysell_, _level_indicator_);
    if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "SendTrade " << GetTradeTypeChar(_buysell_) << " " << dep_market_view_.shortcode()
                             << " of " << param_set_.retail_max_ord_size_ << " @ IntPx: " << _int_px_
                             << " mkt: " << dep_market_view_.bestbid_size() << " @ " << dep_market_view_.bestbid_price()
                             << " X " << dep_market_view_.bestask_price() << " @ " << dep_market_view_.bestask_size()
                             << DBGLOG_ENDL_FLUSH;
    }
  }

  if (t_last_ord_size_ > 0) {
    order_manager_.SendTradeIntPx(_int_px_, t_last_ord_size_, _buysell_, _level_indicator_);
    if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "SendTrade " << GetTradeTypeChar(_buysell_) << " " << dep_market_view_.shortcode()
                             << " of " << t_last_ord_size_ << " @ IntPx: " << _int_px_
                             << " mkt: " << dep_market_view_.bestbid_size() << " @ " << dep_market_view_.bestbid_price()
                             << " X " << dep_market_view_.bestask_price() << " @ " << dep_market_view_.bestask_size()
                             << DBGLOG_ENDL_FLUSH;
    }
  }
}

void RetailTrading::ReconcileParams() {
  int t_max_loss_ = 0;
  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    if (t_max_loss_ < param_set_vec_[i].max_loss_) {
      t_max_loss_ = param_set_vec_[i].max_loss_;
    }
  }

  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    param_set_vec_[i].stdev_cap_ = std::max(param_set_vec_[i].stdev_cap_, 1.00);
    param_set_vec_[i].max_loss_ = t_max_loss_;

    // sanity range checks, in case parmfile has garbage values
    // using for quoting
    param_set_vec_[i].zeropos_place_ = std::max(0.25, param_set_vec_[i].zeropos_place_);
    param_set_vec_[i].zeropos_place_ *= dep_market_view_.min_price_increment();

    param_set_vec_[i].owp_retail_offer_thresh_ = std::max(0.2, param_set_vec_[i].owp_retail_offer_thresh_);
    param_set_vec_[i].owp_retail_offer_thresh_ *= dep_market_view_.min_price_increment();

    // using for closing
    param_set_vec_[i].improve_ = std::max(0.6, param_set_vec_[i].improve_);
    if (param_set_vec_[i].allowed_to_improve_) {
      param_set_vec_[i].aggressive_ =
          std::max(std::max(0.8, param_set_vec_[i].improve_), param_set_vec_[i].aggressive_);
    } else {
      param_set_vec_[i].aggressive_ = std::max(0.8, param_set_vec_[i].aggressive_);
    }
  }
  param_set_.stdev_cap_ = std::max(param_set_.stdev_cap_, 1.00);
  retail_stdev_ = 1;

  param_set_.max_loss_ = t_max_loss_;

  param_set_.zeropos_place_ = std::max(0.25, param_set_.zeropos_place_);
  param_set_.zeropos_place_ *= dep_market_view_.min_price_increment();

  param_set_.owp_retail_offer_thresh_ = std::max(0.2, param_set_.owp_retail_offer_thresh_);
  param_set_.owp_retail_offer_thresh_ *= dep_market_view_.min_price_increment();

  param_set_.improve_ = std::max(0.6, param_set_.improve_);
  if (param_set_.allowed_to_improve_) {
    param_set_.aggressive_ = std::max(std::max(0.8, param_set_.improve_), param_set_.aggressive_);
  } else {
    param_set_.aggressive_ = std::max(0.8, param_set_.aggressive_);
  }
}
}
