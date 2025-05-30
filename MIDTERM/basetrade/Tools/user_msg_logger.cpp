/**
   \file Tools/user_msg_logger.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   India
   +91 80 4190 3551
*/
#include <signal.h>

#include "dvccode/CDef/debug_logger.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

#include "dvccode/TradingInfo/network_account_info_manager.hpp"

#include "dvccode/ExternalData/simple_live_dispatcher.hpp"

#include "dvccode/ORSMessages/control_message_listener.hpp"
#include "dvccode/ORSMessages/control_message_livesource.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

class ControlMessageLogger : public HFSAT::ControlMessageListener {
 public:
  ControlMessageLogger(HFSAT::DebugLogger &_dbglogger_, HFSAT::Watch &_watch_)
      : dbglogger_(_dbglogger_), watch_(_watch_) {}

  ~ControlMessageLogger() {}

  void OnControlUpdate(const HFSAT::ControlMessage &_control_message_, const char *symbol_, const int trader_id) {
    std::ostringstream t_oss_;
    t_oss_ << watch_.UTCTimeString() << " [" << symbol_ << "] [" << trader_id << "] [" << _control_message_ << "]\n";
    dbglogger_ << t_oss_.str();
    dbglogger_.DumpCurrentBuffer();
  }

 private:
  HFSAT::DebugLogger &dbglogger_;
  HFSAT::Watch &watch_;
};

HFSAT::DebugLogger dbglogger_(10240, 1);

void termination_handler(int signal_) {
  dbglogger_.Close();

  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "USAGE : " << argv[0] << " TRADER_ID1 TRADER_ID2 .. " << std::endl;
    exit(-1);
  }

  signal(SIGINT, termination_handler);

  std::vector<int> trader_ids_vec_;
  for (int arg_ = 1; arg_ < argc; ++arg_) {
    trader_ids_vec_.push_back(atoi(argv[arg_]));
  }

  int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "/home/sghosh/logs/alllogs/user_msg_logger." << tradingdate_;
  std::string logfilename_ = t_temp_oss_.str();

  dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::app);
  dbglogger_.DumpCurrentBuffer();

  HFSAT::Watch watch_(dbglogger_, tradingdate_);

  HFSAT::SimpleLiveDispatcher simple_live_dispatcher_;

  std::string network_account_info_filename_ = HFSAT::FileUtils::AppendHome(
      std::string(BASESYSINFODIR) + "TradingInfo/NetworkInfo/network_account_info_filename.txt");
  HFSAT::NetworkAccountInfoManager network_account_info_manager_(network_account_info_filename_);

  // Create a livesource for control messages.
  HFSAT::DataInfo control_recv_data_info_ = network_account_info_manager_.GetControlRecvDataInfo();
  HFSAT::ControlMessageLiveSource *p_control_message_livesource_ = new HFSAT::ControlMessageLiveSource(
      dbglogger_, control_recv_data_info_.bcast_ip_, control_recv_data_info_.bcast_port_,
      HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_Control));

  ControlMessageLogger *p_control_message_logger_ = new ControlMessageLogger(dbglogger_, watch_);

  for (unsigned int trader_id_ = 0; trader_id_ < trader_ids_vec_.size(); ++trader_id_) {
    p_control_message_livesource_->AddControlMessageListener(trader_ids_vec_[trader_id_], p_control_message_logger_);
  }
  p_control_message_livesource_->SetExternalTimeListener(&watch_);

  simple_live_dispatcher_.AddSimpleExternalDataLiveListenerSocket(
      p_control_message_livesource_, p_control_message_livesource_->socket_file_descriptor());

  // start event loop
  simple_live_dispatcher_.RunLive();
}
