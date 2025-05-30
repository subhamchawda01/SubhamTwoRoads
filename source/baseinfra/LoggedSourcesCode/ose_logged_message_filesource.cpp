#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/ose_logged_message_filesource.hpp"

namespace HFSAT {

OSELoggedMessageFileSource::OSELoggedMessageFileSource(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
    bool t_use_fake_faster_data_)
    : ExternalDataListener(),
      dbglogger_(t_dbglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      p_price_level_global_listener_(NULL),
      p_order_level_global_listener_(NULL),
      p_order_level_listener_sim_(NULL),
      p_time_keeper_(NULL),
      bulk_file_reader_(),
      next_event_(),
      security_id_to_prev_processed_seq(),
      trading_location_file_read_(r_trading_location_),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      is_processing_snapshot_(),
      trade_time_manager_(HFSAT::TradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_preevent_YYYYMMDD_)),
      use_order_book_(true),
      tradingdate_(t_preevent_YYYYMMDD_),
      use_fake_faster_data_(t_use_fake_faster_data_) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

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

void OSELoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // keep reading the next_event_
    // to check if next_event_timestamp_
    // if greater than r_endtime_
    do {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSECommonStruct));
      if (available_len_ < sizeof(OSE_MDS::OSECommonStruct)) {  // data not found in file
        next_event_timestamp_ =
            ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
        rw_hasevents_ = false;
        break;
      }
      // TODO handle need_to_add_delay_usecs_
      next_event_timestamp_ =
          next_event_.time_;  // need more work here to optimize, or preferably send ttime_t from MDS or ORS
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    } while (next_event_timestamp_ < r_start_time_ && !use_order_book_);

  } else {  // data file not open
    rw_hasevents_ = false;
  }

  if (use_order_book_) {
    SetTimeToSkipUntilFirstEvent(r_start_time_);
  }
}

void OSELoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& _hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSECommonStruct));
    if (available_len_ <
        sizeof(OSE_MDS::OSECommonStruct)) {           /* not enough data to fulfill this request to read a struct */
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

inline void OSELoggedMessageFileSource::_ProcessThisMsg() {
  switch (next_event_.msg_) {
    case OSE_MDS::OSE_DELTA: {
      int security_id_ = sec_name_indexer_.GetIdFromSecname(next_event_.data_.ose_dels_.contract_);
      if (security_id_ < 0) {
        break;
      }

      if (tradingdate_ == DateTime::Get_UTC_YYYYMMDD_from_ttime(next_event_.time_)) {
        p_time_keeper_->OnTimeReceived(next_event_.time_, security_id_);
      }

      if (next_event_.data_.ose_dels_.level_ > 0) {  // ignoring level 0 events right now

        TradeType_t _buysell_ = TradeType_t(next_event_.data_.ose_dels_.type_ - 1);
        switch (next_event_.data_.ose_dels_.action_) {
          case 10: {
            if (is_processing_snapshot_.find(security_id_) == is_processing_snapshot_.end()) {
              if (p_order_level_global_listener_) {
                p_order_level_global_listener_->resetBook(security_id_);
              }

              if (p_order_level_listener_sim_) {
                p_order_level_listener_sim_->ResetBook(security_id_);
              }
              is_processing_snapshot_[security_id_] = true;
            }

            security_id_to_prev_processed_seq[security_id_] = next_event_.data_.ose_dels_.seq_num;

            if (next_event_.data_.ose_dels_.price_ == 0) break;

            if (p_order_level_global_listener_) {
              p_order_level_global_listener_->OnOrderLevelSnapNew(
                  security_id_, next_event_.data_.ose_dels_.order_num, _buysell_, next_event_.data_.ose_dels_.level_,
                  next_event_.data_.ose_dels_.price_, next_event_.data_.ose_dels_.qty_diff_,
                  false /* next_event_.data_.ose_dels_.intermediate_ */);
            }

            if (p_order_level_listener_sim_) {
              p_order_level_listener_sim_->OnOrderAdd(
                  security_id_, _buysell_, next_event_.data_.ose_dels_.level_, next_event_.data_.ose_dels_.order_num,
                  next_event_.data_.ose_dels_.price_, next_event_.data_.ose_dels_.qty_diff_);
            }
          } break;
          case 0: {
            if ((is_processing_snapshot_.find(security_id_) != is_processing_snapshot_.end()) &&
                (is_processing_snapshot_[security_id_]))
              is_processing_snapshot_[security_id_] = false;

            if (security_id_to_prev_processed_seq.find(security_id_) != security_id_to_prev_processed_seq.end()) {
              if (next_event_.data_.ose_dels_.seq_num <= security_id_to_prev_processed_seq[security_id_]) break;
            }

            security_id_to_prev_processed_seq[security_id_] = next_event_.data_.ose_dels_.seq_num;

            if (next_event_.data_.ose_dels_.price_ == 0) break;

            if (p_order_level_global_listener_) {
              p_order_level_global_listener_->OnOrderLevelNew(
                  security_id_, next_event_.data_.ose_dels_.order_num, _buysell_, next_event_.data_.ose_dels_.level_,
                  next_event_.data_.ose_dels_.price_, next_event_.data_.ose_dels_.qty_diff_,
                  false /* next_event_.data_.ose_dels_.intermediate_ */);
            }

            if (p_order_level_listener_sim_) {
              p_order_level_listener_sim_->OnOrderAdd(
                  security_id_, _buysell_, next_event_.data_.ose_dels_.level_, next_event_.data_.ose_dels_.order_num,
                  next_event_.data_.ose_dels_.price_, next_event_.data_.ose_dels_.qty_diff_);
            }
          } break;
          case 1: {
            if ((is_processing_snapshot_.find(security_id_) != is_processing_snapshot_.end()) &&
                (is_processing_snapshot_[security_id_]))
              is_processing_snapshot_[security_id_] = false;

            if (security_id_to_prev_processed_seq.find(security_id_) != security_id_to_prev_processed_seq.end()) {
              if (next_event_.data_.ose_dels_.seq_num <= security_id_to_prev_processed_seq[security_id_]) break;
            }

            security_id_to_prev_processed_seq[security_id_] = next_event_.data_.ose_dels_.seq_num;

            if (next_event_.data_.ose_dels_.price_ == 0) break;

            if (p_order_level_global_listener_) {
              p_order_level_global_listener_->OnOrderLevelDelete(security_id_, next_event_.data_.ose_dels_.order_num,
                                                                 _buysell_, next_event_.data_.ose_dels_.level_,
                                                                 next_event_.data_.ose_dels_.price_, false);
            }

            if (p_order_level_listener_sim_) {
              p_order_level_listener_sim_->OnOrderDelete(
                  security_id_, _buysell_, next_event_.data_.ose_dels_.level_, next_event_.data_.ose_dels_.order_num,
                  next_event_.data_.ose_dels_.price_, next_event_.data_.ose_dels_.qty_diff_);
            }
          } break;
          case 2: {
            if ((is_processing_snapshot_.find(security_id_) != is_processing_snapshot_.end()) &&
                (is_processing_snapshot_[security_id_]))
              is_processing_snapshot_[security_id_] = false;

            if (security_id_to_prev_processed_seq.find(security_id_) != security_id_to_prev_processed_seq.end()) {
              if (next_event_.data_.ose_dels_.seq_num <= security_id_to_prev_processed_seq[security_id_]) break;
            }

            security_id_to_prev_processed_seq[security_id_] = next_event_.data_.ose_dels_.seq_num;

            if (next_event_.data_.ose_dels_.price_ == 0) break;

            if (p_order_level_global_listener_) {
              p_order_level_global_listener_->OnOrderLevelChange(
                  security_id_, next_event_.data_.ose_dels_.order_num, _buysell_, next_event_.data_.ose_dels_.level_,
                  next_event_.data_.ose_dels_.price_, next_event_.data_.ose_dels_.qty_diff_, false);
            }

            if (p_order_level_listener_sim_) {
              p_order_level_listener_sim_->OnOrderModify(
                  security_id_, _buysell_, next_event_.data_.ose_dels_.level_, next_event_.data_.ose_dels_.order_num,
                  next_event_.data_.ose_dels_.price_, next_event_.data_.ose_dels_.qty_diff_, 0, 0, 0);
            }
          } break;
          default: {
            fprintf(stderr, "Weird message type in OSELiveDataSource::ProcessAllEvents OSE_ORDER %d | action_ %c\n",
                    (int)next_event_.msg_, next_event_.data_.ose_dels_.action_);
          } break;
        }
      }
    } break;
    case OSE_MDS::OSE_TRADE: {
      int security_id_ = sec_name_indexer_.GetIdFromSecname(next_event_.data_.ose_trds_.contract_);
      if (security_id_ < 0) {
        break;
      }

      if (tradingdate_ == DateTime::Get_UTC_YYYYMMDD_from_ttime(next_event_.time_)) {
        p_time_keeper_->OnTimeReceived(next_event_.time_, security_id_);
      }

      if (p_price_level_global_listener_) {
        p_price_level_global_listener_->OnTrade(security_id_, next_event_.data_.ose_trds_.trd_px_,
                                                next_event_.data_.ose_trds_.trd_qty_, kTradeTypeNoInfo);
      }

      if (p_order_level_global_listener_) {
        p_order_level_global_listener_->OnTrade(security_id_, next_event_.data_.ose_trds_.trd_px_,
                                                next_event_.data_.ose_trds_.trd_qty_, kTradeTypeNoInfo);
      }

      if (p_order_level_listener_sim_) {
        p_order_level_listener_sim_->OnTrade(security_id_, next_event_.data_.ose_trds_.trd_px_,
                                             next_event_.data_.ose_trds_.trd_qty_, kTradeTypeNoInfo);
      }
    } break;

    case OSE_MDS::OSE_TRADE_DELTA: {
      int security_id_ = sec_name_indexer_.GetIdFromSecname(next_event_.data_.ose_dels_.contract_);
      TradeType_t _buysell_ = TradeType_t(next_event_.data_.ose_dels_.type_ - 1);

      if (tradingdate_ == DateTime::Get_UTC_YYYYMMDD_from_ttime(next_event_.time_)) {
        p_time_keeper_->OnTimeReceived(next_event_.time_, security_id_);
      }

      if (p_order_level_listener_sim_) {
        p_order_level_listener_sim_->OnOrderExec(
            security_id_, _buysell_, next_event_.data_.ose_dels_.level_, next_event_.data_.ose_dels_.order_num,
            next_event_.data_.ose_dels_.price_, next_event_.data_.ose_dels_.qty_diff_, 0);
      }
    } break;

    default: {
      fprintf(stderr, "Weird message type in OSELoggedMessageFileSource::ProcessAllEvents %d \n",
              (int)next_event_.msg_);
    } break;
  }
}

void OSELoggedMessageFileSource::ProcessAllEvents() {
  NotifyExternalDataListenerListener(security_id_);
  while (true) {
    if (tradingdate_ == DateTime::Get_UTC_YYYYMMDD_from_ttime(next_event_timestamp_)) {
      p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
    }

    _ProcessThisMsg();

    // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSECommonStruct));
    if (available_len_ <
        sizeof(OSE_MDS::OSECommonStruct)) {           /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      break;
    } else {
      next_event_timestamp_ = next_event_.time_;  // TODO : Converting from timeval to ttime_t. Need more work here to
                                                  // optimize, or preferably send ttime_t from MDS or ORS
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  }
}

void OSELoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  NotifyExternalDataListenerListener(security_id_);
  while (next_event_timestamp_ <= _endtime_) {
    // first alert Watch
    if (tradingdate_ == DateTime::Get_UTC_YYYYMMDD_from_ttime(next_event_timestamp_)) {
      p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
    }

    _ProcessThisMsg();

    // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSECommonStruct));
    if (available_len_ <
        sizeof(OSE_MDS::OSECommonStruct)) {           /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      break;
    } else {
      next_event_timestamp_ =
          next_event_.time_;  // need more work here to optimize, or preferably send ttime_t from MDS or ORS
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  }
}
}
