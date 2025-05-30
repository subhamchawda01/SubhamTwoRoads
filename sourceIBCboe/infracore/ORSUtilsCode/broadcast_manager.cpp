/**
   \file ORSUtilsCode/broadcast_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */

#include "infracore/ORSUtils/broadcast_manager.hpp"
#include "dvccode/Utils/rdtscp_timer.hpp"

namespace HFSAT {

BroadcastManager* BroadcastManager::GetUniqueInstance(DebugLogger& t_dbglogger_, std::string bcast_ip_, int bcast_port_,
                                                      int32_t base_writer_id) {
  static BroadcastManager* bcast_manager_ = NULL;

  if (bcast_manager_ == NULL) {
    // This will ensure regardless who calls getunique instance first in case the ordering changes by mistake, we don't
    // go further till a point of missing ORS reply
    // Ideally it would be best done by using a Set/Get Instance mechanism
    if (std::string("") == bcast_ip_ || 0 >= bcast_port_) {
      t_dbglogger_ << "FAILED TO INITIALIZE BROADCAST MANAGER, BCAST IP : " << bcast_ip_
                   << " BCAST PORT : " << bcast_port_ << "\n";
      t_dbglogger_.DumpCurrentBuffer();

      std::cerr << "FAILED TO INITIALIZE BROADCAST MANAGER, BCAST IP : " << bcast_ip_ << " BCAST PORT : " << bcast_port_
                << "\n";

      exit(-1);
    }

    bcast_manager_ = new BroadcastManager(t_dbglogger_, bcast_ip_, bcast_port_, base_writer_id);
  }

  return bcast_manager_;
}

BroadcastManager::BroadcastManager(DebugLogger& t_dbglogger_, std::string bcast_ip_, int bcast_port_,
                                   int32_t base_writer_id)
    : dbglogger_(t_dbglogger_), position_manager_(ORS::PositionManager::GetUniqueInstance()), ors_recovery_reply(){
  memset((void*)&ors_recovery_reply, 0, sizeof(GenericORSReplyStructLiveProShm));
  broadcaster_ = new Broadcaster(t_dbglogger_, bcast_ip_, bcast_port_, base_writer_id);
}

BroadcastManager::~BroadcastManager() {}

void BroadcastManager::StartThread() { broadcaster_->run(); }

void BroadcastManager::StopTread() { broadcaster_->stop(); }

void BroadcastManager::BroadcastOrderNotFoundNotification(const GenericORSRequestStruct& t_this_client_request_,
                                                          const int t_server_assigned_client_id_) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;
  generic_ors_reply_struct_.client_request_time_ =
      t_this_client_request_.client_request_time_;  // DO NOT USE THIS AS SOURCE TIME - @ravi

  memcpy(generic_ors_reply_struct_.symbol_, t_this_client_request_.symbol_, kSecNameLen);
  generic_ors_reply_struct_.price_ = 0;  // not used by BaseOrderManager in client
  generic_ors_reply_struct_.orr_type_ = kORRType_None;
  generic_ors_reply_struct_.server_assigned_client_id_ = t_server_assigned_client_id_;
  generic_ors_reply_struct_.size_remaining_ = 0;  // not used by BaseOrderManager in client
  generic_ors_reply_struct_.buysell_ = t_this_client_request_.buysell_;
  generic_ors_reply_struct_.server_assigned_order_sequence_ = t_this_client_request_.server_assigned_order_sequence_;
  generic_ors_reply_struct_.client_assigned_order_sequence_ = t_this_client_request_.client_assigned_order_sequence_;
  generic_ors_reply_struct_.size_executed_ = 0;    // not used by BaseOrderManager in client
  generic_ors_reply_struct_.client_position_ = 0;  // not used by BaseOrderManager in client
  generic_ors_reply_struct_.global_position_ = 0;  // invalid not used by BaseOrderManager in client
  generic_ors_reply_struct_.int_price_ =
      t_this_client_request_.int_price_;  // used by BaseOrderManager in client to track down order

  generic_ors_reply_struct_.query_order_ptr_ = 0;
  generic_ors_reply_struct_.ors_order_ptr_ = 0;
  generic_ors_reply_struct_.order_flags_ = (true == t_this_client_request_.is_reserved_type_) ? 1 : 0;
  generic_ors_reply_struct_.query_start_cycles_delta_ = 0; 
  broadcaster_->Push(generic_ors_reply_struct_);
}

