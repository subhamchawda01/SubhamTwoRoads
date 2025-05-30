/**
    \file MDSMessagesCode/hkex_logged_message_filesource.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066
         India
         +91 80 4060 0717
 */
#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/hkomd_logged_message_filesource.hpp"

namespace HFSAT {

HKOMDPFLoggedMessageFileSource::HKOMDPFLoggedMessageFileSource(
    DebugLogger& t_debuglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, TradingLocation_t r_trading_location_,
    bool use_todays_data_, bool t_use_fake_faster_data_, bool is_hk_eq)
    : ExternalDataListener(),
      dbglogger_(t_debuglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      price_level_global_listener_hkomd_(NULL),
      m_time_keeper_(NULL),
      bulk_file_reader_(),
      next_event_(),
      trading_location_file_read_(r_trading_location_),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      hkex_trade_time_manager_(HFSAT::HkexTradeTimeManager::GetUniqueInstance(t_sec_name_indexer_)),
      use_fake_faster_data_(t_use_fake_faster_data_),
      is_hk_eq_(is_hk_eq)

{
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;
  dbglogger_ << "in constructor\n";
  dbglogger_.DumpCurrentBuffer();

  std::string m_hkex_filename_ = "";

  if (is_hk_eq_) {
    m_hkex_filename_ = HKStocksPFLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                                 trading_location_file_read_, use_todays_data_);
  } else {
    m_hkex_filename_ = HKOMDCPFLoggedMessagefileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                               trading_location_file_read_, use_todays_data_);
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
  //     dbglogger_
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
  if (m_hkex_filename_.find("NO_FILE_AVAILABLE") == std::string::npos) {
    bulk_file_reader_.open(m_hkex_filename_);
  } else {
    dbglogger_ << "For HKEX symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
               << trading_location_file_read_ << " returned filename = " << m_hkex_filename_ << "\n";
    dbglogger_.DumpCurrentBuffer();

    std::cerr << "For HKEX symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << m_hkex_filename_ << std::endl;
  }
  // bulk_file_reader_.open ( m_hkex_filename_ ) ;
}

void HKOMDLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {
    do {
      size_t available_len = bulk_file_reader_.read(&next_event_, sizeof(HKOMD_MDS::HKOMDCommonStruct));
      if (available_len < sizeof(HKOMD_MDS::HKOMDCommonStruct)) {
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

void HKOMDLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& _hasevents_) {
  if (bulk_file_reader_.is_open()) {
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKOMD_MDS::HKOMDCommonStruct));
    if (available_len_ < sizeof(HKOMD_MDS::HKOMDCommonStruct)) {
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

inline bool HKOMDLoggedMessageFileSource::SetNextTimeStamp() {
  // read the next_event_
  // set next_event_timestamp_
  register size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKOMD_MDS::HKOMDCommonStruct));
  if (available_len_ < sizeof(HKOMD_MDS::HKOMDCommonStruct)) {
    next_event_timestamp_ = ttime_t(time_t(0), 0);
    return false;
  } else {
    next_event_timestamp_ = next_event_.time_;
    if (need_to_add_delay_usecs_) {
      next_event_timestamp_.addusecs(delay_usecs_to_add_);
    }
    return true;
  }
  return true;
}

void HKOMDLoggedMessageFileSource::ProcessThisMsg() {
  NotifyExternalDataListenerListener(security_id_);
  m_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

  switch (next_event_.msg_) {
    case HKOMD_MDS::HKOMD_TRADE: {
      // else outside matching engine or structured_trading trades, passive order hits only
      if (next_event_.data_.trade_.deal_type_ != 1 && next_event_.data_.trade_.deal_type_ != 3 &&
          next_event_.data_.trade_.order_id_ != 0) {
        return;
      }

      TradeType_t t_buy_sell_ = kTradeTypeNoInfo;
      if (next_event_.data_.trade_.side_ == 2) {
        t_buy_sell_ = kTradeTypeBuy;
      } else if (next_event_.data_.trade_.side_ == 3) {
        t_buy_sell_ = kTradeTypeSell;
      }

      if (order_level_global_listener_hkomd_) {
        order_level_global_listener_hkomd_->OnTrade(security_id_, next_event_.data_.trade_.order_id_,
                                                    next_event_.data_.trade_.price_, next_event_.data_.trade_.quantity_,
                                                    t_buy_sell_);
      }

      NotifyOrderExec(security_id_, t_buy_sell_, next_event_.data_.trade_.order_id_, next_event_.data_.trade_.price_,
                      next_event_.data_.trade_.quantity_);
    } break;
    case HKOMD_MDS::HKOMD_ORDER: {
      TradeType_t t_buy_sell_ = kTradeTypeNoInfo;
      if (next_event_.data_.order_.side_ == 0) {
        t_buy_sell_ = kTradeTypeBuy;
      } else if (next_event_.data_.order_.side_ == 1) {
        t_buy_sell_ = kTradeTypeSell;
      }

      bool intermediate_ = false;
      intermediate_ =
          ((int(next_event_.data_.order_.intermediate_) == 1) || (next_event_.data_.order_.intermediate_ == 'Y') ||
           (next_event_.data_.order_.intermediate_ == 'I'));

      switch (next_event_.data_.order_.action_) {
        case '0':  // add order
        {
          if (order_level_global_listener_hkomd_) {
            order_level_global_listener_hkomd_->OnOrderAdd(
                security_id_, next_event_.data_.order_.order_id_, next_event_.data_.order_.price_,
                next_event_.data_.order_.quantity_, t_buy_sell_, intermediate_);
          }

          NotifyOrderAdd(security_id_, t_buy_sell_, next_event_.data_.order_.order_id_, next_event_.data_.order_.price_,
                         next_event_.data_.order_.quantity_);
        } break;
        case '1':  // Modify
        {
          if (order_level_global_listener_hkomd_) {
            order_level_global_listener_hkomd_->OnOrderModify(
                security_id_, next_event_.data_.order_.order_id_, next_event_.data_.order_.price_,
                next_event_.data_.order_.quantity_, t_buy_sell_, intermediate_);
          }

          NotifyOrderModify(security_id_, t_buy_sell_, next_event_.data_.order_.order_id_,
                            next_event_.data_.order_.price_, next_event_.data_.order_.quantity_);
        } break;
        case '2': {
          if (order_level_global_listener_hkomd_) {
            order_level_global_listener_hkomd_->OnOrderDelete(security_id_, next_event_.data_.order_.order_id_,
                                                              t_buy_sell_, false, intermediate_);
          }

          NotifyOrderDelete(security_id_, t_buy_sell_, next_event_.data_.order_.order_id_);
        } break;
        case '3':
        case '4': {
          NotifyResetBook(security_id_);
        } break;
        default: { std::cerr << " action not recognised for hkomd event\n"; } break;
      }
    } break;
    default: {
      dbglogger_ << "Weird msgtype " << next_event_.msg_ << " in " << __func__ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      exit(-1);
    } break;
  }
}

void HKOMDLoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    ProcessThisMsg();

    if (!SetNextTimeStamp()) return;
  }
}

void HKOMDLoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  while (next_event_timestamp_ <= _endtime_) {
    ProcessThisMsg();

    if (!SetNextTimeStamp()) return;
  }
}

void HKOMDLoggedMessageFileSource::AddOrderLevelListener(OrderLevelListenerHK* p_this_listener_) {
  if (p_this_listener_ != NULL) {
    VectorUtils::UniqueVectorAdd(order_level_listener_vec_, p_this_listener_);
  }
}

void HKOMDLoggedMessageFileSource::NotifyOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                  const int64_t t_order_id_, const double t_price_,
                                                  const uint32_t t_size_) {
  for (std::vector<OrderLevelListenerHK*>::iterator iter_ = order_level_listener_vec_.begin();
       iter_ != order_level_listener_vec_.end(); iter_++) {
    (*iter_)->OnOrderAdd(t_security_id_, t_buysell_, t_order_id_, t_price_, t_size_);
  }
}

void HKOMDLoggedMessageFileSource::NotifyOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                     const int64_t t_order_id_, const double t_price_,
                                                     const uint32_t t_size_) {
  for (std::vector<OrderLevelListenerHK*>::iterator iter_ = order_level_listener_vec_.begin();
       iter_ != order_level_listener_vec_.end(); iter_++) {
    (*iter_)->OnOrderModify(t_security_id_, t_buysell_, t_order_id_, t_price_, t_size_);
  }
}

void HKOMDLoggedMessageFileSource::NotifyOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                     const int64_t t_order_id_) {
  for (std::vector<OrderLevelListenerHK*>::iterator iter_ = order_level_listener_vec_.begin();
       iter_ != order_level_listener_vec_.end(); iter_++) {
    (*iter_)->OnOrderDelete(t_security_id_, t_buysell_, t_order_id_);
  }
}

void HKOMDLoggedMessageFileSource::NotifyOrderExec(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                   const int64_t t_order_id_, const double t_traded_price_,
                                                   const uint32_t t_traded_size_) {
  for (std::vector<OrderLevelListenerHK*>::iterator iter_ = order_level_listener_vec_.begin();
       iter_ != order_level_listener_vec_.end(); iter_++) {
    (*iter_)->OnOrderExec(security_id_, t_buysell_, t_order_id_, t_traded_price_, t_traded_size_);
  }
}

void HKOMDLoggedMessageFileSource::NotifyResetBook(const unsigned int t_security_id_) {
  for (std::vector<OrderLevelListenerHK*>::iterator iter_ = order_level_listener_vec_.begin();
       iter_ != order_level_listener_vec_.end(); iter_++) {
    (*iter_)->ResetBook(t_security_id_);
  }
}
}
