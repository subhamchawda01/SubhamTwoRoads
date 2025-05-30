/**
    \file LoggedSources/hkex_logged_message_filesource.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066
         India
         +91 80 4060 0717
*/
#ifndef HKEX_LOGGED_MESSAGE_FILESOURCE_H
#define HKEX_LOGGED_MESSAGE_FILESOURCE_H

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/LoggedSources/hkomd_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkstocks_logged_message_filenamer.hpp"
#include "baseinfra/MarketAdapter/hkex_trade_time_manager.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

namespace HFSAT {

class HKOMDLoggedMessageFileSource : public ExternalDataListener {
 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;
  OrderLevelGlobalListenerHKOMD* order_level_global_listener_hkomd_;
  ExternalTimeListener* m_time_keeper_;
  BulkFileReader bulk_file_reader_;
  HKOMD_MDS::HKOMDCommonStruct next_event_;
  TradingLocation_t trading_location_file_read_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;
  FullBook next_book_;
  HFSAT::HkexTradeTimeManager& hkex_trade_time_manager_;
  bool use_fake_faster_data_;
  std::vector<OrderLevelListenerHK*> order_level_listener_vec_;
  bool is_hk_eq_;

 public:
  HKOMDLoggedMessageFileSource(DebugLogger& t_debuglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                               const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                               const char* t_exchange_symbol_, TradingLocation_t r_trading_location_,
                               bool use_todays_data_ = false, bool t_use_fake_faster_data_ = true,
                               bool is_hk_eq = false)
      : dbglogger_(t_debuglogger_),
        sec_name_indexer_(t_sec_name_indexer_),
        security_id_(t_security_id_),
        exchange_symbol_(t_exchange_symbol_),
        order_level_global_listener_hkomd_(NULL),
        m_time_keeper_(NULL),
        bulk_file_reader_(),
        next_event_(),
        trading_location_file_read_(r_trading_location_),
        delay_usecs_to_add_(0),
        need_to_add_delay_usecs_(false),
        hkex_trade_time_manager_(HFSAT::HkexTradeTimeManager::GetUniqueInstance(t_sec_name_indexer_)),
        use_fake_faster_data_(t_use_fake_faster_data_),
        is_hk_eq_(is_hk_eq)

  {
    next_event_timestamp_.tv_sec = 0;
    next_event_timestamp_.tv_usec = 0;
    dbglogger_ << "in constructor\n";
    dbglogger_.DumpCurrentBuffer();

    std::string m_hkex_filename_ = "";
    if (is_hk_eq_) {
      m_hkex_filename_ = HKStocksLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                                 trading_location_file_read_, use_todays_data_);
    } else {
      m_hkex_filename_ = HKOMDLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                              trading_location_file_read_, use_todays_data_);
    }

    int added_delay = 0;
    // added delays is used by the robustness check script to variate line delay by a constant number
    added_delay = TradingLocationUtils::GetAddedDelay(trading_location_file_read_, r_trading_location_);
    delay_usecs_to_add_ = added_delay;

    if (trading_location_file_read_ != r_trading_location_) {
      delay_usecs_to_add_ +=
          TradingLocationUtils::GetMSecsBetweenTradingLocations(trading_location_file_read_, r_trading_location_) *
          1000;
    }
    if (delay_usecs_to_add_ != 0) need_to_add_delay_usecs_ = true;
    //     dbglogger_
    if (TradingLocationUtils::GetUsecsForFasterDataForShortcodeAtLocation(
            sec_name_indexer_.GetShortcodeFromId(security_id_), r_trading_location_, t_preevent_YYYYMMDD_,
            use_fake_faster_data_) !=
        0) {  // Experiment with the possibility of using faster data for this product at this location.
      need_to_add_delay_usecs_ = true;
      delay_usecs_to_add_ -= TradingLocationUtils::GetUsecsForFasterDataForShortcodeAtLocation(
          sec_name_indexer_.GetShortcodeFromId(security_id_), r_trading_location_, t_preevent_YYYYMMDD_,
          use_fake_faster_data_);
    }

