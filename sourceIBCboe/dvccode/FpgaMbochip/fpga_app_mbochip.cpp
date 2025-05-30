#include <EventApi.hpp>
#include <iostream>
#include <cstdlib>
#include <iostream>

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

#include "dvccode/Utils/rdtscp_timer.hpp"

using namespace MBOCHIP::SiliconMD::EApi;
using namespace MBOCHIP::SiliconMD::MApi;
HFSAT::DebugLogger* global_dbglogger_;

/// signal handler
void sighandler(int signum) {
  if (NULL != global_dbglogger_) {
    global_dbglogger_->Close();
    global_dbglogger_ = NULL;
  }
  std::cout << "Printing the results of CPU Profiler " << std::endl;
  const std::vector<HFSAT::CpucycleProfilerSummaryStruct> prof_summary =
      HFSAT::CpucycleProfiler::GetUniqueInstance().GetCpucycleSummary();

  for (unsigned int ii = 0; ii < prof_summary.size(); ii++) {
    if (prof_summary[ii].total_occurrence_ > 0) {
      std::cout << prof_summary[ii].tag_name_ << " " << prof_summary[ii].fifty_percentile_ << " "
                << ((double)prof_summary[ii].ninetyfive_percentile_ / (double)prof_summary[ii].fifty_percentile_) << ' '
                << prof_summary[ii].total_occurrence_ << std::endl;
    }
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
      t_temp_oss_ << "FATAL - FPGA APP MBOCHIP received SIGSEGV on " << hostname;
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

// std::map<long long, timeval > cout_map;
// std::map<long long, long long > cout_map;
std::map<long long, struct timespec> cout_map;

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

  // instantiate the EventApi with MessageApi as message api and NSEFPGAReceiver as the receiver
  EventConfig eventConfig("/home/pengine/prod/live_configs/configNSE.toml");
 //  eventConfig.events_return.mbpBookHw = true;
 //  eventConfig.events_return.mbpBookHwPreMatched = false;
  EventApi<MessageApi<NSEFPGAReceiver>> api(eventConfig);

  std::string filename(argv[1]);

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
        api.subscribeBook(token);
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
  std::string logfilename = "/spare/local/MDSlogs/fpga_app_writer_dbg" + std::string("_") +
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
    HFSAT::AllocateCPUUtils::GetUniqueInstance().AllocateCPUOrExit("fpga_app_mbochip");
  }
  HFSAT::CpucycleProfiler& cpucycle_profiler_ = HFSAT::CpucycleProfiler::SetUniqueInstance(5);
  cpucycle_profiler_.SetTag(1, "ReceiveEvent");
  // start the receiving loop
  bool running = true;
  while (running) {
    // for every iteration of the loop, call the receiveEvent function
    //  HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
    const Event* event = api.receiveEvent();
    // test if the receiveEvent function returned an event
    if (event) {
      // if it did, print the event
      // std::cout << *event << std::endl << std::endl << std::endl;

      // if the event is a fatal error, something is horribly wrong, terminate the program
      switch (event->type) {
          //      HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
        /*
        case MBOCHIP::SiliconMD::EApi::EventMessageType::FatalError:
          std::cout << "FATAL ERROR: " << event->channel->channelId << std::endl;
          running = false;
          break;
        case MBOCHIP::SiliconMD::EApi::EventMessageType::MissingMessage:
          std::cout << "MISSING MESSAGE: " << event->channel->channelId << std::endl;
          running = false;
          break;
        case MBOCHIP::SiliconMD::EApi::EventMessageType::HwBookStall:
          std::cout << "HW BOOK STALL: " << event->channel->channelId << std::endl;
          running = false;
          break;
        */
        case MBOCHIP::SiliconMD::EApi::EventMessageType::HwBook: {
          int32_t msg_seq_no = (event->message.HwBook_->msgSeqNum);
          int32_t token = (event->message.HwBook_->token);
          //          timeval tv;
          struct timespec ts;
          timespec_get(&ts, TIME_UTC);
          //          gettimeofday(&tv, NULL);
          
          // Uncomment for Book comparsion
/*  
          std::cout << "FpgaLatencyTimeStamping " << token << " " << msg_seq_no << " " << ts.tv_sec << "."
                    << std::setw(9) << std::setfill('0') << ts.tv_nsec << " ";
          if (0 < event->message.HwBook_->ask.size() && 0 < event->message.HwBook_->bid.size()) {
            for (unsigned int level = 0;
                 level < 1 && level < event->message.HwBook_->ask.size() && level < event->message.HwBook_->bid.size();
                 level++) {
              std::cout << event->message.HwBook_->ask[level].price / 100.00 << " "
                        << event->message.HwBook_->ask[level].quantity << " "
                        << event->message.HwBook_->bid[level].price / 100.00 << " "
                        << event->message.HwBook_->bid[level].quantity << " ";
            }
          } else {
            std::cout << " 0 0 0 0";
          }
          std::cout << std::endl;
          continue;
*/          

          //           cout_map[msg_seq_no] = ts;

          cout_map[(long long)msg_seq_no * 10000000 + (long long)token] = ts;
          //          cout_map[msg_seq_no] = HFSAT::GetCpucycleCountForTimeTick();
          // GetReadTimeStampCounter();
          if (cout_map.size() > 4500000) {
            for (auto itr : cout_map) {
              //   std::cout << "FpgaLatencyTimeStamping TOKEN " << itr.first <<  " " << itr.second << " 0 0 0 0"
              //   <<std::endl;
              std::cout << "FpgaLatencyTimeStamping token " << itr.first << " " << std::fixed << itr.second.tv_sec
                        << "." << std::setw(9) << std::setfill('0') << itr.second.tv_nsec << " 0 0 0 0" << std::endl;
              //          dbglogger_ << "\n";
            }
            exit(0);
          }

/*            for (unsigned int level = 0; level < 1 && level < event->message.HwBook_->ask.size() && level <
            event->message.HwBook_->bid.size(); level++) dbglogger_ << event->message.HwBook_->ask[level].price / 100.00
            << " "
                       << event->message.HwBook_->ask[level].quantity << " "
                       << event->message.HwBook_->bid[level].price / 100.00 << " "
                       << event->message.HwBook_->bid[level].quantity << " "; */
          //          dbglogger_ << "\n";
        } break;
        default:
          break;
      }
    }
  }
}
