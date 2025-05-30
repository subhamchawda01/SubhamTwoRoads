// =====================================================================================
//
//       Filename:  BSEEngine.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/06/2012 08:57:41 AM
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

#ifndef BSET_BSE_BSE_ENGINE_HPP
#define BSET_BSE_BSE_ENGINE_HPP

#define USE_ENCRYPTION 1
#define USE_NORMAL_SOCKET 0

#include <string>
#include <map>
#include <ctime>
#include <fstream>
#include <memory>
#include <algorithm>

#include "infracore/BasicOrderRoutingServer/defines.hpp"  // for DEF_MAX_SEC_ID
#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"
#include "dvccode/Utils/bse_daily_token_symbol_handler.hpp"
#include "dvccode/Utils/bse_refdata_loader.hpp"
#include "dvccode/CDef/bse_security_definition.hpp"
#include "dvccode/Utils/openssl_tls_client_socket.hpp"
#include "dvccode/Utils/openssl_crypto_engine.hpp"

#include "infracore/BasicOrderRoutingServer/engine_listener.hpp"
#include "dvccode/Utils/settings.hpp"

#include "infracore/BSE/BSEBSE/BSEUserPasswordChangeRequest.hpp"
#include "infracore/BasicOrderRoutingServer/base_engine.hpp"
#include "infracore/ORSUtils/broadcast_manager.hpp"
#include "infracore/BasicOrderRoutingServer/position_manager.hpp"
#include "infracore/BasicOrderRoutingServer/sequence_generator.hpp"

// BSEBSE
#include "infracore/BSE/BSEBSE/BSEMessageDefs.hpp"
#include "infracore/BSE/BSEBSE/BSEConnectionGatewayRequest.hpp"
#include "infracore/BSE/BSEBSE/BSEConnectionGatewayResponse.hpp"
#include "infracore/BSE/BSEBSE/BSEHeartbeat.hpp"
#include "infracore/BSE/BSEBSE/BSEHeartbeatNotification.hpp"
#include "infracore/BSE/BSEBSE/BSEMessageLength.hpp"
#include "infracore/BSE/BSEBSE/BSESessionLogon.hpp"
#include "infracore/BSE/BSEBSE/BSESessionLogonResponse.hpp"
#include "infracore/BSE/BSEBSE/BSESessionRegistrationRequest.hpp"
#include "infracore/BSE/BSEBSE/BSESessionRegistrationResponse.hpp"
#include "infracore/BSE/BSEBSE/BSESessionLogout.hpp"
#include "infracore/BSE/BSEBSE/BSESessionLogoutResponse.hpp"
#include "infracore/BSE/BSEBSE/BSEUserLogon.hpp"
#include "infracore/BSE/BSEBSE/BSEUserLogonResponse.hpp"
#include "infracore/BSE/BSEBSE/BSEUserLogout.hpp"
#include "infracore/BSE/BSEBSE/BSEUserLogoutResponse.hpp"
#include "infracore/BSE/BSEBSE/BSEOrderConfirmResponse.hpp"
#include "infracore/BSE/BSEBSE/BSENewOrderSingleShortResponse.hpp"
#include "infracore/BSE/BSEBSE/BSENewOrderSingleResponse.hpp"
#include "infracore/BSE/BSEBSE/BSEModifyOrderSingleShortResponse.hpp"
#include "infracore/BSE/BSEBSE/BSECancelOrderSingleResponse.hpp"
#include "infracore/BSE/BSEBSE/BSEReject.hpp"
#include "infracore/BSE/BSEBSE/BSEForcedLogoutNotification.hpp"
#include "infracore/BSE/BSEBSE/BSETradingSessionStatusBroadcast.hpp"
#include "infracore/BSE/BSEBSE/BSEOrderExecResponse.hpp"
#include "infracore/BSE/BSEBSE/BSEOrderExecNotification.hpp"
#include "infracore/BSE/BSEBSE/BSENewOrderSingleShort.hpp"
#include "infracore/BSE/BSEBSE/BSENewOrderSingle.hpp"
#include "infracore/BSE/BSEBSE/BSECancelOrderSingle.hpp"
#include "infracore/BSE/BSEBSE/BSEModifyOrderSingleShort.hpp"
#include "infracore/BSE/BSEBSE/BSEMassCancellationEvent.hpp"
#include "infracore/BSE/BSEBSE/BSEOrderExecReport.hpp"

