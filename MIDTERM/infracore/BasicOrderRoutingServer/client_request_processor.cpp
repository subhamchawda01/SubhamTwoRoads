/**
   \file BasicOrderRoutingServer/client_request_processor.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */

#include <string.h>  // for memcpy

#include "dvccode/CDef/defines.hpp"

#include "dvccode/CDef/ors_messages.hpp"
#include "infracore/BasicOrderRoutingServer/client_request_processor.hpp"
#include "infracore/BasicOrderRoutingServer/sequence_generator.hpp"

#define PROFILE_CLIENT_PROCESSOR 0

namespace HFSAT {
namespace ORS {

ClientRequestProcessor::ClientRequestProcessor(DebugLogger& dbglogger, AccountThread& account_thread,
                                               Settings& settings, bool multi_session)
    : simple_security_symbol_indexer_(HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance()),
      dbglogger_(dbglogger),
      margin_checker_(
          MarginChecker::GetUniqueInstance(dbglogger, HFSAT::StringToExchSource(settings.getValue("Exchange")))),
      bcast_manager_(BroadcastManager::GetUniqueInstance(dbglogger_, "", 0,
                                                         (atoi(settings.getValue("Client_Base").c_str())) << 16)),
      settings_(settings),
      order_manager_(OrderManager::GetUniqueInstance()),
      position_manager_(PositionManager::GetUniqueInstance()),
      account_thread_(account_thread),
      p_base_engine_(account_thread.getEngine()),  // todo: remove this check, always choose
                                                   // _p_engine once its default value (NULL)
                                                   // is removed
      server_assigned_order_sequence_generator_(SequenceGenerator::GetUniqueInstance(settings.getValue("Exchange"))),

