/**
   \file MDSMessagesCode/eobi_logged_message_filesource.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/LoggedSources/eobi_logged_message_filesource.hpp"

namespace HFSAT {

EOBILoggedMessageFileSource::EOBILoggedMessageFileSource(
    DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_, const unsigned int t_preevent_YYYYMMDD_,
    const unsigned int t_security_id_, const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
    bool use_todays_data_, bool t_use_fake_faster_data_)
    : ExternalDataListener(),
      dbglogger_(t_dbglogger_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_id_(t_security_id_),
      exchange_symbol_(t_exchange_symbol_),
      p_order_global_listener_eobi_(NULL),
      p_order_level_listener_sim_(NULL),
      p_time_keeper_(NULL),
      bulk_file_reader_(),
      next_event_(),
      trading_location_file_read_(r_trading_location_),
      delay_usecs_to_add_(0),
      need_to_add_delay_usecs_(false),
      skip_intermediate_message_(false),
      use_fake_faster_data_(t_use_fake_faster_data_),
      eobi_fast_order_manager_(HFSAT::BaseUtils::EOBIFastOrderManager::GetUniqueInstance(t_security_id_)) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  std::string t_eobi_filename_ = EOBILoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                                     trading_location_file_read_, use_todays_data_);

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
    bulk_file_reader_.open(t_eobi_filename_);
  } else {
    std::cerr << "For EOBI symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
              << trading_location_file_read_ << " returned filename = " << t_eobi_filename_ << std::endl;
  }
}

void EOBILoggedMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {
    if (next_event_timestamp_.tv_sec == 0 && next_event_timestamp_.tv_usec == 0) {
      // read the next_event_
      // set next_event_timestamp_
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EOBI_MDS::EOBICommonStructOld));
      if (available_len_ <
          sizeof(EOBI_MDS::EOBICommonStructOld)) { /* not enough data to fulfill this request to read a struct */
        next_event_timestamp_ =
            ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
        rw_hasevents_ = false;
      } else {
        next_event_timestamp_ = next_event_.time_;  // TODO : Converting from timeval to ttime_t. Need more work here to
                                                    // optimize, or preferably send ttime_t from MDS or ORS
        if (need_to_add_delay_usecs_) {
          switch (next_event_.msg_) {
            case EOBI_MDS::EOBI_ORDER: {
              if (!next_event_.data_.order_.intermediate_) {
                next_event_timestamp_.addusecs(delay_usecs_to_add_);
              }
            } break;
            default: { next_event_timestamp_.addusecs(delay_usecs_to_add_); } break;
          }
        }
      }
    }
  } else {
    rw_hasevents_ = false;
  }
  SetTimeToSkipUntilFirstEvent(r_start_time_);
}

void EOBILoggedMessageFileSource::ComputeEarliestDataTimestamp(bool& _hasevents_) {
  if (bulk_file_reader_.is_open()) {
    // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EOBI_MDS::EOBICommonStructOld));
    if (available_len_ <
        sizeof(EOBI_MDS::EOBICommonStructOld)) {         /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      _hasevents_ = false;
    } else {
      next_event_timestamp_ = next_event_.time_;  // TODO : Converting from timeval to ttime_t. Need more work here to
                                                  // optimize, or preferably send ttime_t from MDS or ORS
      if (need_to_add_delay_usecs_) {
        switch (next_event_.msg_) {
          case EOBI_MDS::EOBI_ORDER: {
            if (!next_event_.data_.order_.intermediate_) {
              next_event_timestamp_.addusecs(delay_usecs_to_add_);
            }
          } break;
          default: { next_event_timestamp_.addusecs(delay_usecs_to_add_); } break;
        }
      }
    }
  } else {
    _hasevents_ = false;
  }
}

