// =====================================================================================
//
//       Filename:  combined_mds_messages_liffe_processor.cpp
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

#include "baseinfra/MarketAdapter/liffe_trade_time_manager.hpp"

namespace HFSAT {

class CombinedMDSMessagesLIFFEProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  PriceLevelGlobalListener*
      p_price_level_global_listener_;  ///< Listeners of LIFFE messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;
  LiffeTradeTimeManager& liffe_trade_time_manager_;

 public:
  CombinedMDSMessagesLIFFEProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_time_keeper_(NULL),
        liffe_trade_time_manager_(HFSAT::LiffeTradeTimeManager::GetUniqueInstance(_sec_name_indexer_))

  {}

  ~CombinedMDSMessagesLIFFEProcessor() {}

  inline void SetPriceLevelGlobalListener(PriceLevelGlobalListener* p_new_listener_) {
    p_price_level_global_listener_ = p_new_listener_;
  }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline bool IsNormalTradeTime(int security_id_, timeval tv_) {
    return liffe_trade_time_manager_.isValidTimeToTrade(security_id_, tv_.tv_sec % 86400);
  }

  inline void ProcessLIFFEEvent(LIFFE_MDS::LIFFECommonStruct* next_event_) {
    switch (next_event_->msg_) {
      case LIFFE_MDS::LIFFE_DELTA: {
        int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->data_.liffe_dels_.contract_);

        // Check Security - Process Only Required Ones
        if (security_id < 0) return;

        if (!IsNormalTradeTime(security_id, next_event_->time_)) break;
        p_time_keeper_->OnTimeReceived(next_event_->time_);

        TradeType_t _buysell_ = TradeType_t('2' - next_event_->data_.liffe_dels_.type_);

        switch (next_event_->data_.liffe_dels_.action_) {
          case '1': {  // NEW
            p_price_level_global_listener_->OnPriceLevelNew(
                security_id, _buysell_, next_event_->data_.liffe_dels_.level_, next_event_->data_.liffe_dels_.price_,
                next_event_->data_.liffe_dels_.size_, next_event_->data_.liffe_dels_.num_ords_,
                next_event_->data_.liffe_dels_.intermediate_);

          } break;
          case '2': {  // MODIFY
            p_price_level_global_listener_->OnPriceLevelChange(
                security_id, _buysell_, next_event_->data_.liffe_dels_.level_, next_event_->data_.liffe_dels_.price_,
                next_event_->data_.liffe_dels_.size_, next_event_->data_.liffe_dels_.num_ords_,
                next_event_->data_.liffe_dels_.intermediate_);

          } break;
          case '3': {  // DELETE
            p_price_level_global_listener_->OnPriceLevelDelete(
                security_id, _buysell_, next_event_->data_.liffe_dels_.level_, next_event_->data_.liffe_dels_.price_,
                next_event_->data_.liffe_dels_.intermediate_);
          } break;
          default: {
            std::cerr << " Weird message type in LIFFEShmDataSource::ProcessAllEvents LIFFE_DELTA : "
                      << next_event_->msg_ << "\n";
          } break;
        }

      } break;

      case LIFFE_MDS::LIFFE_TRADE: {
        int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->data_.liffe_dels_.contract_);

        if (security_id < 0) break;

        if (!IsNormalTradeTime(security_id, next_event_->time_)) break;
        p_time_keeper_->OnTimeReceived(next_event_->time_);

        // this will always fall into tradetype no info, liffe doesn't have agg_side into in the trade msg
        TradeType_t _buysell_ =
            ((next_event_->data_.liffe_trds_.agg_side_ == 'B')
                 ? (kTradeTypeBuy)
                 : ((next_event_->data_.liffe_trds_.agg_side_ == 'S') ? kTradeTypeSell : kTradeTypeNoInfo));

        p_price_level_global_listener_->OnTrade(security_id, next_event_->data_.liffe_trds_.trd_px_,
                                                next_event_->data_.liffe_trds_.trd_qty_, _buysell_);

      } break;

      default: {
        std::cerr << " Weird message type in LIFFEShmSource::ProcessAllEvents " << next_event_->msg_ << "\n";

      } break;
    }
  }
};
}
