/* =====================================================================================

   Filename:  ExecLogicCode/index_futures_mean_reverting_trading.cpp

   Description:

   Version:  1.0
   Created:  Monday 30 May 2016 05:06:09  UTC
   Revision:  none
   Compiler:  g++

   Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011

   Address:  Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   Phone:  +91 80 4190 3551

   =====================================================================================
*/
#include <numeric>
#include "dvccode/Utils/CPUAffinity.hpp"
#include "dvctrade/ExecLogic/index_futures_mean_reverting_trading.hpp"
#include "baseinfra/VolatileTradingInfo/shortcode_ezone_vec.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils_weighted.hpp"

#define INVALID_MEAN_REVERTING_ORDER_PRICE -1
#define MR_LT_DURATION 60
#define MR_ST_DURATION 5
namespace HFSAT {

IndexFuturesMeanRevertingTrading::IndexFuturesMeanRevertingTrading(
    DebugLogger& _dbglogger_, const Watch& _watch_, const std::vector<SecurityMarketView*> _all_market_view_fut_vec_,
    const std::vector<SecurityMarketView*> _dep_market_view_fut_vec_,
    const std::vector<SmartOrderManager*> _order_manager_fut_vec_, const std::string& _global_paramfilename_,
    const bool _livetrading_, MultBasePNL* _mult_base_pnl_, EconomicEventsManager& t_economic_events_manager_,
    int _trading_start_utc_mfm_, int _trading_end_utc_mfm_, unsigned int _runtime_id_)

    // subscribe px types of each product
    // Make Indicators
    // listen to those indicators, e.g. indicator -> add_unweighted_indicator_listener(1u, this);
    // subscribe to watch
    // multbaspnl.listener(this)
    : MultExecInterface(_dbglogger_, _watch_, _dep_market_view_fut_vec_, _order_manager_fut_vec_,
                        _global_paramfilename_, _livetrading_, _trading_start_utc_mfm_, _trading_end_utc_mfm_),
      total_pnl_(0),
      mult_base_pnl_(_mult_base_pnl_),
      is_affined_(false),
      last_affine_attempt_msecs_(0),
      first_affine_attempt_msecs_(0),
      economic_events_manager_(t_economic_events_manager_),
      all_market_view_vec_(_all_market_view_fut_vec_),
      severity_to_getflat_on_(1.0),
      severity_to_getflat_on_base_(1.0),
      severity_change_end_msecs_(_trading_end_utc_mfm_),
      pnl_sampling_timestamps_(),
      pnl_samples_(),
      next_open_pnl_log_timestamp_(0),
      sample_index_(0),
      runtime_id_(_runtime_id_) {
  num_total_sources_ = _all_market_view_fut_vec_.size();
  num_total_products_ = _dep_market_view_fut_vec_.size();
  current_dv01_vec_.resize(num_total_products_);

  if (global_param_set_->read_severity_to_getflat_on_) {
    severity_to_getflat_on_base_ = global_param_set_->severity_to_getflat_on_;
    severity_to_getflat_on_ = severity_to_getflat_on_base_;
  }

  first_dep_smv_ = _dep_market_view_fut_vec_[0];
  economic_events_manager_.AdjustSeverity(first_dep_smv_->shortcode(), first_dep_smv_->exch_source());
  economic_events_manager_.AllowEconomicEventsFromList(first_dep_smv_->shortcode());
  allowed_events_present_ = economic_events_manager_.AllowedEventsPresent();

  {
    // for the first dep shortcode .. depending on time of day calculate the numbers to stop for
    // ezones_all_events_ .. the economic szones such that all events with severity >= 1 we should stop for
    // ezones_strong_events_ ... the economic zones where we should stop in periods wherethe applicable severity of all
    // events of that period >= 2
    // ezones_super_events_ ... only severity >= 3 events
    // after getting these event vectors
    // ask economic times manager to precompute the times when we should stop and when we should not

    GetEZVecForShortcode(first_dep_smv_->shortcode(), _trading_start_utc_mfm_, ezone_vec_);
    DBGLOG_CLASS_FUNC << "After checking at mfm " << _trading_start_utc_mfm_ << " Stopping for EZones:";
    for (auto i = 0u; i < ezone_vec_.size(); i++) {
      dbglogger_ << ' ' << GetStrFromEconomicZone(ezone_vec_[i]);
    }
    dbglogger_ << DBGLOG_ENDL_FLUSH;

    //( t_trading_end_utc_mfm_ - 1 ) to avoid events overlapping period for EU and US
    GetEZVecForShortcode(first_dep_smv_->shortcode(), trading_end_utc_mfm_ - 1, ezone_vec_);
    DBGLOG_CLASS_FUNC << "After checking at mfm " << (trading_end_utc_mfm_ - 1) << " Stopping for EZones:";
    for (auto i = 0u; i < ezone_vec_.size(); i++) {
      dbglogger_ << ' ' << GetStrFromEconomicZone(ezone_vec_[i]);
    }
    dbglogger_ << DBGLOG_ENDL_FLUSH;
  }

  mult_base_pnl_->AddListener(this);
  _watch_.subscribe_TimePeriod(this);

  Initialize();

  if (global_param_set_->portprice_type_ == 2) {
    for (int i = 0; i < num_total_sources_; i++) {
      all_market_view_vec_[i]->ComputeMktPrice();
    }
  }
  if (global_param_set_->portprice_type_ == 1) {
    for (int i = 0; i < num_total_sources_; i++) {
      all_market_view_vec_[i]->ComputeMidPrice();
    }
  }

  is_ready_ = false;
  all_smv_ready_ = false;
  record_hedged_unhedged_risks_ = false;
}

void IndexFuturesMeanRevertingTrading::ComputeCurrentSeverity() {
  applicable_severity_ = 0;
  for (auto i = 0u; i < ezone_vec_.size(); i++) {
    applicable_severity_ += economic_events_manager_.GetCurrentSeverity(ezone_vec_[i]);
  }

  if (watch_.msecs_from_midnight() > severity_change_end_msecs_ &&
      severity_to_getflat_on_ != severity_to_getflat_on_base_) {
    severity_to_getflat_on_ = severity_to_getflat_on_base_;
    DBGLOG_TIME << "Seteco set to " << severity_to_getflat_on_ << " at time " << watch_.msecs_from_midnight()
                << DBGLOG_ENDL_FLUSH;
    severity_change_end_msecs_ = trading_end_utc_mfm_;
  }

  ProcessAllowedEco();

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
      TradingLogic();
    }
  } else if (getflat_due_to_allowed_economic_event_) {
    getflat_due_to_economic_times_ = true;
    TradingLogic();
  } else if (getflat_due_to_economic_times_) {
    getflat_due_to_economic_times_ = false;
    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME << "getflat_due_to_economic_times_ set to false" << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
  }
}

void IndexFuturesMeanRevertingTrading::ProcessAllowedEco() {
  if (!allowed_events_present_) {
    return;
  }
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
      getflat_due_to_allowed_economic_event_ = false;
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

// we would need to add portfolio here as well
/**
 * Format of product file:
 * INDEX
 * I1
 * I2
 * INDEXEND
 * PRODUCT
 * P1
 * P2
 * PRODUCTEND
 * PORTFOLIO
 * PF1
 * PF2
 * PORTFOLIOEND
 */
void IndexFuturesMeanRevertingTrading::CollectORSShortCodes(DebugLogger& _dbglogger_, std::string product_filename,
                                                            std::vector<std::string>& source_shortcode_vec_,
                                                            std::vector<std::string>& ors_source_needed_vec_) {
  std::ifstream paramlistfile_;
  paramlistfile_.open(product_filename);
  //  bool paramset_file_list_read_ = false;
  if (paramlistfile_.is_open()) {
    const int kParamfileListBufflerLen = 1024;
    char readlinebuffer_[kParamfileListBufflerLen];
    bzero(readlinebuffer_, kParamfileListBufflerLen);
    while (paramlistfile_.good()) {
      bzero(readlinebuffer_, kParamfileListBufflerLen);
      paramlistfile_.getline(readlinebuffer_, kParamfileListBufflerLen);
      PerishableStringTokenizer st_(readlinebuffer_, kParamfileListBufflerLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() < 1) {
        continue;
      }
      std::string this_product_ = std::string(tokens_[0]);
      if (!this_product_.empty()) {
        VectorUtils::UniqueVectorAdd(source_shortcode_vec_, this_product_);
        VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, this_product_);
      }
    }
  }
}

void IndexFuturesMeanRevertingTrading::CollectTradingShortCodes(DebugLogger& _dbglogger_, std::string product_filename,
                                                                std::vector<std::string>& _trading_product_vec_) {
  std::ifstream paramlistfile_;
  paramlistfile_.open(product_filename);
  //  bool paramset_file_list_read_ = false;
  if (paramlistfile_.is_open()) {
    const int kParamfileListBufflerLen = 1024;
    char readlinebuffer_[kParamfileListBufflerLen];
    bzero(readlinebuffer_, kParamfileListBufflerLen);
    while (paramlistfile_.good()) {
      bzero(readlinebuffer_, kParamfileListBufflerLen);
      paramlistfile_.getline(readlinebuffer_, kParamfileListBufflerLen);
      PerishableStringTokenizer st_(readlinebuffer_, kParamfileListBufflerLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() < 1 || (tokens_.size() > 1 && strcmp(tokens_[1], "NoTrade") == 0)) {
        continue;
      }
      std::string this_product_ = std::string(tokens_[0]);
      if (!this_product_.empty()) {
        VectorUtils::UniqueVectorAdd(_trading_product_vec_, this_product_);
      }
    }
  }
}

void IndexFuturesMeanRevertingTrading::AddIndicatorListener() {
  if (global_param_set_->use_book_indicator_) {
    /*      for (auto i = 0u; i < shortcode_vec_.size(); i++)
          {
            BidAskToPayNotionalDynamicSD* t_book_indicator_ = BidAskToPayNotionalDynamicSD::GetUniqueInstance(
       dbglogger_, watch_, *dep_market_view_vec_[i], 5, 1000000, 300, kPriceTypeMidprice);
            t_book_indicator_->add_unweighted_indicator_listener(shortcode_vec_.size()+i, this);
          } */
  }
  if (global_param_set_->use_trade_indicator_) {
    for (auto i = 0u; i < shortcode_vec_.size(); i++) {
      DiffTRSizeAvgTPxBasepx* t_trade_indicator_ = DiffTRSizeAvgTPxBasepx::GetUniqueInstance(
          dbglogger_, watch_, *dep_market_view_vec_[i], 2, kPriceTypeMidprice);
      t_trade_indicator_->add_unweighted_indicator_listener(2 * shortcode_vec_.size() + i, this);
    }
  }
}