      this_exchange_source_(HFSAT::StringToExchSource(settings.getValue("Exchange"))),
      base_writer_id_((atoi(settings.getValue("Client_Base").c_str())) << 16),
      is_multi_session_(multi_session),
      fake_order_(nullptr),
      fake_cached_req_(),
      modify_order_(),
      active_num_clients_(0),
      active_num_clients_to_specify_worstcase_pos_(0),
      allow_modify_for_partial_execs_(false),
      allow_cxl_before_conf_(false),
      next_new_order_for_send_(NULL) {
  margin_checker_.SetQueryRiskFeatureFlag(
      (settings.has("QueryRiskChecks") && (settings.getValue("QueryRiskChecks") == "Y")));

  // Only Exchanges We Want To Disable SelfTrade Matching
  if (std::string("NSE_FO") == settings_.getValue("Exchange") ||
      std::string("NSE_CD") == settings_.getValue("Exchange") ||
      std::string("NSE_EQ") == settings_.getValue("Exchange")) {
    DBGLOG_CLASS_FUNC_LINE_INFO << "SELF TRADE DISABLED FOR CLIENT" << DBGLOG_ENDL_FLUSH;
    allow_modify_for_partial_execs_ = true;
  }

  if ((true == settings_.has("AllowCxlBeforeConf")) && ("Y" == settings_.getValue("AllowCxlBeforeConf"))) {
    allow_cxl_before_conf_ = true;
  }

  fake_order_ = new Order();
  fake_cached_req_.orq_request_type_ = ORQ_FAKE_SEND;

  next_new_order_for_send_ = order_manager_.GetNewOrder();
}

ClientRequestProcessor::~ClientRequestProcessor() {}

void ClientRequestProcessor::ProcessClientRequest(GenericORSRequestStruct& req, int saci) {
  switch (req.orq_request_type_) {
    case ORQ_SEND:
    case ORQ_FOK_SEND:
    case ORQ_IOC: {
      ProcessOrderSendRequest(req, saci);
      return;
    }
    case ORQ_CANCEL: {
      ProcessOrderCancelRequest(req, saci);
      return;
    }
    case ORQ_CXLREPLACE: {
      ProcessOrderCancelReplace(req, saci);
      return;
    }

    case ORQ_RECOVER: {
      ProcessPacketRecovery(req);
      return;
    }

    case ORQ_HEARTBEAT: {
      ProcessHeartBeatRequest(req);
      return;
    }

    case ORQ_PROCESS_QUEUE: {
      ProcessQueueFlushRequest(req);
      return;
    }

    case ORQ_FAKE_SEND: {
      ProcessFakeSendRequest(req);
      return;
    }

    case ORQ_RISK: {
      ProcessRiskRequest(req, saci);
    }

    default: {}
      return;
  }
}

void ClientRequestProcessor::ProcessPacketRecovery(GenericORSRequestStruct& ors_reply_recovery_request) {
  DBGLOG_CLASS_FUNC_LINE_ERROR << "CLIENT : " << ors_reply_recovery_request.client_assigned_order_sequence_
                               << " SEEMED TO HAVE DROPPED THE PACKET WITH SEQUENCE : "
                               << ors_reply_recovery_request.server_assigned_order_sequence_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

  // TODO log error and send mail
  // NOTE : Please note than the server_assigned_order_sequence_ is a place holder for the actual
  // intended usage of the value server_assigned_message_sequence for a client as a means of transmission
  // without introducing additional fields in the struct, Similarly client_assigned_order_sequence_ means SACI
  // for these type of requests
  if (ors_reply_recovery_request.server_assigned_order_sequence_ <= 0 ||
      ors_reply_recovery_request.client_assigned_order_sequence_ <= base_writer_id_)
    return;

  // Push Event to Recovery
  bcast_manager_->PushRecoveryEvent(ors_reply_recovery_request);
}

void ClientRequestProcessor::ProcessHeartBeatRequest(GenericORSRequestStruct& hbt_req) {
  p_base_engine_->CheckToSendHeartbeat();
}

void ClientRequestProcessor::ProcessQueueFlushRequest(GenericORSRequestStruct& hbt_req) {
  p_base_engine_->ProcessMessageQueue();
}

/*
 * The entire function call ProcessFakeSendRequest below aims to bring required values in the cache. It has no logical
 * code which is used while sending/canceling orders.
 */
void ClientRequestProcessor::ProcessFakeSendRequest(GenericORSRequestStruct& req) {
  switch (fake_cached_req_.orq_request_type_) {
    case ORQ_SEND: {
      const int security_id = simple_security_symbol_indexer_.GetIdFromChar16(fake_cached_req_.symbol_);

      if (security_id < 0) {
        return;
      }

      ORSRejectionReason_t margin_retval = kORSOrderAllowed;

      if (ORQ_SEND == fake_cached_req_.orq_request_type_) {
        margin_retval = margin_checker_.Allows(security_id, fake_cached_req_.price_,
                                               fake_cached_req_.size_requested_, fake_cached_req_.buysell_);
      }

      if (margin_retval != kORSOrderAllowed) {
        return;
      }

      fake_order_->security_id_ = security_id;
      fake_order_->server_assigned_client_id_ = -1;
      fake_order_->server_assigned_order_sequence_ = -1;

      // dummy assignment to load account_thread in cache
      account_thread_.was_last_order_throttled_ = false;

      // dummy saos just to load into cache
      volatile int temp_saos = 0;

      volatile Order* temp_order = order_manager_.GetOrderByOrderSequence(temp_saos);
      (void)temp_order;

      const volatile PositionInfo* temp_pos_info = position_manager_.GetPositionInfoStruct(fake_order_->security_id_);
      (void)temp_pos_info;

      p_base_engine_->ProcessFakeSend(fake_order_, ORQ_SEND);

    } break;

    case ORQ_CANCEL: {
      return;
    } break;

    case ORQ_FAKE_SEND: {
      return;
    }

    case ORQ_IOC: {
      return;
    } break;

    default: {
      DBGLOG_CLASS_FUNC_LINE << "Request Type not handled for fake send " << fake_cached_req_.orq_request_type_
                             << DBGLOG_ENDL_FLUSH;
      return;
    }
  }
}

void ClientRequestProcessor::ProcessRiskRequest(GenericORSRequestStruct& req, int saci) {
  if (req.size_requested_ < 0) {
    dbglogger_ << "ProcessRiskRequest Error: Worstpos requested by saci " << saci << " is " << req.size_requested_
               << "\n";
    dbglogger_.DumpCurrentBuffer();
    return;
  }

  const int security_id = simple_security_symbol_indexer_.GetIdFromChar16(req.symbol_);

  if (security_id < 0 && !!strcmp(req.symbol_, HFSAT::kExceptionProductKey)) {
    dbglogger_ << "ProcessRiskRequest Error: Sec requested by saci invalid " << req.symbol_ << "\n";
    dbglogger_.DumpCurrentBuffer();
    return;
  }

  int dummy_to_ignore = 0;  // Not Required here
  int prev_worstcase_pos = margin_checker_.GetClientWorstCasePositionLimit(saci, dummy_to_ignore);
  margin_checker_.SetClientWorstCasePositionLimit(saci, req.size_requested_, security_id);
  if (prev_worstcase_pos == -1) {
    active_num_clients_to_specify_worstcase_pos_++;
    margin_checker_.SetQueryCumulativeRiskChecks(active_num_clients_to_specify_worstcase_pos_ >= active_num_clients_);
  }
}

void ClientRequestProcessor::ProcessOrderMirrorRequest(GenericORSRequestStruct& req, int saci) {
  if (!is_multi_session_) {
    std::cout << "Mirror supported for Multisession only" << std::endl;
    return;
  }

  const int security_id = simple_security_symbol_indexer_.GetIdFromChar16(req.symbol_);
  if (security_id < 0) {
    bcast_manager_->BroadcastORSRejection(req, saci, kORSRejectSecurityNotFound);
    return;
  }

  // Implicit cast from int16_t to int(int32_t)
  int mirror_factor = req.mirror_factor_;

  ORSRejectionReason_t margin_retval = kORSOrderAllowed;
  //=================================MARGIN CHECK ==========================
  // perform margin check with the mirror factor
  if (ORQ_SEND == req.orq_request_type_) {
    margin_retval =
        margin_checker_.Allows(security_id, req.price_, req.size_requested_ * mirror_factor, req.buysell_);
  }

  if (margin_retval != kORSOrderAllowed) {
    if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
      DBGLOG_CLASS_FUNC << "orq_request_type_ = ORQ_SEND margin_retval = "
                        << HFSAT::ORSRejectionReasonStr(margin_retval) << "\n";
      dbglogger_ << "OrderRequest: Side/Size/Price: " << req.buysell_ << " " << req.size_requested_* mirror_factor
                 << " @ " << req.price_ << " CurrPos: " << position_manager_.GetGlobalPosition(security_id)
                 << " Current Risk Limit for " << req.symbol_ << ": MaxPos : " << margin_checker_.getMaxPos(security_id)
                 << " Max OrdSize : " << margin_checker_.getMaxOrdSize(security_id)
                 << " Max Live Orders : " << margin_checker_.getMaxLiveOrd(security_id)
                 << " WorstCase Pos : " << margin_checker_.getWorstPos(security_id) << "\n";
    }

    bcast_manager_->BroadcastORSRejection(req, saci, margin_retval);
    return;
  }
  //========================================================================

