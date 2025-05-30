// =====================================================================================
//
//       Filename:  combined_mds_messages_ors_reply_processor.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/04/2014 06:29:36 AM
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"

#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"

#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/ORSMessages/ors_message_listener.hpp"

namespace HFSAT {
namespace ProORSReply {
class CombinedMDSMessagesProShmORSReplyProcessor {
 private:
  bool is_filtering_enabled_;
  std::set<int32_t> ors_base_set_;

 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;

  //@Import : Only brought here to map a correct mktprice from the security id, the vector holds a key as security id
  // and points to an SMV object
  HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_;

  std::vector<std::vector<OrderNotFoundListener*> > order_not_found_listener_vec_map_;  ///< map (vector) from
  /// security_id to vector of
  /// listeners to ORS messages of
  /// type kORRType_None
  std::vector<std::vector<OrderSequencedListener*> > order_sequenced_listener_vec_map_;  ///< map (vector) from
  /// security_id to vector of
  /// listeners to ORS messages of
  /// type kORRType_Seqd
  std::vector<std::vector<OrderConfirmedListener*> > order_confirmed_listener_vec_map_;  ///< map (vector) from
  /// security_id to vector of
  /// listeners to ORS messages of
  /// type kORRType_Conf
  std::vector<std::vector<OrderConfCxlReplaceRejectListener*> > order_conf_cxlreplace_reject_listener_vec_map_;

  std::vector<std::vector<OrderConfCxlReplacedListener*> > order_conf_cxlreplaced_listener_vec_map_;  ///< map (vector)
  /// from
  /// security_id to
  /// vector of
  /// listeners to
  /// ORS messages of
  /// type
  /// kORRType_CxRe
  std::vector<std::vector<OrderCxlSeqdListener*> > order_cxl_sequenced_listener_vec_map_;  ///< map (vector) from
  /// security_id to vector of
  /// listeners to ORS messages of
  /// type kORRType_CxlSeqd
  std::vector<std::vector<OrderCanceledListener*> > order_canceled_listener_vec_map_;  ///< map (vector) from
  /// security_id to vector of
  /// listeners to ORS messages of
  /// type kORRType_Cxld
  std::vector<std::vector<OrderExecutedListener*> > order_executed_listener_vec_map_;  ///< map (vector) from
  /// security_id to vector of
  /// listeners to ORS messages of
  /// type kORRType_Exec
  std::vector<std::vector<OrderRejectedListener*> > order_rejected_listener_vec_map_;  ///< map (vector) from
  /// security_id to vector of
  /// listeners to ORS messages of
  /// type kORRType_Rejc
  std::vector<std::vector<OrderRejectedDueToFundsListener*> >
      order_rejected_due_to_funds_listener_vec_map_;  ///< vector of listeners to ORS messages of type
  /// kORRType_Rejc_Funds
  std::vector<std::vector<OrderInternallyMatchedListener*> >
      order_internally_matched_listener_vec_map_;  ///< map (vector) from security_id to vector of listeners to ORS
  /// messages of type kORRType_IntExec

  ExternalTimeListener* p_time_keeper_;  ///< only meant for Watch
  bool using_shm_for_ors_reply_;

