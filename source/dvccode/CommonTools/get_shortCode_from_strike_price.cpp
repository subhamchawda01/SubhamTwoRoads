#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include "dvccode/CDef/security_definitions.hpp"
int main(int argc, char** argv) {
  if (argc < 5) {
    std::cerr << "Usage : <exec> <input file with shortcodes> <output file> <date> <Exchange>" << std::endl;
    exit(0);
  }
  std::string input_file = argv[1];
  std::string output_file = argv[2];
  std::string line;
  int input_date_ = atoi(argv[3]);
  std::string exchange_ = argv[4];
  std::ifstream infile;
  infile.open(input_file.c_str(), std::ifstream::in);
  std::ofstream outfile;
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadBSESecurityDefinitions();
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  outfile.open(output_file.c_str(), std::ofstream::out);
  while (std::getline(infile, line)) {
    std::string shc_;
    std::vector<std::string> trimmed_str;
    boost::split(trimmed_str, line, boost::is_any_of(" "));
    double strike_price = std::stod(trimmed_str[2]);
    int expr = std::stoi(trimmed_str[1]);
    if (exchange_ == "NSE") {
      if (trimmed_str[3] == std::string("CE")) {
        shc_ = HFSAT::NSESecurityDefinitions::GetShortcodeFromCanonical(trimmed_str[0], expr, strike_price, 1);
      } else {
        shc_ = HFSAT::NSESecurityDefinitions::GetShortcodeFromCanonical(trimmed_str[0], expr, strike_price, 0);
      }
    } else {
      if (trimmed_str[3] == std::string("CE")) {
        shc_ = HFSAT::BSESecurityDefinitions::GetShortcodeFromCanonical(trimmed_str[0], expr, strike_price, 1);
      } else {
        shc_ = HFSAT::BSESecurityDefinitions::GetShortcodeFromCanonical(trimmed_str[0], expr, strike_price, 0);
      }
    }
    outfile << shc_ << " " << trimmed_str[4] << std::endl;
  }
  infile.close();
  outfile.close();
  return 0;
}
