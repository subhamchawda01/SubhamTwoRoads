#include "dvccode/CDef/mds_messages.hpp"
#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/ice_logged_message_filesource.hpp"

namespace HFSAT {

ICELoggedMessageFileSource::ICELoggedMessageFileSource(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
    bool t_use_order_feed_, bool t_use_fake_faster_data_)
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
      liffe_trade_time_manager_(
          HFSAT::LiffeTradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_preevent_YYYYMMDD_)),
      has_trading_hours_started_(false),
      has_trading_hours_closed_(false),
      event_queue_(),
      events_left_(0),
      next_non_intermediate_time_(0, 0),
      use_fake_faster_data_(t_use_fake_faster_data_),
      price_level_global_listener_vec_(),
      order_level_listener_vec_(),
      trade_listener_vec_(),
      use_order_feed_(t_use_order_feed_) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  // find the filename
  std::string t_ice_filename_ = "";

  if (t_use_order_feed_) {
    t_ice_filename_ = CommonLoggedMessageFileNamer::GetOrderFeedFilename(
        kExchSourceICE, t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
  } else {
    t_ice_filename_ = CommonLoggedMessageFileNamer::GetName(kExchSourceICE, t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                            trading_location_file_read_);
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

  if (TradingLocationUtils::GetUsecsForFasterDataForShortcodeAtLocation(
          sec_name_indexer_.GetShortcodeFromId(security_id_), r_trading_location_, t_preevent_YYYYMMDD_,
          use_fake_faster_data_) !=
      0) {  // Experiment with the possibility of using faster data for this product at this location.
    need_to_add_delay_usecs_ = true;
    delay_usecs_to_add_ -= TradingLocationUtils::GetUsecsForFasterDataForShortcodeAtLocation(
        sec_name_indexer_.GetShortcodeFromId(security_id_), r_trading_location_, t_preevent_YYYYMMDD_,
        use_fake_faster_data_);
  }

  ComputeFirstNonIntermediateTime(t_ice_filename_);
  DBGLOG_CLASS_FUNC << "For ICE symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                    << " trading_location " << trading_location_file_read_ << " DUsecs " << delay_usecs_to_add_
                    << " @use " << r_trading_location_ << " returned filename = " << t_ice_filename_
                    << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(t_ice_filename_);
  } else {
    std::cerr << "For ICE symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_ice_filename_ << std::endl;
  }
}

void ICELoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {
    if (use_order_feed_) {
      if (next_event_timestamp_.tv_sec == 0 && next_event_timestamp_.tv_usec == 0) {
        // read the next_event_
        // set next_event_timestamp_
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(ICE_MDS::ICECommonStruct));
        if (available_len_ <
            sizeof(ICE_MDS::ICECommonStruct)) { /* not enough data to fulfill this request to read a struct */
          next_event_timestamp_ =
              ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
          rw_hasevents_ = false;
        } else {
          // timeval to ttime_t
          next_event_timestamp_ = next_event_.time_;
          if (need_to_add_delay_usecs_) {
            next_event_timestamp_.addusecs(delay_usecs_to_add_);
          }
        }
      }
    } else {
      // keep reading the next_event_
      // to check if next_event_timestamp_
      // if greater than r_endtime_
      do {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(ICE_MDS::ICECommonStruct));
        if (available_len_ < sizeof(ICE_MDS::ICECommonStruct)) {  // data not found in file
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
      } while (next_event_timestamp_ < r_start_time_);
    }
  } else {  // data file not open
    rw_hasevents_ = false;
  }

  if (use_order_feed_) {
    SetTimeToSkipUntilFirstEvent(r_start_time_);
  }
}

void ICELoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(ICE_MDS::ICECommonStruct));
    if (available_len_ < sizeof(ICE_MDS::ICECommonStruct)) {  // data not found in file
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

inline void ICELoggedMessageFileSource::_ProcessThisMsg() {
  NotifyExternalDataListenerListener(security_id_);
  NotifyPriceLevelUpdate(&next_event_, sizeof(next_event_));

  switch (next_event_.msg_) {
    case ICE_MDS::ICE_MS: {
      NotifyResetBook(security_id_);
    } break;

    case ICE_MDS::ICE_FOD: {
      ICE_MDS::ICEFODStruct& ice_fod_ = next_event_.data_.ice_fods_;
      TradeType_t buysell_ =
          ice_fod_.side_ == '1' ? kTradeTypeBuy : ((ice_fod_.side_ == '2') ? kTradeTypeSell : kTradeTypeNoInfo);

      p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

      switch (ice_fod_.type_) {
        case 0:  // Add
        {
          // Compute the order priority as the combination of
          // entry_time_in_millis and seq_within_millis
          int seq_ = ice_fod_.seq_within_millis_;
          int64_t entry_time_ = ice_fod_.order_entry_time_;
          int64_t priority_ = entry_time_ * 1000000 + seq_;

          NotifyOrderAdd(security_id_, buysell_, ice_fod_.order_id_, ice_fod_.price_, ice_fod_.size_, priority_);
        } break;
        case 1:  // Modify
        {
          NotifyOrderModify(security_id_, buysell_, ice_fod_.order_id_, ice_fod_.price_, ice_fod_.size_);
        } break;
        case 2:  // Delete
        {
          NotifyOrderDelete(security_id_, ice_fod_.order_id_);
        } break;
        case 3:  // Snapshot
        {
          // Compute the order priority as the combination of
          // entry_time_in_millis and seq_within_millis
          int seq_ = ice_fod_.seq_within_millis_;
          int64_t entry_time_ = ice_fod_.order_entry_time_;
          int64_t priority_ = entry_time_ * 1000000 + seq_;

          NotifyOrderAdd(security_id_, buysell_, ice_fod_.order_id_, ice_fod_.price_, ice_fod_.size_, priority_);
        } break;
        default: { } break; }
    } break;
    case ICE_MDS::ICE_PL: {
      if (next_event_.data_.ice_pls_.level_ > 0) {  // ignoring level 0 events right now

        TradeType_t buysell_ = next_event_.data_.ice_pls_.side_ == '1'
                                   ? kTradeTypeBuy
                                   : ((next_event_.data_.ice_pls_.side_ == '2') ? kTradeTypeSell : kTradeTypeNoInfo);

        p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

        switch (next_event_.data_.ice_pls_.action_) {
          case '0': {
            NotifyPriceLevelNew(security_id_, buysell_, next_event_.data_.ice_pls_.level_,
                                next_event_.data_.ice_pls_.price_, next_event_.data_.ice_pls_.size_,
                                next_event_.data_.ice_pls_.orders_, next_event_.data_.ice_pls_.intermediate_);

          } break;
          case '1': {
            NotifyPriceLevelChange(security_id_, buysell_, next_event_.data_.ice_pls_.level_,
                                   next_event_.data_.ice_pls_.price_, next_event_.data_.ice_pls_.size_,
                                   next_event_.data_.ice_pls_.orders_, next_event_.data_.ice_pls_.intermediate_);
          } break;
          case '2': {
            NotifyPriceLevelDelete(security_id_, buysell_, next_event_.data_.ice_pls_.level_,
                                   next_event_.data_.ice_pls_.price_, next_event_.data_.ice_pls_.intermediate_);
          } break;
          default: { } break; }
      }
    } break;
    case ICE_MDS::ICE_TRADE: {
      p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

      TradeType_t _buysell_ = ((next_event_.data_.ice_trds_.side_ == '1')
                                   ? (kTradeTypeBuy)
                                   : ((next_event_.data_.ice_trds_.side_ == '2') ? kTradeTypeSell : kTradeTypeNoInfo));

      if (next_event_.data_.ice_trds_.off_market_trade_type_ == ' ' &&
          next_event_.data_.ice_trds_.is_system_priced_leg_ == 'N') {
        NotifyTrade(security_id_, next_event_.data_.ice_trds_.price_, next_event_.data_.ice_trds_.size_, _buysell_);

        TradeType_t buysell_ = kTradeTypeNoInfo;

        if (_buysell_ == kTradeTypeBuy) {
          buysell_ = kTradeTypeSell;
        } else if (_buysell_ == kTradeTypeSell) {
          buysell_ = kTradeTypeBuy;
        }

        NotifyOrderExec(security_id_, buysell_, next_event_.data_.ice_trds_.trade_id_,
                        next_event_.data_.ice_trds_.price_, next_event_.data_.ice_trds_.size_);
      }
    } break;

    case ICE_MDS::ICE_RESET_BEGIN: {
      p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
      for (std::vector<PriceLevelGlobalListener*>::iterator iter_ = price_level_global_listener_vec_.begin();
           iter_ != price_level_global_listener_vec_.end(); iter_++) {
        (*iter_)->OnResetBegin(security_id_);
      }
    } break;
    case ICE_MDS::ICE_RESET_END: {
      p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
      for (std::vector<PriceLevelGlobalListener*>::iterator iter_ = price_level_global_listener_vec_.begin();
           iter_ != price_level_global_listener_vec_.end(); iter_++) {
        (*iter_)->OnResetEnd(security_id_);
      }
    } break;
    default: { } break; }
}

inline bool ICELoggedMessageFileSource::_SetNextTimeStamp() {
  if (events_left_ <= 0) {
    events_left_ = 0;
    bool found_non_intermediate_event_ = false;

    while (!found_non_intermediate_event_) {
      if (bulk_file_reader_.read(&next_event_, sizeof(ICE_MDS::ICECommonStruct)) < sizeof(ICE_MDS::ICECommonStruct)) {
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

void ICELoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void ICELoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void ICELoggedMessageFileSource::ComputeFirstNonIntermediateTime(std::string logged_filesource_name_) {
  BulkFileReader this_filesource_;
  this_filesource_.open(logged_filesource_name_);
  if (this_filesource_.is_open()) {
    while (true) {
      size_t available_len_ = this_filesource_.read(&next_event_, sizeof(ICE_MDS::ICECommonStruct));
      if (available_len_ < sizeof(ICE_MDS::ICECommonStruct)) {
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

void ICELoggedMessageFileSource::SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {
  for (std::vector<OrderLevelListenerICE*>::iterator iter_ = order_level_listener_vec_.begin();
       iter_ != order_level_listener_vec_.end(); iter_++) {
    (*iter_)->SetTimeToSkipUntilFirstEvent(r_start_time_);
  }
}

void ICELoggedMessageFileSource::AddOrderLevelListener(OrderLevelListenerICE* p_this_listener_) {
  if (p_this_listener_ != NULL) {
    VectorUtils::UniqueVectorAdd(order_level_listener_vec_, p_this_listener_);
  }
}

void ICELoggedMessageFileSource::AddPriceLevelGlobalListener(PriceLevelGlobalListener* p_this_listener_) {
  if (p_this_listener_ != NULL) {
    VectorUtils::UniqueVectorAdd(price_level_global_listener_vec_, p_this_listener_);
  }
}

void ICELoggedMessageFileSource::AddTradeListener(TradeGlobalListener* t_listener_) {
  if (t_listener_ != NULL) {
    VectorUtils::UniqueVectorAdd(trade_listener_vec_, t_listener_);
  }
}

void ICELoggedMessageFileSource::NotifyOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                const int64_t t_order_id_, const double t_price_,
                                                const uint32_t t_size_, const int64_t priority_) {
  for (std::vector<OrderLevelListenerICE*>::iterator iter_ = order_level_listener_vec_.begin();
       iter_ != order_level_listener_vec_.end(); iter_++) {
    (*iter_)->OnOrderAdd(t_security_id_, t_buysell_, t_order_id_, t_price_, t_size_, priority_);
  }
}

void ICELoggedMessageFileSource::NotifyOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                   const int64_t t_order_id_, const double t_price_,
                                                   const uint32_t t_size_) {
  for (std::vector<OrderLevelListenerICE*>::iterator iter_ = order_level_listener_vec_.begin();
       iter_ != order_level_listener_vec_.end(); iter_++) {
    (*iter_)->OnOrderModify(t_security_id_, t_buysell_, t_order_id_, t_price_, t_size_);
  }
}

void ICELoggedMessageFileSource::NotifyOrderDelete(const uint32_t t_security_id_, const int64_t t_order_id_) {
  for (std::vector<OrderLevelListenerICE*>::iterator iter_ = order_level_listener_vec_.begin();
       iter_ != order_level_listener_vec_.end(); iter_++) {
    (*iter_)->OnOrderDelete(t_security_id_, t_order_id_);
  }
}

void ICELoggedMessageFileSource::NotifyOrderExec(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                 const int64_t t_order_id_, const double t_traded_price_,
                                                 const uint32_t t_traded_size_) {
  for (std::vector<OrderLevelListenerICE*>::iterator iter_ = order_level_listener_vec_.begin();
       iter_ != order_level_listener_vec_.end(); iter_++) {
    (*iter_)->OnOrderExec(security_id_, t_buysell_, t_order_id_, t_traded_price_, t_traded_size_);
  }
}

void ICELoggedMessageFileSource::NotifyResetBook(const unsigned int t_security_id_) {
  for (std::vector<OrderLevelListenerICE*>::iterator iter_ = order_level_listener_vec_.begin();
       iter_ != order_level_listener_vec_.end(); iter_++) {
    (*iter_)->ResetBook(t_security_id_);
  }
}

void ICELoggedMessageFileSource::NotifyPriceLevelUpdate(void* ptr_to_price_level_update, int length_of_bytes) {
  for (auto listener_ : price_level_global_listener_vec_) {
    listener_->OnPriceLevelUpdate(ptr_to_price_level_update, length_of_bytes, HFSAT::MDS_MSG::ICE);
  }
}

void ICELoggedMessageFileSource::NotifyPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                     const int t_level_added_, const double t_price_,
                                                     const int t_new_size_, const int t_new_ordercount_,
                                                     const bool t_is_intermediate_message_) {
  for (std::vector<PriceLevelGlobalListener*>::iterator iter_ = price_level_global_listener_vec_.begin();
       iter_ != price_level_global_listener_vec_.end(); iter_++) {
    (*iter_)->OnPriceLevelNew(t_security_id_, t_buysell_, t_level_added_, t_price_, t_new_size_, t_new_ordercount_,
                              t_is_intermediate_message_);
  }
}

void ICELoggedMessageFileSource::NotifyPriceLevelDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                        const int t_level_removed_, const double t_price_,
                                                        const bool t_is_intermediate_message_) {
  for (std::vector<PriceLevelGlobalListener*>::iterator iter_ = price_level_global_listener_vec_.begin();
       iter_ != price_level_global_listener_vec_.end(); iter_++) {
    (*iter_)->OnPriceLevelDelete(t_security_id_, t_buysell_, t_level_removed_, t_price_, t_is_intermediate_message_);
  }
}

void ICELoggedMessageFileSource::NotifyPriceLevelChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                        const int t_level_changed_, const double t_price_,
                                                        const int t_new_size_, const int t_new_ordercount_,
                                                        const bool t_is_intermediate_message_) {
  for (std::vector<PriceLevelGlobalListener*>::iterator iter_ = price_level_global_listener_vec_.begin();
       iter_ != price_level_global_listener_vec_.end(); iter_++) {
    (*iter_)->OnPriceLevelChange(t_security_id_, t_buysell_, t_level_changed_, t_price_, t_new_size_, t_new_ordercount_,
                                 t_is_intermediate_message_);
  }
}

void ICELoggedMessageFileSource::NotifyTrade(const unsigned int t_security_id_, const double t_trade_price_,
                                             const int t_trade_size_, const TradeType_t t_buysell_) {
  for (std::vector<PriceLevelGlobalListener*>::iterator iter_ = price_level_global_listener_vec_.begin();
       iter_ != price_level_global_listener_vec_.end(); iter_++) {
    (*iter_)->OnTrade(t_security_id_, t_trade_price_, t_trade_size_, t_buysell_);
  }

  //  for (std::vector<TradeGlobalListener*>::iterator iter_ = trade_listener_vec_.begin();
  //       iter_ != trade_listener_vec_.end(); iter_++) {
  //    (*iter_)->OnTrade(t_security_id_, t_trade_price_, t_trade_size_, t_buysell_);
  //  }
}
}
