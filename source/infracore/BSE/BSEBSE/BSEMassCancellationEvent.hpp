// =====================================================================================
//
//       Filename:  BSEMassCancellationEvent.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/04/2012 12:12:14 PM
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

class BSEMassCancellationEvent {
 private:
  DeleteAllOrderQuoteEventBroadcastT bse_mass_cancellation_event_;

 public:
  BSEMassCancellationEvent() { memset((void*)(&bse_mass_cancellation_event_), 0, sizeof(DeleteAllOrderQuoteEventBroadcastT)); }
  ~BSEMassCancellationEvent() {}

  DeleteAllOrderQuoteEventBroadcastT *ProcessMassCancellationEvent(char const *msg_ptr) {
    bse_mass_cancellation_event_.MessageHeaderOut.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_mass_cancellation_event_.MessageHeaderOut.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_mass_cancellation_event_.RBCHeaderME.TrdRegTSTimeOut = *((uint64_t *)(msg_ptr + 8));
    bse_mass_cancellation_event_.RBCHeaderME.SendingTime = *((uint64_t *)(msg_ptr + 16));
    bse_mass_cancellation_event_.RBCHeaderME.ApplSubID = *((uint32_t *)(msg_ptr + 24));
    bse_mass_cancellation_event_.RBCHeaderME.PartitionID = *((uint16_t *)(msg_ptr + 28));
    memcpy((void*)bse_mass_cancellation_event_.RBCHeaderME.ApplMsgID,
           (void*)(msg_ptr + 30),LEN_APPL_MSGID);
    bse_mass_cancellation_event_.RBCHeaderME.ApplID = *((uint8_t *)(msg_ptr + 46));
    bse_mass_cancellation_event_.RBCHeaderME.ApplResendFlag = *((uint8_t *)(msg_ptr + 47));
    bse_mass_cancellation_event_.RBCHeaderME.LastFragment = *((uint8_t *)(msg_ptr + 48));
    bse_mass_cancellation_event_.MassActionReportID = *((uint64_t *)(msg_ptr + 56));
    bse_mass_cancellation_event_.SecurityID = *((int64_t *)(msg_ptr + 64));
    bse_mass_cancellation_event_.MarketSegmentID = *((int32_t *)(msg_ptr + 72));
    bse_mass_cancellation_event_.MassActionReason = *((uint8_t *)(msg_ptr + 76));
    bse_mass_cancellation_event_.ExecInst = *((uint8_t *)(msg_ptr + 77));

    return &bse_mass_cancellation_event_;
  }

};
}
}
