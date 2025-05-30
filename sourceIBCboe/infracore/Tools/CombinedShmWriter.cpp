// =====================================================================================
//
//       Filename:  CombinedShmWriter.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/08/2014 06:35:15 AM
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

#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <set>
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/chix_mds_defines.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/online_debug_logger.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/CDef/currency_convertor.hpp"
#include "dvccode/CDef/sgx_mds_defines.hpp"

#include "infracore/lwfixfast/livesources/new_ntp_raw_live_data_source.hpp"
#include "infracore/lwfixfast/livesources/new_puma_raw_live_data_source.hpp"

#include "infracore/LiveSources/generic_live_data_source.hpp"
#include "infracore/LiveSources/common_live_source.hpp"
#include "dvccode/CombinedControlUtils/combined_control_message_livesource.hpp"

#include "infracore/lwfixfast/ntp_md_processor.hpp"
#include "infracore/lwfixfast/bmf_md_processor.hpp"
#include "infracore/lwfixfast/puma_md_processor.hpp"

#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/Utils/mds_shm_interface.hpp"
#include "dvccode/Utils/load_our_defined_products.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/Utils/combined_source_generic_logger.hpp"
#include "dvccode/Utils/data_daemon_config.hpp"
#include "dvccode/Utils/allocate_cpu.hpp"

#include "infracore/Tools/live_products_manager.hpp"

#include "infracore/NSEMD/nse_tbt_raw_md_handler.hpp"
#include "infracore/BSEMD/bse_tbt_raw_md_handler.hpp"

HFSAT::DebugLogger* global_dbglogger_;
std::vector<HFSAT::SimpleExternalDataLiveListener*> list_of_raw_exchange_source_we_want_to_cleanup_;
HFSAT::SimpleLiveDispatcher* global_simple_live_dispatcher = NULL;

void DumpCombinedWriterProfileStats() {
  std::string summary = HFSAT::CpucycleProfiler::GetUniqueInstance().GetCpucycleSummaryString();

  std::cout << "CPU Cycle Profiler Stats: \n" << summary << std::endl;

  std::ofstream out;
  std::string file_name =
      "/spare/local/logs/profilelogs/combined_writer_profile." + HFSAT::DateTime::GetCurrentIsoDateLocalAsString();
  out.open(file_name, std::ios_base::app);

  out << "CPU Cycle Profiler Stats: \n" << summary;
  out.close();
}

void print_usage(const char* prg_name) {
  printf(" This is the Combined SHM Writer Executable \n");
  printf(" Usage:%s --config <config_file>\n", prg_name);
}

static struct option data_options[] = {{"help", no_argument, 0, 'h'},
                                       {"config", required_argument, 0, 'a'},
                                       {"operating_mode", required_argument, 0, 'b'},
                                       {"exchange", required_argument, 0, 'c'},
                                       {0, 0, 0, 0}};

