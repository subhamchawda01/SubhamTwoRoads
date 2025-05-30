/**
    \file test_ors_rejection_alert.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include <iostream>
#include "dvccode/Utils/ors_rejections_alert_thread.hpp"

int main(int argc, char **argv) {
  HFSAT::ORSRejectionAlerts th;
  th.run();

  std::string symbol;
  int rejection_reason;
  while (true) {
    std::cout << "enter symbol, reason : ";
    std::cin >> symbol;
    std::cin >> rejection_reason;

    th.addToRejectionAlertQueue((HFSAT::ORSRejectionReason_t(rejection_reason)), symbol);
  }
}
