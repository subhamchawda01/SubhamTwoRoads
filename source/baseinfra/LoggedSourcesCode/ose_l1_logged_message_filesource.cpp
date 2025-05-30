#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/ose_l1_logged_message_filesource.hpp"
#include "baseinfra/MarketAdapter/ose_l1_price_market_view_manager.hpp"

namespace HFSAT {

OSEL1LoggedMessageFileSource::OSEL1LoggedMessageFileSource(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
    bool t_use_fake_faster_data_)
    : ExternalDataListener(),
      dbglogger_(t_dbglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      p_full_book_global_listener_(NULL),
      p_time_keeper_(NULL),
      bulk_file_reader_(),
      next_event_(),
      trading_location_file_read_(r_trading_location_),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      trade_time_manager_(HFSAT::TradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_preevent_YYYYMMDD_)),
      use_fake_faster_data_(t_use_fake_faster_data_) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  // find the filename
  std::string t_ose_filename_ =
      OSEL1LoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
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
    bulk_file_reader_.open(t_ose_filename_);
  } else {
    DBGLOG_CLASS_FUNC << "For OSE symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                      << " trading_location " << trading_location_file_read_
                      << " returned filename = " << t_ose_filename_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    std::cerr << "For OSE symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_ose_filename_ << std::endl;
  }
  // bulk_file_reader_.open ( t_ose_filename_ ) ;
}

void OSEL1LoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // keep reading the next_event_
    // to check if next_event_timestamp_
    // if greater than r_endtime_
    do {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSEPLCommonStruct));
      if (available_len_ < sizeof(OSE_MDS::OSEPLCommonStruct)) {  // data not found in file
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

void OSEL1LoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSEPLCommonStruct));
    if (available_len_ < sizeof(OSE_MDS::OSEPLCommonStruct)) {  // data not found in file
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

inline void OSEL1LoggedMessageFileSource::_ProcessThisMsg() {
  int secId = sec_name_indexer_.GetIdFromSecname(next_event_.contract_);
  if (secId < 0) return;

  NotifyExternalDataListenerListener(secId);
  p_time_keeper_->OnTimeReceived(next_event_timestamp_, secId);

  switch (next_event_.get_buy_sell_trade()) {
    case OSE_MDS::kL1BUY: {
      if (!IsNormalTradeTime(secId, next_event_timestamp_)) break;
      ((OSEL1PriceMarketViewManager*)p_full_book_global_listener_)
          ->OnL1Change(secId, kTradeTypeBuy, next_event_.price, next_event_.size, next_event_.order_count_, false);
    } break;
    case OSE_MDS::kL1SELL:
      if (!IsNormalTradeTime(secId, next_event_timestamp_)) break;
      ((OSEL1PriceMarketViewManager*)p_full_book_global_listener_)
          ->OnL1Change(secId, kTradeTypeSell, next_event_.price, next_event_.size, next_event_.order_count_, false);
      break;
    case OSE_MDS::kL1TRADE:
      if (!IsNormalTradeTime(secId, next_event_timestamp_)) break;
      ((OSEL1PriceMarketViewManager*)p_full_book_global_listener_)
          ->OnTrade(secId, next_event_.price, next_event_.size, kTradeTypeNoInfo);
      break;
  }
}

inline bool OSEL1LoggedMessageFileSource::_SetNextTimeStamp() {
  // read the next_event_
  // set next_event_timestamp_
  register size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSEPLCommonStruct));
  if (available_len_ <
      sizeof(OSE_MDS::OSEPLCommonStruct)) { /* not enough data to fulfill this request to read a struct */
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

void OSEL1LoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void OSEL1LoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}
}
