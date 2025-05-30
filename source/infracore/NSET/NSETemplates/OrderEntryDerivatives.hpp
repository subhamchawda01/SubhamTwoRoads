// =====================================================================================
//
//       Filename:  OrderEntryDerivatives.hpp
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

// NSE FO segment defines

#define NSE_ORDERENTRY_REQUEST_TRANSACTIONCODE_OFFSET (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH)
#define NSE_ORDERENTRY_REQUEST_TRANSACTIONCODE_LENGTH sizeof(int16_t)

#define NSE_ORDERENTRY_REQUEST_USERID_OFFSET \
  (NSE_ORDERENTRY_REQUEST_TRANSACTIONCODE_OFFSET + NSE_ORDERENTRY_REQUEST_TRANSACTIONCODE_LENGTH)
#define NSE_ORDERENTRY_REQUEST_USERID_LENGTH sizeof(int32_t)

#define NSE_ORDERENTRY_REQUEST_REASONCODE_OFFSET \
  (NSE_ORDERENTRY_REQUEST_USERID_OFFSET + NSE_ORDERENTRY_REQUEST_USERID_LENGTH)
#define NSE_ORDERENTRY_REQUEST_REASONCODE_LENGTH sizeof(int16_t)

#define NSE_ORDERENTRY_REQUEST_TOKENNUM_OFFSET \
  (NSE_ORDERENTRY_REQUEST_REASONCODE_OFFSET + NSE_ORDERENTRY_REQUEST_REASONCODE_LENGTH)
#define NSE_ORDERENTRY_REQUEST_TOKENNUM_LENGTH sizeof(int32_t)

#define NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_OFFSET \
  (NSE_ORDERENTRY_REQUEST_TOKENNUM_OFFSET + NSE_ORDERENTRY_REQUEST_TOKENNUM_LENGTH)
#define NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_LENGTH \
  LENGTH_OF_NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_CHAR_FIELD

#define NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_OFFSET      \
  (NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_OFFSET + \
   NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_LENGTH)
#define NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_LENGTH \
  LENGTH_OF_NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_CHAR_FIELD

#define NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_EXPIRYDATE_OFFSET \
  (NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_OFFSET +        \
   NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_LENGTH)
#define NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_EXPIRYDATE_LENGTH sizeof(int32_t)

#define NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_STRIKEPRICE_OFFSET \
  (NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_EXPIRYDATE_OFFSET +     \
   NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_EXPIRYDATE_LENGTH)
#define NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_STRIKEPRICE_LENGTH sizeof(int32_t)

#define NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_OFFSET \
  (NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_STRIKEPRICE_OFFSET +   \
   NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_STRIKEPRICE_LENGTH)
#define NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_LENGTH 2

#define NSE_ORDERENTRY_REQUEST_ACCOUNTNUMBER_OFFSET                \
  (NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_OFFSET + \
   NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_LENGTH)
#define NSE_ORDERENTRY_REQUEST_ACCOUNTNUMBER_LENGTH LENGTH_OF_NSE_ORDERENTRY_REQUEST_ACCOUNTNUMBER_CHAR_FIELD

#define NSE_ORDERENTRY_REQUEST_BOOKTYPE_OFFSET \
  (NSE_ORDERENTRY_REQUEST_ACCOUNTNUMBER_OFFSET + NSE_ORDERENTRY_REQUEST_ACCOUNTNUMBER_LENGTH)
#define NSE_ORDERENTRY_REQUEST_BOOKTYPE_LENGTH sizeof(int16_t)

#define NSE_ORDERENTRY_REQUEST_BUYSELLINDICATOR_OFFSET \
  (NSE_ORDERENTRY_REQUEST_BOOKTYPE_OFFSET + NSE_ORDERENTRY_REQUEST_BOOKTYPE_LENGTH)
#define NSE_ORDERENTRY_REQUEST_BUYSELLINDICATOR_LENGTH sizeof(int16_t)

#define NSE_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_OFFSET \
  (NSE_ORDERENTRY_REQUEST_BUYSELLINDICATOR_OFFSET + NSE_ORDERENTRY_REQUEST_BUYSELLINDICATOR_LENGTH)
