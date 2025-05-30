// =====================================================================================
//
//       Filename:  OrderEntry.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/29/2015 11:11:05 PM
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

#include "infracore/NSET/NSETemplates/RequestPacket.hpp"
#include "infracore/NSET/NSETemplates/RequestHeader.hpp"
#include "dvccode/CDef/debug_logger.hpp"

namespace HFSAT {
namespace NSE {

/*
 * Generic order cancel interface for nse trimmed template.
 * Please extend this for both derivatives and cash market.
 */

class OrderCancelRequest {
 protected:
  char *order_change_request_buffer;
  char const *msg_ptr;

 public:
  OrderCancelRequest() {
    order_change_request_buffer = nullptr;
    msg_ptr = nullptr;
  }
  virtual ~OrderCancelRequest() {}

  virtual void InitializeStaticFields() {}

  // Sets packet seq no and length
  virtual void SetPacketLength(int16_t const &packet_length) {
    *((int16_t *)(msg_ptr + NSE_PACKET_LENGTH_OFFSET)) = hton16(packet_length);
  }

  virtual void SetPacketSequenceNumber(int32_t const &packet_sequence_number) {
    *((int32_t *)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)) = hton32(packet_sequence_number);
  }

  // =================================== Order Entry Field Setter Functions

  virtual void SetTransactionCode(int16_t const &message_header_transaction_code) {}

  virtual void SetTokenNumber(int32_t const &token_number) {}
  virtual void SetInstrumentDesc(char const *inst_desc) {}

  virtual void SetContractDescInstrumentName(char const *instrument_name) {}

  virtual void SetContractDescSymbol(char const *symbol) {}

  virtual void SetContractDescExpiryDate(int32_t const &expiry) {}

  virtual void SetContractDescStrikePrice(int32_t const &strike) {}

  virtual void SetContractDescOptionType(char const *option_type) {}
  virtual void SetAccountNumber(char const *broker_id) {}
  virtual void SetBuySell(int16_t const &buy_sell) {}

  virtual void SetVolume(int32_t const &volume) {}

  virtual void SetBranchId(int16_t const &branch_id) {}

  virtual void SetPrice(int32_t const &price) {}

  virtual void SetEntryDateTime(int32_t const &entry_date) {}

  virtual void SetLastModifiedDateTime(int32_t const &modified_date) {}

  virtual void SetModified() {}

  virtual void SetTraderId(int32_t const &trader_id) {}
  
  virtual void SetPan(char const *pan) {}
  
  virtual void SetBrokerId(char const *broker_id) {}

  virtual void SetOrderNumber(int64_t const &order_num) {}

  virtual void SetSaos(int32_t const &saos) {}

  virtual void SetNNF(double const &nnf) {}

  virtual void SetLastActivityReference(int64_t &last_activity_reference) {}

  virtual void AddPreOpen() {}

  virtual void SetAlgoId(int32_t const &algo_id) {}

 public:

  virtual void SetPreLoadedOrderCancelRequestFields(int32_t const &user_id, char const *broker_id, double const &nnf, int32_t const &algo_id, char const *pan, int16_t const &branch_id) {
    SetBranchId(branch_id);
    SetTraderId(user_id);
    SetPan(pan);
    SetBrokerId(broker_id);
    SetAccountNumber(broker_id);
    SetNNF(nnf);
    SetAlgoId(algo_id);
  }

  virtual void SetDynamicOrderCancelRequestFields(int32_t const &packet_sequence_number, int64_t const &order_num,
                                                  int32_t const &saos, int32_t const &entry_date,
                                                  int32_t const &last_date, int16_t const &buy_sell,
                                                  int32_t const &volume, int32_t const &disclosed_volume,
                                                  int32_t const &price, InstrumentDesc *inst_desc, 
						  int64_t last_activity_reference_, bool add_preopen) = 0;

