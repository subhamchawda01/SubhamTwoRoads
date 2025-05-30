/*
 * retail_fly_trading_manager.cpp
 *
 *  Created on: Mar 19, 2015
 *      Author: archit
 */

#include "dvctrade/ExecLogic/retail_fly_trading_manager.hpp"

namespace HFSAT {

RetailFlyTradingManager::RetailFlyTradingManager(
    DebugLogger& _dbglogger_, const Watch& _watch_, const std::vector<const SecurityMarketView*>& _p_smv_vec_,
    MultBasePNL* _mult_base_pnl_, const std::string& _common_paramfilename_, const bool _livetrading_,
    EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
    const int t_trading_end_utc_mfm_, const int t_runtime_id_, const std::vector<std::string>& _paramfilename_vec_,
    const std::string& _fly_secname_)
    : TradingManager(_dbglogger_, _watch_),
      p_smv_vec_(_p_smv_vec_),
      my_position_vec_(p_smv_vec_.size(), 0),
      fp_position_vec_(p_smv_vec_.size(), 0),
      bom_position_vec_(p_smv_vec_.size(), 0),
      my_global_position_vec_(p_smv_vec_.size(), 0),
      projected_pc1_risk_vec_(p_smv_vec_.size(), 0),
      target_price_vec_(p_smv_vec_.size(), 0.0),
      total_pnl_(0),
      is_ready_(false),
      is_ready_vec_(p_smv_vec_.size(), false),
      is_data_interrupted_vec_(p_smv_vec_.size(), false),
      trading_start_utc_mfm_(t_trading_start_utc_mfm_),
      trading_end_utc_mfm_(t_trading_end_utc_mfm_),
      runtime_id_(t_runtime_id_),
      livetrading_(_livetrading_),
      should_be_getting_flat_(false),
      start_not_given_(livetrading_),
      getflat_due_to_external_getflat_(livetrading_),
      getflat_due_to_close_(false),
      getflat_due_to_max_loss_(false),
      getflat_due_to_economic_times_(false),
      getflat_due_to_market_data_interrupt_(false),
      getflat_due_to_allowed_economic_event_(false),
      getflat_due_to_freeze_(false),
      last_allowed_event_index_(0u),
      economic_events_manager_(t_economic_events_manager_),
      ezone_vec_(),
      severity_to_getflat_on_base_(1.00),
      severity_to_getflat_on_(1.00),
      severity_change_end_msecs_(t_trading_end_utc_mfm_),
      applicable_severity_(0.00),
      allowed_events_present_(false),
      getflat_retail_update_type_(HFSAT::CDef::knormal),
      order_manager_vec_(p_smv_vec_.size(), NULL),
      dv01_vec_(p_smv_vec_.size(), 1.00),
      price_factor_vec_(p_smv_vec_.size(), 1.00),
      size_factor_vec_(p_smv_vec_.size(), 1.00),
      p_mult_base_pnl_(_mult_base_pnl_),
      common_param_set_(ParamSet(_common_paramfilename_, watch_.YYYYMMDD(), "NONAME")),
      last_retail_update_msecs_(0),
      retail_offer_listeners_(),
      last_retail_offer_(),
      last_retail_offered_bidprice_(0.0),
      last_retail_offered_askprice_(0.0),
      last_full_logging_msecs_(0),
      last_trade_was_buy_(false),
      last_trade_was_sell_(false),
      last_buy_price_(0.0),
      last_sell_price_(0.0),
      moving_avg_spread_(0),
      last_new_page_msecs_(0),
      page_width_msecs_(500),
      last_price_recorded_(0),
      current_spread_mkt_price_(0),
      retail_offer_bid_fra_(0.0),
      retail_offer_ask_fra_(0.0),
      last_agg_buy_send_msecs_(0),
      last_agg_sell_send_msecs_(0) {
  if (p_smv_vec_.size() < 2) {
    ExitVerbose(kExitErrorCodeGeneral, "RetailFlyTradingManager called with < 2 legs");
  }

  if (p_smv_vec_.size() != _paramfilename_vec_.size()) {
    std::cerr << "p_smv_vec_.size() != _paramfilename_vec_.size() : " << p_smv_vec_.size()
              << " != " << _paramfilename_vec_.size() << std::endl;
    exit(0);
  }

  SetPriceFactVec();
  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    p_smv_vec_[i]->subscribe_OnReady(this);
    p_smv_vec_[i]->ComputeMktPrice();
    p_smv_vec_[i]->ComputeOrderWPrice();
    secid_idx_map_[p_smv_vec_[i]->security_id()] = i;
  }

  if (_fly_secname_ == "NONAME") {
    // can add handling for naming in this
    SpreadUtils::GetSpreadSecname(p_smv_vec_, fly_secname_string_);
  } else {
    fly_secname_string_ = _fly_secname_;
  }

  min_min_price_increment_to_use_ = p_smv_vec_[0]->min_price_increment() * fabs(price_factor_vec_[0]);
  for (unsigned int i = 1; i < p_smv_vec_.size(); i++) {
    min_min_price_increment_to_use_ =
        std::min(min_min_price_increment_to_use_, p_smv_vec_[i]->min_price_increment() * fabs(price_factor_vec_[i]));
  }

  belly_min_price_increment_ = p_smv_vec_[belly_index_]->min_price_increment();

  _mult_base_pnl_->AddListener(this);
  watch_.subscribe_BigTimePeriod(this);

  if (livetrading_) {
    for (auto i = 0u; i < p_smv_vec_.size(); i++) {
      PromOrderManager* t_p_prom_order_manager_ = PromOrderManager::GetCreatedInstance(p_smv_vec_[i]->shortcode());
      if (t_p_prom_order_manager_ != NULL) {
        t_p_prom_order_manager_->AddGlobalPositionChangeListener(this);
      } else {
        std::cerr << "prom order manager not initialized for " << p_smv_vec_[i]->shortcode()
                  << ". RetailFlyTradingManager Exiting\n.";
        exit(0);
      }
    }
  }

  //***********Eco Event Initializations*************//
  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    economic_events_manager_.AllowEconomicEventsFromList(p_smv_vec_[i]->shortcode());
    allowed_events_present_ = economic_events_manager_.AllowedEventsPresent();

    // allowedeco load work only for one of the two legs, as following function clears the vec
    if (allowed_events_present_) {
      break;
    }
  }
  economic_events_manager_.AdjustSeverity(p_smv_vec_[0]->shortcode(), p_smv_vec_[0]->exch_source());

  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    GetEZVecForShortcode(p_smv_vec_[i]->shortcode(), t_trading_start_utc_mfm_, ezone_vec_);
    GetEZVecForShortcode(p_smv_vec_[i]->shortcode(), t_trading_end_utc_mfm_ - 1,
                         ezone_vec_);  //( t_trading_end_utc_mfm_ - 1 ) to avoid events overlapping period for EU and US
  }

  DBGLOG_CLASS_FUNC << "After checking at mfm " << t_trading_start_utc_mfm_ << " and " << t_trading_end_utc_mfm_ - 1
                    << " . Stopping for EZones:";
  for (auto i = 0u; i < ezone_vec_.size(); i++) {
    dbglogger_ << ' ' << GetStrFromEconomicZone(ezone_vec_[i]);
  }
  dbglogger_ << DBGLOG_ENDL_FLUSH;

  //***********Param Loading*************//
  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    param_set_vec_.push_back(ParamSet(_paramfilename_vec_[i], watch_.YYYYMMDD(), p_smv_vec_[i]->shortcode()));
  }
  CheckParamSet();
  ReconcileParams();

  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    // this will be reset when both smv's are ready using common_max_pos and dv01 ratio
    bidsize_to_show_maxpos_limit_vec_.push_back(param_set_vec_[i].max_position_);
    asksize_to_show_maxpos_limit_vec_.push_back(param_set_vec_[i].max_position_);

    // this is used mainly to avoid breaching ORS maxpos limits
    bidsize_to_show_global_maxpos_limit_vec_.push_back(param_set_vec_[i].max_global_position_);
    asksize_to_show_global_maxpos_limit_vec_.push_back(param_set_vec_[i].max_global_position_);
  }

  //******reserve days loading***** *//
  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    reserve_days_vec_.push_back(HFSAT::CurveUtils::_get_term_(watch_.YYYYMMDD(), p_smv_vec_[i]->shortcode()));
    DBGLOG_CLASS_FUNC << "Security Reserve Days: " << p_smv_vec_[i]->shortcode() << " " << reserve_days_vec_[i]
                      << DBGLOG_ENDL_FLUSH;
  }
  if (p_smv_vec_.size() == 2) {
    size_ratio_vec_ = {1, 1};
  } else if (p_smv_vec_.size() == 3) {
    fra_sp_vec_.push_back(FRAVars(0.0, 0.0, 0.0));
    fra_sp_vec_.push_back(FRAVars(0.0, 0.0, 0.0));
    size_ratio_vec_ = {1, 2, 1};
  } else if (p_smv_vec_.size() == 4) {
    fra_sp_vec_.push_back(FRAVars(0.0, 0.0, 0.0));
    fra_sp_vec_.push_back(FRAVars(0.0, 0.0, 0.0));
    fra_sp_vec_.push_back(FRAVars(0.0, 0.0, 0.0));
    size_ratio_vec_ = {1, 3, 3, 1};
  }
  
  if (common_param_set_.retail_offer_fra_) {
    current_pu_vec_.resize(p_smv_vec_.size(), 1.0);
    inv_pu_bid_vec_.resize(p_smv_vec_.size(), 1.0);
    inv_pu_ask_vec_.resize(p_smv_vec_.size(), 1.0);
    inv_pu_mid_vec_.resize(p_smv_vec_.size(), 1.0);
    offered_bid_price_vec_.resize(p_smv_vec_.size(), 0.0);
    offered_ask_price_vec_.resize(p_smv_vec_.size(), 0.0);

    if (p_smv_vec_.size() == 3) {
      size_ratio_vec_ = {1, 2, 1};
    } else if (p_smv_vec_.size() == 4) {
      size_ratio_vec_ = {1, 3, 3, 1};
    }
  }

  const char* t_freeze_start_time_ = "BRT_1550";
  const char* t_freeze_end_time_ = "BRT_1651";
  freeze_start_mfm_ = GetMsecsFromMidnightFromHHMMSS(DateTime::GetUTCHHMMSSFromTZHHMMSS(
      watch_.YYYYMMDD(), DateTime::GetHHMMSSTime(t_freeze_start_time_ + 4), t_freeze_start_time_));
  freeze_end_mfm_ = GetMsecsFromMidnightFromHHMMSS(DateTime::GetUTCHHMMSSFromTZHHMMSS(
      watch_.YYYYMMDD(), DateTime::GetHHMMSSTime(t_freeze_end_time_ + 4), t_freeze_end_time_));
}

void RetailFlyTradingManager::ReportResults(HFSAT::BulkFileWriter& trades_writer_) {
  int t_total_volume_ = 0;
  int t_supporting_orders_filled_ = 0;
  int t_bestlevel_orders_filled_ = 0;
  int t_aggressive_orders_filled_ = 0;
  int t_improve_orders_filled_ = 0;

  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    t_total_volume_ += order_manager_vec_[i]->trade_volume();
    t_supporting_orders_filled_ +=
        order_manager_vec_[i]->SupportingOrderFilledPercent() * order_manager_vec_[i]->trade_volume();
    t_bestlevel_orders_filled_ +=
        order_manager_vec_[i]->BestLevelOrderFilledPercent() * order_manager_vec_[i]->trade_volume();
    t_improve_orders_filled_ +=
        order_manager_vec_[i]->ImproveOrderFilledPercent() * order_manager_vec_[i]->trade_volume();
    t_aggressive_orders_filled_ +=
        order_manager_vec_[i]->AggressiveOrderFilledPercent() * order_manager_vec_[i]->trade_volume();
  }

  if (t_total_volume_ != 0) {
    t_supporting_orders_filled_ /= t_total_volume_;
    t_bestlevel_orders_filled_ /= t_total_volume_;
    t_improve_orders_filled_ /= t_total_volume_;
    t_aggressive_orders_filled_ /= t_total_volume_;
  }

  printf("SIMRESULT %d %d %d %d %d %d\n", total_pnl_, (t_total_volume_), t_supporting_orders_filled_,
         t_bestlevel_orders_filled_, t_aggressive_orders_filled_, t_improve_orders_filled_);
}

void RetailFlyTradingManager::OnTimePeriodUpdate(const int num_pages_to_add_) {
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
        CPUManager::AffinToInitCores(int(getpid()));
      }
    }
  } else {
    getflat_due_to_close_ = false;
  }

  if (getflat_due_to_freeze_) {
    if (watch_.msecs_from_midnight() > freeze_end_mfm_) {
      getflat_due_to_freeze_ = false;
    }
  } else {
    if ((watch_.msecs_from_midnight() > freeze_start_mfm_) && (watch_.msecs_from_midnight() < freeze_end_mfm_)) {
      getflat_due_to_freeze_ = true;
    }
  }

  if (livetrading_ && (total_pnl_ < -common_param_set_.max_loss_)) {
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
            t_oss_ << "Retail Fly Strategy: " << runtime_id_ << " getflat_due_to_max_loss_: " << total_pnl_
                   << " product " << fly_secname_string_ << " on " << hostname_ << "\n";

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

  if (watch_.msecs_from_midnight() - last_retail_update_msecs_ > RETAIL_UPDATE_TIMEOUT_MSECS) {
    // just to send an update to GUI if we are idle for too long
    QuoteAndClose();
  }
}

bool RetailFlyTradingManager::AddPosition(const std::string& _shc_, int _position_offset_) {
  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    if (_shc_ == p_smv_vec_[i]->shortcode()) {
      if (!p_smv_vec_[i]->is_ready()) {
        DBGLOG_TIME_CLASS_FUNC << "Failed to add position. SMV not ready for " << p_smv_vec_[i]->shortcode()
                               << DBGLOG_ENDL_FLUSH;
        return false;
      }

      _position_offset_ = MathUtils::GetFlooredMultipleOf(_position_offset_, p_smv_vec_[i]->min_order_size());

      if (_position_offset_ == 0 || abs(my_position_vec_[i] + _position_offset_) > param_set_vec_[i].max_position_) {
        // just for sanity
        DBGLOG_TIME_CLASS_FUNC << "Failed to add position. Attempt to add position " << _position_offset_
                               << " == 0 || > Max position " << param_set_vec_[i].max_position_ << DBGLOG_ENDL_FLUSH;
        return false;
      }

      TradeType_t t_buysell_ = _position_offset_ > 0 ? kTradeTypeBuy : kTradeTypeSell;
      double t_exec_price_ = _position_offset_ > 0 ? p_smv_vec_[i]->bestbid_price()
                                                   : p_smv_vec_[i]->bestask_price();  // assuming a trade at best level

      // simulate a GUI Trade
      DBGLOG_TIME_CLASS_FUNC << "BEFORE: " << p_smv_vec_[i]->shortcode() << " FPPOS: " << fp_position_vec_[i]
                             << " BOMPOS: " << bom_position_vec_[i] << " MYPOS: " << my_position_vec_[i]
                             << " TRADE: " << abs(_position_offset_) << " " << GetTradeTypeChar(t_buysell_) << " @ "
                             << t_exec_price_ << DBGLOG_ENDL_FLUSH;

      fp_position_vec_[i] += _position_offset_;
      my_position_vec_[i] = bom_position_vec_[i] + fp_position_vec_[i];

      DBGLOG_TIME_CLASS_FUNC << "After: FPPOS: " << fp_position_vec_[i] << " BOMPOS: " << bom_position_vec_[i]
                             << " MYPOS: " << my_position_vec_[i] << DBGLOG_ENDL_FLUSH;

      UpdatePC1Risk();

      // adjust quote, send orders and log
      AdjustMaxPosLimitOfferSizes();
      QuoteAndClose();

      order_manager_vec_[i]->AddToTradeVolume(abs(_position_offset_));
      // emulating a retail trade, because this will be generally used to match the actual breakup for the GUI trade
      order_manager_vec_[i]->base_pnl().OnRetailExec(my_position_vec_[i], abs(_position_offset_), t_buysell_,
                                                     t_exec_price_, p_smv_vec_[i]->GetIntPx(t_exec_price_),
                                                     p_smv_vec_[i]->security_id());
      return true;
    }
  }

  DBGLOG_TIME_CLASS_FUNC << _shc_ << " not found in strat for " << fly_secname_string_ << DBGLOG_ENDL_FLUSH;
  return false;
}

