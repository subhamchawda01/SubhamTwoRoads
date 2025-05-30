/**
    \file puma_md_processor.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include <unordered_map>
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "infracore/lwfixfast/livesources/new_puma_raw_live_data_source.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/mds_logger.hpp"
#include "dvccode/Utils/mds_shm_interface.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/Utils/shm_writer.hpp"

namespace PUMA_MD_PROCESSOR {

struct PUMARefStruct {
  uint64_t id;
  const char* secname;
  int group_id;
  int channel_no;
  uint32_t rpt_seq;
  uint32_t trading_status;
  bool recovery;
  bool product_seen;
  bool is_following_group;
};

class PumaMDProcessor {
 private:
  PumaMDProcessor(HFSAT::MulticastSenderSocket* sock_, HFSAT::NewPumaRawLiveDataSource* rls,
                  HFSAT::FastMdConsumerMode_t mode, bool recovery_flag_, int bmf_feed_type,
                  bool is_full_reference_mode);

  PumaMDProcessor(const PumaMDProcessor&);  // disable copy constructor

  bool recovery_flag_;
  HFSAT::MulticastSenderSocket* sock_;
  bool is_full_reference_mode_;

 public:
  MDSLogger<NTP_MDS::NTPCommonStruct> mdsLogger;
  HFSAT::NewPumaRawLiveDataSource* pumaLiveSource;
  NTP_MDS::NTPCommonStruct* cstr;

 private:
  NTP_MDS::NTPCommonStruct* old_cstr_;
  bool is_old_valid;
  uint64_t old_sec_id;
  NTP_MDS::NTPCommonStruct* old_trd_cstr_;
  bool old_trd_valid;
  uint64_t old_trd_sec_id;
  int bmf_feed_type_;

 public:
  std::unordered_map<uint64_t, PUMARefStruct*> ref_data_;
  std::map<uint64_t, bool> product_seen;

  /// ofstream for writing product reference information
  std::ofstream rfile;
  std::ofstream yesterday_settlement_file_;

  // reading reference information
  std::ifstream dfile;

  std::map<uint64_t, uint8_t> security_trading_status_;
  std::map<std::string, uint8_t> security_group_trading_status_;

  int tradingdate_;

  HFSAT::FastMdConsumerMode_t mode_;

  SHM::ShmWriter<NTP_MDS::NTPCommonStruct>* shm_writer_;

  HFSAT::Utils::MDSShmInterface* mds_shm_interface_;
  HFSAT::MDS_MSG::GenericMDSMessage* generic_mds_message_;

  std::map<uint64_t, uint32_t> rpt_seq_map_;
  /// specific to recovery mode
  std::map<uint64_t, bool> recovery_;

  timeval md_time_;

  static PumaMDProcessor* ins;

  void seqReset();

  /// functions to dispatch quotes/trades
  void flushQuoteQueue(bool intermediate_);
  void dispatchQuote(uint64_t sec_id_);
  void flushTradeQueue();
  void updateSeqno(uint64_t secid, uint32_t seqno);
  void dispatchTrade(uint64_t sec_id_);

  /// for reference mode handling
  void dumpSecurityDefinition(uint64_t secId, const std::string& symbol, const std::string& secGroup);

  inline bool IsProcessingSecurity(const uint64_t sec_id) {
    if (ref_data_.find(sec_id) == ref_data_.end()) {
      return false;
    }

    return true;
  }

 private:
  static PumaMDProcessor& SetInstance(HFSAT::MulticastSenderSocket* sock, HFSAT::NewPumaRawLiveDataSource* rls,
                                      HFSAT::FastMdConsumerMode_t mode, bool recovery_flag, int bmf_feed_type,
                                      bool is_full_reference_mode = false);

 public:
  /// Makes sure that there is only one instance of PumaMDProcessor
  static PumaMDProcessor& GetInstance();

  static void InitMcastMode(HFSAT::MulticastSenderSocket* sock_, HFSAT::NewPumaRawLiveDataSource* rls,
                            bool recovery_flag, int bmf_feed_type);
  static void InitRefMode(HFSAT::NewPumaRawLiveDataSource* rls, bool recovery_flag, int bmf_feed_type,
                          bool is_full_reference_mode);
  static void InitRawMode(HFSAT::NewPumaRawLiveDataSource* rls, bool recovery_flag, int bmf_feed_type);
  static void InitLoggerMode(HFSAT::NewPumaRawLiveDataSource* rls, bool recovery_flag, int bmf_feed_type);
  static void InitProShmMode(HFSAT::NewPumaRawLiveDataSource* rls, bool recovery_flag, int bmf_feed_type);
  static void InitComShmMode(HFSAT::NewPumaRawLiveDataSource* rls, bool recovery_flag, int bmf_feed_type);

  void cleanup();

  void UpdateBookDelta(uint64_t sec_id, uint16_t level, uint16_t num_ords, double price, int32_t size, uint32_t seq_no,
                       char entry_type, uint32_t update_action);

  void UpdateTrade(uint64_t sec_id, int32_t size, uint64_t total_qty, uint32_t seq_no, double price, bool is_last,
                   bool is_cross);

  void UpdateOpeningPrice(uint64_t sec_id, double price, int32_t size, uint32_t seq_no, uint32_t open_close_flag);

  void DumpClosePrice(uint64_t sec_id, double price);

  void UpdateImbalance(uint64_t sec_id, int32_t size, uint32_t seq_no, char condition);

  bool UpdateSeqno(uint64_t sec_id, uint32_t seq_no);

  void UpdateSecurityTradingStatus(uint64_t sec_id, uint32_t trading_status, uint64_t open_time, uint64_t transact_time,
                                   uint32_t trading_event, uint32_t rpt_seq);

  void UpdateGroupTradingStatusRefresh(uint64_t sec_id, uint32_t trading_status, uint64_t open_time,
                                       uint64_t transact_time, uint32_t trading_event, uint32_t rpt_seq);

  void UpdateGroupTradingStatusInc(int group_id, uint32_t trading_status, uint64_t open_time, uint64_t transact_time,
                                   uint32_t trading_event, uint32_t rpt_seq);

  void OpenRefFiles();
  void LoadRefData();
  void OpenSettlementFile();
  inline int GetFeedType() { return bmf_feed_type_; }
  bool filterQCond(char* conds, int len);

  std::set<int> channel_recovery_;
  std::map<uint64_t, const char*> idmap_;
  int num_recovery_securities_;
  timeval recovery_start_time_;
  std::map<const char*, uint64_t> sec_to_id_map_;
  std::map<uint64_t, std::vector<NTP_MDS::NTPCommonStruct*> > message_buffer_;

  void StartAllRecovery();
  void StartRecovery(uint64_t sec_id);
  void EndRecovery(uint64_t sec_id);
  void EndAllRecovery();

  void PrintGroups();
};
}