/// signal handler
void sighandler(int signum) {
  int32_t consumers_still_attached = HFSAT::Utils::MDSShmInterface::GetUniqueInstance().ConsumersAttached();
  std::cout << "Number Of Consumers : " << consumers_still_attached << std::endl;

  if (consumers_still_attached > 1) {
    static bool alert_send = false;
    if (false == alert_send) {
      HFSAT::SlackManager slack_manager(TESTCHANNEL);
      char hostname[128];
      hostname[127] = '\0';
      gethostname(hostname, 127);
      std::ostringstream t_temp_oss;
      t_temp_oss << "ALERT: CombinedShmWriter Received Stop Signal Of : " << signum << " While There Are -> "
                 << (consumers_still_attached - 1) << " Consumers Still Attached On : " << hostname;
      slack_manager.sendNotification(t_temp_oss.str());

      //      HFSAT::Email e;
      //      e.setSubject(t_temp_oss.str());
      //      e.addRecepient("nseall@tworoads.co.in");
      //      e.addSender("nseall@tworoads.co.in");
      //      e.sendMail();

      alert_send = true;
    }
  }

  DumpCombinedWriterProfileStats();

  if (NULL != global_dbglogger_) {
    global_dbglogger_->Close();
    global_dbglogger_ = NULL;
  }

  for (uint32_t raw_source_counter = 0; raw_source_counter < list_of_raw_exchange_source_we_want_to_cleanup_.size();
       raw_source_counter++) {
    if (NULL != list_of_raw_exchange_source_we_want_to_cleanup_[raw_source_counter]) {
      list_of_raw_exchange_source_we_want_to_cleanup_[raw_source_counter]->CleanUp();
      delete list_of_raw_exchange_source_we_want_to_cleanup_[raw_source_counter];
      list_of_raw_exchange_source_we_want_to_cleanup_[raw_source_counter] = NULL;
    }
  }

  if (NULL != global_simple_live_dispatcher) {
    global_simple_live_dispatcher->CleanUp();
    sleep(5);
    delete global_simple_live_dispatcher;
    global_simple_live_dispatcher = NULL;
  }

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
      t_temp_oss_ << "FATAL - CombinedShmWriter received SIGSEGV on " << hostname;
      subject_ = t_temp_oss_.str();
    }

    e.setSubject(subject_);
    e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
    e.addSender("ravi.parikh@tworoads.co.in");
    e.content_stream << "host_machine: " << hostname << "<br/>";
    e.content_stream << t_temp_oss.str() << "<br/>";

    e.sendMail();

    signal(signum, SIG_DFL);
    kill(getpid(), signum);

    exit(0);
  }

  fprintf(stderr, "Received signal %d \n", signum);

  NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance().cleanup();
  PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance().cleanup();

  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::CurrencyConvertor::RemoveInstance();

  HFSAT::Utils::CombinedSourceGenericLogger::ResetUniqueInstance();

  exit(0);
}

