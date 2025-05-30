#include "dvccode/CDef/mds_messages.hpp"
#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/chix_l1_logged_message_filesource.hpp"
#include "baseinfra/MarketAdapter/l1_price_market_view_manager.hpp"

namespace HFSAT {

CHIXL1LoggedMessageFileSource::CHIXL1LoggedMessageFileSource(
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
      use_fake_faster_data_(t_use_fake_faster_data_) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  // find the filename
  std::string t_chix_filename_ =
      CHIXL1LoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);

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
    bulk_file_reader_.open(t_chix_filename_);
  } else {
    DBGLOG_CLASS_FUNC << "For CHIX symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                      << " trading_location " << trading_location_file_read_
                      << " returned filename = " << t_chix_filename_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    std::cerr << "For CHIX symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_chix_filename_ << std::endl;
  }
  // bulk_file_reader_.open ( t_chix_filename_ ) ;
}

void CHIXL1LoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // keep reading the next_event_
    // to check if next_event_timestamp_
    // if greater than r_endtime_
    do {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(BATSCHI_PL_MDS::BatsChiPLCommonStruct));
      if (available_len_ < sizeof(BATSCHI_PL_MDS::BatsChiPLCommonStruct)) {  // data not found in file
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

void CHIXL1LoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(BATSCHI_PL_MDS::BatsChiPLCommonStruct));
    if (available_len_ < sizeof(BATSCHI_PL_MDS::BatsChiPLCommonStruct)) {  // data not found in file
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

inline void CHIXL1LoggedMessageFileSource::_ProcessThisMsg() {
  int secId = sec_name_indexer_.GetIdFromSecname(next_event_.contract_);
  if (secId < 0) {
    return;
  }
  NotifyExternalDataListenerListener(secId);
  p_price_level_global_listener_->OnPriceLevelUpdate(&next_event_, sizeof(next_event_), HFSAT::MDS_MSG::CHIX_L1);

  if (next_event_.size <= 0) {
    // Only Update the Watch if the incoming message is of type non-intermediate
    if (!next_event_.intermediate_) {
      p_time_keeper_->OnTimeReceived(next_event_timestamp_, secId);
    }

    if (next_event_.buy_sell_ == 0) {
      ((L1PriceMarketViewManager*)p_price_level_global_listener_)
          ->DeleteTop((unsigned int)secId, kTradeTypeBuy, next_event_.price, next_event_.intermediate_);
    } else {
      ((L1PriceMarketViewManager*)p_price_level_global_listener_)
          ->DeleteTop((unsigned int)secId, kTradeTypeSell, next_event_.price, next_event_.intermediate_);
    }
    return;
  }

  if (next_event_.trade_ == 1) {
    ((L1PriceMarketViewManager*)p_price_level_global_listener_)
        ->OnTrade(secId, next_event_.price, next_event_.size, kTradeTypeNoInfo);
  } else if (next_event_.buy_sell_ == 0) {
    if (!next_event_.intermediate_) {
      p_time_keeper_->OnTimeReceived(next_event_timestamp_, secId);
    }

    ((L1PriceMarketViewManager*)p_price_level_global_listener_)
        ->OnL1Change(secId, kTradeTypeBuy, next_event_.price, next_event_.size, next_event_.order_count_,
                     next_event_.intermediate_);
  } else if (next_event_.buy_sell_ == 1) {
    if (!next_event_.intermediate_) {
      p_time_keeper_->OnTimeReceived(next_event_timestamp_, secId);
    }

    ((L1PriceMarketViewManager*)p_price_level_global_listener_)
        ->OnL1Change(secId, kTradeTypeSell, next_event_.price, next_event_.size, next_event_.order_count_,
                     next_event_.intermediate_);
  }
}

inline bool CHIXL1LoggedMessageFileSource::_SetNextTimeStamp() {
  // read the next_event_
  // set next_event_timestamp_
  register size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(BATSCHI_PL_MDS::BatsChiPLCommonStruct));
  if (available_len_ <
      sizeof(BATSCHI_PL_MDS::BatsChiPLCommonStruct)) { /* not enough data to fulfill this request to read a struct */
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

void CHIXL1LoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void CHIXL1LoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}
}
