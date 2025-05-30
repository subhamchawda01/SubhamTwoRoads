// =====================================================================================
//
//       Filename:  combined_mds_messages_hkex_processor.cpp
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

#include "baseinfra/MarketAdapter/indexed_hkomd_price_level_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/hkex_trade_time_manager.hpp"

namespace HFSAT {

class CombinedMDSMessagesHKOMDPFProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest
  PriceLevelGlobalListener* hkomd_price_level_global_listener_;
  ExternalTimeListener* p_time_keeper_;
  HkexTradeTimeManager& hkex_trade_time_manager_;

 public:
  CombinedMDSMessagesHKOMDPFProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        hkomd_price_level_global_listener_(NULL),
        p_time_keeper_(NULL),
        hkex_trade_time_manager_(HkexTradeTimeManager::GetUniqueInstance(_sec_name_indexer_))

  {}

  ~CombinedMDSMessagesHKOMDPFProcessor() {}

  inline void SetPriceLevelGlobalListener(PriceLevelGlobalListener* p_new_listener_) {
    hkomd_price_level_global_listener_ = p_new_listener_;
  }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline bool IsNormalTradeTime(int securityId, ttime_t tv) {
    return hkex_trade_time_manager_.isValidTimeToTrade(securityId, tv.tv_sec % 86400);
  }

  inline void ProcessHKOMDPFEvent(HKOMD_MDS::HKOMDPFCommonStruct* next_event_, struct timeval& _shm_writer_time_) {
    if (next_event_->getContract() == NULL) {
      DBGLOG_CLASS_FUNC << "Event with NULL contract received." << DBGLOG_ENDL_FLUSH;
      return;
    }
    int security_id_ = sec_name_indexer_.GetIdFromSecname(next_event_->getContract());
    if (0 > security_id_) return;

    switch (next_event_->msg_) {
      case HKOMD_MDS::HKOMD_PF_TRADE:
        // filter msgs which don't have type as BLANK
        {
          if (!IsNormalTradeTime(security_id_, _shm_writer_time_)) break;

          p_time_keeper_->OnTimeReceived(_shm_writer_time_);

          TradeType_t buysell_ = (next_event_->data_.trade_.side_ == 2) ? kTradeTypeBuy : kTradeTypeSell;
          hkomd_price_level_global_listener_->OnTrade(security_id_, next_event_->data_.trade_.price_,
                                                      next_event_->data_.trade_.quantity_, buysell_);
        }
        break;
      case HKOMD_MDS::HKOMD_PF_DELTA: {
        if (!IsNormalTradeTime(security_id_, _shm_writer_time_)) break;

        p_time_keeper_->OnTimeReceived(_shm_writer_time_);

        bool intermediate_ = false;
        TradeType_t buysell_ = kTradeTypeBuy;
        if (next_event_->data_.delta_.side_ == 0) {
          buysell_ = kTradeTypeBuy;
        } else if (next_event_->data_.delta_.side_ == 1) {
          buysell_ = kTradeTypeSell;
        } else {
          return;
        }
        if (next_event_->data_.delta_.intermediate_ == 'Y' || (int)next_event_->data_.delta_.intermediate_ == 1) {
          intermediate_ = true;
        }
        switch (next_event_->data_.delta_.action_) {
          case 0:  // new level
          {
            hkomd_price_level_global_listener_->OnPriceLevelNew(
                security_id_, buysell_, next_event_->data_.delta_.level_, next_event_->data_.delta_.price_,
                next_event_->data_.delta_.quantity_, next_event_->data_.delta_.num_orders_, intermediate_);

          } break;
          case 1:  // change
          {
            hkomd_price_level_global_listener_->OnPriceLevelChange(
                security_id_, buysell_, next_event_->data_.delta_.level_, next_event_->data_.delta_.price_,
                next_event_->data_.delta_.quantity_, next_event_->data_.delta_.num_orders_, intermediate_);
          } break;
          case 2: {
            // hkomd_price_level_global_listener_ -> OnOrderLevelDelete( security_id_,
            hkomd_price_level_global_listener_->OnPriceLevelDelete(security_id_, buysell_,
                                                                   next_event_->data_.delta_.level_,
                                                                   next_event_->data_.delta_.price_, intermediate_);

          } break;
          case 8: {
            // std::cerr << " RESET MESSAGE in HKOMDCPF @" << _shm_writer_time_ << " SecName:" <<
            // HFSAT::SecurityNameIndexer::GetUniqueInstance ( ).GetSecurityNameFromId ( security_id_ ) <<  "\n";
          } break;
          case 74: {
            std::cerr << " CLEAR Message in HKOMDCPF \n";
          } break;
          default: { std::cerr << " action not recognised for hkomd event\n"; } break;
        }
      } break;
      default: {
        dbglogger_ << "Weird msgtype " << next_event_->msg_ << " in " << __func__ << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        exit(-1);
      }
    }
  }
};
}
