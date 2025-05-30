/**
   \file MarketAdapter/base_market_view_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_MARKETADAPTER_BASE_MARKET_VIEW_MANAGER_H
#define BASE_MARKETADAPTER_BASE_MARKET_VIEW_MANAGER_H

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvccode/ORSMessages/shortcode_ors_message_livesource_map.hpp"
#include "baseinfra/MarketAdapter/smv_utils.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_ors_reply_processor.hpp"

#define ORS_TRACKER_ARRAY_SIZE 8192

namespace HFSAT {

typedef enum {
  kBookManagerOK = 1,
  kBookManagerReturn,
  kBookManagerBreak,
  kBookManagerL1Changed
} BookManagerErrorCode_t;

/// Base Class of EUREXPriceLevelMarketViewManager and FullBookMarketViewManager and BMFOrderLevelMarketViewManager
class BaseMarketViewManager : public OrderExecutedListener,
                              public OrderConfirmedListener,
                              public OrderSequencedListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  const SecurityNameIndexer& sec_name_indexer_;

  const std::vector<SecurityMarketView*> security_market_view_map_; /**< map from security_id to SecurityMarketView *,
                                                                       making a copy instead of a reference of the
                                                                       vector in main ?OPT? */

  // functions
 public:
  BaseMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                        const SecurityNameIndexer& t_sec_name_indexer_,
                        const std::vector<SecurityMarketView*>& t_security_market_view_map_);

  virtual ~BaseMarketViewManager() {}

  void ShowMarket(int t_security_id_);

  virtual void RebuildIndexHighAccess(const uint32_t t_security_id_, const TradeType_t t_buysell_, int new_int_price_);

  virtual void RebuildIndexLowAccess(const uint32_t t_security_id_, const TradeType_t t_buysell_, int new_int_price_);

  virtual void ReScaleBook(const uint32_t security_id, const TradeType_t buysell, int int_price);

  /// Build index based on the int_price_: assign limit_int_price_, limit_ordercount_ and limit_size_ for all levels
  virtual void BuildIndex(const uint32_t t_security_id_, const TradeType_t t_buysell_, int int_price_);

  /**
   * This function in base_market_view_manager doesn't get called anywhere, whoever feels like calling it, please
   * implement the tests
   *
   *
   * This function deletes the levels if we receive a sub-best execution
   *
   * @param t_server_assigned_client_id_
   * @param _client_assigned_order_sequence_
   * @param _server_assigned_order_sequence_
   * @param _security_id_
   * @param _price_
   * @param r_buysell_
   * @param _size_remaining_
   * @param _size_executed_
   * @param _client_position_
   * @param _global_position_
   * @param r_int_price_
   * @param server_assigned_message_sequence
   */
  virtual void OrderExecuted(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                             const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                             const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                             const int _size_executed_, const int _client_position_, const int _global_position_,
                             const int r_int_price_, const int32_t server_assigned_message_sequence,
                             const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  void UpdateBestBidVariablesUsingOurOrders(const unsigned int t_security_id_);

  void UpdateBestAskVariablesUsingOurOrders(const unsigned int t_security_id_);

  inline void UpdateBestVariablesUsingOurOrders(const unsigned int t_security_id_) {
    UpdateBestBidVariablesUsingOurOrders(t_security_id_);
    UpdateBestAskVariablesUsingOurOrders(t_security_id_);
  }

  /**
   * Update the best variables for sec_id with index provided
   * @param t_security_id_
   * @param index
   */

  inline void UpdateBestBidVariables(const unsigned int t_security_id_, const unsigned int index) {
    SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

    smv_.market_update_info_.bestbid_int_price_ = smv_.market_update_info_.bidlevels_[index].limit_int_price_;
    smv_.market_update_info_.bestbid_ordercount_ = smv_.market_update_info_.bidlevels_[index].limit_ordercount_;
    smv_.market_update_info_.bestbid_price_ = smv_.market_update_info_.bidlevels_[index].limit_price_;
    smv_.market_update_info_.bestbid_size_ = smv_.market_update_info_.bidlevels_[index].limit_size_;

    // Yield
    if (!smv_.price_to_yield_map_.empty()) {
      smv_.hybrid_market_update_info_.bestbid_int_price_ = smv_.market_update_info_.bidlevels_[index].limit_int_price_;
      smv_.hybrid_market_update_info_.bestbid_ordercount_ =
          smv_.market_update_info_.bidlevels_[index].limit_ordercount_;
      smv_.hybrid_market_update_info_.bestbid_price_ =
          smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestbid_int_price_];
      smv_.hybrid_market_update_info_.bestbid_size_ = smv_.market_update_info_.bidlevels_[index].limit_size_;
    }
  }

  /**
   * Update the best ask with index provided
   * @param t_security_id_
   */
  inline void UpdateBestAskVariables(const unsigned int t_security_id_, const unsigned int index) {
    SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

    smv_.market_update_info_.bestask_int_price_ = smv_.market_update_info_.asklevels_[index].limit_int_price_;
    smv_.market_update_info_.bestask_ordercount_ = smv_.market_update_info_.asklevels_[index].limit_ordercount_;
    smv_.market_update_info_.bestask_price_ = smv_.market_update_info_.asklevels_[index].limit_price_;
    smv_.market_update_info_.bestask_size_ = smv_.market_update_info_.asklevels_[index].limit_size_;

    // Yield
    if (!smv_.price_to_yield_map_.empty()) {
      smv_.hybrid_market_update_info_.bestask_int_price_ = smv_.market_update_info_.asklevels_[index].limit_int_price_;
      smv_.hybrid_market_update_info_.bestask_ordercount_ =
          smv_.market_update_info_.asklevels_[index].limit_ordercount_;
      smv_.hybrid_market_update_info_.bestask_price_ =
          smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestask_int_price_];
      smv_.hybrid_market_update_info_.bestask_size_ = smv_.market_update_info_.asklevels_[index].limit_size_;
    }
  }

  inline void UpdateBestVariables(const unsigned int t_security_id_) {
    SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
    UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);
    UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);
  }

