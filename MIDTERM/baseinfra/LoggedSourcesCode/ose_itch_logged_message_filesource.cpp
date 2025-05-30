#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/ose_itch_logged_message_filesource.hpp"

namespace HFSAT {

OSEItchLoggedMessageFileSource::OSEItchLoggedMessageFileSource(
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
      next_event_old_(),
      trading_location_file_read_(r_trading_location_),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      has_trading_hours_started_(false),
      has_trading_hours_closed_(false),
      use_fake_faster_data_(t_use_fake_faster_data_),
      trade_time_manager_(HFSAT::TradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_preevent_YYYYMMDD_)),
      ose_listener_vec_global_(),
      ose_listener_vec_() {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  use_old_struct_ = t_preevent_YYYYMMDD_ < USE_NEW_STRUCT_FROM;

  // find the filename
  std::string t_ose_filename_ =
      OSELoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);

  int added_delay = 0;
  // added delays is used by the robustness check script to variate line delay by a constant number
  added_delay = TradingLocationUtils::GetAddedDelay(trading_location_file_read_, r_trading_location_);
  delay_usecs_to_add_ = added_delay;

  if (trading_location_file_read_ != r_trading_location_) {
    delay_usecs_to_add_ +=
        TradingLocationUtils::GetMSecsBetweenTradingLocations(trading_location_file_read_, r_trading_location_) * 1000;
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

  // Open file with BulkFileReader
  if (t_ose_filename_.find("NO_FILE_AVAILABLE") == std::string::npos) {
    bulk_file_reader_.open(t_ose_filename_);
  } else {
    DBGLOG_CLASS_FUNC << "For OSE symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                      << " trading_location " << trading_location_file_read_
                      << " returned filename = " << t_ose_filename_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    std::cerr << "For OSE symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_ose_filename_ << std::endl;
  }
  // bulk_file_reader_.open ( t_ose_filename_ ) ;
}

void OSEItchLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  // keep reading the next_event_
  // to check if next_event_timestamp_
  // if greater than r_endtime_
  if (bulk_file_reader_.is_open()) {
    if (next_event_timestamp_ == ttime_t(0, 0)) {
      size_t available_len, struct_len;
      if (use_old_struct_) {
        available_len = bulk_file_reader_.read(&next_event_old_, sizeof(OSE_ITCH_MDS::OSECommonStructOld));
        struct_len = sizeof(OSE_ITCH_MDS::OSECommonStructOld);
      } else {
        available_len = bulk_file_reader_.read(&next_event_, sizeof(OSE_ITCH_MDS::OSECommonStruct));
        struct_len = sizeof(OSE_ITCH_MDS::OSECommonStruct);
      }

      // data not found in file
      if (available_len < struct_len) {
        rw_hasevents_ = false;
      } else {
        next_event_timestamp_ = use_old_struct_ ? next_event_old_.time_ : next_event_.time_;

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

void OSEItchLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
                                      // set next_event_timestamp_
    size_t available_len, struct_len;
    if (use_old_struct_) {
      available_len = bulk_file_reader_.read(&next_event_old_, sizeof(OSE_ITCH_MDS::OSECommonStructOld));
      struct_len = sizeof(OSE_ITCH_MDS::OSECommonStructOld);
    } else {
      available_len = bulk_file_reader_.read(&next_event_, sizeof(OSE_ITCH_MDS::OSECommonStruct));
      struct_len = sizeof(OSE_ITCH_MDS::OSECommonStruct);
    }

    if (available_len < struct_len) {                 // data not found in file
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      rw_hasevents_ = false;
    } else {
      // need more work here to optimize, or preferably send ttime_t from MDS or ORS
      next_event_timestamp_ = use_old_struct_ ? next_event_old_.time_ : next_event_.time_;

      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  } else {
    rw_hasevents_ = false;
  }
}

TradeType_t OSEItchLoggedMessageFileSource::GetTradeType(char type) {
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

TradeType_t OSEItchLoggedMessageFileSource::GetTradeTypeOpposite(char type) {
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

char OSEItchLoggedMessageFileSource::GetTradeTypeOppositeChar(char type) {
  switch (type) {
    case 'B': {
      return 'S';
      break;
    }
    case 'S': {
      return 'B';
      break;
    }
    default: {
      return '-';
      break;
    }
  }
}

inline void OSEItchLoggedMessageFileSource::ProcessThisMsg() {
  NotifyExternalDataListenerListener(security_id_);
  p_time_keeper_->OnTimeReceived(next_event_timestamp_);

  if (use_old_struct_) {
    switch (next_event_old_.msg_type) {
      case OSE_ITCH_MDS::kOSEAdd: {
        if (next_event_old_.data.add.price <= -200000) {
          return;
        }
        order_price_map_[next_event_old_.data.add.order_id] = next_event_old_.data.add.price;
        order_size_map_[next_event_old_.data.add.order_id] = next_event_old_.data.add.size;
        NotifyOrderAdd(security_id_, next_event_old_.data.add.side, next_event_old_.data.add.order_id,
                       next_event_old_.data.add.priority, next_event_old_.data.add.price,
                       next_event_old_.data.add.size);
        break;
      }
      case OSE_ITCH_MDS::kOSEDelete: {
        if (order_price_map_.find(next_event_old_.data.del.order_id) != order_price_map_.end()) {
          order_price_map_.erase(next_event_old_.data.del.order_id);
          order_size_map_.erase(next_event_old_.data.del.order_id);
        }
        NotifyOrderDelete(security_id_, next_event_old_.data.del.side, next_event_old_.data.del.order_id);
        break;
      }
      case OSE_ITCH_MDS::kOSEExec: {
        // Fetch price from order maps
        double trade_px = 0.0;
        uint32_t size_remaining = 0;
        if (order_price_map_.find(next_event_old_.data.exec.order_id) != order_price_map_.end()) {
          trade_px = order_price_map_[next_event_old_.data.exec.order_id];
          if (order_size_map_[next_event_old_.data.exec.order_id] >= next_event_old_.data.exec.size_exec) {
            order_size_map_[next_event_old_.data.exec.order_id] -= next_event_old_.data.exec.size_exec;
          } else {
            // std::cerr << "Trade not possible: " << next_event_.data.exec.order_id << " " <<
            // next_event_.data.exec.size_exec << " " << order_size_map_[next_event_.data.exec.order_id] << "\n";
          }
          size_remaining = order_size_map_[next_event_old_.data.exec.order_id];
        } else {
          // std::cerr << "Trade not possible2: " << next_event_.data.exec.order_id << " " <<
          // next_event_.data.exec.size_exec << " " << "\n";
        }
        NotifyOrderExec(security_id_, GetTradeTypeOppositeChar(next_event_old_.data.exec.side),
                        next_event_old_.data.exec.order_id, trade_px, next_event_old_.data.exec.size_exec,
                        size_remaining);
        break;
      }
      case OSE_ITCH_MDS::kOSEExecWithTrade: {
        // Notify with opposite sides since we reverse the sides while logging OSE data (as OSE provides passive side,
        // not
        // aggressive)
        uint32_t size_remaining = 0;
        auto size_itr = order_size_map_.find(next_event_old_.data.exec_info.order_id);
        if (size_itr != order_size_map_.end()) {
          if (size_itr->second >= next_event_old_.data.exec_info.size_exec) {
            size_itr->second -= next_event_old_.data.exec_info.size_exec;
          } else {
            // std::cerr << "Trade not possible3: " << next_event_.data.exec_info.order_id << " " <<
            // next_event_.data.exec_info.size_exec << " " << order_size_map_[next_event_.data.exec_info.order_id] <<
            // "\n";
          }
          size_remaining = size_itr->second;
        } else {
          // std::cerr << "Trade not possible4: " << next_event_.data.exec_info.order_id << " " <<
          // next_event_.data.exec_info.size_exec << " " << "\n";
        }
        NotifyOrderExecWithTradeInfo(security_id_, GetTradeTypeOppositeChar(next_event_old_.data.exec_info.side),
                                     next_event_old_.data.exec_info.order_id, next_event_old_.data.exec_info.price,
                                     next_event_old_.data.exec_info.size_exec, size_remaining);
        break;
      }
      case OSE_ITCH_MDS::kOSETradingStatus: {
        std::string status = next_event_old_.data.status.state;
        NotifyTradingStatus(security_id_, status);
        break;
      }
      case OSE_ITCH_MDS::kOSEResetBegin: {
        NotifyResetBook(security_id_);
        break;
      }
      case OSE_ITCH_MDS::kOSEEquilibriumPrice:
      case OSE_ITCH_MDS::kOSEResetEnd:
      case OSE_ITCH_MDS::kOSEPriceNotification: {
        // Not implemented right now
        break;
      }
      default: {
        std::cerr << "Unknown msg_type in OSE: " << next_event_.msg_type << std::endl;
        break;
      }
    }
  } else {
    switch (next_event_.msg_type) {
      case OSE_ITCH_MDS::kOSEAdd: {
        if (next_event_.data.add.price <= -200000) {
          return;
        }
        order_price_map_[next_event_.data.add.order_id] = next_event_.data.add.price;
        order_size_map_[next_event_.data.add.order_id] = next_event_.data.add.size;
        NotifyOrderAdd(security_id_, next_event_.data.add.side, next_event_.data.add.order_id,
                       next_event_.data.add.priority, next_event_.data.add.price, next_event_.data.add.size,
                       next_event_.intermediate);
        break;
      }
      case OSE_ITCH_MDS::kOSEDelete: {
        if (order_price_map_.find(next_event_.data.del.order_id) != order_price_map_.end()) {
          order_price_map_.erase(next_event_.data.del.order_id);
          order_size_map_.erase(next_event_.data.del.order_id);
        }
        NotifyOrderDelete(security_id_, next_event_.data.del.side, next_event_.data.del.order_id,
                          next_event_.intermediate);
        break;
      }
      case OSE_ITCH_MDS::kOSEExec: {
        // Fetch price from order maps
        double trade_px = 0.0;
        uint32_t size_remaining = 0;
        if (order_price_map_.find(next_event_.data.exec.order_id) != order_price_map_.end()) {
          trade_px = order_price_map_[next_event_.data.exec.order_id];
          if (order_size_map_[next_event_.data.exec.order_id] >= next_event_.data.exec.size_exec) {
            order_size_map_[next_event_.data.exec.order_id] -= next_event_.data.exec.size_exec;
          } else {
            // std::cerr << "Trade not possible: " << next_event_.data.exec.order_id << " " <<
            // next_event_.data.exec.size_exec << " " << order_size_map_[next_event_.data.exec.order_id] << "\n";
          }
          size_remaining = order_size_map_[next_event_.data.exec.order_id];
        } else {
          // std::cerr << "Trade not possible2: " << next_event_.data.exec.order_id << " " <<
          // next_event_.data.exec.size_exec << " " << "\n";
        }
        NotifyOrderExec(security_id_, GetTradeTypeOppositeChar(next_event_.data.exec.side),
                        next_event_.data.exec.order_id, trade_px, next_event_.data.exec.size_exec, size_remaining,
                        next_event_.intermediate);
        break;
      }
      case OSE_ITCH_MDS::kOSEExecWithTrade: {
        // Notify with opposite sides since we reverse the sides while logging OSE data (as OSE provides passive side,
        // not
        // aggressive)
        uint32_t size_remaining = 0;
        auto size_itr = order_size_map_.find(next_event_.data.exec_info.order_id);
        if (size_itr != order_size_map_.end()) {
          if (size_itr->second >= next_event_.data.exec_info.size_exec) {
            size_itr->second -= next_event_.data.exec_info.size_exec;
          } else {
            // std::cerr << "Trade not possible3: " << next_event_.data.exec_info.order_id << " " <<
            // next_event_.data.exec_info.size_exec << " " << order_size_map_[next_event_.data.exec_info.order_id] <<
            // "\n";
          }
          size_remaining = size_itr->second;
        } else {
          // std::cerr << "Trade not possible4: " << next_event_.data.exec_info.order_id << " " <<
          // next_event_.data.exec_info.size_exec << " " << "\n";
        }
        NotifyOrderExecWithTradeInfo(security_id_, GetTradeTypeOppositeChar(next_event_.data.exec_info.side),
                                     next_event_.data.exec_info.order_id, next_event_.data.exec_info.price,
                                     next_event_.data.exec_info.size_exec, size_remaining);
        break;
      }
      case OSE_ITCH_MDS::kOSETradingStatus: {
        std::string status = next_event_.data.status.state;
        NotifyTradingStatus(security_id_, status);
        break;
      }
      case OSE_ITCH_MDS::kOSEResetBegin: {
        NotifyResetBook(security_id_);
        break;
      }
      case OSE_ITCH_MDS::kOSEEquilibriumPrice:
      case OSE_ITCH_MDS::kOSEResetEnd:
      case OSE_ITCH_MDS::kOSEPriceNotification: {
        // Not implemented right now
        break;
      }
      default: {
        std::cerr << "Unknown msg_type in OSE: " << next_event_.msg_type << std::endl;
        break;
      }
    }
  }
}

inline bool OSEItchLoggedMessageFileSource::SetNextTimeStamp() {
  // read the next_event_
  // set next_event_timestamp_
  register size_t available_len, struct_len;
  if (use_old_struct_) {
    available_len = bulk_file_reader_.read(&next_event_old_, sizeof(OSE_ITCH_MDS::OSECommonStructOld));
    struct_len = sizeof(OSE_ITCH_MDS::OSECommonStructOld);
  } else {
    available_len = bulk_file_reader_.read(&next_event_, sizeof(OSE_ITCH_MDS::OSECommonStruct));
    struct_len = sizeof(OSE_ITCH_MDS::OSECommonStruct);
  }

  if (available_len < struct_len) {
    next_event_timestamp_ = ttime_t(time_t(0), 0);
    return false;
  } else {
    next_event_timestamp_ = use_old_struct_ ? next_event_old_.time_ : next_event_.time_;
    if (need_to_add_delay_usecs_) {
      next_event_timestamp_.addusecs(delay_usecs_to_add_);
    }
    return true;
  }
  return true;
}

void OSEItchLoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    ProcessThisMsg();

    if (!SetNextTimeStamp()) return;
  }
}

void OSEItchLoggedMessageFileSource::ProcessEventsTill(const ttime_t endtime) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= endtime) {
    ProcessThisMsg();

    if (!SetNextTimeStamp()) return;
  }
}

void OSEItchLoggedMessageFileSource::SetTimeToSkipUntilFirstEvent(const ttime_t start_time) {}

void OSEItchLoggedMessageFileSource::AddOSEListener(OrderFeedGlobalListener* listener) {
  if (listener != NULL) {
    VectorUtils::UniqueVectorAdd(ose_listener_vec_global_, listener);
  }
}

void OSEItchLoggedMessageFileSource::AddOSEListener(OrderLevelListener* listener) {
  if (listener != NULL) {
    VectorUtils::UniqueVectorAdd(ose_listener_vec_, listener);
  }
}

void OSEItchLoggedMessageFileSource::NotifyOrderAdd(const uint32_t security_id, const uint8_t side,
                                                    const uint64_t order_id, const uint32_t priority,
                                                    const double price, const uint32_t size, bool intermediate) {
  // For OrderFeedGlobalListener
  for (auto listener : ose_listener_vec_global_) {
    listener->OnOrderAdd(security_id, order_id, side, price, size, priority, intermediate);
  }

  // For OrderLevelListener
  for (auto listener : ose_listener_vec_) {
    listener->OnOrderAdd(security_id, GetTradeType(side), order_id, priority, price, size);
  }
}
void OSEItchLoggedMessageFileSource::NotifyOrderDelete(const uint32_t security_id, const uint8_t side,
                                                       const uint64_t order_id, bool intermediate) {
  // For OrderFeedGlobalListener
  for (auto listener : ose_listener_vec_global_) {
    listener->OnOrderDelete(security_id, order_id, side, intermediate);
  }

  // For OrderLevelListener
  for (auto listener : ose_listener_vec_) {
    listener->OnOrderDelete(security_id, GetTradeType(side), order_id);
  }
}
void OSEItchLoggedMessageFileSource::NotifyOrderExec(const uint32_t security_id, const uint8_t side,
                                                     const uint64_t order_id, const double exec_price,
                                                     const uint32_t size_exec, const uint32_t size_remaining,
                                                     bool intermediate) {
  // For OrderFeedGlobalListener
  for (auto listener : ose_listener_vec_global_) {
    listener->OnOrderExec(security_id, order_id, side, exec_price, size_exec, intermediate);
  }

  // For OrderLevelListener
  for (auto listener : ose_listener_vec_) {
    listener->OnOrderExec(security_id, GetTradeType(side), order_id, exec_price, size_exec, size_remaining);
  }
}
void OSEItchLoggedMessageFileSource::NotifyOrderExecWithTradeInfo(const uint32_t security_id, const uint8_t side,
                                                                  const uint64_t order_id, const double exec_price,
                                                                  const uint32_t size_exec,
                                                                  const uint32_t size_remaining, bool intermediate) {
  // For OrderFeedGlobalListener
  for (auto listener : ose_listener_vec_global_) {
    listener->OnOrderExecWithTradeInfo(security_id, order_id, side, exec_price, size_exec, intermediate);
  }

  // For OrderLevelListener
  for (auto listener : ose_listener_vec_) {
    // For OrderLevelListener we do not have OnOrderExecWithTradeInfo implemented
    listener->OnOrderExec(security_id, GetTradeType(side), order_id, exec_price, size_exec, size_remaining);
  }
}
void OSEItchLoggedMessageFileSource::NotifyResetBook(const unsigned int security_id) {
  for (auto listener : ose_listener_vec_global_) {
    listener->OnOrderResetBegin(security_id);
  }
}
void OSEItchLoggedMessageFileSource::NotifyTradingStatus(const unsigned int security_id, std::string status) {
  for (auto listener : ose_listener_vec_global_) {
    listener->OnTradingStatus(security_id, status);
  }
}
}
