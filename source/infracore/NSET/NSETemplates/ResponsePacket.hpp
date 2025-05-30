// =====================================================================================
//
//       Filename:  ResponsePacket.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/30/2015 07:24:44 AM
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

#include "infracore/NSET/NSETemplates/RequestHeader.hpp"
#include "infracore/NSET/NSETemplates/RequestPacket.hpp"

//===========================  NSE PACKET RESPONSE START ===================================================//

#define NSE_PACKET_RESPONSE_LENGTH (NSE_PACKET_LENGTH_LENGTH + NSE_PACKET_SEQUENCE_LENGTH + NSE_PACKET_CHECKSUM_LENGTH)

namespace HFSAT {
namespace NSE {

class NSEPacketResponse {
 private:
  ProcessedPacketHeader processed_packet_header_;

 public:
  NSEPacketResponse() { memset((void *)&processed_packet_header_, 0, sizeof(ProcessedPacketHeader)); }

  inline ProcessedPacketHeader *ProcessPakcet(char const *msg_ptr) {
    processed_packet_header_.packet_length = ntoh16(*((int16_t *)(msg_ptr + NSE_PACKET_LENGTH_OFFSET)));
    processed_packet_header_.packet_sequnece_number = ntoh32(*((int32_t *)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)));

    return &processed_packet_header_;
  }
};
}
}
