// =====================================================================================
//
//       Filename:  nse_tap_invitation_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/21/2015 09:16:00 AM
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

namespace HFSAT {
namespace NSE {

class NSETAPInvitation {
 private:
  volatile std::atomic<bool> is_tap_invitation_message_ready_;
  HFSAT::Lock invitation_lock_;

  NSETAPInvitation() {}
  NSETAPInvitation(NSETAPInvitation const& disabled_copy_constructor);

 public:
  static NSETAPInvitation& GetUniqueInstance() {
    static NSETAPInvitation unique_instance;
    return unique_instance;
  }

  bool IsInvitationAvailable() {
    bool is_ready = false;

    invitation_lock_.LockMutex();
    is_ready = is_tap_invitation_message_ready_;
    invitation_lock_.UnlockMutex();

    return is_ready;
  }

  void UpdateInvitationValue(bool invitation_value) {
    invitation_lock_.LockMutex();
    is_tap_invitation_message_ready_ = invitation_value;
    invitation_lock_.UnlockMutex();
  }
};
}
}
