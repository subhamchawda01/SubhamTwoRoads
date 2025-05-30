// New BMFEPEngine.hpp
// Similar to CME, BMFBELL
#ifndef BMFEP_BMFFIX_ENGINE_HPP
#define BMFEP_BMFFIX_ENGINE_HPP

#include <atomic>
#include <ctime>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <fstream>

#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/CDef/fwd_decl.hpp"
#include "infracore/BasicOrderRoutingServer/defines.hpp"  // for DEF_MAX_SEC_ID
#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"

#include "infracore/BasicOrderRoutingServer/engine_listener.hpp"
#include "dvccode/Utils/settings.hpp"

#include "infracore/BasicOrderRoutingServer/base_engine.hpp"
#include "infracore/BasicOrderRoutingServer/ors_controller_thread.hpp"

#include "dvccode/Utils/spinlock.hpp"  // Thread safety
#include "dvccode/Utils/lock.hpp"      // Thread safety
#include "dvccode/Utils/multicast_receiver_socket.hpp"

#include "dvccode/Utils/tcp_client_socket.hpp"

#include "infracore/BMFEP/bmfep_fix_container.hpp"

#include "dvccode/CDef/refdata_locator.hpp"  // For BMFEP reference file location.
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/fwd_decl.hpp"
#include "dvccode/CDef/ors_messages.hpp"

#include "infracore/BasicOrderRoutingServer/exchange_playback_manager.hpp"
#include "infracore/BMFEP/bmf_playback_manager.hpp"
#include "infracore/BMFEP/bmf_playback_defines.hpp"

//====================================   FOR NEW OPTIMIZED FIX PARSER =====================================// @ Ravi

#define UINT32CASTVALUE *(uint32_t*)

#define DELIM (char)1

#define SKIP_OVER_TAG_8_WITH_VALUE_LENGTH 9       // 8=FIX.4.4
#define SKIP_OVER_TAG_9_WITHOUT_VALUE_LENGTH 2    // 9=
#define SKIP_OVER_TAG_35_WITH_VALUE_LENGTH 4      // 35=8
#define SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH 3   // 34=
#define SKIP_OVER_TAG_11_WIHTOUT_VALUE_LENGTH 3   // 11=
#define SKIP_OVER_TAG_41_WIHTOUT_VALUE_LENGTH 3   // 41=
#define SKIP_OVER_TAG_38_WIHTOUT_VALUE_LENGTH 3   // 38=
#define SKIP_OVER_TAG_32_WIHTOUT_VALUE_LENGTH 3   // 32=
#define SKIP_OVER_TAG_54_WIHTOUT_VALUE_LENGTH 3   // 54=
#define SKIP_OVER_TAG_55_WIHTOUT_VALUE_LENGTH 3   // 55=
#define SKIP_OVER_TAG_151_WIHTOUT_VALUE_LENGTH 4  // 151=
#define SKIP_OVER_SAFE_COMBINED_TAGS_49_52_56_1_6 50
#define SKIP_OVER_SAFE_COMBINED_TAGS_49_52_56 40
#define SKIP_OVER_SAFE_COMBINED_TAGS_39_40_44_48_54_55_59_60_75_150_151 50
#define SKIP_OVER_COMBINED_TAGS_14_17 15
#define SKIP_OVER_TAG_31_WITHOUT_VALUE_LENGTH 3
#define SKIP_OVER_COMBINED_TAGS_37_38_39_40_44_48_54_55_59_60_75_150 60
#define SKIP_OVER_COMBINED_TAGS_49_52_56 30
#define SKIP_OVER_SAFE_COMBINED_TAGS_14_17_37_38 30
#define SKIP_OVER_TAG_151_WITHOUT_VALUE_LENGTH 4
#define SKIP_OVER_TAG_112_WIHTOUT_VALUE_LENGTH 4
#define SKIP_OVER_TAG_198_WITHOUT_VALUE_LENGTH 4
#define SKIP_OVER_TAG_32_WITHOUT_VALUE_LENGTH 3
#define SKIP_OVER_SAFE_COMBINED_TAGS_39_40_41 14
#define SKIP_OVER_TAG_44_WITHOUT_VALUE_LENGTH 3
#define SKIP_OVER_COMBINED_TAGS_54_55_59_60_75_150 60
#define LENGTH_OF_DELIM 1  // '\001'

