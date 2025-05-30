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
#include "dvccode/CDef/nse_shm_interface_defines.hpp"
#include "dvccode/CDef/bse_shm_interface_defines.hpp"
#include "dvccode/Utils/combined_mds_messages_multi_shm_base.hpp"
#include "dvccode/Utils/tcp_direct_client_zocket_with_logging.hpp"
#include "dvccode/Utils/udp_direct_muxer.hpp"
#include "dvccode/IBUtils/contract_manager.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_control_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_pro_nse_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_bse_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_pro_ors_reply_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_bmf_fpga_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_shm_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_ibkr_processor.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"
#include "infracore/IBKRMD/ibkr_l1_md_handler.hpp"

namespace HFSAT {
namespace MDSMessages {

class CombinedMDSMessagesMultiShmProcessor : public HFSAT::MDSMessages::CombinedMDSMessagesNSEProShmBase,
                                             public HFSAT::MDSMessages::CombinedMDSMessagesBSEShmBase,
                                             public HFSAT::MDSMessages::CombinedMDSMessagesORSProShmBase,
                                             public HFSAT::IBKRMD::MarketEventListener,
                                             public HFSAT::SimpleExternalDataLiveListener {
 private:
  HFSAT::FastMdConsumerMode_t processing_mode_;
  std::set<HFSAT::MDS_MSG::MDSMessageExchType> processor_added_;

  HFSAT::DebugLogger &dbglogger_;
  HFSAT::SecurityNameIndexer &sec_name_indexer_;

  // this is meant to be a local copy to track queue position, compare with volatile shared memory counter
  int64_t index_;
  int64_t ors_reply_index_;
  bool keep_me_running_;

  NSE_MDS::NSETBTDataCommonStructProShm nse_generic_mds_message_;
  EOBI_MDS::EOBICommonStruct bse_generic_mds_message_;
  HFSAT::GenericORSReplyStructLiveProShm ors_reply_mds_message_;
  IBL1UpdateTick ib_tick_update_;
  uint32_t nse_generic_mds_message_pkt_size_;
  uint32_t bse_generic_mds_message_pkt_size_;
  uint32_t ors_reply_mds_shm_struct_pkt_size_;
  uint32_t ib_tick_update_pkt_size_;

  HFSAT::RuntimeProfiler &runtime_profiler_;

  HFSAT::CombinedMDSMessagesControlProcessor *control_processor_;
  HFSAT::CombinedMDSMessagesBMFFPGAProcessor *bmf_mds_fpga_processor_;
  HFSAT::ProNSE::CombinedMDSMessagesProNSEProcessor *nse_mds_processor_;
  HFSAT::BSE::CombinedMDSMessagesBSEProcessor *bse_mds_processor_;
  HFSAT::IBKR::CombinedMDSMessagesIBKRProcessor *ibkr_mds_processor_;
  HFSAT::ProORSReply::CombinedMDSMessagesProShmORSReplyProcessor *ors_reply_processor_;
  HFSAT::MDSMessages::CombinedMDSMessagesShmProcessor *combined_mds_shm_processor;
  HFSAT::Utils::UDPDirectMultipleZocket* udp_direct_multiple_zockets_ptr_;

 public:
  CombinedMDSMessagesMultiShmProcessor(HFSAT::DebugLogger &_dgblogger_, HFSAT::SecurityNameIndexer &_sec_name_indexer_,
                                       HFSAT::FastMdConsumerMode_t _mode_,HFSAT::Utils::UDPDirectMultipleZocket* udp_direct_multiple_zockets_ptr=nullptr)
      : CombinedMDSMessagesNSEProShmBase(),
        CombinedMDSMessagesBSEShmBase(),
        CombinedMDSMessagesORSProShmBase(),
        processing_mode_(_mode_),
        dbglogger_(_dgblogger_),
        sec_name_indexer_(_sec_name_indexer_),
        index_(-1),
        ors_reply_index_(-1),
        keep_me_running_(false),
        nse_generic_mds_message_(),
        bse_generic_mds_message_(),
        ors_reply_mds_message_(),
        ib_tick_update_(),
        nse_generic_mds_message_pkt_size_(sizeof(NSE_MDS::NSETBTDataCommonStructProShm)),
        bse_generic_mds_message_pkt_size_(sizeof(EOBI_MDS::EOBICommonStruct)),
        ors_reply_mds_shm_struct_pkt_size_(sizeof(HFSAT::GenericORSReplyStructLiveProShm)),
        ib_tick_update_pkt_size_(sizeof(IBL1UpdateTick)),
        runtime_profiler_(HFSAT::RuntimeProfiler::GetUniqueInstance()),
        control_processor_(NULL),
        bmf_mds_fpga_processor_(NULL),
        nse_mds_processor_(NULL),
        bse_mds_processor_(NULL),
        ibkr_mds_processor_(NULL),
        ors_reply_processor_(NULL),
        combined_mds_shm_processor(NULL),
        udp_direct_multiple_zockets_ptr_ (udp_direct_multiple_zockets_ptr) {}

  CombinedMDSMessagesMultiShmProcessor(HFSAT::DebugLogger &_dgblogger_, HFSAT::SecurityNameIndexer &_sec_name_indexer_,
                                       HFSAT::FastMdConsumerMode_t _mode_, HFSAT::MDSMessages::CombinedMDSMessagesShmProcessor *_combined_mds_shm_processor_,
                                       HFSAT::Utils::UDPDirectMultipleZocket* udp_direct_multiple_zockets_ptr=nullptr)
      : CombinedMDSMessagesNSEProShmBase(),
        CombinedMDSMessagesBSEShmBase(),
        CombinedMDSMessagesORSProShmBase(),
        processing_mode_(_mode_),
        dbglogger_(_dgblogger_),
        sec_name_indexer_(_sec_name_indexer_),
        index_(-1),
        ors_reply_index_(-1),
        keep_me_running_(false),
        nse_generic_mds_message_(),
        bse_generic_mds_message_(),
        nse_generic_mds_message_pkt_size_(sizeof(NSE_MDS::NSETBTDataCommonStructProShm)),
        bse_generic_mds_message_pkt_size_(sizeof(EOBI_MDS::EOBICommonStruct)),
        ors_reply_mds_shm_struct_pkt_size_(sizeof(HFSAT::GenericORSReplyStructLiveProShm)),
        runtime_profiler_(HFSAT::RuntimeProfiler::GetUniqueInstance()),
        control_processor_(NULL),
        bmf_mds_fpga_processor_(NULL),
        nse_mds_processor_(NULL),
        bse_mds_processor_(NULL),
        ors_reply_processor_(NULL),
        combined_mds_shm_processor(_combined_mds_shm_processor_),
        udp_direct_multiple_zockets_ptr_ (udp_direct_multiple_zockets_ptr) {}

  ~CombinedMDSMessagesMultiShmProcessor() {}

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

  void ResetRecoveryIndex(){
    if (nse_mds_processor_ != NULL ){
        nse_mds_processor_->ResetNseRecoveryIndex();
    }  else if ( bse_mds_processor_ != NULL) {
                bse_mds_processor_->ResetBseRecoveryIndex();
    }
  }

  void CleanUp() { keep_me_running_ = false; }

  inline void AddDataSourceForProcessing(HFSAT::MDS_MSG::MDSMessageExchType _source_, void *_p_book_listener_,
                                         ExternalTimeListener *_p_time_keeper_,
                                         HFSAT::HybridSecurityManager *_hybrid_security_manager_ = NULL) {
    // Check if the source already added or not
    if (processor_added_.find(_source_) != processor_added_.end()) return;

    // Add this processor for callbacks
    processor_added_.insert(_source_);

    switch (_source_) {
      case HFSAT::MDS_MSG::BSE: {
        bse_mds_processor_ = new HFSAT::BSE::CombinedMDSMessagesBSEProcessor(dbglogger_, sec_name_indexer_);
        bse_mds_processor_->SetOrderGlobalListenerBSE((OrderGlobalListenerBSE *)_p_book_listener_);
        bse_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
      } break;
      case HFSAT::MDS_MSG::NSE: {
        nse_mds_processor_ = new HFSAT::ProNSE::CombinedMDSMessagesProNSEProcessor(dbglogger_, sec_name_indexer_);
        nse_mds_processor_->SetOrderGlobalListenerNSE((OrderGlobalListenerNSE *)_p_book_listener_);
        nse_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
      } break;
      case HFSAT::MDS_MSG::IBKR: {
        ibkr_mds_processor_ = new HFSAT::IBKR::CombinedMDSMessagesIBKRProcessor(dbglogger_, sec_name_indexer_);
        ibkr_mds_processor_->SetOrderGlobalListenerIBKR((OrderGlobalListenerIBKR *)_p_book_listener_);
        ibkr_mds_processor_->SetExternalTimeListener(_p_time_keeper_);
      } break;

      default: { std::cerr << " No handling for this source yet \n"; } break;
    }
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
      ors_reply_processor_ = new HFSAT::ProORSReply::CombinedMDSMessagesProShmORSReplyProcessor(
          dbglogger_, sec_name_indexer_, _sid_to_smv_ptr_map_);
      ors_reply_processor_->SetExternalTimeListener(_p_time_keeper_);
    }
  }