  Order* const p_new_order = next_new_order_for_send_;
  // It is possible that GetNewOrder() returns NULL, when we have explicitly instructed it not to allow any new orders
  if (NULL == p_new_order ) {
    DBGLOG_CLASS_FUNC << "For some reason order_manager_.GetNewOrder() returns NULL" << DBGLOG_ENDL_FLUSH;
    //try again
    next_new_order_for_send_ = order_manager_.GetNewOrder();
    return;
  }


  //========================================================================
  // fill only p_new_order

  if (req.orq_request_type_ != ORQ_IOC) {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "Mirror order isn't IOC. Rejecting" << DBGLOG_ENDL_FLUSH;
    bcast_manager_->BroadcastORSRejection(req, saci, kORSRejectNewOrdersDisabled);
    //pre-fetch a new one
    next_new_order_for_send_ = order_manager_.GetNewOrder(); 
    return;
  } else {
    p_new_order->is_ioc = true;
  }

  p_new_order->ors_timestamp_.tv_sec = req.client_request_time_.tv_sec;
  p_new_order->ors_timestamp_.tv_usec = req.client_request_time_.tv_usec;

  memcpy(p_new_order->symbol_, req.symbol_, kSecNameLen);
  p_new_order->security_id_ = security_id;
  p_new_order->server_assigned_client_id_ = saci;
  p_new_order->price_ = req.price_;
  p_new_order->int_price_ = req.int_price_;
  p_new_order->size_remaining_ = req.size_requested_;
  p_new_order->total_size_ = 0 ;
  p_new_order->size_executed_ = 0;
  p_new_order->size_disclosed_ = 0;
  p_new_order->buysell_ = req.buysell_;
  p_new_order->client_assigned_order_sequence_ = req.client_assigned_order_sequence_;
  p_new_order->server_assigned_order_sequence_ =
      server_assigned_order_sequence_generator_.GetNextSequence(mirror_factor);  // for all
  p_new_order->is_confirmed_ = false;
  p_new_order->order_not_found_ = false;
  p_new_order->ignore_from_global_pos = req.ignore_from_global_pos;
  p_new_order->csw_start_cycle_count_ = req.t2t_cshmw_start_time_ ;

