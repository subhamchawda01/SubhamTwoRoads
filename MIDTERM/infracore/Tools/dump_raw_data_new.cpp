#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <signal.h>
#include <fstream>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"

#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"

#include "dvccode/ExternalData/dump_raw_data_listener.hpp"

std::vector<HFSAT::SimpleExternalDataLiveListener*> listeners;

void termHandler(int signum) {
  for (unsigned int i = 0; i < listeners.size(); i++) {
    // close sockets and BulkFileWriters
    listeners[i]->CleanUp();
  }

  exit(0);
}

int main(int argc, char* argv[]) {
  signal(SIGINT, termHandler);
  signal(SIGSEGV, termHandler);

  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " Exchange_name" << std::endl;
    exit(0);
  }

  std::string config_file = "/home/dvcinfra/";
  config_file.append(argv[1]);
  config_file.append("_raw_data_config");

  std::ifstream config;  // format: reference/incremental side_a/side_b IP PORT IP_CODE
  config.open(config_file.c_str());

  HFSAT::NetworkAccountInterfaceManager network_account_interface_manager_();

  HFSAT::SimpleLiveDispatcher simple_live_dispatcher_(-1);

  std::string mode;
  std::string i_mode;

  std::string exch;
  std::string ip;
  int port;
  int ip_code = 1;
  int date = HFSAT::DateTime::GetCurrentIsoDateLocal();

  config >> exch;
  char final_config_file_name[200];
  char i_a_name[200];
  char i_b_name[200];
  char r_name[200];

  sprintf(final_config_file_name, "/spare/local/MDSlogs/Raw/config_%s_%d", exch.c_str(), date);
  sprintf(i_a_name, "/spare/local/MDSlogs/Raw/incremental_a_%s_%d", exch.c_str(), date);
  sprintf(i_b_name, "/spare/local/MDSlogs/Raw/incremental_b_%s_%d", exch.c_str(), date);
  sprintf(r_name, "/spare/local/MDSlogs/Raw/reference_%s_%d", exch.c_str(), date);

  HFSAT::BulkFileWriter bfw_i_a(i_a_name, N, std::ios::app | std::ios::binary);
  if (!bfw_i_a.is_open()) {
    std::cerr << "Cannot open file: " << i_a_name << std::endl;
    exit(0);
  }

  HFSAT::BulkFileWriter bfw_i_b(i_b_name, N, std::ios::app | std::ios::binary);
  if (!bfw_i_b.is_open()) {
    std::cerr << "Cannot open file: " << i_b_name << std::endl;
    exit(0);
  }

  HFSAT::BulkFileWriter bfw_r(r_name, N, std::ios::app | std::ios::binary);
  if (!bfw_r.is_open()) {
    std::cerr << "Cannot open file: " << r_name << std::endl;
    exit(0);
  }

  if (!exch.compare("PUMA")) exch = "BMF_EQ";

  if (!exch.compare("ICE_FOD") || !exch.compare("ICE_PL")) exch = "ICE";

  while (config >> mode) {
    config >> i_mode >> ip >> port >> ip_code;
    if (mode.compare("reference") == 0) {
      listeners.push_back(new HFSAT::DumpRawDataListener(ip, port, ip_code, exch, &bfw_r));
      simple_live_dispatcher_.AddSimpleExternalDataLiveListenerSocket(
          listeners.back(),
          (((HFSAT::DumpRawDataListener*)(listeners.back()))->GetMulticastReceiverSocket())->socket_file_descriptor(),
          true);

    }

    else if (mode.compare("incremental") == 0) {
      if (i_mode.compare("side_a") == 0) {
        listeners.push_back(new HFSAT::DumpRawDataListener(ip, port, ip_code, exch, &bfw_i_a));
        simple_live_dispatcher_.AddSimpleExternalDataLiveListenerSocket(
            listeners.back(),
            (((HFSAT::DumpRawDataListener*)(listeners.back()))->GetMulticastReceiverSocket())->socket_file_descriptor(),
            true);
      }

      else if (i_mode.compare("side_b") == 0) {
        listeners.push_back(new HFSAT::DumpRawDataListener(ip, port, ip_code, exch, &bfw_i_b));
        simple_live_dispatcher_.AddSimpleExternalDataLiveListenerSocket(
            listeners.back(),
            (((HFSAT::DumpRawDataListener*)(listeners.back()))->GetMulticastReceiverSocket())->socket_file_descriptor(),
            true);
      } else {
        std::cout << "Invalid option : " << mode << " " << i_mode << std::endl;
        std::cout << "Skipping data from " << ip_code << " " << port << std::endl;
      }
    }

    else {
      std::cout << "Invalid option : " << mode << " " << i_mode << std::endl;
      std::cout << "Skipping data from " << ip_code << " " << port << std::endl;
    }
  }

  std::ofstream final_config(final_config_file_name, std::ios::binary);

  config.clear();
  config.seekg(0, config.beg);  // setting pointer to beginning

  final_config << config.rdbuf();
  config.close();
  final_config.close();
  remove(config_file.c_str());

  simple_live_dispatcher_.RunLive();

  return 1;
}