#include "infracore/BSE/bse_container.hpp"

#include "dvccode/Utils/tcp_client_socket.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/lock.hpp"  // Thread safety
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/bse_algo_tagging.hpp"

#include "infracore/BasicOrderRoutingServer/order_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/async_writer.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "dvccode/Utils/tcp_direct_client_zocket_with_logging.hpp"

#define MAX_SEND_MESSAGE_SEQUENCE_LIMIT 81200
#define MAX_CXL_MESSAGE_SEQUENCE_LIMIT 81200
#define MAX_MOD_MESSAGE_SEQUENCE_LIMIT 81200
#define MAX_SAOS_TO_METADATA_ENTRIES_LIMIT 262144
#define SAFE_OFFSET 128
#define BSE_PACKET_RESPONSE_LENGTH ((sizeof(int32_t) + sizeof(int16_t) + 2))

namespace HFSAT {
namespace BSE {

class BSEEngine : public ORS::BaseEngine, public SimpleSecuritySymbolIndexerListener {
 public:

  struct SaosPriceStruct {
    int32_t server_assigned_sequence_;
    double price_;
    uint32_t size_;
    bool is_buy_otherwise_sell_;
    bool all_data_added_;

    void Initialize() {
      server_assigned_sequence_ = 0;
      price_ = 0.0;
      size_ = 0;
      is_buy_otherwise_sell_ = false;
      all_data_added_ = false;
    }

    std::string ToString() {
      std::ostringstream t_temp_oss;

      t_temp_oss << "SAOS : " << server_assigned_sequence_ << " Price : " << price_ << " Size : " << size_
                 << " BuySell : " << is_buy_otherwise_sell_ << " FlushMarker : " << all_data_added_ << "\n";

      return t_temp_oss.str();
    }
  };

  struct ReferenceData {
    uint32_t market_segment_id_;
    int64_t security_id_;
  };

  BSEEngine(HFSAT::ORS::Settings &settings, HFSAT::DebugLogger &dbglogger, std::string output_log_dir,
            int32_t engine_id, AsyncWriter *pWriter, AsyncWriter *pReader);
  ~BSEEngine();

  void SendOrder(HFSAT::ORS::Order *order, const int32_t& prod_token_);
  void SendMktOrder(HFSAT::ORS::Order *order, const int32_t& prod_token_, const int32_t& prod_id_);
  void CancelOrder(HFSAT::ORS::Order *order, const int32_t& prod_token_, const int32_t& prod_id_);
  void ModifyOrder(HFSAT::ORS::Order *order, const int32_t& prod_token_, ORS::Order *orig_order);
  void SendOrder(HFSAT::ORS::Order* rp_order_);
  void SendOrder(std::vector<HFSAT::ORS::Order*> multi_leg_order_ptr_vec_) override {}
  void SendSpreadOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_) override {}
  void SendThreeLegOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_, HFSAT::ORS::Order *order3_) override {}
  void SendTwoLegOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_) override {}
  void CancelOrder(HFSAT::ORS::Order* rp_order_);
  void ModifyOrder(HFSAT::ORS::Order* rp_order_, HFSAT::ORS::Order* rp_order2_);

  void Connect();
  void DisConnect();
  void SessionLogin();
  void SecondarySessionLogin();
  void SessionRegistrationRequest();
  void Login();
  void Logout();
  void sendHeartbeat();
  void CheckToSendHeartbeat();
  void GatewayRequest();
  virtual void ProcessLockFreeTCPDirectRead();