void RetailFlyTradingManager::FPOrderExecuted(const char* _secname_, double _price_, TradeType_t r_buysell_,
                                              int _size_executed_) {
  if (_size_executed_ <= 0 || fly_secname_string_.compare(_secname_) != 0) {
    DBGLOG_TIME_CLASS_FUNC << "Unknown security " << _secname_ << " in " << fly_secname_string_ << DBGLOG_ENDL_FLUSH;
    return;
  }

  std::vector<TradeInfoStruct> leg_trades_;
  BreakFlyTrade(_secname_, _price_, _size_executed_, r_buysell_, leg_trades_);

  // update position vec
  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    DBGLOG_TIME_CLASS_FUNC << "BEFORE: " << p_smv_vec_[i]->shortcode() << " FPPOS: " << fp_position_vec_[i]
                           << " BOMPOS: " << bom_position_vec_[i] << " MYPOS: " << my_position_vec_[i]
                           << " TRADE: " << leg_trades_[i].trd_qty_ << " " << GetTradeTypeChar(leg_trades_[i].buysell_)
                           << " @ " << leg_trades_[i].trd_px_ << DBGLOG_ENDL_FLUSH;

    fp_position_vec_[i] +=
        (leg_trades_[i].buysell_ == kTradeTypeBuy) ? leg_trades_[i].trd_qty_ : -leg_trades_[i].trd_qty_;
    my_position_vec_[i] = bom_position_vec_[i] + fp_position_vec_[i];

    order_manager_vec_[i]->AddToTradeVolume(leg_trades_[i].trd_qty_);
    DBGLOG_TIME_CLASS_FUNC << "After: FPPOS: " << fp_position_vec_[i] << " BOMPOS: " << bom_position_vec_[i]
                           << " MYPOS: " << my_position_vec_[i] << DBGLOG_ENDL_FLUSH;
  }
  UpdatePC1Risk();

  // adjust quote, send orders and log
  AdjustMaxPosLimitOfferSizes();
  QuoteAndClose();

  // log trade
  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    // Notifying basepnl here just to have correct value of PC1Risk in tradesfile
    order_manager_vec_[i]->base_pnl().OnRetailExec(
        my_position_vec_[i], leg_trades_[i].trd_qty_, leg_trades_[i].buysell_, leg_trades_[i].trd_px_,
        p_smv_vec_[i]->GetIntPx(leg_trades_[i].trd_px_), p_smv_vec_[i]->security_id());
  }

  if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
    PrintFullStatus();
    for (auto i = 0u; i < p_smv_vec_.size(); i++) {
      order_manager_vec_[i]->LogFullStatus();
    }
  }
}

void RetailFlyTradingManager::OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                                     const double _price_, const int r_int_price_, const int _security_id_) {
  if (secid_idx_map_.find(_security_id_) == secid_idx_map_.end()) {
    DBGLOG_TIME_CLASS_FUNC << "Unknown secid " << _security_id_ << DBGLOG_ENDL_FLUSH;
    return;
  }

  int t_vec_idx_ = secid_idx_map_[_security_id_];

  if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "BEFORE: " << p_smv_vec_[t_vec_idx_]->shortcode()
                           << " FPPOS: " << fp_position_vec_[t_vec_idx_] << " BOMPOS: " << bom_position_vec_[t_vec_idx_]
                           << " MYPOS: " << my_position_vec_[t_vec_idx_] << " TRADE: " << _exec_quantity_ << " "
                           << GetTradeTypeChar(_buysell_) << " @ " << _price_ << DBGLOG_ENDL_FLUSH;
  }

  bom_position_vec_[t_vec_idx_] = _new_position_;
  my_position_vec_[t_vec_idx_] = bom_position_vec_[t_vec_idx_] + fp_position_vec_[t_vec_idx_];

  UpdatePC1Risk();
  AdjustMaxPosLimitOfferSizes();
  QuoteAndClose();

  order_manager_vec_[t_vec_idx_]->base_pnl().OnExec(my_position_vec_[t_vec_idx_], _exec_quantity_, _buysell_, _price_,
                                                    r_int_price_, _security_id_);

  if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "After: "
                           << " FPPOS: " << fp_position_vec_[t_vec_idx_] << " BOMPOS: " << bom_position_vec_[t_vec_idx_]
                           << " MYPOS: " << my_position_vec_[t_vec_idx_] << DBGLOG_ENDL_FLUSH;
  }
}

void RetailFlyTradingManager::TradingLogic() {
  double spread_bestbid_price_ = 0.0;
  double spread_bestask_price_ = 0.0;

  int spread_bestbid_size_ = 0;
  int spread_bestask_size_ = 0;

  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    spread_bestbid_price_ += price_factor_vec_[i] * (price_factor_vec_[i] > 0 ? p_smv_vec_[i]->bestbid_price()
                                                                              : p_smv_vec_[i]->bestask_price());

    spread_bestask_price_ += price_factor_vec_[i] * (price_factor_vec_[i] > 0 ? p_smv_vec_[i]->bestask_price()
                                                                              : p_smv_vec_[i]->bestbid_price());

    spread_bestbid_size_ += price_factor_vec_[i] *
                            (price_factor_vec_[i] > 0 ? p_smv_vec_[i]->bestbid_size() : p_smv_vec_[i]->bestask_size());

    spread_bestask_size_ += price_factor_vec_[i] *
                            (price_factor_vec_[i] > 0 ? p_smv_vec_[i]->bestask_size() : p_smv_vec_[i]->bestbid_size());
  }

  if (last_trade_was_buy_ && (spread_bestask_price_ <= last_buy_price_)) {
    /*    DBGLOG_TIME_CLASS_FUNC << "Last trade buy"
                               << " best ask price: " << spread_bestask_price_ << " BuyPrice: " << last_buy_price_
                               << " best ask size: " << spread_bestask_size_ << " best bid size: " <<
       spread_bestbid_size_ << DBGLOG_ENDL_FLUSH;*/
    for (auto i = 0u; i < p_smv_vec_.size(); i++) {
      if (price_factor_vec_[i] > 0) {
        order_manager_vec_[i]->CancelAsksEqAboveIntPrice(p_smv_vec_[i]->bestask_int_price());
      } else if (price_factor_vec_[i] < 0) {
        order_manager_vec_[i]->CancelBidsEqAboveIntPrice(p_smv_vec_[i]->bestbid_int_price());
      }
    }
    return;
  } else if (last_trade_was_sell_ && (spread_bestbid_price_ >= last_sell_price_)) {
    /*      DBGLOG_TIME_CLASS_FUNC << "Last trade sell"
            << " best buy price: " << spread_bestbid_price_ << " Sell price: " << last_sell_price_ <<
       DBGLOG_ENDL_FLUSH;*/
    for (auto i = 0u; i < p_smv_vec_.size(); i++) {
      if (price_factor_vec_[i] < 0) {
        order_manager_vec_[i]->CancelAsksEqAboveIntPrice(p_smv_vec_[i]->bestask_int_price());
      } else if (price_factor_vec_[i] > 0) {
        order_manager_vec_[i]->CancelBidsEqAboveIntPrice(p_smv_vec_[i]->bestbid_int_price());
      }
    }
    return;
  }
  /*
    DBGLOG_TIME_CLASS_FUNC << "Last trade"
        << " best buy price: " << spread_bestbid_price_
        << " best sell price: " << spread_bestask_price_
        << " Buy price: " << last_buy_price_
        << " Sell price: " << last_sell_price_ << DBGLOG_ENDL_FLUSH; */

  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    /*    deciding size to place     */
    if (my_position_vec_[i] == 0) {
      order_manager_vec_[i]->CancelAllOrders();
    } else if (my_position_vec_[i] > 0) {
      order_manager_vec_[i]->CancelAllBidOrders();

      bool done_for_this_round_ = false;
      // using this instead of mkt_price to avoid problems when spd>1
      double t_ask_placing_bias_ =
          p_smv_vec_[i]->bestask_size() / double(p_smv_vec_[i]->bestbid_size() + p_smv_vec_[i]->bestask_size());

      int top_bid_size_to_hit_ = std::min(my_position_vec_[i], param_set_vec_[i].unit_trade_size_);
      // place size such that potential pc1_risk <= retail_max_pc1_risk_
      top_bid_size_to_hit_ = std::min(top_bid_size_to_hit_,
                                      std::max(0, param_set_vec_[i].retail_max_pc1_risk_ + projected_pc1_risk_vec_[i]));

      if (param_set_vec_[i].allowed_to_aggress_ &&
          (p_smv_vec_[i]->spread_increments() <= param_set_vec_[i].max_int_spread_to_cross_) &&
          (p_smv_vec_[i]->bestbid_size() <
           param_set_vec_[i].max_size_to_aggress_) &&  // if not too high size available on bid size
          t_ask_placing_bias_ > param_set_vec_[i].aggressive_ &&
          // agg_cooloff_interval_ will be very small ~ 5-10 msecs, just to prevent simultaneous aggressive orders
          // across securities
          (last_agg_sell_send_msecs_ == 0 ||
           watch_.msecs_from_midnight() - last_agg_sell_send_msecs_ > common_param_set_.agg_cooloff_interval_) &&
          projected_pc1_risk_vec_[i] > 5) {
        int t_size_already_placed_ =
            order_manager_vec_[i]->SumAskSizeConfirmedEqAboveIntPrice(p_smv_vec_[i]->bestbid_int_price()) +
            order_manager_vec_[i]->SumAskSizeUnconfirmedEqAboveIntPrice(p_smv_vec_[i]->bestbid_int_price());
        if (top_bid_size_to_hit_ > t_size_already_placed_) {
          SendTradeAndLog(i, p_smv_vec_[i]->bestbid_int_price(), top_bid_size_to_hit_ - t_size_already_placed_,
                          kTradeTypeSell, 'A');
          done_for_this_round_ = true;
        }
      }

      if (!done_for_this_round_ && param_set_vec_[i].allowed_to_improve_ &&
          (p_smv_vec_[i]->spread_increments() >= param_set_vec_[i].min_int_spread_to_improve_) &&
          t_ask_placing_bias_ > param_set_vec_[i].improve_) {
        int t_size_already_placed_ =
            order_manager_vec_[i]->SumAskSizeConfirmedEqAboveIntPrice(p_smv_vec_[i]->bestask_int_price() - 1) +
            order_manager_vec_[i]->SumAskSizeUnconfirmedEqAboveIntPrice(p_smv_vec_[i]->bestask_int_price() - 1);
        if (top_bid_size_to_hit_ > t_size_already_placed_) {
          SendTradeAndLog(i, p_smv_vec_[i]->bestask_int_price() - 1, top_bid_size_to_hit_ - t_size_already_placed_,
                          kTradeTypeSell, 'I');
          done_for_this_round_ = true;
        }
      }

      // limiting order size on best level by some %age of mkt size, by default 50%
      int top_ask_size_to_place_ = MathUtils::GetFlooredMultipleOf(
          std::min(double(top_bid_size_to_hit_),
                   param_set_vec_[i].retail_size_factor_to_place_* p_smv_vec_[i]->bestask_size()),
          p_smv_vec_[i]->min_order_size());

      // this will also include any agg/imp order placed above
      int t_best_ask_size_already_placed_ =
          order_manager_vec_[i]->SumAskSizeConfirmedEqAboveIntPrice(p_smv_vec_[i]->bestask_int_price()) +
          order_manager_vec_[i]->SumAskSizeUnconfirmedEqAboveIntPrice(p_smv_vec_[i]->bestask_int_price());

      if (top_ask_size_to_place_ > t_best_ask_size_already_placed_) {
        // done_for_this_round_ is just for sanity, should always go inside this if
        if (!done_for_this_round_ && (p_smv_vec_[i]->bestask_price() - p_smv_vec_[i]->mkt_size_weighted_price() >
                                          0.2 * p_smv_vec_[0]->min_price_increment() ||
                                      projected_pc1_risk_vec_[i] > 5)) {
          SendTradeAndLog(i, p_smv_vec_[i]->bestask_int_price(),
                          top_ask_size_to_place_ - t_best_ask_size_already_placed_, kTradeTypeSell, 'B');
          done_for_this_round_ = true;
        }
      } else {
        // canceling any extra best orders to impose maxpc1risk constraint
        int t_max_best_ask_size_allowed_ = projected_pc1_risk_vec_[i] + param_set_vec_[i].retail_max_pc1_risk_;
        if (t_max_best_ask_size_allowed_ <= 0) {
          // current_pc1_risk is too high, so cancel all best orders and wait for fills in other leg, this should happen
          // only on >=2 level sweeps
          order_manager_vec_[i]->CancelAsksEqAboveIntPrice(p_smv_vec_[i]->bestask_int_price());
        } else if (t_best_ask_size_already_placed_ > t_max_best_ask_size_allowed_) {
          // this will happen when our non-best order becomes best
          order_manager_vec_[i]->KeepAskSizeInPriceRange(t_max_best_ask_size_allowed_,
                                                         p_smv_vec_[i]->bestask_int_price());
        }
      }

      // cancel orders to avoid over-fill in sweeps, orders cancelled here would be mainly non-best
      // this is rather a very liberal check - otherwise we will keep non-best orders for better queue position
      order_manager_vec_[i]->KeepAskSizeInPriceRange(my_position_vec_[i]);
    } else {
      order_manager_vec_[i]->CancelAllAskOrders();

      bool done_for_this_round_ = false;
      // using this instead of mkt_price to avoid problems when spd>1
      double t_bid_placing_bias_ =
          p_smv_vec_[i]->bestbid_size() / double(p_smv_vec_[i]->bestbid_size() + p_smv_vec_[i]->bestask_size());

      int top_ask_size_to_lift_ = std::min(-my_position_vec_[i], param_set_vec_[i].unit_trade_size_);
      // place size such that potential pc1_risk <= retail_max_pc1_risk_
      top_ask_size_to_lift_ = std::min(
          top_ask_size_to_lift_, std::max(0, param_set_vec_[i].retail_max_pc1_risk_ - projected_pc1_risk_vec_[i]));

      if (param_set_vec_[i].allowed_to_aggress_ &&
          (p_smv_vec_[i]->spread_increments() <= param_set_vec_[i].max_int_spread_to_cross_) &&
          (p_smv_vec_[i]->bestask_size() <
           param_set_vec_[i].max_size_to_aggress_) &&  // if not too high size available on ask size
          t_bid_placing_bias_ > param_set_vec_[i].aggressive_ &&
          // agg_cooloff_interval_ will be very small ~ 5-10 msecs, just to prevent simultaneous aggressive orders
          // across securities
          (last_agg_buy_send_msecs_ == 0 ||
           watch_.msecs_from_midnight() - last_agg_buy_send_msecs_ > common_param_set_.agg_cooloff_interval_) &&
          projected_pc1_risk_vec_[i] < -5) {
        int t_size_already_placed_ =
            order_manager_vec_[i]->SumBidSizeConfirmedEqAboveIntPrice(p_smv_vec_[i]->bestask_int_price()) +
            order_manager_vec_[i]->SumBidSizeUnconfirmedEqAboveIntPrice(p_smv_vec_[i]->bestask_int_price());
        if (top_ask_size_to_lift_ > t_size_already_placed_) {
          SendTradeAndLog(i, p_smv_vec_[i]->bestask_int_price(), top_ask_size_to_lift_ - t_size_already_placed_,
                          kTradeTypeBuy, 'A');
          done_for_this_round_ = true;
        }
      }

      if (!done_for_this_round_ && param_set_vec_[i].allowed_to_improve_ &&
          (p_smv_vec_[i]->spread_increments() >= param_set_vec_[i].min_int_spread_to_improve_) &&
          t_bid_placing_bias_ > param_set_vec_[i].improve_) {
        int t_size_already_placed_ =
            order_manager_vec_[i]->SumBidSizeConfirmedEqAboveIntPrice(p_smv_vec_[i]->bestbid_int_price() + 1) +
            order_manager_vec_[i]->SumBidSizeUnconfirmedEqAboveIntPrice(p_smv_vec_[i]->bestbid_int_price() + 1);
        if (top_ask_size_to_lift_ > t_size_already_placed_) {
          SendTradeAndLog(i, p_smv_vec_[i]->bestbid_int_price() + 1, top_ask_size_to_lift_ - t_size_already_placed_,
                          kTradeTypeBuy, 'I');
          done_for_this_round_ = true;
        }
      }

      // limiting order size on best level by some %age of mkt size, by default 50%
      int top_bid_size_to_place_ = MathUtils::GetFlooredMultipleOf(
          std::min(double(top_ask_size_to_lift_),
                   param_set_vec_[i].retail_size_factor_to_place_* p_smv_vec_[i]->bestbid_size()),
          p_smv_vec_[i]->min_order_size());

      // this will also include any agg order placed above
      int t_best_bid_size_already_placed_ =
          order_manager_vec_[i]->SumBidSizeConfirmedEqAboveIntPrice(p_smv_vec_[i]->bestbid_int_price()) +
          order_manager_vec_[i]->SumBidSizeUnconfirmedEqAboveIntPrice(p_smv_vec_[i]->bestbid_int_price());

      if (top_bid_size_to_place_ > t_best_bid_size_already_placed_) {
        // done_for_this_round_ is for sanity, ideally it should never fail this check
        if (!done_for_this_round_ && (p_smv_vec_[i]->mkt_size_weighted_price() - p_smv_vec_[i]->bestbid_price() >
                                          0.2 * p_smv_vec_[0]->min_price_increment() ||
                                      projected_pc1_risk_vec_[i] < -5)) {
          SendTradeAndLog(i, p_smv_vec_[i]->bestbid_int_price(),
                          top_bid_size_to_place_ - t_best_bid_size_already_placed_, kTradeTypeBuy, 'B');
          done_for_this_round_ = true;
        }
      } else {
        // canceling any extra best orders to impose maxpc1risk constraint
        int t_max_best_bid_size_allowed_ = -projected_pc1_risk_vec_[i] + param_set_vec_[i].retail_max_pc1_risk_;
        if (t_max_best_bid_size_allowed_ <= 0) {
          // current_pc1_risk is too high, so cancel all best orders and wait for fills in other leg, this should happen
          // only on >=2 level sweeps
          order_manager_vec_[i]->CancelBidsEqAboveIntPrice(p_smv_vec_[i]->bestbid_int_price());
        } else if (t_best_bid_size_already_placed_ > t_max_best_bid_size_allowed_) {
          // this will happen when our non-best order becomes best
          order_manager_vec_[i]->KeepBidSizeInPriceRange(t_max_best_bid_size_allowed_,
                                                         p_smv_vec_[i]->bestbid_int_price());
        }
      }

      // cancel orders to avoid over-fill in sweeps, orders cancelled here would be mainly non-best
      // this is rather a very liberal check - otherwise we will keep non-best orders for better queue position
      order_manager_vec_[i]->KeepBidSizeInPriceRange(-my_position_vec_[i]);
    }
  }
}