  char const *GetOrderCancelRequestBuffer() const { return order_change_request_buffer; }
  virtual int32_t GetOrderCancelMsgLength() = 0;
};

// ------------------------------------- Order Modify Request -------------------------------

/*
 * Generic order modify interface for nse trimmed template.
 * Please extend this for both derivatives and cash market.
 */

class OrderModifyRequest {
 protected:
  char *order_change_request_buffer;
  char const *msg_ptr;

  void InitializeStaticFields() {}

 public:
  OrderModifyRequest() {
    order_change_request_buffer = nullptr;
    msg_ptr = nullptr;
  }
  virtual ~OrderModifyRequest() {}

  // Sets packet seq no and length
  virtual void SetPacketLength(int16_t const &packet_length) {
    *((int16_t *)(msg_ptr + NSE_PACKET_LENGTH_OFFSET)) = hton16(packet_length);
  }

  virtual void SetPacketSequenceNumber(int32_t const &packet_sequence_number) {
    *((int32_t *)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)) = hton32(packet_sequence_number);
  }

  // =================================== Order Entry Field Setter Functions

  virtual void SetTransactionCode(int16_t const &message_header_transaction_code) {}

  virtual void SetTokenNumber(int32_t const &token_number) {}

  virtual void SetInstrumentDesc(char const *inst_desc) {}

  virtual void SetContractDescInstrumentName(char const *instrument_name) {}

  virtual void SetContractDescSymbol(char const *symbol) {}

  virtual void SetContractDescExpiryDate(int32_t const &expiry) {}

  virtual void SetContractDescStrikePrice(int32_t const &strike) {}

  virtual void SetContractDescOptionType(char const *option_type) {}
  virtual void SetAccountNumber(char const *broker_id) {}
  virtual void SetBuySell(int16_t const &buy_sell) {}

  virtual void SetVolume(int32_t const &volume, int32_t const &disclosed_volume) {}

  virtual void SetPrice(int32_t const &price) {}

  virtual void SetEntryDateTime(int32_t const &entry_date) {}

  virtual void SetLastModifiedDateTime(int32_t const &modified_date) {}
  virtual void SetModified() {}
  virtual void SetTraded() {}
  virtual void SetTradedVolume(int32_t const &volume) { std::cout << " SHOULD NEVER REACH HERE : " << std::endl; }
  virtual void SetBranchId(int16_t const &branch_id) {}

  virtual void SetTraderId(int32_t const &trader_id) {}

  virtual void SetPan(char const *pan) {}

  virtual void SetBrokerId(char const *broker_id) {}

  virtual void SetOrderNumber(int64_t const &order_num) {}

  virtual void SetSaos(int32_t const &saos) {}
	
  virtual void SetNNF(double const &nnf) {}
	
  virtual void SetLastActivityReference(int64_t &last_activity_reference) {}

  virtual void AddPreOpen() {}

  virtual void SetAlgoId(int32_t const &algo_id) {}

 public:
  virtual void SetPreLoadedOrderModifyRequestFields(int32_t const &user_id, char const *broker_id, double const &nnf, int32_t const &algo_id, char const *pan, int16_t const &branch_id) {
    SetBranchId(branch_id);
    SetTraderId(user_id);
    SetPan(pan);
    SetBrokerId(broker_id);
    SetAccountNumber(broker_id);
    SetModified();
    SetNNF(nnf);
    SetAlgoId(algo_id);
  }

  virtual void SetDynamicOrderModifyRequestFields(int32_t const &packet_sequence_number, int64_t const &order_num,
                                                  int32_t saos, int32_t const &entry_date, int32_t const &last_date,
                                                  int16_t const &buy_sell, int32_t const &volume,
                                                  int32_t const &disclosed_volume, int32_t const &price, int32_t const &traded_volume, 
						  InstrumentDesc *inst_desc, int64_t last_activity_reference_, bool add_preopen) = 0;

  char const *GetOrderModifyRequestBuffer() const { return order_change_request_buffer; }
  virtual int32_t GetOrderModifyMsgLength() = 0;
};
}
}
