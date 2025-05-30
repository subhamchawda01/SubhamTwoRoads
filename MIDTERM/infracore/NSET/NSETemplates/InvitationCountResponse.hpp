// =====================================================================================
//
//       Filename:  InvitationCountResponse.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/21/2015 05:30:44 AM
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

#include "infracore/NSET/NSETemplates/RequestPacket.hpp"
#include "infracore/NSET/NSETemplates/RequestHeader.hpp"

#define NSE_TAP_INVITATION_COUNT_OFFSET (NSE_REQUEST_START_OFFSET)
#define NSE_TAP_INVITATION_COUNT_LENGTH sizeof(int16_t)

namespace HFSAT {
namespace NSE {

class InvitationCountResponse {
 private:
};
}
}