void IndexFuturesMeanRevertingTrading::PrintHedgedUnhedgedDetails() {
  if (livetrading_) return;

  std::vector<double> pos_vec_intervals_;

  // Computing avg hedged and unhedged positions
  pos_vec_intervals_.resize(pos_vec_tstamps_.size());
  std::vector<double> abs_spread_pos_vec_(spread_pos_vec_);
  std::vector<double> abs_unhedged_pos_vec_(unhedged_pos_vec_);
  unsigned i = 0;

  for (; i < pos_vec_tstamps_.size() - 1; i++) {
    pos_vec_intervals_[i] = pos_vec_tstamps_[i + 1] - pos_vec_tstamps_[i];
    // DBGLOG_TIME << "Pos: " << pos_vec_tstamps_[i] << " " << spread_pos_vec_[i] << " " << unhedged_pos_vec_[i] <<
    // DBGLOG_ENDL_FLUSH;

    if (abs_spread_pos_vec_[i] < 0) abs_spread_pos_vec_[i] *= -1;
    if (abs_unhedged_pos_vec_[i] < 0) abs_unhedged_pos_vec_[i] *= -1;
  }
  // adding the entry for the starting closed positions
  pos_vec_intervals_[i] = pos_vec_tstamps_[0] - trading_start_utc_mfm_;
  spread_pos_vec_[i] = 0;
  unhedged_pos_vec_[i] = 0;
  abs_spread_pos_vec_[i] = 0;
  abs_unhedged_pos_vec_[i] = 0;

  double avg_hedged_pos_ = VectorUtils::GetWeightedMean(spread_pos_vec_, pos_vec_intervals_);
  double avg_unhedged_pos_ = VectorUtils::GetWeightedMean(unhedged_pos_vec_, pos_vec_intervals_);
  double avg_abs_hedged_pos_ = VectorUtils::GetWeightedMean(abs_spread_pos_vec_, pos_vec_intervals_);
  double avg_abs_unhedged_pos_ = VectorUtils::GetWeightedMean(abs_unhedged_pos_vec_, pos_vec_intervals_);

  // Computing avg hedged ttc
  pos_vec_intervals_.clear();
  int prev_mfm_ = trading_start_utc_mfm_;
  bool is_flat_ = true;
  for (unsigned i = 0; i < pos_vec_tstamps_.size(); i++) {
    if (is_flat_ && spread_pos_vec_[i] != 0) {
      is_flat_ = false;
      prev_mfm_ = pos_vec_tstamps_[i];
    }
    if (!is_flat_ && spread_pos_vec_[i] == 0) {
      pos_vec_intervals_.push_back(pos_vec_tstamps_[i] - prev_mfm_);
      // DBGLOG_TIME << "Hedged Open: " << prev_mfm_ << " " << pos_vec_tstamps_[i] << DBGLOG_ENDL_FLUSH;
      is_flat_ = true;
    }
  }
  double avg_hedged_ttc_ = VectorUtils::GetMean(pos_vec_intervals_) / 1000;

  // Computing avg unhedged ttc
  pos_vec_intervals_.clear();
  prev_mfm_ = trading_start_utc_mfm_;
  is_flat_ = true;
  for (unsigned i = 0; i < pos_vec_tstamps_.size(); i++) {
    if (is_flat_ && unhedged_pos_vec_[i] != 0) {
      is_flat_ = false;
      prev_mfm_ = pos_vec_tstamps_[i];
    }
    if (!is_flat_ && unhedged_pos_vec_[i] == 0) {
      pos_vec_intervals_.push_back(pos_vec_tstamps_[i] - prev_mfm_);
      // DBGLOG_TIME << "Unhedged Open: " << prev_mfm_ << " " << pos_vec_tstamps_[i] << DBGLOG_ENDL_FLUSH;
      is_flat_ = true;
    }
  }
  double avg_unhedged_ttc_ = VectorUtils::GetMean(pos_vec_intervals_) / 1000;

  DBGLOG_TIME_CLASS << "Hedged Positions: Avg " << avg_hedged_pos_ << " AvgAbs " << avg_abs_hedged_pos_ << " TTC "
                    << avg_hedged_ttc_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME_CLASS << "Unhedged Positions: Avg " << avg_unhedged_pos_ << " AvgAbs " << avg_abs_unhedged_pos_ << " TTC "
                    << avg_unhedged_ttc_ << DBGLOG_ENDL_FLUSH;
}

void IndexFuturesMeanRevertingTrading::PrintOrderStats() {
  if (livetrading_) return;

  std::vector<int> lvls_total_time_;
  int num_lvls_ = 5;

  for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
    lvls_total_time_.clear();
    lvls_total_time_.resize(5, 0);

    for (int i = 0; i < (int)asklvls_tstamp_[t_ctr_].size() - 1; i++) {
      if (asklvls_from_best_[t_ctr_][i] >= 0 && asklvls_from_best_[t_ctr_][i] < num_lvls_) {
        lvls_total_time_[asklvls_from_best_[t_ctr_][i]] += asklvls_tstamp_[t_ctr_][i + 1] - asklvls_tstamp_[t_ctr_][i];
      }
    }
    int total_time_ = VectorUtils::GetSum(lvls_total_time_);
    DBGLOG_TIME_CLASS << "Order-levels-times for " << dep_market_view_vec_[t_ctr_]->shortcode();
    for (int i = 0; i < num_lvls_; i++) {
      dbglogger_ << " lvl-" << i << ":" << int(100 * lvls_total_time_[i] / total_time_) << "%";
    }
    dbglogger_ << DBGLOG_ENDL_FLUSH;
  }
}

void IndexFuturesMeanRevertingTrading::OnTimePeriodUpdate(const int num_pages_to_add) {
  ComputeCurrentSeverity();

  // Set getflat appropriately.
  // after specified endtime, getflat is set appropriately
  if (watch_.msecs_from_midnight() > trading_end_utc_mfm_) {
    for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
      if (!should_be_getting_flat_[t_ctr_]) {
        DBGLOG_TIME << " getflat_due_to_close_ current: " << watch_.msecs_from_midnight() << " " << trading_end_utc_mfm_
                    << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
        GetFlatTradingLogic(t_ctr_);
      }
    }
  }

  double current_unrealized_pnl_ = 0;
  for (int i = 0; i < num_total_products_; i++) {
    current_unrealized_pnl_ += order_manager_vec_[i]->base_pnl().total_pnl();
  }

  if (!livetrading_) {
    // update pnl_samples vec for SIM
    if (sample_index_ < pnl_sampling_timestamps_.size() &&
        watch_.msecs_from_midnight() >= pnl_sampling_timestamps_[sample_index_]) {
      pnl_samples_.push_back((int)current_unrealized_pnl_);
      sample_index_++;
    }
  } else {
    // if livetrading, log the open-pnl every 15secs
    if (watch_.msecs_from_midnight() >= next_open_pnl_log_timestamp_) {
      if (global_param_set_->use_dv01_ratios_) {
        ResetUnitSizes();
      }
      DBGLOG_TIME << " OpenPnl: ";
      for (int i = 0; i < num_total_products_; i++) {
        dbglogger_ << shortcode_vec_[i] << " (" << product_position_in_lots_[i] << ")"
                   << (int)order_manager_vec_[i]->base_pnl().total_pnl() << " ";
      }
      dbglogger_ << " Overall: " << (int)current_unrealized_pnl_ << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
      next_open_pnl_log_timestamp_ = watch_.msecs_from_midnight() + 60000;
    }
  }

  if (current_unrealized_pnl_ < global_min_pnl_) {
    global_min_pnl_ = current_unrealized_pnl_;
  }
}

void IndexFuturesMeanRevertingTrading::OnIndicatorUpdate(const unsigned int& _indicator_index_,
                                                         const double& _new_value_) {
  if (_indicator_index_ >= shortcode_vec_.size() && _indicator_index_ < 2 * shortcode_vec_.size()) {
    book_indicator_values_[_indicator_index_ - shortcode_vec_.size()] = _new_value_;
  } else if (_indicator_index_ >= 2 * shortcode_vec_.size() && _indicator_index_ < 3 * shortcode_vec_.size()) {
    trade_indicator_values_[_indicator_index_ - 2 * shortcode_vec_.size()] = _new_value_;
  }
}

// Supports StartTrading/StopTrading/DumpPositions
void IndexFuturesMeanRevertingTrading::OnControlUpdate(const ControlMessage& _control_message_, const char* symbol,
                                                       const int trader_id) {
  switch (_control_message_.message_code_) {
    case kControlMessageCodeGetflat: {
      GetAllFlat();
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << "getflat_due_to_external_getflat_ " << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
    } break;
    case kControlMessageCodeStartTrading: {
      for (int i = 0; i < num_total_products_; i++) {
        should_be_getting_flat_[i] = false;
      }
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << "StartTrading Called " << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      for (int i = 0; i < num_total_products_; i++) {
        order_manager_vec_[i]->UnsetCancelAllOrders();
      }
      external_freeze_trading_ = false;  // need to unfreeze since otherwise will nto be able to send orders
      freeze_due_to_rejects_ = false;
      for (int i = 0; i < num_total_products_; i++) {
        order_manager_vec_[i]->ResetRejectsBasedFreeze();
      };
    } break;
    case kControlMessageCodeFreezeTrading: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "FreezeTrading called " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      external_freeze_trading_ = true;
    } break;
    case kControlMessageCodeUnFreezeTrading: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "UnFreezeTrading called " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      external_freeze_trading_ = false;
      freeze_due_to_rejects_ = false;
      for (int i = 0; i < num_total_products_; i++) {
        order_manager_vec_[i]->ResetRejectsBasedFreeze();
      };
    } break;
    case kControlMessageCodeCancelAllFreezeTrading: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "CancelAllFreezeTrading called " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      // On receiving this, we will not be placing any new orders.
      // We will just cancel old orders
      for (int i = 0; i < num_total_products_; i++) {
        order_manager_vec_[i]
            ->CancelAllOrders(); /* cancelallorders flag in ordermanager is used to indicate that unconfirmed
             orders are to be canceled as soon as they are confirmed. Hence we need to set
             that */
      };
      external_freeze_trading_ = true;
    } break;
    case kControlMessageCodeDumpPositions: {
      for (int i = 0; i < num_total_products_; i++) {
        dbglogger_ << shortcode_vec_[i].c_str() << " Pos " << product_position_in_lots_[i] << '\n';
      }
      DBGLOG_DUMP;
    } break;
    case kControlMessageCodeShowOrders: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "ShowOrders called." << DBGLOG_ENDL_FLUSH;

        for (int i = 0; i < num_total_products_; i++) {
          order_manager_vec_[i]->LogFullStatus();
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      }
    } break;
    case kControlMessageReloadEconomicEvents: {
      DBGLOG_TIME_CLASS << "IndexFuturesMeanRevertingTrading::Got refreshecoevents user_msg " << DBGLOG_ENDL_FLUSH;
      economic_events_manager_.ReloadDB();
      economic_events_manager_.AllowEconomicEventsFromList(first_dep_smv_->shortcode());
      allowed_events_present_ = economic_events_manager_.AllowedEventsPresent();
      getflat_due_to_allowed_economic_event_ = false;
      economic_events_manager_.AdjustSeverity(first_dep_smv_->shortcode(), first_dep_smv_->exch_source());
    } break;
    case kControlMessageShowEconomicEvents: {
      DBGLOG_TIME_CLASS << "IndexFuturesMeanRevertingTrading::Got showecoevents user_msg " << DBGLOG_ENDL_FLUSH;
      economic_events_manager_.ShowDB();
    } break;
    case kControlMessageCodeShowIndicators: {
      DBGLOG_TIME << "ShowIndicators called " << DBGLOG_ENDL_FLUSH;
      for (int i = 0; i < num_total_products_; i++) {
        DBGLOG_TIME_CLASS_FUNC << "Instrument: " << dep_market_view_vec_[i]->shortcode()
                               << " stdev: " << stdev_residuals_[i] << " beta: " << inst_betas_[i] << " port_price "
                               << last_port_prices_[i] << " mid_price " << dep_market_view_vec_[i]->mid_price()
                               << DBGLOG_ENDL_FLUSH;
      }
      if (livetrading_) {
        DBGLOG_DUMP;
      }
    } break;
    case kControlMessageCodeSetEcoSeverity: {
      DBGLOG_TIME << "IgnoreEconomicNumbers called." << DBGLOG_ENDL_FLUSH;
      severity_to_getflat_on_ = std::max(1.00, _control_message_.doubleval_1_);
      int t_severity_change_end_msecs_ = GetMsecsFromMidnightFromHHMMSS(_control_message_.intval_1_);
      int complete_days_append_ = watch_.msecs_from_midnight() / 86400000;
      t_severity_change_end_msecs_ += complete_days_append_ * 86400000;

      severity_change_end_msecs_ = std::min(trading_end_utc_mfm_, t_severity_change_end_msecs_);
      DBGLOG_TIME << "Seteco set to " << severity_to_getflat_on_ << " at time " << watch_.msecs_from_midnight()
                  << " with end time as " << severity_change_end_msecs_ << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
      ComputeCurrentSeverity();
    } break;
    case kControlMessageCodeSetMaxLoss: {
      if (_control_message_.intval_1_ > global_param_set_->strat_max_loss_ / FAT_FINGER_FACTOR &&
          _control_message_.intval_1_ < global_param_set_->strat_max_loss_ * FAT_FINGER_FACTOR) {
        global_param_set_->strat_max_loss_ = _control_message_.intval_1_;
      }

      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "SetStratMaxLoss called for max_loss_ = " << _control_message_.intval_1_
                          << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      if (CheckStratMaxLoss()) TradingLogic();
    } break;
    case kControlMessageCodeSetMaxPosition: {
      int target_maxlots_ = std::max(1, _control_message_.intval_1_);
      if (target_maxlots_ > global_param_set_->inst_maxlots_ / FAT_FINGER_FACTOR &&
          target_maxlots_ < global_param_set_->inst_maxlots_ * FAT_FINGER_FACTOR) {
        global_param_set_->inst_maxlots_ = target_maxlots_;
      }
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "SetMaxPosition called with " << _control_message_.intval_1_
                          << " and MaxLots set to global_ " << global_param_set_->inst_maxlots_ << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }

      BuildThresholds();
    } break;
    case kControlMessageCodeSetStartTime: {
      int old_trading_start_utc_mfm_ = -1;

      if (_control_message_.intval_1_ >= 0 && _control_message_.intval_1_ <= 2359) {  // utc hhmm time sanity check.
        old_trading_start_utc_mfm_ = trading_start_utc_mfm_;
        trading_start_utc_mfm_ = GetMsecsFromMidnightFromHHMM(_control_message_.intval_1_) + watch_.day_offset();
      }

      DBGLOG_TIME_CLASS << "SetStartTime called with " << _control_message_.intval_1_
                        << " and trading_start_utc_mfm_ set to " << trading_start_utc_mfm_ << " from "
                        << old_trading_start_utc_mfm_ << DBGLOG_ENDL_FLUSH;

      if (livetrading_) {
        DBGLOG_DUMP;
      }

      TradingLogic();
    } break;
    case kControlMessageCodeSetEndTime: {
      int old_trading_end_utc_mfm_ = -1;

      if (_control_message_.intval_1_ >= 0 && _control_message_.intval_1_ <= 2359) {  // utc hhmm time sanity check.
        old_trading_end_utc_mfm_ = trading_end_utc_mfm_;
        trading_end_utc_mfm_ = GetMsecsFromMidnightFromHHMM(_control_message_.intval_1_) +
                               watch_.day_offset();  // no solution if someone calls it during the previous day UTC
                                                     // itself.Trading will stop immediately in this pathological case
      }
      DBGLOG_TIME_CLASS << "SetEndTime called with " << _control_message_.intval_1_
                        << " and trading_end_utc_mfm_ set to " << trading_end_utc_mfm_ << " from "
                        << old_trading_end_utc_mfm_ << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
      TradingLogic();
    } break;
    case kControlMessageCodeSetUnitTradeSize: {
      // intval_1_ is the scaling ratio in percentage
      // example: if we want to half the UTSs, intval_1_ is 50
      // example: if we want to double the UTSs, intval_1_ is 200
      DBGLOG_TIME_CLASS << "SetUnitTradeSize called with Scaling Factor " << _control_message_.intval_1_
                        << DBGLOG_ENDL_FLUSH;
      for (int i = 0; i < num_total_products_; i++) {
        if (_control_message_.intval_1_ > 100 / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < 100 * FAT_FINGER_FACTOR) {
          double scale_ = (double)_control_message_.intval_1_ / 100;

          // scale the UTS and maxlots
          inst_unitlots_[i] = (int)round(inst_unitlots_[i] * scale_);
          inst_worstcaselots_[i] = (int)round(inst_worstcaselots_[i] * scale_);
          inst_maxlots_[i] = (int)round(inst_maxlots_[i] * scale_);

          // scale the thresholds
          inst_increase_threshold_[i] = inst_increase_threshold_[i] / scale_;
          inst_decrease_threshold_[i] = inst_decrease_threshold_[i] / scale_;

          if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
            DBGLOG_TIME_CLASS << "Instrument " << dep_market_view_vec_[i]->shortcode() << " UTS: " << inst_unitlots_[i]
                              << " MaxLots: " << inst_maxlots_[i] << " WorstcaseLots: " << inst_worstcaselots_[i]
                              << DBGLOG_ENDL_FLUSH;
            if (livetrading_) {
              DBGLOG_DUMP;
            }
          }
        }
      }
      SetBuySellThresholds();
      TradingLogic();
    } break;
    default:
      break;
  };
}

