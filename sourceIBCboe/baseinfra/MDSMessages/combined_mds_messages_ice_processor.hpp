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

class CombinedMDSMessagesICEProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  PriceLevelGlobalListener*
      p_price_level_global_listener_;  ///< Listeners of LIFFE messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;

 public:
  CombinedMDSMessagesICEProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_price_level_global_listener_(NULL),
        p_time_keeper_(NULL) {}

  ~CombinedMDSMessagesICEProcessor() {}

  inline void SetPriceLevelGlobalListener(PriceLevelGlobalListener* p_new_listener_) {
    p_price_level_global_listener_ = p_new_listener_;
  }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline void ProcessICEEvent(ICE_MDS::ICECommonStructLive* next_event_) {
    switch (next_event_->msg_) {
      case ICE_MDS::ICE_PL: {
        int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->contract_);

        // Check Security - Process Only Required Ones
        if (security_id < 0) return;

        p_time_keeper_->OnTimeReceived(next_event_->time_);

        if (next_event_->data_.ice_pls_.level_ > 0) {
          TradeType_t _buysell_ = next_event_->data_.ice_pls_.side_ == '1'
                                      ? kTradeTypeBuy
                                      : (next_event_->data_.ice_pls_.side_ == '2' ? kTradeTypeSell : kTradeTypeNoInfo);

          switch (next_event_->data_.ice_pls_.action_) {
            case '0': {
              p_price_level_global_listener_->OnPriceLevelNew(
                  security_id, _buysell_, next_event_->data_.ice_pls_.level_, next_event_->data_.ice_pls_.price_,
                  next_event_->data_.ice_pls_.size_, next_event_->data_.ice_pls_.orders_,
                  next_event_->data_.ice_pls_.intermediate_);
            } break;

            case '1': {
              p_price_level_global_listener_->OnPriceLevelChange(
                  security_id, _buysell_, next_event_->data_.ice_pls_.level_, next_event_->data_.ice_pls_.price_,
                  next_event_->data_.ice_pls_.size_, next_event_->data_.ice_pls_.orders_,
                  next_event_->data_.ice_pls_.intermediate_);
            } break;

            case '2': {
              p_price_level_global_listener_->OnPriceLevelDelete(
                  security_id, _buysell_, next_event_->data_.ice_pls_.level_, next_event_->data_.ice_pls_.price_,
                  next_event_->data_.ice_pls_.intermediate_);
            } break;

            default: {
              std::cerr << " Weird message type in CombinedMDSMessagesICEProcessor::ProcessICEEvent ICE_PL : "
                        << next_event_->data_.ice_pls_.action_ << "\n";
            } break;
          }
        }
      } break;

      case ICE_MDS::ICE_TRADE: {
        int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->contract_);

        if (security_id < 0) break;

        p_time_keeper_->OnTimeReceived(next_event_->time_);

        TradeType_t _buysell_ = next_event_->data_.ice_pls_.side_ == '1'
                                    ? kTradeTypeBuy
                                    : (next_event_->data_.ice_pls_.side_ == '2' ? kTradeTypeSell : kTradeTypeNoInfo);

        p_price_level_global_listener_->OnTrade(security_id, next_event_->data_.ice_trds_.price_,
                                                next_event_->data_.ice_trds_.size_, _buysell_);

      } break;

      case ICE_MDS::ICE_RESET_BEGIN: {
        int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->contract_);
        if (security_id < 0) break;

        p_time_keeper_->OnTimeReceived(next_event_->time_);
        p_price_level_global_listener_->OnResetBegin(security_id);
      } break;
      case ICE_MDS::ICE_RESET_END: {
        int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->contract_);
        if (security_id < 0) break;

        p_time_keeper_->OnTimeReceived(next_event_->time_);
        p_price_level_global_listener_->OnResetEnd(security_id);
      } break;

      default: {
        std::cerr << " Weird message type in CombinedMDSMessagesICEProcessor::ProcessICEEvent msg : "
                  << next_event_->msg_ << "\n";
      } break;
    }
  }
};
}
