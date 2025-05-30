#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/eurex_logged_message_filesource.hpp"
#include "dvccode/CDef/mds_messages.hpp"

namespace HFSAT {

EUREXLoggedMessageFileSource::EUREXLoggedMessageFileSource(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
    bool t_use_fake_faster_data_)
    : ExternalDataListener(),
      dbglogger_(t_dbglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      p_price_level_global_listener_(NULL),
      p_time_keeper_(NULL),
      bulk_file_reader_(),
      next_event_(),
      trading_location_file_read_(r_trading_location_),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      event_queue_(),
      events_left_(0),
      next_non_intermediate_time_(0, 0),
      use_fake_faster_data_(t_use_fake_faster_data_) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  // find the filename
  std::string t_eurex_filename_ =
      EUREXLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
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

  DBGLOG_CLASS_FUNC << "For EUREX symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                    << " trading_location " << trading_location_file_read_ << " DUsecs " << delay_usecs_to_add_
                    << " @use " << r_trading_location_ << " returned filename = " << t_eurex_filename_
                    << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(t_eurex_filename_);
  } else {
    std::cerr << "For EUREX symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_eurex_filename_ << std::endl;
  }
}

void EUREXLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // keep reading the next_event_
    // to check if next_event_timestamp_
    // if greater than r_endtime_
    do {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EUREX_MDS::EUREXCommonStruct));
      if (available_len_ < sizeof(EUREX_MDS::EUREXCommonStruct)) {  // data not found in file
        next_event_timestamp_ =
            ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
        rw_hasevents_ = false;
        break;
      }
      next_event_timestamp_ =
          next_event_.time_;  // need more work here to optimize, or preferably send ttime_t from MDS or ORS
      if (need_to_add_delay_usecs_) {
        switch (next_event_.msg_) {
          case EUREX_MDS::EUREX_DELTA: {
            if (!next_event_.data_.eurex_dels_.intermediate_) {
              next_event_timestamp_.addusecs(delay_usecs_to_add_);
            }
          } break;
          default: { next_event_timestamp_.addusecs(delay_usecs_to_add_); } break;
        }
      }
    } while (next_event_timestamp_ < r_start_time_);

  } else {  // data file not open
    rw_hasevents_ = false;
  }
}

void EUREXLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EUREX_MDS::EUREXCommonStruct));
    if (available_len_ < sizeof(EUREX_MDS::EUREXCommonStruct)) {  // data not found in file
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      rw_hasevents_ = false;
    } else {
      next_event_timestamp_ =
          next_event_.time_;  // need more work here to optimize, or preferably send ttime_t from MDS or ORS
      if (need_to_add_delay_usecs_) {
        switch (next_event_.msg_) {
          case EUREX_MDS::EUREX_DELTA: {
            if (!next_event_.data_.eurex_dels_.intermediate_) {
              next_event_timestamp_.addusecs(delay_usecs_to_add_);
            }
          } break;
          default: { next_event_timestamp_.addusecs(delay_usecs_to_add_); } break;
        }
      }
    }
  } else {
    rw_hasevents_ = false;
  }
}

inline void EUREXLoggedMessageFileSource::_ProcessThisMsg() {
  NotifyExternalDataListenerListener(security_id_);
  p_price_level_global_listener_->OnPriceLevelUpdate(&next_event_, sizeof(next_event_), HFSAT::MDS_MSG::EUREX);

  switch (next_event_.msg_) {
    case EUREX_MDS::EUREX_DELTA: {
      if (next_event_.data_.eurex_dels_.level_ > 0) {  // ignoring level 0 events right now

        // Only Update the watch on incoming non-intermedaite message
        if (!next_event_.data_.eurex_dels_.intermediate_) {
          p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
        }

        TradeType_t _buysell_ = TradeType_t('2' - next_event_.data_.eurex_dels_.type_);

        // TODO : send next_event_.data_.eurex_dels_.trd_qty_ also

        switch (next_event_.data_.eurex_dels_.action_) {
          case '1': {
            p_price_level_global_listener_->OnPriceLevelNew(
                security_id_, _buysell_, next_event_.data_.eurex_dels_.level_, next_event_.data_.eurex_dels_.price_,
                next_event_.data_.eurex_dels_.size_, next_event_.data_.eurex_dels_.num_ords_,
                next_event_.data_.eurex_dels_.intermediate_);
          } break;
          case '2': {
            p_price_level_global_listener_->OnPriceLevelChange(
                security_id_, _buysell_, next_event_.data_.eurex_dels_.level_, next_event_.data_.eurex_dels_.price_,
                next_event_.data_.eurex_dels_.size_, next_event_.data_.eurex_dels_.num_ords_,
                next_event_.data_.eurex_dels_.intermediate_);
          } break;
          case '3': {
            p_price_level_global_listener_->OnPriceLevelDelete(
                security_id_, _buysell_, next_event_.data_.eurex_dels_.level_, next_event_.data_.eurex_dels_.price_,
                next_event_.data_.eurex_dels_.intermediate_);
          } break;
          case '4': {
            p_price_level_global_listener_->OnPriceLevelDeleteFrom(security_id_, _buysell_,
                                                                   next_event_.data_.eurex_dels_.level_,
                                                                   next_event_.data_.eurex_dels_.intermediate_);
          } break;
          case '5': {
            p_price_level_global_listener_->OnPriceLevelDeleteThrough(security_id_, _buysell_,
                                                                      next_event_.data_.eurex_dels_.level_,
                                                                      next_event_.data_.eurex_dels_.intermediate_);
          } break;
          case '6': {
            p_price_level_global_listener_->OnPriceLevelOverlay(
                security_id_, _buysell_, next_event_.data_.eurex_dels_.level_, next_event_.data_.eurex_dels_.price_,
                next_event_.data_.eurex_dels_.size_, next_event_.data_.eurex_dels_.num_ords_,
                next_event_.data_.eurex_dels_.intermediate_);
          } break;
          default: { } break; }
      }
    } break;
    case EUREX_MDS::EUREX_TRADE: {
      p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

      TradeType_t _buysell_ = ((next_event_.data_.eurex_trds_.agg_side_ == 'B')
                                   ? (kTradeTypeBuy)
                                   : ((next_event_.data_.eurex_trds_.agg_side_ == 'S')
                                          ? kTradeTypeSell
                                          : kTradeTypeNoInfo));  // TODO see if semantics of 'B' and kTradeTypeBuy match

      p_price_level_global_listener_->OnTrade(security_id_, next_event_.data_.eurex_trds_.trd_px_,
                                              next_event_.data_.eurex_trds_.trd_qty_, _buysell_);
    } break;
    default: { } break; }
}

inline bool EUREXLoggedMessageFileSource::_SetNextTimeStamp() {
  if (events_left_ <= 0) {
    events_left_ = 0;
    bool found_non_intermediate_event_ = false;

    while (!found_non_intermediate_event_) {
      if (bulk_file_reader_.read(&next_event_, sizeof(EUREX_MDS::EUREXCommonStruct)) <
          sizeof(EUREX_MDS::EUREXCommonStruct)) {
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

void EUREXLoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void EUREXLoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}
}
