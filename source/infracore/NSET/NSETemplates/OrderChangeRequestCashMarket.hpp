// =====================================================================================
//
//       Filename:  OrderChangeRequestCashMarket.hpp
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

#include "infracore/NSET/NSETemplates/cash_market_order_structure_defines.hpp"
#include "infracore/NSET/NSETemplates/cash_market_order_mod_cxl_request_defines.hpp"
#include "infracore/NSET/NSETemplates/OrderChangeRequest.hpp"

#define NSE_CM_CHANGE_REQUEST_LENGTH                                                      \
  (NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_STP_RESERVED_OFFSET + \
   NSE_CM_ORDER_MODCXL_REQUEST_STP_RESERVED_LENGTH)

namespace HFSAT {
namespace NSE {

/*
 * Defines Order cancel semantics for cash market(CM)
 * Extends generic orderCancel template for NSE trimmed structure.
 */
class OrderCancelRequestCashMarket : public OrderCancelRequest {
  /*
   private:
    SecurityInfoCashMarket sec_info;
  */
 public:
  void InitializeStaticFields() {
    //================================= @ Packet

    // Total Length Of The Packet To Be Sent
    SetPacketLength(NSE_CM_CHANGE_REQUEST_LENGTH);

    // ================================ @ Order Cancel

    SetTransactionCode(ORDER_CANCEL_IN_TR);
    *((char *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_MOD_CAN_BY_OFFSET)) = 'T';
    memset((void *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ACCOUNTNUMBER_OFFSET),
           ' ', NSE_CM_ORDER_MODCXL_REQUEST_ACCOUNTNUMBER_LENGTH);
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_BOOKTYPE_OFFSET)) =
        hton16(1);  // regular order
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                  NSE_CM_ORDER_MODCXL_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) =
        hton16((int16_t)(1 << 12));  // day order

