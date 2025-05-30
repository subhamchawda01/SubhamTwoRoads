#include "dvccode/CDef/mds_messages.hpp"
#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/asx_pf_logged_message_filesource.hpp"

namespace HFSAT {

ASXPFLoggedMessageFileSource::ASXPFLoggedMessageFileSource(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
    bool t_use_fake_faster_data_)
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
      trade_time_manager_(HFSAT::TradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_preevent_YYYYMMDD_)),
      has_trading_hours_started_(false),
      has_trading_hours_closed_(false),
      use_fake_faster_data_(t_use_fake_faster_data_),
      price_level_global_listener_vec_(),
      YYYYMMDD_(t_preevent_YYYYMMDD_) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;
  start_timestamp_.tv_sec = 0;
  start_timestamp_.tv_usec = 0;
  end_timestamp_.tv_sec = 0;
  end_timestamp_.tv_usec = 0;

  shortcode_ = sec_name_indexer_.GetShortcodeFromId(security_id_);

  // find the filename
  std::string t_asx_filename_;
  if (HFSAT::HybridSecurityManager::IsHybridExch(std::string(t_exchange_symbol_))) {
    t_asx_filename_ = CommonLoggedMessageFileNamer::GetName(
        kExchSourceASX,
        (HFSAT::HybridSecurityManager::GetActualExchFromHybrid(std::string(t_exchange_symbol_))).c_str(),
        t_preevent_YYYYMMDD_, trading_location_file_read_);
  } else {
    t_asx_filename_ = CommonLoggedMessageFileNamer::GetName(kExchSourceASX, t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                            trading_location_file_read_);
  }

  HandleExpiryWeek(t_asx_filename_);

  int added_delay = 0;
  // added delays is used by the robustness check script to variate line delay by a constant number
  added_delay = TradingLocationUtils::GetAddedDelay(trading_location_file_read_, r_trading_location_);
  delay_usecs_to_add_ = added_delay;

  if (trading_location_file_read_ != r_trading_location_) {
    delay_usecs_to_add_ +=
        TradingLocationUtils::GetUSecsBetweenTradingLocations(trading_location_file_read_, r_trading_location_);
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

  DBGLOG_CLASS_FUNC << "For ASXPF symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                    << " trading_location " << trading_location_file_read_ << " DUsecs " << delay_usecs_to_add_
                    << " @use " << r_trading_location_ << " returned filename = " << t_asx_filename_
                    << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(t_asx_filename_);
  } else {
    std::cerr << "For ASXPF symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_asx_filename_ << std::endl;
  }
}

void ASXPFLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // keep reading the next_event_
    // to check if next_event_timestamp_
    // if greater than r_endtime_
    do {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(ASX_MDS::ASXPFCommonStruct));
      if (available_len_ < sizeof(ASX_MDS::ASXPFCommonStruct)) {  // data not found in file
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

    if (rw_hasevents_ && start_timestamp_ > next_event_timestamp_) {
      SeekToFirstEventAfter(start_timestamp_, rw_hasevents_);
      std::cout << YYYYMMDD_ << " " << shortcode_ << " Skipping events from begenning till AST_1645" << std::endl;
    }

  } else {  // data file not open
    rw_hasevents_ = false;
  }
}

void ASXPFLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(ASX_MDS::ASXPFCommonStruct));
    if (available_len_ < sizeof(ASX_MDS::ASXPFCommonStruct)) {  // data not found in file
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      rw_hasevents_ = false;
    } else {
      next_event_timestamp_ =
          next_event_.time_;  // need more work here to optimize, or preferably send ttime_t from MDS or ORS

      if (start_timestamp_ > next_event_timestamp_) {
        SeekToFirstEventAfter(start_timestamp_, rw_hasevents_);
        std::cout << YYYYMMDD_ << " " << shortcode_ << " Skipping events from begenning till AST_1645" << std::endl;
      }

      if (need_to_add_delay_usecs_) {
        next_event_timestamp_.addusecs(delay_usecs_to_add_);
      }
    }
  } else {
    rw_hasevents_ = false;
  }
}

