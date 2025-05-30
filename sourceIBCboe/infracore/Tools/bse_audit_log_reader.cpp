#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include "string.h"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CDef/bse_security_definition.hpp"
#include "infracore/BSE/BSEEngine.hpp"

void ParseAuditIn(std::string filename);
void ParseAuditOut(std::string filename);

std::string segment;

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "Usage : <exec> <AUDIT_IN/AUDIT_OUT> <BSE_EQ> <input-file>" << std::endl;
    exit(-1);
  }

  std::string option(argv[1]);
  std::string segment_(argv[2]);
  segment = segment_;
  std::string filename(argv[3]);

/*
  if (segment == "NSE_EQ") {
    nse_msgs_handler_ = new HFSAT::NSE::NseMsgHandlerCashMarket();
  } else {
    nse_msgs_handler_ = new HFSAT::NSE::NseMsgHandlerDerivatives();
  }
*/

  if (option == "AUDIT_IN") {
    ParseAuditIn(filename);
  } else if (option == "AUDIT_OUT") {
    ParseAuditOut(filename);
  } else {
    std::cout << "Invalid Option" << std::endl;
  }

  return 0;
}


void ParseAuditOut(std::string filename) {
  HFSAT::BulkFileReader reader1;

  reader1.open(filename);

  while (true) {
    char nse_msg_ptr[MAX_BSE_REQUEST_BUFFER_SIZE];
    size_t available_len_1 = reader1.read(nse_msg_ptr, BSE_REQUEST_HEADER_LENGTH);
    // we read the header from the file

    if (available_len_1 < BSE_REQUEST_HEADER_LENGTH) break;
    uint32_t this_bse_message_bodylength_ = (uint32_t)(*((char *)(nse_msg_ptr)));
    uint16_t this_bse_template_id_ = *((uint16_t *)(nse_msg_ptr + 4));

    available_len_1 = reader1.read((nse_msg_ptr + BSE_REQUEST_HEADER_LENGTH), (this_bse_message_bodylength_ - BSE_REQUEST_HEADER_LENGTH));
    if (available_len_1 < (this_bse_message_bodylength_ - BSE_REQUEST_HEADER_LENGTH)) break;

    const char* msg_ptr = nse_msg_ptr;

    switch (this_bse_template_id_) {
      case TID_GATEWAY_REQUEST: {
        std::cout << "GATEWAY_CONNECTION: ";
        HFSAT::BSE::BSEConnectionGatewayRequest process_geteway_request_;
        GatewayRequestT* gateway_request_ = process_geteway_request_.ProcessGatewayRequest(msg_ptr);
        
        std::cout << gateway_request_->ToString() << std::endl;

      } break;

      case TID_LOGON_REQUEST: {
        std::cout << "SESSION_LOGON: ";
        HFSAT::BSE::BSESessionLogon process_session_login_request_;
        LogonRequestT* session_login_request_ = process_session_login_request_.ProcessLogonRequest(msg_ptr);
        
        std::cout << session_login_request_->ToString() << std::endl;
        
      } break;

      case TID_USER_LOGIN_REQUEST: {
        std::cout << "USER_LOGON: ";
        HFSAT::BSE::BSEUserLogon process_user_login_request_;
        UserLoginRequestT* user_login_request_ = process_user_login_request_.ProcessUserLoginRequest(msg_ptr);
        
        std::cout << user_login_request_->ToString() << std::endl;

      } break;

      case TID_USER_LOGOUT_REQUEST: {
        std::cout << "USER_LOGOUT: ";
        HFSAT::BSE::BSEUserLogout process_user_logout_request_;
        UserLogoutRequestT* user_logout_request_ = process_user_logout_request_.ProcessUserLogoutRequest(msg_ptr);
        
        std::cout << user_logout_request_->ToString() << std::endl;

      } break;

      case TID_LOGOUT_REQUEST: {
        std::cout << "SESSION_LOGOUT: ";
        HFSAT::BSE::BSESessionLogout process_session_logout_request_;
        LogoutRequestT* session_logout_request_ = process_session_logout_request_.ProcessLogoutRequest(msg_ptr);
        
        std::cout << session_logout_request_->ToString() << std::endl;

      } break;

      case TID_NEW_ORDER_SINGLE_SHORT_REQUEST: {
        std::cout << "NEW_ORDER: ";
        HFSAT::BSE::BSENewOrderSingleShort process_single_short_request_;
        NewOrderSingleShortRequestT* single_short_request_ = process_single_short_request_.ProcessNewOrderSingleShortRequest(msg_ptr);
        
        std::cout << single_short_request_->ToString() << std::endl;

      } break;

      case TID_DELETE_ORDER_SINGLE_REQUEST: {
        std::cout << "CANCEL_ORDER: ";
        HFSAT::BSE::BSECancelOrderSingle process_cancel_order_request_;
        DeleteOrderSingleRequestT* cancel_order_request_ = process_cancel_order_request_.ProcessCancelOrderSingleShortRequest(msg_ptr);
        
        std::cout << cancel_order_request_->ToString() << std::endl;

      } break;

      case TID_MODIFY_ORDER_SINGLE_SHORT_REQUEST: {
        std::cout << "MODIFY_ORDER_CONFIRMATION: ";
        HFSAT::BSE::BSEModifyOrderSingleShort process_modify_order_request_;
        ModifyOrderSingleShortRequestT* modify_order_request_ = process_modify_order_request_.ProcessModifyOrderSingleShortRequest(msg_ptr);
        
        std::cout << modify_order_request_->ToString() << std::endl;

      } break;

      case TID_HEARTBEAT: {
/*
        std::cout << "HEARTBEAT: ";
        HFSAT::BSE::BSEHeartbeat process_heartbeat_request_;
        HeartbeatT* heartbeat_request_ = process_heartbeat_request_.ProcessHeartbeatRequest(msg_ptr);
        
        std::cout << heartbeat_request_->ToString() << std::endl;
*/

      } break;

      default: {
        std::cout << "Unexpected Reply : " << this_bse_template_id_ << std::endl;
      } break;
    }

    //size_t msg_len =
    //    (this_bse_message_bodylength_ - BSE_RESPONSE_HEADER_LENGTH);
    //reader1.read(nse_msg_ptr, msg_len);
  }
}


