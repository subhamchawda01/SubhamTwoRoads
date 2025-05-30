// =====================================================================================
//
//       Filename:  exanic_rx_buffer.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/13/2018 10:03:39 AM
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

#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <vector>
#include <sstream>
#include <exanic/exanic.h>
#include <exanic/fifo_rx.h>
#include <exanic/filter.h>

#include "dvccode/Utils/sem_utils.hpp"
#include "dvccode/Utils/async_writer.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"
#include "dvccode/Utils/tcp_direct_client_zocket_with_logging.hpp"
#define MAXEVENTS 1024
#define MAX_NSE_DATA_BUFFER 65536
#define EXANIC_BUFFER_MD_CHANNELS "exanic-nse-tbt-mcast.txt"
namespace HFSAT {
namespace Utils {

//  struct rx_msg {
//    struct zft_msg msg;
//    struct iovec iov[1];
//  };

struct ExanicSocket {
  exanic_rx_t *rx_;
  bool packets_left_;
  uint32_t socket_fd_to_last_seen_seq_;
  char segment_type_;
  bool is_trade_exec_fd_;
  bool is_spot_idx_fd_;
  bool is_oi_data_fd_;
  HFSAT::SimpleExternalDataLiveListener *listener_;
};

class ExanicMultipleRx {
 private:
  char const *device_name;
  int device_port;
  int max_iov_;
  int sz_received;
  int buffer_number;
  exanic_cycles32_t exa_timestamp;
  exanic_t *exanic;
  char data_buffer[MAX_NSE_DATA_BUFFER];
  struct epoll_event ev_[32];
  struct ExanicSocket exanic_sockets_[32];
  int32_t last_allocated_exanic_counter_;
  std::map<std::string, int> exanic_ip_bufferId;
  HFSAT::RuntimeProfiler &runtime_profiler_;
  EventTimeoutListener *event_timeout_listener_;
  std::vector<exanic_rx_t *> exanic_rx_buffers;

 public:
  ExanicMultipleRx() : runtime_profiler_(HFSAT::RuntimeProfiler::GetUniqueInstance()) {
    device_name = "exanic0";
    exanic = exanic_acquire_handle(device_name);
    if (!exanic) {
      fprintf(stderr, "exanic_acquire_handle: %s\n", exanic_get_last_error());
      std::cout << " Exanic NOT SETUP ON SYSTEM ethier device name is diff or exanic card not installed\n " << std::endl;
      return;
    }
    device_port = 1;
    buffer_number = 1;
    std::cout << "Exanic Port used 0/1 is " << device_port << std::endl;
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);
    std::ostringstream exanic_host_nse_tbt_filename;
    exanic_host_nse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << EXANIC_BUFFER_MD_CHANNELS;
    std::string bufferid_nse_channel_filename = exanic_host_nse_tbt_filename.str();
    std::cout << "File Name: " << bufferid_nse_channel_filename << std::endl;
    std::ifstream nse_buffer_channels_file;
    nse_buffer_channels_file.open(bufferid_nse_channel_filename.c_str());
    if (!nse_buffer_channels_file.is_open()) {
	    std::cout << "Failed To Load The EXANIC BUFFER ID in TBT Multicast File : " << bufferid_nse_channel_filename
                                   << std::endl;
      exit(-1);
    }
    char buffer[1024];
    int MAX_LINE_SIZE = 1024;
    while (nse_buffer_channels_file.good()) {
	nse_buffer_channels_file.getline(buffer, MAX_LINE_SIZE);
        std::string line_buffer = buffer;

        // Comments
        if (line_buffer.find("#") != std::string::npos) continue;	
        HFSAT::PerishableStringTokenizer pst(buffer, MAX_LINE_SIZE);
        std::vector<char const*> const& tokens = pst.GetTokens();

        // We expect to read StreamId, StreamIP, StreamPort
        if (tokens.size() != 3) continue;
        std::string key_value = tokens[1] + std::string(tokens[2]);
        exanic_ip_bufferId[key_value] = stoi(tokens[0]); 
	std::cout << "Key: " << key_value << " VALUE : "<< exanic_ip_bufferId[key_value] << std::endl;
    }
  }

