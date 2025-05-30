// =====================================================================================
//
//       Filename:  RequestPacket.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/28/2015 05:05:25 PM
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

#include "infracore/NSET/NSETemplates/DataDefines.hpp"

//==============================================  NSE PACKET REQUEST START====================================//

#define NSE_PACKET_LENGTH_OFFSET NSE_REQUEST_START_OFFSET
#define NSE_PACKET_LENGTH_LENGTH sizeof(int16_t)

#define NSE_PACKET_SEQUENCE_OFFSET (NSE_PACKET_LENGTH_OFFSET + NSE_PACKET_LENGTH_LENGTH)
#define NSE_PACKET_SEQUENCE_LENGTH sizeof(int32_t)

//#define NSE_PACKET_RESERVED_SEQUENCE_NUMBER_OFFSET (NSE_PACKET_SEQUENCE_OFFSET + NSE_PACKET_SEQUENCE_LENGTH)
//#define NSE_PACKET_RESERVED_SEQUENCE_NUMBER_LENGTH sizeof(int16_t)

#define NSE_PACKET_CHECKSUM_OFFSET (NSE_PACKET_SEQUENCE_OFFSET + NSE_PACKET_SEQUENCE_LENGTH)
#define NSE_PACKET_CHECKSUM_LENGTH PACKET_REQUEST_MD5SUM_SIZE

//#define NSE_PACKET_MESSAGE_COUNT_OFFSET (NSE_PACKET_CHECKSUM_OFFSET + NSE_PACKET_CHECKSUM_LENGTH)
//#define NSE_PACKET_MESSAGE_COUNT_LENGTH sizeof(uint16_t)

#define NSE_PACKET_REQUEST_LENGTH (NSE_PACKET_LENGTH_LENGTH + NSE_PACKET_SEQUENCE_LENGTH + NSE_PACKET_CHECKSUM_LENGTH)

template <typename T>
void swap_endian_t(T& pX) {
  // should static assert that T is a POD

  char& raw = reinterpret_cast<char&>(pX);
  std::reverse(&raw, &raw + sizeof(T));
}

template <typename T>
T swap_endian(T pX) {
  swap_endian_t(pX);
  return pX;
}
