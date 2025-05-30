/**
 \file Tools/eurex_audit_log_reader.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 354, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551

 */

#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

#include "infracore/ETI/ETIETI/ETIMessageDefs.hpp"
// ETIETI
#include "infracore/ETI/ETIETI/ETIMessageDefs.hpp"
#include "infracore/ETI/ETIETI/ETIConnectionGatewayRequest.hpp"
#include "infracore/ETI/ETIETI/ETISessionLogon.hpp"
#include "infracore/ETI/ETIETI/ETIUserLogon.hpp"
#include "infracore/ETI/ETIETI/ETIHeartbeat.hpp"
#include "infracore/ETI/ETIETI/ETISessionLogout.hpp"
#include "infracore/ETI/ETIETI/ETIUserLogout.hpp"
#include "infracore/ETI/ETIETI/ETIMessageLength.hpp"

#include "infracore/ETI/eti_container.hpp"

void ParseAuditOut(std::string filename);
void ParseAuditIn(std::string filename);

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: <exec> <AUDIT_IN/AUDIT_OUT> <input-file>" << std::endl;
    exit(-1);
  }

  std::string option(argv[1]);
  std::string filename(argv[2]);

  if (option == "AUDIT_OUT") {
    ParseAuditOut(filename);
  } else if (option == "AUDIT_IN") {
    ParseAuditIn(filename);
  } else {
    std::cout << "Invalid Option" << std::endl;
  }

  return 0;
}

