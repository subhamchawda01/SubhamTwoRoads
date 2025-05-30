/**
    \file LoggedSources/ice_logged_message_filesource.hpp
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
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
#include "baseinfra/LoggedSources/common_logged_message_filenamer.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "baseinfra/MarketAdapter/liffe_trade_time_manager.hpp"

namespace HFSAT {

/// @brief reads ICE logged data stored in file for this { symbol, tradingdate }
/// Reads ICE_MDS::ICECommonStruct
///    depending on ICE_MDS::ICECommonStruct::msgType
///    and for msgType == ICE_MDS::ICE_DELTA
///        depending on ICE_MDS::ICECommonStruct::data_::ice_dels_::action_
///        calls the listeners, if any, of the security, action_
class ICELoggedMessageFileSource : public ExternalDataListener {
 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;

  ExternalTimeListener* p_time_keeper_;

  BulkFileReader bulk_file_reader_;
  ICE_MDS::ICECommonStruct next_event_;

  /// To account for multiple locations
  TradingLocation_t trading_location_file_read_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;

  HFSAT::LiffeTradeTimeManager& liffe_trade_time_manager_;

  bool has_trading_hours_started_;
  bool has_trading_hours_closed_;
  ttime_t first_non_intermediate_time_;

  std::vector<ICE_MDS::ICECommonStruct> event_queue_;
  int events_left_;
  ttime_t next_non_intermediate_time_;
  bool use_fake_faster_data_;

  std::vector<PriceLevelGlobalListener*> price_level_global_listener_vec_;
  std::vector<OrderLevelListenerICE*> order_level_listener_vec_;
  std::vector<TradeGlobalListener*> trade_listener_vec_;

  bool use_order_feed_;

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
  ICELoggedMessageFileSource(DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                             const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                             const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
                             bool t_use_order_feed_, bool t_use_fake_faster_data_ = true);

  void AddDelay(int usecs_) {
    need_to_add_delay_usecs_ = true;
    delay_usecs_to_add_ += 30;
  }

  ~ICELoggedMessageFileSource() {}

  inline int socket_file_descriptor() const { return 0; }

  void SeekToFirstEventAfter(const ttime_t _endtime_, bool& rw_hasevents_);

  void ComputeEarliestDataTimestamp(bool& t_hasevents_);

  void ProcessAllEvents();

  void ProcessEventsTill(const ttime_t t_endtime_);

  inline bool HasTradeHoursStartedToBuildIndexBook(const unsigned int sec) {
    return liffe_trade_time_manager_.hasTradingStarted(security_id_, sec % 86400);
  }

  inline bool HasTradeHoursClosedToBuildIndexBook(const unsigned int sec) {
    return liffe_trade_time_manager_.hasTradingClosed(security_id_, sec % 86400);
  }

  inline bool IsNormalTradeTimeToBuildIndexBook(const unsigned int sec) {
    return liffe_trade_time_manager_.isValidTimeToTrade(security_id_, sec % 86400);
  }

  inline void SetPriceLevelGlobalListener(PriceLevelGlobalListener* p_new_listener_) {
    AddPriceLevelGlobalListener(p_new_listener_);
  }

  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { p_time_keeper_ = t_new_listener_; }

  void AddOrderLevelListener(OrderLevelListenerICE* p_this_listener_);
  void NotifyPriceLevelUpdate(void* ptr_to_price_level_update, int length_of_bytes);

  void NotifyOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                      const double t_price_, const uint32_t t_size_, const int64_t priority_);

  void NotifyOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                         const double t_price_, const uint32_t t_size_);

  void NotifyOrderDelete(const uint32_t t_security_id_, const int64_t t_order_id_);

  void NotifyOrderExec(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                       const double t_traded_price_, const uint32_t t_traded_size_);

  void NotifyResetBook(const unsigned int t_security_id_);

  void AddTradeListener(TradeGlobalListener* t_listener_);

  void AddPriceLevelGlobalListener(PriceLevelGlobalListener* p_this_listener_);
  void NotifyPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_added_,
                           const double t_price_, const int t_new_size_, const int t_new_ordercount_,
                           const bool t_is_intermediate_message_);
  void NotifyPriceLevelDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                              const int t_level_removed_, const double t_price_, const bool t_is_intermediate_message_);
  void NotifyPriceLevelChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                              const int t_level_changed_, const double t_price_, const int t_new_size_,
                              const int t_new_ordercount_, const bool t_is_intermediate_message_);
  void NotifyTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
                   const TradeType_t t_buysell_);

  void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_);

 private:
  bool _SetNextTimeStamp();
  void _ProcessThisMsg();
  void ComputeFirstNonIntermediateTime(std::string logged_message_filesource_name_);
};
}
