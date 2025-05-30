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
#include "baseinfra/MDSMessages/combined_mds_messages_shm_base.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_control_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_ors_reply_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_ntp_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_bmf_fpga_processor.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_bmfeq_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_nse_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_bse_processor.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#define CCPROFILING_TRADEINIT 0

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
  bool keep_me_running_;

  HFSAT::MDS_MSG::GenericMDSMessage generic_mds_message_;
  uint32_t generic_mds_message_pkt_size_;

  HFSAT::RuntimeProfiler &runtime_profiler_;

  HFSAT::CombinedMDSMessagesControlProcessor *control_processor_;
  HFSAT::CombinedMDSMessagesORSReplyProcessor *ors_reply_processor_;
  HFSAT::CombinedMDSMessagesNTPProcessor *ntp_mds_processor_;
  HFSAT::CombinedMDSMessagesBMFEQProcessor *bmfeq_mds_processor_;
  HFSAT::CombinedMDSMessagesNSEProcessor *nse_mds_processor_;
  HFSAT::BSE::CombinedMDSMessagesBSEProcessor *bse_mds_processor_;
  // book building modification purpose
  HFSAT::CombinedMDSMessagesBMFFPGAProcessor *bmf_mds_fpga_processor_;

  bool fpga_added_;

 public:
  CombinedMDSMessagesShmProcessor(HFSAT::DebugLogger &_dgblogger_, HFSAT::SecurityNameIndexer &_sec_name_indexer_,
                                  HFSAT::FastMdConsumerMode_t _mode_)
      : CombinedMDSMessagesShmBase(),
        processing_mode_(_mode_),
        dbglogger_(_dgblogger_),
        sec_name_indexer_(_sec_name_indexer_),
        index_(-1),
        keep_me_running_(false),
        generic_mds_message_(),
        generic_mds_message_pkt_size_(sizeof(HFSAT::MDS_MSG::GenericMDSMessage)),
        runtime_profiler_(HFSAT::RuntimeProfiler::GetUniqueInstance()),
        control_processor_(NULL),
        ors_reply_processor_(NULL),
        ntp_mds_processor_(NULL),
        bmfeq_mds_processor_(NULL),
        nse_mds_processor_(NULL),
        bse_mds_processor_(NULL),
        // book building modification purpose
        bmf_mds_fpga_processor_(NULL),
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
      case HFSAT::MDS_MSG::NSE: {
        nse_mds_processor_ = new HFSAT::CombinedMDSMessagesNSEProcessor(dbglogger_, sec_name_indexer_);
        nse_mds_processor_->SetOrderGlobalListenerNSE((OrderGlobalListenerNSE *)_p_book_listener_);
        nse_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
      } break;
      case HFSAT::MDS_MSG::BSE: {
        bse_mds_processor_ = new HFSAT::BSE::CombinedMDSMessagesBSEProcessor(dbglogger_, sec_name_indexer_);
        bse_mds_processor_->SetOrderGlobalListenerBSE((OrderGlobalListenerBSE *)_p_book_listener_);
        bse_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
      } break;
      default: { std::cerr << " No handling for this source yet \n"; } break;
    }
  }

  inline void ProcessFPGAMessages(bool _keep_in_loop_ = true) {
    for (std::set<HFSAT::MDS_MSG::MDSMessageExchType>::iterator itr = fpga_processor_added_.begin();
         itr != fpga_processor_added_.end(); ++itr) {
      switch (*itr) {
        case HFSAT::MDS_MSG::NTP: {
          bmf_mds_fpga_processor_->ProcessBMFFPGAEvents(
              _keep_in_loop_);  // Make a non-blocking call to FPGA API and process received message
        } break;
        default: { } break; }
    }
  }

  inline void ProcessAllEvents(int _socket_fd_) { RunLiveShmSource(false); }

  inline void RunLiveShmSource(bool _keep_in_loop_ = true) override {
    keep_me_running_ = true;
    while (keep_me_running_) {
      // Call function only when atleast one flga source added
      if (fpga_added_) {
        ProcessFPGAMessages();  // Assuming for now that all FPGA sources are considered primary
      }

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

      runtime_profiler_.Start(HFSAT::ProfilerType::kCOMBINEDSHMWRITER, generic_mds_message_.t2t_cshmw_start_time_);
      runtime_profiler_.End(HFSAT::ProfilerType::kCOMBINEDSHMWRITER,
                            generic_mds_message_.t2t_cshmw_start_time_ + generic_mds_message_.t2t_cshmw_time_);
      runtime_profiler_.Start(HFSAT::ProfilerType::kTRADEINIT);

      switch (processing_mode_) {
        case HFSAT::kComShmConsumer: {
          // Check if the processor for this is added or not
          if (processor_added_.find(generic_mds_message_.mds_msg_exch_) == processor_added_.end()) {
            continue;
          }

          switch (generic_mds_message_.mds_msg_exch_) {
            case HFSAT::MDS_MSG::CONTROL: {
              control_processor_->ProcessControlEvent(&(generic_mds_message_.generic_data_.control_data_));
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.time_);

            } break;

            case HFSAT::MDS_MSG::ORS_REPLY: {
              ors_reply_processor_->ProcessORSReplyEvent(&(generic_mds_message_.generic_data_.ors_reply_data_));
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.time_);

            } break;

            case HFSAT::MDS_MSG::NTP:
            case HFSAT::MDS_MSG::NTP_LS: {
              ntp_mds_processor_->ProcessNTPEvent(&(generic_mds_message_.generic_data_.ntp_data_));
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.ntp_data_.time_);

            } break;
            case HFSAT::MDS_MSG::BMF_EQ: {
              bmfeq_mds_processor_->ProcessBMFEQEvent(&(generic_mds_message_.generic_data_.bmf_eq_data_));
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.bmf_eq_data_.time_);
            } break;
            case HFSAT::MDS_MSG::NSE: {
              nse_mds_processor_->ProcessNSEEvent(&generic_mds_message_.generic_data_.nse_data_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.nse_data_.source_time);
            } break;
            case HFSAT::MDS_MSG::BSE: {
              bse_mds_processor_->ProcessBSEEvent(&generic_mds_message_.generic_data_.bse_data_);
              ProcessQueueStats(queue_position_, index_, generic_mds_message_.generic_data_.bse_data_.source_time);
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