void RetailFlyTradingManager::SendTradeAndLog(int _idx_, int _int_px_, int _size_, TradeType_t _buysell_,
                                              char _level_indicator_) {
  unsigned int t_num_ords_to_send_ = _size_ / param_set_vec_[_idx_].retail_max_ord_size_;
  int t_last_ord_size_ = _size_ % param_set_vec_[_idx_].retail_max_ord_size_;

  if (_size_ > 0 && _level_indicator_ == 'A') {
    if (_buysell_ == kTradeTypeBuy) {
      last_agg_buy_send_msecs_ = watch_.msecs_from_midnight();
    } else if (_buysell_ == kTradeTypeSell) {
      last_agg_sell_send_msecs_ = watch_.msecs_from_midnight();
    }
  }

  for (auto i = 0u; i < t_num_ords_to_send_; i++) {
    order_manager_vec_[_idx_]->SendTradeIntPx(_int_px_, param_set_vec_[_idx_].retail_max_ord_size_, _buysell_,
                                              _level_indicator_);
    if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "SendTrade " << GetTradeTypeChar(_buysell_) << " " << p_smv_vec_[_idx_]->shortcode()
                             << " of " << param_set_vec_[_idx_].retail_max_ord_size_ << " @ IntPx: " << _int_px_
                             << " mkt: " << p_smv_vec_[_idx_]->bestbid_size() << " @ "
                             << p_smv_vec_[_idx_]->bestbid_price() << " X " << p_smv_vec_[_idx_]->bestask_price()
                             << " @ " << p_smv_vec_[_idx_]->bestask_size() << DBGLOG_ENDL_FLUSH;
    }
  }

  if (t_last_ord_size_ > 0) {
    order_manager_vec_[_idx_]->SendTradeIntPx(_int_px_, t_last_ord_size_, _buysell_, _level_indicator_);
    if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "SendTrade " << GetTradeTypeChar(_buysell_) << " " << p_smv_vec_[_idx_]->shortcode()
                             << " of " << t_last_ord_size_ << " @ IntPx: " << _int_px_
                             << " mkt: " << p_smv_vec_[_idx_]->bestbid_size() << " @ "
                             << p_smv_vec_[_idx_]->bestbid_price() << " X " << p_smv_vec_[_idx_]->bestask_price()
                             << " @ " << p_smv_vec_[_idx_]->bestask_size() << DBGLOG_ENDL_FLUSH;
    }
  }
}

void RetailFlyTradingManager::UpdatePC1Risk() {
  double t_pc1_risk_in_dv01_ = 0;
  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    t_pc1_risk_in_dv01_ += my_position_vec_[i] * dv01_vec_[i];
  }
  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    projected_pc1_risk_vec_[i] =
        MathUtils::RoundOff(t_pc1_risk_in_dv01_ / dv01_vec_[i], p_smv_vec_[i]->min_order_size());
  }

  // currently just sending PC1 risk in terms of short leg assuming 100% correlation of legs or 0 PC2 risk
  p_mult_base_pnl_->UpdateTotalRisk(projected_pc1_risk_vec_[p_smv_vec_.size() - 1]);
}

void RetailFlyTradingManager::SMVOnReady() {
  if (!is_ready_) {
    for (auto i = 0u; i < p_smv_vec_.size(); i++) {
      if (p_smv_vec_[i]->is_ready()) {
        if (!is_ready_vec_[i]) {
          is_ready_vec_[i] = true;
          DBGLOG_TIME_CLASS_FUNC << "SACI: " << order_manager_vec_[i]->server_assigned_client_id_ << "("
                                 << p_smv_vec_[i]->shortcode() << ") got ready! shc: " << fly_secname_string_
                                 << " queryid: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        }
        target_price_vec_[i] = p_smv_vec_[i]->mkt_size_weighted_price();
      }
    }

    if (!VectorUtils::LinearSearchValue(is_ready_vec_, false) &&
        watch_.msecs_from_midnight() > trading_start_utc_mfm_) {
      is_ready_ = true;
      DBGLOG_TIME_CLASS_FUNC << "All SACIs got ready! shc: " << fly_secname_string_ << " queryid: " << runtime_id_
                             << DBGLOG_ENDL_FLUSH;
      current_spread_mkt_price_ = 0.0;
      for (auto i = 0u; i < p_smv_vec_.size(); i++) {
        current_spread_mkt_price_ += price_factor_vec_[i] * p_smv_vec_[i]->mkt_size_weighted_price();
      }
      UpdateDv01Ratio();
      QuoteAndClose();
    }
  } else {
    bool propogate_this_update_ = false;

    for (auto i = 0u; i < p_smv_vec_.size(); i++) {
      if (fabs(target_price_vec_[i] - p_smv_vec_[i]->mkt_size_weighted_price()) >
              MIN_PRICE_CHANGE * p_smv_vec_[i]->min_price_increment() ||
          p_smv_vec_[i]->spread_increments() >
              1)  // for slow prods with spd>1, mkt_price wont change even if sizes are changing
      {
        propogate_this_update_ = true;
        break;
      }
    }

    if (propogate_this_update_) {
      current_spread_mkt_price_ = 0.0;
      for (auto i = 0u; i < p_smv_vec_.size(); i++) {
        target_price_vec_[i] = p_smv_vec_[i]->mkt_size_weighted_price();
        current_spread_mkt_price_ += price_factor_vec_[i] * p_smv_vec_[i]->mkt_size_weighted_price();
      }
      QuoteAndClose();
    }
  }
}

void RetailFlyTradingManager::QuoteAndClose() {
  if (is_ready_) {
    ProcessGetFlat();
    ComputeMovingAverage();
    SetOfferVars();
    TradingLogic();
  }
}

void RetailFlyTradingManager::ProcessGetFlat() {
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

bool RetailFlyTradingManager::RetailShouldbeGettingFlat(HFSAT::CDef::RetailUpdateType& _retail_update_type_) {
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
  } else if (getflat_due_to_market_data_interrupt_) {
    _retail_update_type_ = HFSAT::CDef::kunstablemarket;
    return true;
  } else if (getflat_due_to_freeze_) {
    _retail_update_type_ = HFSAT::CDef::kunstablemarket;
    return true;
  }

  _retail_update_type_ = HFSAT::CDef::knormal;
  return false;
}

void RetailFlyTradingManager::UpdateDv01Ratio() {
  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    dv01_vec_[i] = HFSAT::CurveUtils::dv01(p_smv_vec_[i]->shortcode(), watch_.YYYYMMDD(),
                                           p_smv_vec_[i]->mkt_size_weighted_price());
  }

  if (common_param_set_.retail_max_pc1_risk_ <= 0) {
    common_param_set_.retail_max_pc1_risk_ = common_param_set_.unit_trade_size_;
    DBGLOG_CLASS_FUNC_LINE << "Setting MaxPC1Risk of " << fly_secname_string_ << " to same as UTS "
                           << common_param_set_.retail_max_pc1_risk_ << " in " << runtime_id_ << "\n";
  }

  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    double t_dv01_ratio_ = dv01_vec_[p_smv_vec_.size() - 1] / dv01_vec_[i];

    param_set_vec_[i].unit_trade_size_ =
        MathUtils::RoundOff(t_dv01_ratio_ * common_param_set_.unit_trade_size_, p_smv_vec_[i]->min_order_size());
    DBGLOG_CLASS_FUNC_LINE << "Changing UTS of " << p_smv_vec_[i]->shortcode() << " to "
                           << param_set_vec_[i].unit_trade_size_ << " in " << runtime_id_ << "\n";

    param_set_vec_[i].retail_max_pc1_risk_ =
        MathUtils::RoundOff(t_dv01_ratio_ * common_param_set_.retail_max_pc1_risk_, p_smv_vec_[i]->min_order_size());
    DBGLOG_CLASS_FUNC_LINE << "Changing MaxPC1Risk of " << p_smv_vec_[i]->shortcode() << " to "
                           << param_set_vec_[i].retail_max_pc1_risk_ << " in " << runtime_id_ << "\n";

    param_set_vec_[i].max_position_ =
        MathUtils::RoundOff(t_dv01_ratio_ * common_param_set_.max_position_, p_smv_vec_[i]->min_order_size());
    DBGLOG_CLASS_FUNC_LINE << "Changing max_position of " << p_smv_vec_[i]->shortcode() << " to "
                           << param_set_vec_[i].max_position_ << " in " << runtime_id_ << "\n";

    // trade size is in terms of longest leg
    size_factor_vec_[i] = (dv01_vec_[0] / dv01_vec_[i]) * fabs(price_factor_vec_[i]);
  }

  AdjustMaxPosLimitOfferSizes();
}

