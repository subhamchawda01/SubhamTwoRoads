// =====================================================================================
//
//       Filename:  order_depth_view_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  10/20/2015 11:15:31 AM
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

#include "baseinfra/BaseUtils/eobi_fast_order_manager.hpp"

namespace HFSAT {
namespace BaseUtils {

std::map<int32_t, EOBIFastOrderManager *> EOBIFastOrderManager::secid_to_fast_order_manager_map_;

void EOBIFastOrderManager::OnAdd(EOBI_MDS::EOBICompactOrder const &add_event) {
  //    std::cout << "Fast Order Add : " << bid_price_to_level_inception_time_.size () << " " <<
  //    ask_price_to_level_inception_time_.size () << " " << bid_price_to_order_id_to_data_info_.size () << " " <<
  //    ask_price_to_order_id_to_data_info_.size () << std::endl ;

  int32_t int_price = fast_price_convertor_.GetFastIntPx(add_event.price);

  if ('B' == (char)add_event.side) {
    if (bid_price_to_order_id_to_data_info_.end() == bid_price_to_order_id_to_data_info_.find(int_price)) {
      bid_price_to_order_id_to_data_info_[int_price] = new std::map<uint64_t, EOBI_MDS::EOBICompactOrder>();

      // Level Inception
      bid_price_to_level_inception_time_[int_price] =
          (uint64_t)(add_event.time_.tv_sec * 1000000) + (uint64_t)add_event.time_.tv_usec;
    }

    std::map<uint64_t, EOBI_MDS::EOBICompactOrder> &order_map = *(bid_price_to_order_id_to_data_info_[int_price]);
    order_map[add_event.priority_ts] = add_event;

  } else {
    if (ask_price_to_order_id_to_data_info_.end() == ask_price_to_order_id_to_data_info_.find(int_price)) {
      ask_price_to_order_id_to_data_info_[int_price] = new std::map<uint64_t, EOBI_MDS::EOBICompactOrder>();

      // Level Inception
      ask_price_to_level_inception_time_[int_price] =
          (uint64_t)(add_event.time_.tv_sec * 1000000) + (uint64_t)add_event.time_.tv_usec;
    }

    std::map<uint64_t, EOBI_MDS::EOBICompactOrder> &order_map = *(ask_price_to_order_id_to_data_info_[int_price]);
    order_map[add_event.priority_ts] = add_event;
  }
  if (bid_price_to_order_id_to_data_info_.size() > 0 && ask_price_to_order_id_to_data_info_.size() > 0) {
    // int32_t bid_price = bid_price_to_order_id_to_data_info_.begin()->first;
    // int32_t ask_price = ask_price_to_order_id_to_data_info_.begin()->first;

    // int32_t current_bid_size = bid_price_to_order_id_to_data_info_.size();
    // int32_t current_ask_size = ask_price_to_order_id_to_data_info_.size();

    if (bid_price_to_order_id_to_data_info_.begin()->first >= ask_price_to_order_id_to_data_info_.begin()->first) {
      if ('B' == (char)add_event.side) {
        for (auto &itr : ask_price_to_order_id_to_data_info_) {
          if (int_price >= itr.first) {
            delete (itr.second);
            ask_price_to_order_id_to_data_info_.erase(itr.first);
          } else {
            break;
          }
        }

      } else {
        for (auto &itr : bid_price_to_order_id_to_data_info_) {
          if (itr.first >= int_price) {
            delete (itr.second);
            bid_price_to_order_id_to_data_info_.erase(itr.first);
          } else {
            break;
          }
        }
      }
    }
  }

  //    std::cerr << "AfterAdd: " << bid_price_to_order_id_to_data_info_.size () << " " <<
  //    ask_price_to_order_id_to_data_info_.size ()
  //              << bid_price_to_level_inception_time_.size () << " " << ask_price_to_level_inception_time_.size ()
  //              << std::endl ;
  //
  //    std::cerr << " Fast Price :" << GetFastBidPrice(50,100,300000) << std::endl ;
}

void EOBIFastOrderManager::OnDelete(EOBI_MDS::EOBICompactOrder const &delete_event) {
  int32_t int_price = fast_price_convertor_.GetFastIntPx(delete_event.price);

  if ('B' == (char)delete_event.side) {
    if (bid_price_to_order_id_to_data_info_.end() != bid_price_to_order_id_to_data_info_.find(int_price)) {
      std::map<uint64_t, EOBI_MDS::EOBICompactOrder> &order_map = *(bid_price_to_order_id_to_data_info_[int_price]);

      if (order_map.end() != order_map.find(delete_event.priority_ts)) {
        order_map.erase(delete_event.priority_ts);
      }

      if (0 == order_map.size()) {
        bid_price_to_order_id_to_data_info_.erase(int_price);
        bid_price_to_level_inception_time_.erase(int_price);
      }
    }

  } else {
    if (ask_price_to_order_id_to_data_info_.end() != ask_price_to_order_id_to_data_info_.find(int_price)) {
      std::map<uint64_t, EOBI_MDS::EOBICompactOrder> &order_map = *(ask_price_to_order_id_to_data_info_[int_price]);

      if (order_map.end() != order_map.find(delete_event.priority_ts)) {
        order_map.erase(delete_event.priority_ts);
      }

      if (0 == order_map.size()) {
        ask_price_to_order_id_to_data_info_.erase(int_price);
        ask_price_to_level_inception_time_.erase(int_price);
      }
    }
  }

  //    std::cerr << "AfterDelete: " << bid_price_to_order_id_to_data_info_.size () << " " <<
  //    ask_price_to_order_id_to_data_info_.size ()
  //              << bid_price_to_level_inception_time_.size () << " " << ask_price_to_level_inception_time_.size ()
  //              << std::endl ;
}

void EOBIFastOrderManager::OnModify(EOBI_MDS::EOBICompactOrder const &modify_event) {
  EOBI_MDS::EOBICompactOrder temp_struct;
  memcpy((void *)&temp_struct, (void *)&modify_event, sizeof(EOBI_MDS::EOBICompactOrder));

  //    std::cout << " MODIFY FOR : " << temp_struct.prev_priority_ts << " @ : " << temp_struct.time_ <<
  //    std::endl ;

  int32_t prev_int_price = fast_price_convertor_.GetFastIntPx(modify_event.prev_price);
  //    int32_t new_int_price = fast_price_convertor_.GetFastIntPx(modify_event.price);

  if ('B' == (char)modify_event.side) {
    if (bid_price_to_order_id_to_data_info_.end() != bid_price_to_order_id_to_data_info_.find(prev_int_price)) {
      std::map<uint64_t, EOBI_MDS::EOBICompactOrder> &order_map =
          *(bid_price_to_order_id_to_data_info_[prev_int_price]);

      if (order_map.end() != order_map.find(modify_event.prev_priority_ts)) {
        if ((temp_struct.price == temp_struct.prev_price) && (temp_struct.size < temp_struct.prev_size)) {
          temp_struct.time_ = order_map[modify_event.prev_priority_ts].time_;
        }

        //          std::cout << " PRICE : " << temp_struct.price
        //                    << " PPR : " << temp_struct.prev_price
        //                    << " SIZE : " << temp_struct.size
        //                    << " PSIZE : " << temp_struct.prev_size << " NEW TIME : " <<
        //                    temp_struct.time_.tv_sec << std::endl ;

        order_map.erase(modify_event.prev_priority_ts);

      } else {
        //          std::cout << "NO ORDER FOUDN FOR MODIFY : " << temp_struct.prev_priority_ts <<
        //          std::endl ;
      }

      if (0 == order_map.size()) {
        bid_price_to_order_id_to_data_info_.erase(prev_int_price);
        bid_price_to_level_inception_time_.erase(prev_int_price);
      }
    }

    OnAdd(temp_struct);
  } else {
    if (ask_price_to_order_id_to_data_info_.end() != ask_price_to_order_id_to_data_info_.find(prev_int_price)) {
      std::map<uint64_t, EOBI_MDS::EOBICompactOrder> &order_map =
          *(ask_price_to_order_id_to_data_info_[prev_int_price]);

      if (order_map.end() != order_map.find(modify_event.prev_priority_ts)) {
        if ((temp_struct.price == temp_struct.prev_price) && (temp_struct.size < temp_struct.prev_size)) {
          temp_struct.time_ = order_map[modify_event.prev_priority_ts].time_;
        }
        order_map.erase(modify_event.prev_priority_ts);
      }

      if (0 == order_map.size()) {
        ask_price_to_order_id_to_data_info_.erase(prev_int_price);
        ask_price_to_level_inception_time_.erase(prev_int_price);
      }
    }

    OnAdd(temp_struct);
  }

  //    std::cerr << "AfterModify: " << bid_price_to_order_id_to_data_info_.size () << " " <<
  //    ask_price_to_order_id_to_data_info_.size ()
  //              << bid_price_to_level_inception_time_.size () << " " << ask_price_to_level_inception_time_.size ()
  //              << std::endl ;
}

void EOBIFastOrderManager::OnExecutionSummary(EOBI_MDS::EOBICompactOrder const &execution_summary) {
  //    std::cout << " Execution Summary : " << execution_summary.price << " Best Bid : " << (
  //    bid_price_to_order_id_to_data_info_.begin () )->first << " " << ( ask_price_to_order_id_to_data_info_.begin()
  //    )->first << std::endl ;

  int32_t trade_price = fast_price_convertor_.GetFastIntPx(execution_summary.price);

  if ('B' == (char)execution_summary.side) {
    for (std::map<int32_t, std::map<uint64_t, EOBI_MDS::EOBICompactOrder> *>::iterator ask_itr =
             ask_price_to_order_id_to_data_info_.begin();
         ask_itr != ask_price_to_order_id_to_data_info_.end();) {
      if (trade_price <= ask_itr->first) break;

      ask_price_to_order_id_to_data_info_.erase(ask_itr->first);
      ask_price_to_level_inception_time_.erase(ask_itr->first);
      ask_itr++;
    }

    if (ask_price_to_order_id_to_data_info_.end() != ask_price_to_order_id_to_data_info_.find(trade_price)) {
      std::map<uint64_t, EOBI_MDS::EOBICompactOrder> &order_map = *(ask_price_to_order_id_to_data_info_[trade_price]);
      std::map<uint64_t, EOBI_MDS::EOBICompactOrder>::iterator itr = order_map.begin();

      int32_t net_size = execution_summary.size;

      while (itr != order_map.end()) {
        if (itr->second.size <= net_size) {
          net_size -= itr->second.size;
          order_map.erase(itr++);
        } else {
          itr->second.size -= net_size;
          net_size -= itr->second.size;
          ++itr;
        }

        if (net_size <= 0) break;
      }

      if (0 == order_map.size()) {
        ask_price_to_order_id_to_data_info_.erase(ask_price_to_order_id_to_data_info_.find(trade_price));
        ask_price_to_level_inception_time_.erase(ask_price_to_level_inception_time_.find(trade_price));
      }
    }
  } else {
    for (std::map<int32_t, std::map<uint64_t, EOBI_MDS::EOBICompactOrder> *, Comparator>::iterator bid_itr =
             bid_price_to_order_id_to_data_info_.begin();
         bid_itr != bid_price_to_order_id_to_data_info_.end();) {
      if (trade_price >= bid_itr->first) break;
      bid_price_to_order_id_to_data_info_.erase(bid_itr->first);
      bid_price_to_level_inception_time_.erase(bid_itr->first);
      bid_itr++;
    }

    if (bid_price_to_order_id_to_data_info_.end() != bid_price_to_order_id_to_data_info_.find(trade_price)) {
      std::map<uint64_t, EOBI_MDS::EOBICompactOrder> &order_map = *(bid_price_to_order_id_to_data_info_[trade_price]);
      std::map<uint64_t, EOBI_MDS::EOBICompactOrder>::iterator itr = order_map.begin();

      int32_t net_size = execution_summary.size;

      while (itr != order_map.end()) {
        if (itr->second.size <= net_size) {
          net_size -= itr->second.size;
          order_map.erase(itr++);
        } else {
          itr->second.size -= net_size;
          net_size -= itr->second.size;
          ++itr;
        }

        if (net_size <= 0) break;
      }

      // exact match
      if (0 == order_map.size()) {
        bid_price_to_order_id_to_data_info_.erase(bid_price_to_order_id_to_data_info_.find(trade_price));
        bid_price_to_level_inception_time_.erase(bid_price_to_level_inception_time_.find(trade_price));
      }
    }
  }
}

double EOBIFastOrderManager::GetFastBidPrice(int32_t const &size, int _num_fast_orders_, int _window_msces_) {
  //    std::cerr << "GetFastBidPrice : " << bid_price_to_order_id_to_data_info_.size () << " " <<
  //    ask_price_to_order_id_to_data_info_.size ()
  //              << bid_price_to_level_inception_time_.size () << " " << ask_price_to_level_inception_time_.size ()
  //              << std::endl ;

  uint64_t this_window_nsec = (uint64_t)_window_msces_ * 1000;

  for (auto &itr : bid_price_to_order_id_to_data_info_) {
    int32_t num_of_orders_count = 0;

    for (auto &order_itr : *(bid_price_to_order_id_to_data_info_[itr.first])) {
      if (num_of_orders_count > _num_fast_orders_) break;
      num_of_orders_count++;

      if ((((uint64_t)order_itr.second.time_.tv_sec * 1000000 + (uint64_t)order_itr.second.time_.tv_usec) <=
           bid_price_to_level_inception_time_[itr.first] + this_window_nsec) &&
          (order_itr.second.size <= size)) {
        //          std::cout << " BID ORDER : " << order_itr.first << " ORDER SIZE : " <<
        //          order_itr.second.size << " SZ : " << size << " Order Time : " <<
        //          order_itr.second.time_.tv_sec << "." << order_itr.second.time_.tv_usec << " INCEPTION TIME : " <<
        //          bid_price_to_level_inception_time_ [ itr.first ] << " WINDOW : " << this_window_nsec << std::endl
        //          ;
        return itr.first;
      }
    }
  }

  std::cerr << " NO FAST PRICE WHEN ASKED WITH SIZE : " << size
            << " BID MAP SIZE : " << bid_price_to_order_id_to_data_info_.size() << " Size : " << size
            << " NumOfFastOrders : " << _num_fast_orders_ << " Window : " << _window_msces_ << std::endl;

  for (auto &itr : bid_price_to_order_id_to_data_info_) {
    for (auto &order_itr : *(bid_price_to_order_id_to_data_info_[itr.first])) {
      std::cerr << " PRICE : " << itr.first << " THIS ORDER : " << order_itr.first
                << " SIZE : " << order_itr.second.size << std::endl;
    }
  }

  return 0.00;
}

double EOBIFastOrderManager::GetFastAskPrice(int32_t const &size, int _num_fast_orders_, int _window_msces_) {
  //    std::cerr << "GetFastBidPrice : " << bid_price_to_order_id_to_data_info_.size () << " " <<
  //    ask_price_to_order_id_to_data_info_.size ()
  //              << bid_price_to_level_inception_time_.size () << " " << ask_price_to_level_inception_time_.size ()
  //              << std::endl ;

  uint64_t this_window_nsec = (uint64_t)_window_msces_ * 1000;

  for (auto &itr : ask_price_to_order_id_to_data_info_) {
    int32_t num_of_orders_count = 0;

    for (auto &order_itr : *(ask_price_to_order_id_to_data_info_[itr.first])) {
      if (num_of_orders_count > _num_fast_orders_) break;
      num_of_orders_count++;

      if ((((uint64_t)order_itr.second.time_.tv_sec * 1000000 + (uint64_t)order_itr.second.time_.tv_usec) <=
           ask_price_to_level_inception_time_[itr.first] + this_window_nsec) &&
          order_itr.second.size <= size) {
        //          std::cout << " ASK ORDER : " << order_itr.first << " ORDER SIZE : " <<
        //          order_itr.second.size << " SZ : " << size << std::endl;
        return itr.first;
      }
    }
  }

  std::cerr << " NO FAST PRICE WHEN ASKED WITH SIZE : " << size
            << " ASK MAP SIZE : " << ask_price_to_order_id_to_data_info_.size() << " Size : " << size
            << " NumOfFastOrders : " << _num_fast_orders_ << " Window : " << _window_msces_ << std::endl;

  for (auto &itr : ask_price_to_order_id_to_data_info_) {
    for (auto &order_itr : *(ask_price_to_order_id_to_data_info_[itr.first])) {
      std::cerr << " PRICE : " << itr.first << " THIS ORDER : " << order_itr.first
                << " SIZE : " << order_itr.second.size << std::endl;
    }
  }

  return 0.00;
}

int32_t EOBIFastOrderManager::GetFastBidNetSize(int32_t const &size, int _num_fast_orders_, int _window_msces_) {
  int32_t fast_bid_price = GetFastBidPrice(size, _num_fast_orders_, _window_msces_);

  //    std::cout << " FAST BID PRICE : " << fast_bid_price << std::endl ;

  if (0.0 == fast_bid_price) return 0;

  uint64_t this_window_nsec = (uint64_t)_window_msces_ * 1000;

  int32_t num_of_orders_count = 0;
  int32_t total_fast_orders_at_this_price = 0;

  for (auto &order_itr : *(bid_price_to_order_id_to_data_info_[fast_bid_price])) {
    //      std::cout << " NUMFAST : " << _num_fast_orders_ << " COUNT : " << num_of_orders_count << " ORDERNUM : " <<
    //      order_itr.first << " TIME : " << order_itr.second.time_.tv_sec << "." << order_itr.second.time_.tv_usec <<
    //      " SIZE : " << order_itr.second.size << " INCEPTION : " << bid_price_to_level_inception_time_
    //      [ fast_bid_price ] << " WINDOWMSEC : " << this_window_nsec << std::endl ;

    if (num_of_orders_count > _num_fast_orders_) break;
    num_of_orders_count++;

    if (((uint64_t)order_itr.second.time_.tv_sec * 1000000 + (uint64_t)order_itr.second.time_.tv_usec) >
        bid_price_to_level_inception_time_[fast_bid_price] + this_window_nsec)
      continue;

    if (order_itr.second.size <= size) total_fast_orders_at_this_price += order_itr.second.size;
  }

  if (0 == total_fast_orders_at_this_price) {
    //      std::cout << " BIDNET 0 HERE " << fast_bid_price << std::endl ;
  }

  return total_fast_orders_at_this_price;
}

int32_t EOBIFastOrderManager::GetFastAskNetSize(int32_t const &size, int _num_fast_orders_, int _window_msces_) {
  int32_t fast_ask_price = GetFastAskPrice(size, _num_fast_orders_, _window_msces_);
  //    std::cout << " FAST ASK PRICE : " << fast_ask_price << std::endl ;
  if (0.0 == fast_ask_price) return 0;

  uint64_t this_window_nsec = (uint64_t)_window_msces_ * 1000;

  int32_t num_of_orders_count = 0;
  int32_t total_fast_orders_at_this_price = 0;

  for (auto &order_itr : *(ask_price_to_order_id_to_data_info_[fast_ask_price])) {
    if (num_of_orders_count > _num_fast_orders_) break;
    num_of_orders_count++;

    if (((uint64_t)order_itr.second.time_.tv_sec * 1000000 + (uint64_t)order_itr.second.time_.tv_usec) >
        ask_price_to_level_inception_time_[fast_ask_price] + this_window_nsec)
      continue;

    if (order_itr.second.size <= size) total_fast_orders_at_this_price += order_itr.second.size;
  }

  if (0 == total_fast_orders_at_this_price) {
    //      std::cout << "ASKNET 0 HERE " << fast_ask_price << std::endl ;
  }

  return total_fast_orders_at_this_price;
}

int32_t EOBIFastOrderManager::GetFastBidNumOrders(int32_t const &size, int _num_fast_orders_, int _window_msces_) {
  int32_t fast_bid_price = GetFastBidPrice(size, _num_fast_orders_, _window_msces_);

  //    std::cout << " FAST BID ORDER PRICE : " << fast_bid_price << std::endl ;

  if (0.0 == fast_bid_price) return 0;
  int32_t num_of_orders_count = 0;

  uint64_t this_window_nsec = (uint64_t)_window_msces_ * 1000;
  //    int32_t total_fast_orders_at_this_price = 0;

  for (auto &order_itr : *(bid_price_to_order_id_to_data_info_[fast_bid_price])) {
    if (num_of_orders_count > _num_fast_orders_) break;

    if (((uint64_t)order_itr.second.time_.tv_sec * 1000000 + (uint64_t)order_itr.second.time_.tv_usec) >
        bid_price_to_level_inception_time_[fast_bid_price] + this_window_nsec)
      continue;

    if (order_itr.second.size <= size) num_of_orders_count++;
  }

  return num_of_orders_count;
}

int32_t EOBIFastOrderManager::GetFastAskNumOrders(int32_t const &size, int _num_fast_orders_, int _window_msces_) {
  int32_t fast_ask_price = GetFastAskPrice(size, _num_fast_orders_, _window_msces_);
  //    std::cout << " FAST ASK ORDER PRICE : " << fast_ask_price << std::endl ;
  if (0.0 == fast_ask_price) return 0;
  int32_t num_of_orders_count = 0;

  uint64_t this_window_nsec = (uint64_t)_window_msces_ * 1000;
  //    int32_t total_fast_orders_at_this_price = 0;

  for (auto &order_itr : *(ask_price_to_order_id_to_data_info_[fast_ask_price])) {
    if (num_of_orders_count > _num_fast_orders_) break;

    if (((uint64_t)order_itr.second.time_.tv_sec * 1000000 + (uint64_t)order_itr.second.time_.tv_usec) >
        ask_price_to_level_inception_time_[fast_ask_price] + this_window_nsec)
      continue;

    if (order_itr.second.size <= size) num_of_orders_count++;
  }
  return num_of_orders_count;
}
}
}
