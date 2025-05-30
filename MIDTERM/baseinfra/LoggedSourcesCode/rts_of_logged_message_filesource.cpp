#include "dvccode/CDef/mds_messages.hpp"
#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/rts_of_logged_message_filesource.hpp"

namespace HFSAT {

RTSOrderFeedLoggedMessageFileSource::RTSOrderFeedLoggedMessageFileSource(
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
      trading_started_(false),
      trading_closed_(false),
      trade_time_manager_(HFSAT::TradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_preevent_YYYYMMDD_)),
      event_queue_(),
      events_left_(0),
      next_non_intermediate_time_(0, 0),
      use_fake_faster_data_(t_use_fake_faster_data_),
      trade_date_(t_preevent_YYYYMMDD_) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  // find the filename
  std::string t_rts_filename_ = CommonLoggedMessageFileNamer::GetOrderFeedFilename(
      kExchSourceRTS, t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
  if (trading_location_file_read_ != r_trading_location_) {
    delay_usecs_to_add_ =
        TradingLocationUtils::GetMSecsBetweenTradingLocations(trading_location_file_read_, r_trading_location_) * 1000;
    if (delay_usecs_to_add_ > 0) need_to_add_delay_usecs_ = true;
  }

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
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(t_rts_filename_);
  } else {
    DBGLOG_CLASS_FUNC << "For RTS symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                      << " trading_location " << trading_location_file_read_
                      << " returned filename = " << t_rts_filename_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    std::cerr << "For RTS symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_rts_filename_ << std::endl;
  }
}

void RTSOrderFeedLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {
    if (next_event_timestamp_ == ttime_t(0, 0)) {
      size_t available_len = bulk_file_reader_.read(&next_event_, sizeof(RTS_MDS::RTSOFCommonStructv2));

      // data not found in file
      if (available_len < sizeof(RTS_MDS::RTSOFCommonStructv2)) {
        next_event_timestamp_ =
            ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
        rw_hasevents_ = false;
      } else {
        next_event_timestamp_ = next_event_.time_;

        if (need_to_add_delay_usecs_) {
          if (!next_event_.is_intermediate) {  // FIXME: Abhishek: why not adding delay to intermediate msgs
            next_event_timestamp_.addusecs(delay_usecs_to_add_);
          }
        }
      }
    }
  }
  // data file not open
  else {
    rw_hasevents_ = false;
  }
}

void RTSOrderFeedLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(RTS_MDS::RTSOFCommonStructv2));
    if (available_len_ < sizeof(RTS_MDS::RTSOFCommonStructv2)) {  // data not found in file
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      rw_hasevents_ = false;
    } else {
      next_event_timestamp_ =
          next_event_.time_;  // need more work here to optimize, or preferably send ttime_t from MDS or ORS
      if (need_to_add_delay_usecs_) {
        if (!next_event_.is_intermediate) {
          next_event_timestamp_.addusecs(delay_usecs_to_add_);
        }
      }
    }
  } else {
    rw_hasevents_ = false;
  }
}

inline void RTSOrderFeedLoggedMessageFileSource::_ProcessThisMsg() {
  NotifyExternalDataListenerListener(security_id_);
  p_time_keeper_->OnTimeReceived(next_event_.time_);

  for (auto order_listener : orderfeed_global_listener_vec_) {
    order_listener->Process(security_id_, &next_event_);
  }
}

inline bool RTSOrderFeedLoggedMessageFileSource::_SetNextTimeStamp() {
  if (events_left_ <= 0) {
    events_left_ = 0;
    bool found_non_intermediate_event_ = false;

    while (!found_non_intermediate_event_) {
      if (bulk_file_reader_.read(&next_event_, sizeof(RTS_MDS::RTSOFCommonStructv2)) <
          sizeof(RTS_MDS::RTSOFCommonStructv2)) {
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

void RTSOrderFeedLoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void RTSOrderFeedLoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void RTSOrderFeedLoggedMessageFileSource::AddOrderLevelGlobalListener(
    OrderGlobalListener<RTS_MDS::RTSOFCommonStructv2>* p_this_listener_) {
  if (p_this_listener_ != NULL) {
    VectorUtils::UniqueVectorAdd(orderfeed_global_listener_vec_, p_this_listener_);
  }
}
}
