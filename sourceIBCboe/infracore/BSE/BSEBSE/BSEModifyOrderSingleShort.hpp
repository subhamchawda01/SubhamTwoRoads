// =====================================================================================
//
//       Filename:  BSEModifyOrderSingleShort.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/05/2012 08:58:42 AM
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

class BSEModifyOrderSingleShort {
 public:
  ModifyOrderSingleShortRequestT bse_modify_order_single_short_request_;

 public:
    
  BSEModifyOrderSingleShort() {
    // initialize Modify order message  -- this is only intended to clear garbage value if any, BSE has novalue concept
    memset((void*)(&bse_modify_order_single_short_request_), 0, sizeof(ModifyOrderSingleShortRequestT));

    // fill up the MessageHeader
    bse_modify_order_single_short_request_.MessageHeaderIn.BodyLen = sizeof(ModifyOrderSingleShortRequestT);
    bse_modify_order_single_short_request_.MessageHeaderIn.TemplateID = TID_MODIFY_ORDER_SINGLE_SHORT_REQUEST;

    
    // TimeInForce - Options
    // 0 - GFD
    // 3 - IOC
    bse_modify_order_single_short_request_.TimeInForce = 7;  // GFD

    // AccountType - Options
    // 20 - Own
    // 30 - Client
    // 40 - Special Client
    // 90 - Institution
    bse_modify_order_single_short_request_.AccountType = 20;

    // All orders where the user desires to show the full LeavesQty in the market data, the
    // MaxShow field can be set as 0 is the request.
    bse_modify_order_single_short_request_.MaxShow = 0;

  }

  void setBSEModifyOrderSingleShortStaticFields(const uint32_t& _sender_sub_id_, const uint64_t& _sender_location_id_,
                                                const char* _algo_id_, const char* _client_code_, const char* _cp_code_) {
    bse_modify_order_single_short_request_.RequestHeader.SenderSubID = _sender_sub_id_;
    bse_modify_order_single_short_request_.SenderLocationID = _sender_location_id_;
    memcpy((void*)(bse_modify_order_single_short_request_.AlgoID), (void*)(_algo_id_), LEN_ALGOID);
    memcpy((void*)(bse_modify_order_single_short_request_.FreeText1), (void*)(_client_code_), LEN_FREE_TEXT1);
    memcpy((void*)(bse_modify_order_single_short_request_.CPCode), (void*)(_cp_code_), LEN_CP_CODE);
  }

  // set the session sequence number
  void setBSEModifyOrderSingleShortMessageSequnece(const uint32_t& _message_sequence_) {
    bse_modify_order_single_short_request_.RequestHeader.MsgSeqNum = _message_sequence_;
  }

  void setBSEModifyOrderSingleShortPrice(const double& _price_) {
    bse_modify_order_single_short_request_.Price = (int64_t)((_price_ * ETI_FIXED_DECIMAL_OFFSET_MULTIPLIER_FOR_PRICE) + 0.5);
  }

  void setBSEModifyOrderSingleShortClOrderId(const uint32_t& _cl_ord_id_) {
    bse_modify_order_single_short_request_.ClOrdID = _cl_ord_id_;
  }

  void setBSEModifyOrderSingleShortOrderQty(const int32_t& _order_qty_) {
    bse_modify_order_single_short_request_.OrderQty = _order_qty_;
  }

  void setBSEModifyOrderSingleShortOrderSide(const uint8_t& _side_) {
    bse_modify_order_single_short_request_.Side = (_side_ == HFSAT::kTradeTypeBuy) ? 1 : 2;
  }

  void setBSEModifyOrderSingleShortOrigClorID(const uint32_t& _orig_cl_ord_id_) {
    bse_modify_order_single_short_request_.OrigClOrdID = _orig_cl_ord_id_;
  }

  void setBSEModifyOrderSingleShortSenderSubID(const uint32_t& _sender_sub_id_) {
    // fill up the RequestHeeader
    bse_modify_order_single_short_request_.RequestHeader.SenderSubID = _sender_sub_id_;
  }

