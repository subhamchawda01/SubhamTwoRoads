/**
    \file BasicOrderRoutingServer/client_request_processor.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066
         India
         +91 80 4060 0717
 */

#ifndef BASE_BASICORDERROUTINGSERVER_CLIENTREQUEST_PROCESSOR_H
#define BASE_BASICORDERROUTINGSERVER_CLIENTREQUEST_PROCESSOR_H

#include <unordered_map>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/tcp_server_socket.hpp"

#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"

#include "infracore/BasicOrderRoutingServer/sequence_generator.hpp"
#include "infracore/BasicOrderRoutingServer/order_manager.hpp"
#include "infracore/BasicOrderRoutingServer/margin_checker.hpp"
#include "infracore/BasicOrderRoutingServer/account_thread.hpp"
#include "infracore/BasicOrderRoutingServer/throttle_manager.hpp"

#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "infracore/ORSUtils/broadcast_manager.hpp"
#include "infracore/NSET/nse_tap_invitation_manager.hpp"

#define MAX_NUMBER_MESSAGES_FROM_CLIENT 100000

namespace HFSAT {
namespace ORS {

/// Receives a request read from SHM and does the required Risk, SelfTrade, OrderCreation and OrderManagerMaps update
class ClientRequestProcessor {
 public:
  ClientRequestProcessor(DebugLogger& dbglogger, AccountThread& account_thread, Settings& settings, bool multi_session);

  virtual ~ClientRequestProcessor();

  /// listen to shm, typically read GenericORSRequestStruct messages from client
  void ProcessClientRequest(GenericORSRequestStruct& req, int saci);

  void ProcessOrderMirrorRequest(GenericORSRequestStruct& req, int saci);
  void ProcessOrderSendRequest(GenericORSRequestStruct& req, int saci);
  void ProcessOrderCancelRequest(GenericORSRequestStruct& req, int saci);
  void ProcessOrderCancelReplace(GenericORSRequestStruct& req, int saci);
  void ProcessRiskRequest(GenericORSRequestStruct& req, int saci);

  // Just pushes a recovery sequence and SACI into bcast manager, other fields are unused
  void ProcessPacketRecovery(GenericORSRequestStruct& req);
  void ProcessHeartBeatRequest(GenericORSRequestStruct& req);
  void ProcessQueueFlushRequest(GenericORSRequestStruct& req);
  void ProcessFakeSendRequest(GenericORSRequestStruct& req);

  /// so all that client has not received sequence of this order
  inline int GetSAOSFromCAOS(int _client_assigned_order_sequence_) const;

  // Cancel pending orders.
  void CancelPendingOrders(int saci);

  void BaseEngineMirrorOrder(Order* new_orders, int mirror_factor);

  void BaseEngineSendOrder(Order* new_order);

  void BaseEngineCancel(Order* this_order);

  void BaseEngineModify(Order* this_order, ORS::Order *orig_order);

  void WriterAdded(int saci);

  void WriterRemoved(int saci);

  void CleanUp();

 protected:
  const SimpleSecuritySymbolIndexer& simple_security_symbol_indexer_;
  DebugLogger& dbglogger_;

 private:
  bool peek_reject_for_time_throttle(int server_sequence, int no_of_orders_) {
    return p_base_engine_->peek_reject_for_time_throttle(server_sequence, no_of_orders_);
  }
  bool reject_for_time_throttle(int server_sequence) {
    return p_base_engine_->reject_for_time_throttle(server_sequence);
  }
  bool reject_for_time_throttle_ioc(int server_sequence) {
    return p_base_engine_->reject_for_time_throttle_ioc(server_sequence);
  }

  MarginChecker& margin_checker_;
  BroadcastManager* bcast_manager_;
  Settings& settings_;

  OrderManager& order_manager_;
  PositionManager& position_manager_;
  AccountThread& account_thread_;

  BaseEngine* p_base_engine_;

  SequenceGenerator& server_assigned_order_sequence_generator_;

  HFSAT::ExchSource_t this_exchange_source_;

  int32_t base_writer_id_;
  bool is_multi_session_;
  bool disable_ors_sequenced_broadcast_;
  Order* fake_order_;
  GenericORSRequestStruct fake_cached_req_;
  Order modify_order_;
  int active_num_clients_;
  int active_num_clients_to_specify_worstcase_pos_;
  bool allow_modify_for_partial_execs_;
  bool allow_cxl_before_conf_;
  Order* next_new_order_for_send_;
};
}
}

#endif  //  BASE_BASICORDERROUTINGSERVER_CLIENTREQUEST_PROCESSOR_H
