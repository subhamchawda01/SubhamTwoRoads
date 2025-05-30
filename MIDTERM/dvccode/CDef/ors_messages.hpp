/**
    \file dvccode/CDef/ors_messages.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#ifndef BASE_ORSMESSAGES_ORS_MESSAGES_H
#define BASE_ORSMESSAGES_ORS_MESSAGES_H

#include <sys/time.h>
#include <sstream>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/ors_defines.hpp"

#define ORS_REPLY_SHM_QUEUE_SIZE 16384
#define SHM_KEY_ORS_REPLY 5999  // default

namespace HFSAT {

/** RetailReplyStruct received from GUI/DropCopy in retail
 * size b(80) on ttime_t ) or b(88) on timeval on x86_64
 */
struct RetailReplyStruct {
  char symbol_[2 * kSecNameLen];          ///< b(16) exchange symbol this order was about
  double price_;                          ///< b(8) price of the order
  ttime_t time_set_by_server_;            ///< b(16) used to sort events, unless recomputed GetTimeOfDay()
  ttime_t client_request_time_;           // basically source time of the data
  ORRType_t orr_type_;                    ///< b(4) type of event that happened
  int server_assigned_message_sequence_;  ///< b(4) to detect if messages missed on this channel
  int server_assigned_client_id_;  ///< b(4) unique identifier of the client whose order this pertains to. Set in a way
  /// such that this is unique across all exchanges or rather OrderRoutingServers.
  /// Suppose each ORS is given a number ( ORS_ID ) from 1 to 255. Base client id is a
  /// unique number from 0x00000001 to 0x00001111. Hence Final client id could be (
  /// ORS_ID << 16 ) | BaseClientId
  int size_remaining_;                  ///< b(4) the size of the order that is live in market
  TradeType_t buysell_;                 ///< b(4) the side of the order buy = 0 / sell = 1
  int server_assigned_order_sequence_;  ///< b(4) unique order sequence number set by ORS
  int client_assigned_order_sequence_;  ///< b(4) unique order request sequence number set by client
  int size_executed_;                   ///< b(4) the size of the order that has already been executed
  int client_position_;                 ///< b(4) position of this client on this symbol
  int global_position_;                 ///< b(4) position of all clients on this symbol
  int int_price_;  ///< b(4) int_price_ as computed by the client in this order, mostly to save computation in the
  /// client. The ORS does not do anything with this
  uint64_t exch_assigned_sequence_;
  int pad_;  ///< b(4) padding for alignment

  const char* getContract() {
    const std::string prefix = "ORS_";
    static char contract[kSecNameLen + 5] = {0};

    memset(contract, 0, sizeof(contract));
    memcpy(contract, prefix.c_str(), prefix.length());
    memcpy(contract + prefix.length(), symbol_, kSecNameLen);

    return contract;
  }

  inline const char* ToString(const ORRType_t t_orr_type_t_) {
    switch (t_orr_type_t_) {
      case kORRType_Seqd:
        return "Seqd";
      case kORRType_Conf:
        return "Conf";
      case kORRType_CxRe:
        return "CxRe";
      case kORRType_Cxld:
        return "Cxld";
      case kORRType_Exec:
        return "Exec";
      case kORRType_Rejc:
        return "Rejc";
      case kORRType_CxlRejc:
        return "CxlRejc";
      case kORRType_ORSConf:
        return "ORSConf";
      case kORRType_IntExec:
        return "IntExec";
      case kORRType_Rejc_Funds:
        return "Rejc_Funds";
      case kORRType_Wake_Funds:
        return "Wake_Funds";
      case kORRType_CxlSeqd:
        return "CxlSeqd";
      case kORRType_CxReSeqd:
        return "CxReSeqd";
      default:
        return "kORRType_None";
    }
    return "kORRType_None";
  }

  std::string ToString() {
    std::ostringstream t_temp_oss_;

    t_temp_oss_ << "Sym: " << symbol_ << " Px: " << std::fixed << std::setprecision(6) << price_
                << " ST: " << time_set_by_server_.val << " CT: " << client_request_time_ << " ORR: " << ToString(orr_type_)
                << " SAMS: " << server_assigned_message_sequence_ << " SACI: " << server_assigned_client_id_
                << " SR: " << size_remaining_ << " BS: " << (int)(buysell_)
                << " SAOS: " << server_assigned_order_sequence_ << " CAOS: " << client_assigned_order_sequence_
                << " SE: " << size_executed_ << " CP: " << client_position_ << " GP: " << global_position_
                << " IP: " << int_price_ << " Seq: " << exch_assigned_sequence_ << "\n";

    return t_temp_oss_.str();
  }
};

/** OrderReplyStruct received from OrderRoutingServer on UDP
 * size b(80) on ttime_t ) or b(88) on timeval on x86_64
 */
