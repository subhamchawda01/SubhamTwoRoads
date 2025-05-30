// =====================================================================================
//
//       Filename:  combined_mds_messages_eurex_source.cpp
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

class CombinedMDSMessagesEOBIPFProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  PriceLevelGlobalListener*
      p_price_level_global_listener_;  ///< Listeners of EOBIPF messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;

 public:
  CombinedMDSMessagesEOBIPFProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_time_keeper_(NULL)

  {}

  ~CombinedMDSMessagesEOBIPFProcessor() {}

  inline void SetPriceLevelGlobalListener(PriceLevelGlobalListener* p_new_listener_) {
    p_price_level_global_listener_ = p_new_listener_;
  }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline void ProcessEOBIPFEvent(EUREX_MDS::EUREXCommonStruct* next_event_) {
    switch (next_event_->msg_) {
      case EUREX_MDS::EUREX_DELTA: {
        int security_id_ = sec_name_indexer_.GetIdFromSecname((next_event_->data_).eurex_dels_.contract_);

        if (security_id_ < 0) {
          break;
        }

        p_time_keeper_->OnTimeReceived(next_event_->time_);

        if (next_event_->data_.eurex_dels_.level_ >= 0) {
          TradeType_t _buysell_ = TradeType_t('2' - next_event_->data_.eurex_dels_.type_);

          switch (next_event_->data_.eurex_dels_.action_) {
            case '1': {
              p_price_level_global_listener_->OnPriceLevelNew(
                  security_id_, _buysell_, next_event_->data_.eurex_dels_.level_, next_event_->data_.eurex_dels_.price_,
                  next_event_->data_.eurex_dels_.size_, next_event_->data_.eurex_dels_.num_ords_,
                  next_event_->data_.eurex_dels_.intermediate_);
            } break;
            case '2': {
              p_price_level_global_listener_->OnPriceLevelChange(
                  security_id_, _buysell_, next_event_->data_.eurex_dels_.level_, next_event_->data_.eurex_dels_.price_,
                  next_event_->data_.eurex_dels_.size_, next_event_->data_.eurex_dels_.num_ords_,
                  next_event_->data_.eurex_dels_.intermediate_);
            } break;
            case '3': {
              p_price_level_global_listener_->OnPriceLevelDelete(
                  security_id_, _buysell_, next_event_->data_.eurex_dels_.level_, next_event_->data_.eurex_dels_.price_,
                  next_event_->data_.eurex_dels_.intermediate_);
            } break;
            case '4': {
              p_price_level_global_listener_->OnPriceLevelDeleteFrom(security_id_, _buysell_,
                                                                     next_event_->data_.eurex_dels_.level_,
                                                                     next_event_->data_.eurex_dels_.intermediate_);
            } break;
            case '5': {
              p_price_level_global_listener_->OnPriceLevelDeleteThrough(security_id_, _buysell_,
                                                                        next_event_->data_.eurex_dels_.level_,
                                                                        next_event_->data_.eurex_dels_.intermediate_);
            } break;
            case '6': {
              p_price_level_global_listener_->OnPriceLevelOverlay(
                  security_id_, _buysell_, next_event_->data_.eurex_dels_.level_, next_event_->data_.eurex_dels_.price_,
                  next_event_->data_.eurex_dels_.size_, next_event_->data_.eurex_dels_.num_ords_,
                  next_event_->data_.eurex_dels_.intermediate_);
            } break;
            default: {
              fprintf(stderr, "Weird action type in EOBIPriceFeedLiveDataSource::_ProcessThisMsg EUREX_DELTA %c \n",
                      next_event_->data_.eurex_dels_.action_);
            } break;
          }
        }
      } break;
      case EUREX_MDS::EUREX_TRADE: {
        int security_id_ = sec_name_indexer_.GetIdFromSecname(next_event_->data_.eurex_trds_.contract_);

        if (security_id_ < 0) {
          break;
        }

        p_time_keeper_->OnTimeReceived(next_event_->time_);

        TradeType_t _buysell_ =
            ((next_event_->data_.eurex_trds_.agg_side_ == '2')
                 ? (kTradeTypeBuy)
                 : ((next_event_->data_.eurex_trds_.agg_side_ == '1') ? kTradeTypeSell : kTradeTypeNoInfo));

        p_price_level_global_listener_->OnTrade(security_id_, next_event_->data_.eurex_trds_.trd_px_,
                                                next_event_->data_.eurex_trds_.trd_qty_, _buysell_);
      } break;
      default: {
        fprintf(stderr, "Weird message type in EOBIPriceFeedLiveDataSource::_ProcessThisMsg %d \n",
                (int)next_event_->msg_);
      } break;
    }
  }
};
}
