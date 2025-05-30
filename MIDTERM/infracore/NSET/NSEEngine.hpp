// =====================================================================================
//
//       Filename:  NSEEngine.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/18/2014 08:30:37 AM
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

#include <ctime>
#include <stack>
#include "dvccode/CDef/fwd_decl.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "infracore/BasicOrderRoutingServer/base_engine.hpp"
#include "infracore/BasicOrderRoutingServer/sequence_generator.hpp"
#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/lock.hpp"
#include "dvccode/Utils/nse_refdata_loader.hpp"
#include "dvccode/Utils/vector_bucket.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

// All Defines For NSET
#include "infracore/NSET/NSETemplates/DataDefines.hpp"
#include "infracore/ORSUtils/broadcast_manager.hpp"
#include "infracore/NSET/NSETemplates/OrderPriceChangeRequest.hpp"  //required for optimised order modify

#include "infracore/NSET/nse_container.hpp"
#include "dvccode/Utils/ors_trading_control_hbt_thread.hpp"

#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/Utils/nse_daily_token_symbol_handler.hpp"
#include "infracore/NSET/nse_msg_handler_cash_market.hpp"
#include "infracore/NSET/nse_msg_handler_derivatives.hpp"

namespace HFSAT {
namespace NSE {

class NSEEngine : public ORS::BaseEngine, public SimpleSecuritySymbolIndexerListener {
 private:
  struct NSEOrderMetaData {
    int32_t entry_date_time;
    int32_t last_modified_date_time;
    int32_t state;  // 0 : no such order, 1:order sent, 2:first exec received
    int32_t last_remaining_size;
    int32_t price_1;
    int32_t price_2;
    int16_t buy_sell;
    int16_t buy_sell_1;
    void Initialize() {
      state = 0;
      price_1 = -1;
      price_1 = -1;
      buy_sell = -1;
      buy_sell_1 = -1;
      entry_date_time = 0;
      last_modified_date_time = 0;
    }
  };

  bool keep_engine_running_;
  time_t last_send_time_;
  TcpClientSocketWithLogging *nse_session_socket_;
  TCPClientSocket *nse_gr_socket_;
  int32_t read_offset_;
  char nse_msg_buffer_[MAX_NSE_RESPONSE_BUFFER_SIZE];

  // Processed Response Holders
  ProcessedPacketHeader *processed_packet_header_;
  ProcessedResponseHeader *processed_inner_response_header_;
  ProcessedResponseHeader *processed_response_header_;
  ProcessedSystemInformationResponse *processed_system_information_response_;
  ProcessedLogonResponse *processed_logon_response_;
  ProcessedGRResponse *processed_gr_response_;
  ProcessedBoxLoginResponse *processed_boxlogin_response_;
  ProcessedSeurityUpdateInfoResponse *processed_security_update_info_response_;
  ProcessedSecurityStatusUpdateInfo *processed_security_status_update_response_;
  ProcessedParticipantUpdateInfo *processed_participant_update_info_response_;
  ProcessedDownloadIndex *processed_index_update_info_response_;
  ProcessedDownloadIndexMap *processed_index_map_update_info_response_;
  ProcessedInstrumentUpdateInfo *processed_instrument_update_info_response_;
  ProcessedOrderResponse *processed_order_response_;
  ProcessedTradeConfirmationResponse *processed_trade_response_;
  ProcessedOrderResponseNonTR *processed_order_response_non_tr_;

  // nse msg handler
  NseMsgHandler *nse_msgs_handler_;
  bool is_optimized_modify_supported_;
  // Optimised modify template: only specific to derivatives
  OrderPriceModifyRequest pr_modify_order_;

  std::vector<std::string> nse_gateway_ip_list_;
  std::string nse_gateway_ip_;
  int32_t nse_gateway_port_;
  HFSAT::DebugLogger &dbglogger_;
  bool use_affinity_;
  uint32_t next_message_sequnece_;
  uint32_t last_processed_sequnce_;
  bool is_logged_in_;
  bool is_connected_;
  bool allow_new_orders_;
  std::map<std::string, uint32_t> exchange_symbol_to_exchange_security_code_map_;
  std::map<uint32_t, std::string> exchange_security_code_to_exchange_symbol_map_;
  uint32_t msecs_from_midnight_;
  uint32_t last_midnight_sec_;
  std::vector<int32_t> unique_message_sequence_to_saos_vec_;
  std::vector<uint32_t> saos_to_unique_message_sequence_vec_;
  char branch_code_;
  uint16_t branch_sequence_number_;
  uint32_t heartbeat_interval_;
  int32_t user_id_;
  std::string pan_;
  std::string broker_id_;
  int32_t branch_id_;
  int32_t version_;
  int64_t branch_wise_limit_;
  double nnf_;
  int32_t price_multiplier_;
  std::string account_;
  std::string subaccount_;
  std::string executing_giveup_firm_number_;
  char origin_type_;
  std::string origin_type_str_;
  std::string tag_50_clearing_info_;
  std::stack<uint32_t> gmd_pending_sequences_stack_;
  NSEContainer container_;
  tr1::unordered_map<int64_t, int32_t> exch_order_num_to_saos_;
  HFSAT::Utils::NSERefDataLoader &ref_data_loader;
  HFSAT::BroadcastManager *bcast_manager_;
  HFSAT::Utils::NSEDailyTokenSymbolHandler &nse_daily_token_symbol_handler_;
  std::map<int32_t, std::string> token_to_internal_exchange_symbol_;
  HFSAT::NSESecurityDefinitions &nse_sec_def_;
  char nse_segment_type_;
  std::vector<HFSAT::FastPriceConvertor *> fast_px_convertor_vec_;
  bool keep_trying_different_gateways_;
  std::atomic<bool> is_local_db_updated_;
  HFSAT::ORS::Settings &setting_;
  bool is_mkt_order_;
  bool is_pre_open_;
  std::string trading_ip_;
  int32_t trading_port_;
  char session_key_[8];
  int16_t box_id_;
  std::string tap_ip_;
  std::string tap_gateway_;
  std::string tap_interface_;
  int32_t algo_id_;
  bool is_alert_batch_cancellation;

