// =====================================================================================
//
//       Filename:  combined_mds_messages_shm_processor.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/30/2014 12:39:20 PM
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
#include <stdint.h>

#include <iostream>
#include <fstream>

#include <set>

#include "dvccode/CDef/mds_messages.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_cme_fpga_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_shm_base.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_cme_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_eurex_ls_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_liffe_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_eobi_ls_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_control_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_ors_reply_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_rts_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_rts_of_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_micex_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_cme_ls_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_ntp_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_eobi_pf_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_ose_pf_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_cfe_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_ose_l1_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_ose_cf_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_tmx_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_ice_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_asx_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_ose_itch_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_hkomdpf_processor.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_bmfeq_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_aflash_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_retail_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_nse_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_nse_l1_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_eobi_compact_of_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_tmx_obf_processor.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_sgx_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_tmx_obf_of_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_ose_itch_of_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_bmf_fpga_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_micex_of_processor.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#define CCPROFILING_TRADEINIT 1

namespace HFSAT {
namespace MDSMessages {

class CombinedMDSMessagesShmProcessor : public HFSAT::MDSMessages::CombinedMDSMessagesShmBase,
                                        public HFSAT::SimpleExternalDataLiveListener {
 private:
  HFSAT::FastMdConsumerMode_t processing_mode_;
  std::set<HFSAT::MDS_MSG::MDSMessageExchType> processor_added_;
  std::set<HFSAT::MDS_MSG::MDSMessageExchType> fpga_processor_added_;

  HFSAT::DebugLogger &dbglogger_;
  HFSAT::SecurityNameIndexer &sec_name_indexer_;

  // this is meant to be a local copy to track queue position, compare with volatile shared memory counter
  int64_t index_;
  int64_t ors_reply_index_;
  bool keep_me_running_;

  HFSAT::MDS_MSG::GenericMDSMessage generic_mds_message_;
  uint32_t generic_mds_message_pkt_size_;

  ProfilerTimeInfo time_info_;

  HFSAT::CombinedMDSMessagesCMEProcessor *cme_mds_processor_;
  HFSAT::CombinedMDSMessagesCMEFPGAProcessor *cme_mds_fpga_processor_;
  HFSAT::CombinedMDSMessagesEUREXLSProcessor *eurex_ls_mds_processor_;
  HFSAT::CombinedMDSMessagesEOBILSProcessor *eobi_ls_mds_processor_;
  HFSAT::CombinedMDSMessagesLIFFEProcessor *liffe_mds_processor_;
  HFSAT::CombinedMDSMessagesControlProcessor *control_processor_;
  HFSAT::CombinedMDSMessagesORSReplyProcessor *ors_reply_processor_;
  HFSAT::CombinedMDSMessagesRTSProcessor *rts_mds_processor_;
  HFSAT::CombinedMDSMessagesRTSOFProcessor *rts_mds_of_processor_;
  HFSAT::CombinedMDSMessagesMICEXProcessor *micex_mds_processor_;
  HFSAT::CombinedMDSMessagesCMELSProcessor *cme_ls_mds_processor_;
  HFSAT::CombinedMDSMessagesNTPProcessor *ntp_mds_processor_;
  HFSAT::CombinedMDSMessagesEOBIPFProcessor *eobi_pf_mds_processor_;
  HFSAT::CombinedMDSMessagesOSEPFProcessor *ose_pf_mds_processor_;
  HFSAT::CombinedMDSMessagesCFEProcessor *cfe_mds_processor_;
  HFSAT::CombinedMDSMessagesOSEL1Processor *ose_l1_mds_processor_;
  HFSAT::CombinedMDSMessagesOSECFProcessor *ose_cf_mds_processor_;
  HFSAT::CombinedMDSMessagesTMXProcessor *tmx_mds_processor_;
  HFSAT::CombinedMDSMessagesICEProcessor *ice_mds_processor_;
  HFSAT::CombinedMDSMessagesASXProcessor *asx_mds_processor_;
  HFSAT::CombinedMDSMessagesSGXProcessor *sgx_mds_processor_;
  HFSAT::CombinedMDSMessagesOSEITCHProcessor *ose_itch_mds_processor_;
  HFSAT::CombinedMDSMessagesTMXOBFProcessor *tmx_obf_mds_processor_;
  HFSAT::CombinedMDSMessagesHKOMDPFProcessor *hkomdpf_mds_processor_;
  HFSAT::CombinedMDSMessagesBMFEQProcessor *bmfeq_mds_processor_;
  HFSAT::CombinedMDSMessagesAflashProcessor *aflash_mds_processor_;
  HFSAT::CombinedMDSMessagesRetailProcessor *retail_mds_processor_;
  HFSAT::CombinedMDSMessagesNSEProcessor *nse_mds_processor_;
  HFSAT::CombinedMDSMessagesNSEL1Processor *nse_l1_mds_processor_;
  HFSAT::CombinedMDSMessagesEOBICompactOFProcessor *eobi_compact_of_mds_processor_;
  // book building modification purpose
  HFSAT::CombinedMDSMessagesTMXOBFOFProcessor *tmx_obf_of_mds_processor_;
  HFSAT::CombinedMDSMessagesOSEItchOFProcessor *ose_itch_of_mds_processor_;
  HFSAT::CombinedMDSMessagesBMFFPGAProcessor *bmf_mds_fpga_processor_;
  HFSAT::CombinedMDSMessagesMICEXOFOFProcessor *micex_cr_of_mds_processor_;

  bool fpga_added_;

 public:
  CombinedMDSMessagesShmProcessor(HFSAT::DebugLogger &_dgblogger_, HFSAT::SecurityNameIndexer &_sec_name_indexer_,
                                  HFSAT::FastMdConsumerMode_t _mode_)
      : CombinedMDSMessagesShmBase(),
        processing_mode_(_mode_),
        dbglogger_(_dgblogger_),
        sec_name_indexer_(_sec_name_indexer_),
        index_(-1),
        ors_reply_index_(-1),
        keep_me_running_(false),
        generic_mds_message_(),
        generic_mds_message_pkt_size_(sizeof(HFSAT::MDS_MSG::GenericMDSMessage)),
        cme_mds_processor_(NULL),
        cme_mds_fpga_processor_(NULL),
        eurex_ls_mds_processor_(NULL),
        eobi_ls_mds_processor_(NULL),
        liffe_mds_processor_(NULL),
        control_processor_(NULL),
        ors_reply_processor_(NULL),
        rts_mds_processor_(NULL),
        rts_mds_of_processor_(NULL),
        micex_mds_processor_(NULL),
        cme_ls_mds_processor_(NULL),
        ntp_mds_processor_(NULL),
        eobi_pf_mds_processor_(NULL),
        ose_pf_mds_processor_(NULL),
        cfe_mds_processor_(NULL),
        ose_l1_mds_processor_(NULL),
        ose_cf_mds_processor_(NULL),
        tmx_mds_processor_(NULL),
        ice_mds_processor_(NULL),
        asx_mds_processor_(NULL),
        sgx_mds_processor_(NULL),
        ose_itch_mds_processor_(NULL),
        tmx_obf_mds_processor_(NULL),
        hkomdpf_mds_processor_(NULL),
        bmfeq_mds_processor_(NULL),
        aflash_mds_processor_(NULL),
        retail_mds_processor_(NULL),
        nse_mds_processor_(NULL),
        nse_l1_mds_processor_(NULL),
        eobi_compact_of_mds_processor_(NULL),
        // book building modification purpose
        tmx_obf_of_mds_processor_(NULL),
        ose_itch_of_mds_processor_(NULL),
        bmf_mds_fpga_processor_(NULL),
        micex_cr_of_mds_processor_(NULL),
        fpga_added_(false) {}

  ~CombinedMDSMessagesShmProcessor() {}

  inline void AddControlSourceForProcessing(HFSAT::MDS_MSG::MDSMessageExchType _source_, const int trader_id_,
                                            HFSAT::ControlMessageListener *control_listener_,
                                            ExternalTimeListener *_p_time_keeper_) {
    if (_source_ != HFSAT::MDS_MSG::CONTROL) {
      std::cerr << " Only Control Listeners Can be Added BY this Call, Ignoring \n";
    }

    if (control_processor_ == NULL) {  // Create Source if doesn't exist
      processor_added_.insert(_source_);
      control_processor_ = new HFSAT::CombinedMDSMessagesControlProcessor(dbglogger_);
    }

    control_processor_->AddControlMessageListener(trader_id_, control_listener_);
    control_processor_->SetExternalTimeListener(_p_time_keeper_);
  }

  //@Note : This has a special argument in the form of SMV pointers list in order to be aware of the live mkt prices
  inline void AddORSreplySourceForProcessing(HFSAT::MDS_MSG::MDSMessageExchType _source_,
                                             ExternalTimeListener *_p_time_keeper_,
                                             HFSAT::SecurityMarketViewPtrVec &_sid_to_smv_ptr_map_) {
    if (_source_ != HFSAT::MDS_MSG::ORS_REPLY) {
      std::cerr << " Only ORS Reply Listeners Can be Added BY this Call, Ignoring \n";
    }

    if (ors_reply_processor_ == NULL) {
      processor_added_.insert(_source_);
      ors_reply_processor_ =
          new HFSAT::CombinedMDSMessagesORSReplyProcessor(dbglogger_, sec_name_indexer_, _sid_to_smv_ptr_map_);
      ors_reply_processor_->SetExternalTimeListener(_p_time_keeper_);
    }
  }

  void CleanUp() { keep_me_running_ = false; }

  inline CombinedMDSMessagesORSReplyProcessor *GetORSReplyProcessor() {
    if (ors_reply_processor_ == NULL) {
      std::cerr << "GetORSReplyProcessor called before initializing the ors reply processor \n";
      exit(-1);
    }

    return ors_reply_processor_;
  }

  inline void ExcludeQuincySourceInstrumentsFromCME() {}

  inline void AddFPGADataSourceForProcessing(HFSAT::MDS_MSG::MDSMessageExchType _source_, void *_p_book_listener_,
                                             ExternalTimeListener *_p_time_keeper_) {
    // Check if the FPGA source already added or not
    if (fpga_processor_added_.find(_source_) != fpga_processor_added_.end()) return;

    // Add this FPGA processor for callbacks
    fpga_processor_added_.insert(_source_);
    switch (_source_) {
      case HFSAT::MDS_MSG::CME: {
        cme_mds_fpga_processor_ = new HFSAT::CombinedMDSMessagesCMEFPGAProcessor(dbglogger_, sec_name_indexer_);
        cme_mds_fpga_processor_->SetFPGAGlobalListener((FPGAHalfBookGlobalListener *)(_p_book_listener_));
        cme_mds_fpga_processor_->SetExternalTimeListener(_p_time_keeper_);
        fpga_added_ = true;
      } break;
      case HFSAT::MDS_MSG::NTP: {
        bmf_mds_fpga_processor_ = new HFSAT::CombinedMDSMessagesBMFFPGAProcessor(dbglogger_, sec_name_indexer_);
        bmf_mds_fpga_processor_->SetFPGAGlobalListener((BMFFPGAFullBookGlobalListener *)(_p_book_listener_));
        bmf_mds_fpga_processor_->SetExternalTimeListener(_p_time_keeper_);
        fpga_added_ = true;
      } break;
      default: { } break; }
  }

  inline void AddDataSourceForProcessing(HFSAT::MDS_MSG::MDSMessageExchType _source_, void *_p_book_listener_,
                                         ExternalTimeListener *_p_time_keeper_,
                                         HFSAT::HybridSecurityManager *_hybrid_security_manager_ = NULL) {
    // Check if the source already added or not
    if (processor_added_.find(_source_) != processor_added_.end()) return;

    // Add this processor for callbacks
    processor_added_.insert(_source_);

    switch (_source_) {
      case HFSAT::MDS_MSG::CME: {
        if (cme_mds_processor_ == NULL) {
          cme_mds_processor_ = new HFSAT::CombinedMDSMessagesCMEProcessor(dbglogger_, sec_name_indexer_);
          cme_mds_processor_->SetPriceLevelGlobalListener((PriceLevelGlobalListener *)(_p_book_listener_));
          cme_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }

      } break;

      case HFSAT::MDS_MSG::LIFFE:
      case HFSAT::MDS_MSG::LIFFE_LS: {
        if (liffe_mds_processor_ == NULL) {
          liffe_mds_processor_ = new HFSAT::CombinedMDSMessagesLIFFEProcessor(dbglogger_, sec_name_indexer_);
          liffe_mds_processor_->SetPriceLevelGlobalListener((PriceLevelGlobalListener *)(_p_book_listener_));
          liffe_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }

      } break;

      case HFSAT::MDS_MSG::EUREX_LS: {
        if (eurex_ls_mds_processor_ == NULL) {
          eurex_ls_mds_processor_ = new HFSAT::CombinedMDSMessagesEUREXLSProcessor(dbglogger_, sec_name_indexer_);
          eurex_ls_mds_processor_->SetPriceLevelGlobalListener((PriceLevelGlobalListener *)_p_book_listener_);
          eurex_ls_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }

      } break;

      case HFSAT::MDS_MSG::EOBI_LS: {
        if (eobi_ls_mds_processor_ == NULL) {
          eobi_ls_mds_processor_ =
              new HFSAT::CombinedMDSMessagesEOBILSProcessor(dbglogger_, sec_name_indexer_, _hybrid_security_manager_);
          eobi_ls_mds_processor_->SetPriceLevelGlobalListener((PriceLevelGlobalListener *)_p_book_listener_);
          eobi_ls_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }

      } break;

      case HFSAT::MDS_MSG::RTS: {
        if (rts_mds_processor_ == NULL) {
          rts_mds_processor_ = new HFSAT::CombinedMDSMessagesRTSProcessor(dbglogger_, sec_name_indexer_);
          rts_mds_processor_->SetPriceLevelGlobalListener((PriceLevelGlobalListener *)(_p_book_listener_));
          rts_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }

      } break;
      case HFSAT::MDS_MSG::RTS_OF: {
        if (rts_mds_of_processor_ == NULL) {
          rts_mds_of_processor_ = new HFSAT::CombinedMDSMessagesRTSOFProcessor(dbglogger_, sec_name_indexer_);
          rts_mds_of_processor_->SetOrderfeedListener(
              (OrderGlobalListener<RTS_MDS::RTSOFCommonStructv2> *)(_p_book_listener_));
          rts_mds_of_processor_->SetExternalTimeListener(_p_time_keeper_);
        }

      } break;

      // Initialize only 1 processor for MICEX CR and EQ

      case HFSAT::MDS_MSG::MICEX: {
        if (micex_mds_processor_ == NULL) {
          micex_mds_processor_ = new HFSAT::CombinedMDSMessagesMICEXProcessor(dbglogger_, sec_name_indexer_);
          micex_mds_processor_->SetPriceLevelGlobalListener((PriceLevelGlobalListener *)(_p_book_listener_));
          micex_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }

      } break;

      case HFSAT::MDS_MSG::MICEX_OF: {
        if (micex_cr_of_mds_processor_ == NULL) {
          micex_cr_of_mds_processor_ = new HFSAT::CombinedMDSMessagesMICEXOFOFProcessor(dbglogger_, sec_name_indexer_);
          micex_cr_of_mds_processor_->SetOrderfeedListener(
              (OrderGlobalListener<MICEX_OF_MDS::MICEXOFCommonStruct> *)(_p_book_listener_));
          micex_cr_of_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }

      } break;

      case HFSAT::MDS_MSG::CME_LS: {
        if (cme_ls_mds_processor_ == NULL) {
          cme_ls_mds_processor_ =
              new HFSAT::CombinedMDSMessagesCMELSProcessor(dbglogger_, sec_name_indexer_, _hybrid_security_manager_);
          cme_ls_mds_processor_->SetPriceLevelGlobalListener((PriceLevelGlobalListener *)_p_book_listener_);
          cme_ls_mds_processor_->SetExternalTimeListener(_p_time_keeper_);

          ExcludeQuincySourceInstrumentsFromCME();
        }

      } break;

      case HFSAT::MDS_MSG::NTP:
      case HFSAT::MDS_MSG::NTP_LS: {
        if (ntp_mds_processor_ == NULL) {
          ntp_mds_processor_ = new HFSAT::CombinedMDSMessagesNTPProcessor(dbglogger_, sec_name_indexer_);
          ntp_mds_processor_->SetPriceLevelGlobalListener((NTPPriceLevelGlobalListener *)(_p_book_listener_));
          ntp_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }

      } break;

      case HFSAT::MDS_MSG::BMF_EQ: {
        if (bmfeq_mds_processor_ == NULL) {
          bmfeq_mds_processor_ = new HFSAT::CombinedMDSMessagesBMFEQProcessor(dbglogger_, sec_name_indexer_);
          bmfeq_mds_processor_->SetPriceLevelGlobalListener((NTPPriceLevelGlobalListener *)(_p_book_listener_));
          bmfeq_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }
      } break;
      case HFSAT::MDS_MSG::EOBI_PF: {
        if (eobi_pf_mds_processor_ == NULL) {
          eobi_pf_mds_processor_ = new HFSAT::CombinedMDSMessagesEOBIPFProcessor(dbglogger_, sec_name_indexer_);
          eobi_pf_mds_processor_->SetPriceLevelGlobalListener((PriceLevelGlobalListener *)(_p_book_listener_));
          eobi_pf_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }

      } break;

      case HFSAT::MDS_MSG::OSE_PF: {
        if (ose_pf_mds_processor_ == NULL) {
          ose_pf_mds_processor_ = new HFSAT::CombinedMDSMessagesOSEPFProcessor(dbglogger_, sec_name_indexer_);
          ose_pf_mds_processor_->SetPriceLevelGlobalListener((PriceLevelGlobalListener *)_p_book_listener_);
          ose_pf_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }

      } break;

      case HFSAT::MDS_MSG::OSE_CF: {
        if (ose_cf_mds_processor_ == NULL) {
          ose_cf_mds_processor_ = new HFSAT::CombinedMDSMessagesOSECFProcessor(dbglogger_, sec_name_indexer_);
          ose_cf_mds_processor_->SetPriceLevelGlobalListener((PriceLevelGlobalListener *)_p_book_listener_);
          ose_cf_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }

      } break;

      case HFSAT::MDS_MSG::CSM: {
        if (cfe_mds_processor_ == NULL) {
          cfe_mds_processor_ = new HFSAT::CombinedMDSMessagesCFEProcessor(dbglogger_, sec_name_indexer_);
          cfe_mds_processor_->SetCFEPriceLevelGlobalListener((CFEPriceLevelGlobalListener *)(_p_book_listener_));
          cfe_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }

      } break;

      case HFSAT::MDS_MSG::OSE_L1: {
        if (ose_l1_mds_processor_ == NULL) {
          ose_l1_mds_processor_ = new HFSAT::CombinedMDSMessagesOSEL1Processor(dbglogger_, sec_name_indexer_);
          ose_l1_mds_processor_->SetFullBookGlobalListener((FullBookGlobalListener *)_p_book_listener_);
          ose_l1_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }

      } break;

      case HFSAT::MDS_MSG::TMX:
      case HFSAT::MDS_MSG::TMX_LS: {
        if (tmx_mds_processor_ == NULL) {
          tmx_mds_processor_ = new HFSAT::CombinedMDSMessagesTMXProcessor(dbglogger_, sec_name_indexer_);
          tmx_mds_processor_->SetFullBookGlobalListener((FullBookGlobalListener *)_p_book_listener_);
          tmx_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }
      } break;

      case HFSAT::MDS_MSG::TMX_OBF: {
        if (tmx_obf_mds_processor_ == NULL) {
          tmx_obf_mds_processor_ = new HFSAT::CombinedMDSMessagesTMXOBFProcessor(dbglogger_, sec_name_indexer_);
          tmx_obf_mds_processor_->SetPriceLevelGlobalListener((PriceLevelGlobalListener *)_p_book_listener_);
          tmx_obf_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }
      } break;

      // book building modification purpose
      case HFSAT::MDS_MSG::TMX_OBF_OF: {
        if (tmx_obf_of_mds_processor_ == NULL) {
          tmx_obf_of_mds_processor_ = new HFSAT::CombinedMDSMessagesTMXOBFOFProcessor(dbglogger_, sec_name_indexer_);
          tmx_obf_of_mds_processor_->SetPriceLevelGlobalListener((OrderFeedGlobalListener *)_p_book_listener_);
          tmx_obf_of_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }
      } break;

      case HFSAT::MDS_MSG::ICE:
      case HFSAT::MDS_MSG::ICE_LS: {
        if (ice_mds_processor_ == NULL) {
          ice_mds_processor_ = new HFSAT::CombinedMDSMessagesICEProcessor(dbglogger_, sec_name_indexer_);
          ice_mds_processor_->SetPriceLevelGlobalListener((PriceLevelGlobalListener *)_p_book_listener_);
          ice_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }
      } break;
      case HFSAT::MDS_MSG::ASX: {
        if (asx_mds_processor_ == NULL) {
          asx_mds_processor_ = new HFSAT::CombinedMDSMessagesASXProcessor(dbglogger_, sec_name_indexer_);
          asx_mds_processor_->SetPriceLevelGlobalListener((PriceLevelGlobalListener *)_p_book_listener_);
          asx_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }
      } break;
      case HFSAT::MDS_MSG::SGX: {
        if (sgx_mds_processor_ == NULL) {
          sgx_mds_processor_ = new HFSAT::CombinedMDSMessagesSGXProcessor(dbglogger_, sec_name_indexer_);
          sgx_mds_processor_->SetPriceLevelGlobalListener((PriceLevelGlobalListener *)_p_book_listener_);
          sgx_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }
      } break;

      case HFSAT::MDS_MSG::OSE_ITCH_PF: {
        if (ose_itch_mds_processor_ == NULL) {
          ose_itch_mds_processor_ = new HFSAT::CombinedMDSMessagesOSEITCHProcessor(dbglogger_, sec_name_indexer_);
          ose_itch_mds_processor_->SetPriceLevelGlobalListener((PriceLevelGlobalListener *)_p_book_listener_);
          ose_itch_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }
      } break;

      case HFSAT::MDS_MSG::OSE_ITCH_OF: {
        if (ose_itch_of_mds_processor_ == NULL) {
          ose_itch_of_mds_processor_ = new HFSAT::CombinedMDSMessagesOSEItchOFProcessor(dbglogger_, sec_name_indexer_);
          ose_itch_of_mds_processor_->SetOrderLevelGlobalListener((OrderFeedGlobalListener *)_p_book_listener_);
          ose_itch_of_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }
      } break;

      case HFSAT::MDS_MSG::HKOMDPF: {
        if (hkomdpf_mds_processor_ == NULL) {
          hkomdpf_mds_processor_ = new HFSAT::CombinedMDSMessagesHKOMDPFProcessor(dbglogger_, sec_name_indexer_);
          hkomdpf_mds_processor_->SetPriceLevelGlobalListener((PriceLevelGlobalListener *)_p_book_listener_);
          hkomdpf_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }
      } break;
      case HFSAT::MDS_MSG::AFLASH: {
        if (aflash_mds_processor_ == NULL) {
          aflash_mds_processor_ = HFSAT::CombinedMDSMessagesAflashProcessor::GetUniqueInstance(dbglogger_);
          aflash_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
        }
      } break;
      case HFSAT::MDS_MSG::RETAIL: {
        retail_mds_processor_ =
            HFSAT::CombinedMDSMessagesRetailProcessor::GetUniqueInstance(dbglogger_, _p_time_keeper_);
      } break;
      case HFSAT::MDS_MSG::NSE: {
        nse_mds_processor_ = new HFSAT::CombinedMDSMessagesNSEProcessor(dbglogger_, sec_name_indexer_);
        nse_mds_processor_->SetOrderGlobalListenerNSE((OrderGlobalListenerNSE *)_p_book_listener_);
        nse_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
      } break;
      case HFSAT::MDS_MSG::NSE_L1: {
        nse_l1_mds_processor_ = new HFSAT::CombinedMDSMessagesNSEL1Processor(dbglogger_, sec_name_indexer_);
        nse_l1_mds_processor_->SetL1DataListener((L1DataListener *)_p_book_listener_);
        nse_l1_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
      } break;
      case HFSAT::MDS_MSG::EOBI_OF: {
        eobi_compact_of_mds_processor_ =
            new HFSAT::CombinedMDSMessagesEOBICompactOFProcessor(dbglogger_, sec_name_indexer_);
        eobi_compact_of_mds_processor_->SetEOBIOrderLevelGlobalListener((OrderGlobalListenerEOBI *)_p_book_listener_);
        eobi_compact_of_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
      } break;

      default: { std::cerr << " No handling for this source yet \n"; } break;
    }
  }

  inline void ProcessFPGAMessages(bool _keep_in_loop_ = true) {
    for (std::set<HFSAT::MDS_MSG::MDSMessageExchType>::iterator itr = fpga_processor_added_.begin();
         itr != fpga_processor_added_.end(); ++itr) {
      switch (*itr) {
        case HFSAT::MDS_MSG::CME: {
#if CCPROFILING_TRADEINIT
          int start_tag = 22;
          int end_tag = 25;
          for (int i = start_tag; i <= end_tag; i++) {
            HFSAT::CpucycleProfiler::GetUniqueInstance().StartNewSession(i);
          }

          HFSAT::CpucycleProfiler::GetUniqueInstance().Start(22);
#endif
          cme_mds_fpga_processor_->ProcessCMEFPGAEvents(
              _keep_in_loop_);  // Make a non-blocking call to FPGA API and process received message
        } break;
        case HFSAT::MDS_MSG::NTP: {
          bmf_mds_fpga_processor_->ProcessBMFFPGAEvents(
              _keep_in_loop_);  // Make a non-blocking call to FPGA API and process received message
        } break;
        default: { } break; }
    }
  }

  inline void ProcessAllEvents(int _socket_fd_) { RunLiveShmSource(false); }



  inline bool CheckAndProcessORSReply() {
    // has to be volatile, waiting on shared memory segment queue position
    volatile uint64_t queue_position_ = GetORSReplyQueuePos();
    // events are available only if the queue position at source is higher by 1, circular queue, will lag behind by 1
    // packet at start
    if (-1 == ors_reply_index_) {
      ors_reply_index_ = queue_position_;
    }

    if (ors_reply_index_ == (int64_t)queue_position_) return false;

    // memcpy is only done as safegaurd from writer writing the same segment
    ors_reply_shm_queue_->Pop(generic_mds_message_, ors_reply_index_);
    ors_reply_index_++;
    int32_t lag_value = (queue_position_ - ors_reply_index_) >= 0 ? (queue_position_ - ors_reply_index_)
                                                            : (queue_position_ - ors_reply_index_ + ORS_REPLY_MDS_QUEUE_SIZE);

    if (lag_value >= ORS_REPLY_MDS_QUEUE_SIZE_LAG_THRESHOLD) {
          struct timeval tv;
          gettimeofday(&tv, NULL);
          DBGLOG_CLASS_FUNC_LINE_ERROR << "CME_ORS QUEUE LAG AT THIS POINT : " << tv.tv_sec << "." << tv.tv_usec
                                       << " LAG VALUE = " << lag_value << DBGLOG_ENDL_DUMP;
    }

    // std::cout << "MSSG READ FROM ORSREPLY " << generic_mds_message_.generic_data_.ors_reply_data_.ToString()<< std::endl;
    ors_reply_processor_->ProcessORSReplyEvent(&(generic_mds_message_.generic_data_.ors_reply_data_));

    return true;
  }

  inline void RunLiveShmSource(bool _keep_in_loop_ = true) override {
    keep_me_running_ = true;
    while (keep_me_running_) {
      // Call function only when atleast one flga source added
      if (fpga_added_) {
        ProcessFPGAMessages();  // Assuming for now that all FPGA sources are considered primary
      }
      if (ors_reply_processor_ != NULL ) 
        while (CheckAndProcessORSReply())
          ; // process all the ors reply and move to mkt update
      
      // has to be volatile, waiting on shared memory segment queue position
      volatile uint64_t queue_position_ = GetQueuePos();

      // events are available only if the queue position at source is higher by 1, circular queue, will lag behind by 1
      // packet at start
      if (-1 == index_) {
        index_ = queue_position_;
      }

      if (index_ == (int64_t)queue_position_) {
        // THis is kind of hack to work with Retail
        if (!_keep_in_loop_) break;

        continue;
      }
    
#if CCPROFILING_TRADEINIT
      int start_tag = 22;
      int end_tag = 25;
      for (int i = start_tag; i <= end_tag; i++) {
        HFSAT::CpucycleProfiler::GetUniqueInstance().StartNewSession(i);
      }

      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(22);
#endif

      if (!using_thread_safe_shm_) {
        index_ = (index_ + 1) & (GENERIC_MDS_QUEUE_SIZE - 1);

        int32_t lag_value = (queue_position_ - index_) >= 0 ? (queue_position_ - index_)
                                                            : (queue_position_ - index_ + GENERIC_MDS_QUEUE_SIZE);

        if (lag_value >= GENERIC_MDS_QUEUE_SIZE_LAG_THRESHOLD) {
          struct timeval tv;
          gettimeofday(&tv, NULL);
          DBGLOG_CLASS_FUNC_LINE_ERROR << "QUEUE LAG AT THIS POINT : " << tv.tv_sec << "." << tv.tv_usec
                                       << " LAG VALUE = " << lag_value << DBGLOG_ENDL_DUMP;
        }

        // memcpy is only done as safegaurd from writer writing the same segment
        memcpy(&generic_mds_message_, (HFSAT::MDS_MSG::GenericMDSMessage *)(generic_mds_shm_struct_ + index_),
               generic_mds_message_pkt_size_);
      } else {
        // memcpy is only done as safegaurd from writer writing the same segment
        shm_queue_->Pop(generic_mds_message_, index_);
        index_++;
      }

      time_info_.cshmw_start_time = generic_mds_message_.t2t_cshmw_start_time_;
      time_info_.cshmw_end_time = generic_mds_message_.t2t_cshmw_start_time_ + generic_mds_message_.t2t_cshmw_time_;
      HFSAT::RuntimeProfiler::GetUniqueInstance(HFSAT::ProfilerType::kTRADEINIT).Start(time_info_);
      // std::cout<<"GOING FOR MKT UPDATE NSE"<<std::endl;
      switch (processing_mode_) {
        case HFSAT::kComShmConsumer: {
          // Check if the processor for this is added or not
          if (processor_added_.find(generic_mds_message_.mds_msg_exch_) == processor_added_.end()) {
            continue;
          }

          switch (generic_mds_message_.mds_msg_exch_) {
            case HFSAT::MDS_MSG::CME: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(5);
#endif
              cme_mds_processor_->ProcessCMEEvent(&(generic_mds_message_.generic_data_.cme_data_));
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.cme_data_.time_);

            } break;

            case HFSAT::MDS_MSG::LIFFE:
            case HFSAT::MDS_MSG::LIFFE_LS: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(6);
#endif
              liffe_mds_processor_->ProcessLIFFEEvent(&(generic_mds_message_.generic_data_.liffe_data_));
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.liffe_data_.time_);

            } break;

            case HFSAT::MDS_MSG::EUREX_LS: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(7);
#endif
              eurex_ls_mds_processor_->ProcessEUREXLSEvent(&(generic_mds_message_.generic_data_.eurex_ls_data_),
                                                           generic_mds_message_.time_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.time_);

            } break;

            case HFSAT::MDS_MSG::EOBI_LS: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(7);
#endif
              eobi_ls_mds_processor_->ProcessEOBILSEvent(&(generic_mds_message_.generic_data_.eobi_ls_data_),
                                                         generic_mds_message_.time_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.time_);

            } break;

