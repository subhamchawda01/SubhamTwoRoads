// =====================================================================================
//
//       Filename:  combined_mds_messages_nse_l1_processor.cpp
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

#include "baseinfra/MarketAdapter/generic_l1_data_market_view_manager.hpp"

namespace HFSAT {

class CombinedMDSMessagesNSEL1Processor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  L1DataListener* l1_data_listener_;
  ExternalTimeListener* p_time_keeper_;

 public:
  CombinedMDSMessagesNSEL1Processor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_), sec_name_indexer_(_sec_name_indexer_), l1_data_listener_(NULL), p_time_keeper_(NULL) {}

  ~CombinedMDSMessagesNSEL1Processor() {}

  inline void SetL1DataListener(L1DataListener* p_new_listener_) { l1_data_listener_ = p_new_listener_; }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline void ProcessNSEL1Event(HFSAT::GenericL1DataStruct* next_event_) {
    int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->getContract());
    if (0 > security_id) return;

    switch (next_event_->type) {
      case HFSAT::GenericL1DataType::L1_DELTA: {
        p_time_keeper_->OnTimeReceived(next_event_->time);
        l1_data_listener_->OnL1New(security_id, *next_event_);
      } break;
      case HFSAT::GenericL1DataType::L1_TRADE: {
        p_time_keeper_->OnTimeReceived(next_event_->time);
        l1_data_listener_->OnTrade(security_id, *next_event_);

      } break;
    }
  }
};
}