#define NSE_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_LENGTH sizeof(int32_t)

#define NSE_ORDERENTRY_REQUEST_VOLUME_OFFSET \
  (NSE_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_OFFSET + NSE_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_LENGTH)
#define NSE_ORDERENTRY_REQUEST_VOLUME_LENGTH sizeof(int32_t)

#define NSE_ORDERENTRY_REQUEST_PRICE_OFFSET \
  (NSE_ORDERENTRY_REQUEST_VOLUME_OFFSET + NSE_ORDERENTRY_REQUEST_VOLUME_LENGTH)
#define NSE_ORDERENTRY_REQUEST_PRICE_LENGTH sizeof(int32_t)

#define NSE_ORDERENTRY_REQUEST_GOODTILLDATE_OFFSET \
  (NSE_ORDERENTRY_REQUEST_PRICE_OFFSET + NSE_ORDERENTRY_REQUEST_PRICE_LENGTH)
#define NSE_ORDERENTRY_REQUEST_GOODTILLDATE_LENGTH sizeof(int32_t)

#define NSE_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET \
  (NSE_ORDERENTRY_REQUEST_GOODTILLDATE_OFFSET + NSE_ORDERENTRY_REQUEST_GOODTILLDATE_LENGTH)
#define NSE_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_LENGTH sizeof(int16_t)

#define NSE_ORDERENTRY_REQUEST_BRNACHID_OFFSET \
  (NSE_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET + NSE_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_LENGTH)
#define NSE_ORDERENTRY_REQUEST_BRNACHID_LENGTH sizeof(int16_t)

#define NSE_ORDERENTRY_REQUEST_TRADERID_OFFSET \
  (NSE_ORDERENTRY_REQUEST_BRNACHID_OFFSET + NSE_ORDERENTRY_REQUEST_BRNACHID_LENGTH)
#define NSE_ORDERENTRY_REQUEST_TRADERID_LENGTH sizeof(int32_t)

#define NSE_ORDERENTRY_REQUEST_BROKERID_OFFSET \
  (NSE_ORDERENTRY_REQUEST_TRADERID_OFFSET + NSE_ORDERENTRY_REQUEST_TRADERID_LENGTH)
#define NSE_ORDERENTRY_REQUEST_BROKERID_LENGTH LENGTH_OF_NSE_ORDERENTRY_REQUEST_BROKERID_CHAR_FIELD

#define NSE_ORDERENTRY_REQUEST_OPENCLOSE_OFFSET \
  (NSE_ORDERENTRY_REQUEST_BROKERID_OFFSET + NSE_ORDERENTRY_REQUEST_BROKERID_LENGTH)
#define NSE_ORDERENTRY_REQUEST_OPENCLOSE_LENGTH sizeof(char)

#define NSE_ORDERENTRY_REQUEST_SETTLOR_OFFSET \
  (NSE_ORDERENTRY_REQUEST_OPENCLOSE_OFFSET + NSE_ORDERENTRY_REQUEST_OPENCLOSE_LENGTH)
#define NSE_ORDERENTRY_REQUEST_SETTLOR_LENGTH LENGTH_OF_NSE_ORDERENTRY_REQUEST_SETTLOR_CHAR_FIELD

#define NSE_ORDERENTRY_REQUEST_PROCLIENTINDICATOR_OFFSET \
  (NSE_ORDERENTRY_REQUEST_SETTLOR_OFFSET + NSE_ORDERENTRY_REQUEST_SETTLOR_LENGTH)
#define NSE_ORDERENTRY_REQUEST_PROCLIENTINDICATOR_LENGTH sizeof(int16_t)

#define NSE_ORDERENTRY_REQUEST_ADDITIONALORDERFLAGS_STRUCT_OFFSET \
  (NSE_ORDERENTRY_REQUEST_PROCLIENTINDICATOR_OFFSET + NSE_ORDERENTRY_REQUEST_PROCLIENTINDICATOR_LENGTH)
#define NSE_ORDERENTRY_REQUEST_ADDITIONALORDERFLAGS_STRUCT_LENGTH sizeof(int8_t)

