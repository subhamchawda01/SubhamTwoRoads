// =====================================================================================
//
//       Filename:  combined_mds_messages_csm_processor.cpp
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

namespace HFSAT {

class CombinedMDSMessagesCFEProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  CFEPriceLevelGlobalListener*
      p_price_level_global_listener_;  ///< Listeners of CSM messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;

 public:
  CombinedMDSMessagesCFEProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_time_keeper_(NULL)

  {}

  ~CombinedMDSMessagesCFEProcessor() {}

  inline void SetCFEPriceLevelGlobalListener(CFEPriceLevelGlobalListener* p_new_listener_) {
    p_price_level_global_listener_ = p_new_listener_;
  }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline void ProcessCSMEvent(CSM_MDS::CSMCommonStruct* next_event_) {
    int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->contract_);

    if (security_id < 0) return;

    p_time_keeper_->OnTimeReceived(next_event_->time_);

    switch (next_event_->msg_) {
      case CSM_MDS::CSM_DELTA: {
        TradeType_t buysell_ = next_event_->data_.csm_dels_.type_ == '0' ? kTradeTypeBuy : kTradeTypeSell;

        switch (next_event_->data_.csm_dels_.action_) {
          case 0: {
            p_price_level_global_listener_->OnPriceLevelNew(
                security_id, buysell_, next_event_->data_.csm_dels_.level_, next_event_->data_.csm_dels_.price_,
                next_event_->data_.csm_dels_.size_[0], 1, next_event_->data_.csm_dels_.intermediate_);
          } break;
          case 1: {
            p_price_level_global_listener_->OnPriceLevelChange(
                security_id, buysell_, next_event_->data_.csm_dels_.level_, next_event_->data_.csm_dels_.price_,
                next_event_->data_.csm_dels_.size_[0], 1, next_event_->data_.csm_dels_.intermediate_);
          } break;
          case 2: {
            p_price_level_global_listener_->OnPriceLevelDelete(
                security_id, buysell_, next_event_->data_.csm_dels_.level_, next_event_->data_.csm_dels_.price_,
                next_event_->data_.csm_dels_.intermediate_);
          } break;
          case 5: {
            p_price_level_global_listener_->OnPriceLevelOverlay(
                security_id, buysell_, next_event_->data_.csm_dels_.level_, next_event_->data_.csm_dels_.price_,
                next_event_->data_.csm_dels_.size_[0], 1, next_event_->data_.csm_dels_.intermediate_);
          } break;
          case 9: {
          } break;
          default:
            break;
        }
      } break;

      case CSM_MDS::CSM_TRADE: {
        double trade_price = next_event_->data_.csm_trds_.trd_px_;

        // Spread trade
        if (next_event_->data_.csm_trds_.trade_condition[0] == 'S') {
          p_price_level_global_listener_->OnSpreadTrade(security_id, trade_price,
                                                        next_event_->data_.csm_trds_.trd_qty_);
        } else {
          p_price_level_global_listener_->OnTrade(security_id, trade_price, next_event_->data_.csm_trds_.trd_qty_);
        }
      } break;

      case CSM_MDS::CSM_TOB: {
        if (next_event_->data_.csm_dels_.size_[0] == 0 && next_event_->data_.csm_dels_.intermediate_) {
          return;
        }

        TradeType_t buysell_ = next_event_->data_.csm_dels_.type_ == '0' ? kTradeTypeBuy : kTradeTypeSell;
        p_price_level_global_listener_->OnPriceLevelChange(
            security_id, buysell_, next_event_->data_.csm_dels_.level_, next_event_->data_.csm_dels_.price_,
            next_event_->data_.csm_dels_.size_[0], 1, next_event_->data_.csm_dels_.intermediate_);
      } break;

      default: { } break; }
  }
};
}
