#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/asx_itch_logged_message_filesource.hpp"

namespace HFSAT {

ASXItchLoggedMessageFileSource::ASXItchLoggedMessageFileSource(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
    bool t_use_fake_faster_data_)
    : ExternalDataListener(),
      dbglogger_(t_dbglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      p_time_keeper_(NULL),
      bulk_file_reader_(),
      next_event_(),
      trading_location_file_read_(r_trading_location_),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      trade_time_manager_(HFSAT::TradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_preevent_YYYYMMDD_)),
      has_trading_hours_started_(false),
      has_trading_hours_closed_(false),
      use_fake_faster_data_(t_use_fake_faster_data_),
      asx_listener_vec_(),
      YYYYMMDD_(t_preevent_YYYYMMDD_) {
  is_spread_ = sec_name_indexer_.GetShortcodeFromId(security_id_).substr(0, 3) == "SP_";
  shortcode_ = sec_name_indexer_.GetShortcodeFromId(security_id_);
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;
  start_timestamp_.tv_sec = 0;
  start_timestamp_.tv_usec = 0;
  end_timestamp_.tv_sec = 0;
  end_timestamp_.tv_usec = 0;
  // find the filename
  std::string asx_file;
  if (HFSAT::HybridSecurityManager::IsHybridExch(std::string(t_exchange_symbol_))) {
    asx_file = CommonLoggedMessageFileNamer::GetOrderFeedFilename(
        kExchSourceASX,
        (HFSAT::HybridSecurityManager::GetActualExchFromHybrid(std::string(t_exchange_symbol_))).c_str(),
        t_preevent_YYYYMMDD_, trading_location_file_read_);
  } else {
    asx_file = CommonLoggedMessageFileNamer::GetOrderFeedFilename(kExchSourceASX, t_exchange_symbol_,
                                                                  t_preevent_YYYYMMDD_, trading_location_file_read_);
  }

  HandleExpiryWeek(asx_file);
  int added_delay = 0;
  // added delays is used by the robustness check script to variate line delay by a constant number
  added_delay = TradingLocationUtils::GetAddedDelay(trading_location_file_read_, r_trading_location_);
  delay_usecs_to_add_ = added_delay;

  if (trading_location_file_read_ != r_trading_location_) {
    delay_usecs_to_add_ +=
        TradingLocationUtils::GetUSecsBetweenTradingLocations(trading_location_file_read_, r_trading_location_);
  }
  if (delay_usecs_to_add_ != 0) need_to_add_delay_usecs_ = true;

  if (TradingLocationUtils::GetUsecsForFasterDataForShortcodeAtLocation(
          sec_name_indexer_.GetShortcodeFromId(security_id_), r_trading_location_, t_preevent_YYYYMMDD_,
          use_fake_faster_data_) !=
      0) {  // Experiment with the possibility of using faster data for this product at this location.
    need_to_add_delay_usecs_ = true;
    delay_usecs_to_add_ -= TradingLocationUtils::GetUsecsForFasterDataForShortcodeAtLocation(
        sec_name_indexer_.GetShortcodeFromId(security_id_), r_trading_location_, t_preevent_YYYYMMDD_,
        use_fake_faster_data_);
  }

  DBGLOG_CLASS_FUNC << "For ASX symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                    << " trading_location " << trading_location_file_read_ << " DUsecs " << delay_usecs_to_add_
                    << " @use " << r_trading_location_ << " returned filename = " << asx_file << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(asx_file);
  } else {
    std::cerr << "For ASX symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << asx_file << std::endl;
  }
}

void ASXItchLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  // keep reading the next_event_
  // to check if next_event_timestamp_
  // if greater than r_endtime_
  if (bulk_file_reader_.is_open()) {
    if (next_event_timestamp_ == ttime_t(0, 0)) {
      size_t available_len = bulk_file_reader_.read(&next_event_, sizeof(ASX_ITCH_MDS::ASXItchOrder));

      // data not found in file
      if (available_len < sizeof(ASX_ITCH_MDS::ASXItchOrder)) {
        rw_hasevents_ = false;
      } else {
        next_event_timestamp_ = next_event_.time;

        if (need_to_add_delay_usecs_) {
          next_event_timestamp_.addusecs(delay_usecs_to_add_);
        }
      }
    }
  }
  // data file not open
  else {
    rw_hasevents_ = false;
  }
}

void ASXItchLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(ASX_ITCH_MDS::ASXItchOrder));
    if (available_len_ < sizeof(ASX_ITCH_MDS::ASXItchOrder)) {  // data not found in file
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      rw_hasevents_ = false;
    } else {
      next_event_timestamp_ =
          next_event_.time;  // need more work here to optimize, or preferably send ttime_t from MDS or ORS
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  } else {
    rw_hasevents_ = false;
  }
}

TradeType_t ASXItchLoggedMessageFileSource::GetTradeType(char type) {
  switch (type) {
    case 'B': {
      return kTradeTypeBuy;
      break;
    }
    case 'S': {
      return kTradeTypeSell;
      break;
    }
    default: {
      return kTradeTypeNoInfo;
      break;
    }
  }
}

TradeType_t ASXItchLoggedMessageFileSource::GetTradeTypeOpposite(char type) {
  switch (GetTradeType(type)) {
    case kTradeTypeBuy: {
      return kTradeTypeSell;
      break;
    }
    case kTradeTypeSell: {
      return kTradeTypeBuy;
      break;
    }
    default: {
      return kTradeTypeNoInfo;
      break;
    }
  }
}

inline void ASXItchLoggedMessageFileSource::ProcessThisMsg() {
  // Skip msg if product break time
  // if (!IsNormalTradeTime(security_id_, next_event_timestamp_)) return;
  NotifyExternalDataListenerListener(security_id_);
  p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

  switch (next_event_.msg_type) {
    case ASX_ITCH_MDS::kASXAdd: {
      NotifyOrderAdd(security_id_, GetTradeType(next_event_.add.side), next_event_.add.order_id,
                     next_event_.add.priority, next_event_.add.price, next_event_.add.size);
      break;
    }
    case ASX_ITCH_MDS::kASXVolDel: {
      NotifyVolumeDelete(security_id_, GetTradeType(next_event_.vol_del.side), next_event_.vol_del.order_id,
                         next_event_.vol_del.size);
      break;
    }
    case ASX_ITCH_MDS::kASXDelete: {
      NotifyOrderDelete(security_id_, GetTradeType(next_event_.del.side), next_event_.del.order_id);
      break;
    }
    case ASX_ITCH_MDS::kASXExec: {
      // Notify with opposite sides since we reverse the sides while logging ASX data (as ASX provides passive side, not
      // aggressive)
      NotifyOrderExec(security_id_, GetTradeType(next_event_.exec.side), next_event_.exec.order_id,
                      next_event_.exec.exec_price, next_event_.exec.size_exec, next_event_.exec.size_remaining);
      break;
    }
    case ASX_ITCH_MDS::kASXExecPx: {
      NotifyOrderExecWithPx(security_id_, next_event_.exec_px.order_id, next_event_.exec_px.opposite_order_id,
                            next_event_.exec_px.exec_price, next_event_.exec_px.size_exec,
                            next_event_.exec_px.size_remaining);
      break;
    }
    case ASX_ITCH_MDS::kASXImpliedAdd: {
      if (is_spread_) {
        NotifyOrderAdd(security_id_, GetTradeType(next_event_.add.side), next_event_.add.order_id,
                       next_event_.add.priority, next_event_.add.price, next_event_.add.size);
      }
      break;
    }

    case ASX_ITCH_MDS::kASXImpliedDelete: {
      if (is_spread_) {
        NotifyOrderDelete(security_id_, GetTradeType(next_event_.del.side), next_event_.del.order_id);
      }
      break;
    }
    case ASX_ITCH_MDS::kASXImpliedReplaced: {
      if (is_spread_) {
        NotifyOrderModify(security_id_, GetTradeType(next_event_.add.side), next_event_.add.order_id,
                          next_event_.add.priority, next_event_.add.price, next_event_.add.size);
      }
      break;
    }
    case ASX_ITCH_MDS::kASXCombExec: {
      NotifyOrderExec(security_id_, GetTradeType(next_event_.sp_exec.side), next_event_.sp_exec.order_id,
                      next_event_.sp_exec.exec_price, next_event_.sp_exec.size_exec,
                      next_event_.sp_exec.size_remaining);
      break;
    }

    case ASX_ITCH_MDS::kASXMktStatus:
      break;
    case ASX_ITCH_MDS::kASXResetBegin: {
      NotifyResetBook(security_id_);
      break;
    }
    case ASX_ITCH_MDS::kASXResetEnd:
    case ASX_ITCH_MDS::kASXRecoveryComplete: {
      // Not implemented right now
      break;
    }
    default: {
      std::cerr << "Unknown msg_type in ASX: " << next_event_.msg_type << std::endl;
      break;
    }
  }
}

