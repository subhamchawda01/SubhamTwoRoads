/**
   \file MDSMessage/ntp_logged_message_file_source.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_MDSMESSAGES_NTP_LOGGED_MESSAGE_FILESOURCE_H
#define BASE_MDSMESSAGES_NTP_LOGGED_MESSAGE_FILESOURCE_H

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"
#include "baseinfra/MarketAdapter/trade_time_manager.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#define LOGGED_STRUCT_IS_LARGER

namespace HFSAT {

/// @brief reads NTP logged data stored in file for this { symbol, tradingdate }
/// Reads NTP_MDS::NTPCommonStruct
///    depending on NTP_MDS::NTPCommonStruct::msgType
///    and for msgType == NTP_MDS::NTP_DELTA
///        depending on NTP_MDS::NTPCommonStruct::data_::ntp_dels_::action_
///        calls the listeners, if any, of the security, action_
class NTPLoggedMessageFileSource : public ExternalDataListener {
 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;

  PriceLevelGlobalListener*
      p_price_level_global_listener_;  ///< Listeners of NTP messages recorded in LiveTrading of all types
  OrderLevelGlobalListener* p_order_level_global_listener_;  // --
  PriceLevelOrderBookGlobalListener* p_price_level_order_book_global_listener_;
  NTPPriceLevelGlobalListener* p_ntp_price_level_global_listener_;  // NTP price level listener
  OrderLevelListenerSim* p_order_level_listener_sim_;

  ExternalTimeListener* p_time_keeper_;
  HFSAT::TradeTimeManager& trade_time_manager_;
  BulkFileReader bulk_file_reader_;
  NTP_MDS::NTPCommonStruct next_event_;
  TradingLocation_t trading_location_file_read_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;
  bool use_order_book_;
  ttime_t first_non_intermediate_time_;

  std::vector<NTP_MDS::NTPCommonStruct> event_queue_;
  int events_left_;
  ttime_t next_non_intermediate_time_;
  bool use_fake_faster_data_;

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
  NTPLoggedMessageFileSource(DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                             const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                             const char* t_exchange_symbol_, TradingLocation_t r_trading_location_,
                             bool use_order_depth_book_ = false, bool is_bmf_equity_ = false,
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
  void SetOrderLevelGlobalListener(OrderLevelGlobalListener* p_new_listener_) {
    p_order_level_global_listener_ = p_new_listener_;
  }  // --
  void SetPriceLevelOrderBookGlobalListener(PriceLevelOrderBookGlobalListener* p_new_listener_) {
    p_price_level_order_book_global_listener_ = p_new_listener_;
  }
  void SetNTPPriceLevelGlobalListener(NTPPriceLevelGlobalListener* p_new_listner_) {
    p_ntp_price_level_global_listener_ = p_new_listner_;
  }

  void SetOrderLevelListenerSim(OrderLevelListenerSim* p_new_listner_) { p_order_level_listener_sim_ = p_new_listner_; }

  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { p_time_keeper_ = t_new_listener_; }

  inline void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {
    if (p_price_level_order_book_global_listener_) {
      p_price_level_order_book_global_listener_->SetTimeToSkipUntilFirstEvent(r_start_time_);
    }

    if (p_order_level_global_listener_) {
      p_order_level_global_listener_->SetTimeToSkipUntilFirstEvent(r_start_time_);
    }

    if (p_order_level_listener_sim_) {
      p_order_level_listener_sim_->SetTimeToSkipUntilFirstEvent(r_start_time_);
    }
  }

  inline bool IsNormalTradeTime(int security_id_, ttime_t tv_) {
    return trade_time_manager_.isValidTimeToTrade(security_id_, tv_.tv_sec % 86400);
  }


 private:
  bool _SetNextTimeStamp();
  MktStatus_t GetMarketStatus(NTP_MDS::NTPCommonStruct& _next_event_);
  void _ProcessThisMsg();
  void ComputeFirstNonIntermediateTime(std::string logged_message_filesource_name_);
};
}
#endif  // BASE_MDSMESSAGES_NTP_LOGGED_MESSAGE_FILESOURCE_H
