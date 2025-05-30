// =====================================================================================
//
//       Filename:  combined_mds_messages_ose_l1_processor.cpp
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

#include "baseinfra/MarketAdapter/ose_l1_price_market_view_manager.hpp"
#include "dvccode/CommonTradeUtils/trade_time_manager.hpp"

namespace HFSAT {

class CombinedMDSMessagesOSEL1Processor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  FullBookGlobalListener* p_fullbook_listener_;  ///< Listeners of EUREX_LS messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;
  HFSAT::TradeTimeManager& trade_time_manager_;

 public:
  CombinedMDSMessagesOSEL1Processor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_time_keeper_(NULL),
        trade_time_manager_(HFSAT::TradeTimeManager::GetUniqueInstance(_sec_name_indexer_))

  {}

  ~CombinedMDSMessagesOSEL1Processor() {}

  inline void SetFullBookGlobalListener(FullBookGlobalListener* p_new_listener_) {
    p_fullbook_listener_ = p_new_listener_;
  }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline bool IsNormalTradeTime(int security_id_, timeval tv_) {
    return trade_time_manager_.isValidTimeToTrade(security_id_, tv_.tv_sec % 86400);
  }

  inline void ProcessOSEL1Event(OSE_MDS::OSEPLCommonStruct* next_event_, struct timeval& _shm_writer_time_) {
    int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->getContract());
    if (0 > security_id) return;

    switch (next_event_->get_buy_sell_trade()) {
      case OSE_MDS::kL1BUY: {
        if (!IsNormalTradeTime(security_id, next_event_->time_)) break;
        p_time_keeper_->OnTimeReceived(next_event_->time_);
        p_fullbook_listener_->OnL1Change(security_id, kTradeTypeBuy, next_event_->price, next_event_->size,
                                         next_event_->order_count_, false);
      } break;
      case OSE_MDS::kL1SELL: {
        if (!IsNormalTradeTime(security_id, next_event_->time_)) break;
        p_time_keeper_->OnTimeReceived(next_event_->time_);
        p_fullbook_listener_->OnL1Change(security_id, kTradeTypeSell, next_event_->price, next_event_->size,
                                         next_event_->order_count_, false);
      } break;

      case OSE_MDS::kL1TRADE: {
        if (!IsNormalTradeTime(security_id, next_event_->time_)) break;
        p_time_keeper_->OnTimeReceived(next_event_->time_);
        p_fullbook_listener_->OnTrade(security_id, next_event_->price, next_event_->size, kTradeTypeNoInfo);
      } break;
    }
  }
};
}
