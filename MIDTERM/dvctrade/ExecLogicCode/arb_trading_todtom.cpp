/**
   \file ExecLogicCode/arb_trading_todtom.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "dvctrade/ExecLogic/arb_trading_todtom.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"

#define FAT_FINGER_FACTOR 5
namespace HFSAT {

void ArbTradingTodTom::CollectORSShortCodes(DebugLogger& dbglogger, const std::string& strategy_name,
                                            const std::string& dep_shortcode,
                                            std::vector<std::string>& source_shortcode_vec,
                                            std::vector<std::string>& ors_source_needed_vec,
                                            std::vector<std::string>& dependant_shortcode_vec) {
  if (strategy_name != StrategyName()) {
    return;
  }

  if (dep_shortcode.compare("USD000000TOD") == 0 || dep_shortcode.compare("USD000UTSTOM") == 0 ||
      dep_shortcode.compare("USD000TODTOM") == 0) {
    VectorUtils::UniqueVectorAdd(source_shortcode_vec, std::string("USD000000TOD"));
    VectorUtils::UniqueVectorAdd(ors_source_needed_vec, std::string("USD000000TOD"));
    VectorUtils::UniqueVectorAdd(dependant_shortcode_vec, std::string("USD000000TOD"));
    VectorUtils::UniqueVectorAdd(source_shortcode_vec, std::string("USD000UTSTOM"));
    VectorUtils::UniqueVectorAdd(ors_source_needed_vec, std::string("USD000UTSTOM"));
    VectorUtils::UniqueVectorAdd(dependant_shortcode_vec, std::string("USD000UTSTOM"));
    VectorUtils::UniqueVectorAdd(source_shortcode_vec, std::string("USD000TODTOM"));
    VectorUtils::UniqueVectorAdd(ors_source_needed_vec, std::string("USD000TODTOM"));
    VectorUtils::UniqueVectorAdd(dependant_shortcode_vec, std::string("USD000TODTOM"));
  }
}

void ArbTradingTodTom::GetDepShortcodes(const std::string& dep_shortcode, std::vector<std::string>& dep_shortcode_vec) {
  if (dep_shortcode.compare("USD000000TOD") == 0 || dep_shortcode.compare("USD000UTSTOM") == 0 ||
      dep_shortcode.compare("USD000TODTOM") == 0) {
    VectorUtils::UniqueVectorAdd(dep_shortcode_vec, std::string("USD000000TOD"));
    VectorUtils::UniqueVectorAdd(dep_shortcode_vec, std::string("USD000UTSTOM"));
    VectorUtils::UniqueVectorAdd(dep_shortcode_vec, std::string("USD000TODTOM"));
  }
}

ArbTradingTodTom::ArbTradingTodTom(DebugLogger& dbglogger, const Watch& watch, const SecurityMarketView& tod_smv,
                                   const SecurityMarketView& tom_smv, const SecurityMarketView& tod_tom_smv,
                                   SmartOrderManager& tod_order_manager, SmartOrderManager& tom_order_manager,
                                   SmartOrderManager& tod_tom_order_manager, const std::string& paramfilename,
                                   const bool livetrading, ttime_t start_time, ttime_t end_time, int runtime_id,
                                   PnlWriter* pnl_writer)
    : ExecInterface(dbglogger, watch, tod_smv, tod_order_manager, paramfilename, livetrading),
      tod_smv_(tod_smv),
      tom_smv_(tom_smv),
      tod_tom_smv_(tod_tom_smv),
      tod_om_(tod_order_manager),
      tom_om_(tom_order_manager),
      tod_tom_om_(tod_tom_order_manager),
      tod_secid_(tod_smv.security_id()),
      tom_secid_(tom_smv.security_id()),
      tod_tom_secid_(tod_tom_smv_.security_id()),
      tod_bid_int_price_(0),
      tod_ask_int_price_(0),
      tod_bid_price_(0.0),
      tod_ask_price_(0.0),
      tod_mid_int_price_(0),
      tod_bid_ask_spread_(1),
      tod_bid_size_(0),
      tod_ask_size_(0),
      tom_bid_int_price_(0),
      tom_ask_int_price_(0),
      tom_bid_price_(0.0),
      tom_ask_price_(0.0),
      tom_mid_int_price_(0),
      tom_bid_ask_spread_(1),
      tom_bid_size_(0),
      tom_ask_size_(0),
      tod_tom_bid_int_price_(0),
      tod_tom_ask_int_price_(0),
      tod_tom_bid_price_(0.0),
      tod_tom_ask_price_(0.0),
      tod_tom_mid_int_price_(0),
      tod_tom_bid_ask_spread_(1),
      tod_tom_bid_size_(0),
      tod_tom_ask_size_(0),
      tod_position_(0),
      tom_position_(0),
      tod_tom_position_(0),
      spread_adj_tod_position_(0),
      spread_adj_tom_position_(0),
      spread_position_(0),
      accumulated_spread_position_(0),
      excess_tod_position_(0),
      excess_tom_position_(0),
      tod_best_tom_agg_(false),
      tom_best_tod_agg_(false),
      synth_spread_buy_(0.0),
      synth_spread_sell_(0.0),
      synth_spread_buy_thresh_flag_(false),
      synth_spread_sell_thresh_flag_(false),
      synth_spread_buy_flag_(false),
      synth_spread_sell_flag_(false),
      num_lvl_order_keep_(0),
      last_new_page_msecs_(0.0),
      unit_trade_size_(100),
      arb_place_thresh_(500),
      start_time_(start_time),
      end_time_(end_time),
      external_getflat_(livetrading),
      runtime_id_(runtime_id),
      getflat_due_to_maxloss_(false),
      num_arb_chances_(0),
      last_arb_size_(0),
      combined_pnl_(new CombinedPnlTodTom(dbglogger, watch, tod_smv, tom_smv, tod_tom_smv, pnl_writer)),
      tod_mkt_status_(kMktTradingStatusOpen),
      tom_mkt_status_(kMktTradingStatusOpen),
      tod_tom_mkt_status_(kMktTradingStatusOpen),
      getflat_due_to_mkt_status_(false),
      is_ready_(false) {
  tod_smv_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  tom_smv_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  tod_tom_smv_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);

  tod_smv_.subscribe_MktStatus(this);
  tom_smv_.subscribe_MktStatus(this);
  tod_tom_smv_.subscribe_MktStatus(this);

  tom_om_.AddPositionChangeListener(this);
  tom_om_.AddExecutionListener(this);
  tom_om_.AddCancelRejectListener(this);
  tom_om_.AddRejectDueToFundsListener(this);
  tom_om_.AddFokFillRejectListener(this);

  tod_tom_om_.AddPositionChangeListener(this);
  tod_tom_om_.AddExecutionListener(this);
  tod_tom_om_.AddCancelRejectListener(this);
  tod_tom_om_.AddRejectDueToFundsListener(this);
  tod_tom_om_.AddFokFillRejectListener(this);

  arb_place_thresh_ = param_set_.arb_place_thresh_;
  num_lvl_order_keep_ = param_set_.num_lvl_order_keep_;

  if (param_set_.arb_best_shc_.compare("USD000000TOD") == 0) {
    tod_best_tom_agg_ = true;
  } else if (param_set_.arb_best_shc_.compare("USD000UTSTOM") == 0) {
    tom_best_tod_agg_ = true;
  }
  unit_trade_size_ = param_set_.unit_trade_size_;
}

void ArbTradingTodTom::GetFlatTradingLogic() {
  ReconcilePositions();
  int t_tod_position_ = spread_adj_tod_position_ + 100 * spread_position_;
  int t_tom_position_ = spread_adj_tom_position_ - 100 * spread_position_;

  GetFlatTradingLogic(t_tod_position_, tod_om_, tod_smv_);
  GetFlatTradingLogic(t_tom_position_, tom_om_, tom_smv_);
  GetFlatTradingLogic(spread_position_, tod_tom_om_, tod_tom_smv_);
}

void ArbTradingTodTom::GetFlatTradingLogic(int t_position_, SmartOrderManager& order_manager_,
                                           const SecurityMarketView& t_smv_) {
  // only passive execution in getting out

  if (t_position_ == 0) {  // nothing to be done, cancel all remaining orders
    order_manager_.CancelAllOrders();
  } else if (t_position_ > 0) {
    // long hence cancel all bid orders
    order_manager_.CancelAllBidOrders();

    bool done_for_this_round_ = false;

    if (!done_for_this_round_) {
      // haven't agressed or places any orders in this round
      int eqabove_best_size_ordered_ = order_manager_.SumAskSizeConfirmedEqAboveIntPrice(t_smv_.bestask_int_price()) +
                                       order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(t_smv_.bestask_int_price());
      {
        int trade_size_required_ = MathUtils::GetFlooredMultipleOf(std::min(t_position_, param_set_.unit_trade_size_),
                                                                   dep_market_view_.min_order_size());

        if (eqabove_best_size_ordered_ < trade_size_required_) {  // if the required size is not present then add more
          order_manager_.SendTrade(t_smv_.bestask_price(), t_smv_.bestask_int_price(),
                                   trade_size_required_ - eqabove_best_size_ordered_, kTradeTypeSell, 'B');
        }
        // not doing anything if the placed size is more than required // well u can send a replace
      }
    }

    // instead of cancelling all non-best orders, cancel just enough to avoid overfill
    int t_total_asks_placed_ = order_manager_.SumAskSizes();
    if (t_total_asks_placed_ > t_position_) {
      order_manager_.KeepAskSizeInPriceRange(t_position_);
    }
  } else {  // my_position_ < 0
    // short hence cancel all sell orders
    order_manager_.CancelAllAskOrders();

    bool done_for_this_round_ = false;

    if (!done_for_this_round_) {
      // haven't agressed or places any orders in this round
      int eqabove_best_size_ordered_ = order_manager_.SumBidSizeConfirmedEqAboveIntPrice(t_smv_.bestbid_int_price()) +
                                       order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(t_smv_.bestbid_int_price());

      int trade_size_required_ = MathUtils::GetFlooredMultipleOf(std::min(-t_position_, param_set_.unit_trade_size_),
                                                                 dep_market_view_.min_order_size());

      if (eqabove_best_size_ordered_ < trade_size_required_) {  // if the required size is not present then add more
        order_manager_.SendTrade(t_smv_.bestbid_price(), t_smv_.bestbid_int_price(),
                                 trade_size_required_ - eqabove_best_size_ordered_, kTradeTypeBuy, 'B');
        // not doing anything if the placed size is more than required
      }
      // instead of cancelling all non-best orders cancel, just enough to avoid overfill
      int t_total_bids_placed_ = order_manager_.SumBidSizes();
      if (t_total_bids_placed_ > -t_position_) {
        order_manager_.KeepBidSizeInPriceRange(-t_position_);
      }
    }
  }
}

// TODO: make this check lighter
bool ArbTradingTodTom::ValidPrices() {
  if (tod_bid_int_price_ == kInvalidIntPrice || tod_ask_int_price_ == kInvalidIntPrice ||
      tom_bid_int_price_ == kInvalidIntPrice || tom_ask_int_price_ == kInvalidIntPrice ||
      tod_tom_bid_int_price_ == kInvalidIntPrice || tod_tom_ask_int_price_ == kInvalidIntPrice ||
      tod_bid_int_price_ >= tod_ask_int_price_ || tom_bid_int_price_ >= tom_ask_int_price_ ||
      tod_tom_bid_int_price_ >= tod_ask_int_price_ || tod_bid_size_ == 0 || tod_ask_size_ == 0 || tom_bid_size_ == 0 ||
      tom_ask_size_ == 0 || tod_tom_bid_size_ == 0 || tod_tom_ask_size_ == 0) {
    return false;
  }

  return true;
}

std::string ArbTradingTodTom::GetCurrentMarket() {
  std::ostringstream oss;
  oss << " Position: "
      << "(" << combined_pnl_->GetTODPosition() << "+" << combined_pnl_->GetTOMPosition() << "+"
      << combined_pnl_->GetTODTOMPosition() << ").";
  oss << " Tod: [" << tod_bid_size_ << " " << tod_bid_int_price_ << " * " << tod_ask_int_price_ << " " << tod_ask_size_
      << "]"
      << " Tom: [" << tom_bid_size_ << " " << tom_bid_int_price_ << " * " << tom_ask_int_price_ << " " << tom_ask_size_
      << "]"
      << " TodTom: [" << tod_tom_bid_size_ << " " << tod_tom_bid_int_price_ << " * " << tod_tom_ask_int_price_ << " "
      << tod_tom_ask_size_ << "]";
  oss << " Pnl: " << combined_pnl_->GetUnrealizedPnl();

  return oss.str();
}

bool ArbTradingTodTom::ShouldGetFlat() {
  if (!tod_smv_.is_ready() || !tom_smv_.is_ready() || !tod_tom_smv_.is_ready()) {
    return true;
  }

  if (tod_mkt_status_ != kMktTradingStatusOpen || tom_mkt_status_ != kMktTradingStatusOpen ||
      tod_tom_mkt_status_ != kMktTradingStatusOpen) {
    if (!getflat_due_to_mkt_status_) {
      getflat_due_to_mkt_status_ = true;
      DBGLOG_TIME << "getflat_due_to_mkt_status. TOD mktstatus: " << tod_mkt_status_
                  << " TOM mktstatus: " << tom_mkt_status_ << " TODTOM mktstatus: " << tod_tom_mkt_status_
                  << DBGLOG_ENDL_FLUSH;
    }
  } else {
    if (getflat_due_to_mkt_status_) {
      DBGLOG_TIME << "resuming from getflat_due_to_mkt_status." << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    getflat_due_to_mkt_status_ = false;
  }

  if (getflat_due_to_mkt_status_) {
    return true;
  }

  if (!ValidPrices()) {
    return true;
  }

  if (watch_.tv() < start_time_ || watch_.tv() > end_time_) {
    return true;
  }

  if (external_getflat_) {
    return true;
  }

  if (livetrading_ && combined_pnl_->GetUnrealizedPnl() < -param_set_.max_loss_) {
    if (!getflat_due_to_maxloss_) {
      getflat_due_to_maxloss_ = true;
      DBGLOG_TIME << " getflat_due_to_max_loss_ " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;

      // Send the mail if live-trading and within trading window
      if (livetrading_ && watch_.tv() > start_time_ && watch_.tv() < end_time_) {
        // Send out an email / alert
        char hostname[128];
        hostname[127] = '\0';
        gethostname(hostname, 127);

        std::string getflat_email_string = "";
        {
          std::ostringstream oss;
          oss << "Strategy: " << runtime_id_ << " getflat_due_to_max_loss_: " << combined_pnl_->GetUnrealizedPnl()
              << " ArbTradingTodTom products " << tod_smv_.shortcode() << " (" << tod_smv_.secname() << ") and "
              << tom_smv_.shortcode() << " (" << tom_smv_.secname() << ") and " << tod_tom_smv_.shortcode() << ")"
              << " on " << hostname << "\n";
          getflat_email_string = oss.str();
        }

        Email email;
        email.setSubject(getflat_email_string);
        email.addRecepient("nseall@tworoads.co.in");
        email.addSender("nseall@tworoads.co.in");
        email.content_stream << getflat_email_string << "<br/>";
        email.sendMail();
      }
    }
  } else {
    if (getflat_due_to_maxloss_) {
      DBGLOG_TIME << "resuming from getflat_due_to_maxloss." << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    getflat_due_to_maxloss_ = false;
  }

  if (getflat_due_to_maxloss_) {
    return true;
  }

  return false;
}

bool ArbTradingTodTom::IsLastIOCOrderIncomplete() {
  if (tom_om_.IOCOrderExists() || tod_om_.IOCOrderExists() || tod_tom_om_.IOCOrderExists()) {
    return true;
  }
  return false;
}

void ArbTradingTodTom::TradingLogic(int security_id) {
  if (!is_ready_) {
    return;
  }

  if (ShouldGetFlat()) {
    GetFlatTradingLogic();
    return;
  }
  // ReconcilePositions ( );

  OrderPlacingLogic();
}

void ArbTradingTodTom::OrderPlacingLogic() {
  if (tod_best_tom_agg_) {
    if (synth_spread_buy_flag_) {
      tod_om_.CancelAllBidOrders();

      int t_tod_sell_placed_ = tod_om_.SumAskSizeConfirmedEqAboveIntPrice(tod_ask_int_price_ + num_lvl_order_keep_) +
                               tod_om_.SumAskSizeUnconfirmedEqAboveIntPrice(tod_ask_int_price_ + num_lvl_order_keep_);

      int t_tod_sell_size_ = std::max(0, unit_trade_size_ - std::abs(spread_adj_tod_position_) - t_tod_sell_placed_);

      if (t_tod_sell_size_ > 0) {
        tod_om_.SendTrade(tod_smv_.GetDoublePx(tod_ask_int_price_), tod_ask_int_price_, t_tod_sell_size_,
                          kTradeTypeSell, 'B', kOrderDay);
        // std::cout << watch_.tv().tv_sec << "." << watch_.tv().tv_usec << " tod pos : " << tod_position_ << " tom pos
        // : " << tom_position_ << " tod_tom pos: " << tod_tom_position_ << ". Send Tod Sell at " << tod_ask_int_price_
        // << " of size " << t_tod_sell_size_ << std::endl;
        // std::cout << watch_.tv().tv_sec << "." << watch_.tv().tv_usec << " synth spread buy : " << synth_spread_buy_
        // << std::endl;
      }

      //      if ( spread_adj_tod_position_ < 0 && spread_adj_tom_position_ >=0 && ( -spread_adj_tom_position_ -
      //      spread_adj_tod_position_ ) > 0 )
      if (-tod_position_ - tom_position_ > 0) {
        int t_tom_buy_size_ = std::min(unit_trade_size_, -spread_adj_tom_position_ - spread_adj_tod_position_);
        if (tom_bid_ask_spread_ == 1) {
          tom_mid_int_price_ = tom_ask_int_price_;
        }
        SendAggBuy(tom_smv_, tom_om_, tom_ask_int_price_, tom_mid_int_price_, t_tom_buy_size_);
        // std::cout << watch_.tv().tv_sec << "." << watch_.tv().tv_usec << " tod pos : " << tod_position_ << " tom pos
        // : " << tom_position_ << " tod_tom pos: " << tod_tom_position_ << ". Send Tom Buy at " << tom_ask_int_price_
        // << " of size " << t_tom_buy_size_ << std::endl;
        // std::cout << watch_.tv().tv_sec << "." << watch_.tv().tv_usec << " synth spread buy : " << synth_spread_buy_
        // << std::endl;
      }

      if (accumulated_spread_position_ > 50) {
        SendAggSell(tod_tom_smv_, tod_tom_om_, tod_tom_bid_int_price_, tod_tom_bid_int_price_, 1);
        // std::cout << watch_.tv().tv_sec << "." << watch_.tv().tv_usec << " tod pos : " << tod_position_ << " tom pos
        // : " << tom_position_ << " tod_tom pos: " << tod_tom_position_ << ". Send TodTom Sell at " <<
        // tod_tom_bid_int_price_ << " of size 1" << std::endl;
        // std::cout << watch_.tv().tv_sec << "." << watch_.tv().tv_usec << " synth spread buy : " << synth_spread_buy_
        // << std::endl;
      }

    } else if (synth_spread_buy_ > 0 && spread_adj_tod_position_ < 0) {
      if (-tod_position_ - tom_position_ > 0) {
        int t_tom_buy_size_ = std::min(unit_trade_size_, -spread_adj_tom_position_ - spread_adj_tod_position_);
        if (tom_bid_ask_spread_ == 1) {
          tom_mid_int_price_ = tom_ask_int_price_;
        }
        SendAggBuy(tom_smv_, tom_om_, tom_ask_int_price_, tom_mid_int_price_, t_tom_buy_size_);
        // std::cout << watch_.tv().tv_sec << "." << watch_.tv().tv_usec << " TOM close out. tod pos : " <<
        // tod_position_ << " tom pos : " << tom_position_ << " tod_tom pos: " << tod_tom_position_ << ". Send Tom Buy
        // at " << tom_ask_int_price_ << " of size " << t_tom_buy_size_ << std::endl;
        // std::cout << watch_.tv().tv_sec << "." << watch_.tv().tv_usec << " synth spread buy : " << synth_spread_buy_
        // << std::endl;
      }

      if (accumulated_spread_position_ > 50) {
        SendAggSell(tod_tom_smv_, tod_tom_om_, tod_tom_bid_int_price_, tod_tom_bid_int_price_, 1);
        // std::cout << watch_.tv().tv_sec << "." << watch_.tv().tv_usec << " TODTOM close out. tod pos : " <<
        // tod_position_ << " tom pos : " << tom_position_ << " tod_tom pos: " << tod_tom_position_ << ". Send TodTom
        // Sell at " << tod_tom_bid_int_price_ << " of size 1" << std::endl;
        // std::cout << watch_.tv().tv_sec << "." << watch_.tv().tv_usec << " synth spread buy : " << synth_spread_buy_
        // << std::endl;
      }
    } else if (synth_spread_sell_flag_) {
      tod_om_.CancelAllAskOrders();

      if (tod_bid_ask_spread_ == 1) {
        tod_mid_int_price_ = tod_bid_int_price_;
      }

      int t_tod_buy_placed_ = tod_om_.SumBidSizeConfirmedEqAboveIntPrice(tod_bid_int_price_ - num_lvl_order_keep_) +
                              tod_om_.SumBidSizeUnconfirmedEqAboveIntPrice(tod_bid_int_price_ - num_lvl_order_keep_);

      int t_tod_buy_size_ = std::max(0, unit_trade_size_ - std::abs(spread_adj_tod_position_) - t_tod_buy_placed_);
      if (t_tod_buy_size_ > 0) {
        tod_om_.SendTrade(tod_smv_.GetDoublePx(tod_bid_int_price_), tod_bid_int_price_, t_tod_buy_size_, kTradeTypeBuy,
                          'B', kOrderDay);
      }

      if (spread_adj_tod_position_ > 0 && spread_adj_tom_position_ <= 0 &&
          (spread_adj_tod_position_ + spread_adj_tom_position_) > 0) {
        int t_tom_sell_size_ = std::min(unit_trade_size_, spread_adj_tod_position_ + spread_adj_tom_position_);
        if (tom_bid_ask_spread_ == 1) {
          tom_mid_int_price_ = tom_bid_int_price_;
        }
        SendAggSell(tom_smv_, tom_om_, tom_bid_int_price_, tom_mid_int_price_, t_tom_sell_size_);
      }

      if (accumulated_spread_position_ < -50) {
        SendAggBuy(tod_tom_smv_, tod_tom_om_, tod_tom_ask_int_price_, tod_tom_ask_int_price_, 1);
      }

    } else {
      tom_om_.CancelAllOrders();
      tod_tom_om_.CancelAllOrders();

      if (excess_tod_position_ > 0) {
        tod_om_.CancelAllBidOrders();

        int t_tod_sell_placed_ = tod_om_.SumAskSizeConfirmedEqAboveIntPrice(tod_ask_int_price_ + num_lvl_order_keep_) +
                                 tod_om_.SumAskSizeUnconfirmedEqAboveIntPrice(tod_ask_int_price_ + num_lvl_order_keep_);
        int t_tod_sell_size_ = std::max(0, excess_tod_position_ - t_tod_sell_placed_);

        if (t_tod_sell_size_ > 0) {
          tod_om_.SendTrade(tod_smv_.GetDoublePx(tod_ask_int_price_), tod_ask_int_price_, t_tod_sell_size_,
                            kTradeTypeSell, 'B', kOrderDay);
        }
      } else if (excess_tod_position_ < 0) {
        tod_om_.CancelAllAskOrders();

        int t_tod_buy_placed_ = tod_om_.SumBidSizeConfirmedEqAboveIntPrice(tod_bid_int_price_ - num_lvl_order_keep_) +
                                tod_om_.SumBidSizeUnconfirmedEqAboveIntPrice(tod_bid_int_price_ - num_lvl_order_keep_);

        int t_tod_buy_size_ = std::max(0, -excess_tod_position_ - t_tod_buy_placed_);
        if (t_tod_buy_size_ > 0) {
          tod_om_.SendTrade(tod_smv_.GetDoublePx(tod_bid_int_price_), tod_bid_int_price_, t_tod_buy_size_,
                            kTradeTypeBuy, 'B', kOrderDay);
        }
      } else {
        tod_om_.CancelAllOrders();
      }
    }
  } else if (tom_best_tod_agg_) {
    if (synth_spread_buy_flag_) {
      tom_om_.CancelAllAskOrders();

      if (tom_bid_ask_spread_ == 1) {
        tom_mid_int_price_ = tom_bid_int_price_;
      }

      int t_tom_buy_placed_ = tom_om_.SumBidSizeConfirmedEqAboveIntPrice(tom_bid_int_price_ - num_lvl_order_keep_) +
                              tom_om_.SumBidSizeUnconfirmedEqAboveIntPrice(tom_bid_int_price_ - num_lvl_order_keep_);

      int t_tom_buy_size_ = std::max(0, unit_trade_size_ - std::abs(spread_adj_tom_position_) - t_tom_buy_placed_);
      if (t_tom_buy_size_ > 0) {
        tom_om_.SendTrade(tom_smv_.GetDoublePx(tom_bid_int_price_), tom_bid_int_price_, t_tom_buy_size_, kTradeTypeBuy,
                          'B', kOrderDay);
      }

      if (spread_adj_tom_position_ > 0 && spread_adj_tod_position_ <= 0 &&
          (spread_adj_tod_position_ + spread_adj_tom_position_) > 0) {
        int t_tod_sell_size_ = std::min(unit_trade_size_, spread_adj_tom_position_ + spread_adj_tod_position_);
        if (tod_bid_ask_spread_ == 1) {
          tod_mid_int_price_ = tod_bid_int_price_;
        }
        SendAggSell(tod_smv_, tod_om_, tod_bid_int_price_, tod_mid_int_price_, t_tod_sell_size_);
      }

      if (accumulated_spread_position_ > 50) {
        SendAggSell(tod_tom_smv_, tod_tom_om_, tod_tom_bid_int_price_, tod_tom_bid_int_price_, 1);
      }
    } else if (synth_spread_sell_flag_) {
      tom_om_.CancelAllBidOrders();

      if (tom_bid_ask_spread_ == 1) {
        tom_mid_int_price_ = tom_ask_int_price_;
      }

      int t_tom_sell_placed_ = tom_om_.SumAskSizeConfirmedEqAboveIntPrice(tom_ask_int_price_ + num_lvl_order_keep_) +
                               tom_om_.SumAskSizeUnconfirmedEqAboveIntPrice(tom_ask_int_price_ + num_lvl_order_keep_);

      int t_tom_sell_size_ = std::max(0, unit_trade_size_ - std::abs(spread_adj_tom_position_) - t_tom_sell_placed_);

      if (t_tom_sell_size_ > 0) {
        tom_om_.SendTrade(tod_smv_.GetDoublePx(tom_ask_int_price_), tom_ask_int_price_, t_tom_sell_size_,
                          kTradeTypeSell, 'B', kOrderDay);
      }

      if (spread_adj_tom_position_ < 0 && spread_adj_tod_position_ >= 0 &&
          (-spread_adj_tod_position_ - spread_adj_tom_position_) > 0) {
        int t_tod_buy_size_ = std::min(unit_trade_size_, -spread_adj_tod_position_ - spread_adj_tom_position_);
        if (tod_bid_ask_spread_ == 1) {
          tod_mid_int_price_ = tom_ask_int_price_;
        }
        SendAggBuy(tod_smv_, tod_om_, tod_ask_int_price_, tod_mid_int_price_, t_tod_buy_size_);
      }

      if (accumulated_spread_position_ < -50) {
        SendAggBuy(tod_tom_smv_, tod_tom_om_, tod_tom_ask_int_price_, tod_tom_ask_int_price_, 1);
      }
    } else {
      tod_om_.CancelAllOrders();
      tod_tom_om_.CancelAllOrders();

      if (excess_tom_position_ > 0) {
        tom_om_.CancelAllOrders();

        if (tom_bid_ask_spread_ == 1) {
          tom_mid_int_price_ = tom_ask_int_price_;
        }

        int t_tom_sell_placed_ = tom_om_.SumAskSizeConfirmedEqAboveIntPrice(tom_ask_int_price_ + num_lvl_order_keep_) +
                                 tom_om_.SumAskSizeUnconfirmedEqAboveIntPrice(tom_ask_int_price_ + num_lvl_order_keep_);
        int t_tom_sell_size_ = std::max(0, excess_tom_position_ - t_tom_sell_placed_);

        if (t_tom_sell_size_ > 0) {
          tom_om_.SendTrade(tom_smv_.GetDoublePx(tom_ask_int_price_), tom_ask_int_price_, t_tom_sell_size_,
                            kTradeTypeSell, 'B', kOrderDay);
        }
      } else if (excess_tom_position_ < 0) {
        tom_om_.CancelAllBidOrders();

        if (tom_bid_ask_spread_ == 1) {
          tom_mid_int_price_ = tom_bid_int_price_;
        }

        int t_tom_buy_placed_ = tom_om_.SumBidSizeConfirmedEqAboveIntPrice(tom_bid_int_price_ - num_lvl_order_keep_) +
                                tom_om_.SumBidSizeUnconfirmedEqAboveIntPrice(tom_bid_int_price_ - num_lvl_order_keep_);

        int t_tom_buy_size_ = std::max(0, -excess_tom_position_ - t_tom_buy_placed_);
        if (t_tom_buy_size_ > 0) {
          tom_om_.SendTrade(tom_smv_.GetDoublePx(tom_bid_int_price_), tom_bid_int_price_, t_tom_buy_size_,
                            kTradeTypeBuy, 'B', kOrderDay);
        }
      }
    }
  }
}

void ArbTradingTodTom::SendAggBuy(const SecurityMarketView& _smv_, SmartOrderManager& _om_, int _best_ask_int_price_,
                                  int _mid_int_price_, int _size_) {
  if (_size_ == 0) {
    return;
  }

  _om_.CancelBidsEqBelowIntPrice(_mid_int_price_ + 1);

  int t_order_placed_ = _om_.SumBidSizeConfirmedEqAboveIntPrice(_mid_int_price_) +
                        _om_.SumBidSizeUnconfirmedEqAboveIntPrice(_mid_int_price_);
  int t_order_to_place_ = _size_ - t_order_placed_;

  if (t_order_to_place_ > 0) {
    _om_.SendTrade(_smv_.GetDoublePx(_best_ask_int_price_), _best_ask_int_price_, t_order_to_place_, kTradeTypeBuy, 'A',
                   kOrderDay);
  }
}

void ArbTradingTodTom::SendAggSell(const SecurityMarketView& _smv_, SmartOrderManager& _om_, int _best_bid_int_price_,
                                   int _mid_int_price_, int _size_) {
  if (_size_ == 0) {
    return;
  }

  _om_.CancelAsksEqBelowIntPrice(_mid_int_price_ + 1);

  int t_order_placed_ = _om_.SumAskSizeConfirmedEqAboveIntPrice(_mid_int_price_) +
                        _om_.SumAskSizeUnconfirmedEqAboveIntPrice(_mid_int_price_);
  int t_order_to_place_ = _size_ - t_order_placed_;

  if (t_order_to_place_ > 0) {
    _om_.SendTrade(_smv_.GetDoublePx(_best_bid_int_price_), _best_bid_int_price_, t_order_to_place_, kTradeTypeSell,
                   'A', kOrderDay);
  }
}

void ArbTradingTodTom::UpdateVariables(int security_id) {
  // DOL
  if (security_id == tod_secid_) {
    tod_bid_int_price_ = tod_smv_.bestbid_int_price();
    tod_ask_int_price_ = tod_smv_.bestask_int_price();
    tod_bid_price_ = tod_smv_.bestbid_price();
    tod_ask_price_ = tod_smv_.bestask_price();
    tod_bid_size_ = tod_smv_.bestbid_size();
    tod_ask_size_ = tod_smv_.bestask_size();
    tod_mid_int_price_ = (int)(tod_bid_int_price_ + tod_ask_int_price_) / 2;
    tod_bid_ask_spread_ = tod_ask_int_price_ - tod_bid_int_price_;
  }

  // WDO
  else if (security_id == tom_secid_) {
    tom_bid_int_price_ = tom_smv_.bestbid_int_price();
    tom_ask_int_price_ = tom_smv_.bestask_int_price();
    tom_bid_price_ = tom_smv_.bestbid_price();
    tom_ask_price_ = tom_smv_.bestask_price();
    tom_bid_size_ = tom_smv_.bestbid_size();
    tom_ask_size_ = tom_smv_.bestask_size();
    tom_mid_int_price_ = (int)(tom_bid_int_price_ + tom_ask_int_price_) / 2;
    tom_bid_ask_spread_ = tom_ask_int_price_ - tom_bid_int_price_;
  }

  else if (security_id == tod_tom_secid_) {
    tod_tom_bid_int_price_ = tod_tom_smv_.bestbid_int_price();
    tod_tom_ask_int_price_ = tod_tom_smv_.bestask_int_price();
    tod_tom_bid_price_ = tod_tom_smv_.bestbid_price();
    tod_tom_ask_price_ = tod_tom_smv_.bestask_price();
    tod_tom_bid_size_ = tod_tom_smv_.bestbid_size();
    tod_tom_ask_size_ = tod_tom_smv_.bestask_size();
    tod_tom_mid_int_price_ = (int)(tod_tom_bid_int_price_ + tod_tom_ask_int_price_) / 2;
    tod_tom_bid_ask_spread_ = tod_tom_ask_int_price_ - tod_tom_bid_int_price_;
  }

  if (tod_best_tom_agg_) {
    synth_spread_buy_ = -100 * 1000 * tom_ask_price_ + 100 * 1000 * tod_ask_price_ + 1 * 100000 * tod_tom_bid_price_ -
                        100 * 1000 * 3 * 0.01 * 0.0011 * tod_ask_price_ - 25;
    synth_spread_sell_ = 100 * 1000 * tom_bid_price_ - 100 * 1000 * tod_bid_price_ - 1 * 100000 * tod_tom_ask_price_ -
                         100 * 1000 * 3 * 0.01 * 0.0011 * tod_bid_price_ - 25;
    synth_spread_buy_thresh_flag_ = synth_spread_buy_ > arb_place_thresh_ ? true : false;
    synth_spread_sell_thresh_flag_ = synth_spread_sell_ > arb_place_thresh_ ? true : false;
    synth_spread_buy_flag_ = synth_spread_buy_thresh_flag_;
    // synth_spread_sell_flag_ = synth_spread_sell_thresh_flag_;
    synth_spread_sell_flag_ = false;
  } else if (tom_best_tod_agg_) {
    synth_spread_buy_ = -100 * 1000 * tom_bid_price_ + 100 * 1000 * tod_bid_price_ + 1 * 100000 * tod_tom_bid_price_ -
                        100 * 1000 * 3 * 0.01 * 0.0011 * tod_bid_price_ - 25;
    synth_spread_sell_ = 100 * 1000 * tom_ask_price_ - 100 * 1000 * tod_ask_price_ - 1 * 100000 * tod_tom_ask_price_ -
                         100 * 1000 * 3 * 0.01 * 0.0011 * tod_ask_price_ - 25;
    synth_spread_buy_thresh_flag_ = synth_spread_buy_ > arb_place_thresh_ ? true : false;
    synth_spread_sell_thresh_flag_ = synth_spread_sell_ > arb_place_thresh_ ? true : false;
    synth_spread_buy_flag_ = synth_spread_buy_thresh_flag_;
    synth_spread_sell_flag_ = synth_spread_sell_thresh_flag_;
  }

  /*
    if ( synth_spread_buy_thresh_flag_ && synth_spread_sell_thresh_flag_ ) {
      synth_spread_sell_flag_ = true;
      synth_spread_buy_flag_ = false;

      if ( synth_spread_buy_ > synth_spread_sell_ ) {
        synth_spread_buy_flag_ = true;
        synth_spread_sell_flag_ = false;
      }
    }
    */
}

