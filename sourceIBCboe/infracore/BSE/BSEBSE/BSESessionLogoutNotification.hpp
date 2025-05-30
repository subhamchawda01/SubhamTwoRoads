// =====================================================================================
//
//       Filename:  BSESessionLogoutNotification.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/04/2012 12:36:04 PM
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

class BSESessionLogoutNotification {
 private:
  ForcedLogoutNotificationT bse_force_logout_;

 public:
   BSESessionLogoutNotification() { memset((void*)(&bse_force_logout_), 0, sizeof(ForcedLogoutNotificationT)); }
  ~BSESessionLogoutNotification() {}

  ForcedLogoutNotificationT *ProcessForceLogout(char const *msg_ptr) {
    bse_force_logout_.MessageHeaderOut.BodyLen = (uint32_t *)(msg_ptr + 0);
    bse_force_logout_.MessageHeaderOut.TemplateID = (uint16_t *)(msg_ptr + 4);
    bse_force_logout_.NotifHeader.SendingTime = (uint64_t *)(msg_ptr + 8);
    bse_force_logout_.VarTextLen = (uint16_t *)(msg_ptr + 16);
    memcpy((void*)(bse_force_logout_.VarText, (void*)(msg_ptr + 24), LEN_VAR_TEXT);

    return &bse_force_logout_;
  }
};
}
}