//  void ProcessFakeSend(HFSAT::ORS::Order* ord, ORQType_t type) override;

  void thread_main();
  
  virtual int32_t onInputAvailable(int32_t socket, char *buffer, int32_t length);

  void ProcessGeneralControlCommand(const char *input_stream, int stream_length) override;

  void OnAddString(uint32_t num_sid);

  void printHexString(const char* c, int len) {
    for (int i = 0; i < len; ++i) {
      uint8_t ch = c[i];
      printf("%02x ", ch);
    }
    printf("\n");
  }

  void printStringFromByte(const char* c, int len) {
    for (int i = 0; i < len; ++i) {
      uint8_t ch = c[i];
      printf("%c", ch);
    }
    printf("\n");
  }

  std::string getBinaryToHexString(const char* message_, int length) {
    std::ostringstream t_oss_;

    for (int byte_no_ = 0; byte_no_ < length; ++byte_no_) {
      char byte_[5] = {'\0'};
      sprintf(byte_, "%02X", (0xFF & message_[byte_no_]));
      t_oss_ << byte_ << " ";
    }

    return t_oss_.str();
  }

  int processBSEExchangeMsg(char* msg_char_buf_, const int& this_msg_length_);
  uint32_t ProcessEncryptedExchangeResponse(char *bse_msg_char_buf, const int &this_msg_length_);

  void GetNewPassword(char * str);
  void EmailPasswordChange(std::string alert_msg_);
  bool ValidatePostMarketTimings();
  void processBSEConnectionGatewayResponse(const char* msg_char_buf_);
  void processBSESessionLogonResponse(const char* msg_char_buf_);
  void processBSESessionLogoutResponse(const char* msg_char_buf_);
  void processBSESessionForcedLogoutResponse(const char* msg_char_buf_);
  void processBSEUserLoginResponse(const char* msg_char_buf_);
  void processBSEUserLogout(const char* msg_char_buf_);
  void processBSEUserLogoutResponse(const char* msg_char_buf_);
  void processBSESessionPasswordChangeResponse(const char* msg_char_buf_);
  void processBSESessionRegistrationResponse(const char* msg_char_buf_);
  void processBSEUserPasswordChangeResponse(const char* msg_char_buf_);
  void processBSERejectResponse(const char* msg_char_buf_);
  void processBSEHeartbeatResponse(const char* msg_char_buf_);
  void processBSENewOrderNRResponse(const char* msg_char_buf_);
  void processBSEModifyOrderNRResponse(const char* msg_char_buf_);
  void processBSENewOrderResponse(const char* msg_char_buf_);
  void processBSEMassCancellationEventResponse(const char* msg_char_buf_);
  void processBSEOrderExecutionResponse(const char* msg_char_buf_);
  void processBSEImmediateExecutionResponse(const char* msg_char_buf_);
  void processBSECancelOrderSingleNRResponse(const char* msg_char_buf_);
  void processBSECancelOrderSingleResponse(const char* msg_char_buf_);
  void processBSEOrderConfirmationResponse(const char* msg_char_buf_);
  void processBSEExtendedOrderInfoResponse(const char* msg_char_buf_);
  void processBSEMassCancellationResponse(const char* msg_char_buf_);
  void processBSERiskCollateralAlertResponse(const char* msg_char_buf_);
  void processBSERiskCollateralAlertTraderResponse(const char* msg_char_buf_);
  void OnControlHubDisconnectionShutdown() final;
  void EmailGeneric(char* alert_msg_);
  void EnableDisableOnlyIOC(const bool is_risk_reduction_set_);
  void CleanUp();
  void SessionLogout();

 private:
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::ORS::BSEContainer container_;
  bool keep_engine_running_;
  bool keep_trying_different_gateways_;
  bool is_connected_;
  bool is_logged_in_;
  bool is_mkt_order_;
  uint32_t next_seq_num_;
  HFSAT::ORS::Order dummy_order_;
  int heartbeat_interval_;
  bool is_read_socket_open_;
  bool is_secure_box_registered_;
  char session_key_[8];
  char crypto_key_[32];
  char crypto_iv_[16];

  int last_server_seqnum_;
  uint32_t last_proc_req_seq_num_;

  bool allow_new_orders_;


#if USE_ENCRYPTION
  HFSAT::Utils::OpenSSLTLSClientSocket *bse_connection_gateway_socket_;
#else 
  TCPClientSocket *bse_connection_gateway_socket_;
#endif

