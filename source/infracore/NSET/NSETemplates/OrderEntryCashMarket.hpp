// =====================================================================================
//
//       Filename:  OrderEntryCashMarket.hpp
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
//        Address:  Suite No 353, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include "infracore/NSET/NSETemplates/OrderEntry.hpp"
#include "infracore/NSET/NSETemplates/cash_market_order_structure_defines.hpp"

namespace HFSAT {
namespace NSE {
/*
 * Defines Order entry semantics for cash market(CM)
 * Extends generic orderEntry template for NSE trimmed structure.
 */

class OrderEntryRequestCashMarket : public OrderEntryRequest {
  /* private:
    SecurityInfoCashMarket sec_info;
  */
 public:
  
  void InitializeStaticFields() {
    //================================= @ Packet

    // Total Length Of The Packet To Be Sent
    SetPacketLength(NSE_CM_ORDERENTRY_REQUEST_LENGTH);

    // ================================ @ Order Entry

    SetTransactionCode(BOARD_LOT_IN_TR);

    memset((void *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_ACCOUNTNUMBER_OFFSET), ' ',
           NSE_CM_ORDERENTRY_REQUEST_ACCOUNTNUMBER_LENGTH);
    *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_BOOKTYPE_OFFSET)) = hton16(1);  // regular order
    *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) =
        hton16((int16_t)(1 << 12));  // day order

