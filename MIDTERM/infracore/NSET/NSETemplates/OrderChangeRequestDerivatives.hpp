// =====================================================================================
//
//       Filename:  OrderChangeRequestDerivatives.hpp
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

#include "infracore/NSET/NSETemplates/OrderChangeRequest.hpp"
#include "dvccode/CDef/debug_logger.hpp"

#define NSE_CHANGE_REQUEST_TRANSACTIONCODE_OFFSET (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH)
#define NSE_CHANGE_REQUEST_TRANSACTION_LENGTH sizeof(int16_t)

#define NSE_CHANGE_REQUEST_USERID_OFFSET \
  (NSE_CHANGE_REQUEST_TRANSACTIONCODE_OFFSET + NSE_CHANGE_REQUEST_TRANSACTION_LENGTH)
#define NSE_CHANGE_REQUEST_USERID_LENGTH sizeof(int32_t)

#define NSE_CHANGE_REQUEST_MOD_CXL_BY_OFFSET (NSE_CHANGE_REQUEST_USERID_OFFSET + NSE_CHANGE_REQUEST_USERID_LENGTH)
#define NSE_CHANGE_REQUEST_MOD_CXL_BY_LENGTH sizeof(int8_t)

#define NSE_CHANGE_REQUEST_PAD_OFFSET (NSE_CHANGE_REQUEST_MOD_CXL_BY_OFFSET + NSE_CHANGE_REQUEST_MOD_CXL_BY_LENGTH)
#define NSE_CHANGE_REQUEST_PAD_LENGTH sizeof(int8_t)

#define NSE_CHANGE_REQUEST_TOKENNUM_OFFSET (NSE_CHANGE_REQUEST_PAD_OFFSET + NSE_CHANGE_REQUEST_PAD_LENGTH)
#define NSE_CHANGE_REQUEST_TOKENNUM_LENGTH sizeof(int32_t)

#define NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_OFFSET \
  (NSE_CHANGE_REQUEST_TOKENNUM_OFFSET + NSE_CHANGE_REQUEST_TOKENNUM_LENGTH)
#define NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_LENGTH \
  LENGTH_OF_NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_CHAR_FIELD

#define NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_OFFSET      \
  (NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_OFFSET + \
   NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_LENGTH)
#define NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_LENGTH \
  LENGTH_OF_NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_CHAR_FIELD

#define NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_EXPIRYDATE_OFFSET \
  (NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_OFFSET + NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_LENGTH)
#define NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_EXPIRYDATE_LENGTH sizeof(int32_t)

#define NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_STRIKEPRICE_OFFSET \
  (NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_EXPIRYDATE_OFFSET +     \
   NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_EXPIRYDATE_LENGTH)
#define NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_STRIKEPRICE_LENGTH sizeof(int32_t)

#define NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_OFFSET \
  (NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_STRIKEPRICE_OFFSET +   \
   NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_STRIKEPRICE_LENGTH)
#define NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_LENGTH 2

#define NSE_CHANGE_REQUEST_ORDERNUMBER_OFFSET                  \
  (NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_OFFSET + \
   NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_LENGTH)
#define NSE_CHANGE_REQUEST_ORDERNUMBER_LENGTH sizeof(double)

#define NSE_CHANGE_REQUEST_ACCOUNTNUMBER_OFFSET \
  (NSE_CHANGE_REQUEST_ORDERNUMBER_OFFSET + NSE_CHANGE_REQUEST_ORDERNUMBER_LENGTH)
#define NSE_CHANGE_REQUEST_ACCOUNTNUMBER_LENGTH LENGTH_OF_NSE_CHANGE_REQUEST_ACCOUNTNUMBER_CHAR_FIELD

#define NSE_CHANGE_REQUEST_BOOKTYPE_OFFSET \
  (NSE_CHANGE_REQUEST_ACCOUNTNUMBER_OFFSET + NSE_CHANGE_REQUEST_ACCOUNTNUMBER_LENGTH)