  ExanicMultipleRx(ExanicMultipleRx const &disabled_copy_constructor) = delete;

 public:
  static ExanicMultipleRx &GetUniqueInstance() {
    static ExanicMultipleRx unique_instance;
    return unique_instance;
  }

  void CreateSocketAndAddToMuxer(std::string ip, int32_t port, HFSAT::SimpleExternalDataLiveListener *listener,
                                 char seg_type, bool is_trade_exec_fd, bool is_spot_idx_fd, bool is_oi_data_fd_) {
    if (last_allocated_exanic_counter_ > 32) {
      std::cout << "CAN'T ALLOCATE ANY MORE exanic ZOCKETS : " << std::endl;
      std::exit(-1);
    }
    else if (exanic_ip_bufferId.size() == 0){
      std::cout << "exanic_ip_bufferId NOT SETUP in ExanicMultipleRx class " << std::endl;
      std::exit(-1);
    }
    exanic_rx_t *rx;
    exanic_ip_filter_t filter;
    struct in_addr src_addr, dst_addr;
    std::cout << "PORT ID" << port << std::endl;
//    rx = exanic_acquire_unused_filter_buffer(exanic, device_port);
    buffer_number = exanic_ip_bufferId[ip + to_string(port)];
    std::cout << "Buffer Number: " << buffer_number <<std::endl;
    rx = exanic_acquire_rx_buffer(exanic, device_port, buffer_number);
    if (!rx) {
      fprintf(stderr, "exanic_acquire_rx_buffer: %s\n", exanic_get_last_error());
      std::cout << "Unable to acquire buffer " << rx->buffer_number << std::endl;
      exit(1);
    }
    std::cout << "BUFFER ID " << buffer_number << std::endl;
    inet_aton("0", &src_addr);  // src = 0
    inet_aton(ip.c_str(), &dst_addr);
    filter.src_addr = src_addr.s_addr;
    filter.dst_addr = dst_addr.s_addr;
    filter.src_port = htons(0);  // src = 0
    filter.dst_port = htons(port);
    filter.protocol = 17;  // UDP protocal =17
    int my_filter = exanic_filter_add_ip(exanic, rx, &filter);
    std::cout << "Filter ADDED ID " << my_filter << std::endl;
    if (my_filter == -1) {
      fprintf(stderr, "%s: %s\n", device_name, exanic_get_last_error());
      exit(1);
    }

    exanic_sockets_[last_allocated_exanic_counter_].rx_ = rx;
    exanic_sockets_[last_allocated_exanic_counter_].listener_ = listener;
    exanic_sockets_[last_allocated_exanic_counter_].socket_fd_to_last_seen_seq_ = UINT_MAX;
    exanic_sockets_[last_allocated_exanic_counter_].segment_type_ = seg_type;
    exanic_sockets_[last_allocated_exanic_counter_].is_trade_exec_fd_ = is_trade_exec_fd;
    exanic_sockets_[last_allocated_exanic_counter_].is_spot_idx_fd_ = is_spot_idx_fd;
    exanic_sockets_[last_allocated_exanic_counter_].is_oi_data_fd_ = is_oi_data_fd_;
    last_allocated_exanic_counter_++;

    std::cout << "SOCKET CREATED : " << ip << " " << port << std::endl;
  }

  void AddEventTimeoutNotifyListener(EventTimeoutListener *listener) { event_timeout_listener_ = listener; }