    *((char *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_SUSPENDED_OFFSET)) =
        ' ';  // blank while sending order entry request
    memset((void *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_SETTLOR_OFFSET), ' ',
           NSE_CM_ORDER_MODCXL_REQUEST_SETTLOR_LENGTH);
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                  NSE_CM_ORDER_MODCXL_REQUEST_PROCLIENTINDICATOR_OFFSET)) = hton16(2);
    *((double *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_NFFIELD_OFFSET)) =
        (double)swap_endian((double)111111111111000);  // 00 56 BC 93 84 43 D9 42

    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ALGOID_OFFSET)) = 0;

    // This has to be 0 - Algo category, this was actually made as reserved field by the exchange
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ALGOCAT_OFFSET)) = 0;
  }

 public:
  OrderCancelRequestCashMarket() {
    order_change_request_buffer = (char *)calloc(NSE_CM_CHANGE_REQUEST_LENGTH, sizeof(char));
    msg_ptr = (order_change_request_buffer + NSE_REQUEST_START_OFFSET);

    // Initialize The Static Fields
    InitializeStaticFields();
  }

  // =================================== Order Cancel Field Setter Functions

  inline void SetTransactionCode(int16_t const &message_header_transaction_code) {
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_TRANSACTIONCODE_OFFSET)) =
        hton16(message_header_transaction_code);
  }
  inline void SetProClient(int16_t const &proclient){
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                  NSE_CM_ORDER_MODCXL_REQUEST_PROCLIENTINDICATOR_OFFSET)) = hton16(proclient);
  }

  /*
    inline void SetInstrumentDesc(char const *inst_desc) {
      memcpy((void *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                      NSE_CM_ORDER_MODCXL_REQUEST_CONTRACT_SECINFO_STRUCT_SYMBOL_OFFSET),
             inst_desc, sizeof(SecurityInfoCashMarket));
    }

    inline void SetOrderNumber(int64_t const &order_num) {
      *((int64_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ORDERNUMBER_OFFSET)) =
          (int64_t)ntoh64(order_num);
    }
  */
  // For broker’s own order, this field should be set to the broker code.
  inline void SetAccountNumber(char const *broker_id) {
    memcpy((void *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ACCOUNTNUMBER_OFFSET),
           (void *)broker_id, NSE_CM_ORDER_MODCXL_REQUEST_ACCOUNTNUMBER_LENGTH);
  }
  
  inline void SetSettlor(char const *settlor_) {
    memcpy((void *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_SETTLOR_OFFSET), (void *)settlor_,
           NSE_CM_ORDER_MODCXL_REQUEST_SETTLOR_LENGTH);  // this field should be set to blank for a pro order
  }

  /*
    inline void SetBuySell(int16_t const &buy_sell) {
      *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_BUYSELLINDICATOR_OFFSET))
    =
          hton16(buy_sell);
    }

    inline void SetVolume(int32_t const &volume) {
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_DISCLOSEDVOLUME_OFFSET))
    =
          hton32(volume);
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_VOLUME_OFFSET)) =
          hton32(volume);
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                    NSE_CM_ORDER_MODCXL_REQUEST_DISCLOSEDVOLUMEREMAINING_OFFSET)) = 0;
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                    NSE_CM_ORDER_MODCXL_REQUEST_TOTALVOLUMEREMAINING_OFFSET)) = hton32(volume);
    }

    inline void SetPrice(int32_t const &price) {
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_PRICE_OFFSET)) =
          hton32(price);
    }

    inline void SetEntryDateTime(int32_t const &entry_date) {
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ENTRYDATETIME_OFFSET)) =
          hton32(entry_date);
    }

    inline void SetLastModifiedDateTime(int32_t const &modified_date) {
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_LASTMODIFIED_OFFSET)) =
          hton32(modified_date);
    }
  */
  inline void SetBranchId(int16_t const &branch_id) {
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_BRANCHID_OFFSET)) =
        hton16(branch_id);
  }

  inline void SetTraderId(int32_t const &trader_id) {
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_TRADERID_OFFSET)) =
        hton32(trader_id);
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_USERID_OFFSET)) =
        hton32(trader_id);
  }

  inline void SetPan(char const *pan) {
    memcpy((void *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_PAN_OFFSET), pan,
           NSE_CM_ORDER_MODCXL_REQUEST_PAN_LENGTH);
  }

  inline void SetBrokerId(char const *broker_id) {
    memcpy((void *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_BROKERID_OFFSET),
           (void *)broker_id, NSE_CM_ORDER_MODCXL_REQUEST_BROKERID_LENGTH);
  }

  inline void SetNNF(double const &nnf) {  // 1-RL, 3-SL, 5-Odd Lot
    *((double *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_NFFIELD_OFFSET)) =
        (double)swap_endian((double)nnf);  // 00 56 BC 93 84 43 D9 42
  }
  /*
    // setting saos in transaction id field, verified
    inline void SetSaos(int32_t const &saos) {
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_TRANSACTION_ID_OFFSET)) =
          hton32(saos);
    }

    inline void AddPreOpen() final {
      *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
    NSE_CM_ORDER_MODCXL_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) |=
          hton16((int16_t)(1 << 3));  // ioc order
    }
  */
  inline void SetAlgoId(int32_t const &algo_id) {
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ALGOID_OFFSET)) =
        hton32(algo_id);
  }

  void SetDynamicOrderCancelRequestFields(int32_t const &packet_sequence_number, int16_t const &buy_sell,
                                          int32_t const &price, HFSAT::ORS::Order *order, InstrumentDesc *inst_desc,
                                          SecurityInfoCashMarket *sec_info, bool add_preopen = false) {
    memcpy((void *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                    NSE_CM_ORDER_MODCXL_REQUEST_CONTRACT_SECINFO_STRUCT_SYMBOL_OFFSET),
           sec_info->GetSecurityInfoCashMarketAsBuffer(), sec_info_cash_size);
    *((int32_t *)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)) = hton32(packet_sequence_number);
    *((int64_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ORDERNUMBER_OFFSET)) =
        (int64_t)ntoh64((int64_t)order->exch_assigned_seq_);
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ENTRYDATETIME_OFFSET)) =
        hton32(order->entry_dt_);
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_LASTMODIFIED_OFFSET)) =
        hton32(order->last_mod_dt_);
    *((int64_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_LASTACTIVITYREFERENCE_OFFSET)) =
        hton64(order->last_activity_reference_);
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_BUYSELLINDICATOR_OFFSET)) =
        hton16(buy_sell);
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_DISCLOSEDVOLUME_OFFSET)) =
        hton32(order->size_remaining_);
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_VOLUME_OFFSET)) =
        hton32(order->size_remaining_);
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                  NSE_CM_ORDER_MODCXL_REQUEST_DISCLOSEDVOLUMEREMAINING_OFFSET)) = 0;
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                  NSE_CM_ORDER_MODCXL_REQUEST_TOTALVOLUMEREMAINING_OFFSET)) = hton32(order->size_remaining_);
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_PRICE_OFFSET)) =
        hton32(price);
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_TRANSACTION_ID_OFFSET)) =
        hton32(order->server_assigned_order_sequence_);

    //  sec_info.SetSecurityInfo(inst_desc->symbol_, inst_desc->option_type_);
    // SetInstrumentDesc(sec_info->GetSecurityInfoCashMarketAsBuffer());
    // SetPacketSequenceNumber(packet_sequence_number);

    //  SetOrderNumber(order_num);
    // SetEntryDateTime(entry_date);
    // SetLastModifiedDateTime(last_date);
    // SetBuySell(buy_sell);
    // SetVolume(volume);
    // SetPrice(price);
    // SetSaos(saos);

    if (true == add_preopen) {
      *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                    NSE_CM_ORDER_MODCXL_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) |=
          hton16((int16_t)(1 << 3));  // ioc order
      // AddPreOpen();
    }

    if (AVX512_SUPPORTED)
    HFSAT::MD5::MD5_AVX512VL((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_CM_CHANGE_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
    else
    HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_CM_CHANGE_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
  }

  int32_t GetOrderCancelMsgLength() { return NSE_CM_CHANGE_REQUEST_LENGTH; }
};

