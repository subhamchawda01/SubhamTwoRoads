// =====================================================================================
//
//       Filename:  combined_mds_messages_ose_pf_processor.cpp
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

#include "baseinfra/MarketAdapter/trade_time_manager.hpp"

namespace HFSAT {

class CombinedMDSMessagesOSEPFProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  PriceLevelGlobalListener*
      p_price_level_global_listener_;  ///< Listeners of EUREX_LS messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;
  HFSAT::TradeTimeManager& trade_time_manager_;

 public:
  CombinedMDSMessagesOSEPFProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_price_level_global_listener_(NULL),
        p_time_keeper_(NULL),
        trade_time_manager_(HFSAT::TradeTimeManager::GetUniqueInstance(_sec_name_indexer_))

  {}

  ~CombinedMDSMessagesOSEPFProcessor() {}

  inline void SetPriceLevelGlobalListener(PriceLevelGlobalListener* p_new_listener_) {
    p_price_level_global_listener_ = p_new_listener_;
  }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }
  inline bool IsNormalTradeTime(int security_id_, timeval tv_) {
    return trade_time_manager_.isValidTimeToTrade(security_id_, tv_.tv_sec % 86400);
  }

  inline void ProcessOSEPFEvent(OSE_MDS::OSEPriceFeedCommonStruct* next_event_, struct timeval& _shm_writer_time_) {
    int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->getContract());
    if (0 > security_id) return;

    switch (next_event_->type_) {
      case 0:
      case 1: {
        if (!IsNormalTradeTime(security_id, _shm_writer_time_)) break;

        if (OSE_MDS::PRICEFEED_TRADE_ORDER == next_event_->price_feed_msg_type_) break;

        p_time_keeper_->OnTimeReceived(_shm_writer_time_);

        TradeType_t _buysell_ = (next_event_->type_ == 0) ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;

        switch (next_event_->price_feed_msg_type_) {
          case OSE_MDS::PRICEFEED_NEW: {
            p_price_level_global_listener_->OnPriceLevelNew(security_id, _buysell_, next_event_->price_level_,
                                                            next_event_->price, next_event_->size,
                                                            next_event_->order_count_, false);

          } break;

          case OSE_MDS::PRICEFEED_CHANGE: {
            p_price_level_global_listener_->OnPriceLevelChange(security_id, _buysell_, next_event_->price_level_,
                                                               next_event_->price, next_event_->size,
                                                               next_event_->order_count_, false);

          } break;

          case OSE_MDS::PRICEFEED_DELETE: {
            p_price_level_global_listener_->OnPriceLevelDelete(security_id, _buysell_, next_event_->price_level_,
                                                               next_event_->price, false);

          } break;

          default: { } break; }

      } break;

      case 2: {
        if (!IsNormalTradeTime(security_id, _shm_writer_time_)) break;

        p_time_keeper_->OnTimeReceived(_shm_writer_time_);

        if (next_event_->price_level_ == 11 || next_event_->price_level_ == 12) {
          p_price_level_global_listener_->OnOffMarketTrade(security_id, next_event_->price, next_event_->size,
                                                           HFSAT::kTradeTypeNoInfo);
        } else {
          p_price_level_global_listener_->OnTrade(security_id, next_event_->price, next_event_->size,
                                                  HFSAT::kTradeTypeNoInfo);
        }

      } break;

      default: { } break; }
  }
};
}
