/**
    \file LoggedSources/cme_logged_message_filesource.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_MDSMESSAGES_CME_LOGGED_MESSAGE_FILESOURCE_H
#define BASE_MDSMESSAGES_CME_LOGGED_MESSAGE_FILESOURCE_H

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/MarketAdapter/trade_time_manager.hpp"
#include "baseinfra/LoggedSources/common_logged_message_filenamer.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"



#define LOGGED_STRUCT_IS_LARGER

namespace HFSAT {

/// @brief reads CME logged data stored in file for this { symbol, tradingdate }
/// Reads CME_MDS::CMECommonStruct
///    depending on CME_MDS::CMECommonStruct::msgType
///    and for msgType == CME_MDS::CME_DELTA
///        depending on CME_MDS::CMECommonStruct::data_::cme_dels_::action_
///        calls the listeners, if any, of the security, action_
class CMELoggedMessageFileSource : public ExternalDataListener {
 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;

  PriceLevelGlobalListener*
      p_price_level_global_listener_;  ///< Listeners of CME messages recorded in LiveTrading of all types

  ExternalTimeListener* p_time_keeper_;
  BulkFileReader bulk_file_reader_;
  CME_MDS::CMECommonStruct next_event_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;
  ttime_t first_non_intermediate_time_;

  std::vector<CME_MDS::CMECommonStruct> event_queue_;
  int events_left_;
  ttime_t next_non_intermediate_time_;
  bool use_fake_faster_data_;
  HFSAT::TradeTimeManager& trade_time_manager_;

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
  CMELoggedMessageFileSource(DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                             const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                             const char* t_exchange_symbol_, TradingLocation_t r_trading_location_,
                             bool t_use_fake_faster_data_ = true);

  inline int socket_file_descriptor() const { return 0; }

  void SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_);

  void ComputeEarliestDataTimestamp(bool& t_hasevents_);

  void ProcessAllEvents();

  void ProcessEventsTill(const ttime_t t_endtime_);


  inline bool IsNormalTradeTime(int security_id_, ttime_t tv_) {
      return trade_time_manager_.isValidTimeToTrade(security_id_, tv_.tv_sec % 86400);
    }


  inline void SetPriceLevelGlobalListener(PriceLevelGlobalListener* p_new_listener_) {
    p_price_level_global_listener_ = p_new_listener_;
  }

  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { p_time_keeper_ = t_new_listener_; }

 private:
  bool _SetNextTimeStamp();
  void _ProcessThisMsg();
  void ComputeFirstNonIntermediateTime(std::string logged_message_filesource_name_);
};
}
#endif  // BASE_MDSMESSAGES_CME_LOGGED_MESSAGE_FILESOURCE_H
