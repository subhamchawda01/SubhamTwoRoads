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
#include "dvccode/Utils/mds_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "infracore/BasicOrderRoutingServer/position_manager.hpp"
#include "baseinfra/OrderRouting/base_sim_order.hpp"
#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"

namespace HFSAT {

class BroadcastManagerSim {
 private:
  DebugLogger& dbglogger_;
  const HFSAT::Watch& watch_;

  HFSAT::ORS::PositionManager& position_manager_;
  GenericORSReplyStructLiveProShm ors_recovery_reply;
  const SimpleSecuritySymbolIndexer& simple_security_symbol_indexer_;
  std::map<std::string, std::ofstream*> exchange_symbol_to_ors_bcast_log_map_;
  int base_writer_id_;
   
  BroadcastManagerSim(DebugLogger& dbglogger, Watch& _watch_, int base_writer_id);

 public:
  static BroadcastManagerSim* GetUniqueInstance(DebugLogger& t_dbglogger_, Watch& _watch_, int base_writer_id);
  ~BroadcastManagerSim();
  void SetId(int programId){base_writer_id_ = programId; }
  void processMsgRecvd(GenericORSReplyStructLiveProShm generic_ors_reply_struct_);
  void AddToMap(std::string exchange_symbol);
  void BroadcastORSRejection(const BaseSimOrder* t_this_order_, const ORSRejectionReason_t t_rejection_reason_, const uint64_t min_throttle_wait = 0);
  void BroadcastCancelRejection(const BaseSimOrder* t_this_order_,
                                const CxlRejectReason_t t_exch_level_rejection_reason_, const uint64_t min_throttle_wait = 0);
  void BroadcastSequenced(const BaseSimOrder* t_this_order_, int saos = -1);
  void BroadcastCxlSequenced(const BaseSimOrder* t_this_order_, const HFSAT::ttime_t& t_client_request_time_);
  void BroadcastCxlReSequenced(const BaseSimOrder* t_this_order_);
  void BroadcastConfirm(const BaseSimOrder* t_this_order_);
  void BroadcastConfirmCxlReplace(const BaseSimOrder* t_this_order_);
  void BroadcastCancelNotification(const BaseSimOrder* t_this_order_);
  void BroadcastExecNotification(const BaseSimOrder* t_this_order_);
  void BroadcastORSCancelReplaceRejection(const char *security_name_, int server_assigned_client_id,int client_assigned_order_sequence,
                                                    HFSAT::TradeType_t buysell, int size_remaining, double old_price, int old_int_price, const CxlReplaceRejectReason_t rejection_reason_);
};
}
