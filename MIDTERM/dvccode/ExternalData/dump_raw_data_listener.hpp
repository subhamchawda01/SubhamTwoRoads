#pragma once
#include <arpa/inet.h>
#include <errno.h>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <time.h>

#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"

#define N 4000000
namespace HFSAT {

class DumpRawDataListener : public SimpleExternalDataLiveListener {
  MulticastReceiverSocket* multicast_receiver_socket_;
  std::string interface_;
  int ip_code_;
  HFSAT::BulkFileWriter* bfw_;
  char msg_buf[N];
  int msg_len;
  timeval time_;

 public:
  DumpRawDataListener(const std::string& md_udp_ip, const int md_udp_port, const int ip_code, std::string exch,
                      HFSAT::BulkFileWriter* bfw);

  void ProcessAllEvents(int this_socket_fd_);
  MulticastReceiverSocket* GetMulticastReceiverSocket();
  void CloseMulticastReceiverSocket();
  void CleanUp();
};
}
