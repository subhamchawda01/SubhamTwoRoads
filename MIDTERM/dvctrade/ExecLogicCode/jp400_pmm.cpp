/**
   \file ExecLogicCode/jp400_pmm.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/ExecLogic/jp400_pmm.hpp"
// exec_logic_code / defines.hpp was empty

namespace HFSAT {

void JP400PMM::CollectORSShortCodes(DebugLogger& _dbglogger_, const std::string& r_dep_shortcode_,
                                    std::vector<std::string>& source_shortcode_vec_,
                                    std::vector<std::string>& ors_source_needed_vec_) {
  HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, r_dep_shortcode_);
  HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, r_dep_shortcode_);

  if (r_dep_shortcode_.compare("JP400_0") == 0 || r_dep_shortcode_.compare("JN400_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("TOPIX_0"));
  }
  // We can add NK,ES also
}

JP400PMM::JP400PMM(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
                   SmartOrderManager& _order_manager_, const SecurityMarketView& _indep_market_view_,
                   const std::string& _paramfilename_, const bool _livetrading_,
                   MulticastSenderSocket* _p_strategy_param_sender_socket_,
                   EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                   const int t_trading_end_utc_mfm_, const int t_runtime_id_,
                   const std::vector<std::string> _this_model_source_shortcode_vec_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_),
      indep_market_view_(_indep_market_view_),
      p_dep_indicator_(NULL),
      p_indep_indicator_(NULL),
      dep_trend_(0.0),
      indep_trend_(0.0) {
  beta_ = 0.090;
  best_indep_bid_price_ = kInvalidPrice;
  best_indep_bid_int_price_ = kInvalidIntPrice;
  best_indep_bid_size_ = 0;

  best_indep_ask_price_ = kInvalidPrice;
  best_indep_ask_int_price_ = kInvalidIntPrice;
  best_indep_ask_size_ = 0;
  indep_dep_int_price_ratio_ = 2;

  status_ = false;
  total_quoted_msecs_ = 0;
  quote_start_time_ = quote_end_time_ = t_trading_start_utc_mfm_;
  is_quoted_ = false;
  projected_price_ = 0;
  bid_place_int_price_ = ask_place_int_price_ = 0;
  indep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  indep_market_view_.subscribe_tradeprints(this);
  dep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  dep_market_view_.subscribe_tradeprints(this);

  p_dep_indicator_ =
      SimpleTrend::GetUniqueInstance(_dbglogger_, _watch_, _dep_market_view_.shortcode(), 300, kPriceTypeMktSizeWPrice);
  p_indep_indicator_ = SimpleTrend::GetUniqueInstance(_dbglogger_, _watch_, indep_market_view_.shortcode(), 300,
                                                      kPriceTypeMktSizeWPrice);

  p_dep_indicator_->add_unweighted_indicator_listener(1u, this);
  p_indep_indicator_->add_unweighted_indicator_listener(2u, this);

  if (!param_set_.read_max_bid_ask_order_diff_ || !param_set_.read_min_size_to_quote_) {
    std::cerr << "JP400_0 missing max_bid_ask_order_diff_ or min_size_to_quote_ \n";
    exit(1);
  }
  param_set_.read_min_size_to_quote_ = false;
}

void JP400PMM::OnTimePeriodUpdate(const int num_pages_to_add_) {
  ProcessTimePeriodUpdate(num_pages_to_add_);
  if (param_set_.read_min_size_to_quote_ && param_set_.read_max_bid_ask_order_diff_) {
    // ProcessGetFlat();
    if (watch_.msecs_from_midnight() > trading_start_utc_mfm_ && dep_market_view_.is_ready() &&
        !should_be_getting_flat_ && status_) {
      TradingLogic();
    }
  }
}

void JP400PMM::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (_indicator_index_ == 1u)
    dep_trend_ = _new_value_;
  else if (_indicator_index_ == 2u)
    indep_trend_ = _new_value_;
  else
    return;

  projected_price_ = p_dep_indicator_->GetMovingAverage() + beta_ * indep_trend_;
  TradingLogic();
}

void JP400PMM::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (!status_) {
    if (indep_market_view_.is_ready_complex(2) && dep_market_view_.is_ready()) {
      projected_price_ = (dep_market_view_.bestbid_price() + dep_market_view_.bestask_price()) / 2.0;
      bid_place_int_price_ = dep_market_view_.bestbid_int_price();
      ask_place_int_price_ = dep_market_view_.bestask_int_price();
      status_ = true;
    } else {
      DBGLOG_TIME << "status is false " << dep_market_view_.bestbid_price() << " " << dep_market_view_.bestask_price()
                  << " |--| " << indep_market_view_.market_update_info_.bestbid_price_ << " "
                  << indep_market_view_.market_update_info_.bestask_price_ << DBGLOG_ENDL_FLUSH;
      if (!dep_market_view_.is_ready()) {
        DBGLOG_TIME << "status is false dep is not ready " << DBGLOG_ENDL_FLUSH;
      }
      // return ;
    }
  }

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && !livetrading_) {
    if (last_update_ttime_.tv_sec == 0 && last_update_ttime_.tv_usec == 0) {
      last_update_ttime_ = watch_.tv();
    } else {
      HFSAT::ttime_t t_diff_ = watch_.tv() - last_update_ttime_;
      last_update_ttime_ = watch_.tv();
      if ((watch_.msecs_from_midnight() > (trading_start_utc_mfm_ - 5000)) &&
          (watch_.msecs_from_midnight() < trading_end_utc_mfm_)) {
        HFSAT::usleep(t_diff_.tv_sec * 1000000 + t_diff_.tv_usec);
        HFSAT::usleep(t_diff_.tv_sec * 1000000 + t_diff_.tv_usec);
      }
    }
  }
  if (_security_id_ == dep_market_view_.security_id()) {
    NonSelfMarketUpdate();
  } else {
    best_indep_bid_price_ = indep_market_view_.bestbid_price();
    best_indep_bid_int_price_ = indep_market_view_.bestbid_int_price();
    best_indep_bid_size_ = indep_market_view_.bestbid_size();

    best_indep_ask_price_ = indep_market_view_.bestask_price();
    best_indep_ask_int_price_ = indep_market_view_.bestask_int_price();
    best_indep_ask_size_ = indep_market_view_.bestask_size();
  }
  TradingLogic();
}

void JP400PMM::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                            const MarketUpdateInfo& _market_update_info_) {
  if (!status_) {
    return;
  }

  if (_security_id_ == dep_market_view_.security_id()) {
    NonSelfMarketUpdate();
  } else {
    best_indep_bid_price_ = indep_market_view_.bestbid_price();
    best_indep_bid_int_price_ = indep_market_view_.bestbid_int_price();
    best_indep_bid_size_ = indep_market_view_.bestbid_size();

    best_indep_ask_price_ = indep_market_view_.bestask_price();
    best_indep_ask_int_price_ = indep_market_view_.bestask_int_price();
    best_indep_ask_size_ = indep_market_view_.bestask_size();
  }

  if (!getflat_due_to_max_opentrade_loss_) {
    if (order_manager_.base_pnl().opentrade_unrealized_pnl() < -max_opentrade_loss_) {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << "getflat_due_to_max_opentrade_loss_ ! Current opentradepnl : "
                    << order_manager_.base_pnl().opentrade_unrealized_pnl() << " < " << -max_opentrade_loss_
                    << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
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
                   << " getflat_due_to_max_opentrade_loss_: " << order_manager_.base_pnl().opentrade_unrealized_pnl()
                   << " product " << dep_market_view_.shortcode() << " = " << dep_market_view_.secname() << " on "
                   << hostname_ << "\n";

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
      getflat_due_to_max_opentrade_loss_ = true;
      last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_ = watch_.msecs_from_midnight();
      num_opentrade_loss_hits_++;
    }
  }

  TradingLogic();
}

void JP400PMM::TradingLogic() {
  if (!status_) {
    is_quoted_ = false;
    return;
  }
  if (!ShouldTrade()) {
    GetFlatFokTradingLogic();

    if (is_quoted_) {
      quote_end_time_ = watch_.msecs_from_midnight();
      total_quoted_msecs_ += quote_end_time_ - quote_start_time_;
      is_quoted_ = false;
    }
    return;
  } else if (!ShouldQuote()) {
    order_manager_.CancelAllOrders();
    if (is_quoted_) {
      quote_end_time_ = watch_.msecs_from_midnight();
      total_quoted_msecs_ += quote_end_time_ - quote_start_time_;
      is_quoted_ = false;
    }
    return;
  }
  if (!is_quoted_) {
    quote_start_time_ = watch_.msecs_from_midnight();
    is_quoted_ = true;
  }

  if (!IsProjectedBetweenBidAsk()) {
    if (projected_price_ >= dep_market_view_.bestask_price()) {
      bid_place_int_price_ = dep_market_view_.bestbid_int_price();
      ask_place_int_price_ = dep_market_view_.bestbid_int_price() + ((int)param_set_.max_bid_ask_order_diff_);
    } else if (projected_price_ <= dep_market_view_.bestbid_price()) {
      ask_place_int_price_ = dep_market_view_.bestask_int_price();
      bid_place_int_price_ = dep_market_view_.bestask_int_price() - ((int)param_set_.max_bid_ask_order_diff_);
    }
  } else {
    if (my_position_ >= param_set_.unit_trade_size_) {
      ask_place_int_price_ = dep_market_view_.bestask_int_price();
      bid_place_int_price_ = dep_market_view_.bestask_int_price() - ((int)param_set_.max_bid_ask_order_diff_);
    } else if (my_position_ <= -param_set_.unit_trade_size_) {
      bid_place_int_price_ = dep_market_view_.bestbid_int_price();
      ask_place_int_price_ = dep_market_view_.bestbid_int_price() + ((int)param_set_.max_bid_ask_order_diff_);
    } else {
      int spread = dep_market_view_.bestask_int_price() - dep_market_view_.bestbid_int_price();
      if (spread == 4) {
        bid_place_int_price_ = dep_market_view_.bestbid_int_price();
        ask_place_int_price_ = dep_market_view_.bestask_int_price();
      } else if (spread == 3) {
        if (dep_market_view_.mkt_size_weighted_price() > dep_market_view_.mid_price()) {
          bid_place_int_price_ = dep_market_view_.bestbid_int_price();
          ask_place_int_price_ = dep_market_view_.bestbid_int_price() + ((int)param_set_.max_bid_ask_order_diff_);
        } else {
          ask_place_int_price_ = dep_market_view_.bestask_int_price();
          bid_place_int_price_ = dep_market_view_.bestask_int_price() - ((int)param_set_.max_bid_ask_order_diff_);
        }
      } else if (spread == 2) {
        bid_place_int_price_ =
            dep_market_view_.bestbid_int_price() - (((int)param_set_.max_bid_ask_order_diff_) / 2 - 1);
        ask_place_int_price_ =
            dep_market_view_.bestask_int_price() + (((int)param_set_.max_bid_ask_order_diff_) / 2 - 1);
      } else if (spread == 1) {
        if (dep_market_view_.mkt_size_weighted_price() > dep_market_view_.mid_price()) {
          bid_place_int_price_ =
              dep_market_view_.bestbid_int_price() - (((int)param_set_.max_bid_ask_order_diff_) / 2 - 1);
          ask_place_int_price_ = dep_market_view_.bestask_int_price() + (((int)param_set_.max_bid_ask_order_diff_) / 2);
        } else {
          ask_place_int_price_ =
              dep_market_view_.bestask_int_price() + (((int)param_set_.max_bid_ask_order_diff_) / 2 - 1);
          bid_place_int_price_ = dep_market_view_.bestbid_int_price() - (((int)param_set_.max_bid_ask_order_diff_) / 2);
        }
      }
    }
  }

  {
    order_manager_.CancelBidsEqBelowIntPrice(bid_place_int_price_ - 1);
    order_manager_.CancelBidsEqAboveIntPrice(bid_place_int_price_ + 1);
  }

  {
    order_manager_.CancelAsksEqBelowIntPrice(ask_place_int_price_ + 1);
    order_manager_.CancelAsksEqAboveIntPrice(ask_place_int_price_ - 1);
  }

  if (my_position_ + order_manager_.SumBidSizes() + param_set_.unit_trade_size_ <= param_set_.worst_case_position_) {
    if (order_manager_.GetTotalBidSizeOrderedAtIntPx(bid_place_int_price_) < (int)param_set_.min_size_to_quote_) {
      int size_to_be_placed_ =
          std::min((int)param_set_.min_size_to_quote_,
                   std::max(0, ((int)param_set_.min_size_to_quote_) - order_manager_.SumBidSizes()));
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "min size to quote " << (int)param_set_.min_size_to_quote_ << " Num orders "
                               << order_manager_.GetTotalBidSizeOrderedAtIntPx(bid_place_int_price_)
                               << " size_to_be_placed_ " << size_to_be_placed_ << " SumBidSizes "
                               << order_manager_.SumBidSizes() << DBGLOG_ENDL_FLUSH;
      }
      if (size_to_be_placed_ > 0) {
        order_manager_.SendTradeIntPx(bid_place_int_price_, size_to_be_placed_, kTradeTypeBuy, 'B');
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "SupportingSendTrade B of " << size_to_be_placed_ << " @ "
                                 << (dep_market_view_.min_price_increment() * bid_place_int_price_)
                                 << " IntPx: " << bid_place_int_price_ << " mkt: " << dep_market_view_.bestbid_size()
                                 << " @ " << dep_market_view_.bestbid_int_price() << " X "
                                 << dep_market_view_.bestask_int_price() << " @ " << dep_market_view_.bestask_size()
                                 << " sumbidsizes " << order_manager_.SumBidSizes() << " + my_position_ "
                                 << my_position_ << " Num orders "
                                 << order_manager_.GetTotalBidSizeOrderedAtIntPx(bid_place_int_price_)
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    } else if (order_manager_.GetTotalBidSizeOrderedAtIntPx(bid_place_int_price_) >
               (int)param_set_.min_size_to_quote_) {
      order_manager_.CancelAllBidOrders();
      order_manager_.SendTradeIntPx(bid_place_int_price_, (int)param_set_.min_size_to_quote_, kTradeTypeBuy, 'B');
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "Cancelled and then SupportingSendTrade B of " << param_set_.min_size_to_quote_
                               << " @ " << (dep_market_view_.min_price_increment() * bid_place_int_price_)
                               << " IntPx: " << bid_place_int_price_ << " mkt: " << dep_market_view_.bestbid_size()
                               << " @ " << dep_market_view_.bestbid_int_price() << " X "
                               << dep_market_view_.bestask_int_price() << " @ " << dep_market_view_.bestask_size()
                               << " sumbidsizes " << order_manager_.SumBidSizes() << " + my_position_ " << my_position_
                               << " Num orders " << order_manager_.GetTotalBidSizeOrderedAtIntPx(bid_place_int_price_)
                               << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  if (-my_position_ + order_manager_.SumAskSizes() + (param_set_.unit_trade_size_) <= param_set_.worst_case_position_) {
    if (order_manager_.GetTotalAskSizeOrderedAtIntPx(ask_place_int_price_) < (int)param_set_.min_size_to_quote_) {
      int size_to_be_placed_ =
          std::min((int)param_set_.min_size_to_quote_,
                   std::max(0, ((int)param_set_.min_size_to_quote_) - order_manager_.SumAskSizes()));
      if (size_to_be_placed_ > 0) {
        order_manager_.SendTradeIntPx(ask_place_int_price_, size_to_be_placed_, kTradeTypeSell, 'B');
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "SupportingSendTrade S of " << size_to_be_placed_ << " @ "
                                 << (dep_market_view_.min_price_increment() * ask_place_int_price_)
                                 << " IntPx: " << ask_place_int_price_ << " mkt: " << dep_market_view_.bestbid_size()
                                 << " @ " << dep_market_view_.bestbid_int_price() << " X "
                                 << dep_market_view_.bestask_int_price() << " @ " << dep_market_view_.bestask_size()
                                 << " sumasksizes " << order_manager_.SumAskSizes() << " + my_position_ "
                                 << my_position_ << " Num orders "
                                 << order_manager_.GetTotalAskSizeOrderedAtIntPx(ask_place_int_price_)
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    } else if (order_manager_.GetTotalAskSizeOrderedAtIntPx(ask_place_int_price_) >
               (int)param_set_.min_size_to_quote_) {
      order_manager_.CancelAllAskOrders();
      order_manager_.SendTradeIntPx(ask_place_int_price_, (int)param_set_.min_size_to_quote_, kTradeTypeSell, 'B');
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "Cancelled and SupportingSendTrade S of " << param_set_.min_size_to_quote_ << " @ "
                               << (dep_market_view_.min_price_increment() * ask_place_int_price_)
                               << " IntPx: " << ask_place_int_price_ << " mkt: " << dep_market_view_.bestbid_size()
                               << " @ " << dep_market_view_.bestbid_int_price() << " X "
                               << dep_market_view_.bestask_int_price() << " @ " << dep_market_view_.bestask_size()
                               << " sumasksizes " << order_manager_.SumAskSizes() << " + my_position_ " << my_position_
                               << " Num orders " << order_manager_.GetTotalAskSizeOrderedAtIntPx(ask_place_int_price_)
                               << DBGLOG_ENDL_FLUSH;
      }
    }
  }
  return;
}

void JP400PMM::PrintFullStatus() {}

inline void JP400PMM::CallPlaceCancelNonBestLevels() {}

bool JP400PMM::IsProjectedBetweenBidAsk() {
  if (projected_price_ >= dep_market_view_.bestbid_price() && projected_price_ <= dep_market_view_.bestask_price())
    return true;
  return false;
}

bool JP400PMM::ShouldTrade() {
  // add check of max pos/loss ?
  // add check of volatility
  // add check of spread
  // GetFlatFokTradingLogic();
  if (indep_market_view_.bestask_int_price() - indep_market_view_.bestbid_int_price() >= 2) {
    return false;
  } else if (dep_market_view_.bestask_int_price() - dep_market_view_.bestbid_int_price() >=
             ((int)param_set_.max_bid_ask_order_diff_) - 1) {
    return false;
  } else if (watch_.msecs_from_midnight() >= trading_end_utc_mfm_ ||
             watch_.msecs_from_midnight() <= trading_start_utc_mfm_) {
    return false;
  } else if (projected_price_ - dep_market_view_.bestbid_price() >=
                 ((int)param_set_.max_bid_ask_order_diff_) * dep_market_view_.min_price_increment() &&
             my_position_ < 0) {
    return false;
  } else if (projected_price_ - dep_market_view_.bestask_price() <=
                 -((int)param_set_.max_bid_ask_order_diff_) * dep_market_view_.min_price_increment() &&
             my_position_ > 0) {
    return false;
  } else if (ShouldBeGettingFlat()) {
    return false;
  }
  return true;
}

bool JP400PMM::ShouldQuote() {
  if (projected_price_ - dep_market_view_.bestbid_price() >
          ((int)param_set_.max_bid_ask_order_diff_) * dep_market_view_.min_price_increment() ||
      projected_price_ - dep_market_view_.bestask_price() <
          -((int)param_set_.max_bid_ask_order_diff_) * dep_market_view_.min_price_increment() ||
      projected_price_ <= 0 || dep_market_view_.bestbid_price() <= 0) {
    return false;
  }
  return true;
}

void JP400PMM::ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool conservative_close_) {
  if (livetrading_) {
    printf("SIMRESULT %d %d %d %d %d %d\n",
           (int)order_manager_.base_pnl().ReportConservativeTotalPNL(conservative_close_),
           order_manager_.trade_volume(), order_manager_.SupportingOrderFilledPercent(),
           order_manager_.BestLevelOrderFilledPercent(), order_manager_.AggressiveOrderFilledPercent(),
           order_manager_.ImproveOrderFilledPercent());
  } else {
    {
      int t_pnl_ = (int)(order_manager_.base_pnl().ReportConservativeTotalPNL(true));
      printf("SIMRESULT %d %d %d %d %d %d %d\n", t_pnl_, order_manager_.trade_volume(),
             order_manager_.SupportingOrderFilledPercent(), order_manager_.BestLevelOrderFilledPercent(),
             order_manager_.AggressiveOrderFilledPercent(), order_manager_.ImproveOrderFilledPercent(),
             total_quoted_msecs_ / 4650);

      if ((!strncmp(getenv("USER"), "sghosh", strlen("rkumar")))) {
        if (num_stdev_calls_ > 0) {
          printf("AVG_STDEV %f \n", sum_stdev_calls_ / num_stdev_calls_);
        }
      }

      trades_writer_ << "SIMRESULT " << runtime_id_ << " " << t_pnl_ << " " << order_manager_.trade_volume() << " "
                     << order_manager_.SupportingOrderFilledPercent() << " "
                     << order_manager_.BestLevelOrderFilledPercent() << " "
                     << order_manager_.AggressiveOrderFilledPercent() << " "
                     << order_manager_.ImproveOrderFilledPercent() << " " << improve_cancel_counter_ << " "
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

      if ((!strncmp(getenv("USER"), "sghosh", strlen("sghosh"))) ||
          (!strncmp(getenv("USER"), "mayank", strlen("mayank"))) ||
          (!strncmp(getenv("USER"), "kishenp", strlen("kishenp")))) {
        printf("FILLRATIO %d\n", order_manager_.AllOrderFilledPercent());
        printf("SEND-MSG-COUNT %7d | CXL-MSG-COUNT %7d | TOTAL-MSG-COUNT %7d\n", order_manager_.SendOrderCount(),
               order_manager_.CxlOrderCount(), (order_manager_.SendOrderCount() + order_manager_.CxlOrderCount()));
      }
    }
  }
}
}
