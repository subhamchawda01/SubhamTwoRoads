#include "dvccode/Utils/send_alert.hpp"
#include <iostream>
int main() {
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);
  std::string alert_message = std::string(hostname) + " voice alert test.";
  bool retVal = HFSAT::SendAlert::sendAlert(alert_message);
  std::cout << retVal << "\n";
  return 0;
}
// g++ -o test_alert -O2 test_alert.cpp  -I../../infracore_install/ -L../../infracore_install/lib/
