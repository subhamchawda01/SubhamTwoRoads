/**
    \file LoggedSources/retail_logged_message_filesource.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     dress:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_MDSMESSAGES_RETAIL_LOGGED_MESSAGE_FILESOURCE_H
#define BASE_MDSMESSAGES_RETAIL_LOGGED_MESSAGE_FILESOURCE_H

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/LoggedSources/retail_logged_message_filenamer.hpp"

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/CDef/math_utils.hpp"
#include "baseinfra/TradeUtils/spread_utils.hpp"
#include "baseinfra/MDSMessages/defines.hpp"
#include "dvccode/LiveSources/retail_trade_listener.hpp"

namespace HFSAT {

/// @brief reads RETAIL-EBS logged data stored in file for this { symbol, tradingdate }
/// Reads RETAIL_MDS::EUREXCommonStruct
///    depending on RETAIL_MDS::RETAILCommonStruct::msgType
///    and for msgType == RETAIL_MDS::RETAIL_DELTA
///        depending on RETAIL_MDS::RETAILCommonStruct::data_::retail_dels_::action_
///        calls the listeners, if any, of the security, action_
class RETAILLoggedMessageFileSource : public ExternalDataListener {
 protected:
  DebugLogger &dbglogger_;
  SecurityNameIndexer &sec_name_indexer_;
  const unsigned int security_id_;  // this would be -1 for DI1 spread sources as they don't trade in real
  const char *exchange_symbol_;

  std::vector<FPOrderExecutedListener *> fp_ord_exec_listener_vec_;
  std::vector<RetailTradeListener *> ret_trd_listener_vec_;

  ExternalTimeListener *p_time_keeper_;

  BulkFileReader bulk_file_reader_;
  RETAIL_MDS::RETAILCommonStruct next_event_;

  /// To account for multiple locations
  TradingLocation_t trading_location_file_read_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;
  int YYYYMMDD_;

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
  RETAILLoggedMessageFileSource(DebugLogger &t_dbglogger_, SecurityNameIndexer &t_sec_name_indexer_,
                                const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                                const char *t_exchange_symbol_, const TradingLocation_t r_trading_location_,
                                bool manual_mode = false);

  ~RETAILLoggedMessageFileSource() {}

  inline int socket_file_descriptor() const { return 0; }

  void SeekToFirstEventAfter(const ttime_t _endtime_, bool &rw_hasevents_);

  void ComputeEarliestDataTimestamp(bool &t_hasevents_);

  void ProcessAllEvents();

  void ProcessEventsTill(const ttime_t t_endtime_);

  inline void AddFPOrderExecutedListener(FPOrderExecutedListener *_p_new_listener_) {
    if (_p_new_listener_ != NULL) {
      VectorUtils::UniqueVectorAdd(fp_ord_exec_listener_vec_, _p_new_listener_);
    }
  }

  inline void AddRetailTradeListener(RetailTradeListener *_p_new_listener_) {
    if (_p_new_listener_ != NULL) {
      VectorUtils::UniqueVectorAdd(ret_trd_listener_vec_, _p_new_listener_);
    }
  }

  inline void SetExternalTimeListener(ExternalTimeListener *t_new_listener_) { p_time_keeper_ = t_new_listener_; }

 private:
  bool _SetNextTimeStamp();
  void _ProcessThisMsg();
};
}
#endif  // BASE_MDSMESSAGES_RETAIL_LOGGED_MESSAGE_FILESOURCE_H