  //========================================================================

  // order_manager_.AddToActiveMirrorMaps(p_new_order, mirror_factor);
  order_manager_.AddToActiveMap(p_new_order);

  // write access to order size sum maps on control flow of ClientThread
  if (p_new_order->buysell_ == kTradeTypeBuy) {
    position_manager_.AddBidSize(p_new_order->security_id_, p_new_order->size_remaining_ * mirror_factor,
                                 p_new_order->size_remaining_ * mirror_factor, mirror_factor);
  } else {
    position_manager_.AddAskSize(p_new_order->security_id_, p_new_order->size_remaining_ * mirror_factor,
                                 p_new_order->size_remaining_ * mirror_factor, mirror_factor);
  }

  BaseEngineMirrorOrder(p_new_order, mirror_factor);

  //pre-fetch a new one
  next_new_order_for_send_ = order_manager_.GetNewOrder();

}

void ClientRequestProcessor::ProcessOrderSendRequest(GenericORSRequestStruct& req, int saci) {
  if (req.is_mirror_order_ && req.mirror_factor_ > 1) {
    ProcessOrderMirrorRequest(req, saci);
    return;
  }

  const int _security_id_ = simple_security_symbol_indexer_.GetIdFromChar16(req.symbol_);

  // if security_id not found reject.
  if (_security_id_ < 0) {
    bcast_manager_->BroadcastORSRejection(req, saci, kORSRejectSecurityNotFound);
    return;
  }

  // Check if this order is allowed according to margin values
  ORSRejectionReason_t margin_retval = kORSOrderAllowed;
  //=================================MARGIN CHECK ==========================
  // Only Check Margin For Normal Orders
  if (ORQ_SEND == req.orq_request_type_) {
    margin_retval = margin_checker_.Allows(_security_id_, req.price_, req.size_requested_, req.buysell_);
  }

  if (margin_retval != kORSOrderAllowed) {
    if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
      DBGLOG_CLASS_FUNC << "orq_request_type_ = ORQ_SEND margin_retval = "
                        << HFSAT::ORSRejectionReasonStr(margin_retval) << "\n";
      dbglogger_ << "OrderRequest: Side/Size/Price: " << req.buysell_ << " " << req.size_requested_ << " @ "
                 << req.price_ << " CurrPos: " << position_manager_.GetGlobalPosition(_security_id_)
                 << " Current Risk Limit for " << req.symbol_
                 << ": MaxPos : " << margin_checker_.getMaxPos(_security_id_)
                 << " Max OrdSize : " << margin_checker_.getMaxOrdSize(_security_id_)
                 << " Max Live Orders : " << margin_checker_.getMaxLiveOrd(_security_id_)
                 << " WorstCase Pos : " << margin_checker_.getWorstPos(_security_id_) << "\n";
    }

    bcast_manager_->BroadcastORSRejection(req, saci, margin_retval);
    return;
  }
//============================================================================