inline void EOBILoggedMessageFileSource::_ProcessThisMsg() {
  NotifyExternalDataListenerListener(security_id_);
  if (next_event_.data_.order_.intermediate_ == false) {
    skip_intermediate_message_ = true;
  }

  if (!next_event_.data_.order_.intermediate_) {
    p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
  }
  if (p_order_global_listener_eobi_) {
    switch (next_event_.msg_) {
      case EOBI_MDS::EOBI_ORDER: {
        TradeType_t t_buysell_ = next_event_.data_.order_.side == 'B' ? kTradeTypeBuy : kTradeTypeSell;

        switch (next_event_.data_.order_.action_) {
          case '0': {
            if (skip_intermediate_message_ && next_event_.data_.order_.intermediate_) {
            } else {
              if (eobi_fast_order_manager_.IsListeningOnFastBook()) {
                EOBI_MDS::EOBICompactOrder eobi_compact_order;
                EOBI_MDS::ConvertOrderEventToCompactEvent(next_event_, eobi_compact_order);
                eobi_fast_order_manager_.OnAdd(eobi_compact_order);
              }

              p_order_global_listener_eobi_->OnOrderAdd(security_id_, t_buysell_, next_event_.data_.order_.price,
                                                        next_event_.data_.order_.size,
                                                        next_event_.data_.order_.intermediate_);
            }
            break;
          }
          case '1': {
            // since intermediate is hardcoded here as false
            p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

            if (eobi_fast_order_manager_.IsListeningOnFastBook()) {
              EOBI_MDS::EOBICompactOrder eobi_compact_order;
              EOBI_MDS::ConvertOrderEventToCompactEvent(next_event_, eobi_compact_order);
              eobi_fast_order_manager_.OnAdd(eobi_compact_order);
            }

            p_order_global_listener_eobi_->OnOrderModify(
                security_id_, t_buysell_, next_event_.data_.order_.price, next_event_.data_.order_.size,
                next_event_.data_.order_.prev_price, next_event_.data_.order_.prev_size);
            break;
          }
          case '2': {
            // since intermediate is hardcoded here as false
            p_time_keeper_->OnTimeReceived(next_event_timestamp_);

            if (eobi_fast_order_manager_.IsListeningOnFastBook()) {
              EOBI_MDS::EOBICompactOrder eobi_compact_order;
              EOBI_MDS::ConvertOrderEventToCompactEvent(next_event_, eobi_compact_order);
              eobi_fast_order_manager_.OnAdd(eobi_compact_order);
            }

            p_order_global_listener_eobi_->OnOrderDelete(security_id_, t_buysell_, next_event_.data_.order_.price,
                                                         (int)next_event_.data_.order_.size, true, false);
            break;
          }
          case '3': {
            // since intermediate is hardcoded here as false
            p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

            if (eobi_fast_order_manager_.IsListeningOnFastBook()) {
              EOBI_MDS::EOBICompactOrder eobi_compact_order;
              EOBI_MDS::ConvertOrderEventToCompactEvent(next_event_, eobi_compact_order);
              eobi_fast_order_manager_.OnAdd(eobi_compact_order);
            }

            p_order_global_listener_eobi_->OnOrderMassDelete(security_id_);
            break;
          }
          case '4': {
            // If Trade, update the watch
            p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

            if (eobi_fast_order_manager_.IsListeningOnFastBook()) {
              EOBI_MDS::EOBICompactOrder eobi_compact_order;
              EOBI_MDS::ConvertOrderEventToCompactEvent(next_event_, eobi_compact_order);
              eobi_fast_order_manager_.OnAdd(eobi_compact_order);
            }

            p_order_global_listener_eobi_->OnPartialOrderExecution(
                security_id_, t_buysell_, next_event_.data_.order_.price, next_event_.data_.order_.size);
            break;
          }
          case '5': {
            // If Trade, update the watch
            p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
            p_order_global_listener_eobi_->OnFullOrderExecution(
                security_id_, t_buysell_, next_event_.data_.order_.price, next_event_.data_.order_.size);
            break;
          }
          case '6': {
            // If Trade, update the watch
            p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);

            if (eobi_fast_order_manager_.IsListeningOnFastBook()) {
              EOBI_MDS::EOBICompactOrder eobi_compact_order;
              EOBI_MDS::ConvertOrderEventToCompactEvent(next_event_, eobi_compact_order);
              eobi_fast_order_manager_.OnAdd(eobi_compact_order);
            }

            p_order_global_listener_eobi_->OnExecutionSummary(security_id_, t_buysell_, next_event_.data_.order_.price,
                                                              next_event_.data_.order_.size);
            break;
          }
          default: {
            fprintf(stderr, "Weird message type in EOBILoggedMessageFileSource::ProcessAllEvents %d \n",
                    (int)next_event_.msg_);
            break;
          }
        }
        break;
      }
      default: { break; }
    }
  }

  if (p_order_level_listener_sim_) {
    switch (next_event_.msg_) {
      case EOBI_MDS::EOBI_ORDER: {
        TradeType_t t_buysell_ = next_event_.data_.order_.side == 'B' ? kTradeTypeBuy : kTradeTypeSell;

        switch (next_event_.data_.order_.action_) {
          case '0': {
            p_order_level_listener_sim_->OnOrderAdd(security_id_, t_buysell_, next_event_.data_.order_.priority_ts, 0,
                                                    next_event_.data_.order_.price, next_event_.data_.order_.size);
            break;
          }
          case '1': {
            // since intermediate is hardcoded here as false
            p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
            p_order_level_listener_sim_->OnOrderModifyWithPrevOrderId(
                security_id_, t_buysell_, next_event_.data_.order_.priority_ts, next_event_.data_.order_.price,
                next_event_.data_.order_.size, next_event_.data_.order_.prev_priority_ts,
                next_event_.data_.order_.prev_price, next_event_.data_.order_.prev_size);
            break;
          }
          case '2': {
            // since intermediate is hardcoded here as false
            p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
            p_order_level_listener_sim_->OnOrderDelete(security_id_, t_buysell_, next_event_.data_.order_.priority_ts);
            break;
          }
          case '3': {
            break;
          }
          case '4': {
            // If Trade, update the watch
            p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
            p_order_level_listener_sim_->OnOrderExec(security_id_, t_buysell_, next_event_.data_.order_.priority_ts,
                                                     next_event_.data_.order_.price, next_event_.data_.order_.size, 1);
            break;
          }
          case '5': {
            // If Trade, update the watch
            p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
            p_order_level_listener_sim_->OnOrderExec(security_id_, t_buysell_, next_event_.data_.order_.priority_ts,
                                                     next_event_.data_.order_.price, next_event_.data_.order_.size, 0);
            break;
          }
          case '6': {
            break;
          }
          default: {
            fprintf(stderr, "Weird message type in EOBILoggedMessageFileSource::ProcessAllEvents %d \n",
                    (int)next_event_.msg_);
            break;
          }
        }
        break;
      }
      default: { break; }
    }
  }
}