#define TAG_35_EXECUTIONREPORT_INT_VALUE 0x383D3533
#define TAG_11_INT_VALUE 0x3D313101
#define TAG_39_INT_VALUE 0x3D393301
#define TAG_38_INT_VALUE 0x3D383301
#define TAG_32_INT_VALUE 0x3D323301
#define TAG_54_INT_VALUE 0x3D343501
#define TAG_55_INT_VALUE 0x3D353501
#define TAG_151_INT_VALUE 0x3D313531
#define TAG_39_NEW_ORDER_INT_VALUE 0x303D3933
#define TAG_39_PARTIAL_FILL_INT_VALUE 0x313D3933
#define TAG_39_FILL_INT_VALUE 0x323D3933
#define TAG_39_CANCEL_INT_VALUE 0x343D3933
#define TAG_39_EXPIRE_INT_VALUE 0x433D3933
#define TAG_39_REPLACED_INT_VALUE 0x353D3933
#define TAG_39_REJECTED_INT_VALUE 0x383D3933
#define TAG_198_INT_VALUE 0x3D383931
#define TAG_31_INT_VALUE 0x3D313301
#define TAG_44_INT_VALUE 0x3D343401
#define TAG_151_INT_VALUE 0x3D313531
#define TAG_112_INT_VALUE 0x3D323131
#define TAG_434_INT_VALUE 0x3D343334
#define TAG_35_LOGON_RESPONSE_INT_VALUE 0x413D3533
#define TAG_35_HEARTBEAT_INT_VALUE 0x303D3533
#define TAG_35_TEST_REQUEST_INT_VALUE 0x313D3533
#define TAG_35_RESEND_REQUEST_INT_VALUE 0x323D3533
#define TAG_35_REJECT_INT_VALUE 0x333D3533
#define TAG_35_SEQUENCE_REST_INT_VALUE 0x343D3533
#define TAG_35_LOGOUT_INT_VALUE 0x353D3533
#define TAG_35_ORDER_CANCEL_REJECT_INT_VALUE 0x393D3533
#define TAG_35_BUSINESS_MESSAGE_REJECT_INT_VALUE 0x6A3D3533
#define TRAILER_TAG_INT_VALUE 0x3D303101

#define LENGTH_OF_TRAILER_TAG 8
#define MAX_DECIMAL_PLACES_SUPPORTED 8
#define MINIMUM_APPROXIMATE_LENGTH_OF_THE_TWO_OR_MORE_EXECUTION_REPORT 400
#define MAXIMUM_APPROXIMATE_LENGTH_OF_ONE_SESSION_LEVEL_MESSAGE 150

const double floating_price_decimal_lookup_[] = {1, 0.1, 0.01, 0.001, 0.0001, 0.00001, 0.000001, 0.0000001, 0.00000001};

//=============================================================================================================================================//

// Pre-production tests?
#define _NEED_MODIFY_FUNC_ false

namespace HFSAT {
namespace ORS {

using BMFEPFIX::BMFEPClientThread;

struct SequenceMetaData {
  int32_t server_assigned_sequence;
  bool is_send_otherwise_cxl_;
  bool flush_trigger_;

 public:
  SequenceMetaData() {
    server_assigned_sequence = 0;
    is_send_otherwise_cxl_ = false;
    flush_trigger_ = false;
  }
};

class BMFEPEngine : public BaseEngine, SimpleSecuritySymbolIndexerListener {
 public:
  BMFEPEngine(Settings settings, HFSAT::DebugLogger& logger, std::string output_log_dir_, int _id, AsyncWriter* pWriter,
              AsyncWriter* pReader);
  ~BMFEPEngine();

  /// technical session control calls
  void Connect();
  void DisConnect();
  void Login();
  void Logout();
  void ResendRequest(BMFEPFIX::t_FIX_SeqNum begin_seq_num);

  void sendHeartbeat();
  void CheckToSendHeartbeat();
  void ProcessMessageQueue() override;
  void ProcessFakeSend(HFSAT::ORS::Order* ord, ORQType_t type) override;

  /// BMFEP Specific calls for optimization
  void SendOrder(const Order* r_order_, BMFEPFIX::NewOrder* p_target_new_order_);
  void CancelOrder(const Order* r_order_, BMFEPFIX::CancelOrder* p_target_cancel_order_);
  void ModifyOrder(const Order* r_order_, BMFEPFIX::CancelReplace* p_target_cancel_replace_);

  void SendOrder(Order* order) override;
  void CancelOrder(Order* order) override;
  void ModifyOrder(ORS::Order *order, ORS::Order *orig_order) override;

