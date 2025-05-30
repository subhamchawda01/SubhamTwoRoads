#ifndef _CBOE_MDS_DEFINES_
#define _CBOE_MDS_DEFINES_

#include <iomanip>
#include <inttypes.h>
#include <sstream>
#include <string>
#include <sys/time.h>
#include "dvccode/CDef/defines.hpp"
#define DEBUG 0

namespace CBOE_UDP_MDS {

struct CBOERefData {
  int64_t token;
  int64_t expiry;
  double strike_price;
  char instrument[7];
  char symbol[11];
  char option_type[3];
  HFSAT::ExchSource_t exch_source;

  CBOERefData() : token(0), expiry(0), strike_price(-1) {
    memset(instrument, 0, sizeof(instrument));
    memset(symbol, 0, sizeof(symbol));
    memset(option_type, 0, sizeof(option_type));
  }

  void Init(int32_t token_, int64_t expiry_, double strike_price_, const char *instrument_, const char *symbol_,
            const char *option_type_, HFSAT::ExchSource_t ex_src) {
    CBOERefData();
    token = token_;
    expiry = expiry_;
    strike_price = strike_price_;
    memcpy(instrument, instrument_, sizeof(instrument) - 1);
    memcpy(symbol, symbol_, strlen(symbol_));
    memcpy(option_type, option_type_, sizeof(option_type) - 1);
    exch_source = ex_src;
  }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "token: " << token << "\n";
    t_temp_oss_ << "expiry: " << expiry << "\n";
    t_temp_oss_ << "strike_price: " << strike_price << "\n";
    t_temp_oss_ << "instrument: " << instrument << "\n";
    t_temp_oss_ << "symbol: " << symbol << "\n";
    t_temp_oss_ << "option_type: " << option_type << "\n";
    return t_temp_oss_.str();
  }
};
}


#endif
