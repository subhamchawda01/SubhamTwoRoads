/**
   \file MDSMessages/combined_mds_messages_tmx_obf_processor.hpp

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

class CombinedMDSMessagesTMXOBFProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  PriceLevelGlobalListener*
      p_price_level_global_listener_;  ///< Listeners of LIFFE messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;

 public:
  CombinedMDSMessagesTMXOBFProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_price_level_global_listener_(NULL),
        p_time_keeper_(NULL) {}

  ~CombinedMDSMessagesTMXOBFProcessor() {}

  inline void SetPriceLevelGlobalListener(PriceLevelGlobalListener* p_new_listener_) {
    p_price_level_global_listener_ = p_new_listener_;
  }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline void ProcessTMXOBFEvent(TMX_OBF_MDS::TMXPFCommonStruct* next_event_) {
    switch (next_event_->msg_) {
      case TMX_OBF_MDS::TMXPriceFeedmsgType::TMX_PF_DELTA: {
        int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->contract_);

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

      case TMX_OBF_MDS::TMXPriceFeedmsgType::TMX_PF_TRADE: {
        int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->contract_);

        if (security_id < 0) break;

        p_time_keeper_->OnTimeReceived(next_event_->time_);

        TradeType_t _buysell_ = next_event_->data_.trade_.side_ == 'B'
                                    ? kTradeTypeBuy
                                    : (next_event_->data_.trade_.side_ == 'S' ? kTradeTypeSell : kTradeTypeNoInfo);

        p_price_level_global_listener_->OnTrade(security_id, next_event_->data_.trade_.price_,
                                                next_event_->data_.trade_.quantity_, _buysell_);

      } break;

      default: {
        std::cerr << " Weird message type in CombinedMDSMessagesTMXOBFProcessor::ProcessASXEvent msg : "
                  << static_cast<int>(next_event_->msg_) << "\n";
      } break;
    }
  }
};
}
