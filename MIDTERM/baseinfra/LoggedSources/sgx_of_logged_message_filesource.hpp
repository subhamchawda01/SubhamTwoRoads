#pragma once

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/hybrid_sec.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/LoggedSources/common_logged_message_filenamer.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "baseinfra/MarketAdapter/trade_time_manager.hpp"
#include "dvccode/CDef/sgx_mds_defines.hpp"

namespace HFSAT {

class SGXOrderLoggedMesageFileSource : public ExternalDataListener {
 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;

  ExternalTimeListener* p_time_keeper_;
  BulkFileReader bulk_file_reader_;
  SGX_ITCH_MDS::SGXItchOrder next_event_;
  SGX_ITCH_MDS::SGXItchOrder previous_event_;

  TradingLocation_t trading_location_file_read_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;
  HFSAT::TradeTimeManager& trade_time_manager_;
  bool has_trading_hours_started_;
  bool has_trading_hours_closed_;
  bool use_fake_faster_data_;

  // vector of listener for sgx data
  std::vector<OrderLevelListener*> sgx_listener_vec_;

  ttime_t start_timestamp_;
  ttime_t end_timestamp_;

  int YYYYMMDD_;
  std::string shortcode_;

 public:
  SGXOrderLoggedMesageFileSource(DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                                 const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                                 const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
                                 bool t_use_fake_faster_data_ = true);

  ~SGXOrderLoggedMesageFileSource() {}
  inline void SeekToFirstEventAfter(const ttime_t _endtime_, bool& rw_hasevents_){};
  void ProcessThisMsg();
  void ComputeEarliestDataTimestamp(bool& t_hasevents_);
  TradeType_t GetTradeType(unsigned int type);

  void ProcessAllEvents();

  inline bool IsNormalTradeTime(int securityId, ttime_t tv) {
    return trade_time_manager_.isValidTimeToTrade(securityId, tv.tv_sec % 86400);
  }
  void ProcessEventsTill(const ttime_t t_endtime_);

  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { p_time_keeper_ = t_new_listener_; }

  void AddSGXListener(OrderLevelListener* listener);

  void NotifyOrderAdd(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                      const double price, const uint32_t size);

  void NotifyOrderDelete(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id);
  void NotifyOrderExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                       const double exec_price, const uint32_t size_exec, const uint32_t size_remaining);
  void NotifyOrderModify(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                         const double price, const uint32_t size);
  bool SetNextTimeStamp();
  void NotifyResetBook(const unsigned int security_id);
  inline int socket_file_descriptor() const { return 0; }
  inline void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_){};
};
}
