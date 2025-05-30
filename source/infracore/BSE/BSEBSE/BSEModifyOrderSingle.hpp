// =====================================================================================
//
//       Filename:  BSEModifyOrderSingle.hpp
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

class BSEModifyOrderSingle {
 public:
  ModifyOrderSingleRequestT bse_modify_order_single_request_;

 public:
    
  BSEModifyOrderSingle() {
    // initialize Modify order message  -- this is only intended to clear garbage value if any, BSE has novalue concept
    memset((void*)(&bse_modify_order_single_request_), 0, sizeof(ModifyOrderSingleRequestT));

    // fill up the MessageHeader
    bse_modify_order_single_request_.MessageHeaderIn.BodyLen = sizeof(ModifyOrderSingleRequestT);
    bse_modify_order_single_request_.MessageHeaderIn.TemplateID = TID_MODIFY_ORDER_SINGLE_SHORT_REQUEST;

    
    // TimeInForce - Options
    // 0 - GFD
    // 3 - IOC
    bse_modify_order_single_request_.TimeInForce = 0;  // GFD

    // AccountType - Options
    // 20 - Own
    // 30 - Client
    // 40 - Special Client
    // 90 - Institution
    bse_modify_order_single_request_.AccountType = 20;

    //@ApplSeqIndicator - Options
    // 0 - lean order
    // 1 - standard order
    bse_modify_order_single_request_.ApplSeqIndicator = 0;

    //@OrderType - Options
    // 5 - Market
    // 2 - Limit
    // 3 - Stop
    // 4 - Stop Limit
    // 6 - Block Deal
    bse_modify_order_single_request_.OrdType = 2;

    // ExecInst - OPtions
    // 1 - Persistent
    // 2 - Non Persistent
    // 5 - Persistent BOC
    // 6 - Non ( 5 )
    bse_modify_order_single_request_.ExecInst = 2;

    bse_modify_order_single_request_.StopPx = ETI_SIGNED_8_BYTE_INT_NOVALUE;
    bse_modify_order_single_request_.MaxPricePercentage = ETI_SIGNED_8_BYTE_INT_NOVALUE;
    bse_modify_order_single_request_.MessageTag = ETI_SIGNED_4_BYTE_INT_NOVALUE;
    bse_modify_order_single_request_.PriceValidityCheckType = 0;
    bse_modify_order_single_request_.TradingCapacity = 1;
    memcpy((void*)(&bse_modify_order_single_request_.Account), "A1", 2);
    bse_modify_order_single_request_.PositionEffect[LEN_POSITIONEFFECT - 1] = ENUM_POSITION_EFFECT_CLOSE_CHAR;
    memcpy((void*)(bse_modify_order_single_request_.RegulatoryText), " ", LEN_REGULATORY_TEXT);
    memcpy((void*)(bse_modify_order_single_request_.FreeText3), " ", LEN_FREE_TEXT3);

  }

  void setBSEModifyOrderSingleStaticFields(const uint32_t& _sender_sub_id_, const uint64_t& _sender_location_id_, const uint32_t& _session_id_,
                                                const char* _algo_id_, const char* _client_code_, const char* _cp_code_) {
    bse_modify_order_single_request_.RequestHeader.SenderSubID = _sender_sub_id_;
    bse_modify_order_single_request_.SenderLocationID = _sender_location_id_;
    bse_modify_order_single_request_.TargetPartyIDSessionID = _session_id_; 
    memcpy((void*)(bse_modify_order_single_request_.AlgoID), (void*)(_algo_id_), LEN_ALGOID);
    memcpy((void*)(bse_modify_order_single_request_.FreeText1), (void*)(_client_code_), LEN_FREE_TEXT1);
    memcpy((void*)(bse_modify_order_single_request_.CPCode), (void*)(_cp_code_), LEN_CP_CODE);
  }

  // set the session sequence number
  void setBSEModifyOrderSingleMessageSequnece(const uint32_t& _message_sequence_) {
    bse_modify_order_single_request_.RequestHeader.MsgSeqNum = _message_sequence_;
  }

  void setBSEModifyOrderSinglePrice(const double& _price_) {
    bse_modify_order_single_request_.Price = (int64_t)((_price_ * ETI_FIXED_DECIMAL_OFFSET_MULTIPLIER_FOR_PRICE) + 0.5);
  }

  void setBSEModifyOrderSingleClOrderId(const uint32_t& _cl_ord_id_) {
    bse_modify_order_single_request_.ClOrdID = _cl_ord_id_;
  }

  void setBSEModifyOrderSingleOrderQty(const int32_t& _order_qty_) {
    bse_modify_order_single_request_.OrderQty = _order_qty_;
  }