  inline void AddFPGADataSourceForProcessing(HFSAT::MDS_MSG::MDSMessageExchType _source_, void *_p_book_listener_,
                                             ExternalTimeListener *_p_time_keeper_) {
    switch (_source_) {
      case HFSAT::MDS_MSG::NTP: {
        bmf_mds_fpga_processor_ = new HFSAT::CombinedMDSMessagesBMFFPGAProcessor(dbglogger_, sec_name_indexer_);
        bmf_mds_fpga_processor_->SetFPGAGlobalListener((BMFFPGAFullBookGlobalListener *)(_p_book_listener_));
        bmf_mds_fpga_processor_->SetExternalTimeListener(_p_time_keeper_);
      } break;
      default: { } break; }
  }

  inline HFSAT::ProORSReply::CombinedMDSMessagesProShmORSReplyProcessor *GetProShmORSReplyProcessor() {
    if (ors_reply_processor_ == NULL) {
      std::cerr << "GetORSReplyProcessor called before initializing the ors reply processor \n";
      exit(-1);
    }

    return ors_reply_processor_;
  }

  inline void ProcessFPGAMessages(bool _keep_in_loop_ = true) {
    bmf_mds_fpga_processor_->ProcessBMFFPGAEvents(
        _keep_in_loop_);  // Make a non-blocking call to FPGA API and process received message
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
    ors_reply_shm_queue_->Pop(ors_reply_mds_message_, ors_reply_index_);

    ors_reply_index_++;

    runtime_profiler_.Start(HFSAT::ProfilerType::kCOMBINEDSHMWRITER, 0);
    runtime_profiler_.End(HFSAT::ProfilerType::kCOMBINEDSHMWRITER, 0);
    runtime_profiler_.Start(HFSAT::ProfilerType::kTRADEINIT);

    if (kORRType_Control != ors_reply_mds_message_.orr_type_) {
      ors_reply_processor_->ProcessORSReplyEvent(&ors_reply_mds_message_);
    } else {  // control msg

      HFSAT::GenericControlRequestStruct ctrl_msg;
      memset((void *)&ctrl_msg, 0, sizeof(HFSAT::GenericControlRequestStruct));

      struct timeval tv;
      gettimeofday(&tv, NULL);

      ctrl_msg.time_set_by_frontend_ = tv;
      ctrl_msg.trader_id_ = ors_reply_mds_message_.order_flags_;
      ctrl_msg.control_message_.message_code_ = (HFSAT::ControlMessageCode_t)(ors_reply_mds_message_.global_position_);
      ctrl_msg.control_message_.intval_1_ = ors_reply_mds_message_.int_price_;
      memcpy((void *)ctrl_msg.control_message_.strval_1_, (void *)ors_reply_mds_message_.symbol_,
             sizeof(ors_reply_mds_message_.symbol_));
      control_processor_->ProcessControlEvent(&ctrl_msg);
    }

    return true;
  }

