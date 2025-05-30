/**
   \file MDSMessageCode/puma_logged_message_filesource.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/puma_logged_message_filesource.hpp"

namespace HFSAT {

PUMALoggedMessageFileSource::PUMALoggedMessageFileSource(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, TradingLocation_t r_trading_location_,
    bool t_use_fake_faster_data_)
    : ExternalDataListener(),
      dbglogger_(t_dbglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      p_price_level_global_listener_(NULL),
      p_ntp_price_level_global_listener_(NULL),
      p_time_keeper_(NULL),
      bulk_file_reader_(),
      next_event_(),
      trading_location_file_read_(r_trading_location_),
      trade_time_manager_(HFSAT::TradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_preevent_YYYYMMDD_)),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      use_fake_faster_data_(t_use_fake_faster_data_),
      has_trading_started_(false),
      has_trading_ended_(false) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  // find the filename
  std::string t_puma_filename_ =
      PUMALoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);

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

  DBGLOG_CLASS_FUNC << "For PUMA symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                    << " trading_location " << trading_location_file_read_ << " DUsecs " << delay_usecs_to_add_
                    << " @use " << r_trading_location_ << " returned filename = " << t_puma_filename_
                    << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(t_puma_filename_);
  } else {
    std::cerr << "For PUMA symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_puma_filename_ << std::endl;
  }
  // bulk_file_reader_.open ( t_cme_filename_ ) ;
}

void PUMALoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // keep reading the next_event_
    // to check if next_event_timestamp_
    // if greater than r_endtime_
    do {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(PUMA_MDS::PumaCommonStruct));
      if (available_len_ < sizeof(PUMA_MDS::PumaCommonStruct)) {  // data not found in file
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

void PUMALoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& _hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(PUMA_MDS::PumaCommonStruct));
    if (available_len_ <
        sizeof(PUMA_MDS::PumaCommonStruct)) {         /* not enough data to fulfill this request to read a struct */
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