    // Open file with BulkFileReader
    if (m_hkex_filename_.find("NO_FILE_AVAILABLE") == std::string::npos) {
      bulk_file_reader_.open(m_hkex_filename_);
    } else {
      dbglogger_ << "For HKEX symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
                 << trading_location_file_read_ << " returned filename = " << m_hkex_filename_ << "\n";
      dbglogger_.DumpCurrentBuffer();

      std::cerr << "For HKEX symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
                << trading_location_file_read_ << " returned filename = " << m_hkex_filename_ << std::endl;
    }
    // bulk_file_reader_.open ( m_hkex_filename_ ) ;
  }

  inline int socket_file_descriptor() const { return 0; }

  void SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_);

  void ComputeEarliestDataTimestamp(bool& t_hasevents_);

  void ProcessAllEvents();

  inline bool IsNormalTradeTime(int securityId, ttime_t tv) {
    return hkex_trade_time_manager_.isValidTimeToTrade(securityId, tv.tv_sec % 86400);
  }

  void ProcessEventsTill(const ttime_t t_endtime_);

  void SetOrderLevelGlobalListener(OrderLevelGlobalListenerHKOMD* t_order_level_listener_) {
    order_level_global_listener_hkomd_ = t_order_level_listener_;
  }

  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { m_time_keeper_ = t_new_listener_; }

  void AddOrderLevelListener(OrderLevelListenerHK* p_this_listener_);

  void NotifyOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                      const double t_price_, const uint32_t t_size_);

  void NotifyOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                         const double t_price_, const uint32_t t_size_);

  void NotifyOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_);

  void NotifyOrderExec(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                       const double t_traded_price_, const uint32_t t_traded_size_);

  void NotifyResetBook(const unsigned int t_security_id_);

 private:
  bool SetNextTimeStamp();
  void ProcessThisMsg();
};

class HKOMDPFLoggedMessageFileSource : public ExternalDataListener {
 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;
  PriceLevelGlobalListener* price_level_global_listener_hkomd_;
  ExternalTimeListener* m_time_keeper_;
  BulkFileReader bulk_file_reader_;
  HKOMD_MDS::HKOMDPFCommonStruct next_event_;
  TradingLocation_t trading_location_file_read_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;
  FullBook next_book_;
  HFSAT::HkexTradeTimeManager& hkex_trade_time_manager_;
  bool use_fake_faster_data_;
  bool is_hk_eq_;

 public:
  HKOMDPFLoggedMessageFileSource(DebugLogger& t_debuglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                                 const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                                 const char* t_exchange_symbol_, TradingLocation_t r_trading_location_,
                                 bool use_todays_data_ = false, bool t_use_fake_faster_data_ = true,
                                 bool is_hk_eq = false);

  inline int socket_file_descriptor() const { return 0; }

  void SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_);

  void ComputeEarliestDataTimestamp(bool& t_hasevents_);

  void ProcessAllEvents();

  inline bool IsNormalTradeTime(int securityId, ttime_t tv) {
    return hkex_trade_time_manager_.isValidTimeToTrade(securityId, tv.tv_sec % 86400);
  }

  void ProcessEventsTill(const ttime_t t_endtime_);

  void SetPriceLevelGlobalListener(PriceLevelGlobalListener* t_price_level_listener_) {
    price_level_global_listener_hkomd_ = t_price_level_listener_;
  }

  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { m_time_keeper_ = t_new_listener_; }
};

class HKOMDCPFLoggedMessageFileSource : public ExternalDataListener {
 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;
  PriceLevelGlobalListener* price_level_global_listener_hkomd_;
  ExternalTimeListener* m_time_keeper_;
  BulkFileReader bulk_file_reader_;
  HKOMD_MDS::HKOMDPFCommonStruct next_event_;
  TradingLocation_t trading_location_file_read_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;
  FullBook next_book_;
  HFSAT::HkexTradeTimeManager& hkex_trade_time_manager_;
  bool use_fake_faster_data_;
  bool is_hk_eq_;

