/**
   \file MDSMessageCode/aflash_logged_message_filesource.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/aflash_logged_message_filesource.hpp"

namespace HFSAT {

/**
 * @param t_dbglogger_ for logging errors
 * @param t_sec_name_indexer_ to detect if the security is of interest and not to process if not. If string matching is
 * more efficient we could use t_exchange_symbol_ as well.
 * @param t_preevent_YYYYMMDD_ tradingdate to load the appropriate file
 * @param t_security_id_ also same as t_sec_name_indexer_ [ t_exchange_symbol_ ]
 * @param t_exchange_symbol_ needed to match
 *
 * For now assuming t_exchange_symbol_ matching is not required
 */
AFLASHLoggedMessageFileSource::AFLASHLoggedMessageFileSource(DebugLogger& t_dbglogger_,
                                                             const unsigned int t_preevent_YYYYMMDD_,
                                                             TradingLocation_t r_trading_location_)
    : dbglogger_(t_dbglogger_),
      p_time_keeper_(NULL),
      price_level_global_listener_(NULL),
      bulk_file_reader_(),
      next_event_(),
      trading_location_file_read_(r_trading_location_),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      event_queue_(),
      events_left_(0),
      next_non_intermediate_time_(0, 0) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  // find the filename
  std::string t_exchange_symbol_ = "AFL";
  std::string t_aflash_filename_ =
      AFLASHLoggedMessageFileNamer::GetName(t_preevent_YYYYMMDD_, trading_location_file_read_);

  int added_delay = 0;
  // added delays is used by the robustness check script to variate line delay by a constant number
  added_delay = TradingLocationUtils::GetAddedDelay(trading_location_file_read_, r_trading_location_);
  delay_usecs_to_add_ = added_delay;

  if (trading_location_file_read_ != r_trading_location_) {
    delay_usecs_to_add_ +=
        TradingLocationUtils::GetMSecsBetweenTradingLocations(trading_location_file_read_, r_trading_location_) * 1000;
  }
  if (delay_usecs_to_add_ != 0) need_to_add_delay_usecs_ = true;

  ComputeFirstNonIntermediateTime(t_aflash_filename_);

  DBGLOG_CLASS_FUNC << "For AFLASH symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                    << " trading_location " << trading_location_file_read_ << " DUsecs " << delay_usecs_to_add_
                    << " @use " << r_trading_location_ << " returned filename = " << t_aflash_filename_
                    << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(t_aflash_filename_);
  } else {
    std::cerr << "For AFLASH symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_aflash_filename_ << std::endl;
  }

  aflashprocessor_ = HFSAT::CombinedMDSMessagesAflashProcessor::GetUniqueInstance(dbglogger_);
}

void AFLASHLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // keep reading the next_event_
    // to check if next_event_timestamp_
    // if greater than r_endtime_
    do {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(AFLASH_MDS::AFlashCommonStruct));
      if (available_len_ < sizeof(AFLASH_MDS::AFlashCommonStruct)) {  // data not found in file
        next_event_timestamp_ =
            ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
        rw_hasevents_ = false;
        break;
      }
      next_event_timestamp_ =
          next_event_.time_;  // need more work here to optimize, or preferably send ttime_t from MDS or ORS

      if (first_non_intermediate_time_ >= r_start_time_) {
        next_event_timestamp_ = first_non_intermediate_time_;
        return;
      }

      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    } while (next_event_timestamp_ <= r_start_time_);

  } else {  // data file not open
    rw_hasevents_ = false;
  }
}

void AFLASHLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& _hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(AFLASH_MDS::AFlashCommonStruct));
    if (available_len_ <
        sizeof(AFLASH_MDS::AFlashCommonStruct)) {     /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      _hasevents_ = false;
    } else {
      next_event_timestamp_ = next_event_.time_;  // TODO : Converting from timeval to ttime_t. Need more work here to
                                                  // optimize, or preferably send ttime_t from MDS or ORS
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  } else {
    _hasevents_ = false;
  }
}

inline bool AFLASHLoggedMessageFileSource::_SetNextTimeStamp() {
  if (events_left_ <= 0) {
    events_left_ = 0;
    bool found_non_intermediate_event_ = false;

    while (!found_non_intermediate_event_) {
      if (bulk_file_reader_.read(&next_event_, sizeof(AFLASH_MDS::AFlashCommonStruct)) <
          sizeof(AFLASH_MDS::AFlashCommonStruct)) {
        next_event_timestamp_ = ttime_t(0, 0);
        return false;
      }

      if (next_event_.time_.tv_sec != 0) {
        next_non_intermediate_time_ = next_event_.time_;
        found_non_intermediate_event_ = true;
      }

      event_queue_.push_back(next_event_);
      events_left_++;
    }
  }

  if (events_left_ > 0) {
    next_event_ = event_queue_[0];
    next_event_timestamp_ = next_non_intermediate_time_;

    if (need_to_add_delay_usecs_) {
      next_event_timestamp_.addusecs(delay_usecs_to_add_);
    }

    events_left_--;
    event_queue_.erase(event_queue_.begin());

    return true;
  }

  return true;
}

inline void AFLASHLoggedMessageFileSource::_ProcessThisMsg() {
  p_time_keeper_->OnTimeReceived(next_event_timestamp_);

  if (price_level_global_listener_) {
    price_level_global_listener_->OnPriceLevelUpdate(&next_event_, sizeof(next_event_), HFSAT::MDS_MSG::AFLASH);
  }

  AFLASH_MDS::AFlashCommonStructLive next_event_live_;
  next_event_live_.time_ = next_event_.time_;
  next_event_live_.uid_ = next_event_.uid_;
  memcpy(next_event_live_.symbol_, next_event_.symbol_, 4);
  for (uint32_t i = 0; i < AFLASH_MDS::MAX_FIELDS; i++) {
    next_event_live_.fields[i] = next_event_.fields[i];
  }
  next_event_live_.category_id_ = next_event_.category_id_;
  next_event_live_.type_ = next_event_.type_;
  next_event_live_.version_ = next_event_.version_;
  next_event_live_.nfields_ = next_event_.nfields_;
  aflashprocessor_->ProcessAflashEvent(&next_event_live_);
}

void AFLASHLoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void AFLASHLoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void AFLASHLoggedMessageFileSource::ComputeFirstNonIntermediateTime(std::string logged_filesource_name_) {
  BulkFileReader this_filesource_;
  this_filesource_.open(logged_filesource_name_);
  if (this_filesource_.is_open()) {
    while (true) {
      size_t available_len_ = this_filesource_.read(&next_event_, sizeof(AFLASH_MDS::AFlashCommonStruct));
      if (available_len_ < sizeof(AFLASH_MDS::AFlashCommonStruct)) {
        break;
      } else {
        if (next_event_.time_.tv_sec == 0) {
          continue;
        } else {
          first_non_intermediate_time_ = next_event_.time_;
          break;
        }
      }
    }
    this_filesource_.close();
  }
}

void AFLASHLoggedMessageFileSource::AddPriceLevelGlobalListener(PriceLevelGlobalListener* p_this_listener_) {
  price_level_global_listener_ = p_this_listener_;
}
}