struct GenericORSReplyStructLive {
  char symbol_[kSecNameLen];              ///< b(16) exchange symbol this order was about
  double price_;                          ///< b(8) price of the order
  ttime_t time_set_by_server_;            ///< b(16) used to sort events, unless recomputed GetTimeOfDay()
  ttime_t client_request_time_;           // basically source time of the data
  ORRType_t orr_type_;                    ///< b(4) type of event that happened
  int server_assigned_message_sequence_;  ///< b(4) to detect if messages missed on this channel
  int server_assigned_client_id_;  ///< b(4) unique identifier of the client whose order this pertains to. Set in a way
  /// such that this is unique across all exchanges or rather OrderRoutingServers.
  /// Suppose each ORS is given a number ( ORS_ID ) from 1 to 255. Base client id is a
  /// unique number from 0x00000001 to 0x00001111. Hence Final client id could be (
  /// ORS_ID << 16 ) | BaseClientId
  int size_remaining_;                  ///< b(4) the size of the order that is live in market
  TradeType_t buysell_;                 ///< b(4) the side of the order buy = 0 / sell = 1
  int server_assigned_order_sequence_;  ///< b(4) unique order sequence number set by ORS
  int client_assigned_order_sequence_;  ///< b(4) unique order request sequence number set by client
  int size_executed_;                   ///< b(4) the size of the order that has already been executed
  int client_position_;                 ///< b(4) position of this client on this symbol
  int global_position_;                 ///< b(4) position of all clients on this symbol
  int int_price_;  ///< b(4) int_price_ as computed by the client in this order, mostly to save computation in the
  int pad_;        ///< b(4) padding for alignment
  /// client. The ORS does not do anything with this
  uint64_t exch_assigned_sequence_;

  const char* getContract() {
    const std::string prefix = "ORS_";
    static char contract[kSecNameLen + 5] = {0};

    memset(contract, 0, sizeof(contract));
    memcpy(contract, prefix.c_str(), prefix.length());
    memcpy(contract + prefix.length(), symbol_, kSecNameLen);

    return contract;
  }

  inline const char* ToString(const ORRType_t t_orr_type_t_) {
    switch (t_orr_type_t_) {
      case kORRType_Seqd:
        return "Seqd";
      case kORRType_Conf:
        return "Conf";
      case kORRType_CxRe:
        return "CxRe";
      case kORRType_Cxld:
        return "Cxld";
      case kORRType_Exec:
        return "Exec";
      case kORRType_Rejc:
        return "Rejc";
      case kORRType_CxlRejc:
        return "CxlRejc";
      case kORRType_ORSConf:
        return "ORSConf";
      case kORRType_IntExec:
        return "IntExec";
      case kORRType_Rejc_Funds:
        return "Rejc_Funds";
      case kORRType_Wake_Funds:
        return "Wake_Funds";
      case kORRType_CxlSeqd:
        return "CxlSeqd";
      case kORRType_CxReRejc:
        return "CxReRejc";
      case kORRType_CxReSeqd:
        return "CxReSeqd";
      default:
        return "kORRType_None";
    }
    return "kORRType_None";
  }

  std::string ToString() {
    std::ostringstream t_temp_oss_;

    t_temp_oss_ << "Sym: " << symbol_ << " Px: " << std::fixed << std::setprecision(6) << price_
                << " ST: " << time_set_by_server_.val << " CT: " << client_request_time_ << " ORR: " << ToString(orr_type_)
                << " SAMS: " << server_assigned_message_sequence_ << " SACI: " << server_assigned_client_id_
                << " SR: " << size_remaining_ << " BS: " << (int)(buysell_)
                << " SAOS: " << server_assigned_order_sequence_ << " CAOS: " << client_assigned_order_sequence_
                << " SE: " << size_executed_ << " CP: " << client_position_ << " GP: " << global_position_
                << " IP: " << int_price_ << " Seq: " << exch_assigned_sequence_ << "\n";

    return t_temp_oss_.str();
  }
};

struct GenericORSReplyStruct {
  char symbol_[kSecNameLen];              ///< b(16) exchange symbol this order was about
  double price_;                          ///< b(8) price of the order
  ttime_t time_set_by_server_;            ///< b(16) used to sort events, unless recomputed GetTimeOfDay()
  ttime_t client_request_time_;           // basically source time of the data
  ORRType_t orr_type_;                    ///< b(4) type of event that happened
  int server_assigned_message_sequence_;  ///< b(4) to detect if messages missed on this channel
  int server_assigned_client_id_;  ///< b(4) unique identifier of the client whose order this pertains to. Set in a way
  /// such that this is unique across all exchanges or rather OrderRoutingServers.
  /// Suppose each ORS is given a number ( ORS_ID ) from 1 to 255. Base client id is a
  /// unique number from 0x00000001 to 0x00001111. Hence Final client id could be (
  /// ORS_ID << 16 ) | BaseClientId
  int size_remaining_;                  ///< b(4) the size of the order that is live in market
  TradeType_t buysell_;                 ///< b(4) the side of the order buy = 0 / sell = 1
  int server_assigned_order_sequence_;  ///< b(4) unique order sequence number set by ORS
  int client_assigned_order_sequence_;  ///< b(4) unique order request sequence number set by client
  int size_executed_;                   ///< b(4) the size of the order that has already been executed
  int client_position_;                 ///< b(4) position of this client on this symbol
  int global_position_;                 ///< b(4) position of all clients on this symbol
  int int_price_;  ///< b(4) int_price_ as computed by the client in this order, mostly to save computation in the
  /// client. The ORS does not do anything with this
  uint64_t exch_assigned_sequence_;
  int pad_;  ///< b(4) padding for alignment