  // From Thread class inherited from BaseEngine.
  void thread_main();

  virtual int onInputAvailable(int sock);
  virtual std::vector<int> init();

  void OnAddString(unsigned int t_num_security_id_);

  // Will be accessed from HBT manager
  time_t& lastMsgSentAtThisTime() { return last_send_time_; }
  inline bool is_engine_running() { return run_engine_; }

  // Some fields which are needed to create new order objects.
  bool make_optimize_assumptions_;

  std::string senderCompID_;
  std::string targetCompID_;
  std::string accountID_;
  // std::string senderLocationID_;
  // std::string targetLocationID_;
  std::string password_;
  std::string appl_name_;
  std::string appl_version_;

  std::string sender_location_;
  std::string entering_trader_;
  std::string entering_firm_;

  std::map<std::string, std::string> symbol_to_sec_id_map_;
  int id_;  // useful for debugging

 private:
  void sendLogon();
  void sendLogout();

  void sendTestRequest();

  void processExchMsg(char* msg_char_buf_);
  void processMsgs(char* msg_buf, int size);
  // Some callbacks called by Message processor
  void onLogon();
  void onLogout();

  void onExecReport(HFSAT::ORS::BMFEPFIX::ExecutionReport& exec_report_);
  void onCxlReject(HFSAT::ORS::BMFEPFIX::ExecutionReport& cxl_reject_report_);

  void onSessionLevelReject(HFSAT::ORS::BMFEPFIX::t_FIX_SeqNum server_seqnum_);
  void onBusinessLevelReject(HFSAT::ORS::BMFEPFIX::t_FIX_SeqNum server_seqnum_);

  // This may be used to make a clean exit, after closing the TCP connection.
  bool run_engine_;

  // TCP to exchange.
  TcpClientSocketWithLogging tcp_client_socket_;

  char socket_connect_host_[128];
  int socket_connect_port_;
  int heartbeat_interval_;

  BMFEPFixContainer container_;

  // Sequencing information
  BMFEPFIX::t_FIX_SeqNum last_seq_num_;
  BMFEPFIX::t_FIX_SeqNum last_proc_seq_num_;
  BMFEPFIX::t_FIX_SeqNum tag_7_resend_req_;

  // Session info.
  bool is_logged_in_;

  // Heartbeat and test request session info.
  bool is_uncomfirmed_heartbeat_;
  bool is_uncomfirmed_testrequest_;
  bool is_uncomfirmed_testrequest_2_;

  // Maintain last sent out time, in case of a resend request.
  time_t last_send_time_;

  // Session & business reject reasons.
  std::map<BMFEPFIX::t_FIX_SeqNum, BMFEPFIX::t_FIX_SeqNum> saos_to_msg_seq_;
  std::map<BMFEPFIX::t_FIX_SeqNum, BMFEPFIX::t_FIX_SeqNum> msg_seq_to_saos_;
  std::map<BMFEPFIX::t_FIX_SeqNum, bool> msg_seq_to_is_cxl_;

  // File to read and write sequence no. information.
  std::string seq_file_name_;
  std::string fixlog_file_name_;

  std::ofstream fixlog_ofs_;

  std::vector<unsigned int> security_id_list_;
  HFSAT::BulkFileWriter* fix_bulk_writer_;
  HFSAT::BulkFileWriter* fix_bulk_writer_our_msg;

  bool resend_request_processed_;

  bool use_dropcopy_mcast_packets_;
  HFSAT::MulticastReceiverSocket dropcopy_multicast_receiver_socket_;

  std::map<int, bool> on_exec_saos_processed_;
  std::map<int, bool> on_conf_saos_processed_;
  std::map<int, bool> on_cxl_saos_processed_;
  std::map<int, bool> on_mod_saos_processed_;
  std::map<int, bool> on_rej_saos_processed_;
  std::map<int, bool> on_cxlrej_saos_processed_;
  std::map<int, bool> on_tradebust_saos_processed_;
  std::map<std::string, bool> on_pexec_saos_processed_;
  char* msg_char_buf_;
  int read_offset_;
  std::string this_exch_source_;

 private:
  char* current_processing_ptr_;
  char* current_tag_11_sequence_number_ptr_;

  int32_t last_processed_exchange_reply_sequence_number_;
  int32_t current_server_assigned_sequnece_in_processing_;
  uint64_t current_exchange_sequence_number_;

