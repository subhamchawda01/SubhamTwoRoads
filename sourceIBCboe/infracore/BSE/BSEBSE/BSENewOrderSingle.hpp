// =====================================================================================
//
//       Filename:  BSENewOrderSingle.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/04/2012 02:47:41 PM
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

////////////////////////////////////////   WE ARE NOT USING THIS TEMPLATE AND HENCE THIS NEEDS TO BE UPDATED IF WE
/// INTEND TO USE THIS    ////////////////////////////////////////// - ravi

#include "infracore/BSE/BSEBSE/BSEMessageDefs.hpp"

namespace HFSAT {
namespace BSE {

class BSENewOrderSingle {
 public:
  NewOrderSingleRequestT bse_new_order_single_request_;

 public:
//  BSENewOrderSingle(const uint32_t& _sender_sub_id_, const int32_t& _market_segment_id_, const uint32_t& _security_id_,
//                    const uint8_t trading_capacity, const char *_algo_id_, const char *_free_text1_, const char *_cp_code_,
//                    const char *_regulatory_text_, const char *_free_text3_) {
    BSENewOrderSingle() {
    // initialize new order single message
    memset((void*)(&bse_new_order_single_request_), 0, sizeof(NewOrderSingleRequestT));

    // fill up the message header
    bse_new_order_single_request_.MessageHeaderIn.BodyLen = sizeof(NewOrderSingleRequestT);
    bse_new_order_single_request_.MessageHeaderIn.TemplateID = TID_NEW_ORDER_SINGLE_REQUEST;

    bse_new_order_single_request_.PositionEffect[LEN_POSITIONEFFECT - 1] = ENUM_POSITION_EFFECT_CLOSE_CHAR; 

    //@ApplSeqIndicator - Options
    // 0 - lean order
    // 1 - standard order
    bse_new_order_single_request_.ApplSeqIndicator = 0;

    //@OrderType - Options
    // 5 - Market
    // 2 - Limit
    // 3 - Stop
    // 4 - Stop Limit
    bse_new_order_single_request_.OrdType = 5;

    //@Timeinforce - Options
    // 0 - Day
    // 1 - GTC
    // 2 - IOC
    // 3 - GTD
    bse_new_order_single_request_.TimeInForce = 0;

    // ExecInst - OPtions
    // 1 - Persistent
    // 2 - Non Persistent
    // 5 - Persistent BOC
    // 6 - Non ( 5 )
    bse_new_order_single_request_.ExecInst = 2;

    // TradingCapacity - Options
    // 1 - Customer ( Agency )
    // 5 - Pricipal ( Proprietary )
    // 6 - Market Maker
    bse_new_order_single_request_.TradingCapacity = 1;

    // AccountType - Options
    // 20 - OWN
    // 30 - CLIENT
    // 40 - SPECIAL CLIENT
    // 90 - INSTITUTION
    bse_new_order_single_request_.AccountType = 20;

    // STPCFlag - Options
    // 0 - PASSIVE
    // 1 - ACTIVE
    bse_new_order_single_request_.STPCFlag = 0;


    bse_new_order_single_request_.MaxPricePercentage = 3;

    // no value
    bse_new_order_single_request_.Price = ETI_SIGNED_8_BYTE_INT_NOVALUE;
    bse_new_order_single_request_.StopPx = ETI_SIGNED_8_BYTE_INT_NOVALUE;
    bse_new_order_single_request_.Filler1 = ETI_UNSIGNED_8_BYTE_INT_NOVALUE;
    bse_new_order_single_request_.Filler2 = ETI_UNSIGNED_4_BYTE_INT_NOVALUE;
    bse_new_order_single_request_.ExpireDate = ETI_UNSIGNED_4_BYTE_INT_NOVALUE;
    bse_new_order_single_request_.RegulatoryID = ETI_UNSIGNED_4_BYTE_INT_NOVALUE;
    bse_new_order_single_request_.Filler4 = ETI_UNSIGNED_2_BYTE_INT_NOVALUE;
    memset(bse_new_order_single_request_.ContraBroker, 'A', LEN_CONTRA_BROKER);
    memset(bse_new_order_single_request_.PartyIDOrderOriginationFirm, 'A', LEN_PARTY_ID_ORDER_ORIGINATION_FIRM);
    memset(bse_new_order_single_request_.PartyIDBeneficiary, 'A', LEN_PARTY_ID_BENEFICIARY);
    bse_new_order_single_request_.PriceValidityCheckType = 0;
    bse_new_order_single_request_.Filler5 = ETI_UNSIGNED_1_BYTE_INT_NOVALUE;
    bse_new_order_single_request_.TradingSessionSubID = ETI_UNSIGNED_1_BYTE_INT_NOVALUE;
    memcpy(bse_new_order_single_request_.Account, "A1", 2);
    memset(bse_new_order_single_request_.PartyIDLocationID, 'A', LEN_PARTY_ID_LOCATIONID);
    memset(bse_new_order_single_request_.CustOrderHandlingInst, ' ', LEN_CUST_ORDER_HANDLING_INST);
    memset(bse_new_order_single_request_.RegulatoryText, ' ', LEN_REGULATORY_TEXT);
    
  }

  void setBSENewOrderSingleStaticFields(const uint32_t& _sender_sub_id_, const uint64_t& _sender_location_id_,
                                                const char* _algo_id_, const char* _client_code_, const char* _cp_code_) {
    bse_new_order_single_request_.RequestHeader.SenderSubID = _sender_sub_id_;
    bse_new_order_single_request_.SenderLocationID = _sender_location_id_;
    memcpy((void*)(bse_new_order_single_request_.AlgoID), (void*)(_algo_id_), LEN_ALGOID);
    memcpy((void*)(bse_new_order_single_request_.FreeText1), (void*)(_client_code_), LEN_FREE_TEXT1);
    memcpy((void*)(bse_new_order_single_request_.CPCode), (void*)(_cp_code_), LEN_CP_CODE);
  }


  // update dynamic fiels - a sigle function call is better than havign multiple for individual fields
  void setBSENewOrderSingleDynamicFields(HFSAT::ORS::Order *rp_order_,
                                         const uint32_t& _message_sequence_,const int32_t& _market_segment_id_,
                                         const uint32_t& _security_id_) {
    bse_new_order_single_request_.RequestHeader.MsgSeqNum = _message_sequence_;
    bse_new_order_single_request_.ClOrdID = rp_order_->server_assigned_order_sequence_;
    bse_new_order_single_request_.OrderQty = rp_order_->size_remaining_;
    bse_new_order_single_request_.MaxShow = rp_order_->size_remaining_;
    bse_new_order_single_request_.MarketSegmentID = _market_segment_id_;  // Product identifier
    bse_new_order_single_request_.SimpleSecurityID = _security_id_;       // SecurityId from Refmode
    bse_new_order_single_request_.Side = (rp_order_->buysell_ == HFSAT::kTradeTypeBuy) ? 1 : 2;
  }

  // get MsgLength to write
  int getBSENewOrderSingleMsgLength() { return (sizeof(NewOrderSingleRequestT)); }

};
}
}
