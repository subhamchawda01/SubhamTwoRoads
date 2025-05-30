#include "dvccode/CDef/mds_messages.hpp"
#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/liffe_logged_message_filesource.hpp"

namespace HFSAT {

LIFFELoggedMessageFileSource::LIFFELoggedMessageFileSource(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
    bool t_use_fake_faster_data_)
    : ExternalDataListener(),
      dbglogger_(t_dbglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      trading_date_(t_preevent_YYYYMMDD_),
      p_time_keeper_(NULL),
      bulk_file_reader_(),
      next_event_(),
      trading_location_file_read_(r_trading_location_),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      liffe_trade_time_manager_(
          HFSAT::LiffeTradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_preevent_YYYYMMDD_)),
      processing_intermediate_(NotSure),
      has_trading_hours_started_(false),
      has_trading_hours_closed_(false),
      event_queue_(),
      events_left_(0),
      next_non_intermediate_time_(0, 0),
      use_fake_faster_data_(t_use_fake_faster_data_),
      price_level_global_listener_vec_() {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  // find the filename
  std::string t_liffe_filename_ =
      LIFFELoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
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

  ComputeFirstNonIntermediateTime(t_liffe_filename_);
  DBGLOG_CLASS_FUNC << "For LIFFE symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                    << " trading_location " << trading_location_file_read_ << " DUsecs " << delay_usecs_to_add_
                    << " @use " << r_trading_location_ << " returned filename = " << t_liffe_filename_
                    << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(t_liffe_filename_);
  } else {
    std::cerr << "For LIFFE symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_liffe_filename_ << std::endl;
  }
}

void LIFFELoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // keep reading the next_event_
    // to check if next_event_timestamp_
    // if greater than r_endtime_
    do {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(LIFFE_MDS::LIFFECommonStruct));
      if (available_len_ < sizeof(LIFFE_MDS::LIFFECommonStruct)) {  // data not found in file
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
        switch (next_event_.msg_) {
          case LIFFE_MDS::LIFFE_DELTA: {
            if (!next_event_.data_.liffe_dels_.intermediate_) {
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

void LIFFELoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(LIFFE_MDS::LIFFECommonStruct));
    if (available_len_ < sizeof(LIFFE_MDS::LIFFECommonStruct)) {  // data not found in file
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      rw_hasevents_ = false;
    } else {
      next_event_timestamp_ =
          next_event_.time_;  // need more work here to optimize, or preferably send ttime_t from MDS or ORS
      if (need_to_add_delay_usecs_) {
        switch (next_event_.msg_) {
          case LIFFE_MDS::LIFFE_DELTA: {
            if (!next_event_.data_.liffe_dels_.intermediate_) {
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

inline void LIFFELoggedMessageFileSource::_ProcessThisMsg() {
  NotifyExternalDataListenerListener(security_id_);
  NotifyPriceLevelUpdate(&next_event_, sizeof(next_event_));

  if (!has_trading_hours_started_) {
    if (HasTradeHoursStartedToBuildIndexBook(next_event_.time_.tv_sec)) {
      has_trading_hours_started_ = true;

    } else {
      return;
    }
  }

  if (!has_trading_hours_closed_) {
    if (HasTradeHoursClosedToBuildIndexBook(next_event_.time_.tv_sec)) {
      has_trading_hours_closed_ = true;

      return;
    }

  } else {
    return;
  }

  switch (next_event_.msg_) {
    case LIFFE_MDS::LIFFE_DELTA: {
      if (next_event_.data_.liffe_dels_.level_ > 0) {  // ignoring level 0 events right now

        TradeType_t _buysell_ = TradeType_t('2' - next_event_.data_.liffe_dels_.type_);

        // TODO : send next_event_.data_.liffe_dels_.trd_qty_ also

        switch (next_event_.data_.liffe_dels_.action_) {
          case '1':
          case '2':
          case '3': {
            // Ignore Level 1 Data
            if (next_event_.data_.liffe_dels_.level_ == 1) break;

            // See what to do with intermediate messages
            if (processing_intermediate_ == NotSure) {
              if (next_event_.time_.tv_sec > TAGGING_INTERMEDIATE_SINCE) {
                processing_intermediate_ = Process;

              } else {
                processing_intermediate_ = DontProcess;
              }
            }

            bool intermediate = false;

            // Once we know what to do with intermediate handle watch
            if (processing_intermediate_ == Process) {
              if (next_event_.time_.tv_sec != 0) {
                p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
              }

              // Mark Intermediate for processing
              intermediate = next_event_.data_.liffe_dels_.intermediate_;

            } else {
              p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
            }

            // New Size is 0, represents order has been pulled or traded
            // In XDP feed, action was set to '1'(new) for delete with size set to 0.
            // After Optiq MDG into prod, we have moved handling for modify and delete in action '2' and '3'
            // respectively.
            if (trading_date_ <= LIFFE_XDP_LAST_DATE) {
              if (next_event_.data_.liffe_dels_.size_ == 0)
                NotifyPriceLevelDelete(security_id_, _buysell_, next_event_.data_.liffe_dels_.level_,
                                       next_event_.data_.liffe_dels_.price_, intermediate);
              else
                NotifyPriceLevelNew(security_id_, _buysell_, next_event_.data_.liffe_dels_.level_,
                                    next_event_.data_.liffe_dels_.price_, next_event_.data_.liffe_dels_.size_, 1,
                                    intermediate);
            } else {  // for Optiq feed
              if (next_event_.data_.liffe_dels_.action_ == '1')
                NotifyPriceLevelNew(security_id_, _buysell_, next_event_.data_.liffe_dels_.level_,
                                    next_event_.data_.liffe_dels_.price_, next_event_.data_.liffe_dels_.size_,
                                    next_event_.data_.liffe_dels_.num_ords_, intermediate);
              else if (next_event_.data_.liffe_dels_.action_ == '2')
                NotifyPriceLevelChange(security_id_, _buysell_, next_event_.data_.liffe_dels_.level_,
                                       next_event_.data_.liffe_dels_.price_, next_event_.data_.liffe_dels_.size_,
                                       next_event_.data_.liffe_dels_.num_ords_, intermediate);
              else if (next_event_.data_.liffe_dels_.action_ == '3')
                NotifyPriceLevelDelete(security_id_, _buysell_, next_event_.data_.liffe_dels_.level_,
                                       next_event_.data_.liffe_dels_.price_, intermediate);
            }

          } break;
          default: { } break; }
      }
    } break;
    case LIFFE_MDS::LIFFE_TRADE: {
      p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

      // this will always fall into tradetype no info, liffe doesn't have agg_side into in the trade msg
      TradeType_t _buysell_ = ((next_event_.data_.liffe_trds_.agg_side_ == 'B')
                                   ? (kTradeTypeBuy)
                                   : ((next_event_.data_.liffe_trds_.agg_side_ == 'S')
                                          ? kTradeTypeSell
                                          : kTradeTypeNoInfo));  // TODO see if semantics of 'B' and kTradeTypeBuy match

      NotifyTrade(security_id_, next_event_.data_.liffe_trds_.trd_px_, next_event_.data_.liffe_trds_.trd_qty_,
                  _buysell_);
    } break;
    default: { } break; }
}

inline bool LIFFELoggedMessageFileSource::_SetNextTimeStamp() {
  if (events_left_ <= 0) {
    events_left_ = 0;
    bool found_non_intermediate_event_ = false;

    while (!found_non_intermediate_event_) {
      if (bulk_file_reader_.read(&next_event_, sizeof(LIFFE_MDS::LIFFECommonStruct)) <
          sizeof(LIFFE_MDS::LIFFECommonStruct)) {
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

void LIFFELoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void LIFFELoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void LIFFELoggedMessageFileSource::ComputeFirstNonIntermediateTime(std::string logged_filesource_name_) {
  BulkFileReader this_filesource_;
  this_filesource_.open(logged_filesource_name_);
  if (this_filesource_.is_open()) {
    while (true) {
      size_t available_len_ = this_filesource_.read(&next_event_, sizeof(LIFFE_MDS::LIFFECommonStruct));
      if (available_len_ < sizeof(LIFFE_MDS::LIFFECommonStruct)) {
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

void LIFFELoggedMessageFileSource::AddPriceLevelGlobalListener(PriceLevelGlobalListener* p_this_listener_) {
  if (p_this_listener_ != NULL) {
    VectorUtils::UniqueVectorAdd(price_level_global_listener_vec_, p_this_listener_);
  }
}

void LIFFELoggedMessageFileSource::NotifyPriceLevelUpdate(void* ptr_to_price_level_update, int length_of_bytes) {
  for (auto listener_ : price_level_global_listener_vec_) {
    listener_->OnPriceLevelUpdate(ptr_to_price_level_update, length_of_bytes, HFSAT::MDS_MSG::LIFFE);
  }
}

void LIFFELoggedMessageFileSource::NotifyPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                       const int t_level_added_, const double t_price_,
                                                       const int t_new_size_, const int t_new_ordercount_,
                                                       const bool t_is_intermediate_message_) {
  for (std::vector<PriceLevelGlobalListener*>::iterator iter_ = price_level_global_listener_vec_.begin();
       iter_ != price_level_global_listener_vec_.end(); iter_++) {
    (*iter_)->OnPriceLevelNew(t_security_id_, t_buysell_, t_level_added_, t_price_, t_new_size_, t_new_ordercount_,
                              t_is_intermediate_message_);
  }
}

void LIFFELoggedMessageFileSource::NotifyPriceLevelDelete(const unsigned int t_security_id_,
                                                          const TradeType_t t_buysell_, const int t_level_removed_,
                                                          const double t_price_,
                                                          const bool t_is_intermediate_message_) {
  for (std::vector<PriceLevelGlobalListener*>::iterator iter_ = price_level_global_listener_vec_.begin();
       iter_ != price_level_global_listener_vec_.end(); iter_++) {
    (*iter_)->OnPriceLevelDelete(t_security_id_, t_buysell_, t_level_removed_, t_price_, t_is_intermediate_message_);
  }
}

void LIFFELoggedMessageFileSource::NotifyPriceLevelChange(const unsigned int t_security_id_,
                                                          const TradeType_t t_buysell_, const int t_level_changed_,
                                                          const double t_price_, const int t_new_size_,
                                                          const int t_new_ordercount_,
                                                          const bool t_is_intermediate_message_) {
  for (std::vector<PriceLevelGlobalListener*>::iterator iter_ = price_level_global_listener_vec_.begin();
       iter_ != price_level_global_listener_vec_.end(); iter_++) {
    (*iter_)->OnPriceLevelChange(t_security_id_, t_buysell_, t_level_changed_, t_price_, t_new_size_, t_new_ordercount_,
                                 t_is_intermediate_message_);
  }
}

void LIFFELoggedMessageFileSource::NotifyTrade(const unsigned int t_security_id_, const double t_trade_price_,
                                               const int t_trade_size_, const TradeType_t t_buysell_) {
  for (std::vector<PriceLevelGlobalListener*>::iterator iter_ = price_level_global_listener_vec_.begin();
       iter_ != price_level_global_listener_vec_.end(); iter_++) {
    (*iter_)->OnTrade(t_security_id_, t_trade_price_, t_trade_size_, t_buysell_);
  }
}
}
