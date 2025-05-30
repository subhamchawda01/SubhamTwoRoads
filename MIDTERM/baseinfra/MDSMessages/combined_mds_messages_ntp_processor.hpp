// =====================================================================================
//
//       Filename:  combined_mds_messages_ntp_source.cpp
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

class CombinedMDSMessagesNTPProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  NTPPriceLevelGlobalListener*
      p_price_level_global_listener_;  ///< Listeners of NTP messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;

 public:
  CombinedMDSMessagesNTPProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_price_level_global_listener_(NULL),
        p_time_keeper_(NULL) {}

  ~CombinedMDSMessagesNTPProcessor() {}

  inline void SetPriceLevelGlobalListener(NTPPriceLevelGlobalListener* p_new_listener_) {
    p_price_level_global_listener_ = p_new_listener_;
  }

  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline MktStatus_t GetMarketStatus(NTP_MDS::NTPCommonStruct* next_event) {
    switch (next_event->msg_) {
      case NTP_MDS::NTP_DELTA: {
        switch (next_event->data_.ntp_dels_.flags[1]) {
          case 2: {
            return kMktTradingStatusPause;
          } break;
          case 4: {
            return kMktTradingStatusClosed;
          } break;
          case 17: {
            return kMktTradingStatusOpen;
          } break;
          case 18: {
            return kMktTradingStatusForbidden;
          } break;
          case 20: {
            return kMktTradingStatusUnknown;
          } break;
          case 21: {
            return kMktTradingStatusReserved;
          } break;
          case 101: {
            return kMktTradingStatuFinalClosingCall;
          } break;
          default: { return kMktTradingStatusOpen; } break;
        }
        break;
      }
      case NTP_MDS::NTP_TRADE: {
        switch (next_event->data_.ntp_trds_.flags_[1]) {
          case 2: {
            return kMktTradingStatusPause;
          } break;
          case 4: {
            return kMktTradingStatusClosed;
          } break;
          case 17: {
            return kMktTradingStatusOpen;
          } break;
          case 18: {
            return kMktTradingStatusForbidden;
          } break;
          case 20: {
            return kMktTradingStatusUnknown;
          } break;
          case 21: {
            return kMktTradingStatusReserved;
          } break;
          case 101: {
            return kMktTradingStatuFinalClosingCall;
          } break;
          default: { return kMktTradingStatusOpen; } break;
        }
      } break;
      default: { return kMktTradingStatusOpen; }
    }
    return kMktTradingStatusOpen;
  }

  inline void ProcessNTPEvent(NTP_MDS::NTPCommonStruct* next_event) {
    switch (next_event->msg_) {
      case NTP_MDS::NTP_DELTA: {
        auto& delta = next_event->data_.ntp_dels_;
        int security_id = sec_name_indexer_.GetIdFromSecname(delta.contract_);

        if (security_id < 0) {
          break;
        }

        p_time_keeper_->OnTimeReceived(next_event->time_);

        if (delta.level_ > 0) {  // ignoring level 0 events right now

          MktStatus_t this_status = GetMarketStatus(next_event);
          p_price_level_global_listener_->OnMarketStatusUpdate(security_id, this_status);

          if (this_status != kMktTradingStatusOpen) {
            return;
          }

          TradeType_t buysell = TradeType_t(delta.type_ - '0');

          switch (delta.action_) {
            case 0: {
              p_price_level_global_listener_->OnPriceLevelNew(security_id, buysell, delta.level_, delta.price_,
                                                              delta.size_, delta.num_ords_, delta.intermediate_);
              break;
            }
            case 1: {
              p_price_level_global_listener_->OnPriceLevelChange(security_id, buysell, delta.level_, delta.price_,
                                                                 delta.size_, delta.num_ords_, delta.intermediate_);
              break;
            }
            case 2: {
              p_price_level_global_listener_->OnPriceLevelDelete(security_id, buysell, delta.level_, delta.price_,
                                                                 delta.intermediate_);
              break;
            }
            case 3: {  // one side of the book is to be cleared entirely
              p_price_level_global_listener_->OnPriceLevelDeleteThru(security_id, buysell, delta.intermediate_);
              break;
            }
            case 4: {  // needs price ?
              p_price_level_global_listener_->OnPriceLevelDeleteFrom(security_id, buysell, delta.level_, delta.price_,
                                                                     delta.intermediate_);
              break;
            }
          }
          break;
        }
      } break;
      case NTP_MDS::NTP_TRADE: {
        auto& trade = next_event->data_.ntp_trds_;
        int security_id = sec_name_indexer_.GetIdFromSecname(trade.contract_);

        if (security_id < 0) {
          break;
        }

        p_time_keeper_->OnTimeReceived(next_event->time_);
        MktStatus_t this_status = GetMarketStatus(next_event);
        p_price_level_global_listener_->OnMarketStatusUpdate(security_id, this_status);

        if (this_status != kMktTradingStatusOpen) {
          return;
        }

        if (next_event->data_.ntp_trds_.flags_[0] != 'X') {
          p_price_level_global_listener_->OnTrade(security_id, trade.trd_px_, trade.trd_qty_);
        }
        break;
      }

      case NTP_MDS::NTP_STATUS: {
        break;
      }

      case NTP_MDS::NTP_OPENPRICE: {
        break;
      }

      case NTP_MDS::NTP_IMBALANCE: {
        break;
      }

      default: {
        fprintf(stderr, "Weird message type in NTPLiveSource::ProcessAllEvents %d \n", (int)next_event->msg_);
      }
    }
  }
};
}
