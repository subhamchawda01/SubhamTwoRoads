/**
    \file LoggedSources/krx_logged_message_filesource.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         India
         +91 80 4190 3551
*/
#ifndef KRX_LOGGED_MESSAGE_FILESOURCE_H
#define KRX_LOGGED_MESSAGE_FILESOURCE_H

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#include "baseinfra/LoggedSources/krx_logged_message_filenamer.hpp"

namespace HFSAT {

/// @brief reads KRX logged data stored in file for this { symbol, tradingdate }
/// Reads KRX_MDS::KRXCommonStruct
///    depending on KRX_MDS::KRXCommonStruct::msgType
///    and for msgType == KRX_MDS::KRX_DELTA
///        depending on KRX_MDS::KRXCommonStruct::data_::bmf_dels_::action_
///        calls the listeners, if any, of the security, action_
class KRXLoggedMessageFileSource : public ExternalDataListener {
 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;
  ExternalTimeListener* m_time_keeper_;
  BulkFileReader bulk_file_reader_;
  KRX_MDS::KRXCommonStruct next_event_;
  TradingLocation_t trading_location_file_read_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;
  // HFSAT::TmxTradeTimeManager& krx_trade_time_manager_;
  std::vector<FullBookGlobalListener*> fullbook_listener_vec_;

  bool use_fake_faster_data_;

 public:
  KRXLoggedMessageFileSource(DebugLogger& t_debuglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                             const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                             const char* t_exchange_symbol_, TradingLocation_t r_trading_location_,
                             bool t_use_fake_faster_data_ = true);

  inline int socket_file_descriptor() const { return 0; }

  /*inline bool IsNormalTradeTime(int securityId, ttime_t tv) {
    return krx_trade_time_manager_.isValidTimeToTrade(securityId, tv.tv_sec % 86400);
  }*/

  void SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_);

  void ComputeEarliestDataTimestamp(bool& t_hasevents_);

  void ProcessAllEvents();

  void ProcessEventsTill(const ttime_t t_endtime_);

  inline void SetFullBookGlobalListener(FullBookGlobalListener* p_new_listener_) {
    AddFullBookGlobalListener(p_new_listener_);
  }

  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { m_time_keeper_ = t_new_listener_; }

  void AddFullBookGlobalListener(FullBookGlobalListener* p_this_listener_);
  void NotifyFullBookChange(const unsigned int t_security_id_, const FullBook* t_full_book_);
  void NotifyTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
                   const TradeType_t t_buysell_);

 private:
  void _ProcessThisMsg();
};
}
#endif
