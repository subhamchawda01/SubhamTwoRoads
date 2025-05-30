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
#include "dvccode/Utils/exchange_names.hpp"
#include "dvccode/Utils/holiday_manager.hpp"
#include <stdio.h>

void split(const std::string& s, char delim, std::vector<std::string>& elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode input_date_YYYYMMDD [feature_type (1=settlement price/2=expiry/3=exch_symbol]" << std::endl;
    exit(0);
  }
  std::string _this_shortcode_ = argv[1];
  int tradingdate_ = atoi(argv[2]);
  int feature_type_ = 1;

  if (argc > 3) {
    feature_type_ = atoi(argv[3]);
  }

    
  if(feature_type_ == 1) {
    int t_intdate_ = HFSAT::DateTime::CalcNextWeekDay(tradingdate_);
    while (HFSAT::HolidayManagerNoThrow::IsExchangeHoliday(HFSAT::EXCHANGE_KEYS::kExchSourceNSEStr, t_intdate_, true)) {
      t_intdate_ = HFSAT::DateTime::CalcNextWeekDay(t_intdate_);
    }
    HFSAT::SecurityDefinitions::GetUniqueInstance(t_intdate_).LoadNSESecurityDefinitions();
    std::cout << HFSAT::NSESecurityDefinitions::GetLastClose(_this_shortcode_) << std::endl;
  } else if(feature_type_ == 2) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
    std::cout << HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(_this_shortcode_) << std::endl;
  } else if(feature_type_ == 3) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
    std::cout << HFSAT::NSESecurityDefinitions::GetExchSymbolNSE(_this_shortcode_) << std::endl;
  }

  return 0;
}