void RetailFlyTradingManager::OnControlUpdate(const ControlMessage& _control_message_, const char* symbol,
                                              const int trader_id) {
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
      if (_control_message_.intval_1_ > common_param_set_.unit_trade_size_ / FAT_FINGER_FACTOR &&
          _control_message_.intval_1_ < common_param_set_.unit_trade_size_ * FAT_FINGER_FACTOR) {
        common_param_set_.unit_trade_size_ =
            std::max(p_smv_vec_[p_smv_vec_.size() - 1]->min_order_size(),
                     HFSAT::MathUtils::GetFlooredMultipleOf(_control_message_.intval_1_,
                                                            p_smv_vec_[p_smv_vec_.size() - 1]->min_order_size()));

        for (auto i = 0u; i < p_smv_vec_.size(); i++) {
          double t_dv01_ratio_ = dv01_vec_[p_smv_vec_.size() - 1] / dv01_vec_[i];
          param_set_vec_[i].unit_trade_size_ = std::max(
              p_smv_vec_[i]->min_order_size(),
              MathUtils::RoundOff(t_dv01_ratio_ * common_param_set_.unit_trade_size_, p_smv_vec_[i]->min_order_size()));
          DBGLOG_CLASS_FUNC_LINE << "Changing UTS of " << p_smv_vec_[i]->shortcode() << " to "
                                 << param_set_vec_[i].unit_trade_size_ << " in " << runtime_id_ << "\n";
        }
      }
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "SetUnitTradeSize " << runtime_id_ << " called with " << _control_message_.intval_1_
                          << " and UnitTradeSize for commonparam set to " << common_param_set_.unit_trade_size_ << ""
                          << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
    } break;
    case kControlMessageCodeAddPosition: {
      DBGLOG_TIME_CLASS << "kControlMessageCodeAddPosition called for " << _control_message_.strval_1_ << " with "
                        << _control_message_.intval_1_ << " in strat for " << fly_secname_string_ << "(" << runtime_id_
                        << ")" << DBGLOG_ENDL_FLUSH;
      AddPosition(std::string(_control_message_.strval_1_), _control_message_.intval_1_);
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    } break;
    case kControlMessageCodeSetMaxUnitRatio: {
      if (int(_control_message_.doubleval_1_ * common_param_set_.unit_trade_size_ + 0.5) >
              common_param_set_.max_position_ / FAT_FINGER_FACTOR &&
          int(_control_message_.doubleval_1_ * common_param_set_.unit_trade_size_ + 0.5) <
              common_param_set_.max_position_ * FAT_FINGER_FACTOR) {
        common_param_set_.max_position_ =
            std::max(p_smv_vec_[p_smv_vec_.size() - 1]->min_order_size(),
                     int(_control_message_.doubleval_1_ * common_param_set_.unit_trade_size_ + 0.5));
        for (auto i = 0u; i < p_smv_vec_.size(); i++) {
          double t_dv01_ratio_ = dv01_vec_[p_smv_vec_.size() - 1] / dv01_vec_[i];
          param_set_vec_[i].max_position_ = std::max(
              p_smv_vec_[i]->min_order_size(),
              MathUtils::RoundOff(t_dv01_ratio_ * common_param_set_.max_position_, p_smv_vec_[i]->min_order_size()));
          DBGLOG_CLASS_FUNC_LINE << "Changing max_position of " << p_smv_vec_[i]->shortcode() << " to "
                                 << param_set_vec_[i].max_position_ << " in " << runtime_id_ << "\n";
        }
        AdjustMaxPosLimitOfferSizes();
      }
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "SetMaxUnitRatio " << runtime_id_ << " called with " << _control_message_.doubleval_1_
                          << " and MaxPosition for commonparam set to " << common_param_set_.max_position_ << ""
                          << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
    } break;
    case kControlMessageCodeSetMaxGlobalRisk: {
      // dummy for maxpc1 risk
      if (_control_message_.intval_1_ > common_param_set_.retail_max_pc1_risk_ / FAT_FINGER_FACTOR &&
          _control_message_.intval_1_ < common_param_set_.retail_max_pc1_risk_ / FAT_FINGER_FACTOR) {
        common_param_set_.retail_max_pc1_risk_ =
            std::max(p_smv_vec_[p_smv_vec_.size() - 1]->min_order_size(), _control_message_.intval_1_);

        for (auto i = 0u; i < p_smv_vec_.size(); i++) {
          double t_dv01_ratio_ = dv01_vec_[p_smv_vec_.size() - 1] / dv01_vec_[i];
          param_set_vec_[i].retail_max_pc1_risk_ =
              std::max(p_smv_vec_[i]->min_order_size(),
                       MathUtils::RoundOff(t_dv01_ratio_ * common_param_set_.retail_max_pc1_risk_,
                                           p_smv_vec_[i]->min_order_size()));
          DBGLOG_CLASS_FUNC_LINE << "Changing MaxPC1Risk of " << p_smv_vec_[i]->shortcode() << " to "
                                 << param_set_vec_[i].retail_max_pc1_risk_ << " in " << runtime_id_ << "\n";
        }
      }
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "SetMaxGlobalRisk " << runtime_id_ << " called with " << _control_message_.intval_1_
                          << " and MaxPC1Risk for commonparam set to " << common_param_set_.retail_max_pc1_risk_ << ""
                          << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
    } break;
    case kControlMessageCodeSetMaxPosition: {
      if (_control_message_.intval_1_ > common_param_set_.max_position_ / FAT_FINGER_FACTOR &&
          _control_message_.intval_1_ < common_param_set_.max_position_ * FAT_FINGER_FACTOR) {
        common_param_set_.max_position_ =
            std::max(p_smv_vec_[p_smv_vec_.size() - 1]->min_order_size(), _control_message_.intval_1_);

        for (auto i = 0u; i < p_smv_vec_.size(); i++) {
          double t_dv01_ratio_ = dv01_vec_[p_smv_vec_.size() - 1] / dv01_vec_[i];
          param_set_vec_[i].max_position_ = std::max(
              p_smv_vec_[i]->min_order_size(),
              MathUtils::RoundOff(t_dv01_ratio_ * common_param_set_.max_position_, p_smv_vec_[i]->min_order_size()));
          DBGLOG_CLASS_FUNC_LINE << "Changing max_position of " << p_smv_vec_[i]->shortcode() << " to "
                                 << param_set_vec_[i].max_position_ << " in " << runtime_id_ << "\n";
        }
        AdjustMaxPosLimitOfferSizes();
      }
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "SetMaxUnitRatio " << runtime_id_ << " called with " << _control_message_.intval_1_
                          << " and MaxPosition for commonparam set to " << common_param_set_.max_position_ << ""
                          << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
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
    case kControlMessageCodeSetMaxLoss: {
      if (_control_message_.intval_1_ > common_param_set_.max_loss_ / FAT_FINGER_FACTOR &&
          _control_message_.intval_1_ < common_param_set_.max_loss_ * FAT_FINGER_FACTOR) {
        common_param_set_.max_loss_ = _control_message_.intval_1_;
      }

      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "SetMaxLoss " << runtime_id_
                          << " called for abs_max_loss_ = " << _control_message_.intval_1_
                          << " and MaxLoss for commonparam set to " << common_param_set_.max_loss_ << DBGLOG_ENDL_FLUSH;

        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
    } break;

    case kControlMessageCodeShowOrders: {
      DBGLOG_TIME_CLASS << "ShowOrders " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
      PrintFullStatus();
      for (auto i = 0u; i < p_smv_vec_.size(); i++) {
        order_manager_vec_[i]->LogFullStatus();
      }
      if (livetrading_) {
        DBGLOG_DUMP;
      }
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
      for (auto i = 0u; i < p_smv_vec_.size(); i++) {
        economic_events_manager_.AllowEconomicEventsFromList(p_smv_vec_[i]->shortcode());
        allowed_events_present_ = economic_events_manager_.AllowedEventsPresent();

        // allowedeco load work only for one of the two legs, as following function clears the vec
        if (allowed_events_present_) {
          break;
        }
      }
      economic_events_manager_.AdjustSeverity(p_smv_vec_[0]->shortcode(), p_smv_vec_[0]->exch_source());
    } break;

    case kControlMessageShowEconomicEvents: {
      DBGLOG_TIME_CLASS << "Showecoevents user_msg " << DBGLOG_ENDL_FLUSH;
      economic_events_manager_.ShowDB();
    } break;

    default:
      break;
  }
}

void RetailFlyTradingManager::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                      const int msecs_since_last_receive_) {
  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    if (_security_id_ == p_smv_vec_[i]->security_id() && !is_data_interrupted_vec_[i]) {
      is_data_interrupted_vec_[i] = true;
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC << "Setting getflat_due_to_market_data_interrupt_ = true for "
                               << p_smv_vec_[i]->shortcode() << " in queryid: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      getflat_due_to_market_data_interrupt_ = true;
      QuoteAndClose();
      break;
    }
  }
}

void RetailFlyTradingManager::OnMarketDataResumed(const unsigned int _security_id_) {
  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    if (_security_id_ == p_smv_vec_[i]->security_id() && is_data_interrupted_vec_[i]) {
      is_data_interrupted_vec_[i] = false;
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC << "market_data_resumed for " << p_smv_vec_[i]->shortcode()
                               << " in queryid: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      break;
    }
  }

  getflat_due_to_market_data_interrupt_ = !HFSAT::VectorUtils::CheckAllForValue(is_data_interrupted_vec_, false);
  if (!getflat_due_to_market_data_interrupt_) {
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "Setting getflat_due_to_market_data_interrupt_ = false in queryid: " << runtime_id_
                             << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
    // SMVOnReady(); this willl get called on smv ready
  }
}

void RetailFlyTradingManager::CheckParamSet() {
  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    if (!param_set_vec_[i].read_zeropos_place_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "zeropos_place_");
    }
    if (param_set_vec_[i].allowed_to_aggress_ && !param_set_vec_[i].read_aggressive_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "aggressive_");
    }
    if (param_set_vec_[i].allowed_to_improve_ && !param_set_vec_[i].read_improve_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "improve_");
    }
    if (!param_set_vec_[i].read_max_global_position_) {
      ExitVerbose(kStrategyDescParamFileIncomplete, "max_global_position_");
    }
  }

  if (!common_param_set_.read_max_position_) {
    ExitVerbose(kStrategyDescParamFileIncomplete, "max_position_ in common_param_set_");
  }
  if (!common_param_set_.read_max_global_position_) {
    // ExitVerbose ( kStrategyDescParamFileIncomplete, "max_global_position_ in common_param_set_" );
  }
  if (!common_param_set_.read_unit_trade_size_) {
    ExitVerbose(kStrategyDescParamFileIncomplete, "unit_trade_size_ in common_param_set_");
  }
  if (!common_param_set_.read_max_loss_ && livetrading_) {
    ExitVerbose(kStrategyDescParamFileIncomplete, "max_loss_ in common_param_set_");
  }
  if (!common_param_set_.read_zeropos_place_) {
    ExitVerbose(kStrategyDescParamFileIncomplete, "zeropos_place_ in common_param_set_");
  }
}

void RetailFlyTradingManager::SetOrderManager(SmartOrderManager* _p_om_, const std::string& _shc_) {
  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    if (p_smv_vec_[i]->shortcode() == _shc_ && order_manager_vec_[i] == NULL) {
      order_manager_vec_[i] = _p_om_;
      order_manager_vec_[i]->AddExecutionListener(this);
      DBGLOG_CLASS_FUNC << "Setting OM for " << _shc_ << DBGLOG_ENDL_FLUSH;

      order_manager_vec_[i]->RemoveExecutionLister(&(order_manager_vec_[i]->base_pnl()));
      DBGLOG_CLASS_FUNC << "Removing BasePnl from ExecutionListeners of OM for " << _shc_ << DBGLOG_ENDL_FLUSH;

      break;
    }
  }
}

void RetailFlyTradingManager::ReconcileParams() {
  double min_fly_spd_ = 0.0;
  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    // sanity range checks, in case parmfile has garbage values
    // using for quoting
    param_set_vec_[i].zeropos_place_ = std::max(0.01, param_set_vec_[i].zeropos_place_);
    param_set_vec_[i].zeropos_place_ *= p_smv_vec_[i]->min_price_increment();

    if (!param_set_vec_[i].read_owp_retail_offer_thresh_) {
      // 0.25 => 0.2 , taking more liberal threshold for spd/fly than outrights
      param_set_vec_[i].owp_retail_offer_thresh_ = 0.2;
    } else {
      param_set_vec_[i].owp_retail_offer_thresh_ = std::max(0.01, param_set_vec_[i].owp_retail_offer_thresh_);
    }

    param_set_vec_[i].owp_retail_offer_thresh_ *= p_smv_vec_[i]->min_price_increment();

    // using for closing
    param_set_vec_[i].improve_ = std::max(0.6, param_set_vec_[i].improve_);
    if (param_set_vec_[i].allowed_to_improve_) {
      param_set_vec_[i].aggressive_ =
          std::max(std::max(0.8, param_set_vec_[i].improve_), param_set_vec_[i].aggressive_);
    } else {
      param_set_vec_[i].aggressive_ = std::max(0.8, param_set_vec_[i].aggressive_);
    }

    param_set_vec_[i].retail_max_ord_size_ =
        std::max(1, param_set_vec_[i].retail_max_ord_size_ / p_smv_vec_[i]->min_order_size()) *
        p_smv_vec_[i]->min_order_size();
    min_fly_spd_ += fabs(price_factor_vec_[i]) * p_smv_vec_[i]->min_price_increment();
  }

  // uts specified is used for shorter expiry
  common_param_set_.unit_trade_size_ =
      std::max(1, common_param_set_.unit_trade_size_ / p_smv_vec_[p_smv_vec_.size() - 1]->min_order_size()) *
      p_smv_vec_[p_smv_vec_.size() - 1]->min_order_size();
  common_param_set_.zeropos_place_ = std::max(0.01, common_param_set_.zeropos_place_) * min_fly_spd_;

  if (!common_param_set_.read_agg_cooloff_interval_) {
    common_param_set_.read_agg_cooloff_interval_ =
        10;  // default agg_cooloff is only 10msecs , this is just to avoid simultaneous aggressing across securities
  }
}

void RetailFlyTradingManager::SetOfferVars() {
  // initialize retailoffer struct
  HFSAT::CDef::RetailOffer this_retail_offer_;
  this_retail_offer_.offered_bid_price_ = MIN_INVALID_PRICE;
  this_retail_offer_.offered_ask_price_ = MAX_INVALID_PRICE;
  this_retail_offer_.offered_bid_size_ = 0;
  this_retail_offer_.offered_ask_size_ = 0;
  this_retail_offer_.retail_update_type_ = HFSAT::CDef::knormal;
  this_retail_offer_.product_split_.product_type_ = HFSAT::CDef::spread;

  if (p_smv_vec_.size() == 2) {
    this_retail_offer_.product_split_.product_type_ = HFSAT::CDef::spread;
  } else if (p_smv_vec_.size() == 3) {
    this_retail_offer_.product_split_.product_type_ = HFSAT::CDef::butterfly;
  }

  // This code depends on how enums are defined. If we switch the order of
  // spread and butterfly for instance this will be an error.
  // We should write code with less dependencies.
  // TODO: anshul
  for (auto i = 0u; i < this_retail_offer_.product_split_.product_type_; i++) {
    strcpy(this_retail_offer_.product_split_.sub_product_bid_[i].shortcode_, p_smv_vec_[i]->shortcode().c_str());
    strcpy(this_retail_offer_.product_split_.sub_product_bid_[i].security_name_, p_smv_vec_[i]->secname());
    this_retail_offer_.product_split_.sub_product_bid_[i].product_price_ = MAX_INVALID_PRICE;
    this_retail_offer_.product_split_.sub_product_bid_[i].product_size_ = 0;
    this_retail_offer_.product_split_.sub_product_bid_[i].buysell_ =
        price_factor_vec_[i] > 0.0 ? kTradeTypeSell : kTradeTypeBuy;

    strcpy(this_retail_offer_.product_split_.sub_product_ask_[i].shortcode_, p_smv_vec_[i]->shortcode().c_str());
    strcpy(this_retail_offer_.product_split_.sub_product_ask_[i].security_name_, p_smv_vec_[i]->secname());
    this_retail_offer_.product_split_.sub_product_ask_[i].product_price_ = MAX_INVALID_PRICE;
    this_retail_offer_.product_split_.sub_product_ask_[i].product_size_ = 0;
    this_retail_offer_.product_split_.sub_product_ask_[i].buysell_ =
        price_factor_vec_[i] > 0.0 ? kTradeTypeBuy : kTradeTypeSell;
  }

  // check for getflat condition
  if (should_be_getting_flat_) {
    // control wont come out of this if block without returning
    // send invalid prices in getflat situations
    this_retail_offer_.retail_update_type_ = getflat_retail_update_type_;
    NotifyListeners(this_retail_offer_);
    return;
  }

  if (p_smv_vec_.size() == 2) {
    ComputeSpreadOffer(this_retail_offer_);
  } else if (p_smv_vec_.size() == 3) {
    ComputeFlyOffer(this_retail_offer_);
  } else if (p_smv_vec_.size() == 4) {
    ComputeFlyOffer(this_retail_offer_);
  }

  // notify the listeners
  NotifyListeners(this_retail_offer_);
}

