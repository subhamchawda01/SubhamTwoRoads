// =====================================================================================
//
//       Filename:  generic_live_source_shm_data_logger.cpp
//
//    Description:  Only Meant For EUREX LIVE SHM SOURCE With LOW BW Structs
//
//        Version:  1.0
//        Created:  11/08/2012 08:25:34 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <map>

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/mds_logger.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/Utils/mds_shm_interface.hpp"
#include "dvccode/Utils/load_low_bandwidth_code_mapping.hpp"

std::string exch_ = "";

class LiveMDSLogger {
 protected:
  MDSLogger<HFSAT::MDS_MSG::GenericMDSMessage> *mdsLogger;
  bool set_time;  // set time by logger. we already expect the m-casted data to have time, but we want the logger
                  // time-stamp to be set explicitly

  key_t generic_mds_shm_key_;
  int generic_mds_shmid_;

  volatile HFSAT::MDS_MSG::GenericMDSMessage *generic_mds_shm_struct_;
  struct shmid_ds generic_shmid_ds_;

  // this is meant to be a local copy to track queue position, compare with volatile shared memory counter
  int index_;

  std::map<uint8_t, std::string> eurex_product_code_to_shortcode_;
  std::map<std::string, std::string> eurex_shortcode_to_exchange_symbol_;

  std::map<uint8_t, std::string> cme_product_code_to_shortcode_;
  std::map<std::string, std::string> cme_shortcode_to_exchange_symbol_;

  EUREX_MDS::EUREXCommonStruct eurex_cstr_;
  CME_MDS::CMECommonStruct cme_cstr_;

 public:
  LiveMDSLogger(std::string exch_, bool set_time_)
      : mdsLogger(new MDSLogger<HFSAT::MDS_MSG::GenericMDSMessage>(exch_)),
        set_time(set_time_),
        generic_mds_shm_key_(GENERIC_MDS_SHM_KEY),
        generic_mds_shmid_(-1),
        generic_mds_shm_struct_(NULL),
        index_(-1),
        eurex_product_code_to_shortcode_(),
        eurex_shortcode_to_exchange_symbol_(),
        cme_product_code_to_shortcode_(),
        cme_shortcode_to_exchange_symbol_()

  {
    HFSAT::Utils::LowBWCodeMappingLoader::LoadMappingForGivenFile(
        DEF_LS_PRODUCTCODE_SHORTCODE_, eurex_product_code_to_shortcode_, eurex_shortcode_to_exchange_symbol_);
    HFSAT::Utils::LowBWCodeMappingLoader::LoadMappingForGivenFile(
        DEF_CME_LS_PRODUCTCODE_SHORTCODE_, cme_product_code_to_shortcode_, cme_shortcode_to_exchange_symbol_);

    if ((generic_mds_shmid_ =
             shmget(generic_mds_shm_key_,
                    (size_t)(GENERIC_MDS_QUEUE_SIZE * (sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) + sizeof(int)),
                    IPC_CREAT | 0666)) < 0) {
      if (errno == EINVAL)
        std::cerr << "Invalid segment size specified \n";

      else if (errno == EEXIST)
        std::cerr << "Segment already exists \n";

      else if (errno == EIDRM)
        std::cerr << " Segment is marked for deletion \n";

      else if (errno == ENOENT)
        std::cerr << " Segment Doesn't Exist \n";

      else if (errno == EACCES)
        std::cerr << " Permission Denied \n";

      else if (errno == ENOMEM)
        std::cerr << " Not Enough Memory To Create Shm Segment \n";

      exit(1);
    }

    if ((generic_mds_shm_struct_ = (volatile HFSAT::MDS_MSG::GenericMDSMessage *)shmat(generic_mds_shmid_, NULL, 0)) ==
        (volatile HFSAT::MDS_MSG::GenericMDSMessage *)-1) {
      perror("shmat failed");
      exit(0);
    }

    if (shmctl(generic_mds_shmid_, IPC_STAT, &generic_shmid_ds_) == -1) {
      perror("shmctl");
      exit(1);
    }

    if (generic_shmid_ds_.shm_nattch == 1) {
      memset((void *)((HFSAT::MDS_MSG::GenericMDSMessage *)generic_mds_shm_struct_), 0,
             (GENERIC_MDS_QUEUE_SIZE * sizeof(HFSAT::MDS_MSG::GenericMDSMessage) + sizeof(int)));
    }

    mdsLogger->run();
  }

  ~LiveMDSLogger() {
    mdsLogger->closeFiles();

    shmdt((void *)generic_mds_shm_struct_);

    if (shmctl(generic_mds_shmid_, IPC_STAT, &generic_shmid_ds_) == -1) {
      perror("shmctl");
      exit(1);
    }

    if (generic_shmid_ds_.shm_nattch == 0)
      shmctl(generic_mds_shmid_, IPC_RMID, 0);  // remove shm segment if no users are attached
  }

