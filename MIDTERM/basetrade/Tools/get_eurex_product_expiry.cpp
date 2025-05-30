/**
   \file CDefCode/exchange_symbol_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iomanip>

#include <boost/date_time/gregorian/gregorian.hpp>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/security_definitions.hpp"

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "dvccode/CDef/exchange_symbol_manager.hpp"

inline unsigned int YYYYMMDD_from_date(const boost::gregorian::date& d1);
bool IsEUREXMonth(const std::string& _pure_basename_, const int _this_month_);
int GetEUREXLastTradingDateYYYYMM(std::string _pure_basename_, const int next_eurex_month_, const int next_eurex_year_);
inline bool IsEUREXExchangeDate(const std::string& _pure_basename_, boost::gregorian::date& d1);
void SetToNextEUREXMonth(const std::string& _pure_basename_, int& _next_eurex_month_, int& _next_eurex_year_);

/// input arguments : input_date
int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " shc_basename month year" << std::endl;
    std::cerr << "Usage: " << argv[0] << argc << std::endl;
    exit(0);
  }

  std::string basename_ = argv[1];
  int month_ = atoi(argv[2]);
  int year_ = atoi(argv[3]);

  //  int day_ = atoi ( argv [ 4] ) ;

  // while ( ! IsEUREXMonth ( basename_ , month_ ) )
  // {
  //   SetToNextEUREXMonth ( basename_, month_, year_ ) ;
  //}

  int expiry_ = GetEUREXLastTradingDateYYYYMM(basename_, month_, year_);

  // if ( expiry_ % 100 <= day_ )
  //  {

  //      month_ = month_ + 1;

  //    while ( ! IsEUREXMonth ( basename_ , month_ ) )

  //	{
  //	  SetToNextEUREXMonth ( basename_, month_, year_ ) ;
  //	}

  //      expiry_ =  GetEUREXLastTradingDateYYYYMM ( basename_, month_ , year_ )  ;

  // }

  std::cout << expiry_ << "\n";
  return expiry_;
}

const char* EUREX_FGBS = "FGBS";
const char* EUREX_FGBM = "FGBM";
const char* EUREX_FGBL = "FGBL";
const char* EUREX_FGBX = "FGBX";
const char* EUREX_FBTS = "FBTS";
const char* EUREX_FBTP = "FBTP";
const char* EUREX_FOAT = "FOAT";
const char* EUREX_CONF = "CONF";
const char* EUREX_FESX = "FESX";
const char* EUREX_FDAX = "FDAX";
const char* EUREX_FSMI = "FSMI";
const char* EUREX_FEXF = "FEXF";
const char* EUREX_FESB = "FESB";
const char* EUREX_FSTB = "FSTB";
const char* EUREX_FSTS = "FSTS";
const char* EUREX_FSTO = "FSTO";
const char* EUREX_FSTG = "FSTG";
const char* EUREX_FSTI = "FSTI";
const char* EUREX_FSTM = "FSTM";
const char* EUREX_FXXP = "FXXP";
const char* EUREX_OKS2 = "OKS2";
const char* EUREX_FEXD = "FEXD";
const char* EUREX_FRDX = "FRDX";
const char* EUREX_FVS = "FVS";  // check if anywhere we use assumption of eurex shortcode size being 4 and enable
const char* EUREX_FEU3 = "FEU3";

const boost::gregorian::date_duration one_day_date_duration(1);

inline unsigned int YYYYMMDD_from_date(const boost::gregorian::date& d1) {
  boost::gregorian::date::ymd_type ymd = d1.year_month_day();
  return (((ymd.year * 100 + ymd.month) * 100) + ymd.day);
}

inline bool IsEUREXExchangeDate(const std::string& _pure_basename_, boost::gregorian::date& d1) {
  // TODO only using weekends as holidays, need to add EUREX holiday from Quantlib
  return ((d1.day_of_week() != boost::gregorian::Saturday) && (d1.day_of_week() != boost::gregorian::Sunday));
}

bool IsEUREXMonth(const std::string& _pure_basename_, const int _this_month_) {
  if (_pure_basename_.compare(EUREX_FVS) == 0) return true;  // fvs is a monthly contract
  return ((_this_month_ % 3) == 0);
}

void SetToNextEUREXMonth(const std::string& _pure_basename_, int& _next_eurex_month_, int& _next_eurex_year_) {
  if (_pure_basename_.compare(EUREX_FVS) == 0) {  // FVS is monthly

    if (_next_eurex_month_ == 12) {
      _next_eurex_month_ = 1;
      _next_eurex_year_++;

    } else {
      _next_eurex_month_++;
    }

  } else {
    if (_next_eurex_month_ == 12) {
      _next_eurex_month_ = 3;
      _next_eurex_year_++;
    } else {
      do {
        _next_eurex_month_++;
      } while (!IsEUREXMonth(_pure_basename_, _next_eurex_month_));
    }
  }
}

/// Last Trading Day = Final Settlement Day is the third Friday, if this is an exchange day; otherwise the exchange day
/// immediately preceding that day.
int GetEUREXLastTradingDateYYYYMM(std::string _pure_basename_, const int next_eurex_month_,
                                  const int next_eurex_year_) {
  if ((_pure_basename_.compare(EUREX_FESX) == 0) || (_pure_basename_.compare(EUREX_FDAX) == 0) ||
      (_pure_basename_.compare(EUREX_FEXF) == 0) || (_pure_basename_.compare(EUREX_FESB) == 0) ||
      (_pure_basename_.compare(EUREX_FSTB) == 0) || (_pure_basename_.compare(EUREX_FSTS) == 0) ||
      (_pure_basename_.compare(EUREX_FSTO) == 0) || (_pure_basename_.compare(EUREX_FSTG) == 0) ||
      (_pure_basename_.compare(EUREX_FSTI) == 0) || (_pure_basename_.compare(EUREX_FSTM) == 0) ||
      (_pure_basename_.compare(EUREX_FXXP) == 0) || (_pure_basename_.compare(EUREX_FVS) == 0) ||
      (_pure_basename_.compare(EUREX_FSMI) == 0)) {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                       boost::gregorian::Friday, next_eurex_month_);
    boost::gregorian::date d1 = ndm.get_date(next_eurex_year_);

    while (!IsEUREXExchangeDate(_pure_basename_, d1)) d1 -= one_day_date_duration;

    return YYYYMMDD_from_date(d1);
  }
  if ((_pure_basename_.compare(EUREX_OKS2) == 0)) {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::second,
                                                       boost::gregorian::Thursday, next_eurex_month_);
    boost::gregorian::date d1 = ndm.get_date(next_eurex_year_);

    while (!IsEUREXExchangeDate(_pure_basename_, d1)) d1 -= one_day_date_duration;

    return YYYYMMDD_from_date(d1);
  }

  if ((_pure_basename_.compare(EUREX_FEU3) == 0)) {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                       boost::gregorian::Wednesday, next_eurex_month_);
    boost::gregorian::date d1 = ndm.get_date(next_eurex_year_);

    d1 -= one_day_date_duration;
    d1 -= one_day_date_duration;
    d1 -= one_day_date_duration;

    return YYYYMMDD_from_date(d1);
  }

  if ((_pure_basename_.compare(EUREX_FGBS) == 0) || (_pure_basename_.compare(EUREX_FGBM) == 0) ||
      (_pure_basename_.compare(EUREX_FGBL) == 0) || (_pure_basename_.compare(EUREX_FGBX) == 0) ||
      (_pure_basename_.compare(EUREX_FBTS) == 0) || (_pure_basename_.compare(EUREX_FBTP) == 0) ||
      (_pure_basename_.compare(EUREX_CONF) == 0) || (_pure_basename_.compare(EUREX_FOAT) == 0)) {
    boost::gregorian::date d1(next_eurex_year_, next_eurex_month_, 10);  ///< tenth calendar day of the contract month

    // if this isn't an exchange day then return the next exchange day after it
    while (!IsEUREXExchangeDate(_pure_basename_, d1)) {
      d1 += one_day_date_duration;
    }

    // Docs says Last trading day is 2 days before delivery day
    for (auto i = 0u; i < 2; i++) {
      do {
        d1 -= one_day_date_duration;
      } while (!IsEUREXExchangeDate(_pure_basename_, d1));
    }

    return YYYYMMDD_from_date(d1);
  }

  // default day 1
  boost::gregorian::date d1(next_eurex_year_, next_eurex_month_, 10);
  // d1 -= one_day_date_duration;
  return YYYYMMDD_from_date(d1);
}
