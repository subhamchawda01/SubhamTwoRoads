#include "dvccode/CDef/mds_messages.hpp"
#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/rts_logged_message_filesource.hpp"

namespace HFSAT {

RTSLoggedMessageFileSource::RTSLoggedMessageFileSource(
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
      price_level_global_listener_vec_(),
      trade_date_(t_preevent_YYYYMMDD_) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;
  // lot_size_ = HFSAT::SecurityDefinitions::GetContractMinOrderSize ( sec_name_indexer_.GetShortcodeFromId (
  // security_id_ ) , t_preevent_YYYYMMDD_ );
  // find the filename
  std::string t_rts_filename_ =
      RTSLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
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
  // bulk_file_reader_.open ( t_rts_filename_ ) ;
}

void RTSLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // keep reading the next_event_
    // to check if next_event_timestamp_
    // if greater than r_endtime_
    do {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(RTS_MDS::RTSCommonStruct));
      if (available_len_ < sizeof(RTS_MDS::RTSCommonStruct)) {  // data not found in file
        next_event_timestamp_ =
            ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
        rw_hasevents_ = false;
        break;
      }
      next_event_timestamp_ =
          next_event_.time_;  // need more work here to optimize, or preferably send ttime_t from MDS or ORS
      if (need_to_add_delay_usecs_) {
        switch (next_event_.msg_) {
          case RTS_MDS::RTS_DELTA: {
            if (!next_event_.data_.rts_dels_.intermediate_) {
              next_event_timestamp_.addusecs(delay_usecs_to_add_);
            }
          } break;
          default:  // currently we dont tag the intermediate with 000.000 time in order message so not having the check
          {
            next_event_timestamp_.addusecs(delay_usecs_to_add_);
          } break;
        }
      }
    } while (next_event_timestamp_ < r_start_time_);

  } else {  // data file not open
    rw_hasevents_ = false;
  }
}

void RTSLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(RTS_MDS::RTSCommonStruct));
    if (available_len_ < sizeof(RTS_MDS::RTSCommonStruct)) {  // data not found in file
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      rw_hasevents_ = false;
    } else {
      next_event_timestamp_ =
          next_event_.time_;  // need more work here to optimize, or preferably send ttime_t from MDS or ORS
      if (need_to_add_delay_usecs_) {
        switch (next_event_.msg_) {
          case RTS_MDS::RTS_DELTA: {
            if (!next_event_.data_.rts_dels_.intermediate_) {
              next_event_timestamp_.addusecs(delay_usecs_to_add_);
            }
          } break;
          default:  // currently we dont tag the intermediate with 000.000 time in order message so not having the check
          {
            next_event_timestamp_.addusecs(delay_usecs_to_add_);
          } break;
        }
      }
    }
  } else {
    rw_hasevents_ = false;
  }
}

inline void RTSLoggedMessageFileSource::_ProcessThisMsg() {
  if (!IsNormalTradeTime(security_id_, next_event_timestamp_)) {
    return;
  }
  NotifyExternalDataListenerListener(security_id_);
  p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
  NotifyPriceLevelUpdate(&next_event_, sizeof(next_event_));

  switch (next_event_.msg_) {
    case RTS_MDS::RTS_DELTA: {
      next_event_.data_.rts_dels_.num_ords_ = trade_date_ >= RTS_OF_DATE ? next_event_.data_.rts_dels_.num_ords_ : 1;

      if (next_event_.data_.rts_dels_.level_ > 0) {  // ignoring level 0 events right now

        TradeType_t _buysell_ = TradeType_t(next_event_.data_.rts_dels_.type_ - '0');
        bool intermediate = next_event_.data_.rts_dels_.intermediate_;

        // Only update the watch if the message is non-intermediate
        if (next_event_.time_.tv_sec != 0) {
          p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
        }

        switch (next_event_.data_.rts_dels_.action_) {
          case '0':
            NotifyPriceLevelNew(security_id_, _buysell_, next_event_.data_.rts_dels_.level_,
                                next_event_.data_.rts_dels_.price_, next_event_.data_.rts_dels_.size_,
                                next_event_.data_.rts_dels_.num_ords_,
                                intermediate);  // RTS does not have order # information.
            break;
          case '1':
            NotifyPriceLevelChange(security_id_, _buysell_, next_event_.data_.rts_dels_.level_,
                                   next_event_.data_.rts_dels_.price_, next_event_.data_.rts_dels_.size_,
                                   next_event_.data_.rts_dels_.num_ords_, intermediate);
            break;
          case '2':
            NotifyPriceLevelDelete(security_id_, _buysell_, next_event_.data_.rts_dels_.level_,
                                   next_event_.data_.rts_dels_.price_, intermediate);
            break;
          default:
            fprintf(stderr, "Weird message type in RtsMDProcessor::flushQuoteQueue RTS_DELTA  \n");
            break;
        }
      }
    } break;
    case RTS_MDS::RTS_TRADE: {
      p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

      TradeType_t _buysell_ =
          ((next_event_.data_.rts_trds_.agg_side_ == 1)
               ? (HFSAT::kTradeTypeBuy)
               : ((next_event_.data_.rts_trds_.agg_side_ == 2) ? (HFSAT::kTradeTypeSell) : HFSAT::kTradeTypeNoInfo));

      NotifyTrade(security_id_, next_event_.data_.rts_trds_.trd_px_, next_event_.data_.rts_trds_.trd_qty_, _buysell_);
    } break;
    default: { } break; }
}