#define LOW_ACCESS_INDEX 50
  BookManagerErrorCode_t SanitizeAskSide(const unsigned int t_security_id_);
  BookManagerErrorCode_t SanitizeBidSide(const unsigned int t_security_id_);
  BookManagerErrorCode_t AdjustBidIndex(const unsigned int t_security_id_, int t_bid_int_price_, int& t_bid_index_);
  BookManagerErrorCode_t AdjustAskIndex(const unsigned int t_security_id_, int t_ask_int_price_, int& t_ask_index_);
#undef LOW_ACCESS_INDEX

  virtual void OrderConfirmed(const int t_server_assigned_client_id, const int t_client_assigned_order_sequence,
                              const int t_server_assigned_order_sequence, const unsigned int t_security_id,
                              const double t_price, const TradeType_t r_buysell, const int t_size_remaining,
                              const int t_size_executed, const int t_client_position, const int global_position,
                              const int r_int_price, const int32_t server_assigned_message_sequence,
                              const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  virtual void OrderORSConfirmed(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                 const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                 const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                 const int _size_executed_, const int r_int_price_,
                                 const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                 const ttime_t time_set_by_server);

  virtual void OrderSequenced(const int t_server_assigned_client_id, const int t_client_assigned_order_sequence,
                              const int t_server_assigned_order_sequence, const unsigned int t_security_id,
                              const double t_price, const TradeType_t r_buysell, const int size_remaining,
                              const int t_size_executed, const int client_position, const int global_position,
                              const int r_int_price, const int32_t server_assigned_message_sequence,
                              const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  /**
   * Rests the book
   * @param t_security_id
   */
  void ResetBook(unsigned int t_security_id);
};
}
#endif  // BASE_MARKETADAPTER_BASE_MARKET_VIEW_MANAGER_H
