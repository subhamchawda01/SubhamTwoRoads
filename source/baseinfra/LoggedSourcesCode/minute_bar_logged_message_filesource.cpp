/**
   \file MDSMessagesCode/minute_bar_logged_message_filesource.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <algorithm>
#include <cstring>
#include "baseinfra/LoggedSources/minute_bar_logged_message_filesource.hpp"

namespace HFSAT {

MinuteBarLoggedMessageFileSource::MinuteBarLoggedMessageFileSource(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
    bool use_todays_data_)
    : ExternalDataListener(),
      dbglogger_(t_dbglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      listener_(nullptr),
      p_time_keeper_(NULL),
      bulk_file_reader_(),
      next_event_(),
      trading_location_file_read_(r_trading_location_) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  std::string t_nse_filename_ =
      MinuteBarLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(t_nse_filename_);
  } else {
    bulk_file_reader_.open(t_nse_filename_);
    std::cerr << "For MinuteBar symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
              << " trading_location " << trading_location_file_read_ << " returned filename = " << t_nse_filename_
              << std::endl;
  }
}

void MinuteBarLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {
    if (next_event_timestamp_.tv_sec == 0 && next_event_timestamp_.tv_usec == 0) {
      // read the next_event_
      // set next_event_timestamp_
      size_t available_len_;
      do {
        available_len_ = bulk_file_reader_.read(&next_event_, sizeof(DataBar));

        if (available_len_ < sizeof(DataBar)) { /* not enough data to fulfill this request to read a struct */
          next_event_timestamp_ =
              ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
          rw_hasevents_ = false;
          break;
        } else {
          next_event_timestamp_.tv_sec = next_event_.tv_sec_;
          next_event_timestamp_.tv_usec = 0;
        }
      } while (next_event_timestamp_ < r_start_time_);
    }
  } else {
    rw_hasevents_ = false;
  }
}

void MinuteBarLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& _hasevents_) {
  if (bulk_file_reader_.is_open()) {
    // read the next_event_
    // set next_event_timestamp_
    size_t available_len_;
    do {
      available_len_ = bulk_file_reader_.read(&next_event_, sizeof(DataBar));
    } while (available_len_ != sizeof(DataBar));

    if (available_len_ < sizeof(DataBar)) {           /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      _hasevents_ = false;
    } else {
      next_event_timestamp_.tv_sec = next_event_.tv_sec_;
      next_event_timestamp_.tv_usec = 0;
    }
  } else {
    _hasevents_ = false;
  }
}

void MinuteBarLoggedMessageFileSource::_ProcessThisMsg() {
  p_time_keeper_->OnTimeReceived(next_event_timestamp_);

  if (listener_) {
    listener_->OnNewBar(security_id_, next_event_);
  }
}

void MinuteBarLoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();
    size_t available_len_;
    do {
      available_len_ = bulk_file_reader_.read(&next_event_, sizeof(DataBar));
    } while (available_len_ != sizeof(DataBar));

    if (available_len_ < sizeof(DataBar)) {           /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      break;
    } else {
      next_event_timestamp_.tv_sec = next_event_.tv_sec_;
      next_event_timestamp_.tv_usec = 0;
    }
  }
}

void MinuteBarLoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    // read the next_event_
    // set next_event_timestamp_
    size_t available_len_;
    available_len_ = bulk_file_reader_.read(&next_event_, sizeof(DataBar));

    if (available_len_ < sizeof(DataBar)) {           /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      break;
    } else {
      next_event_timestamp_.tv_sec = next_event_.tv_sec_;
      next_event_timestamp_.tv_usec = 0;
    }
  }
}

void MinuteBarLoggedMessageFileSource::SetMinuteBarDataListener(MinuteBarDataListener* listener) {
  listener_ = listener;
}
}