            case HFSAT::MDS_MSG::CONTROL: {
              control_processor_->ProcessControlEvent(&(generic_mds_message_.generic_data_.control_data_));
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.time_);

            } break;

            case HFSAT::MDS_MSG::ORS_REPLY: {
              // std::cout<<"MDS_MSG MAIN SHM MSSG READ FROM ORSREPLY "<<  std::endl;
              ors_reply_processor_->ProcessORSReplyEvent(&(generic_mds_message_.generic_data_.ors_reply_data_));
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.time_);

            } break;

            case HFSAT::MDS_MSG::RTS: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(9);
#endif
              rts_mds_processor_->ProcessRTSEvent(&(generic_mds_message_.generic_data_.rts_data_));
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.rts_data_.time_);

            } break;

            case HFSAT::MDS_MSG::RTS_OF: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(9);
#endif

              rts_mds_of_processor_->ProcessRTSEvent(&(generic_mds_message_.generic_data_.rts_of_data_));
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.rts_of_data_.time_);

            } break;

            // Same Handling for MICEX_CR and MICEX_EQ

            case HFSAT::MDS_MSG::MICEX: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(18);
#endif
              micex_mds_processor_->ProcessMICEXEvent(&(generic_mds_message_.generic_data_.micex_data_));
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.micex_data_.time_);

            } break;

            case HFSAT::MDS_MSG::MICEX_OF: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(18);
