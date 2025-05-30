/**
   \file FastDataDaemon.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <algorithm>
#include <set>
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/online_debug_logger.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/CDef/currency_convertor.hpp"
#include "dvccode/CombinedControlUtils/combined_control_message_livesource.hpp"
#include "infracore/NSEMD/nse_tbt_raw_md_handler.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/Utils/mds_shm_interface.hpp"
#include "dvccode/Utils/load_our_defined_products.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/Utils/data_daemon_config.hpp"
#include "dvccode/Utils/allocate_cpu.hpp"
#include "infracore/Tools/live_products_manager.hpp"
#include "infracore/LiveSources/multi_shm_live_data_source.hpp"
#include "dvccode/Utils/tcp_direct_client_zocket_with_logging.hpp"

#include "dvccode/Utils/bse_daily_token_symbol_handler.hpp"
#include "infracore/BSEMD/bse_tbt_raw_md_handler.hpp"
#include "infracore/BSEMD/bse_raw_md_handler.hpp"
#include "infracore/BSEMD/bse_md_processor.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

HFSAT::DebugLogger* global_dbglogger_;
HFSAT::BSEMD::BSETBTRawMDHandler* global_ds;

/// signal handler
void SigHandler(int signum) {
  std::cout << "RECEIVED SIGNAL... " << signum << std::endl;
  HFSAT::Utils::CombinedSourceGenericLogger::GetUniqueInstance().FlushAndCloseFiles();
  std::cout << "TOTAL MKT DROPS... "  << global_ds->total_mkt_drops_occurance() << " COUNT: " << global_ds->total_mkt_drops() <<std::endl;
  std::cout << "CLOSED MDS FILES..." << std::endl;

  if (NULL != global_dbglogger_) {
    global_dbglogger_->Close();
    global_dbglogger_ = NULL;
  }

//  EOBI_MD_Processor::EOBIMDProcessor::GetInstance().cleanup();

  if (SIGSEGV == signum) {
    std::ostringstream t_temp_oss;
    t_temp_oss << "RECEIVED SIGSEGV, CAN'T CONTINUE,";
    t_temp_oss << "CHECK FOR LOGS, GENERATING CORE FILE...";

    // also send an alert
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);
    HFSAT::Email e;

    std::string subject_ = "";
    {
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << "FATAL - BSENSEShmWriterUDPDirect received SIGSEGV on " << hostname;
      subject_ = t_temp_oss_.str();
    }
    std::cout << subject_ << std::endl;

//    e.setSubject(subject_);
//    e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
//    e.addSender("ravi.parikh@tworoads.co.in");
//    e.content_stream << "host_machine: " << hostname << "<br/>";
//    e.content_stream << t_temp_oss.str() << "<br/>";
//
//    e.sendMail();

    signal(signum, SIG_DFL);
    kill(getpid(), signum);

  }

  exit(0);
}

int main(int argc, char** argv) {
  /// set signal handler .. add other signals later
  struct sigaction sigact;
  memset(&sigact, '\0', sizeof(sigact));
  sigact.sa_handler = SigHandler;
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGSEGV, &sigact, NULL);

  HFSAT::FastMdConsumerMode_t mode = HFSAT::kProShm;
 
  
  if (argc > 1 && std::string(argv[1]) == "LOGGER") mode = HFSAT::kConvertLogger;
  if (argc > 1 && std::string(argv[1]) == "OEBU") mode = HFSAT::kProShm;
  
  //For Shm
  (HFSAT::Utils::NSEMDSShmInterface::GetUniqueInstance()).Initialize();
  (HFSAT::Utils::BSEMDSShmInterface::GetUniqueInstance()).Initialize();


  /// input arguments
  std::string exch_ = "BSE";
  std::string log_dir_ = "";

  if (mode & HFSAT::kProShm) (HFSAT::Utils::MDSShmInterface::GetUniqueInstance()).Initialize();
  HFSAT::DebugLogger dbglogger_(1024000, 1);
  global_dbglogger_ = &dbglogger_;
  
//  std::string logfilename = "/spare/local/MDSlogs/bseshm_writer_dbg_snapshot_zocket" + std::string("_") +
  std::string logfilename = "/spare/local/MDSlogs/bseshm_writer_dbg" + std::string("_") + 
                            HFSAT::DateTime::GetCurrentIsoDateLocalAsString() + ".log";

  dbglogger_.OpenLogFile(logfilename.c_str(), std::ios::out | std::ios::app);
  global_dbglogger_ = &dbglogger_;

  dbglogger_ << "OpenDbglogger\n";
  dbglogger_.DumpCurrentBuffer();
  
  int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_);

  HFSAT::SimpleLiveDispatcher simple_live_dispatcher;
  HFSAT::Utils::UDPDirectMuxer & udp_direct_muxer_ptr = (HFSAT::Utils::UDPDirectMuxer::GetUniqueInstance());
//  HFSAT::Utils::UDPDirectMultipleZocket& udp_direct_multiple_zockets = HFSAT::Utils::UDPDirectMultipleZocket::GetUniqueInstance();
  HFSAT::CombinedControlMessageLiveSource* p_cmd_control_live_source_ = nullptr;

  HFSAT::LiveProductsManager& live_product_manager = HFSAT::LiveProductsManager::GetUniqueInstance();

  HFSAT::SecurityDefinitions::GetUniqueInstance().LoadBSESecurityDefinitions();
  HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(tradingdate_);
  HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(tradingdate_);


  HFSAT::SecurityNameIndexer::GetUniqueInstance();

  HFSAT::NSEMD::NSETBTRawMDHandler::GetUniqueInstance(dbglogger_, simple_live_dispatcher, p_cmd_control_live_source_, mode, "BSE_NSE", true); 
  HFSAT::BSEMD::BSETBTRawMDHandler& ds = HFSAT::BSEMD::BSETBTRawMDHandler::GetUniqueInstance(dbglogger_, simple_live_dispatcher, p_cmd_control_live_source_, mode, "BSE_NSE", true);
  global_ds = &ds;

  HFSAT::NetworkAccountInfoManager network_account_info_manager_;
  auto control_ip_port = network_account_info_manager_.GetCombControlDataInfo();
  auto& interface_manager = HFSAT::NetworkAccountInterfaceManager::instance();
  auto network_interface = interface_manager.GetInterfaceForApp(HFSAT::k_Control);

  std::cout << " IP: " << control_ip_port.bcast_ip_ << " Port: " << control_ip_port.bcast_port_
            << " Iface: " << network_interface << "\n";

  p_cmd_control_live_source_ = new HFSAT::CombinedControlMessageLiveSource(
      dbglogger_, control_ip_port.bcast_ip_, control_ip_port.bcast_port_, network_interface, MULTICAST, HFSAT::kProShm, false);

  if (p_cmd_control_live_source_ != nullptr) {
    p_cmd_control_live_source_->AddCombinedControlMessageListener(&live_product_manager);
  } else {
    std::cerr << "CombinedControlMessageLiveSource NULL. ReqbasedCombinedWriter would not work" << std::endl;
  }

  HFSAT::DataInfo control_recv_data_info_ = network_account_info_manager_.GetControlRecvDataInfo();
//  HFSAT::Utils::CombinedSourceGenericLogger::GetUniqueInstance().SubscribeToCombinedControlMessage(p_cmd_control_live_source_);

  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);

  std::cout << " GOING FOR CORE : " << std::endl;

  // We do not want to affine at NY servers
  if (std::string(hostname).find("sdv-ny4-srv") == std::string::npos) {
    HFSAT::AllocateCPUUtils::GetUniqueInstance().AllocateCPUOrExit("BSEShmWriter");
  }

  std::cout << " CORE ALLOCATED : " << std::endl;
  //-1 so we don't timeout from that loop as we don't have to process anything else here
  udp_direct_muxer_ptr.RunMuxerLiveDispatcherWithTimeOut(-1);
//  udp_direct_multiple_zockets.RunLiveDispatcher();

  return EXIT_SUCCESS;
}