void BroadcastManager::BroadcastORSRejection(const ORS::Order& t_this_order_,
                                             const ORSRejectionReason_t t_rejection_reason_, const uint64_t min_throttle_wait_ /*= 0*/) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;
  generic_ors_reply_struct_.client_request_time_ = t_this_order_.ors_timestamp_;

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_.symbol_, kSecNameLen);
  generic_ors_reply_struct_.price_ = t_this_order_.price_;

  generic_ors_reply_struct_.orr_type_ = kORRType_Rejc;
  generic_ors_reply_struct_.server_assigned_client_id_ = t_this_order_.server_assigned_client_id_;
  generic_ors_reply_struct_.size_remaining_ = t_this_order_.size_remaining_;
  generic_ors_reply_struct_.buysell_ = t_this_order_.buysell_;
  generic_ors_reply_struct_.server_assigned_order_sequence_ = 0;
  generic_ors_reply_struct_.client_assigned_order_sequence_ = t_this_order_.client_assigned_order_sequence_;
  generic_ors_reply_struct_.size_executed_ = t_rejection_reason_;   // used to convey rejection_reason_
  generic_ors_reply_struct_.int_price_ = t_this_order_.int_price_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_position_ = 0;
  generic_ors_reply_struct_.global_position_ = 0;
  generic_ors_reply_struct_.query_order_ptr_ = t_this_order_.query_order_ptr_;
  generic_ors_reply_struct_.ors_order_ptr_ = t_this_order_.ors_order_ptr_;

  generic_ors_reply_struct_.csw_start_cycles_ = t_this_order_.csw_start_cycle_count_;
  generic_ors_reply_struct_.query_start_cycles_delta_ = min_throttle_wait_;
  //  generic_ors_reply_struct_.csw_end_cycles_delta_ = (t_this_order_.csw_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_start_cycles_delta_ = (t_this_order_.query_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_end_cycles_delta_ = (t_this_order_.query_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.ors_start_cycles_delta_ = (t_this_order_.ors_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.ors_end_cycles_delta_ =
      (HFSAT::GetCpucycleCountForTimeTick() - t_this_order_.csw_start_cycle_count_);

  generic_ors_reply_struct_.order_flags_ =
      (true == t_this_order_.is_reserved_type_ || true == t_this_order_.is_ioc) ? 1 : 0;

  broadcaster_->Push(generic_ors_reply_struct_);
}

void BroadcastManager::BroadcastORSRejection(const GenericORSRequestStruct& t_this_client_request_,
                                             const int t_server_assigned_client_id_,
                                             const ORSRejectionReason_t t_rejection_reason_) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;

  generic_ors_reply_struct_.client_request_time_ = t_this_client_request_.client_request_time_;

  // we need a lock because the struct generic_ors_reply_struct_ could be simultaneously operated upon by more than one
  // thread,
  memcpy(generic_ors_reply_struct_.symbol_, t_this_client_request_.symbol_, kSecNameLen);
  generic_ors_reply_struct_.price_ = t_this_client_request_.price_;
  generic_ors_reply_struct_.orr_type_ = kORRType_Rejc;
  generic_ors_reply_struct_.server_assigned_client_id_ = t_server_assigned_client_id_;
  generic_ors_reply_struct_.size_remaining_ = t_this_client_request_.size_requested_;
  generic_ors_reply_struct_.buysell_ = t_this_client_request_.buysell_;
  generic_ors_reply_struct_.server_assigned_order_sequence_ = 0;  // invalid field
  generic_ors_reply_struct_.client_assigned_order_sequence_ = t_this_client_request_.client_assigned_order_sequence_;
  generic_ors_reply_struct_.size_executed_ = t_rejection_reason_;            // used to convey rejection_reason_
  generic_ors_reply_struct_.client_position_ = 0;                            // invalid field
  generic_ors_reply_struct_.global_position_ = 0;                            // invalid field
  generic_ors_reply_struct_.int_price_ = t_this_client_request_.int_price_;  // used by BaseOrderManager in client

  generic_ors_reply_struct_.query_order_ptr_ = 0;
  generic_ors_reply_struct_.ors_order_ptr_ = 0;

  generic_ors_reply_struct_.order_flags_ = (true == t_this_client_request_.is_reserved_type_) ? 1 : 0;
  generic_ors_reply_struct_.query_start_cycles_delta_ = 0;
  broadcaster_->Push(generic_ors_reply_struct_);
}