bool IndexFuturesMeanRevertingTrading::CheckSMVOnReadyAllProducts() {
  if (all_smv_ready_) {
    return true;
  } else {
    unsigned int i = 0u;
    for (; i < all_market_view_vec_.size(); i++) {
      if (!all_market_view_vec_[i]->is_ready()) {
        break;
      }
    }
    if (i == all_market_view_vec_.size()) {
      all_smv_ready_ = true;
    }

    return all_smv_ready_;
  }
}

void IndexFuturesMeanRevertingTrading::OnMarketUpdate(const unsigned int _security_id_,
                                                      const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (livetrading_ && (watch_.msecs_from_midnight() >
                         (trading_start_utc_mfm_ - (RELEASE_CORE_BEFORE_GIVEN_INTERVAL_IN_MSEC - 30 * 1000)))) {
      // allocating CPU 1 minute before start_time as it takes few seconds for allocation which delays initial order
      // placing
      AllocateCPU();

      // We are not starting the query unless it gets affined
      if (!is_affined_) return;
    }

    if (!all_smv_ready_) CheckSMVOnReadyAllProducts();

    // DO NOT START ANY PROCESS UNTIL ALL UNDERLIER SMV's ARE READY
    if (!all_smv_ready_) return;

    if (all_smv_ready_ && watch_.msecs_from_midnight() > trading_start_utc_mfm_) {
      is_ready_ = true;
      record_hedged_unhedged_risks_ = true;
    }
  }

  // if book has crossed last price then update last price and recompute affected portfolio prices
  double t_diff_ = 0;
  if (all_market_view_vec_[_security_id_]->bestbid_price() > last_inst_prices_[_security_id_] &&
      all_market_view_vec_[_security_id_]->bestbid_size() > 0) {
    t_diff_ = all_market_view_vec_[_security_id_]->bestbid_price() - last_inst_prices_[_security_id_];
  }
  if (all_market_view_vec_[_security_id_]->bestask_price() < last_inst_prices_[_security_id_] &&
      all_market_view_vec_[_security_id_]->bestask_size() > 0 &&
      all_market_view_vec_[_security_id_]->bestask_price() > 0) {
    t_diff_ = all_market_view_vec_[_security_id_]->bestask_price() - last_inst_prices_[_security_id_];
  }

  if (fabs(t_diff_) > 1e-5) {
    UpdatePortPxForId(_security_id_, t_diff_);
    CheckStratMaxLoss();
    TradingLogic();
  }

  if (!livetrading_) {
    for (int i = 0; i < num_total_products_; i++) {
      int ask_order_lvl_ = GetHighestAskIntPrice(i);
      if (ask_order_lvl_ > 0) ask_order_lvl_ = ask_order_lvl_ - all_market_view_vec_[i]->bestask_int_price();

      if (ask_order_lvl_ != prev_ask_order_lvl_[i]) {
        asklvls_tstamp_[i].push_back(watch_.msecs_from_midnight());
        asklvls_from_best_[i].push_back(ask_order_lvl_);
        prev_ask_order_lvl_[i] = ask_order_lvl_;
      }

      int bid_order_lvl_ = GetHighestBidIntPrice(i);
      if (bid_order_lvl_ > 0) bid_order_lvl_ = all_market_view_vec_[i]->bestask_int_price() - bid_order_lvl_;

      if (bid_order_lvl_ != prev_bid_order_lvl_[i]) {
        bidlvls_tstamp_[i].push_back(watch_.msecs_from_midnight());
        bidlvls_from_best_[i].push_back(bid_order_lvl_);
        prev_bid_order_lvl_[i] = bid_order_lvl_;
      }
    }
  }
}

int IndexFuturesMeanRevertingTrading::GetHighestAskIntPrice(int security_id_) {
  int top_index_ = order_manager_vec_[security_id_]->GetConfirmedTopAskIndex();
  if (top_index_ != -1) top_index_ = order_manager_vec_[security_id_]->GetAskIntPrice(top_index_);

  return top_index_;
}

int IndexFuturesMeanRevertingTrading::GetHighestBidIntPrice(int security_id_) {
  int top_index_ = order_manager_vec_[security_id_]->GetConfirmedTopBidIndex();
  if (top_index_ != -1) top_index_ = order_manager_vec_[security_id_]->GetBidIntPrice(top_index_);

  return top_index_;
}

void IndexFuturesMeanRevertingTrading::AllocateCPU() {
  // Retry every 10 secs
  if (is_affined_ || watch_.msecs_from_midnight() - last_affine_attempt_msecs_ < 10000) {
    return;
  }

  if (first_affine_attempt_msecs_ == 0) {
    first_affine_attempt_msecs_ = watch_.msecs_from_midnight();
  }

  std::vector<std::string> affinity_process_list_vec;
  process_type_map process_and_type = AffinityAllocator::parseProcessListFile(affinity_process_list_vec);

  // Construct name by which we'll identify a process in affinity tracking
  std::ostringstream t_temp_oss;
  t_temp_oss << "tradeinit-" << runtime_id_;

  int32_t core_assigned = CPUManager::allocateFirstBestAvailableCore(process_and_type, affinity_process_list_vec,
                                                                     getpid(), t_temp_oss.str(), false);
  is_affined_ = (core_assigned >= 0);

  // If we don't get a core even after 50 secs of first attempt , then exit from the query.
  // TODO - A lot of the code here has been duplicated across functions and classes,
  // Ideally we should have a parent class for all affinity management
  if (!is_affined_ && (watch_.msecs_from_midnight() - first_affine_attempt_msecs_ > 50000)) {
    DBGLOG_CLASS_FUNC_LINE_INFO << " PID : " << getpid()
                                << " not getting affined even after 50 secs. Stopping the query." << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    ExitVerbose(kExitErrorCodeGeneral, "Query not able to get affined");
  }

  last_affine_attempt_msecs_ = watch_.msecs_from_midnight();
  DBGLOG_CLASS_FUNC_LINE_INFO << " AFFINED TO : " << core_assigned << " PID : " << getpid() << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;
}

void IndexFuturesMeanRevertingTrading::UpdatePortPxForId(int _security_id_, double px_diff_) {
  last_inst_prices_[_security_id_] = last_inst_prices_[_security_id_] + px_diff_;
  std::vector<std::pair<int, double>>::iterator t_iter_;
  for (t_iter_ = predictor_vec_[_security_id_].begin(); t_iter_ != predictor_vec_[_security_id_].end(); t_iter_++) {
    int t_port_idx_ = (*t_iter_).first;
    double t_port_wt_ = (*t_iter_).second;
    last_port_prices_[t_port_idx_] = last_port_prices_[t_port_idx_] + t_port_wt_ * px_diff_;
    // target prices might have changed for this .. so recompute best prices to place at
    SetBestPrices(t_port_idx_);
  }
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "InstrumentPrice: " << all_market_view_vec_[_security_id_]->shortcode() << " "
                           << last_inst_prices_[_security_id_] << DBGLOG_ENDL_FLUSH;
  }
}

