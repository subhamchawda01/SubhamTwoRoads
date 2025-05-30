// =====================================================================================
//
//       Filename:  combined_mds_messages_ose_cf_processor.cpp
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

class CombinedMDSMessagesOSECFProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  PriceLevelGlobalListener* p_price_level_global_listener_;  ///< Listeners of OSE PriceFeed from OSE CF Feed messages
  /// in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;

  HFSAT::TradeTimeManager& trade_time_manager_;

 public:
  CombinedMDSMessagesOSECFProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_price_level_global_listener_(NULL),
        p_time_keeper_(NULL),
        trade_time_manager_(HFSAT::TradeTimeManager::GetUniqueInstance(_sec_name_indexer_)) {}

  ~CombinedMDSMessagesOSECFProcessor() {}

  inline void SetPriceLevelGlobalListener(PriceLevelGlobalListener* p_new_listener_) {
    p_price_level_global_listener_ = p_new_listener_;
  }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline bool IsNormalTradeTime(int security_id_, timeval tv_) {
    return trade_time_manager_.isValidTimeToTrade(security_id_, tv_.tv_sec % 86400);
  }

  inline void ProcessOSECFEvent(OSE_MDS::OSECombinedCommonStruct* next_event_, struct timeval& _shm_writer_time_) {
    // Process PriceFede Only And Trade
    if (!next_event_->is_pricefeed_) return;

    int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->getContract());
    if (0 > security_id) {
      return;
    }

    if (!IsNormalTradeTime(security_id, _shm_writer_time_)) {
      return;
    }

    switch (next_event_->feed_msg_type_) {
      case OSE_MDS::FEED_NEW:
      case OSE_MDS::FEED_DELETE:
      case OSE_MDS::FEED_CHANGE: {
        p_time_keeper_->OnTimeReceived(_shm_writer_time_);

        TradeType_t _buysell_ = (next_event_->type_ == 1) ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;

        switch (next_event_->feed_msg_type_) {
          case OSE_MDS::FEED_NEW: {
            p_price_level_global_listener_->OnPriceLevelNew(security_id, _buysell_, next_event_->price_feed_level_,
                                                            next_event_->price_, next_event_->agg_size_,
                                                            next_event_->order_count_, false);

          } break;

          case OSE_MDS::FEED_CHANGE: {
            p_price_level_global_listener_->OnPriceLevelChange(security_id, _buysell_, next_event_->price_feed_level_,
                                                               next_event_->price_, next_event_->agg_size_,
                                                               next_event_->order_count_, false);

          } break;

          case OSE_MDS::FEED_DELETE: {
            p_price_level_global_listener_->OnPriceLevelDelete(security_id, _buysell_, next_event_->price_feed_level_,
                                                               next_event_->price_, false);

          } break;

          default: { } break; }

      } break;

      case OSE_MDS::FEED_TRADE: {
        p_time_keeper_->OnTimeReceived(_shm_writer_time_);

        if (next_event_->price_feed_level_ == 11 || next_event_->price_feed_level_ == 12) {
          p_price_level_global_listener_->OnOffMarketTrade(security_id, next_event_->price_, next_event_->agg_size_,
                                                           HFSAT::kTradeTypeNoInfo);
        } else {
          p_price_level_global_listener_->OnTrade(security_id, next_event_->price_, next_event_->agg_size_,
                                                  HFSAT::kTradeTypeNoInfo);
        }

      } break;

      default: { } break; }
  }
};
}
