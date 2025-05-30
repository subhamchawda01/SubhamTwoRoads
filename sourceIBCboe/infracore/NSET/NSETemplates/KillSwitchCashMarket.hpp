// =====================================================================================
//
//       Filename:  KillSwitchCashMarket.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  03012023 11:11:05 PM
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

#include "infracore/NSET/NSETemplates/KillSwitch.hpp"


#define NSE_CM_KILL_SWITCH_REQUEST_TRANSACTIONCODE_OFFSET (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH)
#define NSE_CM_KILL_SWITCH_REQUEST_TRADERID_OFFSET (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH + 8)
#define NSE_CM_KILL_SWITCH_REQUEST_MESSAGE_LENGTH_OFFSET (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH + 38)
#define NSE_CM_KILL_SWITCH_REQUEST_SECINFO_OFFSET (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH + 54)
#define NSE_CM_KILL_SWITCH_REQUEST_TRANSACTIONCODE_OFFSET (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH)

#define NSE_CM_KILL_SWITCH_REQUEST_SEC_INFO_OFFSET \
  (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH + 54)
#define NSE_CM_KILL_SWITCH_REQUEST_SEC_INFO_LENGTH 12


#define NSE_CM_KILL_SWITCH_REQUEST_BOOKTYPE_OFFSET \
  (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH + 92)
#define NSE_CM_KILL_SWITCH_REQUEST_BOOKTYPE_LENGTH 2

#define NSE_CM_KILL_SWITCH_REQUEST_ACCOUNTNUMBER_OFFSET \
  (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH + 82)
#define NSE_CM_KILL_SWITCH_REQUEST_ACCOUNTNUMBER_LENGTH 10

#define NSE_CM_KILL_SWITCH_REQUEST_BRNACHID_OFFSET \
  (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH + 142)
#define NSE_CM_KILL_SWITCH_REQUEST_BRNACHID_LENGTH 2

#define NSE_CM_KILL_SWITCH_REQUEST_USERID_OFFSET \
  (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH + 144)
#define NSE_CM_KILL_SWITCH_REQUEST_USERID_LENGTH 4

#define NSE_CM_KILL_SWITCH_REQUEST_BROKERID_OFFSET \
  (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH + 148)
#define NSE_CM_KILL_SWITCH_REQUEST_BROKERID_LENGTH 5

#define NSE_CM_KILL_SWITCH_REQUEST_SETTLOR_OFFSET \
  (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH + 178)
#define NSE_CM_KILL_SWITCH_REQUEST_SETTLOR_LENGTH 12

#define NSE_CM_KILL_SWITCH_REQUEST_PROCLIENTINDICATOR_OFFSET \
  (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH + 190)
#define NSE_CM_KILL_SWITCH_REQUEST_PROCLIENTINDICATOR_LENGTH 2

#define NSE_CM_KILL_SWITCH_REQUEST_NFFIELD_OFFSET \
  (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH + 194)
#define NSE_CM_KILL_SWITCH_REQUEST_NFFIELD_LENGTH 8

#define NSE_CM_KILL_SWITCH_REQUEST_PAN_OFFSET \
  (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH + 214)
#define NSE_CM_KILL_SWITCH_REQUEST_PAN_LENGTH 10

#define NSE_CM_KILL_SWITCH_REQUEST_ALGOID_OFFSET \
  (NSE_REQUEST_START_OFFSET + NSE_PACKET_REQUEST_LENGTH + 224)
#define NSE_CM_KILL_SWITCH_REQUEST_ALGOID_LENGTH 4


#define NSE_CM_KILL_SWITCH_MESSAGE_LENGTH 290
#define NSE_CM_KILL_SWITCH_PACKET_LENGTH (NSE_CM_KILL_SWITCH_MESSAGE_LENGTH + NSE_PACKET_REQUEST_LENGTH)



namespace HFSAT {
namespace NSE {
/*
 * Defines Order entry semantics for cash market(CM)
 * Extends generic orderEntry template for NSE trimmed structure.
 */

class KillSwitchCashMarket : public KillSwitch{
  /* private:
    SecurityInfoCashMarket sec_info;
  */

 public:

  inline void SetPacketLength(int16_t const &packet_length) {
    *((int16_t *)(msg_ptr + NSE_PACKET_LENGTH_OFFSET)) = hton16(packet_length);
  }