  inline void RunLiveShmSourceForBMF(bool _keep_in_loop_ = true) {
    keep_me_running_ = true;

    while (keep_me_running_) {
      ProcessFPGAMessages();  // Assuming for now that all FPGA sources are considered primary
      CheckAndProcessORSReply();
    }
  }

  inline void RunLiveShmSourceForNSEOEBU(bool _keep_in_loop_ = true) {
    std::cout<<"RunLiveShmSourceFOR NSE OEBU\n";
    keep_me_running_ = true;

    while (keep_me_running_) {
      while (CheckAndProcessORSReply());

      // has to be volatile, waiting on shared memory segment queue position
      volatile uint64_t queue_position_ = GetQueuePos();

      // events are available only if the queue position at source is higher by 1, circular queue, will lag behind by 1
      // packet at start
      if (-1 == index_) {
        index_ = queue_position_;
      }

      if (index_ == (int64_t)queue_position_) continue;

      shm_queue_->Pop(nse_generic_mds_message_, index_);
      index_++;

      runtime_profiler_.Start(HFSAT::ProfilerType::kCOMBINEDSHMWRITER, nse_generic_mds_message_.t2t_cshmw_start_time);
      runtime_profiler_.End(HFSAT::ProfilerType::kCOMBINEDSHMWRITER, nse_generic_mds_message_.t2t_cshw_end_time);
      runtime_profiler_.Start(HFSAT::ProfilerType::kTRADEINIT);

      if ((kMsgTypeControl == nse_generic_mds_message_.msg_type) && (control_processor_ != NULL)) {
        HFSAT::GenericControlRequestStruct ctrl_msg;
        memset((void *)&ctrl_msg, 0, sizeof(HFSAT::GenericControlRequestStruct));

        ctrl_msg.time_set_by_frontend_ = nse_generic_mds_message_.source_time;
        ctrl_msg.trader_id_ = nse_generic_mds_message_.msg_seq;
        ctrl_msg.control_message_.message_code_ =
            (HFSAT::ControlMessageCode_t)(nse_generic_mds_message_.data.control_msg.message_code_);
        ctrl_msg.control_message_.intval_1_ = nse_generic_mds_message_.data.control_msg.intval_1_;
        memcpy((void *)ctrl_msg.control_message_.strval_1_, (void *)nse_generic_mds_message_.data.control_msg.strval_1_,
               sizeof(nse_generic_mds_message_.data.control_msg.strval_1_));
        control_processor_->ProcessControlEvent(&ctrl_msg);

      } else {
        nse_mds_processor_->ProcessNSEEvent(&nse_generic_mds_message_);
      }
    }
  }

