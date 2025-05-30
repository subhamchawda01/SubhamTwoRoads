// =====================================================================================
//
//       Filename:  mid_term_order_routing_defines.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/08/2016 09:30:05 AM
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
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"
#include "midterm/MidTerm/trade_bar_generator.hpp"
#include "dvccode/Utils/lock.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"
//#include "MidTerm/TWAP.hpp"
//#include "MidTerm/VWAP.hpp"
//#include "MidTerm/PVOL.hpp"
//#include "MidTerm/execution_logic.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/ors_defines.hpp"
#include "midterm/MidTerm/midterm_constants.hpp"
namespace MIDTERM {

enum class OrderType {
  kInvalid = 0,
  kOrderSend,
  kOrderModify,
  kOrderCancel,
  kRecoverPosition,
  kReplyAllExecs
};
enum class RejectionReason {
  kInvalid = 0,
  kSendRejected,
  kCancelRejected,
  kModifyRejected
};
enum class RiskCheckStatus {
  kInvalid = 0,
  kAllowed,
  kRejectedDueToOutOfReferencePriceBand,
  kRejectedDueToMktPriceCheck,
  kRejectedDueToOrderSizeCheck,
  kRejectedDueToMaxPositionCheck,
  kRejectedDueToLiveOrdersCheck,
  kRejectedDueToWorstCasePositionCheck,
  kRejectedDueToSecNotFound,
  kRejectedDueToThrottle,
  kRejectedDueToEstimatedLossCheck
};

struct MarketMakerInfo {
  int32_t unique_order_id;
  char shortcode[24];
  char side;
  int32_t total_size;
  int32_t size_executed;
  double exec_price;
  double price;

  MarketMakerInfo() {
    size_executed = 0;
    exec_price = -1;
    unique_order_id = -1;
    side = 'I';
    total_size = -1;
    price = -1;
  }

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "------------MARKET MAKER INFO STRUCT-----------"
               << std::endl;
    t_temp_oss << "Order_ID:			" << unique_order_id
               << std::endl;
    t_temp_oss << "Shortcode:			" << shortcode << std::endl;
    t_temp_oss << "Side:				" << side << std::endl;
    t_temp_oss << "Total_Size:		" << total_size << std::endl;
    t_temp_oss << "Executed_Size:		" << size_executed << std::endl;
    t_temp_oss << "Order_Price:		" << price << std::endl;
    t_temp_oss << "-----------------------------------------------"
               << std::endl;
    return t_temp_oss.str();
  }
};

struct OrderRequest {
  char shortcode[24]; // null padded shortcode
  char side;          // 'B' or 'S'
  int32_t sent_time;
  int32_t unique_order_id; // unique across products, stragtegies
  OrderType order_type;
  double order_price;
  int32_t order_int_price;
  int32_t order_size;
  double modify_new_order_price;
  int32_t modify_new_order_int_price;
  int32_t modify_new_order_size;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "------------------ORDER_REQUEST---------------" << std::endl;
    t_temp_oss << "Shortcode:        " << shortcode << std::endl;
    t_temp_oss << "Order_ID:         " << unique_order_id << std::endl;
    t_temp_oss << "Sent_Time:        " << sent_time << std::endl;
    t_temp_oss << "Side(B/S):        " << side << std::endl;
    t_temp_oss << "Order_Size:       " << order_size << std::endl;
    t_temp_oss << "Order_Price:      " << order_price << std::endl;
    t_temp_oss << "Order_Int_Price:  " << order_int_price << std::endl;
    t_temp_oss << "Modify_Size:      " << modify_new_order_size << std::endl;
    t_temp_oss << "Modify_Price:     " << modify_new_order_price << std::endl;
    t_temp_oss << "Modify_Int_Price: " << modify_new_order_int_price
               << std::endl;
    t_temp_oss << "OrderType:        ";
    if (order_type == OrderType::kOrderSend)
      t_temp_oss << "kOrderSend";
    else if (order_type == OrderType::kOrderModify)
      t_temp_oss << "kOrderModify";
    else if (order_type == OrderType::kOrderCancel)
      t_temp_oss << "kOrderCancel";
    else
      t_temp_oss << "Unrecognized for MidTerm";
    t_temp_oss << std::endl;
    t_temp_oss << "-----------------------------------------------"
               << std::endl;
    return t_temp_oss.str();
  }
};

