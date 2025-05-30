/**
   \file BasicOrderRoutingServer/account_thread.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */

#ifndef BASE_BASICORDERROUTINGSERVER_ACCOUNTTHREAD_H
#define BASE_BASICORDERROUTINGSERVER_ACCOUNTTHREAD_H

#include <typeinfo>

#include "dvccode/Utils/tcp_client_socket.hpp"

#include "dvccode/CDef/debug_logger.hpp"

#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"

#include "infracore/BasicOrderRoutingServer/defines.hpp"
#include "infracore/BasicOrderRoutingServer/order.hpp"
#include "infracore/BasicOrderRoutingServer/position_manager.hpp"
#include "infracore/BasicOrderRoutingServer/order_manager.hpp"
#include "infracore/BasicOrderRoutingServer/engine_listener.hpp"
#include "infracore/BasicOrderRoutingServer/base_engine.hpp"
#include "dvccode/Utils/settings.hpp"
#include "infracore/BasicOrderRoutingServer/base_hbt_manager.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/Utils/lock.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/client_logging_segment_initializer.hpp"

#include "infracore/ORSUtils/broadcast_manager.hpp"
#include "dvccode/Utils/ors_trading_control_hbt_thread.hpp"
#include "infracore/ORSUtils/ors_pnl_manager.hpp"
#include "infracore/ORSUtils/ors_margin_manager.hpp"

namespace HFSAT {
namespace ORS {

/// Class that conducts all order management on behalf of this account
class AccountThread : public EngineListener {
 private:
  /// make copy constructor private to only allow reference
  AccountThread(const AccountThread&);

 public:
  AccountThread(DebugLogger& _dbglogger_, OrderManager& _order_manager_, Settings& settings,
                HFSAT::Utils::ClientLoggingSegmentInitializer* _client_logging_segment_initializer_ptr_,
                std::string t_output_log_dir_);
  ~AccountThread();
  /// order management related functions .. through various client_threads
  void SendTrade(Order* p_new_order_);
  void Cancel(const int _server_assigned_order_sequence_);
  void Cancel(Order* p_this_order_);
  void CxlReplace(Order* p_new_order_);
  void AdjustSymbol(const char*);
  void ChangeSpreadRatio(const char*, const char*);

  /// technical session related functions .. through various command threads
  void Logout();
  void DisConnect();
  void Connect();
  void Login();
  void Initialize(std::string t_output_log_dir_);

  /// technical callbacks
  void OnConnect(bool success) override {
    dbglogger_ << "Connected\n";
    dbglogger_.DumpCurrentBuffer();
    connected_ = success;
  }
  void OnLogin(bool success) override { loggedin_ = success; }
  void OnLogout() override { loggedin_ = false; }
  void OnDisconnect() override { connected_ = false; }  ///< raise signal to exit via main

  /// order management related callbacks
  void OnOrderConf(Order* ord) override;
  void OnOrderCxl(Order* ord) override;
  void OnOrderMod(Order* ord) override;
  void OnOrderExec(Order* ord) override;
  void OnReject(Order* ord) override;
  void OnCxlReject(Order* ord, CxlRejectReason_t _cxl_reject_reason_);  // --
  void OnTradeBust(Order* ord) override {
    fprintf(stderr, "TradeBust called for order %s %d @ %f \n", ord->exch_assigned_order_sequence_, ord->size_executed_,
            ord->price_);
  }

  void OnReject(int server_assigned_seqnum, const ORSRejectionReason_t rejection_reason_,
                uint64_t _exch_seq_ = 0) override;
  void OnOrderModReject(int server_assigned_sequence_number,
                        const CxlReplaceRejectReason_t rejection_reason_ = kExchCancelReplaceReject) override;
  void OnRejectDueToFunds(int server_assigned_seqnum);
  void OnCxlReject(int server_assigned_seqnum, CxlRejectReason_t _cxl_reject_reason_, uint64_t _exch_seq_ = 0) override;

