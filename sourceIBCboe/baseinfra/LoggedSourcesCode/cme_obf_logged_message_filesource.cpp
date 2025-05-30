#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/cme_obf_logged_message_filesource.hpp"

namespace HFSAT {

CMEOBFLoggedMessageFileSource::CMEOBFLoggedMessageFileSource(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
    bool t_use_fake_faster_data_)
    : ExternalDataListener(),
      dbglogger_(t_dbglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      p_time_keeper_(nullptr),
      bulk_file_reader_(),
      next_event_(),
      trading_location_file_read_(r_trading_location_),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      use_fake_faster_data_(t_use_fake_faster_data_),
      order_listener_vec_(),
      YYYYMMDD_(t_preevent_YYYYMMDD_) {
  shortcode_ = sec_name_indexer_.GetShortcodeFromId(security_id_);
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;
  start_timestamp_.tv_sec = 0;
  start_timestamp_.tv_usec = 0;
  end_timestamp_.tv_sec = 0;
  end_timestamp_.tv_usec = 0;
  // find the filename
  std::string cme_obf_file;
  cme_obf_file = CommonLoggedMessageFileNamer::GetOrderFeedFilename(kExchSourceCME, t_exchange_symbol_,
                                                                    t_preevent_YYYYMMDD_, trading_location_file_read_);
  std::string t_cme_filename_ = CommonLoggedMessageFileNamer::GetName(
      kExchSourceCME, t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);

  int added_delay = 0;
  // added delays is used by the robustness check script to variate line delay by a constant number
  added_delay = TradingLocationUtils::GetAddedDelay(trading_location_file_read_, r_trading_location_);
  delay_usecs_to_add_ = added_delay;

  if (trading_location_file_read_ != r_trading_location_) {
    delay_usecs_to_add_ +=
        TradingLocationUtils::GetUSecsBetweenTradingLocations(trading_location_file_read_, r_trading_location_);
  }
  if (delay_usecs_to_add_ != 0) need_to_add_delay_usecs_ = true;
  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(cme_obf_file);
  } else {
    std::cerr << "For CME symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << cme_obf_file << std::endl;
  }
}

void CMEOBFLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  // keep reading the next_event_
  // to check if next_event_timestamp_
  // if greater than r_endtime_
  if (bulk_file_reader_.is_open()) {
    if (next_event_timestamp_ == ttime_t(0, 0)) {
      size_t available_len = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMEOBFCommonStruct));

      // data not found in file
      if (available_len < sizeof(CME_MDS::CMEOBFCommonStruct)) {
        rw_hasevents_ = false;
      } else {
        next_event_timestamp_ = next_event_.time_;

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

void CMEOBFLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMEOBFCommonStruct));
    if (available_len_ < sizeof(CME_MDS::CMEOBFCommonStruct)) {  // data not found in file
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      rw_hasevents_ = false;
    } else {
      next_event_timestamp_ =
          next_event_.time_;  // need more work here to optimize, or preferably send ttime_t from MDS or ORS
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  } else {
    rw_hasevents_ = false;
  }
}

inline void CMEOBFLoggedMessageFileSource::ProcessThisMsg() {
  NotifyExternalDataListenerListener(security_id_);
  p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

  switch (next_event_.msg_) {
    case CME_MDS::CME_DELTA: {
      switch (next_event_.data_.cme_dels_.action) {
        case '0': {
          NotifyOrderAdd(security_id_, GetTradeType(next_event_.data_.cme_dels_.type),
                         next_event_.data_.cme_dels_.order_id, next_event_.data_.cme_dels_.order_priority,
                         next_event_.data_.cme_dels_.price, next_event_.data_.cme_dels_.order_qty);

        } break;
        case '1': {
          NotifyOrderModify(security_id_, GetTradeType(next_event_.data_.cme_dels_.type),
                            next_event_.data_.cme_dels_.order_id, next_event_.data_.cme_dels_.order_priority,
                            next_event_.data_.cme_dels_.price, next_event_.data_.cme_dels_.order_qty);

        } break;
        case '2': {
          NotifyOrderDelete(security_id_, GetTradeType(next_event_.data_.cme_dels_.type),
                            next_event_.data_.cme_dels_.order_id);
        } break;
        default: {
          fprintf(stderr, "Unknown Delta message type in CME OBF Processor ::flushQuoteQueue CME_DELTA  \n");
        } break;
      }

    } break;
    case CME_MDS::CME_TRADE: {
      // Legacy usage for OF.
      // Replaced by CME_EXEC in newer data feed. But kept in place to be used for older data.
      NotifyOrderExec(security_id_, GetTradeType(next_event_.data_.cme_trds_.agg_side_), 0,
                      next_event_.data_.cme_trds_.trd_px_, next_event_.data_.cme_trds_.trd_qty_,
                      next_event_.data_.cme_trds_.tot_qty_ - next_event_.data_.cme_trds_.trd_qty_);
    } break;
    case CME_MDS::CME_EXEC: {
      // Since exec messages don't have the following information
      // Intentionally specifying default values for:
      //  - TradeType: No info
      //  - trade_price: No info/0
      //  - Remaining trade size: No info/0
      // Listeners should use their own logic to process events for other information.
      NotifyOrderExec(security_id_, kTradeTypeNoInfo, next_event_.data_.cme_excs_.order_id, 0,
                      next_event_.data_.cme_excs_.order_qty, 0);
    } break;

    case CME_MDS::CME_RESET_BEGIN: {
      NotifyResetBegin(security_id_);
    } break;
    case CME_MDS::CME_RESET_END: {
      NotifyResetEnd(security_id_);
    } break;
    default: { std::cerr << "Unknown msg_type in CME: " << next_event_.msg_ << std::endl; } break;
  }
}

