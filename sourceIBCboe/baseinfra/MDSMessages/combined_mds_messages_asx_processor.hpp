/**
   \file MDSMessages/combined_mds_messages_tmx_processor.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#pragma once

#include "baseinfra/MDSMessages/mds_message_listener.hpp"

namespace HFSAT {

class CombinedMDSMessagesASXProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  PriceLevelGlobalListener*
      p_price_level_global_listener_;  ///< Listeners of LIFFE messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;

 public:
  CombinedMDSMessagesASXProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_price_level_global_listener_(NULL),
        p_time_keeper_(NULL) {}

  ~CombinedMDSMessagesASXProcessor() {}

  inline void SetPriceLevelGlobalListener(PriceLevelGlobalListener* p_new_listener_) {
    p_price_level_global_listener_ = p_new_listener_;
  }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline void ProcessASXEvent(ASX_MDS::ASXPFCommonStruct* next_event_) {
    switch (next_event_->msg_) {
      case ASX_MDS::ASX_PF_DELTA: {
        int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->data_.delta_.contract_);

        // Check Security - Process Only Required Ones
        if (security_id < 0) return;

        p_time_keeper_->OnTimeReceived(next_event_->time_);

        if (next_event_->data_.delta_.level_ > 0) {
          TradeType_t _buysell_ = next_event_->data_.delta_.side_ == 'B'
                                      ? kTradeTypeBuy
                                      : (next_event_->data_.delta_.side_ == 'S' ? kTradeTypeSell : kTradeTypeNoInfo);

          switch (next_event_->data_.delta_.action_) {
            case '0': {
              p_price_level_global_listener_->OnPriceLevelNew(
                  security_id, _buysell_, next_event_->data_.delta_.level_, next_event_->data_.delta_.price_,
                  next_event_->data_.delta_.quantity_, next_event_->data_.delta_.num_orders_,
                  next_event_->data_.delta_.intermediate_);
            } break;

            case '1': {
              p_price_level_global_listener_->OnPriceLevelChange(
                  security_id, _buysell_, next_event_->data_.delta_.level_, next_event_->data_.delta_.price_,
                  next_event_->data_.delta_.quantity_, next_event_->data_.delta_.num_orders_,
                  next_event_->data_.delta_.intermediate_);
            } break;

            case '2': {
              p_price_level_global_listener_->OnPriceLevelDelete(
                  security_id, _buysell_, next_event_->data_.delta_.level_, next_event_->data_.delta_.price_,
                  next_event_->data_.delta_.intermediate_);
            } break;

            default: {
              std::cerr << " Weird message type in CombinedMDSMessagesASXProcessor::ProcessASXEvent ASX_PL : "
                        << next_event_->data_.delta_.action_ << "\n";
            } break;
          }
        }
      } break;

      case ASX_MDS::ASX_PF_TRADE: {
        int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->data_.trade_.contract_);

        if (security_id < 0) break;

        p_time_keeper_->OnTimeReceived(next_event_->time_);

        TradeType_t _buysell_ = next_event_->data_.trade_.side_ == 'B'
                                    ? kTradeTypeBuy
                                    : (next_event_->data_.trade_.side_ == 'S' ? kTradeTypeSell : kTradeTypeNoInfo);

        p_price_level_global_listener_->OnTrade(security_id, next_event_->data_.trade_.price_,
                                                next_event_->data_.trade_.quantity_, _buysell_);

      } break;

      case ASX_MDS::ASX_PF_RESET_BEGIN: {
        int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->data_.reset_.contract_);
        if (security_id < 0) break;

        p_time_keeper_->OnTimeReceived(next_event_->time_);
        p_price_level_global_listener_->OnResetBegin(security_id);
      } break;
      case ASX_MDS::ASX_PF_RESET_END: {
        int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->data_.reset_.contract_);
        if (security_id < 0) break;

        p_time_keeper_->OnTimeReceived(next_event_->time_);
        p_price_level_global_listener_->OnResetEnd(security_id);
      } break;

      default: {
        std::cerr << " Weird message type in CombinedMDSMessagesASXProcessor::ProcessASXEvent msg : "
                  << next_event_->msg_ << "\n";
      } break;
    }
  }
};
}
