#include "baseinfra/LoggedSources/retail_logged_message_filesource.hpp"

namespace HFSAT {

RETAILLoggedMessageFileSource::RETAILLoggedMessageFileSource(
    DebugLogger &t_dbglogger_, SecurityNameIndexer &t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char *t_exchange_symbol_, const TradingLocation_t r_trading_location_,
    bool manual_mode)
    : ExternalDataListener(),
      dbglogger_(t_dbglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      fp_ord_exec_listener_vec_(),
      ret_trd_listener_vec_(),
      p_time_keeper_(NULL),
      bulk_file_reader_(),
      next_event_(),
      trading_location_file_read_(r_trading_location_),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      YYYYMMDD_(t_preevent_YYYYMMDD_) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  // find the filename
  std::string t_retail_filename_;
  if (manual_mode) {
    t_retail_filename_ = RETAILManualLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                                     trading_location_file_read_);
  } else {
    t_retail_filename_ =
        RETAILLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
  }
  int added_delay = 0;
  // added delays is used by the robustness check script to variate line delay by a constant number
  added_delay = TradingLocationUtils::GetAddedDelay(trading_location_file_read_, r_trading_location_);
  delay_usecs_to_add_ = added_delay;

  if (trading_location_file_read_ != r_trading_location_) {
    delay_usecs_to_add_ +=
        TradingLocationUtils::GetMSecsBetweenTradingLocations(trading_location_file_read_, r_trading_location_) * 1000;
  }
  if (delay_usecs_to_add_ != 0) need_to_add_delay_usecs_ = true;

  DBGLOG_CLASS_FUNC << "For RETAIL symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                    << " trading_location " << trading_location_file_read_ << " DUsecs " << delay_usecs_to_add_
                    << " @use " << r_trading_location_ << " returned filename = " << t_retail_filename_
                    << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(t_retail_filename_);
  } else {
    std::cerr << "For RETAIL symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_retail_filename_ << std::endl;
  }
}

void RETAILLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool &rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // keep reading the next_event_
    // to check if next_event_timestamp_
    // if greater than r_endtime_
    do {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(RETAIL_MDS::RETAILCommonStruct));
      if (available_len_ < sizeof(RETAIL_MDS::RETAILCommonStruct)) {  // data not found in file
        next_event_timestamp_ =
            ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
        rw_hasevents_ = false;
        break;
      }
      next_event_timestamp_ =
          next_event_.time_;  // need more work here to optimize, or preferably send ttime_t from MDS or ORS
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    } while (next_event_timestamp_ < r_start_time_);

  } else {  // data file not open
    rw_hasevents_ = false;
  }
}

void RETAILLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool &rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(RETAIL_MDS::RETAILCommonStruct));
    if (available_len_ < sizeof(RETAIL_MDS::RETAILCommonStruct)) {  // data not found in file
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

inline void RETAILLoggedMessageFileSource::_ProcessThisMsg() {
  NotifyExternalDataListenerListener(security_id_);
  p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

  switch (next_event_.msg_) {
    case RETAIL_MDS::RETAIL_TRADE: {
      // TODO: include implied RetailTrades for outrights by breaking spd/fly trades
      // Call only OnRetailTradeRequest for those trades
      if (next_event_.data_.retail_trds_.trd_qty_ > 0) {
        for (auto i = 0u; i < fp_ord_exec_listener_vec_.size(); i++) {
          fp_ord_exec_listener_vec_[i]->FPOrderExecuted(
              next_event_.data_.retail_trds_.contract_, next_event_.data_.retail_trds_.trd_px_,
              next_event_.data_.retail_trds_.agg_side_ == 'B' ? kTradeTypeBuy : kTradeTypeSell,
              next_event_.data_.retail_trds_.trd_qty_);
        }
      }

      for (auto i = 0u; i < ret_trd_listener_vec_.size(); i++) {
        ret_trd_listener_vec_[i]->OnRetailTradeRequest(
            next_event_.data_.retail_trds_.contract_, next_event_.data_.retail_trds_.trd_px_,
            next_event_.data_.retail_trds_.agg_side_ == 'B' ? kTradeTypeBuy : kTradeTypeSell,
            next_event_.data_.retail_trds_.trd_qty_, next_event_.data_.retail_trds_.quoted_qty_,
            next_event_.data_.retail_trds_.requested_qty_);
      }

    } break;
    default: { } break; }
}

inline bool RETAILLoggedMessageFileSource::_SetNextTimeStamp() {
  // read the next_event_
  // set next_event_timestamp_
  register size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(RETAIL_MDS::RETAILCommonStruct));
  if (available_len_ <
      sizeof(RETAIL_MDS::RETAILCommonStruct)) { /* not enough data to fulfill this request to read a struct */
    next_event_timestamp_ = ttime_t(
        time_t(0), 0);  // to indicate to calling process in HistoricalDispatcher that we don't have any more data
    return false;       // to indicate to calling process ProcessEventsTill or ProcessAllEvents
  } else {
    next_event_timestamp_ = next_event_.time_;  // TODO : Converting from timeval to ttime_t. Need more work here to
                                                // optimize, or preferably send ttime_t from MDS or ORS
    if (need_to_add_delay_usecs_) {
      next_event_timestamp_.addusecs(delay_usecs_to_add_);
    }
    return true;
  }
  return true;
}

void RETAILLoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void RETAILLoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}
}