void RetailFlyTradingManager::ComputeSpreadOffer(HFSAT::CDef::RetailOffer& _this_retail_offer_) {
  // computing prices of Spread
  double spread_bestbid_price_ = 0;
  double spread_bestask_price_ = 0;
  double spread_mid_price_ = 0;
  double target_price_ = 0;
  int spread_bidsize_to_offer_ = 0;
  double spread_offer_bid_price_ = spread_bestbid_price_;
  int spread_asksize_to_offer_ = 0;
  double spread_offer_ask_price_ = spread_bestask_price_;

  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    spread_bestbid_price_ += price_factor_vec_[i] * (price_factor_vec_[i] > 0 ? p_smv_vec_[i]->bestbid_price()
                                                                              : p_smv_vec_[i]->bestask_price());
    spread_bestask_price_ += price_factor_vec_[i] * (price_factor_vec_[i] > 0 ? p_smv_vec_[i]->bestask_price()
                                                                              : p_smv_vec_[i]->bestbid_price());
    spread_mid_price_ += price_factor_vec_[i] * p_smv_vec_[i]->mid_price();
    target_price_ += price_factor_vec_[i] * p_smv_vec_[i]->mkt_size_weighted_price();
  }

  /*  DBGLOG_TIME_CLASS_FUNC << " Moving Avg: " << moving_avg_spread_ <<
      " CurrentMktPrice: " << target_price_ << DBGLOG_ENDL_NOFLUSH;*/

  // checking price cutt-offs for offering bid
  ComputeSpreadBidOffer(spread_offer_bid_price_, spread_bidsize_to_offer_, spread_bestbid_price_, target_price_);

  // checking price cutt-offs for offering ask
  ComputeSpreadAskOffer(spread_offer_ask_price_, spread_asksize_to_offer_, spread_bestask_price_, target_price_);
  UpdateSizeRatio();

  // setting px,size,type of retailoffer struct
  if (spread_bidsize_to_offer_ <= 0 && spread_asksize_to_offer_ <= 0) {
    _this_retail_offer_.retail_update_type_ = HFSAT::CDef::kSubBestBidAsk;
    _this_retail_offer_.offered_bid_price_ = spread_bestbid_price_;
    _this_retail_offer_.offered_ask_price_ = spread_bestask_price_;
  } else if (spread_bidsize_to_offer_ <= 0) {
    _this_retail_offer_.retail_update_type_ = HFSAT::CDef::kSubBestBid;
    _this_retail_offer_.offered_bid_price_ = spread_bestbid_price_;
  } else if (spread_asksize_to_offer_ <= 0) {
    _this_retail_offer_.retail_update_type_ = HFSAT::CDef::kSubBestAsk;
    _this_retail_offer_.offered_ask_price_ = spread_bestask_price_;
  }

  if (spread_bidsize_to_offer_ > 0) {
    _this_retail_offer_.offered_bid_size_ = spread_bidsize_to_offer_;
    _this_retail_offer_.offered_bid_price_ = spread_offer_bid_price_;
  } else {
    // Offering min size at second level
    _this_retail_offer_.offered_bid_size_ = p_smv_vec_[0]->min_order_size();
    _this_retail_offer_.offered_bid_price_ = spread_bestbid_price_ - min_min_price_increment_to_use_;
  }

  if (spread_asksize_to_offer_ > 0) {
    _this_retail_offer_.offered_ask_size_ = spread_asksize_to_offer_;
    _this_retail_offer_.offered_ask_price_ = spread_offer_ask_price_;
  } else {
    // Offering min size at second level
    _this_retail_offer_.offered_ask_size_ = p_smv_vec_[0]->min_order_size();
    _this_retail_offer_.offered_ask_price_ = spread_bestask_price_ + min_min_price_increment_to_use_;
  }

  last_retail_offered_bidprice_ = _this_retail_offer_.offered_bid_price_;
  last_retail_offered_askprice_ = _this_retail_offer_.offered_ask_price_;

  FillLegInfo(_this_retail_offer_);

  if (last_retail_offer_bid_vec_.size() >= 5) {
    last_retail_offer_bid_price_vec_.erase(last_retail_offer_bid_price_vec_.begin());
    last_retail_offer_bid_vec_.erase(last_retail_offer_bid_vec_.begin());
    bid_size_ratio_vec_vec_.erase(bid_size_ratio_vec_vec_.begin());
  }

  int t_bid_offer_index_ = GetIndex(last_retail_offer_bid_price_vec_, _this_retail_offer_.offered_bid_price_);
  if (t_bid_offer_index_ == -1) {
    last_retail_offer_bid_price_vec_.push_back(_this_retail_offer_.offered_bid_price_);
    last_retail_offer_bid_vec_.push_back(_this_retail_offer_);
    bid_size_ratio_vec_vec_.push_back(size_ratio_vec_);
  } else {
    last_retail_offer_bid_price_vec_[t_bid_offer_index_] = _this_retail_offer_.offered_bid_price_;
    last_retail_offer_bid_vec_[t_bid_offer_index_] = _this_retail_offer_;
    bid_size_ratio_vec_vec_[t_bid_offer_index_] = size_ratio_vec_;
  }
  
  if (last_retail_offer_ask_vec_.size() >= 5) {
    last_retail_offer_ask_price_vec_.erase(last_retail_offer_ask_price_vec_.begin());
    last_retail_offer_ask_vec_.erase(last_retail_offer_ask_vec_.begin());
    ask_size_ratio_vec_vec_.erase(ask_size_ratio_vec_vec_.begin());
  }

  int t_ask_offer_index_ = GetIndex(last_retail_offer_ask_price_vec_, _this_retail_offer_.offered_ask_price_);
  if (t_ask_offer_index_ == -1) {
    last_retail_offer_ask_price_vec_.push_back(_this_retail_offer_.offered_ask_price_);
    last_retail_offer_ask_vec_.push_back(_this_retail_offer_);
    ask_size_ratio_vec_vec_.push_back(size_ratio_vec_);
  } else {
    last_retail_offer_ask_price_vec_[t_ask_offer_index_] = _this_retail_offer_.offered_ask_price_;
    last_retail_offer_ask_vec_[t_ask_offer_index_] = _this_retail_offer_;
    ask_size_ratio_vec_vec_[t_ask_offer_index_] = size_ratio_vec_;
  }
}

void RetailFlyTradingManager::ComputeSpreadBidOffer(double& _spread_offer_bid_price_, int& _spread_bidsize_to_offer_,
                                                    double _spread_bestbid_price_, double _target_price_) {
  // deciding bidsize to offer if all cut-offs cleared

  bool last_bidoffer_was_at_bid_plus_one_ = MathUtils::DblPxCompare(
      last_retail_offered_bidprice_, _spread_bestbid_price_ + min_min_price_increment_to_use_ / 2.0,
      min_min_price_increment_to_use_ / 2.0);
  bool last_bidoffer_was_at_bid_ = MathUtils::DblPxCompare(last_retail_offered_bidprice_, _spread_bestbid_price_,
                                                           min_min_price_increment_to_use_ / 2.0);

  if ((_target_price_ - _spread_bestbid_price_ - min_min_price_increment_to_use_ / 2.0 >
           common_param_set_.zeropos_place_ ||
       (last_bidoffer_was_at_bid_plus_one_ &&
        _target_price_ - _spread_bestbid_price_ - min_min_price_increment_to_use_ >
            (1 - common_param_set_.retail_price_threshold_tolerance_) * common_param_set_.zeropos_place_))) {
    _spread_bidsize_to_offer_ = 9999999;
    // agg in first best in second
    int t_size_to_offer_agg1_best2_ = 9999999;
    int t_size_to_offer_best1_agg2_ = 9999999;

    for (auto i = 0u; i < p_smv_vec_.size(); i++) {
      int t_max_size_proj_in_spd_agg1_best2_ = 0;
      int t_max_size_proj_in_spd_best1_agg2_ = 0;

      if (price_factor_vec_[i] > 0) {
        int t_max_bid_size_long_leg_agg_ =
            std::min(std::max(0, int(p_smv_vec_[i]->bestask_size() * param_set_vec_[i].retail_size_factor_to_offer_) -
                                     my_position_vec_[i]),
                     std::min(bidsize_to_show_maxpos_limit_vec_[i], bidsize_to_show_global_maxpos_limit_vec_[i]));
        t_max_size_proj_in_spd_agg1_best2_ = HFSAT::MathUtils::GetFlooredMultipleOf(
            t_max_bid_size_long_leg_agg_ * dv01_vec_[i] / dv01_vec_[0], p_smv_vec_[i]->min_order_size());

        int t_max_bid_size_long_leg_best_ =
            std::min(std::max(0, int(p_smv_vec_[i]->bestbid_size() * param_set_vec_[i].retail_size_factor_to_offer_) -
                                     my_position_vec_[i]),
                     std::min(bidsize_to_show_maxpos_limit_vec_[i], bidsize_to_show_global_maxpos_limit_vec_[i]));
        t_max_size_proj_in_spd_best1_agg2_ = HFSAT::MathUtils::GetFlooredMultipleOf(
            t_max_bid_size_long_leg_best_ * dv01_vec_[i] / dv01_vec_[0], p_smv_vec_[i]->min_order_size());
      } else {
        int t_max_ask_size_short_leg_best_ =
            std::min(std::max(0, int(p_smv_vec_[i]->bestask_size() * param_set_vec_[i].retail_size_factor_to_offer_) +
                                     my_position_vec_[i]),
                     std::min(asksize_to_show_maxpos_limit_vec_[i], asksize_to_show_global_maxpos_limit_vec_[i]));
        t_max_size_proj_in_spd_agg1_best2_ = HFSAT::MathUtils::GetFlooredMultipleOf(
            t_max_ask_size_short_leg_best_ * dv01_vec_[i] / dv01_vec_[0], p_smv_vec_[i]->min_order_size());

        int t_max_ask_size_short_leg_agg_ =
            std::min(std::max(0, int(p_smv_vec_[i]->bestbid_size() * param_set_vec_[i].retail_size_factor_to_offer_) +
                                     my_position_vec_[i]),
                     std::min(asksize_to_show_maxpos_limit_vec_[i], asksize_to_show_global_maxpos_limit_vec_[i]));
        t_max_size_proj_in_spd_best1_agg2_ = HFSAT::MathUtils::GetFlooredMultipleOf(
            t_max_ask_size_short_leg_agg_ * dv01_vec_[i] / dv01_vec_[0], p_smv_vec_[i]->min_order_size());
      }

      t_size_to_offer_agg1_best2_ = std::min(t_size_to_offer_agg1_best2_, t_max_size_proj_in_spd_agg1_best2_);
      t_size_to_offer_best1_agg2_ = std::min(t_size_to_offer_best1_agg2_, t_max_size_proj_in_spd_best1_agg2_);
    }

    _spread_bidsize_to_offer_ =
        std::min(_spread_bidsize_to_offer_, std::max(t_size_to_offer_agg1_best2_, t_size_to_offer_best1_agg2_));

    if (_spread_bidsize_to_offer_ > 0 && last_bidoffer_was_at_bid_plus_one_ &&
        abs(last_retail_offer_.offered_bid_size_ - _spread_bidsize_to_offer_) <
            common_param_set_.retail_size_tolerance_ * last_retail_offer_.offered_bid_size_) {
      _spread_bidsize_to_offer_ = last_retail_offer_.offered_bid_size_;
    }

    if (_spread_bidsize_to_offer_ > 0) {
      _spread_offer_bid_price_ = _spread_bestbid_price_ + min_min_price_increment_to_use_ / 2.0;
      if (_target_price_ - _spread_bestbid_price_ - min_min_price_increment_to_use_ >
          common_param_set_.zeropos_place_) {
        _spread_offer_bid_price_ = _spread_bestbid_price_ + min_min_price_increment_to_use_;
      }
      // DBGLOG_TIME_CLASS_FUNC << " Bid Agg: " << _spread_offer_bid_price_ <<
      //  " Size: " << _spread_bidsize_to_offer_ << DBGLOG_ENDL_NOFLUSH;
    }
  }

  if (_spread_bidsize_to_offer_ <= 0 &&
      (_target_price_ - _spread_bestbid_price_ > common_param_set_.zeropos_place_ ||
       (last_bidoffer_was_at_bid_ &&
        _target_price_ - _spread_bestbid_price_ >
            (1 - common_param_set_.retail_price_threshold_tolerance_) * common_param_set_.zeropos_place_))) {
    _spread_bidsize_to_offer_ = 9999999;
    for (auto i = 0u; i < p_smv_vec_.size(); i++) {
      int t_max_size_proj_in_spd_ = 0;
      if (price_factor_vec_[i] > 0) {
        int t_max_bid_size_long_leg_ =
            std::min(std::max(0, int(p_smv_vec_[i]->bestbid_size() * param_set_vec_[i].retail_size_factor_to_offer_) -
                                     my_position_vec_[i]),
                     std::min(bidsize_to_show_maxpos_limit_vec_[i], bidsize_to_show_global_maxpos_limit_vec_[i]));
        t_max_size_proj_in_spd_ = HFSAT::MathUtils::GetFlooredMultipleOf(
            t_max_bid_size_long_leg_ * dv01_vec_[i] / dv01_vec_[0], p_smv_vec_[i]->min_order_size());
      } else {
        int t_max_ask_size_short_leg_ =
            std::min(std::max(0, int(p_smv_vec_[i]->bestask_size() * param_set_vec_[i].retail_size_factor_to_offer_) +
                                     my_position_vec_[i]),
                     std::min(asksize_to_show_maxpos_limit_vec_[i], asksize_to_show_global_maxpos_limit_vec_[i]));
        t_max_size_proj_in_spd_ = HFSAT::MathUtils::GetFlooredMultipleOf(
            t_max_ask_size_short_leg_ * dv01_vec_[i] / dv01_vec_[0], p_smv_vec_[i]->min_order_size());
      }

      _spread_bidsize_to_offer_ = std::min(_spread_bidsize_to_offer_, t_max_size_proj_in_spd_);
      if (_spread_bidsize_to_offer_ <= 0) {
        break;
      }
    }

    if (_spread_bidsize_to_offer_ > 0 && last_bidoffer_was_at_bid_ &&
        abs(last_retail_offer_.offered_bid_size_ - _spread_bidsize_to_offer_) <
            common_param_set_.retail_size_tolerance_ * last_retail_offer_.offered_bid_size_) {
      _spread_bidsize_to_offer_ = last_retail_offer_.offered_bid_size_;
    }

    if (_spread_bidsize_to_offer_ > 0) {
      _spread_offer_bid_price_ = _spread_bestbid_price_;
    }
  }
}

