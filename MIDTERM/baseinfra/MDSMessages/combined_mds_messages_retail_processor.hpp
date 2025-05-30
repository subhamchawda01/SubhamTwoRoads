// =====================================================================================
//
//       Filename:  combined_mds_messages_retail_processor.cpp
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

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/LiveSources/retail_trade_listener.hpp"

namespace HFSAT {

class CombinedMDSMessagesRetailProcessor {
 protected:
  DebugLogger& dbglogger_;  ///< error logger
  std::map<std::string, std::vector<RetailTradeListener*> > ret_trd_listener_vec_map_;
  ExternalTimeListener* p_time_keeper_;

  CombinedMDSMessagesRetailProcessor(DebugLogger& _dbglogger_, ExternalTimeListener* _p_time_keeper_)
      : dbglogger_(_dbglogger_), p_time_keeper_(_p_time_keeper_) {}

  CombinedMDSMessagesRetailProcessor(CombinedMDSMessagesRetailProcessor const& copy);
  CombinedMDSMessagesRetailProcessor& operator=(CombinedMDSMessagesRetailProcessor const& copy);

  ~CombinedMDSMessagesRetailProcessor() {}

 public:
  static CombinedMDSMessagesRetailProcessor* GetUniqueInstance(DebugLogger& _dbglogger_,
                                                               ExternalTimeListener* _p_time_keeper_) {
    static CombinedMDSMessagesRetailProcessor p_uniqueinstance_(_dbglogger_, _p_time_keeper_);
    return &p_uniqueinstance_;
  }

  inline void AddRetailTradeListener(const std::string& _secname_, RetailTradeListener* _new_listener_) {
    DBGLOG_CLASS_FUNC << _secname_ << DBGLOG_ENDL_FLUSH;
    if (VectorUtils::UniqueVectorAdd(ret_trd_listener_vec_map_[_secname_], _new_listener_)) {
      DBGLOG_CLASS_FUNC << "New RetailTradeListener added for " << _secname_ << ". Total # of Listeners "
                        << ret_trd_listener_vec_map_[_secname_].size() << DBGLOG_ENDL_FLUSH;
    }
  }

  inline void ProcessRetailEvent(RETAIL_MDS::RETAILCommonStruct* next_event_) {
    if (next_event_ == NULL) {
      DBGLOG_CLASS_FUNC << "Error: The RETAILCommonStruct received is NULL." << DBGLOG_ENDL_FLUSH;
      return;
    }

    switch (next_event_->msg_) {
      case RETAIL_MDS::RETAIL_TRADE: {
        p_time_keeper_->OnTimeReceived(next_event_->time_);
        DBGLOG_CLASS_FUNC << next_event_->ToString() << DBGLOG_ENDL_FLUSH;

        std::string t_symbol_str_ = std::string(next_event_->data_.retail_trds_.contract_);
        // TODO: should we break spd/fly trades into outrights and notify outright listeners as well ?
        if (ret_trd_listener_vec_map_.find(t_symbol_str_) != ret_trd_listener_vec_map_.end()) {
          std::vector<RetailTradeListener*>& order_executed_listener_vec_ = ret_trd_listener_vec_map_[t_symbol_str_];
          DBGLOG_CLASS_FUNC << "Order Executed for " << t_symbol_str_
                            << ". Distributing to listeners : " << order_executed_listener_vec_.size()
                            << DBGLOG_ENDL_FLUSH;

          for (size_t i = 0; i < order_executed_listener_vec_.size(); i++) {
            order_executed_listener_vec_[i]->OnRetailTradeRequest(
                next_event_->data_.retail_trds_.contract_, next_event_->data_.retail_trds_.trd_px_,
                next_event_->data_.retail_trds_.agg_side_ == 'B' ? kTradeTypeBuy : kTradeTypeSell,
                next_event_->data_.retail_trds_.trd_qty_, next_event_->data_.retail_trds_.quoted_qty_,
                next_event_->data_.retail_trds_.requested_qty_);
          }
        }
      } break;

      default: { } break; }
  }
};
}
