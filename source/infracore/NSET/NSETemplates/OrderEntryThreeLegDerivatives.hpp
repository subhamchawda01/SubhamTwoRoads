// =====================================================================================
//
//       Filename:  SpreadOrderEntry.hpp
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
#include "dvccode/CDef/basic_ors_defines.hpp"
#include "infracore/NSET/NSETemplates/DataDefines.hpp"
#include "infracore/NSET/NSETemplates/SpreadOrderEntry.hpp"

namespace HFSAT {
namespace NSE {

class OrderEntryThreeLegRequestDerivatives {
 private:
  char order_entry_request_buffer[NSE_SPREAD_ORDERENTRY_REQUEST_LENGTH];
  char const *msg_ptr;

 private:
  void InitializeStaticFields() {
    //================================= @ Packet

    // Total Length Of The Packet To Be Sent
    SetPacketLength(NSE_SPREAD_ORDERENTRY_REQUEST_LENGTH);

    // Reserved Sequence is always filled with 0
    //    SetPacketReservedSequenceNumber(0);

    // Message Count is 1 as we are only sending one order message
    //    SetPacketMessageCount(1);

    //================================= @ Message Header

    // Login Transaction Code
    SetMessageHeaderTransactionCode(THRL_BOARD_LOT_IN);

    // LogTime Shoudl Be Set To 0 While Sending To Exch
    SetMessageHeaderLogTime(0);

    // Alphachar should be set to blank ( ' ' ) while sending to host
    SetMessageHeaderAlphaChar("  ");

    // ErrorCode Should be 0 While Sending To Exch
    SetMessageHeaderErrorCode(0);

    // All TimeStamp Fields Should Be Set To Numeric 0 While Sending To Exch
    memset((void *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TIMESTAMP_OFFSET), 0,
           NSE_REQUEST_MESSAGE_HEADER_TIMESTAMP_LENGTH + NSE_REQUEST_MESSAGE_HEADER_TIMESTAMP1_LENGTH +
               NSE_REQUEST_MESSAGE_HEADER_TIMESTAMP2_LENGTH);

    // Length of the message header and the following order entry request, hence packet length has been deducted
    *((int16_t *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_MESSAGELENGTH_OFFSET)) =
        hton16(NSE_SPREAD_ORDERENTRY_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH);

    // ================================ @ Order Entry

    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_CALEVEL_OFFSET)) = hton16(0);
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_CONTRACT_DESC_STRUCT_CALEVEL_OFFSET)) =
        hton16(0);
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_3_CONTRACT_DESC_STRUCT_CALEVEL_OFFSET)) =
        hton16(0);
    memset((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_OPBROKERID_OFFSET), ' ',
           NSE_SPREAD_ORDERENTRY_REQUEST_OPBROKERID_LENGTH);
    memset((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_OPBROKERID_OFFSET), ' ',
           NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_OPBROKERID_LENGTH);
    memset((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_3_OPBROKERID_OFFSET), ' ',
           NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_3_OPBROKERID_LENGTH);
    //        *( ( char * ) ( msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_CLOSEOUTFLAG_OFFSET ) ) = ' ' ;
    memset((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_ORDERTYPE_OFFSET), ' ',
           NSE_SPREAD_ORDERENTRY_REQUEST_ORDERTYPE_LENGTH);
    memset((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_ORDERNUMBER_OFFSET), ' ',
           NSE_SPREAD_ORDERENTRY_REQUEST_ORDERNUMBER_LENGTH);
    memset((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_ACCOUNTNUMBER_OFFSET), ' ',
           NSE_SPREAD_ORDERENTRY_REQUEST_ACCOUNTNUMBER_LENGTH);

    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_BOOKTYPE_OFFSET)) = hton16(1);  // Normal order
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) =
        hton16((int16_t)(1 << 11));  // day order
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_STORDERFLAGS_STRUCT_OFFSET)) =
        hton16((int16_t)(1 << 11));  // day order
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_3_STORDERFLAGS_STRUCT_OFFSET)) =
        hton16((int16_t)(1 << 11));  // day order
    memset((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_CORDFILLER_OFFSET), ' ',
           NSE_SPREAD_ORDERENTRY_REQUEST_CORDFILLER_LENGTH);
    *((char *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_OPENCLOSE_OFFSET)) = 'O';
    *((char *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_OPENCLOSE_OFFSET)) = 'O';
    *((char *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_3_OPENCLOSE_OFFSET)) = 'O';
    memset((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_SETTLOR_OFFSET), ' ',
           NSE_SPREAD_ORDERENTRY_REQUEST_SETTLOR_LENGTH);
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_PROCLIENTINDICATOR_OFFSET)) = hton16(2);
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_SETTLEMENTPERIOD_OFFSET)) = hton16(10);
   // *((int8_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_ADDITIONALORDERFLAGS_STRUCT_OFFSET)) = (int8_t)(1 << 1);  // 0
   // *((int8_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_ADDITIONALORDERFLAGS_STRUCT_OFFSET)) =
   //  (int8_t)18;  // 00010010 ( BITS: 000 1 (Self Trade) 00 1 (COD) 0 )
    *((double *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_NFFIELD_OFFSET)) =
        (double)swap_endian((double)111111111111000);  // 00 56 BC 93 84 43 D9 42
  }

 public:
  OrderEntryThreeLegRequestDerivatives() : msg_ptr(order_entry_request_buffer + NSE_REQUEST_START_OFFSET) {
    // Initialize OrderEntry Request Buffer
    memset((void *)order_entry_request_buffer, 0, NSE_SPREAD_ORDERENTRY_REQUEST_LENGTH);

    // Initialize The Static Fields
    InitializeStaticFields();
  }
  
 private:
  inline void SetPacketLength(int16_t const &packet_length) {
    *((int16_t *)(msg_ptr + NSE_PACKET_LENGTH_OFFSET)) = hton16(packet_length);
  }

  inline void SetPacketSequenceNumber(int32_t const &packet_sequence_number) {
    *((int32_t *)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)) = hton32(packet_sequence_number);
  }

  //  inline void SetPacketReservedSequenceNumber(int16_t const &packet_reserved_sequence_number) {
  //    *((int16_t *)(msg_ptr + NSE_PACKET_RESERVED_SEQUENCE_NUMBER_OFFSET)) = hton16(packet_reserved_sequence_number);
  //  }

  inline void SetPacketMD5CheckSum(char const *packet_md5sum_checksum) {
    memcpy((void *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET), packet_md5sum_checksum, NSE_PACKET_CHECKSUM_LENGTH);
  }

  //  inline void SetPacketMessageCount(int16_t const &packet_message_count) {
  //    *((int16_t *)(msg_ptr + NSE_PACKET_MESSAGE_COUNT_OFFSET)) = hton16(packet_message_count);
  //  }

  inline void SetMessageHeaderTransactionCode(int16_t const &message_header_transaction_code) {
    *((int16_t *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET)) =
        hton16(message_header_transaction_code);
  }

  inline void SetMessageHeaderLogTime(int32_t const &message_header_logtime) {
    *((int32_t *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_LOGTIME_OFFSET)) = hton32(message_header_logtime);
  }

  inline void SetMessageHeaderAlphaChar(char const *message_header_alpha_char) {
    memcpy((void *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_ALPHACHAR_OFFSET), message_header_alpha_char,
           NSE_REQUEST_MESSAGE_HEADER_ALPHACHAR_LENGTH);
  }

  inline void SetMessageHeaderTraderId(int32_t const &message_header_trader_id) {
    *((int32_t *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRADERID_OFFSET)) = hton32(message_header_trader_id);
  }

  inline void SetMessageHeaderErrorCode(int16_t const &message_header_error_code) {
    *((int16_t *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_ERRORCODE_OFFSET)) = hton16(message_header_error_code);
  }

  // =================================== Order Entry Field Setter Functions

  inline void SetTokenNumber(int32_t const &token_number) {
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_TOKENNO_OFFSET)) = hton32(token_number);
  }

  inline void SetInstrumentDesc(char const *inst_desc) {
    memcpy((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_TOKENNO_OFFSET), inst_desc, sizeof(InstrumentDesc));
  }

  inline void SetContractDescInstrumentName(char const *instrument_name) {
    memcpy((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_OFFSET),
           instrument_name, NSE_SPREAD_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_LENGTH);
  }

  inline void SetContractDescSymbol(char const *symbol) {
    memcpy((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_OFFSET), symbol,
           NSE_SPREAD_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_LENGTH);
  }

  inline void SetContractDescExpiryDate(int32_t const &expiry) {
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_EXPIRYDATE_OFFSET)) = hton32(expiry);
  }

  inline void SetContractDescStrikePrice(int32_t const &strike) {
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_STRIKEPRICE_OFFSET)) = hton32(strike);
  }

  inline void SetContractDescOptionType(char const *option_type) {
    memcpy((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_OFFSET), option_type,
           NSE_SPREAD_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_OPTIONTYPE_LENGTH);
  }

  inline void SetTokenNumber2(int32_t const &token_number) {
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_TOKENNO_OFFSET)) = hton32(token_number);
  }

  inline void SetInstrumentDesc2(char const *inst_desc) {
    memcpy((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_TOKENNO_OFFSET), inst_desc,
           sizeof(InstrumentDesc));
  }

  inline void SetContractDescInstrumentName2(char const *instrument_name) {
    memcpy((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_OFFSET),
           instrument_name, NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_LENGTH);
  }

  inline void SetContractDescSymbol2(char const *symbol) {
    memcpy((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_CONTRACT_DESC_STRUCT_SYMBOL_OFFSET), symbol,
           NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_CONTRACT_DESC_STRUCT_SYMBOL_LENGTH);
  }

  inline void SetContractDescExpiryDate2(int32_t const &expiry) {
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_CONTRACT_DESC_STRUCT_EXPIRYDATE_OFFSET)) =
        hton32(expiry);
  }

  inline void SetContractDescStrikePrice2(int32_t const &strike) {
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_CONTRACT_DESC_STRUCT_STRIKEPRICE_OFFSET)) =
        hton32(strike);
  }

  inline void SetContractDescOptionType2(char const *option_type) {
    memcpy((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_CONTRACT_DESC_STRUCT_OPTIONTYPE_OFFSET),
           option_type, NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_CONTRACT_DESC_STRUCT_OPTIONTYPE_LENGTH);
  }

  inline void SetBuySell(int16_t const &buy_sell, int16_t const &buy_sell_2) {
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_BUYSELLINDICATOR_OFFSET)) = hton16(buy_sell);
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_BUYSELLINDICATOR_OFFSET)) = hton16(buy_sell_2);
  }

  inline void SetVolume(int32_t const &volume, int32_t const &volume_2) {
    //        *( ( int32_t * ) ( msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_OFFSET) ) = hton32 ( volume ) ;
    //        *( ( int32_t * ) ( msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_DISCLOSEDVOLUMEREMAINING_OFFSET) ) = hton32 (
    //        volume ) ;
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_TOTALVOLUMEREMAINING_OFFSET)) = hton32(volume);
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_VOLUME_OFFSET)) = hton32(volume);

    //        *( ( int32_t * ) ( msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_DISCLOSEDVOLUME_OFFSET) ) = hton32
    //        ( volume ) ;
    //        *( ( int32_t * ) ( msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_DISCLOSEDVOLUMEREMAINING_OFFSET) )
    //        = hton32 ( volume ) ;
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_TOTALVOLUMEREMAINING_OFFSET)) = hton32(volume_2);
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_VOLUME_OFFSET)) = hton32(volume_2);
  }

  inline void SetPrice(int32_t const &price) {
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_PRICEDIFF_OFFSET)) = hton32(price);
  }

  inline void SetModified() {
    int16_t flag = 1;
    flag = (flag << 3);
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) |= hton16(flag);
  }

  inline void SetBranchId(int16_t const &branch_id) {
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_BRNACHID_OFFSET)) = hton16(branch_id);
  }

  inline void SetTraderId(int32_t const &trader_id) {
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_TRADERID_OFFSET)) = hton32(trader_id);
  }

  inline void SetBrokerId(char const *broker_id) {
    memcpy((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_BROKERID_OFFSET), (void *)broker_id,
           NSE_SPREAD_ORDERENTRY_REQUEST_BROKERID_LENGTH);
  }

  inline void SetSaos(int32_t const &saos) {
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_FLLERFLAGS_1_TO_8_OFFSET)) = hton32(saos);
  }

  inline void SetNNF(double const &nnf) {  // 1-RL, 3-SL, 5-Odd Lot
  std::cout <<"THREE LEG NNF SET" <<*((double *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_NFFIELD_OFFSET)) << std::endl;
    *((double *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_NFFIELD_OFFSET)) =
        (double)swap_endian((double)nnf);  // 00 56 BC 93 84 43 D9 42
  }
  inline void SetPan(char const *pan) {
    // NEW 
   memcpy((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_PAN_OFFSET), pan, NSE_SPREAD_ORDERENTRY_REQUEST_PAN_LENGTH);
  }
  inline void SetAlgoId(int32_t const &algo_id) {
    // NEW 
   *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_ALGOID_OFFSET)) = hton32(algo_id);
  }
  void SetAccountNumber(char const *broker_id) {}

 public:
  inline void SetPreLoadedOrderEntryRequestFields(int32_t const &user_id, char const *broker_id, double const &nnf,
                                                  int32_t const &algo_id, char const *pan, int16_t const &branch_id) {
    SetMessageHeaderTraderId(user_id); // Need to check this one
    SetBranchId(branch_id);
    SetTraderId(user_id);
    SetPan(pan);
    SetBrokerId(broker_id);
    SetAccountNumber(broker_id);
    SetNNF(nnf);
    SetAlgoId(algo_id);
  }
  
  inline void SetDynamicOrderEntryRequestFields(int32_t const &packet_sequence_number, int16_t const &buy_sell_1,
                                                 int16_t const &buy_sell_2, int16_t const &buy_sell_3,
                                                 int32_t const &price_1, int32_t const &price_2, int32_t const &price_3,
                                                 HFSAT::ORS::Order *order_1, HFSAT::ORS::Order *order_2, HFSAT::ORS::Order *order_3,
                                                 InstrumentDesc *inst_desc_1,InstrumentDesc *inst_desc_2,InstrumentDesc *inst_desc_3,
                                                 SecurityInfoCashMarket *sec_info_1,SecurityInfoCashMarket *sec_info_2, SecurityInfoCashMarket *sec_info_3,
                                                 bool is_mo, bool add_preopen) {
    *((int32_t *)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)) = hton32(packet_sequence_number);
    
    // For Multi-leg orders i.e. 2L/3L orders all tokens in the respective legs should be from the same stream. 
    memcpy((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_TOKENNO_OFFSET ), inst_desc_1->GetInstrumentDescAsBuffer(),
           sizeof(InstrumentDesc));
    memcpy((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_TOKENNO_OFFSET), inst_desc_2->GetInstrumentDescAsBuffer(),
           sizeof(InstrumentDesc));
    memcpy((void *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_3_TOKENNO_OFFSET), inst_desc_3->GetInstrumentDescAsBuffer(),
           sizeof(InstrumentDesc));

   
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_BUYSELLINDICATOR_OFFSET)) = hton16(buy_sell_1);
    *((int16_t *)(msg_ptr +  NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_BUYSELLINDICATOR_OFFSET)) = hton16(buy_sell_2);
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_3_BUYSELLINDICATOR_OFFSET)) = hton16(buy_sell_3);
/*
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) |= hton16(1 << 8);  // SET AON FLAG
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_STORDERFLAGS_STRUCT_OFFSET)) |= hton16(1 << 8);  // SET AON FLAG
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_3_STORDERFLAGS_STRUCT_OFFSET)) |= hton16(1 << 8);  // SET AON FLAG

    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MINIMUMFILL_OR_AONVOLUME_OFFSET )) = hton32(order_1->size_remaining_);
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_MINIMUMFILL_OR_AONVOLUME_OFFSET )) = hton32(order_2->size_remaining_);
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_3_MINIMUMFILL_OR_AONVOLUME_OFFSET )) = hton32(order_3->size_remaining_);
*/ 
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_OFFSET)) = hton32(0);
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_DISCLOSEDVOLUME_OFFSET)) = hton32(0);
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_3_DISCLOSEDVOLUME_OFFSET)) = hton32(0);

    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_TOTALVOLUMEREMAINING_OFFSET)) = hton32(order_1->size_remaining_);
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_TOTALVOLUMEREMAINING_OFFSET)) = hton32(order_2->size_remaining_);
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_3_TOTALVOLUMEREMAINING_OFFSET)) = hton32(order_3->size_remaining_);

    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_VOLUME_OFFSET)) = hton32(order_1->size_remaining_);
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_VOLUME_OFFSET)) = hton32(order_2->size_remaining_);
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_3_VOLUME_OFFSET)) = hton32(order_3->size_remaining_);
 

    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) |= hton16(1 << 9);  // SET IOC FLAG
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_STORDERFLAGS_STRUCT_OFFSET)) |= hton16(1 << 9);  // SET IOC FLAG
    *((int16_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_3_STORDERFLAGS_STRUCT_OFFSET)) |= hton16(1 << 9);  // SET IOC FLAG

    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_PRICE_OFFSET)) = hton32(price_1);
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_2_PRICE_OFFSET)) = hton32(price_2);
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_MS_SPD_LEG_3_PRICE_OFFSET)) = hton32(price_3);

    // *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_CORDFILLER_OFFSET)) = hton32(order_1->server_assigned_order_sequence_); tried FIllX, FILL1_8
    *((int32_t *)(msg_ptr + NSE_SPREAD_ORDERENTRY_REQUEST_PRICEDIFF_OFFSET)) = hton32(order_1->server_assigned_order_sequence_);  // hack to get saos from exchange
    
    if (AVX512_SUPPORTED)
    HFSAT::MD5::MD5_AVX512VL((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_SPREAD_ORDERENTRY_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
    else
    HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_SPREAD_ORDERENTRY_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));


    // std::cout << "Order Entry ThreeLeg Size:: " << NSE_SPREAD_ORDERENTRY_REQUEST_LENGTH << std::endl;
  }

  inline int32_t GetThreeLegOrderEntryMsgLength(){ return NSE_SPREAD_ORDERENTRY_REQUEST_LENGTH;}

  // char const *GetOrderEntryRequestBuffer() const { return order_entry_request_buffer; }
  char const *GetOrderEntryThreeLegRequestDerivativesBuffer() const { return order_entry_request_buffer; }
};
}
}
