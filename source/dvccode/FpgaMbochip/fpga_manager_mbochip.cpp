#include <iostream>
#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <set>
#include <fstream>
#include <ManagerApi.hpp>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/online_debug_logger.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/CDef/currency_convertor.hpp"
#include "dvccode/CombinedControlUtils/combined_control_message_livesource.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/Utils/mds_shm_interface.hpp"
#include "dvccode/Utils/load_our_defined_products.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/Utils/data_daemon_config.hpp"
#include "dvccode/Utils/allocate_cpu.hpp"
#include "dvccode/Utils/allocate_cpu.hpp"

using namespace MBOCHIP::SiliconMD::EApi;
using namespace MBOCHIP::SiliconMD::MApi;

HFSAT::DebugLogger* global_dbglogger_;

/// signal handler
void sighandler(int signum) {
  if (NULL != global_dbglogger_) {
    global_dbglogger_->Close();
    global_dbglogger_ = NULL;
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
      t_temp_oss_ << "FATAL - FPGA MANAGER MBOCHIP received SIGSEGV on " << hostname;
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

  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::CurrencyConvertor::RemoveInstance();

  exit(0);
}

int main(int argc, char** argv) {
  struct sigaction sigact;
  memset(&sigact, '\0', sizeof(sigact));
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGSEGV, &sigact, NULL);
  if (argc != 2) {
    std::cerr << "Usage : <exec> <input-file> " << std::endl;
    exit(-1);
  }

  int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_);

  HFSAT::SecurityDefinitions::GetUniqueInstance().LoadNSESecurityDefinitions();
  HFSAT::Utils::NSERefDataLoader::GetUniqueInstance(tradingdate_);
  HFSAT::Utils::NSEDailyTokenSymbolHandler& nse_daily_token_symbol_handler_ =
      HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(tradingdate_);

  std::string filename(argv[1]);
  // initialize the channel filter
  ChannelFilter channel011filter{};
  ChannelFilter channel012filter{};
  ChannelFilter channel013filter{};
  ChannelFilter channel014filter{};
  ChannelFilter channel015filter{};
  ChannelFilter channel016filter{};

  string shortcode_;
  ifstream shortcode_file(filename);
  while (getline(shortcode_file, shortcode_)) {
    if (shortcode_ == "") continue;
    bool Kexist = HFSAT::ExchangeSymbolManager::CheckIfContractSpecExists(shortcode_);
    if (Kexist == true) {
      std::string exchange_symbol = HFSAT::ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(shortcode_);
      std::string internal_symbol = HFSAT::NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol);
      char segment = HFSAT::NSESecurityDefinitions::GetSegmentTypeFromShortCode(shortcode_);
      int32_t token = nse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(internal_symbol.c_str(), segment);
      if (token != -1 /*Dummy Token*/) {
        std::cout << "Valid " << shortcode_ << " " << token << std::endl;
        char initial = shortcode_.at(4);
        if (initial == 'A' || initial == 'G' || initial == 'R' || initial == 'V') {
          std::cout << shortcode_ << " Added to ChannelId: 016" << std::endl;
          channel016filter.instruments.push_back(token);
        } else if (initial == 'B' || initial == 'E' || initial == 'F' || initial == 'L') {
          std::cout << shortcode_ << " Added to ChannelId: 011" << std::endl;
          channel011filter.instruments.push_back(token);
        } else if (initial == 'N' || initial == 'P' || initial == 'T') {
          std::cout << shortcode_ << " Added to ChannelId: 012" << std::endl;
          channel012filter.instruments.push_back(token);
        } else if (initial == 'D' || initial == 'J' || initial == 'M') {
          std::cout << shortcode_ << " Added to ChannelId: 013" << std::endl;
          channel013filter.instruments.push_back(token);
        } else if (initial == 'C' || initial == 'I' || initial == 'U' || initial =='W') {
          std::cout << shortcode_ << " Added to ChannelId: 014" << std::endl;
          channel014filter.instruments.push_back(token);
        } else if (initial == 'H' || initial == 'K' || initial == 'O' || initial == 'S' || initial == 'Z') {
          std::cout << shortcode_ << " Added to ChannelId: 015" << std::endl;
          channel015filter.instruments.push_back(token);
        } else {
          std::cout << "Invalid ShortCode Name 4th Character " << shortcode_ << std::endl;
        }
      } else {
        std::cout << " Invalid" << shortcode_ << " " << token << std::endl;
        exit(-1);
      }
    } else {
      std::cout << " Invalid" << shortcode_ << std::endl;
      exit(-1);
    }
  }
  shortcode_file.close();

  HFSAT::DebugLogger dbglogger_(10240, 1);
  std::string logfilename = "/spare/local/MDSlogs/fpga_manager_writer_dbg" + std::string("_") +
                            HFSAT::DateTime::GetCurrentIsoDateLocalAsString() + ".log";

  dbglogger_.OpenLogFile(logfilename.c_str(), std::ios::out | std::ios::app);
  global_dbglogger_ = &dbglogger_;

  dbglogger_ << "OpenDbglogger\n";
  dbglogger_.DumpCurrentBuffer();

  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);

  // We do not want to affine at NY servers
  if (std::string(hostname).find("sdv-ny4-srv") == std::string::npos) {
    HFSAT::AllocateCPUUtils::GetUniqueInstance().AllocateCPUOrExit("fpga_manager_mbochip");
  }

  /**********************************************************************************
   * Event API and channels configuration
   **********************************************************************************/
  ChannelConfig config{};
  config.recovery = true;
  EventConfig eventConfig("/home/pengine/prod/live_configs/configNSE.toml");
  /**********************************************************************************
   * Initialize the API
   **********************************************************************************/
  ManagerApi<MessageApi<NSEFPGAReceiver>> api(eventConfig);

  /**********************************************************************************
   * Instruments filtering
   **********************************************************************************/
  if (channel011filter.instruments.size() > 0) {
    api.enableChannel("011", config, channel011filter);
  } 
  if (channel012filter.instruments.size() > 0) {
    api.enableChannel("012", config, channel012filter);
  } 
  if (channel013filter.instruments.size() > 0) {
    api.enableChannel("013", config, channel013filter);
  } 
  if (channel014filter.instruments.size() > 0) {
    api.enableChannel("014", config, channel014filter);
  } 
  if (channel015filter.instruments.size() > 0) {
    api.enableChannel("015", config, channel015filter);
  } 
  if (channel016filter.instruments.size() > 0) {
    api.enableChannel("016", config, channel016filter);
  }

  // start the receiving loop
  bool running = true;
  // bool printEvents = false;
  while (running) {
    // for every iteration of the loop, call the receiveEvent function
    const Event* event = api.receiveEvent();

    // test if the receiveEvent function returned an event
    if (event) {
      // if it did, print the event
      //   if (printEvents) std::cout << *event << std::endl << std::endl << std::endl;

      // if the event is a fatal error, something is horribly wrong, terminate the program
      switch (event->type) {
        case MBOCHIP::SiliconMD::EApi::EventMessageType::FatalError:
          std::cout << "FATAL ERROR: " << event->channel->channelId << std::endl;
          running = false;
          break;
        case MBOCHIP::SiliconMD::EApi::EventMessageType::SnapshotError:
          std::cout << "SNAPSHOT ERROR: " << event->channel->channelId << std::endl;
          running = false;
          break;
        case MBOCHIP::SiliconMD::EApi::EventMessageType::MissingMessage:
          std::cout << "MISSING MESSAGE: " << event->channel->channelId << std::endl;
          running = false;
          break;
        default:
          break;
      }
    }
  }
}