inline bool ASXItchLoggedMessageFileSource::SetNextTimeStamp() {
  // read the next_event_
  // set next_event_timestamp_
  register size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(ASX_ITCH_MDS::ASXItchOrder));
  if (available_len_ < sizeof(ASX_ITCH_MDS::ASXItchOrder)) {
    next_event_timestamp_ = ttime_t(time_t(0), 0);
    return false;
  } else {
    next_event_timestamp_ = next_event_.time;
    if (need_to_add_delay_usecs_) {
      next_event_timestamp_.addusecs(delay_usecs_to_add_);
    }
    return true;
  }
  return true;
}

void ASXItchLoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    ProcessThisMsg();

    if (!SetNextTimeStamp()) return;
  }
}

void ASXItchLoggedMessageFileSource::ProcessEventsTill(const ttime_t endtime) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= endtime) {
    ProcessThisMsg();

    if (!SetNextTimeStamp()) return;
  }
}

void ASXItchLoggedMessageFileSource::SetTimeToSkipUntilFirstEvent(const ttime_t start_time) {}

void ASXItchLoggedMessageFileSource::AddASXListener(OrderLevelListener* listener) {
  if (listener != NULL) {
    VectorUtils::UniqueVectorAdd(asx_listener_vec_, listener);
  }
}

void ASXItchLoggedMessageFileSource::NotifyOrderAdd(const uint32_t security_id, const TradeType_t buysell,
                                                    const uint64_t order_id, const uint32_t priority,
                                                    const double price, const uint32_t size) {
  for (auto listener : asx_listener_vec_) {
    listener->OnOrderAdd(security_id, buysell, order_id, priority, price, size);
  }
}
void ASXItchLoggedMessageFileSource::NotifyOrderModify(const uint32_t security_id, const TradeType_t buysell,
                                                       const uint64_t order_id, const uint32_t priority,
                                                       const double price, const uint32_t size) {
  for (auto listener : asx_listener_vec_) {
    listener->OnOrderModify(security_id, buysell, order_id, priority, price, size);
  }
}
void ASXItchLoggedMessageFileSource::NotifyVolumeDelete(const uint32_t security_id, const TradeType_t buysell,
                                                        const uint64_t order_id, const uint32_t size) {
  for (auto listener : asx_listener_vec_) {
    listener->OnVolumeDelete(security_id, buysell, order_id, size);
  }
}
void ASXItchLoggedMessageFileSource::NotifyOrderDelete(const uint32_t security_id, const TradeType_t buysell,
                                                       const uint64_t order_id) {
  for (auto listener : asx_listener_vec_) {
    listener->OnOrderDelete(security_id, buysell, order_id);
  }
}
void ASXItchLoggedMessageFileSource::NotifyOrderExec(const uint32_t security_id, const TradeType_t buysell,
                                                     const uint64_t order_id, const double exec_price,
                                                     const uint32_t size_exec, const uint32_t size_remaining) {
  for (auto listener : asx_listener_vec_) {
    listener->OnOrderExec(security_id, buysell, order_id, exec_price, size_exec, size_remaining);
  }
}
void ASXItchLoggedMessageFileSource::NotifyOrderExecWithPx(const uint32_t security_id, const uint64_t order_id,
                                                           const uint64_t opposite_order_id, const double exec_price,
                                                           const uint32_t size_exec, const uint32_t size_remaining) {
  for (auto listener : asx_listener_vec_) {
    // 0 is passed in ask_remaining as the new ASX feed only provides size remaining after March 2017
    listener->OnOrderExecWithPx(security_id, order_id, opposite_order_id, exec_price, size_exec, size_remaining, 0);
  }
}
void ASXItchLoggedMessageFileSource::NotifyOrderSpExec(const uint32_t security_id, const TradeType_t buysell,
                                                       const uint64_t order_id, const double exec_price,
                                                       const uint32_t size_exec, const uint32_t size_remaining) {
  for (auto listener : asx_listener_vec_) {
    listener->OnOrderSpExec(security_id, buysell, order_id, exec_price, size_exec, size_remaining);
  }
}
void ASXItchLoggedMessageFileSource::NotifyResetBook(const unsigned int security_id) {
  for (auto listener : asx_listener_vec_) {
    listener->ResetBook(security_id);
  }
}

