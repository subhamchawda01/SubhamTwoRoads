/**
   \file ORSUtils/broadcast_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */

#pragma once

#include "dvccode/CDef/defines.hpp"

#include "infracore/ORSUtils/broadcaster.hpp"
#include "infracore/BasicOrderRoutingServer/position_manager.hpp"
#include "infracore/BasicOrderRoutingServer/order.hpp"

namespace HFSAT {

class BroadcastManager {
 private:
  DebugLogger& dbglogger_;

  ORS::PositionManager& position_manager_;
  GenericORSReplyStructLive ors_recovery_reply;

  Broadcaster* broadcaster_;

  BroadcastManager(DebugLogger& dbglogger, std::string bcast_ip, int bcast_port, int32_t base_writer_id);

 public:
  static BroadcastManager* GetUniqueInstance(DebugLogger& t_dbglogger_, std::string bcast_ip_, int bcast_port_,
                                             int32_t base_writer_id);
  ~BroadcastManager();

  void StartThread();
  void StopTread();

  void BroadcastOrderNotFoundNotification(const GenericORSRequestStruct& t_this_client_request_,
                                          const int t_server_assigned_client_id_);
  void BroadcastORSRejection(const GenericORSRequestStruct& t_this_client_request_,
                             const int t_server_assigned_client_id_, const ORSRejectionReason_t t_rejection_reason_);
  void BroadcastORSRejection(const ORS::Order& t_this_order_, const ORSRejectionReason_t t_rejection_reason_);
  void BroadcastExchRejection(const ORS::Order& t_this_order_, const ORSRejectionReason_t t_rejection_reason_);
  void BroadcastExchRejectionDueToFunds(const ORS::Order& t_this_order_, const bool t_should_get_flat_ = true);
  void BroadcastCancelRejection(const ORS::Order& t_this_order_,
                                const CxlRejectReason_t t_exch_level_rejection_reason_);
  void BroadcastSequenced(const ORS::Order& t_this_order_, int saos = -1);
  void BroadcastCxlSequenced(const ORS::Order& t_this_order_, const HFSAT::ttime_t& t_client_request_time_);
  void BroadcastCxlReSequenced(const ORS::Order& t_this_order_, const HFSAT::ttime_t& t_client_request_time_);
  void BroadcastORSSequenced(const ORS::Order& t_this_order_, const GenericORSRequestStruct& t_this_client_request_);
  void BroadcastConfirm(const ORS::Order& t_this_order_);
  void BroadcastORSConfirm(const ORS::Order& t_this_order_, const GenericORSRequestStruct& t_this_client_request_);
  void BroadcastConfirmCxlReplace(const ORS::Order& t_this_order_);
  void BroadcastCancelNotification(const ORS::Order& t_this_order_);
  void BroadcastExecNotification(const ORS::Order& t_this_order_);
  void BroadcastORSExecNotification(const ORS::Order& t_this_order_, int t_current_size_executed_,
                                    const GenericORSRequestStruct& t_this_client_request_);
  void BroadcastORSCancelReplaceRejection(const HFSAT::GenericORSRequestStruct& t_this_client_request_,
                                          const int t_server_assigned_client_id_,
                                          const CxlReplaceRejectReason_t t_rejection_reason_);
  void BroadcastORSCancelReplaceRejection(const ORS::Order& t_this_order_,
                                          const CxlReplaceRejectReason_t t_rejection_reason_, double old_price,
                                          int old_int_price);
  void BroadcastCancelReplaceExchRejection(const ORS::Order& t_this_order_,
                                           const CxlReplaceRejectReason_t t_rejection_reason_);
  void BroadcastORSTradeBustNotification(const ORS::Order& t_this_order_);

  //@@Function : PushRecoveryEvent
  //
  // This function pushes an ors reply event which triggers a re broadcast of stored earlier packet
  // which query might have missed in the multicast transmission
  //
  //@@Args :
  //
  //@ors_reply_recovery_request - a dummay ors reply event with only two fields filled up as saci and sams
  void PushRecoveryEvent(const GenericORSRequestStruct& ors_reply_recovery_request);
};
}