  inline void RunLiveShmSourceForBSEOEBU(bool _keep_in_loop_ = true) {
    //std::cout<<"RunLiveShmSource\n";
    keep_me_running_ = true;

    while (keep_me_running_) {
      //std::cout<<"keep_me_running_:: ";
      combined_mds_shm_processor->RunLiveShmSource(false);

      // has to be volatile, waiting on shared memory segment queue position
      volatile uint64_t queue_position_ = GetQueuePosBSE();

      // events are available only if the queue position at source is higher by 1, circular queue, will lag behind by 1
      // packet at start
      if (-1 == index_) {
        index_ = queue_position_;
      }

      if (index_ == (int64_t)queue_position_) continue;

      bse_shm_queue_->Pop(bse_generic_mds_message_, index_);
      index_++;

      bse_mds_processor_->ProcessBSEEvent(&bse_generic_mds_message_);
    }
  }

  //since ib data is coming from internal socket, has to be processed with normal socket over SLD
  void OnMarketEventDispatch(IBL1UpdateTick* market_event, bool is_timeout){
    if(false == is_timeout){
      ibkr_mds_processor_->ProcessIBKREvent(market_event);
    }else{
      combined_mds_shm_processor->RunLiveShmSource(false);
    }
  }

  inline void RunLiveShmSource(bool _keep_in_loop_ = true) override {
    keep_me_running_ = true;
    bool alert_sent = false;

    std::cout << "RunLiveShmSource :"  << std::endl;

    while (keep_me_running_) {
      combined_mds_shm_processor->RunLiveShmSource(false);
      // has to be volatile, waiting on shared memory segment queue position
      volatile uint64_t queue_position_ = GetQueuePos();

      // events are available only if the queue position at source is higher by 1, circular queue, will lag behind by 1
      // packet at start
      if (-1 == index_) {
        index_ = queue_position_;
      }

      if (index_ == (int64_t)queue_position_) continue;

      int32_t query_lag = ((int64_t)queue_position_ - index_);

      if (query_lag > 1024 && false == alert_sent) {
        alert_sent = true;
        DBGLOG_CLASS_FUNC_LINE_ERROR << "QUERY LAGGING MORE THAN " << query_lag
                                     << " PACKETS : " << (int64_t)queue_position_ << " " << index_ << DBGLOG_ENDL_DUMP;
      } else if (true == alert_sent && query_lag < 128) {
        alert_sent = false;
      }

      shm_queue_->Pop(nse_generic_mds_message_, index_);
      index_++;

      runtime_profiler_.Start(HFSAT::ProfilerType::kCOMBINEDSHMWRITER, nse_generic_mds_message_.t2t_cshmw_start_time);
      runtime_profiler_.End(HFSAT::ProfilerType::kCOMBINEDSHMWRITER, nse_generic_mds_message_.t2t_cshw_end_time);
      runtime_profiler_.Start(HFSAT::ProfilerType::kTRADEINIT);

      if (kMsgTypeControl == nse_generic_mds_message_.msg_type) {
        HFSAT::GenericControlRequestStruct ctrl_msg;
        memset((void *)&ctrl_msg, 0, sizeof(HFSAT::GenericControlRequestStruct));

        ctrl_msg.time_set_by_frontend_ = nse_generic_mds_message_.source_time;
        ctrl_msg.trader_id_ = nse_generic_mds_message_.msg_seq;
        ctrl_msg.control_message_.message_code_ =
            (HFSAT::ControlMessageCode_t)(nse_generic_mds_message_.data.control_msg.message_code_);
        ctrl_msg.control_message_.intval_1_ = nse_generic_mds_message_.data.control_msg.intval_1_;
        memcpy((void *)ctrl_msg.control_message_.strval_1_, (void *)nse_generic_mds_message_.data.control_msg.strval_1_,
               sizeof(nse_generic_mds_message_.data.control_msg.strval_1_));
        control_processor_->ProcessControlEvent(&ctrl_msg);

      } else {
        nse_mds_processor_->ProcessNSEEvent(&nse_generic_mds_message_);
      }
    }
  }