  inline void ReadAndDispatchEvents(ExanicSocket *exa_socket) {
/*      sz_received = exanic_receive_frame(exa_socket->rx_, data_buffer, MAX_NSE_DATA_BUFFER, &exa_timestamp);
      runtime_profiler_.Start(HFSAT::ProfilerType::kCOMBINEDSHMWRITER);
      (exa_socket->listener_)
          ->ProcessEventsFromUDPDirectRead(*data_buffer, sz_received , exa_socket->segment_type_,
                                           exa_socket->is_trade_exec_fd_, exa_socket->socket_fd_to_last_seen_seq_);
*/    }

inline void NotifyTimeOut() { event_timeout_listener_->OnEventsTimeout(); }

void RunLiveDispatcherWithTimeOut(int64_t timeout) {
  /*    int64_t wait_timeout = timeout;
      struct epoll_event event_array[MAXEVENTS];
      int epollfd = -1, timeout_ = 0;
      epollfd = epoll_create(MAXEVENTS);
      if ( epollfd == -1 ) {
          std::cerr << "epoll creation failed\n";
          return;
      }
      for (int counter_ = 0; counter_ < MAXEVENTS; counter_++) {
      memset(&event_array[counter_], 0, sizeof(epoll_event));
      }
      for (int32_t fd_counter = 0; fd_counter < last_allocated_exanic_counter_; fd_counter++) {

        event_array[primary_fd_counter].data.fd = exanic_sockets_[fd_counter].rx_->buffer_number;

        std::cout << " Adding FD : " << fd_counter << "\n";
        (std::cout).flush();
        // associate Level Triggered epoll event to the fd
        event_array[fd_counter].events = EPOLLIN;

        es = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd_counter, &(event_array[fd_counter]));

        if (es == -1) {
          std::cerr << " Error Adding Epoll Event \n";
          exit(-1);
        }
      }

      while (true) {
        int number_of_descriptors_with_data = epoll_wait(epollfd, events, MAXEVENTS, timeout_);

        if (0 == no_of_events) {
          NotifyTimeOut();
          continue;
        }

        for (int32_t ev_counter = 0; ev_counter < no_of_events; ev_counter++) {
          ReadAndDispatchEvents(exanic_sockets_[events[data_fd_counter].data.fd]);
        }
      }
  */
}

void RunLiveDispatcherWithNotify(){
  bool no_of_events;
  while (true) {
    no_of_events = false;
    for (int counter_ = 0; counter_ < last_allocated_exanic_counter_; counter_++) {
      ssize_t sz =
          exanic_receive_frame(exanic_sockets_[counter_].rx_, data_buffer, MAX_NSE_DATA_BUFFER, &exa_timestamp);

      if (sz > 42) {
        const char *msg_ptr = data_buffer + 42;
        sz = sz - 42;
	runtime_profiler_.Start(HFSAT::ProfilerType::kCOMBINEDSHMWRITER);
        exanic_sockets_[counter_].listener_->ProcessEventsFromUDPDirectRead(
            msg_ptr, sz, exanic_sockets_[counter_].segment_type_, exanic_sockets_[counter_].is_trade_exec_fd_,
            exanic_sockets_[counter_].is_spot_idx_fd_, exanic_sockets_[counter_].is_oi_data_fd_, exanic_sockets_[counter_].socket_fd_to_last_seen_seq_);
	no_of_events = true;
      }
    }
    if (false == no_of_events) {
	NotifyTimeOut();
    }
  }
}

void RunLiveDispatcher() {
  while (true) {
    for (int counter_ = 0; counter_ < last_allocated_exanic_counter_; counter_++) {
      ssize_t sz =
          exanic_receive_frame(exanic_sockets_[counter_].rx_, data_buffer, MAX_NSE_DATA_BUFFER, &exa_timestamp);

      //		if (sz <= 42) {NotifyTimeOut(); continue;}
      if (sz > 42) {
	const char *msg_ptr = data_buffer + 42;
        sz = sz - 42;
	runtime_profiler_.Start(HFSAT::ProfilerType::kCOMBINEDSHMWRITER);
        exanic_sockets_[counter_].listener_->ProcessEventsFromUDPDirectRead(
            msg_ptr, sz, exanic_sockets_[counter_].segment_type_, exanic_sockets_[counter_].is_trade_exec_fd_,
            exanic_sockets_[counter_].is_spot_idx_fd_, exanic_sockets_[counter_].is_oi_data_fd_, exanic_sockets_[counter_].socket_fd_to_last_seen_seq_);
      }
    }

    //      		NotifyTimeOut();
  }
}
};
}  // namespace Utils
}  // namespace HFSAT
