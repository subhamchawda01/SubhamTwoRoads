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
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"

#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "infracore/lwfixfast/livesources/new_ntp_raw_live_data_source.hpp"
#include "infracore/lwfixfast/livesources/new_puma_raw_live_data_source.hpp"

#include "infracore/lwfixfast/ntp_md_processor.hpp"
#include "infracore/lwfixfast/ntp_ord_md_processor.hpp"
#include "infracore/lwfixfast/bmf_md_processor.hpp"
#include "infracore/lwfixfast/puma_md_processor.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"



// Prints error message, usage and exits (if exit_run is true)
void PrintUsage(const char* prg_name, std::string message, bool exit_run = false) {
  // provided error msg
  printf("%s\n", message.c_str());

  // usage
  printf(" This is the FixFast Data Daemon Executable \n");
  printf(
      " Usage:%s --exchange <exch> --mode <mode> [--bcast_ip <address> --bcast_port <port>] [ --log_dir <dir_name> ]\n",
      prg_name);
  printf(" --exchange Exchange Name (accepted values CME/EUREX/NTP/BMF/CMEMDP/EOBI/CSM) \n");
  printf(" --mode     Mode of operation (accepted values DATA/REFERENCE/LOGGER/SHMWRITER/INTEGRATED) \n");
  printf(" --bcast_ip Multicast Address for Market Data. Compulsory for DATA mode.\n");
  printf(" --bcast_port Broadcast Port for Market Data. Compulsory for DATA mode.\n");
  printf(" --log_dir Dir where mds data is logged. Optionally present only for LOGGER mode.\n");
  printf(
      " --exchange-mode Exchange-Mode pair. For example: CME-DATA, CFE-LOGGER ... (to be used only with INTEGRATED "
      "mode)\n");

  // exit (if true)
  if (exit_run) {
    exit(1);
  }
}

static struct option data_options[] = {{"help", no_argument, 0, 'h'},
                                       {"exchange", required_argument, 0, 'a'},
                                       {"mode", required_argument, 0, 'b'},
                                       {"bcast_ip", required_argument, 0, 'd'},
                                       {"bcast_port", required_argument, 0, 'e'},
                                       {"log_dir", required_argument, 0, 'f'},
                                       {"exchange-mode", required_argument, 0, 'g'},
                                       {0, 0, 0, 0}};

/// signal handler
void SigHandler(int signum) {
  fprintf(stderr, "Received signal %d \n", signum);

  NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance().cleanup();
  BMF_MD_PROCESSOR::BmfMDProcessor::GetInstance().cleanup();
  exit(0);
}

void ProcessInput(int argc, char** argv, int& c, int& hflag, std::string& exch_, std::string& mode_,
                  std::vector<std::string>& exchanges_, std::map<std::string, std::vector<std::string> >& modes_,
                  std::string& log_dir_, std::string& bcast_ip, int& bcast_port) {
  std::string temp_str = "";
  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "", data_options, &option_index);
    if (c == -1) break;
    switch (c) {
      case 'h':
        hflag = 1;
        break;

      case 'a':
        exch_ = optarg;
        break;

      case 'b':
        mode_ = optarg;
        break;

      case 'd':
        bcast_ip = optarg;
        break;

      case 'e':
        bcast_port = atoi(optarg);
        break;

      case 'f':
        log_dir_ = optarg;
        break;

      case 'g': {
        temp_str = optarg;  // temp_str is of the form exchange-data
        size_t delimiter = temp_str.find('-');
        if (delimiter == std::string::npos || delimiter <= 0 || delimiter >= (temp_str.length() - 1)) {
          // The user must have provided an invalid value for --exchange-mode argument
          fprintf(stderr, "Invalid value in exchange-mode argument: Must be of the form <exchange>-<mode>\n");
          exit(-1);
        }
        std::string this_exchange = temp_str.substr(0, delimiter);
        HFSAT::VectorUtils::UniqueVectorAdd(exchanges_, this_exchange);
        if (modes_.find(this_exchange) == modes_.end()) {
          std::vector<std::string> temp_vec;
          modes_[this_exchange] = temp_vec;
        }
        std::cout << "adding mode " << this_exchange << ", " << temp_str.substr(delimiter + 1) << "\n";
        modes_[this_exchange].push_back(temp_str.substr(delimiter + 1));
      } break;

      case '?':
        if (optopt == 'a' || optopt == 'b' || optopt == 'd' || optopt == 'e') {
          fprintf(stderr, "Option %c requires an argument .. will exit \n", optopt);
          exit(-1);
        }
        break;

      default:
        fprintf(stderr, "Weird option specified .. no handling yet \n");
        break;
    }
  }

  if (hflag) {
    PrintUsage(argv[0], "", true);
  }
}