void BroadcastManager::BroadcastORSCancelReplaceRejection(const GenericORSRequestStruct& t_this_client_request_,
                                                          const int t_server_assigned_client_id_,
                                                          const CxlReplaceRejectReason_t t_rejection_reason_) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;

  generic_ors_reply_struct_.client_request_time_ = t_this_client_request_.client_request_time_;

  // we need a lock because the struct generic_ors_reply_struct_ could be simultaneously operated upon by more than one
  // thread,
  memcpy(generic_ors_reply_struct_.symbol_, t_this_client_request_.symbol_, kSecNameLen);
  generic_ors_reply_struct_.price_ = t_this_client_request_.price_;
  generic_ors_reply_struct_.orr_type_ = kORRType_CxReRejc;
  generic_ors_reply_struct_.server_assigned_client_id_ = t_server_assigned_client_id_;
  generic_ors_reply_struct_.size_remaining_ = t_this_client_request_.size_requested_;
  generic_ors_reply_struct_.buysell_ = t_this_client_request_.buysell_;
  generic_ors_reply_struct_.server_assigned_order_sequence_ = 0;  // invalid field
  generic_ors_reply_struct_.client_assigned_order_sequence_ = t_this_client_request_.client_assigned_order_sequence_;
  generic_ors_reply_struct_.size_executed_ = t_rejection_reason_;            // used to convey rejection_reason_
  generic_ors_reply_struct_.client_position_ = 0;                            // invalid field
  generic_ors_reply_struct_.global_position_ = 0;                            // invalid field
  generic_ors_reply_struct_.int_price_ = t_this_client_request_.int_price_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.query_order_ptr_ = 0;
  generic_ors_reply_struct_.ors_order_ptr_ = 0;
  generic_ors_reply_struct_.query_start_cycles_delta_ = 0;
  generic_ors_reply_struct_.order_flags_ = (true == t_this_client_request_.is_reserved_type_) ? 1 : 0;

  broadcaster_->Push(generic_ors_reply_struct_);
}

void BroadcastManager::BroadcastORSCancelReplaceRejection(const ORS::Order& t_this_order_,
                                                          const CxlReplaceRejectReason_t t_rejection_reason_,
                                                          double old_price, int old_int_price, const uint64_t min_throttle_wait_ /*= 0*/) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;
  generic_ors_reply_struct_.client_request_time_ = t_this_order_.ors_timestamp_;

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_.symbol_, kSecNameLen);
  generic_ors_reply_struct_.price_ = old_price;

  generic_ors_reply_struct_.orr_type_ = kORRType_CxReRejc;
  generic_ors_reply_struct_.server_assigned_client_id_ = t_this_order_.server_assigned_client_id_;
  generic_ors_reply_struct_.size_remaining_ = t_this_order_.size_remaining_;
  generic_ors_reply_struct_.buysell_ = t_this_order_.buysell_;
  generic_ors_reply_struct_.server_assigned_order_sequence_ = 0;
  generic_ors_reply_struct_.client_assigned_order_sequence_ = t_this_order_.client_assigned_order_sequence_;
  generic_ors_reply_struct_.size_executed_ = t_rejection_reason_;  // used to convey rejection_reason_
  generic_ors_reply_struct_.int_price_ = old_int_price;            // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_position_ = 0;
  generic_ors_reply_struct_.global_position_ = 0;

  generic_ors_reply_struct_.query_order_ptr_ = t_this_order_.query_order_ptr_;
  generic_ors_reply_struct_.ors_order_ptr_ = t_this_order_.ors_order_ptr_;

  generic_ors_reply_struct_.csw_start_cycles_ = t_this_order_.csw_start_cycle_count_;
  generic_ors_reply_struct_.query_start_cycles_delta_ = min_throttle_wait_;
  //  generic_ors_reply_struct_.csw_end_cycles_delta_ = (t_this_order_.csw_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_start_cycles_delta_ = (t_this_order_.query_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_end_cycles_delta_ = (t_this_order_.query_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.ors_start_cycles_delta_ = (t_this_order_.ors_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.ors_end_cycles_delta_ =
      (HFSAT::GetCpucycleCountForTimeTick() - t_this_order_.csw_start_cycle_count_);

  generic_ors_reply_struct_.order_flags_ =
      (true == t_this_order_.is_reserved_type_ || true == t_this_order_.is_ioc) ? 1 : 0;

  broadcaster_->Push(generic_ors_reply_struct_);
}

void BroadcastManager::BroadcastExchRejection(const ORS::Order& t_this_order_,
                                              const ORSRejectionReason_t t_rejection_reason_) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;

  // This should now be the data source time, since client will provide the mds time
  generic_ors_reply_struct_.client_request_time_ = t_this_order_.ors_timestamp_;

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_.symbol_, kSecNameLen);
  generic_ors_reply_struct_.price_ = t_this_order_.price_;

  generic_ors_reply_struct_.orr_type_ = kORRType_Rejc;
  generic_ors_reply_struct_.server_assigned_client_id_ = t_this_order_.server_assigned_client_id_;
  generic_ors_reply_struct_.size_remaining_ = t_this_order_.size_remaining_;
  generic_ors_reply_struct_.buysell_ = t_this_order_.buysell_;
  generic_ors_reply_struct_.server_assigned_order_sequence_ = t_this_order_.server_assigned_order_sequence_;
  generic_ors_reply_struct_.client_assigned_order_sequence_ = t_this_order_.client_assigned_order_sequence_;
  generic_ors_reply_struct_.size_executed_ = t_rejection_reason_;   // used to convey rejection_reason_
  generic_ors_reply_struct_.int_price_ = t_this_order_.int_price_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_.server_assigned_client_id_);
  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(t_this_order_.security_id_);

  generic_ors_reply_struct_.query_order_ptr_ = t_this_order_.query_order_ptr_;
  generic_ors_reply_struct_.ors_order_ptr_ = t_this_order_.ors_order_ptr_;

  generic_ors_reply_struct_.csw_start_cycles_ = t_this_order_.csw_start_cycle_count_;
  //  generic_ors_reply_struct_.csw_end_cycles_delta_ = (t_this_order_.csw_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_start_cycles_delta_ = (t_this_order_.query_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_end_cycles_delta_ = (t_this_order_.query_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.ors_start_cycles_delta_ = (t_this_order_.ors_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.ors_end_cycles_delta_ =
      (t_this_order_.ors_end_cycle_count_ - t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.ors_end_cycles_from_exchange_ = HFSAT::GetCpucycleCountForTimeTick();

  generic_ors_reply_struct_.order_flags_ =
      (true == t_this_order_.is_reserved_type_ || true == t_this_order_.is_ioc) ? 1 : 0;

  broadcaster_->Push(generic_ors_reply_struct_);
}

