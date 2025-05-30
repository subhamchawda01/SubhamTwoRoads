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

class CombinedMDSMessagesBMFEQProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  NTPPriceLevelGlobalListener*
      p_price_level_global_listener_;  ///< Listeners of NTP messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;

 public:
  CombinedMDSMessagesBMFEQProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_price_level_global_listener_(NULL),
        p_time_keeper_(NULL)

  {}

  ~CombinedMDSMessagesBMFEQProcessor() {}

  inline void SetPriceLevelGlobalListener(NTPPriceLevelGlobalListener* p_new_listener_) {
    p_price_level_global_listener_ = p_new_listener_;
  }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }
  inline MktStatus_t GetMarketStatus(NTP_MDS::NTPCommonStruct* _next_event_) {
    switch (_next_event_->msg_) {
      case NTP_MDS::NTP_DELTA: {
        switch (_next_event_->data_.ntp_dels_.flags[1]) {
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
      case NTP_MDS::NTP_TRADE: {
        switch (_next_event_->data_.ntp_trds_.flags_[1]) {
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

  inline void ProcessBMFEQEvent(NTP_MDS::NTPCommonStruct* next_event_) {
    switch (next_event_->msg_) {
      case NTP_MDS::NTP_DELTA: {
        int security_id_ = sec_name_indexer_.GetIdFromSecname(next_event_->data_.ntp_dels_.contract_);
        if (security_id_ < 0) break;

        p_time_keeper_->OnTimeReceived(next_event_->time_);
        if (next_event_->data_.ntp_dels_.level_ > 0) {  // ignoring level 0 events right now

          MktStatus_t this_status_ = GetMarketStatus(next_event_);
          p_price_level_global_listener_->OnMarketStatusUpdate(security_id_, this_status_);

          if (this_status_ != kMktTradingStatusOpen) {
            return;
          }

          TradeType_t _buysell_ = TradeType_t(next_event_->data_.ntp_dels_.type_ - '0');

          {
            switch (next_event_->data_.ntp_dels_.action_) {
              case 0: {
                p_price_level_global_listener_->OnPriceLevelNew(
                    security_id_, _buysell_, next_event_->data_.ntp_dels_.level_, next_event_->data_.ntp_dels_.price_,
                    next_event_->data_.ntp_dels_.size_, next_event_->data_.ntp_dels_.num_ords_,
                    next_event_->data_.ntp_dels_.intermediate_);
              } break;
              case 1: {
                p_price_level_global_listener_->OnPriceLevelChange(
                    security_id_, _buysell_, next_event_->data_.ntp_dels_.level_, next_event_->data_.ntp_dels_.price_,
                    next_event_->data_.ntp_dels_.size_, next_event_->data_.ntp_dels_.num_ords_,
                    next_event_->data_.ntp_dels_.intermediate_);
              } break;
              case 2: {
                p_price_level_global_listener_->OnPriceLevelDelete(
                    security_id_, _buysell_, next_event_->data_.ntp_dels_.level_, next_event_->data_.ntp_dels_.price_,
                    next_event_->data_.ntp_dels_.intermediate_);
              } break;
              case 3: {  // one side of the book is to be cleared entirely
                p_price_level_global_listener_->OnPriceLevelDeleteThru(security_id_, _buysell_,
                                                                       next_event_->data_.ntp_dels_.intermediate_);
              } break;
              case 4: {
                // needs price ?
                p_price_level_global_listener_->OnPriceLevelDeleteFrom(
                    security_id_, _buysell_, next_event_->data_.ntp_dels_.level_, next_event_->data_.ntp_dels_.price_,
                    next_event_->data_.ntp_dels_.intermediate_);
              } break;
            }
            break;
          }
        }
      } break;
      case NTP_MDS::NTP_TRADE: {
        int security_id_ = sec_name_indexer_.GetIdFromSecname(next_event_->data_.ntp_trds_.contract_);
        if (security_id_ < 0) break;

        p_time_keeper_->OnTimeReceived(next_event_->time_);
        MktStatus_t this_status_ = GetMarketStatus(next_event_);
        p_price_level_global_listener_->OnMarketStatusUpdate(security_id_, this_status_);
        if (this_status_ != kMktTradingStatusOpen) {
          return;
        }

        p_price_level_global_listener_->OnTrade(security_id_, next_event_->data_.ntp_trds_.trd_px_,
                                                next_event_->data_.ntp_trds_.trd_qty_);
      } break;

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
        fprintf(stderr, "Weird message type in NTPLiveSource::ProcessAllEvents %d \n", (int)next_event_->msg_);
      }
    }
  }
};
}
