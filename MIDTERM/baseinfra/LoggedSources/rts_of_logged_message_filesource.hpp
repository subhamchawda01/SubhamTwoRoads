/**
   \file LoggedSources/rts_logged_message_filesource.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_MDSMESSAGES_RTS_OF_LOGGED_MESSAGE_FILESOURCE_H
#define BASE_MDSMESSAGES_RTS_OF_LOGGED_MESSAGE_FILESOURCE_H

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/LoggedSources/common_logged_message_filenamer.hpp"
#include "baseinfra/MarketAdapter/trade_time_manager.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"

namespace HFSAT {

/// @brief reads RTSOFCommonStructv2 logged data stored in file for this { symbol, tradingdate }
/// Reads RTS_MDS::RTSOFCommonStructv2
///    depending on RTS_MDS::RTSOFCommonStructv2::msg_type
///        calls the listeners, if any, of the security
class RTSOrderFeedLoggedMessageFileSource : public ExternalDataListener {
 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;

  ExternalTimeListener* p_time_keeper_;

  BulkFileReader bulk_file_reader_;
  RTS_MDS::RTSOFCommonStructv2 next_event_;

  /// To account for multiple locations
  TradingLocation_t trading_location_file_read_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;

  bool trading_started_;
  bool trading_closed_;

  HFSAT::TradeTimeManager& trade_time_manager_;

  std::vector<RTS_MDS::RTSOFCommonStructv2> event_queue_;
  int events_left_;
  ttime_t next_non_intermediate_time_;
  bool use_fake_faster_data_;

  std::vector<OrderGlobalListener<RTS_MDS::RTSOFCommonStructv2>*> orderfeed_global_listener_vec_;

  int trade_date_;

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
  RTSOrderFeedLoggedMessageFileSource(DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                                      const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                                      const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
                                      bool t_use_fake_faster_data_ = true);

  inline int socket_file_descriptor() const { return 0; }

  void SeekToFirstEventAfter(const ttime_t _endtime_, bool& rw_hasevents_);

  void ComputeEarliestDataTimestamp(bool& t_hasevents_);

  void ProcessAllEvents();

  void ProcessEventsTill(const ttime_t t_endtime_);

  inline bool IsNormalTradeTime(int security_id_, ttime_t tv_) {
    return trade_time_manager_.isValidTimeToTrade(security_id_, tv_.tv_sec % 86400);
  }

  inline void SetOrderLevelGlobalListener(OrderGlobalListener<RTS_MDS::RTSOFCommonStructv2>* p_this_listener_) {
    AddOrderLevelGlobalListener(p_this_listener_);
  }

  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { p_time_keeper_ = t_new_listener_; }

  void AddOrderLevelGlobalListener(OrderGlobalListener<RTS_MDS::RTSOFCommonStructv2>* p_this_listener_);

 private:
  bool _SetNextTimeStamp();
  void _ProcessThisMsg();
};
}
#endif  // BASE_MDSMESSAGES_RTS_LOGGED_MESSAGE_FILESOURCE_H
