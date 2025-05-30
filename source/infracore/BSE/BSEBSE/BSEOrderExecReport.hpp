// =====================================================================================
//
//       Filename:  BSEOrderExecReport.hpp
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

class BSEOrderExecReport {
 private:
  OrderExecReportBroadcastT bse_order_exec_report_;

 public:
  BSEOrderExecReport() { memset((void*)(&bse_order_exec_report_), 0, sizeof(OrderExecReportBroadcastT)); }
  ~BSEOrderExecReport() {}

  OrderExecReportBroadcastT *ProcessOrderExecReport(char const *msg_ptr) {
    bse_order_exec_report_.MessageHeaderOut.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_order_exec_report_.MessageHeaderOut.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_order_exec_report_.RBCHeaderME.TrdRegTSTimeOut = *((uint64_t *)(msg_ptr + 8));
    bse_order_exec_report_.RBCHeaderME.SendingTime = *((uint64_t *)(msg_ptr + 16));
    bse_order_exec_report_.RBCHeaderME.ApplSubID = *((uint32_t *)(msg_ptr + 24));
    bse_order_exec_report_.RBCHeaderME.PartitionID = *((uint16_t *)(msg_ptr + 28));
    memcpy((void*)bse_order_exec_report_.RBCHeaderME.ApplMsgID,
           (void*)(msg_ptr + 30),LEN_APPL_MSGID);
    bse_order_exec_report_.RBCHeaderME.ApplID = *((uint8_t *)(msg_ptr + 46));
    bse_order_exec_report_.RBCHeaderME.ApplResendFlag = *((uint8_t *)(msg_ptr + 47));
    bse_order_exec_report_.RBCHeaderME.LastFragment = *((uint8_t *)(msg_ptr + 48));
    bse_order_exec_report_.OrderID = *((uint64_t *)(msg_ptr + 56));
    bse_order_exec_report_.ClOrdID = *((uint64_t *)(msg_ptr + 64));
    bse_order_exec_report_.OrigClOrdID = *((uint64_t *)(msg_ptr + 72));
    bse_order_exec_report_.SecurityID = *((int64_t *)(msg_ptr + 80));
    bse_order_exec_report_.MaxPricePercentage = *((int64_t *)(msg_ptr + 88));
    bse_order_exec_report_.SenderLocationID = *((uint64_t *)(msg_ptr + 96));
    bse_order_exec_report_.ExecID = *((uint64_t *)(msg_ptr + 104));
    bse_order_exec_report_.TrdRegTSEntryTime = *((uint64_t *)(msg_ptr + 112));
    bse_order_exec_report_.TrdRegTSTimePriority = *((uint64_t *)(msg_ptr + 120));
    bse_order_exec_report_.Price = *((int64_t *)(msg_ptr + 128));
    bse_order_exec_report_.StopPx = *((int64_t *)(msg_ptr + 136));
    bse_order_exec_report_.UnderlyingDirtyPrice = *((int64_t *)(msg_ptr + 144));
    bse_order_exec_report_.Yield = *((int64_t *)(msg_ptr + 152));
    bse_order_exec_report_.ActivityTime = *((uint64_t *)(msg_ptr + 160));
    bse_order_exec_report_.MarketSegmentID = *((int32_t *)(msg_ptr + 180));
    bse_order_exec_report_.MessageTag = *((int32_t *)(msg_ptr + 184));
    bse_order_exec_report_.LeavesQty = *((int32_t *)(msg_ptr + 188));
    bse_order_exec_report_.MaxShow = *((int32_t *)(msg_ptr + 192));
    bse_order_exec_report_.CumQty = *((int32_t *)(msg_ptr + 196));
    bse_order_exec_report_.CxlQty = *((int32_t *)(msg_ptr + 200));
    bse_order_exec_report_.OrderQty = *((int32_t *)(msg_ptr + 204));
    bse_order_exec_report_.PartyIDExecutingUnit = *((uint32_t *)(msg_ptr + 212));
    bse_order_exec_report_.PartyIDSessionID = *((uint32_t *)(msg_ptr + 216));
    bse_order_exec_report_.PartyIDExecutingTrader = *((uint32_t *)(msg_ptr + 220));
    bse_order_exec_report_.PartyIDEnteringTrader = *((uint32_t *)(msg_ptr + 224));
    bse_order_exec_report_.NoLegExecs = *((uint16_t *)(msg_ptr + 230));
    bse_order_exec_report_.ExecRestatementReason = *((uint16_t *)(msg_ptr + 232));
    bse_order_exec_report_.AccountType = *((uint8_t *)(msg_ptr + 234));
    bse_order_exec_report_.PartyIDEnteringFirm = *((uint8_t *)(msg_ptr + 235));
    bse_order_exec_report_.ProductComplex = *((uint8_t *)(msg_ptr + 236));
    memcpy((void*)bse_order_exec_report_.OrdStatus,
           (void*)(msg_ptr + 237),LEN_ORD_STATUS);
    memcpy((void*)bse_order_exec_report_.ExecType,
           (void*)(msg_ptr + 238),LEN_EXEC_TYPE);
    bse_order_exec_report_.Side = *((uint8_t *)(msg_ptr + 239));
    bse_order_exec_report_.OrdType = *((uint8_t *)(msg_ptr + 240));
    bse_order_exec_report_.TimeInForce = *((uint8_t *)(msg_ptr + 242));
    bse_order_exec_report_.ExecInst = *((uint8_t *)(msg_ptr + 243));
    bse_order_exec_report_.ApplSeqIndicator = *((uint8_t *)(msg_ptr + 245));
    bse_order_exec_report_.STPCFlag = *((uint8_t *)(msg_ptr + 246));
    memcpy((void*)bse_order_exec_report_.PositionEffect,
           (void*)(msg_ptr + 250),LEN_POSITION_EFFECT);
    memcpy((void*)bse_order_exec_report_.RegulatoryText,
           (void*)(msg_ptr + 275),LEN_REGULATORY_TEXT);
    memcpy((void*)bse_order_exec_report_.AlgoID,
           (void*)(msg_ptr + 295),LEN_ALGOID);
    memcpy((void*)bse_order_exec_report_.FreeText1,
           (void*)(msg_ptr + 311),LEN_FREE_TEXT1);
    memcpy((void*)bse_order_exec_report_.CPCode,
           (void*)(msg_ptr + 323),LEN_CP_CODE);
    memcpy((void*)bse_order_exec_report_.FreeText3,
           (void*)(msg_ptr + 335),LEN_FREE_TEXT3);
    bse_order_exec_report_.NoFills = *((uint8_t *)(msg_ptr + 347));
    bse_order_exec_report_.NoLegs = *((uint8_t *)(msg_ptr + 348));
    bse_order_exec_report_.Triggered = *((uint8_t *)(msg_ptr + 349));
    
    return &bse_order_exec_report_;
  }

};
}
}