  inline void RunLiveShmSourceBSE(bool _keep_in_loop_ = true) override {
    keep_me_running_ = true;
    bool alert_sent = false;

    while (keep_me_running_) {
      while (CheckAndProcessORSReply())
        ;

      // has to be volatile, waiting on shared memory segment queue position
      volatile uint64_t queue_position_ = GetQueuePosBSE();

      // events are available only if the queue position at source is higher by 1, circular queue, will lag behind by 1
      // packet at start
      if (-1 == index_) {
        index_ = queue_position_;
      }

      if (index_ == (int64_t)queue_position_) continue;

      int32_t query_lag = ((int64_t)queue_position_ - index_);

      if (query_lag > 1024 && false == alert_sent) {
        alert_sent = true;
        DBGLOG_CLASS_FUNC_LINE_ERROR << "QUERY LAGGING MORE THAN " << query_lag
                                     << " PACKETS : " << (int64_t)queue_position_ << " " << index_ << DBGLOG_ENDL_DUMP;
      } else if (true == alert_sent && query_lag < 128) {
        alert_sent = false;
      }

      bse_shm_queue_->Pop(bse_generic_mds_message_, index_);
      index_++;

      bse_mds_processor_->ProcessBSEEvent(&bse_generic_mds_message_);
    }
  }

  inline void RunLiveShmSourceBSEOEBU(bool _keep_in_loop_ = true) {
    //std::cout<<"RunLiveShmSource\n";
    keep_me_running_ = true;

    while (keep_me_running_) {
      //std::cout<<"keep_me_running_:: ";
      combined_mds_shm_processor->RunLiveShmSource(false);

      // has to be volatile, waiting on shared memory segment queue position
      volatile uint64_t queue_position_ = GetQueuePosBSE();

      // events are available only if the queue position at source is higher by 1, circular queue, will lag behind by 1
      // packet at start
      if (-1 == index_) {
        index_ = queue_position_;
      }

      if (index_ == (int64_t)queue_position_) continue;

      bse_shm_queue_->Pop(bse_generic_mds_message_, index_);

      index_++;
      bse_mds_processor_->ProcessBSEEvent(&bse_generic_mds_message_);
      }
    }


    inline void RunUDPDirectNSEOEBU() {
      std::cout<<"RunUDPDirectNSEOEBU\n";
      bool  keep_me_running_ = true;

      while (keep_me_running_) {
        combined_mds_shm_processor->RunLiveShmSource(false);
        udp_direct_multiple_zockets_ptr_->RunLiveDispatcherOEBU();
      }

    }
    
    inline void RunUDPDirectBSEOEBU() {
      std::cout<<"RunUDPDirectBSEOEBU\n";
      bool  keep_me_running_ = true;

      while (keep_me_running_) {
        combined_mds_shm_processor->RunLiveShmSource(false);
        udp_direct_multiple_zockets_ptr_->RunLiveDispatcherOEBU();
      }

    }
    
    inline void RunUDPDirectConsole() {
      std::cout<<"RunUDPDirectConsole\n";

      udp_direct_multiple_zockets_ptr_->RunLiveDispatcher();
    }

    inline void RunLiveShmSourceConsole(bool _keep_in_loop_ = true) {

        keep_me_running_ = true;
        while (keep_me_running_) {
          while (CheckAndProcessORSReply());
        }
    }

};
}
}
