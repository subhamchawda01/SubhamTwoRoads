/**
    \file LoggedSources/hkex_logged_message_filesource.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         India
         +91 80 4190 3551
*/
#pragma once

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/LoggedSources/hkex_logged_message_filenamer.hpp"
#include "baseinfra/MarketAdapter/hkex_trade_time_manager.hpp"

namespace HFSAT {

/// @brief reads HKEX logged data stored in file for this { symbol, tradingdate }
/// Reads HKEX_MDS::HKEXCommonStruct
///    depending on HKEX_MDS::HKEXCommonStruct::msgType
///    and for msgType == HKEX_MDS::HKEX_DELTA
///        depending on HKEX_MDS::HKEXCommonStruct::data_::bmf_dels_::action_
///        calls the listeners, if any, of the security, action_
class HKEXLoggedMessageFileSource : public ExternalDataListener {
 protected:
  DebugLogger& dbglogger_;
  const SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;
  ExternalTimeListener* m_time_keeper_;
  BulkFileReader bulk_file_reader_;
  HKEX_MDS::HKEXCommonStruct next_event_;
  TradingLocation_t trading_location_file_read_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;
  HFSAT::HkexTradeTimeManager& hkex_trade_time_manager_;
  std::vector<HKHalfBookGlobalListener*> hk_halfbook_listener_vec_;

  bool use_fake_faster_data_;

 public:
  HKEXLoggedMessageFileSource(DebugLogger& t_debuglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                              const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                              const char* t_exchange_symbol_, TradingLocation_t r_trading_location_,
                              bool t_use_fake_faster_data_ = true);

  inline int socket_file_descriptor() const { return 0; }

  void SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_);

  void ComputeEarliestDataTimestamp(bool& t_hasevents_);

  void ProcessAllEvents();

  inline bool IsNormalTradeTime(int securityId, ttime_t tv) {
    return hkex_trade_time_manager_.isValidTimeToTrade(securityId, tv.tv_sec % 86400);
  }

  void ProcessEventsTill(const ttime_t t_endtime_);

  inline void SetHKHalfBookGlobalListener(HKHalfBookGlobalListener* p_new_listener_) {
    AddHKHalfBookGlobalListener(p_new_listener_);
  }

  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { m_time_keeper_ = t_new_listener_; }

  void AddHKHalfBookGlobalListener(HKHalfBookGlobalListener* p_this_listener_);
  void NotifyHKHalfBookChange(const unsigned int t_security_id_, const HKEX_MDS::HKEXBookStruct& t_hkex_book_struct_);
  void NotifyHKTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_);

 private:
  void _ProcessThisMsg();
};
}