void EOBILoggedMessageFileSource::ProcessAllEvents() {
  while (true) {
    _ProcessThisMsg();

    // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EOBI_MDS::EOBICommonStructOld));
    if (available_len_ <
        sizeof(EOBI_MDS::EOBICommonStructOld)) {         /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      break;
    } else {
      next_event_timestamp_ = next_event_.time_;  // TODO : Converting from timeval to ttime_t. Need more work here to
                                                  // optimize, or preferably send ttime_t from MDS or ORS
      if (need_to_add_delay_usecs_) {
        switch (next_event_.msg_) {
          case EOBI_MDS::EOBI_ORDER: {
            if (!next_event_.data_.order_.intermediate_) {
              next_event_timestamp_.addusecs(delay_usecs_to_add_);
            }
          } break;
          default: { next_event_timestamp_.addusecs(delay_usecs_to_add_); } break;
        }
      }
    }
  }
}

void EOBILoggedMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes mdu_type_
  while (next_event_timestamp_ <= _endtime_) {
    _ProcessThisMsg();

    // read the next_event_
    // set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EOBI_MDS::EOBICommonStructOld));
    if (available_len_ <
        sizeof(EOBI_MDS::EOBICommonStructOld)) {         /* not enough data to fulfill this request to read a struct */
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      break;
    } else {
      next_event_timestamp_ =
          next_event_.time_;  // need more work here to optimize, or preferably send ttime_t from MDS or ORS
      if (need_to_add_delay_usecs_) {
        switch (next_event_.msg_) {
          case EOBI_MDS::EOBI_ORDER: {
            if (!next_event_.data_.order_.intermediate_) {
              next_event_timestamp_.addusecs(delay_usecs_to_add_);
            }
          } break;
          default: { next_event_timestamp_.addusecs(delay_usecs_to_add_); } break;
        }
      }
    }
  }
}
}