void IndexFuturesMeanRevertingTrading::SetBestPrices(int _security_id_) {
  int pos = abs(product_position_in_lots_[_security_id_]);
  double t_buy_k_ = inst_base_threshold_[_security_id_];
  double t_sell_k_ = inst_base_threshold_[_security_id_];

  if (product_position_in_lots_[_security_id_] > 0) {
    t_buy_k_ = buy_thresholds_[_security_id_][pos];
    t_sell_k_ = sell_thresholds_[_security_id_][pos];
    t_sell_k_ = std::max(0.0, t_sell_k_ -
                                  global_param_set_->time_hysterisis_factor_ *
                                      (watch_.msecs_from_midnight() / 1000.0 - seconds_at_last_buy_[_security_id_]));
  } else if (product_position_in_lots_[_security_id_] < 0) {
    t_buy_k_ = sell_thresholds_[_security_id_][pos];
    t_sell_k_ = buy_thresholds_[_security_id_][pos];
    t_buy_k_ = std::max(0.0, t_buy_k_ -
                                 global_param_set_->time_hysterisis_factor_ *
                                     (watch_.msecs_from_midnight() / 1000.0 - seconds_at_last_sell_[_security_id_]));
  }

  double t_buy_keep_k_ = t_buy_k_ - global_param_set_->place_keep_diff_;
  double t_sell_keep_k_ = t_sell_k_ - global_param_set_->place_keep_diff_;

  bid_int_price_to_place_at_[_security_id_] =
      std::min(dep_market_view_vec_[_security_id_]->bestbid_int_price(),
               (int)floor((last_port_prices_[_security_id_] * inst_betas_[_security_id_] -
                           t_buy_k_ * stdev_residuals_[_security_id_]) /
                          dep_market_view_vec_[_security_id_]->min_price_increment()));
  int t_bid_intpx_ = bid_int_price_to_place_at_[_security_id_];

  bid_int_price_to_keep_at_[_security_id_] =
      std::min(dep_market_view_vec_[_security_id_]->bestbid_int_price(),
               (int)floor((last_port_prices_[_security_id_] * inst_betas_[_security_id_] -
                           t_buy_keep_k_ * stdev_residuals_[_security_id_]) /
                          dep_market_view_vec_[_security_id_]->min_price_increment()));

  // to avoid excessive messages
  if (bid_int_price_to_place_at_[_security_id_] <
      dep_market_view_vec_[_security_id_]->bestbid_int_price() - support_lvl_buffer_[_security_id_]) {
    bid_int_price_to_place_at_[_security_id_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
  }
  if (bid_int_price_to_keep_at_[_security_id_] <
      dep_market_view_vec_[_security_id_]->bestbid_int_price() - support_lvl_buffer_[_security_id_]) {
    bid_int_price_to_keep_at_[_security_id_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
  }

  ask_int_price_to_place_at_[_security_id_] =
      std::max(dep_market_view_vec_[_security_id_]->bestask_int_price(),
               (int)ceil((last_port_prices_[_security_id_] * inst_betas_[_security_id_] +
                          t_sell_k_ * stdev_residuals_[_security_id_]) /
                         dep_market_view_vec_[_security_id_]->min_price_increment()));
  int t_ask_intpx_ = ask_int_price_to_place_at_[_security_id_];

  ask_int_price_to_keep_at_[_security_id_] =
      std::max(dep_market_view_vec_[_security_id_]->bestask_int_price(),
               (int)ceil((last_port_prices_[_security_id_] * inst_betas_[_security_id_] +
                          t_sell_keep_k_ * stdev_residuals_[_security_id_]) /
                         dep_market_view_vec_[_security_id_]->min_price_increment()));

  // to avoid excessive messages
  if (ask_int_price_to_place_at_[_security_id_] >
      dep_market_view_vec_[_security_id_]->bestask_int_price() + support_lvl_buffer_[_security_id_]) {
    ask_int_price_to_place_at_[_security_id_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
  }
  if (ask_int_price_to_keep_at_[_security_id_] >
      dep_market_view_vec_[_security_id_]->bestask_int_price() + support_lvl_buffer_[_security_id_]) {
    ask_int_price_to_keep_at_[_security_id_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
  }

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "Instrument: " << dep_market_view_vec_[_security_id_]->shortcode() << " Prices_to_place:"
                           << " Bid " << bid_int_price_to_place_at_[_security_id_] << " (" << t_bid_intpx_ << ")"
                           << " Ask " << ask_int_price_to_place_at_[_security_id_] << " (" << t_ask_intpx_ << ")"
                           << " Mkt Px " << last_inst_prices_[_security_id_] << " Port_Px "
                           << last_port_prices_[_security_id_] << " Beta " << inst_betas_[_security_id_] << " Stdev "
                           << stdev_residuals_[_security_id_] << " Pos " << product_position_in_lots_[_security_id_]
                           << DBGLOG_ENDL_FLUSH;
  }
}

void IndexFuturesMeanRevertingTrading::UpdatePrices() {
  // update  vectors and recompute betas
  if (watch_.msecs_from_midnight() - msecs_at_last_vec_processing_ >= 60000) {
    for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
      if (dont_trade_[t_ctr_] == false) {
        inst_prices_[t_ctr_].push_back(last_inst_prices_[t_ctr_]);
        port_prices_[t_ctr_].push_back(last_port_prices_[t_ctr_]);
        inst_prices_[t_ctr_].erase(inst_prices_[t_ctr_].begin());
        port_prices_[t_ctr_].erase(port_prices_[t_ctr_].begin());
        double t_new_beta_ = std::inner_product(inst_prices_[t_ctr_].begin(), inst_prices_[t_ctr_].end(),
                                                port_prices_[t_ctr_].begin(), 0.0) /
                             std::inner_product(port_prices_[t_ctr_].begin(), port_prices_[t_ctr_].end(),
                                                port_prices_[t_ctr_].begin(), 0.0);
        inst_betas_[t_ctr_] = t_new_beta_;
        online_price_update_count_[t_ctr_] += 1;
        DBGLOG_TIME_CLASS_FUNC << "Instrument: " << dep_market_view_vec_[t_ctr_]->shortcode()
                               << " Inst_Prices: " << last_inst_prices_[t_ctr_]
                               << " Port_Prices: " << last_port_prices_[t_ctr_]
                               << " OnlineBeta: " << inst_betas_[t_ctr_]
                               << " Stdev Residuals: " << stdev_residuals_[t_ctr_] << DBGLOG_ENDL_FLUSH;

        // DBGLOG_TIME_CLASS_FUNC << "Instrument: " << all_market_view_vec_[t_ctr_]->shortcode() << " port_price " <<
        // last_port_prices_[t_ctr_] << " inst_price " << last_inst_prices_[t_ctr_] << " Beta " <<
        // t_new_beta_ << " online_price " << online_price_update_count_[t_ctr_] << DBGLOG_ENDL_FLUSH;

        // update the residual_stdev only after the price_vec has all elements from today's run
        if (online_price_update_count_[t_ctr_] >= (int)global_param_set_->hist_price_length_) {
          double t_del_residual_ = residuals_[t_ctr_][0];
          residual_sum_[t_ctr_] -= t_del_residual_;
          residual_sumsqr_[t_ctr_] -= (t_del_residual_ * t_del_residual_);
          double t_add_residual_ = last_inst_prices_[t_ctr_] - inst_betas_[t_ctr_] * last_port_prices_[t_ctr_];
          residual_sum_[t_ctr_] += t_add_residual_;
          residual_sumsqr_[t_ctr_] += (t_add_residual_ * t_add_residual_);
          residuals_[t_ctr_].push_back(t_add_residual_);
          residuals_[t_ctr_].erase(residuals_[t_ctr_].begin());
          stdev_residuals_[t_ctr_] = sqrt(residual_sumsqr_[t_ctr_] / global_param_set_->hist_error_length_ -
                                          residual_sum_[t_ctr_] / global_param_set_->hist_error_length_ *
                                              residual_sum_[t_ctr_] / global_param_set_->hist_error_length_);
          DBGLOG_TIME_CLASS_FUNC << "Instrument: " << dep_market_view_vec_[t_ctr_]->shortcode()
                                 << " Inst_Prices: " << last_inst_prices_[t_ctr_]
                                 << " Port_Prices: " << last_port_prices_[t_ctr_]
                                 << " OnlineBeta: " << inst_betas_[t_ctr_]
                                 << " Stdev Residuals: " << stdev_residuals_[t_ctr_]
                                 << " Last Added Residual: " << t_add_residual_ << DBGLOG_ENDL_FLUSH;
        }
      }
    }
    msecs_at_last_vec_processing_ = watch_.msecs_from_midnight();
  }
}

void IndexFuturesMeanRevertingTrading::OnTradePrint(const unsigned int _security_id_,
                                                    const TradePrintInfo& _trade_print_info_,
                                                    const MarketUpdateInfo& _market_update_info_) {
  if (!all_smv_ready_) return;

  // if book has crossed last price then update last price and recompute affected portfolio prices
  if (global_param_set_->portprice_type_ == 0) {
    double t_diff_ = _trade_print_info_.trade_price_ - last_inst_prices_[_security_id_];
    if (fabs(t_diff_) > 0) {
      UpdatePortPxForId(_security_id_, t_diff_);
    }
  } else if (global_param_set_->portprice_type_ == 1) {
    double t_diff_ = all_market_view_vec_[_security_id_]->mid_price() - last_inst_prices_[_security_id_];
    // DBGLOG_TIME << " security " << _security_id_ << " mid price " << all_market_view_vec_[_security_id_]->mid_price()
    // << DBGLOG_ENDL_FLUSH;
    if (fabs(t_diff_) > 0) {  // 0.5 * all_market_view_vec_[_security_id_]->min_price_increment()
      UpdatePortPxForId(_security_id_, t_diff_);
    }
  } else if (global_param_set_->portprice_type_ == 2) {
    double t_diff_ = all_market_view_vec_[_security_id_]->mkt_size_weighted_price() - last_inst_prices_[_security_id_];
    // DBGLOG_TIME << " security " << _security_id_ << " mkt price " <<
    // all_market_view_vec_[_security_id_]->mkt_size_weighted_price() << DBGLOG_ENDL_FLUSH;
    if (fabs(t_diff_) > 0.5 * all_market_view_vec_[_security_id_]->min_price_increment()) {
      UpdatePortPxForId(_security_id_, t_diff_);
    }
  }

  // start logging the prices for beta computation only after (start_time - hist_price_length_)
  if (watch_.msecs_from_midnight() < trading_start_utc_mfm_ - 60000 * (int)global_param_set_->hist_price_length_) {
    return;
  }

  UpdatePrices();
  CheckStratMaxLoss();
  TradingLogic();
}

bool IndexFuturesMeanRevertingTrading::CheckStratMaxLoss() {
  bool overall_should_be_getting_flat_ = false;

  for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
    if (order_manager_vec_[t_ctr_]->base_pnl().total_pnl() < -1 * global_param_set_->product_maxloss_ &&
        !should_be_getting_flat_[t_ctr_]) {
      DBGLOG_TIME << " getflat_due_to_product_maxloss_ " << shortcode_vec_[t_ctr_] << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
      should_be_getting_flat_[t_ctr_] = true;
    }
  }

  if (total_pnl_ < -1.0 * global_param_set_->strat_max_loss_) {
    for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
      if (!should_be_getting_flat_[t_ctr_]) {
        DBGLOG_TIME << " getflat_due_to_strat_maxloss_ " << shortcode_vec_[t_ctr_] << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
        should_be_getting_flat_[t_ctr_] = true;
      }
    }
    overall_should_be_getting_flat_ = true;
  }
  return overall_should_be_getting_flat_;
}