  // a single call should be enough to fill all the dynamic fields
  void setBSEModifyOrderSingleShortDynamicFields(HFSAT::ORS::Order* rp_order_,const uint32_t& _message_sequence_,
                                                 const uint32_t& _simple_security_id_) {

    bse_modify_order_single_short_request_.RequestHeader.MsgSeqNum = _message_sequence_;
    bse_modify_order_single_short_request_.OrderID = rp_order_->exch_assigned_seq_;
    bse_modify_order_single_short_request_.ClOrdID = rp_order_->server_assigned_order_sequence_;
    bse_modify_order_single_short_request_.OrigClOrdID = rp_order_->server_assigned_order_sequence_;
    bse_modify_order_single_short_request_.Price = (int64_t)((rp_order_->price_ * ETI_FIXED_DECIMAL_OFFSET_MULTIPLIER_FOR_PRICE) + 0.5);
    bse_modify_order_single_short_request_.ActivityTime = rp_order_->last_activity_reference_;
    bse_modify_order_single_short_request_.OrderQty = rp_order_->size_remaining_;
    bse_modify_order_single_short_request_.SimpleSecurityID = _simple_security_id_;
    bse_modify_order_single_short_request_.Side = (rp_order_->buysell_ == HFSAT::kTradeTypeBuy) ? 1 : 2;
  }

  // get MsgLength to write
  int getBSEModifyOrderSingleShortMsgLength() { return (sizeof(ModifyOrderSingleShortRequestT)); }

  ModifyOrderSingleShortRequestT *ProcessModifyOrderSingleShortRequest(char const *msg_ptr) {
    bse_modify_order_single_short_request_.MessageHeaderIn.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_modify_order_single_short_request_.MessageHeaderIn.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_modify_order_single_short_request_.RequestHeader.MsgSeqNum = *((uint32_t *)(msg_ptr + 16));
    bse_modify_order_single_short_request_.RequestHeader.SenderSubID = *((uint32_t *)(msg_ptr + 20));
    bse_modify_order_single_short_request_.OrderID = *((uint64_t *)(msg_ptr + 24));
    bse_modify_order_single_short_request_.ClOrdID = *((uint64_t *)(msg_ptr + 32));
    bse_modify_order_single_short_request_.OrigClOrdID = *((uint64_t *)(msg_ptr + 40));
    bse_modify_order_single_short_request_.Price = *((int64_t *)(msg_ptr + 48));
    bse_modify_order_single_short_request_.SenderLocationID = *((uint64_t *)(msg_ptr + 56));
    bse_modify_order_single_short_request_.ActivityTime = *((uint64_t *)(msg_ptr + 64));
    bse_modify_order_single_short_request_.OrderQty = *((int32_t *)(msg_ptr + 72));
    bse_modify_order_single_short_request_.MaxShow = *((int32_t *)(msg_ptr + 76));
    bse_modify_order_single_short_request_.SimpleSecurityID = *((uint32_t *)(msg_ptr + 80));
    bse_modify_order_single_short_request_.AccountType = *((uint8_t *)(msg_ptr + 90));
    bse_modify_order_single_short_request_.Side = *((uint8_t *)(msg_ptr + 91));
    bse_modify_order_single_short_request_.TimeInForce = *((uint8_t *)(msg_ptr + 93));
    memcpy((void*)bse_modify_order_single_short_request_.AlgoID,
           (void*)(msg_ptr + 95),LEN_ALGOID);
    memcpy((void*)bse_modify_order_single_short_request_.FreeText1,
           (void*)(msg_ptr + 111),LEN_FREE_TEXT1);
    memcpy((void*)bse_modify_order_single_short_request_.CPCode,
           (void*)(msg_ptr + 123),LEN_CP_CODE);

    return &bse_modify_order_single_short_request_;
  }


};
}
}
