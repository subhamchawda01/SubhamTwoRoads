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
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/Utils/allocate_cpu.hpp"

#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "infracore/lwfixfast/livesources/new_ntp_raw_live_data_source.hpp"
#include "infracore/lwfixfast/livesources/new_puma_raw_live_data_source.hpp"
#include "infracore/lwfixfast/ntp_md_processor.hpp"
#include "infracore/lwfixfast/ntp_ord_md_processor.hpp"
#include "infracore/lwfixfast/bmf_md_processor.hpp"
#include "infracore/lwfixfast/puma_md_processor.hpp"
#include "infracore/LiveSources/generic_live_data_source.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"

#define FILTERED_SECURITIES_FOR_MULTICAST_FILE "/home/pengine/prod/live_configs/filtered_security_for_multicast.txt"

void print_usage(const char* prg_name) {
  printf(" This is the FixFast Data Daemon Executable \n");
  printf(
      " Usage:%s --exchange <exch> --mode <mode> [--bcast_ip <address> --bcast_port <port>] [ --log_dir <dir_name> ]\n",
      prg_name);
  printf(" --exchange Exchange Name (accepted values CME/EUREX/NTP/BMF/CMEMDP/EOBI/CSM/SGX) \n");
  printf(" --mode     Mode of operation (accepted values DATA/REFERENCE/LOGGER/SHMWRITER) \n");
  printf(" --bcast_ip Multicast Address for Market Data. Compulsory for DATA mode.\n");
  printf(" --bcast_port Broadcast Port for Market Data. Compulsory for DATA mode.\n");
  printf(" --log_dir Dir where mds data is logged. Optionally present only for LOGGER mode.\n");
}

static struct option data_options[] = {{"help", no_argument, 0, 'h'},
                                       {"exchange", required_argument, 0, 'a'},
                                       {"mode", required_argument, 0, 'b'},
                                       {"bcast_ip", required_argument, 0, 'd'},
                                       {"bcast_port", required_argument, 0, 'e'},
                                       {"log_dir", required_argument, 0, 'f'},
                                       {0, 0, 0, 0}};

HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericControlRequestStruct>* generic_control_live_data_source = NULL;
HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericORSReplyStruct>* generic_ors_live_data_source = NULL;
/// signal handler
void sighandler(int signum) {
  fprintf(stderr, "Received signal %d \n", signum);

  NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance().cleanup();
  BMF_MD_PROCESSOR::BmfMDProcessor::GetInstance().cleanup();
  if (generic_control_live_data_source != NULL) {
    generic_control_live_data_source->CleanUp();
  }
  if (generic_ors_live_data_source != NULL) {
    generic_ors_live_data_source->CleanUp();
  }

  std::cout << "Just Before Exiting..." << std::endl;

  exit(0);
}

void LoadFilteredSecurities(std::string source_exch) {
  std::ifstream needed_products_file_;
  needed_products_file_.open(FILTERED_SECURITIES_FOR_MULTICAST_FILE, std::ifstream::in);

  if (!needed_products_file_.is_open()) {
    std::cerr << "Could not open " << FILTERED_SECURITIES_FOR_MULTICAST_FILE << std::endl;
    exit(-1);
  }

  HFSAT::SecurityNameIndexer& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  char line[1024];
  while (!needed_products_file_.eof()) {
    bzero(line, 1024);
    needed_products_file_.getline(line, 1024);
    if (strlen(line) == 0 || strstr(line, "#") != NULL) continue;
    HFSAT::PerishableStringTokenizer st_(line, 1024);
    const std::vector<const char*>& tokens_ = st_.GetTokens();
    if (tokens_.size() < 2) {
      std::cerr << "Malformatted line in " << FILTERED_SECURITIES_FOR_MULTICAST_FILE << std::endl;
      exit(-1);
    }

    std::string shortcode = tokens_[0];
    std::string exch = tokens_[1];

    if (source_exch == exch) {
      const char* exch_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode);
      sec_name_indexer.AddString(exch_symbol, shortcode);
      std::cout << "Multicasting data for exch :  " << exch << " : " << shortcode << std::endl;
    }
  }
  needed_products_file_.close();
}

