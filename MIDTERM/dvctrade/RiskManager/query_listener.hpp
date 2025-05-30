#pragma once
#include "dvctrade/RiskManager/risk_notifier.hpp"

namespace HFSAT {

// Listens for new queryid/SACI/tag triplets
class QueryListener : public HFSAT::Thread {
  RiskNotifier* notifier_;
  int port_num_;
  HFSAT::TCPServerSocket tcp_socket_;

 public:
  QueryListener(RiskNotifier* notifier, int port_num)
      : notifier_(notifier), port_num_(port_num), tcp_socket_(port_num) {}

  void thread_main() {
    QueryTag query_tag;
    while (true) {
      int read_len = tcp_socket_.ReadNFixed(sizeof(query_tag), &query_tag);
      std::cerr << "Got QueryTag msg: " << read_len << ", " << query_tag.ToString() << "\n";
      (std::cerr).flush();
      if (read_len < 0) {  // Some network error
        std::cerr << "Couldn't read query saci msg" << std::endl;
        (std::cerr).flush();
      } else
        notifier_->AddSACIMapping(query_tag);
    }
    std::cerr << "QueryListener exited. " << std::endl;
    (std::cerr).flush();
  }
};
}
