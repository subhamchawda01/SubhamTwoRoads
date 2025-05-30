/**
   \file ExecLogicCode/arb_trading.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "dvctrade/ExecLogic/directional_synthetic_trading.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"

#define FAT_FINGER_FACTOR 5
namespace HFSAT {

void DirectionalSyntheticTrading::CollectORSShortCodes(DebugLogger& dbglogger, const std::string& strategy_name,
                                                       const std::string& dep_shortcode,
                                                       std::vector<std::string>& source_shortcode_vec,
                                                       std::vector<std::string>& ors_source_needed_vec,
                                                       std::vector<std::string>& dependant_shortcode_vec) {
  if (strategy_name != StrategyName()) {
    return;
  }

  HFSAT::SyntheticSecurityManager& synthetic_security_manager_ = HFSAT::SyntheticSecurityManager::GetUniqueInstance();
  if (synthetic_security_manager_.IsSyntheticSecurity(dep_shortcode)) {
    std::vector<std::string> t_constituent_vec_ = synthetic_security_manager_.GetConstituentSHC(dep_shortcode);
    for (unsigned int const_shc_idx_ = 0; const_shc_idx_ < t_constituent_vec_.size(); const_shc_idx_++) {
      VectorUtils::UniqueVectorAdd(source_shortcode_vec, t_constituent_vec_[const_shc_idx_]);
      VectorUtils::UniqueVectorAdd(ors_source_needed_vec, t_constituent_vec_[const_shc_idx_]);
      VectorUtils::UniqueVectorAdd(dependant_shortcode_vec, t_constituent_vec_[const_shc_idx_]);
    }
  } else {
    std::cerr << dep_shortcode << " Isn't a synthetic security" << '\n';
  }
}

void DirectionalSyntheticTrading::GetDepShortcodes(const std::string& dep_shortcode,
                                                   std::vector<std::string>& dep_shortcode_vec) {
  HFSAT::SyntheticSecurityManager& synthetic_security_manager_ = HFSAT::SyntheticSecurityManager::GetUniqueInstance();

  if (synthetic_security_manager_.IsSyntheticSecurity(dep_shortcode)) {
    std::vector<std::string> t_constituent_vec_ = synthetic_security_manager_.GetConstituentSHC(dep_shortcode);
    if (t_constituent_vec_.size() != 2) {
      std::cerr << dep_shortcode << " doesn't exactly 2 shortcodes" << std::endl;
    }
    for (unsigned int const_shc_idx_ = 0; const_shc_idx_ < t_constituent_vec_.size(); const_shc_idx_++) {
      VectorUtils::UniqueVectorAdd(dep_shortcode_vec, t_constituent_vec_[const_shc_idx_]);
    }
  }
}

DirectionalSyntheticTrading::DirectionalSyntheticTrading(
    DebugLogger& dbglogger, const Watch& watch, const SecurityMarketView& _dep_market_view_,
    const SecurityMarketView* leg1_smv, const SecurityMarketView* leg2_smv, SmartOrderManager* leg1_order_manager,
    SmartOrderManager* leg2_order_manager, const std::string& paramfilename, const bool livetrading, ttime_t start_time,
    ttime_t end_time, int runtime_id, PnlWriter* pnl_writer)
    : ExecInterface(dbglogger, watch, *leg1_smv, *leg1_order_manager, paramfilename, livetrading),
      dep_market_view_(_dep_market_view_),
      synth_sec_id_(_dep_market_view_.security_id()),
      map_pos_increment_(1),
      map_pos_increment_vec_(param_set_vec_.size(), 1),
      position_tradevarset_map_(2 * MAX_POS_MAP_SIZE + 1),
      position_tradevarset_map_vec_(param_set_vec_.size(), position_tradevarset_map_),
      P2TV_zero_idx_(MAX_POS_MAP_SIZE),
      current_tradevarset_(),
      current_bid_tradevarset_(),
      current_ask_tradevarset_(),
      current_bid_keep_tradevarset_(),
      current_ask_keep_tradevarset_(),
      current_position_tradevarset_map_index_(MAX_POS_MAP_SIZE),
      param_index_to_use_(param_set_vec_.size() - 1),
      my_position_(0),
      spread_adj_leg1_pos_(0),
      spread_adj_leg2_pos_(0),
      is_synth_flat_(true),
      is_outright_risk_(false),
      best_nonself_bid_price_(0.0),
      best_nonself_bid_int_price_(0),
      best_nonself_bid_size_(0),
      best_nonself_ask_price_(0.0),
      best_nonself_ask_int_price_(0),
      best_nonself_ask_size_(0),
      is_dv01_not_updated_(true),
      is_di_spread_(false),
      start_time_(start_time),
      end_time_(end_time),
      tradingdate_(watch_.YYYYMMDD()),
      runtime_id_(runtime_id),
      is_ready_(false),
      target_price_(0),
      targetbias_numbers_(0),
      top_bid_place_(false),
      top_bid_keep_(false),
      top_ask_place_(false),
      top_ask_keep_(false),
      top_bid_improve_(false),
      top_ask_lift_(false),
      top_ask_improve_(false),
      top_bid_hit_(false),
      placed_bids_this_round_(false),
      canceled_bids_this_round_(false),
      closeout_extra_pos_long_(false),
      placed_asks_this_round_(false),
      canceled_asks_this_round_(false),
      closeout_extra_pos_short_(false),
      bid_improve_keep_(false),
      ask_improve_keep_(false),
      last_buy_msecs_(0),
      last_sell_msecs_(0),
      combined_pnl_(new CombinedPnlSynthetic(dbglogger, watch, *leg1_smv, *leg2_smv, pnl_writer)),
      min_unrealized_pnl_(0.0),
      external_getflat_(livetrading),
      getflat_due_to_maxloss_(false),
      getflat_due_to_mkt_status_(false),
      is_affined_(false),
      last_affine_attempt_msecs_(0),
      is_alert_raised_(false) {
  smv_vec_.push_back(leg1_smv);
  smv_vec_.push_back(leg2_smv);
  leg_om_vec_.push_back(leg1_order_manager);
  leg_om_vec_.push_back(leg2_order_manager);

  for (auto i = 0u; i < smv_vec_.size(); i++) {
    secid_to_idx_map_[smv_vec_[i]->security_id()] = i;
    leg_positions_vec_.push_back(0);
    leg_max_position_vec_.push_back(param_set_.max_position_);
    leg_bid_int_price_vec_.push_back(0);
    leg_ask_int_price_vec_.push_back(0);
    leg_bid_price_vec_.push_back(0.0);
    leg_ask_price_vec_.push_back(0.0);
    leg_bid_size_vec_.push_back(0);
    leg_ask_size_vec_.push_back(0);
    leg_bid_orders_vec_.push_back(0);
    leg_ask_orders_vec_.push_back(0);
    uts_vec_.push_back(param_set_.unit_trade_size_);  // will update later as per dv01
    leg_bid_trade_size_vec_.push_back(param_set_.unit_trade_size_);
    leg_ask_trade_size_vec_.push_back(param_set_.unit_trade_size_);
    leg_bid_imp_trade_size_vec_.push_back(0);
    leg_ask_imp_trade_size_vec_.push_back(0);
    dv01_vec_.push_back(-1.0);
    shortcode_vec_.push_back(smv_vec_[i]->shortcode());
    last_agg_buy_msecs_vec_.push_back(0);
    last_agg_sell_msecs_vec_.push_back(0);
    leg_mkt_status_vec_.push_back(kMktTradingStatusOpen);
  }

  if (dep_market_view_.shortcode().find("SPDI1") != std::string::npos) {
    is_di_spread_ = true;
  }

  dep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);

  smv_vec_[0]->subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  smv_vec_[1]->subscribe_price_type(this, kPriceTypeMktSizeWPrice);

  smv_vec_[0]->subscribe_MktStatus(this);
  smv_vec_[1]->subscribe_MktStatus(this);
  watch_.subscribe_FifteenMinutesPeriod(this);

  leg_om_vec_[1]->AddPositionChangeListener(this);
  leg_om_vec_[1]->AddExecutionListener(this);
  leg_om_vec_[1]->AddCancelRejectListener(this);
  leg_om_vec_[1]->AddRejectDueToFundsListener(this);
  leg_om_vec_[1]->AddFokFillRejectListener(this);

  tradevarset_builder_ = new TradeVarSetBuilder(dbglogger_, watch_, livetrading_);
  BuildTradeVarSets();
}

void DirectionalSyntheticTrading::GetFlatLogic() {
  if (is_synth_flat_) {
    leg_om_vec_[0]->CancelAllOrders();
    leg_om_vec_[1]->CancelAllOrders();
  } else {
    GetFlat(0);
    GetFlat(1);
  }
}

void DirectionalSyntheticTrading::GetFlat(unsigned int _index_) {
  if (leg_positions_vec_[_index_] == 0) {
    leg_om_vec_[_index_]->CancelAllOrders();
  } else if (leg_positions_vec_[_index_] > 0) {
    leg_om_vec_[_index_]->CancelAllBidOrders();

    leg_om_vec_[_index_]->CancelAsksBelowIntPrice(leg_ask_int_price_vec_[_index_]);
    int pos_to_close_ = leg_positions_vec_[_index_];
    int trade_size_required_ = MathUtils::GetFlooredMultipleOf(std::min(pos_to_close_, uts_vec_[_index_]),
                                                               smv_vec_[_index_]->min_order_size());

    int size_ordered_ = leg_om_vec_[_index_]->GetTotalAskSizeEqAboveIntPx(leg_ask_int_price_vec_[_index_]);

    if (size_ordered_ < trade_size_required_) {
      leg_om_vec_[_index_]->SendTrade(leg_ask_price_vec_[_index_], leg_ask_int_price_vec_[_index_],
                                      trade_size_required_ - size_ordered_, kTradeTypeSell, 'B');
    }
  } else if (leg_positions_vec_[_index_] < 0) {
    leg_om_vec_[_index_]->CancelAllAskOrders();

    leg_om_vec_[_index_]->CancelBidsBelowIntPrice(leg_bid_int_price_vec_[_index_]);
    int pos_to_close_ = -leg_positions_vec_[_index_];
    int trade_size_required_ = MathUtils::GetFlooredMultipleOf(std::min(pos_to_close_, uts_vec_[_index_]),
                                                               smv_vec_[_index_]->min_order_size());

    int size_ordered_ = leg_om_vec_[_index_]->GetTotalBidSizeEqAboveIntPx(leg_bid_int_price_vec_[_index_]);

    if (size_ordered_ < trade_size_required_) {
      leg_om_vec_[_index_]->SendTrade(leg_bid_price_vec_[_index_], leg_bid_int_price_vec_[_index_],
                                      trade_size_required_ - size_ordered_, kTradeTypeBuy, 'B');
    }
  }
}

// TODO: make this check lighter
bool DirectionalSyntheticTrading::ValidPrices() {
  if (leg_bid_int_price_vec_[0] == kInvalidIntPrice || leg_ask_int_price_vec_[0] == kInvalidIntPrice ||
      leg_bid_int_price_vec_[1] == kInvalidIntPrice || leg_ask_int_price_vec_[1] == kInvalidIntPrice ||
      leg_bid_int_price_vec_[0] >= leg_ask_int_price_vec_[0] ||
      leg_bid_int_price_vec_[1] >= leg_ask_int_price_vec_[1] || leg_bid_size_vec_[0] == 0 ||
      leg_ask_size_vec_[0] == 0 || leg_bid_size_vec_[1] == 0 || leg_ask_size_vec_[1] == 0 ||
      leg_bid_orders_vec_[0] == 0 || leg_ask_orders_vec_[0] == 0 || leg_bid_orders_vec_[1] == 0 ||
      leg_ask_orders_vec_[1] == 0) {
    return false;
  }

  return true;
}

/*
std::string DirectionalSyntheticTrading::GetCurrentMarket() {
  std::ostringstream oss;
  oss << " Position: " << my_position() << "(" << combined_pnl_->GetDOLPosition() << "+"
      << combined_pnl_->GetWDOPosition() << ").";
  oss << " DOL: [" << leg_bid_size_vec_[0] << " " << leg_bid_int_price_vec_[0] << " * " << leg_ask_int_price_vec_[0] <<
" " << leg_ask_size_vec_[0]
      << "]"
      << " WDO: [" << leg_bid_size_vec_[1] << " " << leg_bid_int_price_vec_[0] << " * " << leg_ask_int_price_vec_[0] <<
" " << leg_ask_size_vec_[1]
      << "]";
  oss << " Pnl: " << combined_pnl_->GetUnrealizedPnl();

  return oss.str();
}
*/

