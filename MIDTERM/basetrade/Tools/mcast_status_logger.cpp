/**
    \file Tools/mcast_status_logger.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <getopt.h>
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include <map>
#include <string>
#define SLEEP_INTERVAL 10
void print_usage(const char* prg_name) {
  printf("This is mcast_status_logger exec \n");
  printf("Usage:%s --logfile <log> --progid <id> --config <cfile> \n", prg_name);
  printf("--logfile File name where bandwidth data is stored \n");
  printf("--config File with channel info \n");
  printf("--progid Id name for usage with mcast reader \n");
}

struct McastInfo {
  std::string src_name_;
  std::string mcast_ip_;
  int mcast_port_;
  std::string iface_;

  McastInfo(const std::string& t_src_name_, const std::string& t_mcast_ip_, const int& t_mcast_port_,
            const std::string& t_iface_)
      : src_name_(t_src_name_), mcast_ip_(t_mcast_ip_), mcast_port_(t_mcast_port_), iface_(t_iface_) {}

  std::string toString() {
    std::stringstream ss;
    ss << src_name_ << "\t" << mcast_ip_ << "\t" << mcast_port_ << "\t" << iface_ << "\t";
    return ss.str();
  }
};

void LoadMcastInfoVec(const std::string& mcast_info_filename_, std::vector<McastInfo>& mcast_info_vec_) {
  std::ifstream mcast_info_file_;
  mcast_info_file_.open(mcast_info_filename_.c_str(), std::ifstream::in);
  if (mcast_info_file_.is_open()) {
    const int kNetworkAccountInfoFileLineBufferLen = 1024;
    char readline_buffer_[kNetworkAccountInfoFileLineBufferLen];
    bzero(readline_buffer_, kNetworkAccountInfoFileLineBufferLen);

    while (mcast_info_file_.good()) {
      bzero(readline_buffer_, kNetworkAccountInfoFileLineBufferLen);
      mcast_info_file_.getline(readline_buffer_, kNetworkAccountInfoFileLineBufferLen);
      HFSAT::PerishableStringTokenizer st_(readline_buffer_, kNetworkAccountInfoFileLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() >= 5) {  // MCAST TEXT MCAST_IP MCAST_PORT IFACE

        if (strcmp(tokens_[0], "MCAST") == 0) {
          McastInfo this_mcast_info_(tokens_[1], tokens_[2], atoi(tokens_[3]), tokens_[4]);
          mcast_info_vec_.push_back(this_mcast_info_);
        }
      }
    }
  }
}

void SetupMcastReceiverSockets(const std::vector<McastInfo>& mcast_info_vec_,
                               std::vector<HFSAT::MulticastReceiverSocket*>& mcast_receiver_socket_vec_,
                               std::map<int, HFSAT::MulticastReceiverSocket*>& port_to_p_mrs_map_) {
  for (auto i = 0u; i < mcast_info_vec_.size(); i++) {
    if (port_to_p_mrs_map_.find(mcast_info_vec_[i].mcast_port_) == port_to_p_mrs_map_.end()) {
      mcast_receiver_socket_vec_.push_back(new HFSAT::MulticastReceiverSocket(
          mcast_info_vec_[i].mcast_ip_, mcast_info_vec_[i].mcast_port_, mcast_info_vec_[i].iface_));
      mcast_receiver_socket_vec_.back()->SetNonBlocking();
    } else {
      HFSAT::MulticastReceiverSocket* p_mrs_map_ = port_to_p_mrs_map_[mcast_info_vec_[i].mcast_port_];
      p_mrs_map_->McastJoin(mcast_info_vec_[i].mcast_ip_);
    }
  }
}

static struct option msloptions[] = {{"help", no_argument, 0, 'h'},
                                     {"logfile", required_argument, 0, 'l'},
                                     {"progid", required_argument, 0, 'p'},
                                     {"config", required_argument, 0, 'c'},
                                     {0, 0, 0, 0}};

int main(int argc, char** argv) {
  int c;
  /// input arguments
  std::string cfile;
  std::string lfile;

  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "", msloptions, &option_index);
    if (c == -1) break;
    switch (c) {
      case 'h':
        print_usage(argv[0]);
        exit(-1);
        break;

      case 'l':
        lfile = optarg;
        break;

      case 'c':
        cfile = optarg;
        break;

      case 'p':
        break;

      case '?':
        if (optopt == 'l' || optopt == 'c') {
          fprintf(stderr, "Option %c requires an argument .. will exit \n", optopt);
          print_usage(argv[0]);
          exit(-1);
        }
        break;

      default:
        fprintf(stderr, "Weird option specified .. no handling \n");
        print_usage(argv[0]);
        exit(-1);
    }
  }

  /// check for valid path of config and logfile
  if (cfile.empty() || lfile.empty()) {
    printf("Error .. invalid input option. Will exit \n");
    print_usage(argv[0]);
    exit(-1);
  }

  std::vector<McastInfo> mcast_info_vec_;
  std::vector<HFSAT::MulticastReceiverSocket*> mcast_receiver_socket_vec_;
  std::map<int, HFSAT::MulticastReceiverSocket*> port_to_p_mrs_map_;

  LoadMcastInfoVec(cfile.c_str(), mcast_info_vec_);

  SetupMcastReceiverSockets(mcast_info_vec_, mcast_receiver_socket_vec_, port_to_p_mrs_map_);

  if (mcast_receiver_socket_vec_.empty()) {
    std::cerr << "Did not find any mcast pairs" << std::endl;
    exit(0);
  }

  std::vector<int> time_idle_vec_;
  time_idle_vec_.resize(mcast_receiver_socket_vec_.size(), 0);
  std::vector<int> total_bytes_read_;
  total_bytes_read_.resize(mcast_receiver_socket_vec_.size(), 0);
  std::vector<int> bytes_curr_read_;
  bytes_curr_read_.resize(mcast_receiver_socket_vec_.size(), 0);

  HFSAT::DebugLogger snaplogger_(10240, 100);
  snaplogger_.OpenLogFile(lfile.c_str(), std::ofstream::out);
  time_t now;

  char buf_[1024 * 1024];
  while (1) {
    time(&now);
    snaplogger_ << "Time :" << asctime(localtime(&now)) << "\n";
    for (auto i = 0u; i < mcast_receiver_socket_vec_.size(); i++) {
      int received_size_;
      bool recvd_some_ = false;
      bytes_curr_read_[i] = 0;
      do {
        received_size_ =
            std::max(0, (int)read(mcast_receiver_socket_vec_[i]->socket_file_descriptor(), buf_, 1024 * 1024));
        if (received_size_ > 0) {
          time_idle_vec_[i] = 0;
          recvd_some_ = true;
          bytes_curr_read_[i] += received_size_;
          total_bytes_read_[i] += received_size_;
        }
      } while (received_size_ > 0);
      if (!recvd_some_) time_idle_vec_[i] += SLEEP_INTERVAL;
      snaplogger_ << (mcast_info_vec_[i]).toString() << "\t" << time_idle_vec_[i] << "\t" << bytes_curr_read_[i] << "\t"
                  << total_bytes_read_[i] << "\n";
    }
    snaplogger_ << "\n\n";
    snaplogger_.DumpCurrentBuffer();
    sleep(SLEEP_INTERVAL);
  }
}