inline bool PUMALoggedMessageFileSource::_SetNextTimeStamp() {
  // read the next_event_
  // set next_event_timestamp_
  if (bulk_file_reader_.read(&next_event_, sizeof(PUMA_MDS::PumaCommonStruct)) <
      sizeof(PUMA_MDS::PumaCommonStruct)) {         /* not enough data to fulfill this request to read a struct */
    next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
    return false;
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

MktStatus_t PUMALoggedMessageFileSource::GetMarketStatus(PUMA_MDS::PumaCommonStruct& _next_event_) {
  switch (_next_event_.msg_) {
    case PUMA_MDS::PUMA_DELTA: {
      switch (_next_event_.data_.puma_dels_.flags[1]) {
        case 2: {
          return kMktTradingStatusPause;
        } break;
        case 4: {
          return kMktTradingStatusClosed;
        } break;
        case 17: {
          return kMktTradingStatusOpen;
        } break;
        case 18: {
          return kMktTradingStatusForbidden;
        } break;
        case 20: {
          return kMktTradingStatusUnknown;
        } break;
        case 21: {
          return kMktTradingStatusReserved;
        } break;
        case 101: {
          return kMktTradingStatuFinalClosingCall;
        } break;
        default: { return kMktTradingStatusOpen; } break;
      }
    } break;
    case PUMA_MDS::PUMA_TRADE: {
      switch (_next_event_.data_.puma_trds_.flags_[1]) {
        case 2: {
          return kMktTradingStatusPause;
        } break;
        case 4: {
          return kMktTradingStatusClosed;
        } break;
        case 17: {
          return kMktTradingStatusOpen;
        } break;
        case 18: {
          return kMktTradingStatusForbidden;
        } break;
        case 20: {
          return kMktTradingStatusUnknown;
        } break;
        case 21: {
          return kMktTradingStatusReserved;
        } break;
        case 101: {
          return kMktTradingStatuFinalClosingCall;
        } break;
        default: { return kMktTradingStatusOpen; } break;
      }
    } break;
    default: { return kMktTradingStatusOpen; }
  }
  return kMktTradingStatusOpen;
}

inline void PUMALoggedMessageFileSource::_ProcessThisMsg() {
  p_ntp_price_level_global_listener_->OnPriceLevelUpdate(&next_event_, sizeof(next_event_), HFSAT::MDS_MSG::BMF_EQ);
  if (!has_trading_started_) {
    if (IsNormalTradeTime(security_id_, next_event_timestamp_)) {
      has_trading_started_ = true;
    } else {
      return;
    }
  }

  if (!has_trading_ended_) {
    if (!IsNormalTradeTime(security_id_, next_event_timestamp_) && has_trading_started_) {
      has_trading_ended_ = true;
    }
  } else {
    return;
  }

  NotifyExternalDataListenerListener(security_id_);
  switch (next_event_.msg_) {
    case PUMA_MDS::PUMA_DELTA: {
      if (next_event_.data_.puma_dels_.level_ > 0) {
        // Only Update the Watch if the incoming message is of type non-intermediate
        if (!next_event_.data_.puma_dels_.intermediate_) {
          p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
        }

        MktStatus_t this_status_ = GetMarketStatus(next_event_);
        p_ntp_price_level_global_listener_->OnMarketStatusUpdate(security_id_, this_status_);
        if (this_status_ != kMktTradingStatusOpen) {
          return;
        }

        TradeType_t _buysell_ = kTradeTypeNoInfo;
        if (next_event_.data_.puma_dels_.type_ == '0') {
          _buysell_ = kTradeTypeBuy;
        } else if (next_event_.data_.puma_dels_.type_ == '1') {
          _buysell_ = kTradeTypeSell;
        }

        if (next_event_.data_.puma_dels_.action_ != 2 && next_event_.data_.puma_dels_.action_ != 3 &&
            next_event_.data_.puma_dels_.action_ != 4 && next_event_.data_.puma_dels_.price_ == 0) {
          break;
        }

        switch (next_event_.data_.puma_dels_.action_) {
          case 0: {
            p_ntp_price_level_global_listener_->OnPriceLevelNew(
                security_id_, _buysell_, next_event_.data_.puma_dels_.level_, next_event_.data_.puma_dels_.price_,
                next_event_.data_.puma_dels_.size_, next_event_.data_.puma_dels_.num_ords_,
                next_event_.data_.puma_dels_.intermediate_);
          } break;
          case 1: {
            p_ntp_price_level_global_listener_->OnPriceLevelChange(
                security_id_, _buysell_, next_event_.data_.puma_dels_.level_, next_event_.data_.puma_dels_.price_,
                next_event_.data_.puma_dels_.size_, next_event_.data_.puma_dels_.num_ords_,
                next_event_.data_.puma_dels_.intermediate_);
          } break;
          case 2: {
            p_ntp_price_level_global_listener_->OnPriceLevelDelete(
                security_id_, _buysell_, next_event_.data_.puma_dels_.level_, next_event_.data_.puma_dels_.price_,
                next_event_.data_.puma_dels_.intermediate_);
          } break;
          case 3: {
            p_ntp_price_level_global_listener_->OnPriceLevelDeleteThru(security_id_, _buysell_,
                                                                       next_event_.data_.puma_dels_.intermediate_);
          } break;
          case 4: {
            p_ntp_price_level_global_listener_->OnPriceLevelDeleteFrom(
                security_id_, _buysell_, next_event_.data_.puma_dels_.level_, next_event_.data_.puma_dels_.price_,
                next_event_.data_.puma_dels_.intermediate_);
          } break;
          default: {
            fprintf(stderr, "Weird message type in PUMALoggedMessageFileSource::ProcessAllEvents PUMA_DELTA: %d\n",
                    next_event_.data_.puma_dels_.action_);
          } break;
        }
      }
    } break;
    case PUMA_MDS::PUMA_TRADE: {
      p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

      MktStatus_t this_status_ = GetMarketStatus(next_event_);
      p_ntp_price_level_global_listener_->OnMarketStatusUpdate(security_id_, this_status_);
      if (this_status_ != kMktTradingStatusOpen) {
        return;
      }

      if (next_event_.data_.puma_trds_.flags_[0] != 'X') {
        p_ntp_price_level_global_listener_->OnTrade(security_id_, next_event_.data_.puma_trds_.trd_px_,
                                                    next_event_.data_.puma_trds_.trd_qty_);
      } else {
        p_ntp_price_level_global_listener_->OnOTCTrade(security_id_, next_event_.data_.puma_trds_.trd_px_,
                                                       next_event_.data_.puma_trds_.trd_qty_);
      }
    } break;
    // Temporary hack to prevent false error messages
    case PUMA_MDS::PUMA_OPENPRICE:
    case PUMA_MDS::PUMA_IMBALANCE:
    case PUMA_MDS::PUMA_STATUS: {
    } break;
    default: {
      fprintf(stderr, "Weird message type in PUMALoggedMessageFileSource::ProcessAllEvents : %d\n", next_event_.msg_);
    } break;
  }
}

void PUMALoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void PUMALoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}
}