  double current_execution_price_;
  int32_t current_execution_price_factor_;
  uint32_t current_executed_qty_;
  uint32_t current_remaining_qty_;
  bool started_processing_exec_notifs_;

  bool is_resend_request_sent_in_playback_mode_;
  bool is_live_order_cancelled_in_playback_mode_;
  int last_heartbeat_seq_num_in_playback_mode_;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> playback_mgr_;
  std::set<std::string> playback_exchange_symbols_;
  bool is_exchange_stp_enabled_;

  void BeginPlaybackMode();
  void CancelLiveOrdersInPlaybackMode();
  void EndPlaybackMode();
  void ProcessHeartbeatForPlayback(const int heartbeat_seq_num);

  inline bool ProcessExchangeSequenceAndNotifyFurtherProcessing(const uint64_t& _received_seq_,
                                                                const uint64_t& _last_processed_seq_num_,
                                                                const bool& _is_logout_);
  inline void ProcessLogonResponse(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                                   bool& _optimized_processing_failed_);
  inline void ProcessHeartbeat(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                               bool& _optimized_processing_failed_);
  inline void ProcessResendRequest(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                                   bool& _optimized_processing_failed_);
  inline void ProcessReject(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                            bool& _optimized_processing_failed_);
  inline void ProcessOrderCancelReject(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                                       bool& _optimized_processing_failed_);
  inline void ProcessBusinessMessageReject(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                                           bool& _optimized_processing_failed_);
  inline void ProcessLogout(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                            bool& _optimized_processing_failed_);
  inline void ProcessTestRequest(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                                 bool& _optimized_processing_failed_);
  inline void ProcessSequenceReset(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                                   bool& _optimized_processing_failed_);
  inline void ProcessOrderConfirmation(char* _start_of_tag_39_ptr_, char* _single_message_end_ptr_,
                                       bool& _optimized_processing_failed_);
  inline void ProcessPartialAndCompleteFill(char* _start_of_tag_11_ptr_, char* _single_message_end_ptr_,
                                            bool& _optimized_processing_failed_);
  inline void ProcessOrderCancellation(char* _start_of_tag_39_ptr_, char* _single_message_end_ptr_,
                                       bool& _optimized_processing_failed_);
  inline void ProcessCancelReplacement(char* _start_of_tag_39_ptr_, char* _single_message_end_ptr_,
                                       bool& _optimized_processing_failed_);
  inline void ProcessExchangeReject(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                                    bool& _optimized_processing_failed_);
  inline void ProcessExecutionReport(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                                     bool& _optimized_processing_failed_);
  inline void ProcessSingleMessage(char* _single_message_start_ptr_, char* _single_message_end_ptr_,
                                   bool& _optimized_processing_failed_, bool& _processed_session_level_message_);
  inline void ProcessExchangeReply(char* _exchange_reply_, char* _end_of_complete_message_ptr_,
                                   const int32_t& _msg_length_, bool& _optimized_processing_failed_);
  inline int32_t DecodeBMFMessage(char* _msg_char_buf_, const int32_t& _msg_length_,
                                  bool& _optimized_processing_failed_);

  inline void UpdateSequenceMetadataVector(const BMFEPFIX::t_FIX_SeqNum& seqnum_, const int& order_saos_,
                                           const bool& is_send_or_cxl_, const bool& flush_);

 private:
  std::atomic<uint32_t> MAX_SEQUENCE_NUMBERS_WE_EXPECT_DURING_ONE_RUN;
  // This Vector helps us identifying
  std::vector<SequenceMetaData*> sequence_to_metadata_vec_;
  int32_t starting_sequence_offset_;
  std::vector<HFSAT::FastPriceConvertor*> fast_px_convertor_vec_;

  void SendEmailNotification(const std::string _mail_body_) {
    // also send an alert
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);
    HFSAT::Email e;

    e.setSubject("FATAL ERROR : ENGINE PROCESSING FAILURE");
    e.addRecepient("nseall@tworoads.co.in");
    e.addSender("nseall@tworoads.co.in");
    e.content_stream << "host_machine: " << hostname << "<br/>";
    e.content_stream << _mail_body_ << "<br/>";

    e.sendMail();
  }

  GenericORSRequestStruct processe_queue_req_;

  bool is_test_request_cached_;
  bool is_resend_request_cached_;
  bool is_seq_reset_cached_;
  bool is_dup_resend_;

 public:
  void CommitFixSequences();
};
}
}

#endif