  Order* const p_new_order_ = next_new_order_for_send_;

  // It is possible that GetNewOrder() returns NULL, when we have explicitly instructed it not to allow any new orders
  if (p_new_order_ == NULL) {
    DBGLOG_CLASS_FUNC << "For some reason order_manager_.GetNewOrder() returns NULL" << DBGLOG_ENDL_FLUSH;
    //Try again
    next_new_order_for_send_ = order_manager_.GetNewOrder();
    return;
  }

  p_new_order_->ors_timestamp_.tv_sec = req.client_request_time_.tv_sec;
  p_new_order_->ors_timestamp_.tv_usec = req.client_request_time_.tv_usec;

  // initialization, set symbol, security_id_, server_assigned_client_id_, price_, buysell_
  // size_remaining_ to the size requested in this order
  // size_executed_ = 0
  // client_assigned_order_sequence_(clientorderid... needed by client on kORRType_Seqd)
  // int_price_(saves computation since client gets it back and can zoom to the map key)

  memcpy(p_new_order_->symbol_, req.symbol_,
         kSecNameLen);  // TODO_OPT is this any slower than casting to 2 64bit int and copying ?
  p_new_order_->security_id_ = _security_id_;
  p_new_order_->server_assigned_client_id_ = saci;
  p_new_order_->price_ = req.price_;
  p_new_order_->int_price_ = req.int_price_;
  p_new_order_->size_remaining_ = req.size_requested_;
  p_new_order_->size_executed_ = 0;  // at this point it should be 0
  p_new_order_->size_disclosed_ = req.size_disclosed_;
  p_new_order_->buysell_ = req.buysell_;
  p_new_order_->client_assigned_order_sequence_ = req.client_assigned_order_sequence_;
  p_new_order_->server_assigned_order_sequence_ = server_assigned_order_sequence_generator_.GetNextSequence();
  p_new_order_->is_confirmed_ = false;
  p_new_order_->order_not_found_ = false;
  p_new_order_->ignore_from_global_pos = req.ignore_from_global_pos;
  p_new_order_->csw_start_cycle_count_ = req.t2t_cshmw_start_time_ ;

  if (req.orq_request_type_ != ORQ_IOC) {
    p_new_order_->is_ioc = false;
  } else {
    p_new_order_->is_ioc = true;
  }

#if NOT_USING_ZERO_LOGGIN_ORS

  dbglogger_ << "\n NEW ORDER price : " << p_new_order_->price_ << " size " << p_new_order_->size_remaining_ << "\n";
#endif

  //----------------------------------------------------------------------
  //  In Any Scenario, Ensure The FOK order doesn't go further than this block
  //----------------------------------------------------------------------
  if (ORQ_FOK_SEND == req.orq_request_type_) {
    DBGLOG_CLASS_FUNC << "orq_request_type_ = ORQ_FOK_SEND, Message rejected as fill can not happen"
                      << "\n";
    dbglogger_.DumpCurrentBuffer();

    //----------------------------------------------------------------------
    //  If size is not executed - we failed to match internally so broadcast FOK rejection, Broadcast CancelNotif
    //  Otherwise @//ravi
    //----------------------------------------------------------------------
    if (0 == p_new_order_->size_executed_) {
      bcast_manager_->BroadcastORSRejection(req, saci, kORSRejectFOKSendFailed);

    } else {
      bcast_manager_->BroadcastCancelNotification(*p_new_order_);
    }

    server_assigned_order_sequence_generator_.RevertToPreviousSequence();

    //pre-fetch a new one
    next_new_order_for_send_ = order_manager_.GetNewOrder();
    return;

  }  ////FOK BLOCK END