void ArbTradingTodTom::OnMarketUpdate(const unsigned int security_id, const MarketUpdateInfo& market_update_info) {
  if (!is_ready_) {
    if (tod_smv_.is_ready_complex(2) && tom_smv_.is_ready_complex(2)) {
      is_ready_ = true;
    }
  } else {
    UpdateVariables(security_id);
  }

  if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "OpenPosition." << GetCurrentMarket() << DBGLOG_ENDL_FLUSH;
  }

  TradingLogic(security_id);
}

void ArbTradingTodTom::OnTradePrint(const unsigned int security_id, const TradePrintInfo& trade_print_info,
                                    const MarketUpdateInfo& market_update_info) {
  if (!is_ready_) {
    if (tod_smv_.is_ready_complex(2) && tom_smv_.is_ready_complex(2)) {
      is_ready_ = true;
    }
  } else {
    UpdateVariables(security_id);
  }

  if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "OpenPosition. ";

    if ((int)security_id == tod_secid_) {
      dbglogger_ << tod_smv_.secname() << " ";
    } else if ((int)security_id == tom_secid_) {
      dbglogger_ << tom_smv_.secname() << " ";
    } else if ((int)security_id == tod_tom_secid_) {
      dbglogger_ << tod_tom_smv_.secname() << " ";
    }

    dbglogger_ << trade_print_info.ToString() << GetCurrentMarket() << DBGLOG_ENDL_FLUSH;
  }

  TradingLogic(security_id);
}