#define NSE_ORDERENTRY_REQUEST_PAD_OFFSET                      \
  (NSE_ORDERENTRY_REQUEST_ADDITIONALORDERFLAGS_STRUCT_OFFSET + \
   NSE_ORDERENTRY_REQUEST_ADDITIONALORDERFLAGS_STRUCT_LENGTH)
#define NSE_ORDERENTRY_REQUEST_PAD_LENGTH sizeof(int8_t)

#define NSE_ORDERENTRY_REQUEST_FILLER_OFFSET (NSE_ORDERENTRY_REQUEST_PAD_OFFSET + NSE_ORDERENTRY_REQUEST_PAD_LENGTH)
#define NSE_ORDERENTRY_REQUEST_FILLER_LENGTH sizeof(int32_t)

#define NSE_ORDERENTRY_REQUEST_NFFIELD_OFFSET \
  (NSE_ORDERENTRY_REQUEST_FILLER_OFFSET + NSE_ORDERENTRY_REQUEST_FILLER_LENGTH)
#define NSE_ORDERENTRY_REQUEST_NFFIELD_LENGTH sizeof(double)

#define NSE_ORDERENTRY_REQUEST_PAN_OFFSET \
  (NSE_ORDERENTRY_REQUEST_NFFIELD_OFFSET + NSE_ORDERENTRY_REQUEST_NFFIELD_LENGTH)
#define NSE_ORDERENTRY_REQUEST_PAN_LENGTH 10

#define NSE_ORDERENTRY_REQUEST_ALGOID_OFFSET (NSE_ORDERENTRY_REQUEST_PAN_OFFSET + NSE_ORDERENTRY_REQUEST_PAN_LENGTH)
#define NSE_ORDERENTRY_REQUEST_ALGOID_LENGTH 4

#define NSE_ORDERENTRY_REQUEST_ALGOCAT_OFFSET \
  (NSE_ORDERENTRY_REQUEST_ALGOID_OFFSET + NSE_ORDERENTRY_REQUEST_ALGOID_LENGTH)
#define NSE_ORDERENTRY_REQUEST_ALGOCAT_LENGTH 2

#define NSE_ORDERENTRY_REQUEST_STP_RESERVED_OFFSET \
  (NSE_ORDERENTRY_REQUEST_ALGOCAT_OFFSET + NSE_ORDERENTRY_REQUEST_ALGOCAT_LENGTH)
#define NSE_ORDERENTRY_REQUEST_STP_RESERVED_LENGTH 32

#define NSE_ORDERENTRY_REQUEST_LENGTH                                                                                 \
  (NSE_PACKET_REQUEST_LENGTH + NSE_ORDERENTRY_REQUEST_TRANSACTIONCODE_LENGTH + NSE_ORDERENTRY_REQUEST_USERID_LENGTH + \
   NSE_ORDERENTRY_REQUEST_REASONCODE_LENGTH + NSE_ORDERENTRY_REQUEST_TOKENNUM_LENGTH +                                \
   NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_LENGTH +                                                \
   NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_LENGTH +                                                        \
   NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_EXPIRYDATE_LENGTH +                                                    \
   NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_STRIKEPRICE_LENGTH +                                                   \
   NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_LENGTH + NSE_ORDERENTRY_REQUEST_ACCOUNTNUMBER_LENGTH +      \
   NSE_ORDERENTRY_REQUEST_BOOKTYPE_LENGTH + NSE_ORDERENTRY_REQUEST_BUYSELLINDICATOR_LENGTH +                          \
   NSE_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_LENGTH + NSE_ORDERENTRY_REQUEST_VOLUME_LENGTH +                             \
   NSE_ORDERENTRY_REQUEST_PRICE_LENGTH + NSE_ORDERENTRY_REQUEST_GOODTILLDATE_LENGTH +                                 \
   NSE_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_LENGTH + NSE_ORDERENTRY_REQUEST_BRNACHID_LENGTH +                       \
   NSE_ORDERENTRY_REQUEST_TRADERID_LENGTH + NSE_ORDERENTRY_REQUEST_BROKERID_LENGTH +                                  \
   NSE_ORDERENTRY_REQUEST_OPENCLOSE_LENGTH + NSE_ORDERENTRY_REQUEST_SETTLOR_LENGTH +                                  \
   NSE_ORDERENTRY_REQUEST_PROCLIENTINDICATOR_LENGTH + NSE_ORDERENTRY_REQUEST_ADDITIONALORDERFLAGS_STRUCT_LENGTH +     \
   NSE_ORDERENTRY_REQUEST_PAD_LENGTH + NSE_ORDERENTRY_REQUEST_FILLER_LENGTH + NSE_ORDERENTRY_REQUEST_NFFIELD_LENGTH + \
   48)