int main(int argc, char** argv) {
  /// set signal handler .. add other signals later
  struct sigaction sigact;
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);

  int c;
  int hflag = 0;
  /// input arguments
  std::string exch_;
  std::string mode_;
  std::string log_dir_ = "";

  std::string bcast_ip = "";
  int bcast_port = -1;

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
    print_usage(argv[0]);
    exit(-1);
  }

  if (exch_.empty() ||

      (exch_ != "CONTROL" && exch_ != "NTP" && exch_ != "BMF_PUMA" && exch_ != "BMF" && exch_ != "PUMA" &&
       exch_ != "NTP_ORD" && exch_ != "ORS_REPLY") ||

      (mode_.empty() ||
       (mode_ != "DATA" && mode_ != "REFERENCE" && mode_ != "FULLREFERENCE" && mode_ != "LOGGER" &&
        mode_ != "SHMWRITER" && mode_ != "PF_CONVERTOR" && mode_ != "CONVERT_LOGGER" && mode_ != "NCLOGGER" &&
        mode_ != "CMEOBFDATA" && mode_ != "OSEOFDATA" && mode_ != "OSEPFDATA"))) {
    printf("Error .. invalid input option. Will exit.\n");
    print_usage(argv[0]);
    exit(-1);
  }

  HFSAT::AllocateCPUUtils::GetUniqueInstance().AllocateCPUOrExit("MarketDataDaemon");

  int today_date = HFSAT::DateTime::GetCurrentIsoDateLocal();

  HFSAT::DebugLogger dbglogger_(1024000, 1);
  std::ostringstream t_temp_oss;
  t_temp_oss << "/spare/local/MDSlogs/"
             << "mktdd_" << exch_ << "_" << mode_ << "_" << today_date << ".log";
  dbglogger_.OpenLogFile(t_temp_oss.str().c_str());

  HFSAT::SecurityDefinitions::GetUniqueInstance(today_date);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(today_date);

  if ((mode_ == "DATA") && (bcast_ip.empty() || bcast_port == -1)) {
    HFSAT::NetworkAccountInfoManager network_account_info_manager_;
    HFSAT::DataInfo data_info_;
    data_info_ = network_account_info_manager_.GetSrcDataInfo(HFSAT::StringToExchSource(exch_));
    bcast_ip = data_info_.bcast_ip_;
    bcast_port = data_info_.bcast_port_;
    dbglogger_ << "Using Bcast IP And Port As " << bcast_ip << " X " << bcast_port << "\n";
    dbglogger_.DumpCurrentBuffer();
  }

  HFSAT::FastMdConsumerMode_t mode =
      (mode_ == "REFERENCE" || mode_ == "FULLREFERENCE")
          ? HFSAT::kReference
          : ((mode_ == "LOGGER" || mode_ == "NCLOGGER")
                 ? HFSAT::kLogger
                 : ((mode_ == "SHMWRITER") ? HFSAT::kProShm
                                           : ((mode_ == "CONVERT_LOGGER")
                                                  ? HFSAT::kConvertLogger
                                                  : ((mode_ == "CMEOBFDATA") || (mode_ == "OSEOFDATA")
                                                         ? HFSAT::kMcastOF
                                                         : ((mode_ == "OSEPFDATA") ? HFSAT::kMcast : HFSAT::kMcast)))));

  bool is_full_reference_mode = (mode_ == "FULLREFERENCE");

  if (mode == HFSAT::kProShm) (HFSAT::Utils::MDSShmInterface::GetUniqueInstance()).Initialize();

  if (exch_ == "OSE" && mode == HFSAT::kMcast) {
    LoadFilteredSecurities(exch_);
  }

  if ((exch_.substr(0, 7) == "EOBI_PF" && mode == HFSAT::kLogger) || mode_.compare("PF_CONVERTOR") == 0) {
    mode = HFSAT::kPriceFeedLogger;
  }

  if (mode_.compare("PF_SHMWRITER") == 0) {
    std::cerr << " Mode: Converted Pricefeed Shm writer\n";
    mode = HFSAT::kComShmLogger;
  }

  if ("NCLOGGER" == mode_) {
    mode_ = "LOGGER";
    log_dir_ = "/spare/local/MDSlogs/NonCombined/" + exch_ + "/";
    std::cout << "Using NonCombined Logging Dir As : " << log_dir_ << std::endl;
  }

  HFSAT::SimpleLiveDispatcher* simple_live_dispatcher;
  if ((exch_ == "AFLASH" || exch_ == "AFLASH_NEW") && mode_ == "DATA") {
    simple_live_dispatcher = new HFSAT::SimpleLiveDispatcher(1000);
  } else {
    simple_live_dispatcher = new HFSAT::SimpleLiveDispatcher();
  }

  HFSAT::CpucycleProfiler::SetUniqueInstance(50);
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  HFSAT::MulticastSenderSocket* sock = NULL;

  if (exch_ == "NTP") {
    HFSAT::NewPumaRawLiveDataSource* ds = new HFSAT::NewPumaRawLiveDataSource(
        dbglogger_, sec_name_indexer_, simple_live_dispatcher, mode, is_full_reference_mode, 2);

    if (mode == HFSAT::kMcast) {
      std::string interface =
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBMF, HFSAT::k_MktDataMcast);
      sock = new HFSAT::MulticastSenderSocket(bcast_ip, bcast_port, interface);
    }

    ds->SetMultiSenderSocket(sock);
    ds->start();

    if (mode == HFSAT::kLogger && log_dir_ != "")
      NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance().mdsLogger.set_log_dir(log_dir_);  // must not call before start
  } else if (exch_ == "BMF_PUMA") {
    HFSAT::NewNTPRawLiveDataSource* ds =
        new HFSAT::NewNTPRawLiveDataSource(dbglogger_, sec_name_indexer_, simple_live_dispatcher, mode, 5);

    if (mode == HFSAT::kMcast) {
      std::string interface =
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBMF, HFSAT::k_MktDataMcast);
      sock = new HFSAT::MulticastSenderSocket(bcast_ip, bcast_port, interface);
    }

    ds->SetMultiSenderSocket(sock);
    ds->start();

    if (mode == HFSAT::kLogger && log_dir_ != "")
      NTP_MD_PROCESSOR::NtpMDProcessor::GetInstance().mdsLogger.set_log_dir(log_dir_);  // must not call before start
  } else if (exch_ == "NTP_ORD") {
    HFSAT::NewPumaRawLiveDataSource* ds = new HFSAT::NewPumaRawLiveDataSource(
        dbglogger_, sec_name_indexer_, simple_live_dispatcher, mode, is_full_reference_mode, 3);

    if (mode == HFSAT::kMcast) {
      std::string interface =
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBMF, HFSAT::k_MktDataMcast);
      sock = new HFSAT::MulticastSenderSocket(bcast_ip, bcast_port, interface);
    }

    ds->SetMultiSenderSocket(sock);
    ds->start();

    if (mode == HFSAT::kLogger && log_dir_ != "")
      NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor::GetInstance().mdsLogger.set_log_dir(
          log_dir_);  // must not call before start
  } else if (exch_ == "PUMA") {
    HFSAT::NewPumaRawLiveDataSource* ds = new HFSAT::NewPumaRawLiveDataSource(
        dbglogger_, sec_name_indexer_, simple_live_dispatcher, mode, is_full_reference_mode);

    if (mode == HFSAT::kMcast) {
      std::string interface =
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourcePUMA, HFSAT::k_MktDataMcast);
      sock = new HFSAT::MulticastSenderSocket(bcast_ip, bcast_port, interface);
    }

    ds->SetMultiSenderSocket(sock);
    ds->start();

    if (mode == HFSAT::kLogger && log_dir_ != "")
      PUMA_MD_PROCESSOR::PumaMDProcessor::GetInstance().mdsLogger.set_log_dir(log_dir_);  // must not call before start
  } else if (exch_ == "BMF") {
    HFSAT::NewNTPRawLiveDataSource* ds =
        new HFSAT::NewNTPRawLiveDataSource(dbglogger_, sec_name_indexer_, simple_live_dispatcher, mode, 4);

    if (mode == HFSAT::kMcast) {
      std::string interface =
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBMF, HFSAT::k_MktDataMcast);
      sock = new HFSAT::MulticastSenderSocket(bcast_ip, bcast_port, interface);
    }

    ds->SetMultiSenderSocket(sock);
    ds->start();

    if (mode == HFSAT::kMcast) {
      std::string interface =
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBMF, HFSAT::k_MktDataMcast);
      sock = new HFSAT::MulticastSenderSocket(bcast_ip, bcast_port, interface);
    }

    if (mode == HFSAT::kLogger && log_dir_ != "")
      BMF_MD_PROCESSOR::BmfMDProcessor::GetInstance().mdsLogger.set_log_dir(log_dir_);  // must not call before start
  } else if (exch_ == "CONTROL") {
    HFSAT::NetworkAccountInfoManager network_account_info_manager_;
    HFSAT::DataInfo control_recv_data_info_ = network_account_info_manager_.GetControlRecvDataInfo();
    generic_control_live_data_source =
        new HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericControlRequestStruct>(
            dbglogger_, sec_name_indexer_, HFSAT::MDS_MSG::CONTROL, true, mode);
    generic_control_live_data_source->AddLiveSocket(
        control_recv_data_info_.bcast_ip_, control_recv_data_info_.bcast_port_,
        HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_Control));

    std::vector<int32_t> socket_fd_vec;
    generic_control_live_data_source->GetLiveSocketsFdList(socket_fd_vec);
    for (auto socket_fd_ : socket_fd_vec) {
      simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(generic_control_live_data_source, socket_fd_);
    }
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
      default: {
        current_exch_source_ = HFSAT::kExchSourceMAX;
        dummy_shortcode_ = "";
      } break;
    }

    HFSAT::NetworkAccountInfoManager network_account_info_manager_;
    HFSAT::DataInfo data_info_ = network_account_info_manager_.GetDepDataInfo(current_exch_source_, dummy_shortcode_);
    generic_ors_live_data_source = new HFSAT::MDSMessages::GenericLiveDataSource<HFSAT::GenericORSReplyStruct>(
        dbglogger_, sec_name_indexer_, HFSAT::MDS_MSG::ORS_REPLY, true, mode);
    generic_ors_live_data_source->AddLiveSocket(
        data_info_.bcast_ip_, data_info_.bcast_port_,
        HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_ORS_LS));

    std::vector<int32_t> socket_fd_vec;
    generic_ors_live_data_source->GetLiveSocketsFdList(socket_fd_vec);
    for (auto socket_fd_ : socket_fd_vec) {
      simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(generic_ors_live_data_source, socket_fd_);
    }
  }

  else {
    std::cout << "No matching exchange found. please check your arguments.\n";
  }
  // true for data daemon mode
  simple_live_dispatcher->RunLive();

  return 0;
}