#endif
              micex_cr_of_mds_processor_->ProcessMICEXOFEvent(&(generic_mds_message_.generic_data_.micex_of_data_));
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.micex_of_data_.time_);

            } break;

            case HFSAT::MDS_MSG::CME_LS: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(5);
#endif
              cme_ls_mds_processor_->ProcessCMELSEvent(&(generic_mds_message_.generic_data_.cme_ls_data_),
                                                       generic_mds_message_.time_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.time_);

            } break;

            case HFSAT::MDS_MSG::NTP:
            case HFSAT::MDS_MSG::NTP_LS: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(19);
#endif
              ntp_mds_processor_->ProcessNTPEvent(&(generic_mds_message_.generic_data_.ntp_data_));
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.ntp_data_.time_);

            } break;
            case HFSAT::MDS_MSG::BMF_EQ: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(19);
#endif
              bmfeq_mds_processor_->ProcessBMFEQEvent(&(generic_mds_message_.generic_data_.bmf_eq_data_));
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.bmf_eq_data_.time_);
            } break;
            case HFSAT::MDS_MSG::EOBI_PF: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(7);
#endif
              eobi_pf_mds_processor_->ProcessEOBIPFEvent(&(generic_mds_message_.generic_data_.eobi_pf_data_));
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.eobi_pf_data_.time_);

            } break;

            case HFSAT::MDS_MSG::OSE_PF: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(8);
