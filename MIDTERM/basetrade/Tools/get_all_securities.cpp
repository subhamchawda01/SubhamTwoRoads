#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/security_definitions.hpp"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage : <exec> <date>\n";
    exit(0);
  }
  int input_date_ = atoi(argv[1]);
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  std::vector<std::string> shortcode_list_;
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).GetAllSecurities(&shortcode_list_);
  for (std::string shc_ : shortcode_list_) {
    std::cout << shc_ << std::endl;
  }
  return 0;
}