#define NSE_CHANGE_REQUEST_BOOKTYPE_LENGTH sizeof(int16_t)

#define NSE_CHANGE_REQUEST_BUYSELLINDICATOR_OFFSET \
  (NSE_CHANGE_REQUEST_BOOKTYPE_OFFSET + NSE_CHANGE_REQUEST_BOOKTYPE_LENGTH)
#define NSE_CHANGE_REQUEST_BUYSELLINDICATOR_LENGTH sizeof(int16_t)

#define NSE_CHANGE_REQUEST_DISCLOSEDVOLUME_OFFSET \
  (NSE_CHANGE_REQUEST_BUYSELLINDICATOR_OFFSET + NSE_CHANGE_REQUEST_BUYSELLINDICATOR_LENGTH)
#define NSE_CHANGE_REQUEST_DISCLOSEDVOLUME_LENGTH sizeof(int32_t)

#define NSE_CHANGE_REQUEST_DISCLOSEDVOLUMEREMAINING_OFFSET \
  (NSE_CHANGE_REQUEST_DISCLOSEDVOLUME_OFFSET + NSE_CHANGE_REQUEST_DISCLOSEDVOLUME_LENGTH)
#define NSE_CHANGE_REQUEST_DISCLOSEDVOLUMEREMAINING_LENGTH sizeof(int32_t)

#define NSE_CHANGE_REQUEST_TOTALVOLUMEREMAINING_OFFSET \
  (NSE_CHANGE_REQUEST_DISCLOSEDVOLUMEREMAINING_OFFSET + NSE_CHANGE_REQUEST_DISCLOSEDVOLUMEREMAINING_LENGTH)
#define NSE_CHANGE_REQUEST_TOTALVOLUMEREMAINING_LENGTH sizeof(int32_t)

#define NSE_CHANGE_REQUEST_VOLUME_OFFSET \
  (NSE_CHANGE_REQUEST_TOTALVOLUMEREMAINING_OFFSET + NSE_CHANGE_REQUEST_TOTALVOLUMEREMAINING_LENGTH)
#define NSE_CHANGE_REQUEST_VOLUME_LENGTH sizeof(int32_t)

#define NSE_CHANGE_REQUEST_VOLUMEFILLEDTODAY_OFFSET \
  (NSE_CHANGE_REQUEST_VOLUME_OFFSET + NSE_CHANGE_REQUEST_VOLUME_LENGTH)
#define NSE_CHANGE_REQUEST_VOLUMEFILLEDTODAY_LENGTH sizeof(int32_t)

#define NSE_CHANGE_REQUEST_PRICE_OFFSET \
  (NSE_CHANGE_REQUEST_VOLUMEFILLEDTODAY_OFFSET + NSE_CHANGE_REQUEST_VOLUMEFILLEDTODAY_LENGTH)
#define NSE_CHANGE_REQUEST_PRICE_LENGTH sizeof(int32_t)

#define NSE_CHANGE_REQUEST_GOODTILLDATE_OFFSET (NSE_CHANGE_REQUEST_PRICE_OFFSET + NSE_CHANGE_REQUEST_PRICE_LENGTH)
#define NSE_CHANGE_REQUEST_GOODTILLDATE_LENGTH sizeof(int32_t)

#define NSE_CHANGE_REQUEST_ENTRYDATETIME_OFFSET \
  (NSE_CHANGE_REQUEST_GOODTILLDATE_OFFSET + NSE_CHANGE_REQUEST_GOODTILLDATE_LENGTH)
#define NSE_CHANGE_REQUEST_ENTRYDATETIME_LENGTH sizeof(int32_t)

#define NSE_CHANGE_REQUEST_LASTMODIFIED_OFFSET \
  (NSE_CHANGE_REQUEST_ENTRYDATETIME_OFFSET + NSE_CHANGE_REQUEST_ENTRYDATETIME_LENGTH)