  const char* getContract() {
    const std::string prefix = "ORS_";
    static char contract[kSecNameLen + 5] = {0};

    memset(contract, 0, sizeof(contract));
    memcpy(contract, prefix.c_str(), prefix.length());
    memcpy(contract + prefix.length(), symbol_, kSecNameLen);

    return contract;
  }

  inline const char* ToString(const ORRType_t t_orr_type_t_) {
    switch (t_orr_type_t_) {
      case kORRType_Seqd:
        return "Seqd";
      case kORRType_Conf:
        return "Conf";
      case kORRType_CxRe:
        return "CxRe";
      case kORRType_Cxld:
        return "Cxld";
      case kORRType_Exec:
        return "Exec";
      case kORRType_Rejc:
        return "Rejc";
      case kORRType_CxlRejc:
        return "CxlRejc";
      case kORRType_ORSConf:
        return "ORSConf";
      case kORRType_IntExec:
        return "IntExec";
      case kORRType_Rejc_Funds:
        return "Rejc_Funds";
      case kORRType_Wake_Funds:
        return "Wake_Funds";
      case kORRType_CxlSeqd:
        return "CxlSeqd";
      case kORRType_CxReSeqd:
        return "CxReSeqd";
      default:
        return "kORRType_None";
    }
    return "kORRType_None";
  }

  std::string ToString() {
    std::ostringstream t_temp_oss_;

    t_temp_oss_ << "Sym: " << symbol_ << " Px: " << std::fixed << std::setprecision(6) << price_
                << " ST: " << time_set_by_server_.val << " CT: " << client_request_time_ << " ORR: " << ToString(orr_type_)
                << " SAMS: " << server_assigned_message_sequence_ << " SACI: " << server_assigned_client_id_
                << " SR: " << size_remaining_ << " BS: " << (int)(buysell_)
                << " SAOS: " << server_assigned_order_sequence_ << " CAOS: " << client_assigned_order_sequence_
                << " SE: " << size_executed_ << " CP: " << client_position_ << " GP: " << global_position_
                << " IP: " << int_price_ << " Seq: " << exch_assigned_sequence_ << "\n";

    return t_temp_oss_.str();
  }
};

/** @brief OrderRequestStruct sent to OrderRoutingServer via SHM
 *
 * size 48 bytes on x86_64
 */
struct GenericORSRequestStruct {
  char symbol_[kSecNameLen];            ///< b(16) used in every call
  double price_;                        ///< b(8) used almost in every call, expect probably Replay
  ORQType_t orq_request_type_;          ///< b(4) used to see what sort of request this is
  int server_assigned_order_sequence_;  ///< b(4) used in CancelOrder / CancelReplaceOrder / ReplayOrder
  int size_requested_;                  ///< b(4) used in SendOrder
  TradeType_t buysell_;                 ///< b(4) the side of the order buy = 0 / sell = 1 .. used in SendOrder
  int client_assigned_order_sequence_;  ///< b(4) used in SendOrder / ReplayOrder
  int int_price_;  ///< b(4) mostly to save computation in the client. The ORS does not do anything with this
  ttime_t client_request_time_;
  int size_disclosed_;			
  bool ignore_from_global_pos;  // Flag to ignore from global position, for arb trading and other structured strategies
  bool is_mirror_order_;
  int16_t mirror_factor_;       // In case of mirror order, this field is also used as replication factor.
  uint64_t t2t_cshmw_start_time_;
  uint64_t t2t_cshmw_end_time_;
  uint64_t t2t_tradeinit_start_time_;
  uint64_t t2t_tradeinit_end_time_;

  std::string ToString() const {
    static std::stringstream ss;
    ss.str("");
    ss << "GenericORSRequestStruct: { symbol_: " << symbol_ << ", price_: " << price_
       << ", orq_request_type_: " << orq_request_type_
       << ", server_assigned_order_sequence_: " << server_assigned_order_sequence_
       << ", size_requested_: " << size_requested_ << ", buysell_: " << buysell_
       << ", client_assigned_order_sequence_: " << client_assigned_order_sequence_ << ", int_price_: " << int_price_
       << ", client_request_time_ : " << client_request_time_.tv_sec << "." << client_request_time_.tv_usec
       << ", ignore_from_global_pos: " << ignore_from_global_pos << ", size_disclosed_: " << size_disclosed_
       << ", is_mirror_order_: " << is_mirror_order_ << ", mirror_factor_:" << mirror_factor_ << "}";
    return ss.str();
  }
};
}

#endif  // BASE_ORSMESSAGES_ORS_MESSAGES_H
