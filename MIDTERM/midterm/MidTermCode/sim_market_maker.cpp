// =====================================================================================
//
//       Filename:  base_order_manager.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Thursday 18 March 2016 02:50:58  GMT
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
#include "midterm/MidTerm/sim_market_maker.hpp"
#include "midterm/MidTerm/base_algo_manager.hpp"
namespace MIDTERM {

// update market_
void SimMarketMaker::OnMarketUpdate(
    unsigned int const security_id,
    HFSAT::MarketUpdateInfo const &market_update_info) {
  // dbglogger_ << __func__ << " SimMarketMaker " << watch_.tv().tv_sec << "\n";
  for (uint32_t i = 0; i < mm_maker_.size(); i++) {
    MarketMakerInfo mm_info_ = mm_maker_[i];
    if (security_id != (unsigned int)sec_name_indexer_.GetIdFromString(mm_info_.shortcode))
      continue;
    // dbglogger_ << mm_info_.ToString() << "\n";
    // dbglogger_ << market_update_info.ToString() << "\n";
    if (mm_info_.side == 'B') {
      // Case when we can execute in the market
      if (mm_info_.price >= market_update_info.bestask_price_) {
        // Case when the liquidity is tight
        if ((mm_info_.total_size - mm_info_.size_executed) >
            market_update_info.bestask_size_) {
          mm_info_.size_executed += market_update_info.bestask_size_;
          mm_maker_[i] = mm_info_;
        }
        // Case when there is enough liquidity
        // Executing the remaining size
        else {
          mm_info_.size_executed = mm_info_.total_size;
          mm_maker_.erase(mm_maker_.begin() + i);
        }
        dbglogger_ << "Sending execution...\n";
        SendResponseToAlgo(
            mm_info_.shortcode, HFSAT::kORRType_Exec, mm_info_.side,
            mm_info_.unique_order_id, market_update_info.bestask_price_,
            mm_info_.total_size, mm_info_.total_size - mm_info_.size_executed);
      }
    } else if (mm_info_.side == 'S') {
      if (mm_info_.price <= market_update_info.bestbid_price_) {
        if ((mm_info_.total_size - mm_info_.size_executed) >
            market_update_info.bestbid_size_) {
          mm_info_.size_executed += market_update_info.bestbid_size_;
          mm_maker_[i] = mm_info_;
        } else {
          mm_info_.size_executed = mm_info_.total_size;
          mm_maker_.erase(mm_maker_.begin() + i);
        }
        SendResponseToAlgo(
            mm_info_.shortcode, HFSAT::kORRType_Exec, mm_info_.side,
            mm_info_.unique_order_id, market_update_info.bestbid_price_,
            mm_info_.total_size, mm_info_.total_size - mm_info_.size_executed);
      }
    } else {
      std::cerr << "Unrecognizable side accessed in simulator..."
                << DBGLOG_ENDL_FLUSH;
    }
  }
}

void SimMarketMaker::SendResponseToAlgo(char *shortcode_,
                                        HFSAT::ORRType_t type_, char side_,
                                        int32_t id_, double price_,
                                        int32_t size_,
                                        int32_t size_remaining_) {
  OrderResponse response_;
  memset((void *)&response_, 0, sizeof(OrderResponse));
  strncpy(response_.shortcode, shortcode_, 24);
  response_.side = side_;
  gettimeofday(&response_.response_time, NULL);
  response_.unique_order_id = id_;
  response_.order_response_type = type_;
  response_.response_price = price_;
  response_.response_int_price = -1;
  response_.response_size = size_;
  response_.size_remaining = size_remaining_;
  response_.client_position = -1;
  response_.client_overnight_position = -1;
  BaseAlgoManager &base_ =
      BaseAlgoManager::GetUniqueInstance(Mode::kNSESimMode, dbglogger_, watch_);
  base_.order_response_ = response_;
  base_.ProcessSingleResponse();
}

void SimMarketMaker::ReceiveOrder(OrderRequest request_) {
  dbglogger_ << "Order Received..." << DBGLOG_ENDL_FLUSH;
  // Now add to the data structure that maintains info about requests
  if (request_.order_type == OrderType::kOrderCancel) {
    // This is a cancel order
    for (uint32_t i = 0; i < mm_maker_.size(); i++) {
      if (mm_maker_[i].unique_order_id == request_.unique_order_id) {
        mm_maker_.erase(mm_maker_.begin() + i);
        dbglogger_ << "MM sending cancel confirmation to algo...\n";
        SendResponseToAlgo(request_.shortcode, HFSAT::kORRType_Cxld,
                           request_.side, request_.unique_order_id,
                           request_.order_price, 0, request_.order_size);
        return;
      }
    }
    // Reaching here implies cancel request for order cannot be executed as it
    // is probably completed
    SendResponseToAlgo(request_.shortcode, HFSAT::kORRType_CxRe, request_.side,
                       request_.unique_order_id, request_.order_price, 0,
                       request_.order_size);
  } else if (request_.order_type == OrderType::kOrderSend) {
    MarketMakerInfo mm_info_;
    memset((void *)&mm_info_, 0, sizeof(MarketMakerInfo));
    strncpy(mm_info_.shortcode, request_.shortcode, 24);
    mm_info_.unique_order_id = request_.unique_order_id;
    mm_info_.side = request_.side;
    mm_info_.size_executed = 0;
    mm_info_.total_size = request_.order_size;
    mm_info_.price = request_.order_price;
    mm_maker_.push_back(mm_info_);
    // Send response to algo manager class
    // Send a confirmation as soon as we receive the order
    dbglogger_ << "MM sending confirmation to algo...\n";
    SendResponseToAlgo(request_.shortcode, HFSAT::kORRType_Conf, request_.side,
                       request_.unique_order_id, request_.order_price, 0,
                       request_.order_size);
  } else if (request_.order_type == OrderType::kOrderModify) {
    // Don't support modify orders
  } else {
    dbglogger_ << "UNSUPPORTED ORDER REQUEST TYPE..." << DBGLOG_ENDL_FLUSH;
    exit(1);
  }

  // Send response to algo manager class
  // Send a confirmation as soon as we receive the order
  // SendResponseToAlgo(request_.shortcode, HFSAT::kORRType_Conf, request_.side,
  // request_.unique_order_id,
  //                   request_.order_price, 0, request_.order_size);
}
}
