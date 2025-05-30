// =====================================================================================
//
//       Filename:  execution_logic.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Friday 22 January 2016 01:33:45  GMT
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
#include "midterm/MidTerm/execution_logic.hpp"
#include "midterm/MidTerm/sim_order_manager.hpp"

namespace MIDTERM {

// Initialize static var here
int ExecutionLogic::order_id_ = 150000;

void ExecutionLogic::OnOrderConfirmed(OrderResponse response_) {
  MarketLock();
  if (order_status_info_.find(response_.unique_order_id) !=
      order_status_info_.end()) {
    dbglogger_ << "Order: " << response_.unique_order_id
               << " has been confirmed...\n";
    order_status_info_[response_.unique_order_id].status =
        Order_Status::kNSEOrderConfirmed;
  }
  MarketUnlock();
}

// TODO:: HANDLE PARTIAL EXECUTION, CAN ARISE WHEN >1 ORDER SIZE REQUESTED
void ExecutionLogic::OnOrderExecuted(OrderResponse response_) {
  MarketLock();
  if (order_status_info_.find(response_.unique_order_id) !=
      order_status_info_.end()) {
    if (response_.size_remaining > 0) {
      order_status_info_[response_.unique_order_id].status =
          Order_Status::kNSEOrderPartiallyExecuted;
      order_status_info_[response_.unique_order_id].timestamp =
          watch_.tv().tv_sec;
      order_status_info_[response_.unique_order_id].quantity =
          response_.response_size;
      dbglogger_ << "ALERT -> Partial execution has occurred, is this handled ?"
                 << DBGLOG_ENDL_FLUSH;
    } else {
      dbglogger_ << "Complete execution for Order ID: "
                 << response_.unique_order_id << DBGLOG_ENDL_FLUSH;
      // Need to divide by lotsize in end of trade analysis
      executions.push_back(std::make_pair(response_.response_size / lotsize_,
                                          response_.response_price));
      orders_received_++;
      order_status_info_[response_.unique_order_id].status =
          Order_Status::kNSEOrderExecuted;
      order_status_info_[response_.unique_order_id].timestamp =
          watch_.tv().tv_sec;
      order_status_info_[response_.unique_order_id].quantity =
          response_.response_size;
    }
  }
  MarketUnlock();
}

void ExecutionLogic::OnOrderCancelled(OrderResponse response_) {
  MarketLock();
  if (order_status_info_.find(response_.unique_order_id) !=
      order_status_info_.end()) {
    order_status_info_[response_.unique_order_id].status =
        Order_Status::kNSEOrderCancelled;
  }
  MarketUnlock();
}

void ExecutionLogic::OnOrderCancelRejected(OrderResponse response_) {
  MarketLock();
  if (order_status_info_.find(response_.unique_order_id) !=
      order_status_info_.end()) {
    dbglogger_ << "CAUTION -> Cancel order has been rejected...execution has "
                  "been received possibly"
               << DBGLOG_ENDL_FLUSH;
  }
  MarketUnlock();
}

//. TODO:: Add handling for this
void ExecutionLogic::OnOrderRejected(OrderResponse response_) {
  if (order_status_info_.find(response_.unique_order_id) !=
      order_status_info_.end()) {
    dbglogger_ << "ALERT -> ORDER REJECT FOR " << response_.unique_order_id
               << DBGLOG_ENDL_FLUSH;
    order_status_info_[response_.unique_order_id].status =
        Order_Status::kNSEOrderRejected;
  }
}

// update market_
void ExecutionLogic::OnMarketUpdate(
    unsigned int const security_id,
    HFSAT::MarketUpdateInfo const &market_update_info) {
  MarketLock();
  market_[security_id].best_bid_price = market_update_info.bestbid_price_;
  market_[security_id].best_ask_price = market_update_info.bestask_price_;
  market_[security_id].best_bid_size = market_update_info.bestbid_size_;
  market_[security_id].best_ask_size = market_update_info.bestask_size_;
  market_[security_id].best_bid_num_orders =
      market_update_info.bestbid_ordercount_;
  market_[security_id].best_ask_num_orders =
      market_update_info.bestask_ordercount_;
  MarketUnlock();
}

void ExecutionLogic::SendOrder(int32_t &order_id_, string shortcode_,
                               char side_, int32_t size_, double price_,
                               OrderType order_type_) {
  Sleep(1);
  OrderRequest order_;
  memset((void *)&order_, 0, sizeof(OrderRequest));
  memcpy((void *)order_.shortcode, (void *)shortcode_.c_str(),
         std::min(24, int(shortcode_.length())));

  order_.side = side_;
  // time
  order_.sent_time = watch_.tv().tv_sec;
  order_.unique_order_id = order_id_;
  order_.order_type = order_type_;
  // price
  order_.order_price = price_;
  order_.order_int_price = -1;
  order_.modify_new_order_price = -1;
  order_.modify_new_order_int_price = -1;
  // size
  order_.order_size = size_ * lotsize_;
  order_.modify_new_order_size = -1;

  dbglogger_ << order_.ToString() << '\n';
  trader_.SendTrade(order_);
  dbglogger_ << "Done sending orders at time: " << watch_.tv().tv_sec << "\n";
}

// NO FAILSAFE LOGIC IMPLEMENTED - HAD BEEN DISCUSSED EARLIER - PLS FIX - RK
std::map<int, int>
ExecutionLogic::GetAverageVolumeProfile(std::string product) {
  std::map<int, int> time_to_volume_map_;

  std::ifstream if_;
  int32_t trading_date = HFSAT::DateTime::GetCurrentIsoDateLocal();

  std::string filepath_ = VOLUME_PROFILE_FILEPATH;
  std::string filename_;
  int historical_date = trading_date;

  // INCORRECT LOGIC SUBJECT TO ALL SORTS OF ERROR CASES - PLS FIX - RK
  // I'm sorry but aren't we going to the most recent day for which data
  // is available and picking up vol profile off that ?
  int number_of_days = 0;
  while (true) {
    historical_date = HFSAT::DateTime::CalcPrevWeekDay(historical_date);
    number_of_days++;
    std::stringstream date_;
    date_ << historical_date;
    filename_ = filepath_ + date_.str() + "/" + "VO_avg_" + product + "_" +
                date_.str() + ".txt";
    if_.open(filename_.c_str());
    if (if_.is_open())
      break;
  }

  char line_[LINEBUFFER];
  while (if_.good()) {
    if_.getline(line_, LINEBUFFER);
    // line_ like ->
    // UTC_TIME: 040000 TOTAL_VOL: 32503050 COUNT: 30 AVG: 1083435
    HFSAT::PerishableStringTokenizer st_(line_, LINEBUFFER);
    const std::vector<const char *> &tokens_ = st_.GetTokens();
    // Get the first four char of the time
    char tempbuf[5];
    memcpy(tempbuf, &tokens_[1][0], 4);
    tempbuf[4] = '\0';

    time_t timestamp_ =
        HFSAT::DateTime::GetTimeUTC(historical_date, atoi(tempbuf));
    // lets keep start time rather than end time which is kept currently
    time_to_volume_map_[timestamp_ - (VOLUME_PROFILE_GRANULARITY * 60) +
                        (3600 * 24 * number_of_days)] = atoi(tokens_[7]);
  }
  return time_to_volume_map_;
}
}
