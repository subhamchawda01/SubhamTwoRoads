// =====================================================================================
//
//       Filename:  KillSwitch.hpp
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
#include "dvccode/CDef/debug_logger.hpp"
#include "infracore/NSET/NSETemplates/RequestPacket.hpp"
#include "infracore/NSET/NSETemplates/RequestHeader.hpp"
#include "infracore/NSET/NSETemplates/cash_market_order_structure_defines.hpp"





namespace HFSAT {
namespace NSE {
/*
 * Defines Order entry semantics for cash market(CM)
 * Extends generic orderEntry template for NSE trimmed structure.
 */

class KillSwitch {
  /* private:
    SecurityInfoCashMarket sec_info;
  */
 protected:
  char *kill_switch_request_buffer;
  char const *msg_ptr;
  int32_t sec_info_cash_size;

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

  virtual char const* GetKillSwitchRequestBuffer() const = 0 ;

  void InitializeStaticFields() {}

 public:
  KillSwitch() {
    kill_switch_request_buffer = nullptr;
    msg_ptr = nullptr;
    sec_info_cash_size = sizeof(SecurityInfoCashMarket);
  }

  // =================================== Order Entry Field Setter Functions

  virtual void SetTransactionCode(int16_t const &message_header_transaction_code) {}

  virtual void SetProClient(int16_t const &proclient){}

  virtual void SetAccountNumber(char const *broker_id) {}

  virtual void SetSettlor(char const *settlor_) {}

  virtual void SetBookType(int16_t const &book_type) {}  
 
  virtual void SetBranchId(int16_t const &branch_id) {}
 
  virtual void SetTraderId(int32_t const &trader_id) {}

  virtual void UpdateAccountNumber(char const *broker_id) {}

  virtual void SetPan(char const *pan) {}

  virtual void SetBrokerId(char const *broker_id) {}

  virtual void SetNNF(double const &nnf) {}

  virtual void SetAlgoId(int32_t const &algo_id) {}
  

  virtual void SetSecInfo(SecurityInfoCashMarket *sec_info = nullptr){}

  virtual void SetInstrumentDescInfo(InstrumentDesc *inst_desc){}

  virtual void SetDynamicKillSwitchRequestFields(int32_t const &packet_sequence_number) {}

  virtual void SetDynamicKillSwitchRequestFields(int32_t const &packet_sequence_number,SecurityInfoCashMarket *sec_info) {}

  virtual void SetMd5sum(){}


  virtual int32_t GetKillSwitchMsgLength() = 0 ;
  
};
}
}