    *((char *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_SUSPENDED_OFFSET)) = ' ';  // blank while sending order entry request
    memset((void *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_SETTLOR_OFFSET), ' ',
           NSE_CM_ORDERENTRY_REQUEST_SETTLOR_LENGTH);  // this field should be set to blank for a pro order
                                                       //(broker’s own order).
    *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_PROCLIENTINDICATOR_OFFSET)) = hton16(2);
    *((double *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_NFFIELD_OFFSET)) =
        (double)swap_endian((double)111111111111000);  // 00 56 BC 93 84 43 D9 42

    *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_ALGOID_OFFSET)) = hton32(0);

    // This has to be 0 - Algo category, this was actually made as reserved field by the exchange
    *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_ALGOCAT_OFFSET)) = hton16(0);
  }

 public:
  OrderEntryRequestCashMarket() {
    order_entry_request_buffer = (char *)calloc(NSE_CM_ORDERENTRY_REQUEST_LENGTH, sizeof(char));
    msg_ptr = order_entry_request_buffer + NSE_REQUEST_START_OFFSET;

    // Initialize The Static Fields
    InitializeStaticFields();
  }

  // =================================== Order Entry Field Setter Functions

  inline void SetTransactionCode(int16_t const &message_header_transaction_code) {
    *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_TRANSACTIONCODE_OFFSET)) =
        hton16(message_header_transaction_code);
  }

  inline void SetProClient(int16_t const &proclient){
    // ‘1’ represents the client’s order.
    // '2’ represents a broker’s order.
    *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_PROCLIENTINDICATOR_OFFSET)) = hton16(proclient); // default is 2
  }
  /*
    inline void SetInstrumentDesc(char const *inst_desc) {
      memcpy((void *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_CONTRACT_SECINFO_STRUCT_SYMBOL_OFFSET), inst_desc,
             sizeof(SecurityInfoCashMarket));
    }
  */
  inline void SetIOC() {
    *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) =
        hton16((int16_t)(1 << 10));  // ioc order
  }

  inline void AddPreOpen() final {
    *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) |=
        hton16((int16_t)(1 << 3));  // ioc order
  }
  /*
    inline void SetDay() {
      *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) =
          hton16((int16_t)(1 << 12));  // day order
    }
  */
  // For broker’s own order, this field should be set to the broker code.
  inline void SetAccountNumber(char const *broker_id) {
    memcpy((void *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_ACCOUNTNUMBER_OFFSET), (void *)broker_id,
           NSE_CM_ORDERENTRY_REQUEST_ACCOUNTNUMBER_LENGTH);
  }

  inline void SetSettlor(char const *settlor_) {
    memcpy((void *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_SETTLOR_OFFSET), (void *)settlor_,
           NSE_CM_ORDERENTRY_REQUEST_SETTLOR_LENGTH);  // this field should be set to blank for a pro order
  }


  inline void SetBookType(int16_t const &book_type) {  // 1-RL, 3-SL, 5-Odd Lot
    *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_BOOKTYPE_OFFSET)) = hton16(book_type);
  }
  /*
    inline void SetBuySell(int16_t const &buy_sell) {
      *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_BUYSELLINDICATOR_OFFSET)) = hton16(buy_sell);
    }

    inline void SetVolume(int32_t const &volume, int32_t const &disclosed_volume) {
      *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_OFFSET)) = hton32(disclosed_volume);
      *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_VOLUME_OFFSET)) = hton32(volume);
    }

    inline void SetPrice(int32_t const &price) {
      *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_PRICE_OFFSET)) = hton32(price);
    }
  */
  inline void SetBranchId(int16_t const &branch_id) {
    *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_BRNACHID_OFFSET)) = hton16(branch_id);
  }

  inline void SetTraderId(int32_t const &trader_id) {
    *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_TRADERID_OFFSET)) = hton32(trader_id);
    *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_USERID_OFFSET)) = hton32(trader_id);
  }

  inline void SetPan(char const *pan) {
    memcpy((void *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_PAN_OFFSET), pan, NSE_CM_ORDERENTRY_REQUEST_PAN_LENGTH);
  }

  inline void SetBrokerId(char const *broker_id) {
    memcpy((void *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_BROKERID_OFFSET), (void *)broker_id,
           NSE_CM_ORDERENTRY_REQUEST_BROKERID_LENGTH);
  }
  /*
    // SAOS for new order in TransactionId....
    inline void SetSaos(int32_t const &saos) {
      *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_TRANSACTION_ID_OFFSET)) = hton32(saos);
    }
  */
  inline void SetNNF(double const &nnf) {
    *((double *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_NFFIELD_OFFSET)) =
        (double)swap_endian((double)nnf);  // 00 56 BC 93 84 43 D9 42
  }

  inline void SetAlgoId(int32_t const &algo_id) {
    *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_ALGOID_OFFSET)) = hton32(algo_id);
  }

  inline void SetDynamicOrderEntryRequestFields(int32_t const &packet_sequence_number, int16_t const &buy_sell,
                                                int32_t const &price, HFSAT::ORS::Order *order,
                                                InstrumentDesc *inst_desc, SecurityInfoCashMarket *sec_info,
                                                bool is_mo = false, bool add_preopen = false) {

    //std::cout << "INSIDE SetDynamicOrderEntryRequestFields" << std::endl;
    //    sec_info.SetSecurityInfo(inst_desc->symbol_, inst_desc->option_type_);
    memcpy((void *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_CONTRACT_SECINFO_STRUCT_SYMBOL_OFFSET),
           sec_info->GetSecurityInfoCashMarketAsBuffer(), sec_info_cash_size);
    *((int32_t *)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)) = hton32(packet_sequence_number);
    *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_BUYSELLINDICATOR_OFFSET)) = hton16(buy_sell);
    *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_PRICE_OFFSET)) = hton32(price);
    *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_TRANSACTION_ID_OFFSET)) =
        hton32(order->server_assigned_order_sequence_);
    // SetInstrumentDesc(sec_info->GetSecurityInfoCashMarketAsBuffer());
    // SetPacketSequenceNumber(packet_sequence_number);
    // SetBuySell(buy_sell);
    // SetPrice(price);
    // SetSaos(saos);

    if (__builtin_expect(!order->is_ioc, true) && __builtin_expect(!is_mo, true)) {
      *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) =
          hton16((int16_t)(1 << 12));  // day order
      *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_OFFSET)) = hton32(order->size_disclosed_);
      *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_VOLUME_OFFSET)) = hton32(order->size_remaining_);
      // SetDay();
      // SetVolume(volume, disclosed_volume);
    } else if (__builtin_expect(is_mo, false)) {
      // market order needs DAY flag to be set and
      // disclosed volume needs to be set to 0
      *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) =
          hton16((int16_t)(1 << 12));  // day order
      *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_PRICE_OFFSET)) = hton32(0);
      *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_OFFSET)) = hton32(0);
      *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_VOLUME_OFFSET)) = hton32(order->size_remaining_);
      // SetDay();
      // SetPrice(0);
      // SetVolume(volume, 0);
    } else if (order->is_ioc) {
      *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) = hton16((int16_t)(1 << 10));
      *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_OFFSET)) = hton32(0);
      *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_VOLUME_OFFSET)) = hton32(order->size_remaining_);
      // SetIOC();
      // disclosed volume needs to be set to 0, in case of ioc orders
      // SetVolume(volume, 0);
    }

    if (true == add_preopen) {
      *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) = hton16((int16_t)(1 << 12));
      *((int16_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) |= hton16((int16_t)(1 << 3));
      *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_OFFSET)) = hton32(0);
      *((int32_t *)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_VOLUME_OFFSET)) = hton32(order->size_remaining_);

      // SetDay();
      // AddPreOpen();
      // SetVolume(volume, 0);
    }
    if (AVX512_SUPPORTED)
    HFSAT::MD5::MD5_AVX512VL((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_CM_ORDERENTRY_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
    else
    HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_CM_ORDERENTRY_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
    //std::cout << "MDSUM COMPUTED LEAVING\n";
  }

  inline void SetDynamicOrderEntryRequestFields(int32_t const &packet_sequence_number, int16_t const &buy_sell_1,
                                                 int16_t const &buy_sell_2, int16_t const &buy_sell_3,
                                                 int32_t const &price_1,int32_t const &price_2,int32_t const &price_3,
                                                 HFSAT::ORS::Order *order_1, HFSAT::ORS::Order *order_2, HFSAT::ORS::Order *order_3,
                                                 InstrumentDesc *inst_desc_1,InstrumentDesc *inst_desc_2,InstrumentDesc *inst_desc_3,
                                                 SecurityInfoCashMarket *sec_info_1,SecurityInfoCashMarket *sec_info_2, SecurityInfoCashMarket *sec_info_3,
                                                 bool is_mo, bool add_preopen) {
    std::cout << "Error::MulitLeg Modify Not implemented... "<< std::endl;
  }

  int32_t GetOrderEntryMsgLength() { return NSE_CM_ORDERENTRY_REQUEST_LENGTH; }
  
};
}
}