#define NSE_CHANGE_REQUEST_LASTMODIFIED_LENGTH sizeof(int32_t)

#define NSE_CHANGE_REQUEST_STORDERFLAGS_STRUCT_OFFSET \
  (NSE_CHANGE_REQUEST_LASTMODIFIED_OFFSET + NSE_CHANGE_REQUEST_LASTMODIFIED_LENGTH)
#define NSE_CHANGE_REQUEST_STORDERFLAGS_STRUCT_LENGTH sizeof(int16_t)

#define NSE_CHANGE_REQUEST_BRNACHID_OFFSET \
  (NSE_CHANGE_REQUEST_STORDERFLAGS_STRUCT_OFFSET + NSE_CHANGE_REQUEST_STORDERFLAGS_STRUCT_LENGTH)
#define NSE_CHANGE_REQUEST_BRNACHID_LENGTH sizeof(int16_t)

#define NSE_CHANGE_REQUEST_TRADERID_OFFSET (NSE_CHANGE_REQUEST_BRNACHID_OFFSET + NSE_CHANGE_REQUEST_BRNACHID_LENGTH)
#define NSE_CHANGE_REQUEST_TRADERID_LENGTH sizeof(int32_t)

#define NSE_CHANGE_REQUEST_BROKERID_OFFSET (NSE_CHANGE_REQUEST_TRADERID_OFFSET + NSE_CHANGE_REQUEST_TRADERID_LENGTH)
#define NSE_CHANGE_REQUEST_BROKERID_LENGTH LENGTH_OF_NSE_CHANGE_REQUEST_BROKERID_CHAR_FIELD

#define NSE_CHANGE_REQUEST_OPENCLOSE_OFFSET (NSE_CHANGE_REQUEST_BROKERID_OFFSET + NSE_CHANGE_REQUEST_BROKERID_LENGTH)
#define NSE_CHANGE_REQUEST_OPENCLOSE_LENGTH sizeof(char)

#define NSE_CHANGE_REQUEST_SETTLOR_OFFSET (NSE_CHANGE_REQUEST_OPENCLOSE_OFFSET + NSE_CHANGE_REQUEST_OPENCLOSE_LENGTH)
#define NSE_CHANGE_REQUEST_SETTLOR_LENGTH LENGTH_OF_NSE_CHANGE_REQUEST_SETTLOR_CHAR_FIELD

#define NSE_CHANGE_REQUEST_PROCLIENTINDICATOR_OFFSET \
  (NSE_CHANGE_REQUEST_SETTLOR_OFFSET + NSE_CHANGE_REQUEST_SETTLOR_LENGTH)
#define NSE_CHANGE_REQUEST_PROCLIENTINDICATOR_LENGTH sizeof(int16_t)

#define NSE_CHANGE_REQUEST_ADDITIONALORDERFLAGS_STRUCT_OFFSET \
  (NSE_CHANGE_REQUEST_PROCLIENTINDICATOR_OFFSET + NSE_CHANGE_REQUEST_PROCLIENTINDICATOR_LENGTH)
#define NSE_CHANGE_REQUEST_ADDITIONALORDERFLAGS_STRUCT_LENGTH sizeof(int8_t)

#define NSE_CHANGE_REQUEST_PAD_1_OFFSET \
  (NSE_CHANGE_REQUEST_ADDITIONALORDERFLAGS_STRUCT_OFFSET + NSE_CHANGE_REQUEST_ADDITIONALORDERFLAGS_STRUCT_LENGTH)
#define NSE_CHANGE_REQUEST_PAD_1_LENGTH sizeof(int8_t)

#define NSE_CHANGE_REQUEST_FILLER_OFFSET (NSE_CHANGE_REQUEST_PAD_1_OFFSET + NSE_CHANGE_REQUEST_PAD_1_LENGTH)
#define NSE_CHANGE_REQUEST_FILLER_LENGTH sizeof(int32_t)