inline bool CMEOBFLoggedMessageFileSource::SetNextTimeStamp() {
  // read the next_event_
  // set next_event_timestamp_
  register size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMEOBFCommonStruct));
  if (available_len_ < sizeof(CME_MDS::CMEOBFCommonStruct)) {
    next_event_timestamp_ = ttime_t(time_t(0), 0);
    return false;
  } else {
    next_event_timestamp_ = next_event_.time_;
    if (need_to_add_delay_usecs_) {
      next_event_timestamp_.addusecs(delay_usecs_to_add_);
    }
    return true;
  }
  return true;
}

void CMEOBFLoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    ProcessThisMsg();

    if (!SetNextTimeStamp()) return;
  }
}

void CMEOBFLoggedMessageFileSource::ProcessEventsTill(const ttime_t endtime) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= endtime) {
    ProcessThisMsg();

    if (!SetNextTimeStamp()) return;
  }
}

void CMEOBFLoggedMessageFileSource::AddOrderLevelListener(OrderLevelListener* listener) {
  if (listener != nullptr) {
    VectorUtils::UniqueVectorAdd(order_listener_vec_, listener);
  }
}

void CMEOBFLoggedMessageFileSource::NotifyResetBegin(const uint32_t security_id) {
  for (auto listener : order_listener_vec_) {
    listener->OnResetBegin(security_id);
  }
}

void CMEOBFLoggedMessageFileSource::NotifyResetEnd(const uint32_t security_id) {
  for (auto listener : order_listener_vec_) {
    listener->OnResetEnd(security_id);
  }
}

void CMEOBFLoggedMessageFileSource::NotifyOrderAdd(const uint32_t security_id, const TradeType_t buysell,
                                                   const uint64_t order_id, const uint32_t priority, const double price,
                                                   const uint32_t size) {
  for (auto listener : order_listener_vec_) {
    listener->OnOrderAdd(security_id, buysell, order_id, priority, price, size);
  }
}
void CMEOBFLoggedMessageFileSource::NotifyOrderModify(const uint32_t security_id, const TradeType_t buysell,
                                                      const uint64_t order_id, const uint32_t priority,
                                                      const double price, const uint32_t size) {
  for (auto listener : order_listener_vec_) {
    listener->OnOrderModify(security_id, buysell, order_id, priority, price, size);
  }
}

void CMEOBFLoggedMessageFileSource::NotifyOrderDelete(const uint32_t security_id, const TradeType_t buysell,
                                                      const uint64_t order_id) {
  for (auto listener : order_listener_vec_) {
    listener->OnOrderDelete(security_id, buysell, order_id);
  }
}
void CMEOBFLoggedMessageFileSource::NotifyOrderExec(const uint32_t security_id, const TradeType_t buysell,
                                                    const uint64_t order_id, const double exec_price,
                                                    const uint32_t size_exec, const uint32_t size_remaining) {
  for (auto listener : order_listener_vec_) {
    listener->OnOrderExec(security_id, buysell, order_id, exec_price, size_exec, size_remaining);
  }
}

TradeType_t CMEOBFLoggedMessageFileSource::GetTradeType(const char type) {
  switch (type) {
    case '0': {
      return kTradeTypeBuy;
      break;
    }
    case '1': {
      return kTradeTypeSell;
      break;
    }
    default: {
      return kTradeTypeNoInfo;
      break;
    }
  }
}
}
