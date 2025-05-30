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
#include "infracore/BasicOrderRoutingServer/shm_client_receiver.hpp"
#include "dvccode/Utils/eti_algo_tagging.hpp"
#include "dvccode/Utils/client_logging_segment_initializer.hpp"
//#include "dvccode/Utils/onload_tcp_warmup_thread.hpp"
#include "dvccode/Utils/query_t2t.hpp"
#include "dvccode/Utils/saci_generator.hpp"
#include "infracore/ORSUtils/broadcast_manager.hpp"
#include "infracore/ORSUtils/ors_pnl_manager.hpp"
#include "infracore/ORSUtils/ors_margin_manager.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

// Made loggers global to access in termination_handler
// HFSAT::OnlineDebugLogger *online_dbglogger_ = new HFSAT::OnlineDebugLogger(1024,-1);
// HFSAT::DebugLogger &dbglogger_ = *(online_dbglogger_);
HFSAT::DebugLogger dbglogger_(4 * 1024 * 1024, 1);

// Made p_settings_ global to access in termination_handler
HFSAT::ORS::Settings* p_settings_ = NULL;
HFSAT::ORS::ControlReceiver* p_global_control_receiver_ = NULL;

HFSAT::ORS::ShmClientReceiver* p_shm_client_receiver = NULL;

HFSAT::Utils::ClientLoggingSegmentInitializer* client_logging_segment_initializer_ptr = NULL;

HFSAT::BroadcastManager* bcast_manager_ = NULL;

void termination_handler_segv(int signum) {

  dbglogger_.DumpCurrentBuffer();
  dbglogger_.Close();
  // Save Fix Sequences
  if (NULL != HFSAT::ORS::AccountManager::GetAccountThread()) {
    HFSAT::ORS::AccountManager::GetAccountThread()->CommitFixSequences();
  }

  HFSAT::ORS::SequenceGenerator::GetUniqueInstance(p_settings_->getValue("Exchange")).persist_seq_num();

  HFSAT::ORS::PositionManager::GetUniqueInstance().DumpPMState();
  HFSAT::ORS::OrderManager::GetUniqueInstance().DumpOMState();

  if (p_shm_client_receiver) {
    p_shm_client_receiver->StopClientThread();
  }

  // dump saci gen, to ensure unique saci for queries on restart
  HFSAT::SaciIncrementGenerator::GetUniqueInstance().PersistSaciIncrementGenerator();
  HFSAT::SaciIncrementGenerator::ResetUniqueInstance();

  std::ostringstream t_temp_oss;
  t_temp_oss << "RECEIVED SIGSEGV, CAN'T CONTINUE, CHECK FOR LOGS, GENERATING CORE FILE...";

  // also send an alert
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);
  HFSAT::Email e;

  e.setSubject("Fatal Error: ORS received SIGSEGV in " + std::string(hostname));
  e.addRecepient("nseall@tworoads.co.in");
  e.addSender("nseall@tworoads.co.in");
  e.content_stream << "host_machine: " << hostname << "<br/>";
  e.content_stream << t_temp_oss.str() << "<br/>";

  e.sendMail();

  signal(signum, SIG_DFL);
  kill(getpid(), signum);
}