// Called from BaseOrderManager, because we have subscribed to execs by calling AddExecutionListener
// OnExec is called before OnPositionChange
// Note: base_pnl writes the trade upon OnExec call and not OnPositionChange call
// Subscribed for both DOL and WDO.
void ArbTradingTodTom::OnExec(const int new_position, const int exec_quantity, const TradeType_t buysell,
                              const double price, const int int_price, const int _security_id_) {
  combined_pnl_->OnExec(new_position, exec_quantity, buysell, price, int_price, _security_id_);
  tod_position_ = combined_pnl_->GetTODPosition();
  tom_position_ = combined_pnl_->GetTOMPosition();
  tod_tom_position_ = combined_pnl_->GetTODTOMPosition();
  ReconcilePositions();
  OrderPlacingLogic();
}

// Called from BaseOrderManager, because we have subscribed to position change by calling AddPositionListener
// Subscribed for both DOL and WDO.
void ArbTradingTodTom::OnPositionChange(int new_position, int position_diff, const unsigned int security_id) {}

void ArbTradingTodTom::ReportResults(HFSAT::BulkFileWriter& trades_writer, bool conservative_close) {
  int tod_volume = tod_om_.trade_volume();
  int tom_volume = tom_om_.trade_volume();
  int tod_tom_volume = tod_tom_om_.trade_volume();
  int total_volume = tod_volume + tom_volume + tod_tom_volume;

  printf("SIMRESULT %d %d %d %d %d %d %d %d\n", (int)combined_pnl_->GetUnrealizedPnl(), total_volume, tod_volume,
         tom_volume, tod_tom_volume, combined_pnl_->GetTODPosition(), combined_pnl_->GetTOMPosition(),
         combined_pnl_->GetTODTOMPosition());
}