void ParseAuditIn(std::string filename) {

  std::map <int,std::string> new_order_map_;
  std::map <int,std::string> modify_order_map_;
  std::map <int,std::string> cancel_order_map_;

  std::vector<std::string> tokens_;
  HFSAT::PerishableStringTokenizer::StringSplit(filename, '.', tokens_);
  int yyyymmdd = std::stoi(tokens_[2]); 
  std::cout << yyyymmdd << std::endl;
  HFSAT::BSESecurityDefinitions::GetUniqueInstance(yyyymmdd);
  std::cout << "\n\n" << std::endl;
  HFSAT::BulkFileReader reader1;

  reader1.open(filename);

  while (true) {
    char nse_msg_ptr[MAX_BSE_RESPONSE_BUFFER_SIZE];
    size_t available_len_1 = reader1.read(nse_msg_ptr, BSE_RESPONSE_HEADER_LENGTH);
    // we read the header from the file

    if (available_len_1 < BSE_RESPONSE_HEADER_LENGTH) break;
    uint32_t this_bse_message_bodylength_ = *((uint32_t *)(nse_msg_ptr));
    uint16_t this_bse_template_id_ = *((uint16_t *)(nse_msg_ptr + 4));

    available_len_1 = reader1.read((nse_msg_ptr + BSE_RESPONSE_HEADER_LENGTH), (this_bse_message_bodylength_ - BSE_RESPONSE_HEADER_LENGTH));
    if (available_len_1 < (this_bse_message_bodylength_ - BSE_RESPONSE_HEADER_LENGTH)) break;

    const char* msg_ptr = nse_msg_ptr;

    switch (this_bse_template_id_) {
      case TID_GATEWAY_RESPONSE: {
        std::cout << "GATEWAY_CONNECTION: ";
        HFSAT::BSE::BSEConnectionGatewayResponse process_geteway_response_;
        GatewayResponseT* gateway_response_ = process_geteway_response_.ProcessGatewayResponse(msg_ptr);
        
        std::cout << gateway_response_->ToString() << std::endl;

      } break;

      case TID_LOGON_RESPONSE: {
        std::cout << "SESSION_LOGON: ";
        HFSAT::BSE::BSESessionLogonResponse process_session_login_response_;
        LogonResponseT* session_login_response_ = process_session_login_response_.ProcessLogonResponse(msg_ptr);
        
        std::cout << session_login_response_->ToString() << std::endl;
        
      } break;

      case TID_USER_LOGIN_RESPONSE: {
        std::cout << "USER_LOGON: ";
        HFSAT::BSE::BSEUserLogonResponse process_user_login_response_;
        UserLoginResponseT* user_login_response_ = process_user_login_response_.ProcessUserLoginResponse(msg_ptr);
        
        std::cout << user_login_response_->ToString() << std::endl;

      } break;

      case TID_USER_LOGOUT_RESPONSE: {
        std::cout << "USER_LOGOUT: ";
        HFSAT::BSE::BSEUserLogoutResponse process_user_logout_response_;
        UserLogoutResponseT* user_logout_response_ = process_user_logout_response_.ProcessUserLogoutResponse(msg_ptr);
        
        std::cout << user_logout_response_->ToString() << std::endl;

      } break;

      case TID_LOGOUT_RESPONSE: {
        std::cout << "SESSION_LOGOUT: ";
        HFSAT::BSE::BSESessionLogoutResponse process_session_logout_response_;
        LogoutResponseT* session_logout_response_ = process_session_logout_response_.ProcessLogoutResponse(msg_ptr);
        
        std::cout << session_logout_response_->ToString() << std::endl;

      } break;

      case TID_REJECT: {
        HFSAT::BSE::BSEReject process_reject_;
        RejectT* reject_ = process_reject_.ProcessReject(msg_ptr);
        std::string response_string = reject_->ToString();
        int reject_seq = reject_->NRResponseHeaderME.MsgSeqNum;

        if ( new_order_map_.find(reject_seq) != new_order_map_.end() ) {
          std::string new_response = new_order_map_[reject_seq];
          std::vector<std::string> response_tokens_;
          HFSAT::PerishableStringTokenizer::StringSplit(new_response, ' ', response_tokens_);

          std::string t_temp_string;
          t_temp_string = "MODIFY_ORDER_REJECT: " + response_tokens_[0] + " " + response_tokens_[1] + " "
                       + response_tokens_[20] + " " + response_tokens_[21] + " " + response_tokens_[26] + " "
                       + response_tokens_[27] + " " + response_tokens_[22] + " " + response_tokens_[23] + " "
                       + response_string ; 
/*          t_temp_string = "NEW_ORDER_REJECT: " + response_tokens_[0] + " " + response_tokens_[1] + " "
                       + response_tokens_[2] + " " + response_tokens_[3] + " " + response_tokens_[4] + " "
                       + response_tokens_[5] + " " + response_tokens_[6] + " " + response_tokens_[7] + " "
                       + response_tokens_[8] + " " + response_tokens_[9] + " " + response_tokens_[10] + " "
                       + response_tokens_[11] + " " + response_tokens_[12] + " " + response_tokens_[13] + " "
                       + response_string ; 
*/
          std::cout << t_temp_string << std::endl;
        }
        else if ( modify_order_map_.find(reject_seq) != modify_order_map_.end() ) {
          std::string modify_response = modify_order_map_[reject_seq];
          std::vector<std::string> response_tokens_;
          HFSAT::PerishableStringTokenizer::StringSplit(modify_response, ' ', response_tokens_);

          std::string t_temp_string;
          t_temp_string = "MODIFY_ORDER_REJECT: " + response_tokens_[0] + " " + response_tokens_[1] + " "
                       + response_tokens_[20] + " " + response_tokens_[21] + " " + response_tokens_[38] + " "
                       + response_tokens_[39] + " " + response_tokens_[30] + " " + response_tokens_[31] + " "
                       + response_tokens_[22] + " " + response_tokens_[23] + " " + response_string ;
/*          t_temp_string = "MODIFY_ORDER_REJECT: " + response_tokens_[0] + " " + response_tokens_[1] + " "
                       + response_tokens_[2] + " " + response_tokens_[3] + " " + response_tokens_[4] + " "
                       + response_tokens_[5] + " " + response_tokens_[6] + " " + response_tokens_[7] + " "
                       + response_tokens_[8] + " " + response_tokens_[9] + " " + response_tokens_[10] + " "
                       + response_tokens_[11] + " " + response_tokens_[12] + " " + response_tokens_[13] + " "
                       + response_string ;
*/
          std::cout << t_temp_string << std::endl;
        }
        else if ( cancel_order_map_.find(reject_seq) != cancel_order_map_.end() ) {
          std::string cancel_response = cancel_order_map_[reject_seq];
          std::vector<std::string> response_tokens_;
          HFSAT::PerishableStringTokenizer::StringSplit(cancel_response, ' ', response_tokens_);

          std::string t_temp_string;
          t_temp_string = "CANCEL_ORDER_REJECT: " + response_tokens_[0] + " " + response_tokens_[1] + " "
                       + response_tokens_[20] + " " + response_tokens_[21] + " " + response_tokens_[32] + " "
                       + response_tokens_[33] + " " + response_tokens_[22] + " " + response_tokens_[23] + " "
                       + response_string ;
/*          t_temp_string = "CANCEL_ORDER_REJECT: " + response_tokens_[0] + " " + response_tokens_[1] + " "
                       + response_tokens_[2] + " " + response_tokens_[3] + " " + response_tokens_[4] + " "
                       + response_tokens_[5] + " " + response_tokens_[6] + " " + response_tokens_[7] + " "
                       + response_tokens_[8] + " " + response_tokens_[9] + " " + response_tokens_[10] + " "
                       + response_tokens_[11] + " " + response_tokens_[12] + " " + response_tokens_[13] + " "
                       + response_string ;
*/
          std::cout << t_temp_string << std::endl;
        }
        else {
          std::string t_temp_string;
          t_temp_string = "REJECT: " + response_string;
          std::cout << t_temp_string << std::endl;
        }

      } break;

      case TID_NEW_ORDER_NR_RESPONSE: {
        std::cout << "NEW_ORDER_CONFIRMATION: ";
        HFSAT::BSE::BSENewOrderSingleShortResponse process_single_short_response_;
        NewOrderNRResponseT* single_short_response_ = process_single_short_response_.ProcessNewOrderSingleShortResponse(msg_ptr);
        std::string exch_symbol = to_string(single_short_response_->SecurityID);
        std::string shortcode = HFSAT::BSESecurityDefinitions::GetUniqueInstance(yyyymmdd).GetShortCodeFromExchangeId(exch_symbol);
        std::string response_string = single_short_response_->ToString();     

        std::cout << "Symbol: " << shortcode << " " << response_string << std::endl;

        int new_msg_seq_ = single_short_response_->NRResponseHeaderME.MsgSeqNum;
        std::string t_temp_string;
        t_temp_string = "Symbol: " + shortcode + " " + response_string;
        new_order_map_[new_msg_seq_] = t_temp_string; 

      } break;

      case TID_DELETE_ORDER_NR_RESPONSE: {
        std::cout << "CANCEL_ORDER_CONFIRMATION: ";
        HFSAT::BSE::BSECancelOrderSingleResponse process_cancel_order_response_;
        DeleteOrderNRResponseT* cancel_order_response_ = process_cancel_order_response_.ProcessCancelOrderSingleShortResponse(msg_ptr);
        std::string exch_symbol = to_string(cancel_order_response_->SecurityID);
        std::string shortcode = HFSAT::BSESecurityDefinitions::GetUniqueInstance(yyyymmdd).GetShortCodeFromExchangeId(exch_symbol);
     
        std::string response_string = cancel_order_response_->ToString();

        std::cout << "Symbol: " << shortcode << " " << response_string << std::endl;

        int cancel_msg_seq = cancel_order_response_->NRResponseHeaderME.MsgSeqNum;
        std::string t_temp_string;
        t_temp_string = "Symbol: " + shortcode + " " + response_string;
        cancel_order_map_[cancel_msg_seq] = t_temp_string;

      } break;

      case TID_MODIFY_ORDER_NR_RESPONSE: {
        std::cout << "MODIFY_ORDER_CONFIRMATION: ";
        HFSAT::BSE::BSEModifyOrderSingleShortResponse process_modify_order_response_;
        ModifyOrderNRResponseT* modify_order_response_ = process_modify_order_response_.ProcessModifyOrderSingleShortResponse(msg_ptr);
        std::string exch_symbol = to_string(modify_order_response_->SecurityID);
        std::string shortcode = HFSAT::BSESecurityDefinitions::GetUniqueInstance(yyyymmdd).GetShortCodeFromExchangeId(exch_symbol);
     
        std::string response_string = modify_order_response_->ToString();

        std::cout << "Symbol: " << shortcode << " " << response_string << std::endl;

        int modify_msg_seq = modify_order_response_->NRResponseHeaderME.MsgSeqNum;
        std::string t_temp_string;
        t_temp_string = "Symbol: " + shortcode + " " + response_string;
        modify_order_map_[modify_msg_seq] = t_temp_string;

      } break;

      case TID_GW_ORDER_ACKNOWLEDGEMENT: {
        std::cout << "ORDER_CONFIRMATION: ";
        HFSAT::BSE::BSEOrderConfirmResponse process_order_response_;
        GwOrderAcknowledgementT* order_response_ = process_order_response_.ProcessOrderConfirmResponse(msg_ptr);
        
        std::cout << order_response_->ToString() << std::endl;

      } break;

      case TID_TRADING_SESSION_STATUS_BROADCAST: {
        std::cout << "SESSION_STATUS_BROADCAST: ";
        HFSAT::BSE::BSETradingSessionStatusBroadcast process_session_status_;
        TradingSessionStatusBroadcastT* session_status_ = process_session_status_.ProcessSessionStatus(msg_ptr);
        
        std::cout << session_status_->ToString() << std::endl;

      } break;

      case TID_ORDER_EXEC_RESPONSE: {
        std::cout << "TRADE_CONFIRMATION: ";
        HFSAT::BSE::BSEOrderExecResponse process_order_exec_;
        OrderExecResponseT* order_exec_ = process_order_exec_.ProcessOrderExecResponse(msg_ptr);
        std::string exch_symbol = to_string(order_exec_->SecurityID);
        std::string shortcode = HFSAT::BSESecurityDefinitions::GetUniqueInstance(yyyymmdd).GetShortCodeFromExchangeId(exch_symbol);
     
        std::cout << "symbol: " << shortcode << " ";
        std::cout << order_exec_->ToString() << std::endl;

      } break;

      case TID_ORDER_EXEC_NOTIFICATION: {
        std::cout << "TRADE_CONFIRMATION: ";
        HFSAT::BSE::BSEOrderExecNotification process_order_exec_notification_;
        OrderExecNotificationT* order_exec_notification_ = process_order_exec_notification_.ProcessOrderExecNotification(msg_ptr);
        std::string exch_symbol = to_string(order_exec_notification_->SecurityID);
        std::string shortcode = HFSAT::BSESecurityDefinitions::GetUniqueInstance(yyyymmdd).GetShortCodeFromExchangeId(exch_symbol);
     
        std::cout << "symbol: " << shortcode << " ";
        std::cout << order_exec_notification_->ToString() << std::endl;

      } break;

      case TID_FORCED_LOGOUT_NOTIFICATION: {
        std::cout << "FORCED_LOGOUT: ";
        HFSAT::BSE::BSEForcedLogoutNotification process_forced_logout_;
        ForcedLogoutNotificationT* forced_logout_ = process_forced_logout_.ProcessForceLogout(msg_ptr);
        
        std::cout << forced_logout_->ToString() << std::endl;

      } break;
      
      case TID_HEARTBEAT_NOTIFICATION: {
/*
        std::cout << "HEARTBEAT_NOTIFICATION: ";
        HFSAT::BSE::BSEHeartbeatNotification process_heartbeat_notification_;
        HeartbeatNotificationT* heartbeat_notification_ = process_heartbeat_notification_.ProcessHeartbeatNotification(msg_ptr);
        
        std::cout << heartbeat_notification_->ToString() << std::endl;
*/
      } break;

      case TID_DELETE_ALL_ORDER_QUOTE_EVENT_BROADCAST: {
        std::cout << "MASS_CANCELLTION: ";
        HFSAT::BSE::BSEMassCancellationEvent process_mass_cancel_event_;
        DeleteAllOrderQuoteEventBroadcastT* mass_cancel_event_ = process_mass_cancel_event_.ProcessMassCancellationEvent(msg_ptr);

        std::cout << mass_cancel_event_->ToString() << std::endl;

      } break;

      case TID_ORDER_EXEC_REPORT_BROADCAST: {
        std::cout << "EXTENDED_ORDER_INFO: ";
        HFSAT::BSE::BSEOrderExecReport process_exec_order_report_;
        OrderExecReportBroadcastT* exec_order_report_ = process_exec_order_report_.ProcessOrderExecReport(msg_ptr);

        std::cout << exec_order_report_->ToString() << std::endl;

      } break;

      default: {
        std::cout << "Unexpected Reply : " << this_bse_template_id_ << std::endl;
      } break;
    }

    //size_t msg_len =
    //    (this_bse_message_bodylength_ - BSE_RESPONSE_HEADER_LENGTH);
    //reader1.read(nse_msg_ptr, msg_len);
  }
}
