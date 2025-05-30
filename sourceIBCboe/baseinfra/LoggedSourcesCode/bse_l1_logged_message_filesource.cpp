/**
   \file MDSMessagesCode/bse_logged_message_filesource.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <algorithm>
#include <cstring>
#include "baseinfra/LoggedSources/bse_l1_logged_message_filesource.hpp"

namespace HFSAT {

BSEL1LoggedMessageFileSource::BSEL1LoggedMessageFileSource(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
    bool use_todays_data_)
    : ExternalDataListener(),
      dbglogger_(t_dbglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      listener_(nullptr),
      p_time_keeper_(NULL),
      bulk_file_reader_(),
      next_event_(),
      trading_location_file_read_(r_trading_location_),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      skip_intermediate_message_(false),
      actual_data_skip_(false),
      last_seen_delta_type_(HFSAT::kTradeTypeNoInfo),
      is_spot_(false)	{
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  strcpy(shortcode_, BSESecurityDefinitions::GetShortCodeFromExchangeSymbol(exchange_symbol_).c_str());
  std::string t_bse_filename_ =
      BSEL1LoggedMessageFileNamer::GetName(exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
  is_spot_ = BSESecurityDefinitions::IsSpotIndex(shortcode_);
  // should be irrelevant currently

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
          sec_name_indexer_.GetShortcodeFromId(security_id_), r_trading_location_, t_preevent_YYYYMMDD_, false) !=
      0) {  // Experiment with the possibility of using faster data for this product at this location.
    need_to_add_delay_usecs_ = true;
    delay_usecs_to_add_ -= TradingLocationUtils::GetUsecsForFasterDataForShortcodeAtLocation(
        sec_name_indexer_.GetShortcodeFromId(security_id_), r_trading_location_, t_preevent_YYYYMMDD_, false);
  }

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(t_bse_filename_);
  } else {
    bulk_file_reader_.open(t_bse_filename_);
    std::cerr << "For BSE symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_bse_filename_ << std::endl;
  }
}

void BSEL1LoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {

  if(false == actual_data_skip_){

    if (bulk_file_reader_.is_open()) {
      if (next_event_timestamp_.tv_sec == 0 && next_event_timestamp_.tv_usec == 0) {
        // read the next_event_
        // set next_event_timestamp_
        size_t available_len_;
        do {
          available_len_ = bulk_file_reader_.read(&next_event_, sizeof(GenericL1DataStruct));
        } while (available_len_ == sizeof(GenericL1DataStruct) && strcmp(next_event_.symbol, shortcode_) != 0);

        if (available_len_ < sizeof(GenericL1DataStruct)) { /* not enough data to fulfill this request to read a struct */
          next_event_timestamp_ =
              ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
          rw_hasevents_ = false;
        } else {
          next_event_timestamp_ = next_event_.time;
          if (need_to_add_delay_usecs_) {
            next_event_timestamp_.addusecs(delay_usecs_to_add_);
          }
        }
      }
    } else {
      rw_hasevents_ = false;
    }

  }else{

    if (bulk_file_reader_.is_open()) {
      if (next_event_timestamp_.tv_sec == 0 && next_event_timestamp_.tv_usec == 0) {
        // read the next_event_
        // set next_event_timestamp_
        size_t available_len_;
        do {
          available_len_ = bulk_file_reader_.read(&next_event_, sizeof(GenericL1DataStruct));
          next_event_timestamp_ = next_event_.time;

        } while (available_len_ == sizeof(GenericL1DataStruct) &&
                 next_event_timestamp_ < r_start_time_);

        if (available_len_ <
            sizeof(GenericL1DataStruct)) { /* not enough data to fulfill this request to read a struct */
          next_event_timestamp_ =
              ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
          rw_hasevents_ = false;
        } else {
          next_event_timestamp_ = next_event_.time;
          if (need_to_add_delay_usecs_) {
            next_event_timestamp_.addusecs(delay_usecs_to_add_);
          }
        }
      }
    } else {
      rw_hasevents_ = false;
    }

  }

  SetTimeToSkipUntilFirstEvent(r_start_time_);
}

void BSEL1LoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& _hasevents_) {
  if (bulk_file_reader_.is_open()) {
    // read the next_event_
    // set next_event_timestamp_
    size_t available_len_;
    do {
      available_len_ = bulk_file_reader_.read(&next_event_, sizeof(GenericL1DataStruct));
    } while (available_len_ == sizeof(GenericL1DataStruct) && strcmp(next_event_.symbol, shortcode_) != 0);

    if (available_len_ < sizeof(GenericL1DataStruct)) { /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      _hasevents_ = false;
    } else {
      next_event_timestamp_ = next_event_.time;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  } else {
    _hasevents_ = false;
  }
}

void BSEL1LoggedMessageFileSource::_ProcessThisMsg() {
  NotifyExternalDataListenerListener(security_id_);
  p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

  if (listener_) {
    switch (next_event_.type) {
      case GenericL1DataType::L1_DELTA:
        listener_->OnL1New(security_id_, next_event_);
        break;

      case GenericL1DataType::L1_TRADE:
	if (is_spot_)
          listener_->OnIndexPrice(security_id_, next_event_.price);
	else
          listener_->OnTrade(security_id_, next_event_);
        break;
      default:
        dbglogger_ << "BSEL1LoggedMessageFileSource::ProcessThisMsg : Error. Invalid Msge Type\n";
        break;
    }
  }
}

void BSEL1LoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();
    size_t available_len_;
    do {
      available_len_ = bulk_file_reader_.read(&next_event_, sizeof(GenericL1DataStruct));
    } while (available_len_ == sizeof(GenericL1DataStruct) && strcmp(next_event_.symbol, shortcode_) != 0);

    if (available_len_ < sizeof(GenericL1DataStruct)) { /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      break;
    } else {
      next_event_timestamp_ = next_event_.time;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  }
}

void BSEL1LoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    // read the next_event_
    // set next_event_timestamp_
    size_t available_len_;
    do {
      available_len_ = bulk_file_reader_.read(&next_event_, sizeof(GenericL1DataStruct));
    } while (available_len_ == sizeof(GenericL1DataStruct) && strcmp(next_event_.symbol, shortcode_) != 0);

    if (available_len_ < sizeof(GenericL1DataStruct)) { /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      break;
    } else {
      next_event_timestamp_ = next_event_.time;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  }
}

void BSEL1LoggedMessageFileSource::SetTimeToSkipUntilFirstEvent(const ttime_t start_time) {
  if (listener_) {
    listener_->SetTimeToSkipUntilFirstEvent(start_time);
  }
}

void BSEL1LoggedMessageFileSource::SetL1DataListener(L1DataListener* listener) { listener_ = listener; }
}
