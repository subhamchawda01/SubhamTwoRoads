#include "dvccode/CDef/bse_mds_defines.hpp"
#include "baseinfra/LoggedSources/bse_pf_logged_message_filesource.hpp"
#include "baseinfra/MDSMessages/defines.hpp"

namespace HFSAT {

BSEPFLoggedMessageFileSource::BSEPFLoggedMessageFileSource(
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
      has_trading_hours_started_(false),
      has_trading_hours_closed_(false),
      use_fake_faster_data_(t_use_fake_faster_data_),
      price_level_global_listener_vec_() {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  // find the filename
  std::string t_bse_filename_;
  if (HFSAT::HybridSecurityManager::IsHybridExch(std::string(t_exchange_symbol_))) {
    t_bse_filename_ = CommonLoggedMessageFileNamer::GetName(
        kExchSourceBSE,
        (HFSAT::HybridSecurityManager::GetActualExchFromHybrid(std::string(t_exchange_symbol_))).c_str(),
        t_preevent_YYYYMMDD_, trading_location_file_read_);
  } else {
    t_bse_filename_ = CommonLoggedMessageFileNamer::GetName(kExchSourceBSE, t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                            trading_location_file_read_);
  }

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

  DBGLOG_CLASS_FUNC << "For BSEPF symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                    << " trading_location " << trading_location_file_read_ << " DUsecs " << delay_usecs_to_add_
                    << " @use " << r_trading_location_ << " returned filename = " << t_bse_filename_
                    << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(t_bse_filename_);
  } else {
    std::cerr << "For BSEPF symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_bse_filename_ << std::endl;
  }
}

void BSEPFLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // keep reading the next_event_
    // to check if next_event_timestamp_
    // if greater than r_endtime_
    do {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(BSE_MDS::BSEPFCommonStruct));
      if (available_len_ < sizeof(BSE_MDS::BSEPFCommonStruct)) {  // data not found in file
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
    } while (next_event_timestamp_ <= r_start_time_);

  } else {  // data file not open
    rw_hasevents_ = false;
  }
}

void BSEPFLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(BSE_MDS::BSEPFCommonStruct));
    if (available_len_ < sizeof(BSE_MDS::BSEPFCommonStruct)) {  // data not found in file
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

inline void BSEPFLoggedMessageFileSource::_ProcessThisMsg() {
  NotifyExternalDataListenerListener(security_id_);
  p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

  switch (next_event_.msg_) {
    case BSE_MDS::BSE_PF_DELTA: {
      TradeType_t buysell_ = kTradeTypeNoInfo;
      if (next_event_.data_.delta_.side_ == BSE_MDS::BSEOrderSide::kBSEAsk) {
        buysell_ = kTradeTypeSell;
      } else if (next_event_.data_.delta_.side_ == BSE_MDS::BSEOrderSide::kBSEBid) {
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
          std::cerr << "Unknown action in BSE_DELTA: " << next_event_.data_.delta_.action_ << std::endl;
        } break;
      }
    } break;
    case BSE_MDS::BSE_PF_TRADE: {
      TradeType_t buysell_ = kTradeTypeNoInfo;
      if (next_event_.data_.trade_.side_ == BSE_MDS::BSEOrderSide::kBSEBid) {
        buysell_ = kTradeTypeBuy;
      } else if (next_event_.data_.trade_.side_ == BSE_MDS::BSEOrderSide::kBSEAsk) {
        buysell_ = kTradeTypeSell;
      }

      NotifyTrade(security_id_, next_event_.data_.trade_.price_, next_event_.data_.trade_.quantity_, buysell_);
    } break;
    default: { std::cerr << "Unknown msg_type in BSEPF: " << next_event_.msg_ << std::endl; } break;
  }
}

inline bool BSEPFLoggedMessageFileSource::_SetNextTimeStamp() {
  // read the next_event_
  // set next_event_timestamp_
  register size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(BSE_MDS::BSEPFCommonStruct));
  if (available_len_ < sizeof(BSE_MDS::BSEPFCommonStruct)) {
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

void BSEPFLoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void BSEPFLoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void BSEPFLoggedMessageFileSource::SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {}

void BSEPFLoggedMessageFileSource::AddPriceLevelGlobalListener(PriceLevelGlobalListener* p_this_listener_) {
  if (p_this_listener_ != NULL) {
    VectorUtils::UniqueVectorAdd(price_level_global_listener_vec_, p_this_listener_);
  }
}

void BSEPFLoggedMessageFileSource::NotifyPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                       const int t_level_added_, const double t_price_,
                                                       const int t_new_size_, const int t_new_ordercount_,
                                                       const bool t_is_intermediate_message_) {
  for (auto listener_ : price_level_global_listener_vec_) {
    listener_->OnPriceLevelNew(t_security_id_, t_buysell_, t_level_added_, t_price_, t_new_size_, t_new_ordercount_,
                               t_is_intermediate_message_);
  }
}

void BSEPFLoggedMessageFileSource::NotifyPriceLevelDelete(const unsigned int t_security_id_,
                                                          const TradeType_t t_buysell_, const int t_level_removed_,
                                                          const double t_price_,
                                                          const bool t_is_intermediate_message_) {
  for (auto listener_ : price_level_global_listener_vec_) {
    listener_->OnPriceLevelDelete(t_security_id_, t_buysell_, t_level_removed_, t_price_, t_is_intermediate_message_);
  }
}

void BSEPFLoggedMessageFileSource::NotifyPriceLevelChange(const unsigned int t_security_id_,
                                                          const TradeType_t t_buysell_, const int t_level_changed_,
                                                          const double t_price_, const int t_new_size_,
                                                          const int t_new_ordercount_,
                                                          const bool t_is_intermediate_message_) {
  for (auto listener_ : price_level_global_listener_vec_) {
    listener_->OnPriceLevelChange(t_security_id_, t_buysell_, t_level_changed_, t_price_, t_new_size_,
                                  t_new_ordercount_, t_is_intermediate_message_);
  }
}

void BSEPFLoggedMessageFileSource::NotifyTrade(const unsigned int t_security_id_, const double t_trade_price_,
                                               const int t_trade_size_, const TradeType_t t_buysell_) {
  for (auto listener_ : price_level_global_listener_vec_) {
    listener_->OnTrade(t_security_id_, t_trade_price_, t_trade_size_, t_buysell_);
  }
}
}
