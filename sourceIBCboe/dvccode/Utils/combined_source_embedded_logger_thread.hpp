// =====================================================================================
//
//       Filename:  combined_source_embedded_logger_thread.hpp
//
//    Description:  A Utility Class to Log the Combined Source Data Which is Embedded Into the Writer Itself, The class
//    continuos to read from the shm to keep track of SHM issues.
//
//        Version:  1.0
//        Created:  11/05/2014 12:53:58 PM
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

#pragma once

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
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/Utils/mds_shm_interface.hpp"
#include "dvccode/Utils/load_low_bandwidth_code_mapping.hpp"
#include "dvccode/CombinedControlUtils/combined_control_message_listener.hpp"
#include "dvccode/Utils/mds_logger.hpp"

namespace HFSAT {
namespace Utils {

// The Class Is A Thread Since This Will A Offline Class Having Main Purpose As Just to Read Data From Shm And Another
// Thread Keeps Writing To The Files
class CombinedSourceEmbeddedLoggerThread : public HFSAT::Thread {
 private:
  // The Thread Which is Actually Writing To The Files
  MDSLogger<HFSAT::MDS_MSG::GenericMDSMessage> *files_logging_thread_;
  key_t generic_mds_shm_key_;
  int32_t generic_mds_shmid_;

  volatile HFSAT::MDS_MSG::GenericMDSMessage *generic_mds_shm_struct_;
  struct shmid_ds generic_shmid_ds_;

  int32_t index_;

  // These Are Specific Handling WHich We Need to Convert the Low Bandwidth Structs Sent Over Multicast to The Original
  // Common Struct Form For the Compatibility Reasons
  std::map<uint8_t, std::string> eurex_product_code_to_shortcode_;
  std::map<std::string, std::string> eurex_shortcode_to_exchange_symbol_;

  std::map<uint8_t, std::string> cme_product_code_to_shortcode_;
  std::map<std::string, std::string> cme_shortcode_to_exchange_symbol_;

  EUREX_MDS::EUREXCommonStruct eurex_cstr_;
  CME_MDS::CMECommonStruct cme_cstr_;

 public:
  CombinedSourceEmbeddedLoggerThread(std::set<HFSAT::ExchSource_t> *_list_of_exch_sources_,
                                     HFSAT::CombinedControlMessageLiveSource *p_cmd_control_live_source_)
      :

        files_logging_thread_(new MDSLogger<HFSAT::MDS_MSG::GenericMDSMessage>(
            "GENERIC", _list_of_exch_sources_)),  // ALways Going to Be GENERIC - The Directory under
                                                  // /spare/local/MDSlogs/ where logs are stored
        generic_mds_shm_key_(GENERIC_MDS_SHM_KEY),
        generic_mds_shmid_(-1),
        generic_mds_shm_struct_(NULL),
        index_(-1),
        eurex_product_code_to_shortcode_(),
        eurex_shortcode_to_exchange_symbol_(),
        cme_product_code_to_shortcode_(),
        cme_shortcode_to_exchange_symbol_(),
        eurex_cstr_(),
        cme_cstr_()

  {
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);

    if (std::string(hostname).find("sdv-ind") != std::string::npos) {
      HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());
    }

    // Load the Simple Utility Class Which Know How to Convert the Low Bandwidth Struct To The Original Compatible
    // Formaat
    HFSAT::Utils::LowBWCodeMappingLoader::LoadMappingForGivenFile(
        DEF_LS_PRODUCTCODE_SHORTCODE_, eurex_product_code_to_shortcode_, eurex_shortcode_to_exchange_symbol_);
    HFSAT::Utils::LowBWCodeMappingLoader::LoadMappingForGivenFile(
        DEF_CME_LS_PRODUCTCODE_SHORTCODE_, cme_product_code_to_shortcode_, cme_shortcode_to_exchange_symbol_);

    // SHM Segment Join Related Work
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

    if (p_cmd_control_live_source_) {
      p_cmd_control_live_source_->AddCombinedControlMessageListener(files_logging_thread_);
    }

    files_logging_thread_->run();
  }

  ~CombinedSourceEmbeddedLoggerThread() {
    // ENsure All the Data is flushed before we exit since the Logging is not immediate to the files to reduce I/O calls
    files_logging_thread_->closeFiles();
  }

  void thread_main() {
    AffinInitCores();  // Make Sure The Thread is Assigned to init cores, since it's not a latency sensitive task, The
                       // Time is also set by the writer

    HFSAT::MDS_MSG::GenericMDSMessage cstr_;

    memset((void *)&cstr_, 0, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));

    while (true) {  // Sit Tight For Signal, Until Then keep polling shm segment for events

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

      // memcpy is only done as safegaurd from writer writing the same segment, At this point it's important to know
      // which struct this is, to modify timestamp, convert Low bandwidth to compatible forms etc,
      memcpy(&cstr_, (HFSAT::MDS_MSG::GenericMDSMessage *)(generic_mds_shm_struct_ + index_),
             sizeof(HFSAT::MDS_MSG::GenericMDSMessage));

      if (cstr_.mds_msg_exch_ == HFSAT::MDS_MSG::NSE) {
        continue;
        // Req Based Combined Writer. Already being written from nse processor.
      }

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

      } else if (HFSAT::MDS_MSG::ICE_LS == cstr_.mds_msg_exch_) {
        cstr_.generic_data_.ice_data_.time_ = cstr_.time_;
      } else if (HFSAT::MDS_MSG::ICE_CF == cstr_.mds_msg_exch_) {
        cstr_.generic_data_.ice_cf_data_.time_ = cstr_.time_;
      } else if (HFSAT::MDS_MSG::HKOMDPF == cstr_.mds_msg_exch_) {
        cstr_.generic_data_.hkomd_pf_data_.time_ = cstr_.time_;
      } else if (HFSAT::MDS_MSG::AFLASH == cstr_.mds_msg_exch_) {
        cstr_.generic_data_.aflash_data_.time_ = cstr_.time_;
      } else if (HFSAT::MDS_MSG::NTP_LS == cstr_.mds_msg_exch_) {
        cstr_.generic_data_.ntp_data_.time_ = cstr_.time_;
      }

      files_logging_thread_->log(cstr_);  // Again This is Just a DataCopy call via a lockfree queue, the logger thread
                                          // will pick from the queue and write it to a file
    }
  }

  // To Ensure Data is Not Lost When The Signal is Triggered
  void closeFiles() {
    if (NULL != files_logging_thread_) {
      files_logging_thread_->closeFiles();
    }
  }
};
}
}