  // Throttle check for send order (non-internally matched)
  // Only check for throttle for non multi session engines. For multi session we will check in MultiSessionEngine class
  if (!is_multi_session_) {
    bool is_reject_for_time_throttle = reject_for_time_throttle(p_new_order_->server_assigned_order_sequence_);
    if (is_reject_for_time_throttle && p_new_order_->is_ioc) {
      is_reject_for_time_throttle = reject_for_time_throttle_ioc(p_new_order_->server_assigned_order_sequence_);
    }

    if (is_reject_for_time_throttle) {
      bcast_manager_->BroadcastORSRejection(req, saci, kORSRejectThrottleLimitReached);

      server_assigned_order_sequence_generator_.RevertToPreviousSequence();

      //pre-fetch a new one
      next_new_order_for_send_ = order_manager_.GetNewOrder();
      return;
    }
  }

  account_thread_.was_last_order_throttled_ = false;

  order_manager_.AddToActiveMap(p_new_order_);

  // write access to order size sum maps on control flow of ClientThread
  if (p_new_order_->buysell_ == kTradeTypeBuy) {
    position_manager_.AddBidSize(p_new_order_->security_id_, p_new_order_->size_remaining_,
                                 p_new_order_->size_remaining_, 1);
  } else {
    position_manager_.AddAskSize(p_new_order_->security_id_, p_new_order_->size_remaining_,
                                 p_new_order_->size_remaining_, 1);
  }

  BaseEngineSendOrder(p_new_order_);

#if NOT_USING_ZERO_LOGGIN_ORS

  dbglogger_ << "\n Sent order : " << p_new_order_->server_assigned_order_sequence_ << "\n";
  dbglogger_ << " p_new_order_->size_remaining_ : " << p_new_order_->size_remaining_ << "\n";
  dbglogger_ << " p_new_order_->size_executed_ : " << p_new_order_->size_executed_ << "\n\n";

#endif

  // At this point, the orders are sequenced so broadcast it
  if (!is_multi_session_) {
    bcast_manager_->BroadcastSequenced(*p_new_order_);
  }

  //pre-fetch a new one
  next_new_order_for_send_ = order_manager_.GetNewOrder();
  // copying last sent request in the fake request
  memcpy(&fake_cached_req_, &req, sizeof(GenericORSRequestStruct));
}

void ClientRequestProcessor::ProcessOrderCancelRequest(GenericORSRequestStruct& req, int saci) {
  Order* p_this_order_ = order_manager_.GetOrderByOrderSequence(req.server_assigned_order_sequence_);

  if (p_this_order_ == NULL) {
    // Most likely already cancelled or filled
    return;
  }

  if (p_this_order_->order_not_found_) {
    bcast_manager_->BroadcastCancelRejection(*p_this_order_, kCxlRejectReasonOrderNotConfirmed);
    return;
  }

  if ((false == allow_cxl_before_conf_) && (false == p_this_order_->is_confirmed_)) {
    bcast_manager_->BroadcastCancelRejection(*p_this_order_, kCxlRejectReasonOrderNotConfirmed);
    return;
  }

  // Check for throttle before sending cancel to exchange
  p_this_order_->ors_timestamp_.tv_sec = req.client_request_time_.tv_sec;
  p_this_order_->ors_timestamp_.tv_usec = req.client_request_time_.tv_usec;
  p_this_order_->csw_start_cycle_count_ = req.t2t_cshmw_start_time_ ;

  if (!is_multi_session_) {
    if (reject_for_time_throttle(p_this_order_->server_assigned_order_sequence_)) {
      bcast_manager_->BroadcastCancelRejection(*p_this_order_, kCxlRejectReasonThrottle);
      return;
    }
  }

  BaseEngineCancel(p_this_order_);
  if (!is_multi_session_) {
    bcast_manager_->BroadcastCxlSequenced(*p_this_order_, req.client_request_time_);
  }
}

