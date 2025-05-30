/**
    \file Tools/mcast_bw_simple_tool.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include <time.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <signal.h>
#include <ncurses.h>
#include <map>

#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#define MCAST_BW_IDLE_WARN 30
#define MCAST_BW_IDLE_CRITICAL 90

struct McastInfo {
  std::string src_name_;
  std::string mcast_ip_;
  int mcast_port_;

  McastInfo(const std::string& t_src_name_, const std::string& t_mcast_ip_, const int& t_mcast_port_)
      : src_name_(t_src_name_), mcast_ip_(t_mcast_ip_), mcast_port_(t_mcast_port_) {}
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

      if (tokens_.size() >= 4) {  // MCAST TEXT MCAST_IP MCAST_PORT

        if (strcmp(tokens_[0], "MCAST") == 0) {
          McastInfo this_mcast_info_(tokens_[1], tokens_[2], atoi(tokens_[3]));
          mcast_info_vec_.push_back(this_mcast_info_);
        }
      }
    }
  }
}

void SetupMcastReceiverSockets(const std::vector<McastInfo>& mcast_info_vec_,
                               std::vector<HFSAT::MulticastReceiverSocket*>& mcast_receiver_socket_vec_,
                               std::map<int, HFSAT::MulticastReceiverSocket*>& port_to_p_mrs_map_,
                               std::string preferred_interface) {
  for (auto i = 0u; i < mcast_info_vec_.size(); i++) {
    if (port_to_p_mrs_map_.find(mcast_info_vec_[i].mcast_port_) == port_to_p_mrs_map_.end()) {
      mcast_receiver_socket_vec_.push_back(new HFSAT::MulticastReceiverSocket(
          mcast_info_vec_[i].mcast_ip_, mcast_info_vec_[i].mcast_port_, preferred_interface));
      mcast_receiver_socket_vec_.back()->SetNonBlocking();
    } else {
      HFSAT::MulticastReceiverSocket* p_mrs_map_ = port_to_p_mrs_map_[mcast_info_vec_[i].mcast_port_];
      p_mrs_map_->McastJoin(mcast_info_vec_[i].mcast_ip_, preferred_interface);
    }
  }
}

/// signal handler
void sighandler(int signum) {
  endwin();
  raise(SIGKILL);
}

int main(int argc, char** argv) {
  std::vector<McastInfo> mcast_info_vec_;
  std::vector<HFSAT::MulticastReceiverSocket*> mcast_receiver_socket_vec_;
  std::map<int, HFSAT::MulticastReceiverSocket*> port_to_p_mrs_map_;

  std::string mcast_info_filename_ = "";

  if (argc < 2) {
    std::cerr << " Usage: " << argv[0] << " mcast_info_filename [optinal:network_interface_name]" << std::endl;
    exit(0);
  }

  mcast_info_filename_ = argv[1];

  LoadMcastInfoVec(mcast_info_filename_, mcast_info_vec_);

  std::string def_interface = "eth5";
  if (argc == 3) def_interface = argv[2];

  SetupMcastReceiverSockets(mcast_info_vec_, mcast_receiver_socket_vec_, port_to_p_mrs_map_, def_interface);

  if (mcast_receiver_socket_vec_.empty()) {
    std::cerr << "Did not find any mcast pairs" << std::endl;
    exit(0);
  }

  std::vector<int> time_idle_vec_;
  time_idle_vec_.resize(mcast_receiver_socket_vec_.size(), 0);

  /// set signal handler .. add other signals later
  struct sigaction sigact;
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);

  initscr();
  start_color();
  assume_default_colors(COLOR_BLACK, COLOR_WHITE);
  init_pair(1, COLOR_GREEN, COLOR_WHITE);
  init_pair(2, COLOR_YELLOW, COLOR_WHITE);
  init_pair(3, COLOR_RED, COLOR_WHITE);

  char buf_[1024 * 1024];
  while (1) {
    move(0, 0);
    for (auto i = 0u; i < mcast_receiver_socket_vec_.size(); i++) {
      int received_size_ = std::max(0, mcast_receiver_socket_vec_[i]->ReadN(1024 * 1024, buf_));
      if (received_size_ > 0)
        time_idle_vec_[i] = 0;
      else
        time_idle_vec_[i] += 1;
      if (time_idle_vec_[i] < MCAST_BW_IDLE_WARN) {
        attron(COLOR_PAIR(1));
        mvprintw(i, 0, "%-20s", mcast_info_vec_[i].src_name_.c_str());
        mvprintw(i, 25, "%s:%d", mcast_info_vec_[i].mcast_ip_.c_str(), mcast_info_vec_[i].mcast_port_);
        mvprintw(i, 60, "%d\n", received_size_);
        attroff(COLOR_PAIR(1));
      } else if (time_idle_vec_[i] < MCAST_BW_IDLE_CRITICAL) {
        attron(COLOR_PAIR(2));
        mvprintw(i, 0, "%-20s", mcast_info_vec_[i].src_name_.c_str());
        mvprintw(i, 25, "%s:%d", mcast_info_vec_[i].mcast_ip_.c_str(), mcast_info_vec_[i].mcast_port_);
        mvprintw(i, 60, "%d\n", received_size_);
        attroff(COLOR_PAIR(2));
      } else {
        attron(COLOR_PAIR(3));
        mvprintw(i, 0, "%-20s", mcast_info_vec_[i].src_name_.c_str());
        mvprintw(i, 25, "%s:%d", mcast_info_vec_[i].mcast_ip_.c_str(), mcast_info_vec_[i].mcast_port_);
        mvprintw(i, 60, "%d\n", received_size_);
        attroff(COLOR_PAIR(3));
      }
    }
    refresh();
    //      sleep ( 1 );
  }
  exit(0);
}