void ValidateExchangeMode(const char* exec_name, bool is_integrated_mode_, HFSAT::FastMdConsumerMode_t mode,
                          std::string exch_, std::string bcast_ip, int bcast_port) {
  if (exch_.empty() || (exch_ != "NTP" && exch_ != "BMF" && 
                        exch_ != "PUMA" && exch_ != "NTP_ORD")) {
    PrintUsage(exec_name, "Error .. invalid input option. Will exit.", true);
  }

  // If the present operating mode is DATA (mcast) mode, then ensure that it is supported for this exchange
  // If the mode is INTEGRATED, bcast ip and post have to be fetched automatically from network_account_info file
  if (is_integrated_mode_ && (mode & HFSAT::kMcast)) {
    if ((exch_ != "CME") && (exch_ != "EOBI") && (exch_ != "LIFFE") && (exch_ != "RTS") && (exch_ != "ICE")) {
      // Not suported for this exchange
      PrintUsage(exec_name, "INTEGRATED mode not supported for DATA mode for exch: " + exch_, true);
    }
  } else if ((mode & HFSAT::kMcast) && (bcast_ip.empty() || bcast_port <= 0)) {
    PrintUsage(exec_name, "Improper Broadcast Parameters specified for DATA MODE. Will exit.\n", true);
  }
}

HFSAT::MulticastSenderSocket* GetMcastSenderSocket(HFSAT::FastMdConsumerMode_t mode, bool is_integrated_mode_,
                                                   std::string bcast_ip, int bcast_port,
                                                   HFSAT::ExchSource_t exchange_src, std::string shortcode = "",
                                                   std::string exch_str = "") {
  HFSAT::NetworkAccountInfoManager network_account_info_manager_;
  HFSAT::MulticastSenderSocket* sock = NULL;
  if (mode & HFSAT::kMcast) {
    std::string interface;
    if (exch_str == "") {
      interface = HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(exchange_src, HFSAT::k_MktDataMcast);
    } else {
      interface = HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(exch_str, HFSAT::k_MktDataMcast);
    }
    if (is_integrated_mode_) {
      // Fetch Bcast IP and port from file
      HFSAT::DataInfo data_info_ = network_account_info_manager_.GetSrcDataInfo(exchange_src, shortcode);
      sock = new HFSAT::MulticastSenderSocket(data_info_.bcast_ip_, data_info_.bcast_port_, interface);
    } else {
      // Rely on provided values for Bcast IP and port
      sock = new HFSAT::MulticastSenderSocket(bcast_ip, bcast_port, interface);
    }
  }
  return sock;
}

// Create corresponding data sources (and add to simple_live_dispatcher)
void InitializeDataSources(HFSAT::SimpleLiveDispatcher* simple_live_dispatcher, HFSAT::FastMdConsumerMode_t mode,
                           std::string exch_, std::string log_dir_, std::string bcast_ip, int bcast_port,
                           bool is_integrated_mode_) {
  HFSAT::DebugLogger dbglogger_(1024000, 1);
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  HFSAT::MulticastSenderSocket* sock = NULL;

  if (exch_ == "NTP") {
    HFSAT::NewNTPRawLiveDataSource* ds =
        new HFSAT::NewNTPRawLiveDataSource(dbglogger_, sec_name_indexer_, simple_live_dispatcher, mode, 1);

    sock = GetMcastSenderSocket(mode, is_integrated_mode_, bcast_ip, bcast_port, HFSAT::kExchSourceBMF);

    ds->SetMultiSenderSocket(sock);
    ds->start();

    if ((mode & HFSAT::kLogger) && log_dir_ != "")
      NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance().mdsLogger.set_log_dir(log_dir_ + "/" +
                                                                            exch_);  // must not call before start
  } else if (exch_ == "NTP_ORD") {
    HFSAT::NewNTPRawLiveDataSource* ds =
        new HFSAT::NewNTPRawLiveDataSource(dbglogger_, sec_name_indexer_, simple_live_dispatcher, mode, 2);

    sock = GetMcastSenderSocket(mode, is_integrated_mode_, bcast_ip, bcast_port, HFSAT::kExchSourceBMF);

    ds->SetMultiSenderSocket(sock);
    ds->start();

    if ((mode & HFSAT::kLogger) && log_dir_ != "")
      NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor::GetInstance().mdsLogger.set_log_dir(
          log_dir_ + "/" + exch_);  // must not call before start
  } else if (exch_ == "PUMA") {
    HFSAT::NewPumaRawLiveDataSource* ds =
        new HFSAT::NewPumaRawLiveDataSource(dbglogger_, sec_name_indexer_, simple_live_dispatcher, mode, false);

    sock = GetMcastSenderSocket(mode, is_integrated_mode_, bcast_ip, bcast_port, HFSAT::kExchSourcePUMA);

    ds->SetMultiSenderSocket(sock);
    ds->start();

    if ((mode & HFSAT::kLogger) && log_dir_ != "")
      PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance().mdsLogger.set_log_dir(log_dir_ + "/" +
                                                                              exch_);  // must not call before start
  } else if (exch_ == "BMF") {
    HFSAT::NewNTPRawLiveDataSource* ds =
        new HFSAT::NewNTPRawLiveDataSource(dbglogger_, sec_name_indexer_, simple_live_dispatcher, mode, 4);

    sock = GetMcastSenderSocket(mode, is_integrated_mode_, bcast_ip, bcast_port, HFSAT::kExchSourceBMF);

    ds->SetMultiSenderSocket(sock);
    ds->start();

    if ((mode & HFSAT::kLogger) && log_dir_ != "")
      BMF_MD_PROCESSOR::BmfMDProcessor::GetInstance().mdsLogger.set_log_dir(log_dir_ +
                                                                            "/NTP");  // must not call before start
  }else { 
    std::cout << "No matching exchange found. please check your arguments.\n";
  }
}

