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

#pragma once

#define WINDOW_MSECS 10000000000000000000000
#define NUM_OF_FAST_ORDERS_AT_PRICE 10000
#define FAST_SIZE 10000

#include "dvccode/CDef/eobi_mds_defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"

namespace HFSAT {
namespace BaseUtils {

struct Comparator {
  bool operator()(int32_t const &price1, int32_t const &price2) { return price1 > price2 ? true : false; }
};
struct FastBook {
  int32_t fast_best_bid_price_;
  int32_t fast_best_ask_price_;
  int32_t fast_best_bid_size_;
  int32_t fast_best_ask_size_;
};

class EOBIFastOrderManager {
 private:
  static std::map<int32_t, EOBIFastOrderManager *> secid_to_fast_order_manager_map_;
  HFSAT::SecurityDefinitions &security_definitions_;
  HFSAT::SecurityNameIndexer &sec_name_indexer_;
  FastPriceConvertor fast_price_convertor_;
  std::map<int32_t, std::map<uint64_t, EOBI_MDS::EOBICompactOrder> *, Comparator> bid_price_to_order_id_to_data_info_;
  std::map<int32_t, std::map<uint64_t, EOBI_MDS::EOBICompactOrder> *> ask_price_to_order_id_to_data_info_;
  std::map<int32_t, uint64_t> bid_price_to_level_inception_time_;
  std::map<int32_t, uint64_t> ask_price_to_level_inception_time_;
  std::map<std::string, struct FastBook> fast_book_map_;
  bool is_computing_fast_orders_;

  EOBIFastOrderManager(uint32_t sec_id)
      : security_definitions_(HFSAT::SecurityDefinitions::GetUniqueInstance()),
        sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
        fast_price_convertor_(security_definitions_.GetContractMinPriceIncrementWithDateAlreadySet(
            sec_name_indexer_.GetShortcodeFromId(sec_id))),
        bid_price_to_order_id_to_data_info_(),
        ask_price_to_order_id_to_data_info_(),
        bid_price_to_level_inception_time_(),
        ask_price_to_level_inception_time_(),
        fast_book_map_(),
        is_computing_fast_orders_(false) {}

 public:
  static EOBIFastOrderManager &GetUniqueInstance(uint32_t sec_id) {
    if (secid_to_fast_order_manager_map_.end() == secid_to_fast_order_manager_map_.find(sec_id)) {
      secid_to_fast_order_manager_map_[sec_id] = new EOBIFastOrderManager(sec_id);
    }
    return *(secid_to_fast_order_manager_map_[sec_id]);
  }

  static void SetFastBookListenerForAll() {
    for (auto &itr : secid_to_fast_order_manager_map_) {
      (itr.second)->SetFastBookListener();
    }
  }

  void SetFastBookListener() { is_computing_fast_orders_ = true; }

  bool IsListeningOnFastBook() { return is_computing_fast_orders_; }

  void OnAdd(EOBI_MDS::EOBICompactOrder const &add_event);
  void OnDelete(EOBI_MDS::EOBICompactOrder const &delete_event);
  void OnModify(EOBI_MDS::EOBICompactOrder const &modify_event);

  void OnFullTrade(EOBI_MDS::EOBICompactOrder const &fulltrade_event) {}

  void OnExecutionSummary(EOBI_MDS::EOBICompactOrder const &execution_summary);
  double GetFastBidPrice(int32_t const &size, int _num_fast_orders_, int _window_msces_);
  double GetFastAskPrice(int32_t const &size, int _num_fast_orders_, int _window_msces_);
  int32_t GetFastBidNetSize(int32_t const &size, int _num_fast_orders_, int _window_msces_);
  int32_t GetFastAskNetSize(int32_t const &size, int _num_fast_orders_, int _window_msces_);
  int32_t GetFastBidNumOrders(int32_t const &size, int _num_fast_orders_, int _window_msces_);

  int32_t GetFastAskNumOrders(int32_t const &size, int _num_fast_orders_, int _window_msces_);

  void SetFastBook(std::string _indicator_description_, FastBook _my_fast_book_) {
    fast_book_map_[_indicator_description_] = _my_fast_book_;
  }

  FastBook GetFastBook(std::string _indicator_description_) { return fast_book_map_[_indicator_description_]; }
};
}
}
