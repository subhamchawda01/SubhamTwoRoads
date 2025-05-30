/**
   \file CommonTradeUtilsCode/price_volatility_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "baseinfra/TradeUtils/price_volatility_manager.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

namespace HFSAT {
PriceVolatilityManager::PriceVolatilityManager(HFSAT::DebugLogger &_dbglogger_, HFSAT::Watch &_watch_,
                                               HFSAT::SecurityNameIndexer &_sec_name_indexer_,
                                               HFSAT::SecurityMarketViewPtrVec &_sid_to_smv_ptr_map_,
                                               int _trading_date_)
    : dbglogger_(_dbglogger_),
      watch_(_watch_),
      sec_name_indexer_(_sec_name_indexer_),
      security_id_to_smv_(_sid_to_smv_ptr_map_),

      trading_date_(_trading_date_),

      is_price_manager_enabled_(false),

      sid_to_max_price_change_(),
      sid_to_last_price_update_time_(),
      sid_to_max_price_(),
      sid_to_min_price_(),

      sid_to_bad_data_time_(),
      sid_to_day_max_price_(),
      sid_to_day_min_price_(),
      sid_to_time_to_price_(),

      sid_to_max_price_swing_(),
      sid_to_price_reversion_start_(),
      sid_to_price_reversion_peak_(),

      sid_to_time_to_trade_(),
      sid_to_max_trade_(),
      sid_to_last_trade_update_time_(),
      sid_to_last_trade_update_msecs_(),
      sid_to_sum_trades_since_last_update_time_(),

      sid_to_large_price_move_periods_(),
      sid_to_large_trade_periods_(),
      sid_to_price_reversion_periods_(),

      sid_to_l1tradeinfo_(),
      sid_to_l2info_(),

      sid_to_price_event_listeners_(),
      sid_to_listener_notified_() {
#if _DETECT_LARGE_PRICE_CHANGES_
  LoadPriceMoves();
#endif
#if _DETECT_LARGE_TRADES_
  LoadTradeSizes();
#endif
#if _DETECT_PRICE_REVERSIONS_
  LoadMaxPriceSwings();
#endif
}

PriceVolatilityManager::~PriceVolatilityManager() {}

// Price change listeners.
// Compute price change and verify under tolerable threshold.
// Currently treating price of an instrument as its mkt_price_
void PriceVolatilityManager::OnMarketUpdate(const unsigned int _security_id_,
                                            const MarketUpdateInfo &_market_update_info_) {
  if (watch_.msecs_from_midnight() - sid_to_last_price_update_time_[_security_id_] < PRICE_UPDATE_TIME_FRAME_MSECS ||
      !is_price_manager_enabled_) {
    // It has not been the minimum time frame since last update
    return;
  }

#if _DETECT_LARGE_ORDERS_
  {  // CHECK FOR VERY LARGE ORDERS ON L1 / L2.
    SecurityMarketView &market_view_ = *security_id_to_smv_[_security_id_];

    if (market_view_.IsBidBookL2() && market_view_.IsAskBookL2()) {
      L2Info &l2_info_ = sid_to_l2info_[_security_id_];
      L2Info new_l2_info_(market_view_);

      bool is_book_normal_ = true;

      for (unsigned int t_level_ = 0; t_level_ < 2; ++t_level_) {
        {  // (i) Huge incremental order added / removed.
          const int approx_bid_order_size_ =
              (new_l2_info_.bid_sz_[t_level_] - l2_info_.bid_sz_[t_level_]) /
              (abs(new_l2_info_.bid_order_count_[t_level_] - l2_info_.bid_order_count_[t_level_]));

          const int approx_ask_order_size_ =
              (new_l2_info_.ask_sz_[t_level_] - l2_info_.ask_sz_[t_level_]) /
              (abs(new_l2_info_.ask_order_count_[t_level_] - l2_info_.ask_order_count_[t_level_]));

          // TODO : Read these sizes from a file per product.
          // Currently only for DOL.
          if (approx_bid_order_size_ > 2000) {
            NotifyLargeOrderSizeListeners(_security_id_, t_level_, kTradeTypeBuy, approx_bid_order_size_);
            is_book_normal_ = false;
            break;
          }
          if (approx_ask_order_size_ > 2000) {
            NotifyLargeOrderSizeListeners(_security_id_, t_level_, kTradeTypeSell, approx_ask_order_size_);
            is_book_normal_ = false;
            break;
          }
        }

        {  // Compute moving avg. order_sizes.

          // TODO : Again, these factors will differ on a per product basis.
          // Currently only for DOL.
          if (new_l2_info_.bid_avg_order_size_[t_level_] > 20 * l2_info_.bid_avg_order_size_[t_level_]) {
            NotifyLargeOrderSizeListeners(_security_id_, t_level_, kTradeTypeBuy,
                                          new_l2_info_.bid_avg_order_size_[t_level_]);
            is_book_normal_ = false;
            break;
          }

          if (new_l2_info_.ask_avg_order_size_[t_level_] > 20 * l2_info_.ask_avg_order_size_[t_level_]) {
            NotifyLargeOrderSizeListeners(_security_id_, t_level_, kTradeTypeBuy,
                                          new_l2_info_.ask_avg_order_size_[t_level_]);
            is_book_normal_ = false;
            break;
          }

          // See what would be good weights to use here.
          new_l2_info_.bid_avg_order_size_[t_level_] =
              l2_info_.bid_avg_order_size_[t_level_] * 0.7 + new_l2_info_.bid_avg_order_size_[t_level_] * 0.3;
          new_l2_info_.ask_avg_order_size_[t_level_] =
              l2_info_.ask_avg_order_size_[t_level_] * 0.7 + new_l2_info_.ask_avg_order_size_[t_level_] * 0.3;
        }
      }

      sid_to_l2info_[_security_id_] = new_l2_info_;

      if (is_book_normal_) {
        NotifyNormalBookListeners(_security_id_);
      }
    }
  }
#endif

  double current_price_ = security_id_to_smv_[_security_id_]->mkt_size_weighted_price();

#if _DETECT_LARGE_PRICE_CHANGES_
  if ((watch_.msecs_from_midnight() - sid_to_last_price_update_time_[_security_id_]) >
      PRICE_MOVE_DURATION_MSECS) {  // CHECK FOR ABNORMAL PRICE MOVES.
    if (sid_to_last_price_update_time_[_security_id_] == 0) {
      // First time this was called, update time and return.
      sid_to_max_price_[_security_id_] = MIN_PRICE_DOUBLE;
      sid_to_min_price_[_security_id_] = MAX_PRICE_DOUBLE;
      sid_to_last_price_update_time_[_security_id_] = watch_.msecs_from_midnight();
      return;
    }

    if (false) {
      const double t_price_change_ = fabs(sid_to_max_price_[_security_id_] - sid_to_min_price_[_security_id_]) /
                                     HFSAT::SecurityDefinitions::GetContractMinPriceIncrement(
                                         sec_name_indexer_.GetShortcodeFromId(_security_id_), trading_date_);

      DBGLOG_TIME_CLASS_FUNC_LINE << " mfm=" << watch_.msecs_from_midnight()
                                  << " lastmfm=" << sid_to_last_price_update_time_[_security_id_]
                                  << " max=" << sid_to_max_price_[_security_id_]
                                  << " min=" << sid_to_min_price_[_security_id_] << " pxchange=" << t_price_change_
                                  << " thresh=" << sid_to_max_price_change_[_security_id_]
                                  << ((t_price_change_ > sid_to_max_price_change_[_security_id_]) ? " LPM "
                                                                                                  : " NON-LPM ")
                                  << " lpm periods #=" << sid_to_large_price_move_periods_[_security_id_].size()
                                  << DBGLOG_ENDL_FLUSH;
    }

    if (sid_to_max_price_[_security_id_] > MIN_PRICE_DOUBLE && sid_to_min_price_[_security_id_] < MAX_PRICE_DOUBLE) {
      const double t_price_change_ = fabs(sid_to_max_price_[_security_id_] - sid_to_min_price_[_security_id_]) /
                                     HFSAT::SecurityDefinitions::GetContractMinPriceIncrement(
                                         sec_name_indexer_.GetShortcodeFromId(_security_id_), trading_date_);
      if (t_price_change_ > sid_to_max_price_change_[_security_id_]) {
        NotifyLargePriceChangeListeners(_security_id_,
                                        (t_price_change_ / sid_to_max_price_change_[_security_id_] + 0.0000001));

        sid_to_large_price_move_periods_[_security_id_].push_back(ttime_t(watch_.tv() - ttime_t(120, 0)));
        sid_to_large_price_move_periods_[_security_id_].push_back(watch_.tv());
      } else {
        NotifyNormalPriceListeners(_security_id_);
      }
    }

    sid_to_max_price_[_security_id_] = MIN_PRICE_DOUBLE;
    sid_to_min_price_[_security_id_] = MAX_PRICE_DOUBLE;
    sid_to_last_price_update_time_[_security_id_] = watch_.msecs_from_midnight();
  } else {
    sid_to_max_price_[_security_id_] = std::max(sid_to_max_price_[_security_id_], current_price_);
    sid_to_min_price_[_security_id_] = std::min(sid_to_min_price_[_security_id_], current_price_);
  }
#endif

#if _DETECT_PRICE_REVERSIONS_
  if (sid_to_bad_data_time_.find(_security_id_) == sid_to_bad_data_time_.end()) {
    sid_to_bad_data_time_[_security_id_] = watch_.tv() + ttime_t(10 * 60, 0);
  }

  if (sid_to_bad_data_time_[_security_id_] < watch_.tv()) {
    sid_to_day_max_price_[_security_id_] = std::max(sid_to_day_max_price_[_security_id_], current_price_);
    sid_to_day_min_price_[_security_id_] = std::min(sid_to_day_min_price_[_security_id_], current_price_);

    sid_to_time_to_price_[_security_id_][watch_.tv()] = current_price_;

    // Some flow logic.
    if (sid_to_price_reversion_start_[_security_id_] == ttime_t(0, 0)) {
      sid_to_price_reversion_start_[_security_id_] = watch_.tv();
      sid_to_price_reversion_peak_[_security_id_] = watch_.tv();
    } else {
      const double t_start_price_ = sid_to_time_to_price_[_security_id_][sid_to_price_reversion_start_[_security_id_]];
      const double t_peak_price_ = sid_to_time_to_price_[_security_id_][sid_to_price_reversion_peak_[_security_id_]];

      bool is_moving_up_ = (t_peak_price_ > (t_start_price_ + 0.000001));
      bool is_moving_down_ = ((t_peak_price_ + 0.000001) < t_start_price_);

      // std::cout << watch_.tv ( ) << " px " << current_price_
      // 	      << " start_@ " << sid_to_price_reversion_start_ [ _security_id_ ] << "=" << t_start_price_
      // 	      << " peak_@ " << sid_to_price_reversion_peak_ [ _security_id_ ] << "=" << t_peak_price_
      // 	      << " is_moving_up_=" << ( is_moving_up_ ? "true" : "false" )
      // 	      << std::endl;

      if (sid_to_price_reversion_start_[_security_id_] == sid_to_price_reversion_peak_[_security_id_]) {
        sid_to_price_reversion_peak_[_security_id_] = watch_.tv();
      } else if ((is_moving_up_ && (current_price_ > t_peak_price_)) ||
                 (is_moving_down_ && (current_price_ < t_peak_price_))) {
        sid_to_price_reversion_peak_[_security_id_] = watch_.tv();
      } else if (fabs(current_price_ - t_peak_price_) >
                 PRICE_REVERSION_THRESH * sid_to_max_price_swing_[_security_id_]) {
        sid_to_price_reversion_periods_[_security_id_].push_back(sid_to_price_reversion_peak_[_security_id_] -
                                                                 ttime_t(30, 0));
        sid_to_price_reversion_periods_[_security_id_].push_back(sid_to_price_reversion_peak_[_security_id_] +
                                                                 ttime_t(30, 0));

        // std::cout << "PR "
        // 	  << "[ " << sid_to_price_reversion_start_ [ _security_id_ ] << " - " << sid_to_price_reversion_peak_ [
        // _security_id_ ] << " - " << watch_.tv ( ) << " ]\n"
        // 	  << "PR "
        // 	  << "[ " << t_start_price_ << " - " << t_peak_price_ << " - " << current_price_ << " ]"
        // 	  << std::endl;

        sid_to_price_reversion_start_[_security_id_] = watch_.tv();
        sid_to_price_reversion_peak_[_security_id_] = watch_.tv();
      }
    }
  }
#endif
}

void PriceVolatilityManager::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                                          const MarketUpdateInfo &_market_update_info_) {
#if _DETECT_LARGE_TRADES_
  if (_security_id_ <
      sid_to_time_to_trade_.size()) {  // Only if someone actually bothered to subscribe to this feature.
    sid_to_time_to_trade_[_security_id_][watch_.tv()] += _trade_print_info_.size_traded_;
    sid_to_sum_trades_since_last_update_time_[_security_id_] += _trade_print_info_.size_traded_;

    if (watch_.msecs_from_midnight() - sid_to_last_trade_update_msecs_[_security_id_] >
        TRADE_MOVE_DURATION_MSECS) {                              // Check for LARGE TRADES
      if (sid_to_last_trade_update_msecs_[_security_id_] == 0) {  // First time.
        sid_to_last_trade_update_msecs_[_security_id_] = watch_.msecs_from_midnight();
        sid_to_last_trade_update_time_[_security_id_] = watch_.tv();

        OnMarketUpdate(_security_id_, _market_update_info_);
        return;
      }

      // TODO ... trivial improvement by caching in constructor ... horribly lazy code
      int t_contract_min_order_size_ = HFSAT::SecurityDefinitions::GetContractMinOrderSize(
          sec_name_indexer_.GetShortcodeFromId(_security_id_), trading_date_);

      if (sid_to_sum_trades_since_last_update_time_[_security_id_] / t_contract_min_order_size_ >
          sid_to_max_trade_[_security_id_]) {
        sid_to_large_trade_periods_[_security_id_].push_back(sid_to_last_trade_update_time_[_security_id_]);
        sid_to_large_trade_periods_[_security_id_].push_back(watch_.tv());
      }

      sid_to_sum_trades_since_last_update_time_[_security_id_] = 0;
      sid_to_last_trade_update_msecs_[_security_id_] = watch_.msecs_from_midnight();
      sid_to_last_trade_update_time_[_security_id_] = watch_.tv();
    }
  }
#endif
  OnMarketUpdate(_security_id_, _market_update_info_);
}

void PriceVolatilityManager::LoadPriceMoves() {
  // This will push default values for securities we don't find move info. for.
  sid_to_max_price_change_.resize(sec_name_indexer_.NumSecurityId(), 1000.0);

  std::ifstream price_move_info_;

  std::string price_move_info_filename_ = "";

  {
    std::ostringstream t_oss_;
    t_oss_ << std::string(BASETRADEINFODIR) + std::string(PRICE_MOVE_INFO_FILE_NAME);
    price_move_info_filename_ = t_oss_.str();
  }

  price_move_info_.open(price_move_info_filename_.c_str(), std::ifstream::in);

  if (!price_move_info_.is_open()) {
    if (dbglogger_.CheckLoggingLevel(PVM_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << " Could not open valid price_move_info_file=" << price_move_info_filename_
                                  << DBGLOG_ENDL_FLUSH;
      return;
    }
  }

  char line_[100];
  memset(line_, 0, sizeof(line_));

  // Read in price moves for instruments we are interested in.
  while (price_move_info_.getline(line_, sizeof(line_))) {
    const char *p_shortcode_ = strtok(line_, " \t");
    if (sec_name_indexer_.HasString(p_shortcode_)) {  // Only add information if we are interested in this security.
      const char *p_move_ = strtok(NULL, " \t\n");

      if (!p_move_) {
        if (dbglogger_.CheckLoggingLevel(PVM_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << "Malformed line for security : " << p_shortcode_ << DBGLOG_ENDL_FLUSH;
        }

        continue;
      }

      const unsigned int security_id_ = sec_name_indexer_.GetIdFromString(p_shortcode_);

      // WARNING : Errors in atof are not being detected.
      double t_avg_price_moves_per_sec_ = fabs(atof(p_move_));

      sid_to_max_price_change_[security_id_] =
          PRICE_MOVE_THRESH_MULTIPLIER * sqrt(t_avg_price_moves_per_sec_ * PRICE_MOVE_DURATION_SECS);

      if (dbglogger_.CheckLoggingLevel(PVM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << "Shortcode=" << p_shortcode_ << " Security Id=" << security_id_
                                    << " AvgPriceMovesPerSec=" << t_avg_price_moves_per_sec_
                                    << " PRICE_MOVE_THRESH_MULTIPLIER=" << PRICE_MOVE_THRESH_MULTIPLIER
                                    << " PRICE_MOVE_DURATION_SECS=" << PRICE_MOVE_DURATION_SECS
                                    << " PriceMoveThreshFactor=" << sid_to_max_price_change_[security_id_]
                                    << DBGLOG_ENDL_FLUSH;
      }
    }

    memset(line_, 0, sizeof(line_));
  }

  price_move_info_.close();
}

void PriceVolatilityManager::LoadTradeSizes() {
  // This will push default values for securities we don't find move info. for.
  sid_to_max_trade_.resize(sec_name_indexer_.NumSecurityId(), 10000.0);

  std::ifstream trade_size_info_;

  std::string trade_size_info_filename_ = "";

  {
    std::ostringstream t_oss_;
    t_oss_ << std::string(BASETRADEINFODIR) + std::string(TRADE_INFO_FILE_NAME);
    trade_size_info_filename_ = t_oss_.str();
  }

  trade_size_info_.open(trade_size_info_filename_.c_str(), std::ifstream::in);

  if (!trade_size_info_.is_open()) {
    if (dbglogger_.CheckLoggingLevel(PVM_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << " Could not open valid trade_size_info_file=" << trade_size_info_filename_
                                  << DBGLOG_ENDL_FLUSH;
      return;
    }
  }

  char line_[100];
  memset(line_, 0, sizeof(line_));

  // Read in trade sizes for instruments we are interested in.
  while (trade_size_info_.getline(line_, sizeof(line_))) {
    const char *p_shortcode_ = strtok(line_, " \t");
    if (sec_name_indexer_.HasString(p_shortcode_)) {  // Only add information if we are interested in this security.
      const char *p_size_ = strtok(NULL, " \t\n");

      if (!p_size_) {
        if (dbglogger_.CheckLoggingLevel(PVM_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << "Malformed line for security : " << p_shortcode_ << DBGLOG_ENDL_FLUSH;
        }

        continue;
      }

      const unsigned int security_id_ = sec_name_indexer_.GetIdFromString(p_shortcode_);

      // WARNING : Errors in atof are not being detected.
      double t_avg_trade_size_per_sec_ = fabs(atof(p_size_));

      sid_to_max_trade_[security_id_] =
          TRADE_MOVE_THRESH_MULTIPLIER * (t_avg_trade_size_per_sec_ * TRADE_MOVE_DURATION_SECS);

      if (dbglogger_.CheckLoggingLevel(PVM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << "Shortcode=" << p_shortcode_ << " Security Id=" << security_id_
                                    << " AvgTradeSizePerSec=" << t_avg_trade_size_per_sec_
                                    << " TRADE_MOVE_THRESH_MULTIPLIER=" << TRADE_MOVE_THRESH_MULTIPLIER
                                    << " TRADE_MOVE_DURATION_SECS=" << TRADE_MOVE_DURATION_SECS
                                    << " TradeMoveThreshFactor=" << sid_to_max_trade_[security_id_]
                                    << DBGLOG_ENDL_FLUSH;
      }
    }

    memset(line_, 0, sizeof(line_));
  }

  trade_size_info_.close();
}

void PriceVolatilityManager::LoadMaxPriceSwings() {
  // This will push default values for securities we don't find move info. for.
  sid_to_max_price_swing_.resize(sec_name_indexer_.NumSecurityId(), 10000.0);

  std::ifstream price_swing_info_;

  std::string price_swing_info_filename_ = "";

  {
    std::ostringstream t_oss_;
    t_oss_ << std::string(BASETRADEINFODIR) + std::string(PRICE_SWING_FILE_NAME);
    price_swing_info_filename_ = t_oss_.str();
  }

  price_swing_info_.open(price_swing_info_filename_.c_str(), std::ifstream::in);

  if (!price_swing_info_.is_open()) {
    if (dbglogger_.CheckLoggingLevel(PVM_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << " Could not open valid price_swing_info_file=" << price_swing_info_filename_
                                  << DBGLOG_ENDL_FLUSH;
      return;
    }
  }

  char line_[100];
  memset(line_, 0, sizeof(line_));

  // Read in price swings for instruments we are interested in.
  while (price_swing_info_.getline(line_, sizeof(line_))) {
    const char *p_shortcode_ = strtok(line_, " \t");
    if (sec_name_indexer_.HasString(p_shortcode_)) {  // Only add information if we are interested in this security.
      const char *p_swing_ = strtok(NULL, " \t\n");

      if (!p_swing_) {
        if (dbglogger_.CheckLoggingLevel(PVM_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << "Malformed line for security : " << p_shortcode_ << DBGLOG_ENDL_FLUSH;
        }

        continue;
      }

      const unsigned int security_id_ = sec_name_indexer_.GetIdFromString(p_shortcode_);

      // WARNING : Errors in atof are not being detected.
      double t_price_swing_ = fabs(atof(p_swing_));

      sid_to_max_price_swing_[security_id_] = t_price_swing_;

      if (dbglogger_.CheckLoggingLevel(PVM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << "Shortcode=" << p_shortcode_ << " Security Id=" << security_id_
                                    << " MaxIntradayPriceSwing=" << t_price_swing_
                                    << " ThreshForPriceReversion=" << PRICE_REVERSION_THRESH << DBGLOG_ENDL_FLUSH;
      }
    }

    memset(line_, 0, sizeof(line_));
  }

  price_swing_info_.close();
}
}