int main(int argc, char** argv) {
  /// set signal handler .. add other signals later
  struct sigaction sigact;
  sigact.sa_handler = SigHandler;
  sigaction(SIGINT, &sigact, NULL);

  int c;
  int hflag = 0;
  /// input arguments
  std::string exch_;
  std::string mode_;
  std::vector<std::string> exchanges_;                      // For INTEGRATED mode
  std::map<std::string, std::vector<std::string> > modes_;  // For INTEGRATED mode
  std::string log_dir_ = "";

  std::string bcast_ip = "";
  int bcast_port = -1;

  ProcessInput(argc, argv, c, hflag, exch_, mode_, exchanges_, modes_, log_dir_, bcast_ip, bcast_port);

  HFSAT::SimpleLiveDispatcher* simple_live_dispatcher = new HFSAT::SimpleLiveDispatcher();
  bool is_integrated_mode_ = false;

  if (mode_ != "INTEGRATED") {
    if (exchanges_.size() > 0) {
      // The user must have provided --exchange-mode input without INTEGRATED mode.
      fprintf(stderr, "Conflicting modes. --exchange-mode to be used only when mode is INTEGRATED \n");
      exit(-1);
    }
    exchanges_.push_back(exch_);
    modes_[exch_].push_back(mode_);
  } else {
    if (exchanges_.size() <= 0) {
      // The user must have provided no --exchange-mode input in INTEGRATED mode.
      fprintf(stderr, "No --exchange-mode values provided, while the mode is INTEGRATED \n");
      exit(-1);
    }
    is_integrated_mode_ = true;
  }

  for (unsigned int exch_counter = 0; exch_counter < exchanges_.size(); exch_counter++) {
    exch_ = exchanges_[exch_counter];
    // This variable will be a bitwise OR (|) of all modes
    HFSAT::FastMdConsumerMode_t mode = (HFSAT::FastMdConsumerMode_t)0;
    for (size_t mode_ctr = 0; mode_ctr < modes_[exch_].size(); ++mode_ctr) {
      mode_ = modes_[exch_][mode_ctr];
      HFSAT::FastMdConsumerMode_t temp_mode = HFSAT::GetModeFromString(mode_);
      if (temp_mode == HFSAT::kModeMax) {
        PrintUsage(argv[0], "Exiting. Invalid mode: " + mode_, true);
      }
      mode = mode | temp_mode;
      std::cout << exch_ << " -> " << mode_ << "\n";
    }

    std::cout << exch_ << " => " << mode << "\n";

    // Check provided mode for invalid values
    ValidateExchangeMode(argv[0], is_integrated_mode_, mode, exch_, bcast_ip, bcast_port);

    if (mode & HFSAT::kProShm) (HFSAT::Utils::MDSShmInterface::GetUniqueInstance()).Initialize();

    if (exch_ == "EOBI_PF" && (mode & HFSAT::kLogger)) {
      mode = mode & (~HFSAT::kLogger);
      mode = mode | HFSAT::kPriceFeedLogger;
    }

    InitializeDataSources(simple_live_dispatcher, mode, exch_, log_dir_, bcast_ip, bcast_port, is_integrated_mode_);
  }

  // true for data daemon mode
  simple_live_dispatcher->RunLive();

  return 0;
}