bool DirectionalSyntheticTrading::ShouldGetFlat() {
  if (!smv_vec_[0]->is_ready() || !smv_vec_[1]->is_ready()) {
    return true;
  }

  if (leg_mkt_status_vec_[0] != kMktTradingStatusOpen || leg_mkt_status_vec_[1] != kMktTradingStatusOpen) {
    if (!getflat_due_to_mkt_status_) {
      getflat_due_to_mkt_status_ = true;
      DBGLOG_TIME << "getflat_due_to_mkt_status. Leg1 mktstatus: " << leg_mkt_status_vec_[0]
                  << " Leg2 mktstatus: " << leg_mkt_status_vec_[1] << DBGLOG_ENDL_FLUSH;
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

  if (combined_pnl_->GetUnrealizedPnl() < -param_set_.max_loss_) {
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
              << " DirectionalSyntheticTrading products " << smv_vec_[0]->shortcode() << " (" << smv_vec_[0]->secname()
              << ") and " << smv_vec_[0]->shortcode() << " (" << smv_vec_[1]->secname() << ")"
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

bool DirectionalSyntheticTrading::IsLastIOCOrderIncomplete() {
  if (leg_om_vec_[1]->IOCOrderExists() || leg_om_vec_[0]->IOCOrderExists()) {
    return true;
  }
  return false;
}

void DirectionalSyntheticTrading::TradingLogic() {
  // change the if condition to optimize the
  // leg that is latency sensitive
  if (ShouldGetFlat()) {
    GetFlatLogic();
    return;
  }

  SetBuyDirectives();
  SetSellDirectives();

  placed_bids_this_round_ = false;
  canceled_bids_this_round_ = false;
  closeout_extra_pos_long_ = false;

  if (param_set_.synth_agg_exec_) {
    if (top_bid_place_) {
      if (is_outright_risk_) {
        PlaceCancelBuyImpOrders();
      } else {
        PlaceCancelBuyAggOrders();
      }
    } else {
      if (is_outright_risk_ && top_bid_keep_) {
        PlaceCancelBuyImpOrders();
      } else if (!top_bid_keep_) {
        // Canceling from best price, not the agg price; just to be safe
        leg_om_vec_[0]->CancelBidsEqAboveIntPrice(leg_bid_int_price_vec_[0]);
        leg_om_vec_[1]->CancelAsksEqAboveIntPrice(leg_ask_int_price_vec_[1]);

        if (is_outright_risk_) {
          closeout_extra_pos_long_ = true;
        }
      }
    }
  } else {
    if (top_bid_place_ && is_outright_risk_) {
      PlaceCancelBuyImpOrders();
    }

    if (!placed_bids_this_round_) {
      if (top_bid_place_) {
        PlaceCancelBuyBestOrders();
      } else {
        if (is_outright_risk_ && top_bid_keep_) {
          PlaceCancelBuyImpOrders();
        } else if (!top_bid_keep_) {
          leg_om_vec_[0]->CancelBidsEqAboveIntPrice(leg_bid_int_price_vec_[0]);
          leg_om_vec_[1]->CancelAsksEqAboveIntPrice(leg_ask_int_price_vec_[1]);

          if (is_outright_risk_) {
            closeout_extra_pos_long_ = true;
          }
        }
      }
    }
  }

  // Active ASK order management
  placed_asks_this_round_ = false;
  canceled_asks_this_round_ = false;
  closeout_extra_pos_short_ = false;

  if (param_set_.synth_agg_exec_) {
    if (top_ask_place_) {
      if (is_outright_risk_) {
        PlaceCancelSellImpOrders();
      } else {
        PlaceCancelSellAggOrders();
      }
    } else {
      if (is_outright_risk_ && top_ask_keep_) {
        PlaceCancelSellImpOrders();
      } else if (!top_ask_keep_) {
        // Canceling from best price, not the agg price; just to be safe
        leg_om_vec_[0]->CancelAsksEqAboveIntPrice(leg_ask_int_price_vec_[0]);
        leg_om_vec_[1]->CancelBidsEqAboveIntPrice(leg_bid_int_price_vec_[1]);

        if (is_outright_risk_) {
          closeout_extra_pos_short_ = true;
        }
      }
    }
  } else {
    if (top_ask_place_ && is_outright_risk_) {
      PlaceCancelSellImpOrders();
    }

    if (!placed_asks_this_round_) {
      if (top_ask_place_) {
        PlaceCancelSellBestOrders();
      } else {
        if (is_outright_risk_ && top_ask_keep_) {
          PlaceCancelSellImpOrders();
        } else if (!top_ask_keep_) {
          leg_om_vec_[0]->CancelAsksEqAboveIntPrice(leg_ask_int_price_vec_[0]);
          leg_om_vec_[1]->CancelBidsEqAboveIntPrice(leg_bid_int_price_vec_[1]);

          if (is_outright_risk_) {
            closeout_extra_pos_short_ = true;
          }
        }
      }
    }
  }

  // Closing out extra pos in case we neither want to buy and sell
  if (closeout_extra_pos_long_ && closeout_extra_pos_short_) {
    CancelCloseoutExtraPos();
  }
}

void DirectionalSyntheticTrading::SetBuyDirectives() {
  top_bid_place_ = false;
  top_bid_keep_ = false;

  if (  // check if we have any allowance to buy orders
      (param_set_.max_position_ - my_position_ > 0) &&
      // first check for cooloff_interval
      ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_))) {
    if ((targetbias_numbers_ >= current_bid_tradevarset_.l1bid_place_)) {
      top_bid_place_ = true;
      top_bid_keep_ = true;
    } else {
      if (targetbias_numbers_ >= current_bid_keep_tradevarset_.l1bid_keep_) {
        top_bid_keep_ = true;
      } else {
        top_bid_keep_ = false;
      }
    }
  }
}

void DirectionalSyntheticTrading::SetSellDirectives() {
  top_ask_place_ = false;
  top_ask_keep_ = false;

  if (  // check if we have any allowance to place orders at top level
      (param_set_.max_position_ + my_position_ > 0) &&
      // first check for cooloff_interval
      ((last_sell_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_))) {
    if ((-targetbias_numbers_ >= current_ask_tradevarset_.l1ask_place_)) {
      top_ask_place_ = true;
      top_ask_keep_ = true;
    } else {
      if (-targetbias_numbers_ >= current_ask_keep_tradevarset_.l1ask_keep_) {
        top_ask_keep_ = true;
      } else {
        top_ask_keep_ = false;
      }
    }
  }
}

void DirectionalSyntheticTrading::UpdateVariables(int security_id) {
  if (synth_sec_id_ == security_id) {
    best_nonself_bid_price_ = dep_market_view_.bestbid_price();
    best_nonself_bid_int_price_ = dep_market_view_.bestbid_int_price();
    best_nonself_bid_size_ = dep_market_view_.bestbid_size();

    best_nonself_ask_price_ = dep_market_view_.bestask_price();
    best_nonself_ask_int_price_ = dep_market_view_.bestask_int_price();
    best_nonself_ask_size_ = dep_market_view_.bestask_size();
  } else {
    int t_update_idx_ = secid_to_idx_map_[security_id];
    leg_bid_int_price_vec_[t_update_idx_] = smv_vec_[t_update_idx_]->bestbid_int_price();
    leg_ask_int_price_vec_[t_update_idx_] = smv_vec_[t_update_idx_]->bestask_int_price();
    leg_bid_price_vec_[t_update_idx_] = smv_vec_[t_update_idx_]->bestbid_price();
    leg_ask_price_vec_[t_update_idx_] = smv_vec_[t_update_idx_]->bestask_price();
    leg_bid_size_vec_[t_update_idx_] = smv_vec_[t_update_idx_]->bestbid_size();
    leg_ask_size_vec_[t_update_idx_] = smv_vec_[t_update_idx_]->bestask_size();
    leg_bid_orders_vec_[t_update_idx_] = smv_vec_[t_update_idx_]->bestbid_ordercount();
    leg_ask_orders_vec_[t_update_idx_] = smv_vec_[t_update_idx_]->bestask_ordercount();

    if (is_dv01_not_updated_) {
      if (is_di_spread_) {
        dv01_vec_[t_update_idx_] = HFSAT::CurveUtils::stirs_fut_dv01(smv_vec_[t_update_idx_]->shortcode(), tradingdate_,
                                                                     leg_bid_price_vec_[t_update_idx_]);
      } else {
        dv01_vec_[t_update_idx_] = 1.0;
      }

      is_dv01_not_updated_ = HFSAT::VectorUtils::LinearSearchValue(dv01_vec_, -1.0);

      if (!is_dv01_not_updated_) {
        UpdateDV01Vars();
        UpdatePositions();
      }
    }
  }
}

void DirectionalSyntheticTrading::OnMarketUpdate(const unsigned int security_id,
                                                 const MarketUpdateInfo& market_update_info) {
  UpdateVariables(security_id);
  double t_unrealized_pnl_ = combined_pnl_->GetUnrealizedPnl();

  min_unrealized_pnl_ = min_unrealized_pnl_ < t_unrealized_pnl_ ? min_unrealized_pnl_ : t_unrealized_pnl_;

  if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
    // DBGLOG_TIME_CLASS_FUNC << "OpenPosition." << GetCurrentMarket() << DBGLOG_ENDL_FLUSH;
  }
}

void DirectionalSyntheticTrading::OnTradePrint(const unsigned int security_id, const TradePrintInfo& trade_print_info,
                                               const MarketUpdateInfo& market_update_info) {
  UpdateVariables(security_id);
}

/*
void IndicatorHelper::OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value) {
  if ((int) indicator_index == lt_moving_average_index_ ) {
    lt_moving_average_ = new_value;
  } else if ((int) indicator_index == st_moving_avearge_index_ ) {
    st_moving_avearge_ = new_value;
  }

  targetbias_numbers_ = model_weight_ * model_target_bias_ +
      (1-model_weight_) * ( current_price_ - intraday_ema_weight_ * lt_moving_average_ + (1 -intraday_ema_weight_) *
historical_moving_average_ );
}*/

// Called from BaseOrderManager, because we have subscribed to execs by calling AddExecutionListener
// OnExec is called before OnPositionChange
// Note: base_pnl writes the trade upon OnExec call and not OnPositionChange call
// Subscribed for both DOL and WDO.
void DirectionalSyntheticTrading::OnExec(const int new_position, const int exec_quantity, const TradeType_t buysell,
                                         const double price, const int int_price, const int _security_id_) {
  combined_pnl_->OnExec(new_position, exec_quantity, buysell, price, int_price, _security_id_);
  leg_positions_vec_[0] = combined_pnl_->GetLeg1Position();
  leg_positions_vec_[1] = combined_pnl_->GetLeg2Position();
  UpdatePositions();
  TradeVarSetLogic(my_position_);
  //  TradingLogic();
}

// Called from BaseOrderManager, because we have subscribed to position change by calling AddPositionListener
// Subscribed for both DOL and WDO.
void DirectionalSyntheticTrading::OnPositionChange(int new_position, int position_diff,
                                                   const unsigned int security_id) {}

void DirectionalSyntheticTrading::ReportResults(HFSAT::BulkFileWriter& trades_writer, bool conservative_close) {
  double leg1_pnl = combined_pnl_->GetLeg1Pnl();
  double leg2_pnl = combined_pnl_->GetLeg2Pnl();
  // double total_pnl = leg1_pnl + leg2_pnl;

  int leg1_volume = leg_om_vec_[0]->trade_volume();
  int leg2_volume = leg_om_vec_[1]->trade_volume();
  int total_volume = leg1_volume + leg2_volume;

  std::cout << "Leg1. PNL: " << leg1_pnl << " Volume: " << leg1_volume
            << " Position: " << combined_pnl_->GetLeg1Position() << std::endl;
  std::cout << "Leg2. PNL: " << leg2_pnl << " Volume: " << leg2_volume
            << " Position: " << combined_pnl_->GetLeg2Position() << std::endl;
  std::cout << "Total. PNL: " << combined_pnl_->GetUnrealizedPnl() << " Volume: " << total_volume
            << " Position: " << my_position_ << std::endl;
  std::cout << "Min.PNL: " << min_unrealized_pnl_ << std::endl;
}

void DirectionalSyntheticTrading::PlaceCancelBuyAggOrders() {
  // Sending Agg orders
  if (leg_bid_trade_size_vec_[0] > 0 &&
      watch_.msecs_from_midnight() - last_agg_buy_msecs_vec_[0] > param_set_.agg_cooloff_interval_) {
    if ((leg_om_vec_[0]->SumBidSizeUnconfirmedEqAboveIntPrice(leg_bid_int_price_vec_[0] + 1) == 0) &&
        (leg_om_vec_[0]->SumBidSizeConfirmedEqAboveIntPrice(leg_bid_int_price_vec_[0]) == 0)) {
      leg_om_vec_[0]->SendTrade(leg_ask_price_vec_[0], leg_ask_int_price_vec_[0], leg_bid_trade_size_vec_[0],
                                kTradeTypeBuy, 'A');

      placed_bids_this_round_ = true;
      last_agg_buy_msecs_vec_[0] = watch_.msecs_from_midnight();
      last_buy_msecs_ = watch_.msecs_from_midnight();
    } else {
      leg_om_vec_[0]->CancelBidsEqAboveIntPrice(leg_bid_int_price_vec_[0]);
    }
  }

  if (leg_ask_trade_size_vec_[1] &&
      watch_.msecs_from_midnight() - last_agg_sell_msecs_vec_[1] > param_set_.agg_cooloff_interval_) {
    if ((leg_om_vec_[1]->SumAskSizeUnconfirmedEqAboveIntPrice(leg_ask_int_price_vec_[1] - 1) == 0) &&
        (leg_om_vec_[1]->SumAskSizeConfirmedEqAboveIntPrice(leg_ask_int_price_vec_[1]) == 0)) {
      leg_om_vec_[1]->SendTrade(leg_bid_price_vec_[1], leg_bid_int_price_vec_[1], leg_ask_trade_size_vec_[1],
                                kTradeTypeSell, 'A');

      placed_bids_this_round_ = true;
      last_agg_sell_msecs_vec_[1] = watch_.msecs_from_midnight();
      last_buy_msecs_ = watch_.msecs_from_midnight();
    } else {
      leg_om_vec_[1]->CancelAsksEqAboveIntPrice(leg_ask_int_price_vec_[1]);
    }
  }
}

void DirectionalSyntheticTrading::PlaceCancelBuyImpOrders() {
  if (!is_outright_risk_) {
    return;
  }

  // Sending the Agg Orders
  if (leg_bid_imp_trade_size_vec_[0] > 0 &&
      watch_.msecs_from_midnight() - last_agg_buy_msecs_vec_[0] > param_set_.agg_cooloff_interval_) {
    if ((leg_om_vec_[0]->SumBidSizeUnconfirmedEqAboveIntPrice(leg_bid_int_price_vec_[0] + 1) == 0) &&
        (leg_om_vec_[0]->SumBidSizeConfirmedEqAboveIntPrice(leg_bid_int_price_vec_[0]) == 0)) {
      leg_om_vec_[0]->SendTrade(leg_ask_price_vec_[0], leg_ask_int_price_vec_[0], leg_bid_imp_trade_size_vec_[0],
                                kTradeTypeBuy, 'A');
      last_agg_buy_msecs_vec_[0] = watch_.msecs_from_midnight();
      last_buy_msecs_ = watch_.msecs_from_midnight();
      placed_bids_this_round_ = true;
    } else {
      leg_om_vec_[0]->CancelBidsEqAboveIntPrice(leg_bid_int_price_vec_[0]);
    }
  }

  if (leg_ask_imp_trade_size_vec_[1] > 0 &&
      watch_.msecs_from_midnight() - last_agg_sell_msecs_vec_[1] > param_set_.agg_cooloff_interval_) {
    if ((leg_om_vec_[1]->SumAskSizeUnconfirmedEqAboveIntPrice(leg_ask_int_price_vec_[1] - 1) == 0) &&
        (leg_om_vec_[1]->SumAskSizeConfirmedEqAboveIntPrice(leg_ask_int_price_vec_[1]) == 0)) {
      leg_om_vec_[1]->SendTrade(leg_bid_price_vec_[1], leg_bid_int_price_vec_[1], leg_ask_imp_trade_size_vec_[1],
                                kTradeTypeSell, 'A');
      last_agg_sell_msecs_vec_[1] = watch_.msecs_from_midnight();
      last_buy_msecs_ = watch_.msecs_from_midnight();
      placed_bids_this_round_ = true;
    } else {
      leg_om_vec_[1]->CancelAsksEqAboveIntPrice(leg_ask_int_price_vec_[1]);
    }
  }
}

void DirectionalSyntheticTrading::PlaceCancelBuyBestOrders() {
  if ((leg_om_vec_[0]->SumBidSizeUnconfirmedEqAboveIntPrice(leg_bid_int_price_vec_[0]) == 0) &&
      (leg_om_vec_[0]->SumBidSizeConfirmedEqAboveIntPrice(leg_bid_int_price_vec_[0]) == 0)) {
    leg_om_vec_[0]->SendTrade(leg_bid_price_vec_[0], leg_bid_int_price_vec_[0], leg_bid_trade_size_vec_[0],
                              kTradeTypeBuy, 'B');
    placed_bids_this_round_ = true;
  }

  if ((leg_om_vec_[1]->SumAskSizeUnconfirmedEqAboveIntPrice(leg_ask_int_price_vec_[1]) == 0) &&
      (leg_om_vec_[1]->SumAskSizeConfirmedEqAboveIntPrice(leg_ask_int_price_vec_[1]) == 0)) {
    leg_om_vec_[1]->SendTrade(leg_ask_price_vec_[1], leg_ask_int_price_vec_[1], leg_ask_trade_size_vec_[1],
                              kTradeTypeSell, 'B');
    placed_bids_this_round_ = true;
  }
}

void DirectionalSyntheticTrading::PlaceCancelSellAggOrders() {
  // Sending Agg orders
  if (leg_ask_trade_size_vec_[0] > 0 &&
      watch_.msecs_from_midnight() - last_agg_sell_msecs_vec_[0] > param_set_.agg_cooloff_interval_) {
    if ((leg_om_vec_[0]->SumAskSizeUnconfirmedEqAboveIntPrice(leg_ask_int_price_vec_[0] - 1) == 0) &&
        (leg_om_vec_[0]->SumAskSizeConfirmedEqAboveIntPrice(leg_ask_int_price_vec_[0]) == 0)) {
      leg_om_vec_[0]->SendTrade(leg_bid_price_vec_[0], leg_bid_int_price_vec_[0], leg_ask_trade_size_vec_[0],
                                kTradeTypeSell, 'A');

      placed_asks_this_round_ = true;
      last_agg_sell_msecs_vec_[0] = watch_.msecs_from_midnight();
      last_sell_msecs_ = watch_.msecs_from_midnight();
    } else {
      leg_om_vec_[0]->CancelAsksEqAboveIntPrice(leg_ask_int_price_vec_[0]);
    }
  }

  if (leg_bid_trade_size_vec_[1] > 0 &&
      watch_.msecs_from_midnight() - last_agg_buy_msecs_vec_[1] > param_set_.agg_cooloff_interval_) {
    if ((leg_om_vec_[1]->SumBidSizeUnconfirmedEqAboveIntPrice(leg_bid_int_price_vec_[1] + 1) == 0) &&
        (leg_om_vec_[1]->SumBidSizeConfirmedEqAboveIntPrice(leg_bid_int_price_vec_[1]) == 0)) {
      leg_om_vec_[1]->SendTrade(leg_ask_price_vec_[1], leg_ask_int_price_vec_[1], leg_bid_trade_size_vec_[1],
                                kTradeTypeBuy, 'A');

      placed_asks_this_round_ = true;
      last_agg_buy_msecs_vec_[1] = watch_.msecs_from_midnight();
      last_sell_msecs_ = watch_.msecs_from_midnight();
    } else {
      leg_om_vec_[1]->CancelBidsEqAboveIntPrice(leg_bid_int_price_vec_[1]);
    }
  }
}

void DirectionalSyntheticTrading::PlaceCancelSellImpOrders() {
  if (!is_outright_risk_) {
    return;
  }

  // Sending the Agg Orders
  if (leg_ask_imp_trade_size_vec_[0] > 0 &&
      watch_.msecs_from_midnight() - last_agg_sell_msecs_vec_[0] > param_set_.agg_cooloff_interval_) {
    if ((leg_om_vec_[0]->SumAskSizeUnconfirmedEqAboveIntPrice(leg_ask_int_price_vec_[0] - 1) == 0) &&
        (leg_om_vec_[0]->SumAskSizeConfirmedEqAboveIntPrice(leg_ask_int_price_vec_[0]) == 0)) {
      leg_om_vec_[0]->SendTrade(leg_bid_price_vec_[0], leg_bid_int_price_vec_[0], leg_ask_imp_trade_size_vec_[0],
                                kTradeTypeSell, 'A');
      last_agg_sell_msecs_vec_[0] = watch_.msecs_from_midnight();
      last_sell_msecs_ = watch_.msecs_from_midnight();
      placed_asks_this_round_ = true;
    } else {
      leg_om_vec_[0]->CancelAsksEqAboveIntPrice(leg_ask_int_price_vec_[0]);
    }
  }

  if (leg_bid_imp_trade_size_vec_[1] > 0 &&
      watch_.msecs_from_midnight() - last_agg_buy_msecs_vec_[1] > param_set_.agg_cooloff_interval_) {
    if ((leg_om_vec_[1]->SumBidSizeUnconfirmedEqAboveIntPrice(leg_bid_int_price_vec_[1] + 1) == 0) &&
        (leg_om_vec_[1]->SumBidSizeConfirmedEqAboveIntPrice(leg_bid_int_price_vec_[1]) == 0)) {
      leg_om_vec_[1]->SendTrade(leg_ask_price_vec_[1], leg_ask_int_price_vec_[1], leg_bid_imp_trade_size_vec_[1],
                                kTradeTypeBuy, 'A');
      last_agg_buy_msecs_vec_[1] = watch_.msecs_from_midnight();
      last_sell_msecs_ = watch_.msecs_from_midnight();
      placed_asks_this_round_ = true;
    } else {
      leg_om_vec_[1]->CancelBidsEqAboveIntPrice(leg_bid_int_price_vec_[1]);
    }
  }
}

void DirectionalSyntheticTrading::PlaceCancelSellBestOrders() {
  if ((leg_om_vec_[0]->SumAskSizeUnconfirmedEqAboveIntPrice(leg_ask_int_price_vec_[0]) == 0) &&
      (leg_om_vec_[0]->SumAskSizeConfirmedEqAboveIntPrice(leg_ask_int_price_vec_[0]) == 0)) {
    leg_om_vec_[0]->SendTrade(leg_ask_price_vec_[0], leg_ask_int_price_vec_[0], leg_ask_trade_size_vec_[0],
                              kTradeTypeSell, 'B');
    placed_asks_this_round_ = true;
  }

  if ((leg_om_vec_[1]->SumBidSizeUnconfirmedEqAboveIntPrice(leg_bid_int_price_vec_[1]) == 0) &&
      (leg_om_vec_[1]->SumBidSizeConfirmedEqAboveIntPrice(leg_bid_int_price_vec_[1]) == 0)) {
    leg_om_vec_[1]->SendTrade(leg_bid_price_vec_[1], leg_bid_int_price_vec_[1], leg_bid_trade_size_vec_[1],
                              kTradeTypeBuy, 'B');
    placed_asks_this_round_ = true;
  }
}

void DirectionalSyntheticTrading::CancelCloseoutExtraPos() {
  if (my_position_ > 0) {
    if (spread_adj_leg1_pos_ != 0) {
      int t_leg1_trade_size_ = std::max(0, std::min(uts_vec_[0], spread_adj_leg1_pos_));

      if ((leg_om_vec_[0]->SumAskSizeUnconfirmedEqAboveIntPrice(leg_ask_int_price_vec_[0]) == 0) &&
          (leg_om_vec_[0]->SumAskSizeConfirmedEqAboveIntPrice(leg_ask_int_price_vec_[0]) == 0) &&
          t_leg1_trade_size_ > 0) {
        leg_om_vec_[0]->SendTrade(leg_ask_price_vec_[0], leg_ask_int_price_vec_[0], t_leg1_trade_size_, kTradeTypeSell,
                                  'B');
      }
    }

    if (spread_adj_leg2_pos_ != 0) {
      int t_leg2_trade_size_ = std::max(0, std::min(uts_vec_[1], -spread_adj_leg2_pos_));

      if ((leg_om_vec_[1]->SumBidSizeUnconfirmedEqAboveIntPrice(leg_bid_int_price_vec_[1]) == 0) &&
          (leg_om_vec_[1]->SumBidSizeConfirmedEqAboveIntPrice(leg_bid_int_price_vec_[1]) == 0) &&
          t_leg2_trade_size_ > 0) {
        leg_om_vec_[1]->SendTrade(leg_bid_price_vec_[1], leg_bid_int_price_vec_[1], t_leg2_trade_size_, kTradeTypeBuy,
                                  'B');
      }
    }
  } else if (my_position_ < 0) {
    if (spread_adj_leg1_pos_ != 0) {
      int t_leg1_trade_size_ = std::max(0, std::min(uts_vec_[0], -spread_adj_leg1_pos_));

      if ((leg_om_vec_[0]->SumBidSizeUnconfirmedEqAboveIntPrice(leg_bid_int_price_vec_[0]) == 0) &&
          (leg_om_vec_[0]->SumBidSizeConfirmedEqAboveIntPrice(leg_bid_int_price_vec_[0]) == 0) &&
          t_leg1_trade_size_ > 0) {
        leg_om_vec_[0]->SendTrade(leg_bid_price_vec_[0], leg_bid_int_price_vec_[0], t_leg1_trade_size_, kTradeTypeBuy,
                                  'B');
      }
    }

    if (spread_adj_leg2_pos_ != 0) {
      int t_leg2_trade_size_ = std::max(0, std::min(uts_vec_[1], spread_adj_leg2_pos_));

      if ((leg_om_vec_[1]->SumAskSizeUnconfirmedEqAboveIntPrice(leg_ask_int_price_vec_[1]) == 0) &&
          (leg_om_vec_[1]->SumAskSizeConfirmedEqAboveIntPrice(leg_ask_int_price_vec_[1]) == 0) &&
          t_leg2_trade_size_ > 0) {
        leg_om_vec_[1]->SendTrade(leg_ask_price_vec_[1], leg_ask_int_price_vec_[1], t_leg2_trade_size_, kTradeTypeSell,
                                  'B');
      }
    }
  } else if (my_position_ == 0) {
    if (spread_adj_leg1_pos_ != 0) {
      GetFlat(0);
    }

    if (spread_adj_leg2_pos_ != 0) {
      GetFlat(1);
    }
  }
}
/*
void DirectionalSyntheticTrading::UpdatePositions() {
  double t_spread_position_ = ( leg_positions_vec_[0] > 0 )
      ? std::max(0.0, std::min((double) leg_positions_vec_[0], -1 * dv01_vec_[1]/dv01_vec_[0] * leg_positions_vec_[1] ))
      : std::min(0.0, std::max((double) leg_positions_vec_[0], -1 * dv01_vec_[1]/dv01_vec_[0] * leg_positions_vec_[1]
));

  spread_adj_leg1_pos_ = leg_positions_vec_[0] - (int) round ( t_spread_position_ );
  spread_adj_leg2_pos_ = leg_positions_vec_[1] + (int) round ( dv01_vec_[0]/dv01_vec_[1] * t_spread_position_ );
  my_position_ = (int) round ( t_spread_position_ );

  is_synth_flat_ = ( leg_positions_vec_[0] == 0 && leg_positions_vec_[1] == 0 ) ? true : false;

  leg_bid_trade_size_vec_[0] = uts_vec_[0];
  leg_ask_trade_size_vec_[0] = uts_vec_[0];
  leg_bid_trade_size_vec_[1] = uts_vec_[1];
  leg_ask_trade_size_vec_[1] = uts_vec_[1];

  // Changing sizes to show as per the positions
  if ( spread_adj_leg1_pos_ > 0 ) {
    leg_bid_trade_size_vec_[0] = MathUtils::GetFlooredMultipleOf (
        std::max( 0, std::min( uts_vec_[0] - spread_adj_leg1_pos_, leg_max_position_vec_[0] - leg_positions_vec_[0] ) ),
        smv_vec_[0]->min_order_size() ); // To not hit max pos
  } else {
    leg_ask_trade_size_vec_[0] = MathUtils::GetFlooredMultipleOf (
        std::max( 0, std::min( uts_vec_[0] + spread_adj_leg1_pos_, leg_max_position_vec_[0] + leg_positions_vec_[0] ) ),
        smv_vec_[0]->min_order_size() );
  }

  if ( spread_adj_leg2_pos_ > 0 ) {
    leg_bid_trade_size_vec_[1] = MathUtils::GetFlooredMultipleOf (
        std::max( 0, std::min ( uts_vec_[1] - spread_adj_leg2_pos_, leg_max_position_vec_[1] - leg_positions_vec_[0] )
),
        smv_vec_[1]->min_order_size() ); // To not hit max pos
  } else {
    leg_ask_trade_size_vec_[1] = MathUtils::GetFlooredMultipleOf (
        std::max( 0, std::min ( uts_vec_[1] + spread_adj_leg2_pos_, leg_max_position_vec_[1] + leg_positions_vec_[1] )
),
        smv_vec_[1]->min_order_size() );
  }
}*/

void DirectionalSyntheticTrading::UpdatePositions() {
  int current_leg1_projected_pos_ = leg1_to_leg2_pos_map_[leg_positions_vec_[0]];
  int current_leg2_projected_pos_ = leg2_to_leg1_pos_map_[leg_positions_vec_[1]];

  my_position_ = (leg_positions_vec_[0] > 0)
                     ? std::max(0, std::min(leg_positions_vec_[0], current_leg2_projected_pos_))
                     : std::min(0, std::max(leg_positions_vec_[0], current_leg2_projected_pos_));

  spread_adj_leg1_pos_ = leg_positions_vec_[0] - my_position_;
  spread_adj_leg2_pos_ = leg_positions_vec_[1] - leg1_to_leg2_pos_map_[my_position_];

  is_synth_flat_ = (leg_positions_vec_[0] == 0 && leg_positions_vec_[1] == 0) ? true : false;

  if (my_position_ > 0) {
    leg_bid_trade_size_vec_[0] = std::min(
        std::min(uts_vec_[0], leg2_to_leg1_pos_map_[leg_positions_vec_[1] - uts_vec_[1]] - leg_positions_vec_[0]),
        leg_max_position_vec_[0] - leg_positions_vec_[0]);
    leg_ask_trade_size_vec_[0] = std::min(
        std::min(uts_vec_[0], leg2_to_leg1_pos_map_[leg_positions_vec_[1] + uts_vec_[1]] - leg_positions_vec_[0]),
        leg_max_position_vec_[0] - leg_positions_vec_[0]);
    leg_bid_trade_size_vec_[1] = std::min(
        std::min(uts_vec_[1], leg1_to_leg2_pos_map_[leg_positions_vec_[0] - uts_vec_[0]] - leg_positions_vec_[1]),
        leg_max_position_vec_[1] + leg_positions_vec_[1]);
    leg_ask_trade_size_vec_[1] = std::min(
        std::min(uts_vec_[1], leg1_to_leg2_pos_map_[leg_positions_vec_[0] + uts_vec_[0]] - leg_positions_vec_[1]),
        leg_max_position_vec_[1] + leg_positions_vec_[1]);
  } else {
    leg_bid_trade_size_vec_[0] = std::min(
        std::min(uts_vec_[0], leg2_to_leg1_pos_map_[leg_positions_vec_[1] - uts_vec_[1]] - leg_positions_vec_[0]),
        leg_max_position_vec_[0] + leg_positions_vec_[0]);
    leg_ask_trade_size_vec_[0] = std::min(
        std::min(uts_vec_[0], leg2_to_leg1_pos_map_[leg_positions_vec_[1] + uts_vec_[1]] - leg_positions_vec_[0]),
        leg_max_position_vec_[0] + leg_positions_vec_[0]);
    leg_bid_trade_size_vec_[1] = std::min(
        std::min(uts_vec_[1], leg1_to_leg2_pos_map_[leg_positions_vec_[0] - uts_vec_[0]] - leg_positions_vec_[1]),
        leg_max_position_vec_[1] - leg_positions_vec_[1]);
    leg_ask_trade_size_vec_[1] = std::min(
        std::min(uts_vec_[1], leg1_to_leg2_pos_map_[leg_positions_vec_[0] + uts_vec_[0]] - leg_positions_vec_[1]),
        leg_max_position_vec_[1] - leg_positions_vec_[1]);
  }

  if (leg_positions_vec_[0] * leg_positions_vec_[1] <= 0) {
    if (current_leg2_projected_pos_ - leg_positions_vec_[0] == 0 ||
        current_leg1_projected_pos_ - leg_positions_vec_[1] == 0) {
      my_position_ = leg_positions_vec_[0];
      is_outright_risk_ = false;
    } else {
      is_outright_risk_ = true;
      leg_bid_imp_trade_size_vec_[0] =
          std::min(uts_vec_[0], std::max(0, current_leg2_projected_pos_ - leg_positions_vec_[0]));
      leg_ask_imp_trade_size_vec_[0] =
          std::min(uts_vec_[0], std::max(0, leg_positions_vec_[0] - current_leg2_projected_pos_));
      leg_bid_imp_trade_size_vec_[1] =
          std::min(uts_vec_[1], std::max(0, current_leg1_projected_pos_ - leg_positions_vec_[1]));
      leg_ask_imp_trade_size_vec_[1] =
          std::min(uts_vec_[1], std::max(0, leg_positions_vec_[1] - current_leg2_projected_pos_));
    }
  }
}

void DirectionalSyntheticTrading::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (!is_dv01_not_updated_) {
    UpdateDV01();
  }
}
void DirectionalSyntheticTrading::UpdateDV01() {
  if (is_di_spread_) {
    for (auto i = 0u; i < shortcode_vec_.size(); i++) {
      dv01_vec_[i] = HFSAT::CurveUtils::stirs_fut_dv01(shortcode_vec_[i], tradingdate_, leg_bid_price_vec_[i]);
    }
    UpdateDV01Vars();
  }
}