 public:
  /**
   * @param _dbglogger_ for logging errors
   * @param _sec_name_indexer_ to detect if the security is of interest and not to process if not
   * @param md_udp_ip the braodcast ip
   * @param md_udp_port the braodcast port
   */
  //@Initialization : The smv list is populated from the base_tradeinit.cpp
  CombinedMDSMessagesProShmORSReplyProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_,
                                             HFSAT::SecurityMarketViewPtrVec& _sid_to_smv_ptr_map_)
      : is_filtering_enabled_(false),
        ors_base_set_(),
        dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        sid_to_smv_ptr_map_(_sid_to_smv_ptr_map_),
        order_not_found_listener_vec_map_(_sec_name_indexer_.NumSecurityId(), std::vector<OrderNotFoundListener*>()),
        order_sequenced_listener_vec_map_(_sec_name_indexer_.NumSecurityId(), std::vector<OrderSequencedListener*>()),
        order_confirmed_listener_vec_map_(_sec_name_indexer_.NumSecurityId(), std::vector<OrderConfirmedListener*>()),
        order_conf_cxlreplace_reject_listener_vec_map_(_sec_name_indexer_.NumSecurityId(),
                                                       std::vector<OrderConfCxlReplaceRejectListener*>()),
        order_conf_cxlreplaced_listener_vec_map_(_sec_name_indexer_.NumSecurityId(),
                                                 std::vector<OrderConfCxlReplacedListener*>()),
        order_cxl_sequenced_listener_vec_map_(_sec_name_indexer_.NumSecurityId(), std::vector<OrderCxlSeqdListener*>()),
        order_canceled_listener_vec_map_(_sec_name_indexer_.NumSecurityId(), std::vector<OrderCanceledListener*>()),
        order_executed_listener_vec_map_(_sec_name_indexer_.NumSecurityId(), std::vector<OrderExecutedListener*>()),
        order_rejected_listener_vec_map_(_sec_name_indexer_.NumSecurityId(), std::vector<OrderRejectedListener*>()),
        order_rejected_due_to_funds_listener_vec_map_(_sec_name_indexer_.NumSecurityId(),
                                                      std::vector<OrderRejectedDueToFundsListener*>()),
        order_internally_matched_listener_vec_map_(_sec_name_indexer_.NumSecurityId(),
                                                   std::vector<OrderInternallyMatchedListener*>()),
        p_time_keeper_(NULL),
        using_shm_for_ors_reply_(HFSAT::UseShmforORSReply())

  {}

  ~CombinedMDSMessagesProShmORSReplyProcessor() {}