inline void ASXPFLoggedMessageFileSource::_ProcessThisMsg() {
  // Skip msg if product break time
  NotifyExternalDataListenerListener(security_id_);
  if (!IsNormalTradeTime(security_id_, next_event_timestamp_)) return;

  p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
  NotifyPriceLevelUpdate(&next_event_, sizeof(next_event_));

  switch (next_event_.msg_) {
    case ASX_MDS::ASX_PF_DELTA: {
      TradeType_t buysell_ = kTradeTypeNoInfo;
      if (next_event_.data_.delta_.side_ == 'S') {
        buysell_ = kTradeTypeSell;
      } else if (next_event_.data_.delta_.side_ == 'B') {
        buysell_ = kTradeTypeBuy;
      }

      switch (next_event_.data_.delta_.action_) {
        case '0': {
          NotifyPriceLevelNew(security_id_, buysell_, next_event_.data_.delta_.level_, next_event_.data_.delta_.price_,
                              next_event_.data_.delta_.quantity_, next_event_.data_.delta_.num_orders_, false);
        } break;
        case '1': {
          NotifyPriceLevelChange(security_id_, buysell_, next_event_.data_.delta_.level_,
                                 next_event_.data_.delta_.price_, next_event_.data_.delta_.quantity_,
                                 next_event_.data_.delta_.num_orders_, false);
        } break;
        case '2': {
          NotifyPriceLevelDelete(security_id_, buysell_, next_event_.data_.delta_.level_,
                                 next_event_.data_.delta_.price_, false);
        } break;
        default: {
          std::cerr << "Unknown action in ASX_DELTA: " << next_event_.data_.delta_.action_ << std::endl;
        } break;
      }
    } break;
    case ASX_MDS::ASX_PF_TRADE: {
      TradeType_t buysell_ = kTradeTypeNoInfo;
      if (next_event_.data_.trade_.side_ == 'B') {
        buysell_ = kTradeTypeBuy;
      } else if (next_event_.data_.trade_.side_ == 'S') {
        buysell_ = kTradeTypeSell;
      }

      NotifyTrade(security_id_, next_event_.data_.trade_.price_, next_event_.data_.trade_.quantity_, buysell_);
    } break;
    case ASX_MDS::ASX_PF_RESET_BEGIN: {
      for (auto listener_ : price_level_global_listener_vec_) {
        listener_->OnResetBegin(security_id_);
      }
    } break;
    case ASX_MDS::ASX_PF_RESET_END: {
      for (auto listener_ : price_level_global_listener_vec_) {
        listener_->OnResetEnd(security_id_);
      }
    } break;
    default: { std::cerr << "Unknown msg_type in ASXPF: " << next_event_.msg_ << std::endl; } break;
  }
}

inline bool ASXPFLoggedMessageFileSource::_SetNextTimeStamp() {
  // read the next_event_
  // set next_event_timestamp_
  register size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(ASX_MDS::ASXPFCommonStruct));
  if (available_len_ < sizeof(ASX_MDS::ASXPFCommonStruct)) {
    next_event_timestamp_ = ttime_t(time_t(0), 0);
    return false;
  } else if (end_timestamp_ != ttime_t(0, 0) && next_event_.time_ > end_timestamp_) {
    std::cout << YYYYMMDD_ << " " << shortcode_ << " Skipping all events after AST_1645" << std::endl;
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

void ASXPFLoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void ASXPFLoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void ASXPFLoggedMessageFileSource::SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {}

void ASXPFLoggedMessageFileSource::AddPriceLevelGlobalListener(PriceLevelGlobalListener* p_this_listener_) {
  if (p_this_listener_ != NULL) {
    VectorUtils::UniqueVectorAdd(price_level_global_listener_vec_, p_this_listener_);
  }
}

void ASXPFLoggedMessageFileSource::NotifyPriceLevelUpdate(void* ptr_to_price_level_update, int length_of_bytes) {
  for (auto listener_ : price_level_global_listener_vec_) {
    listener_->OnPriceLevelUpdate(ptr_to_price_level_update, length_of_bytes, HFSAT::MDS_MSG::ASX);
  }
}

void ASXPFLoggedMessageFileSource::NotifyPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                       const int t_level_added_, const double t_price_,
                                                       const int t_new_size_, const int t_new_ordercount_,
                                                       const bool t_is_intermediate_message_) {
  for (auto listener_ : price_level_global_listener_vec_) {
    listener_->OnPriceLevelNew(t_security_id_, t_buysell_, t_level_added_, t_price_, t_new_size_, t_new_ordercount_,
                               t_is_intermediate_message_);
  }
}

void ASXPFLoggedMessageFileSource::NotifyPriceLevelDelete(const unsigned int t_security_id_,
                                                          const TradeType_t t_buysell_, const int t_level_removed_,
                                                          const double t_price_,
                                                          const bool t_is_intermediate_message_) {
  for (auto listener_ : price_level_global_listener_vec_) {
    listener_->OnPriceLevelDelete(t_security_id_, t_buysell_, t_level_removed_, t_price_, t_is_intermediate_message_);
  }
}

