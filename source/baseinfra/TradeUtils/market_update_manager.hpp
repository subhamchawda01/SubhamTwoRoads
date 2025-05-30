/**
   \file CommonTradeUtils/market_update_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_COMMONTRADEUTILS_MARKET_UPDATE_MANAGER_H
#define BASE_COMMONTRADEUTILS_MARKET_UPDATE_MANAGER_H

#include <vector>

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "baseinfra/TradeUtils/market_update_manager_listener.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"

#define MIN_MSEC_DURATION 5000
#define AS_MSECS_FROM_MIDNIGHT 21600000
#define EU_MSECS_FROM_MIDNIGHT 43200000

namespace HFSAT {

class MarketUpdateManager : public TimePeriodListener,
                            public SecurityMarketViewChangeListener,
                            public SecurityMarketViewStatusListener {
 protected:
  static MarketUpdateManager *p_unique_instance_;

 public:
  static MarketUpdateManager *GetUniqueInstance(HFSAT::DebugLogger &_dbglogger_, HFSAT::Watch &_watch_,
                                                HFSAT::SecurityNameIndexer &_sec_name_indexer_,
                                                HFSAT::SecurityMarketViewPtrVec &_sid_to_smv_ptr_map_,
                                                int _trading_date_) {
    if (p_unique_instance_ == NULL) {
      p_unique_instance_ =
          new MarketUpdateManager(_dbglogger_, _watch_, _sec_name_indexer_, _sid_to_smv_ptr_map_, _trading_date_);
    }

    return p_unique_instance_;
  }

  ~MarketUpdateManager();

  static void RemoveUniqueInstance() {
    if (NULL != p_unique_instance_) {
      delete p_unique_instance_;
      p_unique_instance_ = NULL;
    }
  }

  void loadOverrideFile(const std::string &);

  void AddMarketDataInterruptedListener(HFSAT::MarketDataInterruptedListener *p_market_data_interrupted_listener_) {
    HFSAT::VectorUtils::UniqueVectorAdd(market_data_interrupt_listener_vec_, p_market_data_interrupted_listener_);
  }

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                    const MarketUpdateInfo &_market_update_info_);
  void OnMarketStatusChange(const unsigned int security_id, const MktStatus_t new_market_status);

  void OnTimePeriodUpdate(const int num_pages_to_add_);

  void start() {
    StartListening();
    check_updates_ = true;
    DBGLOG_TIME_CLASS_FUNC_LINE << " ENABLED" << DBGLOG_ENDL_FLUSH;
  }

  void stop() {
    check_updates_ = false;
    DBGLOG_TIME_CLASS_FUNC_LINE << " DISABLED" << DBGLOG_ENDL_FLUSH;
  }

 private:
  MarketUpdateManager(HFSAT::DebugLogger &_dbglogger_, HFSAT::Watch &_watch_,
                      HFSAT::SecurityNameIndexer &_sec_name_indexer_,
                      HFSAT::SecurityMarketViewPtrVec &_sid_to_smv_ptr_map_, int _trading_date_);

  // attaches itself to sources so that it's callbacks get called last
  void StartListening();

  void notifyMarketDataInterruptedListeners(const unsigned int security_id_, const int msecs_since_last_receive_) {
    for (unsigned int listener_ = 0; listener_ < market_data_interrupt_listener_vec_.size(); ++listener_) {
      // we dont interrupt until is_complex_ready is set for correponding smv
      // it got ready earlier but now interruptted !
      if (security_id_to_smv_[security_id_]->is_ready_complex(2)) {
        market_data_interrupt_listener_vec_[listener_]->OnMarketDataInterrupted(security_id_,
                                                                                msecs_since_last_receive_);
      }
    }
  }

  void notifyMarketDataResumedListeners(const unsigned int security_id_) {
    for (unsigned int listener_ = 0; listener_ < market_data_interrupt_listener_vec_.size(); ++listener_) {
      if (security_id_to_smv_[security_id_]->is_ready_complex(2)) {
        market_data_interrupt_listener_vec_[listener_]->OnMarketDataResumed(security_id_);
      }
    }
  }

  void UpdateMarketUpdateTime();
  void UpdateMarketUpdateTimeAS();

 private:
  HFSAT::DebugLogger &dbglogger_;
  HFSAT::Watch &watch_;
  HFSAT::SecurityNameIndexer &sec_name_indexer_;
  SecurityMarketViewPtrVec &security_id_to_smv_;
  bool using_as_hours_update_time_;  // 1 for AS, 2 for others

  int trading_date_;
  bool check_updates_;

  int *security_id_to_min_msec_market_update_;
  int *security_id_to_last_msec_market_update_;
  bool *security_id_to_data_unavailable_;

  int last_check_msecs_from_midnight_;

  std::vector<HFSAT::MarketDataInterruptedListener *> market_data_interrupt_listener_vec_;
};
}

#endif  // BASE_COMMONTRADEUTILS_MARKET_UPDATE_MANAGER_H
