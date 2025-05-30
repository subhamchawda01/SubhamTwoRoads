/**
    \file lwfixfast/eobi_md_processor.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <unordered_map>
#include "dvccode/Utils/bse_daily_token_symbol_handler.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/simple_mempool.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "infracore/BSEMD/indexed_eobi_order_book.hpp"
#include "infracore/BSEMD/indexed_eobi_order_book_for_normal_pricefeed.hpp"

#include "dvccode/CDef/refdata_locator.hpp"
#include "infracore/BSEMD/bse_raw_md_handler.hpp"
#include "dvccode/Utils/mds_logger.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/Utils/mds_shm_interface.hpp"
#include "dvccode/Utils/shm_writer.hpp"
#include "dvccode/CDef/mds_messages.hpp"

#define EOBI_PRICE_DIVIDER 100000000.0

namespace BSE_MD_Processor {

class BSEMDProcessor {
  BSEMDProcessor(HFSAT::MulticastSenderSocket* sock_, HFSAT::BSERawMDHandler* rls,
                  HFSAT::FastMdConsumerMode_t mode_, bool recoveryFlag_, bool full_mode = false);

  BSEMDProcessor(const BSEMDProcessor&);  // disable copy constructor

 private:
  // ====
  // REFERENCE mode variables and functions
  // ====
  std::map<int64_t, bool> prmcast_seen_;
  std::map<std::string, bool> fpga_feed_dumped_;
  std::map<uint32_t, std::string> mktseg_id_2_name_;
  std::map<int64_t, bool> prref_seen_;
  std::map<int64_t, uint32_t> simple_secid_to_expiry_;
  std::ofstream reference_info_file_;  // prod codes
  std::ofstream multicast_info_file_;  // mcast addresses
  std::ofstream fpga_feed_file_;

  HFSAT::MulticastSenderSocket* sock;
  HFSAT::MulticastSenderSocket* secondary_sock;
  HFSAT::BSERawMDHandler* eobiLiveSource;

  std::map<uint8_t, bool> live_source_products_list_;
  std::map<std::string, std::string> exchange_symbol_to_shortcode_map_;
  std::map<std::string, uint8_t> shortcode_to_product_code_map_;

  void ReadContractCodes();
  void ReferenceDataProcessing();
  void ReadProductCodes();
  inline void CreateMdsStruct();
  void StartRecoveryForAllProducts();
  void EndRecoveryForAllProducts();

 public:
  MDSLogger<EOBI_MDS::EOBICommonStruct> mdsLogger;

  uint32_t o_seq_num_;
  int64_t o_security_id_;
  uint32_t o_action_;
  uint8_t o_side_;
  int64_t o_price_;
  int32_t o_size_;
  uint64_t o_trd_reg_ts_;
  uint64_t o_priority_ts_;
  int64_t o_prev_price_;
  int32_t o_prev_size_;
  uint64_t o_prev_priority_ts_;
  bool o_intermediate_;
  uint8_t o_synthetic_match_;

  timeval logger_start_time_;

  unsigned int send_sequence_;

  bool initial_recovery_not_finished_;

  HFSAT::FastMdConsumerMode_t mode;
  bool full_mode_;

  std::map<std::string, bool> ref_mode_product_list_;

  EOBI_MDS::EOBICommonStruct* cstr;

  std::unordered_map<int64_t, char*> idmap_;  // Mapping between security id and contract name
  std::unordered_map<int64_t, double> sec_id_to_min_px_inc_map_;
  std::map<int32_t, char*> mkt_seg_id_to_prod_name_;  // Mapping between market segment id and the product name
  std::unordered_map<int32_t, uint32_t>
      mkt_seg_id_to_seq_num_;  // Mapping between market segment id and the latest message sequence number
  std::unordered_map<int64_t, uint32_t>
      target_msg_seq_num_;  // Mapping between security id and the message sequence number of instrument summary message
  int32_t current_mkt_seg_id_;        // Current market segment id extracted from the packet header
  uint8_t pkt_completion_indicator_;  // Packet completion indicator extracted from the packet header

  std::unordered_map<int64_t, int32_t> sec_to_mkt_seg_id_;
  std::unordered_map<int32_t, int64_t> curr_sec_id_;
  std::unordered_map<int64_t, bool> instr_summry_rcvd_;
  std::map<int64_t, bool> security_in_recovery;
  std::unordered_map<int32_t, int> prod_recovery_count_;
  std::map<int32_t, std::vector<int64_t> > mkt_seg_to_security_;
  std::unordered_map<int64_t, uint8_t> sec_id_to_contract_code_;

  std::unordered_map<int64_t, HFSAT::EobiOrderBook*> order_book_;
  std::unordered_map<std::string, HFSAT::EobiOrderBookForNPF*> order_book_for_normal_pricefeed_;
  std::unordered_map<int64_t, int> sec_id_to_indexed_sec_id_;

  int total_recovery_count_;
  std::vector<std::pair<EOBI_MDS::EOBICommonStruct*, int64_t> > message_buffer_;
  std::vector<std::pair<uint64_t, uint64_t> > price_buffer_;

  HFSAT::Utils::MDSShmInterface* mds_shm_interface_;
  HFSAT::MDS_MSG::GenericMDSMessage* generic_mds_message_;

  timeval md_time_;

  int64_t spread_security_id_;
  int64_t* leg_sec_id_;

  SHM::ShmWriter<EOBI_MDS::EOBICommonStruct>* eobi_shm_writer_;

  HFSAT::BSERawMDHandler* GetLiveSource() { return eobiLiveSource; }

  void InitializeEOBIPriceFeedShortcodes();

  void ProcessOrder();

  void StartRecovery(const char* p_prod_);
  void EndRecovery(const char* p_prod_);
  void StartRecovery(int64_t security_id_);
  void EndRecovery(int64_t security_id_);
  void StartRecoveryForProduct(int32_t security_id_);
  void CheckSequenceNo(int seq_no_, bool start_recovery_on_mismatch_);
  void CheckSequenceNum();

  void dumpProdRefInfo(const uint32_t, const std::string&, int, const std::string&, const std::string&, unsigned int);

  void dumpSLRefInfo(int64_t, uint32_t, uint32_t, double);

  void DumpComplexInstrument(uint32_t mkt_seg_id);
  void DumpFPGAFeeds(uint32_t t_mkt_seg_id_, int t_feed_type_, std::string t_ip1_, int t_port1_, std::string t_ip2_,
                     int t_port2_);
  std::string GetExpiry(uint32_t t_expiry_);

  /// Makes sure that there is only one instance of BSEMDProcessor
  static BSEMDProcessor& GetInstance() {
    return BSEMDProcessor::SetInstance(NULL, NULL, HFSAT::kModeMax, true);  // dummy arguments
    // set instance must be called prior to GetInstance, else we will have erroneous results
  }

  static BSEMDProcessor& SetInstance(HFSAT::MulticastSenderSocket* sock_, HFSAT::BSERawMDHandler* rls,
                                      HFSAT::FastMdConsumerMode_t mode_, bool recovery_flag_, bool full_mode = false) {
    static BSEMDProcessor* instance = NULL;

    if (instance == NULL) {
      instance = new BSEMDProcessor(sock_, rls, mode_, recovery_flag_, full_mode);
    }

    return *instance;
  }

  void cleanup() {
    if (reference_info_file_.is_open()) {
      reference_info_file_.close();
      std::cerr << "closed reference file\n";
    }

    if (multicast_info_file_.is_open()) {
      multicast_info_file_.close();
      std::cerr << "closed mcast info file\n";
    }

    if (mdsLogger.isRunning()) {
      mdsLogger.stop();
    }

    // cleaning up the Price Conversion Books
    std::unordered_map<std::string, HFSAT::EobiOrderBookForNPF*>::iterator itr_;

    for (auto order_book_ : order_book_for_normal_pricefeed_) {
      if (order_book_.second) {
        delete (order_book_.second);
        order_book_.second = NULL;
      }
    }
  }
};
}
