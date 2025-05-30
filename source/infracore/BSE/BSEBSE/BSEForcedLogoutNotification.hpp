// =====================================================================================
//
//       Filename:  BSEForcedLogoutNotification.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/04/2012 12:55:20 PM
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

class BSEForcedLogoutNotification {
 private:
  ForcedLogoutNotificationT bse_forced_logout_;

 public:
   BSEForcedLogoutNotification() { memset((void*)(&bse_forced_logout_), 0, sizeof(ForcedLogoutNotificationT)); }
  ~BSEForcedLogoutNotification() {}

  ForcedLogoutNotificationT *ProcessForceLogout(char const *msg_ptr) {
    bse_forced_logout_.MessageHeaderOut.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_forced_logout_.MessageHeaderOut.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_forced_logout_.NotifHeader.SendingTime = *((uint64_t *)(msg_ptr + 8));
    bse_forced_logout_.VarTextLen = *((int16_t *)(msg_ptr + 16)); 
    memcpy((void*)bse_forced_logout_.VarText,
           (void*)(msg_ptr + 24),LEN_VAR_TEXT);

    return &bse_forced_logout_;
  }
};
}
}

