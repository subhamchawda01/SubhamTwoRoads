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
#include "dvccode/Utils/combined_mds_messages_multi_shm_base.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_control_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_signal_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_pro_nse_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_bse_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_pro_ors_reply_processor.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"
#include "dvccode/Utils/tcp_direct_client_zocket_with_logging.hpp"
#include "infracore/NSEMD/nse_tbt_data_processor.hpp"
#include "infracore/BSEMD/bse_tbt_data_processor.hpp"

#include "dvccode/Utils/rdtsc_timer.hpp"

#define CONTROL_MSG_TRIGGER_COUNT 50000
#define LAG_CONTROL_MSG_TRIGGER_COUNT 5
#define SIGNAL_MSG_TRIGGER_COUNT 1000
#define HFT_LAG_SAFEGUARD_COUNT 20   //Protects HFT strat's hot path against odd slow cases

#define ORS_REPLY_FORCE_READ_MAX_WINDOW_CYCLES 3000000000

namespace HFSAT {
namespace MDSMessages {

class CombinedMDSMessagesDirectProcessor : public HFSAT::MDSMessages::CombinedMDSMessagesORSProShmBase,
                                           public HFSAT::MDSMessages::CombinedMDSMessagesControlProShmBase,
                                           public HFSAT::MDSMessages::CombinedMDSMessagesSignalProShmBase,
                                           public HFSAT::SimpleExternalDataLiveListener,
                                           public HFSAT::Utils::EventTimeoutListener,
                                           public HFSAT::NSEMD::MarketEventListener,
                                           public HFSAT::BSEMD::MarketEventListener {
 private:
  HFSAT::FastMdConsumerMode_t processing_mode_;
  std::set<HFSAT::MDS_MSG::MDSMessageExchType> processor_added_;

  HFSAT::DebugLogger &dbglogger_;
  HFSAT::SecurityNameIndexer &sec_name_indexer_;

  // this is meant to be a local copy to track queue position, compare with volatile shared memory counter
  int64_t ors_reply_index_;
  int64_t control_msg_index_;
  int64_t signal_msg_index_;
  uint64_t control_read_trigger_;
  uint64_t signal_read_trigger_;
  bool keep_me_running_;
  bool use_signal_shm_;

  HFSAT::GenericORSReplyStructLiveProShm ors_reply_mds_message_;
  HFSAT::GenericControlRequestStruct control_mds_message_;
  HFSAT::IVCurveData signal_mds_message_;
  uint32_t ors_reply_mds_shm_struct_pkt_size_;
  int32_t control_msg_pkt_size_;
  int32_t signal_msg_pkt_size_;
  uint64_t control_msg_trigger_count_;
  int32_t no_of_lag_instances_;

  uint64_t last_event_ors_reply_cycles_;
  uint64_t current_md_event_ors_reply_cycles_;

  HFSAT::RuntimeProfiler &runtime_profiler_;

  HFSAT::CombinedMDSMessagesControlProcessor *control_processor_;
  HFSAT::CombinedMDSMessagesSignalProcessor *signal_processor_;
  HFSAT::ProNSE::CombinedMDSMessagesProNSEProcessor *nse_mds_processor_;
  HFSAT::BSE::CombinedMDSMessagesBSEProcessor *bse_mds_processor_;
  HFSAT::ProORSReply::CombinedMDSMessagesProShmORSReplyProcessor *ors_reply_processor_;
  bool oebu_mode;

 public:
  CombinedMDSMessagesDirectProcessor(HFSAT::DebugLogger &_dgblogger_, HFSAT::SecurityNameIndexer &_sec_name_indexer_,
                                     HFSAT::FastMdConsumerMode_t _mode_)
      : CombinedMDSMessagesORSProShmBase(),
        processing_mode_(_mode_),
        dbglogger_(_dgblogger_),
        sec_name_indexer_(_sec_name_indexer_),
        ors_reply_index_(-1),
        control_msg_index_(-1),
        signal_msg_index_(-1),
        control_read_trigger_(0),
        signal_read_trigger_(0),
        keep_me_running_(false),
        use_signal_shm_(false),
        ors_reply_mds_message_(),
        control_mds_message_(),
        ors_reply_mds_shm_struct_pkt_size_(sizeof(HFSAT::GenericORSReplyStructLiveProShm)),
        control_msg_pkt_size_(sizeof(HFSAT::GenericControlRequestStruct)),
        signal_msg_pkt_size_(sizeof(HFSAT::IVCurveData)),
        control_msg_trigger_count_(CONTROL_MSG_TRIGGER_COUNT),
        no_of_lag_instances_(0),
        last_event_ors_reply_cycles_(0),
        current_md_event_ors_reply_cycles_(0),
        runtime_profiler_(HFSAT::RuntimeProfiler::GetUniqueInstance()),
        control_processor_(NULL),
        signal_processor_(new HFSAT::CombinedMDSMessagesSignalProcessor()),
        nse_mds_processor_(NULL),
        bse_mds_processor_(NULL),
        ors_reply_processor_(NULL),
        oebu_mode(false) {}

  ~CombinedMDSMessagesDirectProcessor() {}

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

  void SetSignalShmAccess(bool _use_signal_shm_){
    use_signal_shm_ = _use_signal_shm_;
  }

  void SetControlMsgTriggerCount(int32_t const & trigger_count){
    control_msg_trigger_count_ = trigger_count;
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
      case HFSAT::MDS_MSG::NSE: {
        nse_mds_processor_ = new HFSAT::ProNSE::CombinedMDSMessagesProNSEProcessor(dbglogger_, sec_name_indexer_);
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

  HFSAT::CombinedMDSMessagesSignalProcessor *GetCombinedDirectSignalProcessor(){
    return signal_processor_;
  }

  inline HFSAT::ProORSReply::CombinedMDSMessagesProShmORSReplyProcessor *GetProShmORSReplyProcessor() {
    if (ors_reply_processor_ == NULL) {
      std::cerr << "GetORSReplyProcessor called before initializing the ors reply processor \n";
      exit(-1);
    }

    return ors_reply_processor_;
  }

  inline void ProcessAllEvents(int _socket_fd_) {}

  void SetOebuMode(bool mode){
    dbglogger_ << "SETTING OEBU MODE TO NOT PROCESS ORS REPLY " << DBGLOG_ENDL_FLUSH; 
    oebu_mode = mode;
  }

  inline void OnMarketEventDispatch(NSE_MDS::NSETBTDataCommonStructProShm *market_event) override {
    nse_mds_processor_->ProcessNSEEvent(market_event);
    current_md_event_ors_reply_cycles_ = HFSAT::GetReadTimeStampCounter();

    //Force an ORS / Control Read
    if((current_md_event_ors_reply_cycles_ > last_event_ors_reply_cycles_) && ((current_md_event_ors_reply_cycles_ - last_event_ors_reply_cycles_) > ORS_REPLY_FORCE_READ_MAX_WINDOW_CYCLES)){
      OnEventsTimeout();
      no_of_lag_instances_++;

      //Only want to switch to high control read freq if strat keeps on lagging multiple times
      if(control_msg_trigger_count_ != LAG_CONTROL_MSG_TRIGGER_COUNT && (no_of_lag_instances_ > HFT_LAG_SAFEGUARD_COUNT)){
        control_msg_trigger_count_ = LAG_CONTROL_MSG_TRIGGER_COUNT;
        dbglogger_ << "STRAT LAG OBSERVED, SETTING CONTROL CMD SPEEDUP TO WORST 5 SEC, CURRENT LAG IN CYCLES : " << (current_md_event_ors_reply_cycles_ - last_event_ors_reply_cycles_) << DBGLOG_ENDL_FLUSH;
      }

      last_event_ors_reply_cycles_ = current_md_event_ors_reply_cycles_;
    }

  }

  inline void OnMarketEventDispatch(EOBI_MDS::EOBICommonStruct *market_event) override {
    runtime_profiler_.End(HFSAT::ProfilerType::kCOMBINEDSHMWRITER);
    runtime_profiler_.Start(HFSAT::ProfilerType::kTRADEINIT);
    bse_mds_processor_->ProcessBSEEvent(market_event);

    //Force an ORS / Control Read
    current_md_event_ors_reply_cycles_ = HFSAT::GetReadTimeStampCounter();
    if((current_md_event_ors_reply_cycles_ > last_event_ors_reply_cycles_) && ((current_md_event_ors_reply_cycles_ - last_event_ors_reply_cycles_) > ORS_REPLY_FORCE_READ_MAX_WINDOW_CYCLES)){
      OnEventsTimeout();
      no_of_lag_instances_++;

      //Only want to switch to high control read freq if strat keeps on lagging multiple times
      if(control_msg_trigger_count_ != LAG_CONTROL_MSG_TRIGGER_COUNT && (no_of_lag_instances_ > HFT_LAG_SAFEGUARD_COUNT)){
        control_msg_trigger_count_ = LAG_CONTROL_MSG_TRIGGER_COUNT;
        dbglogger_ << "STRAT LAG OBSERVED, SETTING CONTROL CMD SPEEDUP TO WORST 5 SEC, CURRENT LAG IN CYCLES : " << (current_md_event_ors_reply_cycles_ - last_event_ors_reply_cycles_) << DBGLOG_ENDL_FLUSH;
      }

      last_event_ors_reply_cycles_ = current_md_event_ors_reply_cycles_;
    }

  }

  // For Control Messages
  void ProcessEventsFromUDPDirectRead(char const *msg_ptr, int32_t msg_length, char seg_type, bool is_trade_exec_fd,
                                      bool is_spot_index_fd, bool is_oi_data_fd, uint32_t &udp_msg_seq_no) {
    if (msg_length < control_msg_pkt_size_) return;

    control_processor_->ProcessControlEvent((HFSAT::GenericControlRequestStruct *)msg_ptr);
  }

  inline bool CheckAndProcessControlMsg() {

    // has to be volatile, waiting on shared memory segment queue position
    volatile uint64_t queue_position_ = GetControlMsgQueuePos();
    // events are available only if the queue position at source is higher by 1, circular queue, will lag behind by 1
    // packet at start
    if (-1 == control_msg_index_) {
      control_msg_index_ = queue_position_;
    }

    if (control_msg_index_ == (int64_t)queue_position_) return false;

    // memcpy is only done as safegaurd from writer writing the same segment
    control_shm_queue_->Pop(control_mds_message_, control_msg_index_);
    control_msg_index_++;

    runtime_profiler_.Start(HFSAT::ProfilerType::kCOMBINEDSHMWRITER, 0);
    //    runtime_profiler_.End(HFSAT::ProfilerType::kCOMBINEDSHMWRITER, 0);
    //    runtime_profiler_.Start(HFSAT::ProfilerType::kTRADEINIT);

    control_processor_->ProcessControlEvent(&control_mds_message_);
    return true;
  }

  inline bool CheckAndProcessSignalMsg() {
    // has to be volatile, waiting on shared memory segment queue position
    volatile uint64_t queue_position_ = GetSignalMsgQueuePos();
    // events are available only if the queue position at source is higher by 1, circular queue, will lag behind by 1
    // packet at start
    if (-1 == signal_msg_index_) {
      signal_msg_index_ = queue_position_;
    }

    if (signal_msg_index_ == (int64_t)queue_position_) return false;

    // memcpy is only done as safegaurd from writer writing the same segment
    signal_shm_queue_->Pop(signal_mds_message_, signal_msg_index_);
    signal_msg_index_++;

    runtime_profiler_.Start(HFSAT::ProfilerType::kCOMBINEDSHMWRITER, 0);

    signal_processor_->ProcessSignalEvent(&signal_mds_message_);
    return true;
  }

  inline bool CheckAndProcessORSReply() {

    last_event_ors_reply_cycles_ = HFSAT::GetReadTimeStampCounter();

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
    //    runtime_profiler_.End(HFSAT::ProfilerType::kCOMBINEDSHMWRITER, 0);
    //    runtime_profiler_.Start(HFSAT::ProfilerType::kTRADEINIT);

    ors_reply_processor_->ProcessORSReplyEvent(&ors_reply_mds_message_);

    return true;
  }

  inline void OnEventsTimeout() {

    while (!oebu_mode && CheckAndProcessORSReply())
      ;
    control_read_trigger_++;

    //External Shm Based Signal Access
    if(true == use_signal_shm_){
      signal_read_trigger_++;
      if (SIGNAL_MSG_TRIGGER_COUNT == signal_read_trigger_) {
        while (CheckAndProcessSignalMsg())
          ;
        signal_read_trigger_ = 0;
      }
    }

    if (control_msg_trigger_count_ <= control_read_trigger_) {
      while (!oebu_mode && CheckAndProcessControlMsg())
        ;
      control_read_trigger_ = 0;
    }
  }

};
}
}