  inline void SetPacketSequenceNumber(int32_t const &packet_sequence_number) {
    *((int32_t *)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)) = hton32(packet_sequence_number);
  }

  inline void SetPreLoadedKillSwitchRequestFields(int32_t const &user_id, char const *broker_id, double const &nnf,
                                                  int32_t const &algo_id, char const *pan, int16_t const &branch_id) {
    SetBranchId(branch_id);
    SetTraderId(user_id);
    SetPan(pan);
    SetBrokerId(broker_id);
    SetAccountNumber(broker_id);
    SetNNF(nnf);
    SetAlgoId(algo_id);
    
  }

  char const *GetKillSwitchRequestBuffer() const { return kill_switch_request_buffer; }

  void InitializeStaticFields() {
    //================================= @ Packet

    // Total Length Of The Packet To Be Sent
    SetPacketLength(NSE_CM_KILL_SWITCH_PACKET_LENGTH);

    SetTransactionCode(KILL_SWITCH_IN);

//    *((int16_t *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_BOOKTYPE_OFFSET)) = hton16(1);  // regular order

    memset((void *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_SETTLOR_OFFSET), ' ',
           NSE_CM_KILL_SWITCH_REQUEST_SETTLOR_LENGTH);  // this field should be set to blank for a pro order
                                                       //(broker’s own order).

    memset((void *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_SEC_INFO_OFFSET), ' ',
           NSE_CM_KILL_SWITCH_REQUEST_SEC_INFO_LENGTH);

    // Length of the message header and the following login request, hence packet length has been deducted
    *((int16_t *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_MESSAGE_LENGTH_OFFSET)) =
        hton16(NSE_CM_KILL_SWITCH_MESSAGE_LENGTH);


//    *((int16_t *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_PROCLIENTINDICATOR_OFFSET)) = hton16(2);
//    *((double *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_NFFIELD_OFFSET)) =
//        (double)swap_endian((double)111111111111000);  // 00 56 BC 93 84 43 D9 42

//    *((int32_t *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_ALGOID_OFFSET)) = hton32(0);

    // This has to be 0 - Algo category, this was actually made as reserved field by the exchange
  }

 public:
  KillSwitchCashMarket() {
    kill_switch_request_buffer = (char *)calloc(NSE_CM_KILL_SWITCH_PACKET_LENGTH, sizeof(char));
    msg_ptr = kill_switch_request_buffer + NSE_REQUEST_START_OFFSET;
    memset((void *)(msg_ptr) , 0,
           NSE_CM_KILL_SWITCH_PACKET_LENGTH);
    // Initialize The Static Fields
    InitializeStaticFields();
  }

  // =================================== Order Entry Field Setter Functions

  inline void SetTransactionCode(int16_t const &message_header_transaction_code) {
    *((int16_t *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_TRANSACTIONCODE_OFFSET)) =
        hton16(message_header_transaction_code);
  }

  inline void SetProClient(int16_t const &proclient){
    // ‘1’ represents the client’s order.
    // '2’ represents a broker’s order.
    *((int16_t *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_PROCLIENTINDICATOR_OFFSET)) = hton16(proclient); // default is 2
  }
  /*
    inline void SetInstrumentDesc(char const *inst_desc) {
      memcpy((void *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_CONTRACT_SECINFO_STRUCT_SYMBOL_OFFSET), inst_desc,
             sizeof(SecurityInfoCashMarket));
    }
  */
  
  /*
    inline void SetDay() {
      *((int16_t *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) =
          hton16((int16_t)(1 << 12));  // day order
    }
  */
  // For broker’s own order, this field should be set to the broker code.
  inline void SetAccountNumber(char const *broker_id) {
    memcpy((void *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_ACCOUNTNUMBER_OFFSET), (void *)broker_id,
           NSE_CM_KILL_SWITCH_REQUEST_ACCOUNTNUMBER_LENGTH);
  }

  inline void SetSettlor(char const *settlor_) {
    memcpy((void *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_SETTLOR_OFFSET), (void *)settlor_,
           NSE_CM_KILL_SWITCH_REQUEST_SETTLOR_LENGTH);  // this field should be set to blank for a pro order
  }