void RetailFlyTradingManager::ComputeSpreadAskOffer(double& _spread_offer_ask_price_, int& _spread_asksize_to_offer_,
                                                    double _spread_bestask_price_, double _target_price_) {
  bool last_askoffer_was_at_ask_minus_one_ = MathUtils::DblPxCompare(
      last_retail_offered_askprice_, _spread_bestask_price_ - min_min_price_increment_to_use_ / 2.0,
      min_min_price_increment_to_use_ / 2.0);
  bool last_askoffer_was_at_ask_ = MathUtils::DblPxCompare(last_retail_offered_askprice_, _spread_bestask_price_,
                                                           min_min_price_increment_to_use_ / 2.0);

  // deciding asksize to offer if all cut-offs cleared
  if ((_spread_bestask_price_ - min_min_price_increment_to_use_ / 2.0 - _target_price_ >
           common_param_set_.zeropos_place_ ||
       (last_askoffer_was_at_ask_minus_one_ &&
        _spread_bestask_price_ - min_min_price_increment_to_use_ - _target_price_ >
            (1 - common_param_set_.retail_price_threshold_tolerance_) * common_param_set_.zeropos_place_))) {
    _spread_asksize_to_offer_ = 9999999;
    int t_size_to_offer_agg1_best2_ = 9999999;
    int t_size_to_offer_best1_agg2_ = 9999999;
    for (auto i = 0u; i < p_smv_vec_.size(); i++) {
      int t_max_size_proj_in_spd_agg1_best2_ = 0;
      int t_max_size_proj_in_spd_best1_agg2_ = 0;
      if (price_factor_vec_[i] <= 0) {
        int t_max_bid_size_long_leg_best_ =
            std::min(std::max(0, int(p_smv_vec_[i]->bestbid_size() * param_set_vec_[i].retail_size_factor_to_offer_) -
                                     my_position_vec_[i]),
                     std::min(bidsize_to_show_maxpos_limit_vec_[i], bidsize_to_show_global_maxpos_limit_vec_[i]));
        t_max_size_proj_in_spd_agg1_best2_ = HFSAT::MathUtils::GetFlooredMultipleOf(
            t_max_bid_size_long_leg_best_ * dv01_vec_[i] / dv01_vec_[0], p_smv_vec_[i]->min_order_size());

        int t_max_bid_size_long_leg_agg_ =
            std::min(std::max(0, int(p_smv_vec_[i]->bestask_size() * param_set_vec_[i].retail_size_factor_to_offer_) -
                                     my_position_vec_[i]),
                     std::min(bidsize_to_show_maxpos_limit_vec_[i], bidsize_to_show_global_maxpos_limit_vec_[i]));
        t_max_size_proj_in_spd_best1_agg2_ = HFSAT::MathUtils::GetFlooredMultipleOf(
            t_max_bid_size_long_leg_agg_ * dv01_vec_[i] / dv01_vec_[0], p_smv_vec_[i]->min_order_size());
      } else {
        int t_max_ask_size_short_leg_agg_ =
            std::min(std::max(0, int(p_smv_vec_[i]->bestbid_size() * param_set_vec_[i].retail_size_factor_to_offer_) +
                                     my_position_vec_[i]),
                     std::min(asksize_to_show_maxpos_limit_vec_[i], asksize_to_show_global_maxpos_limit_vec_[i]));
        t_max_size_proj_in_spd_agg1_best2_ = HFSAT::MathUtils::GetFlooredMultipleOf(
            t_max_ask_size_short_leg_agg_ * dv01_vec_[i] / dv01_vec_[0], p_smv_vec_[i]->min_order_size());

        int t_max_ask_size_short_leg_best_ =
            std::min(std::max(0, int(p_smv_vec_[i]->bestask_size() * param_set_vec_[i].retail_size_factor_to_offer_) +
                                     my_position_vec_[i]),
                     std::min(asksize_to_show_maxpos_limit_vec_[i], asksize_to_show_global_maxpos_limit_vec_[i]));
        t_max_size_proj_in_spd_best1_agg2_ = HFSAT::MathUtils::GetFlooredMultipleOf(
            t_max_ask_size_short_leg_best_ * dv01_vec_[i] / dv01_vec_[0], p_smv_vec_[i]->min_order_size());
      }

      t_size_to_offer_agg1_best2_ = std::min(t_size_to_offer_agg1_best2_, t_max_size_proj_in_spd_agg1_best2_);
      t_size_to_offer_best1_agg2_ = std::min(t_size_to_offer_best1_agg2_, t_max_size_proj_in_spd_best1_agg2_);
    }

    _spread_asksize_to_offer_ =
        std::min(_spread_asksize_to_offer_, std::max(t_size_to_offer_agg1_best2_, t_size_to_offer_best1_agg2_));

    if (_spread_asksize_to_offer_ > 0 && last_askoffer_was_at_ask_minus_one_ &&
        abs(last_retail_offer_.offered_ask_size_ - _spread_asksize_to_offer_) <
            common_param_set_.retail_size_tolerance_ * last_retail_offer_.offered_ask_size_) {
      _spread_asksize_to_offer_ = last_retail_offer_.offered_ask_size_;
    }

    if (_spread_asksize_to_offer_ > 0) {
      _spread_offer_ask_price_ = _spread_bestask_price_ - min_min_price_increment_to_use_ / 2.0;
      if (_spread_bestask_price_ - min_min_price_increment_to_use_ - _target_price_ >
          common_param_set_.zeropos_place_) {
        _spread_offer_ask_price_ = _spread_bestask_price_ - min_min_price_increment_to_use_;
      }
      // DBGLOG_TIME_CLASS_FUNC << " Ask Agg: " << _spread_offer_ask_price_ <<
      //  " Size: " << _spread_asksize_to_offer_ << DBGLOG_ENDL_NOFLUSH;
    }
  }

  if (_spread_asksize_to_offer_ <= 0 &&
      (_spread_bestask_price_ - _target_price_ > common_param_set_.zeropos_place_ ||
       (last_askoffer_was_at_ask_minus_one_ &&
        _spread_bestask_price_ - _target_price_ >
            (1 - common_param_set_.retail_price_threshold_tolerance_) * common_param_set_.zeropos_place_))) {
    _spread_asksize_to_offer_ = 9999999;
    for (auto i = 0u; i < p_smv_vec_.size(); i++) {
      int t_max_size_proj_in_spd_ = 0;
      if (price_factor_vec_[i] <= 0) {
        int t_max_bid_size_long_leg_ =
            std::min(std::max(0, int(p_smv_vec_[i]->bestbid_size() * param_set_vec_[i].retail_size_factor_to_offer_) -
                                     my_position_vec_[i]),
                     std::min(bidsize_to_show_maxpos_limit_vec_[i], bidsize_to_show_global_maxpos_limit_vec_[i]));
        t_max_size_proj_in_spd_ = HFSAT::MathUtils::GetFlooredMultipleOf(
            t_max_bid_size_long_leg_ * dv01_vec_[i] / dv01_vec_[0], p_smv_vec_[i]->min_order_size());
      } else {
        int t_max_ask_size_short_leg_ =
            std::min(std::max(0, int(p_smv_vec_[i]->bestask_size() * param_set_vec_[i].retail_size_factor_to_offer_) +
                                     my_position_vec_[i]),
                     std::min(asksize_to_show_maxpos_limit_vec_[i], asksize_to_show_global_maxpos_limit_vec_[i]));
        t_max_size_proj_in_spd_ = HFSAT::MathUtils::GetFlooredMultipleOf(
            t_max_ask_size_short_leg_ * dv01_vec_[i] / dv01_vec_[0], p_smv_vec_[i]->min_order_size());
      }

      _spread_asksize_to_offer_ = std::min(_spread_asksize_to_offer_, t_max_size_proj_in_spd_);
      if (_spread_asksize_to_offer_ <= 0) {
        break;
      }
    }

    if (_spread_asksize_to_offer_ > 0 && last_askoffer_was_at_ask_ &&
        abs(last_retail_offer_.offered_ask_size_ - _spread_asksize_to_offer_) <
            common_param_set_.retail_size_tolerance_ * last_retail_offer_.offered_ask_size_) {
      _spread_asksize_to_offer_ = last_retail_offer_.offered_ask_size_;
    }

    if (_spread_asksize_to_offer_ > 0) {
      _spread_offer_ask_price_ = _spread_bestask_price_;
    }
  }
}

void RetailFlyTradingManager::ComputeFlyOffer(HFSAT::CDef::RetailOffer& _this_retail_offer_) {
  // computing prices of Fly
  double spread_bestbid_price_ = 0;
  double spread_bestask_price_ = 0;
  double belly_mkt_price_ = p_smv_vec_[belly_index_]->mkt_size_weighted_price();
  double belly_bestbid_price_ = p_smv_vec_[belly_index_]->bestbid_price();
  double belly_bestask_price_ = p_smv_vec_[belly_index_]->bestask_price();

  double target_price_ = 0;
  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    spread_bestbid_price_ += price_factor_vec_[i] * (price_factor_vec_[i] > 0 ? p_smv_vec_[i]->bestbid_price()
                                                                              : p_smv_vec_[i]->bestask_price());
    spread_bestask_price_ += price_factor_vec_[i] * (price_factor_vec_[i] > 0 ? p_smv_vec_[i]->bestask_price()
                                                                              : p_smv_vec_[i]->bestbid_price());
    target_price_ += price_factor_vec_[i] * p_smv_vec_[i]->mkt_size_weighted_price();
  }

  // checking price cutt-offs for offering bid
  bool last_bidoffer_was_at_bid_ = MathUtils::DblPxCompare(last_retail_offered_bidprice_, spread_bestbid_price_,
                                                           min_min_price_increment_to_use_ / 2.0);
  bool last_bidoffer_was_at_bid_improve_ =
      MathUtils::DblPxCompare(last_retail_offered_bidprice_, spread_bestbid_price_ + min_min_price_increment_to_use_,
                              min_min_price_increment_to_use_ / 2.0);

  // deciding bidsize to offer if all cut-offs cleared
  int t_spread_bidsize_to_offer_ = 0;
  double spread_offer_bidprice_ = 0.0;
  bool bid_improve_ = false;
  if ((target_price_ - spread_bestbid_price_ > common_param_set_.zeropos_place_ ||
       (last_bidoffer_was_at_bid_ &&
        target_price_ - spread_bestbid_price_ >
            (1 - common_param_set_.retail_price_threshold_tolerance_) * common_param_set_.zeropos_place_)) ||
      (target_price_ - spread_bestbid_price_ - belly_min_price_increment_ > common_param_set_.zeropos_place_ ||
       (last_bidoffer_was_at_bid_improve_ &&
        target_price_ - spread_bestbid_price_ - belly_min_price_increment_ >
            (1 - common_param_set_.retail_price_threshold_tolerance_) * common_param_set_.zeropos_place_))) {
    t_spread_bidsize_to_offer_ = 9999999;
    spread_offer_bidprice_ = spread_bestbid_price_;
    for (auto i = 0u; i < p_smv_vec_.size(); i++) {
      int t_max_size_proj_in_spd_ = 0;
      if (price_factor_vec_[i] > 0) {
        int t_max_bid_size_long_leg_ =
            std::min(std::max(0, int(p_smv_vec_[i]->bestbid_size() * param_set_vec_[i].retail_size_factor_to_offer_) -
                                     my_position_vec_[i]),
                     std::min(bidsize_to_show_maxpos_limit_vec_[i], bidsize_to_show_global_maxpos_limit_vec_[i]));
        t_max_size_proj_in_spd_ = HFSAT::MathUtils::GetFlooredMultipleOf(
            t_max_bid_size_long_leg_ * dv01_vec_[i] / dv01_vec_[0], p_smv_vec_[i]->min_order_size());
      } else {
        int t_max_ask_size_short_leg_ =
            std::min(std::max(0, int(p_smv_vec_[i]->bestask_size() * param_set_vec_[i].retail_size_factor_to_offer_) +
                                     my_position_vec_[i]),
                     std::min(asksize_to_show_maxpos_limit_vec_[i], asksize_to_show_global_maxpos_limit_vec_[i]));
        t_max_size_proj_in_spd_ = HFSAT::MathUtils::GetFlooredMultipleOf(
            t_max_ask_size_short_leg_ * dv01_vec_[i] / dv01_vec_[0], p_smv_vec_[i]->min_order_size());
      }

      t_spread_bidsize_to_offer_ = std::min(t_spread_bidsize_to_offer_, t_max_size_proj_in_spd_);
      if (t_spread_bidsize_to_offer_ <= 0) {
        break;
      }
    }

    if (target_price_ - spread_bestbid_price_ - min_min_price_increment_to_use_ > common_param_set_.zeropos_place_ ||
        (last_bidoffer_was_at_bid_improve_ &&
         target_price_ - spread_bestbid_price_ - belly_min_price_increment_ >
             (1 - common_param_set_.retail_price_threshold_tolerance_) * common_param_set_.zeropos_place_)) {
      if (belly_bestask_price_ - belly_mkt_price_ - (p_smv_vec_[belly_index_]->spread_increments() - 1) >
          0.65 * belly_min_price_increment_) {
        spread_offer_bidprice_ = spread_bestbid_price_ + belly_min_price_increment_;
        bid_improve_ = true;
      }
    }

    if (t_spread_bidsize_to_offer_ > 0 &&
        ((last_bidoffer_was_at_bid_improve_ && bid_improve_) || last_bidoffer_was_at_bid_) &&
        abs(last_retail_offer_.offered_bid_size_ - t_spread_bidsize_to_offer_) <
            common_param_set_.retail_size_tolerance_ * last_retail_offer_.offered_bid_size_) {
      t_spread_bidsize_to_offer_ = last_retail_offer_.offered_bid_size_;
    }
  }

  // checking price cutt-offs for offering ask
  bool last_askoffer_was_at_ask_ = MathUtils::DblPxCompare(last_retail_offered_askprice_, spread_bestask_price_,
                                                           min_min_price_increment_to_use_ / 2.0);
  bool last_askoffer_was_at_ask_improve_ =
      MathUtils::DblPxCompare(last_retail_offered_askprice_, spread_bestask_price_ - min_min_price_increment_to_use_,
                              min_min_price_increment_to_use_ / 2.0);

  // deciding asksize to offer if all cut-offs cleared
  int t_spread_asksize_to_offer_ = 0;
  double spread_offer_askprice_ = 0.0;
  bool ask_improve_ = false;

  if ((spread_bestask_price_ - target_price_ > common_param_set_.zeropos_place_ ||
       (last_askoffer_was_at_ask_ &&
        spread_bestask_price_ - target_price_ >
            (1 - common_param_set_.retail_price_threshold_tolerance_) * common_param_set_.zeropos_place_)) ||
      (spread_bestask_price_ - belly_min_price_increment_ - target_price_ > common_param_set_.zeropos_place_ ||
       (last_askoffer_was_at_ask_improve_ &&
        spread_bestask_price_ - belly_min_price_increment_ - target_price_ >
            (1 - common_param_set_.retail_price_threshold_tolerance_) * common_param_set_.zeropos_place_))) {
    spread_offer_askprice_ = spread_bestask_price_;
    t_spread_asksize_to_offer_ = 9999999;
    for (auto i = 0u; i < p_smv_vec_.size(); i++) {
      int t_max_size_proj_in_spd_ = 0;
      if (price_factor_vec_[i] <= 0) {
        int t_max_bid_size_long_leg_ =
            std::min(std::max(0, int(p_smv_vec_[i]->bestbid_size() * param_set_vec_[i].retail_size_factor_to_offer_) -
                                     my_position_vec_[i]),
                     std::min(bidsize_to_show_maxpos_limit_vec_[i], bidsize_to_show_global_maxpos_limit_vec_[i]));
        t_max_size_proj_in_spd_ = HFSAT::MathUtils::GetFlooredMultipleOf(
            t_max_bid_size_long_leg_ * dv01_vec_[i] / dv01_vec_[0], p_smv_vec_[i]->min_order_size());
      } else {
        int t_max_ask_size_short_leg_ =
            std::min(std::max(0, int(p_smv_vec_[i]->bestask_size() * param_set_vec_[i].retail_size_factor_to_offer_) +
                                     my_position_vec_[i]),
                     std::min(asksize_to_show_maxpos_limit_vec_[i], asksize_to_show_global_maxpos_limit_vec_[i]));
        t_max_size_proj_in_spd_ = HFSAT::MathUtils::GetFlooredMultipleOf(
            t_max_ask_size_short_leg_ * dv01_vec_[i] / dv01_vec_[0], p_smv_vec_[i]->min_order_size());
      }

      t_spread_asksize_to_offer_ = std::min(t_spread_asksize_to_offer_, t_max_size_proj_in_spd_);
      if (t_spread_asksize_to_offer_ <= 0) {
        break;
      }
    }

    if (t_spread_asksize_to_offer_ > 0 && last_askoffer_was_at_ask_ &&
        abs(last_retail_offer_.offered_ask_size_ - t_spread_asksize_to_offer_) <
            common_param_set_.retail_size_tolerance_ * last_retail_offer_.offered_ask_size_) {
      t_spread_asksize_to_offer_ = last_retail_offer_.offered_ask_size_;
    }

    if ((spread_bestask_price_ - belly_min_price_increment_ - target_price_ > common_param_set_.zeropos_place_ ||
         (last_askoffer_was_at_ask_improve_ &&
          spread_bestask_price_ - belly_min_price_increment_ - target_price_ >
              (1 - common_param_set_.retail_price_threshold_tolerance_) * common_param_set_.zeropos_place_))) {
      if (belly_mkt_price_ - belly_bestbid_price_ - (p_smv_vec_[belly_index_]->spread_increments() - 1) >
          0.65 * belly_min_price_increment_) {
        spread_offer_askprice_ = spread_bestask_price_ - belly_min_price_increment_;
        ask_improve_ = true;
      }
    }

    if (t_spread_asksize_to_offer_ > 0 &&
        ((last_askoffer_was_at_ask_improve_ && ask_improve_) || last_askoffer_was_at_ask_) &&
        abs(last_retail_offer_.offered_ask_size_ - t_spread_asksize_to_offer_) <
            common_param_set_.retail_size_tolerance_ * last_retail_offer_.offered_ask_size_) {
      t_spread_asksize_to_offer_ = last_retail_offer_.offered_ask_size_;
    }
  }

  // setting px,size,type of retailoffer struct
  if (t_spread_bidsize_to_offer_ <= 0 && t_spread_asksize_to_offer_ <= 0) {
    _this_retail_offer_.retail_update_type_ = HFSAT::CDef::kSubBestBidAsk;
    spread_offer_bidprice_ = spread_bestbid_price_;
    spread_offer_askprice_ = spread_bestask_price_;
  } else if (t_spread_bidsize_to_offer_ <= 0) {
    _this_retail_offer_.retail_update_type_ = HFSAT::CDef::kSubBestBid;
    spread_offer_bidprice_ = spread_bestbid_price_;
  } else if (t_spread_asksize_to_offer_ <= 0) {
    _this_retail_offer_.retail_update_type_ = HFSAT::CDef::kSubBestAsk;
    spread_offer_askprice_ = spread_bestask_price_;
  }

  if (common_param_set_.retail_offer_fra_) {
    ComputePU();
    ComputeSlideFRA(spread_offer_bidprice_, spread_offer_askprice_);
  }
  UpdateSizeRatio();

  last_retail_offered_bidprice_ = spread_offer_bidprice_;
  if (t_spread_bidsize_to_offer_ > 0) {
    _this_retail_offer_.offered_bid_size_ = t_spread_bidsize_to_offer_;
    _this_retail_offer_.offered_bid_price_ = spread_offer_bidprice_;
    if (common_param_set_.retail_offer_fra_) {
      //_this_retail_offer_.offered_bid_price_ = fra_sp_vec_[0].bestbid_price_ - fra_sp_vec_[1].bestask_price_;
      _this_retail_offer_.offered_bid_price_ = GetBidFRA();
    }
  } else {
    // Offering min size at best level
    _this_retail_offer_.offered_bid_size_ = p_smv_vec_[0]->min_order_size();
    _this_retail_offer_.offered_bid_price_ = spread_offer_bidprice_;
    if (common_param_set_.retail_offer_fra_) {
      _this_retail_offer_.offered_bid_price_ = GetBidFRA();
    }
  }

  last_retail_offered_askprice_ = spread_offer_askprice_;
  if (t_spread_asksize_to_offer_ > 0) {
    _this_retail_offer_.offered_ask_size_ = t_spread_asksize_to_offer_;
    _this_retail_offer_.offered_ask_price_ = spread_offer_askprice_;
    if (common_param_set_.retail_offer_fra_) {
      //_this_retail_offer_.offered_ask_price_ = fra_sp_vec_[0].bestask_price_ - fra_sp_vec_[1].bestbid_price_;
      _this_retail_offer_.offered_ask_price_ = GetAskFRA();
    }
  } else {
    // Offering min size at best level
    _this_retail_offer_.offered_ask_size_ = p_smv_vec_[0]->min_order_size();
    _this_retail_offer_.offered_ask_price_ = spread_offer_askprice_;
    if (common_param_set_.retail_offer_fra_) {
      //_this_retail_offer_.offered_ask_price_ = fra_sp_vec_[0].bestask_price_ - fra_sp_vec_[1].bestbid_price_;
      _this_retail_offer_.offered_ask_price_ = GetAskFRA();
    }
  }

  FillLegInfo(_this_retail_offer_);

  if (last_retail_offer_bid_vec_.size() >= 5) {
    last_retail_offer_bid_price_vec_.erase(last_retail_offer_bid_price_vec_.begin());
    last_retail_offer_bid_vec_.erase(last_retail_offer_bid_vec_.begin());
    bid_size_ratio_vec_vec_.erase(bid_size_ratio_vec_vec_.begin());
  }

  int t_bid_offer_index_ = GetIndex(last_retail_offer_bid_price_vec_, _this_retail_offer_.offered_bid_price_);
  if (t_bid_offer_index_ == -1) {
    last_retail_offer_bid_price_vec_.push_back(_this_retail_offer_.offered_bid_price_);
    last_retail_offer_bid_vec_.push_back(_this_retail_offer_);
    bid_size_ratio_vec_vec_.push_back(size_ratio_vec_);
  } else {
    last_retail_offer_bid_price_vec_[t_bid_offer_index_] = _this_retail_offer_.offered_bid_price_;
    last_retail_offer_bid_vec_[t_bid_offer_index_] = _this_retail_offer_;
    bid_size_ratio_vec_vec_[t_bid_offer_index_] = size_ratio_vec_;
  }
  
  if (last_retail_offer_ask_vec_.size() >= 5) {
    last_retail_offer_ask_price_vec_.erase(last_retail_offer_ask_price_vec_.begin());
    last_retail_offer_ask_vec_.erase(last_retail_offer_ask_vec_.begin());
    ask_size_ratio_vec_vec_.erase(ask_size_ratio_vec_vec_.begin());
  }

  int t_ask_offer_index_ = GetIndex(last_retail_offer_ask_price_vec_, _this_retail_offer_.offered_ask_price_);
  if (t_ask_offer_index_ == -1) {
    last_retail_offer_ask_price_vec_.push_back(_this_retail_offer_.offered_ask_price_);
    last_retail_offer_ask_vec_.push_back(_this_retail_offer_);
    ask_size_ratio_vec_vec_.push_back(size_ratio_vec_);
  } else {
    last_retail_offer_ask_price_vec_[t_ask_offer_index_] = _this_retail_offer_.offered_ask_price_;
    last_retail_offer_ask_vec_[t_ask_offer_index_] = _this_retail_offer_;
    ask_size_ratio_vec_vec_[t_ask_offer_index_] = size_ratio_vec_;
  }

  /*
   DBGLOG_TIME_CLASS_FUNC << "Retail Offer Bid FRA: " << _this_retail_offer_.offered_bid_price_ << " "
                          << " Price: " << t_offered_bidprice_ << " "
                          << "Retail Offer Ask FRA: " << _this_retail_offer_.offered_ask_price_ << " "
                          << "Price: " << t_offered_askprice_ << DBGLOG_ENDL_NOFLUSH;*/
}

