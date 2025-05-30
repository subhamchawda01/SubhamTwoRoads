#include "baseinfra/OrderRouting/base_order.hpp"
#include "dvctrade/SpreadTrading/spread_exec_logic.hpp"

#define HIGH_DEFAULT_STDEV_VALUE 1000

namespace MT_SPRD {
SpreadExecLogic::SpreadExecLogic(HFSAT::DebugLogger& _dbglogger_, HFSAT::BulkFileWriter& trades_writer_,
                                 HFSAT::Watch& _watch_, const int _sec_id_pass_, const int _sec_id_agg_,
                                 HFSAT::SecurityMarketView& _smv_pass_, HFSAT::SecurityMarketView& _smv_agg_,
                                 HFSAT::SmartOrderManager& _om_pass_, HFSAT::SmartOrderManager& _om_agg_,
                                 const int _overnight_pos_pass_, const int _overnight_pos_agg_, ParamSet* t_param_,
                                 bool t_live, bool t_is_banned)
    : dbglogger_(_dbglogger_),
      tradesfile_(trades_writer_),
      watch_(_watch_),
      sec_id_pass_(_sec_id_pass_),
      sec_id_agg_(_sec_id_agg_),
      mkt_view_pass_(_smv_pass_),
      mkt_view_agg_(_smv_agg_),
      om_pass_(_om_pass_),
      om_agg_(_om_agg_),
      overnight_position_pass_(_overnight_pos_pass_),
      overnight_position_agg_(_overnight_pos_agg_),
      best_bid_price_pass_leg_(-1.0),
      best_ask_price_pass_leg_(-1.0),
      best_bid_int_price_pass_leg_(-1),
      best_ask_int_price_pass_leg_(-1),
      best_bid_price_agg_leg_(-1.0),
      best_ask_price_agg_leg_(-1.0),
      best_bid_int_price_agg_leg_(-1),
      best_ask_int_price_agg_leg_(-1),
      bid_int_price_to_place_at_pass_leg_(-1),
      ask_int_price_to_place_at_pass_leg_(-1),
      bid_int_price_to_place_at_agg_leg_(-1),
      ask_int_price_to_place_at_agg_leg_(-1),
      time_bid_order_placed_pass_leg_(0),
      time_ask_order_placed_pass_leg_(0),
      time_bid_order_placed_agg_leg_(0),
      time_ask_order_placed_agg_leg_(0),
      stdev_pass_(HIGH_DEFAULT_STDEV_VALUE),
      stdev_agg_(HIGH_DEFAULT_STDEV_VALUE),
      position_pass_(_overnight_pos_pass_),
      position_agg_(_overnight_pos_agg_),
      desired_pos_pass_(0),
      desired_pos_agg_(0),
      target_spread_(0),
      spread_alpha_(0),
      spread_beta_(0),
      last_order_handling_call_time_(0),
      last_order_handling_pass_leg_midpx_(0),
      last_order_handling_agg_leg_midpx_(0),
      hedge_ratio_(1),
      getflat_mode_(false),
      is_ready_(false),
      param_(t_param_),
      live(t_live),
      is_banned(t_is_banned),
      slack_pos_pass(_overnight_pos_pass_ + 5),
      slack_pos_agg(_overnight_pos_agg_ + 5) {
  om_pass_.AddPositionChangeListener(this);
  om_agg_.AddPositionChangeListener(this);
  om_pass_.AddExecutionListener(this);
  om_agg_.AddExecutionListener(this);
  shortcode_pass_ = _smv_pass_.shortcode();
  shortcode_agg_ = _smv_agg_.shortcode();
}

// dump into trades file : TODO add more data here
void SpreadExecLogic::OnExec(const int _new_position_, const int _exec_quantity_, const HFSAT::TradeType_t _buysell_,
                             const double _price_, const int r_int_price_, const int _security_id_) {
  char t_inst_[32];
  double t_bpx_;
  double t_apx_;
  if ((unsigned)_security_id_ == sec_id_pass_) {
    sprintf(t_inst_, "%s", mkt_view_pass_.secname());
    t_bpx_ = best_bid_price_pass_leg_;
    t_apx_ = best_ask_price_pass_leg_;
  } else {
    sprintf(t_inst_, "%s", mkt_view_agg_.secname());
    t_bpx_ = best_bid_price_agg_leg_;
    t_apx_ = best_ask_price_agg_leg_;
  }
  char t_buf_[1024];
  sprintf(t_buf_, "%d %s %c %5d %.2f [ %f X %f ]", watch_.tv().tv_sec, t_inst_,
          (_buysell_ == HFSAT::kTradeTypeBuy ? 'B' : 'S'), _exec_quantity_, _price_, t_bpx_, t_apx_);
  tradesfile_ << t_buf_ << '\n';
  tradesfile_.DumpCurrentBuffer();

  std::string Shortcode = HFSAT::SecurityNameIndexer::GetUniqueInstance().GetShortcodeFromId(_security_id_);

  // send slack notif
  DBGLOG_TIME_CLASS_FUNC << watch_.tv().tv_usec
                         << " : SLACK : " << (_buysell_ == HFSAT::kTradeTypeBuy ? "Bought" : "Sold") << " " << Shortcode
                         << "(" << t_inst_ << ") " << _exec_quantity_ << "@" << _price_ << DBGLOG_ENDL_FLUSH;
}

// close open trade
void SpreadExecLogic::OnAllEventsConsumed() {
  char t_buf_[1024];
  int t_lotsize_;
  if (position_pass_ != 0) {
    t_lotsize_ = (param_->is_inst1_pass_ ? param_->lotsize_1_ : param_->lotsize_2_);
    sprintf(t_buf_, "%d %s %c %5d %.2f [ %f X %f ]*", watch_.tv().tv_sec, mkt_view_pass_.secname(),
            (position_pass_ < 0 ? 'B' : 'S'), abs(position_pass_ * t_lotsize_),
            (position_pass_ < 0 ? best_ask_price_pass_leg_ : best_bid_price_pass_leg_), best_bid_price_pass_leg_,
            best_ask_price_pass_leg_);
    tradesfile_ << t_buf_ << '\n';
    tradesfile_.DumpCurrentBuffer();
  }
  if (position_agg_ != 0) {
    t_lotsize_ = (param_->is_inst1_pass_ ? param_->lotsize_2_ : param_->lotsize_1_);
    sprintf(t_buf_, "%d %s %c %5d %.2f [ %f X %f ]*", watch_.tv().tv_sec, mkt_view_agg_.secname(),
            (position_agg_ < 0 ? 'B' : 'S'), abs(position_agg_ * t_lotsize_),
            (position_agg_ < 0 ? best_ask_price_agg_leg_ : best_bid_price_agg_leg_), best_bid_price_agg_leg_,
            best_ask_price_agg_leg_);
    tradesfile_ << t_buf_ << '\n';
    tradesfile_.DumpCurrentBuffer();
  }
}

void SpreadExecLogic::OnMarketUpdate(const unsigned int _security_id_,
                                     const HFSAT::MarketUpdateInfo& _market_update_info_) {
  if (_security_id_ == sec_id_pass_) {
    best_bid_price_pass_leg_ = mkt_view_pass_.bestbid_price();
    best_bid_int_price_pass_leg_ = mkt_view_pass_.bestbid_int_price();
    best_ask_price_pass_leg_ = mkt_view_pass_.bestask_price();
    best_ask_int_price_pass_leg_ = mkt_view_pass_.bestask_int_price();
  } else if (_security_id_ == sec_id_agg_) {
    best_bid_price_agg_leg_ = mkt_view_agg_.bestbid_price();
    best_bid_int_price_agg_leg_ = mkt_view_agg_.bestbid_int_price();
    best_ask_price_agg_leg_ = mkt_view_agg_.bestask_price();
    best_ask_int_price_agg_leg_ = mkt_view_agg_.bestask_int_price();
  } else {
    DBGLOG_TIME_CLASS_FUNC << " should not be called with id " << _security_id_ << DBGLOG_ENDL_FLUSH;
  }

  // set isready to true when both market books are true
  if (!is_ready_ && mkt_view_pass_.is_ready_complex(3) && mkt_view_agg_.is_ready_complex(3)) {
    is_ready_ = true;
  }

  if (fabs((0.5 * best_bid_price_pass_leg_ + 0.5 * best_ask_price_pass_leg_) / last_order_handling_pass_leg_midpx_ -
           1.0) > param_->px_thresh_ ||
      fabs((0.5 * best_bid_price_agg_leg_ + 0.5 * best_ask_price_agg_leg_) / last_order_handling_agg_leg_midpx_ - 1.0) >
          param_->px_thresh_ ||
      watch_.msecs_from_midnight() - last_order_handling_call_time_ > (unsigned)param_->time_thresh_) {
    last_order_handling_pass_leg_midpx_ = 0.5 * best_bid_price_pass_leg_ + 0.5 * best_ask_price_pass_leg_;
    last_order_handling_agg_leg_midpx_ = 0.5 * best_bid_price_agg_leg_ + 0.5 * best_ask_price_agg_leg_;
    last_order_handling_call_time_ = watch_.msecs_from_midnight();
    OrderManagement();
  }
}

bool SpreadExecLogic::PriceCheckPassed() {
  double t_px1_, t_px2_ = 0;
  // get px1 and px2 for spread relation for passive execution on passive
  // leg and aggressive execution on agg leg.
  //
  // px1 and px2 refer to first and second instruments of the spread formulation
  if (desired_pos_pass_ > position_pass_) {
    if (param_->is_inst1_pass_) {
      t_px1_ = best_bid_price_pass_leg_;
      t_px2_ = best_bid_price_agg_leg_;  // note - buying pass implies selling agg
    } else {
      t_px2_ = best_bid_price_pass_leg_;
      t_px1_ = best_bid_price_agg_leg_;
    }
  } else {
    if (param_->is_inst1_pass_) {
      t_px1_ = best_ask_price_pass_leg_;
      t_px2_ = best_ask_price_agg_leg_;
    } else {
      t_px2_ = best_ask_price_pass_leg_;
      t_px1_ = best_ask_price_agg_leg_;
    }
  }

  double t_achievable_spread_ = 0;
  if (param_->asset_comp_mode_ == 0 && param_->spread_comp_mode_ == 1)
    t_achievable_spread_ = t_px1_ - spread_beta_ * t_px2_;
  else if (param_->asset_comp_mode_ == 0 && param_->spread_comp_mode_ == 2)
    t_achievable_spread_ = t_px1_ - spread_alpha_ - spread_beta_ * t_px2_;
  else if (param_->asset_comp_mode_ == 1 && param_->spread_comp_mode_ == 1)
    t_achievable_spread_ = log(t_px1_) - spread_beta_ * log(t_px2_);
  else if (param_->asset_comp_mode_ == 1 && param_->spread_comp_mode_ == 2)
    t_achievable_spread_ = log(t_px1_) - spread_alpha_ - spread_beta_ * log(t_px2_);

  if (desired_pos_pass_ - position_pass_ > 0 &&
      desired_pos_pass_ >=
          0)  // target thresh will be the minimal thresh value at which we enter this enhanced long position
  {
    return ((param_->is_inst1_pass_ && t_achievable_spread_ <= target_spread_) ||
            (!param_->is_inst1_pass_ && t_achievable_spread_ >= target_spread_));
  } else if (desired_pos_pass_ <= 0 && desired_pos_pass_ - position_pass_ < 0) {
    return ((param_->is_inst1_pass_ && t_achievable_spread_ >= target_spread_) ||
            (!param_->is_inst1_pass_ && t_achievable_spread_ <= target_spread_));
  } else if ((desired_pos_pass_ == 0 && position_pass_ > 0) ||
             (desired_pos_pass_ > 0 && position_pass_ - desired_pos_pass_ > 0)) {
    return ((param_->is_inst1_pass_ && t_achievable_spread_ >= target_spread_) ||
            (!param_->is_inst1_pass_ && t_achievable_spread_ <= target_spread_));
  } else if ((desired_pos_pass_ == 0 && position_pass_ < 0) ||
             (desired_pos_pass_ < 0 && desired_pos_pass_ - position_pass_ > 0)) {
    return ((param_->is_inst1_pass_ && t_achievable_spread_ <= target_spread_) ||
            (!param_->is_inst1_pass_ && t_achievable_spread_ >= target_spread_));
  } else {
    std::cerr << "Should not get here .. logically \n";
    return (false);
  }
}

void SpreadExecLogic::OnPositionChange(int t_position_, int position_diff_, const unsigned int _security_id_) {
  int t_temp_position_ = 0;
  if (_security_id_ == sec_id_pass_) {
    int t_lotsize_ = (param_->is_inst1_pass_ ? param_->lotsize_1_ : param_->lotsize_2_);
    t_temp_position_ = overnight_position_pass_ + t_position_ / t_lotsize_;
    if (t_temp_position_ != position_pass_) {
      position_pass_ = t_temp_position_;
      OrderManagement();
    }
  } else if (_security_id_ == sec_id_agg_) {
    int t_lotsize_ = (param_->is_inst1_pass_ ? param_->lotsize_2_ : param_->lotsize_1_);
    t_temp_position_ = overnight_position_agg_ + t_position_ / t_lotsize_;
    if (t_temp_position_ != position_agg_) {
      position_agg_ = t_temp_position_;
      OrderManagement();
    }
  } else {
    DBGLOG_TIME_CLASS_FUNC << " called with insupported secid " << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }
}

void SpreadExecLogic::OrderManagement() {
  if (!is_ready_ || is_banned) {
    return;
  }
  // if positions are as desired, cancel all open orders
  if (desired_pos_pass_ == position_pass_ && desired_pos_agg_ == position_agg_) {
    // DBGLOG_TIME_CLASS_FUNC << " Cancel all orders for since positions match " << shortcode_pass_ << ": " <<
    // desired_pos_pass_ << ' '  << position_pass_ << "  ---  " << shortcode_agg_ << ": " << desired_pos_agg_ << ' ' <<
    // position_agg_ << '\n';
    om_pass_.CancelAllOrders();
    om_agg_.CancelAllOrders();
    return;
  }

  // set target prices for buy/sell to defauilt values
  bid_int_price_to_place_at_pass_leg_ = -1;
  ask_int_price_to_place_at_pass_leg_ = -1;
  bid_int_price_to_place_at_agg_leg_ = -1;
  ask_int_price_to_place_at_agg_leg_ = -1;

  // set target price for passive leg if applicable
  if (desired_pos_pass_ != position_pass_) {
    // for passive, i.e. the illiquid leg we place orders to reach size as
    // long as price constraints permit
    bool t_price_check_ = PriceCheckPassed();
    DBGLOG_TIME_CLASS_FUNC << (t_price_check_ ? 'Y' : 'N') << ':' << desired_pos_pass_ << ' ' << position_pass_ << ' '
                           << best_bid_price_pass_leg_ << " -- " << best_ask_price_pass_leg_ << '\t' << desired_pos_agg_
                           << ' ' << position_agg_ << ' ' << best_bid_price_agg_leg_ << " -- "
                           << best_ask_price_agg_leg_ << ' ' << ' ' << target_spread_ << '\n';
    if (t_price_check_ || getflat_mode_)  // place order at passive level
    {
      if (desired_pos_pass_ > position_pass_) {
        bid_int_price_to_place_at_pass_leg_ = best_bid_int_price_pass_leg_;
        DBGLOG_TIME_CLASS_FUNC << " Cancel all sell orders in " << shortcode_pass_ << " and bid px "
                               << best_bid_price_pass_leg_ << '\n';
        om_pass_.CancelAllAskOrders();
      } else {
        ask_int_price_to_place_at_pass_leg_ = best_ask_int_price_pass_leg_;
        DBGLOG_TIME_CLASS_FUNC << " Cancel all buy orders in " << shortcode_pass_ << " and ask px "
                               << best_ask_price_pass_leg_ << '\n';
        om_pass_.CancelAllBidOrders();
      }
    } else {
      DBGLOG_TIME_CLASS_FUNC << " Cancelall orders in " << shortcode_pass_ << " price check false "
                             << DBGLOG_ENDL_FLUSH;
      om_pass_.CancelAllBidOrders();
      om_pass_.CancelAllAskOrders();
    }
  }

  // set target prices for aggressive leg if applicable -- for agg leg
  // enter positions depending on execs on pass leg, while keeping overall
  // position as hedged as possible
  if (desired_pos_agg_ != position_agg_) {
    int t_agg_pos_projected_ = (int)(round(hedge_ratio_ * -1.0 * position_pass_));
    if (t_agg_pos_projected_ > position_agg_) {
      bid_int_price_to_place_at_agg_leg_ = best_bid_int_price_agg_leg_;
      DBGLOG_TIME_CLASS_FUNC << " Cancelall sell orders in " << shortcode_agg_ << " and bid px "
                             << best_bid_price_agg_leg_ << DBGLOG_ENDL_FLUSH;
      om_agg_.CancelAllAskOrders();
    } else if (t_agg_pos_projected_ < position_agg_) {
      ask_int_price_to_place_at_agg_leg_ = best_ask_int_price_agg_leg_;
      DBGLOG_TIME_CLASS_FUNC << " Cancelall buy orders in " << shortcode_agg_ << " and sell px "
                             << best_ask_price_agg_leg_ << DBGLOG_ENDL_FLUSH;
      om_agg_.CancelAllBidOrders();
    } else {
      DBGLOG_TIME_CLASS_FUNC << " Cancelall orders in " << shortcode_agg_ << " since position is hedged "
                             << DBGLOG_ENDL_FLUSH;
      om_agg_.CancelAllBidOrders();
      om_agg_.CancelAllAskOrders();
    }
  }

  if (bid_int_price_to_place_at_pass_leg_ != -1) {
    PlaceSingleBuyOrder(om_pass_, bid_int_price_to_place_at_pass_leg_, time_bid_order_placed_pass_leg_, true);
  }
  if (bid_int_price_to_place_at_agg_leg_ != -1) {
    PlaceSingleBuyOrder(om_agg_, bid_int_price_to_place_at_agg_leg_, time_bid_order_placed_agg_leg_, false);
  }
  if (ask_int_price_to_place_at_pass_leg_ != -1) {
    PlaceSingleSellOrder(om_pass_, ask_int_price_to_place_at_pass_leg_, time_ask_order_placed_pass_leg_, true);
  }
  if (ask_int_price_to_place_at_agg_leg_ != -1) {
    PlaceSingleSellOrder(om_agg_, ask_int_price_to_place_at_agg_leg_, time_ask_order_placed_agg_leg_, false);
  }
}

void SpreadExecLogic::PlaceSingleSellOrder(HFSAT::SmartOrderManager& om_, int order_px_, uint64_t& time_order_placed_,
                                           bool is_pass_) {
  int t_order_vec_top_ask_index_ = om_.GetOrderVecTopAskIndex();
  int t_order_vec_bottom_ask_index_ = om_.GetOrderVecBottomAskIndex();
  int t_existing_int_price_ = -1;
  double t_existing_price_ = -1.0;
  int t_existing_size_ = 0;

  if (t_order_vec_top_ask_index_ != t_order_vec_bottom_ask_index_) {
    DBGLOG_TIME_CLASS_FUNC << "More than one ask orders in SprdExec \n" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  if (om_.GetUnSequencedAsks().size() > 1) {
    DBGLOG_TIME_CLASS_FUNC << " More than one unsequenced sell order in SprdExec \n" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  if (om_.GetUnSequencedAsks().size() > 0) {
    return;                                       // don't do anything if unseq ask order is live
  } else if (t_order_vec_top_ask_index_ != -1) {  // -1 denotes no active orders in ordermanager
    t_existing_int_price_ = om_.GetAskIntPrice(t_order_vec_top_ask_index_);
    t_existing_price_ = (is_pass_ ? mkt_view_pass_.GetDoublePx(t_existing_int_price_)
                                  : mkt_view_agg_.GetDoublePx(t_existing_int_price_));
  }

  HFSAT::BaseOrder* t_order_ = NULL;
  if (t_existing_int_price_ != -1) {
    t_order_ = om_.GetBottomAskOrderAtIntPx(t_existing_int_price_);
    t_existing_size_ = t_order_->size_remaining();
  }

  //    int t_uts_pass_ = (param_->is_inst1_pass_?param_->unit_trade_size_1_:param_->unit_trade_size_2_);
  //    int t_uts_agg_ = (param_->is_inst1_pass_?param_->unit_trade_size_2_:param_->unit_trade_size_1_);
  int t_uts_pass_ = 1;  //(param_->is_inst1_pass_?param_->unit_trade_size_1_:param_->unit_trade_size_2_);
  int t_uts_agg_ = 1;   //(param_->is_inst1_pass_?param_->unit_trade_size_2_:param_->unit_trade_size_1_);
  int t_order_size_to_place_ =
      (is_pass_ ? std::min(t_uts_pass_, position_pass_ - desired_pos_pass_)
                : std::min(position_agg_ - (int)(round(hedge_ratio_ * -1.0 * position_pass_)), t_uts_agg_));
  int t_lotsize_ = ((param_->is_inst1_pass_ == is_pass_) ? param_->lotsize_1_ : param_->lotsize_2_);
  t_order_size_to_place_ = std::max(0, t_order_size_to_place_ * t_lotsize_);
  if (t_order_size_to_place_ == 0) {
    DBGLOG_TIME_CLASS_FUNC << " Order Size Place request for size 0 " << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    return;
  }
  uint64_t t_time_order_placed_ = time_order_placed_;
  uint64_t t_time_thresh_ = (is_pass_ ? param_->pass_ord_mod_time_threshold_ : param_->agg_ord_mod_time_threshold_);
  double t_px_thresh_ =
      (is_pass_ ? param_->pass_ord_mod_std_threshold_ * stdev_pass_ : param_->agg_ord_mod_std_threshold_ * stdev_agg_);
  double t_order_price_to_place_dbl_ =
      (is_pass_ ? mkt_view_pass_.GetDoublePx(order_px_) : mkt_view_agg_.GetDoublePx(order_px_));

  // if there is no order active place an order
  if (t_existing_int_price_ == -1) {
    DBGLOG_TIME_CLASS_FUNC << " Send Sell Order for " << (is_pass_ ? shortcode_pass_ : shortcode_agg_) << ' '
                           << t_order_size_to_place_ << " @ " << order_px_ << DBGLOG_ENDL_FLUSH;
    om_.SendTradeIntPx(order_px_, t_order_size_to_place_, HFSAT::kTradeTypeSell, 'B', HFSAT::kOrderDay);
    time_order_placed_ = watch_.msecs_from_midnight();

    // send slack notif
    int current_pos = (is_pass_) ? position_pass_ : position_agg_;
    int slack_pos = (is_pass_) ? slack_pos_pass : slack_pos_agg;

    if (current_pos != slack_pos) {
      DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " : SLACK : SellRequest "
                             << (is_pass_ ? shortcode_pass_ : shortcode_agg_) << ' ' << t_order_size_to_place_ << "@"
                             << order_px_ * 0.05 << DBGLOG_ENDL_FLUSH;

      // Update slack variables
      if (is_pass_) {
        slack_pos_pass = position_pass_;
      } else {
        slack_pos_agg = position_agg_;
      }
    }

  }
  // else if needed modify size at existing price
  else if (t_existing_size_ != t_order_size_to_place_ &&
           watch_.msecs_from_midnight() - t_time_order_placed_ <= t_time_thresh_ &&
           fabs(t_order_price_to_place_dbl_ - t_existing_price_) <= t_px_thresh_) {
    DBGLOG_TIME_CLASS_FUNC << " Modify Sell Order Size for " << (is_pass_ ? shortcode_pass_ : shortcode_agg_)
                           << " Old_Sz " << t_existing_size_ << " New_Sz " << t_order_size_to_place_
                           << DBGLOG_ENDL_FLUSH;
    om_.ModifyOrderAndLog(t_order_, t_existing_price_, t_existing_int_price_, t_order_size_to_place_);
    time_order_placed_ = watch_.msecs_from_midnight();
  }
  // else if we need to modify price
  else if (watch_.msecs_from_midnight() - t_time_order_placed_ > t_time_thresh_ ||
           fabs(t_order_price_to_place_dbl_ - t_existing_price_) > t_px_thresh_) {
    DBGLOG_TIME_CLASS_FUNC << " Modify Sell Order Price for " << (is_pass_ ? shortcode_pass_ : shortcode_agg_)
                           << " Old_Px " << t_existing_price_ << " New_Px " << t_order_price_to_place_dbl_
                           << DBGLOG_ENDL_FLUSH;
    om_.ModifyOrderAndLog(t_order_, t_order_price_to_place_dbl_, order_px_, t_order_size_to_place_);
    time_order_placed_ = watch_.msecs_from_midnight();
  }
}

void SpreadExecLogic::PlaceSingleBuyOrder(HFSAT::SmartOrderManager& om_, int order_px_, uint64_t& time_order_placed_,
                                          bool is_pass_) {
  int t_order_vec_top_bid_index_ = om_.GetOrderVecTopBidIndex();
  int t_order_vec_bottom_bid_index_ = om_.GetOrderVecBottomBidIndex();
  int t_existing_int_price_ = -1;
  double t_existing_price_ = -1.0;
  int t_existing_size_ = 0;

  if (t_order_vec_top_bid_index_ != t_order_vec_bottom_bid_index_) {
    DBGLOG_TIME_CLASS_FUNC << "More than one bid orders in SprdExec \n" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  if (om_.GetUnSequencedBids().size() > 1) {
    DBGLOG_TIME_CLASS_FUNC << " More than one unsequenced order in SprdExec \n" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  if (om_.GetUnSequencedBids().size() > 0) {
    return;                                       // don't do anything if unseq bid order is live
  } else if (t_order_vec_top_bid_index_ != -1) {  // -1 denotes no active orders in ordermanager
    t_existing_int_price_ = om_.GetBidIntPrice(t_order_vec_top_bid_index_);
    t_existing_price_ = (is_pass_ ? mkt_view_pass_.GetDoublePx(t_existing_int_price_)
                                  : mkt_view_agg_.GetDoublePx(t_existing_int_price_));
  }

  HFSAT::BaseOrder* t_order_ = NULL;
  if (t_existing_int_price_ != -1) {
    t_order_ = om_.GetBottomBidOrderAtIntPx(t_existing_int_price_);
    t_existing_size_ = t_order_->size_remaining();
  }

  int t_uts_pass_ = 1;  //(param_->is_inst1_pass_?param_->unit_trade_size_1_:param_->unit_trade_size_2_);
  int t_uts_agg_ = 1;   //(param_->is_inst1_pass_?param_->unit_trade_size_2_:param_->unit_trade_size_1_);
  //    int t_uts_pass_ = (param_->is_inst1_pass_?param_->unit_trade_size_1_:param_->unit_trade_size_2_);
  //    int t_uts_agg_ = (param_->is_inst1_pass_?param_->unit_trade_size_2_:param_->unit_trade_size_1_);
  int t_order_size_to_place_ =
      (is_pass_ ? std::min(t_uts_pass_, desired_pos_pass_ - position_pass_)
                : std::min((int)(round(hedge_ratio_ * -1.0 * position_pass_)) - position_agg_, t_uts_agg_));
  int t_lotsize_ = ((param_->is_inst1_pass_ == is_pass_) ? param_->lotsize_1_ : param_->lotsize_2_);
  t_order_size_to_place_ = std::max(0, t_order_size_to_place_ * t_lotsize_);
  if (t_order_size_to_place_ == 0) {
    DBGLOG_TIME_CLASS_FUNC << " Order Size Place request for size 0 " << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    return;
  }
  uint64_t t_time_order_placed_ = time_order_placed_;
  uint64_t t_time_thresh_ = (is_pass_ ? param_->pass_ord_mod_time_threshold_ : param_->agg_ord_mod_time_threshold_);
  double t_px_thresh_ =
      (is_pass_ ? param_->pass_ord_mod_std_threshold_ * stdev_pass_ : param_->agg_ord_mod_std_threshold_ * stdev_agg_);
  double t_order_price_to_place_dbl_ =
      (is_pass_ ? mkt_view_pass_.GetDoublePx(order_px_) : mkt_view_agg_.GetDoublePx(order_px_));

  // if there is no order active place an order
  if (t_existing_int_price_ == -1) {
    DBGLOG_TIME_CLASS_FUNC << " Send Buy Order for " << (is_pass_ ? shortcode_pass_ : shortcode_agg_) << ' '
                           << t_order_size_to_place_ << " @ " << order_px_ << DBGLOG_ENDL_FLUSH;
    om_.SendTradeIntPx(order_px_, t_order_size_to_place_, HFSAT::kTradeTypeBuy, 'B', HFSAT::kOrderDay);
    time_order_placed_ = watch_.msecs_from_midnight();

    // send slack notif
    int current_pos = (is_pass_) ? position_pass_ : position_agg_;
    int slack_pos = (is_pass_) ? slack_pos_pass : slack_pos_agg;

    if (current_pos != slack_pos) {
      DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " : SLACK : BuyRequest " << (is_pass_ ? shortcode_pass_ : shortcode_agg_)
                             << ' ' << t_order_size_to_place_ << "@" << order_px_ * 0.05 << DBGLOG_ENDL_FLUSH;

      // Update slack variables
      if (is_pass_) {
        slack_pos_pass = position_pass_;
      } else {
        slack_pos_agg = position_agg_;
      }
    }
  }
  // else if needed modify size at existing price
  else if (t_existing_size_ != t_order_size_to_place_ &&
           watch_.msecs_from_midnight() - t_time_order_placed_ <= t_time_thresh_ &&
           fabs(t_order_price_to_place_dbl_ - t_existing_price_) <= t_px_thresh_) {
    DBGLOG_TIME_CLASS_FUNC << " Modify Buy Order Size for " << (is_pass_ ? shortcode_pass_ : shortcode_agg_)
                           << " Old_Sz " << t_existing_size_ << " New_Sz " << t_order_size_to_place_
                           << DBGLOG_ENDL_FLUSH;
    om_.ModifyOrderAndLog(t_order_, t_existing_price_, t_existing_int_price_, t_order_size_to_place_);
    time_order_placed_ = watch_.msecs_from_midnight();
  }
  // else if we need to modify price
  else if (watch_.msecs_from_midnight() - t_time_order_placed_ > t_time_thresh_ ||
           fabs(t_order_price_to_place_dbl_ - t_existing_price_) > t_px_thresh_) {
    DBGLOG_TIME_CLASS_FUNC << " Modify Buy Order Price for " << (is_pass_ ? shortcode_pass_ : shortcode_agg_)
                           << " Old_Px " << t_existing_price_ << " New_Px " << t_order_price_to_place_dbl_
                           << DBGLOG_ENDL_FLUSH;
    om_.ModifyOrderAndLog(t_order_, t_order_price_to_place_dbl_, order_px_, t_order_size_to_place_);
    time_order_placed_ = watch_.msecs_from_midnight();
  }
}
}
