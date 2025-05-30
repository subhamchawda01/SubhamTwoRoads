/**
    \file lwfixfast/ntp_md_processor.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include <unordered_map>
#include <set>

#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "infracore/lwfixfast/livesources/new_ntp_raw_live_data_source.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/mds_logger.hpp"
#include "dvccode/Utils/mds_shm_interface.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/Utils/shm_writer.hpp"

namespace NTP_MD_PROCESSOR {

struct NTPRefStruct {
  uint64_t id;
  const char* secname;
  int group_id;
  int channel_no;
  uint64_t rpt_seq;
  uint32_t trading_status;
  bool recovery;
  bool product_seen;
};

class NtpMDProcessor {
  NtpMDProcessor(HFSAT::MulticastSenderSocket* sock_, HFSAT::NewNTPRawLiveDataSource* rls,
                 HFSAT::FastMdConsumerMode_t mode, bool recovery_flag_);

  NtpMDProcessor(const NtpMDProcessor&);  // disable copy constructor

 private:
  bool recovery_flag;
  HFSAT::MulticastSenderSocket* sock;

  HFSAT::NewNTPRawLiveDataSource* ntpLiveSource;
  NTP_MDS::NTPCommonStruct* cstr;

  NTP_MDS::NTPCommonStruct* old_cstr_;
  bool is_old_valid;
  uint64_t old_sec_id;
  NTP_MDS::NTPCommonStruct* old_trd_cstr_;
  bool old_trd_valid;
  uint64_t old_trd_sec_id;

  std::set<int> channel_recovery_;
  int num_recovery_securities_;
  timeval recovery_start_time_;

  /// ofstream for writing product reference information
  std::ofstream rfile;
  // reading reference information
  std::ifstream dfile;

  HFSAT::FastMdConsumerMode_t mode;

  SHM::ShmWriter<NTP_MDS::NTPCommonStruct>* shm_writer_;

  HFSAT::Utils::MDSShmInterface* mds_shm_interface_;
  HFSAT::MDS_MSG::GenericMDSMessage* generic_mds_message_;

  timeval md_time_;

  static NtpMDProcessor* ins;

  static NtpMDProcessor& SetInstance(HFSAT::MulticastSenderSocket* sock_, HFSAT::NewNTPRawLiveDataSource* rls,
                                     HFSAT::FastMdConsumerMode_t mode, bool recovery_flag);

  void OpenRefFiles();
  void LoadRefData();

 public:
  MDSLogger<NTP_MDS::NTPCommonStruct> mdsLogger;
  std::map<uint64_t, bool> product_seen;
  std::unordered_map<uint64_t, NTPRefStruct*> ref_data_;

  /// Makes sure that there is only one instance of NtpMDProcessor
  static NtpMDProcessor& GetInstance();
  static void InitMcastMode(HFSAT::MulticastSenderSocket* sock, HFSAT::NewNTPRawLiveDataSource* rls,
                            bool recovery_flag);
  static void InitRefMode(HFSAT::NewNTPRawLiveDataSource* rls, bool recovery_flag);
  static void InitRawMode(HFSAT::NewNTPRawLiveDataSource* rls, bool recovery_flag);
  static void InitLoggerMode(HFSAT::NewNTPRawLiveDataSource* rls, bool recovery_flag);
  static void InitProShmMode(HFSAT::NewNTPRawLiveDataSource* rls, bool recovery_flag);
  static void InitComShmMode(HFSAT::NewNTPRawLiveDataSource* rls, bool recovery_flag);

  void UpdateBookDelta(uint64_t sec_id, uint16_t level, uint16_t num_ords, double price, int32_t size, uint32_t seq_no,
                       char entry_type, uint32_t update_action, uint16_t buyer, uint16_t seller);

  void UpdateTrade(uint64_t sec_id, int32_t size, uint64_t total_qty, uint32_t seq_no, double price, bool is_last,
                   bool is_cross);

  void UpdateOpeningPrice(uint64_t sec_id, double price, int32_t size, uint32_t seq_no, uint32_t open_close_flag);

  void UpdateImbalance(uint64_t sec_id, int32_t size, uint32_t seq_no, char condition);

  void cleanup();

  void seqReset();
  bool UpdateSeqno(uint64_t secid, uint64_t seqno);

  /// functions to dispatch quotes/trades
  void flushQuoteQueue(bool intermediate);
  void dispatchQuote(uint64_t sec_id);
  void flushTradeQueue();
  void dispatchTrade(uint64_t sec_id);

  /// for reference mode handling
  void DumpSecurityDefinition(uint64_t secId, const std::string& symbol, const std::string& secGroup);
  void endRecovery(int channel_id);

  bool IsSecurityInTrdingPhase(uint64_t sec_id);

  void UpdateSecurityTradingStatus(uint64_t sec_id, uint32_t trading_status, uint64_t open_time, uint64_t transact_time,
                                   uint32_t trading_event, uint32_t rpt_seq);

  void UpdateGroupTradingStatusRefresh(uint64_t sec_id, uint32_t trading_status, uint64_t open_time,
                                       uint64_t transact_time, uint32_t trading_event, uint32_t rpt_seq);

  void UpdateGroupTradingStatusInc(int group_id, uint32_t trading_status, uint64_t open_time, uint64_t transact_time,
                                   uint32_t trading_event, uint32_t rpt_seq);

  bool IsProcessingSecurity(const uint64_t& sec_id);

  void StartAllRecovery();
  void StartRecovery(uint64_t sec_id);
  void EndRecovery(uint64_t sec_id);
  void EndAllRecovery();
};
}