void RetailFlyTradingManager::NotifyListeners(const HFSAT::CDef::RetailOffer& _retail_offer_) {
  if (((last_retail_update_msecs_ <= 0) ||
       (watch_.msecs_from_midnight() - last_retail_update_msecs_ >= RETAIL_UPDATE_TIMEOUT_MSECS)) ||
      (last_retail_offer_ != _retail_offer_)  // offer has changed
      ) {
    for (auto i = 0u; i < this->retail_offer_listeners_.size(); i++) {
      retail_offer_listeners_[i]->OnRetailOfferUpdate(fly_secname_string_, _retail_offer_);
    }
    last_retail_offer_ = _retail_offer_;
    last_retail_update_msecs_ = watch_.msecs_from_midnight();

    if (dbglogger_.CheckLoggingLevel(RETAIL_INFO)) {
      LogFullStatus();
    }
  }
}

void RetailFlyTradingManager::AdjustMaxPosLimitOfferSizes() {
  // make this a step function of my_position_, to reduce fluctuation and giving time to retail query to reduce position
  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    bidsize_to_show_maxpos_limit_vec_[i] =
        param_set_vec_[i].max_position_ -
        int(ceil(my_position_vec_[i] / double(param_set_vec_[i].retail_max_position_step_size_))) *
            param_set_vec_[i].retail_max_position_step_size_;
    asksize_to_show_maxpos_limit_vec_[i] =
        param_set_vec_[i].max_position_ -
        int(ceil(-my_position_vec_[i] / double(param_set_vec_[i].retail_max_position_step_size_))) *
            param_set_vec_[i].retail_max_position_step_size_;
  }
}

void RetailFlyTradingManager::PrintFullStatus() {
  DBGLOG_TIME_CLASS_FUNC << fly_secname_string_ << " Retail Sending: " << last_retail_offer_.ToString()
                         << DBGLOG_ENDL_NOFLUSH;

  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    DBGLOG_TIME_CLASS_FUNC << "mkt: " << p_smv_vec_[i]->shortcode() << " [ " << p_smv_vec_[i]->bestbid_size() << " "
                           << p_smv_vec_[i]->bestbid_price() << " * " << p_smv_vec_[i]->bestask_price() << " "
                           << p_smv_vec_[i]->bestask_size() << " ]" << DBGLOG_ENDL_NOFLUSH;
  }

  DBGLOG_TIME_CLASS_FUNC << "[ BidSize:" << last_retail_offer_.offered_bid_size_
                         << " BidPrice: " << last_retail_offer_.offered_bid_price_
                         << " AskPrice: " << last_retail_offer_.offered_ask_price_
                         << " AskSize: " << last_retail_offer_.offered_ask_size_ << " ]" << DBGLOG_ENDL_NOFLUSH;

  /*if ( p_smv_vec_.size() >= 3 ) {
    DBGLOG_TIME_CLASS_FUNC << "FRA12_bid: " << fra_sp_vec_[0].bid_ << " "
        << "FRA12_ask: " << fra_sp_vec_[0].ask_ << " "
        << "FRA23_bid: " << fra_sp_vec_[1].bid_ << " "
        << "FRA23_ask: " << fra_sp_vec_[1].ask_ << DBGLOG_ENDL_NOFLUSH;
  }*/
}

void RetailFlyTradingManager::LogFullStatus() {
  if ((last_full_logging_msecs_ == 0) || (watch_.msecs_from_midnight() - last_full_logging_msecs_ > 5000)) {
    last_full_logging_msecs_ = watch_.msecs_from_midnight();
    PrintFullStatus();
  }
}

void RetailFlyTradingManager::OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) {
  if (secid_idx_map_.find(_security_id_) == secid_idx_map_.end()) {
    DBGLOG_TIME_CLASS_FUNC << "Unknown secid " << _security_id_ << "\n";
    return;
  }

  int t_vec_idx_ = secid_idx_map_[_security_id_];

  my_global_position_vec_[t_vec_idx_] = _new_global_position_;
  AdjustGlobalMaxPosLimitOfferSizes();
  // not calling setoffervars here to save some calls, it would get called on next market update
}

void RetailFlyTradingManager::AdjustGlobalMaxPosLimitOfferSizes() {
  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    // make this a step function of my_position_, to reduce fluctuation and giving time to retail query to reduce
    // position
    bidsize_to_show_global_maxpos_limit_vec_[i] =
        param_set_vec_[i].max_global_position_ -
        int(ceil(my_global_position_vec_[i] / double(param_set_vec_[i].retail_max_global_position_step_size_))) *
            param_set_vec_[i].retail_max_global_position_step_size_;
    asksize_to_show_global_maxpos_limit_vec_[i] =
        param_set_vec_[i].max_global_position_ -
        int(ceil(-my_global_position_vec_[i] / double(param_set_vec_[i].retail_max_global_position_step_size_))) *
            param_set_vec_[i].retail_max_global_position_step_size_;
  }
}

void RetailFlyTradingManager::BreakFlyTrade(const char* _secname_, double _fly_trd_px_, unsigned int _fly_trd_qty_,
                                            TradeType_t _fly_buysell_, std::vector<TradeInfoStruct>& retval_) {
  std::vector<double> t_exec_leg_price_vec_;
  std::vector<double> t_exec_size_ratio_vec_;

  // break trade
  if (_fly_buysell_ == kTradeTypeBuy) {
    last_trade_was_buy_ = true;
    last_trade_was_sell_ = false;
    int t_index_ = GetIndex(last_retail_offer_bid_price_vec_, _fly_trd_px_);    
    if (t_index_ > -1) {
      t_exec_leg_price_vec_ = last_retail_offer_bid_vec_[t_index_].get_bid_leg_prices();
      t_exec_size_ratio_vec_ = bid_size_ratio_vec_vec_[t_index_];
    } else {
      /// TODO FRA
      t_exec_leg_price_vec_ = last_retail_offer_bid_vec_.back().get_bid_leg_prices();
      t_exec_size_ratio_vec_ = bid_size_ratio_vec_vec_.back();
      DBGLOG_TIME_CLASS_FUNC << "Couldn't find the fra to price map "
                             << " for " << _secname_ << " at price " << _fly_trd_px_ << " for Buy order"
                             << DBGLOG_ENDL_FLUSH;
    }
  } else if (_fly_buysell_ == kTradeTypeSell) {
    last_trade_was_buy_ = false;
    last_trade_was_sell_ = true;
    int t_index_ = GetIndex(last_retail_offer_ask_price_vec_, _fly_trd_px_);
    if (t_index_ > -1) {
      t_exec_leg_price_vec_ = last_retail_offer_ask_vec_[t_index_].get_ask_leg_prices();
      t_exec_size_ratio_vec_ = ask_size_ratio_vec_vec_[t_index_];
    } else {
      // TODO FRA
      t_exec_leg_price_vec_ = last_retail_offer_ask_vec_.back().get_ask_leg_prices();
      t_exec_size_ratio_vec_ = ask_size_ratio_vec_vec_.back();
      DBGLOG_TIME_CLASS_FUNC << "Couldn't find the fra to price map "
                             << " for " << _secname_ << " at price " << _fly_trd_px_ << " for Sell order"
                             << DBGLOG_ENDL_FLUSH;
    }
  }

  retval_.clear();
  double t_last_traded_px_pts_ = 0.0;

  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    retval_.emplace_back(TradeInfoStruct());

    retval_[i].trd_qty_ =
        HFSAT::MathUtils::RoundOff(t_exec_size_ratio_vec_[i] * _fly_trd_qty_, p_smv_vec_[i]->min_order_size());

    retval_[i].buysell_ = price_factor_vec_[i] > 0.0 ? _fly_buysell_ : TradeType_t(1 - _fly_buysell_);

    // assuming best price exec, logging when this assumption fails
    retval_[i].trd_px_ = t_exec_leg_price_vec_[i];
    t_last_traded_px_pts_ += price_factor_vec_[i] * retval_[i].trd_px_;
  }

  if (_fly_buysell_ == kTradeTypeBuy) {
    last_buy_price_ = t_last_traded_px_pts_;
  } else if (_fly_buysell_ == kTradeTypeSell) {
    last_sell_price_ = t_last_traded_px_pts_;
  }
}

void RetailFlyTradingManager::SetPriceFactVec() {
  if (p_smv_vec_.size() == 2) {
    price_factor_vec_ = {1, -1};
    belly_index_ = 0;
  } else if (p_smv_vec_.size() == 3) {
    price_factor_vec_ = {1, -2, 1};
    belly_index_ = 1;
  } else if (p_smv_vec_.size() == 4) {
    price_factor_vec_ = {1, -3, 3, -1};
    belly_index_ = 2;
  }
}

void RetailFlyTradingManager::ComputeMovingAverage() {
  if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
    moving_avg_spread_ += inv_decay_sum_ * (current_spread_mkt_price_ - last_price_recorded_);
  } else {
    int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
    if (num_pages_to_add_ >= (int)decay_vector_.size()) {
      moving_avg_spread_ = current_spread_mkt_price_;
      last_price_recorded_ = current_spread_mkt_price_;
      last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
    } else {
      if (num_pages_to_add_ == 1) {
        moving_avg_spread_ = (current_spread_mkt_price_ * inv_decay_sum_) + (moving_avg_spread_ * decay_vector_[1]);
      } else {
        moving_avg_spread_ = (current_spread_mkt_price_ * inv_decay_sum_) +
                             (last_price_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                             (moving_avg_spread_ * decay_vector_[num_pages_to_add_]);
      }
      last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
    }
  }
  last_price_recorded_ = current_spread_mkt_price_;
}

void RetailFlyTradingManager::SetTimeDecayWeights() {
  int trend_history_msecs_ = (int)round(1000 * 900);
  const unsigned int kDecayLength =
      20;  ///< here number of samples are not required to be very high and hence the decaylength target is just 20
  const unsigned int kMinPageWidth = 10;
  const unsigned int kMaxPageWidth =
      200;  ///< keeping kMaxPageWidth low makes the number_fadeoffs_ pretty high and hence keeps lots of sample points
  page_width_msecs_ = std::min(kMaxPageWidth, std::max(kMinPageWidth, (trend_history_msecs_ / kDecayLength)));

  int number_fadeoffs_ = std::max(1, (int)ceil(trend_history_msecs_ / page_width_msecs_));

  decay_page_factor_ = MathUtils::CalcDecayFactor(number_fadeoffs_);

  decay_vector_.resize(2 * number_fadeoffs_);
  decay_vector_sums_.resize(2 * number_fadeoffs_);

  for (auto i = 0u; i < decay_vector_.size(); i++) {
    decay_vector_[i] = pow(decay_page_factor_, (int)i);
  }

  decay_vector_sums_[0] = 0;
  for (unsigned int i = 1; i < decay_vector_sums_.size(); i++) {
    decay_vector_sums_[i] = decay_vector_sums_[i - 1] + decay_vector_[i];
  }

  inv_decay_sum_ = (1 - decay_page_factor_);
}

