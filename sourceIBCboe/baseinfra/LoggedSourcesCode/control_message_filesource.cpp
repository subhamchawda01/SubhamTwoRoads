#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/control_message_filesource.hpp"

namespace HFSAT {

ControlMessageFileSource::ControlMessageFileSource(DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                                                   const unsigned int t_preevent_YYYYMMDD_,
                                                   const unsigned int t_security_id_, const char* t_exchange_symbol_,
                                                   TradingLocation_t t_trading_location_, const int t_query_id_,
                                                   bool& control_file_present)
    : ExternalDataListener(),
      dbglogger_(t_dbglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      query_id_(t_query_id_),
      p_time_keeper_(NULL),
      next_event_(),
      bulk_file_reader_() {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  TradingLocation_t trading_location_ = t_trading_location_;
  // find the filename
  std::string control_filename_ =
      CommonLoggedMessageFileNamer::GetControlFileName(t_preevent_YYYYMMDD_, trading_location_);

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_) {
    bulk_file_reader_.open(control_filename_);
  } else {
    std::cerr << "CONTROL messages file for date: " << t_preevent_YYYYMMDD_
              << " trading_location: " << trading_location_ << " returned filename: " << control_filename_
              << "! Going ahead with normal sim run" << std::endl;
    control_file_present = false;
  }
}

void ControlMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {
    // Reading only one event, Disabling seek for control source (as StartTrading can occur before BuildIndex time)
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(GenericControlRequestStruct));
    if (available_len_ < sizeof(GenericControlRequestStruct)) {
      next_event_timestamp_ = ttime_t(time_t(0), 0);
      rw_hasevents_ = false;
    } else {
      next_event_timestamp_ = next_event_.time_set_by_frontend_;
    }
  } else {  // data file not open
    rw_hasevents_ = false;
  }
}

void ControlMessageFileSource::ComputeEarliestDataTimestamp(bool& _hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(GenericControlRequestStruct));
    if (available_len_ < sizeof(GenericControlRequestStruct)) {  // data not found in file
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      _hasevents_ = false;
    } else {
      next_event_timestamp_ = next_event_.time_set_by_frontend_;
    }
  } else {
    _hasevents_ = false;
  }
}

inline bool ControlMessageFileSource::_SetNextTimeStamp() {
  register size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(GenericControlRequestStruct));
  if (available_len_ <
      sizeof(GenericControlRequestStruct)) { /* not enough data to fulfill this request to read a struct */
    next_event_timestamp_ = ttime_t(
        time_t(0), 0);  // to indicate to calling process in HistoricalDispatcher that we don't have any more data
    return false;       // to indicate to calling process ProcessEventsTill or ProcessAllEvents
  } else {
    next_event_timestamp_ = next_event_.time_set_by_frontend_;
    return true;
  }
  return true;
}

inline void ControlMessageFileSource::_ProcessThisMsg() {
  if (next_event_.trader_id_ != query_id_) {
    return;
  }
  NotifyExternalDataListenerListener(security_id_);
  p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

  for (unsigned index_ = 0; index_ < control_message_listener_vec_.size(); index_++) {
    control_message_listener_vec_[index_]->OnControlUpdate(
        next_event_.control_message_, sec_name_indexer_.GetSecurityNameFromId(security_id_), query_id_);
  }
}

void ControlMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void ControlMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}
}
