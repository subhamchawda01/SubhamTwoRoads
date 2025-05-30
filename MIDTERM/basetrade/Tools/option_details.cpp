/**
 *
 *
 *        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 *             Address:
 *                   Suite No 353, Evoma, #14, Bhattarhalli,
 *                   Old Madras Road, Near Garden City College,
 *                   KR Puram, Bangalore 560049, India
 *                   +91 80 4190 355
 **/

#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include <stdio.h>

/*
[1 50] [2.5] [5-1-5, 5]
[51 100] [5] [5-1-5, 5]
[101 250] [10] [5-1-5, 5]
[251 500] [20] [5-1-5, 5]

[501 1000] [20] [10-1-10, 10]
[1001 Inf] [50] [10-1-10, 10]
*/


void split(const std::string& s, char delim, std::vector<std::string>& elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode input_date_YYYYMMDD [is_prev_expiry_ = 0]" << std::endl;
    exit(0);
  }
  std::string _this_shortcode_ = argv[1];
  int tradingdate_ = atoi(argv[2]);
  bool is_prev_expiry_ = false;

  if (argc > 3) {
    is_prev_expiry_ = (atoi(argv[3]) != 0);
  }

  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
  int expiry_date_;
  if (is_prev_expiry_)
    expiry_date_ = HFSAT::NSESecurityDefinitions::ComputePreviousExpiry(tradingdate_, _this_shortcode_);
  else
    expiry_date_ = HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(_this_shortcode_);

  if (HFSAT::NSESecurityDefinitions::IsOption(_this_shortcode_)) {
    double strike_price_ = HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCode(_this_shortcode_);
    std::vector<std::string> tokens1_;
    split(_this_shortcode_, '_', tokens1_);
    std::cout << expiry_date_ << " " << strike_price_ << std::endl;
    std::cout << "Prev " << HFSAT::NSESecurityDefinitions::GetPrevOptionInCurrentSchema(_this_shortcode_) << "\n";
    std::cout << "Next " << HFSAT::NSESecurityDefinitions::GetNextOptionInCurrentSchema(_this_shortcode_) << "\n";
  } else {
    std::vector<std::string> options_ns_otm_ =
        HFSAT::NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInCurrScheme(_this_shortcode_, -1);
    std::cout << "NewSchemaAll: ";
    for (auto i = 0u; i < options_ns_otm_.size(); i++) std::cout << options_ns_otm_[i] << " ";
    std::cout << std::endl;

    std::vector<std::string> options_os_otm_ =
        HFSAT::NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInPrevScheme(_this_shortcode_, -1);
    std::cout << "OldSchemaAll: ";
    for (auto i = 0u; i < options_os_otm_.size(); i++) std::cout << options_os_otm_[i] << " ";
    std::cout << std::endl;

    std::vector<std::string> options_top4callns_otm_ =
        HFSAT::NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInCurrScheme(_this_shortcode_, 0, 1, 4);
    std::cout << "Top4CallNS: ";
    for (auto i = 0u; i < options_top4callns_otm_.size(); i++) std::cout << options_top4callns_otm_[i] << " ";
    std::cout << std::endl;

    std::vector<std::string> options_top4putns_otm_ =
        HFSAT::NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInCurrScheme(_this_shortcode_, 0, -1, 4);
    std::cout << "Top4PutNS: ";
    for (auto i = 0u; i < options_top4putns_otm_.size(); i++) std::cout << options_top4putns_otm_[i] << " ";
    std::cout << std::endl;
  }

  return 0;
}
