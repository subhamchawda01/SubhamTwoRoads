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
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/mds_logger.hpp"
#include "dvccode/Utils/mds_shm_interface.hpp"
#include "dvccode/CDef/mds_messages.hpp"

namespace NTP_ORD_MD_PROCESSOR {

#define NTP_SEC_TRADING_STATUS_PAUSE 2
#define NTP_SEC_TRADING_STATUS_CLOSE 4
#define NTP_SEC_TRADING_STATUS_OPEN 17
#define NTP_SEC_TRADING_STATUS_FORBIDDEN 18
#define NTP_SEC_TRADING_STATUS_UNKNOWN 20
#define NTP_SEC_TRADING_STATUS_RESERVED 21

class NtpOrdMdProcessor {
  NtpOrdMdProcessor(HFSAT::MulticastSenderSocket* sock_, HFSAT::NewNTPRawLiveDataSource* rls,
                    HFSAT::FastMdConsumerMode_t mode, bool recovery_flag_);

  NtpOrdMdProcessor(const NtpOrdMdProcessor&);  // disable copy constructor

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
  std::map<uint64_t, bool> product_seen;
  std::map<uint64_t, const char*> idmap_;
  std::map<uint64_t, int> idchnmap_;
  std::map<const char*, uint64_t> sec_to_id_map_;
  std::map<uint64_t, uint32_t> rpt_seq_map_;
  std::map<uint64_t, std::vector<NTP_MDS::NTPCommonStruct*> > message_buffer_;

  /// specific to recovery mode
  std::map<uint64_t, bool> recovery_;

  /// required file handles for various modes
  std::ofstream ofile;
  /// ofstream for writing product reference information
  std::ofstream rfile;
  // reading reference information
  std::ifstream dfile;

  std::map<uint64_t, uint8_t> security_trading_status_;

  int tradingdate_;
  int last_midnight_sec_;

  HFSAT::FastMdConsumerMode_t mode;

  key_t key;
  int shmid;

  volatile NTP_MDS::NTPCommonStruct *ntp_shm_queue_, *ntp_shm_queue_pointer_;

  struct shmid_ds shm_ds;
  int count;
  int last_write_seq_num_;
  HFSAT::Utils::MDSShmInterface* mds_shm_interface_;
  HFSAT::MDS_MSG::GenericMDSMessage* generic_mds_message_;

  HFSAT::SecurityNameIndexer& sec_name_indexer_;

  volatile int* shm_queue_index_;

  timeval md_time_;

  void seqReset();
  void updateSeqno(uint64_t secid, uint32_t seqno);

  /// functions to dispatch quotes/trades
  void flushQuoteQueue(bool intermediate_);
  void dispatchQuote(uint64_t sec_id_);
  void flushTradeQueue();
  void dispatchTrade(uint64_t sec_id_);

  /// filter irrelevant MDEntryType messages
  bool filterType(char type);
  bool filterQCond(char* conds, int strlen);

  /// for reference mode handling
  void dumpSecurityDefinition(uint64_t secId, const std::string& symbol, const std::string& secGroup);
  void endRecovery(int channel_id_);

  bool IsSecurityInTrdingPhase(uint64_t sec_id) {
    // does not check if the sec id is valid or not. that should be checked by caller
    //      return security_trading_status_[sec_id] == NTP_SEC_TRADING_STATUS_OPEN;
    return true;
  }

  void updateSecurityTradingStatus(uint64_t sec_id_, uint8_t sec_trading_status_) {
    //      security_trading_status_[sec_id_] = sec_trading_status_;
  }

  inline bool IsProcessingSecurity(const uint64_t& _exch_security_id_) {
    if (idmap_.find(_exch_security_id_) == idmap_.end()) return false;

    // Only Combined Source
    if (HFSAT::kComShm == mode) {
      if (sec_name_indexer_.GetIdFromSecname(idmap_[_exch_security_id_]) < 0) return false;
    }

    return true;
  }

 private:
  static NtpOrdMdProcessor& SetInstance(HFSAT::MulticastSenderSocket* sock_, HFSAT::NewNTPRawLiveDataSource* rls,
                                        HFSAT::FastMdConsumerMode_t mode, bool recovery_flag) {
    static NtpOrdMdProcessor* ins = NULL;
    if (ins == NULL) ins = new NtpOrdMdProcessor(sock_, rls, mode, recovery_flag);
    return *ins;
  }

 public:
  /// Makes sure that there is only one instance of NtpOrdMdProcessor
  static NtpOrdMdProcessor& GetInstance() { return SetInstance(NULL, NULL, HFSAT::kModeMax, true); }

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
  static void InitProShmMode(HFSAT::NewNTPRawLiveDataSource* rls, bool recovery_flag) {
    SetInstance(NULL, rls, HFSAT::kProShm, recovery_flag);
  }
  static void InitComShmMode(HFSAT::NewNTPRawLiveDataSource* rls, bool recovery_flag) {
    SetInstance(NULL, rls, HFSAT::kComShm, recovery_flag);
  }
  void cleanup() {
    if (rfile.is_open()) {
      rfile.close();
      std::cerr << "closed reference file\n";
    }
    if (mdsLogger.isRunning()) mdsLogger.stop();
  }
};
}