int main(int argc, char** argv) {
  /// set signal handler .. add other signals later
  struct sigaction sigact;
  memset(&sigact, '\0', sizeof(sigact));
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGSEGV, &sigact, NULL);

  int c;
  int hflag = 0;
  /// input arguments
  std::string config_file_;
  std::set<std::string> exch_processing_;
  std::string operating_mode_;

  HFSAT::CpucycleProfiler::SetUniqueInstance(10);
  HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(1, "Packet Reading Time");
  HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(2, "Decoding Time");
  HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(3, "Converter Time");
  HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(4, "Total Time");

  (HFSAT::Utils::MDSShmInterface::GetUniqueInstance()).Initialize();

  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "", data_options, &option_index);
    if (c == -1) break;

    switch (c) {
      case 'h':
        hflag = 1;
        break;
      case 'a':
        config_file_ = optarg;
        break;
      case 'b': {
        operating_mode_ = optarg;
        break;
      }
      case 'c': {
        exch_processing_.insert(optarg);
        break;
      }
      case '?':
        if (optopt == 'a') {
          fprintf(stderr, "Option %c requires an argument .. will exit \n", optopt);
          exit(-1);
        }
        break;

      default:
        fprintf(stderr, "Weird option specified .. no handling yet \n");
        break;
    }
  }

  if (hflag || (config_file_ == "" && exch_processing_.empty())) {
    print_usage(argv[0]);
    exit(-1);
  }

  if (config_file_ != "") {
    HFSAT::DataDaemonConfig& config_ = HFSAT::DataDaemonConfig::GetUniqueInstance(config_file_);
    exch_processing_ = config_.GetExchSet();
    operating_mode_ = config_.GetOperatingMode();
  }

  std::vector<std::string> exch_processing_list_(exch_processing_.begin(), exch_processing_.end());
  // CONTROL_COMBINED should be processed first. The following code is written with this assumption.
  std::sort(exch_processing_list_.begin(), exch_processing_list_.end(), [](std::string a, std::string b) -> bool {
    if (a == "CONTROL_COMBINED") {
      return true;
    }
    if (b == "CONTROL_COMBINED") {
      return false;
    }
    return (a < b);  // Sort in Increasing Order
  });

  HFSAT::FastMdConsumerMode_t mode = HFSAT::kComShm;  // Since the Combined SHM Writer Mode is Fixed
  HFSAT::SimpleLiveDispatcher* simple_live_dispatcher = new HFSAT::SimpleLiveDispatcher();
  global_simple_live_dispatcher = simple_live_dispatcher;

  HFSAT::DebugLogger dbglogger_(10240, 1);
  std::string logfilename = "/spare/local/MDSlogs/combined_writer_dbg" + std::string("_") +
                            HFSAT::DateTime::GetCurrentIsoDateLocalAsString() + ".log";

  dbglogger_.OpenLogFile(logfilename.c_str(), std::ios::out | std::ios::app);
  global_dbglogger_ = &dbglogger_;

  dbglogger_ << "OpenDbglogger\n";
  dbglogger_.DumpCurrentBuffer();

  HFSAT::NetworkAccountInfoManager network_account_info_manager_;
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_);
  HFSAT::LiveProductsManager& live_product_manager = HFSAT::LiveProductsManager::GetUniqueInstance();

  if ("HYBRID" == operating_mode_) {
    HFSAT::Utils::CombinedSourceGenericLogger::GetUniqueInstance().EnableLogging();
  }

  std::set<HFSAT::ExchSource_t> exch_source_processing_;

  // made global in order send its pointer further, used to add listeners to this
  HFSAT::CombinedControlMessageLiveSource* p_cmd_control_live_source_ = nullptr;

  for (auto exch_ : exch_processing_list_) {
    if (exch_ == "NTP") {
      // Loading defined products - useful for filtering MCast channels in start
      exch_source_processing_.insert(HFSAT::kExchSourceBMF);
      HFSAT::Utils::LoadOurDefinedProducts::GetUniqueInstance(exch_source_processing_)
          .AddExchange(HFSAT::kExchSourceBMF);

      HFSAT::NewPumaRawLiveDataSource* ds =
          new HFSAT::NewPumaRawLiveDataSource(dbglogger_, sec_name_indexer_, simple_live_dispatcher, mode, false, 2);
      ds->start();

      list_of_raw_exchange_source_we_want_to_cleanup_.push_back(ds);

    } else if (exch_ == "BMF_PUMA") {
      // Loading defined products - useful for filtering MCast channels in start
      exch_source_processing_.insert(HFSAT::kExchSourceBMF);
      HFSAT::Utils::LoadOurDefinedProducts::GetUniqueInstance(exch_source_processing_)
          .AddExchange(HFSAT::kExchSourceBMF);

      HFSAT::NewNTPRawLiveDataSource* ds =
          new HFSAT::NewNTPRawLiveDataSource(dbglogger_, sec_name_indexer_, simple_live_dispatcher, mode, 5);
      ds->start();

      list_of_raw_exchange_source_we_want_to_cleanup_.push_back(ds);

    } else if (exch_ == "BMF_EQ") {
      // Loading defined products - useful for filtering MCast channels in start
      exch_source_processing_.insert(HFSAT::kExchSourceBMFEQ);
      HFSAT::Utils::LoadOurDefinedProducts::GetUniqueInstance(exch_source_processing_)
          .AddExchange(HFSAT::kExchSourceBMFEQ);

      HFSAT::NewPumaRawLiveDataSource* ds =
          new HFSAT::NewPumaRawLiveDataSource(dbglogger_, sec_name_indexer_, simple_live_dispatcher, mode, false);
      ds->start();

      list_of_raw_exchange_source_we_want_to_cleanup_.push_back(ds);

    } else if (exch_ == "BMF") {
      // Loading defined products - useful for filtering MCast channels in start
      exch_source_processing_.insert(HFSAT::kExchSourceBMF);
      HFSAT::Utils::LoadOurDefinedProducts::GetUniqueInstance(exch_source_processing_)
          .AddExchange(HFSAT::kExchSourceBMF);

      HFSAT::NewNTPRawLiveDataSource* ds =
          new HFSAT::NewNTPRawLiveDataSource(dbglogger_, sec_name_indexer_, simple_live_dispatcher, mode, 4);
      ds->start();

      list_of_raw_exchange_source_we_want_to_cleanup_.push_back(ds);

    } else if (exch_ == "SIM_LS") {
      std::string ip = "239.23.0.181";
      int port = 45461;
      std::string iface = "eth0.2482";
      HFSAT::MDSMessages::CommonLiveSource* common_live_source =
          new HFSAT::MDSMessages::CommonLiveSource(dbglogger_, sec_name_indexer_);
      common_live_source->AddLiveSocket(ip, port, iface);

      simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(common_live_source,
                                                                      common_live_source->GetLiveSocketsFd());

    } else if (exch_ == "NSE_L1") {
      HFSAT::DataInfo data_info_ =
          network_account_info_manager_.GetSrcDataInfo(HFSAT::kExchSourceNSE, "NSE_NIFTY_FUT0");
      HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericL1DataStruct>* generic_live_data_source =
          new HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericL1DataStruct>(dbglogger_, sec_name_indexer_,
                                                                                    HFSAT::MDS_MSG::NSE_L1);
      generic_live_data_source->AddLiveSocket(
          data_info_.bcast_ip_, data_info_.bcast_port_,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceNSE, HFSAT::k_MktDataLive));
      std::vector<int32_t> socket_fd_vec;
      generic_live_data_source->GetLiveSocketsFdList(socket_fd_vec);
      for (auto socket_fd_ : socket_fd_vec) {
        simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(generic_live_data_source, socket_fd_);
      }

      exch_source_processing_.insert(HFSAT::kExchSourceNSE);

    } else if (exch_ == "CONTROL") {
      HFSAT::DataInfo control_recv_data_info_ = network_account_info_manager_.GetControlRecvDataInfo();
      HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericControlRequestStruct>* generic_live_data_source =
          new HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericControlRequestStruct>(
              dbglogger_, sec_name_indexer_, HFSAT::MDS_MSG::CONTROL, true);
      generic_live_data_source->AddLiveSocket(
          control_recv_data_info_.bcast_ip_, control_recv_data_info_.bcast_port_,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_Control));
      std::cout << "Listening to Control Msg " << control_recv_data_info_.bcast_ip_ << " " << control_recv_data_info_.bcast_port_ << std::endl;
      std::vector<int32_t> socket_fd_vec;
      generic_live_data_source->GetLiveSocketsFdList(socket_fd_vec);
      for (auto socket_fd_ : socket_fd_vec) {
        simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(generic_live_data_source, socket_fd_);
      }
      list_of_raw_exchange_source_we_want_to_cleanup_.push_back(generic_live_data_source);

    } else if (exch_ == "CONTROL_COMBINED") {
      auto control_ip_port = network_account_info_manager_.GetCombControlDataInfo();

      auto& interface_manager = HFSAT::NetworkAccountInterfaceManager::instance();
      auto network_interface = interface_manager.GetInterfaceForApp(HFSAT::k_Control);

      std::cout << "Combined Control IP: " << control_ip_port.bcast_ip_ << " Port: " << control_ip_port.bcast_port_
                << " Iface: " << network_interface << "\n";

      p_cmd_control_live_source_ = new HFSAT::CombinedControlMessageLiveSource(
          dbglogger_, control_ip_port.bcast_ip_, control_ip_port.bcast_port_, network_interface, MULTICAST, mode);

      // Explicitly set Combined Control as Primary Source
      simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(
          p_cmd_control_live_source_, p_cmd_control_live_source_->socket_file_descriptor(), true);

    } else if (exch_ == "ORS_REPLY") {
      HFSAT::TradingLocation_t curr_location_ = HFSAT::TradingLocationUtils::GetTradingLocationFromHostname();
      HFSAT::ExchSource_t current_exch_source_ = HFSAT::kExchSourceMAX;
      std::string dummy_shortcode_ = "";

      switch (curr_location_) {
        case HFSAT::kTLocCHI: {
          current_exch_source_ = HFSAT::kExchSourceCME;
          dummy_shortcode_ = "ZN_0";
        } break;
        case HFSAT::kTLocTMX: {
          current_exch_source_ = HFSAT::kExchSourceTMX;
          dummy_shortcode_ = "CGB_0";
        } break;
        case HFSAT::kTLocFR2: {
          current_exch_source_ = HFSAT::kExchSourceEUREX;
          dummy_shortcode_ = "FESX_0";
        } break;
        case HFSAT::kTLocBMF: {
          current_exch_source_ = HFSAT::kExchSourceBMF;
          dummy_shortcode_ = "BR_DOL_0";
        } break;
        case HFSAT::kTLocBSL: {
          current_exch_source_ = HFSAT::kExchSourceLIFFE;
          dummy_shortcode_ = "LFR_0";
        } break;
        case HFSAT::kTLocJPY: {
          current_exch_source_ = HFSAT::kExchSourceJPY;
          dummy_shortcode_ = "NK_0";
        } break;
        case HFSAT::kTLocHK: {
          current_exch_source_ = HFSAT::kExchSourceHONGKONG;
          dummy_shortcode_ = "HSI_0";
        } break;
        case HFSAT::kTLocCFE: {
          current_exch_source_ = HFSAT::kExchSourceCFE;
          dummy_shortcode_ = "VX_0";
        } break;
        case HFSAT::kTLocNY4: {
          current_exch_source_ = HFSAT::kExchSourceCME;
          dummy_shortcode_ = "ZN_0";
        } break;
        case HFSAT::kTLocSYD: {
          current_exch_source_ = HFSAT::kExchSourceASX;
          dummy_shortcode_ = "XT_0";
        } break;
        case HFSAT::kTLocSPR: {
          current_exch_source_ = HFSAT::kExchSourceSGX;
          dummy_shortcode_ = "SGX_NK_0";
        } break;
        case HFSAT::kTLocNSE: {
          current_exch_source_ = HFSAT::kExchSourceNSE;
          dummy_shortcode_ = "NSE_NIFTY_FUT0";
        } break;
        case HFSAT::kTLocBSE: {
          current_exch_source_ = HFSAT::kExchSourceBSE;
          dummy_shortcode_ = "BSE_SBIN";
        } break;
        default: {
          current_exch_source_ = HFSAT::kExchSourceMAX;
          dummy_shortcode_ = "";
        } break;
      }

      HFSAT::DataInfo data_info_ = network_account_info_manager_.GetDepDataInfo(current_exch_source_, dummy_shortcode_);
      HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericORSReplyStructLive>* generic_live_data_source =
          new HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericORSReplyStructLive>(
              dbglogger_, sec_name_indexer_, HFSAT::MDS_MSG::ORS_REPLY, true);
      generic_live_data_source->AddLiveSocket(
          data_info_.bcast_ip_, data_info_.bcast_port_,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_ORS_LS));
      std:: cout << "ORSB REPLY IP:  " << data_info_.bcast_ip_ << " " << data_info_.bcast_port_ << std::endl;
      std::vector<int32_t> socket_fd_vec;
      generic_live_data_source->GetLiveSocketsFdList(socket_fd_vec);
      for (auto socket_fd_ : socket_fd_vec) {
        // ALways add ORS sockets as primary sockets except at NY4
        simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(generic_live_data_source, socket_fd_, true);
      }

    } else if (exch_ == "ORS_REPLY_MULTISHM_Q19") {
      std::string ors_reply_beta_ip = "239.23.0.111";
      int32_t ors_reply_beta_port = 44444;

      HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericORSReplyStructLiveProShm>* generic_live_data_source =
          new HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericORSReplyStructLiveProShm>(
              dbglogger_, sec_name_indexer_, HFSAT::MDS_MSG::ORS_REPLY_MULTISHM, true);
      generic_live_data_source->AddLiveSocket(
          ors_reply_beta_ip.c_str(), ors_reply_beta_port,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_ORS_LS));

      std::vector<int32_t> socket_fd_vec;
      generic_live_data_source->GetLiveSocketsFdList(socket_fd_vec);
      for (auto socket_fd_ : socket_fd_vec) {
        // ALways add ORS sockets as primary sockets except at NY4
        simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(generic_live_data_source, socket_fd_, true);
      }
      std:: cout << "ORSB REPLY MULTIQ19 IP: 239.23.0.111 44444" << std::endl;

    } else if (exch_ == "ORS_REPLY_MULTISHM_S7") {
      std::string ors_reply_beta_ip = "239.23.0.177";
      int32_t ors_reply_beta_port = 39874;

      HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericORSReplyStructLiveProShm>* generic_live_data_source =
          new HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericORSReplyStructLiveProShm>(
              dbglogger_, sec_name_indexer_, HFSAT::MDS_MSG::ORS_REPLY_MULTISHM, true);
      generic_live_data_source->AddLiveSocket(
          ors_reply_beta_ip.c_str(), ors_reply_beta_port,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_ORS_LS));

      std::vector<int32_t> socket_fd_vec;
      generic_live_data_source->GetLiveSocketsFdList(socket_fd_vec);
      for (auto socket_fd_ : socket_fd_vec) {
        // ALways add ORS sockets as primary sockets except at NY4
        simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(generic_live_data_source, socket_fd_, true);
      }
    } else if (exch_ == "ORS_REPLY_MULTISHM_GRT") {
      std::string ors_reply_beta_ip = "239.23.0.176";
      int32_t ors_reply_beta_port = 39864;

      HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericORSReplyStructLiveProShm>* generic_live_data_source =
          new HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericORSReplyStructLiveProShm>(
              dbglogger_, sec_name_indexer_, HFSAT::MDS_MSG::ORS_REPLY_MULTISHM, true);
      generic_live_data_source->AddLiveSocket(
          ors_reply_beta_ip.c_str(), ors_reply_beta_port,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_ORS_LS));

      std::vector<int32_t> socket_fd_vec;
      generic_live_data_source->GetLiveSocketsFdList(socket_fd_vec);
      for (auto socket_fd_ : socket_fd_vec) {
        // ALways add ORS sockets as primary sockets except at NY4
        simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(generic_live_data_source, socket_fd_, true);
      }
      std:: cout << "ORSB REPLY GRT IP: 239.23.0.176 39864" << std::endl;
    } else if (exch_ == "ORS_REPLY_MULTISHM_IBKR") {
      std::string ors_reply_beta_ip = "239.23.0.171";
      int32_t ors_reply_beta_port = 49171;

      HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericORSReplyStructLiveProShm>* generic_live_data_source =
          new HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericORSReplyStructLiveProShm>(
              dbglogger_, sec_name_indexer_, HFSAT::MDS_MSG::ORS_REPLY_MULTISHM, true);
      generic_live_data_source->AddLiveSocket(
          ors_reply_beta_ip.c_str(), ors_reply_beta_port,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_ORS_LS));

      std::vector<int32_t> socket_fd_vec;
      generic_live_data_source->GetLiveSocketsFdList(socket_fd_vec);
      for (auto socket_fd_ : socket_fd_vec) {
        // ALways add ORS sockets as primary sockets except at NY4
        simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(generic_live_data_source, socket_fd_, true);
      }
      std:: cout << "ORSB REPLY GRT IP: 239.23.0.171 49171" << std::endl;

    } else if ("NTP_LS" == exch_) {
      HFSAT::DataInfo data_info_ = network_account_info_manager_.GetSrcDataInfo(HFSAT::kExchSourceBMF, "BR_DOL_0");
      HFSAT::MDSMessages::GenericLiveDataSource<NTP_MDS::NTPCommonStruct>* generic_live_data_source =
          new HFSAT::MDSMessages::GenericLiveDataSource<NTP_MDS::NTPCommonStruct>(dbglogger_, sec_name_indexer_,
                                                                                  HFSAT::MDS_MSG::NTP);
      generic_live_data_source->AddLiveSocket(
          data_info_.bcast_ip_, data_info_.bcast_port_,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBMF, HFSAT::k_MktDataLive));

      std::vector<int32_t> socket_fd_vec;
      generic_live_data_source->GetLiveSocketsFdList(socket_fd_vec);
      for (auto socket_fd_ : socket_fd_vec) {
        simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(generic_live_data_source, socket_fd_);
      }

      exch_source_processing_.insert(HFSAT::kExchSourceNTP);

    } else if ("NSE" == exch_) {
      HFSAT::SecurityDefinitions::GetUniqueInstance().LoadNSESecurityDefinitions();
      HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(tradingdate_);
      HFSAT::NSEMD::NSETBTRawMDHandler::GetUniqueInstance(dbglogger_, *simple_live_dispatcher,
                                                          p_cmd_control_live_source_, HFSAT::kComShm);

    } else if ("BSE" == exch_) {
      HFSAT::SecurityDefinitions::GetUniqueInstance().LoadBSESecurityDefinitions();

    } else if( "ORS_REPLY_COMBINED" == exch_ ) {
      std::string ors_reply_beta_ip = "239.23.0.186";
      int32_t ors_reply_beta_port = 49864;

      HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericORSReplyStructLiveProShm>* generic_live_data_source =
          new HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericORSReplyStructLiveProShm>(
              dbglogger_, sec_name_indexer_, HFSAT::MDS_MSG::ORS_REPLY_MULTISHM, true);
      generic_live_data_source->AddLiveSocket(
          ors_reply_beta_ip.c_str(), ors_reply_beta_port, "eth0.227");

      std::vector<int32_t> socket_fd_vec;
      generic_live_data_source->GetLiveSocketsFdList(socket_fd_vec);
      for (auto socket_fd_ : socket_fd_vec) {
        // ALways add ORS sockets as primary sockets except at NY4
        simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(generic_live_data_source, socket_fd_, true);
      }
    	
    }
    else {
      std::cerr << " Not Implemented For This Exchange : " << exch_ << " Exiting \n";
      exit(-1);
    }
  }

  if (p_cmd_control_live_source_ != nullptr) {
    p_cmd_control_live_source_->AddCombinedControlMessageListener(&live_product_manager);
  } else {
    std::cerr << "CombinedControlMessageLiveSource NULL. ReqbasedCombinedWriter would not work" << std::endl;
  }

  // This is required for filtering, and while filtering this call is must
  HFSAT::Utils::LoadOurDefinedProducts::GetUniqueInstance(exch_source_processing_)
      .AddExchanges(exch_source_processing_);
  HFSAT::Utils::CombinedSourceGenericLogger::GetUniqueInstance().AddExchSources(&exch_source_processing_);

  //if ("HYBRID" == operating_mode_) {
    HFSAT::Utils::CombinedSourceGenericLogger::GetUniqueInstance().SubscribeToCombinedControlMessage(
        p_cmd_control_live_source_);
    // params to this logger thread:
    // 1) exch_source_processing_ -> needed for per exchange tolerace in mds logger
    // 2) p_cmd_control_live_source_ -> needed to add mds_logger as a listener to cmd_control_live_source

    /*combined_source_embedded_logger_thread =
        new HFSAT::Utils::CombinedSourceEmbeddedLoggerThread(&exch_source_processing_, p_cmd_control_live_source_);
    GLOBAL_PTR_FOR_SIG_HANDLE = combined_source_embedded_logger_thread;
    combined_source_embedded_logger_thread->run();*/
  //}

  exch_source_processing_.clear();

  network_account_info_manager_.CleanUp();

  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);

  // We do not want to affine at NY servers
  if (std::string(hostname).find("sdv-ny4-srv") == std::string::npos) {
    HFSAT::AllocateCPUUtils::GetUniqueInstance().AllocateCPUOrExit("CombinedShmWriter");
  }

  simple_live_dispatcher->RunLive();

  return 0;
}