void IndexFuturesMeanRevertingTrading::OnExec(const int _new_position_, const int _exec_quantity_,
                                              const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                                              const int _security_id_) {
  // update our position variables here
  product_position_in_lots_[_security_id_] = _new_position_ / dep_market_view_vec_[_security_id_]->min_order_size();

  if (_buysell_ == HFSAT::kTradeTypeBuy) {
    seconds_at_last_buy_[_security_id_] = watch_.msecs_from_midnight() / 1000;
  } else {
    seconds_at_last_sell_[_security_id_] = watch_.msecs_from_midnight() / 1000;
  }

  SetBestPrices(_security_id_);
  /*
  int t_tot_lots_ = 0;
  double t_tot_pnl_ = 0.0;

  for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
    t_tot_lots_ += product_position_in_lots_[t_ctr_];
    t_tot_pnl_ += order_manager_vec_[t_ctr_]->base_pnl().ReportConservativeTotalPNL(true);
  }
*/
  TradingLogic();

  // record the hedged and unhedged positions till getflat after close
  if (!livetrading_) {
    double current_risk_ = my_portfolio_risk();
    double spread_risk_ = ((double)product_position_in_lots_[0] / inst_unitlots_[0]) - current_risk_;
    if (record_hedged_unhedged_risks_) {
      spread_pos_vec_.push_back(spread_risk_);
      unhedged_pos_vec_.push_back(current_risk_);
      pos_vec_tstamps_.push_back(watch_.msecs_from_midnight());

      if (watch_.msecs_from_midnight() > trading_end_utc_mfm_ && current_risk_ == 0 && spread_risk_ == 0) {
        record_hedged_unhedged_risks_ = false;
      }
    }
  }
}

void IndexFuturesMeanRevertingTrading::ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_) {
  int t_total_volume_ = 0;
  int t_total_orderc_ = 0;
  int t_total_order_ = 0;
  int t_total_pnl_ = 0;
  int t_supporting_orders_filled_ = 0;
  int t_bestlevel_orders_filled_ = 0;
  int t_aggressive_orders_filled_ = 0;
  int t_improve_orders_filled_ = 0;

  for (int i = 0; i < num_total_products_; i++) {
    int ind_volume_ = order_manager_vec_[i]->trade_volume();
    int ind_orderc_ = order_manager_vec_[i]->SendOrderCount() + order_manager_vec_[i]->CxlOrderCount();
    double ind_pnl_ = order_manager_vec_[i]->base_pnl().ReportConservativeTotalPNL(_conservative_close_);

    t_supporting_orders_filled_ += order_manager_vec_[i]->SupportingOrderFilledPercent() * ind_volume_;
    t_bestlevel_orders_filled_ += order_manager_vec_[i]->BestLevelOrderFilledPercent() * ind_volume_;
    t_improve_orders_filled_ += order_manager_vec_[i]->ImproveOrderFilledPercent() * ind_volume_;
    t_aggressive_orders_filled_ += order_manager_vec_[i]->AggressiveOrderFilledPercent() * ind_volume_;
    t_total_volume_ += ind_volume_;
    t_total_orderc_ += ind_orderc_;
    t_total_pnl_ += ind_pnl_;

    std::cout << watch_.YYYYMMDD() << ' ' << shortcode_vec_[i] << " " << ind_pnl_ << " " << ind_volume_ << " "
              << order_manager_vec_[i]->SupportingOrderFilledPercent() << " "
              << order_manager_vec_[i]->BestLevelOrderFilledPercent() << " "
              << order_manager_vec_[i]->ImproveOrderFilledPercent() << " "
              << order_manager_vec_[i]->AggressiveOrderFilledPercent() << "\n";
  }

  if (t_total_volume_ > 0) {
    t_supporting_orders_filled_ /= t_total_volume_;
    t_bestlevel_orders_filled_ /= t_total_volume_;
    t_improve_orders_filled_ /= t_total_volume_;
    t_aggressive_orders_filled_ /= t_total_volume_;
  }

  printf("SIMRESULT %d %d %d %d %d %d\n", total_pnl_, t_total_volume_, t_supporting_orders_filled_,
         t_bestlevel_orders_filled_, t_aggressive_orders_filled_, t_improve_orders_filled_);

  std::stringstream st;

  st << "EOD_MIN_PNL: " << runtime_id_ << " " << (int)global_min_pnl_ << "\n";

  for (int i = 0; i < num_total_products_; i++) {
    st << "EOD_MSG_COUNT: " << runtime_id_ << " " << secname_vec_[i] << " "
       << (order_manager_vec_[i]->SendOrderCount() + order_manager_vec_[i]->CxlOrderCount() +
           order_manager_vec_[i]->ModifyOrderCount()) << "\n";
    t_total_order_ += order_manager_vec_[i]->SendOrderCount() + order_manager_vec_[i]->CxlOrderCount() +
                      order_manager_vec_[i]->ModifyOrderCount();
  }

  st << "EOD_MSG_COUNT: " << runtime_id_ << " TOTAL " << t_total_order_ << "\n";
  st << "SIMRESULT " << runtime_id_ << " " << total_pnl_ << " " << t_total_volume_ << " " << t_supporting_orders_filled_
     << " " << t_bestlevel_orders_filled_ << " " << t_aggressive_orders_filled_ << " " << t_improve_orders_filled_
     << "\n";
  st << "PNLSAMPLES " << runtime_id_ << " ";
  if (pnl_samples_.size() > 0) {
    for (auto i = 0u; i < pnl_samples_.size(); i++) {
      st << pnl_sampling_timestamps_[i] << " " << pnl_samples_[i] << " ";
    }
    st << "\n";
  } else {
    st << trading_end_utc_mfm_ << " " << total_pnl_ << "\n";
  }

  trades_writer_ << st.str();
  dbglogger_ << st.str();

  PrintHedgedUnhedgedDetails();

  PrintOrderStats();
  // st.str(std::string());
  // st.clear();
}

// Not used anywhere. Is just being written to comply with pure virtual function
int IndexFuturesMeanRevertingTrading::my_global_position() const {
  // sum all position in pos_vec_
  double pos_ = 0;
  for (int i = 0; i < num_total_products_; i++) {
    pos_ += product_position_in_lots_[i] / inst_unitlots_[i];
  }
  return ((int)pos_);
}

double IndexFuturesMeanRevertingTrading::my_portfolio_risk() {
  // sum all position in pos_vec_
  double pos_ = 0;
  for (int i = 0; i < num_total_products_; i++) {
    pos_ = pos_ + ((double)product_position_in_lots_[i] / inst_unitlots_[i]);
  }
  return (pos_);
}

int IndexFuturesMeanRevertingTrading::my_position_for_outright(SecurityMarketView* _dep_market_view_) {
  int pos_ = 0;
  for (int i = 0; i < num_total_products_; i++) {
    if (_dep_market_view_ == dep_market_view_vec_[i]) {
      pos_ = product_position_in_lots_[i];
      break;
    }
  }
  return pos_;
}

void IndexFuturesMeanRevertingTrading::UpdatePNL(int _total_pnl_) {
  total_pnl_ = _total_pnl_;
  double current_risk_ = my_portfolio_risk();
  mult_base_pnl_->UpdateTotalRisk(current_risk_);

  double spread_risk_ = ((double)product_position_in_lots_[0] / inst_unitlots_[0]) - current_risk_;
  mult_base_pnl_->UpdatePortRisk(spread_risk_);
}

void IndexFuturesMeanRevertingTrading::SetBuySellThresholds() {
  for (int i = 0; i < num_total_products_; i++) {
    buy_thresholds_[i].resize(inst_maxlots_[i] + 1);
    sell_thresholds_[i].resize(inst_maxlots_[i] + 1);
    for (int j = 0; j <= inst_maxlots_[i]; j++) {
      buy_thresholds_[i][j] = inst_base_threshold_[i] + ((double)inst_increase_threshold_[i] * j);
      sell_thresholds_[i][j] = inst_base_threshold_[i] - ((double)inst_decrease_threshold_[i] * j);
    }
  };
  for (int i = 0; i < num_total_products_; i++) {
    for (int j = -1 * inst_maxlots_[i]; j < 0; j++) {
      dbglogger_ << dep_market_view_vec_[i]->shortcode() << " Position: " << j
                 << " BuyThreshold: " << sell_thresholds_[i][-j] << " SellThreshold: " << buy_thresholds_[i][-j]
                 << '\n';
    }
    for (int j = 0; j <= inst_maxlots_[i]; j++) {
      dbglogger_ << dep_market_view_vec_[i]->shortcode() << " Position: " << j
                 << " BuyThreshold: " << buy_thresholds_[i][j] << " SellThreshold: " << sell_thresholds_[i][j] << '\n';
    }
  };
};

void IndexFuturesMeanRevertingTrading::BuildThresholds() {
  for (int i = 0; i < num_total_products_; i++) {
    if (global_param_set_->use_notional_scaling_ || global_param_set_->use_notional_n2d_scaling_) {
      double t_inst_notional_per_lot_;

      if (global_param_set_->use_notional_scaling_) {
        // max introduced since last_inst_prices_ for an instrument is not necessarily present in histfile.
        t_inst_notional_per_lot_ =
            std::max(100000.0, last_inst_prices_[i] * (all_market_view_vec_[i]->min_order_size()));
      } else {
        t_inst_notional_per_lot_ =
            last_inst_prices_[i] * all_market_view_vec_[i]->min_order_size() *
            SecurityDefinitions::contract_specification_map_[dep_market_view_vec_[i]->shortcode()].numbers_to_dollars_;
      }

      inst_unitlots_[i] = std::max(1, (int)round(global_param_set_->notional_for_unit_lot_ *
                                                 global_param_set_->inst_uts_ / t_inst_notional_per_lot_));

      inst_maxlots_[i] =
          std::max(1, (int)round(inst_unitlots_[i] * global_param_set_->inst_maxlots_ / global_param_set_->inst_uts_));

      inst_worstcaselots_[i] = std::max(
          1, (int)round(inst_unitlots_[i] * global_param_set_->inst_worstcaselots_ / global_param_set_->inst_uts_));

      if (global_param_set_->read_base_threshold_vec_ && global_param_set_->base_threshold_vec_.size() > (unsigned)i) {
        inst_base_threshold_[i] = global_param_set_->base_threshold_vec_[i];
      } else {
        inst_base_threshold_[i] = global_param_set_->base_threshold_;
      }
      inst_increase_threshold_[i] =
          global_param_set_->increase_threshold_ * global_param_set_->inst_maxlots_ / inst_maxlots_[i];
      inst_decrease_threshold_[i] =
          global_param_set_->decrease_threshold_ * global_param_set_->inst_maxlots_ / inst_maxlots_[i];

    } else if (global_param_set_->use_dv01_ratios_) {
      current_dv01_vec_[i] =
          HFSAT::CurveUtils::dv01(dep_market_view_vec_[i]->shortcode(), watch_.YYYYMMDD(), last_inst_prices_[i]);
      inst_unitlots_[i] =
          std::max(1, (int)round(current_dv01_vec_[0] * global_param_set_->inst_uts_ / current_dv01_vec_[i]));
      inst_maxlots_[i] =
          std::max(1, (int)round(inst_unitlots_[i] * global_param_set_->inst_maxlots_ / global_param_set_->inst_uts_));
      inst_worstcaselots_[i] = std::max(
          1, (int)round(inst_unitlots_[i] * global_param_set_->inst_worstcaselots_ / global_param_set_->inst_uts_));

      inst_increase_threshold_[i] =
          global_param_set_->increase_threshold_ * global_param_set_->inst_maxlots_ / inst_maxlots_[i];
      inst_decrease_threshold_[i] =
          global_param_set_->decrease_threshold_ * global_param_set_->inst_maxlots_ / inst_maxlots_[i];
      if (global_param_set_->read_base_threshold_vec_ && global_param_set_->base_threshold_vec_.size() > (unsigned)i) {
        inst_base_threshold_[i] = global_param_set_->base_threshold_vec_[i];
      } else {
        inst_base_threshold_[i] = global_param_set_->base_threshold_;
      }
    } else if (global_param_set_->use_custom_scaling_) {
      inst_worstcaselots_[i] =
          (int)round(global_param_set_->inst_worstcaselots_ * global_param_set_->custom_scaling_vec_[i]);
      inst_maxlots_[i] = (int)round(global_param_set_->inst_maxlots_ * global_param_set_->custom_scaling_vec_[i]);

      if (global_param_set_->read_inst_mur_vec_) {
        inst_unitlots_[i] = (int)round(inst_maxlots_[i] / global_param_set_->inst_mur_vec_[i]);
      } else {
        inst_unitlots_[i] = (int)round(global_param_set_->inst_uts_ * global_param_set_->custom_scaling_vec_[i]);
      }

      if (global_param_set_->read_base_threshold_vec_ && global_param_set_->base_threshold_vec_.size() > (unsigned)i) {
        inst_base_threshold_[i] = global_param_set_->base_threshold_vec_[i];
      } else {
        inst_base_threshold_[i] = global_param_set_->base_threshold_;
      }
      inst_increase_threshold_[i] =
          global_param_set_->increase_threshold_ * global_param_set_->inst_maxlots_ / inst_maxlots_[i];
      inst_decrease_threshold_[i] =
          global_param_set_->decrease_threshold_ * global_param_set_->inst_maxlots_ / inst_maxlots_[i];

    } else {
      inst_unitlots_[i] = global_param_set_->inst_uts_;
      inst_worstcaselots_[i] = global_param_set_->inst_worstcaselots_;
      inst_maxlots_[i] = global_param_set_->inst_maxlots_;
      if (global_param_set_->read_base_threshold_vec_ && global_param_set_->base_threshold_vec_.size() > (unsigned)i) {
        inst_base_threshold_[i] = global_param_set_->base_threshold_vec_[i];
      } else {
        inst_base_threshold_[i] = global_param_set_->base_threshold_;
      }
      inst_increase_threshold_[i] = global_param_set_->increase_threshold_;
      inst_decrease_threshold_[i] = global_param_set_->decrease_threshold_;
    }

    dbglogger_ << dep_market_view_vec_[i]->shortcode() << " IL " << inst_unitlots_[i] << " IM " << inst_maxlots_[i]
               << " BT " << inst_base_threshold_[i] << " IT " << inst_increase_threshold_[i] << " DT "
               << inst_decrease_threshold_[i] << '\n';
  }

  SetBuySellThresholds();
}

