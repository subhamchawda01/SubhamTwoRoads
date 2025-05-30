#include "baseinfra/LoggedSources/sgx_of_logged_message_filesource.hpp"

namespace HFSAT {
SGXOrderLoggedMesageFileSource::SGXOrderLoggedMesageFileSource(
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
      sgx_listener_vec_(),
      YYYYMMDD_(t_preevent_YYYYMMDD_) {
  shortcode_ = sec_name_indexer_.GetShortcodeFromId(security_id_);
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;
  start_timestamp_.tv_sec = 0;
  start_timestamp_.tv_usec = 0;
  end_timestamp_.tv_sec = 0;
  end_timestamp_.tv_usec = 0;
  // get the file for SGX
  std::string sgx_file;
  sgx_file = CommonLoggedMessageFileNamer::GetOrderFeedFilename(kExchSourceSGX, t_exchange_symbol_,
                                                                t_preevent_YYYYMMDD_, trading_location_file_read_);

  // handle the delay to add in SIM due to difference in the exchange data read location and the product trading
  // location
  int delay_to_add_ = 0;
  delay_to_add_ = TradingLocationUtils::GetAddedDelay(trading_location_file_read_, r_trading_location_);
  delay_usecs_to_add_ = delay_to_add_;

  // if the trading location and data read location is not the same then add the delay
  if (trading_location_file_read_ != r_trading_location_) {
    delay_usecs_to_add_ +=
        TradingLocationUtils::GetUSecsBetweenTradingLocations(trading_location_file_read_, r_trading_location_);
  }

  DBGLOG_CLASS_FUNC << "For SGX symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                    << " trading_location " << trading_location_file_read_ << " DUsecs " << delay_usecs_to_add_
                    << " @use " << r_trading_location_ << " returned filename = " << sgx_file << DBGLOG_ENDL_FLUSH;

  // Open the file for reading
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(sgx_file);
  } else {
    std::cerr << "For SGX symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << sgx_file << std::endl;
  }
}

void SGXOrderLoggedMesageFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(SGX_ITCH_MDS::SGXItchOrder));
    if (available_len_ < sizeof(SGX_ITCH_MDS::SGXItchOrder)) {  // No data in file
      next_event_timestamp_ = ttime_t(time_t(0), 0);
      rw_hasevents_ = false;
    } else {
      next_event_timestamp_ = next_event_.time;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  } else {  // file is not open should an error be raised
    rw_hasevents_ = false;
  }
}

void SGXOrderLoggedMesageFileSource::ProcessEventsTill(const ttime_t endtime) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= endtime) {
    ProcessThisMsg();

    if (!SetNextTimeStamp()) return;
  }
}

TradeType_t SGXOrderLoggedMesageFileSource::GetTradeType(unsigned int type) {
  switch (type) {
    case 1: {
      return kTradeTypeBuy;
      break;
    }
    case 2: {
      return kTradeTypeSell;
      break;
    }
    default: {
      return kTradeTypeNoInfo;
      break;
    }
  }
}

void SGXOrderLoggedMesageFileSource::ProcessThisMsg() {
  // Skip msg if product break time

  if (!IsNormalTradeTime(security_id_, next_event_timestamp_)) return;

  NotifyExternalDataListenerListener(security_id_);
  p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

  switch (next_event_.order_type) {
    case SGX_ITCH_MDS::kAdd: {
      NotifyOrderAdd(security_id_, GetTradeType(next_event_.side), next_event_.order_id, next_event_.add.price,
                     next_event_.add.size);
      break;
    }
    case SGX_ITCH_MDS::kDelete: {
      // the struct doesnot have the price for the delete
      NotifyOrderDelete(security_id_, GetTradeType(next_event_.side), next_event_.order_id);
      break;
    }
    case SGX_ITCH_MDS::kExec: {  // exec_price is not provided by the exchange so passing -1 here, would have to handle
                                 // it in book manager
      // size remaining for the order is not provided by the exchange , would have to handle it in book manager
      NotifyOrderExec(security_id_, GetTradeType(next_event_.side), next_event_.order_id, -1, next_event_.exec.size,
                      -1);
      break;
    }
    case SGX_ITCH_MDS::kResetBegin: {
      NotifyResetBook(security_id_);
      break;
    }
    case SGX_ITCH_MDS::kExecWithPrice: {
      NotifyOrderExec(security_id_, GetTradeType(next_event_.side), next_event_.order_id, -1, next_event_.exec.size,
                      -1);
      break;
    }
    case SGX_ITCH_MDS::kResetEnd: {
      break;
    }
    case SGX_ITCH_MDS::kTradingStatus: {
      break;
    }
    default: {
      std::cerr << "Unknown msg_type in SGX: " << next_event_.order_type << std::endl;
      break;
    }
  }
}

void SGXOrderLoggedMesageFileSource::ProcessAllEvents() {
  while (true) {
    ProcessThisMsg();

    if (!SetNextTimeStamp()) return;
  }
}

inline bool SGXOrderLoggedMesageFileSource::SetNextTimeStamp() {
  // read the next_event_
  // set next_event_timestamp_
  register size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(SGX_ITCH_MDS::SGXItchOrder));
  if (available_len_ < sizeof(SGX_ITCH_MDS::SGXItchOrder)) {
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

void SGXOrderLoggedMesageFileSource::AddSGXListener(OrderLevelListener* listener) {
  if (listener != NULL) {
    VectorUtils::UniqueVectorAdd(sgx_listener_vec_, listener);
  }
}

void SGXOrderLoggedMesageFileSource::NotifyOrderAdd(const uint32_t security_id, const TradeType_t buysell,
                                                    const uint64_t order_id, const double price, const uint32_t size) {
  for (unsigned int listener_index = 0; listener_index < sgx_listener_vec_.size(); listener_index++) {
    sgx_listener_vec_[listener_index]->OnOrderAdd(security_id, buysell, order_id, 0, price, size);
  }
}

void SGXOrderLoggedMesageFileSource::NotifyOrderDelete(const uint32_t security_id, const TradeType_t buysell,
                                                       const uint64_t order_id) {
  for (unsigned int listener_index = 0; listener_index < sgx_listener_vec_.size(); listener_index++) {
    sgx_listener_vec_[listener_index]->OnOrderDelete(security_id, buysell, order_id);
  }
}

void SGXOrderLoggedMesageFileSource::NotifyOrderExec(const uint32_t security_id, const TradeType_t buysell,
                                                     const uint64_t order_id, const double exec_price,
                                                     const uint32_t size_exec, const uint32_t size_remaining) {
  for (unsigned int listener_index = 0; listener_index < sgx_listener_vec_.size(); listener_index++) {
    sgx_listener_vec_[listener_index]->OnOrderExec(security_id, buysell, order_id, exec_price, size_exec,
                                                   size_remaining);
  }
}

void SGXOrderLoggedMesageFileSource::NotifyOrderModify(const uint32_t security_id, const TradeType_t buysell,
                                                       const uint64_t order_id, const double price,
                                                       const uint32_t size) {
  for (unsigned int listener_index = 0; listener_index < sgx_listener_vec_.size(); listener_index++) {
    sgx_listener_vec_[listener_index]->OnOrderModify(security_id, buysell, order_id, 0, price, size);
  }
}

void SGXOrderLoggedMesageFileSource::NotifyResetBook(const unsigned int security_id) {
  for (auto listener : sgx_listener_vec_) {
    listener->ResetBook(security_id);
  }
}
}