HFSAT::Utils::OpenSSLCrypto open_ssl_crypto_;
char bse_msg_char_buf_[MAX_BSE_RESPONSE_BUFFER_SIZE];

  std::vector<std::string> bse_gateway_ip_list_;
  std::string bse_gateway_ip_;
  int32_t bse_gateway_port_;
  char *tcp_direct_read_buffer_;
  int32_t tcp_direct_read_length_;
  bool was_tcp_direct_read_successful_;
  std::string tap_gateway_;
  std::string tap_interface_;
  std::string tap_ip_;

 public:
  uint32_t username_;
  uint32_t session_id_;
  uint64_t sender_location_id_;
  int32_t send_write_length;
  int32_t send_mkt_write_length;
  int32_t cancel_write_length;
  int32_t modify_write_length;

 private:
  std::tr1::unordered_map<int64_t, std::tr1::unordered_map<uint64_t,uint64_t>> securityid_to_saos_orderid_map_;
  std::tr1::unordered_map<uint32_t, std::pair<int, char>> seqno_to_saos_ordertype_;
  std::tr1::unordered_map<int, std::string> saos_to_symbol_;
  BSE::BSEMessageLength bse_pre_computed_message_lengths_;
  BSE::BSEConnectionGatewayRequest bse_connection_gateway_request_;
  BSE::BSESessionRegistrationRequest bse_session_registration_request_;
  BSE::BSESessionRegistrationResponse bse_session_registration_response_;
  BSE::BSESessionLogon bse_session_logon_request_;
  BSE::BSESessionLogout bse_session_logout_request_;
  BSE::BSEUserLogon bse_user_logon_request_;
  BSE::BSEUserLogout bse_user_logout_request_;
  BSE::BSENewOrderSingleShort bse_new_order_single_short_;
  BSE::BSENewOrderSingle bse_new_order_single_;
  BSE::BSECancelOrderSingle bse_cancel_order_single_;
  BSE::BSEModifyOrderSingleShort bse_modify_order_single_short_;
  HFSAT::BSESecurityDefinitions &bse_sec_def_;
  HFSAT::Utils::BSEDailyTokenSymbolHandler &bse_daily_token_symbol_handler_;
  HFSAT::Utils::BSERefDataLoader &ref_data_loader;
  HFSAT::ORS::Settings &setting_;
  char bse_segment_type_;
  std::string bse_session_logon_request_primary_ip_address_;
  int32_t bse_session_logon_request_primary_port_;
  std::string bse_session_logon_request_secondary_ip_address_;
  int32_t bse_session_logon_request_secondary_port_;
  // Lock to make sure that sequence no.s sent are synced.
  HFSAT::Lock send_seq_num_lock_;
  int read_offset_;
  HFSAT::ORS::OrderManager& order_manager_;
  std::vector<HFSAT::FastPriceConvertor*> fast_px_convertor_vec_;
  char user_password_[LEN_PASSWORD];
  char session_password_[LEN_PASSWORD];
  string algo_id_;
  char client_code_[LEN_FREE_TEXT1];
  char cp_code_[LEN_CP_CODE];
  int last_user_pw_change_date;
  char new_session_password[LEN_PASSWORD];
  char new_user_password[LEN_PASSWORD];
  char input_buffer[8192];

  // Message logs.
  std::string output_log_dir_;
  std::string bselog_file_name_;
  std::ofstream bselog_ofs_;

  time_t last_send_time_;

 public:
  std::map<std::string, ReferenceData> exchange_symbol_to_reference_data_map_;
  std::map<int, std::string> security_id_to_exchange_symbol_map_;

  std::vector<SaosPriceStruct*> saos_to_metadata_;
  BSE::BSEHeartbeat bse_heartbeat_request_;

 public:
  time_t& lastMsgSentAtThisTime() { return last_send_time_; }
  inline bool is_engine_running() { return keep_engine_running_; }
  std::pair<int, char> GetSaosTypePairUsingSeqNo(uint32_t seq_no) {
     int saos_val = -1;  //-1 is the default return value
     char type = 'I';
     auto it = seqno_to_saos_ordertype_.find(seq_no);
     if (it != seqno_to_saos_ordertype_.end()) {
       std::pair<int, char> temp_val = it->second;
       seqno_to_saos_ordertype_.erase(it);
       return temp_val;
     } else {
       return make_pair(saos_val,type);
     }
  }
  bool is_alert_batch_cancellation;
  int32_t id_;

 private:
  unsigned int MAX_SAOS_METADATA_ENTRIES;

  bool tagging_bse_algo_;

  bool trigger_order_manager_read_now_on_;

  bool is_risk_reduction_set;
  HFSAT::Utils::TCPDirectLockFreeSocket tcp_direct_client_zocket_;


};
}
}

#endif
