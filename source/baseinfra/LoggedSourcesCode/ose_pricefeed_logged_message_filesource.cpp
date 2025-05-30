// =====================================================================================
//
//       Filename:  ose_pricefeed_logged_message_filesource.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/27/2013 09:45:08 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 353, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include "dvccode/CDef/mds_messages.hpp"
#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/ose_pricefeed_logged_message_filesource.hpp"

namespace HFSAT {

OSEPriceFeedLoggedMessageFileSource::OSEPriceFeedLoggedMessageFileSource(
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
      trade_time_manager_(HFSAT::TradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_preevent_YYYYMMDD_)),
      ose_trade_analyzer_(new HFSAT::Utils::OSETradeAnalyzer()),
      trade_packet_(),
      trade_agg_side_(HFSAT::kTradeTypeNoInfo),
      is_using_older_tse_2_ose_data_(false),
      use_fake_faster_data_(t_use_fake_faster_data_) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  // Check Whether We are using older TSE -> OSE converted data or so, as we will ignore the L1 messages for that
  std::string ose_contract_name = t_sec_name_indexer_.GetShortcodeFromId(t_security_id_);

  if (ose_contract_name == "TOPIX_0" || ose_contract_name == "TOPIXM_0" || ose_contract_name == "TPX30_0" ||
      ose_contract_name == "JGBM_0" || ose_contract_name == "JGBL_0" || ose_contract_name == "JGBSL_0" ||
      ose_contract_name == "JGBLM_0")

  {
    if (t_preevent_YYYYMMDD_ < 20140320) {
      is_using_older_tse_2_ose_data_ = true;
    }
  }

  // find the filename
  std::string t_ose_pricefeed_filename_ = OSEPriceFeedLoggedMessageFileNamer::GetName(
      t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);

  int added_delay = 0;
  // added delays is used by the robustness check script to variate line delay by a constant number
  added_delay = TradingLocationUtils::GetAddedDelay(trading_location_file_read_, r_trading_location_);
  delay_usecs_to_add_ = added_delay;

  if (trading_location_file_read_ != r_trading_location_) {
    delay_usecs_to_add_ +=
        TradingLocationUtils::GetUSecsBetweenTradingLocations(trading_location_file_read_, r_trading_location_);
  }
  if (delay_usecs_to_add_ > 0) need_to_add_delay_usecs_ = true;

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
    bulk_file_reader_.open(t_ose_pricefeed_filename_);
  } else {
    DBGLOG_CLASS_FUNC << "For OSEPriceFeed symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                      << " trading_location " << trading_location_file_read_
                      << " returned filename = " << t_ose_pricefeed_filename_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    std::cerr << "For OSEPriceFeed symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
              << " trading_location " << trading_location_file_read_
              << " returned filename = " << t_ose_pricefeed_filename_ << std::endl;
  }
}

void OSEPriceFeedLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // keep reading the next_event_
    // to check if next_event_timestamp_
    // if greater than r_endtime_
    do {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSEPriceFeedCommonStruct));
      if (available_len_ < sizeof(OSE_MDS::OSEPriceFeedCommonStruct)) {  // data not found in file
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

void OSEPriceFeedLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSEPriceFeedCommonStruct));
    if (available_len_ < sizeof(OSE_MDS::OSEPriceFeedCommonStruct)) {  // data not found in file
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

inline void OSEPriceFeedLoggedMessageFileSource::_ProcessThisMsg() {
  int secId = sec_name_indexer_.GetIdFromSecname(next_event_.contract_);
  if (secId < 0) {
    return;
  }
  NotifyExternalDataListenerListener(security_id_);
  p_price_level_global_listener_->OnPriceLevelUpdate(&next_event_, sizeof(next_event_), HFSAT::MDS_MSG::OSE_PF);

  if (next_event_.time_.tv_sec < 1396897200) {
    switch (next_event_.type_) {
      case 0:
      case 1: {
        if (!IsNormalTradeTime(security_id_, next_event_timestamp_)) break;

        // Using Older Converted TSE -> OSE data ? If so skip level 1 messages, LIFFE Connect APIs
        if (is_using_older_tse_2_ose_data_) {
          if (1 == next_event_.price_level_) {
            break;
          }
        }

        if (OSE_MDS::PRICEFEED_TRADE_ORDER == next_event_.price_feed_msg_type_) break;

        if (!IsNormalTradeTime(security_id_, next_event_timestamp_)) break;
        if (OSE_MDS::PRICEFEED_TRADE_ORDER == next_event_.price_feed_msg_type_) break;

        p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

        TradeType_t _buysell_ = (next_event_.type_ == 0) ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;

        switch (next_event_.price_feed_msg_type_) {
          case OSE_MDS::PRICEFEED_NEW: {
            p_price_level_global_listener_->OnPriceLevelNew(security_id_, _buysell_, next_event_.price_level_,
                                                            next_event_.price, next_event_.size,
                                                            next_event_.order_count_, false);
          } break;

          case OSE_MDS::PRICEFEED_CHANGE: {
            p_price_level_global_listener_->OnPriceLevelChange(security_id_, _buysell_, next_event_.price_level_,
                                                               next_event_.price, next_event_.size,
                                                               next_event_.order_count_, false);
          } break;
          case OSE_MDS::PRICEFEED_DELETE: {
            p_price_level_global_listener_->OnPriceLevelDelete(security_id_, _buysell_, next_event_.price_level_,
                                                               next_event_.price, false);
          } break;
          default: { } break; }
      } break;
      case 2: {
        if (!IsNormalTradeTime(security_id_, next_event_timestamp_)) break;

        if (next_event_.size > 0) {
          p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

          p_price_level_global_listener_->OnTrade(security_id_, next_event_.price, next_event_.size,
                                                  HFSAT::kTradeTypeNoInfo);
        }
      } break;
      default: { } break; }

  } else {
    switch (next_event_.type_) {
      case 0:
      case 1: {
        if (!IsNormalTradeTime(security_id_, next_event_timestamp_)) {
          ose_trade_analyzer_->StoreDeltaPacketSeq(&next_event_);
          break;
        }

        if (OSE_MDS::PRICEFEED_TRADE_ORDER == next_event_.price_feed_msg_type_) {
          ose_trade_analyzer_->OnTradeOrderInput(&next_event_);
          break;
        }

        // Flush Last Trade Message
        if (ose_trade_analyzer_->FlushAnyPendingTradeBeforeDelta(trade_packet_, trade_agg_side_)) {
          p_time_keeper_->OnTimeReceived(trade_packet_.time_, security_id_);
          p_price_level_global_listener_->OnTrade(security_id_, trade_packet_.price, trade_packet_.size,
                                                  trade_agg_side_);
        }

        ose_trade_analyzer_->StoreDeltaPacketSeq(&next_event_);

        p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

        TradeType_t _buysell_ = (next_event_.type_ == 0) ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;

        switch (next_event_.price_feed_msg_type_) {
          case OSE_MDS::PRICEFEED_NEW: {
            p_price_level_global_listener_->OnPriceLevelNew(security_id_, _buysell_, next_event_.price_level_,
                                                            next_event_.price, next_event_.size,
                                                            next_event_.order_count_, false);
          } break;

          case OSE_MDS::PRICEFEED_CHANGE: {
            p_price_level_global_listener_->OnPriceLevelChange(security_id_, _buysell_, next_event_.price_level_,
                                                               next_event_.price, next_event_.size,
                                                               next_event_.order_count_, false);
          } break;
          case OSE_MDS::PRICEFEED_DELETE: {
            p_price_level_global_listener_->OnPriceLevelDelete(security_id_, _buysell_, next_event_.price_level_,
                                                               next_event_.price, false);
          } break;
          default: { } break; }
      } break;
      case 2: {
        if (!IsNormalTradeTime(security_id_, next_event_timestamp_)) break;

        if (next_event_.price_level_ == 11 || next_event_.price_level_ == 12) {
          p_price_level_global_listener_->OnOffMarketTrade(security_id_, next_event_.price, next_event_.size,
                                                           kTradeTypeNoInfo);
        } else {
          if (next_event_.size > 0) {
            // Flush Last Trade Message Before Storing Next one
            if (ose_trade_analyzer_->FlushAnyPendingTradeBeforeDelta(trade_packet_, trade_agg_side_)) {
              p_time_keeper_->OnTimeReceived(trade_packet_.time_, security_id_);
              p_price_level_global_listener_->OnTrade(security_id_, trade_packet_.price, trade_packet_.size,
                                                      trade_agg_side_);
            }

            ose_trade_analyzer_->StoreAndAnalyzeTradePacket(next_event_);
          }
        }
      } break;
      default: { } break; }
  }
}

inline bool OSEPriceFeedLoggedMessageFileSource::_SetNextTimeStamp() {
  // read the next_event_
  // set next_event_timestamp_
  register size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSEPriceFeedCommonStruct));
  if (available_len_ <
      sizeof(OSE_MDS::OSEPriceFeedCommonStruct)) { /* not enough data to fulfill this request to read a struct */
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

void OSEPriceFeedLoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void OSEPriceFeedLoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}
}
