// =====================================================================================
//
//       Filename:  BSEOrderExecNotification.hpp
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

class BSEOrderExecNotification {
 private:
  OrderExecNotificationT bse_order_exec_notification_;

 public:
   BSEOrderExecNotification() { memset((void*)(&bse_order_exec_notification_), 0, sizeof(OrderExecNotificationT)); }
  ~BSEOrderExecNotification() {}

  OrderExecNotificationT *ProcessOrderExecNotification(char const *msg_ptr) {

    bse_order_exec_notification_.MessageHeaderOut.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_order_exec_notification_.MessageHeaderOut.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_order_exec_notification_.RBCHeaderME.TrdRegTSTimeOut = *((uint64_t *)(msg_ptr + 8));
    bse_order_exec_notification_.RBCHeaderME.SendingTime = *((uint64_t *)(msg_ptr + 16));
    bse_order_exec_notification_.RBCHeaderME.ApplSubID = *((uint32_t *)(msg_ptr + 24));
    bse_order_exec_notification_.RBCHeaderME.PartitionID = *((uint16_t *)(msg_ptr + 28));
    memcpy((void*)(bse_order_exec_notification_.RBCHeaderME.ApplMsgID),
           (void*)(msg_ptr + 30),LEN_APPL_MSGID);
    bse_order_exec_notification_.RBCHeaderME.ApplID = *((uint8_t *)(msg_ptr + 46));
    bse_order_exec_notification_.RBCHeaderME.ApplResendFlag = *((uint8_t *)(msg_ptr + 47));
    bse_order_exec_notification_.RBCHeaderME.LastFragment = *((uint8_t *)(msg_ptr + 48));
    bse_order_exec_notification_.OrderID = *((uint64_t *)(msg_ptr + 56));
    bse_order_exec_notification_.SenderLocationID = *((uint64_t *)(msg_ptr + 64));
    bse_order_exec_notification_.ClOrdID = *((uint64_t *)(msg_ptr + 72));
    bse_order_exec_notification_.OrigClOrdID = *((uint64_t *)(msg_ptr + 80));
    bse_order_exec_notification_.SecurityID = *((int64_t *)(msg_ptr + 88));
    bse_order_exec_notification_.ExecID = *((uint64_t *)(msg_ptr + 96));
    bse_order_exec_notification_.ActivityTime = *((uint64_t *)(msg_ptr + 104));
    bse_order_exec_notification_.MessageTag = *((uint64_t *)(msg_ptr + 124));
    bse_order_exec_notification_.MarketSegmentID = *((int32_t *)(msg_ptr + 128));
    bse_order_exec_notification_.LeavesQty = *((int32_t *)(msg_ptr + 132));
    bse_order_exec_notification_.CumQty = *((int32_t *)(msg_ptr + 136));
    bse_order_exec_notification_.CxlQty = *((int32_t *)(msg_ptr + 140));
    bse_order_exec_notification_.NoLegExecs = *((uint16_t *)(msg_ptr + 144));
    bse_order_exec_notification_.ExecRestatementReason = *((uint16_t *)(msg_ptr + 148));
    bse_order_exec_notification_.AccountType = *((uint8_t *)(msg_ptr + 150));
    bse_order_exec_notification_.ProductComplex = *((uint8_t *)(msg_ptr + 151));
    memcpy((void*)bse_order_exec_notification_.OrdStatus,
           (void*)(msg_ptr + 152),LEN_ORD_STATUS);
    memcpy((void*)bse_order_exec_notification_.ExecType,
           (void*)(msg_ptr + 153),LEN_EXEC_TYPE);
    bse_order_exec_notification_.Triggered = *((uint8_t *)(msg_ptr + 154));
    bse_order_exec_notification_.NoFills = *((uint8_t *)(msg_ptr + 155));
    bse_order_exec_notification_.Side = *((uint8_t *)(msg_ptr + 156));
    memcpy((void*)bse_order_exec_notification_.Account,
           (void*)(msg_ptr + 158),LEN_ACCOUNT);
    memcpy((void*)bse_order_exec_notification_.AlgoID,
           (void*)(msg_ptr + 160),LEN_ALGOID);
    memcpy((void*)bse_order_exec_notification_.FreeText1,
           (void*)(msg_ptr + 176),LEN_FREE_TEXT1);
    memcpy((void*)bse_order_exec_notification_.CPCode,
           (void*)(msg_ptr + 188),LEN_CP_CODE);
    memcpy((void*)bse_order_exec_notification_.FreeText3,
           (void*)(msg_ptr + 200),LEN_FREE_TEXT3);
    bse_order_exec_notification_.FillsGrp[0].FillPx = *((int64_t *)(msg_ptr + 216));

    return &bse_order_exec_notification_;
  }
};
}
}
