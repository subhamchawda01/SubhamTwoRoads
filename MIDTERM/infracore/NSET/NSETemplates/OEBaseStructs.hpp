
#pragma once

#include <cstdint>
#include <cstring>
#include <sstream>
#include <iostream>
#include "infracore/NSET/NSETemplates/DataDefines.hpp"

namespace HFSAT {
namespace NSE {

#pragma pack(push, 1)
struct DCHeader {
  int16_t length_;
  int32_t seq_no_;
  uint32_t checksum_[4];

  DCHeader() {
    length_ = 0;
    seq_no_ = 0;
    checksum_[0] = 0;
    checksum_[1] = 0;
    checksum_[2] = 0;
    checksum_[3] = 0;
  }
  std::string ToString() {
    std::stringstream ss;
    ss << "  length_:" << ntoh16(length_);
    ss << "  seq_no_:" << ntoh32(seq_no_);
    ss << "  checksum_:" << GetChecksumString();
    std::string str = ss.str();
    return str;
  }
  std::string GetChecksumString() {
    char buffer[48];
    buffer[47] = '\0';
    uint8_t* ch = reinterpret_cast<uint8_t*>(&checksum_);
    for (int i = 0; i < 15; i++) {
      sprintf(buffer + i * 3, "%02x:", ch[i]);
    }
    sprintf(buffer + 45, "%02x", ch[15]);
    std::string str(buffer);
    return str;
  }
};
#pragma pack(pop)

#pragma pack(push, 1)
struct MessageHeader {
  int16_t trans_code_;
  int32_t logtime_;
  char alphachar_[2];
  int32_t m_trader_id_;
  int16_t error_code_;
  uint64_t ts_;
  uint64_t ts1_;
  uint64_t ts2_;
  int16_t msg_len_;

  MessageHeader() {
    trans_code_ = 0;  // fill trans code
    logtime_ = 0;
    memset(alphachar_, ' ', 2);
    m_trader_id_ = 0;  // fill traderId
    error_code_ = 0;
    ts_ = 0;
    ts1_ = 0;
    ts2_ = 0;
    msg_len_ = 0;  // fill msgLeg = sizeof(Struct)
  }

  void SetTransCode(int16_t trans_code) { trans_code_ = hton16(trans_code); }
  void SetTraderId(int32_t trader_id) { m_trader_id_ = hton32(trader_id); }
  void SetMsgLen(int16_t msg_len) { msg_len_ = hton16(msg_len); }
  std::string ToString() {
    std::stringstream ss;
    ss << "  trans_code_:" << ntoh16(trans_code_);
    ss << "  traderId:" << ntoh32(m_trader_id_);
    ss << "  error_code_:" << ntoh16(error_code_);
    ss << "  msg_len_:" << ntoh16((int16_t)msg_len_);
    std::string str = ss.str();
    return str;
  }
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ContractDescTr {
  char instrument_type_[6];
  char symbol_[10];
  int32_t expiry_date_;
  int32_t strike_price_;
  char option_type_[2];

  ContractDescTr() {
    memset(instrument_type_, ' ', 6);
    memset(symbol_, ' ', 10);
    expiry_date_ = 0;
    strike_price_ = 0;
    memset(option_type_, ' ', 2);
  }
  std::string ToString() {
    std::stringstream ss;
    ss << "InstrumentType:";
    for (int i = 0; i < 6; i++) {
      ss << instrument_type_[i];
    }
    ss << ",";
    ss << "Symbol:";
    for (int i = 0; i < 10; i++) {
      ss << symbol_[i];
    }
    ss << ",";
    ss << "ExpiryDate:" << ntoh32(expiry_date_) << ",";
    ss << "StrikePrice:" << (int)ntoh32(strike_price_) << ",";
    ss << "OptionType:";
    for (int i = 0; i < 2; i++) {
      ss << option_type_[i];
    }

    std::string str = ss.str();
    return str;
  }
};
#pragma pack(pop)

#pragma pack(push, 1)
union StOrderFlags {
  struct {
    uint16_t AON : 1;
    uint16_t IOC : 1;
    uint16_t GTC : 1;
    uint16_t DAY : 1;
    uint16_t MIT : 1;
    uint16_t SL : 1;
    uint16_t MARKET : 1;
    uint16_t ATO : 1;
    uint16_t Reserved : 3;
    uint16_t FROZEN : 1;
    uint16_t MODIFIED : 1;
    uint16_t TRADED : 1;
    uint16_t MATCHEDIND : 1;
    uint16_t MF : 1;
  };
  uint16_t short_notation_;

  StOrderFlags() { short_notation_ = 0; }
  inline void SetIOCFlags() {
    DAY = 0;
    IOC = 1;
  }
  inline void SetDayFlag() { DAY = 1; }
  inline void SetModifyFlag() {
    DAY = 1;
    MODIFIED = 1;
  }
};
#pragma pack(pop)

}  // namespace NSE
}  // namespace HFSAT
