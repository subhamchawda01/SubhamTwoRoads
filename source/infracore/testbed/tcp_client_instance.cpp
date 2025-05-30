#include <time.h>
#include "dvccode/Utils/tcp_client_socket.hpp"
#include <iostream>
#include <string>
#include <sstream>
#define MAX_SYMBOL_LENGTH_ORS_DATA 16

enum UpdateType { MarketDataUpdate = 0, ORSDataUpdate };

struct ClientORSDataStruct {
  char instrument_[MAX_SYMBOL_LENGTH_ORS_DATA];

  double bid_price_;
  int bid_size_;

  double ask_price_;
  int ask_size_;

  UpdateType update_type_;

  std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << " Symbol : " << instrument_ << " BidPx : " << bid_price_ << " BidSz : " << bid_size_
               << " AskPx : " << ask_price_ << " AskSz : " << ask_size_ << " UpdateType : " << update_type_ << "\n";

    return t_temp_oss.str();
  }
};

int main(int argc, char** argv) {
  std::string tcp_ip_;
  int tcp_port_;

  if (argc < 2) {
    std::cerr << " Usage: " << argv[0] << " <ip> <port> " << std::endl;
    exit(0);
  }

  tcp_ip_ = argv[1];
  tcp_port_ = atoi(argv[2]);

  HFSAT::TCPClientSocket abc_;
  abc_.Connect(tcp_ip_, tcp_port_);
  int length_ = 10000;

  ClientORSDataStruct temp;

  while (true) {
    int length_read_ = abc_.ReadN(sizeof(ClientORSDataStruct), (void*)&temp);
    std::cout << temp.ToString();
  }
}