#define NSE_CHANGE_REQUEST_NFFIELD_OFFSET (NSE_CHANGE_REQUEST_FILLER_OFFSET + NSE_CHANGE_REQUEST_FILLER_LENGTH)
#define NSE_CHANGE_REQUEST_NFFIELD_LENGTH sizeof(double)

#define NSE_CHANGE_REQUEST_PAN_OFFSET (NSE_CHANGE_REQUEST_NFFIELD_OFFSET+NSE_CHANGE_REQUEST_NFFIELD_LENGTH)
#define NSE_CHANGE_REQUEST_PAN_LENGTH 10

#define NSE_CHANGE_REQUEST_ALGOID_OFFSET (NSE_CHANGE_REQUEST_PAN_OFFSET+NSE_CHANGE_REQUEST_PAN_LENGTH)
#define NSE_CHANGE_REQUEST_ALGOID_LENGTH 4 

#define NSE_CHANGE_REQUEST_ALGOCAT_OFFSET (NSE_CHANGE_REQUEST_ALGOID_OFFSET+NSE_CHANGE_REQUEST_ALGOID_LENGTH)
#define NSE_CHANGE_REQUEST_ALGOCAT_LENGTH 2

#define NSE_CHANGE_REQUEST_LASTACTIVITYREFERENCE_OFFSET (NSE_CHANGE_REQUEST_ALGOCAT_OFFSET + NSE_CHANGE_REQUEST_ALGOCAT_LENGTH)
#define NSE_CHANGE_REQUEST_LASTACTIVITYREFERENCE_LENGTH sizeof(int64_t)

#define NSE_CHANGE_REQUEST_STP_RESERVED_OFFSET (NSE_CHANGE_REQUEST_LASTACTIVITYREFERENCE_OFFSET + NSE_CHANGE_REQUEST_LASTACTIVITYREFERENCE_LENGTH)
#define NSE_CHANGE_REQUEST_STP_RESERVED_LENGTH 24

#define NSE_CHANGE_REQUEST_LENGTH (NSE_CHANGE_REQUEST_STP_RESERVED_OFFSET+NSE_CHANGE_REQUEST_STP_RESERVED_LENGTH)