struct OrderResponse {
  char shortcode[24]; // null padded shortcode
  char side;          // 'B' or 'S'
  struct timeval response_time;
  int32_t unique_order_id; // unique across products, stragtegies
  HFSAT::ORRType_t order_response_type;
  double response_price;
  int32_t response_int_price;
  int32_t response_size;
  RejectionReason rejection_reason;

  // These are for reconciliation purpose only
  int32_t size_remaining;
  int32_t client_position;
  int32_t client_overnight_position;
  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "------------------ORDER_RESPONSE---------------"
               << std::endl;
    //    t_temp_oss << "ResponseType:     " << HFSAT::ToString(
    //    order_response_type.ToString ) << std::endl;
    t_temp_oss << "Shortcode:        " << shortcode << std::endl;
    t_temp_oss << "Order_ID:         " << unique_order_id << std::endl;
    t_temp_oss << "Response_Side:    " << side << std::endl;
    t_temp_oss << "Response_Size:    " << response_size << std::endl;
    t_temp_oss << "Response_Price:   " << response_price << std::endl;
    t_temp_oss << "Response_Int_Price:" << response_int_price << std::endl;
    t_temp_oss << "Remaining_Size:   " << size_remaining << std::endl;
    t_temp_oss << "Client_Position:  " << client_position << std::endl;
    t_temp_oss << "Overnight_Position:" << client_overnight_position
               << std::endl;
    t_temp_oss << "Response_Type:    " << order_response_type << std::endl;
    t_temp_oss << "-----------------------------------------------"
               << std::endl;
    return t_temp_oss.str();
  }
};

struct LastSeenMarketSnapshot {
  double best_bid_price;
  double best_ask_price;
  int32_t best_bid_int_price;
  int32_t best_ask_int_price;
  int32_t best_bid_size;
  int32_t best_ask_size;
  int32_t best_bid_num_orders;
  int32_t best_ask_num_orders;
  int32_t update_watch_time;

  LastSeenMarketSnapshot() {
    best_bid_price = best_ask_price = kInvalidPrice;
    best_bid_int_price = best_ask_int_price = kInvalidIntPrice;
    best_bid_size = best_ask_size = best_bid_num_orders = best_ask_num_orders =
        update_watch_time = -1;
  }
};

struct CurrentPortfolioStatus {
  double NAV;
  double Realized_PNL;
  double stoploss_price;
  double init_price;
  double last_execution_price;
  bool has_stoploss;
  int32_t sent_time;
  int32_t position;
  int32_t init_position;
  bool stoploss_executing; // To handle fresh order when stoploss executing
  int32_t orders_to_place;
  bool updated;
  std::string type;

  CurrentPortfolioStatus() {
    init_position = 0;
    stoploss_executing = false;
    NAV = INITIAL_NAV;
    Realized_PNL = 0;
    init_price = 0;
    last_execution_price = 0;
    type = "MARKET";
  }

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "StopLoss\t" << stoploss_price << std::endl;
    t_temp_oss << "Position\t" << position << std::endl;
    t_temp_oss << "Init_Pos\t" << init_position << std::endl;
    t_temp_oss << "Ordering\t" << orders_to_place << std::endl;
    t_temp_oss << "Init_NAV\t" << NAV << std::endl;
    t_temp_oss << "Init_Price\t" << init_price << std::endl;
    t_temp_oss << "RealizedPNL\t" << Realized_PNL << std::endl;
    return t_temp_oss.str();
  }
};

struct ResponseStatus {
  int rejects;
  char side;
  // total size and size executed are positive
  int total_size;
  int size_executed;
};

typedef pair<string, string> key_t;
}