namespace HFSAT {
namespace NSE {

/*
 * Defines Order entry semantics for derivatives market
 * Extends generic orderEntry template for NSE trimmed structure.
 */

class OrderEntryRequestDerivatives : public OrderEntryRequest {
 public:
  void InitializeStaticFields() {
    //================================= @ Packet

    // Total Length Of The Packet To Be Sent
    SetPacketLength(NSE_ORDERENTRY_REQUEST_LENGTH);

    // ================================ @ Order Entry

    SetTransactionCode(BOARD_LOT_IN_TR);

    *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_STRIKEPRICE_OFFSET)) = hton32(-1);
    memset((void *)(msg_ptr + NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_OFFSET), 'X',
           NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_LENGTH);
    memset((void *)(msg_ptr + NSE_ORDERENTRY_REQUEST_ACCOUNTNUMBER_OFFSET), ' ',
           NSE_ORDERENTRY_REQUEST_ACCOUNTNUMBER_LENGTH);
    *((int16_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_BOOKTYPE_OFFSET)) = hton16(1);  // regular order
    *((int16_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) =
        hton16((int16_t)(1 << 11));  // day order

    *((char *)(msg_ptr + NSE_ORDERENTRY_REQUEST_OPENCLOSE_OFFSET)) = 'O';
    memset((void *)(msg_ptr + NSE_ORDERENTRY_REQUEST_SETTLOR_OFFSET), ' ', NSE_ORDERENTRY_REQUEST_SETTLOR_LENGTH);
    *((int16_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_PROCLIENTINDICATOR_OFFSET)) = hton16(2);
    *((int8_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_ADDITIONALORDERFLAGS_STRUCT_OFFSET)) =
        (int8_t)2;  // 00010010 ( BITS: 000 1 (Self Trade) 00 1 (COD) 0 )
    *((double *)(msg_ptr + NSE_ORDERENTRY_REQUEST_NFFIELD_OFFSET)) =
        (double)swap_endian((double)111111111111000);  // 00 56 BC 93 84 43 D9 42

    *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_ALGOID_OFFSET)) = hton32(0);

