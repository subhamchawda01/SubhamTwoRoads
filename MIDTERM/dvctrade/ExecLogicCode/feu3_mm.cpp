/**
   \file ExecLogicCode/feu3_mm.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/ExecLogic/feu3_mm.hpp"
// exec_logic_code / defines.hpp was empty

namespace HFSAT {

void FEU3MM::CollectORSShortCodes(DebugLogger& _dbglogger_, const std::string& r_dep_shortcode_,
                                  std::vector<std::string>& source_shortcode_vec_,
                                  std::vector<std::string>& ors_source_needed_vec_) {
  HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, r_dep_shortcode_);
  HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, r_dep_shortcode_);

  if (r_dep_shortcode_.compare("FEU3_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_0"));
  }
  if (r_dep_shortcode_.compare("FEU3_1") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_1"));
  }
  if (r_dep_shortcode_.compare("FEU3_2") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_2"));
  }
  if (r_dep_shortcode_.compare("FEU3_3") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_3"));
  }
  if (r_dep_shortcode_.compare("FEU3_4") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_4"));
  }
  if (r_dep_shortcode_.compare("FEU3_5") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_5"));
  }
  if (r_dep_shortcode_.compare("FEU3_6") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_6"));
  }
  if (r_dep_shortcode_.compare("FEU3_7") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_7"));
  }
  if (r_dep_shortcode_.compare("FEU3_8") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_8"));
  }
  if (r_dep_shortcode_.compare("FEU3_9") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_9"));
  }
  if (r_dep_shortcode_.compare("FEU3_10") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_10"));
  }
  if (r_dep_shortcode_.compare("FEU3_11") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_11"));
  }
  if (r_dep_shortcode_.compare("FEU3_12") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_12"));
  }
  if (r_dep_shortcode_.compare("FEU3_13") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_13"));
  }
  if (r_dep_shortcode_.compare("FEU3_14") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_14"));
  }
  if (r_dep_shortcode_.compare("FEU3_15") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_15"));
  }
  if (r_dep_shortcode_.compare("FEU3_16") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_16"));
  }
  if (r_dep_shortcode_.compare("FEU3_17") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_17"));
  }
  if (r_dep_shortcode_.compare("FEU3_18") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_18"));
  }
  if (r_dep_shortcode_.compare("FEU3_19") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("LFI_19"));
  }
}

FEU3MM::FEU3MM(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
               SmartOrderManager& _order_manager_, const SecurityMarketView& _indep_market_view_,
               const std::string& _paramfilename_, const bool _livetrading_,
               MulticastSenderSocket* _p_strategy_param_sender_socket_,
               EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
               const int t_trading_end_utc_mfm_, const int t_runtime_id_,
               const std::vector<std::string> _this_model_source_shortcode_vec_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_),
      indep_market_view_(_indep_market_view_) {
  best_indep_bid_price_ = kInvalidPrice;
  best_indep_bid_int_price_ = kInvalidIntPrice;
  best_indep_bid_size_ = 0;

  best_indep_ask_price_ = kInvalidPrice;
  best_indep_ask_int_price_ = kInvalidIntPrice;
  best_indep_ask_size_ = 0;

  status_ = false;
  indep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  indep_market_view_.subscribe_tradeprints(this);

  if (!param_set_.read_max_bid_ask_order_diff_ || !param_set_.read_min_size_to_quote_) {
    std::cerr << "FEU3 missing max_bid_ask_order_diff_ or min_size_to_quote_ \n";
    exit(1);
  }
}

void FEU3MM::OnTimePeriodUpdate(const int num_pages_to_add_) {
  ProcessTimePeriodUpdate(num_pages_to_add_);

  if (param_set_.read_min_size_to_quote_ && param_set_.read_max_bid_ask_order_diff_) {
    ProcessGetFlat();
    if (watch_.msecs_from_midnight() > trading_start_utc_mfm_ && dep_market_view_.is_ready() &&
        !should_be_getting_flat_ && status_) {
      TradingLogic();
    }
  }
}

inline void FEU3MM::CallPlaceCancelNonBestLevels() {}

void FEU3MM::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (!status_) {
    if (indep_market_view_.is_ready_complex(2)) {
      if (indep_market_view_.bestbid_price() >= dep_market_view_.bestbid_price() &&
          indep_market_view_.bestask_price() <= dep_market_view_.bestask_price() && dep_market_view_.is_ready()) {
        status_ = true;
      }
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

void FEU3MM::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
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

void FEU3MM::TradingLogic() {
  if (!status_) {
    return;
  }

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME << dep_market_view_.bestbid_price() << " " << dep_market_view_.bestask_price() << " |--| "
                << indep_market_view_.market_update_info_.bestbid_price_ << " "
                << indep_market_view_.market_update_info_.bestask_price_ << DBGLOG_ENDL_FLUSH;
  }

  int max_bid_ask_diff_ = param_set_.max_bid_ask_order_diff_;
  int allowance_ = (max_bid_ask_diff_ - (int)(best_indep_ask_int_price_ - best_indep_bid_int_price_));

  int max_bid_distance_ = (int)(0.5 * std::max(0, allowance_));
  int max_ask_distance_ = std::max(0, allowance_ - max_bid_distance_);

  double bias_ =
      (SecurityMarketView::GetPriceFromType(kPriceTypeMktSizeWPrice, indep_market_view_.market_update_info_) -
       SecurityMarketView::GetPriceFromType(kPriceTypeMidprice, indep_market_view_.market_update_info_));

  if (bias_ < 0)  // now giving bid an edge
  {
    max_bid_distance_ = max_bid_distance_ + max_ask_distance_;
    max_ask_distance_ = max_bid_distance_ - max_ask_distance_;
    max_bid_distance_ = max_bid_distance_ - max_ask_distance_;
  }

  int min_bid_distance_ =
      std::max(0.0, (max_bid_distance_ - (ceil(param_set_.min_size_to_quote_ / param_set_.unit_trade_size_)) + 1));
  int min_ask_distance_ =
      std::max(0.0, (max_ask_distance_ - (ceil(param_set_.min_size_to_quote_ / param_set_.unit_trade_size_)) + 1));

  // min_distance can be one / two based on bias just to decrease number of messages
  //
  {
    order_manager_.CancelBidsEqBelowIntPrice(best_indep_bid_int_price_ - max_bid_distance_ - 1);
    order_manager_.CancelBidsEqAboveIntPrice(best_indep_bid_int_price_ - min_bid_distance_ + 1);
  }

  {
    order_manager_.CancelAsksEqBelowIntPrice(best_indep_ask_int_price_ + max_ask_distance_ + 1);
    order_manager_.CancelAsksEqAboveIntPrice(best_indep_ask_int_price_ + min_ask_distance_ - 1);
  }

  for (int _t_bid_int_price_ = (int)best_indep_bid_int_price_ - max_bid_distance_;
       _t_bid_int_price_ <= (int)best_indep_bid_int_price_ - min_bid_distance_; _t_bid_int_price_++) {
    if ((int)(my_position_ + order_manager_.SumBidSizes() + (2 * param_set_.unit_trade_size_) <
              param_set_.worst_case_position_)) {
      if (order_manager_.GetTotalBidSizeOrderedAtIntPx(_t_bid_int_price_) == 0) {
        unsigned int size_to_be_placed_ =
            std::min(position_tradevarset_map_[P2TV_zero_idx_].l1bid_trade_size_,
                     std::max(0, (int)param_set_.min_size_to_quote_ - order_manager_.SumBidSizes()));
        if (size_to_be_placed_ == 0) {
          break;
        }
        order_manager_.SendTradeIntPx(_t_bid_int_price_, size_to_be_placed_, kTradeTypeBuy, 'B');
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "SupportingSendTrade B of " << size_to_be_placed_ << " @ "
                                 << (dep_market_view_.min_price_increment() * _t_bid_int_price_)
                                 << " IntPx: " << _t_bid_int_price_
                                 << " level: " << best_indep_bid_int_price_ - _t_bid_int_price_
                                 << " mkt: " << best_indep_bid_size_ << " @ " << best_indep_bid_price_ << " X "
                                 << best_indep_ask_price_ << " @ " << best_indep_ask_size_ << " sumbidsizes "
                                 << order_manager_.SumBidSizes() << " + my_position_ " << my_position_
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  for (int _t_ask_int_price_ = (int)best_indep_ask_int_price_ + max_ask_distance_;
       _t_ask_int_price_ >= (int)best_indep_ask_int_price_ + min_ask_distance_; _t_ask_int_price_--) {
    if ((int)(my_position_ + order_manager_.SumAskSizes() + (2 * param_set_.unit_trade_size_) <
              param_set_.worst_case_position_)) {
      if (order_manager_.GetTotalAskSizeOrderedAtIntPx(_t_ask_int_price_) == 0) {
        unsigned int size_to_be_placed_ =
            std::min(position_tradevarset_map_[P2TV_zero_idx_].l1ask_trade_size_,
                     std::max(0, (int)param_set_.min_size_to_quote_ - order_manager_.SumAskSizes()));
        if (size_to_be_placed_ == 0) {
          break;
        }
        order_manager_.SendTradeIntPx(_t_ask_int_price_, size_to_be_placed_, kTradeTypeSell, 'S');
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "SupportingSendTrade S of " << size_to_be_placed_ << " @ "
                                 << (dep_market_view_.min_price_increment() * _t_ask_int_price_)
                                 << " IntPx: " << _t_ask_int_price_
                                 << " level: " << _t_ask_int_price_ - best_indep_ask_int_price_
                                 << " mkt: " << best_indep_bid_size_ << " @ " << best_indep_bid_price_ << " X "
                                 << best_indep_ask_price_ << " @ " << best_indep_ask_size_ << " sumasksizes "
                                 << order_manager_.SumAskSizes() << " + my_position_ " << my_position_
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }
  return;
}

void FEU3MM::PrintFullStatus() {}
}
