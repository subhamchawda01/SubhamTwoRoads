// =====================================================================================
//
//       Filename:  BSEUserLogonResponse.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/04/2012 11:24:47 AM
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

#include "infracore/BSE/BSEBSE/BSEMessageDefs.hpp"

namespace HFSAT {
namespace BSE {

class BSESessionRegistrationResponse {
 private:
  SessionRegistrationResponseT bse_session_registration_response_;

 public:
   BSESessionRegistrationResponse() { memset((void*)(&bse_session_registration_response_), 0, sizeof(SessionRegistrationResponseT)); }
  ~BSESessionRegistrationResponse() {}

void printHexString(const char *c, int len) {
  int i;
  unsigned char buff[17];
  unsigned char *pc = (unsigned char *)c;

  if (len == 0) {
    printf("  ZERO LENGTH\n");
    return;
  }
  if (len < 0) {
    printf("  NEGATIVE LENGTH: %i\n", len);
    return;
  }

  for (i = 0; i < len; i++) {

    if ((i % 16) == 0) {
      if (i != 0) printf("  %s\n", buff);

      printf("  %04x ", i);
    }

    printf(" %02x", pc[i]);

    if ((pc[i] < 0x20) || (pc[i] > 0x7e))
      buff[i % 16] = '.';
    else
      buff[i % 16] = pc[i];
    buff[(i % 16) + 1] = '\0';

  }

  while ((i % 16) != 0) {
    printf("   ");
    i++;
  }

  printf("  %s\n", buff);

  printf("\n");
  fflush(stdout);
}

  SessionRegistrationResponseT *ProcessSessionRegistrationResponse(char const *msg_ptr) {
    bse_session_registration_response_.MessageHeaderOut.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_session_registration_response_.MessageHeaderOut.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_session_registration_response_.ResponseHeader.RequestTime = *((uint64_t *)(msg_ptr + 8));
    bse_session_registration_response_.ResponseHeader.SendingTime = *((uint64_t *)(msg_ptr + 16));
    bse_session_registration_response_.ResponseHeader.MsgSeqNum = *((uint32_t *)(msg_ptr + 24));
    bse_session_registration_response_.status = *(msg_ptr + 32) == 'Y' ? true : false;

    return &bse_session_registration_response_;
  }
};
}
}