#endif
              ose_pf_mds_processor_->ProcessOSEPFEvent(&(generic_mds_message_.generic_data_.ose_pf_data_),
                                                       generic_mds_message_.time_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.time_);

            } break;

            case HFSAT::MDS_MSG::OSE_CF: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(8);
#endif
              ose_cf_mds_processor_->ProcessOSECFEvent(&(generic_mds_message_.generic_data_.ose_cf_data_),
                                                       generic_mds_message_.time_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.time_);

            } break;

            case HFSAT::MDS_MSG::CSM: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(21);
#endif
              cfe_mds_processor_->ProcessCSMEvent(&(generic_mds_message_.generic_data_.csm_data_));
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.csm_data_.time_);

            } break;

            case HFSAT::MDS_MSG::OSE_L1: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(8);
#endif
              ose_l1_mds_processor_->ProcessOSEL1Event(&(generic_mds_message_.generic_data_.ose_l1_data_),
                                                       generic_mds_message_.time_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.time_);

            } break;

            case HFSAT::MDS_MSG::TMX:
            case HFSAT::MDS_MSG::TMX_LS: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(10);
#endif
              tmx_mds_processor_->ProcessTMXEvent(&generic_mds_message_.generic_data_.tmx_data_,
                                                  generic_mds_message_.time_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.time_);
            } break;

            case HFSAT::MDS_MSG::TMX_OBF: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(10);
