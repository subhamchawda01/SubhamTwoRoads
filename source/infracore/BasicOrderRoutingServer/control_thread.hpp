/**
    \file BasicOrderRoutingServer/control_thread.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */

#ifndef BASE_BASICORDERROUTINGSERVER_CONTROLTHREAD_H
#define BASE_BASICORDERROUTINGSERVER_CONTROLTHREAD_H

#include <time.h>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/assumptions.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/tcp_server_socket.hpp"

#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "infracore/BasicOrderRoutingServer/margin_checker.hpp"
#include "infracore/BasicOrderRoutingServer/account_manager.hpp"
#include "dvccode/Utils/settings.hpp"
#include "infracore/BasicOrderRoutingServer/position_manager.hpp"
#include "infracore/BasicOrderRoutingServer/order_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/send_alert.hpp"

#include "infracore/BasicOrderRoutingServer/control_receiver.hpp"
#include "infracore/ORSUtils/ors_pnl_manager.hpp"
#include "infracore/ORSUtils/ors_margin_manager.hpp"

// A better place for this would be in the ORS itself, rather than in the control_thread.
static int margin_server_fd_ = -1;

namespace HFSAT {
namespace ORS {

struct MiFidLimits {
  int otr_min_vol;
  int otr_min_cnt;
  int otr_limit_vol;
  int otr_limit_cnt;
  int msg_count;
};

/// Separate thread that interacts with one control_client,
/// receives control messages and acts accordingly.
class ControlThread : public Thread {
 public:
  ControlThread(DebugLogger& _dbglogger_, const int _connected_socket_file_descriptor_, Settings& _settings_,
                HFSAT::Utils::ClientLoggingSegmentInitializer* _client_logging_segment_initializer_,
                std::string t_output_log_dir_, bool &_cancel_live_order_flag_,
                std::map<std::string, int>& _sec_to_max_pos_map_,
                std::map<std::string, int>& _sec_to_max_ord_sz_map_, HFSAT::ORS::ControlReceiver*, 
		int32_t thread_id, bool* _is_addts_running_);

  ~ControlThread() {
    dbglogger_ << "Margin server fd " << margin_server_fd_ << "\n";
    dbglogger_.DumpCurrentBuffer();
    Close();
    HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  }

  /// inherited from Thread.
  /// start a while loop of accept
  void thread_main();

  void StopControlThread();

 protected:
 private:
  DebugLogger& dbglogger_;
  SimpleSecuritySymbolIndexer& simple_security_symbol_indexer_;
  MarginChecker& margin_checker_;
  int connected_socket_file_descriptor_;  ///< socket id returned on accept, for this client.

  AccountThread* m_athread;
  Settings& m_settings;
  HFSAT::Utils::ClientLoggingSegmentInitializer* client_logging_segment_initializer_;

  std::map<std::string, int>& sec_to_max_pos_map_;
  std::map<std::string, int>& sec_to_max_ord_sz_map_;
  std::map<std::string, MiFidLimits> shc_to_mifid_limits_;

  /// for reponse to GUI based commands
  int keep_control_threads_running_;

  // added to keep the ors free from dumping positions unnecessarily
  PositionManager& position_manager_;
  OrderManager& order_manager_;

  HFSAT::ORS::ControlReceiver* control_receiver_;
  std::set<std::string> list_of_securities_under_ban_;
  std::set<std::string> list_of_securities_under_physical_settlement_;
  std::set<std::string> list_of_securities_with_unsolicited_sms_;
  std::string exchange_;
  static bool banned_products_listed_;
  static bool physical_settlement_products_listed_;
  static bool unsolicited_sms_products_listed_;

  int32_t thread_id;
  bool is_mifid_check_applicable_;
  bool* is_addts_running_;
  bool &cancel_live_order_flag_;
  void sendControlStruct();

  void Close();
  void sendBaseId();
  void LoadNSEFOSecuritiesUnderBan();
  void LoadNSEFOSecuritiesUnderPhysicalSettlement();
  void LoadNSESecuritiesUnderUnsolicitedSMSList();
  void LoadMifidLimitsFile();
  // MFGlobal checks on the risk parameters.
  bool acceptableMaxPosParams1(std::string& shortcode_, int& max_pos_, int& max_ord_size_);
  bool acceptableMaxPosParams(std::string shortcode_, std::string max_pos_, std::string max_ord_size_,
                              std::string max_worst_case_size_, std::string max_live_ord_);
  void SendEmail(std::string subject, std::string body_);
  void EmailAddSymbolFailed(std::string alert_body_);
  void EmailADDTSCapped(std::string report_string);
  bool IsNumber(const char* str);
  int GetKthPercentOf(int num, int k);
  void DisplaySACIPositions(std::string& out_filename, const std::string& saci_cfg_filename = std::string());
  void DisplaySecurityPositions(std::string& out_filename);
  bool AddMifidLimits(std::string shortcode, int sec_id);

  void AddtradingSymbol(std::string& shortcode_temp, int max_pos, int max_order_size,std::ostringstream& report_string);

  std::map<int, std::string>& SidToShortCodeMap() {
    static std::map<int, std::string> unique_instance_;
    return unique_instance_;
  }
};
}
}
#endif  //  BASE_BASICORDERROUTINGSERVER_CONTROLTHREAD_H