void ParseAuditOut(std::string filename) {
  char buff[10000];

  HFSAT::ETI::ETIConnectionGatewayRequest eti_connection_gateway_request_;
  HFSAT::ETI::ETISessionLogon eti_session_logon_request_;
  HFSAT::ETI::ETIUserLogon eti_user_logon_request_;
  HFSAT::ETI::ETIHeartbeat eti_heartbeat_request_;

  HFSAT::ETI::ETIUserLogout eti_user_logout_request_;
  HFSAT::ETI::ETISessionLogout eti_session_logout_request_;

  NewOrderSingleShortRequestT new_order;
  DeleteOrderSingleRequestT cancel_order;
  ModifyOrderSingleShortRequestT modify_order;

  HFSAT::BulkFileReader reader1, reader2;
  reader1.open(filename);
  reader2.open(filename);

  typedef struct {
    uint32_t len;
    uint16_t id;
  } HeaderPrefix;

  HeaderPrefix header;

  size_t MDS_SIZE_ = sizeof(HeaderPrefix);
  size_t available_len_1 = reader1.read(&header, MDS_SIZE_);

  while (true) {
    if (available_len_1 < MDS_SIZE_) break;

    switch (header.id) {
      case TID_GATEWAY_REQUEST:
        reader1.read(buff, header.len - MDS_SIZE_);
        std::cout << "Gateway Request" << std::endl;
        reader2.read(&eti_connection_gateway_request_, sizeof(eti_connection_gateway_request_));
        break;
      case TID_LOGON_REQUEST:
        reader1.read(buff, header.len - MDS_SIZE_);
        std::cout << "Session Login Request" << std::endl;
        reader2.read(&eti_session_logon_request_, sizeof(eti_session_logon_request_));
        break;
      case TID_USER_LOGIN_REQUEST:
        reader1.read(buff, header.len - MDS_SIZE_);
        std::cout << "User Login Request" << std::endl;
        reader2.read(&eti_user_logon_request_, sizeof(eti_user_logon_request_));
        break;
      case TID_USER_LOGOUT_REQUEST:
        reader1.read(buff, header.len - MDS_SIZE_);
        std::cout << "User Logout Request" << std::endl;
        reader2.read(&eti_user_logout_request_, sizeof(eti_user_logout_request_));
        break;
      case TID_LOGOUT_REQUEST:
        reader1.read(buff, header.len - MDS_SIZE_);
        std::cout << "Session Logout Request" << std::endl;
        reader2.read(&eti_session_logout_request_, sizeof(eti_session_logout_request_));
        break;
      case TID_HEARTBEAT:
        reader1.read(buff, header.len - MDS_SIZE_);
        std::cout << "Heartbeat Request" << std::endl;
        reader2.read(&eti_heartbeat_request_, sizeof(eti_heartbeat_request_));
        break;
      case TID_NEW_ORDER_SINGLE_SHORT_REQUEST:
        char str_NetworkMsgID[9];
        memcpy(str_NetworkMsgID, new_order.MessageHeaderIn.NetworkMsgID, 8);
        str_NetworkMsgID[8] = '\0';
        reader1.read(buff, header.len - MDS_SIZE_);
        reader2.read(&new_order, sizeof(new_order));
        std::cout << "NewOrderRequest : "
                  << "Len: " << new_order.MessageHeaderIn.BodyLen
                  << ", TemplateId: " << new_order.MessageHeaderIn.TemplateID << ", NetworkMsgId: " << str_NetworkMsgID
                  << ", MsgSeqNum: " << new_order.RequestHeader.MsgSeqNum
                  << ", SenderSubID: " << new_order.RequestHeader.SenderSubID << ", ClOrdId: " << new_order.ClOrdID
                  << ", ComplianceID: " << new_order.ExecutingTrader << ", Qty: " << new_order.OrderQty
                  << ", SecId: " << new_order.SimpleSecurityID << ", MatchInstCrossID: " << new_order.MatchInstCrossID
                  << ", EnrichmentRuleID: " << new_order.EnrichmentRuleID << ", Side: " << (int)new_order.Side
                  << ", PriceValidityCheckType: " << (int)new_order.PriceValidityCheckType
                  << ", TimeInForce: " << (int)new_order.TimeInForce << ", ExecInst: " << (int)new_order.ExecInst
                  << ", Trading Capacity : " << (int)new_order.TradingCapacity << std::endl;

        break;
      case TID_DELETE_ORDER_SINGLE_REQUEST:
        reader1.read(buff, header.len - MDS_SIZE_);
        reader2.read(&cancel_order, sizeof(cancel_order));
        std::cout << " Delete Request : "
                  << " Len: " << cancel_order.MessageHeaderIn.BodyLen
                  << " TemplateId: " << cancel_order.MessageHeaderIn.TemplateID
                  << " MsgSeqNum: " << cancel_order.RequestHeader.MsgSeqNum
                  << " SenderSubID: " << cancel_order.RequestHeader.SenderSubID << " OrderID: " << cancel_order.OrderID
                  << " ClOrdId: " << cancel_order.ClOrdID << " OrigClOrdId: " << cancel_order.OrigClOrdID
                  << " ComplianceId: " << cancel_order.ExecutingTrader
                  << " MarketSegmentId: " << cancel_order.MarketSegmentID
                  << " SimpleSecurityID: " << cancel_order.SimpleSecurityID
                  << " TargetPartyIDSessionID: " << cancel_order.TargetPartyIDSessionID << std::endl;
        break;
      case TID_MODIFY_ORDER_SINGLE_SHORT_REQUEST:
        reader1.read(buff, header.len - MDS_SIZE_);
        reader2.read(&modify_order, sizeof(modify_order));
        std::cout << "Modify Request : Trading Capacity : " << (int)modify_order.TradingCapacity
                  << " SecId: " << modify_order.SimpleSecurityID << std::endl;
        break;
      default:
        // Most Probably TCP Warm Dummy Packets of suze 10 bytes
        reader1.read(buff, 10 * sizeof(char) - MDS_SIZE_);
        std::cout << "Unhandled Type : " << header.id << std::endl;
        reader2.read(buff, 10 * sizeof(char));

        break;
    }
    available_len_1 = reader1.read(&header, MDS_SIZE_);
  }
}
void ParseAuditIn(std::string filename) {
  HFSAT::BulkFileReader reader1, reader2;
  reader1.open(filename);
  reader2.open(filename);

  typedef struct {
    uint32_t msg_len;
    uint16_t eti_template_id;
  } HeaderPrefix;

  size_t MDS_SIZE_ = sizeof(HeaderPrefix);

  while (true) {
    char buf[10000];

    HeaderPrefix header;
    size_t available_len_1 = reader1.read(&header, MDS_SIZE_);
    if (available_len_1 < MDS_SIZE_) break;
    reader1.read(buf, header.msg_len - MDS_SIZE_);

    // char msg[10000];
    // reader2.read(msg, header.msg_len);

    switch (header.eti_template_id) {
      case TID_LOGON_RESPONSE: {
        LogonResponseT this_session_logon_response_;
        reader2.read(&this_session_logon_response_, header.msg_len);

        std::cout << " LOGON_RESPONSE : "
                  << " THrottle Time INterval  :" << this_session_logon_response_.ThrottleTimeInterval
                  << " Msg : " << this_session_logon_response_.ThrottleNoMsgs
                  << " Disconnect Limit : " << this_session_logon_response_.ThrottleDisconnectLimit
                  << " HBT : " << this_session_logon_response_.HeartBtInt
                  << " Instance : " << this_session_logon_response_.SessionInstanceID
                  << " Trade Mode : " << (int)this_session_logon_response_.TradSesMode << "\n";
      } break;

      case TID_USER_LOGIN_RESPONSE: {
        UserLoginResponseT this_user_login_response_;
        reader2.read(&this_user_login_response_, header.msg_len);

        std::cout << "USER_LOGIN_RESPONSE : "
                  << "ETI User LOGON Sequence : " << (this_user_login_response_.ResponseHeader).MsgSeqNum << "\n";

      } break;

      case TID_USER_LOGOUT_RESPONSE: {
        reader2.read(buf, header.msg_len);
        std::cout << "USER_LOGOUT_RESPONSE : "
                  << "\n";
      } break;

      case TID_LOGOUT_RESPONSE: {
        LogoutResponseT this_session_logout_response_;
        reader2.read(&this_session_logout_response_, header.msg_len);

        std::cout << "LOGOUT_RESPONSE :"
                  << " ETI Session LOGOUT Sequence : " << (this_session_logout_response_.ResponseHeader).MsgSeqNum
                  << "\n";

      } break;

      case TID_REJECT: {
        RejectT this_reject_response_;
        reader2.read(&this_reject_response_, header.msg_len);
        std::cout << "REJECT :"
                  << " ETI Session Status : " << this_reject_response_.SessionStatus << "\n";
      } break;

      case TID_HEARTBEAT_NOTIFICATION: {
        reader2.read(buf, header.msg_len);
        std::cout << "HEARTBEAT_NOTIFICATION "
                  << "\n";

      } break;

      case TID_NEW_ORDER_NR_RESPONSE:  // this is for standard order
      {
        NewOrderNRResponseT this_new_order_nr_response_;
        reader2.read(&this_new_order_nr_response_, header.msg_len);

        std::cout << "NEW_ORDER_NR_RESPONSE :"
                  << " OrderID : " << this_new_order_nr_response_.OrderID
                  << " ClOrdID : " << this_new_order_nr_response_.ClOrdID
                  << " SecurityID : " << this_new_order_nr_response_.SecurityID
                  << " ExecID : " << this_new_order_nr_response_.ExecID
                  << " ExecRestatementReason : " << this_new_order_nr_response_.ExecRestatementReason << std::endl;
      } break;

      case TID_NEW_ORDER_RESPONSE:  // this is for lean orders
      {
        /// do no processing -- assumes no regular orders are ever sent.
        /// but adding skip to avoid looping here
        reader2.read(buf, header.msg_len);
        std::cout << " NEW_ORDER_RESPONSE " << '\n';

      } break;

      case TID_DELETE_ORDER_NR_RESPONSE:  // this is for lean orders
      {
        DeleteOrderNRResponseT eti_cancel_order_single_nr_response_;
        reader2.read(&eti_cancel_order_single_nr_response_, header.msg_len);

        std::cout << "DELETE_ORDER_NR_RESPONSE :"
                  << " OrderID : " << eti_cancel_order_single_nr_response_.OrderID
                  << " ClOrdID : " << eti_cancel_order_single_nr_response_.ClOrdID
                  << " OrigClOrdID : " << eti_cancel_order_single_nr_response_.OrigClOrdID
                  << " SecurityID : " << eti_cancel_order_single_nr_response_.SecurityID
                  << " ExecID : " << eti_cancel_order_single_nr_response_.ExecID
                  << " CumQty : " << eti_cancel_order_single_nr_response_.CumQty
                  << " CxlQty : " << eti_cancel_order_single_nr_response_.CxlQty
                  << " ExecRestatementReason : " << eti_cancel_order_single_nr_response_.ExecRestatementReason
                  << std::endl;

      } break;

      case TID_DELETE_ALL_ORDER_QUOTE_EVENT_BROADCAST: {
        DeleteAllOrderQuoteEventBroadcastT eti_order_mass_cancellation_response_;
        reader2.read(&eti_order_mass_cancellation_response_, header.msg_len);

        std::string eti_order_mass_cancellation_event_apl_msg_id_ =
            (eti_order_mass_cancellation_response_.RBCHeaderME).ApplMsgID;

        uint8_t eti_order_mass_cancellation_mass_action_reason_ =
            (eti_order_mass_cancellation_response_.MassActionReason);

        std::string mass_cancellation_event_reason_ = "";

        switch (eti_order_mass_cancellation_mass_action_reason_) {
          case 105: {
            mass_cancellation_event_reason_ = " Product State Halt ";
          } break;
          case 106: {
            mass_cancellation_event_reason_ = " Product State Holiday ";
          } break;
          case 107: {
            mass_cancellation_event_reason_ = " Instrument Suspended ";
          } break;
          case 109: {
            mass_cancellation_event_reason_ = " Complex Instrument Deletion ";
          } break;
          case 110: {
            mass_cancellation_event_reason_ = " Volatility Interruption";
          } break;

          default: { mass_cancellation_event_reason_ = " INVALID REASON "; } break;
        }

        uint8_t eti_order_mass_cancellation_exec_inst_ = (eti_order_mass_cancellation_response_.ExecInst);

        std::string cancellation_scope_ = "";

        switch (eti_order_mass_cancellation_exec_inst_) {
          case 1: {
            cancellation_scope_ = " Persistent Orders ";
          } break;
          case 2: {
            cancellation_scope_ = " Non Persistent Orders ";
          } break;
          case 3: {
            cancellation_scope_ = " Persistent & Non Persistent Orders ";
          } break;
          default: { cancellation_scope_ = " INVALID SCOPE "; } break;
        }

        std::cout << "DELETE_ALL_ORDER_QUOTE_EVENT_BROADCAST :"
                  << " MassActionReportID : " << eti_order_mass_cancellation_response_.MassActionReportID
                  << " SecurityID : " << eti_order_mass_cancellation_response_.SecurityID
                  << " MarketSegmentID : " << eti_order_mass_cancellation_response_.MarketSegmentID
                  << " MassActionreason : " << mass_cancellation_event_reason_ << " ExecInst : " << cancellation_scope_
                  << std::endl;

      } break;

      case TID_DELETE_ALL_ORDER_BROADCAST: {
        std::cout << "DELETE_ALL_ORDER_BROADCAST" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_ORDER_EXEC_NOTIFICATION:  // this is book order execution
      {
        OrderExecNotificationT eti_execution_notice_response_;
        reader2.read(&eti_execution_notice_response_, header.msg_len);

        std::cout << "ORDER_EXEC_NOTIFICATION :"
                  << " OrderId : " << eti_execution_notice_response_.OrderID
                  << " ClOrdID : " << eti_execution_notice_response_.ClOrdID
                  << " OrigClOrdId : " << eti_execution_notice_response_.OrigClOrdID
                  << " SecurityID : " << eti_execution_notice_response_.SecurityID
                  << " ExecID : " << eti_execution_notice_response_.ExecID
                  << " MarketSegmentID : " << eti_execution_notice_response_.MarketSegmentID
                  << " LeavesQty : " << eti_execution_notice_response_.LeavesQty
                  << " CumQty : " << eti_execution_notice_response_.CumQty
                  << " CxlQty : " << eti_execution_notice_response_.CxlQty
                  << " NoLegExecs : " << eti_execution_notice_response_.NoLegExecs
                  << " ExecRestatementReason : " << eti_execution_notice_response_.ExecRestatementReason << std::endl;
      } break;

      case TID_ORDER_EXEC_RESPONSE:  // this is book order execution
      {
        OrderExecResponseT eti_execution_notice_response_;
        reader2.read(&eti_execution_notice_response_, header.msg_len);

        std::cout << "ORDER_EXEC_RESPONSE :"
                  << " OrderId : " << eti_execution_notice_response_.OrderID
                  << " ClOrdID : " << eti_execution_notice_response_.ClOrdID
                  << " OrigClOrdId : " << eti_execution_notice_response_.OrigClOrdID
                  << " SecurityID : " << eti_execution_notice_response_.SecurityID
                  << " ExecID : " << eti_execution_notice_response_.ExecID
                  << " TrdRegTSEntryTime : " << eti_execution_notice_response_.TrdRegTSEntryTime
                  << " TrdRefTSTimePriority : " << eti_execution_notice_response_.TrdRegTSTimePriority
                  << " MarketSegmentID : " << eti_execution_notice_response_.MarketSegmentID
                  << " LeavesQty : " << eti_execution_notice_response_.LeavesQty
                  << " CumQty : " << eti_execution_notice_response_.CumQty
                  << " CxlQty : " << eti_execution_notice_response_.CxlQty
                  << " NoLegExecs : " << eti_execution_notice_response_.NoLegExecs
                  << " ExecRestatementReason : " << eti_execution_notice_response_.ExecRestatementReason << std::endl;

      } break;

      case TID_ADD_COMPLEX_INSTRUMENT_RESPONSE: {
        std::cout << "ADD_COMPLEX_INSTRUMENT_RESPONSE" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_BROADCAST_ERROR_NOTIFICATION: {
        std::cout << "BROADCAST_ERROR_NOTIFICATION" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_CROSS_REQUEST_RESPONSE: {
        std::cout << "CROSS_REQUEST_RESPONSE" << std::endl;
        reader2.read(buf, header.msg_len);
      } break;

      case TID_DELETE_ALL_ORDER_NR_RESPONSE: {
        std::cout << "DELETE_ALL_ORDER_NR_RESPONSE" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_DELETE_ALL_ORDER_RESPONSE: {
        std::cout << "DELETE_ALL_ORDER_RESPONSE" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_DELETE_ALL_QUOTE_BROADCAST: {
        std::cout << "DELETE_ALL_QUOTE_BROADCAST" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_DELETE_ALL_QUOTE_RESPONSE: {
        std::cout << "DELETE_ALL_QUOTE_RESPONSE" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_DELETE_ORDER_BROADCAST: {
        std::cout << "DELETE_ORDER_BROADCAST" << std::endl;
        reader2.read(buf, header.msg_len);
      } break;

      case TID_DELETE_ORDER_RESPONSE: {
        std::cout << "DELETE_ORDER_RESPONSE" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_FORCED_LOGOUT_NOTIFICATION: {
        std::cout << "FORCED_LOGOUT_NOTIFICATION" << std::endl;
        reader2.read(buf, header.msg_len);
      } break;

      case TID_GATEWAY_RESPONSE: {
        std::cout << "GATEWAY_RESPONSE" << std::endl;
        reader2.read(buf, header.msg_len);
      } break;

      case TID_INQUIRE_MM_PARAMETER_RESPONSE: {
        std::cout << "INQUIRE_MM_PARAMETER_RESPONSE" << std::endl;
        reader2.read(buf, header.msg_len);
      } break;

      case TID_INQUIRE_SESSION_LIST_RESPONSE: {
        std::cout << "INQUIRE_SESSION_LIST_RESPONSE" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_LEGAL_NOTIFICATION_BROADCAST: {
        std::cout << "LEGAL_NOTIFICATION_BROADCAST" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_MM_PARAMETER_DEFINITION_RESPONSE: {
        std::cout << "MM_PARAMETER_DEFINITION_RESPONSE" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_MASS_QUOTE_RESPONSE: {
        std::cout << "MASS_QUOTE_RESPONSE" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_MODIFY_ORDER_NR_RESPONSE: {
        ModifyOrderNRResponseT modify_order_response;
        reader2.read(&modify_order_response, header.msg_len);

        std::cout << "MODIFY_ORDER_NR_RESPONSE :"
                  << " OrderId : " << modify_order_response.OrderID << " ClOrdID : " << modify_order_response.ClOrdID
                  << " OrigClOrdId : " << modify_order_response.OrigClOrdID
                  << " SecurityID : " << modify_order_response.SecurityID
                  << " ExecID : " << modify_order_response.ExecID << " LeavesQty : " << modify_order_response.LeavesQty
                  << " CumQty : " << modify_order_response.CumQty << " CxlQty : " << modify_order_response.CxlQty
                  << " ExecRestatementReason : " << modify_order_response.ExecRestatementReason << std::endl;

      } break;

      case TID_MODIFY_ORDER_RESPONSE: {
        std::cout << "MODIFY_ORDER_RESPONSE" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_NEWS_BROADCAST: {
        std::cout << "NEWS_BROADCAST" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_ORDER_EXEC_REPORT_BROADCAST: {
        std::cout << "ORDER_EXEC_REPORT_BROADCAST" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_PARTY_ENTITLEMENTS_UPDATE_REPORT: {
        std::cout << "PARTY_ENTITLEMENTS_UPDATE_REPORT" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_QUOTE_ACTIVATION_NOTIFICATION: {
        std::cout << "QUOTE_ACTIVATION_NOTIFICATION" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_QUOTE_ACTIVATION_RESPONSE: {
        std::cout << "QUOTE_ACTIVATION_RESPONSE" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_QUOTE_EXECUTION_REPORT: {
        std::cout << "QUOTE_EXECUTION_REPORT" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_RFQ_RESPONSE: {
        std::cout << "RFQ_RESPONSE" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_RETRANSMIT_ME_MESSAGE_RESPONSE: {
        std::cout << "RETRANSMIT_ME_MESSAGE_RESPONSE" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_RISK_NOTIFICATION_BROADCAST: {
        std::cout << "RISK_NOTIFICATION_BROADCAST" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_SERVICE_AVAILABILITY_BROADCAST: {
        std::cout << "SERVICE_AVAILABILITY_BROADCAST" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_SUBSCRIBE_RESPONSE: {
        std::cout << "SUBSCRIBE_RESPONSE" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_TM_TRADING_SESSION_STATUS_BROADCAST: {
        std::cout << "TM_TRADING_SESSION_STATUS_BROADCAST" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_THROTTLE_UPDATE_NOTIFICATION: {
        std::cout << "THROTTLE_UPDATE_NOTIFICATION" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_TRADE_BROADCAST: {
        std::cout << "TRADE_BROADCAST" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_TRADING_SESSION_STATUS_BROADCAST: {
        std::cout << "TRADING_SESSION_STATUS_BROADCAST" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      case TID_UNSUBSCRIBE_RESPONSE: {
        std::cout << "UNSUBSCRIBE_RESPONSE" << std::endl;
        reader2.read(buf, header.msg_len);

      } break;

      default: {
        std::cout << " ERROR : Engine Failure Unhandled Template " << std::endl;
        reader2.read(buf, header.msg_len);

      } break;
    }
  }
}