void ArbTradingTodTom::ReconcilePositions() {
  spread_adj_tod_position_ = tod_position_ - 100 * tod_tom_position_;
  spread_adj_tom_position_ = tom_position_ + 100 * tod_tom_position_;

  int matched_pos_ = (spread_adj_tom_position_ > 0)
                         ? std::max(0, std::min(spread_adj_tom_position_, -1 * spread_adj_tod_position_))
                         : std::min(0, std::max(spread_adj_tom_position_, -1 * spread_adj_tod_position_));
  accumulated_spread_position_ = matched_pos_;

  if (tod_best_tom_agg_) {
    excess_tod_position_ = spread_adj_tod_position_ + spread_adj_tom_position_;
  } else if (tom_best_tod_agg_) {
    excess_tom_position_ = spread_adj_tom_position_ + spread_adj_tod_position_;
  }

  spread_position_ = matched_pos_ / 100;
}

void ArbTradingTodTom::get_positions(std::vector<std::string>& instrument_vec, std::vector<int>& position_vec) {
  if (abs(combined_pnl_->GetTODPosition()) > 0) {
    instrument_vec.push_back(tod_smv_.secname());
    position_vec.push_back(combined_pnl_->GetTODPosition());
  }

  if (abs(combined_pnl_->GetTOMPosition()) > 0) {
    instrument_vec.push_back(tom_smv_.secname());
    position_vec.push_back(combined_pnl_->GetTOMPosition());
  }

  if (abs(combined_pnl_->GetTODTOMPosition()) > 0) {
    instrument_vec.push_back(tod_tom_smv_.secname());
    position_vec.push_back(combined_pnl_->GetTODTOMPosition());
  }
}