void IndexFuturesMeanRevertingTrading::ResetUnitSizes() {
  for (int i = 0; i < num_total_products_; i++) {
    current_dv01_vec_[i] =
        HFSAT::CurveUtils::dv01(dep_market_view_vec_[i]->shortcode(), watch_.YYYYMMDD(), last_inst_prices_[i]);
    inst_unitlots_[i] =
        std::max(1, (int)round(current_dv01_vec_[0] * global_param_set_->inst_uts_ / current_dv01_vec_[i]));
    inst_maxlots_[i] =
        std::max(1, (int)round(inst_unitlots_[i] * global_param_set_->inst_maxlots_ / global_param_set_->inst_uts_));
    inst_worstcaselots_[i] = std::max(
        1, (int)round(inst_unitlots_[i] * global_param_set_->inst_worstcaselots_ / global_param_set_->inst_uts_));

    inst_increase_threshold_[i] =
        global_param_set_->increase_threshold_ * global_param_set_->inst_maxlots_ / inst_maxlots_[i];
    inst_decrease_threshold_[i] =
        global_param_set_->decrease_threshold_ * global_param_set_->inst_maxlots_ / inst_maxlots_[i];
  }
}

void IndexFuturesMeanRevertingTrading::Initialize() {
  global_min_pnl_ = 0;  // initialize global minimum PNL

  product_position_in_lots_.resize(num_total_products_, 0);
  if (livetrading_) {
    should_be_getting_flat_.resize(num_total_products_, true);
  } else {
    should_be_getting_flat_.resize(num_total_products_, false);
  }

  int sampling_interval_msecs_ =
      HFSAT::ExecLogicUtils::GetSamplingIntervalForPnlSeries(dep_market_view_vec_[0]->shortcode());

  int t_sampling_start_utc_mfm_ = MathUtils::GetFlooredMultipleOf(trading_start_utc_mfm_, sampling_interval_msecs_);
  int t_sampling_end_utc_mfm_ = MathUtils::GetCeilMultipleOf(
      trading_end_utc_mfm_ + 60000, sampling_interval_msecs_);  // Adding 1 min to incorporate getflat also

  for (int sampling_mfm_ = t_sampling_start_utc_mfm_ + sampling_interval_msecs_;
       sampling_mfm_ <= t_sampling_end_utc_mfm_; sampling_mfm_ += sampling_interval_msecs_) {
    pnl_sampling_timestamps_.push_back(sampling_mfm_);
  }

  bid_int_price_to_place_at_.resize(num_total_products_, INVALID_MEAN_REVERTING_ORDER_PRICE);
  ask_int_price_to_place_at_.resize(num_total_products_, INVALID_MEAN_REVERTING_ORDER_PRICE);
  bid_int_price_to_keep_at_.resize(num_total_products_, INVALID_MEAN_REVERTING_ORDER_PRICE);
  ask_int_price_to_keep_at_.resize(num_total_products_, INVALID_MEAN_REVERTING_ORDER_PRICE);

  seconds_at_last_buy_.resize(num_total_products_, 0);
  seconds_at_last_sell_.resize(num_total_products_, 0);

  book_indicator_values_.resize(num_total_products_, 0);
  trade_indicator_values_.resize(num_total_products_, 0);

  for (int i = 0; i < num_total_sources_; i++) {
    all_market_view_vec_[i]->subscribe_price_type(this, kPriceTypeMidprice);
  }

  for (int i = 0; i < num_total_products_; i++) {
    shortcode_vec_.push_back(dep_market_view_vec_[i]->shortcode());
  }

  for (int i = 0; i < num_total_products_; i++) {
    secname_vec_.push_back(dep_market_view_vec_[i]->secname());
  }

  // we create these only for non-index constituents
  residuals_.resize(num_total_products_);
  residual_sum_.resize(num_total_products_);
  residual_sumsqr_.resize(num_total_products_);
  stdev_residuals_.resize(num_total_products_);
  support_lvl_buffer_.resize(num_total_products_);
  inst_prices_.resize(num_total_products_);
  port_prices_.resize(num_total_products_);
  inst_betas_.resize(num_total_products_);
  online_price_update_count_.resize(num_total_products_, 0);
  last_port_prices_.resize(num_total_products_);
  for (int i = 0; i < num_total_products_; i++) {
    residuals_[i].clear();
    inst_prices_[i].clear();
    port_prices_[i].clear();
  }
  msecs_at_last_vec_processing_ = 0;

  last_inst_prices_.resize(num_total_sources_);
  predictor_vec_.resize(num_total_sources_, std::vector<std::pair<int, double>>());
  dont_trade_.resize(num_total_products_, false);

  inst_return_vol_.resize(num_total_products_, 1.0);
  ReadHistValues();
  AddIndicatorListener();

  //  trend_adjust_bid_threshold_.resize(num_total_products_, 0.0);
  //  trend_adjust_ask_threshold_.resize(num_total_products_, 0.0);

  // set inst specific parameters
  inst_unitlots_.resize(num_total_products_);
  inst_maxlots_.resize(num_total_products_);
  inst_worstcaselots_.resize(num_total_products_);
  inst_base_threshold_.resize(num_total_products_);
  inst_increase_threshold_.resize(num_total_products_);
  inst_decrease_threshold_.resize(num_total_products_);
  buy_thresholds_.resize(num_total_products_);
  sell_thresholds_.resize(num_total_products_);
  for (int i = 0; i < num_total_products_; i++) {
    buy_thresholds_[i].clear();
    sell_thresholds_[i].clear();
  }

  BuildThresholds();

  getflat_due_to_economic_times_ = false;
  applicable_severity_ = 0;
  last_allowed_event_index_ = 0;
  getflat_due_to_allowed_economic_event_ = false;
  ComputeCurrentSeverity();

  prev_ask_order_lvl_.resize(num_total_products_);
  asklvls_tstamp_.resize(num_total_products_);
  asklvls_from_best_.resize(num_total_products_);

  prev_bid_order_lvl_.resize(num_total_products_);
  bidlvls_tstamp_.resize(num_total_products_);
  bidlvls_from_best_.resize(num_total_products_);
  for (int i = 0; i < num_total_products_; i++) {
    asklvls_tstamp_[i].clear();
    asklvls_from_best_[i].clear();
    bidlvls_tstamp_[i].clear();
    bidlvls_from_best_[i].clear();

    prev_ask_order_lvl_[i] = -1;
    prev_bid_order_lvl_[i] = -1;
  }
}

