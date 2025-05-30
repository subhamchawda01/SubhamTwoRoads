#include "baseinfra/LoggedSources/ors_message_filesource.hpp"

namespace HFSAT {

ORSMessageFileSource::ORSMessageFileSource(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_,
                                           const unsigned int _preevent_YYYYMMDD_, const unsigned int _security_id_,
                                           const char* _exchange_symbol_, TradingLocation_t r_trading_location_)
    : ExternalDataListener(),
      dbglogger_(_dbglogger_),
      sec_name_indexer_(_sec_name_indexer_),
      security_id_(_security_id_),
      exchange_symbol_(_exchange_symbol_),
      order_not_found_listener_vec_(),
      order_sequenced_listener_vec_(),
      order_confirmed_listener_vec_(),
      order_conf_cxlreplaced_listener_vec_(),
      order_canceled_listener_vec_(),
      order_executed_listener_vec_(),
      order_rejected_listener_vec_(),
      order_internally_matched_listener_vec_(),
      order_conf_cxlreplace_reject_listener_vec_(),
      p_time_keeper_(NULL),
      price_level_global_listener_(NULL),
      bulk_file_reader_(),
      next_event_(),
      trading_location_file_read_(r_trading_location_),
      delay_msecs_to_add_(0),
      need_to_add_delay_msecs_(false) {
  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  // find the filename
  std::string _ors_filename_ =
      ORSMessageFileNamer::GetName(_exchange_symbol_, _preevent_YYYYMMDD_, trading_location_file_read_);

  if (trading_location_file_read_ != r_trading_location_) {
    delay_msecs_to_add_ =
        TradingLocationUtils::GetMSecsBetweenTradingLocations(trading_location_file_read_, r_trading_location_);
    if (delay_msecs_to_add_ > 0) need_to_add_delay_msecs_ = true;
  }
  // Open file with BulkFileReader
  if (kTLocMAX != trading_location_file_read_) {
    bulk_file_reader_.open(_ors_filename_);
  }
}

void ORSMessageFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // keep reading the next_event_
    // to check if next_event_timestamp_
    // if greater than r_endtime_
    do {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(GenericORSReplyStruct));

      if (available_len_ < sizeof(GenericORSReplyStruct)) {  // data not found in file
        next_event_timestamp_ =
            ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
        rw_hasevents_ = false;
        break;
      }
      next_event_timestamp_ =
          next_event_.time_set_by_server_;  // hoping 'time_set_by_server_" is good enough time to work with
    } while (next_event_timestamp_ < r_start_time_);
  } else {  // data file not open
    rw_hasevents_ = false;
  }
}

inline bool ORSMessageFileSource::SetNextTimeStamp() {
  if (bulk_file_reader_.is_open()) {  // keep reading the next_event_

    register size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(GenericORSReplyStruct));

    if (available_len_ < sizeof(GenericORSReplyStruct)) {  // data not found in file
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      return false;
    }
    next_event_timestamp_ =
        next_event_.time_set_by_server_;  // hoping 'time_set_by_server_" is good enough time to work with
    return true;
  } else {                                          // data file not open
    next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
    return false;
  }
}

void ORSMessageFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_) {
  if (bulk_file_reader_.is_open()) {  // read the next_event_ and set next_event_timestamp_
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(GenericORSReplyStruct));
    if (available_len_ < sizeof(GenericORSReplyStruct)) {  // data not found in file
      next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      rw_hasevents_ = false;
    }
    next_event_timestamp_ = next_event_.time_set_by_server_;  // hoping this is good enough
  } else {
    next_event_timestamp_ = ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
    rw_hasevents_ = false;
  }
}

void ORSMessageFileSource::ProcessAllEvents() {
  while (1) {
    ProcessThisEvent();
    if (!SetNextTimeStamp()) return;
  }
}

void ORSMessageFileSource::ProcessEventsTill(const ttime_t _endtime_) {
  // assumes next_event_timestamp_
  // assumes next_event_

  while (next_event_timestamp_ <= _endtime_) {
    ProcessThisEvent();
    if (!SetNextTimeStamp()) return;
  }
}

