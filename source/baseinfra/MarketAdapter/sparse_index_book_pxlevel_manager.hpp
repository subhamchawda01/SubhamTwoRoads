/**
   \file MarketAdapter/spare_index_book_pxlevel_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2017
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include <vector>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/CommonDataStructures/lockfree_simple_mempool.hpp"
#include "baseinfra/MarketAdapter/basic_market_view_structs.hpp"
#include "baseinfra/MarketAdapter/sparse_index_book_hash_extended_data_structure.hpp"
#include "baseinfra/MarketAdapter/sparse_index_book_map_extended_data_structure.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

// Sorted vector of price levels ( UNCROSSED access )
// should have atleast MIN_NUM_SORTED_BOOK_LEVELS ( and atmost
// MAX_NUM_SORTED_BOOK_LEVELS ) levels at all times.
#define MIN_NUM_SORTED_BOOK_LEVELS 5
#define MAX_NUM_SORTED_BOOK_LEVELS 10
#define DEEP_LEVEL_MARKER 1000  // assigned as price_level of untracked deep levels
#define kInvalidSize -1         // move to a more standard location - TODO

// Assumes min and max valid price
#define SP_MIN_PRICE -1000000
#define SP_MAX_PRICE 1000000

// L2 listeners are called only when change happens in top NUM_LEVELS_FOR_L2_CALLBACK
// or if book is crossed
#define NUM_LEVELS_FOR_L2_CALLBACK 5

namespace HFSAT {
// Return type passed to indexedbookmanager.
// k_nochange passed if change is in some deep [ beyond MAX_NUM_SORTED_BOOK_LEVELS ]
// level - please note Ordering of enums matter since they are compared in some places
enum LevelChangeType_t { k_l1price = 0, k_l1size, k_l2change, k_nochange };

class SparseIndexBookPxLevelManager {
 private:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  int num_securities_;

  // variable set to k_nochange on add/delete/modify and initially and tracked
  // through changes. It is returned in add/modify/delete
  LevelChangeType_t change_status_;

  LockFreeSimpleMempool<MarketUpdateInfoLevelStruct> one_pxlevel_mempool_;

  // Sorted Vectors of pxlevels for quick access to ith level
  // These are base pxlevels consequently can cross.
  std::vector<std::vector<MarketUpdateInfoLevelStruct*>> sid_2_base_bid_levels_;
  std::vector<std::vector<MarketUpdateInfoLevelStruct*>> sid_2_base_ask_levels_;

  // To support access to uncrossed underlying book
  // limit_int_price_level_ of MarketUpdateInfoLevelStruct provides
  // index of top uncrossed level in base_{bid/ask}_levels
  // For latency these values are only maintained when book is crossed.
  std::vector<MarketUpdateInfoLevelStruct> sid_2_best_uncrossed_bid_level_;
  std::vector<MarketUpdateInfoLevelStruct> sid_2_best_uncrossed_ask_level_;

  std::vector<bool> is_book_crossed_;
  std::vector<uint64_t> init_book_cross_time_;
  std::vector<bool> is_crossing_side_bid_;
  std::vector<int> num_valid_bid_levels_;
  std::vector<int> num_valid_ask_levels_;

  std::vector<bool> using_predictive_uncross_;

 private:
  // Sanitizes underlying book in cases of packet drops
  //  void CheckAndSanitizeBook(int security_id_);

  // Uncrosses book if needed -- the two functions deal with absense(presence) or hidden orders
  void CheckAndUncrossBook(int security_id_, TradeType_t last_ord_ttype_);
  void CheckAndUncrossBookForHiddenOrders(int security_id_, TradeType_t last_ord_ttype_,
                                          MarketUpdateInfoLevelStruct* prev_bid_best_lvl_,
                                          MarketUpdateInfoLevelStruct* prev_ask_best_lvl_);

  // Assigns price level to a new level and reassigns price levels of existing levels if
  // necessary
  void CheckAndAssignPriceLevel(int security_id_, TradeType_t buysell, MarketUpdateInfoLevelStruct* affected_level_);

  // Gets ith lower(upper) level from the specified price for kTradeTypeBuy(kTradeTypeSell)
  // from underlying DS. Return type is NULL if level does not exist.
  MarketUpdateInfoLevelStruct* GetDeepPL(int security_id_, TradeType_t buysell, int int_price, int depth);

  // Variables and functions to compute LevelChange type on an event.
  double preevent_bestbid_price_;
  double preevent_bestask_price_;
  int preevent_bestbid_size_;
  int preevent_bestask_size_;
  void StorePreEventState(int security_id);
  void SetEventChangeState(int security_id);

  // Checks for price consistency (strict inc/dec) and size ( > 0) of L2 book and x book instances
  void CheckConsistency(int security_id);
  std::string GetStatusStringForm(LevelChangeType_t ctype);

  // Maintains information on whether L2 callback should be invoked
  bool pass_check_for_l2_callback_;

 public:
  SparseIndexBookPxLevelManager(DebugLogger& dbglogger, const Watch& watch, int num_sec_id);
  ~SparseIndexBookPxLevelManager();

  // Called by sparse_book_manager to add an order to pxlevel DS
  LevelChangeType_t AddOrderToPxLevel(int security_id_, TradeType_t buysell_, int int_price_, double price_,
                                      unsigned int size_);

  // Called by sparse_book_manager on Delete, Partial or Complete Trade and Modify Size cases.
  LevelChangeType_t ModifySizeAtPxLevel(int security_id_, TradeType_t buysell_, int int_price_, int size_,
                                        bool decrement_ordercount_);

  // Called by sparse_book_manager to modify an order price (size might change as well)
  LevelChangeType_t ModifyOrderAtDiffLevels(int security_id_, TradeType_t buysell_, int old_int_price_,
                                            unsigned int old_size_, int new_int_price_, double new_price_,
                                            unsigned int new_size_);

  // Accessor functions to get ith ( base - potentially crossed )level variables
  // Convention is 0 denotes best level.
  inline MarketUpdateInfoLevelStruct* GetBidPL(int security_id_, int level_) {
    if (level_ < num_valid_bid_levels_[security_id_]) {
      return sid_2_base_bid_levels_[security_id_][level_];
    } else if ((num_valid_bid_levels_[security_id_] > 0) &&
               (underlying_bid_ds_->num_levels_vec_[security_id_] > level_)) {
      MarketUpdateInfoLevelStruct* tgt_pl_ =
          GetDeepPL(security_id_, HFSAT::kTradeTypeBuy,
                    sid_2_base_bid_levels_[security_id_][num_valid_bid_levels_[security_id_] - 1]->limit_int_price_,
                    level_ - num_valid_bid_levels_[security_id_] + 1);
      return tgt_pl_;
    }

    return NULL;
  }

  inline MarketUpdateInfoLevelStruct* GetAskPL(int security_id_, int level_) {
    if (level_ < num_valid_ask_levels_[security_id_]) {
      return sid_2_base_ask_levels_[security_id_][level_];
    } else if ((num_valid_ask_levels_[security_id_] > 0) &&
               (underlying_ask_ds_->num_levels_vec_[security_id_] > level_)) {
      MarketUpdateInfoLevelStruct* tgt_pl_ =
          GetDeepPL(security_id_, HFSAT::kTradeTypeSell,
                    sid_2_base_ask_levels_[security_id_][num_valid_ask_levels_[security_id_] - 1]->limit_int_price_,
                    level_ - num_valid_ask_levels_[security_id_] + 1);
      return tgt_pl_;
    }
    return NULL;
  }

  double GetBidPrice(int security_id_, int level);
  double GetAskPrice(int security_id_, int level);
  int GetBidSize(int security_id_, int level);
  int GetAskSize(int security_id_, int level);

  inline int NumValidBidLevels(int security_id_) { return num_valid_bid_levels_[security_id_]; }
  inline int NumValidAskLevels(int security_id_) { return num_valid_ask_levels_[security_id_]; }

  // Accessor functions to get ith ( uncrossed ) level variables
  inline MarketUpdateInfoLevelStruct* GetSynBidPL(int security_id_, int level_) {
    if (!is_book_crossed_[security_id_]) {
      return GetBidPL(security_id_, level_);
    } else {  // Book is crossed
      // specific case of handling best level - optimized
      // Case 1. top level
      if (level_ == 0) {
        if (sid_2_best_uncrossed_bid_level_[security_id_].limit_int_price_level_ == SP_INDEX_BOOK_INVALID_PX_LEVEL) {
          return NULL;
        } else {
          return &(sid_2_best_uncrossed_bid_level_[security_id_]);
        }
      } else {
        // Case 2. sub-best syn level
        int t_best_level_ = sid_2_best_uncrossed_bid_level_[security_id_].limit_int_price_level_;
        // Case 2.1 sub-best syn level is in book
        if (t_best_level_ != SP_INDEX_BOOK_INVALID_PX_LEVEL && t_best_level_ != DEEP_LEVEL_MARKER &&
            t_best_level_ + level_ < num_valid_bid_levels_[security_id_]) {
          return sid_2_base_bid_levels_[security_id_][t_best_level_ + level_];
        } else if ((t_best_level_ == SP_INDEX_BOOK_INVALID_PX_LEVEL) ||
                   ((t_best_level_ != DEEP_LEVEL_MARKER) &&
                    (underlying_bid_ds_->num_levels_vec_[security_id_] <= (level_ + t_best_level_)))) {
          // Case 2.2 sub-best level does not exist since top level is non-existent
          return NULL;
        } else {
          // Case 2.3 request for sub-best level from underlying DS
          MarketUpdateInfoLevelStruct* tgt_pl_ =
              GetDeepPL(security_id_, HFSAT::kTradeTypeBuy,
                        sid_2_best_uncrossed_bid_level_[security_id_].limit_int_price_, level_);
          return tgt_pl_;
        }
      }
    }
    return NULL;
  }

  inline MarketUpdateInfoLevelStruct* GetSynAskPL(int security_id_, int level_) {
    if (!is_book_crossed_[security_id_]) {
      return GetAskPL(security_id_, level_);
    } else {  // Book is crossed
      // specific case of handling best level - optimized
      // Case 1. top level
      if (level_ == 0) {
        if (sid_2_best_uncrossed_ask_level_[security_id_].limit_int_price_level_ == SP_INDEX_BOOK_INVALID_PX_LEVEL) {
          return NULL;
        } else {
          return &(sid_2_best_uncrossed_ask_level_[security_id_]);
        }
      } else {
        // Case 2. sub-best syn level
        int t_best_level_ = sid_2_best_uncrossed_ask_level_[security_id_].limit_int_price_level_;
        // Case 2.1 sub-best syn level is in book
        if (t_best_level_ != SP_INDEX_BOOK_INVALID_PX_LEVEL && t_best_level_ != DEEP_LEVEL_MARKER &&
            t_best_level_ + level_ < num_valid_ask_levels_[security_id_]) {
          return sid_2_base_ask_levels_[security_id_][t_best_level_ + level_];
        } else if ((t_best_level_ == SP_INDEX_BOOK_INVALID_PX_LEVEL) ||
                   ((t_best_level_ != DEEP_LEVEL_MARKER) &&
                    (underlying_ask_ds_->num_levels_vec_[security_id_] <= (level_ + t_best_level_)))) {
          // Case 2.2 sub-best level does not exist since top level is non-existent
          return NULL;
        } else {
          // Case 2.3 request for sub-best level from underlying DS
          MarketUpdateInfoLevelStruct* tgt_pl_ =
              GetDeepPL(security_id_, HFSAT::kTradeTypeSell,
                        sid_2_best_uncrossed_ask_level_[security_id_].limit_int_price_, level_);
          return tgt_pl_;
        }
      }
    }
    return NULL;
  }

  double GetSynBidPrice(int security_id_, int level);
  double GetSynAskPrice(int security_id_, int level);
  int GetSynBidSize(int security_id_, int level);
  int GetSynAskSize(int security_id_, int level);
  int GetSynBidIntPrice(int security_id_, int level);
  int GetSynAskIntPrice(int security_id_, int level);

  // Accessor functions to support smv calls
  MarketUpdateInfoLevelStruct* GetPLAtIntPrice(int security_id_, TradeType_t buysell_, int limit_int_price_);

  bool IsBookCrossed(int security_id_) { return is_book_crossed_[security_id_]; }
  uint64_t InitBookCrossTime(int security_id_) { return init_book_cross_time_[security_id_]; }

  // Function to return price levels and sizes after auction style uncross
  // Used internally for uncross book ( in absence of hidden orders )
  // and externally for sanitize use case
  void SetUncrossSizesandPrices(int security_id_, MarketUpdateInfoLevelStruct*& bid_level_,
                                MarketUpdateInfoLevelStruct*& ask_level_, int& bid_size_at_level_,
                                int& ask_size_at_level_);

  int FindCrossedLevels(int security_id_, int agg_ord_sz_, int agg_ord_int_px_, TradeType_t buysell_,
                        int& size_remaning_, bool& is_valid_book_end_, int& last_traded_int_price_);

  // Checks whether we want to use predictive synthetic logic to uncross book.
  // Currently it is simply dependent on whether hidden orders are supported for that instrument on not.
  void CheckAndSetPredictiveUncross(int sec_id_, std::string shortcode_);
  bool UsingPredictiveUncross(int security_id_) { return using_predictive_uncross_[security_id_]; }
  bool IsCrossingSideBid(int security_id_) { return is_crossing_side_bid_[security_id_]; }

  // For debugging purposes ---------------------------------
  void DumpBook(int security_id_, int t_level_);

  BaseSparseIndexDataStructure* underlying_bid_ds_;
  BaseSparseIndexDataStructure* underlying_ask_ds_;
};
}
