#include "dvccode/CDef/mds_messages.hpp"
#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/tmx_of_logged_message_filesource.hpp"

namespace HFSAT {

TMXOFLoggedMessageFileSource::TMXOFLoggedMessageFileSource(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
    bool t_use_fake_faster_data_)
    : dbglogger_(t_dbglogger_),
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
      order_feed_global_listener_vec_() {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  // find the filename
  std::string t_ose_filename_;
  if (HFSAT::HybridSecurityManager::IsHybridExch(std::string(t_exchange_symbol_))) {
    t_ose_filename_ = CommonLoggedMessageFileNamer::GetOrderFeedFilename(
        kExchSourceTMX,
        (HFSAT::HybridSecurityManager::GetActualExchFromHybrid(std::string(t_exchange_symbol_))).c_str(),
        t_preevent_YYYYMMDD_, trading_location_file_read_);
  } else {
    t_ose_filename_ = CommonLoggedMessageFileNamer::GetOrderFeedFilename(
        kExchSourceTMX, t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
  }

  if (trading_location_file_read_ != r_trading_location_) {
    delay_usecs_to_add_ =
        TradingLocationUtils::GetUSecsBetweenTradingLocations(trading_location_file_read_, r_trading_location_);
    if (delay_usecs_to_add_ > 0) need_to_add_delay_usecs_ = true;
  }

  if (TradingLocationUtils::GetUsecsForFasterDataForShortcodeAtLocation(
          sec_name_indexer_.GetShortcodeFromId(security_id_), r_trading_location_, t_preevent_YYYYMMDD_,
          use_fake_faster_data_) !=
      0) {  // Experiment with the possibility of using faster data for this product at this location.
    need_to_add_delay_usecs_ = true;
    delay_usecs_to_add_ -= TradingLocationUtils::GetUsecsForFasterDataForShortcodeAtLocation(
        sec_name_indexer_.GetShortcodeFromId(security_id_), r_trading_location_, t_preevent_YYYYMMDD_,
        use_fake_faster_data_);
  }

  DBGLOG_CLASS_FUNC << "For TMXOF symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_
                    << " trading_location " << trading_location_file_read_ << " DUsecs " << delay_usecs_to_add_
                    << " @use " << r_trading_location_ << " returned filename = " << t_ose_filename_
                    << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(t_ose_filename_);
  } else {
    std::cerr << "For TMXOF symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_ose_filename_ << std::endl;
  }
}

void TMXOFLoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  // keep reading the next_event_
  // to check if next_event_timestamp_
  // if greater than r_endtime_
  if (bulk_file_reader_.is_open()) {
    if (next_event_timestamp_ == ttime_t(0, 0)) {
      size_t available_len = bulk_file_reader_.read(&next_event_, sizeof(TMX_OBF_MDS::TMXCommonStruct));

      // data not found in file
      if (available_len < sizeof(TMX_OBF_MDS::TMXCommonStruct)) {
        rw_hasevents_ = false;
      } else {
        next_event_timestamp_ = next_event_.time_;
        if (need_to_add_delay_usecs_) {
          next_event_timestamp_.addusecs(delay_usecs_to_add_);
        }
      }
    }
  }
  // data file not open
  else {
    rw_hasevents_ = false;
  }
}

void TMXOFLoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(TMX_OBF_MDS::TMXCommonStruct));
    if (available_len_ < sizeof(TMX_OBF_MDS::TMXCommonStruct)) {  // data not found in file
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

// need to change this
inline void TMXOFLoggedMessageFileSource::_ProcessThisMsg() {
  NotifyExternalDataListenerListener(security_id_);
  p_time_keeper_->OnTimeReceived(next_event_timestamp_);

  switch (next_event_.msg_type) {
    // Order Add
    case TMX_OBF_MDS::TMXMsgType::kTMXAdd: {
      NotifyOrderFeedAdd(security_id_, next_event_.data.add.order_id, next_event_.data.add.side,
                         next_event_.data.add.price, next_event_.data.add.size, next_event_.data.add.priority,
                         next_event_.intermediate_);
    } break;

    // Order Replaced
    case TMX_OBF_MDS::TMXMsgType::kTMXModify: {
      NotifyOrderFeedReplace(security_id_, next_event_.data.modify.old_order_id, next_event_.data.modify.side,
                             next_event_.data.modify.price, next_event_.data.modify.size,
                             next_event_.data.modify.order_id, next_event_.intermediate_);
    } break;

    // Order delete
    case TMX_OBF_MDS::TMXMsgType::kTMXDelete: {
      NotifyOrderFeedDelete(security_id_, next_event_.data.del.order_id, next_event_.data.del.side,
                            next_event_.intermediate_);
    } break;

    // Order Exec
    case TMX_OBF_MDS::TMXMsgType::kTMXExec: {
      NotifyOrderFeedExec(security_id_, next_event_.data.exec.order_id, next_event_.data.exec.side,
                          next_event_.data.exec.price, next_event_.data.exec.size_exec, next_event_.intermediate_);
    } break;

    // Order Exec with price change
    case TMX_OBF_MDS::TMXMsgType::kTMXExecWithTrade: {
      // Order Exec with price, we have execs for two order ids ( both buy and sell )
      NotifyOrderFeedExec(security_id_, next_event_.data.exec_info.order_id, next_event_.data.exec_info.side,
                          next_event_.data.exec_info.price, next_event_.data.exec_info.size_exec,
                          next_event_.intermediate_);
    } break;
    case TMX_OBF_MDS::TMXMsgType::kTMXResetBegin: {
      NotifyOrderFeedResetBegin(security_id_);
    } break;
    case TMX_OBF_MDS::TMXMsgType::kTMXResetEnd: {
      NotifyOrderFeedResetEnd(security_id_);
    } break;

    default: {
      //      std::cerr << " ..."
      //                << "\n";
    } break;
  }
}

inline bool TMXOFLoggedMessageFileSource::_SetNextTimeStamp() {
  // read the next_event_
  // set next_event_timestamp_
  register size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(TMX_OBF_MDS::TMXCommonStruct));
  if (available_len_ < sizeof(TMX_OBF_MDS::TMXCommonStruct)) {
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

void TMXOFLoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void TMXOFLoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    if (!_SetNextTimeStamp()) return;
  }
}