void BroadcastManager::BroadcastCancelReplaceExchRejection(const ORS::Order& t_this_order_,
                                                           const CxlReplaceRejectReason_t t_rejection_reason_) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;

  // This should now be the data source time, since client will provide the mds time
  generic_ors_reply_struct_.client_request_time_ = t_this_order_.ors_timestamp_;

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_.symbol_, kSecNameLen);
  generic_ors_reply_struct_.price_ = t_this_order_.price_;

  generic_ors_reply_struct_.orr_type_ = kORRType_CxReRejc;
  generic_ors_reply_struct_.server_assigned_client_id_ = t_this_order_.server_assigned_client_id_;
  generic_ors_reply_struct_.size_remaining_ = t_this_order_.size_remaining_;
  generic_ors_reply_struct_.buysell_ = t_this_order_.buysell_;
  generic_ors_reply_struct_.server_assigned_order_sequence_ = t_this_order_.server_assigned_order_sequence_;
  generic_ors_reply_struct_.client_assigned_order_sequence_ = t_this_order_.client_assigned_order_sequence_;
  generic_ors_reply_struct_.size_executed_ = t_rejection_reason_;   // used to convey rejection_reason_
  generic_ors_reply_struct_.int_price_ = t_this_order_.int_price_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_.server_assigned_client_id_);
  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(t_this_order_.security_id_);

  generic_ors_reply_struct_.query_order_ptr_ = t_this_order_.query_order_ptr_;
  generic_ors_reply_struct_.ors_order_ptr_ = t_this_order_.ors_order_ptr_;

  generic_ors_reply_struct_.csw_start_cycles_ = t_this_order_.csw_start_cycle_count_;
  //  generic_ors_reply_struct_.csw_end_cycles_delta_ = (t_this_order_.csw_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_start_cycles_delta_ = (t_this_order_.query_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_end_cycles_delta_ = (t_this_order_.query_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.ors_start_cycles_delta_ = (t_this_order_.ors_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.ors_end_cycles_delta_ =
      (t_this_order_.ors_end_cycle_count_ - t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.ors_end_cycles_from_exchange_ = HFSAT::GetCpucycleCountForTimeTick();

  generic_ors_reply_struct_.order_flags_ =
      (true == t_this_order_.is_reserved_type_ || true == t_this_order_.is_ioc) ? 1 : 0;

  broadcaster_->Push(generic_ors_reply_struct_);
}

