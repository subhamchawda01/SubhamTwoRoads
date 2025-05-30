// =====================================================================================
//
//       Filename:  ose_pricefeed_logged_message_filesource.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/27/2013 08:21:54 AM
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

#ifndef BASE_MDSMESSAGES_OSE_PRICEFEED_LOGGED_MESSAGE_FILESOURCE_H
#define BASE_MDSMESSAGES_OSE_PRICEFEED_LOGGED_MESSAGE_FILESOURCE_H

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/LoggedSources/ose_pricefeed_logged_message_filenamer.hpp"
#include "dvccode/CommonTradeUtils/trade_time_manager.hpp"

#include "dvccode/Utils/ose_trade_analyzer.hpp"

namespace HFSAT {

class OSEPriceFeedLoggedMessageFileSource : public ExternalDataListener {
 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;

  PriceLevelGlobalListener* p_price_level_global_listener_;

  ExternalTimeListener* p_time_keeper_;

  BulkFileReader bulk_file_reader_;
  OSE_MDS::OSEPriceFeedCommonStruct next_event_;

  /// To account for multiple locations
  TradingLocation_t trading_location_file_read_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;

  HFSAT::TradeTimeManager& trade_time_manager_;

  HFSAT::Utils::OSETradeAnalyzer* ose_trade_analyzer_;
  OSE_MDS::OSEPriceFeedCommonStruct trade_packet_;
  HFSAT::TradeType_t trade_agg_side_;
  bool is_using_older_tse_2_ose_data_;

  bool use_fake_faster_data_;

 public:
  OSEPriceFeedLoggedMessageFileSource(DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                                      const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                                      const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
                                      bool t_use_fake_faster_data_ = true);

  ~OSEPriceFeedLoggedMessageFileSource() {}

  inline int socket_file_descriptor() const { return 0; }

  void SeekToFirstEventAfter(const ttime_t _endtime_, bool& rw_hasevents_);

  void ComputeEarliestDataTimestamp(bool& t_hasevents_);

  void ProcessAllEvents();

  void ProcessEventsTill(const ttime_t t_endtime_);

  inline bool IsNormalTradeTime(int securityId, ttime_t tv) {
    return trade_time_manager_.isValidTimeToTrade(securityId, tv.tv_sec % 86400);
  }

  inline void SetPriceLevelGlobalListener(PriceLevelGlobalListener* p_new_listener_) {
    p_price_level_global_listener_ = p_new_listener_;
  }

  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { p_time_keeper_ = t_new_listener_; }

 private:
  bool _SetNextTimeStamp();
  void _ProcessThisMsg();
};
}

#endif