inline bool RTSLoggedMessageFileSource::_SetNextTimeStamp() {
  if (events_left_ <= 0) {
    events_left_ = 0;
    bool found_non_intermediate_event_ = false;

    while (!found_non_intermediate_event_) {
      if (bulk_file_reader_.read(&next_event_, sizeof(RTS_MDS::RTSCommonStruct)) < sizeof(RTS_MDS::RTSCommonStruct)) {
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

void RTSLoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void RTSLoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void RTSLoggedMessageFileSource::AddPriceLevelGlobalListener(PriceLevelGlobalListener* p_this_listener_) {
  if (p_this_listener_ != NULL) {
    VectorUtils::UniqueVectorAdd(price_level_global_listener_vec_, p_this_listener_);
  }
}

void RTSLoggedMessageFileSource::NotifyPriceLevelUpdate(void* ptr_to_price_level_update, int length_of_bytes) {
  for (auto listener_ : price_level_global_listener_vec_) {
    listener_->OnPriceLevelUpdate(ptr_to_price_level_update, length_of_bytes, HFSAT::MDS_MSG::RTS);
  }
}

void RTSLoggedMessageFileSource::NotifyPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                     const int t_level_added_, const double t_price_,
                                                     const int t_new_size_, const int t_new_ordercount_,
                                                     const bool t_is_intermediate_message_) {
  for (std::vector<PriceLevelGlobalListener*>::iterator iter_ = price_level_global_listener_vec_.begin();
       iter_ != price_level_global_listener_vec_.end(); iter_++) {
    (*iter_)->OnPriceLevelNew(t_security_id_, t_buysell_, t_level_added_, t_price_, t_new_size_, t_new_ordercount_,
                              t_is_intermediate_message_);
  }
}

void RTSLoggedMessageFileSource::NotifyPriceLevelDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                        const int t_level_removed_, const double t_price_,
                                                        const bool t_is_intermediate_message_) {
  for (std::vector<PriceLevelGlobalListener*>::iterator iter_ = price_level_global_listener_vec_.begin();
       iter_ != price_level_global_listener_vec_.end(); iter_++) {
    (*iter_)->OnPriceLevelDelete(t_security_id_, t_buysell_, t_level_removed_, t_price_, t_is_intermediate_message_);
  }
}

void RTSLoggedMessageFileSource::NotifyPriceLevelChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                        const int t_level_changed_, const double t_price_,
                                                        const int t_new_size_, const int t_new_ordercount_,
                                                        const bool t_is_intermediate_message_) {
  for (std::vector<PriceLevelGlobalListener*>::iterator iter_ = price_level_global_listener_vec_.begin();
       iter_ != price_level_global_listener_vec_.end(); iter_++) {
    (*iter_)->OnPriceLevelChange(t_security_id_, t_buysell_, t_level_changed_, t_price_, t_new_size_, t_new_ordercount_,
                                 t_is_intermediate_message_);
  }
}

void RTSLoggedMessageFileSource::NotifyTrade(const unsigned int t_security_id_, const double t_trade_price_,
                                             const int t_trade_size_, const TradeType_t t_buysell_) {
  for (std::vector<PriceLevelGlobalListener*>::iterator iter_ = price_level_global_listener_vec_.begin();
       iter_ != price_level_global_listener_vec_.end(); iter_++) {
    (*iter_)->OnTrade(t_security_id_, t_trade_price_, t_trade_size_, t_buysell_);
  }
}
}