void TMXOFLoggedMessageFileSource::SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {}

void TMXOFLoggedMessageFileSource::AddOrderFeedGlobalListener(OrderFeedGlobalListener* p_this_listener_) {
  if (p_this_listener_ != NULL) {
    VectorUtils::UniqueVectorAdd(order_feed_global_listener_vec_, p_this_listener_);
  }
}

void TMXOFLoggedMessageFileSource::NotifyOrderFeedResetBegin(const unsigned int t_security_id_) {
  for (auto listener_ : order_feed_global_listener_vec_) {
    listener_->OnOrderResetBegin(t_security_id_);
  }
}

void TMXOFLoggedMessageFileSource::NotifyOrderFeedResetEnd(const unsigned int t_security_id_) {
  for (auto listener_ : order_feed_global_listener_vec_) {
    listener_->OnOrderResetEnd(t_security_id_);
  }
}

void TMXOFLoggedMessageFileSource::NotifyOrderFeedAdd(const unsigned int t_security_id_, uint64_t t_order_id_,
                                                      uint8_t t_side_, double t_price_, int t_size_,
                                                      uint32_t t_priority_, bool t_intermediate_) {
  for (auto listener_ : order_feed_global_listener_vec_) {
    listener_->OnOrderAdd(t_security_id_, t_order_id_, t_side_, t_price_, t_size_, t_priority_, t_intermediate_);
  }
}

void TMXOFLoggedMessageFileSource::NotifyOrderFeedDelete(const unsigned int t_security_id_, uint64_t t_order_id_,
                                                         uint8_t t_side_, bool t_intermediate_,
                                                         bool t_is_set_order_info_map_iter_) {
  for (auto listener_ : order_feed_global_listener_vec_) {
    listener_->OnOrderDelete(t_security_id_, t_order_id_, t_side_, t_intermediate_, t_is_set_order_info_map_iter_);
  }
}

void TMXOFLoggedMessageFileSource::NotifyOrderFeedModify(const unsigned int t_security_id_, uint64_t t_order_id_,
                                                         uint8_t t_side_, int t_new_size_, uint64_t t_new_order_id_,
                                                         bool t_intermediate_, bool t_is_set_order_info_map_iter_) {
  for (auto listener_ : order_feed_global_listener_vec_) {
    listener_->OnOrderModify(t_security_id_, t_order_id_, t_side_, t_new_size_, t_new_order_id_, t_intermediate_,
                             t_is_set_order_info_map_iter_);
  }
}

void TMXOFLoggedMessageFileSource::NotifyOrderFeedReplace(const unsigned int t_security_id_, uint64_t t_order_id_,
                                                          uint8_t t_side_, double t_new_price_, int t_new_size_,
                                                          uint64_t t_new_order_id_, bool t_intermediate_) {
  for (auto listener_ : order_feed_global_listener_vec_) {
    listener_->OnOrderReplace(t_security_id_, t_order_id_, t_side_, t_new_price_, t_new_size_, t_new_order_id_,
                              t_intermediate_);
  }
}

void TMXOFLoggedMessageFileSource::NotifyOrderFeedExec(const unsigned int t_security_id_, uint64_t t_order_id_,
                                                       uint8_t t_side_, double t_price_, int t_size_exec_,
                                                       bool t_intermediate_) {
  for (auto listener_ : order_feed_global_listener_vec_) {
    listener_->OnOrderExec(t_security_id_, t_order_id_, t_side_, t_price_, t_size_exec_, t_intermediate_);
  }
}
}
