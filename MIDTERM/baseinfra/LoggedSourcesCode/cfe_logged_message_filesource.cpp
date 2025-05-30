/**
   \file MDSMessagesCode/cfe_logged_message_filesource.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <cmath>

#include "dvccode/CDef/security_definitions.hpp"
#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/cfe_logged_message_filesource.hpp"

namespace HFSAT {

CFELoggedMessageFileSource::CFELoggedMessageFileSource(
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
      yyyymmdd_(t_preevent_YYYYMMDD_),
      use_fake_faster_data_(t_use_fake_faster_data_) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  // find the filename
  std::string t_cfe_filename_ =
      CFELoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);

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

  DBGLOG_CLASS_FUNC << "For CFE symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                    << " trading_location " << trading_location_file_read_ << " DUsecs " << delay_usecs_to_add_
                    << " @use " << r_trading_location_ << " returned filename = " << t_cfe_filename_
                    << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(t_cfe_filename_);
  } else {
    std::cerr << "For CFE symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_cfe_filename_ << std::endl;
  }
}

void CFELoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // keep reading the next_event_
    // to check if next_event_timestamp_
    // if greater than r_endtime_
    do {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CSM_MDS::CSMCommonStruct));
      if (available_len_ < sizeof(CSM_MDS::CSMCommonStruct)) {  // data not found in file
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

void CFELoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CSM_MDS::CSMCommonStruct));
    if (available_len_ < sizeof(CSM_MDS::CSMCommonStruct)) {  // data not found in file
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

inline void CFELoggedMessageFileSource::_ProcessThisMsg() {
  NotifyExternalDataListenerListener(security_id_);
  p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
  p_price_level_global_listener_->OnPriceLevelUpdate(&next_event_, sizeof(next_event_), HFSAT::MDS_MSG::CSM);

  switch (next_event_.msg_) {
    case CSM_MDS::CSM_DELTA: {
      TradeType_t buysell_ = next_event_.data_.csm_dels_.type_ == '0' ? kTradeTypeBuy : kTradeTypeSell;

      switch (next_event_.data_.csm_dels_.action_) {
        case 0: {
          p_price_level_global_listener_->OnPriceLevelNew(
              security_id_, buysell_, next_event_.data_.csm_dels_.level_, next_event_.data_.csm_dels_.price_,
              next_event_.data_.csm_dels_.size_[0], 1, next_event_.data_.csm_dels_.intermediate_);
        } break;
        case 1: {
          p_price_level_global_listener_->OnPriceLevelChange(
              security_id_, buysell_, next_event_.data_.csm_dels_.level_, next_event_.data_.csm_dels_.price_,
              next_event_.data_.csm_dels_.size_[0], 1, next_event_.data_.csm_dels_.intermediate_);
        } break;
        case 2: {
          p_price_level_global_listener_->OnPriceLevelDelete(security_id_, buysell_, next_event_.data_.csm_dels_.level_,
                                                             next_event_.data_.csm_dels_.price_,
                                                             next_event_.data_.csm_dels_.intermediate_);
        } break;
        case 5: {
          p_price_level_global_listener_->OnPriceLevelOverlay(
              security_id_, buysell_, next_event_.data_.csm_dels_.level_, next_event_.data_.csm_dels_.price_,
              next_event_.data_.csm_dels_.size_[0], 1, next_event_.data_.csm_dels_.intermediate_);
        } break;
        case 9: {
        } break;
        default:
          break;
      }
    } break;

    case CSM_MDS::CSM_TRADE: {
      double trade_price = next_event_.data_.csm_trds_.trd_px_;

      // Spread trade
      if (next_event_.data_.csm_trds_.trade_condition[0] == 'S') {
        p_price_level_global_listener_->OnSpreadTrade(security_id_, trade_price, next_event_.data_.csm_trds_.trd_qty_);
      } else {
        p_price_level_global_listener_->OnTrade(security_id_, trade_price, next_event_.data_.csm_trds_.trd_qty_);
      }
    } break;

    case CSM_MDS::CSM_TOB: {
      if (next_event_.data_.csm_dels_.size_[0] == 0 && next_event_.data_.csm_dels_.intermediate_) {
        return;
      }

      TradeType_t buysell_ = next_event_.data_.csm_dels_.type_ == '0' ? kTradeTypeBuy : kTradeTypeSell;
      p_price_level_global_listener_->OnPriceLevelChange(
          security_id_, buysell_, next_event_.data_.csm_dels_.level_, next_event_.data_.csm_dels_.price_,
          next_event_.data_.csm_dels_.size_[0], 1, next_event_.data_.csm_dels_.intermediate_);
    } break;

    default: { } break; }
}

inline bool CFELoggedMessageFileSource::_SetNextTimeStamp() {
  // read the next_event_
  // set next_event_timestamp_
  register size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CSM_MDS::CSMCommonStruct));
  if (available_len_ <
      sizeof(CSM_MDS::CSMCommonStruct)) { /* not enough data to fulfill this request to read a struct */
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

void CFELoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void CFELoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}
}
