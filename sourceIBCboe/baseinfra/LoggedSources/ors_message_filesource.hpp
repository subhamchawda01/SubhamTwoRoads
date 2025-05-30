/**
    \file LoggedSources/ors_message_filesource.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_ORSMESSAGES_ORS_MESSAGE_FILESOURCE_H
#define BASE_ORSMESSAGES_ORS_MESSAGE_FILESOURCE_H

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"

#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/ORSMessages/ors_message_listener.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/LoggedSources/ors_message_filenamer.hpp"

namespace HFSAT {

/** @brief reads ORSMessages stored in file for this { symbol, date } and depending on ORRType_t calls the listeners, if
 * any, of the security and ORRType_t
 */
class ORSMessageFileSource : public ExternalDataListener {
 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;

  std::vector<OrderNotFoundListener*>
      order_not_found_listener_vec_;  ///< vector of listeners to ORS messages of type kORRType_None
  std::vector<OrderSequencedListener*>
      order_sequenced_listener_vec_;  ///< vector of listeners to ORS messages of type kORRType_Seqd
  std::vector<OrderConfirmedListener*>
      order_confirmed_listener_vec_;  ///< vector of listeners to ORS messages of type kORRType_Conf
  std::vector<OrderConfCxlReplacedListener*>
      order_conf_cxlreplaced_listener_vec_;  ///< vector of listeners to ORS messages of type kORRType_CxRe
  std::vector<OrderConfCxlReplaceRejectListener*>
      order_conf_cxlreplace_rejected_listener_vec_;  ///< vector of listeners to ORS messages of type kORRType_CxRe
  std::vector<OrderCanceledListener*>
      order_canceled_listener_vec_;  ///< vector of listeners to ORS messages of type kORRType_Cxld
  std::vector<OrderCxlSeqdListener*>
      order_cxl_seqd_listener_vec_;  ///< vector of listeners to ORS messages of type kORRType_CxlSeq
  std::vector<OrderExecutedListener*>
      order_executed_listener_vec_;  ///< vector of listeners to ORS messages of type kORRType_Exec
  std::vector<OrderRejectedListener*>
      order_rejected_listener_vec_;  ///< vector of listeners to ORS messages of type kORRType_Rejc
  std::vector<OrderInternallyMatchedListener*> order_internally_matched_listener_vec_;  ///< map (vector) from
  std::vector<OrderConfCxlReplaceRejectListener*> order_conf_cxlreplace_reject_listener_vec_;
  /// security_id to vector of
  /// listeners to ORS messages of
  /// type kORRType_IntExec

  ExternalTimeListener* p_time_keeper_;  ///< only meant for Watch
  PriceLevelGlobalListener* price_level_global_listener_;

  BulkFileReader bulk_file_reader_;
  GenericORSReplyStruct next_event_;
  TradingLocation_t trading_location_file_read_;
  int delay_msecs_to_add_;
  bool need_to_add_delay_msecs_;

 public:
  /** Constructor
   *
   * @param _dbglogger_ for logging errors
   * @param _sec_name_indexer_ to detect if the security is of interest and not to process if not
   * @param _preevent_YYYYMMDD_ tradingdate to load the appropriate file
   * @param _security_id_ also same as _sec_name_indexer_ [ _exchange_symbol_ ]
   * @param _exchange_symbol_ needed to match
   *
   * For now assuming that this file has only data of this symbol and hence _exchange_symbol_ matching is not required
   **/
  ORSMessageFileSource(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_,
                       const unsigned int _preevent_YYYYMMDD_, const unsigned int _security_id_,
                       const char* _exchange_symbol_, TradingLocation_t r_trading_location_);

  inline int socket_file_descriptor() const { return 0; }

  void SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_);
  void ComputeEarliestDataTimestamp(bool& _hasevents_);
  void ProcessAllEvents();
  void ProcessEventsTill(const ttime_t _endtime_);
  bool SetNextTimeStamp();
  void ProcessThisEvent();

  void AddOrderNotFoundListener(OrderNotFoundListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_not_found_listener_vec_, _new_listener_);
  }
  void RemoveOrderNotFoundListener(OrderNotFoundListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_not_found_listener_vec_, _new_listener_);
  }

  void AddOrderSequencedListener(OrderSequencedListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_sequenced_listener_vec_, _new_listener_);
  }
  void RemOrderSequencedListener(OrderSequencedListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_sequenced_listener_vec_, _new_listener_);
  }

  void AddOrderConfirmedListener(OrderConfirmedListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_confirmed_listener_vec_, _new_listener_);
  }
  void RemoveOrderConfirmedListener(OrderConfirmedListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_confirmed_listener_vec_, _new_listener_);
  }

  void AddOrderConfCxlReplacedListener(OrderConfCxlReplacedListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_conf_cxlreplaced_listener_vec_, _new_listener_);
  }
  void RemoveOrderConfCxlReplacedListener(OrderConfCxlReplacedListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_conf_cxlreplaced_listener_vec_, _new_listener_);
  }

  void AddOrderConfCxlReplaceRejectListener(OrderConfCxlReplaceRejectListener* new_listener) {
    VectorUtils::UniqueVectorAdd(order_conf_cxlreplace_rejected_listener_vec_, new_listener);
  }
  void RemoveOrderConfCxlReplaceRejectListener(OrderConfCxlReplaceRejectListener* listener) {
    VectorUtils::UniqueVectorRemove(order_conf_cxlreplace_rejected_listener_vec_, listener);
  }

  void AddOrderCanceledListener(OrderCanceledListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_canceled_listener_vec_, _new_listener_);
  }
  void RemoveOrderCanceledListener(OrderCanceledListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_canceled_listener_vec_, _new_listener_);
  }

  void AddOrderCxlSeqdListener(OrderCxlSeqdListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_cxl_seqd_listener_vec_, _new_listener_);
  }
  void RemoveOrderCxlSeqdListener(OrderCxlSeqdListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_cxl_seqd_listener_vec_, _new_listener_);
  }

  void AddOrderExecutedListener(OrderExecutedListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_executed_listener_vec_, _new_listener_);
  }
  void RemoveOrderExecutedListener(OrderExecutedListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_executed_listener_vec_, _new_listener_);
  }

  void AddOrderRejectedListener(OrderRejectedListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_rejected_listener_vec_, _new_listener_);
  }
  void RemoveOrderRejectedListener(OrderRejectedListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_rejected_listener_vec_, _new_listener_);
  }

  void AddOrderInternallyMatchedListener(OrderInternallyMatchedListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_internally_matched_listener_vec_, _new_listener_);
  }
  void RemoveOrderInternallyMatchedListener(OrderInternallyMatchedListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_internally_matched_listener_vec_, _new_listener_);
  }

  void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  void AddPriceLevelGlobalListener(PriceLevelGlobalListener* price_level_global_listener) {
    price_level_global_listener_ = price_level_global_listener;
  }
};
}
#endif  // BASE_ORSMESSAGES_ORS_MESSAGE_FILESOURCE_H
