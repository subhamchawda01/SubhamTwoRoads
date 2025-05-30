// =====================================================================================
//
//       Filename:  combined_mds_messages_bmf_fpga_processor.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/30/2014 01:06:07 PM
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

#include <set>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/simple_security_name_indexer.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"
#include "dvccode/Utils/shm_reader.hpp"

namespace HFSAT {

class CombinedMDSMessagesBMFFPGAProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  BMFFPGAFullBookGlobalListener* fpga_global_listener_;
  ExternalTimeListener* p_time_keeper_;

  HFSAT::RuntimeProfiler& runtime_profiler_;
  SHM::ShmReader<FPGA_MDS::BMFFPGACommonStruct>* shm_reader_;

 public:
  CombinedMDSMessagesBMFFPGAProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        fpga_global_listener_(nullptr),
        p_time_keeper_(nullptr),
        runtime_profiler_(HFSAT::RuntimeProfiler::GetUniqueInstance()),
        shm_reader_(nullptr) {
    shm_reader_ = new SHM::ShmReader<FPGA_MDS::BMFFPGACommonStruct>(SHM_KEY_BMF_FPGA, BMF_FPGA_SHM_QUEUE_SIZE);
  }

  ~CombinedMDSMessagesBMFFPGAProcessor() {}

  inline void SetFPGAGlobalListener(BMFFPGAFullBookGlobalListener* listener_) { fpga_global_listener_ = listener_; }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline MktStatus_t GetMarketStatus(int trading_status) {
    switch (trading_status) {
      case 2: {
        return kMktTradingStatusPause;
      } break;
      case 4: {
        return kMktTradingStatusClosed;
      } break;
      case 17: {
        return kMktTradingStatusOpen;
      } break;
      case 18: {
        return kMktTradingStatusForbidden;
      } break;
      case 20: {
        return kMktTradingStatusUnknown;
      } break;
      case 21: {
        return kMktTradingStatusReserved;
      } break;
      case 101: {
        return kMktTradingStatuFinalClosingCall;
      } break;
      default: { return kMktTradingStatusOpen; } break;
    }
    return kMktTradingStatusOpen;
  }

  inline void ProcessBMFFPGAEvents(bool _keep_in_loop_ = true) {
    int security_id;

    while (true) {
      // Make FPGA API call and process messages
      FPGA_MDS::BMFFPGACommonStruct* event = shm_reader_->ReadNextEvent();

      // No data
      if (event == NULL) {
        break;
      }

      security_id = sec_name_indexer_.GetIdFromSecname(event->getContract());

      runtime_profiler_.Start(HFSAT::ProfilerType::kCOMBINEDSHMWRITER);
      runtime_profiler_.End(HFSAT::ProfilerType::kCOMBINEDSHMWRITER);
      runtime_profiler_.Start(HFSAT::ProfilerType::kTRADEINIT);

      if (security_id < 0) {
        continue;
      }

      p_time_keeper_->OnTimeReceived(event->time_);

      switch (event->msg_) {
        case FPGA_MDS::BMF_FPGA_BOOK: {
          fpga_global_listener_->OnFullBookChange(security_id, &event->data_.fpga_dels_, false, event->is_closed_);
        } break;

        case FPGA_MDS::BMF_FPGA_TRADE: {
          fpga_global_listener_->OnTrade(security_id, event->data_.fpga_trds_.price, event->data_.fpga_trds_.size,
                                         HFSAT::kTradeTypeNoInfo, event->is_closed_);
        } break;

        case FPGA_MDS::BMF_FPGA_TRADING_STATUS: {
          MktStatus_t status = GetMarketStatus(event->data_.fpga_status_.status);
          fpga_global_listener_->OnMarketStatusUpdate(security_id, status);
        } break;

        default: {
          std::cerr << __func__ << " : " << __LINE__ << " Invalid message in BMF FPGA shm processor: " << std::endl;
          break;
        }
      }

      if (!_keep_in_loop_) break;
    }
  }
};
}