// ------------------------------------- Order Modify Request for cash market-------------------------------
/*
 * Defines Order modify semantics for cash market(CM)
 * Extends generic order modify template for NSE trimmed structure.
 */

class OrderModifyRequestCashMarket : public OrderModifyRequest {
  /*private:
   SecurityInfoCashMarket sec_info;
 */
 public:
  void InitializeStaticFields() {
    //================================= @ Packet

    // Total Length Of The Packet To Be Sent
    SetPacketLength(NSE_CM_CHANGE_REQUEST_LENGTH);

    // ================================ @ Order Modify

    SetTransactionCode(ORDER_MOD_IN_TR);
    *((char *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_MOD_CAN_BY_OFFSET)) = 'T';

    memset((void *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ACCOUNTNUMBER_OFFSET),
           ' ', NSE_CM_ORDER_MODCXL_REQUEST_ACCOUNTNUMBER_LENGTH);
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_BOOKTYPE_OFFSET)) =
        hton16(1);  // regular order
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                  NSE_CM_ORDER_MODCXL_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) =
        hton16((int16_t)(1 << 12));  // day order

    *((char *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_SUSPENDED_OFFSET)) =
        ' ';  // blank while sending order entry request
    memset((void *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_SETTLOR_OFFSET), ' ',
           NSE_CM_ORDER_MODCXL_REQUEST_SETTLOR_LENGTH);
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                  NSE_CM_ORDER_MODCXL_REQUEST_PROCLIENTINDICATOR_OFFSET)) = hton16(2);
    *((double *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_NFFIELD_OFFSET)) =
        (double)swap_endian((double)111111111111000);  // 00 56 BC 93 84 43 D9 42

    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ALGOID_OFFSET)) = 0;

    // This has to be 0 - Algo category, this was actually made as reserved field by the exchange
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ALGOCAT_OFFSET)) = 0;
  }

 public:
  OrderModifyRequestCashMarket() {
    order_change_request_buffer = (char *)calloc(NSE_CM_CHANGE_REQUEST_LENGTH, sizeof(char));
    msg_ptr = (order_change_request_buffer + NSE_REQUEST_START_OFFSET);

    // Initialize The Static Fields
    InitializeStaticFields();
  }

  // =================================== Order Modify Field Setter Functions

  inline void SetTransactionCode(int16_t const &message_header_transaction_code) {
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_TRANSACTIONCODE_OFFSET)) =
        hton16(message_header_transaction_code);
  }
    inline void SetProClient(int16_t const &proclient){
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                  NSE_CM_ORDER_MODCXL_REQUEST_PROCLIENTINDICATOR_OFFSET)) = hton16(proclient);
  }

  /*
    inline void SetInstrumentDesc(char const *inst_desc) {
      memcpy((void *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                      NSE_CM_ORDER_MODCXL_REQUEST_CONTRACT_SECINFO_STRUCT_SYMBOL_OFFSET),
             inst_desc, sizeof(SecurityInfoCashMarket));
    }

    inline void SetOrderNumber(int64_t const &order_num) {
      *((int64_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ORDERNUMBER_OFFSET)) =
          (int64_t)ntoh64(order_num);
    }
  */
  // For broker’s own order, this field should be set to the broker code.
  inline void SetAccountNumber(char const *broker_id) {
    memcpy((void *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ACCOUNTNUMBER_OFFSET),
           (void *)broker_id, NSE_CM_ORDER_MODCXL_REQUEST_ACCOUNTNUMBER_LENGTH);
  }

   inline void SetSettlor(char const *settlor_) {
    memcpy((void *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_SETTLOR_OFFSET), (void *)settlor_,
           NSE_CM_ORDER_MODCXL_REQUEST_SETTLOR_LENGTH);  // this field should be set to blank for a pro order
  }
  /*
    inline void SetBuySell(int16_t const &buy_sell) {
      *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_BUYSELLINDICATOR_OFFSET))
    =
          hton16(buy_sell);
    }

    inline void SetVolume(int32_t const &volume, int32_t const &disclosed_volume) {
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_DISCLOSEDVOLUME_OFFSET))
    =
          hton32(disclosed_volume);
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_VOLUME_OFFSET)) =
          hton32(volume);
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                    NSE_CM_ORDER_MODCXL_REQUEST_DISCLOSEDVOLUMEREMAINING_OFFSET)) = 0;
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                    NSE_CM_ORDER_MODCXL_REQUEST_TOTALVOLUMEREMAINING_OFFSET)) = hton32(volume);
    }

    inline void SetPrice(int32_t const &price) {
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_PRICE_OFFSET)) =
          hton32(price);
    }

    inline void SetTradedVolume(int32_t const &volume) {
      //    std::cout << " Set Traded Volume : " << volume << std::endl ;
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                    NSE_CM_ORDER_MODCXL_REQUEST_VOLUMEFILLEDTODAY_OFFSET)) = hton32(volume);
    }

    inline void SetEntryDateTime(int32_t const &entry_date) {
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ENTRYDATETIME_OFFSET)) =
          hton32(entry_date);
    }

    inline void SetLastModifiedDateTime(int32_t const &modified_date) {
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_LASTMODIFIED_OFFSET)) =
          hton32(modified_date);
    }
  */
  inline void SetTraded() {
    int16_t flag = 1;
    flag = (flag << 6);  // for some weird reason, big endian structure works
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                  NSE_CM_ORDER_MODCXL_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) |= hton16(flag);
  }

  inline void SetModified() {
    int16_t flag = 1;
    flag = (flag << 5);  // for some weird reason, big endian structure works
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                  NSE_CM_ORDER_MODCXL_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) |= hton16(flag);
  }

  inline void SetBranchId(int16_t const &branch_id) {
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_BRANCHID_OFFSET)) =
        hton16(branch_id);
  }

  inline void SetTraderId(int32_t const &trader_id) {
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_TRADERID_OFFSET)) =
        hton32(trader_id);
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_USERID_OFFSET)) =
        hton32(trader_id);
  }

  inline void SetPan(char const *pan) {
    memcpy((void *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_PAN_OFFSET), pan,
           NSE_CM_ORDER_MODCXL_REQUEST_PAN_LENGTH);
  }

  inline void SetBrokerId(char const *broker_id) {
    memcpy((void *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_BROKERID_OFFSET),
           (void *)broker_id, NSE_CM_ORDER_MODCXL_REQUEST_BROKERID_LENGTH);
  }

  inline void SetNNF(double const &nnf) {  // 1-RL, 3-SL, 5-Odd Lot
    *((double *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_NFFIELD_OFFSET)) =
        (double)swap_endian((double)nnf);  // 00 56 BC 93 84 43 D9 42
  }
  /*
    inline void SetSaos(int32_t const &saos) {
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_TRANSACTION_ID_OFFSET)) =
          hton32(saos);
    }
  */
  inline void SetAlgoId(int32_t const &algo_id) {
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ALGOID_OFFSET)) =
        hton32(algo_id);
  }
  /*
   public:
    inline void AddPreOpen() final {
      *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
   NSE_CM_ORDER_MODCXL_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) |=
          hton16((int16_t)(1 << 3));  // ioc order
    }
  */
  void SetDynamicOrderModifyRequestFields(int32_t const &packet_sequence_number, int16_t const &buy_sell,
                                          int32_t const &price, HFSAT::ORS::Order *order, InstrumentDesc *inst_desc,
                                          SecurityInfoCashMarket *sec_info, bool add_preopen = false) {
    memcpy((void *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                    NSE_CM_ORDER_MODCXL_REQUEST_CONTRACT_SECINFO_STRUCT_SYMBOL_OFFSET),
           sec_info->GetSecurityInfoCashMarketAsBuffer(), sec_info_cash_size);
    *((int32_t *)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)) = hton32(packet_sequence_number);
    *((int64_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ORDERNUMBER_OFFSET)) =
        (int64_t)ntoh64((int64_t)order->exch_assigned_seq_);
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_ENTRYDATETIME_OFFSET)) =
        hton32(order->entry_dt_);
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_LASTMODIFIED_OFFSET)) =
        hton32(order->last_mod_dt_);
    *((int64_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_LASTACTIVITYREFERENCE_OFFSET)) =
      (int64_t)ntoh64((int64_t)order->last_activity_reference_);
    *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_BUYSELLINDICATOR_OFFSET)) =
        hton16(buy_sell);
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_DISCLOSEDVOLUME_OFFSET)) =
        hton32(order->size_disclosed_);
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_VOLUME_OFFSET)) =
        hton32(order->size_remaining_);
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                  NSE_CM_ORDER_MODCXL_REQUEST_DISCLOSEDVOLUMEREMAINING_OFFSET)) = 0;
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                  NSE_CM_ORDER_MODCXL_REQUEST_TOTALVOLUMEREMAINING_OFFSET)) = hton32(order->size_remaining_);
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                  NSE_CM_ORDER_MODCXL_REQUEST_VOLUMEFILLEDTODAY_OFFSET)) = hton32(order->size_executed_);
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_PRICE_OFFSET)) =
        hton32(price);
    *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_TRANSACTION_ID_OFFSET)) =
        hton32(order->server_assigned_order_sequence_);
    // sec_info.SetSecurityInfo(inst_desc->symbol_, inst_desc->option_type_);
    // SetInstrumentDesc(sec_info->GetSecurityInfoCashMarketAsBuffer());
    // SetPacketSequenceNumber(packet_sequence_number);
    // SetOrderNumber(order_num);
    // SetEntryDateTime(entry_date);
    // SetLastModifiedDateTime(last_date);
    // SetBuySell(buy_sell);
    // SetVolume(volume, disclosed_volume);
    // SetTradedVolume(traded_volume);
    // SetPrice(price);
    // SetSaos(saos);

    if (true == add_preopen) {
      *((int16_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                    NSE_CM_ORDER_MODCXL_REQUEST_STORDERFLAGS_STRUCT_OFFSET)) |=
          hton16((int16_t)(1 << 3));  // ioc order
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                    NSE_CM_ORDER_MODCXL_REQUEST_DISCLOSEDVOLUME_OFFSET)) = hton32(0);
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_VOLUME_OFFSET)) =
          hton32(order->size_remaining_);
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                    NSE_CM_ORDER_MODCXL_REQUEST_DISCLOSEDVOLUMEREMAINING_OFFSET)) = 0;
      *((int32_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                    NSE_CM_ORDER_MODCXL_REQUEST_TOTALVOLUMEREMAINING_OFFSET)) = hton32(order->size_remaining_);
      // AddPreOpen();
      // SetVolume(volume, 0);
    }

    if (AVX512_SUPPORTED)
    HFSAT::MD5::MD5_AVX512VL((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_CM_CHANGE_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
    else
    HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_CM_CHANGE_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
  }

  int32_t GetOrderModifyMsgLength() { return NSE_CM_CHANGE_REQUEST_LENGTH; }
};
}
}