void BroadcastManager::BroadcastCancelRejection(const ORS::Order& t_this_order_,
                                                const CxlRejectReason_t t_exch_level_rejection_reason_, uint64_t min_throttle_wait_ /*= 0*/) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;

  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_.server_assigned_client_id_);
  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(t_this_order_.security_id_);

  generic_ors_reply_struct_.client_request_time_ = t_this_order_.ors_timestamp_;

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_.symbol_, kSecNameLen);
  generic_ors_reply_struct_.price_ = t_this_order_.price_;

  generic_ors_reply_struct_.orr_type_ = kORRType_CxlRejc;
  generic_ors_reply_struct_.server_assigned_client_id_ = t_this_order_.server_assigned_client_id_;
  generic_ors_reply_struct_.size_remaining_ = t_this_order_.size_remaining_;
  generic_ors_reply_struct_.buysell_ = t_this_order_.buysell_;
  generic_ors_reply_struct_.server_assigned_order_sequence_ = t_this_order_.server_assigned_order_sequence_;
  generic_ors_reply_struct_.client_assigned_order_sequence_ = t_this_order_.client_assigned_order_sequence_;
  generic_ors_reply_struct_.size_executed_ = t_exch_level_rejection_reason_;  // used to convey rejection_reason_
  generic_ors_reply_struct_.int_price_ = t_this_order_.int_price_;            // used by BaseOrderManager in client

  generic_ors_reply_struct_.query_order_ptr_ = t_this_order_.query_order_ptr_;
  generic_ors_reply_struct_.ors_order_ptr_ = t_this_order_.ors_order_ptr_;

  generic_ors_reply_struct_.csw_start_cycles_ = t_this_order_.csw_start_cycle_count_;
  //  generic_ors_reply_struct_.csw_end_cycles_delta_ = (t_this_order_.csw_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_start_cycles_delta_ = (t_this_order_.query_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_end_cycles_delta_ = (t_this_order_.query_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.ors_start_cycles_delta_ = (t_this_order_.ors_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);

  if (HFSAT::kExchCancelReject == t_exch_level_rejection_reason_) {
    generic_ors_reply_struct_.ors_end_cycles_delta_ =
        (t_this_order_.ors_end_cycle_count_ - t_this_order_.csw_start_cycle_count_);
    generic_ors_reply_struct_.ors_end_cycles_from_exchange_ = HFSAT::GetCpucycleCountForTimeTick();
  } else {
    generic_ors_reply_struct_.ors_end_cycles_delta_ =
        (HFSAT::GetCpucycleCountForTimeTick() - t_this_order_.csw_start_cycle_count_);
  }

  generic_ors_reply_struct_.query_start_cycles_delta_ = min_throttle_wait_;
  generic_ors_reply_struct_.order_flags_ =
      (true == t_this_order_.is_reserved_type_ || true == t_this_order_.is_ioc) ? 1 : 0;

  broadcaster_->Push(generic_ors_reply_struct_);
}

void BroadcastManager::BroadcastSequenced(const ORS::Order& t_this_order_, int saos) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;

  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_.server_assigned_client_id_);
  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(t_this_order_.security_id_);
  generic_ors_reply_struct_.client_request_time_ =
      t_this_order_.ors_timestamp_;  // So in any case I need to copy somthing, do memset or

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_.symbol_, kSecNameLen);

  generic_ors_reply_struct_.price_ = t_this_order_.price_;

  generic_ors_reply_struct_.orr_type_ = kORRType_Seqd;
  generic_ors_reply_struct_.server_assigned_client_id_ = t_this_order_.server_assigned_client_id_;
  generic_ors_reply_struct_.size_remaining_ = t_this_order_.size_remaining_;
  generic_ors_reply_struct_.buysell_ = t_this_order_.buysell_;
  if (saos > 0) {
    generic_ors_reply_struct_.server_assigned_order_sequence_ = saos;
  } else {
    generic_ors_reply_struct_.server_assigned_order_sequence_ = t_this_order_.server_assigned_order_sequence_;
  }
  generic_ors_reply_struct_.client_assigned_order_sequence_ = t_this_order_.client_assigned_order_sequence_;
  generic_ors_reply_struct_.size_executed_ = t_this_order_.size_executed_;
  generic_ors_reply_struct_.int_price_ = t_this_order_.int_price_;  // used by BaseOrderManager in client

  generic_ors_reply_struct_.query_order_ptr_ = t_this_order_.query_order_ptr_;
  generic_ors_reply_struct_.ors_order_ptr_ = t_this_order_.ors_order_ptr_;

  generic_ors_reply_struct_.csw_start_cycles_ = t_this_order_.csw_start_cycle_count_;
  //  generic_ors_reply_struct_.csw_end_cycles_delta_ = (t_this_order_.csw_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_start_cycles_delta_ = (t_this_order_.query_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_end_cycles_delta_ = (t_this_order_.query_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.ors_start_cycles_delta_ = (t_this_order_.ors_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.ors_end_cycles_delta_ =
      (t_this_order_.ors_end_cycle_count_ - t_this_order_.csw_start_cycle_count_);

  generic_ors_reply_struct_.order_flags_ =
      (true == t_this_order_.is_reserved_type_ || true == t_this_order_.is_ioc) ? 1 : 0;

  broadcaster_->Push(generic_ors_reply_struct_);
}

