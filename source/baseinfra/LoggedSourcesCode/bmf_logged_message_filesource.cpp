#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/bmf_logged_message_filesource.hpp"

namespace HFSAT {

BMFLoggedMessageFileSource::BMFLoggedMessageFileSource(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
    bool t_use_fake_faster_data_)
    : ExternalDataListener(),
      dbglogger_(t_dbglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      p_price_level_global_listener_(NULL),
      p_order_level_global_listener_(NULL),  // --
      p_time_keeper_(NULL),
      bulk_file_reader_(),
      next_event_(),
      trading_location_file_read_(r_trading_location_),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      use_fake_faster_data_(t_use_fake_faster_data_) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  // find the filename
  std::string t_bmf_filename_ =
      BMFLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
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

  DBGLOG_CLASS_FUNC << "For BMF symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                    << " trading_location " << trading_location_file_read_ << " DUsecs " << delay_usecs_to_add_
                    << " @use " << r_trading_location_ << " returned filename = " << t_bmf_filename_
                    << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(t_bmf_filename_);
  } else {
    std::cerr << "For BMF symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_bmf_filename_ << std::endl;
  }
  // bulk_file_reader_.open ( t_bmf_filename_ ) ;
}

void BMFLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // keep reading the next_event_
    // to check if next_event_timestamp_
    // if greater than r_endtime_
    do {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(BMF_MDS::BMFCommonStruct));
      if (available_len_ < sizeof(BMF_MDS::BMFCommonStruct)) {  // data not found in file
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
    } while (next_event_timestamp_ < r_start_time_);

  } else {  // data file not open
    rw_hasevents_ = false;
  }
}

void BMFLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& _hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(BMF_MDS::BMFCommonStruct));
    if (available_len_ <
        sizeof(BMF_MDS::BMFCommonStruct)) {           /* not enough data to fulfill this request to read a struct */
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

void BMFLoggedMessageFileSource::ProcessAllEvents() {
  NotifyExternalDataListenerListener(security_id_);
  while (true) {
    p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

    switch (next_event_.msg_) {
      case BMF_MDS::BMF_DELTA: {
        if (next_event_.data_.bmf_dels_.level_ > 0) {  // ignoring level 0 events right now

          TradeType_t _buysell_ = TradeType_t(next_event_.data_.bmf_dels_.type_ - '0');

          // TODO : send next_event_.data_.bmf_dels_.trd_qty_ also

          switch (next_event_.data_.bmf_dels_.action_) {
            case '0': {
              p_price_level_global_listener_->OnPriceLevelNew(
                  security_id_, _buysell_, next_event_.data_.bmf_dels_.level_, next_event_.data_.bmf_dels_.price_,
                  next_event_.data_.bmf_dels_.size_, next_event_.data_.bmf_dels_.num_ords_, false);
            } break;
            case '1': {
              p_price_level_global_listener_->OnPriceLevelChange(
                  security_id_, _buysell_, next_event_.data_.bmf_dels_.level_, next_event_.data_.bmf_dels_.price_,
                  next_event_.data_.bmf_dels_.size_, next_event_.data_.bmf_dels_.num_ords_, false);
            } break;
            case '2': {
              p_price_level_global_listener_->OnPriceLevelDelete(security_id_, _buysell_,
                                                                 next_event_.data_.bmf_dels_.level_,
                                                                 next_event_.data_.bmf_dels_.price_, false);
            } break;
            default: { } break; }
        }
      } break;
      case BMF_MDS::BMF_ORDER:  // --
      {
        if (next_event_.data_.bmf_ordr_.level_ > 0) {  // ignoring level 0 events right now

          //		  TradeType_t _buysell_ = TradeType_t ( next_event_.data_.bmf_ordr_.type_ - '0' );

          switch (next_event_.data_.bmf_ordr_.action_) {
            case '0': {
              //			p_order_level_global_listener_->OnOrderLevelNew    ( security_id_,
              // next_event_.data_.bmf_ordr_.order_id_, _buysell_, next_event_.data_.bmf_ordr_.level_,
              // next_event_.data_.bmf_ordr_.price_, next_event_.data_.bmf_ordr_.size_,
              // next_event_.data_.bmf_ordr_.num_ords_, false /* next_event_.data_.bmf_dels_.intermediate_ */ ) ;
            } break;
            case '1': {
              //			p_order_level_global_listener_->OnOrderLevelChange ( security_id_,
              // next_event_.data_.bmf_ordr_.order_id_, _buysell_, next_event_.data_.bmf_ordr_.level_,
              // next_event_.data_.bmf_ordr_.price_, next_event_.data_.bmf_ordr_.size_,
              // next_event_.data_.bmf_ordr_.num_ords_, false /* next_event_.data_.bmf_dels_.intermediate_ */ ) ;
            } break;
            case '2': {
              //			p_order_level_global_listener_->OnOrderLevelDelete ( security_id_,
              // next_event_.data_.bmf_ordr_.order_id_, _buysell_, next_event_.data_.bmf_ordr_.level_,
              // next_event_.data_.bmf_ordr_.price_, false /* next_event_.data_.bmf_dels_.intermediate_ */ ) ;
            } break;
            case '9': {
            } break;
            default: {
              fprintf(stderr, "Weird message type in BMFLiveDataSource::ProcessAllEvents BMF_ORDER %d \n",
                      (int)next_event_.msg_);
            } break;
          }
        }
      } break;

      case BMF_MDS::BMF_TRADE: {
        if (p_price_level_global_listener_) {  // --
          p_price_level_global_listener_->OnTrade(security_id_, next_event_.data_.bmf_trds_.trd_px_,
                                                  next_event_.data_.bmf_trds_.trd_qty_, kTradeTypeNoInfo);
        }

        if (p_order_level_global_listener_) {  // --
          p_order_level_global_listener_->OnTrade(security_id_, next_event_.data_.bmf_trds_.trd_px_,
                                                  next_event_.data_.bmf_trds_.trd_qty_, kTradeTypeNoInfo);
        }

      } break;
      default: { } break; }

    // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(BMF_MDS::BMFCommonStruct));
    if (available_len_ <
        sizeof(BMF_MDS::BMFCommonStruct)) {           /* not enough data to fulfill this request to read a struct */
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

void BMFLoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  NotifyExternalDataListenerListener(security_id_);
  while (next_event_timestamp_ <= _endtime_) {
    p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

    switch (next_event_.msg_) {
      case BMF_MDS::BMF_DELTA: {
        if (next_event_.data_.bmf_dels_.level_ > 0) {  // ignoring level 0 events right now

          TradeType_t _buysell_ = TradeType_t('2' - next_event_.data_.bmf_dels_.type_);

          // TODO : send next_event_.data_.bmf_dels_.trd_qty_ also

          switch (next_event_.data_.bmf_dels_.action_) {
            case '1': {
              p_price_level_global_listener_->OnPriceLevelNew(
                  security_id_, _buysell_, next_event_.data_.bmf_dels_.level_, next_event_.data_.bmf_dels_.price_,
                  next_event_.data_.bmf_dels_.size_, next_event_.data_.bmf_dels_.num_ords_, false);
            } break;
            case '2': {
              p_price_level_global_listener_->OnPriceLevelChange(
                  security_id_, _buysell_, next_event_.data_.bmf_dels_.level_, next_event_.data_.bmf_dels_.price_,
                  next_event_.data_.bmf_dels_.size_, next_event_.data_.bmf_dels_.num_ords_, false);
            } break;
            case '3': {
              p_price_level_global_listener_->OnPriceLevelDelete(security_id_, _buysell_,
                                                                 next_event_.data_.bmf_dels_.level_,
                                                                 next_event_.data_.bmf_dels_.price_, false);
            } break;
            default: { } break; }
        }
      } break;

      case BMF_MDS::BMF_ORDER:  // --
      {
        if (next_event_.data_.bmf_ordr_.level_ > 0) {  // ignoring level 0 events right now

          //		  TradeType_t _buysell_ = TradeType_t ('2'-next_event_.data_.bmf_ordr_.type_ );

          switch (next_event_.data_.bmf_ordr_.action_) {
            case '0': {
              //			p_order_level_global_listener_->OnOrderLevelNew    ( security_id_,
              // next_event_.data_.bmf_ordr_.order_id_, _buysell_, next_event_.data_.bmf_ordr_.level_,
              // next_event_.data_.bmf_ordr_.price_, next_event_.data_.bmf_ordr_.size_,
              // next_event_.data_.bmf_ordr_.num_ords_, false /* next_event_.data_.bmf_dels_.intermediate_ */ ) ;
            } break;
            case '1': {
              //			p_order_level_global_listener_->OnOrderLevelChange ( security_id_,
              // next_event_.data_.bmf_ordr_.order_id_, _buysell_, next_event_.data_.bmf_ordr_.level_,
              // next_event_.data_.bmf_ordr_.price_, next_event_.data_.bmf_ordr_.size_,
              // next_event_.data_.bmf_ordr_.num_ords_, false /* next_event_.data_.bmf_dels_.intermediate_ */ ) ;
            } break;
            case '2': {
              //			p_order_level_global_listener_->OnOrderLevelDelete ( security_id_,
              // next_event_.data_.bmf_ordr_.order_id_, _buysell_, next_event_.data_.bmf_ordr_.level_,
              // next_event_.data_.bmf_ordr_.price_, false /* next_event_.data_.bmf_dels_.intermediate_ */ ) ;
            } break;
            default: {
              fprintf(stderr, "Weird message type in BMFLiveDataSource::ProcessAllEvents BMF_ORDER %d \n",
                      (int)next_event_.msg_);
            } break;
          }
        }
      } break;

      case BMF_MDS::BMF_TRADE: {
        if (p_price_level_global_listener_) {  // --
          p_price_level_global_listener_->OnTrade(security_id_, next_event_.data_.bmf_trds_.trd_px_,
                                                  next_event_.data_.bmf_trds_.trd_qty_, kTradeTypeNoInfo);
        }

        if (p_order_level_global_listener_) {  // --
          p_order_level_global_listener_->OnTrade(security_id_, next_event_.data_.bmf_trds_.trd_px_,
                                                  next_event_.data_.bmf_trds_.trd_qty_, kTradeTypeNoInfo);
        }
      } break;
      default: { } break; }

    // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(BMF_MDS::BMFCommonStruct));
    if (available_len_ <
        sizeof(BMF_MDS::BMFCommonStruct)) {           /* not enough data to fulfill this request to read a struct */
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
