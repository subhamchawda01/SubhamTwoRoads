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

#include "dvccode/CDef/debug_logger.hpp"
#include "infracore/NSET/NSETemplates/RequestPacket.hpp"
#include "infracore/NSET/NSETemplates/RequestHeader.hpp"

namespace HFSAT {
namespace NSE {
/*
 * Generic interface for nse order entry trimmed template.
 * Please extend this for both derivatives and cash market.
 */

class OrderEntryRequest {
 protected:
  char *order_entry_request_buffer;
  char const *msg_ptr;

 public:
  OrderEntryRequest() {
    order_entry_request_buffer = nullptr;
    msg_ptr = nullptr;
  }
  virtual ~OrderEntryRequest() {}

  virtual void InitializeStaticFields() {}

  // Sets packet seq no and length
  inline void SetPacketLength(int16_t const &packet_length) {
    *((int16_t *)(msg_ptr + NSE_PACKET_LENGTH_OFFSET)) = hton16(packet_length);
  }

  inline void SetPacketSequenceNumber(int32_t const &packet_sequence_number) {
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
  virtual void SetPrice(int32_t const &price){};
  virtual void SetBranchId(int16_t const &branch_id) {}
  virtual void SetTraderId(int32_t const &trader_id) {}
  virtual void SetPan(char const* pan) {}
  virtual void SetBrokerId(char const *broker_id) {}
  virtual void SetSaos(int32_t const &saos) {}
  virtual void SetBookType(int16_t const &book_type) {}
  virtual void SetNNF(double const &nnf) {}
  virtual void SetAlgoId(int32_t const &algo_id) {}

 public:
  inline void SetPreLoadedOrderEntryRequestFields(int32_t const &user_id, char const *broker_id, double const &nnf, int32_t const &algo_id, char const *pan, int16_t const &branch_id) {
    SetBranchId(branch_id);
    SetTraderId(user_id);
    SetPan(pan);
    SetBrokerId(broker_id);
    SetAccountNumber(broker_id);
    SetNNF(nnf);
    SetAlgoId(algo_id);
  }

  virtual void AddPreOpen() {} //To be overridden in Cash

  virtual void SetDynamicOrderEntryRequestFields(int32_t const &packet_sequence_number, int16_t const &buy_sell,
                                                 int32_t const &volume, int32_t const &disclosed_volume,
                                                 int32_t const &price, int32_t const &saos, bool is_ioc,
                                                 InstrumentDesc *inst_desc, bool is_mo, bool add_preopen) = 0;

  char const *GetOrderEntryRequestBuffer() const { return order_entry_request_buffer; }
  virtual int32_t GetOrderEntryMsgLength() = 0;
};
}
}