void BroadcastManager::BroadcastCxlSequenced(const ORS::Order& t_this_order_,
                                             const HFSAT::ttime_t& t_client_request_time_) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;

  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_.server_assigned_client_id_);
  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(t_this_order_.security_id_);
  generic_ors_reply_struct_.client_request_time_ = t_client_request_time_;

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_.symbol_, kSecNameLen);

  generic_ors_reply_struct_.price_ = t_this_order_.price_;

  generic_ors_reply_struct_.orr_type_ = kORRType_CxlSeqd;  // CxlSequenced
  generic_ors_reply_struct_.server_assigned_client_id_ = t_this_order_.server_assigned_client_id_;
  generic_ors_reply_struct_.size_remaining_ = t_this_order_.size_remaining_;
  generic_ors_reply_struct_.buysell_ = t_this_order_.buysell_;
  generic_ors_reply_struct_.server_assigned_order_sequence_ = t_this_order_.server_assigned_order_sequence_;
  generic_ors_reply_struct_.client_assigned_order_sequence_ = t_this_order_.client_assigned_order_sequence_;
  generic_ors_reply_struct_.size_executed_ = t_this_order_.size_executed_;
  generic_ors_reply_struct_.int_price_ = t_this_order_.int_price_;  // used by BaseOrderManager in client

  generic_ors_reply_struct_.query_order_ptr_ = t_this_order_.query_order_ptr_;
  generic_ors_reply_struct_.ors_order_ptr_ = t_this_order_.ors_order_ptr_;

  generic_ors_reply_struct_.csw_start_cycles_ = t_this_order_.csw_start_cycle_count_;
  //  generic_ors_reply_struct_.csw_end_cycles_delta_ = (t_this_order_.csw_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_start_cycles_delta_ = (t_this_order_.query_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_end_cycles_delta_ = (t_this_order_.query_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.ors_start_cycles_delta_ = (t_this_order_.ors_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.ors_end_cycles_delta_ =
      (t_this_order_.ors_end_cycle_count_ - t_this_order_.csw_start_cycle_count_);

  generic_ors_reply_struct_.order_flags_ =
      (true == t_this_order_.is_reserved_type_ || true == t_this_order_.is_ioc) ? 1 : 0;

  broadcaster_->Push(generic_ors_reply_struct_);
}

void BroadcastManager::BroadcastCxlReSequenced(const ORS::Order& t_this_order_,
                                               const HFSAT::ttime_t& t_client_request_time_) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;

  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_.server_assigned_client_id_);
  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(t_this_order_.security_id_);
  generic_ors_reply_struct_.client_request_time_ = t_client_request_time_;

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_.symbol_, kSecNameLen);

  generic_ors_reply_struct_.price_ = t_this_order_.price_;

  generic_ors_reply_struct_.orr_type_ = kORRType_CxReSeqd;  // CxlReSequenced
  generic_ors_reply_struct_.server_assigned_client_id_ = t_this_order_.server_assigned_client_id_;
  generic_ors_reply_struct_.size_remaining_ = t_this_order_.size_remaining_;
  generic_ors_reply_struct_.buysell_ = t_this_order_.buysell_;
  generic_ors_reply_struct_.server_assigned_order_sequence_ = t_this_order_.server_assigned_order_sequence_;
  generic_ors_reply_struct_.client_assigned_order_sequence_ = t_this_order_.client_assigned_order_sequence_;
  generic_ors_reply_struct_.size_executed_ = t_this_order_.size_executed_;
  generic_ors_reply_struct_.int_price_ = t_this_order_.int_price_;  // used by BaseOrderManager in client

  generic_ors_reply_struct_.query_order_ptr_ = t_this_order_.query_order_ptr_;
  generic_ors_reply_struct_.ors_order_ptr_ = t_this_order_.ors_order_ptr_;

  generic_ors_reply_struct_.csw_start_cycles_ = t_this_order_.csw_start_cycle_count_;
  //  generic_ors_reply_struct_.csw_end_cycles_delta_ = (t_this_order_.csw_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_start_cycles_delta_ = (t_this_order_.query_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_end_cycles_delta_ = (t_this_order_.query_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.ors_start_cycles_delta_ = (t_this_order_.ors_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.ors_end_cycles_delta_ =
      (t_this_order_.ors_end_cycle_count_ - t_this_order_.csw_start_cycle_count_);

  generic_ors_reply_struct_.order_flags_ =
      (true == t_this_order_.is_reserved_type_ || true == t_this_order_.is_ioc) ? 1 : 0;

  broadcaster_->Push(generic_ors_reply_struct_);
}

void BroadcastManager::BroadcastConfirm(const ORS::Order& t_this_order_) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;
  // could take some time, should be done outside mutex, does not matter if not thread safe
  generic_ors_reply_struct_.client_request_time_ = t_this_order_.ors_timestamp_;
  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_.symbol_, kSecNameLen);
  generic_ors_reply_struct_.price_ = t_this_order_.price_;
  generic_ors_reply_struct_.orr_type_ = kORRType_Conf;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.server_assigned_client_id_ =
      t_this_order_.server_assigned_client_id_;                               // used by BaseOrderManager in client
  generic_ors_reply_struct_.size_remaining_ = t_this_order_.size_remaining_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.buysell_ = t_this_order_.buysell_;                // used by BaseOrderManager in client
  generic_ors_reply_struct_.server_assigned_order_sequence_ =
      t_this_order_.server_assigned_order_sequence_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_assigned_order_sequence_ =
      t_this_order_.client_assigned_order_sequence_;  // used by BaseOrderManager in client ( if missed kORRType_Seqd
                                                      // sent earlier )
  generic_ors_reply_struct_.size_executed_ = t_this_order_.size_executed_;
  generic_ors_reply_struct_.int_price_ = t_this_order_.int_price_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_.server_assigned_client_id_);
  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(t_this_order_.security_id_);

  generic_ors_reply_struct_.query_order_ptr_ = t_this_order_.query_order_ptr_;
  generic_ors_reply_struct_.ors_order_ptr_ = t_this_order_.ors_order_ptr_;

  generic_ors_reply_struct_.csw_start_cycles_ = t_this_order_.csw_start_cycle_count_;
  //  generic_ors_reply_struct_.csw_end_cycles_delta_ = (t_this_order_.csw_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_start_cycles_delta_ = (t_this_order_.query_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_end_cycles_delta_ = (t_this_order_.query_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.ors_start_cycles_delta_ = (t_this_order_.ors_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.ors_end_cycles_delta_ =
      (t_this_order_.ors_end_cycle_count_ - t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.ors_end_cycles_from_exchange_ = HFSAT::GetCpucycleCountForTimeTick();

  generic_ors_reply_struct_.order_flags_ =
      (true == t_this_order_.is_reserved_type_ || true == t_this_order_.is_ioc) ? 1 : 0;

  broadcaster_->Push(generic_ors_reply_struct_);
}