void DirectionalSyntheticTrading::UpdateDV01Vars() {
  for (auto i = 0u; i < shortcode_vec_.size(); i++) {
    uts_vec_[i] = MathUtils::GetCeilMultipleOf((int)(dv01_vec_[0] / dv01_vec_[i] * param_set_.unit_trade_size_),
                                               smv_vec_[i]->min_order_size());
    leg_max_position_vec_[i] = MathUtils::GetCeilMultipleOf(
        (int)(dv01_vec_[0] / dv01_vec_[i] * param_set_.max_position_), smv_vec_[i]->min_order_size());
    leg_bid_trade_size_vec_[i] = uts_vec_[i];
    leg_ask_trade_size_vec_[i] = uts_vec_[i];
  }

  leg1_to_leg2_pos_map_.clear();
  leg2_to_leg1_pos_map_.clear();

  for (int i = -2 * leg_max_position_vec_[0]; i <= 2 * leg_max_position_vec_[0]; i += smv_vec_[0]->min_order_size()) {
    leg1_to_leg2_pos_map_[i] =
        MathUtils::SignedRoundOff(-1 * dv01_vec_[0] / dv01_vec_[1] * i, smv_vec_[1]->min_order_size());
  }

  for (int i = -2 * leg_max_position_vec_[1]; i <= 2 * leg_max_position_vec_[1]; i += smv_vec_[1]->min_order_size()) {
    leg2_to_leg1_pos_map_[i] =
        MathUtils::SignedRoundOff(-1 * dv01_vec_[1] / dv01_vec_[0] * i, smv_vec_[0]->min_order_size());
  }
}

