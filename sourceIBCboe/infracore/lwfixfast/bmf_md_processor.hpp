/**
    \file Ntp_md_processor.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "infracore/lwfixfast/livesources/new_ntp_raw_live_data_source.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/Utils/mds_logger.hpp"
#include "infracore/lwfixfast/ntp_md_queue.hpp"

#define ORDER_ID_MASK 0x00000000FFFFFFFF
#define TRADER_ID_MASK 0xFFFFFFFF00000000

namespace BMF_MD_PROCESSOR {

class BmfMDProcessor {
  BmfMDProcessor(HFSAT::MulticastSenderSocket* sock_, HFSAT::NewNTPRawLiveDataSource* rls,
                 HFSAT::FastMdConsumerMode_t mode, bool recovery_flag_);

  BmfMDProcessor(const BmfMDProcessor&);  // disable copy constructor

  HFSAT::FastMdConsumerMode_t mode;
  bool recovery_flag;
  HFSAT::MulticastSenderSocket* sock;

  /// structs to populate for bcasting
 public:
  MDSLogger<NTP_MDS::NTPCommonStruct> mdsLogger;
  HFSAT::NewNTPRawLiveDataSource* ntpLiveSource;
  NTP_MDS::NTPCommonStruct* cstr;

 private:
  NTP_MDS::NTPCommonStruct* old_cstr_;
  bool is_old_valid;
  uint64_t old_sec_id;
  NTP_MDS::NTPCommonStruct* old_trd_cstr_;
  bool old_trd_valid;
  uint64_t old_trd_sec_id;

 public:
  std::map<std::string, bool> product_seen;
  std::map<uint64_t, std::string> idmap_;
  std::map<uint64_t, int> idchnmap_;
  std::map<std::string, uint64_t> sec_to_id_map_;
  std::map<uint64_t, uint32_t> rpt_seq_map_;
  std::map<uint64_t, int> id2OpenPrice_;
  std::map<uint64_t, int> id2SettlementPrice_;
  std::map<uint64_t, int> id2ClosingPrice_;

  /// specific to recovery mode
  std::map<uint64_t, bool> recovery_;

  /// required file handles for various modes
  std::ofstream ofile;
  /// ofstream for writing product reference information
  std::ofstream rfile;
  // reading reference information
  std::ifstream dfile;

  NtpCstrQ ntpCstrQ;
  std::map<uint64_t, NtpCstrQ*> waitQMap;  // incremental messages to be queued

  uint32_t max_rpt_seq;  // used in auto_bmf_template.cpp to identify snapshot end

  static BmfMDProcessor* ins;

  void seqReset();
  void updateSeqno(uint32_t secid, uint32_t seqno);

  /// functions to dispatch quotes/trades
  void flushQuoteQueue(bool intermediate_);
  void dispatchQuote(NTP_MDS::NTPCommonStruct* ntp_cstr);
  void dispatchQuote(uint64_t sec_id_);
  void flushTradeQueue();
  void dispatchTrade(uint64_t sec_id_);

  /// filter irrelevant MDEntryType messages
  bool filterType(char type);
  bool filterQCond(char* conds, int strlen);

  /// for reference mode handling
  void dumpSecurityDefinition(const std::string& secId, const std::string& symbol);
  void endRecovery(int channel_id_);

  bool hasSecId(uint32_t sec_id) { return idmap_.find(sec_id) != idmap_.end(); }

  NtpCstrQ* getWaitQ(uint64_t secId);
  void clearWaitQ(uint64_t secId);

 private:
  static BmfMDProcessor& SetInstance(HFSAT::MulticastSenderSocket* sock_, HFSAT::NewNTPRawLiveDataSource* rls,
                                     HFSAT::FastMdConsumerMode_t mode, bool recovery_flag);

 public:
  /// Makes sure that there is only one instance of BmfMDProcessor
  static BmfMDProcessor& GetInstance() { return SetInstance(NULL, NULL, HFSAT::kModeMax, true); }

  static void InitMcastMode(HFSAT::MulticastSenderSocket* sock_, HFSAT::NewNTPRawLiveDataSource* rls,
                            bool recovery_flag) {
    SetInstance(sock_, rls, HFSAT::kMcast, recovery_flag);
  }
  static void InitRefMode(HFSAT::NewNTPRawLiveDataSource* rls, bool recovery_flag) {
    SetInstance(NULL, rls, HFSAT::kReference, recovery_flag);
  }

  static void InitRawMode(HFSAT::NewNTPRawLiveDataSource* rls, bool recovery_flag) {
    SetInstance(NULL, rls, HFSAT::kRaw, recovery_flag);
  }
  static void InitLoggerMode(HFSAT::NewNTPRawLiveDataSource* rls, bool recovery_flag) {
    SetInstance(NULL, rls, HFSAT::kLogger, recovery_flag);
  }

  void cleanup();
};
}
