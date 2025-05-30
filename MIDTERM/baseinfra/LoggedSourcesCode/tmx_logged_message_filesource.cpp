/**
    \file MDSMessagesCode/tmx_logged_message_filesource.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         India
         +91 80 4190 3551
*/
#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filesource.hpp"

namespace HFSAT {

TMXLoggedMessageFileSource::TMXLoggedMessageFileSource(
    DebugLogger& t_debuglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, TradingLocation_t r_trading_location_,
    bool t_use_fake_faster_data_)
    : ExternalDataListener(),
      dbglogger_(t_debuglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      m_time_keeper_(NULL),
      bulk_file_reader_(),
      next_event_(),
      trading_location_file_read_(r_trading_location_),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      tmx_trade_time_manager_(HFSAT::TmxTradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_preevent_YYYYMMDD_)),
      fullbook_listener_vec_(),
      use_fake_faster_data_(t_use_fake_faster_data_) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  std::string m_tmx_filename_ =
      TMXLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
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
    bulk_file_reader_.open(m_tmx_filename_);
  } else {
    std::cerr << "For TMX symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << m_tmx_filename_ << std::endl;
  }
  // bulk_file_reader_.open ( m_tmx_filename_ ) ;
}

void TMXLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {
    do {
      size_t available_len = bulk_file_reader_.read(&next_event_, sizeof(TMX_MDS::TMXCommonStruct));
      if (available_len < sizeof(TMX_MDS::TMXCommonStruct)) {
        next_event_timestamp_ = ttime_t(time_t(0), 0);
        rw_hasevents_ = false;
        break;
      }
      next_event_timestamp_ = next_event_.time_;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    } while (next_event_timestamp_ < r_start_time);
  } else {
    rw_hasevents_ = false;
  }
}

void TMXLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& _hasevents_) {
  if (bulk_file_reader_.is_open()) {
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(TMX_MDS::TMXCommonStruct));
    if (available_len_ < sizeof(TMX_MDS::TMXCommonStruct)) {
      next_event_timestamp_ = ttime_t(time_t(0), 0);
      _hasevents_ = false;
    } else {
      next_event_timestamp_ = next_event_.time_;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  } else {
    _hasevents_ = false;
  }
}

void TMXLoggedMessageFileSource::_ProcessThisMsg() {
  NotifyExternalDataListenerListener(security_id_);
  m_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

  switch (next_event_.msg_) {
    case TMX_MDS::TMX_QUOTE:
      break;
    case TMX_MDS::TMX_TRADE: {
      if (!IsNormalTradeTime(security_id_, next_event_timestamp_)) break;

      if (next_event_.data_.tmx_trds_.type_ == ' ') {
        NotifyTrade(security_id_, next_event_.data_.tmx_trds_.trd_px_, next_event_.data_.tmx_trds_.trd_qty_,
                    kTradeTypeNoInfo);
      }
    } break;
    case TMX_MDS::TMX_BOOK: {
      if (next_event_.data_.tmx_books_.status_ != 'T') break;

      NotifyFullBookChange(security_id_, (FullBook*)(next_event_.data_.tmx_books_.bid_pxs_));
    } break;
    default: {
      dbglogger_ << "Weird msgtype " << next_event_.msg_ << " in " << __func__ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }
  }
}

void TMXLoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(TMX_MDS::TMXCommonStruct));
    if (available_len_ < sizeof(TMX_MDS::TMXCommonStruct)) {
      next_event_timestamp_ = ttime_t(time_t(0), 0);
      break;
    } else {
      next_event_timestamp_ = next_event_.time_;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  }
}

void TMXLoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(TMX_MDS::TMXCommonStruct));
    if (available_len_ < sizeof(TMX_MDS::TMXCommonStruct)) {
      next_event_timestamp_ = ttime_t(time_t(0), 0);
      break;
    } else {
      next_event_timestamp_ = next_event_.time_;
      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  }
}

void TMXLoggedMessageFileSource::AddFullBookGlobalListener(FullBookGlobalListener* p_this_listener_) {
  if (p_this_listener_ != NULL) {
    VectorUtils::UniqueVectorAdd(fullbook_listener_vec_, p_this_listener_);
  }
}

void TMXLoggedMessageFileSource::NotifyFullBookChange(const unsigned int t_security_id_, const FullBook* t_full_book_) {
  for (std::vector<FullBookGlobalListener*>::iterator iter_ = fullbook_listener_vec_.begin();
       iter_ != fullbook_listener_vec_.end(); iter_++) {
    (*iter_)->OnFullBookChange(t_security_id_, t_full_book_);
  }
}

void TMXLoggedMessageFileSource::NotifyTrade(const unsigned int t_security_id_, const double t_trade_price_,
                                             const int t_trade_size_, const TradeType_t t_buysell_) {
  for (std::vector<FullBookGlobalListener*>::iterator iter_ = fullbook_listener_vec_.begin();
       iter_ != fullbook_listener_vec_.end(); iter_++) {
    (*iter_)->OnTrade(t_security_id_, t_trade_price_, t_trade_size_, t_buysell_);
  }
}
}
