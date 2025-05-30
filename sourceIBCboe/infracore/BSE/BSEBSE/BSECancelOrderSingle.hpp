// =====================================================================================
//
//       Filename:  BSECancelOrderSingle.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/05/2012 09:20:43 AM
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

class BSECancelOrderSingle {
 public:
  DeleteOrderSingleRequestT bse_cancel_order_single_request_;

 public:
    // initialize cancel order single message
  BSECancelOrderSingle() {
    memset((void*)(&bse_cancel_order_single_request_), 0, sizeof(DeleteOrderSingleRequestT));

    // fill up the MessageHeader
    bse_cancel_order_single_request_.MessageHeaderIn.BodyLen = sizeof(DeleteOrderSingleRequestT);
    bse_cancel_order_single_request_.MessageHeaderIn.TemplateID = TID_DELETE_ORDER_SINGLE_REQUEST;

    bse_cancel_order_single_request_.MessageTag = 0;
  }

  void setBSECancelOrderSingleStaticFields(const uint32_t& _session_id_, const uint32_t& _sender_sub_id_,
                                           const char* _algo_id_) {
    bse_cancel_order_single_request_.TargetPartyIDSessionID = _session_id_;
    bse_cancel_order_single_request_.RequestHeader.SenderSubID = _sender_sub_id_;
    memcpy((void*)(bse_cancel_order_single_request_.AlgoID), (void*)(_algo_id_) , LEN_ALGOID);
  }

  void setBSECancelOrderSingleSessionID(const uint32_t& _session_id_) {
    bse_cancel_order_single_request_.TargetPartyIDSessionID = _session_id_;
  }

  void setBSECancelOrderSingleSenderSubID(const uint32_t& _sender_sub_id_) {
    bse_cancel_order_single_request_.RequestHeader.SenderSubID = _sender_sub_id_;
  }


  void setBSECancelOrderSingleMessageSequence(const uint32_t& _message_sequence_) {
    bse_cancel_order_single_request_.RequestHeader.MsgSeqNum = _message_sequence_;
  }

  void setBSECancelOrderSingleDynamicFields(HFSAT::ORS::Order *rp_order_, const uint32_t& _message_sequence_,
                                            const int32_t& _market_segment_id_, const uint32_t& _security_id_) {
    bse_cancel_order_single_request_.RequestHeader.MsgSeqNum = _message_sequence_;
    bse_cancel_order_single_request_.OrderID = rp_order_->exch_assigned_seq_;
    bse_cancel_order_single_request_.ClOrdID = rp_order_->server_assigned_order_sequence_;
    bse_cancel_order_single_request_.OrigClOrdID = rp_order_->server_assigned_order_sequence_;
    bse_cancel_order_single_request_.ActivityTime = rp_order_->last_activity_reference_;
    bse_cancel_order_single_request_.MarketSegmentID = _market_segment_id_;
    bse_cancel_order_single_request_.SimpleSecurityID = _security_id_;
  }

  int getBSECancelOrderSingleMsgLength() { return (sizeof(DeleteOrderSingleRequestT)); }

  DeleteOrderSingleRequestT *ProcessCancelOrderSingleShortRequest(char const *msg_ptr) {
    bse_cancel_order_single_request_.MessageHeaderIn.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_cancel_order_single_request_.MessageHeaderIn.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_cancel_order_single_request_.RequestHeader.MsgSeqNum = *((uint32_t *)(msg_ptr + 16));
    bse_cancel_order_single_request_.RequestHeader.SenderSubID = *((uint32_t *)(msg_ptr + 20));
    bse_cancel_order_single_request_.OrderID = *((uint64_t *)(msg_ptr + 24));
    bse_cancel_order_single_request_.ClOrdID = *((uint64_t *)(msg_ptr + 32));
    bse_cancel_order_single_request_.OrigClOrdID = *((uint64_t *)(msg_ptr + 40));
    bse_cancel_order_single_request_.ActivityTime = *((uint32_t *)(msg_ptr + 48));
    bse_cancel_order_single_request_.MessageTag = *((int32_t *)(msg_ptr + 56));
    bse_cancel_order_single_request_.MarketSegmentID = *((int32_t *)(msg_ptr + 60));
    bse_cancel_order_single_request_.SimpleSecurityID = *((uint32_t *)(msg_ptr + 64));
    bse_cancel_order_single_request_.TargetPartyIDSessionID = *((uint32_t *)(msg_ptr + 68));
    memcpy((void*)bse_cancel_order_single_request_.AlgoID,	
           (void*)(msg_ptr + 76),LEN_ALGOID);

    return &bse_cancel_order_single_request_;
  }

};
}
}