    // This has to be 0 - Algo category, this was actually made as reserved field by the exchange
    *((int16_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_ALGOCAT_OFFSET)) = hton16(0);
  }

 public:
  OrderEntryRequestDerivatives() {
    order_entry_request_buffer = (char *)calloc(NSE_ORDERENTRY_REQUEST_LENGTH, sizeof(char));
    msg_ptr = (order_entry_request_buffer + NSE_REQUEST_START_OFFSET);

    // Initialize The Static Fields
    InitializeStaticFields();
  }

  // =================================== Order Entry Field Setter Functions

  inline void SetTransactionCode(int16_t const &message_header_transaction_code) {
    *((int16_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_TRANSACTIONCODE_OFFSET)) = hton16(message_header_transaction_code);
  }

  inline void SetProClient(int16_t const &proclient){
    // ‘1’ represents the client’s order.
    // '2’ represents a broker’s order.
    *((int16_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_PROCLIENTINDICATOR_OFFSET)) = hton16(proclient); // default is 2
  }
  // For broker’s own order, this field should be set to the broker code.
  inline void UpdateAccountNumber(char const *broker_id) { // setAccountNumber SHould be blank for FO
    memcpy((void *)(msg_ptr + NSE_ORDERENTRY_REQUEST_ACCOUNTNUMBER_OFFSET), (void *)broker_id,
           NSE_ORDERENTRY_REQUEST_ACCOUNTNUMBER_LENGTH);
  }

  inline void SetSettlor(char const *settlor_) {
    memcpy((void *)(msg_ptr + NSE_ORDERENTRY_REQUEST_SETTLOR_OFFSET), (void *)settlor_,
           NSE_ORDERENTRY_REQUEST_SETTLOR_LENGTH);  // this field should be set to blank for a pro order
  }

  inline void SetTokenNumber(int32_t const &token_number) {
    *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_TOKENNUM_OFFSET)) = hton32(token_number);
  }

  /*
    inline void SetInstrumentDesc(char const *inst_desc) {
      memcpy((void *)(msg_ptr + NSE_ORDERENTRY_REQUEST_TOKENNUM_OFFSET), inst_desc, sizeof(InstrumentDesc));
    }
  */
  inline void SetContractDescInstrumentName(char const *instrument_name) {
    memcpy((void *)(msg_ptr + NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_OFFSET), instrument_name,
           NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_LENGTH);
  }

  inline void SetContractDescSymbol(char const *symbol) {
    memcpy((void *)(msg_ptr + NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_OFFSET), symbol,
           NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_LENGTH);
  }

  inline void SetContractDescExpiryDate(int32_t const &expiry) {
    *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_EXPIRYDATE_OFFSET)) = hton32(expiry);
  }

  inline void SetContractDescStrikePrice(int32_t const &strike) {
    *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_STRIKEPRICE_OFFSET)) = hton32(strike);
  }

  inline void SetContractDescOptionType(char const *option_type) {
    memcpy((void *)(msg_ptr + NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_OFFSET), option_type,
           NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_LENGTH);
  }

  /*
    inline void SetBuySell(int16_t const &buy_sell) {
      *((int16_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_BUYSELLINDICATOR_OFFSET)) = hton16(buy_sell);
    }

    inline void SetVolume(int32_t const &volume, int32_t const &disclosed_volume) {
      *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_OFFSET)) = hton32(disclosed_volume);
      *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_VOLUME_OFFSET)) = hton32(volume);
    }
    inline void SetPrice(int32_t const &price) {
      *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_PRICE_OFFSET)) = hton32(price);
    }
  */
  inline void SetBranchId(int16_t const &branch_id) {
    *((int16_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_BRNACHID_OFFSET)) = hton16(branch_id);
  }

  inline void SetTraderId(int32_t const &trader_id) {
    *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_TRADERID_OFFSET)) = hton32(trader_id);
    *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_USERID_OFFSET)) = hton32(trader_id);
  }

  inline void SetPan(char const *pan) {
    memcpy((void *)(msg_ptr + NSE_ORDERENTRY_REQUEST_PAN_OFFSET), pan, NSE_ORDERENTRY_REQUEST_PAN_LENGTH);
  }

  inline void SetBrokerId(char const *broker_id) {
    memcpy((void *)(msg_ptr + NSE_ORDERENTRY_REQUEST_BROKERID_OFFSET), (void *)broker_id,
           NSE_ORDERENTRY_REQUEST_BROKERID_LENGTH);
  }
  /*
    inline void SetSaos(int32_t const &saos) {
      *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_FILLER_OFFSET)) = hton32(saos);
    }
  */
  inline void SetBookType(int16_t const &book_type) {  // 1-RL, 3-SL, 5-Odd Lot
    *((int16_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_BOOKTYPE_OFFSET)) = hton16(book_type);
  }

  inline void SetNNF(double const &nnf) {  // 1-RL, 3-SL, 5-Odd Lot
    *((double *)(msg_ptr + NSE_ORDERENTRY_REQUEST_NFFIELD_OFFSET)) =
        (double)swap_endian((double)nnf);  // 00 56 BC 93 84 43 D9 42
  }

  inline void SetIOC() {
    int16_t flag = 1;
    flag = (flag << 9);
    *((int16_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) = hton16(flag);
  }
  /*
    inline void SetDay() {
      int16_t flag = 1;
      flag = (flag << 11);
      *((int16_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) = hton16(flag);
    }
  */
  inline void SetAlgoId(int32_t const &algo_id) {
    *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_ALGOID_OFFSET)) = hton32(algo_id);
  }
  /*
    inline void SetDynamicOrderEntryRequestFields(int32_t const &packet_sequence_number, int16_t const &buy_sell,
                                                  int32_t const &volume, int32_t const &disclosed_volume,
                                                  int32_t const &price, int32_t const &saos, bool is_ioc,
                                                  InstrumentDesc *inst_desc, SecurityInfoCashMarket *sec_info, bool
    is_mo = false, bool add_preopen = false) {
  */

  inline void SetDynamicOrderEntryRequestFields(int32_t const &packet_sequence_number, int16_t const &buy_sell,
                                                int32_t const &price, HFSAT::ORS::Order *order,
                                                InstrumentDesc *inst_desc, SecurityInfoCashMarket *sec_info,
                                                bool is_mo = false, bool add_preopen = false) {
    memcpy((void *)(msg_ptr + NSE_ORDERENTRY_REQUEST_TOKENNUM_OFFSET), inst_desc->GetInstrumentDescAsBuffer(),
           sizeof(InstrumentDesc));
    *((int32_t *)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)) = hton32(packet_sequence_number);
    *((int16_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_BUYSELLINDICATOR_OFFSET)) = hton16(buy_sell);
    *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_PRICE_OFFSET)) = hton32(price);
    *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_FILLER_OFFSET)) = hton32(order->server_assigned_order_sequence_);

    // SetInstrumentDesc(inst_desc->GetInstrumentDescAsBuffer());
    // SetPacketSequenceNumber(packet_sequence_number);
    // SetBuySell(buy_sell);
    // SetPrice(price);
    // SetSaos(saos);

    if (__builtin_expect(!order->is_ioc, true) && __builtin_expect(!is_mo, true)) {
      *((int16_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) = hton16(1 << 11);
      *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_OFFSET)) = hton32(order->size_disclosed_);
      *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_VOLUME_OFFSET)) = hton32(order->size_remaining_);
      // SetDay();
      // SetVolume(volume, disclosed_volume);
    } else if (__builtin_expect(is_mo, false)) {
      // market order needs DAY flag to be set and
      // disclosed volume needs to be set to 0
      *((int16_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) = hton16(1 << 11);
      *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_PRICE_OFFSET)) = hton32(0);
      *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_OFFSET)) = hton32(0);
      *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_VOLUME_OFFSET)) = hton32(order->size_remaining_);
      // SetDay();
      // SetPrice(0);
      // SetVolume(volume, 0);
    } else if (order->is_ioc) {
      *((int16_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) = hton16(1 << 9);
      // SetIOC();
      // disclosed volume needs to be set to 0, in case of ioc orders
      *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_OFFSET)) = hton32(0);
      *((int32_t *)(msg_ptr + NSE_ORDERENTRY_REQUEST_VOLUME_OFFSET)) = hton32(order->size_remaining_);
      // SetVolume(volume, 0);
    }
    if (AVX512_SUPPORTED)
    HFSAT::MD5::MD5_AVX512VL((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_ORDERENTRY_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
    else
    HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_ORDERENTRY_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
  }

  inline void SetDynamicOrderEntryRequestFields(int32_t const &packet_sequence_number, int16_t const &buy_sell_1,
                                                 int16_t const &buy_sell_2, int16_t const &buy_sell_3,
                                                 int32_t const &price_1,int32_t const &price_2,int32_t const &price_3,
                                                 HFSAT::ORS::Order *order_1, HFSAT::ORS::Order *order_2, HFSAT::ORS::Order *order_3,
                                                 InstrumentDesc *inst_desc_1,InstrumentDesc *inst_desc_2,InstrumentDesc *inst_desc_3,
                                                 SecurityInfoCashMarket *sec_info_1,SecurityInfoCashMarket *sec_info_2, SecurityInfoCashMarket *sec_info_3,
                                                 bool is_mo = false, bool add_preopen = false) {
    std::cout << "Empty Below.." << std::endl;

    }

  
  int32_t GetOrderEntryMsgLength() { return NSE_ORDERENTRY_REQUEST_LENGTH; }
};
}
}