namespace HFSAT {
namespace NSE {

/*
 * Defines Order cancel semantics for derivatives market
 * Extends generic orderCancel template for NSE trimmed structure.
 */

class OrderCancelRequestDerivatives : public OrderCancelRequest {
 public:
  void InitializeStaticFields() {
    //================================= @ Packet

    // Total Length Of The Packet To Be Sent
    SetPacketLength(NSE_CHANGE_REQUEST_LENGTH);

    // ================================ @ Order Cancel

    SetTransactionCode(ORDER_CANCEL_IN_TR);
    *((char *)(msg_ptr + NSE_CHANGE_REQUEST_MOD_CXL_BY_OFFSET)) = 'T';

    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_STRIKEPRICE_OFFSET)) = hton32(-1);
    memset((void *)(msg_ptr + NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_OFFSET), 'X',
           NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_LENGTH);
    memset((void *)(msg_ptr + NSE_CHANGE_REQUEST_ACCOUNTNUMBER_OFFSET), ' ', NSE_CHANGE_REQUEST_ACCOUNTNUMBER_LENGTH);
    *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_BOOKTYPE_OFFSET)) = hton16(1);  // regular order
    *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) = hton16((int16_t)(1 << 11));  // day order
    // *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_BRNACHID_OFFSET)) = hton16(1);

    *((char *)(msg_ptr + NSE_CHANGE_REQUEST_OPENCLOSE_OFFSET)) = 'O';
    memset((void *)(msg_ptr + NSE_CHANGE_REQUEST_SETTLOR_OFFSET), ' ', NSE_CHANGE_REQUEST_SETTLOR_LENGTH);
    *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_PROCLIENTINDICATOR_OFFSET)) = hton16(2);
    *((int8_t *)(msg_ptr + NSE_CHANGE_REQUEST_ADDITIONALORDERFLAGS_STRUCT_OFFSET)) =
        (int8_t)18;  // 00010010 ( BITS: 000 1 (Self Trade) 00 1 (COD) 0 )
    *((double *)(msg_ptr + NSE_CHANGE_REQUEST_NFFIELD_OFFSET)) =
        (double)swap_endian((double)111111111111000);  // 00 56 BC 93 84 43 D9 42

    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_ALGOID_OFFSET)) = 0;

    //This has to be 0 - Algo category, this was actually made as reserved field by the exchange
    *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_ALGOCAT_OFFSET)) = 0;


  }

 public:
  OrderCancelRequestDerivatives() {
    order_change_request_buffer = (char *)calloc(NSE_CHANGE_REQUEST_LENGTH, sizeof(char));
    msg_ptr = (order_change_request_buffer + NSE_REQUEST_START_OFFSET);

    // Initialize The Static Fields
    InitializeStaticFields();
  }

  // =================================== Order Cancel Field Setter Functions

  inline void SetTransactionCode(int16_t const &message_header_transaction_code) {
    *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_TRANSACTIONCODE_OFFSET)) = hton16(message_header_transaction_code);
  }

  inline void SetTokenNumber(int32_t const &token_number) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_TOKENNUM_OFFSET)) = hton32(token_number);
  }

  inline void SetInstrumentDesc(char const *inst_desc) {
    memcpy((void *)(msg_ptr + NSE_CHANGE_REQUEST_TOKENNUM_OFFSET), inst_desc, sizeof(InstrumentDesc));
  }
  
  inline void SetBranchId(int16_t const &branch_id) {
    *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_BRNACHID_OFFSET)) = hton16(branch_id);
  }

  inline void SetContractDescInstrumentName(char const *instrument_name) {
    memcpy((void *)(msg_ptr + NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_OFFSET), instrument_name,
           NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_LENGTH);
  }

  inline void SetContractDescSymbol(char const *symbol) {
    memcpy((void *)(msg_ptr + NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_OFFSET), symbol,
           NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_LENGTH);
  }

  inline void SetContractDescExpiryDate(int32_t const &expiry) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_EXPIRYDATE_OFFSET)) = hton32(expiry);
  }

  inline void SetContractDescStrikePrice(int32_t const &strike) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_STRIKEPRICE_OFFSET)) = hton32(strike);
  }

  inline void SetContractDescOptionType(char const *option_type) {
    memcpy((void *)(msg_ptr + NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_OFFSET), option_type,
           NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_LENGTH);
  }

  inline void SetBuySell(int16_t const &buy_sell) {
    *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_BUYSELLINDICATOR_OFFSET)) = hton16(buy_sell);
  }

  inline void SetVolume(int32_t const &volume) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_DISCLOSEDVOLUME_OFFSET)) = hton32(volume);
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_VOLUME_OFFSET)) = hton32(volume);
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_DISCLOSEDVOLUMEREMAINING_OFFSET)) = hton32(volume);
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_TOTALVOLUMEREMAINING_OFFSET)) = hton32(volume);
  }

  inline void SetPrice(int32_t const &price) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_PRICE_OFFSET)) = hton32(price);
  }

  inline void SetEntryDateTime(int32_t const &entry_date) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_ENTRYDATETIME_OFFSET)) = hton32(entry_date);
  }

  inline void SetLastModifiedDateTime(int32_t const &modified_date) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_LASTMODIFIED_OFFSET)) = hton32(modified_date);
  }

  inline void SetModified() {
    int16_t flag = 1;
    flag = (flag << 3);
    *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) |= hton16(flag);
  }

  inline void SetTraderId(int32_t const &trader_id) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_TRADERID_OFFSET)) = hton32(trader_id);
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_USERID_OFFSET)) = hton32(trader_id);
  }

  inline void SetPan(char const *pan) {
   memcpy((void *)(msg_ptr + NSE_CHANGE_REQUEST_PAN_OFFSET), pan, NSE_CHANGE_REQUEST_PAN_LENGTH); 
  }
 
  inline void SetBrokerId(char const *broker_id) {
    memcpy((void *)(msg_ptr + NSE_CHANGE_REQUEST_BROKERID_OFFSET), (void *)broker_id,
           NSE_CHANGE_REQUEST_BROKERID_LENGTH);
  }

  inline void SetOrderNumber(int64_t const &order_num) {
    *((int64_t *)(msg_ptr + NSE_CHANGE_REQUEST_ORDERNUMBER_OFFSET)) = (int64_t)ntoh64(order_num);
  }

  inline void SetSaos(int32_t const &saos) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_FILLER_OFFSET)) = hton32(saos);
  }

  inline void SetNNF(double const &nnf) {  // 1-RL, 3-SL, 5-Odd Lot
    *((double *)(msg_ptr + NSE_CHANGE_REQUEST_NFFIELD_OFFSET)) =
        (double)swap_endian((double)nnf);  // 00 56 BC 93 84 43 D9 42
  }

  inline void SetLastActivityReference(int64_t &last_Activity_ref) {
    *((int64_t*)(msg_ptr + NSE_CHANGE_REQUEST_LASTACTIVITYREFERENCE_OFFSET)) = 
		(int64_t)hton64(last_Activity_ref);
  }

  inline void SetAlgoId(int32_t const & algo_id){
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_ALGOID_OFFSET)) = hton32(algo_id);
  }

  inline void SetDynamicOrderCancelRequestFields(int32_t const &packet_sequence_number, int64_t const &order_num,
                                                 int32_t const &saos, int32_t const &entry_date,
                                                 int32_t const &last_date, int16_t const &buy_sell,
                                                 int32_t const &volume, int32_t const &disclosed_volume,
                                                 int32_t const &price, InstrumentDesc *inst_desc, 
					 	 int64_t last_activity_reference_, bool add_preopen = false) {
    SetInstrumentDesc(inst_desc->GetInstrumentDescAsBuffer());
    SetPacketSequenceNumber(packet_sequence_number);
    SetOrderNumber(order_num);
    SetEntryDateTime(entry_date);
    SetLastModifiedDateTime(last_date);
    SetBuySell(buy_sell);
    SetVolume(volume);
    SetPrice(price);
    SetSaos(saos);
	SetLastActivityReference(last_activity_reference_);
    HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_CHANGE_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
  }
  int32_t GetOrderCancelMsgLength() { return NSE_CHANGE_REQUEST_LENGTH; }
};