void IndexFuturesMeanRevertingTrading::ReadHistValues() {
  char t_filename_[1024];
  sprintf(t_filename_, "%s%d", (global_param_set_->histfile_prefix_).c_str(), watch_.YYYYMMDD());
  std::ifstream histfile_;
  histfile_.open(t_filename_);

  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  if (histfile_.is_open()) {
    const int kParamfileListBufflerLen = 51200;
    char readlinebuffer_[kParamfileListBufflerLen];
    bzero(readlinebuffer_, kParamfileListBufflerLen);
    int t_stk_id_ = 0;
    while (histfile_.good()) {
      bzero(readlinebuffer_, kParamfileListBufflerLen);
      histfile_.getline(readlinebuffer_, kParamfileListBufflerLen);
      std::string this_line_ = std::string(readlinebuffer_);
      PerishableStringTokenizer st_(readlinebuffer_, kParamfileListBufflerLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() == 0) {
        continue;
      }
      if (strcmp(tokens_[0], "Port:") == 0) {
        // format of this line will be target_stock num_predictors pred_stock_1 weight_1 .... pred_stock_n weight_n
        t_stk_id_ = sec_name_indexer_.GetIdFromString(tokens_[1]);
        if (t_stk_id_ < 0) {
          // data for a stock which is not in our trading config  .. continue
          continue;

        } else {
          unsigned int t_num_preds_ = atoi(tokens_[2]);
          if (tokens_.size() != 3 + 2 * t_num_preds_) {
            std::cerr << "Malformed first line of stock data in hist file .. exiting \n";
            exit(-1);
          }
          for (unsigned int t_ctr_ = 0; t_ctr_ < t_num_preds_; t_ctr_++) {
            int t_pred_id_ = sec_name_indexer_.GetIdFromString(tokens_[3 + 2 * t_ctr_]);
            double t_weight_ = atof(tokens_[4 + 2 * t_ctr_]);
            if (t_pred_id_ < 0) {
              std::cerr << " Predictor " << tokens_[3 + 2 * t_ctr_] << " not present in strat config .. exiting \n";
              exit(-1);
            }
            std::pair<int, double> pred_entry_(t_stk_id_, t_weight_);
            predictor_vec_[t_pred_id_].push_back(pred_entry_);
          }
        }

      } else if (strcmp(tokens_[0], "HIST_PRICES") == 0 &&
                 tokens_.size() == (2 * global_param_set_->hist_price_length_ + 1) && t_stk_id_ >= 0) {
        // format of line is stk_px{t-1} port_px{t-1} ... stk_px{0} port_px{0}
        for (unsigned int t_ctr_ = 0; t_ctr_ < global_param_set_->hist_price_length_; t_ctr_++) {
          inst_prices_[t_stk_id_].push_back(atof(tokens_[t_ctr_ * 2 + 1]));
          port_prices_[t_stk_id_].push_back(atof(tokens_[t_ctr_ * 2 + 2]));
        }
        inst_betas_[t_stk_id_] = std::inner_product(inst_prices_[t_stk_id_].begin(), inst_prices_[t_stk_id_].end(),
                                                    port_prices_[t_stk_id_].begin(), 0.0) /
                                 std::inner_product(port_prices_[t_stk_id_].begin(), port_prices_[t_stk_id_].end(),
                                                    port_prices_[t_stk_id_].begin(), 0.0);
        last_port_prices_[t_stk_id_] = atof(tokens_[2 * global_param_set_->hist_price_length_]);

        dbglogger_ << "Instrument: " << dep_market_view_vec_[t_stk_id_]->shortcode() << " beta "
                   << inst_betas_[t_stk_id_] << " Pxs " << last_port_prices_[t_stk_id_] << '\n';

      } else if ((strcmp(tokens_[0], "HIST_ERROR") == 0) &&
                 (tokens_.size() == global_param_set_->hist_error_length_ + 1) && (t_stk_id_ >= 0)) {
        for (unsigned int t_ctr_ = 0; t_ctr_ < global_param_set_->hist_error_length_; t_ctr_++) {
          residuals_[t_stk_id_].push_back(atof(tokens_[t_ctr_ + 1]));
        }
        residual_sum_[t_stk_id_] = std::accumulate(residuals_[t_stk_id_].begin(), residuals_[t_stk_id_].end(), 0.0);
        residual_sumsqr_[t_stk_id_] = std::inner_product(residuals_[t_stk_id_].begin(), residuals_[t_stk_id_].end(),
                                                         residuals_[t_stk_id_].begin(), 0.0);
        stdev_residuals_[t_stk_id_] = sqrt(residual_sumsqr_[t_stk_id_] / global_param_set_->hist_error_length_ -
                                           residual_sum_[t_stk_id_] / global_param_set_->hist_error_length_ *
                                               residual_sum_[t_stk_id_] / global_param_set_->hist_error_length_);

        dbglogger_ << "Instrument: " << dep_market_view_vec_[t_stk_id_]->shortcode() << " last error "
                   << residuals_[t_stk_id_][global_param_set_->hist_error_length_ - 1] << " stdev "
                   << stdev_residuals_[t_stk_id_] << '\n';

      } else if (strcmp(tokens_[0], "LAST_PRICE") == 0) {
        // format is LAST_PRICE INST1 PX1 .. INSTN PXN
        for (unsigned int t_ctr_ = 1; t_ctr_ < tokens_.size(); t_ctr_ = t_ctr_ + 2) {
          int t_sec_id_ = sec_name_indexer_.GetIdFromString(tokens_[t_ctr_]);
          if (t_sec_id_ >= 0) {
            last_inst_prices_[t_sec_id_] = atof(tokens_[t_ctr_ + 1]);
          }
        }

      } else if (strcmp(tokens_[0], "INST_VOLATILITY") == 0) {
        // format is INST_VOLATILITY INST1 VOL1 .. INSTN VOLN
        // values are normalized for banknifty vol being 1
        for (unsigned int t_ctr_ = 1; t_ctr_ < tokens_.size(); t_ctr_ = t_ctr_ + 2) {
          int t_sec_id_ = sec_name_indexer_.GetIdFromString(tokens_[t_ctr_]);
          if (t_sec_id_ >= 0) {
            inst_return_vol_[t_sec_id_] = atof(tokens_[t_ctr_ + 1]);
          }
        }
      } else if (strcmp(tokens_[0], "INSAMPLE_CORR") == 0 || strcmp(tokens_[0], "STATS") == 0 ||
                 strcmp(tokens_[0], "ZERO_CROSSING") == 0 || strcmp(tokens_[0], "TIME_SIGNAL_AVG") == 0) {
      } else if (t_stk_id_ >= 0 && tokens_.size() > 0) {
        std::cerr << "Error - hist file format incorrect " << tokens_[0] << " " << tokens_.size() << '\n';
      }
    }
  } else {
    ExitVerbose(kExitErrorCodeGeneral, "HistFile Does not exist!!");
  }
  histfile_.close();

  // disable trading in products which are not specified in hist file
  for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
    if (residuals_[t_ctr_].size() != global_param_set_->hist_error_length_ ||
        inst_prices_[t_ctr_].size() != global_param_set_->hist_price_length_) {
      dont_trade_[t_ctr_] = true;
    }
  }

  for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
    support_lvl_buffer_[t_ctr_] = global_param_set_->change_threshold_;
    dbglogger_ << "Instrument: " << dep_market_view_vec_[t_ctr_]->shortcode() << " Support_lvl_ticks "
               << support_lvl_buffer_[t_ctr_] << '\n';
  }
  dbglogger_ << '\n';

  // validate portfolio constituents -- debug mode
  for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
    DumpPortfolioConstituents(t_ctr_);
  }
}

void IndexFuturesMeanRevertingTrading::DumpPortfolioConstituents(int sec_id) {
  DBGLOG_TIME_CLASS_FUNC << " Instrument " << shortcode_vec_[sec_id] << " Last port px " << last_port_prices_[sec_id]
                         << DBGLOG_ENDL_FLUSH;

  double t_comp_px_ = 0;
  for (int t_ctr_ = 0; t_ctr_ < num_total_sources_; t_ctr_++) {
    std::vector<std::pair<int, double>>::iterator t_iter_;
    for (t_iter_ = predictor_vec_[t_ctr_].begin(); t_iter_ != predictor_vec_[t_ctr_].end(); t_iter_++) {
      if ((*t_iter_).first == sec_id) {
        dbglogger_ << " Port constituent " << all_market_view_vec_[t_ctr_]->shortcode()
                   << " weight:" << (*t_iter_).second << " inst_px:" << last_inst_prices_[t_ctr_] << '\n';
        t_comp_px_ = t_comp_px_ + (*t_iter_).second * last_inst_prices_[t_ctr_];
      }
    }
  }
  dbglogger_ << "Computed port px " << t_comp_px_ << "\n\n";
}

void IndexFuturesMeanRevertingTrading::PlaceAndCancelOrders() {
  // iterate sequentially for all products
  for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
    // buy order handling
    if (bid_int_price_to_keep_at_[t_ctr_] == INVALID_MEAN_REVERTING_ORDER_PRICE) {
      order_manager_vec_[t_ctr_]->CancelAllBidOrders();
    } else {
      PlaceSingleBuyOrder(t_ctr_, bid_int_price_to_place_at_[t_ctr_], bid_int_price_to_keep_at_[t_ctr_]);
    }

    // ask order handling
    if (ask_int_price_to_keep_at_[t_ctr_] == INVALID_MEAN_REVERTING_ORDER_PRICE) {
      order_manager_vec_[t_ctr_]->CancelAllAskOrders();
    } else {
      PlaceSingleSellOrder(t_ctr_, ask_int_price_to_place_at_[t_ctr_], ask_int_price_to_keep_at_[t_ctr_]);
    }
  }
}

void IndexFuturesMeanRevertingTrading::PlaceSingleBuyOrder(int index, int int_order_px_, int int_keep_px_) {
  HFSAT::SmartOrderManager* t_om_ = order_manager_vec_[index];

  int int_placed_order_px_ = int_order_px_;
  // if any orders present b/w order_px_ and keep_px_, set keep_px_ to that order
  if (int_keep_px_ != int_order_px_) {
    for (int t_price_ = int_order_px_ + 1; t_price_ <= int_keep_px_; t_price_++) {
      if (t_om_->GetTotalBidSizeOrderedAtIntPx(t_price_) > 0) {
        int_placed_order_px_ = t_price_;
        break;
      }
    }
  }

  // cancel all orders above the keep price
  int cancelled_size_ = t_om_->CancelBidsAboveIntPrice(int_placed_order_px_);
  if (cancelled_size_ > 0) {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " Cancelled EqAbove orders for " << shortcode_vec_[index] << ' ' << cancelled_size_
                             << " @ " << int_placed_order_px_ << " best bid "
                             << dep_market_view_vec_[index]->bestbid_int_price() << " Port_Px "
                             << last_port_prices_[index] << " Beta " << inst_betas_[index] << " Stdev "
                             << stdev_residuals_[index] << " Pos " << product_position_in_lots_[index]
                             << DBGLOG_ENDL_FLUSH;
    }
  }

  // return if no order to place
  if (int_order_px_ == INVALID_MEAN_REVERTING_ORDER_PRICE) {
    return;
  }

  double t_gpos = my_portfolio_risk();
  double global_max_pos = global_param_set_->portfolio_maxlots_;
  int max_position_can_be_taken = floor((global_max_pos - t_gpos) * inst_unitlots_[index]);

  // set order_size
  int t_order_size_to_place_ = 0;

  if (should_be_getting_flat_[index] || getflat_due_to_economic_times_) {
    t_order_size_to_place_ =
        std::max(0,
                 std::min({inst_unitlots_[index], -1 * product_position_in_lots_[index], max_position_can_be_taken})) *
        dep_market_view_vec_[index]->min_order_size();
  } else {
    t_order_size_to_place_ =
        std::max(0, std::min({inst_unitlots_[index], inst_maxlots_[index] - product_position_in_lots_[index],
                              max_position_can_be_taken})) *
        dep_market_view_vec_[index]->min_order_size();
  }

  int t_order_size_placed_ = 0;

  if (t_om_->GetTotalBidSizeEqAboveIntPx(int_placed_order_px_) == 0 && t_order_size_to_place_ > 0) {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " Send Buy Order for " << shortcode_vec_[index] << ' ' << t_order_size_to_place_
                             << " @ " << int_order_px_ << " best bid "
                             << dep_market_view_vec_[index]->bestbid_int_price() << " Port_Px "
                             << last_port_prices_[index] << " Beta " << inst_betas_[index] << " Stdev "
                             << stdev_residuals_[index] << " Pos " << product_position_in_lots_[index]
                             << DBGLOG_ENDL_FLUSH;
    }
    t_om_->SendTradeIntPx(int_order_px_, t_order_size_to_place_, HFSAT::kTradeTypeBuy, 'B', HFSAT::kOrderDay);
    t_order_size_placed_ += t_order_size_to_place_;
  }

  /*  Placing and Cancelling of Supporting orders:
   *  If getflat: cancel all supporting orders
   *  Else:
   *  If place_supporting_orders_ is true, place orders till support_lvl_buffer_
   *  If worst_case == 0, cancel all supporting orders
   *  Elseif worst_case > 0, cancel supporting orders in accordance with worst case
   *  Else (worst_case < 0), cancel all orders below support_lvl_buffer_ */
  cancelled_size_ = 0;
  if (!(should_be_getting_flat_[index] || getflat_due_to_economic_times_)) {
    int _bestbid_int_price_ = dep_market_view_vec_[index]->bestbid_int_price();

    if (global_param_set_->place_supporting_orders_ && support_lvl_buffer_[index] > 0) {
      for (int t_int_price_ = int_order_px_ - 1; t_int_price_ >= _bestbid_int_price_ - support_lvl_buffer_[index];
           t_int_price_--) {
        if (t_om_->GetTotalBidSizeOrderedAtIntPx(t_int_price_) == 0) {  // if no orders at this level
          int t_order_size_ =
              std::max(0, std::min(inst_unitlots_[index],
                                   inst_maxlots_[index] - (product_position_in_lots_[index] + t_order_size_placed_))) *
              dep_market_view_vec_[index]->min_order_size();
          t_om_->SendTradeIntPx(t_int_price_, t_order_size_, HFSAT::kTradeTypeBuy, 'B', HFSAT::kOrderDay);
          t_order_size_placed_ += t_order_size_;
        }
      }
    }

    if (inst_worstcaselots_[index] == 0) {
      cancelled_size_ =
          t_om_->CancelBidsBelowIntPrice(int_order_px_);  // _bestbid_int_price_ - support_lvl_buffer_[index]
    } else if (inst_worstcaselots_[index] > 0) {
      int size_to_be_cancelled_ = product_position_in_lots_[index] + t_om_->SumBidSizes() -
                                  std::max(inst_maxlots_[index], inst_worstcaselots_[index]);
      if (size_to_be_cancelled_ > 0) {
        cancelled_size_ = t_om_->CancelBidsFromFar(size_to_be_cancelled_);
      }
    } else {
      cancelled_size_ = t_om_->CancelBidsBelowIntPrice(_bestbid_int_price_ - support_lvl_buffer_[index]);
    }
  } else {
    cancelled_size_ = t_om_->CancelBidsBelowIntPrice(int_order_px_);
  }

  if (cancelled_size_ > 0) {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " Cancelled EqBelow orders for " << shortcode_vec_[index] << ' ' << cancelled_size_
                             << " @ " << int_order_px_ << " best bid "
                             << dep_market_view_vec_[index]->bestbid_int_price() << " Port_Px "
                             << last_port_prices_[index] << " Beta " << inst_betas_[index] << " Stdev "
                             << stdev_residuals_[index] << " Pos " << product_position_in_lots_[index]
                             << DBGLOG_ENDL_FLUSH;
    }
  }
}

