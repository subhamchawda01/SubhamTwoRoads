/*
  author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
  Address:
  Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 355
*/

#include "baseinfra/OptionsUtils/security_utils.hpp"
#include <stdio.h>
#include "dvccode/Utils/exchange_names.hpp"
#include "dvccode/Utils/holiday_manager.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

void split(const std::string& s, char delim, std::vector<std::string>& elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

int main(int argc, char** argv) {
  if (argc < 6) {
    std::cerr << "Usage: " << argv[0] << " shortcode option_type start_date end_date currency_ max_<-1/x>" << std::endl;
    exit(0);
  }
  std::string _this_shortcode_ = argv[1];
  std::string _opt_type_ = argv[2];
  int start_date_ = atoi(argv[3]);
  int end_date_ = atoi(argv[4]);
  bool is_currency_ = ( atoi(argv[5]) > 0 ? true : false );
  int max_ = -1;
  
  if ( argc > 6 ) {
    max_ = atoi(argv[6]);
  }

  
  // for a given month, go to previous month and get expiry date and get expiy date of this month
  // give volume pattern of this month for otm options 
  // date rank1, rank2, rank3 ..rankn ( n==strikeschema )
  
  //  int t_intdate_ = HFSAT::DateTime::CalcNextWeekDay(end_date_);
  // look in t+1 bhavcopy 
  //while (HFSAT::HolidayManagerNoThrow::IsExchangeHoliday(HFSAT::EXCHANGE_KEYS::kExchSourceNSEStr, t_intdate_, true)) {
  //  t_intdate_ = HFSAT::DateTime::CalcNextWeekDay(t_intdate_);
  //}

  int t_intdate_ = end_date_;

  while ( t_intdate_ >= start_date_ )
    {
      HFSAT::OptionsSecurityUtils::GetMVInfo(_this_shortcode_, t_intdate_, _opt_type_, is_currency_, max_);

      t_intdate_ = HFSAT::DateTime::CalcPrevWeekDay(t_intdate_);
      while (HFSAT::HolidayManagerNoThrow::IsExchangeHoliday(HFSAT::EXCHANGE_KEYS::kExchSourceNSEStr, t_intdate_, true)) {
	t_intdate_ = HFSAT::DateTime::CalcPrevWeekDay(t_intdate_);
      }
    }
}