#endif
              tmx_obf_mds_processor_->ProcessTMXOBFEvent(&generic_mds_message_.generic_data_.tmx_obf_data_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.tmx_obf_data_.time_);
            } break;

            // book building modification purpose
            case HFSAT::MDS_MSG::TMX_OBF_OF: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(10);
#endif
              tmx_obf_of_mds_processor_->ProcessTMXOBFOFEvent(&generic_mds_message_.generic_data_.tmx_orderfeed_data_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.tmx_orderfeed_data_.time_);
            } break;

            case HFSAT::MDS_MSG::ICE:
            case HFSAT::MDS_MSG::ICE_LS: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(8);
#endif
              ice_mds_processor_->ProcessICEEvent(&generic_mds_message_.generic_data_.ice_data_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.ice_data_.time_);
            } break;

            case HFSAT::MDS_MSG::ASX: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(11);
#endif
              asx_mds_processor_->ProcessASXEvent(&generic_mds_message_.generic_data_.asx_data_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.asx_data_.time_);
            } break;

            case HFSAT::MDS_MSG::SGX: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(8);
#endif
              sgx_mds_processor_->ProcessEvent(&generic_mds_message_.generic_data_.sgx_data_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.sgx_data_.time_);
            } break;
            case HFSAT::MDS_MSG::OSE_ITCH_PF: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(8);