void DirectionalSyntheticTrading::OnWakeUpifRejectDueToFunds() {}

void DirectionalSyntheticTrading::OnMarketDataInterrupted(const unsigned int security_id,
                                                          const int msecs_since_last_receive) {}

void DirectionalSyntheticTrading::OnMarketDataResumed(const unsigned int security_id) {}

void DirectionalSyntheticTrading::OnGlobalPNLChange(double new_global_PNL) {}

void DirectionalSyntheticTrading::OnOrderChange() {}

void DirectionalSyntheticTrading::OnGlobalPositionChange(const unsigned int security_id, int new_global_position) {}

void DirectionalSyntheticTrading::OnControlUpdate(const ControlMessage& control_message, const char* symbol,
                                                  const int trader_id) {
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
      /*DBGLOG_TIME_CLASS_FUNC << "ShowIndicators " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << leg1_smv_.secname() << " OM: " << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME << leg_om_vec_[0].ShowOrders() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << leg2_smv_.secname() << " OM: " << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME << leg_om_vec_[1].ShowOrders() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << GetCurrentMarket() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << "NumArbChances: " << num_arb_chances_ << " last_size_available: " << last_arb_size_
                             << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << leg2_smv_.secname()
                             << " messages: " << leg_om_vec_[1].SendOrderCount() + leg_om_vec_[1].CxlOrderCount() << " "
                             << leg1_smv_.secname()
                             << " messages: " << leg_om_vec_[0].SendOrderCount() + leg_om_vec_[0].CxlOrderCount()
                             << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << " " << leg1_smv_.secname() << " status: " << leg1_mkt_status_ << " " <<
      leg2_smv_.secname()
                             << " status: " << leg2_mkt_status_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      break;
      break;*/
    }
    case kControlMessageCodeShowOrders: {
      /* DBGLOG_TIME_CLASS_FUNC << "ShowOrders " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << leg1_smv_.secname() << " OM: " << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME << leg_om_vec_[0].ShowOrders() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << leg2_smv_.secname() << " OM: " << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME << leg_om_vec_[1].ShowOrders() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << GetCurrentMarket() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << "NumArbChances: " << num_arb_chances_ << " last_size_available: " << last_arb_size_
                             << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << leg2_smv_.secname()
                             << " messages: " << leg_om_vec_[1].SendOrderCount() + leg_om_vec_[1].CxlOrderCount() << " "
                             << leg1_smv_.secname()
                             << " messages: " << leg_om_vec_[0].SendOrderCount() + leg_om_vec_[0].CxlOrderCount()
                             << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << " " << leg1_smv_.secname() << " status: " << leg1_mkt_status_ << " " <<
      leg2_smv_.secname()
                             << " status: " << leg2_mkt_status_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      break;*/
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

bool DirectionalSyntheticTrading::UpdateTarget(double _target_price_, double _targetbias_numbers_,
                                               int modelmath_index) {
  if (!is_ready_) {
    if (livetrading_ && (watch_.tv() > start_time_)) {
      AllocateCPU();
    }

    if (watch_.tv() > start_time_ && dep_market_view_.is_ready() &&
        (_target_price_ >= dep_market_view_.bestbid_price() && _target_price_ <= dep_market_view_.bestask_price())) {
      is_ready_ = true;
      DBGLOG_TIME_CLASS_FUNC << "SHC: " << dep_market_view_.shortcode() << " got ready! "
                             << " queryid: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      p_base_model_math_->ShowIndicatorValues();
      TradeVarSetLogic(my_position_);
    }
  } else {
    target_price_ = _target_price_;
    targetbias_numbers_ = _targetbias_numbers_;

    if (!is_dv01_not_updated_) {
      TradingLogic();
    }
  }
  return false;
}

void DirectionalSyntheticTrading::OnFokReject(const TradeType_t buysell, const double price, const int int_price,
                                              const int size_remaining) {}

void DirectionalSyntheticTrading::TargetNotReady() {}

void DirectionalSyntheticTrading::OnMarketStatusChange(const unsigned int security_id,
                                                       const MktStatus_t new_market_status) {
  if (security_id == smv_vec_[0]->security_id()) {
    leg_mkt_status_vec_[0] = new_market_status;
  } else if (security_id == smv_vec_[1]->security_id()) {
    leg_mkt_status_vec_[1] = new_market_status;
  }
  ShouldGetFlat();
  DBGLOG_TIME_CLASS_FUNC << "MktStatus for security_id: " << security_id << " changed to: " << new_market_status
                         << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;
}

void DirectionalSyntheticTrading::BuildTradeVarSets() {
  tradevarset_builder_->BuildConstantTradeVarSets(&param_set_, &dep_market_view_, closeout_zeropos_tradevarset_,
                                                  closeout_long_tradevarset_, closeout_short_tradevarset_);
  current_tradevarset_ = closeout_zeropos_tradevarset_;
  for (unsigned i = 0; i < param_set_vec_.size(); i++) {
    tradevarset_builder_->BuildPositionTradeVarSetMap(&param_set_vec_[i], &dep_market_view_,
                                                      position_tradevarset_map_vec_[i], map_pos_increment_vec_[i],
                                                      P2TV_zero_idx_, livetrading_);
    if ((int)i == param_index_to_use_) {
      param_set_.max_position_ = param_set_vec_[param_index_to_use_].max_position_;
    }
  }

  position_tradevarset_map_ = position_tradevarset_map_vec_[param_index_to_use_];
  map_pos_increment_ = map_pos_increment_vec_[param_index_to_use_];
}

void DirectionalSyntheticTrading::TradeVarSetLogic(int t_position) {
  current_position_tradevarset_map_index_ = P2TV_zero_idx_;
  if (map_pos_increment_ > 1) {
    if (t_position > 0) {
      current_position_tradevarset_map_index_ += std::min(MAX_POS_MAP_SIZE, (abs(t_position) / map_pos_increment_));
      current_tradevarset_ = position_tradevarset_map_[current_position_tradevarset_map_index_];
      if ((current_tradevarset_.l1bid_trade_size_ + t_position) > param_set_.max_position_) {
        current_tradevarset_.l1bid_trade_size_ = MathUtils::GetFlooredMultipleOf(
            std::max(0, param_set_.max_position_ - t_position), dep_market_view_.min_order_size());
      }
    } else {
      current_position_tradevarset_map_index_ -= std::min(MAX_POS_MAP_SIZE, (abs(t_position) / map_pos_increment_));
      current_tradevarset_ = position_tradevarset_map_[current_position_tradevarset_map_index_];
      if ((current_tradevarset_.l1ask_trade_size_ - t_position) > param_set_.max_position_) {
        current_tradevarset_.l1ask_trade_size_ = MathUtils::GetFlooredMultipleOf(
            std::max(0, param_set_.max_position_ + t_position), dep_market_view_.min_order_size());
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

  current_bid_tradevarset_ = current_tradevarset_;
  current_bid_keep_tradevarset_ = current_tradevarset_;
  current_ask_tradevarset_ = current_tradevarset_;
  current_ask_keep_tradevarset_ = current_tradevarset_;
}
}
