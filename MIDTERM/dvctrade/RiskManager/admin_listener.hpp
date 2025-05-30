#pragma once
#include "dvctrade/RiskManager/risk_notifier.hpp"

namespace HFSAT {

// Listens for updates (getflat/start-trading) from the central risk server
class AdminListener : public HFSAT::Thread {
  HFSAT::TCPClientSocket& tcp_socket_;
  RiskNotifier* notifier_;

 public:
  AdminListener(RiskNotifier* notifier) : tcp_socket_(notifier->GetClientSocket()), notifier_(notifier) {}

  void thread_main() {
    GetFlatStruct get_flat;
    while (true) {
      int read_len = tcp_socket_.ReadNFixed(sizeof(get_flat), &get_flat);
      std::cerr << "Got flat msg: " << read_len << ", " << get_flat.ToString() << "\n";
      (std::cerr).flush();
      if (read_len < 0) {
        if (notifier_->ReconnectToServer()) {          // Some network error, retry to connect
          tcp_socket_ = notifier_->GetClientSocket();  // reconnected. read again in next try
        } else {
          std::cerr << "AdminListener couldnt reconnect, will try again "
                    << "\n";
          (std::cerr).flush();
        }
        continue;
      }
      notifier_->IssueGetFlat(get_flat);
    }
  }
};
}