void ClientRequestProcessor::ProcessOrderCancelReplace(GenericORSRequestStruct& req, int saci) {
  // from OrderManager::saos_to_order_ptr_map_ find the Order* that has SAOS = _server_assigned_order_sequence_
  Order* p_this_order_ = order_manager_.GetOrderByOrderSequence(req.server_assigned_order_sequence_);

  if (p_this_order_ != NULL && p_this_order_->is_confirmed_) {  // Found order

    // Modify has been sequenced for this order, If we receive an exec and this flag is still true we need to send a
    // modify reject as well to the clients. Although, possibility of race condition still exists due to race
    // condition
    // on accessing/setting the "p_this_order_->modify_seqd" variable by the two threads.

    if ((p_this_order_->size_remaining_ == req.size_requested_) &&
        (fabs(p_this_order_->price_ - req.price_) < 0.00001)) {
      DBGLOG_CLASS_FUNC_LINE_INFO << "Received Modify Request With Same Price and Size: "
                                  << " TS: " << req.client_request_time_.tv_sec << "."
                                  << req.client_request_time_.tv_usec
                                  << " SAOS: " << p_this_order_->server_assigned_order_sequence_
                                  << " SACI: " << p_this_order_->server_assigned_client_id_
                                  << " Size: " << p_this_order_->size_remaining_ << " Px: " << p_this_order_->price_
                                  << DBGLOG_ENDL_FLUSH;
      req.price_ = p_this_order_->price_;
      req.int_price_ = p_this_order_->int_price_;
      bcast_manager_->BroadcastORSCancelReplaceRejection(req, saci, kORSCxReRejectNSEComplianceFailure);
      return;
    }

    p_this_order_->ors_timestamp_.tv_sec = req.client_request_time_.tv_sec;
    p_this_order_->ors_timestamp_.tv_usec = req.client_request_time_.tv_usec;
    p_this_order_->csw_start_cycle_count_ = req.t2t_cshmw_start_time_ ;


    if (!is_multi_session_) {
      if (reject_for_time_throttle(p_this_order_->server_assigned_order_sequence_)) {
        // TODO verify what needs to broadcast here and how it will be handled
        req.price_ = p_this_order_->price_;
        req.int_price_ = p_this_order_->int_price_;
        bcast_manager_->BroadcastORSCancelReplaceRejection(req, saci, kORSCxReRejectThrottleLimitReached);
        return;
      }
    }

    int _security_id_ = p_this_order_->security_id_;
    // Check if this order is allowed according to margin values
    ORSRejectionReason_t margin_retval = margin_checker_.Allows(_security_id_, req.price_, req.size_requested_, req.buysell_,p_this_order_->size_remaining_);

    if (margin_retval != kORSOrderAllowed) {
      DBGLOG_CLASS_FUNC << "orq_request_type_ = ORQ_SEND margin_retval = "
                        << HFSAT::ORSRejectionReasonStr(margin_retval) << "\n";
      dbglogger_ << " Current Risk Limit : MaxPos : " << margin_checker_.getMaxPos(_security_id_)
                 << " Max OrdSize : " << margin_checker_.getMaxOrdSize(_security_id_)
                 << " Max Live Orders : " << margin_checker_.getMaxLiveOrd(_security_id_)
                 << " WorstCase Pos : " << margin_checker_.getWorstPos(_security_id_) << "\n";
      req.price_ = p_this_order_->price_;
      req.int_price_ = p_this_order_->int_price_;

      bcast_manager_->BroadcastORSCancelReplaceRejection(req, saci, (HFSAT::CxlReplaceRejectReason_t)margin_retval);
      return;
    }

    memcpy(&modify_order_, p_this_order_, sizeof(Order));

    modify_order_.price_ = req.price_;
    modify_order_.int_price_ = req.int_price_;
    modify_order_.size_remaining_ = req.size_requested_;
    modify_order_.size_disclosed_ = req.size_disclosed_;

    BaseEngineModify(&modify_order_, p_this_order_);

    if (!is_multi_session_) {
      bcast_manager_->BroadcastCxlReSequenced(*p_this_order_, req.client_request_time_);
    }

  } else {
    if (p_this_order_ != NULL) {
      req.price_ = p_this_order_->price_;
      req.int_price_ = p_this_order_->int_price_;
    }
    bcast_manager_->BroadcastORSCancelReplaceRejection(req, saci, kCxlReRejectOrderNotFound);
  }
}
// Cancel all pending orders for this client_thread.
void ClientRequestProcessor::CancelPendingOrders(int saci) {
  // First attempt to cancel all potentially live orders.

  std::vector<Order*> orders = order_manager_.GetAllOrders();

  /*
   * 1) Loop over all orders
   * 2) Check if order is from given saci, is confirmed and size_remaining is > 0
   */
  for (auto order : orders) {
    if (order && (order->server_assigned_client_id_ == saci) && order->is_confirmed_ && order->size_remaining_ > 0) {
      // Only Reason To Do it before the actual tcp write is if in case resize is triggered and we recevied cxlconf
      // from
      // other thread in worst possible scenario, don't want to add local variable as well
      gettimeofday(&order->ors_timestamp_, NULL);
      BaseEngineCancel(order);

      if (!is_multi_session_) {
        bcast_manager_->BroadcastCxlSequenced(*order, HFSAT::GetTimeOfDay());
      }

      int throttle = 20;
      if (settings_.has("ORSThrottleLimit")) {
        throttle = atoi(settings_.getValue("ORSThrottleLimit").c_str()) - 1;
      }

      if (throttle <= 0) {
        dbglogger_ << " Throttle value : " << throttle << " <= 0 using minimum = 1 \n";
        dbglogger_.DumpCurrentBuffer();
        throttle = 1;
      }

      // 1000/throttle gives amount of millisec to sleep, +1 to avoid boundary case
      long sleep_time = (1000 / throttle + 1) * 1000;
      HFSAT::usleep(sleep_time);  // Throttle
    }
  }
}

