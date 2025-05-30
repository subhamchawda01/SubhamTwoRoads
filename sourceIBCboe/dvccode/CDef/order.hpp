/**
    \file infracore/BasicOrderRoutingServer/order.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */

#ifndef BASE_BASICORDERROUTINGSERVER_ORDER_H
#define BASE_BASICORDERROUTINGSERVER_ORDER_H

#include <strings.h>
#include <sstream>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/basic_ors_defines.hpp"
#include "dvccode/Utils/thread.hpp"  // Is there any reason to include thread in order?

namespace HFSAT {
namespace ORS {
#define EXCH_ASSIGNED_ORDER_SEQUENCE_LEN 20

/// POD struct holding all the information about an order sent to the exchange.
/// Made POD so that we can call memcmp or memcpy.
struct Order {
 public:
  Order()
      : price_(0),
        exch_assigned_seq_(0),
        engine_ptr_(0),
        security_id_(-1),
        server_assigned_client_id_(-1),
        int_price_(0),
        size_remaining_(0),
        size_executed_(0),
        total_size_(0),
        buysell_(kTradeTypeBuy),
        client_assigned_order_sequence_(0),
        server_assigned_order_sequence_(0),
        new_server_assigned_order_sequence(0),
        exch_assigned_order_sequence_length_(0),
        size_disclosed_(0),
        is_confirmed_(false),
        order_not_found_(false),
        partial_cxl_before_exec_(false),
        is_reserved_type_(false),
        is_ioc(false),
        is_modified(false),
        is_mirrored(false),
        send_modify_cancel('I'),
	is_active_order(true),
        message_sequence_number_(-1),
        last_confirmed_size_remain_(0),
        entry_dt_(0),
        last_mod_dt_(0),
        csw_start_cycle_count_(0) {
    bzero(symbol_, kSecNameLen);
    bzero(exch_assigned_order_sequence_, EXCH_ASSIGNED_ORDER_SEQUENCE_LEN);
  }

  char symbol_[kSecNameLen];  ///< trying to change this to const char * and having one repository for char [
  timeval ors_timestamp_;
  double price_;
  uint64_t exch_assigned_seq_;
  uint64_t engine_ptr_;
  uint64_t ors_order_ptr_;
  uint64_t query_order_ptr_;
  /// kSecNameLen ] fields
  int security_id_;                ///< used in bcaster to GetGlobalPosition
  int server_assigned_client_id_;  ///< useful when the exchange says something about this order and we have to send out
  /// the message to clients. We will need this to fill the field client_position_ in
  /// GenericORSReplyStruct
  int int_price_;
  int size_remaining_;
  int size_executed_;
  int total_size_;
  TradeType_t buysell_;
  int client_assigned_order_sequence_;
  int server_assigned_order_sequence_;
  // This is required to track new saos set by RTS/MICEX in case of modifying the order
  int new_server_assigned_order_sequence;
  int exch_assigned_order_sequence_length_;
  int size_disclosed_;
  char exch_assigned_order_sequence_[EXCH_ASSIGNED_ORDER_SEQUENCE_LEN];
  char exch_assigned_order_sequence_padding_[2];
  bool is_confirmed_;
  bool order_not_found_;
  bool partial_cxl_before_exec_;
  bool is_reserved_type_;  // flag to indicate if the messages is reserved type for throttle
  bool is_ioc;
  bool is_modified;
  bool is_mirrored;
  char send_modify_cancel; // send: 'S' , modify: 'M' , cancel: 'C' , invalid: 'I'
  bool is_active_order;
  // when the exchange rejects this order, this is used to get SAOS of the order corresponding to the sequence number
  // sent by the exchange. The logic for obtaining the SAOS is in OrderManager::GetSAOSUsingExchangeSeqNumber()
  unsigned int message_sequence_number_;
  int32_t last_confirmed_size_remain_;
  int32_t entry_dt_;
  int32_t last_mod_dt_;
  int64_t last_activity_reference_;
  uint64_t csw_start_cycle_count_;
  uint64_t csw_end_cycle_count_;
  uint64_t query_start_cycle_count_;
  uint64_t query_end_cycle_count_;
  uint64_t ors_start_cycle_count_;
  uint64_t ors_end_cycle_count_;

  std::string toString() const {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Symbol:       " << symbol_ << "\n";
    t_temp_oss_ << "Sec_Id:       " << security_id_ << "\n";
    t_temp_oss_ << "Client_id:    " << server_assigned_client_id_ << "\n";
    t_temp_oss_ << "Price:        " << std::fixed << std::setprecision(6) << price_ << "\n";
    t_temp_oss_ << "Size_Left:    " << size_remaining_ << "\n";
    t_temp_oss_ << "Size_Exec:    " << size_executed_ << "\n";
    t_temp_oss_ << "Size_Disclosed:    " << size_disclosed_ << "\n";
    t_temp_oss_ << "BuySell:      " << (buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << "\n";
    t_temp_oss_ << "Client_Seqno: " << client_assigned_order_sequence_ << "\n";
    t_temp_oss_ << "Server_Seqno: " << server_assigned_order_sequence_ << "\n";
    t_temp_oss_ << "New_Server_Seqno: " << new_server_assigned_order_sequence << "\n";
    t_temp_oss_ << "Confirmed:    " << (is_confirmed_ ? "CONF" : "NO_CONF") << "\n";
    t_temp_oss_ << "PartialCxl:    " << (partial_cxl_before_exec_ ? "PCXL" : "NO_PXCL") << "\n";
    t_temp_oss_ << "OrderNotFound:" << (order_not_found_ ? "ONOTF" : "OEXIST") << "\n";
    t_temp_oss_ << "Exch_Seqno:   " << exch_assigned_order_sequence_ << "\n";
    t_temp_oss_ << "Exch_Seqno_L: " << exch_assigned_order_sequence_length_ << "\n";
    t_temp_oss_ << "ORS Timestamp:   " << (ors_timestamp_.tv_sec * 1000000 + ors_timestamp_.tv_usec) << "\n";
    t_temp_oss_ << "Exchange Seq : " << exch_assigned_seq_ << "\n";
    t_temp_oss_ << "IsIOC : " << is_ioc << "\n";
    t_temp_oss_ << "IsModified : " << is_modified << "\n";
    t_temp_oss_ << "MsgSeqNum : " << message_sequence_number_ << "\n";
    t_temp_oss_ << "last_confirmed_size_remain_ : " << last_confirmed_size_remain_ << "\n";
    t_temp_oss_ << "entry_dt_ : " << entry_dt_ << "\n";
    t_temp_oss_ << "last_mod_dt_ : " << last_mod_dt_ << "\n";
    t_temp_oss_ << "last_activity_reference_ : " << last_activity_reference_ << "\n";
    t_temp_oss_ << "active_order: " << is_active_order << "\n";
    return t_temp_oss_.str();
  }
  std::string toStringShort() const {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Symbol: " << symbol_ << " Sec_Id: " << security_id_ << " Client_id: " << server_assigned_client_id_  << " Client_Seqno: " 
      << client_assigned_order_sequence_ << " Server_Seqno: " << server_assigned_order_sequence_ << " active_order: " << is_active_order << "\n";
    return t_temp_oss_.str();
  }
  inline bool IsSizeModified() const { return (size_remaining_ == last_confirmed_size_remain_) ? false : true; }
  inline void print() const { fprintf(stderr, "%s \n", toString().c_str()); }
};
}
}
#endif  // BASE_BASICORDERROUTINGSERVER_ORDER_H
