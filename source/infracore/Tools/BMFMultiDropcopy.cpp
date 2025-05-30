// =====================================================================================
//
//       Filename:  BMFMultiDropcopy.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/15/2015 12:45:48 PM
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

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <set>

#include "infracore/BMFEP/bmf_ep_dc_heartbeat_manager.hpp"
#include "infracore/BMFEP/BMFDropCopyEngineThread.hpp"

std::vector<HFSAT::Utils::BMFDropCopyEngine*> list_of_engines_;
std::vector<HFSAT::Utils::BMFEPDCHeartBeatManager*> list_of_hbt_;

void print_usage(const char* prg_name) {
  printf(" This is the Multiple BMF Dropcopy Engine\n");
  printf(" Usage:%s --output-log-dir <outputlogdi> --config <config_file>\n", prg_name);
}

static struct option data_options[] = {{"help", no_argument, 0, 'h'},
                                       {"output-log-dir", required_argument, 0, 'a'},
                                       {"config", required_argument, 0, 'b'},
                                       {0, 0, 0, 0}};

void termination_handler(int _signal_number_) {
  for (auto engine : list_of_engines_) {
    engine->Logout();
  }

  sleep(30);

  exit(-1);
}

int main(int argc, char** argv) {
  signal(SIGINT, termination_handler);
  signal(SIGSEGV, termination_handler);

  int c;
  int hflag = 0;
  /// input arguments

  std::string output_log_dir = "/spare/local/ORSlogs/";
  std::set<std::string> config_set;

  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "", data_options, &option_index);
    if (c == -1) break;

    switch (c) {
      case 'h':
        hflag = 1;
        break;
      case 'a':
        output_log_dir = optarg;
        break;
      case 'b': {
        config_set.insert(optarg);
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

  if (hflag || output_log_dir == "" || config_set.empty()) {
    print_usage(argv[0]);
    exit(-1);
  }

  for (auto config : config_set) {
    HFSAT::ORS::Settings new_settings(config);

    HFSAT::DebugLogger* dbglogger_ = new HFSAT::DebugLogger(10240, 1);
    int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << output_log_dir << new_settings.getValue("SenderCompID") << "/"
                << "log." << tradingdate_;
    dbglogger_->OpenLogFile(t_temp_oss_.str().c_str(), std::ofstream::app);

    HFSAT::DebugLogger* tradelogger_ = new HFSAT::DebugLogger(10240, 1);
    std::ostringstream trd_temp_oss_;
    trd_temp_oss_ << output_log_dir << new_settings.getValue("SenderCompID") << "/"
                  << "trades." << tradingdate_;
    tradelogger_->OpenLogFile(trd_temp_oss_.str().c_str(), std::ofstream::app);

    HFSAT::Utils::BMFDropCopyEngine* new_dc_engine = new HFSAT::Utils::BMFDropCopyEngine(
        new_settings, *dbglogger_, *tradelogger_,
        std::string(output_log_dir) + std::string("/") + std::string(new_settings.getValue("SenderCompID")));
    new_dc_engine->Connect();
    HFSAT::Utils::BMFEPDCHeartBeatManager* new_hbt_manager =
        new HFSAT::Utils::BMFEPDCHeartBeatManager(new_settings, *dbglogger_, false, new_dc_engine);
    new_dc_engine->run();
    sleep(5);
    new_dc_engine->Login();
    new_hbt_manager->run();

    list_of_engines_.push_back(new_dc_engine);
    list_of_hbt_.push_back(new_hbt_manager);
  }

  for (unsigned int i = 0; i < list_of_engines_.size(); i++) {
    list_of_engines_[i]->stop();
    list_of_hbt_[i]->stop();
  }

  return 0;
}