void ClientRequestProcessor::BaseEngineSendOrder(Order* new_order) { p_base_engine_->SendOrder(new_order); }

void ClientRequestProcessor::BaseEngineMirrorOrder(Order* new_order, int mirror_factor) {
  p_base_engine_->MirrorOrder(new_order, mirror_factor);
}

void ClientRequestProcessor::BaseEngineCancel(Order* this_order) { p_base_engine_->CancelOrder(this_order); }

void ClientRequestProcessor::BaseEngineModify(Order* this_order, Order *orig_order) { p_base_engine_->ModifyOrder(this_order, orig_order); }

void ClientRequestProcessor::WriterAdded(int saci) {
  active_num_clients_++;
  margin_checker_.SetQueryCumulativeRiskChecks(active_num_clients_to_specify_worstcase_pos_ >= active_num_clients_);
}

void ClientRequestProcessor::WriterRemoved(int saci) {
  active_num_clients_--;
  int sec_id = -1;
  if (margin_checker_.GetClientWorstCasePositionLimit(saci, sec_id) != -1) {
    active_num_clients_to_specify_worstcase_pos_--;
  }
  margin_checker_.SetClientWorstCasePositionLimit(saci, -1, sec_id);  // Required to decrease sum_clients_worstcase_pos_
  margin_checker_.SetQueryCumulativeRiskChecks(active_num_clients_to_specify_worstcase_pos_ >= active_num_clients_);
}

void ClientRequestProcessor::CleanUp() { account_thread_.Logout(); }
}
}