  void setBSEModifyOrderSingleOrderSide(const uint8_t& _side_) {
    bse_modify_order_single_request_.Side = (_side_ == HFSAT::kTradeTypeBuy) ? 1 : 2;
  }

  void setBSEModifyOrderSingleOrigClorID(const uint32_t& _orig_cl_ord_id_) {
    bse_modify_order_single_request_.OrigClOrdID = _orig_cl_ord_id_;
  }

  void setBSEModifyOrderSingleSenderSubID(const uint32_t& _sender_sub_id_) {
    // fill up the RequestHeeader
    bse_modify_order_single_request_.RequestHeader.SenderSubID = _sender_sub_id_;
  }

  // a single call should be enough to fill all the dynamic fields
  void setBSEModifyOrderSingleDynamicFields(const uint32_t& _message_sequence_, const uint64_t& _order_id_,
                                                 const uint64_t& _client_order_id_, const uint64_t& _original_client_order_id_,
                                                 const double& _price_, const uint64_t& _activity_time_,
                                                 const int32_t& _order_quantity_, const uint32_t& _simple_security_id_,
                                                 const int32_t& _market_segment_id_, const uint8_t& _side_) {

    bse_modify_order_single_request_.RequestHeader.MsgSeqNum = _message_sequence_;
    bse_modify_order_single_request_.OrderID = _order_id_;
    bse_modify_order_single_request_.ClOrdID = _client_order_id_;
    bse_modify_order_single_request_.OrigClOrdID = _original_client_order_id_;
    bse_modify_order_single_request_.Price = (int64_t)((_price_ * ETI_FIXED_DECIMAL_OFFSET_MULTIPLIER_FOR_PRICE) + 0.5);
    bse_modify_order_single_request_.ActivityTime = _activity_time_;
    bse_modify_order_single_request_.OrderQty = _order_quantity_;
    bse_modify_order_single_request_.MaxShow = _order_quantity_;
    bse_modify_order_single_request_.MarketSegmentID = _market_segment_id_;
    bse_modify_order_single_request_.SimpleSecurityID = _simple_security_id_;
    bse_modify_order_single_request_.Side = (_side_ == HFSAT::kTradeTypeBuy) ? 1 : 2;
  }

  // get MsgLength to write
  int getBSEModifyOrderSingleMsgLength() { return (sizeof(ModifyOrderSingleRequestT)); }

  ModifyOrderSingleRequestT *ProcessModifyOrderSingleRequest(char const *msg_ptr) {
    bse_modify_order_single_request_.MessageHeaderIn.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_modify_order_single_request_.MessageHeaderIn.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_modify_order_single_request_.RequestHeader.MsgSeqNum = *((uint32_t *)(msg_ptr + 16));
    bse_modify_order_single_request_.RequestHeader.SenderSubID = *((uint32_t *)(msg_ptr + 20));
    bse_modify_order_single_request_.OrderID = *((uint64_t *)(msg_ptr + 24));
    bse_modify_order_single_request_.ClOrdID = *((uint64_t *)(msg_ptr + 32));
    bse_modify_order_single_request_.OrigClOrdID = *((uint64_t *)(msg_ptr + 40));
    bse_modify_order_single_request_.Price = *((int64_t *)(msg_ptr + 48));
    bse_modify_order_single_request_.SenderLocationID = *((uint64_t *)(msg_ptr + 56));
    bse_modify_order_single_request_.ActivityTime = *((uint64_t *)(msg_ptr + 64));
    bse_modify_order_single_request_.OrderQty = *((int32_t *)(msg_ptr + 72));
    bse_modify_order_single_request_.MaxShow = *((int32_t *)(msg_ptr + 76));
    bse_modify_order_single_request_.SimpleSecurityID = *((uint32_t *)(msg_ptr + 80));
    bse_modify_order_single_request_.AccountType = *((uint8_t *)(msg_ptr + 90));
    bse_modify_order_single_request_.Side = *((uint8_t *)(msg_ptr + 91));
    bse_modify_order_single_request_.TimeInForce = *((uint8_t *)(msg_ptr + 93));
    memcpy((void*)bse_modify_order_single_request_.AlgoID,
           (void*)(msg_ptr + 95),LEN_ALGOID);
    memcpy((void*)bse_modify_order_single_request_.FreeText1,
           (void*)(msg_ptr + 111),LEN_FREE_TEXT1);
    memcpy((void*)bse_modify_order_single_request_.CPCode,
           (void*)(msg_ptr + 123),LEN_CP_CODE);

    return &bse_modify_order_single_request_;
  }


};
}
}
