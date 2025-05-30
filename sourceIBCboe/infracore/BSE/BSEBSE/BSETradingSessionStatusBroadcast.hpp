// =====================================================================================
//
//       Filename:  BSETradingSessionStatusBroadcast.hpp
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

class BSETradingSessionStatusBroadcast {
 private:
  TradingSessionStatusBroadcastT bse_trading_session_status_;

 public:
   BSETradingSessionStatusBroadcast() { memset((void*)(&bse_trading_session_status_), 0, sizeof(TradingSessionStatusBroadcastT)); }
  ~BSETradingSessionStatusBroadcast() {}

  TradingSessionStatusBroadcastT *ProcessSessionStatus(char const *msg_ptr) {
    bse_trading_session_status_.MessageHeaderOut.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_trading_session_status_.MessageHeaderOut.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_trading_session_status_.RBCHeaderME.TrdRegTSTimeOut = *((uint64_t *)(msg_ptr + 8));
    bse_trading_session_status_.RBCHeaderME.SendingTime = *((uint64_t *)(msg_ptr + 16));
    bse_trading_session_status_.RBCHeaderME.ApplSubID = *((uint32_t *)(msg_ptr + 24));
    bse_trading_session_status_.RBCHeaderME.PartitionID = *((uint16_t *)(msg_ptr + 28));
    memcpy((void*)bse_trading_session_status_.RBCHeaderME.ApplMsgID,
           (void*)(msg_ptr + 30),LEN_APPL_MSGID);
    bse_trading_session_status_.RBCHeaderME.ApplID = *((uint8_t *)(msg_ptr + 46));
    bse_trading_session_status_.RBCHeaderME.ApplResendFlag = *((uint8_t *)(msg_ptr + 47));
    bse_trading_session_status_.RBCHeaderME.LastFragment = *((uint8_t *)(msg_ptr + 48));
    bse_trading_session_status_.MarketSegmentID = *((int32_t *)(msg_ptr + 56)); 
    bse_trading_session_status_.TradeDate = *((uint32_t *)(msg_ptr + 60)); 
    bse_trading_session_status_.TradSesEvent = *((uint8_t *)(msg_ptr + 64));
    memcpy((void*)bse_trading_session_status_.RefApplLastMsgID,
           (void*)(msg_ptr + 65),LEN_REF_APPL_LAST_MSGID);

    return &bse_trading_session_status_;
  }
};
}
}

