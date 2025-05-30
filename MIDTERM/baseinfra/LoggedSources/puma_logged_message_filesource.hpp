/**
   \file MDSMessage/puma_logged_message_file_source.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#pragma once

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/LoggedSources/puma_logged_message_filenamer.hpp"
#include "baseinfra/MarketAdapter/trade_time_manager.hpp"

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#define LOGGED_STRUCT_IS_LARGER

namespace HFSAT {

class PUMALoggedMessageFileSource : public ExternalDataListener {
 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;

  PriceLevelGlobalListener*
      p_price_level_global_listener_;  ///< Listeners of PUMA messages recorded in LiveTrading of all types
  NTPPriceLevelGlobalListener* p_ntp_price_level_global_listener_;  // PUMA price level listener

  ExternalTimeListener* p_time_keeper_;

  BulkFileReader bulk_file_reader_;
  PUMA_MDS::PumaCommonStruct next_event_;
  TradingLocation_t trading_location_file_read_;
  TradeTimeManager& trade_time_manager_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;

  bool use_fake_faster_data_;
  bool has_trading_started_;
  bool has_trading_ended_;

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
  PUMALoggedMessageFileSource(DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                              const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                              const char* t_exchange_symbol_, TradingLocation_t r_trading_location_,
                              bool t_use_fake_faster_data_ = true);

  inline int socket_file_descriptor() const { return 0; }

  void SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_);

  void ComputeEarliestDataTimestamp(bool& t_hasevents_);

  void ProcessAllEvents();

  void ProcessEventsTill(const ttime_t t_endtime_);

  // void SetPriceLevelGlobalListener ( PriceLevelGlobalListener * p_new_listener_ )
  // {
  //   p_price_level_global_listener_ = p_new_listener_;
  // }

  void SetNTPPriceLevelGlobalListener(NTPPriceLevelGlobalListener* p_new_listner_) {
    p_ntp_price_level_global_listener_ = p_new_listner_;
  }

  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { p_time_keeper_ = t_new_listener_; }

  inline bool IsNormalTradeTime(int securityId, ttime_t tv) {
    return trade_time_manager_.isValidTimeToTrade(securityId, tv.tv_sec % 86400);
  }

  inline void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {}

 private:
  inline MktStatus_t GetMarketStatus(PUMA_MDS::PumaCommonStruct& _next_event_);
  bool _SetNextTimeStamp();
  void _ProcessThisMsg();
};
}
