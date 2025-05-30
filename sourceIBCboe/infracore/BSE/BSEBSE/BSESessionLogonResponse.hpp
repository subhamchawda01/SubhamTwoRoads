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

class BSESessionLogonResponse {
 private:
  LogonResponseT bse_session_logon_response_;

 public:
   BSESessionLogonResponse() { memset((void*)(&bse_session_logon_response_), 0, sizeof(LogonResponseT)); }
  ~BSESessionLogonResponse() {}

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

  LogonResponseT *ProcessLogonResponse(char const *msg_ptr) {
    bse_session_logon_response_.MessageHeaderOut.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_session_logon_response_.MessageHeaderOut.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_session_logon_response_.ResponseHeader.RequestTime = *((uint64_t *)(msg_ptr + 8));
    bse_session_logon_response_.ResponseHeader.SendingTime = *((uint64_t *)(msg_ptr + 16));
    bse_session_logon_response_.ResponseHeader.MsgSeqNum = *((uint32_t *)(msg_ptr + 24));
    bse_session_logon_response_.ThrottleTimeInterval = *((int64_t *)(msg_ptr + 32));
    bse_session_logon_response_.LastLoginTime = *((uint64_t *)(msg_ptr + 40));
    bse_session_logon_response_.LastLoginIP = *((uint32_t *)(msg_ptr + 48));
    bse_session_logon_response_.ThrottleNoMsgs = *((uint32_t *)(msg_ptr + 52));
    bse_session_logon_response_.ThrottleDisconnectLimit = *((uint32_t *)(msg_ptr + 56));
    bse_session_logon_response_.HeartBtInt = *((uint32_t *)(msg_ptr + 60));
    bse_session_logon_response_.SessionInstanceID = *((uint32_t *)(msg_ptr + 64));
    bse_session_logon_response_.TradSesMode = *((uint8_t *)(msg_ptr + 68));
    bse_session_logon_response_.NoOfPartition = *((uint8_t *)(msg_ptr + 69)); 
    bse_session_logon_response_.DaysLeftForPasswdExpiry = *((uint8_t *)(msg_ptr + 70));
    bse_session_logon_response_.GraceLoginsLeft = *((uint8_t *)(msg_ptr + 71));
    memcpy((void*)bse_session_logon_response_.DefaultCstmApplVerID,
           (void*)(msg_ptr + 72),LEN_DEFAULT_CSTM_APPL_VERID);
/*
   std::cout << "DIFF Values: " << std::endl;
   for(int i=64;i<72; i++){
     std::ostringstream oss;
     oss.str("");
     oss.clear();
     oss << ((unsigned int)(*(uint8_t *)(msg_ptr + i))) ;
     uint32_t a = std::stoi(oss.str()); 
     char b = oss.str()[0];
     std::cout << i << ":" << *((uint8_t *)(msg_ptr + i)) 
   		    << "::" << ((unsigned int)(*(uint8_t *)(msg_ptr + i))) << ":" << oss.str() << ":" << a << ":" << b
  		  //  << ":" << std::atoi((msg_ptr + i)[0]) << ":" << (uint8_t)(std::atoi((msg_ptr + i)[0]))
		    << " " ;
   }
   std::cout << std::endl;

    printHexString(msg_ptr, sizeof(LogonResponseT));
    printf(" %02x", *(unsigned char *)(msg_ptr + 70));
    printf("\n");
*/
    return &bse_session_logon_response_;
  }
};
}
}
