// =====================================================================================
//
//       Filename:  BSENewOrderSingleShort.hpp
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

class BSENewOrderSingleShort {
 public:
  NewOrderSingleShortRequestT bse_new_order_single_short_request_;

 public:
//  BSENewOrderSingleShort(const uint32_t& _sender_sub_id_, const uint32_t& _simple_security_id_,
//                         const uint8_t trading_capacity, const char *_algo_id_, const char *_free_text1_, const char *_cp_code_) {
    BSENewOrderSingleShort() {
    // initialize user logon message  -- this is only intended to clear garbage value if any, BSE has novalue concept
    memset((void*)(&bse_new_order_single_short_request_), 0, sizeof(NewOrderSingleShortRequestT));

    // fill up the MessageHeader
    bse_new_order_single_short_request_.MessageHeaderIn.BodyLen = sizeof(NewOrderSingleShortRequestT);
    bse_new_order_single_short_request_.MessageHeaderIn.TemplateID = TID_NEW_ORDER_SINGLE_SHORT_REQUEST;

    // TimeInForce - Options
    // 0 - GFD
    // 3 - IOC
    bse_new_order_single_short_request_.TimeInForce = 7;  // GFD

    // AccountType - Options
    // 20 - OWN
    // 30 - CLIENT
    // 40 - SPECIAL CLIENT
    // 90 - INSTITUTION
    bse_new_order_single_short_request_.AccountType = 20;

    // STPCFlag - Options
    // 0 - PASSIVE
    // 1 - ACTIVE
    bse_new_order_single_short_request_.STPCFlag = 0;

  }

  void setBSENewOrderSingleShortStaticFields(const uint32_t& _sender_sub_id_, const uint64_t& _sender_location_id_,
                                                const char* _algo_id_, const char* _client_code_, const char* _cp_code_) {
    bse_new_order_single_short_request_.RequestHeader.SenderSubID = _sender_sub_id_;
    bse_new_order_single_short_request_.SenderLocationID = _sender_location_id_;
    memcpy((void*)(bse_new_order_single_short_request_.AlgoID), (void*)(_algo_id_), LEN_ALGOID);
    memcpy((void*)(bse_new_order_single_short_request_.FreeText1), (void*)(_client_code_), LEN_FREE_TEXT1);
    memcpy((void*)(bse_new_order_single_short_request_.CPCode), (void*)(_cp_code_), LEN_CP_CODE);
  }

  // set the session sequence number
  void setBSENewOrderSingleShortMessageSequnece(const uint32_t& _message_sequence_) {
    bse_new_order_single_short_request_.RequestHeader.MsgSeqNum = _message_sequence_;
  }

  void setBSENewOrderSingleShortPrice(const double& _price_) {
    bse_new_order_single_short_request_.Price = (int64_t)((_price_ * ETI_FIXED_DECIMAL_OFFSET_MULTIPLIER_FOR_PRICE) + 0.5);
  }

  void setBSENewOrderSingleShortClOrderId(const uint32_t& _cl_ord_id_) {
    bse_new_order_single_short_request_.ClOrdID = _cl_ord_id_;
  }

  void setBSENewOrderSingleShortOrderQty(const int32_t& _order_qty_) {
    bse_new_order_single_short_request_.OrderQty = _order_qty_;
  }

  void setBSENewOrderSingleShortOrderSide(const uint8_t& _side_) {
    bse_new_order_single_short_request_.Side = (_side_ == HFSAT::kTradeTypeBuy) ? 1 : 2;
  }

  void setBSENewOrderSingleShortSenderSubID(const uint32_t& _sender_sub_id_) { 
    // fill up the RequestHeeader
    bse_new_order_single_short_request_.RequestHeader.SenderSubID = _sender_sub_id_;
  }
    

  // a single call should be enough to fill all the dynamic fields
  void setBSENewOrderSingleShortDynamicFields(HFSAT::ORS::Order *rp_order_,
                                              const uint32_t& _message_sequence_, const uint32_t& _simple_security_id_) {
    bse_new_order_single_short_request_.RequestHeader.MsgSeqNum = _message_sequence_;
    bse_new_order_single_short_request_.Price = (int64_t)((rp_order_->price_ * ETI_FIXED_DECIMAL_OFFSET_MULTIPLIER_FOR_PRICE) + 0.5);
    bse_new_order_single_short_request_.ClOrdID = rp_order_->server_assigned_order_sequence_;
    bse_new_order_single_short_request_.OrderQty = rp_order_->size_remaining_;
    bse_new_order_single_short_request_.MaxShow = rp_order_->size_remaining_;
    bse_new_order_single_short_request_.SimpleSecurityID = _simple_security_id_;
    bse_new_order_single_short_request_.Side = (rp_order_->buysell_ == HFSAT::kTradeTypeBuy) ? 1 : 2;
  }
  // Setting the timeinforce = GTD , GFS, IOC
  void setBSENewOrderSingleShortTimeInforce(const uint8_t _timeInforce_) {
    bse_new_order_single_short_request_.TimeInForce = _timeInforce_;
  }
  // get MsgLength to write
  int getBSENewOrderSingleShortMsgLength() { return (sizeof(NewOrderSingleShortRequestT)); }

  NewOrderSingleShortRequestT *ProcessNewOrderSingleShortRequest(char const *msg_ptr) {
    bse_new_order_single_short_request_.MessageHeaderIn.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_new_order_single_short_request_.MessageHeaderIn.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_new_order_single_short_request_.RequestHeader.MsgSeqNum = *((uint32_t *)(msg_ptr + 16));
    bse_new_order_single_short_request_.RequestHeader.SenderSubID = *((uint32_t *)(msg_ptr + 20));
    bse_new_order_single_short_request_.Price = *((int64_t *)(msg_ptr + 24));
    bse_new_order_single_short_request_.SenderLocationID = *((uint64_t *)(msg_ptr + 32));
    bse_new_order_single_short_request_.ClOrdID = *((uint64_t *)(msg_ptr + 40));
    bse_new_order_single_short_request_.OrderQty = *((int32_t *)(msg_ptr + 48));
    bse_new_order_single_short_request_.MaxShow = *((int32_t *)(msg_ptr + 52));
    bse_new_order_single_short_request_.SimpleSecurityID = *((uint32_t *)(msg_ptr + 56));
    bse_new_order_single_short_request_.AccountType = *((uint8_t *)(msg_ptr + 66));
    bse_new_order_single_short_request_.Side = *((uint8_t *)(msg_ptr + 67));
    bse_new_order_single_short_request_.TimeInForce = *((uint8_t *)(msg_ptr + 69));
    bse_new_order_single_short_request_.STPCFlag = *((uint8_t *)(msg_ptr + 70));
    memcpy((void*)bse_new_order_single_short_request_.AlgoID,
           (void*)(msg_ptr + 72),LEN_ALGOID);
    memcpy((void*)bse_new_order_single_short_request_.FreeText1,
           (void*)(msg_ptr + 88),LEN_FREE_TEXT1);
    memcpy((void*)bse_new_order_single_short_request_.CPCode,
           (void*)(msg_ptr + 100),LEN_CP_CODE);

    return &bse_new_order_single_short_request_;
  }

};
}
}