void BroadcastManager::BroadcastConfirmCxlReplace(const ORS::Order& t_this_order_) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;
  generic_ors_reply_struct_.client_request_time_ = t_this_order_.ors_timestamp_;

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_.symbol_, kSecNameLen);
  generic_ors_reply_struct_.price_ = t_this_order_.price_;
  generic_ors_reply_struct_.orr_type_ = kORRType_CxRe;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.server_assigned_client_id_ =
      t_this_order_.server_assigned_client_id_;                               // used by BaseOrderManager in client
  generic_ors_reply_struct_.size_remaining_ = t_this_order_.size_remaining_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.buysell_ = t_this_order_.buysell_;                // used by BaseOrderManager in client
  generic_ors_reply_struct_.server_assigned_order_sequence_ =
      t_this_order_
          .server_assigned_order_sequence_;  // new SAOS .. the client has the SAOS of the old order that was canceled
  generic_ors_reply_struct_.client_assigned_order_sequence_ =
      t_this_order_.client_assigned_order_sequence_;  // used by BaseOrderManager in client ( if missed kORRType_Seqd
                                                      // sent earlier )
  generic_ors_reply_struct_.size_executed_ = t_this_order_.size_executed_;
  generic_ors_reply_struct_.int_price_ = t_this_order_.int_price_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_.server_assigned_client_id_);
  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(t_this_order_.security_id_);

  generic_ors_reply_struct_.query_order_ptr_ = t_this_order_.query_order_ptr_;
  generic_ors_reply_struct_.ors_order_ptr_ = t_this_order_.ors_order_ptr_;

  generic_ors_reply_struct_.csw_start_cycles_ = t_this_order_.csw_start_cycle_count_;
  //  generic_ors_reply_struct_.csw_end_cycles_delta_ = (t_this_order_.csw_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_start_cycles_delta_ = (t_this_order_.query_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_end_cycles_delta_ = (t_this_order_.query_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.ors_start_cycles_delta_ = (t_this_order_.ors_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.ors_end_cycles_delta_ =
      (t_this_order_.ors_end_cycle_count_ - t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.ors_end_cycles_from_exchange_ = HFSAT::GetCpucycleCountForTimeTick();

  generic_ors_reply_struct_.order_flags_ =
      (true == t_this_order_.is_reserved_type_ || true == t_this_order_.is_ioc) ? 1 : 0;

  broadcaster_->Push(generic_ors_reply_struct_);
}

