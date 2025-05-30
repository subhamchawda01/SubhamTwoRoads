/**
   \file CommonTradeUtils/price_volatility_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_VOLATILETRADINGINFO_PRICE_VOLATILITY_MANAGER_H
#define BASE_VOLATILETRADINGINFO_PRICE_VOLATILITY_MANAGER_H

#include <vector>
#include <algorithm>

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

// To compute price movements
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "dvccode/CommonTradeUtils/economic_events_manager.hpp"

#include "baseinfra/TradeUtils/price_volatility_listener.hpp"  // callbacks on price fluctuations

#define PRICE_MOVE_INFO_FILE_NAME "datageninfo/avg_price_moves.txt"
#define TRADE_INFO_FILE_NAME "datageninfo/avg_trade_sizes.txt"
#define PRICE_SWING_FILE_NAME "datageninfo/avg_max_price_move.txt"

#define PRICE_REVERSION_THRESH 0.08

#define PRICE_MOVE_THRESH_MULTIPLIER 3
#define PRICE_MOVE_DURATION_SECS 120
#define PRICE_MOVE_DURATION_MSECS (PRICE_MOVE_DURATION_SECS * 1000)

#define TRADE_MOVE_THRESH_MULTIPLIER 5
#define TRADE_MOVE_DURATION_SECS 30
#define TRADE_MOVE_DURATION_MSECS (TRADE_MOVE_DURATION_SECS * 1000)

#define MAX_PRICE_DOUBLE 100000000.0
#define MIN_PRICE_DOUBLE -100000000.0

// This field specifies what is a valid time frame for price fluctuation checks.
#define PRICE_UPDATE_TIME_FRAME_MSECS 100

#define _DETECT_LARGE_PRICE_CHANGES_ true
#define _DETECT_LARGE_ORDERS_ false
#define _DETECT_LARGE_TRADES_ true
#define _DETECT_PRICE_REVERSIONS_ true

namespace HFSAT {
class PriceVolatilityManager : public SecurityMarketViewChangeListener {
 public:
  static PriceVolatilityManager *GetUniqueInstance(DebugLogger &_dbglogger_, Watch &_watch_,
                                                   SecurityNameIndexer &_sec_name_indexer_,
                                                   SecurityMarketViewPtrVec &_sid_to_smv_ptr_map_, int _trading_date_) {
    static PriceVolatilityManager *p_unique_instance_ = NULL;

    if (!p_unique_instance_) {
      p_unique_instance_ =
          new PriceVolatilityManager(_dbglogger_, _watch_, _sec_name_indexer_, _sid_to_smv_ptr_map_, _trading_date_);
    }

    return p_unique_instance_;
  }

  ~PriceVolatilityManager();

  // Subscribe to economic events related to greater than acceptable
  // price fluctuations / large orders & sizes / directional trades.
  void AddPriceVolatilityEventListener(const unsigned int security_id_,
                                       PriceVolatilityListener *p_price_event_listener_) {
    sid_to_last_price_update_time_.resize(std::max((int)security_id_ + 1, (int)sid_to_last_price_update_time_.size()));
    sid_to_max_price_.resize(std::max((int)security_id_ + 1, (int)sid_to_max_price_.size()));
    sid_to_min_price_.resize(std::max((int)security_id_ + 1, (int)sid_to_min_price_.size()));

    sid_to_day_max_price_.resize(std::max((int)security_id_ + 1, (int)sid_to_day_max_price_.size()));
    sid_to_day_min_price_.resize(std::max((int)security_id_ + 1, (int)sid_to_day_min_price_.size()));
    sid_to_time_to_price_.resize(std::max((int)security_id_ + 1, (int)sid_to_time_to_price_.size()));
    sid_to_price_reversion_start_.resize(std::max((int)security_id_ + 1, (int)sid_to_price_reversion_start_.size()));
    sid_to_price_reversion_peak_.resize(std::max((int)security_id_ + 1, (int)sid_to_price_reversion_peak_.size()));

    sid_to_time_to_trade_.resize(std::max((int)security_id_ + 1, (int)sid_to_time_to_trade_.size()));
    sid_to_last_trade_update_time_.resize(std::max((int)security_id_ + 1, (int)sid_to_last_trade_update_time_.size()));
    sid_to_last_trade_update_msecs_.resize(
        std::max((int)security_id_ + 1, (int)sid_to_last_trade_update_msecs_.size()));
    sid_to_sum_trades_since_last_update_time_.resize(
        std::max((int)security_id_ + 1, (int)sid_to_sum_trades_since_last_update_time_.size()));

    sid_to_large_price_move_periods_.resize(
        std::max((int)security_id_ + 1, (int)sid_to_large_price_move_periods_.size()));
    sid_to_large_trade_periods_.resize(std::max((int)security_id_ + 1, (int)sid_to_large_trade_periods_.size()));
    sid_to_price_reversion_periods_.resize(
        std::max((int)security_id_ + 1, (int)sid_to_price_reversion_periods_.size()));

    sid_to_price_event_listeners_.resize(std::max((int)security_id_ + 1, (int)sid_to_price_event_listeners_.size()));
    sid_to_listener_notified_.resize(std::max((int)security_id_ + 1, (int)sid_to_listener_notified_.size()));
    sid_to_listener_notified_[security_id_] = false;

    sid_to_last_price_update_time_[security_id_] = 0;
    sid_to_max_price_[security_id_] = MIN_PRICE_DOUBLE;
    sid_to_min_price_[security_id_] = MAX_PRICE_DOUBLE;

    sid_to_day_max_price_[security_id_] = MIN_PRICE_DOUBLE;
    sid_to_day_min_price_[security_id_] = MAX_PRICE_DOUBLE;

    VectorUtils::UniqueVectorAdd(sid_to_price_event_listeners_[security_id_], p_price_event_listener_);
    security_id_to_smv_[security_id_]->subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  }

  void RemovePriceVolatilityEventListener(const unsigned int security_id_,
                                          PriceVolatilityListener *p_price_event_listener_) {
    if (sid_to_price_event_listeners_.size() <= security_id_) {
      return;
    }

    std::vector<PriceVolatilityListener *> &t_price_event_listeners_ = sid_to_price_event_listeners_[security_id_];

    VectorUtils::UniqueVectorRemove(t_price_event_listeners_, p_price_event_listener_);
  }

  void NotifyLargePriceChangeListeners(const unsigned int t_security_id_, const double t_price_dev_factor_) {
    if (t_security_id_ >= sid_to_price_event_listeners_.size()) {
      if (dbglogger_.CheckLoggingLevel(PVM_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " No listeners for t_security_id_ = " << (int)t_security_id_
                                    << DBGLOG_ENDL_FLUSH;
      }
    }

    std::vector<PriceVolatilityListener *> &t_price_event_listeners_ = sid_to_price_event_listeners_[t_security_id_];

    if (!sid_to_listener_notified_[t_security_id_]) {
      for (unsigned int t_listener_ = 0; t_listener_ < t_price_event_listeners_.size(); ++t_listener_) {
        t_price_event_listeners_[t_listener_]->OnLargePriceChangeEvent(t_security_id_, t_price_dev_factor_);
      }

      sid_to_listener_notified_[t_security_id_] = true;
    }
  }

  void NotifyNormalPriceListeners(const unsigned int t_security_id_) {
    if (t_security_id_ >= sid_to_price_event_listeners_.size()) {
      if (dbglogger_.CheckLoggingLevel(PVM_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " No listeners for t_security_id_ = " << (int)t_security_id_
                                    << DBGLOG_ENDL_FLUSH;
      }
    }

    std::vector<PriceVolatilityListener *> &t_price_event_listeners_ = sid_to_price_event_listeners_[t_security_id_];

    if (sid_to_listener_notified_[t_security_id_]) {
      for (unsigned int t_listener_ = 0; t_listener_ < t_price_event_listeners_.size(); ++t_listener_) {
        t_price_event_listeners_[t_listener_]->OnNormalPrice(t_security_id_);
      }

      sid_to_listener_notified_[t_security_id_] = false;
    }
  }

  void NotifyLargeOrderSizeListeners(const unsigned int t_security_id_, const int t_level_,
                                     const TradeType_t t_trade_type_, const int t_large_order_size_) {
    if (t_security_id_ >= sid_to_price_event_listeners_.size()) {
      if (dbglogger_.CheckLoggingLevel(PVM_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " No listeners for t_security_id_ = " << (int)t_security_id_
                                    << DBGLOG_ENDL_FLUSH;
      }
    }

    std::vector<PriceVolatilityListener *> &t_price_event_listeners_ = sid_to_price_event_listeners_[t_security_id_];

    if (!sid_to_listener_notified_[t_security_id_]) {
      for (unsigned int t_listener_ = 0; t_listener_ < t_price_event_listeners_.size(); ++t_listener_) {
        t_price_event_listeners_[t_listener_]->OnLargeOrderSizeEvent(t_security_id_, t_level_, t_trade_type_,
                                                                     t_large_order_size_);
      }

      sid_to_listener_notified_[t_security_id_] = true;
    }
  }

  void NotifyNormalBookListeners(const unsigned int t_security_id_) {
    if (t_security_id_ >= sid_to_price_event_listeners_.size()) {
      if (dbglogger_.CheckLoggingLevel(PVM_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " No listeners for t_security_id_ = " << (int)t_security_id_
                                    << DBGLOG_ENDL_FLUSH;
      }
    }

    std::vector<PriceVolatilityListener *> &t_price_event_listeners_ = sid_to_price_event_listeners_[t_security_id_];

    if (sid_to_listener_notified_[t_security_id_]) {
      for (unsigned int t_listener_ = 0; t_listener_ < t_price_event_listeners_.size(); ++t_listener_) {
        t_price_event_listeners_[t_listener_]->OnNormalBook(t_security_id_);
      }

      sid_to_listener_notified_[t_security_id_] = false;
    }
  }

  void SetPriceManager(const bool t_is_price_manager_enabled_) const {
    is_price_manager_enabled_ = t_is_price_manager_enabled_;
  }

  void EnablePriceManager() const { SetPriceManager(true); }
  void DisablePriceManager() const { SetPriceManager(false); }

  // Price change listeners.
  // Compute price change and verify under tolerable threshold.
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                    const MarketUpdateInfo &_market_update_info_);

  std::vector<ttime_t> LargePriceMovePeriodsForSecId(const unsigned int t_security_id_) {
    if (t_security_id_ < sid_to_large_price_move_periods_.size()) {
      return sid_to_large_price_move_periods_[t_security_id_];
    }

    return std::vector<ttime_t>();  // Return empty vector.
  }

  std::vector<ttime_t> PriceReversionPeriodsForSecId(const unsigned int t_security_id_) {
    if (t_security_id_ < sid_to_price_reversion_periods_.size()) {
      if (false) {  // Print out stats for long-period computation of average thresholds.
        if (sid_to_day_max_price_[t_security_id_] > MIN_PRICE_DOUBLE &&
            sid_to_day_min_price_[t_security_id_] < MAX_PRICE_DOUBLE) {
          std::cout << sec_name_indexer_.GetShortcodeFromId(t_security_id_) << " " << trading_date_ << " MAX "
                    << sid_to_day_max_price_[t_security_id_] << " MIN " << sid_to_day_min_price_[t_security_id_]
                    << std::endl;
        }
      }

      return sid_to_price_reversion_periods_[t_security_id_];
    }

    return std::vector<ttime_t>();  // Return empty vector.
  }

  std::vector<ttime_t> LargeTradePeriodsForSecId(const unsigned int t_security_id_) {
    if (t_security_id_ < sid_to_large_trade_periods_.size()) {
      if (false) {
        // Print out stats for long-period computation of average thresholds.
        std::map<ttime_t, int> &t_time_to_trade_ = sid_to_time_to_trade_[t_security_id_];

        double sum_trade_sizes_ = 0.0;
        ttime_t t_end_time_(0, 0);
        for (std::map<ttime_t, int>::iterator _itr_ = t_time_to_trade_.begin(); _itr_ != t_time_to_trade_.end();
             ++_itr_) {
          sum_trade_sizes_ += (_itr_->second);
          t_end_time_ = _itr_->first;
        }

        std::cout << "Start time=" << t_time_to_trade_.begin()->first << " End time=" << t_end_time_ << std::endl;
        ttime_t total_time_ = (t_end_time_ - t_time_to_trade_.begin()->first);
        double total_secs_ = (total_time_.tv_sec + total_time_.tv_usec / 1000000.0);

        sum_trade_sizes_ /= SecurityDefinitions::GetContractMinOrderSize(
            sec_name_indexer_.GetShortcodeFromId(t_security_id_), trading_date_);

        std::cout << "sum_trade_sizes_units_= " << sum_trade_sizes_ << " total_secs_= " << total_secs_ << std::endl;
      }

      return sid_to_large_trade_periods_[t_security_id_];
    }

    return std::vector<ttime_t>();  // Return empty vector.
  }

 private:
  PriceVolatilityManager(DebugLogger &_dbglogger_, Watch &_watch_, SecurityNameIndexer &_sec_name_indexer_,
                         SecurityMarketViewPtrVec &_sid_to_smv_ptr_map_, int _trading_date_);

  void LoadPriceMoves();
  void LoadTradeSizes();
  void LoadMaxPriceSwings();

 private:
  struct L2Info {
   public:
    L2Info() {
      for (unsigned int t_level_ = 0; t_level_ < 2; ++t_level_) {
        bid_sz_[t_level_] = 0;
        bid_order_count_[t_level_] = 0;
        bid_int_px_[t_level_] = 0;
        ask_sz_[t_level_] = 0;
        ask_order_count_[t_level_] = 0;
        ask_int_px_[t_level_] = 0;

        bid_avg_order_size_[t_level_] = 0;
        ask_avg_order_size_[t_level_] = 0;
      }
    }

    L2Info(const SecurityMarketView &t_security_market_view_) {
      for (unsigned int t_level_ = 0; t_level_ < 2; ++t_level_) {
        bid_sz_[t_level_] = t_security_market_view_.bid_size(t_level_);
        bid_order_count_[t_level_] = t_security_market_view_.bid_order(t_level_);
        bid_int_px_[t_level_] = t_security_market_view_.bid_int_price_level(t_level_);

        ask_sz_[t_level_] = t_security_market_view_.ask_size(t_level_);
        ask_order_count_[t_level_] = t_security_market_view_.ask_order(t_level_);
        ask_int_px_[t_level_] = t_security_market_view_.ask_int_price_level(t_level_);

        bid_avg_order_size_[t_level_] = bid_sz_[t_level_] / (bid_order_count_[t_level_] + 1);
        ask_avg_order_size_[t_level_] = ask_sz_[t_level_] / (ask_order_count_[t_level_] + 1);
      }
    }

    int bid_sz_[2];
    int bid_order_count_[2];
    int bid_int_px_[2];
    int ask_int_px_[2];
    int ask_order_count_[2];
    int ask_sz_[2];

    double bid_avg_order_size_[2];
    double ask_avg_order_size_[2];
  };

  // Time weighted trade impact.
  struct L1TradeInfo {
    double bid_agg_trade_impact_;  // Aggressors on bid.
    double ask_agg_trade_impact_;  // Aggressors on ask.

    int cum_agg_trades_;

    int cum_bid_agg_trades_;
    int cum_ask_agg_trades_;
  };

 private:
  DebugLogger &dbglogger_;
  Watch &watch_;
  SecurityNameIndexer &sec_name_indexer_;
  SecurityMarketViewPtrVec &security_id_to_smv_;

  int trading_date_;

  mutable bool is_price_manager_enabled_;

  // This is the tolerable price swing for each security.
  std::vector<double> sid_to_max_price_change_;

  std::vector<int> sid_to_last_price_update_time_;
  std::vector<double> sid_to_max_price_;
  std::vector<double> sid_to_min_price_;

  std::map<unsigned int, ttime_t> sid_to_bad_data_time_;
  std::vector<double> sid_to_day_max_price_;
  std::vector<double> sid_to_day_min_price_;
  std::vector<std::map<ttime_t, double> > sid_to_time_to_price_;

  std::vector<double> sid_to_max_price_swing_;
  std::vector<ttime_t> sid_to_price_reversion_start_;
  std::vector<ttime_t> sid_to_price_reversion_peak_;

  std::vector<std::map<ttime_t, int> > sid_to_time_to_trade_;
  std::vector<double> sid_to_max_trade_;
  std::vector<ttime_t> sid_to_last_trade_update_time_;
  std::vector<int> sid_to_last_trade_update_msecs_;
  std::vector<int> sid_to_sum_trades_since_last_update_time_;

  // Useless is live , useful in historical ,
  // in conj. with smv_analyser.
  std::vector<std::vector<ttime_t> > sid_to_large_price_move_periods_;
  std::vector<std::vector<ttime_t> > sid_to_large_trade_periods_;
  std::vector<std::vector<ttime_t> > sid_to_price_reversion_periods_;

  // To detect large and/or directional agg. trades.
  std::vector<L1TradeInfo> sid_to_l1tradeinfo_;

  // To detect addition of very large orders.
  std::vector<L2Info> sid_to_l2info_;

  // Maintain a list of all listeners to be notified in case of
  // a price fluctuation that will trigger us to take action.
  std::vector<std::vector<PriceVolatilityListener *> > sid_to_price_event_listeners_;
  std::vector<bool> sid_to_listener_notified_;
};
}

#endif  // BASE_VOLATILETRADINGINFO_PRICE_VOLATILITY_MANAGER_H
