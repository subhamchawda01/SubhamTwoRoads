/**
    \file LoggedSources/cme_logged_message_filesource.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#pragma once

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/hybrid_sec.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/LoggedSources/common_logged_message_filenamer.hpp"
#include "dvccode/CommonTradeUtils/trade_time_manager.hpp"

namespace HFSAT {

class CMEOBFLoggedMessageFileSource : public ExternalDataListener {
 public:
  /**
   * @param t_dbglogger_ for logging errors
   * @param t_sec_name_indexer_ to detect if the security is of interest and not to process if not. If string matching
   * is more efficient we could use t_exchange_symbol_ as well.
   * @param t_preevent_YYYYMMDD_ tradingdate to load the appropriate file
   * @param t_security_id_ also same as t_sec_name_indexer_ [ t_exchange_symbol_ ]
   * @param t_exchange_symbol_ needed to match
   *
   * For now assuming t_exchange_symbol_ matching is not required
   */
  CMEOBFLoggedMessageFileSource(DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                                const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                                const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
                                bool t_use_fake_faster_data_ = true);

  ~CMEOBFLoggedMessageFileSource() {}

  void SeekToFirstEventAfter(const ttime_t _endtime_, bool& rw_hasevents_);
  int socket_file_descriptor() const { return -1; }

  void ComputeEarliestDataTimestamp(bool& t_hasevents_);

  void ProcessAllEvents();

  void ProcessEventsTill(const ttime_t t_endtime_);

  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { p_time_keeper_ = t_new_listener_; }

  void AddOrderLevelListener(OrderLevelListener* listener);

  void NotifyResetBegin(const uint32_t security_id);

  void NotifyResetEnd(const uint32_t security_id);

  void NotifyOrderAdd(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                      const uint32_t priority, const double price, const uint32_t size);
  void NotifyOrderModify(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                         const uint32_t priority, const double price, const uint32_t size);

  void NotifyOrderDelete(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id);
  void NotifyOrderExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                       const double exec_price, const uint32_t size_exec, const uint32_t size_remaining);

 private:
  TradeType_t GetTradeType(const char type);
  bool SetNextTimeStamp();
  void ProcessThisMsg();

 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;

  ExternalTimeListener* p_time_keeper_;

  BulkFileReader bulk_file_reader_;
  CME_MDS::CMEOBFCommonStruct next_event_;

  /// To account for multiple locations
  TradingLocation_t trading_location_file_read_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;
  bool use_fake_faster_data_;

  std::vector<OrderLevelListener*> order_listener_vec_;

  ttime_t start_timestamp_;
  ttime_t end_timestamp_;

  int YYYYMMDD_;
  std::string shortcode_;
};
}
