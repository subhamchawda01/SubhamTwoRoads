/**
   \file OrderRoutingServer/cme_ilink_ors.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/


#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <chrono>

#include <boost/program_options.hpp>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/online_debug_logger.hpp"

#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"

#include "infracore/BasicOrderRoutingServer/control_receiver.hpp"
#include "dvccode/Utils/settings.hpp"
#include "infracore/BasicOrderRoutingServer/sequence_generator.hpp"

#include "infracore/BasicOrderRoutingServer/account_manager.hpp"
#include "infracore/BasicOrderRoutingServer/order_manager.hpp"
#include "infracore/BasicOrderRoutingServer/ors_controller_thread.hpp"

#include "infracore/BasicOrderRoutingServer/liffe_clord_id_generator.hpp"
#include "infracore/BasicOrderRoutingServer/client_data_receiver.hpp"
#include "dvccode/Utils/client_logging_segment_initializer.hpp"
#include "dvccode/Utils/onload_tcp_warmup_thread.hpp"
#include "dvccode/Utils/saci_generator.hpp"
#include "infracore/ORSUtils/broadcast_manager.hpp"
#include "infracore/ORSUtils/ors_pnl_manager.hpp"
#include "infracore/ORSUtils/ors_margin_manager.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "dvccode/Utils/html_generator.hpp"

#include "dvccode/IBUtils/ControlCommandClient.hpp"
#define BSE_DEBUG_INFO 0

// Made loggers global to access in termination_handler
// HFSAT::OnlineDebugLogger *online_dbglogger_ = new HFSAT::OnlineDebugLogger(1024,-1);
// HFSAT::DebugLogger &dbglogger_ = *(online_dbglogger_);
HFSAT::DebugLogger dbglogger_(4 * 1024 * 1024, 1);

// Made p_settings_ global to access in termination_handler
HFSAT::ORS::Settings* p_settings_ = NULL;
HFSAT::ORS::ControlReceiver* p_global_control_receiver_ = NULL;

HFSAT::ORS::ClientDataReceiver* p_client_data_receiver = NULL;

HFSAT::Utils::ClientLoggingSegmentInitializer* client_logging_segment_initializer_ptr = NULL;

HFSAT::BroadcastManager* bcast_manager_ = NULL;

// Create shm client thread when
// a. The ors isn't in the dropcopy mode
// b. and the config has Use_SHM specified
bool UseShm() {
  if (p_settings_ == NULL) {
    return false;
  }

  if (p_settings_->has("Dropcopy") && p_settings_->getValue("Dropcopy") == "yes") {
    return false;
  }

  return (p_settings_->has("Use_SHM") && p_settings_->getValue("Use_SHM") == "yes");
}

// We actually don't want to create tcp based client receiver
// Hence, this is deprecated. Remove this as soon as possible.
bool UseTcp() {
  if (p_settings_ == NULL) {
    return false;
  }

  return false;
}

int main(int argc, char** argv) {
  
  // signal handling, Interrupts and seg faults
//   signal(SIGINT, termination_handler);
//   signal(SIGSEGV, termination_handler_segv);
//   signal(SIGPIPE, SIG_IGN);
/*
  HFSAT::CpucycleProfiler::SetUniqueInstance(50);
  HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(1, "EncryptTimeSend");
  HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(2, "MD5");
  HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(3, "TCPDirect");
*/
  boost::program_options::options_description desc("Allowed Options");
  desc.add_options()("help", "produce help message.")("config", boost::program_options::value<std::string>(),
                                                      "(required) ors configuration file.")(
      "output-log-dir", boost::program_options::value<std::string>()->default_value("/spare/local/curr_portfolio_log"),
      "debuglogger dir")
      ("html-file", boost::program_options::value<std::string>()->default_value("/var/www/html/curr_portfolio_check.html"));

  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);
  if (vm.count("config")) {
    std::string configFile = vm["config"].as<std::string>();
    p_settings_ = new HFSAT::ORS::Settings(configFile);
  } else {
    std::cout << desc << std::endl;
    exit(0);
  }
  std::string html_file_=vm["html-file"].as<std::string>();
  std::string t_output_log_dir_ = vm["output-log-dir"].as<std::string>();
  std::string control_ip_="127.0.0.1";
  int control_recv_port_ = -1;

  if (p_settings_->has("Control_Port")) {
    control_recv_port_ = atoi(p_settings_->getValue("Control_Port").c_str());
  }

  int client_recv_port_ = -1;
  if (p_settings_->has("Client_Port")) client_recv_port_ = atoi(p_settings_->getValue("Client_Port").c_str());

  std::cout<<"Html file: "<<html_file_<<std::endl;
  HFSAT::Utils::HTMLGenerator::setFilePath(html_file_);
  std::string bcast_ip_ = "";
  int bcast_port_ = -1;

  // If ORS config has the Bcast IP and Port than
  // use this to override the n/w info values
  if (true == p_settings_->has("BCast_IP") && true == p_settings_->has("BCast_Port")) {
    bcast_ip_ = p_settings_->getValue("BCast_IP");
    bcast_port_ = atoi(p_settings_->getValue("BCast_Port").c_str());
  } else {
    HFSAT::NetworkAccountInfoManager network_account_info_manager;

    HFSAT::ExchSource_t exch_source = HFSAT::StringToExchSource(p_settings_->getValue("Exchange"));
    if (HFSAT::kExchSourceInvalid == exch_source) {
      std::cerr << "Invalid Exch Source From Exchange String : " << p_settings_->getValue("Exchange") << std::endl;
      std::exit(-1);
    }

    // ZN_0 used here is just a dummy, It could be anything irrespective of Exchange,as we don't use it
    HFSAT::DataInfo data_info = network_account_info_manager.GetDepDataInfo(exch_source, "ZN_0");
    bcast_ip_ = data_info.bcast_ip_;
    bcast_port_ = data_info.bcast_port_;

    if (std::string("127.0.0.1") == bcast_ip_ || 0 == bcast_ip_.length() || 11111 == bcast_port_ || -1 == bcast_port_) {
      std::cerr << "Unable To Fetch ORS Reply Network Details, Got Bcast IP : " << bcast_ip_
                << " Port : " << bcast_port_ << std::endl;
      std::exit(-1);
    }
  }

  std::cout << "Use_Shm " << p_settings_->getValue("Use_SHM") << "Exchange " << p_settings_->getValue("Exchange")
            << std::endl;

  if ((client_recv_port_ < 10000) || (client_recv_port_ > 50000)) {
    std::cerr << " client_recv_port_ " << client_recv_port_ << " not in range 10000 50000 " << std::endl;
  }
  if ((control_recv_port_ < 10000) || (control_recv_port_ > 50000)) {
    std::cerr << " control_recv_port_ " << control_recv_port_ << " not in range 10000 50000 " << std::endl;
  }

  {
    std::ostringstream t_temp_oss_;
    int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
    t_temp_oss_ << vm["output-log-dir"].as<std::string>() << "/log." << tradingdate_;
    std::string logfilename_ = t_temp_oss_.str();

    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out | std::ofstream::app);
    dbglogger_ << "Opened portfolio_check_logfile in Append mode!\n";
    dbglogger_.DumpCurrentBuffer();

    dbglogger_ << p_settings_->ToString() << "\n";
    dbglogger_.DumpCurrentBuffer();
    // dbglogger_.AddLogLevel ( ORS_INFO );
    dbglogger_.AddLogLevel(ORS_ERROR);
  }

  dbglogger_ << "portfolio_check REPLY SET UP ON : " << bcast_ip_ << " X " << bcast_port_ << "\n";
  dbglogger_.DumpCurrentBuffer();

  std::string exch_;
  if (p_settings_->getValue("Exchange") == std::string("NSE_CD") ||
      p_settings_->getValue("Exchange") == std::string("NSE_FO") ||
      p_settings_->getValue("Exchange") == std::string("MSSIM") ||
      p_settings_->getValue("Exchange") == std::string("NSE_EQ")) {
      exch_ = "NSE";
  }
  else if (p_settings_->getValue("Exchange") == std::string("BSE_CD") ||
           p_settings_->getValue("Exchange") == std::string("BSE_FO") ||
           p_settings_->getValue("Exchange") == std::string("BSE_EQ")) {
       exch_ = "BSE";
  }
  else if (p_settings_->getValue("Exchange") == std::string("CBOE_CD") ||
           p_settings_->getValue("Exchange") == std::string("CBOE_FO") ||
           p_settings_->getValue("Exchange") == std::string("CBOE_EQ")) {
       exch_ = "CBOE";
  }

  int tradingdate_ = HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate();

  struct timeval current_time;
  gettimeofday(&current_time, NULL);

    // Make Sure This Is Done Above All Other Classe's Initialization, Needed For ASX TICK Changes
  HFSAT::SecurityDefinitions::ResetASXMpiIfNeeded(tradingdate_, (time_t)current_time.tv_sec);
 //std::cout<<"LOADING security"<< std::string(p_settings_->getValue("Exchange")<<std::endl;
  if (std::string("NSE_CD") == std::string(p_settings_->getValue("Exchange")) ||
      std::string("NSE_EQ") == std::string(p_settings_->getValue("Exchange")) ||
      std::string("NSE_FO") == std::string(p_settings_->getValue("Exchange")) ||
      std::string("SIM") == std::string(p_settings_->getValue("Exchange")) ||
      std::string("MSSIM") == std::string(p_settings_->getValue("Exchange"))) {
    std::cout<<"LOADING nse security"<<std::endl;
    HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
  }
  else if (std::string("BSE_CD") == std::string(p_settings_->getValue("Exchange")) ||
           std::string("BSE_EQ") == std::string(p_settings_->getValue("Exchange")) ||
           std::string("BSE_FO") == std::string(p_settings_->getValue("Exchange"))) {
    std::cout<<"LOADING bse security"<<std::endl;
    HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadBSESecurityDefinitions();
  }
  else if (std::string("CBOE_CD") == std::string(p_settings_->getValue("Exchange")) ||
           std::string("CBOE_EQ") == std::string(p_settings_->getValue("Exchange")) ||
           std::string("CBOE_FO") == std::string(p_settings_->getValue("Exchange"))) {
    std::cout<<"LOADING cboe security"<<std::endl;
    HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadCBOESecurityDefinitions();
  }

  char output_dir[512];
  char trades_file_name[256];

  memset((void*)output_dir, 0, 512);
  memset((void*)trades_file_name, 0, 256);

  memcpy((void*)output_dir, std::string(vm["output-log-dir"].as<std::string>()).c_str(),
         std::string(vm["output-log-dir"].as<std::string>()).length());

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance();

  // We set up recovery file for the position manager
  {
    std::ostringstream t_temp_oss_;
    int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
    t_temp_oss_ << vm["output-log-dir"].as<std::string>() << "/position." << tradingdate_;
    std::string positionfilename_ = t_temp_oss_.str();

    // This will open the recovery file and setup the positions according to the last saved state.
    HFSAT::ORS::PositionManager::GetUniqueInstance().SetRecoveryFile(positionfilename_);
    std::string position_state_ = (HFSAT::ORS::PositionManager::GetUniqueInstance().DumpPMState());
    dbglogger_ << position_state_ << "\n";
    dbglogger_.DumpCurrentBuffer();
  }

  std::cout << "benchmark 1\n";

  // Assuming the client_base is always present and valid value in the configuration file
  HFSAT::BroadcastManager* bcast_manager_ = HFSAT::BroadcastManager::GetUniqueInstance(
      dbglogger_, bcast_ip_, bcast_port_, (atoi(p_settings_->getValue("Client_Base").c_str())) << 16);
  bcast_manager_->StartThread();
 
  std::cout << "benchmark 3\n";
  HFSAT::ORS::AccountManager::GetNewAccountThread(dbglogger_, HFSAT::ORS::OrderManager::GetUniqueInstance(),
                                                  *p_settings_, client_logging_segment_initializer_ptr,
                                                  t_output_log_dir_);
  std::cout << "benchmark 4\n";
    std::cout << "ClientDataReceiver instance\n"; 
  p_client_data_receiver =
      new HFSAT::ORS::ClientDataReceiver(dbglogger_, *p_settings_, p_settings_->getValue("Exchange"));
  
  std::cout<<"Cancel live orders :"<<p_client_data_receiver->cancel_live_order_<<std::endl;

  HFSAT::ORS::ControlReceiver control_receiver_(dbglogger_, *p_settings_, client_logging_segment_initializer_ptr,
                                                vm["output-log-dir"].as<std::string>(),p_client_data_receiver->cancel_live_order_);
  p_global_control_receiver_ = &control_receiver_;
  control_receiver_.run();

  p_client_data_receiver->run();
  std::cout<<"Control Ip: "<<control_ip_<<"control_port :"<<control_recv_port_<<std::endl;
  ControlCommandClient control_message_sender(control_ip_,control_recv_port_);
  while(true){
    std::cout << "Awake now!" << std::endl;
    HFSAT::Utils::HTMLGenerator::clear();
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    control_message_sender.sendControlCommandNoRes({"REQUESTPNL"});
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    control_message_sender.sendControlCommandNoRes({"REQUESTOPENPOS"});
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    control_message_sender.sendControlCommandNoRes({"MARGINUSAGE"});
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    control_message_sender.sendControlCommandNoRes({"REQEXECUTIONS"});
    std::this_thread::sleep_for(std::chrono::seconds(1));

    HFSAT::Utils::HTMLGenerator::writeToFile();

    std::cout << "Sleeping for 1 minute..." << std::endl;
    std::this_thread::sleep_for(std::chrono::minutes(1));
  }
  // HFSAT::ORS::ORSControllerThread::SetUniqueInstance(dbglogger_, *p_settings_);

  p_client_data_receiver->stop();
  p_global_control_receiver_->stop();

  // Channel the program's shut down through the termination handler

  std::cout << "All completed\n";
}