  inline void SetBookType(int16_t const &book_type) {  // 1-RL, 3-SL, 5-Odd Lot
    *((int16_t *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_BOOKTYPE_OFFSET)) = hton16(book_type);
  }
  /*
    inline void SetBuySell(int16_t const &buy_sell) {
      *((int16_t *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_BUYSELLINDICATOR_OFFSET)) = hton16(buy_sell);
    }

    inline void SetVolume(int32_t const &volume, int32_t const &disclosed_volume) {
      *((int32_t *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_DISCLOSEDVOLUME_OFFSET)) = hton32(disclosed_volume);
      *((int32_t *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_VOLUME_OFFSET)) = hton32(volume);
    }

    inline void SetPrice(int32_t const &price) {
      *((int32_t *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_PRICE_OFFSET)) = hton32(price);
    }
  */
  inline void SetBranchId(int16_t const &branch_id) {
    *((int16_t *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_BRNACHID_OFFSET)) = hton16(branch_id);
  }

  inline void SetTraderId(int32_t const &trader_id) {
    *((int32_t *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_TRADERID_OFFSET)) = hton32(trader_id);
    *((int32_t *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_USERID_OFFSET)) = hton32(trader_id);
  }

  inline void SetPan(char const *pan) {
    memcpy((void *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_PAN_OFFSET), pan, NSE_CM_KILL_SWITCH_REQUEST_PAN_LENGTH);
  }

  inline void SetBrokerId(char const *broker_id) {
    memcpy((void *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_BROKERID_OFFSET), (void *)broker_id,
           NSE_CM_KILL_SWITCH_REQUEST_BROKERID_LENGTH);
  }
  /*
    // SAOS for new order in TransactionId....
    inline void SetSaos(int32_t const &saos) {
      *((int32_t *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_TRANSACTION_ID_OFFSET)) = hton32(saos);
    }
  */
  inline void SetNNF(double const &nnf) {
    *((double *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_NFFIELD_OFFSET)) =
        (double)swap_endian((double)nnf);  // 00 56 BC 93 84 43 D9 42
  }

  inline void SetAlgoId(int32_t const &algo_id) {
    *((int32_t *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_ALGOID_OFFSET)) = hton32(algo_id);
  }

  inline void SetSecInfo(SecurityInfoCashMarket *sec_info = nullptr){

    if (nullptr == sec_info){
      memset((void *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_SEC_INFO_OFFSET), ' ',
           NSE_CM_KILL_SWITCH_REQUEST_SEC_INFO_LENGTH);
    }else {
    memcpy((void *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_SEC_INFO_OFFSET),
           sec_info->GetSecurityInfoCashMarketAsBuffer(), sec_info_cash_size);
    }
  }
  inline void SetInstrumentDescInfo(InstrumentDesc *inst_desc){}

  inline void SetDynamicKillSwitchRequestFields(int32_t const &packet_sequence_number) {

    //std::cout << "INSIDE SetDynamicKillSwitchRequestFields" << std::endl;
    //    sec_info.SetSecurityInfo(inst_desc->symbol_, inst_desc->option_type_);
    *((int32_t *)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)) = hton32(packet_sequence_number);

    //std::cout << "MDSUM COMPUTED LEAVING\n";
   // HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
    //                NSE_CM_KILL_SWITCH_MESSAGE_LENGTH,
     //               (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
  }
  inline void SetDynamicKillSwitchRequestFields(int32_t const &packet_sequence_number,SecurityInfoCashMarket *sec_info) {

    memcpy((void *)(msg_ptr + NSE_CM_KILL_SWITCH_REQUEST_SEC_INFO_OFFSET),
           sec_info->GetSecurityInfoCashMarketAsBuffer(), sec_info_cash_size);
    *((int32_t *)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)) = hton32(packet_sequence_number);

    HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_CM_KILL_SWITCH_MESSAGE_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
  }
  
  inline void SetMd5sum(){
    if (AVX512_SUPPORTED)
    HFSAT::MD5::MD5_AVX512VL((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_CM_KILL_SWITCH_MESSAGE_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
    else
    HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_CM_KILL_SWITCH_MESSAGE_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
  }


  int32_t GetKillSwitchMsgLength() { return NSE_CM_KILL_SWITCH_PACKET_LENGTH; }
  
};
}
}