  void OnOrderConf(int server_assigned_seqnum, const char* exch_assigned_seqnum, double price, int size,
                   int exch_assigned_seqnum_length_ = 0, uint64_t _exch_seq_ = 0,
                   int entry_dt = 0, int64_t last_activity_refrence_ = 0) override;  // NSE Conf
  void OnOrderConf(int server_assigned_seqnum, const char* exch_assigned_seqnum, int exch_assigned_seqnum_length_ = 0,
                   uint64_t _exch_seq_ = 0) override;
  void OnOrderCxl(int server_assigned_seqnum, uint64_t _exch_seq_ = 0) override;
  void OnOrderMod(int server_assigned_seqnum, const char* exch_assigned_seqnum, double price, int size) override;
  void OnOrderExec(int server_assigned_seqnum, const char* symbol, TradeType_t trade_type, double price,
                   int size_executed, int size_remaining, uint64_t _exch_seq_ = 0, int32_t last_mod_dt = 0, 
		   int64_t last_activity_reference_ = 0) override;
  // ramkris
  void OnTradeBust(int server_assigned_seqnum, const char* symbol, TradeType_t trade_type, double price,
                   int size_executed, int size_remaining);

  void OnOrderConfBMF(int server_assigned_seqnum, uint64_t _exch_seq_ = 0) override;
  void OnOrderCxlBMF(int server_assigned_seqnum) override;
  void OnOrderExecBMF(int server_assigned_seqnum, double price, int size_executed, int size_remaining) override;
  void OnOrderCancelReplacedBMF(int32_t server_assigned_seq_num, uint64_t exch_assigned_seq_num, double price,
                                int32_t size, std::vector<HFSAT::FastPriceConvertor*>& fast_px_convertor_vec_) override;
  void OnOrderCancelReplaced(int32_t server_assigned_seq_num, uint64_t exch_assigned_seq_num, double price,
                             int32_t size, std::vector<HFSAT::FastPriceConvertor*>& fast_px_convertor_vec_,
                             int32_t last_mod_dt = 0, int64_t last_activity_ref = 0) override;

  void CommitFixSequences();

  void CancelAllPendingOrders() override;
  void ForceCancelBroadcastSAOS(char const * saos) override;
  void ForceTradeBroadcastSAOS(char const * saos) override;
  void ForceRejectBroadcastSAOS(char const * saos) override;
  void ForceRecoverFundRejects(int sec_id);

  void LogORSTrade(const char* symbol, TradeType_t buysell, int size_executed, double price, int saos,
                   int64_t exch_assigned_seq_num, int32_t saci);

  void ProcessGeneralControlCommand(const char* input_stream, int stream_length);
  
  void OnBatchCxlAlert(int32_t user_id_) override;
  
  void EmailForIncorrectPositions(std::string _mail_body_) {
    // also send an alert
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);
    HFSAT::Email e;

    e.setSubject("Error: Incorrect ORS Positions in " + std::string(hostname));
    e.addRecepient("nseall@tworoads.co.in");
    e.addSender("nseall@tworoads.co.in");
    e.content_stream << "host_machine: " << hostname << "<br/>";
    e.content_stream << _mail_body_ << "<br/>";

    e.sendMail();
  }

  void EmailForWeirdEngineReply(std::string _mail_body_) {
    // also send an alert
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);
    HFSAT::Email e;

    e.setSubject("Error: Unexpected received from ORS engine in " + std::string(hostname));
    e.addRecepient("nseall@tworoads.co.in");
    e.addSender("nseall@tworoads.co.in");
    e.content_stream << "host_machine: " << hostname << "<br/>";
    e.content_stream << _mail_body_ << "<br/>";

    e.sendMail();
  }

  // temp for conformance
  BaseEngine* getEngine() { return p_base_engine_; }

 public:
  DebugLogger& dbglogger_;
  const SimpleSecuritySymbolIndexer& simple_security_symbol_indexer_;
  PositionManager& position_manager_;  ///< ref to global position manager to update position and order_size_sum maps
  OrderManager& order_manager_;
  BroadcastManager* bcast_manager_;
  Settings& m_settings;
  HFSAT::Utils::ClientLoggingSegmentInitializer* client_logging_segment_initializer_ptr_;

  BaseEngine* p_base_engine_;

  bool connected_;
  bool loggedin_;

  std::map<int, bool> executed_saos_map_;
  HFSAT::ExchSource_t this_exchange_source_;

  Lock saos_to_eaos_map_lock_;
  tr1::unordered_map<int, bool> sec_id_to_is_rej_funds_state_;
  tr1::unordered_map<int, TradeType_t> sec_id_to_reject_side_;

  bool using_notional_value_checks_;

  HFSAT::CDef::LogBuffer* log_buffer_;

  bool was_last_order_throttled_;
  HFSAT::ORSUtils::ORSPnlManager& ors_pnl_manager_;
  HFSAT::ORSUtils::ORSMarginManager& ors_margin_manager_;

};
}
}

#endif  // BASE_BASICORDERROUTINGSERVER_ACCOUNTTHREAD_H