void termination_handler(int signum) {

  // We store the state of the position manager to a file, for recovery at next startup --
  HFSAT::ORS::PositionManager::GetUniqueInstance().DumpPMRecovery();

  dbglogger_.DumpCurrentBuffer();
  HFSAT::ORS::SequenceGenerator::GetUniqueInstance(p_settings_->getValue("Exchange")).persist_seq_num();

  HFSAT::ORS::ClientOrderIdGenerator::GetUniqueInstance().persist_seq_num();
  // dump saci gen, to ensure unique saci for queries on restart
  HFSAT::SaciIncrementGenerator::GetUniqueInstance().PersistSaciIncrementGenerator();
  HFSAT::SaciIncrementGenerator::ResetUniqueInstance();

  // If the settings file has "Cancel_On_Exit" set, this will cancel all pending orders on exit --
  HFSAT::ORS::AccountThread* account_thread_ = HFSAT::ORS::AccountManager::GetAccountThread();
  bool cancel_on_exit_ = true;  // by default Cancel_On_Exit is true
  if ((p_settings_ != NULL) && (p_settings_->has("Cancel_On_Exit")) &&
      (atoi(p_settings_->getValue("Cancel_On_Exit").c_str()) == 0)) {
    cancel_on_exit_ = false;
  }

  if ((account_thread_ != NULL) && cancel_on_exit_) {
    dbglogger_ << "'Cancel_On_Exit' enabled. Cancelling all pending orders. Please wait -- \n";
    dbglogger_.DumpCurrentBuffer();
    // Cancel all pending orders for all client_threads.
    account_thread_->CancelAllPendingOrders();
    account_thread_->DisConnect();
  } else if (account_thread_ != NULL) {
    dbglogger_ << "'Cancel_On_Exit' disabled. NOT Cancelling all pending orders.\n";
    dbglogger_.DumpCurrentBuffer();
    account_thread_->DisConnect();
  } else {
    dbglogger_ << "'Cancel_On_Exit' disabled. NOT Cancelling all pending orders.\n";
    dbglogger_.DumpCurrentBuffer();
    // account_thread_->DisConnect ( ) ; Please don't put this here, account_thread is NULL => core dump -- .
  }

  if (p_shm_client_receiver) {
    p_shm_client_receiver->StopClientThread();
  }

  // notify control receiver and client receiver that we are going to die
  // notify control threads and client threads that we are going to die
  // notify client threads that we are going to die
  if (p_global_control_receiver_ != NULL) {
    p_global_control_receiver_->StopControlThreads();
  }

  HFSAT::ORS::AccountManager::RemoveAccountThread();
  if (p_settings_) {
    delete p_settings_;
    p_settings_ = NULL;
  }

  if (NULL != client_logging_segment_initializer_ptr) {
    client_logging_segment_initializer_ptr->CleanUp();
  }

  if (bcast_manager_) {
    bcast_manager_->StopTread();
  }

  exit(-1);

  dbglogger_.Close();
  //  tradelogger_.Close ( );
  usleep(5000000);  // Sleep to receive logout confirmations.
  exit(0);
}

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
  signal(SIGINT, termination_handler);
  signal(SIGSEGV, termination_handler_segv);
  signal(SIGPIPE, SIG_IGN);

  boost::program_options::options_description desc("Allowed Options");
  desc.add_options()("help", "produce help message.")("config", boost::program_options::value<std::string>(),
                                                      "(required) ors configuration file.")(
      "output-log-dir", boost::program_options::value<std::string>()->default_value("/spare/local/ORSlogs"),
      "debuglogger dir");

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
  std::string t_output_log_dir_ = vm["output-log-dir"].as<std::string>();

  int control_recv_port_ = -1;

  if (p_settings_->has("Control_Port")) {
    control_recv_port_ = atoi(p_settings_->getValue("Control_Port").c_str());
  }

  int client_recv_port_ = -1;
  if (p_settings_->has("Client_Port")) client_recv_port_ = atoi(p_settings_->getValue("Client_Port").c_str());

  int server_assigned_client_id_base_ = -1;
  if (p_settings_->has("Client_Base"))
    server_assigned_client_id_base_ = atoi(p_settings_->getValue("Client_Base").c_str());

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
            << "\n";

  if ((client_recv_port_ < 10000) || (client_recv_port_ > 50000)) {
    std::cerr << " client_recv_port_ " << client_recv_port_ << " not in range 10000 50000 " << std::endl;
  }
  if ((control_recv_port_ < 10000) || (control_recv_port_ > 50000)) {
    std::cerr << " control_recv_port_ " << control_recv_port_ << " not in range 10000 50000 " << std::endl;
  }
  if ((server_assigned_client_id_base_ < 1) || (server_assigned_client_id_base_ > 32000)) {
    std::cerr << " server_assigned_client_id_base_ " << server_assigned_client_id_base_ << " not in range 1 30000"
              << std::endl;
  }