#endif
              ose_itch_mds_processor_->ProcessOSEEvent(&generic_mds_message_.generic_data_.ose_itch_pf_data_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.ose_itch_pf_data_.time_);
            } break;
            case HFSAT::MDS_MSG::OSE_ITCH_OF: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(29);
#endif
              ose_itch_of_mds_processor_->ProcessOSEItchOFEvent(&generic_mds_message_.generic_data_.ose_itch_of_data_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.ose_itch_of_data_.time_);
            } break;
            case HFSAT::MDS_MSG::HKOMDPF: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(13);
#endif
              hkomdpf_mds_processor_->ProcessHKOMDPFEvent(&generic_mds_message_.generic_data_.hkomd_pf_data_,
                                                          generic_mds_message_.time_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.time_);
            } break;
            case HFSAT::MDS_MSG::AFLASH: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(15);
#endif
              dbglogger_ << "Received Aflash message.. " << generic_mds_message_.mds_msg_exch_ << "\n";
              aflash_mds_processor_->ProcessAflashEvent(&generic_mds_message_.generic_data_.aflash_data_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.aflash_data_.time_);
            } break;
            case HFSAT::MDS_MSG::RETAIL: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(16);
#endif
              retail_mds_processor_->ProcessRetailEvent(&generic_mds_message_.generic_data_.retail_data_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.retail_data_.time_);
            } break;
            case HFSAT::MDS_MSG::NSE: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(14);
