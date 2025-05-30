#pragma once

#include <cinttypes>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <stdint.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include "dvccode/Utils/md5.hpp"

#define hton16 htons
#define hton32 __builtin_bswap32
#define hton64 __builtin_bswap64

#define ntoh16 ntohs
#define ntoh32 __builtin_bswap32
#define ntoh64 __builtin_bswap64

namespace HFSAT {
namespace CBOE {

#pragma pack(push, 1)
struct InstrumentDesc {
  int32_t token_;
  char instrument_type_[6];
  char symbol_[10];
  int32_t expiry_date_;
  int32_t strike_price_;
  char option_type_[2];  // CM uses this as series info in structs

  InstrumentDesc() {
    token_ = 0;
    memset(instrument_type_, ' ', 6);
    memset(symbol_, ' ', 10);
    expiry_date_ = 0;
    strike_price_ = 0;
    memset(option_type_, ' ', 2);
  }

  std::string ToString() {
    std::stringstream ss;
    ss << " Token: " << token_;
    ss << " InstrumentType: ";
    for (int i = 0; i < 6; i++) {
      ss << instrument_type_[i];
    }
    ss << " Symbol: ";
    for (int i = 0; i < 10; i++) {
      ss << symbol_[i];
    }
    ss << " ExpiryDate: " << expiry_date_;
    ss << " StrikePrice: " << (int)(strike_price_);
    ss << " OptionType: ";
    for (int i = 0; i < 2; i++) {
      ss << option_type_[i];
    }
    std::string str = ss.str();
    return str;
  }

  inline void SetInstrumentDesc(int32_t token, char const *instrument_type, char const *symbol, int32_t const &expiry,
                                int32_t const &strike, char const *option_type) {
    token_ = token;
    strncpy(instrument_type_, instrument_type, 6);
    strncpy(symbol_, symbol, 10);
    expiry_date_ = expiry;
    strike_price_ = strike;
    strncpy(option_type_, option_type, 2);
  }

  int GetToken() { return token_; }

  int GetTwiddledToken() { return token_; }
  char const *GetInstrumentDescAsBuffer() const { return (char const *)&token_; }
};
#pragma pack(pop)

}  // namespace CBOE
}  // namespace HFSAT