void IndexFuturesMeanRevertingTrading::PlaceSingleSellOrder(int index, int int_order_px_, int int_keep_px_) {
  HFSAT::SmartOrderManager* t_om_ = order_manager_vec_[index];

  int int_placed_order_px_ = int_order_px_;
  // if any orders present b/w order_px_ and keep_px_, set keep_px_ to that order
  if (int_keep_px_ != int_order_px_) {
    for (int t_price_ = int_order_px_ - 1; t_price_ >= int_keep_px_; t_price_--) {
      if (t_om_->GetTotalAskSizeOrderedAtIntPx(t_price_) > 0) {
        int_placed_order_px_ = t_price_;
        break;
      }
    }
  }

  // cancel all orders above the keep price
  int cancelled_size_ = t_om_->CancelAsksAboveIntPrice(int_placed_order_px_);
  if (cancelled_size_ > 0) {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " Cancelled EqAbove orders for " << shortcode_vec_[index] << ' ' << cancelled_size_
                             << " @ " << int_placed_order_px_ << " best ask "
                             << dep_market_view_vec_[index]->bestask_int_price() << " Port_Px "
                             << last_port_prices_[index] << " Beta " << inst_betas_[index] << " Stdev "
                             << stdev_residuals_[index] << " Pos " << product_position_in_lots_[index]
                             << DBGLOG_ENDL_FLUSH;
    }
  }

  // return if no order to place
  if (int_order_px_ == INVALID_MEAN_REVERTING_ORDER_PRICE) {
    return;
  }

  double t_gpos = my_portfolio_risk();
  double global_max_pos = global_param_set_->portfolio_maxlots_;
  int max_position_can_be_taken = floor((global_max_pos + t_gpos) * inst_unitlots_[index]);

  // set order_size
  int t_order_size_to_place_ = 0;

  if (should_be_getting_flat_[index] || getflat_due_to_economic_times_) {
    t_order_size_to_place_ =
        std::max(0, std::min({inst_unitlots_[index], product_position_in_lots_[index], max_position_can_be_taken})) *
        dep_market_view_vec_[index]->min_order_size();
  } else {
    t_order_size_to_place_ =
        std::max(0, std::min({inst_unitlots_[index], inst_maxlots_[index] + product_position_in_lots_[index],
                              max_position_can_be_taken})) *
        dep_market_view_vec_[index]->min_order_size();
  }

  int t_order_size_placed_ = 0;

  if (t_om_->GetTotalAskSizeEqAboveIntPx(int_placed_order_px_) == 0 && t_order_size_to_place_ > 0) {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " Send Sell Order for " << shortcode_vec_[index] << ' ' << t_order_size_to_place_
                             << " @ " << int_order_px_ << " best ask "
                             << dep_market_view_vec_[index]->bestask_int_price() << " Port_Px "
                             << last_port_prices_[index] << " Beta " << inst_betas_[index] << " Stdev "
                             << stdev_residuals_[index] << " Pos " << product_position_in_lots_[index]
                             << DBGLOG_ENDL_FLUSH;
    }
    t_om_->SendTradeIntPx(int_order_px_, t_order_size_to_place_, HFSAT::kTradeTypeSell, 'B', HFSAT::kOrderDay);
    t_order_size_placed_ += t_order_size_to_place_;
  }

  /*  Placing and Cancelling of Supporting orders:
   *  If getflat: cancel all supporting orders
   *  Else:
   *  If place_supporting_orders_ is true, place orders till support_lvl_buffer_
   *  If worst_case == 0, cancel all supporting orders
   *  Elseif worst_case > 0, cancel supporting orders in accordance with worst case
   *  Else (worst_case < 0), cancel all orders below support_lvl_buffer_ */
  cancelled_size_ = 0;
  if (!(should_be_getting_flat_[index] || getflat_due_to_economic_times_)) {
    int _bestask_int_price_ = dep_market_view_vec_[index]->bestask_int_price();

    if (global_param_set_->place_supporting_orders_ && support_lvl_buffer_[index] > 0) {
      for (int t_int_price_ = int_order_px_ + 1; t_int_price_ <= _bestask_int_price_ + support_lvl_buffer_[index];
           t_int_price_++) {
        if (t_om_->GetTotalAskSizeOrderedAtIntPx(t_int_price_) == 0) {  // if no orders at this level
          int t_order_size_ =
              std::max(0, std::min(inst_unitlots_[index],
                                   inst_maxlots_[index] - (t_order_size_placed_ - product_position_in_lots_[index]))) *
              dep_market_view_vec_[index]->min_order_size();
          t_om_->SendTradeIntPx(t_int_price_, t_order_size_, HFSAT::kTradeTypeSell, 'B', HFSAT::kOrderDay);
          t_order_size_placed_ += t_order_size_;
        }
      }
    }

    if (inst_worstcaselots_[index] == 0) {
      cancelled_size_ =
          t_om_->CancelAsksBelowIntPrice(int_order_px_);  // _bestask_int_price_ + support_lvl_buffer_[index]
    } else if (inst_worstcaselots_[index] > 0) {
      int size_to_be_cancelled_ = -product_position_in_lots_[index] + t_om_->SumAskSizes() -
                                  std::max(inst_maxlots_[index], inst_worstcaselots_[index]);
      if (size_to_be_cancelled_ > 0) {
        cancelled_size_ = t_om_->CancelAsksFromFar(size_to_be_cancelled_);
      }
    } else {
      cancelled_size_ = t_om_->CancelAsksBelowIntPrice(_bestask_int_price_ + support_lvl_buffer_[index]);
    }
  } else {
    cancelled_size_ = t_om_->CancelAsksBelowIntPrice(int_order_px_);
  }

  if (cancelled_size_ > 0) {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " Cancelled EqBelow orders for " << shortcode_vec_[index] << ' ' << cancelled_size_
                             << " @ " << int_order_px_ << " best ask "
                             << dep_market_view_vec_[index]->bestask_int_price() << " Port_Px "
                             << last_port_prices_[index] << " Beta " << inst_betas_[index] << " Stdev "
                             << stdev_residuals_[index] << " Pos " << product_position_in_lots_[index]
                             << DBGLOG_ENDL_FLUSH;
    }
  }
}

/// Called in the following situations - ( i ) Trade happens in market ; (ii) L1 book
/// of some instrument changes; ( iii ) order gets executed.
///@Returns with the buy and sell side order price set.
void IndexFuturesMeanRevertingTrading::TradingLogic() {
  if (!is_ready_ || external_freeze_trading_) {
    return;
  }

  // keep aggregate position under constraint
  double t_gpos_ = my_portfolio_risk();
  bool disallow_long_trades_ = (t_gpos_ >= global_param_set_->portfolio_maxlots_ ? true : false);
  bool disallow_short_trades_ = (-1 * t_gpos_ >= global_param_set_->portfolio_maxlots_ ? true : false);

  // logic for setting price bands here ..
  for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
    if (!should_be_getting_flat_[t_ctr_] && !getflat_due_to_economic_times_) {
      // set bid price first
      if (dont_trade_[t_ctr_] || product_position_in_lots_[t_ctr_] >= inst_maxlots_[t_ctr_] || disallow_long_trades_ ||
          (global_param_set_->use_book_indicator_ && book_indicator_values_[t_ctr_] < 0) ||
          (global_param_set_->use_trade_indicator_ && trade_indicator_values_[t_ctr_] < 0) ||
          (watch_.msecs_from_midnight() / 1000 - seconds_at_last_buy_[t_ctr_] < global_param_set_->cooloff_secs_ &&
           product_position_in_lots_[t_ctr_] >= 0) ||
          (bid_int_price_to_keep_at_[t_ctr_] == INVALID_MEAN_REVERTING_ORDER_PRICE)) {
        bid_int_price_to_keep_at_[t_ctr_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
      } else {
        bid_int_price_to_keep_at_[t_ctr_] =
            std::min(dep_market_view_vec_[t_ctr_]->bestbid_int_price(), bid_int_price_to_keep_at_[t_ctr_]);
        bid_int_price_to_place_at_[t_ctr_] =
            std::min(dep_market_view_vec_[t_ctr_]->bestbid_int_price(), bid_int_price_to_place_at_[t_ctr_]);
      }
      // set ask price
      if (dont_trade_[t_ctr_] || product_position_in_lots_[t_ctr_] + inst_maxlots_[t_ctr_] <= 0 ||
          disallow_short_trades_ || (global_param_set_->use_book_indicator_ && book_indicator_values_[t_ctr_] > 0) ||
          (global_param_set_->use_trade_indicator_ && trade_indicator_values_[t_ctr_] > 0) ||
          (watch_.msecs_from_midnight() / 1000 - seconds_at_last_sell_[t_ctr_] < global_param_set_->cooloff_secs_ &&
           product_position_in_lots_[t_ctr_] <= 0) ||
          (ask_int_price_to_keep_at_[t_ctr_] == INVALID_MEAN_REVERTING_ORDER_PRICE)) {
        ask_int_price_to_keep_at_[t_ctr_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
      } else {
        ask_int_price_to_keep_at_[t_ctr_] =
            std::max(dep_market_view_vec_[t_ctr_]->bestask_int_price(), ask_int_price_to_keep_at_[t_ctr_]);
        ask_int_price_to_place_at_[t_ctr_] =
            std::max(dep_market_view_vec_[t_ctr_]->bestask_int_price(), ask_int_price_to_place_at_[t_ctr_]);
      }
    } else {
      // specific handling of getflat cases
      if (product_position_in_lots_[t_ctr_] == 0) {
        bid_int_price_to_keep_at_[t_ctr_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
        ask_int_price_to_keep_at_[t_ctr_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
      } else if (product_position_in_lots_[t_ctr_] > 0) {
        bid_int_price_to_keep_at_[t_ctr_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
        ask_int_price_to_keep_at_[t_ctr_] = dep_market_view_vec_[t_ctr_]->bestask_int_price();
        ask_int_price_to_place_at_[t_ctr_] = dep_market_view_vec_[t_ctr_]->bestask_int_price();
      } else {
        ask_int_price_to_keep_at_[t_ctr_] = INVALID_MEAN_REVERTING_ORDER_PRICE;
        bid_int_price_to_keep_at_[t_ctr_] = dep_market_view_vec_[t_ctr_]->bestbid_int_price();
        bid_int_price_to_place_at_[t_ctr_] = dep_market_view_vec_[t_ctr_]->bestbid_int_price();
      }
    }
  }
  PlaceAndCancelOrders();
}

void IndexFuturesMeanRevertingTrading::GetFlatTradingLogic(int _product_index_) {
  int thisproduct_positions_ = product_position_in_lots_[_product_index_];
  if (thisproduct_positions_ == 0) {
    order_manager_vec_[_product_index_]->CancelAllOrders();
    should_be_getting_flat_[_product_index_] = true;
  } else if (thisproduct_positions_ > 0) {
    order_manager_vec_[_product_index_]->CancelAllBidOrders();
    should_be_getting_flat_[_product_index_] = true;
  } else if (thisproduct_positions_ < 0) {
    order_manager_vec_[_product_index_]->CancelAllAskOrders();
    should_be_getting_flat_[_product_index_] = true;
  }
}

void IndexFuturesMeanRevertingTrading::GetAllFlat() {
  for (int i = 0; i < num_total_products_; i++) {
    GetFlatTradingLogic(i);
  }
}
}
