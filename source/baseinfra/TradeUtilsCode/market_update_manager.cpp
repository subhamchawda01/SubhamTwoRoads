/**
   \file CommonTradeUtilsCode/market_update_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "baseinfra/TradeUtils/market_update_manager.hpp"

namespace HFSAT {

MarketUpdateManager *MarketUpdateManager::p_unique_instance_ = NULL;

MarketUpdateManager::MarketUpdateManager(HFSAT::DebugLogger &_dbglogger_, HFSAT::Watch &_watch_,
                                         HFSAT::SecurityNameIndexer &_sec_name_indexer_,
                                         HFSAT::SecurityMarketViewPtrVec &_sid_to_smv_ptr_map_, int _trading_date_)
    : dbglogger_(_dbglogger_),
      watch_(_watch_),
      sec_name_indexer_(_sec_name_indexer_),
      security_id_to_smv_(_sid_to_smv_ptr_map_),
      using_as_hours_update_time_(false),

      trading_date_(_trading_date_),
      check_updates_(false),

      security_id_to_min_msec_market_update_(NULL),
      security_id_to_last_msec_market_update_(NULL),
      security_id_to_data_unavailable_(NULL),

      last_check_msecs_from_midnight_(0),

      market_data_interrupt_listener_vec_() {
  security_id_to_min_msec_market_update_ = new int[security_id_to_smv_.size()];
  security_id_to_last_msec_market_update_ = new int[security_id_to_smv_.size()];
  security_id_to_data_unavailable_ = new bool[security_id_to_smv_.size()];

  memset(security_id_to_min_msec_market_update_, 0, security_id_to_smv_.size() * sizeof(int));
  memset(security_id_to_last_msec_market_update_, 0, security_id_to_smv_.size() * sizeof(int));
  memset(security_id_to_data_unavailable_, 0, security_id_to_smv_.size() * sizeof(bool));

  if (watch_.msecs_from_midnight() < AS_MSECS_FROM_MIDNIGHT) {
    UpdateMarketUpdateTimeAS();
  } else {
    UpdateMarketUpdateTime();
  }
}

void MarketUpdateManager::StartListening() {
  for (unsigned int security_id_ = 0; security_id_ < security_id_to_smv_.size(); ++security_id_) {
    if (security_id_to_smv_[security_id_]) {
      security_id_to_smv_[security_id_]->subscribe_L1_Only(this);
      security_id_to_smv_[security_id_]->subscribe_MktStatus(this);
    }
  }

  watch_.subscribe_BigTimePeriod(this);
}

MarketUpdateManager::~MarketUpdateManager() {
  delete[] security_id_to_min_msec_market_update_;
  delete[] security_id_to_last_msec_market_update_;
  delete[] security_id_to_data_unavailable_;
}

void MarketUpdateManager::OnMarketUpdate(const unsigned int _security_id_,
                                         const MarketUpdateInfo &_market_update_info_) {
  if (!check_updates_) {
    return;
  }

  if (!security_id_to_smv_[_security_id_] ||
      security_id_to_min_msec_market_update_[_security_id_] == -1) {  // No smv for this security/not interested.
    return;
  }

  security_id_to_last_msec_market_update_[_security_id_] = watch_.msecs_from_midnight();

  if (security_id_to_data_unavailable_[_security_id_]) {
    if (security_id_to_smv_[_security_id_]->is_ready_complex(2)) {
      security_id_to_data_unavailable_[_security_id_] = false;
      notifyMarketDataResumedListeners(_security_id_);
      if (dbglogger_.CheckLoggingLevel(MUM_INFO)) {
        dbglogger_ << "Shortcode : " << sec_name_indexer_.GetShortcodeFromId(_security_id_)
                   << " MktDataResumed : msecs : " << security_id_to_last_msec_market_update_[_security_id_]
                   << DBGLOG_ENDL_FLUSH;
      }
    }
  }
}

void MarketUpdateManager::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                                       const MarketUpdateInfo &_market_update_info_) {
  OnMarketUpdate(_security_id_, _market_update_info_);
}

void MarketUpdateManager::OnMarketStatusChange(const unsigned int _security_id_,
                                               const MktStatus_t _new_market_status_) {
  const int msecs_from_midnight_ = watch_.msecs_from_midnight();
  int msecs_since_last_receive_ = (security_id_to_last_msec_market_update_[_security_id_] == 0)
                                      ? 0
                                      : (msecs_from_midnight_ - security_id_to_last_msec_market_update_[_security_id_]);

  if (_new_market_status_ != kMktTradingStatusOpen) {
    notifyMarketDataInterruptedListeners(_security_id_, msecs_since_last_receive_);
  } else {
    notifyMarketDataResumedListeners(_security_id_);
  }
}

void MarketUpdateManager::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (!check_updates_) {
    return;
  }

  const int msecs_from_midnight_ = watch_.msecs_from_midnight();

  // we can remove this probably for we anyway introduce artficial break if the avg_l1 goes up significantly ?
  if (using_as_hours_update_time_ && (msecs_from_midnight_ >= AS_MSECS_FROM_MIDNIGHT)) {
    UpdateMarketUpdateTime();
  }

  if (msecs_from_midnight_ > last_check_msecs_from_midnight_ + MIN_MSEC_DURATION) {
    last_check_msecs_from_midnight_ = msecs_from_midnight_;
  } else {
    return;
  }

  // Check times against limits and notify listeners if needed.
  for (unsigned int security_id_ = 0; security_id_ < security_id_to_smv_.size(); ++security_id_) {
    if (!security_id_to_smv_[security_id_] ||
        security_id_to_min_msec_market_update_[security_id_] == -1) {  // No smv for this security/not interested.
      continue;
    }

    int msecs_since_last_receive_ =
        (security_id_to_last_msec_market_update_[security_id_] == 0)
            ? 0
            : (msecs_from_midnight_ - security_id_to_last_msec_market_update_[security_id_]);
    if (!security_id_to_data_unavailable_[security_id_] &&
        msecs_since_last_receive_ > security_id_to_min_msec_market_update_[security_id_]) {
      notifyMarketDataInterruptedListeners(security_id_, msecs_since_last_receive_);
      security_id_to_data_unavailable_[security_id_] = true;

      if (dbglogger_.CheckLoggingLevel(MUM_INFO)) {
        dbglogger_ << "Shortcode : " << sec_name_indexer_.GetShortcodeFromId(security_id_)
                   << " MktDataInterrupted : msecs " << msecs_from_midnight_
                   << " msecs_since_last_receive_ : " << msecs_since_last_receive_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }
}

void MarketUpdateManager::UpdateMarketUpdateTimeAS() {
  for (unsigned int security_id_ = 0; security_id_ < security_id_to_smv_.size(); ++security_id_) {
    if (security_id_to_smv_[security_id_]) {
      // Set up default values of msecs_between_market_updates.
      std::string shortcode_ = sec_name_indexer_.GetShortcodeFromId(security_id_);

      // Currently only adding these instruments, others use 900000.
      // 6E, ES, FESX, FDAX, 6C, 6M 6N, 6A, 6B

      int min_msecs_between_updates_ = -1;  // Default 900 secs.
      if (!shortcode_.compare(0, 2, "6E") || !shortcode_.compare(0, 2, "ES")) {
        min_msecs_between_updates_ = 360000;
      } else if (!shortcode_.compare(0, 2, "ZF") || !shortcode_.compare(0, 2, "ZN") ||
                 !shortcode_.compare(0, 2, "ZB")) {
        min_msecs_between_updates_ = 720000;
      } else if (!shortcode_.compare(0, 2, "ZT")) {
        min_msecs_between_updates_ = -1;
      } else if (!shortcode_.compare(0, 2, "6C") || !shortcode_.compare(0, 2, "6M") ||
                 !shortcode_.compare(0, 2, "6N") || !shortcode_.compare(0, 2, "6A") ||
                 !shortcode_.compare(0, 2, "6B")) {
        min_msecs_between_updates_ = 2160000;  // Increased from 120000
      } else if (!shortcode_.compare(0, 3, "NK_") || !shortcode_.compare(0, 4, "NKM_")) {
        // Till a more elegant solution of non-core indicators has been put in ,
        // using this to deal with OSE - HK timing issues.
        min_msecs_between_updates_ = -1;
      } else if (!shortcode_.compare(0, 4, "NKD_") || !shortcode_.compare(0, 4, "NIY_")) {
        min_msecs_between_updates_ = -1;
      }

      if ((min_msecs_between_updates_ < 100) && (min_msecs_between_updates_ != -1)) {
        min_msecs_between_updates_ = 900000;
      }
      security_id_to_min_msec_market_update_[security_id_] = min_msecs_between_updates_;

      if (dbglogger_.CheckLoggingLevel(MUM_INFO)) {
        dbglogger_ << "Shortcode : " << shortcode_
                   << " Min_msecs_between_updates : " << security_id_to_min_msec_market_update_[security_id_]
                   << DBGLOG_ENDL_FLUSH;
      }
    }
  }
  using_as_hours_update_time_ = true;
}

void MarketUpdateManager::UpdateMarketUpdateTime() {
  for (unsigned int security_id_ = 0; security_id_ < security_id_to_smv_.size(); ++security_id_) {
    if (security_id_to_smv_[security_id_]) {
      // Set up default values of msecs_between_market_updates.
      std::string shortcode_ = sec_name_indexer_.GetShortcodeFromId(security_id_);

      // Currently only adding these instruments, others use 900000.
      // 6E, ES, FESX, FDAX, 6C, 6M 6N, 6A, 6B

      int min_msecs_between_updates_ = -1;  // Default 900 secs.
      if (!shortcode_.compare(0, 4, "FESX") || !shortcode_.compare(0, 4, "FDAX") || !shortcode_.compare(0, 4, "FDXM")) {
        min_msecs_between_updates_ = 90000;
      } else if (!shortcode_.compare(0, 4, "FGBM") || !shortcode_.compare(0, 4, "FGBL")) {
        min_msecs_between_updates_ = 100000;  // changed from 60000
      } else if (!shortcode_.compare(0, 4, "FGBS") || !shortcode_.compare(0, 4, "FGBX")) {
        min_msecs_between_updates_ = -1;  // changed from 300000
      } else if (!shortcode_.compare(0, 2, "6E") || !shortcode_.compare(0, 2, "ES")) {
        min_msecs_between_updates_ = 60000;
      } else if (!shortcode_.compare(0, 2, "ZF") || !shortcode_.compare(0, 2, "ZN") ||
                 !shortcode_.compare(0, 2, "ZB")) {
        min_msecs_between_updates_ = 120000;
      } else if (!shortcode_.compare(0, 2, "ZT")) {
        min_msecs_between_updates_ = -1;
      } else if (!shortcode_.compare(0, 2, "6C") || !shortcode_.compare(0, 2, "6M") ||
                 !shortcode_.compare(0, 2, "6N") || !shortcode_.compare(0, 2, "6A") ||
                 !shortcode_.compare(0, 2, "6B")) {
        min_msecs_between_updates_ = 360000;  // Increased from 120000
      } else if (!shortcode_.compare(0, 6, "BR_IND") || !shortcode_.compare(0, 6, "BR_WIN")) {
        min_msecs_between_updates_ = 200000;
      } else if (!shortcode_.compare(0, 6, "BR_DOL") ||
                 !shortcode_.compare(0, 6, "BR_WDO")) {  // dol just isn't what it used to be.
        min_msecs_between_updates_ = 200000;
      } else if (!shortcode_.compare(0, 3, "BAX")) {
        min_msecs_between_updates_ = -1;  // Ignore all BAX products
      } else if (!shortcode_.compare(0, 3, "CGF") || !shortcode_.compare(0, 3, "CGZ")) {
        min_msecs_between_updates_ = -1;  // Ignore all CG[FZ] products
      } else if (!shortcode_.compare(0, 6, "DI1F16") || !shortcode_.compare(0, 6, "DI1F17") ||
                 !shortcode_.compare(0, 6, "DI1F21")) {
        // Ignore all other DI products
        min_msecs_between_updates_ = 600000;
      } else if (!shortcode_.compare(0, 3, "LFL") || !shortcode_.compare(0, 3, "LFI") ||
                 !shortcode_.compare(0, 4, "FBTS") || !shortcode_.compare(0, 4, "FOAM")) {
        min_msecs_between_updates_ = -1;  // Ignore all LFL/LFI products
      } else if (!shortcode_.compare(0, 4, "FOAT") || !shortcode_.compare(0, 4, "FBTP") ||
                 !shortcode_.compare(0, 4, "FBON")) {
        min_msecs_between_updates_ = 900000;  // Super slow products
      } else if (!shortcode_.compare(0, 5, "YFEBM") || !shortcode_.compare(0, 3, "XFC") ||
                 !shortcode_.compare(0, 4, "XFRC")) {
        min_msecs_between_updates_ = -1;  // Slow Liffe commodities
      } else if (!shortcode_.compare(0, 3, "FVS")) {
        min_msecs_between_updates_ = -1;  // Ignore all FVS contracts
      } else if (!shortcode_.compare(0, 3, "NK_") || !shortcode_.compare(0, 4, "NKM_")) {
        // Till a more elegant solution of non-core indicators has been put in ,
        // using this to deal with OSE - HK timing issues.
        min_msecs_between_updates_ = -1;
      } else if (!shortcode_.compare(0, 4, "NKD_") || !shortcode_.compare(0, 4, "NIY_")) {
        min_msecs_between_updates_ = -1;
      } else if (SecurityDefinitions::GetContractExchSource(shortcode_, 0) == kExchSourceBMFEQ) {
        min_msecs_between_updates_ = -1;
      }

      if ((min_msecs_between_updates_ < 100) && (min_msecs_between_updates_ != -1)) {
        min_msecs_between_updates_ = 900000;
      }
      security_id_to_min_msec_market_update_[security_id_] = min_msecs_between_updates_;

      if (dbglogger_.CheckLoggingLevel(MUM_INFO)) {
        dbglogger_ << "Shortcode : " << shortcode_
                   << " Min_msecs_between_updates : " << security_id_to_min_msec_market_update_[security_id_]
                   << DBGLOG_ENDL_FLUSH;
      }
    }
  }
  using_as_hours_update_time_ = false;
}
}