void ArbTradingTodTom::OnWakeUpifRejectDueToFunds() {}

void ArbTradingTodTom::OnMarketDataInterrupted(const unsigned int security_id, const int msecs_since_last_receive) {}

void ArbTradingTodTom::OnMarketDataResumed(const unsigned int security_id) {}

void ArbTradingTodTom::OnGlobalPNLChange(double new_global_PNL) {}

void ArbTradingTodTom::OnOrderChange() {}

void ArbTradingTodTom::OnGlobalPositionChange(const unsigned int security_id, int new_global_position) {}

void ArbTradingTodTom::OnControlUpdate(const ControlMessage& control_message, const char* symbol, const int trader_id) {
  switch (control_message.message_code_) {
    case kControlMessageCodeGetflat: {
      DBGLOG_TIME_CLASS_FUNC << "getflat_due_to_external_getflat_ " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      external_getflat_ = true;
      break;
    }
    case kControlMessageCodeStartTrading: {
      DBGLOG_TIME_CLASS_FUNC << "StartTrading Called " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      external_getflat_ = false;
      break;
    }
    case kControlMessageCodeShowIndicators: {
      DBGLOG_TIME_CLASS_FUNC << "ShowIndicators " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << tod_smv_.secname() << " OM: " << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME << tod_om_.ShowOrders() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << tom_smv_.secname() << " OM: " << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME << tom_om_.ShowOrders() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << tod_tom_smv_.secname() << " OM: " << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME << tom_om_.ShowOrders() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << GetCurrentMarket() << DBGLOG_ENDL_FLUSH;
      /* DBGLOG_TIME_CLASS_FUNC << "NumArbChances: " << num_arb_chances_ << " last_size_available: " << last_arb_size_
      << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << tom_smv_.secname() << " messages: " << tom_om_.SendOrderCount() +
      tom_om_.CxlOrderCount()
        << " " << tod_smv_.secname() << " messages: " << tod_om_.SendOrderCount() + tod_om_.CxlOrderCount() <<
      DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << " " << tod_smv_.secname() << " status: " << tod_mkt_status_
        << " " << tom_smv_.secname() << " status: " << tom_mkt_status_ << DBGLOG_ENDL_FLUSH;*/
      DBGLOG_DUMP;
      break;
      break;
    }
    case kControlMessageCodeShowOrders: {
      DBGLOG_TIME_CLASS_FUNC << "ShowOrders " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << tod_smv_.secname() << " OM: " << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME << tod_om_.ShowOrders() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << tom_smv_.secname() << " OM: " << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME << tom_om_.ShowOrders() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << tod_tom_smv_.secname() << " OM: " << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME << tod_tom_om_.ShowOrders() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << GetCurrentMarket() << DBGLOG_ENDL_FLUSH;
      /*DBGLOG_TIME_CLASS_FUNC << "NumArbChances: " << num_arb_chances_ << " last_size_available: " << last_arb_size_ <<
      DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << tom_smv_.secname() << " messages: " << tom_om_.SendOrderCount() +
      tom_om_.CxlOrderCount()
        << " " << tod_smv_.secname() << " messages: " << tod_om_.SendOrderCount() + tod_om_.CxlOrderCount() <<
      DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << " " << tod_smv_.secname() << " status: " << tod_mkt_status_
        << " " << tom_smv_.secname() << " status: " << tom_mkt_status_ << DBGLOG_ENDL_FLUSH;*/
      DBGLOG_DUMP;
      break;
    }
    case kControlMessageCodeShowParams: {
      break;
    }
    case kControlMessageCodeSetMaxLoss: {
      if (control_message.intval_1_ > param_set_.max_loss_ / FAT_FINGER_FACTOR &&
          control_message.intval_1_ < param_set_.max_loss_ * FAT_FINGER_FACTOR) {
        param_set_.max_loss_ = control_message.intval_1_;
      }

      DBGLOG_TIME_CLASS << "SetMaxLoss " << runtime_id_ << " called for abs_max_loss_ = " << control_message.intval_1_
                        << " and MaxLoss for param set to " << param_set_.max_loss_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      break;
    }
    case kControlMessageCodeSetOpenTradeLoss: {
      if (control_message.intval_1_ > param_set_.max_opentrade_loss_ / FAT_FINGER_FACTOR &&
          control_message.intval_1_ < param_set_.max_opentrade_loss_ * FAT_FINGER_FACTOR) {
        param_set_.max_opentrade_loss_ = control_message.intval_1_;
      }

      DBGLOG_TIME_CLASS << "SetOpenTradeLoss " << runtime_id_
                        << " called for abs_opentrade_loss_ = " << control_message.intval_1_
                        << " and OpenTradeLoss for param set to " << param_set_.max_opentrade_loss_
                        << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      break;
    }
    case kControlMessageCodeSetMaxPosition: {
      if (control_message.intval_1_ > param_set_.max_position_ / FAT_FINGER_FACTOR &&
          control_message.intval_1_ < param_set_.max_position_ * FAT_FINGER_FACTOR) {
        param_set_.max_position_ = std::max(1, control_message.intval_1_);
        param_set_.max_position_original_ = param_set_.max_position_;
      }
      DBGLOG_TIME_CLASS << "SetMaxPosition " << runtime_id_ << " called with " << control_message.intval_1_
                        << " and MaxPosition for param set to " << param_set_.max_position_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    } break;
    default: { break; }
  }
}

bool ArbTradingTodTom::UpdateTarget(double new_target, double targetbias_numbers, int modelmath_index) { return false; }

void ArbTradingTodTom::OnFokReject(const TradeType_t buysell, const double price, const int int_price,
                                   const int size_remaining) {}

void ArbTradingTodTom::TargetNotReady() {}

void ArbTradingTodTom::OnMarketStatusChange(const unsigned int security_id, const MktStatus_t new_market_status) {
  if ((int)security_id == tod_secid_) {
    tod_mkt_status_ = new_market_status;
  } else if ((int)security_id == tom_secid_) {
    tom_mkt_status_ = new_market_status;
  } else if ((int)security_id == tod_tom_secid_) {
    tod_tom_mkt_status_ = new_market_status;
  }

  DBGLOG_TIME_CLASS_FUNC << "MktStatus for security_id: " << security_id << " changed to: " << new_market_status
                         << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;
}
}
