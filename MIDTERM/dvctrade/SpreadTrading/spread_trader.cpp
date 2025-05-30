#include <numeric>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include "dvctrade/SpreadTrading/spread_trader.hpp"

// these should have no effect on strategy performance numbers
#define STALE_DATA_THRESHOLD \
  100  // in seconds - we process rollover or trade only when all the four(two) datapoints have been snapped within this
       // interval
#define NUM_SECONDS_IN_MIN 60
#define AVG_FUT_COMMISH .0003  // includes txn cost of .00007651; rest being slippage
#define EXPIRY_CHECK_INTERVAL 10000
#define EOD_SECS_JUMP 25000
#define MARKET_DATA_GRANULARITY 1  // bardata length in minutes
#define STDEV_COMP_INTERVAL 60     // minutes
#define STDEV_COMP_LEN 2500        // minutes
// days determining getflat around earnings - can be pushed to param later ( serialized )
#define LONG_PERIOD_TO_EARNINGS 100  // days
#define MIN_DAYS_TO_EARNINGS 1
#define MIN_DAYS_FROM_EARNINGS 2

// these have effect on strategy performance numbers
#define SPREAD_AVERAGING_DURATION 5

namespace MT_SPRD {

SpreadTrader::SpreadTrader(ParamSet* t_param_, ParamSet* t_param_next_month_, HFSAT::DebugLogger& t_debuglogger_,
                           SpreadExecLogic* t_sprd_exec_fm_, SpreadExecLogic* t_sprd_exec_nm_, bool t_pass_leg_first_,
                           bool t_use_adjusted_, time_t t_trading_start_time, bool t_live, bool t_ban)
    : spread_exec_front_month_(t_sprd_exec_fm_),
      spread_exec_next_month_(t_sprd_exec_nm_),
      spread_exec_(t_sprd_exec_fm_),
      is_pass_leg_first_(t_pass_leg_first_),
      dbglogger_(t_debuglogger_) {
  // initialize vars
  watch_ = NULL;
  inst_1_front_month_sid_ = 0;
  inst_1_next_month_sid_ = 0;
  inst_2_front_month_sid_ = 0;
  inst_2_next_month_sid_ = 0;
  expiry_date_yyyymmdd_ = 0;

  unit_size_position_ = 0;
  current_drawdown_ = 0;
  highest_nav_so_far_ = t_param_->NAV_;
  max_drawdown_ = 0;

  spread_values_.clear();
  spread_value_snaptimes_.clear();
  current_spread_ = 0;
  ready_to_trade_ = false;

  traded_spread_difference_.clear();
  sum_x_ = 0;
  sum_x2_ = 0;
  diff_stdev_ = 0;
  stdev_ready_ = false;

  hist_prices_1_.clear();
  hist_prices_2_.clear();
  sum_prices_1_ = 0.0;
  sum_prices_2_ = 0.0;
  sum_prod_prices_ = 0.0;
  sum_prices1_sqr_ = 0.0;
  sum_prices2_sqr_ = 0.0;

  hist_logpx_1_.clear();
  hist_logpx_2_.clear();
  sum_logpx_1_ = 0.0;
  sum_logpx_2_ = 0.0;
  sum_prod_logpx_ = 0.0;
  sum_logpx2_sqr_ = 0.0;

  beta_ = 1;
  intercept_ = 0;

  last_opentrade_type_ = 'U';
  last_exec_type_ = 'U';
  last_exec_entry_time_ = 0;
  last_trade_stopped_out_ = false;

  last_mkt_trade_time_1_0_ = 0;
  last_mkt_trade_time_1_1_ = 0;
  last_mkt_trade_time_2_0_ = 0;
  last_mkt_trade_time_2_1_ = 0;
  last_close_px_1_0_ = 0;
  last_close_px_1_1_ = 0;
  last_close_px_2_0_ = 0;
  last_close_px_2_1_ = 0;
  last_bardata_time_ = 0;

  seen_today_px_1 = false;
  seen_today_px_2 = false;

  num_trades_ = 0;
  trade_returns_.clear();
  max_drawdown_ = 0;
  annualized_returns_ = 0;
  initial_nav_ = t_param_->NAV_;
  annualized_stdev_ = 0;

  is_expiry_day_ = false;
  has_rolled_over_ = false;
  roll_trade_done_ = false;
  last_expiry_check_tm_ = 0;

  param_ = t_param_;
  param_next_month_ = t_param_next_month_;

  is_banned = t_ban;

  open_trade_ = NULL;
  trade_history_.clear();

  time_history_.clear();

  kalman_ = NULL;
  kalman_x.resize(2);
  kalman_P.resize(4);
  kalman_Q.resize(4);
  kalman_R.resize(1);
  kalman_Y.resize(1);
  kalman_H.resize(2);

  use_adjusted_data_ = t_use_adjusted_;

  trading_start_time = t_trading_start_time;

  live = t_live;

  if (param_->spread_comp_mode_ == 0) {
    std::cerr << "Spread comp mode 0 no longer supported .. Exiting \n";
    exit(-1);
  }

  date_to_hlife_.clear();
  date_to_adf_.clear();
  trailing_ret_ = 0;
  curr_day_hlife_ = 0;
  curr_day_adf_ = 0;

  earnings_dates_indices_.clear();
  all_dates_indices_.clear();
  current_day_comparison_index_ = -1;

  zscore_ema_ = 0;
  zscore_at_last_getflat_ = 0;
}

// we load earnings dates from database. GetFlat is set if an earnings is close by.
void SpreadTrader::LoadEarningsMaps() {
  std::vector<int> t_alldates_;
  earnings_dates_indices_.clear();
  all_dates_indices_.clear();
  // Step 1. Create a map with indexes of all dates;
  sqlite3* dbconn;
  if (sqlite3_open_v2(DEF_DB, &dbconn, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
    std::cerr << "Could not open DB .. error " << sqlite3_errmsg(dbconn) << '\n';
    dbglogger_ << "Could not open DB .. error " << sqlite3_errmsg(dbconn) << '\n';
    exit(-1);
  }
  // Prepare statement extracting all dates
  char sql_stat[1024];
  sprintf(sql_stat, "select day from ALLDATES order by day asc");
  sqlite3_stmt* sql_prep_;
  if (sqlite3_prepare_v2(dbconn, sql_stat, -1, &sql_prep_, NULL) != SQLITE_OK) {
    std::cerr << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
    dbglogger_ << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
    exit(-1);
  }

  int t_index_ = 0;
  while (sqlite3_step(sql_prep_) == SQLITE_ROW) {
    all_dates_indices_[sqlite3_column_int(sql_prep_, 0)] = t_index_;
    t_alldates_.push_back(sqlite3_column_int(sql_prep_, 0));
    t_index_++;
  }

  // Step 2. Populate a sorted vector of indices of earnings dates. Current day is compared against this map/index
  // later.
  sprintf(sql_stat, "select DISTINCT day from EARNINGS_DATES where stock == \"%s\" or stock == \"%s\" order by day asc",
          (param_->instrument_1_).c_str(), (param_->instrument_2_).c_str());
  if (sqlite3_prepare_v2(dbconn, sql_stat, -1, &sql_prep_, NULL) != SQLITE_OK) {
    std::cerr << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
    dbglogger_ << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
    exit(-1);
  }

  while (sqlite3_step(sql_prep_) == SQLITE_ROW) {
    int t_curr_date_ = sqlite3_column_int(sql_prep_, 0);
    if (all_dates_indices_.find(t_curr_date_) != all_dates_indices_.end()) {
      earnings_dates_indices_.push_back(all_dates_indices_[t_curr_date_]);
    } else  // earnings date is on a weekend/holiday or a day absent in alldates ( eg missing logged data etc )
    {
      std::vector<int>::iterator t_up_ = std::upper_bound(t_alldates_.begin(), t_alldates_.end(), t_curr_date_);
      // Index of earnings day is t_up_ - t_alldates_.begin() ; we check for duplicates since different holidays can
      // have same business day succedding it.
      // edge cases ( earnings prior to first day of record or after last day of record ) are ignored
      if (t_up_ != t_alldates_.begin() && t_up_ != t_alldates_.end() &&
          (earnings_dates_indices_.size() == 0 ||
           earnings_dates_indices_[earnings_dates_indices_.size() - 1] != (t_up_ - t_alldates_.begin()))) {
        earnings_dates_indices_.push_back(t_up_ - t_alldates_.begin());
      }
    }
  }
  sqlite3_close(dbconn);
}

// Check for earliers earnings prior to and succedding current day. Call GetFlat is dates are in a
// certain range
void SpreadTrader::SetCurrentDayEarningsGetflat(int t_currdate_yyyymmdd_) {
  int t_num_days_to_ = LONG_PERIOD_TO_EARNINGS;
  int t_num_days_from_ = LONG_PERIOD_TO_EARNINGS;
  if (all_dates_indices_.find(t_currdate_yyyymmdd_) != all_dates_indices_.end()) {
    current_day_comparison_index_ = all_dates_indices_[t_currdate_yyyymmdd_];
    // find closest indices prior to and post this date
    std::vector<int>::iterator t_ubound_ =
        std::upper_bound(earnings_dates_indices_.begin(), earnings_dates_indices_.end(), current_day_comparison_index_);
    if (t_ubound_ != earnings_dates_indices_.end()) {
      t_num_days_to_ = (*t_ubound_ - current_day_comparison_index_);
    }
    if (t_ubound_ != earnings_dates_indices_.begin() && t_ubound_ != earnings_dates_indices_.end()) {
      t_ubound_--;
      t_num_days_from_ = (current_day_comparison_index_ - *t_ubound_);
    }
    if (t_num_days_to_ <= MIN_DAYS_TO_EARNINGS || t_num_days_from_ <= MIN_DAYS_FROM_EARNINGS) {
      param_->earnings_getflat_ = true;
      param_next_month_->earnings_getflat_ = true;
    } else {
      param_->earnings_getflat_ = false;
      param_next_month_->earnings_getflat_ = false;
    }
  } else  // default is false
  {
    param_->earnings_getflat_ = false;
    param_next_month_->earnings_getflat_ = false;
  }
  dbglogger_ << "Earnings getflat for " << t_currdate_yyyymmdd_ << " set to " << (param_->earnings_getflat_ ? 'Y' : 'N')
             << ' ' << t_num_days_from_ << ' ' << t_num_days_to_ << " for " << (param_->instrument_1_).c_str() << ':'
             << (param_->instrument_2_).c_str() << '\n';
  dbglogger_.DumpCurrentBuffer();
}

// needs to be called from main SpreadExec since date information is unavailable
void SpreadTrader::LoadStatMaps(int t_start_yyyymmdd_, int t_end_yyyymmdd_) {
  date_to_hlife_.clear();
  date_to_adf_.clear();
  // Step 1. Open DB
  sqlite3* dbconn;
  if (sqlite3_open_v2(DEF_DB, &dbconn, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
    std::cerr << "Could not open DB .. error " << sqlite3_errmsg(dbconn) << '\n';
    dbglogger_ << "Could not open DB .. error " << sqlite3_errmsg(dbconn) << '\n';
    exit(-1);
  }
  // Prepare statement and get adf values and halflife values
  char sql_stat[1024];
  sprintf(sql_stat,
          "select day, adf, halflife from PAIRS_ADF_STAT where day >=\"%d\" and day <= \"%d\" and stock1 == \"%s\" and "
          "stock2 == \"%s\"",
          t_start_yyyymmdd_, t_end_yyyymmdd_, (param_->instrument_1_).c_str(), (param_->instrument_2_).c_str());
  sqlite3_stmt* sql_prep_;
  if (sqlite3_prepare_v2(dbconn, sql_stat, -1, &sql_prep_, NULL) != SQLITE_OK) {
    std::cerr << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
    dbglogger_ << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
    exit(-1);
  }

  while (sqlite3_step(sql_prep_) == SQLITE_ROW) {
    date_to_hlife_[sqlite3_column_int(sql_prep_, 0)] = sqlite3_column_double(sql_prep_, 2);
    date_to_adf_[sqlite3_column_int(sql_prep_, 0)] = sqlite3_column_double(sql_prep_, 1);
  }
  sqlite3_close(dbconn);
}

SpreadExecLogic* SpreadTrader::GetFrontMonthExec() { return (spread_exec_front_month_); }

MT_SPRD::ParamSet SpreadTrader::GetParams() { return (*param_); }

MT_SPRD::ParamSet SpreadTrader::GetParams2() { return (*param_next_month_); }

void SpreadTrader::UpdateLotSizes(int inst1_fut0, int inst1_fut1, int inst2_fut0, int inst2_fut1) {
  param_->SetContractLotSizes(inst1_fut0, inst2_fut0);
  param_next_month_->SetContractLotSizes(inst1_fut1, inst2_fut1);
}

void SpreadTrader::UpdateFlat(bool is_flat) {
  param_->get_flat = is_flat;
  param_next_month_->get_flat = is_flat;
}

// this will enable overriding the uts
void SpreadTrader::SetUnitTradeSize(int t_uts1, int t_uts2) {
  param_->uts_set_externally_ = true;
  param_next_month_->uts_set_externally_ = true;

  if (t_uts1 == 0 || t_uts2 == 0) {
    param_->uts_set_externally_ = false;
    param_next_month_->uts_set_externally_ = false;
  }

  param_->orig_unit_trade_size_1_ = t_uts1;
  param_->orig_unit_trade_size_2_ = t_uts2;

  param_next_month_->orig_unit_trade_size_1_ = t_uts1;
  param_next_month_->orig_unit_trade_size_2_ = t_uts2;
}

void SpreadTrader::InitializeKalman(std::vector<double>& x_, std::vector<double>& P_, std::vector<double>& Q_,
                                    std::vector<double>& R_) {
  // setup vector A for kalman - I
  std::vector<double> A;
  A.push_back(1);
  A.push_back(0);
  A.push_back(0);
  A.push_back(1);

  // setup vector U for kalman - 0
  std::vector<double> U;
  U.push_back(0);

  // setup vector W for kalman - 2*2 I matrix
  std::vector<double> W;
  W.push_back(1);
  W.push_back(0);
  W.push_back(0);
  W.push_back(1);

  // setup vector V for kalman - 1*1 I matrix
  std::vector<double> V;
  V.push_back(1);

  // setup vector H for kalman
  std::vector<double> H;
  H.push_back(1);
  H.push_back(0);

  kalman_ = new Kalman::KalmanReg(2, 1, 2, 1, 1, dbglogger_);
  kalman_->setA(A);
  kalman_->setU(U);
  kalman_->setR(R_);
  kalman_->setW(W);
  kalman_->setV(V);
  kalman_->setH(H);
  kalman_->setQ(Q_);
  kalman_->init_Kalman(x_, P_);
}

void SpreadTrader::SaveKalmanState() {
  kalman_x[0] = kalman_->getKalmanParam(0);
  kalman_x[1] = kalman_->getKalmanParam(1);

  kalman_P[0] = kalman_->getKalmanP(0, 0);
  kalman_P[1] = kalman_->getKalmanP(0, 1);
  kalman_P[2] = kalman_->getKalmanP(1, 0);
  kalman_P[3] = kalman_->getKalmanP(1, 1);

  kalman_Q[0] = kalman_->getKalmanQ(0, 0);
  kalman_Q[1] = kalman_->getKalmanQ(0, 1);
  kalman_Q[2] = kalman_->getKalmanQ(1, 0);
  kalman_Q[3] = kalman_->getKalmanQ(1, 1);

  kalman_R[0] = kalman_->getKalmanR(0, 0);
}

void SpreadTrader::LoadKalmanState(std::vector<double>& x_, std::vector<double>& P_, std::vector<double>& Q_,
                                   std::vector<double>& R_) {
  x_ = kalman_x;
  P_ = kalman_P;
  Q_ = kalman_Q;
  R_ = kalman_R;
}

void SpreadTrader::SetHFTClasses(HFSAT::Watch* t_watch_, unsigned int t_inst1_fm_sid_, unsigned int t_inst1_nm_sid_,
                                 unsigned int t_inst2_fm_sid_, unsigned int t_inst2_nm_sid_, int t_exp_date_) {
  watch_ = t_watch_;
  inst_1_front_month_sid_ = t_inst1_fm_sid_;
  inst_1_next_month_sid_ = t_inst1_nm_sid_;
  inst_2_front_month_sid_ = t_inst2_fm_sid_;
  inst_2_next_month_sid_ = t_inst2_nm_sid_;
  expiry_date_yyyymmdd_ = t_exp_date_;
}

void SpreadTrader::TradeLogic() {
  if (!ready_to_trade_) return;

  if (param_->get_flat || param_->earnings_getflat_ || param_->zscore_getflat_) {
    if (open_trade_) {
      // first close open trade
      dbglogger_ << last_bardata_time_
                 << " Sprd_Trader::TradeLogic::get_flat is calling FillAndPopulateTradeRecords(0)\n";
      FillAndPopulateTradeRecords(0);
      unit_size_position_ = 0;
    }
    if (spread_exec_) {
      if (spread_exec_->HasPosition()) {
        spread_exec_->SetGetflatMode(true);
        spread_exec_->SetDesiredPositions(0, 0, 0, intercept_, beta_);
      }
    }
  }

  double t_current_spread_diff_ = current_spread_;
  double entry_threshold = (roll_trade_done_) ? param_->roll_reentry_threshold_ : param_->entry_spread_threshold_;

  // added checks for stable entry/exit
  if (t_current_spread_diff_ > 0 && unit_size_position_ == 0) {
    t_current_spread_diff_ = std::max(0.0, std::min(current_spread_, spread_values_[spread_values_.size() - 1]));
  } else if (t_current_spread_diff_ < 0 && unit_size_position_ == 0) {
    t_current_spread_diff_ = std::min(0.0, std::max(current_spread_, spread_values_[spread_values_.size() - 1]));
  }
  // get spread implied position
  int t_desired_position_ = 0;

  // Step 1: Compute ideal position
  if (fabs(t_current_spread_diff_) > entry_threshold && !param_->get_flat && !param_->earnings_getflat_ &&
      !param_->zscore_getflat_) {
    if (param_->max_unit_ratio_ > 1) {
      t_desired_position_ = std::min(
          param_->max_unit_ratio_,
          1 + (int)floor((fabs(t_current_spread_diff_) - entry_threshold) / param_->incremental_entry_threshold_));
      t_desired_position_ = (t_current_spread_diff_ > 0 ? t_desired_position_ : -1 * t_desired_position_);
    } else {
      t_desired_position_ = (t_current_spread_diff_ > 0 ? 1 : -1);
    }
  }

  // adjust ideal position in case of outstanding position and spread has not flipped over
  // sufficiently - incorporate effect of place keep diff
  if (unit_size_position_ != 0 && t_desired_position_ * unit_size_position_ >= 0 &&
      abs(t_desired_position_) < abs(unit_size_position_)) {
    double t_frac_thresh_ = 0;
    if (fabs(t_current_spread_diff_) <= entry_threshold || (param_->max_unit_ratio_ == 1)) {
      t_frac_thresh_ = (unit_size_position_ > 0 ? (entry_threshold - t_current_spread_diff_)
                                                : (entry_threshold + t_current_spread_diff_));
    } else {
      double t_frac_pos_ = (fabs(t_current_spread_diff_) - entry_threshold) / param_->incremental_entry_threshold_;
      t_frac_thresh_ = (ceil(t_frac_pos_) - t_frac_pos_) * param_->incremental_entry_threshold_;
    }

    if (t_frac_thresh_ < param_->entry_exit_differential_ && (abs(t_desired_position_) < param_->max_unit_ratio_)) {
      t_desired_position_ = (unit_size_position_ > 0 ? (t_desired_position_ + 1) : (t_desired_position_ - 1));
    }
  }
  // update spread values when using spread_exec_logic
  if (spread_exec_) {
    spread_exec_->UpdateSpreadParameters(intercept_, beta_);
  }

  // at this stage t_desired_position_ corresponds to the desired position
  if (t_desired_position_ != unit_size_position_ && IsTradeAllowed(t_desired_position_)) {
    if (spread_exec_) {
      dbglogger_ << last_bardata_time_ << " Sprd_Trader::Trading_logic " << t_desired_position_ << " : "
                 << unit_size_position_ << " Curr_Sprd_Diff " << t_current_spread_diff_ << " Thresh " << entry_threshold
                 << " --- Prices " << last_close_px_1_0_ << ':' << last_close_px_2_0_ << " spread " << current_spread_
                 << '\n';
    }

    if (unit_size_position_ == 0)  // slightly inefficient since size not recomputed in case of flip
    {
      //       param_->RecomputeLotSize( last_close_px_1_0_, last_close_px_2_0_, beta_,
      //       param_->target_vol_/ComputePnlVol() ); -- being called earlier as part of IsTradeAllowed
      // set hedge ratio of spread exec if we are using it
      if (spread_exec_ && is_pass_leg_first_) {
        spread_exec_->UpdateHedgeRatio(param_->unit_trade_size_2_ * 1.0 / param_->unit_trade_size_1_);
      } else if (spread_exec_ && !is_pass_leg_first_) {
        spread_exec_->UpdateHedgeRatio(param_->unit_trade_size_1_ * 1.0 / param_->unit_trade_size_2_);
      }
    }
    // update target spread and positions in spread_exec_logic
    if (spread_exec_) {
      int t_pass_leg_pos_ = (is_pass_leg_first_ ? t_desired_position_ * -1 * param_->unit_trade_size_1_
                                                : t_desired_position_ * param_->unit_trade_size_2_);
      int t_agg_leg_pos_ = (is_pass_leg_first_ ? t_desired_position_ * param_->unit_trade_size_2_
                                               : t_desired_position_ * -1 * param_->unit_trade_size_1_);

      if (t_desired_position_ > 0 && t_desired_position_ > unit_size_position_) {
        spread_exec_->SetDesiredPositions(
            t_pass_leg_pos_, t_agg_leg_pos_,
            entry_threshold + (t_desired_position_ - 1) * param_->incremental_entry_threshold_, intercept_, beta_);
      } else if (t_desired_position_ < 0 && t_desired_position_ < unit_size_position_) {
        spread_exec_->SetDesiredPositions(
            t_pass_leg_pos_, t_agg_leg_pos_,
            -entry_threshold + (t_desired_position_ + 1) * param_->incremental_entry_threshold_, intercept_, beta_);
      } else if (t_desired_position_ == 0) {
        spread_exec_->SetDesiredPositions(0, 0, param_->entry_spread_threshold_ - param_->entry_exit_differential_,
                                          intercept_, beta_);
      } else if (t_desired_position_ > 0)  // we've reduced our long position
      {
        spread_exec_->SetDesiredPositions(t_pass_leg_pos_, t_agg_leg_pos_,
                                          param_->entry_spread_threshold_ +
                                              t_desired_position_ * param_->incremental_entry_threshold_ -
                                              param_->entry_exit_differential_,
                                          intercept_, beta_);
      } else  // we've reduced our short position
      {
        spread_exec_->SetDesiredPositions(t_pass_leg_pos_, t_agg_leg_pos_,
                                          -param_->entry_spread_threshold_ +
                                              t_desired_position_ * param_->incremental_entry_threshold_ +
                                              param_->entry_exit_differential_,
                                          intercept_, beta_);
      }
    }

    // for flips we make two calls - NOTE - potentially execution inefficient - revisit later
    // TODO later - flips will cause problems in specific situations.
    if (t_desired_position_ * unit_size_position_ < 0) {
      CheckAndPlaceOrder(0);
    }

    CheckAndPlaceOrder(t_desired_position_);
  }
}

void SpreadTrader::HandleStop(bool is_stoploss_) {
  // TODO - put in support to close open position via spread_exec
  dbglogger_ << last_bardata_time_ << " Sprd_Trader::HandleStop is calling FillAndPopulateTradeRecords(0)\n";
  FillAndPopulateTradeRecords(0);
  spread_trade_record_t* last_trade_ = trade_history_[trade_history_.size() - 1];
  last_trade_->was_stopped_out_ = true;
  last_trade_stopped_out_ = true;
}

std::string SpreadTrader::GetFilename(std::string process)  // Function to get output filename to dump result and logs
{
  std::string filename;
  if (process == "NAV")
    filename = NSE_HFTRAP_NAV_SERIES + param_->instrument_1_ + "_" + param_->instrument_2_ + "_" + param_->param_id;
  else if (process == "TRADE")
    filename = NSE_HFTRAP_TRADE_FILE + param_->instrument_1_ + "_" + param_->instrument_2_ + "_" + param_->param_id;
  else
    filename = NSE_HFTRAP_DEBUG_LOGS + param_->instrument_1_ + "_" + param_->instrument_2_ + "_" + param_->param_id;

  if (live) filename += "_" + std::to_string(watch_->YYYYMMDD());

  return (filename);
}

void SpreadTrader::OnAllEventsConsumed() {
  if (open_trade_ != NULL) {
    dbglogger_ << last_bardata_time_ << " Sprd_Trader::OnAllEventsConsumed is calling FillAndPopulateTradeRecords(0)\n";
    FillAndPopulateTradeRecords(0);
    unit_size_position_ = 0;
  }
  // these are no longer required as the strategy updates NAV and Trade file real time.
  // DumpTradeStats();
  // DumpTimeStats();
  if (spread_exec_) {
    spread_exec_->OnAllEventsConsumed();
  }

  std::ofstream t_outfile_(GetFilename("DEBUG"), std::ios::out | std::ios::app);
  std::vector<int> long_points_;
  std::vector<int> short_points_;
  std::vector<double> nav_at_long_;
  std::vector<double> nav_at_short_;
  std::vector<double> px1_at_long_;
  std::vector<double> px1_at_short_;
  std::vector<double> px2_at_long_;
  std::vector<double> px2_at_short_;
  std::vector<double> sprd_at_long_;
  std::vector<double> sprd_at_short_;
  std::vector<double> zs_at_long_;
  std::vector<double> zs_at_short_;
  std::vector<double> beta_at_long_;
  std::vector<double> beta_at_short_;
  std::vector<double> int_at_long_;
  std::vector<double> int_at_short_;

  if (!trade_history_.empty()) {
    double t_run_duration_ = ((trade_history_[trade_history_.size() - 1])->trade_end_time_ -
                              (trade_history_[0])->trade_start_time_);  // in seconds
    annualized_returns_ = (pow(param_->NAV_ / initial_nav_, 365.0 * 86400 / t_run_duration_) - 1.0) * 100.0;
    num_trades_ = trade_history_.size();
    double t_sum_ = 0.0;
    double t_sumsqr_ = 0.0;
    size_t t_ctr_ = STDEV_COMP_INTERVAL;
    int t_numelem_ = 0;
    int t_poscount_ = 0;
    int t_last_pos_ = 0;
    for (; t_ctr_ < time_history_.size(); t_ctr_++) {
      double t_ret_ = (time_history_[t_ctr_]->nav_ / time_history_[t_ctr_ - STDEV_COMP_INTERVAL]->nav_ - 1.0);
      t_sum_ += t_ret_;
      t_sumsqr_ += t_ret_ * t_ret_;
      t_numelem_ += 1;
      if (time_history_[t_ctr_]->pos_ != 0) {
        t_poscount_++;
      }
      t_outfile_ << std::fixed << t_ctr_ << ' ' << time_history_[t_ctr_]->nav_ << ' ' << time_history_[t_ctr_]->px1_
                 << ' ' << time_history_[t_ctr_]->px2_ << ' ' << time_history_[t_ctr_]->curr_spread_ << ' '
                 << time_history_[t_ctr_]->z_score_ << ' ' << time_history_[t_ctr_]->beta_ << ' '
                 << time_history_[t_ctr_]->intercept_ << ' ' << time_history_[t_ctr_]->pnlvol_ << std::endl;

      if (time_history_[t_ctr_]->pos_ != t_last_pos_) {
        if (time_history_[t_ctr_]->pos_ > t_last_pos_) {
          long_points_.push_back(t_ctr_);
          nav_at_long_.push_back(time_history_[t_ctr_]->nav_);
          px1_at_long_.push_back(time_history_[t_ctr_]->px1_);
          px2_at_long_.push_back(time_history_[t_ctr_]->px2_);
          sprd_at_long_.push_back(time_history_[t_ctr_]->curr_spread_);
          zs_at_long_.push_back(time_history_[t_ctr_]->z_score_);
          beta_at_long_.push_back(time_history_[t_ctr_]->beta_);
          int_at_long_.push_back(time_history_[t_ctr_]->intercept_);
        } else {
          short_points_.push_back(t_ctr_);
          nav_at_short_.push_back(time_history_[t_ctr_]->nav_);
          px1_at_short_.push_back(time_history_[t_ctr_]->px1_);
          px2_at_short_.push_back(time_history_[t_ctr_]->px2_);
          sprd_at_short_.push_back(time_history_[t_ctr_]->curr_spread_);
          zs_at_short_.push_back(time_history_[t_ctr_]->z_score_);
          beta_at_short_.push_back(time_history_[t_ctr_]->beta_);
          int_at_short_.push_back(time_history_[t_ctr_]->intercept_);
        }
      }
      t_last_pos_ = time_history_[t_ctr_]->pos_;
    }
    if (t_numelem_ > 0)
      annualized_stdev_ = sqrt(252 * 86400 * (6.5 / 24) / (MARKET_DATA_GRANULARITY * 60 * STDEV_COMP_INTERVAL) *
                               (t_sumsqr_ / t_numelem_ - t_sum_ / t_numelem_ * t_sum_ / t_numelem_)) *
                          100.0;

    t_ctr_ = 0;
    double num_pos_trds_ = 0, notional_traded = 0.0;
    int positions_traded = 0;
    std::vector<single_spread_trade_t*> indv_trades_;
    for (t_ctr_ = 0; t_ctr_ < trade_history_.size(); t_ctr_++) {
      if (trade_history_[t_ctr_]->mtm_pnl_ - trade_history_[t_ctr_]->txn_cost_ > 0) {
        num_pos_trds_ += 1.0;
      }
      indv_trades_ = trade_history_[t_ctr_]->indv_trades_;
      for (auto i = 0u; i < indv_trades_.size(); i++) {
        positions_traded += std::abs(indv_trades_[i]->num_lots1_ * indv_trades_[i]->lotsize1_) +
                            std::abs(indv_trades_[i]->num_lots2_ * indv_trades_[i]->lotsize2_);
        notional_traded +=
            std::abs(indv_trades_[i]->num_lots1_ * indv_trades_[i]->lotsize1_ * indv_trades_[i]->price1_) +
            std::abs(indv_trades_[i]->num_lots2_ * indv_trades_[i]->lotsize2_ * indv_trades_[i]->price2_);
      }
    }
    std::cout << param_->instrument_1_ << "_" << param_->instrument_2_ << "\n";
    std::cout << "Returns:\t" << annualized_returns_ << "\nStdev:\t\t" << annualized_stdev_ << "\nNo_Trades:\t"
              << num_trades_ << "\nDD:\t\t" << max_drawdown_ << std::endl;
    std::cout << "%Pos_Trades:\t" << num_pos_trds_ / num_trades_ * 100.0 << "\nFinal Nav:\t" << param_->NAV_
              << std::endl;
    std::cout << "%Active:\t" << t_poscount_ * 100.0 / t_numelem_ << std::endl;
    std::cout << "Total positions traded:\t" << positions_traded << "\n";
    std::cout << "Total notional traded:\t" << notional_traded << "\n";
  } else {
    std::cout << "Trade history empty!\n";
  }
  t_outfile_.close();
  // char gnuplot_cmd_[900000];
  // char setpoint_nav_[250000];
  // char setpoint_px1_[250000];
  // char setpoint_px2_[250000];
  // char setpoint_zs_[250000];
  // char setpoint_spd_[250000];
  // char setpoint_beta_[250000];
  // char setpoint_int_[250000];
  // std::string t_str_ ="";
  // int x_range = 0;
  // double y_range = 0;

  // //for error cases no need to draw graphs
  // if( long_points_.size() == 0 || short_points_.size() == 0 )
  //   return;

  // x_range = 0.01*(*std::max_element(long_points_.begin(), long_points_.end()) -
  // *std::min_element(long_points_.begin(), long_points_.end()));
  // y_range = 0.04*(*std::max_element(nav_at_long_.begin(), nav_at_long_.end()) -
  // *std::min_element(nav_at_long_.begin(), nav_at_long_.end()));
  // for( size_t i = 0; i < long_points_.size() ; i++ )
  // {
  //   sprintf( setpoint_nav_, "%s set object rect from %d,%f to %d,%f fc rgb \'#008800\' ; ", t_str_.c_str(),
  //   long_points_[i], nav_at_long_[i], (int)(long_points_[i] + x_range), nav_at_long_[i] + y_range );
  //   t_str_ = setpoint_nav_ ;
  // }
  // for( size_t i = 0; i < short_points_.size() ; i++ )
  // {
  //   sprintf( setpoint_nav_, "%s set object rect from %d,%f to %d,%f fc rgb \'#ffd700\' ; ", t_str_.c_str(),
  //   short_points_[i], nav_at_short_[i], (int)(short_points_[i] + x_range), nav_at_short_[i] + y_range );
  //   t_str_ = setpoint_nav_ ;
  // }

  // t_str_.clear();
  // y_range = 0.04*(*std::max_element(px1_at_long_.begin(), px1_at_long_.end()) -
  // *std::min_element(px1_at_long_.begin(), px1_at_long_.end()));
  // for( size_t i = 0; i < long_points_.size() ; i++ )
  // {
  //   sprintf( setpoint_px1_, "%s set object rect from %d,%f to %d,%f fc rgb \'#008800\' ; ", t_str_.c_str(),
  //   long_points_[i], px1_at_long_[i], (int)(long_points_[i] + x_range), px1_at_long_[i] + y_range );
  //   t_str_ = setpoint_px1_ ;
  // }
  // for( size_t i = 0; i < short_points_.size() ; i++ )
  // {
  //   sprintf( setpoint_px1_, "%s set object rect from %d,%f to %d,%f fc rgb \'#ffd700\' ; ", t_str_.c_str(),
  //   short_points_[i], px1_at_short_[i], (int)(short_points_[i] + x_range), px1_at_short_[i] + y_range );
  //   t_str_ = setpoint_px1_ ;
  // }

  // t_str_.clear();
  // y_range = 0.04*(*std::max_element(px2_at_long_.begin(), px2_at_long_.end()) -
  // *std::min_element(px2_at_long_.begin(), px2_at_long_.end()));
  // for( size_t i = 0; i < long_points_.size() ; i++ )
  // {
  //   sprintf( setpoint_px2_, "%s set object rect from %d,%f to %d,%f fc rgb \'#008800\' ; ", t_str_.c_str(),
  //   long_points_[i], px2_at_long_[i], (int)(long_points_[i] + x_range), px2_at_long_[i] + y_range );
  //   t_str_ = setpoint_px2_ ;
  // }
  // for( size_t i = 0; i < short_points_.size() ; i++ )
  // {
  //   sprintf( setpoint_px2_, "%s set object rect from %d,%f to %d,%f fc rgb \'#ffd700\' ; ", t_str_.c_str(),
  //   short_points_[i], px2_at_short_[i], (int)(short_points_[i] + x_range), px2_at_short_[i] + y_range );
  //   t_str_ = setpoint_px2_ ;
  // }

  // t_str_.clear();
  // y_range = 0.4;
  // for( size_t i = 0; i < long_points_.size() ; i++ )
  // {
  //   sprintf( setpoint_zs_, "%s set object rect from %d,%f to %d,%f fc rgb \'#008800\' ; ", t_str_.c_str(),
  //   long_points_[i], zs_at_long_[i], (int)(long_points_[i] + x_range), zs_at_long_[i] + y_range );
  //   t_str_ = setpoint_zs_ ;
  // }
  // for( size_t i = 0; i < short_points_.size() ; i++ )
  // {
  //   sprintf( setpoint_zs_, "%s set object rect from %d,%f to %d,%f fc rgb \'#ffd700\' ; ", t_str_.c_str(),
  //   short_points_[i], zs_at_short_[i], (int)(short_points_[i] + x_range), zs_at_short_[i] + y_range );
  //   t_str_ = setpoint_zs_ ;
  // }

  // t_str_.clear();
  // y_range = 0.04*(*std::max_element(sprd_at_long_.begin(), sprd_at_long_.end()) -
  // *std::min_element(sprd_at_long_.begin(), sprd_at_long_.end()));
  // for( size_t i = 0; i < long_points_.size() ; i++ )
  // {
  //   sprintf( setpoint_spd_, "%s set object rect from %d,%f to %d,%f fc rgb \'#008800\' ; ", t_str_.c_str(),
  //   long_points_[i], sprd_at_long_[i], (int)(long_points_[i] + x_range), sprd_at_long_[i] + y_range );
  //   t_str_ = setpoint_spd_ ;
  // }
  // for( size_t i = 0; i < short_points_.size() ; i++ )
  // {
  //   sprintf( setpoint_spd_, "%s set object rect from %d,%f to %d,%f fc rgb \'#ffd700\' ; ", t_str_.c_str(),
  //   short_points_[i], sprd_at_short_[i], (int)(short_points_[i] + x_range), sprd_at_short_[i] + y_range );
  //   t_str_ = setpoint_spd_ ;
  // }

  // t_str_.clear();
  // y_range = 0.04*(*std::max_element(beta_at_long_.begin(), beta_at_long_.end()) -
  // *std::min_element(beta_at_long_.begin(), beta_at_long_.end()));
  // for( size_t i = 0; i < long_points_.size() ; i++ )
  // {
  //   sprintf( setpoint_beta_, "%s set object rect from %d,%f to %d,%f fc rgb \'#008800\' ; ", t_str_.c_str(),
  //   long_points_[i], beta_at_long_[i], (int)(long_points_[i] + x_range), beta_at_long_[i] + y_range );
  //   t_str_ = setpoint_beta_ ;
  // }
  // for( size_t i = 0; i < short_points_.size() ; i++ )
  // {
  //   sprintf( setpoint_beta_, "%s set object rect from %d,%f to %d,%f fc rgb \'#ffd700\' ; ", t_str_.c_str(),
  //   short_points_[i], beta_at_short_[i], (int)(short_points_[i] + x_range), beta_at_short_[i] + y_range );
  //   t_str_ = setpoint_beta_ ;
  // }

  // t_str_.clear();
  // y_range = std::max(0.04, 0.04*(*std::max_element(int_at_long_.begin(), int_at_long_.end()) -
  // *std::min_element(int_at_long_.begin(), int_at_long_.end())));
  // for( size_t i = 0; i < long_points_.size() ; i++ )
  // {
  //   sprintf( setpoint_int_, "%s set object rect from %d,%f to %d,%f fc rgb \'#008800\' ; ", t_str_.c_str(),
  //   long_points_[i], int_at_long_[i], (int)(long_points_[i] + x_range), int_at_long_[i] + y_range );
  //   t_str_ = setpoint_int_ ;
  // }
  // for( size_t i = 0; i < short_points_.size() ; i++ )
  // {
  //   sprintf( setpoint_int_, "%s set object rect from %d,%f to %d,%f fc rgb \'#ffd700\' ; ", t_str_.c_str(),
  //   short_points_[i], int_at_short_[i], (int)(short_points_[i] + x_range), int_at_short_[i] + y_range );
  //   t_str_ = setpoint_int_ ;
  // }
  std::string TEMPFILE = GetFilename("DEBUG");
  // sprintf( gnuplot_cmd_, "echo \"set multiplot layout 4,2 rowsfirst; %s plot \\\"%s\\\" using 2 with lines title
  // \\\"NAV Series\\\"; unset object; %s plot \\\"%s\\\" using 6 with lines title \\\"ZSCORE\\\"; %s plot \\\"%s\\\"
  // using 3 with lines title \\\"PX1\\\" ; unset object; %s plot \\\"%s\\\" using 4 with lines title \\\"PX2\\\" ;
  // unset object; %s plot \\\"%s\\\" using 5 with lines title \\\"Spread\\\"; unset object; %s plot \\\"%s\\\" using 7
  // with lines title \\\"Beta\\\"; unset object; %s plot \\\"%s\\\" using 8 with lines title \\\"Intercept\\\";unset
  // object; plot \\\"%s\\\" using 9 with lines title \\\"PnlVol\\\"  \" | gnuplot -persist", setpoint_nav_, TEMPFILE,
  // setpoint_zs_, TEMPFILE, setpoint_px1_, TEMPFILE, setpoint_px2_, TEMPFILE, setpoint_spd_, TEMPFILE, setpoint_beta_,
  // TEMPFILE, setpoint_int_, TEMPFILE, TEMPFILE );
  // system( gnuplot_cmd_ );

  unlink(TEMPFILE.c_str());
}

void SpreadTrader::UpdateMTM() {
  if (open_trade_ != NULL) {
    open_trade_->mtm_pnl_ =
        open_trade_->cash_position_ +
        (unit_size_position_ * param_->unit_trade_size_2_ * param_->lotsize_2_ * last_close_px_2_0_) -
        (unit_size_position_ * param_->unit_trade_size_1_ * param_->lotsize_1_ * last_close_px_1_0_);

    double t_curr_pnl_ = open_trade_->mtm_pnl_ + param_->NAV_;
    if (t_curr_pnl_ > highest_nav_so_far_) {
      highest_nav_so_far_ = t_curr_pnl_;
    }
    current_drawdown_ = (highest_nav_so_far_ - t_curr_pnl_) / highest_nav_so_far_;
    if (current_drawdown_ > max_drawdown_) {
      max_drawdown_ = current_drawdown_;
    }

    // trigger stoploss or gain handling
    if (open_trade_->mtm_pnl_ > param_->stop_gain_ / 10000.0 * param_->NAV_) {
      std::cout << "StopGain invoked "
                << " mtm " << open_trade_->mtm_pnl_ << " stopgain level " << param_->stop_gain_ / 10000.0 << " NAV "
                << param_->NAV_ << '\n';
      std::cout << "Cash position " << open_trade_->cash_position_ << " UTS " << unit_size_position_ << " NL1 "
                << param_->unit_trade_size_1_ << " LS1 " << param_->lotsize_1_ << " PX1 " << last_close_px_1_0_
                << " NL2 " << param_->unit_trade_size_2_ << " LS2 " << param_->lotsize_2_ << " PX2 "
                << last_close_px_2_0_ << '\n';
      std::cout << "Last_bardata " << last_bardata_time_ << " is_exp " << (is_expiry_day_ ? 'Y' : 'N') << " has_rolled "
                << (has_rolled_over_ ? 'Y' : 'N') << '\n';
      HandleStop(false);
    } else if (open_trade_->mtm_pnl_ + param_->stop_loss_ / 10000.0 * param_->NAV_ < 0) {
      std::cout << "StopLoss invoked "
                << " mtm " << open_trade_->mtm_pnl_ << " stoploss level " << param_->stop_loss_ / 10000.0 << " NAV "
                << param_->NAV_ << '\n';
      std::cout << "Cash position " << open_trade_->cash_position_ << " UTS " << unit_size_position_ << " NL1 "
                << param_->unit_trade_size_1_ << " LS1 " << param_->lotsize_1_ << " PX1 " << last_close_px_1_0_
                << " NL2 " << param_->unit_trade_size_2_ << " LS2 " << param_->lotsize_2_ << " PX2 "
                << last_close_px_2_0_ << '\n';
      std::cout << "Last_bardata " << last_bardata_time_ << " is_exp " << (is_expiry_day_ ? 'Y' : 'N') << " has_rolled "
                << (has_rolled_over_ ? 'Y' : 'N') << '\n';
      HandleStop(true);
    }
  }
}

void SpreadTrader::DumpTimeStats() {
  if (!time_history_.empty()) {
    std::string filename = GetFilename("NAV");
    std::ofstream t_outfile_(filename, std::ios::out);
    if (t_outfile_.good()) {
      std::cout << "Dumping Time Series: " << filename << "\n";
      t_outfile_ << "Time,\tUTS,\tNumLot1,\tNumLot2,\tPX1,\tPX2,\tInst_Spread,\tSpread,\tZSCORE,\tBeta,\tIntercept,"
                    "\tPnlVol,\tReady,\tNAV\n";
      std::vector<time_record_t*>::iterator titer_;
      char time_str_[1024];
      for (titer_ = time_history_.begin(); titer_ != time_history_.end(); titer_++) {
        time_record_t* this_record_ = *titer_;
        strftime(time_str_, 1024, "%Y-%m-%d %H:%M:%S", gmtime((time_t*)&(this_record_->time_)));
        t_outfile_ << /*time_str_ */ this_record_->time_ << ",\t" << this_record_->pos_ << ",\t" << this_record_->px1_
                   << ",\t" << this_record_->px2_ << ",\t" << this_record_->inst_spread_ << ",\t"
                   << this_record_->curr_spread_ << ",\t" << this_record_->z_score_ << ",\t" << this_record_->beta_
                   << ",\t" << this_record_->intercept_ << ",\t" << this_record_->pnlvol_ << ",\t"
                   << (this_record_->ready_to_trade_ ? 'Y' : 'N') << ",\t" << (this_record_->nav_) << ",\t"
                   << param_->unit_trade_size_1_ << ",\t" << param_->unit_trade_size_2_ << '\n';
      }
      t_outfile_ << "\n\n";
    } else
      std::cout << "Error opening file: " << filename << "\n";
  } else
    std::cout << "Time history(NAV series) is empty!\n";
}

void SpreadTrader::PrintTimeRecord(time_record_t* this_record_) {
  std::string filename = GetFilename("NAV");
  std::ofstream t_outfile_(filename, std::ios::out | std::ios::app);  // open file in append mode
  char time_str_[1024];
  strftime(time_str_, 1024, "%Y-%m-%d %H:%M:%S", gmtime((time_t*)&(this_record_->time_)));
  t_outfile_ << /*time_str_ */ this_record_->time_ << ",\t" << this_record_->pos_ << ",\t" << this_record_->px1_
             << ",\t" << this_record_->px2_ << ",\t" << this_record_->inst_spread_ << ",\t"
             << this_record_->curr_spread_ << ",\t" << this_record_->z_score_ << ",\t" << this_record_->beta_ << ",\t"
             << this_record_->intercept_ << ",\t" << this_record_->pnlvol_ << ",\t"
             << (this_record_->ready_to_trade_ ? 'Y' : 'N') << ",\t" << (this_record_->nav_) << ",\t"
             << param_->unit_trade_size_1_ << ",\t" << param_->unit_trade_size_2_ << '\n';
}

void SpreadTrader::DumpTradeStats() {
  if (!trade_history_.empty()) {
    std::string filename = GetFilename("TRADE");
    std::cout << "Dumping Trade stats: " << filename << "\n";
    std::ofstream t_outfile_(filename, std::ios::out);
    t_outfile_ << "Trade_Number,\tTxn_Cost,\tMtm_Pnl,\tMTM_1,\tMTM_2,\tInitial_Nav,\tFinal_Nav,\tTrade_Start,\tTrade_"
                  "End,\tWas_Stopped,\tTrade_Type,\tIs_Roll,\t"
                  "Trade_Leg,\t Trade_Type,\t Trade_Time,\t UTS,\t Spread,\t LotSize_1,\t Num_Lots1,\t Price1,\t "
                  "LotSize_2,\t Num_Lots,\t Price2\n";
    std::vector<spread_trade_record_t*>::iterator trade_iter_;
    std::vector<single_spread_trade_t*>::iterator leg_iter_;
    int t_trade_ctr_ = 0;
    for (trade_iter_ = trade_history_.begin(); trade_iter_ != trade_history_.end(); trade_iter_++) {
      t_trade_ctr_++;
      spread_trade_record_t* t_one_trade_ = *trade_iter_;
      t_outfile_ << /*std::fixed <<*/ t_trade_ctr_ << ",\t" << t_one_trade_->txn_cost_ << ",\t"
                 << t_one_trade_->mtm_pnl_ << ",\t" << t_one_trade_->cash_pos_1_ << ",\t" << t_one_trade_->cash_pos_2_
                 << ",\t" << t_one_trade_->initial_nav_ << ",\t"
                 << (t_one_trade_->initial_nav_ + t_one_trade_->mtm_pnl_ - t_one_trade_->txn_cost_) << ",\t"
                 << t_one_trade_->trade_start_time_ << ",\t" << t_one_trade_->trade_end_time_ << ",\t"
                 << (t_one_trade_->was_stopped_out_ ? 'Y' : 'N') << ",\t" << t_one_trade_->trade_type_ << ",\t"
                 << (t_one_trade_->is_roll_trade_ ? 'Y' : 'N') << ",\t,\t,\t,\t,\t,\t,\t,\t,\t,\t,\n";

      int t_leg_ctr_ = 0;
      for (leg_iter_ = (t_one_trade_->indv_trades_).begin(); leg_iter_ != (t_one_trade_->indv_trades_).end();
           leg_iter_++) {
        t_leg_ctr_++;
        single_spread_trade_t* t_one_leg_ = *leg_iter_;
        t_outfile_ << /*std::fixed <<*/ t_trade_ctr_ << ",\t,\t,\t,\t,\t,\t,\t,\t,\t,\t,\t," << t_leg_ctr_ << ",\t"
                   << t_one_leg_->trade_type_ << ",\t" << t_one_leg_->trade_time_ << ",\t" << t_one_leg_->num_unit_size_
                   << ",\t" << t_one_leg_->trade_spread_ << ",\t" << t_one_leg_->lotsize1_ << ",\t"
                   << t_one_leg_->num_lots1_ << ",\t" << t_one_leg_->price1_ << ",\t" << t_one_leg_->lotsize2_ << ",\t"
                   << t_one_leg_->num_lots2_ << ",\t" << t_one_leg_->price2_ << '\n';
      }
    }
  } else
    std::cout << "Trade history is empty!\n";
}

void SpreadTrader::PrintOneTrade(spread_trade_record_t* t_one_trade_) {
  std::string filename = GetFilename("TRADE");
  std::ofstream t_outfile_(filename, std::ios::out | std::ios::app);  // open file in append mode
  std::vector<single_spread_trade_t*>::iterator leg_iter_;
  t_outfile_ << /*std::fixed <<*/ "0,\t" << t_one_trade_->txn_cost_ << ",\t" << t_one_trade_->mtm_pnl_ << ",\t"
             << t_one_trade_->cash_pos_1_ << ",\t" << t_one_trade_->cash_pos_2_ << ",\t" << t_one_trade_->initial_nav_
             << ",\t" << (t_one_trade_->initial_nav_ + t_one_trade_->mtm_pnl_ - t_one_trade_->txn_cost_) << ",\t"
             << t_one_trade_->trade_start_time_ << ",\t" << t_one_trade_->trade_end_time_ << ",\t"
             << (t_one_trade_->was_stopped_out_ ? 'Y' : 'N') << ",\t" << t_one_trade_->trade_type_ << ",\t"
             << (t_one_trade_->is_roll_trade_ ? 'Y' : 'N') << ",\t,\t,\t,\t,\t,\t,\t,\t,\t,\t,\n";

  int t_leg_ctr_ = 0;
  for (leg_iter_ = (t_one_trade_->indv_trades_).begin(); leg_iter_ != (t_one_trade_->indv_trades_).end(); leg_iter_++) {
    t_leg_ctr_++;
    single_spread_trade_t* t_one_leg_ = *leg_iter_;
    t_outfile_ << /*std::fixed <<*/ "0,\t,\t,\t,\t,\t,\t,\t,\t,\t,\t,\t," << t_leg_ctr_ << ",\t"
               << t_one_leg_->trade_type_ << ",\t" << t_one_leg_->trade_time_ << ",\t" << t_one_leg_->num_unit_size_
               << ",\t" << t_one_leg_->trade_spread_ << ",\t" << t_one_leg_->lotsize1_ << ",\t"
               << t_one_leg_->num_lots1_ << ",\t" << t_one_leg_->price1_ << ",\t" << t_one_leg_->lotsize2_ << ",\t"
               << t_one_leg_->num_lots2_ << ",\t" << t_one_leg_->price2_ << '\n';
  }
}

bool SpreadTrader::IsTradeAllowed(int t_pos_desired_) {
  bool t_retval_ = true;
  // We place order in all cases except
  //( a ) trade cooloff interval violation
  //( b ) stop loss or stop gain violation
  //( c ) trading_start_time is in future

  if (last_bardata_time_ < (unsigned)trading_start_time) {
    t_retval_ = false;
  }

  // For banned instruments
  if (is_banned) {
    t_retval_ = false;
  }

  if (t_pos_desired_ > unit_size_position_ && unit_size_position_ > 0 && last_exec_type_ == 'B' &&
      last_bardata_time_ - last_exec_entry_time_ <= param_->trade_cooloff_interval_) {
    t_retval_ = false;
  }

  if (t_pos_desired_ < unit_size_position_ && unit_size_position_ < 0 && last_exec_type_ == 'S' &&
      last_bardata_time_ - last_exec_entry_time_ <= param_->trade_cooloff_interval_) {
    t_retval_ = false;
  }

  if (last_trade_stopped_out_ && last_opentrade_type_ == 'B' && t_pos_desired_ > 0) {
    t_retval_ = false;
  }

  if (last_trade_stopped_out_ && last_opentrade_type_ == 'S' && t_pos_desired_ < 0) {
    t_retval_ = false;
  }

  // introduce check for potential case of uts being 0 due to sclaing down in param
  if (unit_size_position_ == 0 && t_pos_desired_ != 0) {
    param_->RecomputeLotSize(last_close_px_1_0_, last_close_px_2_0_, beta_, param_->target_vol_ / ComputePnlVol(),
                             curr_day_hlife_, curr_day_adf_, trailing_ret_);
    if (param_->unit_trade_size_1_ == 0 || param_->unit_trade_size_2_ == 0) {
      t_retval_ = false;
    }
  }
  return t_retval_;
}

void SpreadTrader::CheckAndPlaceOrder(int t_pos_desired_) {
  if (IsTradeAllowed(t_pos_desired_)) {
    dbglogger_ << last_bardata_time_ << " Sprd_Trader::CheckAndPlaceOrder is calling FillAndPopulateTradeRecords("
               << t_pos_desired_ << ")\n";
    FillAndPopulateTradeRecords(t_pos_desired_);
    unit_size_position_ = t_pos_desired_;
  }
}

void SpreadTrader::FillAndPopulateTradeRecords(int t_pos_desired_) {
  // If new trade is starting, we need to create appropriate Trade struct
  if (open_trade_ == NULL) {
    open_trade_ = new spread_trade_record_t();
    open_trade_->trade_start_time_ = last_bardata_time_;
    open_trade_->trade_type_ = (t_pos_desired_ > 0 ? 'B' : 'S');
    last_opentrade_type_ = (t_pos_desired_ > 0 ? 'B' : 'S');
    (open_trade_->indv_trades_).clear();
    open_trade_->cash_position_ = 0;
    open_trade_->initial_nav_ = param_->NAV_;
    last_trade_stopped_out_ = false;
    // debug
    open_trade_->cash_pos_1_ = 0;
    open_trade_->cash_pos_2_ = 0;
  }

  // simulate one order at this point
  single_spread_trade_t* this_trade_ = new single_spread_trade_t();
  this_trade_->trade_time_ = last_bardata_time_;
  this_trade_->trade_type_ = ((t_pos_desired_ - unit_size_position_) > 0 ? 'B' : 'S');
  this_trade_->num_unit_size_ = abs(t_pos_desired_ - unit_size_position_);
  this_trade_->prev_unit_size_ = unit_size_position_;
  this_trade_->trade_spread_ = current_spread_;
  this_trade_->lotsize1_ = param_->lotsize_1_;
  this_trade_->num_lots1_ = (unit_size_position_ - t_pos_desired_) * param_->unit_trade_size_1_;
  this_trade_->price1_ = last_close_px_1_0_;
  this_trade_->lotsize2_ = param_->lotsize_2_;
  this_trade_->num_lots2_ = (t_pos_desired_ - unit_size_position_) * param_->unit_trade_size_2_;
  this_trade_->price2_ = last_close_px_2_0_;
  last_exec_type_ = this_trade_->trade_type_;
  last_exec_entry_time_ = last_bardata_time_;

  // update appropriate open trade vars
  (open_trade_->indv_trades_).push_back(this_trade_);
  (open_trade_->cash_position_) -= (this_trade_->lotsize1_ * this_trade_->num_lots1_ * this_trade_->price1_ +
                                    this_trade_->lotsize2_ * this_trade_->num_lots2_ * this_trade_->price2_);
  (open_trade_->cash_pos_1_) -= this_trade_->lotsize1_ * this_trade_->num_lots1_ * this_trade_->price1_;
  (open_trade_->cash_pos_2_) -= this_trade_->lotsize2_ * this_trade_->num_lots2_ * this_trade_->price2_;
  (open_trade_->txn_cost_) +=
      (fabs(this_trade_->lotsize1_ * this_trade_->num_lots1_ * this_trade_->price1_) * AVG_FUT_COMMISH +
       fabs(this_trade_->lotsize2_ * this_trade_->num_lots2_ * this_trade_->price2_) * AVG_FUT_COMMISH);

  // If trade is finishing, some specific handling is needed
  if (t_pos_desired_ == 0) {
    open_trade_->mtm_pnl_ = open_trade_->cash_position_;
    // TODO - not robust against txn cost effects
    if (open_trade_->mtm_pnl_ / param_->NAV_ + max_drawdown_ < 0) {
      max_drawdown_ = fabs(open_trade_->mtm_pnl_ / param_->NAV_);
    }
    param_->NAV_ += (open_trade_->mtm_pnl_ - open_trade_->txn_cost_);
    if (param_->NAV_ > highest_nav_so_far_) {
      highest_nav_so_far_ = param_->NAV_;
    }
    open_trade_->trade_end_time_ = last_bardata_time_;
    if (open_trade_->indv_trades_.size() == 1) std::cout << " Error - one leg close trade \n";
    trade_history_.push_back(open_trade_);
    PrintOneTrade(open_trade_);
    open_trade_ = NULL;
  }
}

double SpreadTrader::ComputeCurrentAvgSpread() {
  size_t t_num_elems_ = spread_values_.size();
  int t_ctr_ = t_num_elems_ - 1;
  double t_ret_value_ = 0;
  for (; t_ctr_ >= 0 && (t_num_elems_ - t_ctr_ <= SPREAD_AVERAGING_DURATION); t_ctr_--) {
    t_ret_value_ += spread_values_[t_ctr_];
  }
  if (t_num_elems_ >= 1) {
    t_ret_value_ = t_ret_value_ / std::min(SPREAD_AVERAGING_DURATION, (int)t_num_elems_);
  }
  return t_ret_value_;
}

double SpreadTrader::ComputeCurrentInstantaneousSpread(double px1_, double px2_) {
  // deal with initiedge cases
  if (param_->asset_comp_mode_ == 0 && hist_prices_1_.empty() && param_->spread_comp_mode_ != 2) {
    beta_ = px1_ / px2_;
    intercept_ = 0;
  } else if (param_->asset_comp_mode_ == 1 && hist_logpx_1_.empty() && param_->spread_comp_mode_ != 2) {
    beta_ = log(px1_) / log(px2_);
    intercept_ = 0;
  }

  // corresponds to Linear Regression with no intercept term
  if (param_->spread_comp_mode_ == 1) {
    if (param_->asset_comp_mode_ == 0)  // price based
    {
      intercept_ = 0.0;
      if (!hist_prices_1_.empty()) {
        beta_ = sum_prod_prices_ / sum_prices2_sqr_;
      }
      return (px1_ - beta_ * px2_ - intercept_);
    } else  // returns based
    {
      intercept_ = 0.0;
      if (!hist_logpx_1_.empty()) {
        beta_ = sum_prod_logpx_ / sum_logpx2_sqr_;
      }
      return (log(px1_) - beta_ * log(px2_) - intercept_);
    }
  }

  else  // corresponds to kalman
  {
    kalman_H[0] = 1;
    if (param_->asset_comp_mode_ == 0) {
      kalman_H[1] = px2_;
      kalman_Y[0] = px1_;
    } else {
      kalman_H[1] = log(px2_);
      kalman_Y[0] = log(px1_);
    }
    kalman_->setObservation(kalman_Y);
    kalman_->setH(kalman_H);
    kalman_->updateWeights();
    beta_ = kalman_->getKalmanParam(1);
    intercept_ = kalman_->getKalmanParam(0);

    if (param_->asset_comp_mode_ == 0) {
      return (px1_ - kalman_->getKalmanParam(0) - kalman_->getKalmanParam(1) * px2_);
    } else {
      return (log(px1_) - kalman_->getKalmanParam(0) - kalman_->getKalmanParam(1) * log(px2_));
    }
  }
}

void SpreadTrader::DumpLRData() {
  dbglogger_ << " HisPx dump " << '\n';
  size_t t_ctr_ = 0;
  for (; t_ctr_ < hist_prices_2_.size(); t_ctr_++) {
    dbglogger_ << t_ctr_ << ' ' << hist_prices_1_[t_ctr_] << ' ' << hist_prices_2_[t_ctr_] << '\n';
  }
  dbglogger_ << '\n' << " Beta " << beta_ << " intercept " << intercept_ << '\n';
}

void SpreadTrader::CheckAndUpdateSpreadVector(double px1_, double px2_) {
  bool added_or_deleted_ = false;
  size_t num_samples_ = 0;
  num_samples_ = spread_values_.size();
  // pad last seen datapoint to the extent needed
  if (num_samples_ >= 1) {
    while (last_bardata_time_ - spread_value_snaptimes_[num_samples_ - 1] >=
           2 * MARKET_DATA_GRANULARITY * NUM_SECONDS_IN_MIN) {
      spread_value_snaptimes_.push_back(spread_value_snaptimes_[num_samples_ - 1] +
                                        MARKET_DATA_GRANULARITY * NUM_SECONDS_IN_MIN);
      added_or_deleted_ = true;
      spread_values_.push_back(spread_values_[num_samples_ - 1]);
      num_samples_ += 1;
    }
  }
  // remove old datapoints to the extent needed
  while (num_samples_ >= 1 &&
         last_bardata_time_ - spread_value_snaptimes_[0] >= param_->spread_hist_length_ * NUM_SECONDS_IN_MIN) {
    spread_value_snaptimes_.erase(spread_value_snaptimes_.begin());
    added_or_deleted_ = true;
    spread_values_.erase(spread_values_.begin());
    num_samples_ -= 1;
  }

  // remove old data from zscore vector
  while (traded_spread_difference_.size() > param_->zscore_vec_len_) {
    double t_val_ = traded_spread_difference_.front();
    sum_x_ -= t_val_;
    sum_x2_ -= t_val_ * t_val_;
    stdev_ready_ = true;
    traded_spread_difference_.pop_front();
  }

  // remove old data from hist_prices vector
  while (hist_prices_1_.size() > param_->zscore_vec_len_) {
    sum_prices_1_ -= hist_prices_1_.front();
    sum_prices_2_ -= hist_prices_2_.front();
    sum_prod_prices_ -= hist_prices_1_.front() * hist_prices_2_.front();
    sum_prices1_sqr_ -= hist_prices_1_.front() * hist_prices_1_.front();
    sum_prices2_sqr_ -= hist_prices_2_.front() * hist_prices_2_.front();

    hist_prices_1_.pop_front();
    hist_prices_2_.pop_front();
  }

  // remove old data from the hist_logpx vector
  while (hist_logpx_1_.size() > param_->zscore_vec_len_) {
    sum_logpx_1_ -= hist_logpx_1_.front();
    sum_logpx_2_ -= hist_logpx_2_.front();
    sum_prod_logpx_ -= hist_logpx_1_.front() * hist_logpx_2_.front();
    sum_logpx2_sqr_ -= hist_logpx_2_.front() * hist_logpx_2_.front();

    hist_logpx_1_.pop_front();
    hist_logpx_2_.pop_front();
  }

  // add current sample if sufficient time has elapsed
  if (spread_value_snaptimes_.empty() || (last_bardata_time_ - spread_value_snaptimes_[num_samples_ - 1] >=
                                          MARKET_DATA_GRANULARITY * NUM_SECONDS_IN_MIN)) {
    double t_this_value_ = ComputeCurrentInstantaneousSpread(px1_, px2_);
    spread_value_snaptimes_.push_back(last_bardata_time_);
    spread_values_.push_back(t_this_value_);
    num_samples_ += 1;
    added_or_deleted_ = true;
  }

  if (added_or_deleted_) {
    current_spread_ = ComputeCurrentAvgSpread();

    sum_x_ += current_spread_;
    sum_x2_ += current_spread_ * current_spread_;
    traded_spread_difference_.push_back(current_spread_);
    size_t this_vec_size_ = traded_spread_difference_.size();
    diff_stdev_ = sqrt(sum_x2_ / this_vec_size_ - sum_x_ / this_vec_size_ * sum_x_ / this_vec_size_);
    param_->MultiplyThreshBy(diff_stdev_);

    // add to hist prices as well
    hist_prices_1_.push_back(px1_);
    hist_prices_2_.push_back(px2_);
    sum_prices_1_ += px1_;
    sum_prices_2_ += px2_;
    sum_prod_prices_ += px1_ * px2_;
    sum_prices2_sqr_ += px2_ * px2_;
    sum_prices1_sqr_ += px1_ * px1_;

    // set stdev values in spread_exec if we are using spread exec and history is ready
    if (spread_exec_ && hist_prices_1_.size() >= param_->zscore_vec_len_) {
      size_t t_num_elem_ = hist_prices_1_.size();
      if (is_pass_leg_first_) {
        spread_exec_->SetStdevValues(
            sqrt(sum_prices1_sqr_ / t_num_elem_ - sum_prices_1_ / t_num_elem_ * sum_prices_1_ / t_num_elem_),
            sqrt(sum_prices2_sqr_ / t_num_elem_ - sum_prices_2_ / t_num_elem_ * sum_prices_2_ / t_num_elem_));
      } else {
        spread_exec_->SetStdevValues(
            sqrt(sum_prices2_sqr_ / t_num_elem_ - sum_prices_2_ / t_num_elem_ * sum_prices_2_ / t_num_elem_),
            sqrt(sum_prices1_sqr_ / t_num_elem_ - sum_prices_1_ / t_num_elem_ * sum_prices_1_ / t_num_elem_));
      }
    }

    // add to hist_logpx if price history is long enough
    double t_lpx1_ = log(px1_);
    double t_lpx2_ = log(px2_);
    hist_logpx_1_.push_back(t_lpx1_);
    hist_logpx_2_.push_back(t_lpx2_);
    sum_logpx_1_ += t_lpx1_;
    sum_logpx_2_ += t_lpx2_;
    sum_prod_logpx_ += t_lpx1_ * t_lpx2_;
    sum_logpx2_sqr_ += t_lpx2_ * t_lpx2_;
  }

  if (!added_or_deleted_) {
    std::cerr << " Weird Case - check " << last_bardata_time_ << ' ' << spread_value_snaptimes_.size() << ' '
              << (spread_value_snaptimes_.empty() ? 0 : spread_value_snaptimes_[num_samples_ - 1]) << ' ' << px1_ << ' '
              << px2_ << "\n";
  }

  // set values for trailing_ret_
  if (hist_prices_1_.size() >= (unsigned int)param_->ret_compute_duration_mins_) {
    int t_final_index_ = hist_prices_1_.size() - 1;
    int t_initial_index_ = std::max(0, t_final_index_ - param_->ret_compute_duration_mins_);
    double t_ret_1_ = hist_prices_1_[t_final_index_] / hist_prices_1_[t_initial_index_] - 1.0;
    double t_ret_2_ = hist_prices_2_[t_final_index_] / hist_prices_2_[t_initial_index_] - 1.0;
    trailing_ret_ = fabs(t_ret_1_ - t_ret_2_);
  }

  // set ready to trade when some element is deleted from snap vector
  if (!ready_to_trade_ && added_or_deleted_ && stdev_ready_) {
    ready_to_trade_ = true;
  }

  if (ready_to_trade_) {
    // create time record and add it
    time_record_t* t_time_record_ = new time_record_t();
    t_time_record_->time_ = last_bardata_time_;
    t_time_record_->pos_ = unit_size_position_;
    t_time_record_->curr_spread_ = current_spread_;
    t_time_record_->z_score_ = (diff_stdev_ > 1e-7 ? (current_spread_) / diff_stdev_ : 0.0);
    t_time_record_->curr_spread_diff_ = current_spread_;
    t_time_record_->ready_to_trade_ =
        (ready_to_trade_ && !(param_->get_flat) && !(param_->earnings_getflat_) && !(param_->zscore_getflat_));
    t_time_record_->px1_ = px1_;
    t_time_record_->px2_ = px2_;
    t_time_record_->inst_spread_ = spread_values_[spread_values_.size() - 1];
    t_time_record_->nav_ = param_->NAV_ + (open_trade_ != NULL ? open_trade_->mtm_pnl_ - open_trade_->txn_cost_ : 0);
    t_time_record_->beta_ = beta_;
    t_time_record_->intercept_ = intercept_;
    t_time_record_->pnlvol_ = ComputePnlVol();
    time_history_.push_back(t_time_record_);
    PrintTimeRecord(t_time_record_);

    // update zscore
    if (param_->zscore_ema_mult_factor_ > 0)  // nagative value signifies we don't want to
    // use zscore criterion for getting flat
    {
      zscore_ema_ = zscore_ema_ * param_->zscore_ema_mult_factor_ +
                    (1.0 - param_->zscore_ema_mult_factor_) * t_time_record_->z_score_;
      CheckAndSetZScoreGetflat(t_time_record_->z_score_);
    }
  }
}

void SpreadTrader::CheckAndSetZScoreGetflat(double t_curr_zscore_) {
  if (param_->zscore_getflat_ && t_curr_zscore_ * zscore_at_last_getflat_ <= 0) {
    param_->zscore_getflat_ = false;
    dbglogger_ << "ZScore Getflat Reset False at " << last_bardata_time_ << " for " << (param_->instrument_1_).c_str()
               << ':' << (param_->instrument_2_).c_str() << '\n';
  }
  if (!param_->zscore_getflat_ && (fabs(zscore_ema_) > param_->zscore_getflat_threshold_) &&
      (unit_size_position_ * zscore_ema_ > 0)) {
    param_->zscore_getflat_ = true;
    zscore_at_last_getflat_ = zscore_ema_;
    dbglogger_ << "ZScore Getflat Set True at " << last_bardata_time_ << ' ' << fabs(zscore_ema_) << " for "
               << (param_->instrument_1_).c_str() << ':' << (param_->instrument_2_).c_str() << '\n';
  }
}

double SpreadTrader::ComputePnlVol() {
  // not supported for spread
  if (param_->spread_comp_mode_ == 0 || time_history_.size() < STDEV_COMP_LEN / 2) return 0.3;  // TODO - fix this
  // use vol over zscore vec len duration - can be changed later
  // compute hourly vol
  int n_count_ = 0;
  double sum_val_ = 0;
  double sum_sqr_val_ = 0;
  for (int i = time_history_.size() - 1; i >= std::max(STDEV_COMP_INTERVAL, (int)time_history_.size() - STDEV_COMP_LEN);
       i--) {
    double t_one_val_;
    if (param_->asset_comp_mode_ == 0) {
      t_one_val_ = (time_history_[i]->curr_spread_ - time_history_[i - STDEV_COMP_INTERVAL]->curr_spread_) *
                   param_->risk_factor_ / (time_history_[i - STDEV_COMP_INTERVAL]->px1_ * param_->margin_1_ +
                                           time_history_[i - STDEV_COMP_INTERVAL]->beta_ *
                                               time_history_[i - STDEV_COMP_INTERVAL]->px2_ * param_->margin_2_);
    } else {
      t_one_val_ = (time_history_[i]->curr_spread_ - time_history_[i - STDEV_COMP_INTERVAL]->curr_spread_) *
                   param_->risk_factor_ /
                   (param_->margin_1_ + param_->margin_2_ * time_history_[i - STDEV_COMP_INTERVAL]->beta_);
    }
    sum_val_ += t_one_val_;
    sum_sqr_val_ += t_one_val_ * t_one_val_;
    n_count_ += 1;
  }
  return (sqrt((sum_sqr_val_ / n_count_ - sum_val_ / n_count_ * sum_val_ / n_count_)) *
          sqrt(252.0 * 6.5 * 60 / STDEV_COMP_INTERVAL));
}

void SpreadTrader::OnCorporateAction(double px_mult_factor_1_, double px_mult_factor_2_) {
  dbglogger_ << "Before corporate adjustment:\n" << current_spread_ << ", " << spread_values_.back() << ", " << beta_
             << "\n";
  if (param_->asset_comp_mode_ == 0) {
    std::transform(hist_prices_1_.begin(), hist_prices_1_.end(), hist_prices_1_.begin(),
                   std::bind1st(std::multiplies<double>(), px_mult_factor_1_));
    std::transform(hist_prices_2_.begin(), hist_prices_2_.end(), hist_prices_2_.begin(),
                   std::bind1st(std::multiplies<double>(), px_mult_factor_2_));
    std::transform(spread_values_.begin(), spread_values_.end(), spread_values_.begin(),
                   std::bind1st(std::multiplies<double>(), px_mult_factor_1_));
    std::transform(traded_spread_difference_.begin(), traded_spread_difference_.end(),
                   traded_spread_difference_.begin(), std::bind1st(std::multiplies<double>(), px_mult_factor_1_));
    sum_x_ *= px_mult_factor_1_;
    sum_x2_ *= (px_mult_factor_1_ * px_mult_factor_1_);
    sum_prices_1_ *= px_mult_factor_1_;
    sum_prices_2_ *= px_mult_factor_2_;
    sum_prod_prices_ *= (px_mult_factor_1_ * px_mult_factor_2_);
    sum_prices1_sqr_ *= (px_mult_factor_1_ * px_mult_factor_1_);
    sum_prices2_sqr_ *= (px_mult_factor_2_ * px_mult_factor_2_);
    beta_ *= (px_mult_factor_1_ / px_mult_factor_2_);
  } else if (param_->asset_comp_mode_ == 1) {
    double logpx_add_factor_1_ = log(px_mult_factor_1_);
    double logpx_add_factor_2_ = log(px_mult_factor_2_);

    double old_logpx_1_ = hist_logpx_1_.back();
    double old_logpx_2_ = hist_logpx_2_.back();

    std::deque<double> beta_times_logpx2;
    beta_times_logpx2.resize(hist_logpx_2_.size());

    std::transform(hist_logpx_1_.begin(), hist_logpx_1_.end(), hist_logpx_1_.begin(),
                   std::bind1st(std::plus<double>(), logpx_add_factor_1_));
    std::transform(hist_logpx_2_.begin(), hist_logpx_2_.end(), hist_logpx_2_.begin(),
                   std::bind1st(std::plus<double>(), logpx_add_factor_2_));
    sum_prod_logpx_ += hist_logpx_1_.size() * logpx_add_factor_1_ * logpx_add_factor_2_ +
                       logpx_add_factor_1_ * sum_logpx_2_ + logpx_add_factor_2_ * sum_logpx_1_;
    sum_logpx_1_ += hist_logpx_1_.size() * logpx_add_factor_1_;
    sum_logpx_2_ += hist_logpx_2_.size() * logpx_add_factor_2_;

    // Change beta in a way such that the spread remains same, i.e spread is scaled by px1/old_px1.
    double logspread_mult_factor_1_ = hist_logpx_1_.back() / old_logpx_1_;
    double logspread_mult_factor_2_ = hist_logpx_2_.back() / old_logpx_2_;

    beta_ *= logspread_mult_factor_1_ / logspread_mult_factor_2_;

    // Re-adjust sum_logpx2_sqr_ according to new beta_; beta = sum_prod_logpx_/sum_logpx2_sqr_ as given by linear
    // regression
    sum_logpx2_sqr_ = sum_prod_logpx_ / beta_;

    // In below 2 lines, assumption is that size of spread_values_ is smaller than size of hist_logpx
    std::transform(hist_logpx_2_.begin(), hist_logpx_2_.end(), beta_times_logpx2.begin(),
                   std::bind1st(std::multiplies<double>(), beta_));
    std::transform(hist_logpx_1_.end() - spread_values_.size() + 1, hist_logpx_1_.end(), beta_times_logpx2.begin(),
                   spread_values_.begin(), std::minus<double>());

    // Using approximate calculation for traded_spread_difference. This ensures stdev stability.
    sum_x_ *= logspread_mult_factor_1_;
    sum_x2_ *= (logspread_mult_factor_1_ * logspread_mult_factor_1_);
    std::transform(traded_spread_difference_.begin(), traded_spread_difference_.end(),
                   traded_spread_difference_.begin(),
                   std::bind1st(std::multiplies<double>(), logspread_mult_factor_1_));
  }
  // Multiply thrreshold parameters
  size_t this_vec_size_ = traded_spread_difference_.size();
  diff_stdev_ = sqrt(sum_x2_ / this_vec_size_ - sum_x_ / this_vec_size_ * sum_x_ / this_vec_size_);
  param_->MultiplyThreshBy(diff_stdev_);

  current_spread_ = ComputeCurrentAvgSpread();
  dbglogger_ << "After adjustment:\n" << current_spread_ << ", " << spread_values_.back() << ", " << beta_ << "\n";
}

void SpreadTrader::RollOver() {
  if (open_trade_) {
    // first close open trade
    open_trade_->is_roll_trade_ = true;
    FillAndPopulateTradeRecords(0);
    unit_size_position_ = 0;
    roll_trade_done_ = true;
  }
  if (spread_exec_) {
    spread_exec_->SetGetflatMode(true);
    spread_exec_->SetDesiredPositions(0, 0, 0, intercept_, beta_);
    spread_exec_ = spread_exec_next_month_;  // change to next exec
  }

  // Update the param lotsizes to be next month param lotsizes
  param_->lotsize_1_ = param_next_month_->lotsize_1_;
  param_->lotsize_2_ = param_next_month_->lotsize_2_;

  // change all relevant data structures
  last_mkt_trade_time_1_0_ = last_mkt_trade_time_1_1_;
  last_mkt_trade_time_2_0_ = last_mkt_trade_time_2_1_;

  if (param_->asset_comp_mode_ == 0) {
    double px_mult_factor_1_ = last_close_px_1_1_ / last_close_px_1_0_;
    double px_mult_factor_2_ = last_close_px_2_1_ / last_close_px_2_0_;
    std::transform(hist_prices_1_.begin(), hist_prices_1_.end(), hist_prices_1_.begin(),
                   std::bind1st(std::multiplies<double>(), px_mult_factor_1_));
    std::transform(hist_prices_2_.begin(), hist_prices_2_.end(), hist_prices_2_.begin(),
                   std::bind1st(std::multiplies<double>(), px_mult_factor_2_));
    std::transform(spread_values_.begin(), spread_values_.end(), spread_values_.begin(),
                   std::bind1st(std::multiplies<double>(), px_mult_factor_1_));
    sum_prices_1_ *= px_mult_factor_1_;
    sum_prices_2_ *= px_mult_factor_2_;
    sum_prod_prices_ *= (px_mult_factor_1_ * px_mult_factor_2_);
    sum_prices1_sqr_ *= (px_mult_factor_1_ * px_mult_factor_1_);
    sum_prices2_sqr_ *= (px_mult_factor_2_ * px_mult_factor_2_);
    beta_ *= (px_mult_factor_1_ / px_mult_factor_2_);
  } else if (param_->asset_comp_mode_ == 1) {
    double logpx_mult_factor_1_ = log(last_close_px_1_1_) / log(last_close_px_1_0_);
    double logpx_mult_factor_2_ = log(last_close_px_2_1_) / log(last_close_px_2_0_);

    std::transform(hist_logpx_1_.begin(), hist_logpx_1_.end(), hist_logpx_1_.begin(),
                   std::bind1st(std::multiplies<double>(), logpx_mult_factor_1_));
    std::transform(hist_logpx_2_.begin(), hist_logpx_2_.end(), hist_logpx_2_.begin(),
                   std::bind1st(std::multiplies<double>(), logpx_mult_factor_2_));
    std::transform(spread_values_.begin(), spread_values_.end(), spread_values_.begin(),
                   std::bind1st(std::multiplies<double>(), logpx_mult_factor_1_));
    sum_logpx_1_ *= logpx_mult_factor_1_;
    sum_logpx_2_ *= logpx_mult_factor_2_;
    sum_prod_logpx_ *= (logpx_mult_factor_1_ * logpx_mult_factor_2_);
    sum_logpx2_sqr_ *= (logpx_mult_factor_2_ * logpx_mult_factor_2_);
    beta_ *= (logpx_mult_factor_1_ / logpx_mult_factor_2_);
  }
  current_spread_ = ComputeCurrentAvgSpread();
  last_close_px_1_0_ = last_close_px_1_1_;
  last_close_px_2_0_ = last_close_px_2_1_;

  has_rolled_over_ = true;

  if (roll_trade_done_) {
    // initialize new trade and mark it as roll
    CheckAndUpdateSpreadVector(last_close_px_1_1_, last_close_px_2_1_);
    TradeLogic();
  }
}

bool IsExpiryToday(uint64_t time_, int expiry_date) {
  int t_year_ = (expiry_date / 10000) - 1900;
  int t_month_ = ((expiry_date / 100) % 100) - 1;
  int t_day_ = expiry_date % 100;

  time_t bar_time_ = static_cast<time_t>(time_);

  struct tm bar_timeinfo_ = {};
  gmtime_r(&bar_time_, &bar_timeinfo_);

  return (bar_timeinfo_.tm_year == t_year_ && bar_timeinfo_.tm_mon == t_month_ && bar_timeinfo_.tm_mday == t_day_);
}

void SpreadTrader::AdjustTimeOnNewDay(uint64_t time_) {
  uint64_t t_time_add_ = time_ - last_bardata_time_ - MARKET_DATA_GRANULARITY * NUM_SECONDS_IN_MIN;
  if (last_exec_entry_time_ > 0) last_exec_entry_time_ += t_time_add_;

  if (last_mkt_trade_time_1_0_ > 0) last_mkt_trade_time_1_0_ += t_time_add_;

  if (last_mkt_trade_time_1_1_ > 0) last_mkt_trade_time_1_1_ += t_time_add_;

  if (last_mkt_trade_time_2_0_ > 0) last_mkt_trade_time_2_0_ += t_time_add_;

  if (last_mkt_trade_time_2_1_ > 0) last_mkt_trade_time_2_1_ += t_time_add_;

  size_t t_vec_size_ = spread_value_snaptimes_.size();
  size_t t_ctr_ = 0;
  for (; t_ctr_ < t_vec_size_; t_ctr_++) {
    spread_value_snaptimes_[t_ctr_] += t_time_add_;
  }

  // set rollover vars to false
  is_expiry_day_ = false;
  has_rolled_over_ = false;
  roll_trade_done_ = false;

  // update hlife and adf values for the day
  time_t bar_time_ = static_cast<time_t>(time_);

  struct tm bar_timeinfo_ = {};
  gmtime_r(&bar_time_, &bar_timeinfo_);
  int t_yyyymmdd_ = (1900 + bar_timeinfo_.tm_year) * 10000 + (bar_timeinfo_.tm_mon + 1) * 100 + bar_timeinfo_.tm_mday;
  if (date_to_hlife_.find(t_yyyymmdd_) == date_to_hlife_.end() ||
      date_to_adf_.find(t_yyyymmdd_) == date_to_adf_.end()) {
    std::cerr << "Map does not have data for ADF/HalfLife for day " << t_yyyymmdd_ << '\n';
    dbglogger_ << "Map does not have data for ADF/HalfLife for day " << t_yyyymmdd_ << '\n';
    exit(-1);
  } else {
    curr_day_hlife_ = date_to_hlife_[t_yyyymmdd_];
    curr_day_adf_ = date_to_adf_[t_yyyymmdd_];
    dbglogger_ << " Halflife and ADF are " << curr_day_hlife_ << " " << curr_day_adf_ << " for date " << t_yyyymmdd_
               << '\n';
  }
  SetCurrentDayEarningsGetflat(t_yyyymmdd_);
}

void SpreadTrader::OnRawTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                                   const HFSAT::MarketUpdateInfo& _market_update_info_) {
  uint64_t t_epoch_secs_ = (watch_->tv()).tv_sec;
  if (t_epoch_secs_ - last_bardata_time_ > EOD_SECS_JUMP) {
    seen_today_px_1 = false;
    seen_today_px_2 = false;
    AdjustTimeOnNewDay(t_epoch_secs_);
  }

  if (_security_id_ == inst_1_front_month_sid_ && (!is_expiry_day_ || !has_rolled_over_)) {
    seen_today_px_1 = true;
    last_close_px_1_0_ = _trade_print_info_.trade_price_;
    last_mkt_trade_time_1_0_ = t_epoch_secs_;
    if (!use_adjusted_data_) {
      if (t_epoch_secs_ - last_expiry_check_tm_ > EXPIRY_CHECK_INTERVAL) {
        is_expiry_day_ = IsExpiryToday(t_epoch_secs_, expiry_date_yyyymmdd_);
        last_expiry_check_tm_ = t_epoch_secs_;
      }
    }
  } else if (_security_id_ == inst_1_next_month_sid_) {
    if (is_expiry_day_ && has_rolled_over_) {
      seen_today_px_1 = true;
      last_close_px_1_0_ = _trade_print_info_.trade_price_;
      last_mkt_trade_time_1_0_ = t_epoch_secs_;
    } else {
      last_close_px_1_1_ = _trade_print_info_.trade_price_;
      last_mkt_trade_time_1_1_ = t_epoch_secs_;
    }
  } else if (_security_id_ == inst_2_front_month_sid_ && (!is_expiry_day_ || !has_rolled_over_)) {
    seen_today_px_2 = true;
    last_close_px_2_0_ = _trade_print_info_.trade_price_;
    last_mkt_trade_time_2_0_ = t_epoch_secs_;
  } else if (_security_id_ == inst_2_next_month_sid_) {
    if (is_expiry_day_ && has_rolled_over_) {
      seen_today_px_2 = true;
      last_close_px_2_0_ = _trade_print_info_.trade_price_;
      last_mkt_trade_time_2_0_ = t_epoch_secs_;
    } else {
      last_close_px_2_1_ = _trade_print_info_.trade_price_;
      last_mkt_trade_time_2_1_ = t_epoch_secs_;
    }
  }

  // call processing loop every 1 minute
  if (t_epoch_secs_ - last_bardata_time_ >= NUM_SECONDS_IN_MIN) {
    last_bardata_time_ = t_epoch_secs_;
    // TODO parameterize roll time
    if (!use_adjusted_data_ && is_expiry_day_ && !has_rolled_over_ &&
        last_bardata_time_ - last_mkt_trade_time_1_0_ < STALE_DATA_THRESHOLD &&
        last_bardata_time_ - last_mkt_trade_time_1_1_ < STALE_DATA_THRESHOLD &&
        last_bardata_time_ - last_mkt_trade_time_2_0_ < STALE_DATA_THRESHOLD &&
        last_bardata_time_ - last_mkt_trade_time_2_1_ < STALE_DATA_THRESHOLD &&
        (last_bardata_time_ % 86400 >= param_->midnight_secs_to_roll_)) {
      RollOver();
      return;
    }

    // the front month spread vectors always track front month price
    if (last_bardata_time_ - last_mkt_trade_time_1_0_ < STALE_DATA_THRESHOLD &&
        last_bardata_time_ - last_mkt_trade_time_2_0_ < STALE_DATA_THRESHOLD && seen_today_px_1 && seen_today_px_2) {
      CheckAndUpdateSpreadVector(last_close_px_1_0_, last_close_px_2_0_);
      TradeLogic();
      UpdateMTM();
    }
  }
}

// functional equivalent of OnBarUpdate for HFT data
void SpreadTrader::OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                                const HFSAT::MarketUpdateInfo& _market_update_info_) {}

bool SpreadTrader::IsRelevantInstrument(int inst_id_) {
  if (param_->inst1_id == inst_id_ || param_->inst2_id == inst_id_) return (true);

  return (false);
}

void SpreadTrader::OnBarUpdate(int inst_id_, uint64_t time_, bool is_front_month_, int expiry_date_, double price_,
                               bool more_data_in_bar_) {
  if (time_ - last_bardata_time_ > EOD_SECS_JUMP && SpreadTrader::IsRelevantInstrument(inst_id_)) {
    AdjustTimeOnNewDay(time_);
  }

  if (is_front_month_ && inst_id_ == param_->inst1_id && (!is_expiry_day_ || !has_rolled_over_)) {
    last_close_px_1_0_ = price_;
    last_mkt_trade_time_1_0_ = time_;
    // This hook to identify expiry day can be put in any number of ways.
    if (!use_adjusted_data_) {
      if (time_ - last_expiry_check_tm_ > EXPIRY_CHECK_INTERVAL) {
        is_expiry_day_ = IsExpiryToday(time_, expiry_date_);
        last_expiry_check_tm_ = time_;
      }
    }
  } else if (!is_front_month_ && inst_id_ == param_->inst1_id) {
    if (is_expiry_day_ && has_rolled_over_) {
      last_close_px_1_0_ = price_;
      last_mkt_trade_time_1_0_ = time_;
    } else {
      last_close_px_1_1_ = price_;
      last_mkt_trade_time_1_1_ = time_;
    }
  } else if (is_front_month_ && inst_id_ == param_->inst2_id && (!is_expiry_day_ || !has_rolled_over_)) {
    last_close_px_2_0_ = price_;
    last_mkt_trade_time_2_0_ = time_;
  } else if (!is_front_month_ && inst_id_ == param_->inst2_id) {
    if (is_expiry_day_ && has_rolled_over_) {
      last_close_px_2_0_ = price_;
      last_mkt_trade_time_2_0_ = time_;
    } else {
      last_close_px_2_1_ = price_;
      last_mkt_trade_time_2_1_ = time_;
    }
  }

  if (SpreadTrader::IsRelevantInstrument(inst_id_)) last_bardata_time_ = time_;

  // call processing only when all data in bar is complete - to avoid stale data issues
  // TODO parameterize roll time
  if (!more_data_in_bar_ && last_bardata_time_ == time_) {
    if (!use_adjusted_data_ && is_expiry_day_ && !has_rolled_over_ &&
        last_bardata_time_ - last_mkt_trade_time_1_0_ < STALE_DATA_THRESHOLD &&
        last_bardata_time_ - last_mkt_trade_time_1_1_ < STALE_DATA_THRESHOLD &&
        last_bardata_time_ - last_mkt_trade_time_2_0_ < STALE_DATA_THRESHOLD &&
        last_bardata_time_ - last_mkt_trade_time_2_1_ < STALE_DATA_THRESHOLD &&
        (last_bardata_time_ % 86400 >= param_->midnight_secs_to_roll_)) {
      RollOver();
      return;
    }

    // the front month spread vectors always track front month price
    if (last_bardata_time_ - last_mkt_trade_time_1_0_ < STALE_DATA_THRESHOLD &&
        last_bardata_time_ - last_mkt_trade_time_2_0_ < STALE_DATA_THRESHOLD) {
      CheckAndUpdateSpreadVector(last_close_px_1_0_, last_close_px_2_0_);
      TradeLogic();
      UpdateMTM();
    }
  }
}
}