void ORSMessageFileSource::ProcessThisEvent() {
  NotifyExternalDataListenerListener(security_id_);
  // first alert Watch -- now removed
  p_time_keeper_->OnTimeReceived(
      next_event_timestamp_,
      security_id_);  // Diwakar added this to fill orders based on ORS. We should remove this later
  if (price_level_global_listener_) {
    price_level_global_listener_->OnPriceLevelUpdate(&next_event_, sizeof(next_event_), HFSAT::MDS_MSG::ORS_REPLY);
  }

  switch (next_event_.orr_type_) {
    case kORRType_None: {
      for (size_t i = 0; i < order_not_found_listener_vec_.size(); i++) {
        order_not_found_listener_vec_[i]->OrderNotFound(
            next_event_.server_assigned_client_id_, next_event_.client_assigned_order_sequence_,
            next_event_.server_assigned_order_sequence_, security_id_, next_event_.buysell_, next_event_.int_price_,
            next_event_.server_assigned_message_sequence_, next_event_.exch_assigned_sequence_,
            next_event_.time_set_by_server_);
      }
    } break;
    case kORRType_Seqd: {
      for (size_t i = 0; i < order_sequenced_listener_vec_.size(); i++) {
        order_sequenced_listener_vec_[i]->OrderSequenced(
            next_event_.server_assigned_client_id_, next_event_.client_assigned_order_sequence_,
            next_event_.server_assigned_order_sequence_, security_id_, next_event_.price_, next_event_.buysell_,
            next_event_.size_remaining_, next_event_.size_executed_, next_event_.client_position_,
            next_event_.global_position_, next_event_.int_price_, next_event_.server_assigned_message_sequence_,
            next_event_.exch_assigned_sequence_, next_event_.time_set_by_server_);

        order_sequenced_listener_vec_[i]->OrderSequencedAtTime(
            next_event_.server_assigned_client_id_, next_event_.client_assigned_order_sequence_,
            next_event_.server_assigned_order_sequence_, security_id_, next_event_.price_, next_event_.buysell_,
            next_event_.size_remaining_, next_event_.size_executed_, next_event_.client_position_,
            next_event_.global_position_, next_event_.int_price_, next_event_.exch_assigned_sequence_,
            next_event_.time_set_by_server_);
      }
    } break;
    case kORRType_Conf: {
      for (size_t i = 0; i < order_confirmed_listener_vec_.size(); i++) {
        order_confirmed_listener_vec_[i]->OrderConfirmed(
            next_event_.server_assigned_client_id_, next_event_.client_assigned_order_sequence_,
            next_event_.server_assigned_order_sequence_, security_id_, next_event_.price_, next_event_.buysell_,
            next_event_.size_remaining_, next_event_.size_executed_, next_event_.client_position_,
            next_event_.global_position_, next_event_.int_price_, next_event_.server_assigned_message_sequence_,
            next_event_.exch_assigned_sequence_, next_event_.time_set_by_server_);

        order_confirmed_listener_vec_[i]->OrderConfirmedAtTime(
            next_event_.server_assigned_client_id_, next_event_.client_assigned_order_sequence_,
            next_event_.server_assigned_order_sequence_, security_id_, next_event_.price_, next_event_.buysell_,
            next_event_.size_remaining_, next_event_.size_executed_, next_event_.client_position_,
            next_event_.global_position_, next_event_.int_price_, next_event_.exch_assigned_sequence_,
            next_event_.time_set_by_server_);
      }
    } break;
    case kORRType_CxlSeqd: {
      for (size_t i = 0; i < order_cxl_seqd_listener_vec_.size(); i++) {
        order_cxl_seqd_listener_vec_[i]->OrderCxlSequenced(
            next_event_.server_assigned_client_id_, next_event_.client_assigned_order_sequence_,
            next_event_.server_assigned_order_sequence_, security_id_, next_event_.price_, next_event_.buysell_,
            next_event_.size_remaining_, next_event_.client_position_, next_event_.global_position_,
            next_event_.int_price_, next_event_.server_assigned_message_sequence_, next_event_.exch_assigned_sequence_,
            next_event_.time_set_by_server_);

        order_cxl_seqd_listener_vec_[i]->OrderCxlSequencedAtTime(
            next_event_.server_assigned_client_id_, next_event_.client_assigned_order_sequence_,
            next_event_.server_assigned_order_sequence_, security_id_, next_event_.price_, next_event_.buysell_,
            next_event_.size_remaining_, next_event_.client_position_, next_event_.global_position_,
            next_event_.int_price_, next_event_.exch_assigned_sequence_, next_event_.time_set_by_server_);
      }
    } break;
    case kORRType_Cxld: {
      for (size_t i = 0; i < order_canceled_listener_vec_.size(); i++) {
        order_canceled_listener_vec_[i]->OrderCanceled(
            next_event_.server_assigned_client_id_, next_event_.client_assigned_order_sequence_,
            next_event_.server_assigned_order_sequence_, security_id_, next_event_.price_, next_event_.buysell_,
            next_event_.size_remaining_, next_event_.client_position_, next_event_.global_position_,
            next_event_.int_price_, next_event_.server_assigned_message_sequence_, next_event_.exch_assigned_sequence_,
            next_event_.time_set_by_server_);

        order_canceled_listener_vec_[i]->OrderCanceledAtTime(
            next_event_.server_assigned_client_id_, next_event_.client_assigned_order_sequence_,
            next_event_.server_assigned_order_sequence_, security_id_, next_event_.price_, next_event_.buysell_,
            next_event_.size_remaining_, next_event_.client_position_, next_event_.global_position_,
            next_event_.int_price_, next_event_.exch_assigned_sequence_, next_event_.time_set_by_server_);
      }
    } break;
    case kORRType_Exec: {
      for (size_t i = 0; i < order_executed_listener_vec_.size(); i++) {
        order_executed_listener_vec_[i]->OrderExecuted(
            next_event_.server_assigned_client_id_, next_event_.client_assigned_order_sequence_,
            next_event_.server_assigned_order_sequence_, security_id_, next_event_.price_, next_event_.buysell_,
            next_event_.size_remaining_, next_event_.size_executed_, next_event_.client_position_,
            next_event_.global_position_, next_event_.int_price_, next_event_.server_assigned_message_sequence_,
            next_event_.exch_assigned_sequence_, next_event_.time_set_by_server_);
      }
    } break;
    case kORRType_IntExec: {
      for (size_t i = 0; i < order_internally_matched_listener_vec_.size(); i++) {
        order_internally_matched_listener_vec_[i]->OrderInternallyMatched(
            next_event_.server_assigned_client_id_, next_event_.client_assigned_order_sequence_,
            next_event_.server_assigned_order_sequence_, security_id_, next_event_.price_, next_event_.buysell_,
            next_event_.size_remaining_, next_event_.size_executed_, next_event_.client_position_,
            next_event_.global_position_, next_event_.int_price_, next_event_.server_assigned_message_sequence_,
            next_event_.exch_assigned_sequence_, next_event_.time_set_by_server_);
      }
    } break;
    case kORRType_Rejc: {
      for (auto rejected_listener : order_rejected_listener_vec_) {
        rejected_listener->OrderRejected(
            next_event_.server_assigned_client_id_, next_event_.client_assigned_order_sequence_, security_id_,
            next_event_.price_, next_event_.buysell_, next_event_.size_remaining_, next_event_.size_executed_,
            next_event_.int_price_, next_event_.exch_assigned_sequence_, next_event_.time_set_by_server_);
      }
      break;
    }
    case kORRType_CxlRejc: {
      for (auto canceled_listener : order_canceled_listener_vec_) {
        canceled_listener->OrderCancelRejected(
            next_event_.server_assigned_client_id_, next_event_.client_assigned_order_sequence_,
            next_event_.server_assigned_order_sequence_, security_id_, next_event_.price_, next_event_.buysell_,
            next_event_.size_remaining_, next_event_.size_executed_, next_event_.client_position_,
            next_event_.global_position_, next_event_.int_price_, next_event_.exch_assigned_sequence_,
            next_event_.time_set_by_server_);
      }
      break;
    }
    case kORRType_CxRe: {
      for (auto conf_cxl_listener : order_conf_cxlreplaced_listener_vec_) {
        conf_cxl_listener->OrderConfCxlReplaced(
            next_event_.server_assigned_client_id_, next_event_.client_assigned_order_sequence_,
            next_event_.server_assigned_order_sequence_, security_id_, next_event_.price_, next_event_.buysell_,
            next_event_.size_remaining_, next_event_.size_executed_, next_event_.client_position_,
            next_event_.global_position_, next_event_.int_price_, next_event_.server_assigned_message_sequence_,
            next_event_.exch_assigned_sequence_, next_event_.time_set_by_server_);
      }
    } break;
    case kORRType_CxReRejc: {
      for (auto listener : order_conf_cxlreplace_reject_listener_vec_) {
        listener->OrderConfCxlReplaceRejected(
            next_event_.server_assigned_client_id_, next_event_.client_assigned_order_sequence_,
            next_event_.server_assigned_order_sequence_, security_id_, next_event_.price_, next_event_.buysell_,
            next_event_.size_remaining_, next_event_.client_position_, next_event_.global_position_,
            next_event_.int_price_, next_event_.size_executed_, next_event_.server_assigned_message_sequence_,
            next_event_.exch_assigned_sequence_, next_event_.time_set_by_server_);
      }
    } break;
    case kORRType_CxReSeqd: {
    } break;
    default:
      break;
  }
}
}