void RetailFlyTradingManager::ComputeSlideFRA(double _spread_offer_bid_price_, double _spread_offer_ask_price_) {
  if (p_smv_vec_.size() == 3) {
    offered_bid_price_vec_[0] = p_smv_vec_[0]->bestbid_price();
    offered_bid_price_vec_[2] = p_smv_vec_[2]->bestbid_price();
    offered_bid_price_vec_[1] =
        (_spread_offer_bid_price_ - offered_bid_price_vec_[0] - offered_bid_price_vec_[2]) / price_factor_vec_[1];

    offered_ask_price_vec_[0] = p_smv_vec_[0]->bestask_price();
    offered_ask_price_vec_[2] = p_smv_vec_[2]->bestask_price();
    offered_ask_price_vec_[1] =
        (_spread_offer_ask_price_ - offered_ask_price_vec_[0] - offered_ask_price_vec_[2]) / price_factor_vec_[1];

    for (auto i = 0u; i < p_smv_vec_.size(); i++) {
      inv_pu_bid_vec_[i] = pow(1 + offered_bid_price_vec_[i] / 100, reserve_days_vec_[i] / 252.0);
      inv_pu_ask_vec_[i] = pow(1 + offered_ask_price_vec_[i] / 100, reserve_days_vec_[i] / 252.0);
      inv_pu_mid_vec_[i] = pow(1 + p_smv_vec_[i]->mid_price() / 100, reserve_days_vec_[i] / 252.0);
    }

    ComputeFRAFromInvPU(inv_pu_bid_vec_, 0);
    ComputeFRAFromInvPU(inv_pu_ask_vec_, 1);
    ComputeFRAFromInvPU(inv_pu_mid_vec_, 2);

    /*
    for (auto i = 0u; i < p_smv_vec_.size() - 1; i++) {
      fra_sp_vec_[i].offered_bid_fra_ = 100 * (pow(inv_pu_bid_vec_[i] / inv_pu_bid_vec_[i + 1],
                                                   252.0 / (reserve_days_vec_[i] - reserve_days_vec_[i + 1])) -
                                               1);
      fra_sp_vec_[i].offered_ask_fra_ = 100 * (pow(inv_pu_ask_vec_[i] / inv_pu_ask_vec_[i + 1],
                                                   252.0 / (reserve_days_vec_[i] - reserve_days_vec_[i + 1])) -
                                               1);
      fra_sp_vec_[i].current_mid_fra_ = 100 * (pow(inv_pu_mid_vec_[i] / inv_pu_mid_vec_[i + 1],
                                                   252.0 / (reserve_days_vec_[i] - reserve_days_vec_[i + 1])) -
                                               1);
    }*/

    double t_retail_offer_bidprice_ = fra_sp_vec_[0].offered_bid_fra_ - fra_sp_vec_[1].offered_bid_fra_;
    retail_offer_bid_fra_ = ceil(t_retail_offer_bidprice_ * 200.0) / 200.0;
    if (!MathUtils::DblPxCompare(t_retail_offer_bidprice_, retail_offer_bid_fra_, 0.001)) {
      CalibrateBidFRA();
    }

    double t_retail_offer_askprice_ = fra_sp_vec_[0].offered_ask_fra_ - fra_sp_vec_[1].offered_ask_fra_;
    retail_offer_ask_fra_ = floor(t_retail_offer_askprice_ * 200.0) / 200.0;
    if (!MathUtils::DblPxCompare(t_retail_offer_askprice_, retail_offer_ask_fra_, 0.001)) {
      CalibrateAskFRA();
    }
  }
}

void RetailFlyTradingManager::ComputeFRAFromInvPU(std::vector<double>& _inv_pu_vec_, unsigned int _index_) {
  std::vector<double> t_offered_fra_vec_(p_smv_vec_.size() - 1, 0);
  for (auto i = 0u; i < p_smv_vec_.size() - 1; i++) {
    t_offered_fra_vec_[i] =
        100 *
        (pow(_inv_pu_vec_[i] / _inv_pu_vec_[i + 1], 252.0 / (reserve_days_vec_[i] - reserve_days_vec_[i + 1])) - 1);
  }
  if (_index_ == 0) {
    for (auto i = 0u; i < t_offered_fra_vec_.size(); i++) {
      fra_sp_vec_[i].offered_bid_fra_ = t_offered_fra_vec_[i];
    }
  } else if (_index_ == 1) {
    for (auto i = 0u; i < t_offered_fra_vec_.size(); i++) {
      fra_sp_vec_[i].offered_ask_fra_ = t_offered_fra_vec_[i];
    }
  } else if (_index_ == 2) {
    for (auto i = 0u; i < t_offered_fra_vec_.size(); i++) {
      fra_sp_vec_[i].current_mid_fra_ = t_offered_fra_vec_[i];
    }
  }
}

void RetailFlyTradingManager::CalibrateBidFRA() {
  if (p_smv_vec_.size() == 3) {
    double t_price_steps_ = p_smv_vec_[belly_index_]->min_price_increment() / 50.0;
    double t_offered_belly_ask_price_ = offered_bid_price_vec_[belly_index_] - t_price_steps_;
    double t_retail_offer_bid_fra_ = retail_offer_bid_fra_;
    double t_last_bid_fra_recorded_ = fra_sp_vec_[0].offered_bid_fra_ - fra_sp_vec_[1].offered_bid_fra_;
    double t_last_belly_ask_price_recorded_ = offered_bid_price_vec_[belly_index_];

    while (t_offered_belly_ask_price_ >= p_smv_vec_[belly_index_]->bestbid_price()) {
      inv_pu_bid_vec_[1] = pow(1 + t_offered_belly_ask_price_ / 100, reserve_days_vec_[belly_index_] / 252.0);

      ComputeFRAFromInvPU(inv_pu_bid_vec_, 0);
      t_retail_offer_bid_fra_ = fra_sp_vec_[0].offered_bid_fra_ - fra_sp_vec_[1].offered_bid_fra_;

      offered_bid_price_vec_[belly_index_] = t_offered_belly_ask_price_;

      if (MathUtils::DblPxCompare(t_retail_offer_bid_fra_, retail_offer_bid_fra_, 0.001)) {
        break;
      }

      if (t_retail_offer_bid_fra_ - retail_offer_bid_fra_ > 0) {
        if (std::fabs(t_retail_offer_bid_fra_ - retail_offer_bid_fra_) >
            std::fabs(retail_offer_bid_fra_ - t_last_bid_fra_recorded_)) {
          offered_bid_price_vec_[belly_index_] = t_last_belly_ask_price_recorded_;
        }
        break;
      }

      // Recording previous values
      t_last_belly_ask_price_recorded_ = t_offered_belly_ask_price_;
      t_last_bid_fra_recorded_ = t_retail_offer_bid_fra_;
      t_offered_belly_ask_price_ -= t_price_steps_;
    }
  }
}

void RetailFlyTradingManager::CalibrateAskFRA() {
  if (p_smv_vec_.size() == 3) {
    double t_price_steps_ = p_smv_vec_[belly_index_]->min_price_increment() / 50.0;
    double t_offered_belly_bid_price_ = offered_ask_price_vec_[belly_index_] - t_price_steps_;
    double t_retail_offer_ask_fra_ = retail_offer_ask_fra_;
    double t_last_ask_fra_recorded_ = fra_sp_vec_[0].offered_ask_fra_ - fra_sp_vec_[1].offered_ask_fra_;
    double t_last_belly_bid_price_recorded_ = offered_ask_price_vec_[belly_index_];

    while (t_offered_belly_bid_price_ <= p_smv_vec_[belly_index_]->bestask_price()) {
      inv_pu_ask_vec_[1] = pow(1 + t_offered_belly_bid_price_ / 100, reserve_days_vec_[belly_index_] / 252.0);

      ComputeFRAFromInvPU(inv_pu_ask_vec_, 1);
      t_retail_offer_ask_fra_ = fra_sp_vec_[0].offered_ask_fra_ - fra_sp_vec_[1].offered_ask_fra_;

      offered_ask_price_vec_[belly_index_] = t_offered_belly_bid_price_;

      if (MathUtils::DblPxCompare(t_retail_offer_ask_fra_, retail_offer_ask_fra_, 0.001)) {
        break;
      }

      if (t_retail_offer_ask_fra_ - retail_offer_ask_fra_ < 0) {
        if (std::fabs(t_retail_offer_ask_fra_ - retail_offer_ask_fra_) >
            std::fabs(t_last_ask_fra_recorded_ - retail_offer_ask_fra_)) {
          offered_ask_price_vec_[belly_index_] = t_last_belly_bid_price_recorded_;
        }
        break;
      }

      // Recording previous values
      t_last_belly_bid_price_recorded_ = t_offered_belly_bid_price_;
      t_last_ask_fra_recorded_ = t_retail_offer_ask_fra_;
      t_offered_belly_bid_price_ += t_price_steps_;
    }
  }
}

void RetailFlyTradingManager::ComputePU() {
  for (auto i = 0u; i < p_smv_vec_.size(); i++) {
    current_pu_vec_[i] = 100000 / pow(1 + p_smv_vec_[i]->mid_price() / 100, reserve_days_vec_[i] / 252.0);
  }
}

void RetailFlyTradingManager::UpdateSizeRatio() {
  if (common_param_set_.retail_offer_fra_) {
    if (p_smv_vec_.size() == 3) {
      double fra_change_1bps_leg2_leg3_ = pow((fra_sp_vec_[1].current_mid_fra_ + 0.01) / 100 + 1,
                                              (reserve_days_vec_[1] - reserve_days_vec_[2]) / 252.0);
      double fra_change_1bps_leg1_leg2_ = pow((fra_sp_vec_[0].current_mid_fra_ + 0.01) / 100 + 1,
                                              (reserve_days_vec_[0] - reserve_days_vec_[1]) / 252.0);

      double pu_change_leg1_leg2_ = current_pu_vec_[0] - current_pu_vec_[1] / fra_change_1bps_leg1_leg2_;
      double pu_change_leg2_leg3_ = current_pu_vec_[1] - current_pu_vec_[2] / fra_change_1bps_leg2_leg3_;
      double ratio_pu_change_ = pu_change_leg1_leg2_ / pu_change_leg2_leg3_;

      double ratio_pu_leg1_leg2_ = current_pu_vec_[0] / current_pu_vec_[1];
      double ratio_pu_leg2_leg3_ = current_pu_vec_[1] / current_pu_vec_[2];

      size_ratio_vec_[0] = 1.0;
      size_ratio_vec_[1] = ratio_pu_leg1_leg2_ + ratio_pu_change_;
      size_ratio_vec_[2] = ratio_pu_leg2_leg3_ * ratio_pu_change_;
    }
  } else {
    size_ratio_vec_ = size_factor_vec_;
  }
}

void RetailFlyTradingManager::FillLegInfo(HFSAT::CDef::RetailOffer& _this_retail_offer_) {
  if (common_param_set_.retail_offer_fra_) {
    for (auto i = 0u; i < _this_retail_offer_.product_split_.product_type_; i++) {
      _this_retail_offer_.product_split_.sub_product_bid_[i].product_price_ = offered_bid_price_vec_[i];
      _this_retail_offer_.product_split_.sub_product_bid_[i].product_size_ = HFSAT::MathUtils::RoundOff(
          size_ratio_vec_[i] * _this_retail_offer_.offered_bid_size_, p_smv_vec_[i]->min_order_size());
      _this_retail_offer_.product_split_.sub_product_bid_[i].buysell_ =
          price_factor_vec_[i] > 0.0 ? kTradeTypeSell : kTradeTypeBuy;

      _this_retail_offer_.product_split_.sub_product_ask_[i].product_price_ = offered_ask_price_vec_[i];
      _this_retail_offer_.product_split_.sub_product_ask_[i].product_size_ = HFSAT::MathUtils::RoundOff(
          size_ratio_vec_[i] * _this_retail_offer_.offered_ask_size_, p_smv_vec_[i]->min_order_size());
      _this_retail_offer_.product_split_.sub_product_ask_[i].buysell_ =
          price_factor_vec_[i] > 0.0 ? kTradeTypeBuy : kTradeTypeSell;
    }
  } else {
    double exp_fly_bid_price_ = 0.0;
    double exp_fly_ask_price_ = 0.0;
    for (auto i = 0u; i < p_smv_vec_.size(); i++) {
      _this_retail_offer_.product_split_.sub_product_bid_[i].product_price_ =
          price_factor_vec_[i] > 0.0 ? p_smv_vec_[i]->bestbid_price() : p_smv_vec_[i]->bestask_price();
      _this_retail_offer_.product_split_.sub_product_bid_[i].product_size_ = HFSAT::MathUtils::RoundOff(
          size_ratio_vec_[i] * _this_retail_offer_.offered_bid_size_, p_smv_vec_[i]->min_order_size());
      _this_retail_offer_.product_split_.sub_product_bid_[i].buysell_ =
          price_factor_vec_[i] > 0.0 ? kTradeTypeSell : kTradeTypeBuy;
      exp_fly_bid_price_ +=
          price_factor_vec_[i] * _this_retail_offer_.product_split_.sub_product_bid_[i].product_price_;

      _this_retail_offer_.product_split_.sub_product_ask_[i].product_price_ =
          price_factor_vec_[i] > 0.0 ? p_smv_vec_[i]->bestask_price() : p_smv_vec_[i]->bestbid_price();
      _this_retail_offer_.product_split_.sub_product_ask_[i].product_size_ = HFSAT::MathUtils::RoundOff(
          size_ratio_vec_[i] * _this_retail_offer_.offered_ask_size_, p_smv_vec_[i]->min_order_size());
      _this_retail_offer_.product_split_.sub_product_ask_[i].buysell_ =
          price_factor_vec_[i] > 0.0 ? kTradeTypeBuy : kTradeTypeSell;
      exp_fly_ask_price_ +=
          price_factor_vec_[i] * _this_retail_offer_.product_split_.sub_product_ask_[i].product_price_;
    }

    _this_retail_offer_.product_split_.sub_product_bid_[belly_index_].product_price_ +=
        (_this_retail_offer_.offered_bid_price_ - exp_fly_bid_price_) / std::abs(price_factor_vec_[belly_index_]);
    _this_retail_offer_.product_split_.sub_product_ask_[belly_index_].product_price_ +=
        (_this_retail_offer_.offered_ask_price_ - exp_fly_ask_price_) / std::abs(price_factor_vec_[belly_index_]);
  }
}

double RetailFlyTradingManager::GetBidFRA() {
  return retail_offer_bid_fra_;
}

double RetailFlyTradingManager::GetAskFRA() {
  return retail_offer_ask_fra_;
}

int RetailFlyTradingManager::GetIndex(std::vector<double> _price_vec_, double _price_to_compare_) {
  bool t_index_found_ = false;
  int t_index_value_ = -1;
  for (int i = _price_vec_.size() - 1; i >= 0; i--) {
    t_index_found_ = MathUtils::DblPxCompare(_price_to_compare_, _price_vec_[i], 0.001);
    if (t_index_found_) {
      t_index_value_ = i;
      break;
    }
  }
  return t_index_value_;
}
} /* namespace HFSAT */