  void runLogger() {
    HFSAT::MDS_MSG::GenericMDSMessage cstr_;

    memset((void *)&cstr_, 0, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
    // unsigned int last_seen_sequence_ = 0 ;

    while (true) {
      // has to be volatile, waiting on shared memory segment queue position
      volatile int queue_position_ = *((int *)(generic_mds_shm_struct_ + GENERIC_MDS_QUEUE_SIZE));

      // events are available only if the queue position at source is higher by 1, circular queue, will lag behind by 1
      // packet
      if (index_ == -1) {
        index_ = queue_position_;
      }

      if (index_ == queue_position_) {
        continue;
      }

      index_ = (index_ + 1) & (GENERIC_MDS_QUEUE_SIZE - 1);

      // memcpy is only done as safegaurd from writer writing the same segment
      memcpy(&cstr_, (HFSAT::MDS_MSG::GenericMDSMessage *)(generic_mds_shm_struct_ + index_),
             sizeof(HFSAT::MDS_MSG::GenericMDSMessage));

      if (cstr_.mds_msg_exch_ == HFSAT::MDS_MSG::CME_LS) {
        HFSAT::Utils::LowBWCodeMappingLoader::GetCMEStructFromLowBWStruct(cme_product_code_to_shortcode_,
                                                                          cme_shortcode_to_exchange_symbol_, cme_cstr_,
                                                                          cstr_.generic_data_.cme_ls_data_);

        cme_cstr_.time_ = cstr_.time_;

        cstr_.mds_msg_exch_ = HFSAT::MDS_MSG::CME;
        memcpy(&(cstr_.generic_data_.cme_data_), &(cme_cstr_), sizeof(CME_MDS::CMECommonStruct));

      } else if (cstr_.mds_msg_exch_ == HFSAT::MDS_MSG::EUREX_LS) {
        HFSAT::Utils::LowBWCodeMappingLoader::GetEurexStructFromLowBWStruct(
            eurex_product_code_to_shortcode_, eurex_shortcode_to_exchange_symbol_, eurex_cstr_,
            cstr_.generic_data_.eurex_ls_data_);

        eurex_cstr_.time_ = cstr_.time_;

        cstr_.mds_msg_exch_ = HFSAT::MDS_MSG::EUREX;
        memcpy(&(cstr_.generic_data_.eurex_data_), &(eurex_cstr_), sizeof(EUREX_MDS::EUREXCommonStruct));

      } else if (cstr_.mds_msg_exch_ == HFSAT::MDS_MSG::EOBI_LS) {
        HFSAT::Utils::LowBWCodeMappingLoader::GetEurexStructFromLowBWStruct(
            eurex_product_code_to_shortcode_, eurex_shortcode_to_exchange_symbol_, eurex_cstr_,
            cstr_.generic_data_.eobi_ls_data_);

        eurex_cstr_.time_ = cstr_.time_;

        cstr_.mds_msg_exch_ = HFSAT::MDS_MSG::EUREX;
        memcpy(&(cstr_.generic_data_.eurex_data_), &(eurex_cstr_), sizeof(EUREX_MDS::EUREXCommonStruct));

      } else if (cstr_.mds_msg_exch_ == HFSAT::MDS_MSG::LIFFE_LS) {
        cstr_.generic_data_.liffe_data_.time_ = cstr_.time_;

      } else if (cstr_.mds_msg_exch_ == HFSAT::MDS_MSG::CHIX_L1) {
        cstr_.generic_data_.chix_l1_data_.time_ = cstr_.time_;

      } else if (HFSAT::MDS_MSG::OSE_PF == cstr_.mds_msg_exch_) {
        cstr_.generic_data_.ose_pf_data_.time_ = cstr_.time_;

      } else if (HFSAT::MDS_MSG::HKEX == cstr_.mds_msg_exch_) {
        cstr_.generic_data_.hkex_data_.time_ = cstr_.time_;

      } else if (HFSAT::MDS_MSG::OSE_L1 == cstr_.mds_msg_exch_) {
        cstr_.generic_data_.ose_l1_data_.time_ = cstr_.time_;

      } else if (HFSAT::MDS_MSG::OSE_CF == cstr_.mds_msg_exch_) {
        cstr_.generic_data_.ose_cf_data_.time_ = cstr_.time_;

      } else if (HFSAT::MDS_MSG::ICE == cstr_.mds_msg_exch_) {
        // cstr_.generic_data_.ice_data_.time_ = cstr_.time_ ;

      } else if (HFSAT::MDS_MSG::HKOMDPF == cstr_.mds_msg_exch_) {
        cstr_.generic_data_.hkomd_pf_data_.time_ = cstr_.time_;
      } else if (HFSAT::MDS_MSG::AFLASH == cstr_.mds_msg_exch_) {
        cstr_.generic_data_.aflash_data_.time_ = cstr_.time_;
      }

      mdsLogger->log(cstr_);
    }
  }

  void closeFiles() { mdsLogger->closeFiles(); }
};

void *logger;

/// signal handling
void sighandler(int signum) {
  std::cerr << " Received Termination Signal \n";

  if (logger != NULL) {
    std::cerr << " Closing Files Called \n";
    ((LiveMDSLogger *)logger)->closeFiles();
  } else {
    std::cerr << " Logger Handle NULL \n";
  }
  exit(0);
}

int main(int argc, char **argv) {
  /// set signal handler .. add other signals later
  struct sigaction sigact;
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);

  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);

  if (std::string(hostname).find("sdv-ind") != std::string::npos) {
    HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());
  }

  logger = (void *)(new LiveMDSLogger("GENERIC", true));
  ((LiveMDSLogger *)logger)->runLogger();

  return 0;
}
