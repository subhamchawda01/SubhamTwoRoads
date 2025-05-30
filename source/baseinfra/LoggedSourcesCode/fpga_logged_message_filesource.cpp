/**
    \file MDSMessagesCode/fpga_logged_message_filesource.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         India
         +91 80 4190 3551
*/

#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/fpga_logged_message_filesource.hpp"

namespace HFSAT {

FPGALoggedMessageFileSource::FPGALoggedMessageFileSource(
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
      halfbook_listener_vec_(),
      use_fake_faster_data_(t_use_fake_faster_data_) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  std::string m_fpga_filename_ = CommonLoggedMessageFileNamer::GetName(
      kExchSourceCME_FPGA, t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
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

  DBGLOG_CLASS_FUNC << "For symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
                    << trading_location_file_read_ << " DUsecs " << delay_usecs_to_add_ << " @use "
                    << r_trading_location_ << " returned filename = " << m_fpga_filename_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(m_fpga_filename_);
  } else {
    std::cerr << "For FPGA symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << m_fpga_filename_ << std::endl;
  }
}

void FPGALoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {
    do {
      size_t available_len = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMEFPGACommonStruct));
      if (available_len < sizeof(CME_MDS::CMEFPGACommonStruct)) {
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

void FPGALoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& _hasevents_) {
  if (bulk_file_reader_.is_open()) {
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMEFPGACommonStruct));
    if (available_len_ < sizeof(CME_MDS::CMEFPGACommonStruct)) {
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

void FPGALoggedMessageFileSource::_ProcessThisMsg() {
  NotifyExternalDataListenerListener(security_id_);
  m_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

  switch (next_event_.msg_) {
    case CME_MDS::CME_DELTA: {
      bool new_update_bid_, new_update_ask_;
      new_update_bid_ = new_update_ask_ = false;
      for (int ctr = 0; ctr < 5; ctr++) {
        fpga_half_book_[0].num_orders_[ctr] = 0;
        fpga_half_book_[0].sizes_[ctr] = -1;
        fpga_half_book_[0].prices_[ctr] = 0;
        fpga_half_book_[1].num_orders_[ctr] = 0;
        fpga_half_book_[1].sizes_[ctr] = -1;
        fpga_half_book_[1].prices_[ctr] = 0;
      }
      for (int ctr = 0; ctr < 5; ++ctr) {
        fpga_half_book_[0].num_orders_[ctr] = next_event_.data_.cme_dels_[0].num_orders_[ctr];
        fpga_half_book_[0].sizes_[ctr] = next_event_.data_.cme_dels_[0].sizes_[ctr];
        fpga_half_book_[0].prices_[ctr] = next_event_.data_.cme_dels_[0].prices_[ctr];
        if (next_event_.data_.cme_dels_[0].sizes_[ctr] != -1) {
          new_update_bid_ = true;
        }
        fpga_half_book_[1].num_orders_[ctr] = next_event_.data_.cme_dels_[1].num_orders_[ctr];
        fpga_half_book_[1].sizes_[ctr] = next_event_.data_.cme_dels_[1].sizes_[ctr];
        fpga_half_book_[1].prices_[ctr] = next_event_.data_.cme_dels_[1].prices_[ctr];
        if (next_event_.data_.cme_dels_[1].sizes_[ctr] != -1) {
          new_update_ask_ = true;
        }
      }
      bool is_intermediate = new_update_ask_ && new_update_bid_;
      if (new_update_bid_) {
        NotifyHalfBookChange(security_id_, HFSAT::kTradeTypeBuy, &(fpga_half_book_[0]), is_intermediate);
      }
      if (new_update_ask_) {
        NotifyHalfBookChange(security_id_, HFSAT::kTradeTypeSell, &(fpga_half_book_[1]), false);
      }

    } break;

    case CME_MDS::CME_TRADE: {
      TradeType_t trdType;
      if (next_event_.data_.cme_trds_.type_ == '0') {
        trdType = HFSAT::kTradeTypeBuy;
      } else if (next_event_.data_.cme_trds_.type_ == '1') {
        trdType = HFSAT::kTradeTypeSell;
      } else {
        trdType = HFSAT::kTradeTypeNoInfo;
      }

      NotifyTrade(security_id_, next_event_.data_.cme_trds_.price_, next_event_.data_.cme_trds_.size_, trdType);
    } break;
    default: {
      dbglogger_ << "Weird msgtype " << next_event_.msg_ << " in " << __func__ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }
  }
}

void FPGALoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMEFPGACommonStruct));
    if (available_len_ < sizeof(CME_MDS::CMEFPGACommonStruct)) {
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

void FPGALoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMEFPGACommonStruct));
    if (available_len_ < sizeof(CME_MDS::CMEFPGACommonStruct)) {
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

void FPGALoggedMessageFileSource::AddHalfBookGlobalListener(FPGAHalfBookGlobalListener* p_this_listener_) {
  if (p_this_listener_ != NULL) {
    VectorUtils::UniqueVectorAdd(halfbook_listener_vec_, p_this_listener_);
  }
}

void FPGALoggedMessageFileSource::NotifyHalfBookChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                       FPGAHalfBook* t_half_book_, bool is_intermediate_) {
  for (std::vector<FPGAHalfBookGlobalListener*>::iterator iter_ = halfbook_listener_vec_.begin();
       iter_ != halfbook_listener_vec_.end(); iter_++) {
    (*iter_)->OnHalfBookChange(t_security_id_, t_buysell_, t_half_book_, is_intermediate_);
  }
}

void FPGALoggedMessageFileSource::NotifyTrade(const unsigned int t_security_id_, const double t_trade_price_,
                                              const int t_trade_size_, const TradeType_t t_buysell_) {
  for (std::vector<FPGAHalfBookGlobalListener*>::iterator iter_ = halfbook_listener_vec_.begin();
       iter_ != halfbook_listener_vec_.end(); iter_++) {
    (*iter_)->OnTrade(t_security_id_, t_trade_price_, t_trade_size_, t_buysell_);
  }
}
}
