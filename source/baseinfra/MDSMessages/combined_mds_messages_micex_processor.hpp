// =====================================================================================
//
//       Filename:  combined_mds_messages_micex_source.cpp
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
#include "dvccode/CommonTradeUtils/trade_time_manager.hpp"
#pragma once

namespace HFSAT {

class CombinedMDSMessagesMICEXProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  PriceLevelGlobalListener*
      p_price_level_global_listener_;  ///< Listeners of MICEX messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;
  HFSAT::TradeTimeManager& trade_time_manager_;

 public:
  CombinedMDSMessagesMICEXProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_time_keeper_(NULL),
        trade_time_manager_(HFSAT::TradeTimeManager::GetUniqueInstance(sec_name_indexer_))

  {}

  ~CombinedMDSMessagesMICEXProcessor() {}

  inline void SetPriceLevelGlobalListener(PriceLevelGlobalListener* p_new_listener_) {
    p_price_level_global_listener_ = p_new_listener_;
  }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline void ProcessMICEXEvent(MICEX_MDS::MICEXCommonStruct* next_event_) {
    switch (next_event_->msg_) {
      case MICEX_MDS::MICEX_DELTA: {
        int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->data_.micex_dels_.contract_);

        // Check Security - Process Only Required Ones
        if (security_id < 0) return;

        if (!trade_time_manager_.isValidTimeToTrade(security_id, next_event_->time_.tv_sec % 86400)) {
          return;
        }

        p_time_keeper_->OnTimeReceived(next_event_->time_);

        if (next_event_->data_.micex_dels_.level_ > 0) {
          TradeType_t _buysell_ = TradeType_t(next_event_->data_.micex_dels_.type_ - '0');

          switch (next_event_->data_.micex_dels_.action_) {
            case '0': {
              p_price_level_global_listener_->OnPriceLevelNew(
                  security_id, _buysell_, next_event_->data_.micex_dels_.level_, next_event_->data_.micex_dels_.price_,
                  next_event_->data_.micex_dels_.size_, next_event_->data_.micex_dels_.order_count_,
                  next_event_->data_.micex_dels_.intermediate_);
            } break;
            case '1': {
              p_price_level_global_listener_->OnPriceLevelChange(
                  security_id, _buysell_, next_event_->data_.micex_dels_.level_, next_event_->data_.micex_dels_.price_,
                  next_event_->data_.micex_dels_.size_, next_event_->data_.micex_dels_.order_count_,
                  next_event_->data_.micex_dels_.intermediate_);
            } break;
            case '2': {
              p_price_level_global_listener_->OnPriceLevelDelete(
                  security_id, _buysell_, next_event_->data_.micex_dels_.level_, next_event_->data_.micex_dels_.price_,
                  next_event_->data_.micex_dels_.intermediate_);
            } break;
            default: {
              std::cerr << " Weird message type in MICEXShmDataSource::ProcessAllEvents MICEX_DELTA : "
                        << next_event_->msg_ << "\n";
            } break;
          }
        }

      } break;

      case MICEX_MDS::MICEX_TRADE: {
        int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->data_.micex_trds_.contract_);

        // Check Security - Process Only Required Ones
        if (security_id < 0) return;

        p_time_keeper_->OnTimeReceived(next_event_->time_);

        TradeType_t _buysell_ =
            ((next_event_->data_.micex_trds_.agg_side_ == 1)
                 ? (kTradeTypeBuy)
                 : ((next_event_->data_.micex_trds_.agg_side_ == 2) ? kTradeTypeSell : kTradeTypeNoInfo));
        p_price_level_global_listener_->OnTrade(security_id, next_event_->data_.micex_trds_.trd_px_,
                                                next_event_->data_.micex_trds_.trd_qty_, _buysell_);

      } break;

      default: {
        std::cerr << " Weird message type in MICEXShmSource::ProcessAllEvents " << next_event_->msg_ << "\n";

      } break;
    }
  }
};
}