void ASXItchLoggedMessageFileSource::HandleExpiryWeek(std::string& asx_file) {
	  int c_month_ = ((YYYYMMDD_ / 100) % 100);
	  if (c_month_ % 3 == 0) {
	    int start_date_ = DateTime::CalcNextWeekDay(100 * (YYYYMMDD_ / 100) + 7);  // 8th or next business day
	    int end_date_ = DateTime::CalcNextWeekDay(100 * (YYYYMMDD_ / 100) + 14);   // 15th or next business day

	    if (YYYYMMDD_ < 20171201 && (shortcode_.substr(0, 3) == "XTE" || shortcode_.substr(0, 3) == "YTE")) {
	      if (YYYYMMDD_ < start_date_ || YYYYMMDD_ > end_date_) {
	        asx_file += "_INVALID_INTENTIONAL";
	      } else {
	        if (YYYYMMDD_ == start_date_) {
	          start_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(start_date_, "AST_1645");
	        } else if (YYYYMMDD_ == end_date_) {
	          end_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(end_date_, "AST_1645");
	        }
	      }
	    } else if (YYYYMMDD_ >= 20171201 && shortcode_.substr(0,3) == "XTE") {
	       if (YYYYMMDD_ < start_date_ || YYYYMMDD_ > end_date_) {
	         asx_file += "_INVALID_INTENTIONAL";
	       } else {
	         if (YYYYMMDD_ == start_date_) {
	          start_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(start_date_, "AST_1645");
	        } else if (YYYYMMDD_ == end_date_) {
	          end_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(end_date_, "AST_1645");
	        }
	      }
	    } else if (YYYYMMDD_ >= 20171201 && shortcode_.substr(0,3) == "YTE") {
	          asx_file += "_INVALID_INTENTIONAL";
	    } else if (shortcode_.substr(0, 2) == "XT") {
	        if (YYYYMMDD_ > start_date_ && YYYYMMDD_ < end_date_) {
	        asx_file += "_INVALID_INTENTIONAL";
	      } else {
	        if (YYYYMMDD_ == start_date_) {
	          end_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(start_date_, "AST_1645");
	        } else if (YYYYMMDD_ == end_date_) {
	          start_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(end_date_, "AST_1645");
	        }
	      }
	    } else if (YYYYMMDD_ < 20171201 && shortcode_.substr(0, 2) == "YT") {
	        if (YYYYMMDD_ > start_date_ && YYYYMMDD_ < end_date_) {
	        asx_file += "_INVALID_INTENTIONAL";
	      } else {
	        if (YYYYMMDD_ == start_date_) {
	          end_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(start_date_, "AST_1645");
	        } else if (YYYYMMDD_ == end_date_) {
	          start_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(end_date_, "AST_1645");
	        }
	      }
	    } else if (YYYYMMDD_ >= 20171201 && shortcode_.substr(0, 2) == "YT") {
	    	if (YYYYMMDD_ == start_date_) {
	    	  end_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(start_date_, "AST_1645");
	    	} else if (YYYYMMDD_ == end_date_) {
	    	    start_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(end_date_, "AST_1645");
	    	}
	    }
	  } else {
	    if (shortcode_.substr(0, 3) == "XTE" || shortcode_.substr(0, 3) == "YTE") {
	      asx_file += "_INVALID_INTENTIONAL";
	    }
	  }
}
}