 public:
  int32_t id_;

 public:
  NSEEngine(HFSAT::ORS::Settings &settings, HFSAT::DebugLogger &dbglogger, std::string output_log_dir,
            int32_t engine_id, AsyncWriter *pWriter, AsyncWriter *pReader);
  ~NSEEngine();

  void CleanUp();

  // OrderManagement
  void SendOrder(HFSAT::ORS::Order *order, InstrumentDesc *inst_desc);
  void CancelOrder(HFSAT::ORS::Order *order, InstrumentDesc *inst_desc);
  void ModifyOrder(HFSAT::ORS::Order *order, InstrumentDesc *inst_desc, ORS::Order *orig_order);
  void OptModifyOrder(HFSAT::ORS::Order *order, int32_t inst_id, ORS::Order *orig_order);
  void OnAddString(uint32_t num_sid);
  void SendOrder(ORS::Order *order) override;
  void CancelOrder(ORS::Order *order) override;
  void ModifyOrder(ORS::Order *order, ORS::Order *orig_order) override;
  void OnControlHubDisconnectionShutdown() final;

  inline time_t &lastMsgSentAtThisTime() { return last_send_time_; }
  inline bool is_engine_running() { return keep_engine_running_; }

  void AdjustSymbol(const char *);
  void Connect();
  void DisConnect();
  void Login();
  void GRRequest();
  void BoxLoginRequest();
  void Logout();
  void SendHeartbeatReply();
  void CheckToSendHeartbeat();
  void ProcessFakeSend(HFSAT::ORS::Order *ord, ORQType_t type) override;
  void SendSystemInformationRequest();
  void SendUpdateLocalDBRequest(st_market_status &st_mkt_status, st_ex_market_status &st_ex_mkt_status,
                                st_pl_market_status &st_pl_mkt_status, int16_t call_auction_1, int16_t call_auction_2);

  uint32_t ProcessExchangeResponse(char *nse_msg_buffer, const uint32_t &msg_length);
  void ProcessLogonResponse(const char *msg_ptr);
  void ProcessGRResponse(const char *msg_ptr);
  void ProcessBoxLoginResponse(const char *msg_ptr);
  void ProcessLogonFailure(const char *msg_ptr, int16_t error_code);
  void ProcessSystemInformationResponse(const char *msg_ptr);
  inline void ProcessHeartbeatResponse(const char *msg_ptr);
  void ProcessLogoutResponse(const char *msg_ptr);
  void ProcessUpdateLocalDBResponse(const char *msg_ptr);

  void ProcessBcastSecurityMessage(const char *msg_ptr);
  void ProcessBcastSecurityStatusMessage(const char *msg_ptr);
  void ProcessBcastParticipantMessage(const char *msg_ptr);
  void ProcessBcastIndexMessage(const char *msg_ptr);
  void ProcessBcastIndexMapMessage(const char *msg_ptr);
  void ProcessBcastInstrumentMessage(const char *msg_ptr);
  void ProcessOrderConfirmation(const char *msg_ptr);
  void ProcessOrderCancellation(const char *msg_ptr);
  void ProcessOrderModification(const char *msg_ptr);
  void ProcessOrderCxlRejection(const char *msg_ptr);
  void ProcessOrderModRejection(const char *msg_ptr);
  void ProcessOrderRejection(const char *msg_ptr);
  void ProcessTradeConfirmation(const char *msg_ptr);
  void ProcessOrderRejectionNonTR(const char *msg_ptr, int16_t error_code);
  void ProcessOrderCxlRejectionNonTR(const char *msg_ptr, int16_t error_code);
  void ProcessOrderModRejectionNonTR(const char *msg_ptr, int16_t error_code);
  void ProcessControlMessage(const char *msg_ptr);
  std::vector<int> init();

  inline void thread_main();
  virtual int32_t onInputAvailable(int32_t socket);
  void ProcessGeneralControlCommand(const char *input_stream, int stream_length) override;

  void printHexString(const char *c, int len);
  char *trimwhitespace(char *str);  // required to trim extra spaces in string attributes set by exchange

 private:
  std::string GetNewPassword();
  void WaitForLoginSuccess();
  void EmailPasswordChange(std::string alert_msg_);
  bool ValidatePostMarketTimings();
  bool ValidatePreOpenMarketTimings();
};
}
}