//  HFSAT::Utils::OnloadFakeTCPWarmupThread& onload_fake_tcp_warmupthread =
//      HFSAT::Utils::OnloadFakeTCPWarmupThread::GetUniqueInstance();
//
//  if (p_settings_->has("SendFakeTCPPackets") &&
//      std::string("Y") == std::string(p_settings_->getValue("SendFakeTCPPackets"))) {
//    onload_fake_tcp_warmupthread.run();
//  }

  {
    std::ostringstream t_temp_oss_;
    int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
    t_temp_oss_ << vm["output-log-dir"].as<std::string>() << "/log." << tradingdate_;
    std::string logfilename_ = t_temp_oss_.str();

    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out | std::ofstream::app);
    dbglogger_ << "Opened ORSLog in Append mode!\n";
    dbglogger_.DumpCurrentBuffer();

    dbglogger_ << p_settings_->ToString() << "\n";
    dbglogger_.DumpCurrentBuffer();
    // dbglogger_.AddLogLevel ( ORS_INFO );
    dbglogger_.AddLogLevel(ORS_ERROR);
  }

  dbglogger_ << "ORS REPLY SET UP ON : " << bcast_ip_ << " X " << bcast_port_ << "\n";
  dbglogger_.DumpCurrentBuffer();

  HFSAT::ORSUtils::ORSPnlManager& ors_pnl_manager = HFSAT::ORSUtils::ORSPnlManager::GetUniqueInstance(dbglogger_);
  if (p_settings_->has("ORSStopPnl")) {
    ors_pnl_manager.SetORSStopPnl(p_settings_->getIntValue("ORSStopPnl", 0));
  }

  HFSAT::ORSUtils::ORSMarginManager& ors_margin_manager =
      HFSAT::ORSUtils::ORSMarginManager::GetUniqueInstance(dbglogger_);
  if (p_settings_->getValue("Exchange") == std::string("NSE_CD") ||
      p_settings_->getValue("Exchange") == std::string("NSE_FO") ||
      p_settings_->getValue("Exchange") == std::string("MSSIM") ||
      p_settings_->getValue("Exchange") == std::string("NSE_EQ")) {
    if (p_settings_->has("GrossMarginCheck") && p_settings_->has("NetMarginCheck")) {
      ors_margin_manager.EnableMarginChecks();
      ors_margin_manager.SetGrossMarginCheck(atof(p_settings_->getValue("GrossMarginCheck").c_str()));
      ors_margin_manager.SetNetMarginCheck(atof(p_settings_->getValue("NetMarginCheck").c_str()));

      if (p_settings_->getValue("Exchange") == std::string("NSE_FO")) {  // Margin Factor Can Only Be Used For FO
        if (p_settings_->has("MarginFactor")) {
          ors_margin_manager.UpdateMarginFactor(atof(p_settings_->getValue("MarginFactor").c_str()));
          dbglogger_ << "Using Margin Factor : " << atof(p_settings_->getValue("MarginFactor").c_str()) << "\n";
          dbglogger_.DumpCurrentBuffer();
        }
      }
    }
  }

  int tradingdate_ = HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate();

  struct timeval current_time;
  gettimeofday(&current_time, NULL);

  // Make Sure This Is Done Above All Other Classe's Initialization, Needed For ASX TICK Changes
  HFSAT::SecurityDefinitions::ResetASXMpiIfNeeded(tradingdate_, (time_t)current_time.tv_sec);

  if (std::string("NSE_CD") == std::string(p_settings_->getValue("Exchange")) ||
      std::string("NSE_EQ") == std::string(p_settings_->getValue("Exchange")) ||
      std::string("NSE_FO") == std::string(p_settings_->getValue("Exchange")) ||
      std::string("SIM") == std::string(p_settings_->getValue("Exchange")) ||
      std::string("MSSIM") == std::string(p_settings_->getValue("Exchange"))) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
  }

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "/trades." << tradingdate_;
  std::string tradefilename_ = t_temp_oss_.str();

  char output_dir[512];
  char trades_file_name[256];

  memset((void*)output_dir, 0, 512);
  memset((void*)trades_file_name, 0, 256);

  memcpy((void*)output_dir, std::string(vm["output-log-dir"].as<std::string>()).c_str(),
         std::string(vm["output-log-dir"].as<std::string>()).length());
  memcpy((void*)trades_file_name, tradefilename_.c_str(), tradefilename_.length());

  HFSAT::Utils::ClientLoggingSegmentInitializer client_logging_segment_initializer(dbglogger_, 11011, output_dir,
                                                                                   trades_file_name);
  client_logging_segment_initializer_ptr = &client_logging_segment_initializer;

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

  // Setup ExchSource With Order Manager, So it can differentiate between RTS specific code
  HFSAT::ORS::OrderManager::GetUniqueInstance().SetExchangeSource(
      HFSAT::StringToExchSource(p_settings_->getValue("Exchange")));

  // Assuming the client_base is always present and valid value in the configuration file
  HFSAT::BroadcastManager* bcast_manager_ = HFSAT::BroadcastManager::GetUniqueInstance(
      dbglogger_, bcast_ip_, bcast_port_, (atoi(p_settings_->getValue("Client_Base").c_str())) << 16);
  bcast_manager_->StartThread();
  HFSAT::ORS::PositionManager::GetUniqueInstance().SetBaseWriterId(atoi(p_settings_->getValue("Client_Base").c_str()));

  HFSAT::ORS::MarginChecker::GetUniqueInstance(dbglogger_,
                                               HFSAT::StringToExchSource(p_settings_->getValue("Exchange")));

  HFSAT::ORS::AccountManager::GetNewAccountThread(dbglogger_, HFSAT::ORS::OrderManager::GetUniqueInstance(),
                                                  *p_settings_, client_logging_segment_initializer_ptr,
                                                  t_output_log_dir_);

  HFSAT::ORS::ControlReceiver control_receiver_(dbglogger_, *p_settings_, client_logging_segment_initializer_ptr,
                                                vm["output-log-dir"].as<std::string>());
  p_global_control_receiver_ = &control_receiver_;
  control_receiver_.run();

  p_shm_client_receiver =
      new HFSAT::ORS::ShmClientReceiver(dbglogger_, *p_settings_, p_settings_->getValue("Exchange"));
  p_shm_client_receiver->addShmWriterAddRemoveListner();

  p_shm_client_receiver->run();

  HFSAT::ORS::ORSControllerThread::SetUniqueInstance(dbglogger_, *p_settings_);

  p_shm_client_receiver->stop();

  p_global_control_receiver_->stop();

  // Channel the program's shut down through the termination handler
  termination_handler(SIGTERM);
}