#endif
              // std::cout<<"CALL TO ProcessNSEEvent BOOK "<<std::endl;
              nse_mds_processor_->ProcessNSEEvent(&generic_mds_message_.generic_data_.nse_data_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.nse_data_.source_time);
            } break;
            case HFSAT::MDS_MSG::NSE_L1: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(15);
#endif
              nse_l1_mds_processor_->ProcessNSEL1Event(&generic_mds_message_.generic_data_.nse_l1_data_);
              // I need to convert ttime_t to timeval, but not interested for now
              // ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.nse_l1_data_.time_);
            } break;
            case HFSAT::MDS_MSG::EOBI_OF: {
#if CCPROFILING_TRADEINIT
              HFSAT::CpucycleProfiler::GetUniqueInstance().Start(7);
#endif
              eobi_compact_of_mds_processor_->ProcessEOBICompactOFEvent(
                  &generic_mds_message_.generic_data_.eobi_of_data_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.eobi_of_data_.time_);
            } break;
            default: {
              std::cerr << " UNKNOWN PROCESSOR, SHOULD NEVER REACH HERE "
                        << "\n";
              exit(1);

            } break;
          }

        } break;

        default: { std::cerr << " Mode Not Handled Yet : " << processing_mode_ << "\n"; } break;
      }
    }
  }
};
}
}