// ------------------------------------- Order Modify Request -------------------------------

/*
 * Defines Order modify semantics for derivatives market
 * Extends generic orderModify template for NSE trimmed structure.
 */

class OrderModifyRequestDerivatives : public OrderModifyRequest {
 public:
  void InitializeStaticFields() {
    //================================= @ Packet

    // Total Length Of The Packet To Be Sent
    SetPacketLength(NSE_CHANGE_REQUEST_LENGTH);

    // ================================ @ Order Modify

    SetTransactionCode(ORDER_MOD_IN_TR);
    *((char *)(msg_ptr + NSE_CHANGE_REQUEST_MOD_CXL_BY_OFFSET)) = 'T';

    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_STRIKEPRICE_OFFSET)) = hton32(-1);
    memset((void *)(msg_ptr + NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_OFFSET), 'X',
           NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_LENGTH);
    memset((void *)(msg_ptr + NSE_CHANGE_REQUEST_ACCOUNTNUMBER_OFFSET), ' ', NSE_CHANGE_REQUEST_ACCOUNTNUMBER_LENGTH);
    *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_BOOKTYPE_OFFSET)) = hton16(1);  // regular order
    *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) = hton16((int16_t)(1 << 11));  // day order
    // *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_BRNACHID_OFFSET)) = hton16(1);

    *((char *)(msg_ptr + NSE_CHANGE_REQUEST_OPENCLOSE_OFFSET)) = 'O';
    memset((void *)(msg_ptr + NSE_CHANGE_REQUEST_SETTLOR_OFFSET), ' ', NSE_CHANGE_REQUEST_SETTLOR_LENGTH);
    *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_PROCLIENTINDICATOR_OFFSET)) = hton16(2);
    *((int8_t *)(msg_ptr + NSE_CHANGE_REQUEST_ADDITIONALORDERFLAGS_STRUCT_OFFSET)) =
        (int8_t)18;  // 00010010 ( BITS: 000 1 (Self Trade) 00 1 (COD) 0 )
    *((double *)(msg_ptr + NSE_CHANGE_REQUEST_NFFIELD_OFFSET)) =
        (double)swap_endian((double)111111111111000);  // 00 56 BC 93 84 43 D9 42

    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_ALGOID_OFFSET)) = 0;

    //This has to be 0 - Algo category, this was actually made as reserved field by the exchange
    *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_ALGOCAT_OFFSET)) = 0;

  }

 public:
  OrderModifyRequestDerivatives() {
    order_change_request_buffer = (char *)calloc(NSE_CHANGE_REQUEST_LENGTH, sizeof(char));
    msg_ptr = (order_change_request_buffer + NSE_REQUEST_START_OFFSET);

    // Initialize The Static Fields
    InitializeStaticFields();
  }

  // =================================== Order Entry Field Setter Functions

  inline void SetTransactionCode(int16_t const &message_header_transaction_code) {
    *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_TRANSACTIONCODE_OFFSET)) = hton16(message_header_transaction_code);
  }

  inline void SetTokenNumber(int32_t const &token_number) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_TOKENNUM_OFFSET)) = hton32(token_number);
  }

  inline void SetInstrumentDesc(char const *inst_desc) {
    memcpy((void *)(msg_ptr + NSE_CHANGE_REQUEST_TOKENNUM_OFFSET), inst_desc, sizeof(InstrumentDesc));
  }

  inline void SetContractDescInstrumentName(char const *instrument_name) {
    memcpy((void *)(msg_ptr + NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_OFFSET), instrument_name,
           NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_LENGTH);
  }

  inline void SetContractDescSymbol(char const *symbol) {
    memcpy((void *)(msg_ptr + NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_OFFSET), symbol,
           NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_LENGTH);
  }

  inline void SetContractDescExpiryDate(int32_t const &expiry) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_EXPIRYDATE_OFFSET)) = hton32(expiry);
  }

  inline void SetContractDescStrikePrice(int32_t const &strike) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_STRIKEPRICE_OFFSET)) = hton32(strike);
  }
  
  inline void SetBranchId(int16_t const &branch_id) {
    *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_BRNACHID_OFFSET)) = hton16(branch_id);
  }

  inline void SetContractDescOptionType(char const *option_type) {
    memcpy((void *)(msg_ptr + NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_OFFSET), option_type,
           NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_LENGTH);
  }

  inline void SetBuySell(int16_t const &buy_sell) {
    *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_BUYSELLINDICATOR_OFFSET)) = hton16(buy_sell);
  }

  inline void SetVolume(int32_t const &volume, int32_t const &disclosed_volume) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_DISCLOSEDVOLUME_OFFSET)) = hton32(disclosed_volume);
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_VOLUME_OFFSET)) = hton32(volume);
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_DISCLOSEDVOLUMEREMAINING_OFFSET)) = hton32(disclosed_volume);
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_TOTALVOLUMEREMAINING_OFFSET)) = hton32(volume);
  }

  inline void SetTradedVolume(int32_t const &traded_volume) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_VOLUMEFILLEDTODAY_OFFSET)) = hton32(traded_volume);
  }

  inline void SetPrice(int32_t const &price) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_PRICE_OFFSET)) = hton32(price);
  }

  inline void SetEntryDateTime(int32_t const &entry_date) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_ENTRYDATETIME_OFFSET)) = hton32(entry_date);
  }

  inline void SetLastModifiedDateTime(int32_t const &modified_date) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_LASTMODIFIED_OFFSET)) = hton32(modified_date);
  }

  // Big endian protocol works: dont know why
  inline void SetModified() {
    int16_t flag = 1;
    flag = (flag << 4);
    *((int16_t *)(msg_ptr + NSE_CHANGE_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) |= hton16(flag);
  }

  inline void SetTraderId(int32_t const &trader_id) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_TRADERID_OFFSET)) = hton32(trader_id);
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_USERID_OFFSET)) = hton32(trader_id);
  }
 
  inline void SetPan(char const *pan) {
    memcpy((void *)(msg_ptr + NSE_CHANGE_REQUEST_PAN_OFFSET), pan, NSE_CHANGE_REQUEST_PAN_LENGTH); 
  }  

  inline void SetBrokerId(char const *broker_id) {
    memcpy((void *)(msg_ptr + NSE_CHANGE_REQUEST_BROKERID_OFFSET), (void *)broker_id,
           NSE_CHANGE_REQUEST_BROKERID_LENGTH);
  }

  inline void SetOrderNumber(int64_t const &order_num) {
    *((int64_t *)(msg_ptr + NSE_CHANGE_REQUEST_ORDERNUMBER_OFFSET)) = (int64_t)ntoh64(order_num);
  }

  inline void SetSaos(int32_t const &saos) {
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_FILLER_OFFSET)) = hton32(saos);
  }

  inline void SetNNF(double const &nnf) {  // 1-RL, 3-SL, 5-Odd Lot
    *((double *)(msg_ptr + NSE_CHANGE_REQUEST_NFFIELD_OFFSET)) =
        (double)swap_endian((double)nnf);  // 00 56 BC 93 84 43 D9 42
  }

  inline void SetLastActivityReference(int64_t &last_activity_ref) {
    *((int64_t*)(msg_ptr + NSE_CHANGE_REQUEST_LASTACTIVITYREFERENCE_OFFSET)) = 
		(int64_t)hton64(last_activity_ref);	
  }

  inline void SetAlgoId(int32_t const & algo_id){
    *((int32_t *)(msg_ptr + NSE_CHANGE_REQUEST_ALGOID_OFFSET)) = hton32(algo_id);
  }

  inline void SetDynamicOrderModifyRequestFields(int32_t const &packet_sequence_number, int64_t const &order_num,
                                                 int32_t saos, int32_t const &entry_date, int32_t const &last_date,
                                                 int16_t const &buy_sell, int32_t const &volume,
                                                 int32_t const &disclosed_volume, int32_t const &price,
                                                 int32_t const &traded_volume, InstrumentDesc *inst_desc, 
						 int64_t last_activity_reference_, bool add_preopen = false) {
    SetInstrumentDesc(inst_desc->GetInstrumentDescAsBuffer());
    SetPacketSequenceNumber(packet_sequence_number);
    SetOrderNumber(order_num);
    SetEntryDateTime(entry_date);
    SetLastModifiedDateTime(last_date);
    SetBuySell(buy_sell);
    SetVolume(volume, disclosed_volume);
    SetTradedVolume(traded_volume);
    SetPrice(price);
    SetSaos(saos);
	SetLastActivityReference(last_activity_reference_);
    HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_CHANGE_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
  }
  int32_t GetOrderModifyMsgLength() { return NSE_CHANGE_REQUEST_LENGTH; }
};
}
}