void BroadcastManager::BroadcastCancelNotification(const ORS::Order& t_this_order_) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;
  generic_ors_reply_struct_.client_request_time_ = t_this_order_.ors_timestamp_;

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_.symbol_, kSecNameLen);
  generic_ors_reply_struct_.price_ = t_this_order_.price_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.orr_type_ = kORRType_Cxld;      // used by BaseOrderManager in client
  generic_ors_reply_struct_.server_assigned_client_id_ =
      t_this_order_.server_assigned_client_id_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.size_remaining_ = t_this_order_.size_remaining_;
  generic_ors_reply_struct_.buysell_ = t_this_order_.buysell_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.server_assigned_order_sequence_ =
      t_this_order_.server_assigned_order_sequence_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_assigned_order_sequence_ =
      t_this_order_.client_assigned_order_sequence_;                // used by BaseOrderManager in client
  generic_ors_reply_struct_.size_executed_ = 0;                     // ignored in client
  generic_ors_reply_struct_.int_price_ = t_this_order_.int_price_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_.server_assigned_client_id_);
  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(t_this_order_.security_id_);

  generic_ors_reply_struct_.query_order_ptr_ = t_this_order_.query_order_ptr_;
  generic_ors_reply_struct_.ors_order_ptr_ = t_this_order_.ors_order_ptr_;

  generic_ors_reply_struct_.csw_start_cycles_ = t_this_order_.csw_start_cycle_count_;
  //  generic_ors_reply_struct_.csw_end_cycles_delta_ = (t_this_order_.csw_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_start_cycles_delta_ = (t_this_order_.query_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_end_cycles_delta_ = (t_this_order_.query_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.ors_start_cycles_delta_ = (t_this_order_.ors_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.ors_end_cycles_delta_ =
      (t_this_order_.ors_end_cycle_count_ - t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.ors_end_cycles_from_exchange_ = HFSAT::GetCpucycleCountForTimeTick();

  generic_ors_reply_struct_.order_flags_ =
      (true == t_this_order_.is_reserved_type_ || true == t_this_order_.is_ioc) ? 1 : 0;
  
  if(1 == t_this_order_.new_server_assigned_order_sequence){
    generic_ors_reply_struct_.order_flags_ = 2;                     // Margin Breach case to indicate strat for freeze.
  }
//  std::cout<< &generic_ors_reply_struct_<<" "<<&t_this_order_.new_server_assigned_order_sequence<<" "<<t_this_order_.new_server_assigned_order_sequence<< generic_ors_reply_struct_.ToString();
  broadcaster_->Push(generic_ors_reply_struct_);
  if(1 == t_this_order_.new_server_assigned_order_sequence){
   //variable address is reused by following structs which triggers freeze in strats.
    generic_ors_reply_struct_.order_flags_ = 0;
  }
}

void BroadcastManager::BroadcastExecNotification(const ORS::Order& t_this_order_) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;
  generic_ors_reply_struct_.client_request_time_ = t_this_order_.ors_timestamp_;

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_.symbol_, kSecNameLen);
  generic_ors_reply_struct_.price_ = t_this_order_.price_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.orr_type_ = kORRType_Exec;      // used by BaseOrderManager in client
  generic_ors_reply_struct_.server_assigned_client_id_ =
      t_this_order_.server_assigned_client_id_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.size_remaining_ = t_this_order_.size_remaining_;
  generic_ors_reply_struct_.buysell_ = t_this_order_.buysell_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.server_assigned_order_sequence_ =
      t_this_order_.server_assigned_order_sequence_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_assigned_order_sequence_ =
      t_this_order_.client_assigned_order_sequence_;                        // used by BaseOrderManager in client
  generic_ors_reply_struct_.size_executed_ = t_this_order_.size_executed_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.int_price_ = t_this_order_.int_price_;          // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_.server_assigned_client_id_);
  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(t_this_order_.security_id_);

  generic_ors_reply_struct_.query_order_ptr_ = t_this_order_.query_order_ptr_;
  generic_ors_reply_struct_.ors_order_ptr_ = t_this_order_.ors_order_ptr_;

  generic_ors_reply_struct_.csw_start_cycles_ = t_this_order_.csw_start_cycle_count_;
  //  generic_ors_reply_struct_.csw_end_cycles_delta_ = (t_this_order_.csw_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_start_cycles_delta_ = (t_this_order_.query_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_end_cycles_delta_ = (t_this_order_.query_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.ors_start_cycles_delta_ = (t_this_order_.ors_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.ors_end_cycles_delta_ =
      (t_this_order_.ors_end_cycle_count_ - t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.ors_end_cycles_from_exchange_ = HFSAT::GetCpucycleCountForTimeTick();

  generic_ors_reply_struct_.order_flags_ =
      (true == t_this_order_.is_reserved_type_ || true == t_this_order_.is_ioc) ? 1 : 0;

  broadcaster_->Push(generic_ors_reply_struct_);
}

void BroadcastManager::BroadcastMargin(double marginbreach_per){
    GenericORSReplyStructLiveProShm generic_ors_reply_struct_;
  generic_ors_reply_struct_.client_request_time_ = HFSAT::GetTimeOfDay();

  generic_ors_reply_struct_.price_ = marginbreach_per;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.orr_type_ = kORRType_Margin;      // used by BaseOrderManager in client
  generic_ors_reply_struct_.server_assigned_client_id_ = 0;
  generic_ors_reply_struct_.server_assigned_order_sequence_ = 0;
  generic_ors_reply_struct_.client_assigned_order_sequence_ = 0;
  //  generic_ors_reply_struct_.csw_end_cycles_delta_ = (t_this_order_.csw_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_start_cycles_delta_ = (t_this_order_.query_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.query_end_cycles_delta_ = (t_this_order_.query_end_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  //  generic_ors_reply_struct_.ors_start_cycles_delta_ = (t_this_order_.ors_start_cycle_count_ -
  //  t_this_order_.csw_start_cycle_count_);
  generic_ors_reply_struct_.order_flags_ = 0;
  //std::cout<< generic_ors_reply_struct_.ToString();
  broadcaster_->Push(generic_ors_reply_struct_);
}
}
