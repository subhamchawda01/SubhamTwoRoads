// =====================================================================================
//
//       Filename:  MessageHeader.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Wednesday 24 June 2015 07:51:39  GMT
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#define ntoh16 __builtin_bswap16
#define ntoh32 __builtin_bswap32
#define ntoh64 __builtin_bswap64
#define hton16 htons

#include <iostream>

namespace HFSAT {
namespace NSE {

void printHexString(const char* c, int len) {
  for (int i = 0; i < len; ++i) {
    uint8_t ch = c[i];
    printf("%02x ", ch);
  }
  printf("\n");
}

#pragma pack(push, 1)

struct MESSAGE_HEADER {
  int16_t TransactionCode;
  int32_t LogTime;
  char AlphaChar[2];
  int32_t TraderId;
  int16_t ErrorCode;
  char Timestamp[8];
  char TimeStamp1[8];
  char TimeStamp2[8];
  int16_t MessageLength;

  std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << " Transcation Code : " << (int32_t)ntoh16(TransactionCode) << "\n";
    t_temp_oss << " LogTime : " << (int32_t)ntoh32(LogTime) << "\n";
    t_temp_oss << " TraderID : " << (int32_t)ntoh32(TraderId) << "\n";
    t_temp_oss << " ErrorCode : " << (int16_t)ntoh16(ErrorCode) << "\n";

    return t_temp_oss.str();
  }
};

struct ST_BROKER_ELIGIBILITY_PER_MKT {
  uint8_t broker_eligibility_per_mkt_bitmask;
  char pad;
};

struct SYSTEM_INFORMATION_IN {
  MESSAGE_HEADER MessageHeader;
  int32_t LastUpdatePortfolioTime;
};

struct MS_SIGNON {
  MESSAGE_HEADER MessageHeader;

  int32_t UserId;
  char Password[8];
  char NewPassword[8];
  char TraderName[26];
  int32_t LastPasswordChangeDate;
  char BrokerId[5];
  char pad_1;
  int16_t BranchId;
  int32_t VersionNumber;
  int32_t Batch2StartTime;
  char HostSwitchContext;
  char Colour[50];
  char pad_2;
  int16_t UserType;
  double SequenceNumber;
  char WsClassName[14];
  char BrokerStatus;
  char ShowIndex;
  ST_BROKER_ELIGIBILITY_PER_MKT st_broker_eligibility_per_mkt_;
  int16_t MemberType;
  char ClearingStatus;
  char BrokerName[25];
};

struct OnlyPacket {
  int16_t Length;
  int32_t Sequencenumber;
  int16_t ResrvSequencenumber;
  char Checksum[16];
  int16_t MessageCount;
  MESSAGE_HEADER MessageHeader;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << MessageHeader.ToString();

    return t_temp_oss.str();
  }
};

struct Packet {
  int16_t Length;
  int32_t Sequencenumber;
  int16_t ResrvSequencenumber;
  char Checksum[16];
  int16_t MessageCount;

  MS_SIGNON login;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << login.MessageHeader.ToString();

    return t_temp_oss.str();
  }
};

struct NSETAPInvitationStruct {
  MESSAGE_HEADER MessageHeader;
  int16_t InvitationCount;
};

#pragma pack(pop)
}
}