void ASXPFLoggedMessageFileSource::NotifyPriceLevelChange(const unsigned int t_security_id_,
                                                          const TradeType_t t_buysell_, const int t_level_changed_,
                                                          const double t_price_, const int t_new_size_,
                                                          const int t_new_ordercount_,
                                                          const bool t_is_intermediate_message_) {
  for (auto listener_ : price_level_global_listener_vec_) {
    listener_->OnPriceLevelChange(t_security_id_, t_buysell_, t_level_changed_, t_price_, t_new_size_,
                                  t_new_ordercount_, t_is_intermediate_message_);
  }
}

void ASXPFLoggedMessageFileSource::NotifyTrade(const unsigned int t_security_id_, const double t_trade_price_,
                                               const int t_trade_size_, const TradeType_t t_buysell_) {
  for (auto listener_ : price_level_global_listener_vec_) {
    listener_->OnTrade(t_security_id_, t_trade_price_, t_trade_size_, t_buysell_);
  }
}

void ASXPFLoggedMessageFileSource::HandleExpiryWeek(std::string& asx_file) {
	  int c_month_ = ((YYYYMMDD_ / 100) % 100);
	  if (c_month_ % 3 == 0) {
	    int start_date_ = DateTime::CalcNextWeekDay(100 * (YYYYMMDD_ / 100) + 7);  // 8th or next business day
	    int end_date_ = DateTime::CalcNextWeekDay(100 * (YYYYMMDD_ / 100) + 14);   // 15th or next business day

	    if (YYYYMMDD_ < 20171201 && (shortcode_.substr(0, 3) == "XTE" || shortcode_.substr(0, 3) == "YTE")) {
	      if (YYYYMMDD_ < start_date_ || YYYYMMDD_ > end_date_) {
	        asx_file += "_INVALID_INTENTIONAL";
	      } else {
	        if (YYYYMMDD_ == start_date_) {
	          start_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(start_date_, "AST_1645");
	        } else if (YYYYMMDD_ == end_date_) {
	          end_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(end_date_, "AST_1645");
	        }
	      }
	    } else if (YYYYMMDD_ >= 20171201 && shortcode_.substr(0,3) == "XTE") {
	       if (YYYYMMDD_ < start_date_ || YYYYMMDD_ > end_date_) {
	         asx_file += "_INVALID_INTENTIONAL";
	       } else {
	         if (YYYYMMDD_ == start_date_) {
	          start_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(start_date_, "AST_1645");
	        } else if (YYYYMMDD_ == end_date_) {
	          end_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(end_date_, "AST_1645");
	        }
	      }
	    } else if (YYYYMMDD_ >= 20171201 && shortcode_.substr(0,3) == "YTE") {
	          asx_file += "_INVALID_INTENTIONAL";
	    } else if (shortcode_.substr(0, 2) == "XT") {
	        if (YYYYMMDD_ > start_date_ && YYYYMMDD_ < end_date_) {
	        asx_file += "_INVALID_INTENTIONAL";
	      } else {
	        if (YYYYMMDD_ == start_date_) {
	          end_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(start_date_, "AST_1645");
	        } else if (YYYYMMDD_ == end_date_) {
	          start_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(end_date_, "AST_1645");
	        }
	      }
	    } else if (YYYYMMDD_ < 20171201 && shortcode_.substr(0, 2) == "YT") {
	        if (YYYYMMDD_ > start_date_ && YYYYMMDD_ < end_date_) {
	        asx_file += "_INVALID_INTENTIONAL";
	      } else {
	        if (YYYYMMDD_ == start_date_) {
	          end_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(start_date_, "AST_1645");
	        } else if (YYYYMMDD_ == end_date_) {
	          start_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(end_date_, "AST_1645");
	        }
	      }
	    } else if (YYYYMMDD_ >= 20171201 && shortcode_.substr(0, 2) == "YT") {
	    	if (YYYYMMDD_ == start_date_) {
	    	  end_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(start_date_, "AST_1645");
	    	} else if (YYYYMMDD_ == end_date_) {
	    	    start_timestamp_.tv_sec = DateTime::GetTimeFromTZHHMMStr(end_date_, "AST_1645");
	    	}
	    }
	  } else {
	    if (shortcode_.substr(0, 3) == "XTE" || shortcode_.substr(0, 3) == "YTE") {
	      asx_file += "_INVALID_INTENTIONAL";
	    }
	  }
}
}