  inline void AddOrderNotFoundListener(const unsigned int _security_id_, OrderNotFoundListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_not_found_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void RemoveOrderNotFoundListener(const unsigned int _security_id_, OrderNotFoundListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_not_found_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void AddOrderSequencedListener(const unsigned int _security_id_, OrderSequencedListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_sequenced_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void RemOrderSequencedListener(const unsigned int _security_id_, OrderSequencedListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_sequenced_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void AddOrderConfirmedListener(const unsigned int _security_id_, OrderConfirmedListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_confirmed_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void RemoveOrderConfirmedListener(const unsigned int _security_id_, OrderConfirmedListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_confirmed_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void AddOrderConfCxlReplaceRejectListener(const unsigned int security_id,
                                                   OrderConfCxlReplaceRejectListener* listener) {
    VectorUtils::UniqueVectorAdd(order_conf_cxlreplace_reject_listener_vec_map_[security_id], listener);
  }

  inline void RemoveOrderConfCxlReplaceRejectListener(const unsigned int security_id,
                                                      OrderConfCxlReplaceRejectListener* listener) {
    VectorUtils::UniqueVectorRemove(order_conf_cxlreplace_reject_listener_vec_map_[security_id], listener);
  }

  inline void AddOrderConfCxlReplacedListener(const unsigned int _security_id_,
                                              OrderConfCxlReplacedListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_conf_cxlreplaced_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void RemoveOrderConfCxlReplacedListener(const unsigned int _security_id_,
                                                 OrderConfCxlReplacedListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_conf_cxlreplaced_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void AddOrderCanceledListener(const unsigned int _security_id_, OrderCanceledListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_canceled_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void RemoveOrderCanceledListener(const unsigned int _security_id_, OrderCanceledListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_canceled_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void AddOrderCancelSequencedListener(const unsigned int _security_id_, OrderCxlSeqdListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_cxl_sequenced_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void RemoveOrderCancelSequencedListener(const unsigned int _security_id_,
                                                 OrderCxlSeqdListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_cxl_sequenced_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void AddOrderExecutedListener(const unsigned int _security_id_, OrderExecutedListener* _new_listener_) {
    if (_security_id_ >= order_executed_listener_vec_map_
                             .size()) {  // Due to sec-name-indexer changing sizes ( PBSAT ) , this check is needed.
      order_executed_listener_vec_map_.resize(_security_id_ + 1);
    }

    VectorUtils::UniqueVectorAdd(order_executed_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void AddOrderInternallyMatchedListener(const unsigned int _security_id_,
                                                OrderInternallyMatchedListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_internally_matched_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void RemoveOrderInternallyMatchedListener(const unsigned int _security_id_,
                                                   OrderInternallyMatchedListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_internally_matched_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void RemoveOrderExecutedListener(const unsigned int _security_id_, OrderExecutedListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_executed_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void AddOrderRejectedListener(const unsigned int _security_id_, OrderRejectedListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_rejected_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void RemoveOrderRejectedListener(const unsigned int _security_id_, OrderRejectedListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_rejected_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void AddOrderRejectedDueToFundsListener(const unsigned int _security_id_,
                                                 OrderRejectedDueToFundsListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(order_rejected_due_to_funds_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void RemoveOrderRejectedDueToFundsListener(const unsigned int _security_id_,
                                                    OrderRejectedDueToFundsListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(order_rejected_due_to_funds_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  void EnableORSFilteringAndAddORSBase(std::vector<int32_t> ors_saci_vec) {
    is_filtering_enabled_ = true;

    for (auto& itr : ors_saci_vec) {
      ors_base_set_.insert(itr);
    }
  }

  inline void ProcessORSReplyEvent(HFSAT::GenericORSReplyStructLiveProShm* next_event_) {
    HFSAT::ttime_t dummy_time;

    if (HFSAT::ORRType_t::kORRType_Margin == next_event_->orr_type_ ){
        dbglogger_ << "Current Margin :: " << next_event_->price_
                   << "\n";
        dbglogger_.DumpCurrentBuffer();
        for (size_t j = 0; j < order_conf_cxlreplaced_listener_vec_map_.size(); j++) {
            for (size_t i = 0; i < order_conf_cxlreplaced_listener_vec_map_[j].size(); i++) {
              order_conf_cxlreplaced_listener_vec_map_[j][i]->setMargin(next_event_->price_);
            }
        }
        return;
    }
    if (true == is_filtering_enabled_) {
      if (0 != ors_base_set_.size()) {
        int32_t this_ors_msg_base = ((next_event_->server_assigned_client_id_) >> 16);
        if ((ors_base_set_.end() == ors_base_set_.find(this_ors_msg_base))) return;
      }
    }

    register const int security_id_ = sec_name_indexer_.GetIdFromChar16(next_event_->symbol_);

    if (security_id_ >= 0) {
      if (!using_shm_for_ors_reply_ && false == is_filtering_enabled_) {
        p_time_keeper_->OnTimeReceived(dummy_time);
      }

      dummy_time.val = next_event_->query_order_ptr_;

      switch (next_event_->orr_type_) {
        case kORRType_None: {
          std::vector<OrderNotFoundListener*>& order_not_found_listener_vec_ =
              order_not_found_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_not_found_listener_vec_.size(); i++) {
            order_not_found_listener_vec_[i]->OrderNotFound(
                next_event_->server_assigned_client_id_, next_event_->client_assigned_order_sequence_,
                next_event_->server_assigned_order_sequence_, security_id_, next_event_->buysell_,
                next_event_->int_price_, 0, next_event_->ors_order_ptr_, dummy_time);
          }
        } break;
        case kORRType_Seqd: {
          std::vector<OrderSequencedListener*>& order_sequenced_listener_vec_ =
              order_sequenced_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_sequenced_listener_vec_.size(); i++) {
            order_sequenced_listener_vec_[i]->OrderSequenced(
                next_event_->server_assigned_client_id_, next_event_->client_assigned_order_sequence_,
                next_event_->server_assigned_order_sequence_, security_id_, next_event_->price_, next_event_->buysell_,
                next_event_->size_remaining_, next_event_->size_executed_, next_event_->client_position_,
                next_event_->global_position_, next_event_->int_price_, 0, next_event_->ors_order_ptr_, dummy_time);
          }
        } break;
        case kORRType_Conf: {
          std::vector<OrderConfirmedListener*>& order_confirmed_listener_vec_ =
              order_confirmed_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_confirmed_listener_vec_.size(); i++) {
            order_confirmed_listener_vec_[i]->OrderConfirmed(
                next_event_->server_assigned_client_id_, next_event_->client_assigned_order_sequence_,
                next_event_->server_assigned_order_sequence_, security_id_, next_event_->price_, next_event_->buysell_,
                next_event_->size_remaining_, next_event_->size_executed_, next_event_->client_position_,
                next_event_->global_position_, next_event_->int_price_, 0, next_event_->ors_order_ptr_, dummy_time);
          }
        } break;
        case kORRType_ORSConf: {
          std::vector<OrderConfirmedListener*>& order_confirmed_listener_vec_ =
              order_confirmed_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_confirmed_listener_vec_.size(); i++) {
            order_confirmed_listener_vec_[i]->OrderORSConfirmed(
                next_event_->server_assigned_client_id_, next_event_->client_assigned_order_sequence_,
                next_event_->server_assigned_order_sequence_, security_id_, next_event_->price_, next_event_->buysell_,
                next_event_->size_remaining_, next_event_->size_executed_, next_event_->int_price_, 0, 0, dummy_time);
          }
        } break;
        case kORRType_Cxld: {
          if(2 == next_event_->order_flags_){
            for (size_t j = 0; j < order_canceled_listener_vec_map_.size(); j++) {
              for (size_t i = 0; i < order_canceled_listener_vec_map_[j].size(); i++) {
                order_canceled_listener_vec_map_[j][i]->OrderCanceled(
                  next_event_->server_assigned_client_id_, next_event_->client_assigned_order_sequence_,
                  next_event_->server_assigned_order_sequence_, security_id_, next_event_->price_, next_event_->buysell_,
                  next_event_->size_remaining_, next_event_->client_position_, next_event_->global_position_,
                  next_event_->int_price_, next_event_->order_flags_, next_event_->ors_order_ptr_, dummy_time);
              }
            }
          } 
          else{
             std::vector<OrderCanceledListener*>& order_canceled_listener_vec_ =
                 order_canceled_listener_vec_map_[security_id_];
             for (size_t i = 0; i < order_canceled_listener_vec_.size(); i++) {
               order_canceled_listener_vec_[i]->OrderCanceled(
                  next_event_->server_assigned_client_id_, next_event_->client_assigned_order_sequence_,
                  next_event_->server_assigned_order_sequence_, security_id_, next_event_->price_, next_event_->buysell_,
                  next_event_->size_remaining_, next_event_->client_position_, next_event_->global_position_,
                  next_event_->int_price_, 0, next_event_->ors_order_ptr_, dummy_time);
             }
          }
        } break;
        case kORRType_Exec: {
          std::vector<OrderExecutedListener*>& order_executed_listener_vec_ =
              order_executed_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_executed_listener_vec_.size(); i++) {
            order_executed_listener_vec_[i]->OrderExecuted(
                next_event_->server_assigned_client_id_, next_event_->client_assigned_order_sequence_,
                next_event_->server_assigned_order_sequence_, security_id_, next_event_->price_, next_event_->buysell_,
                next_event_->size_remaining_, next_event_->size_executed_, next_event_->client_position_,
                next_event_->global_position_, next_event_->int_price_, 0, next_event_->ors_order_ptr_, dummy_time);
          }
        } break;
        case kORRType_IntExec: {
          std::vector<OrderInternallyMatchedListener*>& order_internally_matched_listener_vec_ =
              order_internally_matched_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_internally_matched_listener_vec_.size(); i++) {
            //@Notes : The trade price sent from the ORS in the double format is unused here since we always want to
            // pass the SMV mktprice,
            ////The reason for doing this is that we can potentially match passive orders at any point and that will
            /// create bias to the pnl computation for the queries involved
            order_internally_matched_listener_vec_[i]->OrderInternallyMatched(
                next_event_->server_assigned_client_id_, next_event_->client_assigned_order_sequence_,
                next_event_->server_assigned_order_sequence_, security_id_,
                (sid_to_smv_ptr_map_[security_id_])->mkt_size_weighted_price(), next_event_->buysell_,
                next_event_->size_remaining_, next_event_->size_executed_, next_event_->client_position_,
                next_event_->global_position_, next_event_->int_price_, 0, next_event_->ors_order_ptr_, dummy_time);
          }
        } break;
        case kORRType_CxlRejc: {
          std::vector<OrderCanceledListener*>& order_canceled_listener_vec_ =
              order_canceled_listener_vec_map_[security_id_];
          const int t_rejection_reason_ = next_event_->size_executed_;
          for (size_t i = 0; i < order_canceled_listener_vec_.size(); i++) {
            order_canceled_listener_vec_[i]->OrderCancelRejected(
                next_event_->server_assigned_client_id_, next_event_->client_assigned_order_sequence_,
                next_event_->server_assigned_order_sequence_, security_id_, next_event_->price_, next_event_->buysell_,
                next_event_->size_remaining_, t_rejection_reason_, next_event_->client_position_,
                next_event_->global_position_, next_event_->int_price_, (uint64_t)next_event_->query_start_cycles_delta_, dummy_time);
          }
        } break;
        case kORRType_Rejc: {
          const int t_rejection_reason_ = next_event_->size_executed_;
          std::vector<OrderRejectedListener*>& order_rejected_listener_vec_ =
              order_rejected_listener_vec_map_[security_id_];

          for (size_t i = 0; i < order_rejected_listener_vec_.size(); i++) {
            order_rejected_listener_vec_[i]->OrderRejected(
                next_event_->server_assigned_client_id_, next_event_->client_assigned_order_sequence_, security_id_,
                next_event_->price_, next_event_->buysell_, next_event_->size_remaining_, t_rejection_reason_,
                next_event_->int_price_, (uint64_t)next_event_->query_start_cycles_delta_, dummy_time);
          }
        } break;
        case kORRType_CxReRejc: {
          std::vector<OrderConfCxlReplaceRejectListener*>& order_modify_reject_listener_vec =
              order_conf_cxlreplace_reject_listener_vec_map_[security_id_];
          for (auto listener : order_modify_reject_listener_vec) {
            listener->OrderConfCxlReplaceRejected(
                next_event_->server_assigned_client_id_, next_event_->client_assigned_order_sequence_,
                next_event_->server_assigned_order_sequence_, security_id_, next_event_->price_, next_event_->buysell_,
                next_event_->size_remaining_, next_event_->client_position_, next_event_->global_position_,
                next_event_->int_price_, next_event_->size_executed_, 0, (uint64_t)next_event_->query_start_cycles_delta_, dummy_time);
          }
        } break;
        case kORRType_CxlSeqd: {
          std::vector<OrderCxlSeqdListener*>& order_cancel_sequenced_listener_vec_ =
              order_cxl_sequenced_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_cancel_sequenced_listener_vec_.size(); i++) {
            order_cancel_sequenced_listener_vec_[i]->OrderCxlSequenced(
                next_event_->server_assigned_client_id_, next_event_->client_assigned_order_sequence_,
                next_event_->server_assigned_order_sequence_, security_id_, next_event_->price_, next_event_->buysell_,
                next_event_->size_remaining_, next_event_->client_position_, next_event_->global_position_,
                next_event_->int_price_, 0, next_event_->ors_order_ptr_, dummy_time);
          }
        } break;
        case kORRType_CxRe: {
          std::vector<OrderConfCxlReplacedListener*>& order_conf_cxlreplaced_listener_vec =
              order_conf_cxlreplaced_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_conf_cxlreplaced_listener_vec.size(); i++) {
            order_conf_cxlreplaced_listener_vec[i]->OrderConfCxlReplaced(
                next_event_->server_assigned_client_id_, next_event_->client_assigned_order_sequence_,
                next_event_->server_assigned_order_sequence_, security_id_, next_event_->price_, next_event_->buysell_,
                next_event_->size_remaining_, next_event_->size_executed_, next_event_->client_position_,
                next_event_->global_position_, next_event_->int_price_, 0, next_event_->ors_order_ptr_, dummy_time);
          }
        } break;
        case kORRType_CxReSeqd: {
        } break;
        default:
          fprintf(stderr, "Weird msgtype in ORSMessagesLiveSource::ProcessAllEvents \n");
          break;
      }
    }
  }
};
}
}