 public:
  HKOMDCPFLoggedMessageFileSource(DebugLogger& t_debuglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                                  const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                                  const char* t_exchange_symbol_, TradingLocation_t r_trading_location_,
                                  bool use_todays_data_ = false, bool t_use_fake_faster_data_ = true,
                                  bool is_hk_eq = false)
      : ExternalDataListener(),
        dbglogger_(t_debuglogger_),
        sec_name_indexer_(t_sec_name_indexer_),
        security_id_(t_security_id_),
        exchange_symbol_(t_exchange_symbol_),
        price_level_global_listener_hkomd_(NULL),
        m_time_keeper_(NULL),
        bulk_file_reader_(),
        next_event_(),
        trading_location_file_read_(r_trading_location_),
        delay_usecs_to_add_(0),
        need_to_add_delay_usecs_(false),
        hkex_trade_time_manager_(HFSAT::HkexTradeTimeManager::GetUniqueInstance(t_sec_name_indexer_)),
        use_fake_faster_data_(t_use_fake_faster_data_),
        is_hk_eq_(is_hk_eq)

  {
    next_event_timestamp_.tv_sec = 0;
    next_event_timestamp_.tv_usec = 0;
    dbglogger_ << "in constructor\n";
    dbglogger_.DumpCurrentBuffer();

    std::string m_hkex_filename_ = "";
    if (is_hk_eq_) {
      m_hkex_filename_ = HKStocksPFLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                                   trading_location_file_read_, use_todays_data_);
    } else {
      m_hkex_filename_ = HKOMDCPFLoggedMessagefileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                                 trading_location_file_read_, use_todays_data_);
    }

    int added_delay = 0;
    // added delays is used by the robustness check script to variate line delay by a constant number
    added_delay = TradingLocationUtils::GetAddedDelay(trading_location_file_read_, r_trading_location_);
    delay_usecs_to_add_ = added_delay;

    if (trading_location_file_read_ != r_trading_location_) {
      delay_usecs_to_add_ +=
          TradingLocationUtils::GetMSecsBetweenTradingLocations(trading_location_file_read_, r_trading_location_) *
          1000;
    }
    if (delay_usecs_to_add_ != 0) need_to_add_delay_usecs_ = true;
    //     dbglogger_
    if (TradingLocationUtils::GetUsecsForFasterDataForShortcodeAtLocation(
            sec_name_indexer_.GetShortcodeFromId(security_id_), r_trading_location_, t_preevent_YYYYMMDD_,
            use_fake_faster_data_)) {  // Experiment with the possibility of using faster data for this product at this
                                       // location.
      need_to_add_delay_usecs_ = true;
      delay_usecs_to_add_ -= TradingLocationUtils::GetUsecsForFasterDataForShortcodeAtLocation(
          sec_name_indexer_.GetShortcodeFromId(security_id_), r_trading_location_, t_preevent_YYYYMMDD_,
          use_fake_faster_data_);
    }

    // Open file with BulkFileReader
    if (m_hkex_filename_.find("NO_FILE_AVAILABLE") == std::string::npos) {
      bulk_file_reader_.open(m_hkex_filename_);
    } else {
      dbglogger_ << "For HKEX symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
                 << trading_location_file_read_ << " returned filename = " << m_hkex_filename_ << "\n";
      dbglogger_.DumpCurrentBuffer();

      std::cerr << "For HKEX symbol " << t_exchange_symbol_ << " date " << t_preevent_YYYYMMDD_ << " trading_location "
                << trading_location_file_read_ << " returned filename = " << m_hkex_filename_ << std::endl;
    }
    // bulk_file_reader_.open ( m_hkex_filename_ ) ;
  }

  inline int socket_file_descriptor() const { return 0; }

  void SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_);

  void ComputeEarliestDataTimestamp(bool& t_hasevents_);

  void ProcessAllEvents();

  inline bool IsNormalTradeTime(int securityId, ttime_t tv) {
    return hkex_trade_time_manager_.isValidTimeToTrade(securityId, tv.tv_sec % 86400);
  }

  void ProcessEventsTill(const ttime_t t_endtime_);

  void SetPriceLevelGlobalListener(PriceLevelGlobalListener* t_price_level_listener_) {
    price_level_global_listener_hkomd_ = t_price_level_listener_;
  }

  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { m_time_keeper_ = t_new_listener_; }
};
}
#endif
