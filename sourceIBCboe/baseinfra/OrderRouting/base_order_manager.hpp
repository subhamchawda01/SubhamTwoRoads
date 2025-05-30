/**
   \file OrderRouting/base_order_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_ORDERROUTING_BASE_ORDER_MANAGER_H
#define BASE_ORDERROUTING_BASE_ORDER_MANAGER_H

#include <map>
#include <vector>
#include <math.h>
#include <set>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/CDef/ors_defines.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/simple_mempool.hpp"

#include "dvccode/ORSMessages/ors_message_listener.hpp"

#include "dvccode/CommonTradeUtils/watch.hpp"

#include "baseinfra/OrderRouting/base_order.hpp"
#include "baseinfra/OrderRouting/base_trader.hpp"

#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "dvccode/CommonTradeUtils/throttle_manager.hpp"

#include "baseinfra/BaseUtils/query_trading_auto_freeze_manager.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#define CXL_WAIT_MSECS 500
#define MODIFY_WAIT_MSECS 500

#define REJECT_COOLOFF_MSECS 1

// Makes no sense to do this in SIM , the large UTSs throw off the calibration.
#define ENABLE_AGG_TRADING_COOLOFF true
#define ENABLE_AGG_TRADING_COOLOFF_ORDER_DISABLE false
#define EMAIL_ON_AGG_COOLOFF false
#define AGG_TIME_FRAME_MSECS 1000
#define AGG_SIZE_THRESHOLD 5
#define RETAIL_SAOS 799997
//#define ORDER_MANAGER_INT_PRICE_RANGE 2048

#define TIME_TO_CHECK_FOR_DROPS 30000  // Every 30 seconds

namespace HFSAT {

/// Main class that listens to ORS reply messages either from ORS ( live trading
/// ) or SMM ( sim trading )
class BaseOrderManager : public OrderNotFoundListener,
                         public OrderSequencedListener,
                         public OrderConfirmedListener,
                         public OrderConfCxlReplaceRejectListener,
                         public OrderConfCxlReplacedListener,
                         public OrderCanceledListener,
                         public OrderExecutedListener,
                         public OrderRejectedListener,
                         public OrderRejectedDueToFundsListener,
                         public OrderInternallyMatchedListener,
                         public TimePeriodListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  SecurityNameIndexer& sec_name_indexer_;
  BaseTrader& base_trader_;
  const std::string dep_shortcode_;
  const unsigned int dep_security_id_;
  const char* dep_symbol_;
  const FastPriceConvertor fast_price_convertor_;

 public:
  const int server_assigned_client_id_;
  int place_cancel_cooloff_;

 protected:
  int client_assigned_order_sequence_;
  int no_new_order_till;
  bool if_reject_set;

  SimpleMempool<BaseOrder> baseorder_mempool_;

  /* On sendorder order is added to vector 'unsequenced_bids_' and map
   * 'intpx_2_sum_bid_unconfirmed_' updated */
  /* On Sequenced order is removed from unsequenced_bids_ and added to
   * intpx_2_bid_order_vec_, map is untouched */
  /* On Confirm order size is moved from intpx_2_sum_bid_unconfirmed_ to
   * intpx_2_sum_bid_confirmed_ */
  //    BidPriceOrderMap intpx_2_bid_order_vec_ ; /**< all sequenced orders,
  //    even unconfirmed ones */

  //    AskPriceSizeMap intpx_2_sum_ask_confirmed_ ;
  //    AskPriceSizeMap intpx_2_sum_ask_unconfirmed_ ; /**< unsequenced +
  //    sequenced but unconfirmed */
  //    AskPriceOrderMap intpx_2_ask_order_vec_ ; /**< all sequenced orders,
  //    even unconfirmed ones */

  int ORDER_MANAGER_INT_PRICE_RANGE;

  bool initial_adjustment_set_;
  int bid_int_price_adjustment_;
  int ask_int_price_adjustment_;

  std::vector<std::vector<BaseOrder*> > bid_order_vec_;
  int order_vec_top_bid_index_;
  int order_vec_bottom_bid_index_;

  std::vector<int> sum_bid_confirmed_;
  std::vector<int> sum_bid_confirmed_orders_;
  int confirmed_top_bid_index_;
  int confirmed_bottom_bid_index_;

  std::vector<int> sum_bid_unconfirmed_;
  std::vector<int> sum_bid_unconfirmed_orders_;
  int unconfirmed_top_bid_index_;
  int unconfirmed_bottom_bid_index_;

  std::vector<std::vector<BaseOrder*> > ask_order_vec_;
  int order_vec_top_ask_index_;
  int order_vec_bottom_ask_index_;

  std::vector<int> sum_ask_confirmed_;
  std::vector<int> sum_ask_confirmed_orders_;
  int confirmed_top_ask_index_;
  int confirmed_bottom_ask_index_;

  std::vector<int> sum_ask_unconfirmed_;
  std::vector<int> sum_ask_unconfirmed_orders_;
  int unconfirmed_top_ask_index_;
  int unconfirmed_bottom_ask_index_;

  std::vector<BaseOrder*> unsequenced_bids_; /**< unsequenced bid orders are stored in a vector */
  std::vector<BaseOrder*> unsequenced_asks_; /**< unsequenced ask orders are stored in a vector */

  bool external_cancel_all_outstanding_orders_;
  int num_unconfirmed_orders_; /**< number of unconfirmed orders, including
                                  unsequenced ones */

  //    std::vector < std::vector < int > > foked_bid_order_caos_at_intpx_ ;
  //    std::vector < std::vector < int > > foked_ask_orders_caos_at_intpx_ ;
  int foked_bid_order_size_sum_;
  int foked_ask_order_size_sum_;

  // Vectors maintaining different total sizes at all levels - from current level till the bottom level.
  std::vector<int> sum_total_bid_unconfirmed_;
  std::vector<int> sum_total_bid_confirmed_;
  std::vector<int> sum_total_ask_unconfirmed_;
  std::vector<int> sum_total_ask_confirmed_;

  PositionChangeListener* s_p_position_change_listener_;
  std::vector<PositionChangeListener*> position_change_listener_vec_;
  ExecutionListener* s_p_execution_listener_;
  std::vector<ExecutionListener*> execution_listener_vec_;
  OrderChangeListener* s_p_order_change_listener_;
  std::vector<OrderChangeListener*> order_change_listener_vec_;
  CancelRejectListener* s_p_cxl_reject_listener_;
  std::vector<CancelRejectListener*> cxl_reject_listener_vec_;
  RejectDueToFundsListener* s_p_reject_funds_listener_;
  std::vector<RejectDueToFundsListener*> reject_due_to_funds_listener_vec_;
  FokFillRejectListener* s_p_fok_fill_reject_listener_;
  std::vector<FokFillRejectListener*> fok_fill_reject_listener_vec_;
  ExchangeRejectsListener* exch_rejects_listener_;
  std::vector<ExchangeRejectsListener*> exch_rejects_listener_vec_;
  ORSRejectsFreezeListener* ors_rejects_freeze_listener_;
  std::vector<ORSRejectsFreezeListener*> ors_rejects_freeze_listener_vec_;

  int client_position_;
  int global_position_;
  int position_offset_;

  int map_clean_counter_;
  int sum_bid_sizes_;
  int sum_ask_sizes_;

  int num_self_trades_;
  int trade_volume_;
  int last_maps_cleaned_msecs_;
  int last_top_replay_msecs_;
  bool queue_sizes_needed_;

  /// these are used only to mark which orders are currently at top level
  int best_bid_int_price_;
  int best_ask_int_price_;

  unsigned int supporting_order_filled_;
  unsigned int best_level_order_filled_;
  unsigned int aggressive_order_filled_;
  unsigned int improve_order_filled_;

  unsigned int total_size_placed_;  // For fill - ratio computations

  int send_order_count_;         // For SendOrderCount ( )
  mutable int cxl_order_count_;  // For CxlOrderCount ( )
  mutable int modify_order_count_;

  int agg_time_frame_msecs_;
  int agg_size_threshold_;

  bool agg_trading_cooloff_;
  bool disable_orders_due_to_agg_cooloff_;
  int agg_cooloff_stop_msecs_;

  int intentional_agg_lasttime_[kTradeTypeNoInfo];
  int intentional_agg_sizes_[kTradeTypeNoInfo];

  int last_exec_int_price_[kTradeTypeNoInfo];
  int last_exec_time_[kTradeTypeNoInfo];
  int unintentional_agg_sizes_;

  bool email_sent_;
  bool is_cancellable_before_confirmation;
  std::vector<int> security_id_to_last_position_;
  int security_position_;

  int* p_ticks_to_keep_bid_int_price_;
  int* p_ticks_to_keep_ask_int_price_;
  ThrottleManager* throttle_manager_;

  bool is_using_in_live_trading_;
  int32_t expected_message_sequence_;
  std::set<int32_t> dropped_sequences_;
  int32_t last_drop_detection_check_time_;
  double disclosed_size_factor_;
  bool set_disclosed_size_;
  bool modify_partial_exec_;
  int32_t number_of_consecutive_exchange_rejects_;
  int32_t last_reject_evaluation_time_;
  int32_t time_based_total_exchange_rejects_;
  int32_t total_exchange_rejects_;
  int32_t reject_based_freeze_timer_;
  HFSAT::BaseUtils::QueryTradingAutoFreezeManager& query_trading_auto_freeze_manager_;
  bool is_auto_freeze_active_;

  /// Keeps track of mfm since last sent message ( SendTrade,Cancel,Modify)
  int last_sent_message_mfm_;

  /// Keeps track of if we have received any message after last send
  bool received_after_last_sent_;

  /// Notify time ( msecs) for listeners about no response from ors
  int last_notify_time_;

  std::vector<ttime_t> cancel_order_seq_time_;
  std::vector<ttime_t> cancel_order_exec_time_;

  BaseOrder template_order;
  BaseOrder cancel_order_;

  // To keep track of modifiable orders
  int top_modifiable_ask_index_;
  int bottom_modifiable_ask_index_;
  int top_modifiable_bid_index_;
  int bottom_modifiable_bid_index_;

  bool use_modify_logic_;

 public:
  BaseOrderManager(DebugLogger& _dbglogger_, const Watch& _watch_, SecurityNameIndexer& _sec_name_indexer_,
                   BaseTrader& _base_trader_, const std::string& _dep_shortcode_, const unsigned int _dep_security_id_,
                   const char* _dep_symbol_, const double _min_price_increment_,
                   int _first_client_assigned_order_sequence_);

  virtual ~BaseOrderManager() {}  ///< needed since GetMidPrice is virtual
  /// function implemented by SmartOrderManager

  void ResetNoResponseFromORSFreeze() {
    NotifyResetByManualInterventionOverRejects();
    reject_based_freeze_timer_ = 0;
  }

  void ResetRejectsBasedFreeze() {
    // Either Timer Has Expired
    // Or External Start Was Given
    // Or External IgnoreFreezeOnRejects Given
    // NOTE : Please Not it's very important that the notify reset is called
    // before
    // resetting the below variables as notification is conditional based on
    // whether reject timer was set
    NotifyResetByManualInterventionOverRejects();

    number_of_consecutive_exchange_rejects_ = 0;
    time_based_total_exchange_rejects_ = 0;
    last_reject_evaluation_time_ = watch_.msecs_from_midnight();
    time_based_total_exchange_rejects_ = 0;
    total_exchange_rejects_ = 0;
    reject_based_freeze_timer_ = 0;
  }

  inline void SetOrderManager(ThrottleManager* t_throttle_manager_) { throttle_manager_ = t_throttle_manager_; }

  inline int GetBidIndex(int t_int_price_) const { return t_int_price_ - bid_int_price_adjustment_; }

  inline int GetAskIndex(int t_int_price_) const {
    return ORDER_MANAGER_INT_PRICE_RANGE - (t_int_price_ - ask_int_price_adjustment_);
  }

  inline bool GetIsCancellableBeforeConfirmation() {
    HFSAT::SecurityDefinitions& security_definition_ = HFSAT::SecurityDefinitions::GetUniqueInstance();
    return security_definition_.cancel_before_conf(dep_symbol_);
  }

  void OnTimePeriodUpdate(const int num_pages_to_add);
  void AddPosition(int exec_quantity, const double price);

  void ResetBidsAsImproveUponCancellation(const int t_best_bid_int_price_, const int t_best_ask_int_price_);
  void ResetAsksAsImproveUponCancellation(const int t_best_bid_int_price_, const int t_best_ask_int_price_);

  inline void SetDisclosedSizeFactor(double disclosed_size_factor) {
    disclosed_size_factor_ = disclosed_size_factor;
    set_disclosed_size_ = true;
  }

 private:
  inline void SetInitialIntPriceAdjustment(int t_int_price_) {
    bid_int_price_adjustment_ = t_int_price_ - ORDER_MANAGER_INT_PRICE_RANGE / 2;
    ask_int_price_adjustment_ = t_int_price_ - ORDER_MANAGER_INT_PRICE_RANGE / 2;

    initial_adjustment_set_ = true;
  }

  inline void ResetBidPriceAdjustment(int t_int_price_) {
    int current_bid_index_ = GetBidIndex(t_int_price_);

    if (current_bid_index_ < 0) {
      int order_vec_top_offset_ = order_vec_top_bid_index_ - current_bid_index_ + ORDER_MANAGER_INT_PRICE_RANGE / 2;
      int confirmed_top_offset_ = confirmed_top_bid_index_ - current_bid_index_ + ORDER_MANAGER_INT_PRICE_RANGE / 2;
      int unconfirmed_top_offset_ = unconfirmed_top_bid_index_ - current_bid_index_ + ORDER_MANAGER_INT_PRICE_RANGE / 2;

      if ((order_vec_top_bid_index_ != -1 && order_vec_top_offset_ > ORDER_MANAGER_INT_PRICE_RANGE) ||
          (confirmed_top_bid_index_ != -1 && confirmed_top_offset_ > ORDER_MANAGER_INT_PRICE_RANGE) ||
          (unconfirmed_top_bid_index_ != -1 && unconfirmed_top_offset_ > ORDER_MANAGER_INT_PRICE_RANGE)) {
        DBGLOG_TIME_CLASS_FUNC << "BaseOrderManager::ResetBidPriceAdjustment: the int_price " << t_int_price_
                               << " for: " << dep_shortcode_ << " is very low.\n"
                               << DBGLOG_ENDL_FLUSH;
        std::cerr << "BaseOrderManager::ResetBidPriceAdjustment: the int_price " << t_int_price_
                  << " for: " << dep_shortcode_ << " is very low.\n"
                  << DBGLOG_ENDL_FLUSH;

        // Compute new int price range
        int max_offset_ = 0;

        if (order_vec_top_bid_index_ != -1 && order_vec_top_offset_ > max_offset_) {
          max_offset_ = order_vec_top_offset_;
        }

        if (confirmed_top_bid_index_ != -1 && confirmed_top_offset_ > max_offset_) {
          max_offset_ = confirmed_top_offset_;
        }

        if (unconfirmed_top_bid_index_ != -1 && unconfirmed_top_offset_ > max_offset_) {
          max_offset_ = unconfirmed_top_offset_;
        }

        int NEW_ORDER_MANAGER_INT_PRICE_RANGE = (max_offset_ - current_bid_index_) * 2;

        int int_price_range_diff_ = NEW_ORDER_MANAGER_INT_PRICE_RANGE - ORDER_MANAGER_INT_PRICE_RANGE;

        ORDER_MANAGER_INT_PRICE_RANGE = NEW_ORDER_MANAGER_INT_PRICE_RANGE;

        // Resize all the vectors
        bid_order_vec_.resize(ORDER_MANAGER_INT_PRICE_RANGE, std::vector<BaseOrder*>());
        sum_bid_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_bid_confirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_total_bid_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_bid_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_bid_unconfirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_total_bid_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);

        ask_order_vec_.resize(ORDER_MANAGER_INT_PRICE_RANGE, std::vector<BaseOrder*>());
        sum_ask_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_ask_confirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_total_ask_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_ask_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_ask_unconfirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_total_ask_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);

        // change ask_index_adjustment_
        ask_int_price_adjustment_ = ask_int_price_adjustment_ - int_price_range_diff_;

        current_bid_index_ = GetBidIndex(t_int_price_);
      }

      // Shift bid_order_vec_
      if (order_vec_bottom_bid_index_ != -1) {
        int order_vec_bottom_offset_ = ORDER_MANAGER_INT_PRICE_RANGE / 2 - current_bid_index_;

        for (int i = order_vec_top_bid_index_; i >= order_vec_bottom_bid_index_; i--) {
          bid_order_vec_[i + order_vec_bottom_offset_] = bid_order_vec_[i];
          bid_order_vec_[i] = std::vector<BaseOrder*>();
        }

        // Reset top/bottom indexes for bid_order_vec_
        order_vec_top_bid_index_ += order_vec_bottom_offset_;
        order_vec_bottom_bid_index_ += order_vec_bottom_offset_;
      }

      // Shift sum_bid_confirmed_ vector
      if (confirmed_bottom_bid_index_ != -1) {
        int confirmed_bottom_offset_ = ORDER_MANAGER_INT_PRICE_RANGE / 2 - current_bid_index_;

        for (int i = confirmed_top_bid_index_; i >= confirmed_bottom_bid_index_; i--) {
          sum_bid_confirmed_[i + confirmed_bottom_offset_] = sum_bid_confirmed_[i];
          sum_bid_confirmed_[i] = 0;
          sum_bid_confirmed_orders_[i + confirmed_bottom_offset_] = sum_bid_confirmed_orders_[i];
          sum_bid_confirmed_orders_[i] = 0;
        }

        // Reset top/bottom indexes for sum_bid_confirmed_
        confirmed_top_bid_index_ += confirmed_bottom_offset_;
        confirmed_bottom_bid_index_ += confirmed_bottom_offset_;
        CalculateSumVectorConfirmed(kTradeTypeBuy, -1);
      }

      // Shift sum_bid_unconfirmed_ vector
      if (unconfirmed_bottom_bid_index_ != -1) {
        int unconfirmed_bottom_offset_ = ORDER_MANAGER_INT_PRICE_RANGE / 2 - current_bid_index_;

        for (int i = unconfirmed_top_bid_index_; i >= unconfirmed_bottom_bid_index_; i--) {
          sum_bid_unconfirmed_[i + unconfirmed_bottom_offset_] = sum_bid_unconfirmed_[i];
          sum_bid_unconfirmed_[i] = 0;
          sum_bid_unconfirmed_orders_[i + unconfirmed_bottom_offset_] = sum_bid_unconfirmed_orders_[i];
          sum_bid_unconfirmed_orders_[i] = 0;
        }

        // Reset top/bottom indexes for sum_bid_unconfirmed_
        unconfirmed_top_bid_index_ += unconfirmed_bottom_offset_;
        unconfirmed_bottom_bid_index_ += unconfirmed_bottom_offset_;
        CalculateSumVectorUnconfirmed(kTradeTypeBuy, -1);
      }

      // Now finally, change the bid_int_price_adjustment_
      bid_int_price_adjustment_ = t_int_price_ - ORDER_MANAGER_INT_PRICE_RANGE / 2;
    } else if (current_bid_index_ >= ORDER_MANAGER_INT_PRICE_RANGE) {
      int order_vec_bottom_offset_ =
          order_vec_bottom_bid_index_ - (current_bid_index_ - ORDER_MANAGER_INT_PRICE_RANGE / 2);
      int confirmed_bottom_offset_ =
          confirmed_bottom_bid_index_ - (current_bid_index_ - ORDER_MANAGER_INT_PRICE_RANGE / 2);
      int unconfirmed_bottom_offset_ =
          unconfirmed_bottom_bid_index_ - (current_bid_index_ - ORDER_MANAGER_INT_PRICE_RANGE / 2);

      if ((order_vec_bottom_bid_index_ != -1 && order_vec_bottom_offset_ < 0) ||
          (confirmed_bottom_bid_index_ != -1 && confirmed_bottom_offset_ < 0) ||
          (unconfirmed_bottom_bid_index_ != -1 && unconfirmed_bottom_offset_ < 0)) {
        std::cerr << "BaseOrderManager::ResetBidPriceAdjustment: the int_price " << t_int_price_
                  << " for: " << dep_shortcode_ << " is very high.\n";
        DBGLOG_TIME_CLASS_FUNC << "BaseOrderManager::ResetBidPriceAdjustment: the int_price " << t_int_price_
                               << " for: " << dep_shortcode_ << " is very high.\n";
        // Compute new int price range
        int min_offset_ = 0;

        if (order_vec_bottom_bid_index_ != -1 && order_vec_bottom_offset_ < min_offset_) {
          min_offset_ = order_vec_bottom_offset_;
        }

        if (confirmed_bottom_bid_index_ != -1 && confirmed_bottom_offset_ < min_offset_) {
          min_offset_ = confirmed_bottom_offset_;
        }

        if (unconfirmed_bottom_bid_index_ != -1 && unconfirmed_bottom_offset_ < min_offset_) {
          min_offset_ = unconfirmed_bottom_offset_;
        }

        int NEW_ORDER_MANAGER_INT_PRICE_RANGE = (current_bid_index_ - min_offset_) * 2;

        int int_price_range_diff_ = NEW_ORDER_MANAGER_INT_PRICE_RANGE - ORDER_MANAGER_INT_PRICE_RANGE;

        ORDER_MANAGER_INT_PRICE_RANGE = NEW_ORDER_MANAGER_INT_PRICE_RANGE;

        // Resize all the vectors
        bid_order_vec_.resize(ORDER_MANAGER_INT_PRICE_RANGE, std::vector<BaseOrder*>());
        sum_bid_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_bid_confirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_total_bid_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_bid_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_bid_unconfirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_total_bid_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);

        ask_order_vec_.resize(ORDER_MANAGER_INT_PRICE_RANGE, std::vector<BaseOrder*>());
        sum_ask_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_ask_confirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_total_ask_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_ask_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_ask_unconfirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_total_ask_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);

        // change ask int price adjustment
        ask_int_price_adjustment_ = ask_int_price_adjustment_ - int_price_range_diff_;

        current_bid_index_ = GetBidIndex(t_int_price_);

        // Since the upper limit of bid_index_ is changed. Return from here
        return;
      }

      // Shift bid_order_vec_
      if (order_vec_top_bid_index_ != -1) {
        int order_vec_top_offset_ = current_bid_index_ - ORDER_MANAGER_INT_PRICE_RANGE / 2;

        for (int i = order_vec_bottom_bid_index_; i <= order_vec_top_bid_index_; i++) {
          bid_order_vec_[i - order_vec_top_offset_] = bid_order_vec_[i];
        }

        // Reset top/bottom indexes for bid_order_vec_
        order_vec_top_bid_index_ -= order_vec_top_offset_;
        order_vec_bottom_bid_index_ -= order_vec_top_offset_;

        // Reset the earlier non-empty positions that are out of range of new
        // top/bottom indexes
        for (int i = 1; i <= order_vec_top_offset_; i++) {
          bid_order_vec_[order_vec_top_bid_index_ + i] = std::vector<BaseOrder*>();
        }
      }

      // Shift sum_bid_confirmed_ vector
      if (confirmed_top_bid_index_ != -1) {
        int confirmed_top_offset_ = current_bid_index_ - ORDER_MANAGER_INT_PRICE_RANGE / 2;

        for (int i = confirmed_bottom_bid_index_; i <= confirmed_top_bid_index_; i++) {
          sum_bid_confirmed_[i - confirmed_top_offset_] = sum_bid_confirmed_[i];
          sum_bid_confirmed_orders_[i - confirmed_top_offset_] = sum_bid_confirmed_[i];
        }

        // Reset top/bottom indexes for sum_bid_confirmed_
        confirmed_top_bid_index_ -= confirmed_top_offset_;
        confirmed_bottom_bid_index_ -= confirmed_top_offset_;

        // Reset the earlier non-empty positions that are out of range of new
        // top/bottom indexes
        for (int i = 1; i <= confirmed_top_offset_; i++) {
          sum_bid_confirmed_[confirmed_top_bid_index_ + i] = 0;
          sum_bid_confirmed_orders_[confirmed_top_bid_index_ + i] = 0;
        }
        CalculateSumVectorConfirmed(kTradeTypeBuy, -1);
      }

      // Shift sum_bid_unconfirmed_ vector
      if (unconfirmed_top_bid_index_ != -1) {
        int unconfirmed_top_offset_ = current_bid_index_ - ORDER_MANAGER_INT_PRICE_RANGE / 2;

        for (int i = unconfirmed_bottom_bid_index_; i <= unconfirmed_top_bid_index_; i++) {
          sum_bid_unconfirmed_[i - unconfirmed_top_offset_] = sum_bid_unconfirmed_[i];
          sum_bid_unconfirmed_orders_[i - unconfirmed_top_offset_] = sum_bid_unconfirmed_orders_[i];
        }

        // Reset top/bottom indexes for sum_bid_unconfirmed_
        unconfirmed_top_bid_index_ -= unconfirmed_top_offset_;
        unconfirmed_bottom_bid_index_ -= unconfirmed_top_offset_;

        // Reset the earlier non-empty positions that are out of range of new
        // top/bottom indexes
        for (int i = 1; i <= unconfirmed_top_offset_; i++) {
          sum_bid_unconfirmed_[unconfirmed_top_bid_index_ + i] = 0;
          sum_bid_unconfirmed_orders_[unconfirmed_top_bid_index_ + i] = 0;
        }
        CalculateSumVectorUnconfirmed(kTradeTypeBuy, -1);
      }

      // Now finally, change the bid_int_price_adjustment_
      bid_int_price_adjustment_ = t_int_price_ - ORDER_MANAGER_INT_PRICE_RANGE / 2;
    }
  }

  inline int GetBidIndexAndAdjustIntPrice(int t_int_price_) {
    if (!initial_adjustment_set_) {
      SetInitialIntPriceAdjustment(t_int_price_);
    }

    int bid_index_ = GetBidIndex(t_int_price_);

    if (bid_index_ < 0 || bid_index_ >= ORDER_MANAGER_INT_PRICE_RANGE) {
      ResetBidPriceAdjustment(t_int_price_);
      bid_index_ = GetBidIndex(t_int_price_);
    }

    return bid_index_;
  }

  inline void AdjustTopBottomOrderVecBidIndexes(int t_bid_index_) {
    if (bid_order_vec_[t_bid_index_].empty()) {
      if (t_bid_index_ == order_vec_top_bid_index_ && t_bid_index_ == order_vec_bottom_bid_index_) {
        order_vec_top_bid_index_ = -1;
        order_vec_bottom_bid_index_ = -1;
      } else if (t_bid_index_ == order_vec_top_bid_index_) {
        order_vec_top_bid_index_--;
        for (; bid_order_vec_[order_vec_top_bid_index_].empty(); order_vec_top_bid_index_--)
          ;
      } else if (t_bid_index_ == order_vec_bottom_bid_index_) {
        order_vec_bottom_bid_index_++;
        for (; bid_order_vec_[order_vec_bottom_bid_index_].empty(); order_vec_bottom_bid_index_++)
          ;
      }
    } else {
      if (order_vec_bottom_bid_index_ == -1) {
        if (order_vec_top_bid_index_ != -1) {
          std::cerr << "top/bottom bid indexes not adjusted\n";
        }
        order_vec_bottom_bid_index_ = t_bid_index_;
        order_vec_top_bid_index_ = t_bid_index_;
      } else if (t_bid_index_ > order_vec_top_bid_index_) {
        order_vec_top_bid_index_ = t_bid_index_;
      } else if (t_bid_index_ < order_vec_bottom_bid_index_) {
        order_vec_bottom_bid_index_ = t_bid_index_;
      }
    }

    if (order_vec_top_bid_index_ < order_vec_bottom_bid_index_) {
      std::cerr << "top_bid_index_ less than bottom index at: " << watch_.tv() << "\n";
    }
  }

  inline void AdjustTopBottomConfirmedBidIndexes(int t_bid_index_) {
    int prev_top_index_ = -1;
    if (sum_bid_confirmed_[t_bid_index_] == 0) {
      if (t_bid_index_ == confirmed_top_bid_index_ && t_bid_index_ == confirmed_bottom_bid_index_) {
        confirmed_top_bid_index_ = -1;
        confirmed_bottom_bid_index_ = -1;
      } else if (t_bid_index_ == confirmed_top_bid_index_) {
        confirmed_top_bid_index_--;
        for (; sum_bid_confirmed_[confirmed_top_bid_index_] == 0; confirmed_top_bid_index_--)
          ;
      } else if (t_bid_index_ == confirmed_bottom_bid_index_) {
        confirmed_bottom_bid_index_++;
        for (; sum_bid_confirmed_[confirmed_bottom_bid_index_] == 0; confirmed_bottom_bid_index_++)
          ;
      }
    } else {
      if (confirmed_bottom_bid_index_ == -1) {
        if (confirmed_top_bid_index_ != -1) {
          std::cerr << "top/bottom bid indexes not adjusted\n";
        }
        confirmed_bottom_bid_index_ = t_bid_index_;
        prev_top_index_ = confirmed_top_bid_index_;
        confirmed_top_bid_index_ = t_bid_index_;
      } else if (t_bid_index_ > confirmed_top_bid_index_) {
        prev_top_index_ = confirmed_top_bid_index_;
        confirmed_top_bid_index_ = t_bid_index_;
      } else if (t_bid_index_ < confirmed_bottom_bid_index_) {
        confirmed_bottom_bid_index_ = t_bid_index_;
      }
    }

    if (confirmed_top_bid_index_ < confirmed_bottom_bid_index_) {
      std::cerr << "top_bid_index_ less than bottom index at: " << watch_.tv() << "\n";
    }

    // -1 happens when top index is not changed or is set to -1
    // in both cases we don't need to recalculate the sum_total vector fully.
    if (prev_top_index_ != -1) CalculateSumVectorConfirmed(kTradeTypeBuy, prev_top_index_);
  }

  inline void AdjustTopBottomUnconfirmedBidIndexes(int t_bid_index_) {
    int prev_top_index_ = -1;
    if (sum_bid_unconfirmed_[t_bid_index_] == 0) {
      if (t_bid_index_ == unconfirmed_top_bid_index_ && t_bid_index_ == unconfirmed_bottom_bid_index_) {
        unconfirmed_top_bid_index_ = -1;
        unconfirmed_bottom_bid_index_ = -1;
      } else if (t_bid_index_ == unconfirmed_top_bid_index_) {
        unconfirmed_top_bid_index_--;
        for (; sum_bid_unconfirmed_[unconfirmed_top_bid_index_] == 0; unconfirmed_top_bid_index_--)
          ;
      } else if (t_bid_index_ == unconfirmed_bottom_bid_index_) {
        unconfirmed_bottom_bid_index_++;
        for (; sum_bid_unconfirmed_[unconfirmed_bottom_bid_index_] == 0; unconfirmed_bottom_bid_index_++)
          ;
      }
    } else {
      if (unconfirmed_bottom_bid_index_ == -1) {
        if (unconfirmed_top_bid_index_ != -1) {
          std::cerr << "top/bottom bid indexes not adjusted\n";
        }
        unconfirmed_bottom_bid_index_ = t_bid_index_;
        prev_top_index_ = unconfirmed_top_bid_index_;
        unconfirmed_top_bid_index_ = t_bid_index_;
      } else if (t_bid_index_ > unconfirmed_top_bid_index_) {
        prev_top_index_ = unconfirmed_top_bid_index_;
        unconfirmed_top_bid_index_ = t_bid_index_;
      } else if (t_bid_index_ < unconfirmed_bottom_bid_index_) {
        unconfirmed_bottom_bid_index_ = t_bid_index_;
      }
    }

    if (unconfirmed_top_bid_index_ < unconfirmed_bottom_bid_index_) {
      std::cerr << "top_bid_index_ less than bottom index at: " << watch_.tv() << "\n";
    }

    // -1 happens when top index is not changed or is set to -1
    // in both cases we don't need to recalculate the sum_total vector fully.
    if (prev_top_index_ != -1) CalculateSumVectorUnconfirmed(kTradeTypeBuy, prev_top_index_);
  }

  inline void ResetAskPriceAdjustment(int t_int_price_) {
    int current_ask_index_ = GetAskIndex(t_int_price_);

    if (current_ask_index_ < 0) {
      int order_vec_top_offset_ = order_vec_top_ask_index_ - current_ask_index_ + ORDER_MANAGER_INT_PRICE_RANGE / 2;
      int confirmed_top_offset_ = confirmed_top_ask_index_ - current_ask_index_ + ORDER_MANAGER_INT_PRICE_RANGE / 2;
      int unconfirmed_top_offset_ = unconfirmed_top_ask_index_ - current_ask_index_ + ORDER_MANAGER_INT_PRICE_RANGE / 2;

      if ((order_vec_top_ask_index_ != -1 && order_vec_top_offset_ > ORDER_MANAGER_INT_PRICE_RANGE) ||
          (confirmed_top_ask_index_ != -1 && confirmed_top_offset_ > ORDER_MANAGER_INT_PRICE_RANGE) ||
          (unconfirmed_top_ask_index_ != -1 && unconfirmed_top_offset_ > ORDER_MANAGER_INT_PRICE_RANGE)) {
        std::cerr << "BaseOrderManager::ResetAskPriceAdjustment: the int_price" << t_int_price_
                  << " for: " << dep_shortcode_ << " is very low.\n";
        DBGLOG_TIME_CLASS_FUNC << "BaseOrderManager::ResetAskPriceAdjustment: the int_price" << t_int_price_
                               << " for: " << dep_shortcode_ << " is very low.\n";
        // Compute new int price range
        int max_offset_ = 0;

        if (order_vec_top_ask_index_ != -1 && order_vec_top_offset_ > max_offset_) {
          max_offset_ = order_vec_top_offset_;
        }

        if (confirmed_top_ask_index_ != -1 && confirmed_top_offset_ > max_offset_) {
          max_offset_ = confirmed_top_offset_;
        }

        if (unconfirmed_top_ask_index_ != -1 && unconfirmed_top_offset_ > max_offset_) {
          max_offset_ = unconfirmed_top_offset_;
        }

        int NEW_ORDER_MANAGER_INT_PRICE_RANGE = (max_offset_ - current_ask_index_) * 2;

        int int_price_range_diff_ = NEW_ORDER_MANAGER_INT_PRICE_RANGE - ORDER_MANAGER_INT_PRICE_RANGE;

        ORDER_MANAGER_INT_PRICE_RANGE = NEW_ORDER_MANAGER_INT_PRICE_RANGE;

        // Resize all the vectors
        bid_order_vec_.resize(ORDER_MANAGER_INT_PRICE_RANGE, std::vector<BaseOrder*>());
        sum_bid_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_bid_confirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_total_bid_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_bid_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_bid_unconfirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_total_bid_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);

        ask_order_vec_.resize(ORDER_MANAGER_INT_PRICE_RANGE, std::vector<BaseOrder*>());
        sum_ask_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_ask_confirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_total_ask_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_ask_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_ask_unconfirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_total_ask_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);

        // change ask int price adjustment
        ask_int_price_adjustment_ = ask_int_price_adjustment_ - int_price_range_diff_;

        current_ask_index_ = GetAskIndex(t_int_price_);
      }

      // Shift ask_order_vec_
      if (order_vec_bottom_ask_index_ != -1) {
        int order_vec_bottom_offset_ = ORDER_MANAGER_INT_PRICE_RANGE / 2 - current_ask_index_;

        for (int i = order_vec_top_ask_index_; i >= order_vec_bottom_ask_index_; i--) {
          ask_order_vec_[i + order_vec_bottom_offset_] = ask_order_vec_[i];
          ask_order_vec_[i] = std::vector<BaseOrder*>();
        }

        // Reset top/bottom indexes for ask_order_vec_
        order_vec_top_ask_index_ += order_vec_bottom_offset_;
        order_vec_bottom_ask_index_ += order_vec_bottom_offset_;
      }

      // Shift sum_ask_confirmed_ vector
      if (confirmed_bottom_ask_index_ != -1) {
        int confirmed_bottom_offset_ = ORDER_MANAGER_INT_PRICE_RANGE / 2 - current_ask_index_;

        for (int i = confirmed_top_ask_index_; i >= confirmed_bottom_ask_index_; i--) {
          sum_ask_confirmed_[i + confirmed_bottom_offset_] = sum_ask_confirmed_[i];
          sum_ask_confirmed_[i] = 0;
          sum_ask_confirmed_orders_[i + confirmed_bottom_offset_] = sum_ask_confirmed_orders_[i];
          sum_ask_confirmed_orders_[i] = 0;
        }

        // Reset top/bottom indexes for sum_ask_confirmed_
        confirmed_top_ask_index_ += confirmed_bottom_offset_;
        confirmed_bottom_ask_index_ += confirmed_bottom_offset_;
        CalculateSumVectorConfirmed(kTradeTypeSell, -1);
      }

      // Shift sum_ask_unconfirmed_ vector
      if (unconfirmed_bottom_ask_index_ != -1) {
        int unconfirmed_bottom_offset_ = ORDER_MANAGER_INT_PRICE_RANGE / 2 - current_ask_index_;

        for (int i = unconfirmed_top_ask_index_; i >= unconfirmed_bottom_ask_index_; i--) {
          sum_ask_unconfirmed_[i + unconfirmed_bottom_offset_] = sum_ask_unconfirmed_[i];
          sum_ask_unconfirmed_[i] = 0;
          sum_ask_unconfirmed_orders_[i + unconfirmed_bottom_offset_] = sum_ask_unconfirmed_orders_[i];
          sum_ask_unconfirmed_orders_[i] = 0;
        }

        // Reset top/bottom indexes for sum_ask_unconfirmed_
        unconfirmed_top_ask_index_ += unconfirmed_bottom_offset_;
        unconfirmed_bottom_ask_index_ += unconfirmed_bottom_offset_;
        CalculateSumVectorUnconfirmed(kTradeTypeSell, -1);
      }

      // Now finally, change the ask_int_price_adjustment_
      ask_int_price_adjustment_ = t_int_price_ - ORDER_MANAGER_INT_PRICE_RANGE / 2;
    }

    else if (current_ask_index_ >= ORDER_MANAGER_INT_PRICE_RANGE) {
      int order_vec_bottom_offset_ =
          order_vec_bottom_ask_index_ - (current_ask_index_ - ORDER_MANAGER_INT_PRICE_RANGE / 2);
      int confirmed_bottom_offset_ =
          confirmed_bottom_ask_index_ - (current_ask_index_ - ORDER_MANAGER_INT_PRICE_RANGE / 2);
      int unconfirmed_bottom_offset_ =
          unconfirmed_bottom_ask_index_ - (current_ask_index_ - ORDER_MANAGER_INT_PRICE_RANGE / 2);

      if ((order_vec_bottom_ask_index_ != -1 && order_vec_bottom_offset_ < 0) ||
          (confirmed_bottom_ask_index_ != -1 && confirmed_bottom_offset_ < 0) ||
          (unconfirmed_bottom_ask_index_ != -1 && unconfirmed_bottom_offset_ < 0)) {
        std::cerr << "BaseOrderManager::ResetAskPriceAdjustment: the int_price " << t_int_price_
                  << " for: " << dep_shortcode_ << " is very high.\n";
        DBGLOG_TIME_CLASS_FUNC << "BaseOrderManager::ResetAskPriceAdjustment: the int_price " << t_int_price_
                               << " for: " << dep_shortcode_ << " is very high.\n";
        // Compute new int price range
        int min_offset_ = 0;

        if (order_vec_bottom_ask_index_ != -1 && order_vec_bottom_offset_ < min_offset_) {
          min_offset_ = order_vec_bottom_offset_;
        }

        if (confirmed_bottom_ask_index_ != -1 && confirmed_bottom_offset_ < min_offset_) {
          min_offset_ = confirmed_bottom_offset_;
        }

        if (unconfirmed_bottom_ask_index_ != -1 && unconfirmed_bottom_offset_ < min_offset_) {
          min_offset_ = unconfirmed_bottom_offset_;
        }

        int NEW_ORDER_MANAGER_INT_PRICE_RANGE = (current_ask_index_ - min_offset_) * 2;

        int int_price_range_diff_ = NEW_ORDER_MANAGER_INT_PRICE_RANGE - ORDER_MANAGER_INT_PRICE_RANGE;

        ORDER_MANAGER_INT_PRICE_RANGE = NEW_ORDER_MANAGER_INT_PRICE_RANGE;

        // Resize all the vectors
        bid_order_vec_.resize(ORDER_MANAGER_INT_PRICE_RANGE, std::vector<BaseOrder*>());
        sum_bid_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_bid_confirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_total_bid_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_bid_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_bid_unconfirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_total_bid_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);

        ask_order_vec_.resize(ORDER_MANAGER_INT_PRICE_RANGE, std::vector<BaseOrder*>());
        sum_ask_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_ask_confirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_total_ask_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_ask_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_ask_unconfirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
        sum_total_ask_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);

        // Change ask_int_price_adjustment_ to keep the current indexes valid
        ask_int_price_adjustment_ = ask_int_price_adjustment_ - int_price_range_diff_;

        current_ask_index_ = GetAskIndex(t_int_price_);

        // Since the upper limit of ask_index_ is changed, return from here.
        return;
      }

      // Shift ask_order_vec_
      if (order_vec_top_ask_index_ != -1) {
        int order_vec_top_offset_ = current_ask_index_ - ORDER_MANAGER_INT_PRICE_RANGE / 2;

        for (int i = order_vec_bottom_ask_index_; i <= order_vec_top_ask_index_; i++) {
          ask_order_vec_[i - order_vec_top_offset_] = ask_order_vec_[i];
        }

        // Reset top/bottom indexes for ask_order_vec_
        order_vec_top_ask_index_ -= order_vec_top_offset_;
        order_vec_bottom_ask_index_ -= order_vec_top_offset_;

        // Reset the earlier non-empty positions that are out of range of new
        // top/bottom indexes
        for (int i = 1; i <= order_vec_top_offset_; i++) {
          ask_order_vec_[order_vec_top_ask_index_ + i] = std::vector<BaseOrder*>();
        }
      }

      // Shift sum_ask_confirmed_ vector
      if (confirmed_top_ask_index_ != -1) {
        int confirmed_top_offset_ = current_ask_index_ - ORDER_MANAGER_INT_PRICE_RANGE / 2;

        for (int i = confirmed_bottom_ask_index_; i <= confirmed_top_ask_index_; i++) {
          sum_ask_confirmed_[i - confirmed_top_offset_] = sum_ask_confirmed_[i];
          sum_ask_confirmed_orders_[i - confirmed_top_offset_] = sum_ask_confirmed_orders_[i];
        }

        // Reset top/bottom indexes for sum_ask_confirmed_
        confirmed_top_ask_index_ -= confirmed_top_offset_;
        confirmed_bottom_ask_index_ -= confirmed_top_offset_;

        // Reset the earlier non-empty positions that are out of range of new
        // top/bottom indexes
        for (int i = 1; i <= confirmed_top_offset_; i++) {
          sum_ask_confirmed_[confirmed_top_ask_index_ + i] = 0;
          sum_ask_confirmed_orders_[confirmed_top_ask_index_ + i] = 0;
        }
        CalculateSumVectorConfirmed(kTradeTypeSell, -1);
      }

      // Shift sum_ask_unconfirmed_ vector
      if (unconfirmed_top_ask_index_ != -1) {
        int unconfirmed_top_offset_ = current_ask_index_ - ORDER_MANAGER_INT_PRICE_RANGE / 2;

        for (int i = unconfirmed_bottom_ask_index_; i <= unconfirmed_top_ask_index_; i++) {
          sum_ask_unconfirmed_[i - unconfirmed_top_offset_] = sum_ask_unconfirmed_[i];
          sum_ask_unconfirmed_orders_[i - unconfirmed_top_offset_] = sum_ask_unconfirmed_orders_[i];
        }

        // Reset top/bottom indexes for sum_ask_unconfirmed_
        unconfirmed_top_ask_index_ -= unconfirmed_top_offset_;
        unconfirmed_bottom_ask_index_ -= unconfirmed_top_offset_;

        // Reset the earlier non-empty positions that are out of range of new
        // top/bottom indexes
        for (int i = 1; i <= unconfirmed_top_offset_; i++) {
          sum_ask_unconfirmed_[unconfirmed_top_ask_index_ + i] = 0;
          sum_ask_unconfirmed_orders_[unconfirmed_top_ask_index_ + i] = 0;
        }
        CalculateSumVectorUnconfirmed(kTradeTypeSell, -1);
      }

      // Now finally, change the ask_int_price_adjustment_
      ask_int_price_adjustment_ = t_int_price_ - ORDER_MANAGER_INT_PRICE_RANGE / 2;
    }
  }

  inline int GetAskIndexAndAdjustIntPrice(int t_int_price_) {
    if (!initial_adjustment_set_) {
      SetInitialIntPriceAdjustment(t_int_price_);
    }

    int ask_index_ = GetAskIndex(t_int_price_);

    if (ask_index_ < 0 || ask_index_ >= ORDER_MANAGER_INT_PRICE_RANGE) {
      ResetAskPriceAdjustment(t_int_price_);
      ask_index_ = GetAskIndex(t_int_price_);
    }

    return ask_index_;
  }

  inline void AdjustTopBottomOrderVecAskIndexes(int t_ask_index_) {
    if (ask_order_vec_[t_ask_index_].empty()) {
      if (t_ask_index_ == order_vec_top_ask_index_ && t_ask_index_ == order_vec_bottom_ask_index_) {
        order_vec_top_ask_index_ = -1;
        order_vec_bottom_ask_index_ = -1;
      } else if (t_ask_index_ == order_vec_top_ask_index_) {
        order_vec_top_ask_index_--;
        for (; ask_order_vec_[order_vec_top_ask_index_].empty(); order_vec_top_ask_index_--)
          ;
      } else if (t_ask_index_ == order_vec_bottom_ask_index_) {
        order_vec_bottom_ask_index_++;
        for (; ask_order_vec_[order_vec_bottom_ask_index_].empty(); order_vec_bottom_ask_index_++)
          ;
      }
    } else {
      if (order_vec_bottom_ask_index_ == -1) {
        if (order_vec_top_ask_index_ != -1) {
          std::cerr << "top/bottom as indexes not adjusted\n";
        }
        order_vec_bottom_ask_index_ = t_ask_index_;
        order_vec_top_ask_index_ = t_ask_index_;
      } else if (t_ask_index_ > order_vec_top_ask_index_) {
        order_vec_top_ask_index_ = t_ask_index_;
      } else if (t_ask_index_ < order_vec_bottom_ask_index_) {
        order_vec_bottom_ask_index_ = t_ask_index_;
      }
    }

    if (order_vec_top_ask_index_ < order_vec_bottom_ask_index_) {
      std::cerr << "top_ask_index_ less than bottom index at: " << watch_.tv() << "\n";
    }
  }

  inline void AdjustTopBottomConfirmedAskIndexes(int t_ask_index_) {
    int prev_top_index_ = -1;
    if (sum_ask_confirmed_[t_ask_index_] == 0) {
      if (t_ask_index_ == confirmed_top_ask_index_ && t_ask_index_ == confirmed_bottom_ask_index_) {
        confirmed_top_ask_index_ = -1;
        confirmed_bottom_ask_index_ = -1;
      } else if (t_ask_index_ == confirmed_top_ask_index_) {
        confirmed_top_ask_index_--;
        for (; sum_ask_confirmed_[confirmed_top_ask_index_] == 0; confirmed_top_ask_index_--)
          ;
      } else if (t_ask_index_ == confirmed_bottom_ask_index_) {
        confirmed_bottom_ask_index_++;
        for (; sum_ask_confirmed_[confirmed_bottom_ask_index_] == 0; confirmed_bottom_ask_index_++)
          ;
      }
    } else {
      if (confirmed_bottom_ask_index_ == -1) {
        if (confirmed_top_ask_index_ != -1) {
          std::cerr << "top/bottom ask indexes not adjusted\n";
        }
        confirmed_bottom_ask_index_ = t_ask_index_;
        prev_top_index_ = confirmed_top_ask_index_;
        confirmed_top_ask_index_ = t_ask_index_;
      } else if (t_ask_index_ > confirmed_top_ask_index_) {
        prev_top_index_ = confirmed_top_ask_index_;
        confirmed_top_ask_index_ = t_ask_index_;
      } else if (t_ask_index_ < confirmed_bottom_ask_index_) {
        confirmed_bottom_ask_index_ = t_ask_index_;
      }
    }

    if (confirmed_top_ask_index_ < confirmed_bottom_ask_index_) {
      std::cerr << "top_ask_index_ less than bottom index at: " << watch_.tv() << "\n";
    }

    // -1 happens when top index is not changed or is set to -1
    // in both cases we don't need to recalculate the sum_total vector fully.
    if (prev_top_index_ != -1) CalculateSumVectorConfirmed(kTradeTypeSell, prev_top_index_);
  }

  inline void AdjustTopBottomUnconfirmedAskIndexes(int t_ask_index_) {
    int prev_top_index_ = -1;
    if (sum_ask_unconfirmed_[t_ask_index_] == 0) {
      if (t_ask_index_ == unconfirmed_top_ask_index_ && t_ask_index_ == unconfirmed_bottom_ask_index_) {
        unconfirmed_top_ask_index_ = -1;
        unconfirmed_bottom_ask_index_ = -1;
      } else if (t_ask_index_ == unconfirmed_top_ask_index_) {
        unconfirmed_top_ask_index_--;
        for (; sum_ask_unconfirmed_[unconfirmed_top_ask_index_] == 0; unconfirmed_top_ask_index_--)
          ;
      } else if (t_ask_index_ == unconfirmed_bottom_ask_index_) {
        unconfirmed_bottom_ask_index_++;
        for (; sum_ask_unconfirmed_[unconfirmed_bottom_ask_index_] == 0; unconfirmed_bottom_ask_index_++)
          ;
      }
    } else {
      if (unconfirmed_bottom_ask_index_ == -1) {
        if (unconfirmed_top_ask_index_ != -1) {
          std::cerr << "top/bottom ask indexes not adjusted\n";
        }
        unconfirmed_bottom_ask_index_ = t_ask_index_;
        prev_top_index_ = unconfirmed_top_ask_index_;
        unconfirmed_top_ask_index_ = t_ask_index_;
      } else if (t_ask_index_ > unconfirmed_top_ask_index_) {
        prev_top_index_ = unconfirmed_top_ask_index_;
        unconfirmed_top_ask_index_ = t_ask_index_;
      } else if (t_ask_index_ < unconfirmed_bottom_ask_index_) {
        unconfirmed_bottom_ask_index_ = t_ask_index_;
      }
    }

    if (unconfirmed_top_ask_index_ < unconfirmed_bottom_ask_index_) {
      std::cerr << "top_ask_index_ less than bottom index at: " << watch_.tv() << "\n";
    }

    // -1 happens when top index is not changed or is set to -1
    // in both cases we don't need to recalculate the sum_total vector fully.
    if (prev_top_index_ != -1) CalculateSumVectorUnconfirmed(kTradeTypeSell, prev_top_index_);
  }

  /*
   Event types:
   P = order place confirmed
   C = order changed (cancelled, executed, modified)
  */
  void AdjustModifiableBidIndices(char event_type, int index) {
    if (event_type == 'P') {
      BaseOrder* t_order = GetTopModifiableBidOrder();
      if (t_order == NULL || index > top_modifiable_bid_index_) {
        BaseOrder* new_order = GetBidOrderAtIndex(index);
        if (new_order->CanBeModified(watch_.msecs_from_midnight() - MODIFY_WAIT_MSECS))
          top_modifiable_bid_index_ = index;
      }
      t_order = GetBottomModifiableBidOrder();
      if (t_order == NULL || index < bottom_modifiable_bid_index_) {
        BaseOrder* new_order = GetBidOrderAtIndex(index);
        if (new_order->CanBeModified(watch_.msecs_from_midnight() - MODIFY_WAIT_MSECS))
          bottom_modifiable_bid_index_ = index;
      }
    } else if (event_type == 'C') {
      BaseOrder* t_order = GetTopModifiableBidOrder();
      if (t_order == NULL) {
        top_modifiable_bid_index_ = -1;
        if (order_vec_top_bid_index_ != -1) {
          for (int i_index = order_vec_top_bid_index_; i_index >= order_vec_bottom_bid_index_; i_index--) {
            BaseOrder* t_order = GetBidOrderAtIndex(i_index);
            if (t_order != NULL && t_order->CanBeModified(watch_.msecs_from_midnight() - MODIFY_WAIT_MSECS)) {
              top_modifiable_bid_index_ = i_index;
              break;
            }
          }
        }
      }
      t_order = GetBottomModifiableBidOrder();
      if (t_order == NULL) {
        bottom_modifiable_bid_index_ = -1;
        if (order_vec_bottom_bid_index_ != -1) {
          for (int i_index = order_vec_bottom_bid_index_; i_index <= order_vec_top_bid_index_; i_index++) {
            BaseOrder* t_order = GetBidOrderAtIndex(i_index);
            if (t_order != NULL && t_order->CanBeModified(watch_.msecs_from_midnight() - MODIFY_WAIT_MSECS)) {
              bottom_modifiable_bid_index_ = i_index;
              break;
            }
          }
        }
      }
    }

    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " New top_modifiable_bid_price_: " << GetBidIntPrice(top_modifiable_bid_index_) << "\n"
                             << " New bottom_modifiable_bid_price_: " << GetBidIntPrice(bottom_modifiable_bid_index_)
                             << "\n"
                             << DBGLOG_ENDL_FLUSH;
    }
  }

  void AdjustModifiableAskIndices(char event_type, int index) {
    if (event_type == 'P') {
      BaseOrder* t_order = GetTopModifiableAskOrder();
      if (t_order == NULL || index > top_modifiable_ask_index_) {
        BaseOrder* new_order = GetAskOrderAtIndex(index);
        if (new_order->CanBeModified(watch_.msecs_from_midnight() - MODIFY_WAIT_MSECS))
          top_modifiable_ask_index_ = index;
      }
      t_order = GetBottomModifiableAskOrder();
      if (t_order == NULL || index < bottom_modifiable_ask_index_) {
        BaseOrder* new_order = GetAskOrderAtIndex(index);
        if (new_order->CanBeModified(watch_.msecs_from_midnight() - MODIFY_WAIT_MSECS))
          bottom_modifiable_ask_index_ = index;
      }
    } else if (event_type == 'C') {
      BaseOrder* t_order = GetTopModifiableAskOrder();
      if (t_order == NULL) {
        top_modifiable_ask_index_ = -1;
        if (order_vec_top_ask_index_ != -1) {
          for (int i_index = order_vec_top_ask_index_; i_index >= order_vec_bottom_ask_index_; i_index--) {
            BaseOrder* t_order = GetAskOrderAtIndex(i_index);
            if (t_order != NULL && t_order->CanBeModified(watch_.msecs_from_midnight() - MODIFY_WAIT_MSECS)) {
              top_modifiable_ask_index_ = i_index;
              break;
            }
          }
        }
      }
      t_order = GetBottomModifiableAskOrder();
      if (t_order == NULL) {
        bottom_modifiable_ask_index_ = -1;
        if (order_vec_bottom_ask_index_ != -1) {
          for (int i_index = order_vec_bottom_ask_index_; i_index <= order_vec_top_ask_index_; i_index++) {
            BaseOrder* t_order = GetAskOrderAtIndex(i_index);
            if (t_order != NULL && t_order->CanBeModified(watch_.msecs_from_midnight() - MODIFY_WAIT_MSECS)) {
              bottom_modifiable_ask_index_ = i_index;
              break;
            }
          }
        }
      }
    }

    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " New top_modifiable_ask_price_: " << GetAskIntPrice(top_modifiable_ask_index_) << "\n"
                             << " New bottom_modifiable_ask_price_: " << GetAskIntPrice(bottom_modifiable_ask_index_)
                             << "\n"
                             << DBGLOG_ENDL_FLUSH;
    }
  }

 public:
  inline int GetFokBidOrderSize() { return foked_bid_order_size_sum_; }
  inline int GetFokAskOrderSize() { return foked_ask_order_size_sum_; }

  inline int GetBidIntPrice(int t_bid_index_) const { return t_bid_index_ + bid_int_price_adjustment_; }

  inline int GetAskIntPrice(int t_ask_index_) const {
    return ORDER_MANAGER_INT_PRICE_RANGE - (t_ask_index_ - ask_int_price_adjustment_);
  }

  void SetKeepBidIntPrice(int* t_ticks_to_keep_bid_int_price_) {
    p_ticks_to_keep_bid_int_price_ = t_ticks_to_keep_bid_int_price_;
  }
  void SetKeepAskIntPrice(int* t_ticks_to_keep_ask_int_price_) {
    p_ticks_to_keep_ask_int_price_ = t_ticks_to_keep_ask_int_price_;
  }

  std::string OMToString();

  inline void InvalidateSecurityFamilyMap() {
    security_id_to_last_position_.resize(sec_name_indexer_.NumSecurityId(), kInvalidPosition);
    security_position_ = 0;
  }

  inline void AddToSecurityFamilyMap(const unsigned int security_id_) {
    security_id_to_last_position_[security_id_] = 0;
  }

  inline void SetAggTradingCooloff(bool t_agg_trading_cooloff_) {
    agg_trading_cooloff_ = t_agg_trading_cooloff_;

    DumpAggVariables();
  }

  inline bool AggTradingCooloff() const { return agg_trading_cooloff_; }

  inline void InitializeAggVariables() {
    agg_time_frame_msecs_ = AGG_TIME_FRAME_MSECS;
    agg_size_threshold_ = AGG_SIZE_THRESHOLD;
    agg_trading_cooloff_ = true;
    disable_orders_due_to_agg_cooloff_ = false;
    agg_cooloff_stop_msecs_ = 0;
    unintentional_agg_sizes_ = 0;
    email_sent_ = false;

    for (int i = 0; i < kTradeTypeNoInfo; ++i) {
      intentional_agg_lasttime_[i] = 0;
      intentional_agg_sizes_[i] = 0;
      last_exec_int_price_[i] = 0;
      last_exec_time_[i] = 0;
    }

    if (!dep_shortcode_.compare("FESX_0")) {
      agg_time_frame_msecs_ = 1000;
      agg_size_threshold_ = 10;
    }

    if (!dep_shortcode_.compare("ZN_0") || !dep_shortcode_.compare("UB_0") || !dep_shortcode_.compare("ZF_0") ||
        !dep_shortcode_.compare("ZB_0")) {
      agg_time_frame_msecs_ = 1000;
      agg_size_threshold_ = 10;
    }
  }

  inline std::string AggVariablesToString() const {
    std::ostringstream t_oss_;

    t_oss_ << " agg_trading_cooloff_=" << (agg_trading_cooloff_ ? "true" : "false")
           << " disable_orders_due_to_agg_cooloff_=" << (disable_orders_due_to_agg_cooloff_ ? "true" : "false")
           << " agg_cooloff_stop_msecs_=" << agg_cooloff_stop_msecs_ << "\n"
           << " intentional_agg_sizes_ [ B ]=" << intentional_agg_sizes_[kTradeTypeBuy]
           << " intentional_agg_sizes_ [ S ]=" << intentional_agg_sizes_[kTradeTypeSell]
           << " intentional_agg_lasttime_ [ B ]=" << intentional_agg_lasttime_[kTradeTypeBuy]
           << " intentional_agg_lasttime_ [ S ]=" << intentional_agg_lasttime_[kTradeTypeSell] << "\n"
           << " last_exec_int_price_ [ B ]=" << last_exec_int_price_[kTradeTypeBuy]
           << " last_exec_int_price_ [ S ]=" << last_exec_int_price_[kTradeTypeSell] << "\n"
           << " last_exec_time_ [ B ]=" << last_exec_time_[kTradeTypeBuy]
           << " last_exec_time_ [ S ]=" << last_exec_time_[kTradeTypeSell] << "\n"
           << " unintentional_agg_sizes_=" << unintentional_agg_sizes_ << "\n"
           << " agg_size_threshold_=" << agg_size_threshold_ << " agg_time_frame_msecs_=" << agg_time_frame_msecs_;

    return t_oss_.str();
  }

  inline void DumpAggVariables() const {
    // DBGLOG_TIME_CLASS_FUNC << AggVariablesToString ( ) << DBGLOG_ENDL_FLUSH;
  }

  inline void EmailAggVariablesOnDisable() const {
#if EMAIL_ON_AGG_COOLOFF
    char hostname_[128];
    hostname_[127] = '\0';
    gethostname(hostname_, 127);

    std::string email_string_ = "";
    std::string email_address_ = "";
    {
      std::ostringstream t_oss_;

      t_oss_ << "Disabling Orders on " << hostname_ << " for " << dep_shortcode_ << "\n";

      email_string_ = t_oss_.str();
    }

    HFSAT::Email email_;
    email_.setSubject(email_string_);

    email_address_ = "sghosh@circulumvite.com";

    email_.addRecepient(email_address_);
    email_.addSender(email_address_);
    email_.content_stream << email_string_ << "<br/>";
    email_.content_stream << AggVariablesToString() << "<br/>";
    email_.sendMail();
#endif
  }

  // Accessors called by Strategy classes
  inline int GetTotalBidSizeOrderedAtIntPx(const int intpx) const {
    return intpx_2_sum_bid_confirmed(intpx) + intpx_2_sum_bid_unconfirmed(intpx);
  }

  inline int GetTotalAskSizeOrderedAtIntPx(const int intpx) const {
    return intpx_2_sum_ask_confirmed(intpx) + intpx_2_sum_ask_unconfirmed(intpx);
  }

  inline int GetTotalBidSizeAboveIntPx(const int intpx) const {
    return SumBidSizeConfirmedAboveIntPrice(intpx) + SumBidSizeUnconfirmedAboveIntPrice(intpx);
  }

  inline int GetTotalAskSizeAboveIntPx(const int intpx) const {
    return SumAskSizeConfirmedAboveIntPrice(intpx) + SumAskSizeUnconfirmedAboveIntPrice(intpx);
  }

  inline int GetTotalBidSizeEqAboveIntPx(const int intpx) const {
    return SumBidSizeConfirmedEqAboveIntPrice(intpx) + SumBidSizeUnconfirmedEqAboveIntPrice(intpx);
  }

  inline int GetTotalAskSizeEqAboveIntPx(const int intpx) const {
    return SumAskSizeConfirmedEqAboveIntPrice(intpx) + SumAskSizeUnconfirmedEqAboveIntPrice(intpx);
  }

  /**
   * Get number of bid orders at given price
   * @param intpx
   * @return
   */
  inline int GetNumBidOrdersAtIntPx(const int intpx) const {
    auto bid_index = GetBidIndex(intpx);
    return (bid_index >= 0 && bid_index < ORDER_MANAGER_INT_PRICE_RANGE)
               ? intpx_2_sum_bid_confirmed_orders(intpx) + intpx_2_sum_bid_unconfirmed_orders(intpx)
               : 0;
  }

  /**
   * get number of ask-orders at given price
   * @param intpx
   * @return
   */
  inline int GetNumAskOrdersAtIntPx(const int intpx) const {
    auto ask_index = GetAskIndex(intpx);
    return (ask_index >= 0 && ask_index < ORDER_MANAGER_INT_PRICE_RANGE)
               ? intpx_2_sum_ask_confirmed_orders(intpx) + +intpx_2_sum_ask_unconfirmed_orders(intpx)
               : 0;
  }

  inline std::vector<int>& SumBidConfirmed() { return sum_bid_confirmed_; }
  inline std::vector<int>& SumBidConfirmedOrders() { return sum_bid_confirmed_orders_; }
  inline int GetConfirmedTopBidIndex() { return confirmed_top_bid_index_; }
  inline int GetConfirmedBottomBidIndex() { return confirmed_bottom_bid_index_; }

  inline std::vector<int>& SumBidUnconfirmed() { return sum_bid_unconfirmed_; }
  inline std::vector<int>& SumBidUnconfirmedOrders() { return sum_bid_unconfirmed_orders_; }
  inline int GetUnconfirmedTopBidIndex() { return unconfirmed_top_bid_index_; }
  inline int GetUnconfirmedBottomBidIndex() { return unconfirmed_bottom_bid_index_; }

  inline std::vector<std::vector<BaseOrder*> >& BidOrderVec() { return bid_order_vec_; }
  inline int GetOrderVecTopBidIndex() { return order_vec_top_bid_index_; }
  inline int GetOrderVecBottomBidIndex() { return order_vec_bottom_bid_index_; }

  inline std::vector<int>& SumAskConfirmed() { return sum_ask_confirmed_; }
  inline std::vector<int>& SumAskConfirmedOrders() { return sum_ask_confirmed_orders_; }
  inline int GetConfirmedTopAskIndex() { return confirmed_top_ask_index_; }
  inline int GetConfirmedBottomAskIndex() { return confirmed_bottom_ask_index_; }

  inline std::vector<int>& SumAskUnconfirmed() { return sum_ask_unconfirmed_; }
  inline std::vector<int>& SumAskUnconfirmedOrders() { return sum_ask_unconfirmed_orders_; }
  inline int GetUnconfirmedTopAskIndex() { return unconfirmed_top_ask_index_; }
  inline int GetUnconfirmedBottomAskIndex() { return unconfirmed_bottom_ask_index_; }

  inline std::vector<std::vector<BaseOrder*> >& AskOrderVec() { return ask_order_vec_; }
  inline int GetOrderVecTopAskIndex() { return order_vec_top_ask_index_; }
  inline int GetOrderVecBottomAskIndex() { return order_vec_bottom_ask_index_; }

  inline int intpx_2_sum_bid_confirmed(const int _intpx_) const {
    int bid_index_ = GetBidIndex(_intpx_);
    return (bid_index_ >= 0 && bid_index_ < ORDER_MANAGER_INT_PRICE_RANGE) ? sum_bid_confirmed_[bid_index_] : 0;
  }

  inline int intpx_2_sum_bid_confirmed_orders(const int _intpx_) const {
    int bid_index_ = GetBidIndex(_intpx_);
    return (bid_index_ >= 0 && bid_index_ < ORDER_MANAGER_INT_PRICE_RANGE) ? sum_bid_confirmed_orders_[bid_index_] : 0;
  }

  inline int intpx_2_sum_bid_unconfirmed(const int _intpx_) const {
    int bid_index_ = GetBidIndex(_intpx_);
    return (bid_index_ >= 0 && bid_index_ < ORDER_MANAGER_INT_PRICE_RANGE) ? sum_bid_unconfirmed_[bid_index_] : 0;
  }

  inline int intpx_2_sum_bid_unconfirmed_orders(const int _intpx_) const {
    int bid_index_ = GetBidIndex(_intpx_);
    return (bid_index_ >= 0 && bid_index_ < ORDER_MANAGER_INT_PRICE_RANGE) ? sum_bid_unconfirmed_orders_[bid_index_]
                                                                           : 0;
  }

  inline int intpx_2_sum_ask_confirmed(const int _intpx_) const {
    int ask_index_ = GetAskIndex(_intpx_);
    return (ask_index_ >= 0 && ask_index_ < ORDER_MANAGER_INT_PRICE_RANGE) ? sum_ask_confirmed_[ask_index_] : 0;
  }

  inline int intpx_2_sum_ask_confirmed_orders(const int _intpx_) const {
    int ask_index_ = GetAskIndex(_intpx_);
    return (ask_index_ >= 0 && ask_index_ < ORDER_MANAGER_INT_PRICE_RANGE) ? sum_ask_confirmed_orders_[ask_index_] : 0;
  }
  inline int intpx_2_sum_ask_unconfirmed(const int _intpx_) const {
    int ask_index_ = GetAskIndex(_intpx_);
    return (ask_index_ >= 0 && ask_index_ < ORDER_MANAGER_INT_PRICE_RANGE) ? sum_ask_unconfirmed_[ask_index_] : 0;
  }

  inline int intpx_2_sum_ask_unconfirmed_orders(const int _intpx_) const {
    int ask_index_ = GetAskIndex(_intpx_);
    return (ask_index_ >= 0 && ask_index_ < ORDER_MANAGER_INT_PRICE_RANGE) ? sum_ask_unconfirmed_orders_[ask_index_]
                                                                           : 0;
  }

  int SumBidSizeUnconfirmedEqAboveIntPrice(const int _intpx_) const {
    int retval = 0;
    int bid_index_ = GetBidIndex(_intpx_);

    if (unconfirmed_top_bid_index_ != -1) {
      if (bid_index_ > unconfirmed_top_bid_index_) {
        return 0;
      }
      if (bid_index_ < unconfirmed_bottom_bid_index_) {
        return sum_total_bid_unconfirmed_[unconfirmed_top_bid_index_];
      }
      return sum_total_bid_unconfirmed_[unconfirmed_top_bid_index_] - sum_total_bid_unconfirmed_[bid_index_] +
             sum_bid_unconfirmed_[bid_index_];
    }

    return retval;
  }

  int SumBidSizeUnconfirmedAboveIntPrice(const int intpx) const {
    int retval = 0;
    int bid_index_ = GetBidIndex(intpx);

    if (unconfirmed_top_bid_index_ != -1) {
      if (bid_index_ > unconfirmed_top_bid_index_) {
        return 0;
      }
      if (bid_index_ < unconfirmed_bottom_bid_index_) {
        return sum_total_bid_unconfirmed_[unconfirmed_top_bid_index_];
      }
      return sum_total_bid_unconfirmed_[unconfirmed_top_bid_index_] - sum_total_bid_unconfirmed_[bid_index_];
    }

    return retval;
  }

  int SumBidSizeConfirmedEqAboveIntPrice(const int _intpx_) const {
    int retval = 0;
    int bid_index_ = GetBidIndex(_intpx_);

    if (confirmed_top_bid_index_ != -1) {
      if (bid_index_ > confirmed_top_bid_index_) {
        return 0;
      }
      if (bid_index_ < confirmed_bottom_bid_index_) {
        return sum_total_bid_confirmed_[confirmed_top_bid_index_];
      }
      return sum_total_bid_confirmed_[confirmed_top_bid_index_] - sum_total_bid_confirmed_[bid_index_] +
             sum_bid_confirmed_[bid_index_];
    }

    return retval;
  }

  int SumBidSizeConfirmedAboveIntPrice(const int _intpx_) const {
    int retval = 0;
    int bid_index_ = GetBidIndex(_intpx_);

    if (confirmed_top_bid_index_ != -1) {
      if (bid_index_ > confirmed_top_bid_index_) {
        // if (retval != 0) std::cout << "Anakin" << "\n";
        return 0;
      }

      if (bid_index_ < confirmed_bottom_bid_index_) {
        // if (retval != sum_total_bid_confirmed_[confirmed_top_bid_index_]) std::cout << "Anakin" << "\n";
        return sum_total_bid_confirmed_[confirmed_top_bid_index_];
      }
      return sum_total_bid_confirmed_[confirmed_top_bid_index_] - sum_total_bid_confirmed_[bid_index_];
    }
    return retval;
  }

  /**
   * Sum of our total size in market
   * @return
   */
  inline int SumBidSizes() const {
    int retval = 0;
    // Sum the total confirmed-unconfirmed sizes
    retval += SumBidUnConfirmedSizes();
    retval += SumBidConfirmedSizes();
    return retval;
  }

  /**
   *
   * @return Sum of unconfirmed bid-sizes
   */
  inline int SumBidUnConfirmedSizes() const {
    int retval = 0;
    if (unconfirmed_bottom_bid_index_ != -1) {
      return sum_total_bid_unconfirmed_[unconfirmed_top_bid_index_];
    }

    return retval;
  }

  /**
   * Returns the sum of confirmed sizes;
   * @return
   */
  inline int SumBidConfirmedSizes() const {
    int retval = 0;
    if (confirmed_bottom_bid_index_ != -1) {
      return sum_total_bid_confirmed_[confirmed_top_bid_index_];
    }
    return retval;
  }
  int SumBidSizeCancelRequested(const int _intpx_) {
    int retval_ = 0;
    int bid_index_ = GetBidIndex(_intpx_);
    if (order_vec_top_bid_index_ != -1) {
      for (int index_ = order_vec_top_bid_index_; index_ >= std::max(bid_index_, order_vec_bottom_bid_index_);
           index_--) {
        if ((p_ticks_to_keep_bid_int_price_) && (*p_ticks_to_keep_bid_int_price_) > 0 &&
            GetBidIntPrice(index_) == (*p_ticks_to_keep_bid_int_price_)) {  // Do not cancel an order
                                                                            // at this bid-int-price
          continue;
        }
        std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];
        if (!_this_base_order_vec_.empty()) {
          for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
            if (_this_base_order_vec_[i]->canceled_) {
              retval_ += _this_base_order_vec_[i]->size_remaining();
            }
          }
        }
      }
    }
    return retval_;
  }

  inline int SumBidSizeCancelRequestedInRange(const int _lower_int_px_, const int _upper_int_px_) {
    // lowe-uppper is in terms of levels
    int retval_ = 0;
    int top_bid_index_ = GetBidIndex(_lower_int_px_);
    int bottom_bid_index_ = GetBidIndex(_upper_int_px_);

    int top_bid_index_to_cancel_ = std::min(order_vec_top_bid_index_, top_bid_index_);
    int bottom_bid_index_to_cancel_ = std::max(bottom_bid_index_, order_vec_bottom_bid_index_);

    if (order_vec_top_bid_index_ != -1) {
      for (int index_ = bottom_bid_index_to_cancel_; index_ <= top_bid_index_to_cancel_; index_++) {
        if ((p_ticks_to_keep_bid_int_price_) && (*p_ticks_to_keep_bid_int_price_) > 0 &&
            GetAskIntPrice(index_) == (*p_ticks_to_keep_bid_int_price_)) {
          continue;
        }
        std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];
        if (!_this_base_order_vec_.empty()) {
          for (int i = _this_base_order_vec_.size() - 1; i >= 0; i--) {
            if (_this_base_order_vec_[i]->canceled_)

            {
              retval_ += _this_base_order_vec_[i]->size_remaining();
            }
          }
        }
      }
    }
    return retval_;
  }

  int SumAskSizeUnconfirmedEqAboveIntPrice(const int _intpx_) const {
    int retval = 0;
    int ask_index_ = GetAskIndex(_intpx_);

    if (unconfirmed_top_ask_index_ != -1) {
      if (ask_index_ > unconfirmed_top_ask_index_) {
        return 0;
      }
      if (ask_index_ < unconfirmed_bottom_ask_index_) {
        return sum_total_ask_unconfirmed_[unconfirmed_top_ask_index_];
      }
      return sum_total_ask_unconfirmed_[unconfirmed_top_ask_index_] - sum_total_ask_unconfirmed_[ask_index_] +
             sum_ask_unconfirmed_[ask_index_];
    }

    return retval;
  }

  int SumAskSizeUnconfirmedAboveIntPrice(const int intpx) const {
    int retval = 0;
    int ask_index_ = GetAskIndex(intpx);

    if (unconfirmed_top_ask_index_ != -1) {
      if (ask_index_ > unconfirmed_top_ask_index_) {
        return 0;
      }
      if (ask_index_ < unconfirmed_bottom_ask_index_) {
        return sum_total_ask_unconfirmed_[unconfirmed_top_ask_index_];
      }
      return sum_total_ask_unconfirmed_[unconfirmed_top_ask_index_] - sum_total_ask_unconfirmed_[ask_index_];
    }

    return retval;
  }

  int SumAskSizeConfirmedEqAboveIntPrice(const int _intpx_) const {
    int retval = 0;
    int ask_index_ = GetAskIndex(_intpx_);

    if (confirmed_top_ask_index_ != -1) {
      if (ask_index_ > confirmed_top_ask_index_) {
        return 0;
      }
      if (ask_index_ < confirmed_bottom_ask_index_) {
        return sum_total_ask_confirmed_[confirmed_top_ask_index_];
      }
      return sum_total_ask_confirmed_[confirmed_top_ask_index_] - sum_total_ask_confirmed_[ask_index_] +
             sum_ask_confirmed_[ask_index_];
    }

    return retval;
  }

  int SumAskSizeConfirmedAboveIntPrice(const int _intpx_) const {
    int retval = 0;
    int ask_index_ = GetAskIndex(_intpx_);

    if (confirmed_top_ask_index_ != -1) {
      if (ask_index_ > confirmed_top_ask_index_) {
        return 0;
      }
      if (ask_index_ < confirmed_bottom_ask_index_) {
        return sum_total_ask_confirmed_[confirmed_top_ask_index_];
      }
      return sum_total_ask_confirmed_[confirmed_top_ask_index_] - sum_total_ask_confirmed_[ask_index_];
    }

    return retval;
  }

  // inline int SumAskSizes ( ) const { return sum_ask_sizes_ ; }
  inline int SumAskSizes() const {
    int retval = 0;

    // Return the sum of total confirmend and unconfirmed ask sizes
    retval += SumAskUnConfirmedSizes();
    retval += SumAskConfirmedSizes();
    return retval;
  }

  // Return the total UnConfirmed Ask size
  inline int SumAskUnConfirmedSizes() const {
    int retval = 0;

    if (unconfirmed_top_ask_index_ != -1) {
      return sum_total_ask_unconfirmed_[unconfirmed_top_ask_index_];
    }
    return retval;
  }

  // Return the confirmed ask size;
  inline int SumAskConfirmedSizes() const {
    int retval = 0;
    if (confirmed_top_ask_index_ != -1) {
      return sum_total_ask_confirmed_[confirmed_top_ask_index_];
    }
    return retval;
  }

  int SumAskSizeCancelRequested(const int _intpx_) {
    int retval_ = 0;
    int ask_index_ = GetAskIndex(_intpx_);

    if (order_vec_top_ask_index_ != -1) {
      for (int index_ = order_vec_top_ask_index_; index_ >= std::max(ask_index_, order_vec_bottom_ask_index_);
           index_--) {
        if ((p_ticks_to_keep_ask_int_price_) && (*p_ticks_to_keep_ask_int_price_) > 0 &&
            GetAskIntPrice(index_) == (*p_ticks_to_keep_ask_int_price_)) {  // Do not cancel an order
                                                                            // at this ask-int-price
          continue;
        }
        std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[index_];
        if (!_this_base_order_vec_.empty()) {
          for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
            if (_this_base_order_vec_[i]->canceled_) {
              retval_ += _this_base_order_vec_[i]->size_remaining();
            }
          }
        }
      }
    }
    return retval_;
  }

  inline int SumAskSizeCancelRequestedInRange(const int _lower_int_px_, const int _upper_int_px_) {
    int retval_ = 0;
    int top_ask_index_ = GetAskIndex(_lower_int_px_);
    int bottom_ask_index_ = GetAskIndex(_upper_int_px_);

    int top_ask_index_to_cancel_ = std::min(order_vec_top_ask_index_, top_ask_index_);
    int bottom_ask_index_to_cancel_ = std::max(bottom_ask_index_, order_vec_bottom_ask_index_);

    if (order_vec_top_ask_index_ != -1) {
      for (int index_ = bottom_ask_index_to_cancel_; index_ <= top_ask_index_to_cancel_; index_++) {
        if ((p_ticks_to_keep_ask_int_price_) && (*p_ticks_to_keep_ask_int_price_) > 0 &&
            GetAskIntPrice(index_) == (*p_ticks_to_keep_ask_int_price_)) {
          continue;
        }
        std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[index_];
        if (!_this_base_order_vec_.empty()) {
          for (int i = _this_base_order_vec_.size() - 1; i >= 0; i--) {
            if (_this_base_order_vec_[i]->canceled_) {
              retval_ += _this_base_order_vec_[i]->size_remaining();
            }
          }
        }
      }
    }
    return retval_;
  }

  BaseOrder* GetBidOrderAtIndex(int index) {
    return (index < 0 || index >= ORDER_MANAGER_INT_PRICE_RANGE || bid_order_vec_[index].empty())
               ? NULL
               : bid_order_vec_[index][0];
  }

  BaseOrder* GetAskOrderAtIndex(int index) {
    return (index < 0 || index >= ORDER_MANAGER_INT_PRICE_RANGE || ask_order_vec_[index].empty())
               ? NULL
               : ask_order_vec_[index][0];
  }

  inline void SetUseModifyFlag(bool to_set) { use_modify_logic_ = to_set; }

  /// Get modifiable orders
  inline BaseOrder* GetTopModifiableBidOrder() { return GetBidOrderAtIndex(top_modifiable_bid_index_); }

  inline BaseOrder* GetBottomModifiableBidOrder() { return GetBidOrderAtIndex(bottom_modifiable_bid_index_); }

  inline BaseOrder* GetTopModifiableAskOrder() { return GetAskOrderAtIndex(top_modifiable_ask_index_); }

  inline BaseOrder* GetBottomModifiableAskOrder() { return GetAskOrderAtIndex(bottom_modifiable_ask_index_); }

  /// Get modifiable bid indices
  inline int GetTopModifiableBidIndex() { return top_modifiable_bid_index_; }

  inline int GetBottomModifiableBidIndex() { return bottom_modifiable_bid_index_; }

  inline int GetTopModifiableAskIndex() { return top_modifiable_ask_index_; }

  inline int GetBottomModifiableAskIndex() { return bottom_modifiable_ask_index_; }

  inline int GetNonBestEmptyBidIntPrice() {
    int bottom_index = 0;
    if (order_vec_bottom_bid_index_ >= 0) bottom_index = order_vec_bottom_bid_index_;
    if (unconfirmed_bottom_bid_index_ >= 0) bottom_index = std::min(bottom_index, unconfirmed_bottom_bid_index_);
    int price = GetBidIntPrice(bottom_index);
    while (GetTotalBidSizeOrderedAtIntPx(price) != 0) {
      bottom_index--;
      price = GetBidIntPrice(bottom_index);
    }

    return price;
  }

  inline int GetNonBestEmptyAskIntPrice() {
    int bottom_index = 0;
    if (order_vec_bottom_ask_index_ >= 0) bottom_index = order_vec_bottom_ask_index_;
    if (unconfirmed_bottom_ask_index_ >= 0) bottom_index = std::min(bottom_index, unconfirmed_bottom_ask_index_);
    int price = GetAskIntPrice(bottom_index);
    while (GetTotalAskSizeOrderedAtIntPx(price) != 0) {
      bottom_index--;
      price = GetAskIntPrice(bottom_index);
    }

    return price;
  }

  /// Get topmost ( oldest ) bid order at the given intpx
  inline BaseOrder* GetTopBidOrderAtIntPx(int _intpx_) {
    int bid_index_ = GetBidIndex(_intpx_);
    return (bid_index_ < 0 || bid_index_ >= ORDER_MANAGER_INT_PRICE_RANGE || bid_order_vec_[bid_index_].empty())
               ? NULL
               : bid_order_vec_[bid_index_][0];
  }

  inline BaseOrder* GetBottomBidOrderAtIntPx(int _intpx_) {
    int bid_index_ = GetBidIndex(_intpx_);
    return (bid_index_ < 0 || bid_index_ >= ORDER_MANAGER_INT_PRICE_RANGE || bid_order_vec_[bid_index_].empty())
               ? NULL
               : bid_order_vec_[bid_index_][bid_order_vec_[bid_index_].size() - 1];
  }

  BaseOrder* GetTopBidOrder();           ///< Get topmost ( oldest ) bid order at any price
  BaseOrder* GetTopConfirmedBidOrder();  ///< Get topmost confirmed bid order at any price

  std::vector<BaseOrder*>& GetUnSequencedBids() { return unsequenced_bids_; }
  std::vector<BaseOrder*>& GetUnSequencedAsks() { return unsequenced_asks_; }

  /// Get topmost ( oldest ) ask order at the given intpx
  inline BaseOrder* GetTopAskOrderAtIntPx(int _intpx_) {
    int ask_index_ = GetAskIndex(_intpx_);
    return (ask_index_ < 0 || ask_index_ >= ORDER_MANAGER_INT_PRICE_RANGE || ask_order_vec_[ask_index_].empty())
               ? NULL
               : ask_order_vec_[ask_index_][0];
  }

  inline BaseOrder* GetBottomAskOrderAtIntPx(int _intpx_) {
    int ask_index_ = GetAskIndex(_intpx_);
    return (ask_index_ < 0 || ask_index_ >= ORDER_MANAGER_INT_PRICE_RANGE || ask_order_vec_[ask_index_].empty())
               ? NULL
               : ask_order_vec_[ask_index_][ask_order_vec_[ask_index_].size() - 1];
  }

  BaseOrder* GetTopAskOrder();           ///< Get topmost ( oldest ) ask order at any price
  BaseOrder* GetTopConfirmedAskOrder();  ///< Get topmost confirmed ask Order at any price

  /* OrderRouting functions, to be called from Strategy class */
  inline void SendTradeDblPx(double price, int size_requested, TradeType_t buysell,
                             char placed_at_level_indicator = 'B', bool is_fok_ = false) {
    OrderType_t order_type = is_fok_ ? kOrderFOK : kOrderDay;
    SendTrade(price, GetIntPx(price), size_requested, buysell, placed_at_level_indicator, order_type);
  }
  inline void SendTradeIntPx(int intpx, int size_requested, TradeType_t buysell, char placed_at_level_indicator = 'B',
                             bool is_fok = false) {
    OrderType_t order_type = is_fok ? kOrderFOK : kOrderDay;
    SendTrade(fast_price_convertor_.GetDoublePx(intpx), intpx, size_requested, buysell, placed_at_level_indicator,
              order_type);
  }
  void SendTrade(const double _price_, const int _intpx_, int _size_requested_, TradeType_t _buysell_,
                 char placed_at_level_indicator_ = 'B', bool is_fok_ = false) {
    OrderType_t order_type = is_fok_ ? kOrderFOK : kOrderDay;
    SendTrade(_price_, _intpx_, _size_requested_, _buysell_, placed_at_level_indicator_, order_type);
  }
  inline void SendTradeIntPx(int intpx, int size_requested, TradeType_t buysell, char placed_at_level_indicator,
                             OrderType_t order_type) {
    SendTrade(fast_price_convertor_.GetDoublePx(intpx), intpx, size_requested, buysell, placed_at_level_indicator,
              order_type);
  }
  BaseOrder* SendTrade(const double price, const int intpx, int size_requested, TradeType_t buysell,
                       char placed_at_level_indicator, OrderType_t order_type);
  void NotifyWorstPosToOrs(int max_position_);
  bool IOCOrderExists();

  /// Return value indicates whether the order was canceled or not
  inline bool Cancel(BaseOrder& t_this_order_) {
    if (place_cancel_cooloff_ > 0) {
      if (watch_.msecs_from_midnight() - t_this_order_.placed_msecs() < place_cancel_cooloff_) {
        return false;
      }
    }

    cancel_order_ = t_this_order_;

    if (cancel_order_.CanBeCanceled(watch_.msecs_from_midnight() - CXL_WAIT_MSECS,
                                    is_cancellable_before_confirmation)) {
      cancel_order_.canceled_ = true;
      cancel_order_.seqd_msecs_ = watch_.msecs_from_midnight();
      cancel_order_.seqd_usecs_ = watch_.tv().tv_usec;
      cancel_order_.cancel_seqd_time_ = watch_.tv();

      // If it is first send message or
      // We have received any message since last send then update the last_sent_message time
      if (last_sent_message_mfm_ == 0 || received_after_last_sent_) {
        last_sent_message_mfm_ = watch_.msecs_from_midnight();
        received_after_last_sent_ = false;
      }

      base_trader_.Cancel(cancel_order_);
      t_this_order_ = cancel_order_;

      // make sure that the throttle manager is used only for exchanges where we
      // need it
      if (throttle_manager_) {
        throttle_manager_->update_throttle_manager(watch_.msecs_from_midnight());
      }

      // For CxlOrderCount ( )
      cxl_order_count_++;

      if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "CanBeCanceled - so sending_cancel of CAOS: "
                               << cancel_order_.client_assigned_order_sequence_
                               << " bs: " << ((cancel_order_.buysell_ == kTradeTypeBuy) ? "BUY" : "SELL")
                               << " sz: " << cancel_order_.size_requested_ << " px: " << cancel_order_.price_
                               << " intpx: " << cancel_order_.int_price_ << DBGLOG_ENDL_FLUSH;
        if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO))  // zero logging
        {
          dbglogger_ << "SYM: " << cancel_order_.security_name_ << " Px: " << cancel_order_.price()
                     << " INTPX: " << cancel_order_.int_price() << " BS: " << GetTradeTypeChar(cancel_order_.buysell())
                     << " ST: " << watch_.tv() << " DT: "
                     << watch_.tv() + ttime_t(0, cancel_order_.seqd_msecs_ * 1000 + cancel_order_.seqd_usecs_ % 1000) -
                            ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                     << " ORR: " << ToString(kORRType_CxlSeqd)
                     << " SAOS: " << cancel_order_.server_assigned_order_sequence()
                     << " CAOS: " << cancel_order_.client_assigned_order_sequence() << " CLTPOS: " << client_position_
                     << " SACI: " << server_assigned_client_id_ << " GBLPOS: " << global_position_
                     << " SIZE: " << cancel_order_.size_remaining() << " SE: " << cancel_order_.size_executed()
                     << " MsgSeq : " << 0 << " Seq: " << 0 << DBGLOG_ENDL_FLUSH;
        }
      }
      return true;
    }
    return false;
  }

  bool ModifyOrderAndLog(BaseOrder* t_this_order_, double _new_price_, int _new_int_price_, int _new_size_);
  bool Modify(BaseOrder* t_this_order_, double _new_price_, int _new_int_price_, int _new_size_);

  int KeepBidSizeInPriceRange(int _keep_size_, int _bottom_px_ = kInvalidIntPrice, int _top_px_ = kInvalidIntPrice);
  int KeepAskSizeInPriceRange(int _keep_size_, int _bottom_px_ = kInvalidIntPrice, int _top_px_ = kInvalidIntPrice);

  void CancelAllBidOrders();
  void CancelAllAskOrders();
  void CancelAllOrders();
  inline void UnsetCancelAllOrders() { external_cancel_all_outstanding_orders_ = false; }

  int CancelReplaceBidOrdersEqAboveAndEqBelowIntPrice(const int _cxl_upper_int_price_, const int _cxl_lower_int_price_,
                                                      const std::vector<double>& _prices_to_place_at_vec_,
                                                      const std::vector<int>& _int_prices_to_place_at_vec_,
                                                      const std::vector<int>& _sizes_to_place_vec_,
                                                      const std::vector<char>& _order_level_indicator_vec_,
                                                      const OrderType_t _order_type_);

  int CancelReplaceAskOrdersEqAboveAndEqBelowIntPrice(const int _cxl_upper_int_price_, const int _cxl_lower_int_price_,
                                                      const std::vector<double>& _prices_to_place_at_vec_,
                                                      const std::vector<int>& _int_prices_to_place_at_vec_,
                                                      const std::vector<int>& _sizes_to_place_vec_,
                                                      const std::vector<char>& _order_level_indicator_vec_,
                                                      const OrderType_t _order_type_);

  /// Currently a lot of replication, we should break this in small functions

  int CancelReplaceBidOrdersEqAboveAndSizeFromFar(const int _cxl_upper_int_price_, const int size_to_cancel_from_far,
                                                  const std::vector<double>& _prices_to_place_at_vec_,
                                                  const std::vector<int>& _int_prices_to_place_at_vec_,
                                                  const std::vector<int>& _sizes_to_place_vec_,
                                                  const std::vector<char>& _order_level_indicator_vec_,
                                                  const OrderType_t _order_type_);

  int CancelReplaceAskOrdersEqAboveAndSizeFromFar(const int _cxl_upper_int_price_, const int size_to_cancel_from_far,
                                                  const std::vector<double>& _prices_to_place_at_vec_,
                                                  const std::vector<int>& _int_prices_to_place_at_vec_,
                                                  const std::vector<int>& _sizes_to_place_vec_,
                                                  const std::vector<char>& _order_level_indicator_vec_,
                                                  const OrderType_t _order_type_);

  inline int CancelBidsInRange(const int _lower_int_px_, const int _upper_int_px_, const int _requested_size_) {
    // lowe-uppper is in terms of levels
    int retval_ = 0;
    int top_bid_index_ = GetBidIndex(_lower_int_px_);
    int bottom_bid_index_ = GetBidIndex(_upper_int_px_);

    int top_bid_index_to_cancel_ = std::min(order_vec_top_bid_index_, top_bid_index_);
    int bottom_bid_index_to_cancel_ = std::max(bottom_bid_index_, order_vec_bottom_bid_index_);

    if (order_vec_top_bid_index_ != -1) {
      for (int index_ = bottom_bid_index_to_cancel_; index_ <= top_bid_index_to_cancel_; index_++) {
        if ((p_ticks_to_keep_bid_int_price_) && (*p_ticks_to_keep_bid_int_price_) > 0 &&
            GetAskIntPrice(index_) == (*p_ticks_to_keep_bid_int_price_)) {
          continue;
        }
        std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];
        if (!_this_base_order_vec_.empty()) {
          for (int i = _this_base_order_vec_.size() - 1; i >= 0; i--) {
            if (retval_ + _this_base_order_vec_[i]->size_remaining() > _requested_size_) {
              if (retval_ > 0) {
                return retval_;
              } else {
                continue;
              }  // look into next order/level if we cant find size,
            }

            if (Cancel(*(_this_base_order_vec_[i]))) {
              retval_ += _this_base_order_vec_[i]->size_remaining();
            }
          }
        }
      }
    }
    return retval_;
  }

  inline int CancelBidsEqAboveIntPrice(const int _intpx_, const int _size_requested_) {
    int retval = 0;
    int bid_index_ = GetBidIndex(_intpx_);

    if (order_vec_top_bid_index_ != -1) {
      for (int index_ = order_vec_top_bid_index_; index_ >= std::max(bid_index_, order_vec_bottom_bid_index_);
           index_--) {
        if ((p_ticks_to_keep_bid_int_price_) && (*p_ticks_to_keep_bid_int_price_) > 0 &&
            GetBidIntPrice(index_) == (*p_ticks_to_keep_bid_int_price_)) {  // Do not cancel an order
                                                                            // at this bid-int-price
          continue;
        }

        std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];
        if (!_this_base_order_vec_.empty()) {
          for (int i = _this_base_order_vec_.size() - 1; i >= 0; i--) {
            if (_this_base_order_vec_[i]->size_remaining() + retval > _size_requested_) {
              return retval;
            }
            if (Cancel(*(_this_base_order_vec_[i]))) {
              if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
                DBGLOG_TIME_CLASS_FUNC << "givenintpx: " << _intpx_
                                       << " CAOS: " << _this_base_order_vec_[i]->client_assigned_order_sequence_
                                       << " bs: "
                                       << ((_this_base_order_vec_[i]->buysell_ == kTradeTypeBuy) ? "BUY" : "SELL")
                                       << " sz: " << _this_base_order_vec_[i]->size_requested_
                                       << " px: " << _this_base_order_vec_[i]->price_
                                       << " intpx: " << _this_base_order_vec_[i]->int_price_ << DBGLOG_ENDL_FLUSH;
              }

              retval += _this_base_order_vec_[i]->size_remaining();
            }
          }
        }
      }
    }

    return retval;
  }

  /**
   * Cancels the orders to reach given size_to_be_reduced.
   * In case it's not able to cancel but size_to_cancel > 0 ( when OrderSize is very high), It tries to modify
   *
   * @param intpx
   * @param size_requested
   * @param min_change_size_to_modify
   * @return
   */
  inline int CancelOrModifyBidOrdersAboveIntPrice(const int intpx, const int size_requested,
                                                  const int min_size_change_to_modify) {
    int retval = 0;
    int bid_index_ = GetBidIndex(intpx);

    if (order_vec_top_bid_index_ != -1) {
      for (int index_ = order_vec_top_bid_index_; index_ >= std::max(bid_index_, order_vec_bottom_bid_index_);
           index_--) {
        if ((p_ticks_to_keep_bid_int_price_) && (*p_ticks_to_keep_bid_int_price_) > 0 &&
            GetBidIntPrice(index_) == (*p_ticks_to_keep_bid_int_price_)) {  // Do not cancel an order
                                                                            // at this bid-int-price
          continue;
        }

        std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];
        if (!_this_base_order_vec_.empty()) {
          for (int i = _this_base_order_vec_.size() - 1; i >= 0; i--) {
            if (_this_base_order_vec_[i]->size_remaining() + retval > size_requested) {
              // The difference of size it could not cancel is more than minimum size to be modified,
              // then modify the order
              if (size_requested - retval > min_size_change_to_modify) {
                auto order = _this_base_order_vec_[i];
                auto new_size = order->size_remaining() - (size_requested - retval);

                // Only if we are able to modify this order, we return the retval
                // If we are not able to modify this order the proceed to next order if its there
                if (ModifyOrderAndLog(order, order->price(), order->int_price(), new_size)) {
                  retval += (size_requested - retval);
                  return retval;
                }
              } else {
                return retval;
              }
            } else {
              // If the order is small enough to cancel then cancel it
              if (Cancel(*(_this_base_order_vec_[i]))) {
                if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
                  DBGLOG_TIME_CLASS_FUNC << "givenintpx: " << intpx
                                         << " CAOS: " << _this_base_order_vec_[i]->client_assigned_order_sequence_
                                         << " bs: "
                                         << ((_this_base_order_vec_[i]->buysell_ == kTradeTypeBuy) ? "BUY" : "SELL")
                                         << " sz: " << _this_base_order_vec_[i]->size_requested_
                                         << " px: " << _this_base_order_vec_[i]->price_
                                         << " intpx: " << _this_base_order_vec_[i]->int_price_ << DBGLOG_ENDL_FLUSH;
                }

                retval += _this_base_order_vec_[i]->size_remaining();
              }
            }
          }
        }
      }
    }

    return retval;
  }

  inline int CancelBidsEqAboveIntPrice(const int _intpx_) {
    int retval = 0;
    int bid_index_ = GetBidIndex(_intpx_);

    if (order_vec_top_bid_index_ != -1) {
      for (int index_ = order_vec_top_bid_index_; index_ >= std::max(bid_index_, order_vec_bottom_bid_index_);
           index_--) {
        if ((p_ticks_to_keep_bid_int_price_) && (*p_ticks_to_keep_bid_int_price_) > 0 &&
            GetBidIntPrice(index_) == (*p_ticks_to_keep_bid_int_price_)) {  // Do not cancel an order
                                                                            // at this bid-int-price
          continue;
        }

        std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];
        if (!_this_base_order_vec_.empty()) {
          for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
            if (Cancel(*(_this_base_order_vec_[i]))) {
              if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
                DBGLOG_TIME_CLASS_FUNC << "givenintpx: " << _intpx_
                                       << " CAOS: " << _this_base_order_vec_[i]->client_assigned_order_sequence_
                                       << " bs: "
                                       << ((_this_base_order_vec_[i]->buysell_ == kTradeTypeBuy) ? "BUY" : "SELL")
                                       << " sz: " << _this_base_order_vec_[i]->size_requested_
                                       << " px: " << _this_base_order_vec_[i]->price_
                                       << " intpx: " << _this_base_order_vec_[i]->int_price_ << DBGLOG_ENDL_FLUSH;
              }

              retval += _this_base_order_vec_[i]->size_remaining();
            }
          }
        }
      }
    }

    return retval;
  }

  inline int CancelBidsAboveIntPrice(const int _intpx_) {
    int retval = 0;
    int bid_index_ = GetBidIndex(_intpx_);

    if (order_vec_top_bid_index_ != -1) {
      for (int index_ = order_vec_top_bid_index_; index_ > std::max(bid_index_, order_vec_bottom_bid_index_ - 1);
           index_--) {
        if ((p_ticks_to_keep_bid_int_price_) && (*p_ticks_to_keep_bid_int_price_) > 0 &&
            GetBidIntPrice(index_) == (*p_ticks_to_keep_bid_int_price_)) {  // Do not cancel an order
                                                                            // at this bid-int-price
          continue;
        }

        std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];
        if (!_this_base_order_vec_.empty()) {
          for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
            if (Cancel(*(_this_base_order_vec_[i]))) {
              if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
                DBGLOG_TIME_CLASS_FUNC << "givenintpx: " << _intpx_
                                       << " CAOS: " << _this_base_order_vec_[i]->client_assigned_order_sequence_
                                       << " bs: "
                                       << ((_this_base_order_vec_[i]->buysell_ == kTradeTypeBuy) ? "BUY" : "SELL")
                                       << " sz: " << _this_base_order_vec_[i]->size_requested_
                                       << " px: " << _this_base_order_vec_[i]->price_
                                       << " intpx: " << _this_base_order_vec_[i]->int_price_ << DBGLOG_ENDL_FLUSH;
              }

              retval += _this_base_order_vec_[i]->size_remaining();
            }
          }
        }
      }
    }

    return retval;
  }

  inline int CancelBidsAtIntPrice(const int _intpx_) {
    if ((p_ticks_to_keep_bid_int_price_) && (*p_ticks_to_keep_bid_int_price_) > 0 &&
        _intpx_ == (*p_ticks_to_keep_bid_int_price_)) {  // Do not cancel an
                                                         // order at this
                                                         // bid-int-price
      return 0;
    }

    int retval = 0;
    int bid_index_ = GetBidIndex(_intpx_);

    if (bid_index_ >= 0 && bid_index_ < ORDER_MANAGER_INT_PRICE_RANGE && order_vec_top_bid_index_ != -1) {
      std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[bid_index_];
      if (!_this_base_order_vec_.empty()) {
        for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
          if (Cancel(*(_this_base_order_vec_[i]))) {
            retval += _this_base_order_vec_[i]->size_remaining();
          }
        }
      }
    }

    return retval;
  }

  inline int CancelBidsFromFar(const int _required_size_, int _max_bid_int_px_to_cancel_ = kInvalidIntPrice) {
    int retval = 0;

    if (order_vec_bottom_bid_index_ != -1) {
      for (int index_ = order_vec_bottom_bid_index_; index_ <= order_vec_top_bid_index_; index_++) {
        if ((p_ticks_to_keep_bid_int_price_) && (*p_ticks_to_keep_bid_int_price_) > 0 &&
            GetBidIntPrice(index_) == (*p_ticks_to_keep_bid_int_price_)) {  // Do not cancel an order
                                                                            // at this bid-int-price
          continue;
        }

        if (_max_bid_int_px_to_cancel_ != kInvalidIntPrice && GetBidIntPrice(index_) > _max_bid_int_px_to_cancel_) {
          break;
        }

        std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];
        if (!_this_base_order_vec_.empty()) {
          for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
            if (Cancel(*(_this_base_order_vec_[i]))) {
              retval += _this_base_order_vec_[i]->size_remaining();
            }
          }
        }
        if (retval >= _required_size_) {
          break;
        }
      }
    }

    return retval;
  }

  inline int CancelBidsBelowIntPrice(const int _intpx_) {
    int retval = 0;

    if (order_vec_top_bid_index_ != -1) {
      for (int index_ = order_vec_top_bid_index_; index_ >= order_vec_bottom_bid_index_; index_--) {
        if ((p_ticks_to_keep_bid_int_price_) && (*p_ticks_to_keep_bid_int_price_) > 0 &&
            GetBidIntPrice(index_) == (*p_ticks_to_keep_bid_int_price_)) {  // Do not cancel an order
                                                                            // at this bid-int-price
          continue;
        }

        if (GetBidIntPrice(index_) < _intpx_)  // TODO : can be replaced with index_ comparison
        {
          std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];
          if (!_this_base_order_vec_.empty()) {
            retval += CancelOrdersInVec(_this_base_order_vec_);
          }
        }
      }
    }

    return retval;
  }

  inline int CancelBidsEqBelowIntPrice(const int _intpx_) {
    int retval = 0;

    if (order_vec_top_bid_index_ != -1) {
      for (int index_ = order_vec_top_bid_index_; index_ >= order_vec_bottom_bid_index_; index_--) {
        if ((p_ticks_to_keep_bid_int_price_) && (*p_ticks_to_keep_bid_int_price_) > 0 &&
            GetBidIntPrice(index_) == (*p_ticks_to_keep_bid_int_price_)) {  // Do not cancel an order
                                                                            // at this bid-int-price
          continue;
        }

        if (GetBidIntPrice(index_) <= _intpx_)  // TODO : calculate index for
                                                // _intpx_ and compare with
                                                // index_
        {
          std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];
          if (!_this_base_order_vec_.empty()) {
            retval += CancelOrdersInVec(_this_base_order_vec_);
          }
        }
      }
    }

    return retval;
  }

  inline int CancelOrdersInVec(const std::vector<BaseOrder*>& _this_base_order_vec_) {
    int retval = 0;
    for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
      if (Cancel(*(_this_base_order_vec_[i]))) {
        retval += _this_base_order_vec_[i]->size_remaining();
      }
    }
    return retval;
  }

  inline int CancelAsksInRange(const int _lower_int_px_, const int _upper_int_px_, const int _requested_size_) {
    int retval_ = 0;

    int top_ask_index_ = GetAskIndex(_lower_int_px_);
    int bottom_ask_index_ = GetAskIndex(_upper_int_px_);

    int top_ask_index_to_cancel_ = std::min(order_vec_top_ask_index_, top_ask_index_);
    int bottom_ask_index_to_cancel_ = std::max(bottom_ask_index_, order_vec_bottom_ask_index_);

    if (order_vec_top_ask_index_ != -1) {
      for (int index_ = bottom_ask_index_to_cancel_; index_ <= top_ask_index_to_cancel_; index_++) {
        if ((p_ticks_to_keep_ask_int_price_) && (*p_ticks_to_keep_ask_int_price_) > 0 &&
            GetAskIntPrice(index_) == (*p_ticks_to_keep_ask_int_price_)) {
          continue;
        }
        std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[index_];
        if (!_this_base_order_vec_.empty()) {
          for (int i = _this_base_order_vec_.size() - 1; i >= 0; i--) {
            if (retval_ + _this_base_order_vec_[i]->size_remaining() > _requested_size_) {
              if (retval_ > 0) {
                return retval_;
              } else {
                continue;
              }  // look into next order/level if we cant find size,
            }

            if (Cancel(*(_this_base_order_vec_[i]))) {
              retval_ += _this_base_order_vec_[i]->size_remaining();
            }
          }
        }
      }
    }
    return retval_;
  }

  inline int CancelAsksEqAboveIntPrice(const int _intpx_, const int requested_size_) {
    int retval = 0;
    int ask_index_ = GetAskIndex(_intpx_);

    if (order_vec_top_ask_index_ != -1) {
      for (int index_ = order_vec_top_ask_index_; index_ >= std::max(ask_index_, order_vec_bottom_ask_index_);
           index_--) {
        if ((p_ticks_to_keep_ask_int_price_) && (*p_ticks_to_keep_ask_int_price_) > 0 &&
            GetAskIntPrice(index_) == (*p_ticks_to_keep_ask_int_price_)) {  // Do not cancel an order
                                                                            // at this ask-int-price
          continue;
        }

        std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[index_];
        if (!_this_base_order_vec_.empty()) {
          for (int i = _this_base_order_vec_.size() - 1; i >= 0; i--) {
            if (retval + _this_base_order_vec_[i]->size_remaining() > requested_size_) {
              return retval;
            }

            if (Cancel(*(_this_base_order_vec_[i]))) {
              retval += _this_base_order_vec_[i]->size_remaining();
            }
          }
        }
      }
    }

    return retval;
  }

  /**
   *
   * @param intpx
   * @param size_requested
   * @param min_size_change_to_modify
   * @return
   */
  inline int CancelOrModifyAskOrdersEqAboveIntPirce(const int intpx, const int size_requested,
                                                    const int min_size_change_to_modify) {
    int retval = 0;
    int ask_index_ = GetAskIndex(intpx);

    if (order_vec_top_ask_index_ != -1) {
      for (int index_ = order_vec_top_ask_index_; index_ >= std::max(ask_index_, order_vec_bottom_ask_index_);
           index_--) {
        if ((p_ticks_to_keep_ask_int_price_) && (*p_ticks_to_keep_ask_int_price_) > 0 &&
            GetAskIntPrice(index_) == (*p_ticks_to_keep_ask_int_price_)) {  // Do not cancel an order
                                                                            // at this ask-int-price
          continue;
        }

        std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[index_];
        if (!_this_base_order_vec_.empty()) {
          for (int i = _this_base_order_vec_.size() - 1; i >= 0; i--) {
            if (_this_base_order_vec_[i]->size_remaining() + retval > size_requested) {
              // The difference of size it could not cancel is more than minimum size to be modified,
              // then modify the order
              if (size_requested - retval > min_size_change_to_modify) {
                auto order = _this_base_order_vec_[i];
                auto new_size = order->size_remaining() - (size_requested - retval);

                // Only if we are able to modify this order, we return the retval
                // If we are not able to modify this order the proceed to next order if its there
                if (ModifyOrderAndLog(order, order->price(), order->int_price(), new_size)) {
                  retval += (size_requested - retval);
                  return retval;
                }
              } else {
                return retval;
              }
            } else {
              if (Cancel(*(_this_base_order_vec_[i]))) {
                retval += _this_base_order_vec_[i]->size_remaining();
              }
            }
          }
        }
      }
    }

    return retval;
  }

  inline int CancelAsksEqAboveIntPrice(const int _intpx_) {
    int retval = 0;
    int ask_index_ = GetAskIndex(_intpx_);

    if (order_vec_top_ask_index_ != -1) {
      for (int index_ = order_vec_top_ask_index_; index_ >= std::max(ask_index_, order_vec_bottom_ask_index_);
           index_--) {
        if ((p_ticks_to_keep_ask_int_price_) && (*p_ticks_to_keep_ask_int_price_) > 0 &&
            GetAskIntPrice(index_) == (*p_ticks_to_keep_ask_int_price_)) {  // Do not cancel an order
                                                                            // at this ask-int-price
          continue;
        }

        std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[index_];
        if (!_this_base_order_vec_.empty()) {
          for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
            if (Cancel(*(_this_base_order_vec_[i]))) {
              retval += _this_base_order_vec_[i]->size_remaining();
            }
          }
        }
      }
    }

    return retval;
  }

  inline int CancelAsksAboveIntPrice(const int _intpx_) {
    int retval = 0;
    int ask_index_ = GetAskIndex(_intpx_);

    if (order_vec_top_ask_index_ != -1) {
      for (int index_ = order_vec_top_ask_index_; index_ > std::max(ask_index_, order_vec_bottom_ask_index_ - 1);
           index_--) {
        if ((p_ticks_to_keep_ask_int_price_) && (*p_ticks_to_keep_ask_int_price_) > 0 &&
            GetAskIntPrice(index_) == (*p_ticks_to_keep_ask_int_price_)) {  // Do not cancel an order
                                                                            // at this ask-int-price
          continue;
        }

        std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[index_];
        if (!_this_base_order_vec_.empty()) {
          for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
            if (Cancel(*(_this_base_order_vec_[i]))) {
              retval += _this_base_order_vec_[i]->size_remaining();
            }
          }
        }
      }
    }

    return retval;
  }

  inline int CancelAsksAtIntPrice(const int _intpx_) {
    if ((p_ticks_to_keep_ask_int_price_) && (*p_ticks_to_keep_ask_int_price_) > 0 &&
        _intpx_ == (*p_ticks_to_keep_ask_int_price_)) {  // Do not cancel an
                                                         // order at this
                                                         // ask-int-price
      return 0;
    }

    int retval = 0;
    int ask_index_ = GetAskIndex(_intpx_);

    if (ask_index_ >= 0 && ask_index_ < ORDER_MANAGER_INT_PRICE_RANGE && order_vec_top_ask_index_ != -1) {
      std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[ask_index_];

      if (!_this_base_order_vec_.empty()) {
        for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
          if (Cancel(*(_this_base_order_vec_[i]))) {
            retval += _this_base_order_vec_[i]->size_remaining();
          }
        }
      }
    }

    return retval;
  }

  inline int CancelAsksFromFar(const int _required_size_, int _min_ask_int_px_to_cancel_ = kInvalidIntPrice) {
    int retval = 0;

    if (order_vec_bottom_ask_index_ != -1) {
      for (int index_ = order_vec_bottom_ask_index_; index_ <= order_vec_top_ask_index_; index_++) {
        if ((p_ticks_to_keep_ask_int_price_) && (*p_ticks_to_keep_ask_int_price_) > 0 &&
            GetAskIntPrice(index_) == (*p_ticks_to_keep_ask_int_price_)) {  // Do not cancel an order
                                                                            // at this ask-int-price
          continue;
        }

        if (_min_ask_int_px_to_cancel_ != kInvalidIntPrice && GetAskIntPrice(index_) < _min_ask_int_px_to_cancel_) {
          break;
        }

        std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[index_];
        if (!_this_base_order_vec_.empty()) {
          for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
            if (Cancel(*(_this_base_order_vec_[i]))) {
              retval += _this_base_order_vec_[i]->size_remaining();
            }
          }
        }
        if (retval >= _required_size_) {
          break;
        }
      }
    }

    return retval;
  }

  inline int CancelAsksBelowIntPrice(const int _intpx_) {
    int retval = 0;

    if (order_vec_top_ask_index_ != -1) {
      for (int index_ = order_vec_top_ask_index_; index_ >= order_vec_bottom_ask_index_; index_--) {
        if ((p_ticks_to_keep_ask_int_price_) && (*p_ticks_to_keep_ask_int_price_) > 0 &&
            GetAskIntPrice(index_) == (*p_ticks_to_keep_ask_int_price_)) {  // Do not cancel an order
                                                                            // at this ask-int-price
          continue;
        }

        if (GetAskIntPrice(index_) > _intpx_) {
          std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[index_];
          if (!_this_base_order_vec_.empty()) {
            retval += CancelOrdersInVec(_this_base_order_vec_);
          }
        }
      }
    }

    return retval;
  }

  inline int CancelAsksEqBelowIntPrice(const int _intpx_) {
    int retval = 0;

    if (order_vec_top_ask_index_ != -1) {
      for (int index_ = order_vec_top_ask_index_; index_ >= order_vec_bottom_ask_index_; index_--) {
        if ((p_ticks_to_keep_ask_int_price_) && (*p_ticks_to_keep_ask_int_price_) > 0 &&
            GetAskIntPrice(index_) == (*p_ticks_to_keep_ask_int_price_)) {  // Do not cancel an order
                                                                            // at this ask-int-price
          continue;
        }

        if (GetAskIntPrice(index_) >= _intpx_) {
          std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[index_];
          if (!_this_base_order_vec_.empty()) {
            retval += CancelOrdersInVec(_this_base_order_vec_);
          }
        }
      }
    }

    return retval;
  }

  void AddPositionChangeListener(PositionChangeListener* p_this_listener_) {
    if (p_this_listener_ == NULL) return;
    if ((s_p_position_change_listener_ == NULL) && (position_change_listener_vec_.empty())) {
      s_p_position_change_listener_ = p_this_listener_;
    } else {
      VectorUtils::UniqueVectorAdd(position_change_listener_vec_, p_this_listener_);
    }
  }

  void AddExecutionListener(ExecutionListener* p_this_listener_) {
    if (p_this_listener_ == NULL) return;
    if ((s_p_execution_listener_ == NULL) && (execution_listener_vec_.empty())) {
      s_p_execution_listener_ = p_this_listener_;
    } else {
      VectorUtils::UniqueVectorAdd(execution_listener_vec_, p_this_listener_);
    }
  }

  void RemoveExecutionLister(ExecutionListener* p_this_listener_) {
    if (p_this_listener_ == s_p_execution_listener_) {
      s_p_execution_listener_ = NULL;
    }
    HFSAT::VectorUtils::UniqueVectorRemove(execution_listener_vec_, p_this_listener_);
  }

  void AddOrderChangeListener(OrderChangeListener* p_this_listener_) {
    if (p_this_listener_ == NULL) return;
    if ((s_p_order_change_listener_ == NULL) && (order_change_listener_vec_.empty())) {
      s_p_order_change_listener_ = p_this_listener_;
    } else {
      VectorUtils::UniqueVectorAdd(order_change_listener_vec_, p_this_listener_);
    }
  }

  void AddCancelRejectListener(CancelRejectListener* p_this_listener_) {
    if (p_this_listener_ == NULL) return;
    if ((s_p_cxl_reject_listener_ == NULL) && (cxl_reject_listener_vec_.empty())) {
      s_p_cxl_reject_listener_ = p_this_listener_;
    } else {
      VectorUtils::UniqueVectorAdd(cxl_reject_listener_vec_, p_this_listener_);
    }
  }

  void AddFokFillRejectListener(FokFillRejectListener* p_this_listener_) {
    if (p_this_listener_ == NULL) return;
    if ((s_p_fok_fill_reject_listener_ == NULL) && (fok_fill_reject_listener_vec_.empty())) {
      s_p_fok_fill_reject_listener_ = p_this_listener_;
    } else {
      VectorUtils::UniqueVectorAdd(fok_fill_reject_listener_vec_, p_this_listener_);
    }
  }

  void AddRejectDueToFundsListener(RejectDueToFundsListener* p_this_listener_) {
    if (p_this_listener_ == NULL) return;
    VectorUtils::UniqueVectorAdd(reject_due_to_funds_listener_vec_, p_this_listener_);
  }

  void AddExchangeRejectsListeners(ExchangeRejectsListener* p_new_listener) {
    if (NULL == p_new_listener) return;
    VectorUtils::UniqueVectorAdd(exch_rejects_listener_vec_, p_new_listener);
  }

  void AddORSRejectsFreezeListener(ORSRejectsFreezeListener* p_new_listener) {
    if (NULL == p_new_listener) return;
    VectorUtils::UniqueVectorAdd(ors_rejects_freeze_listener_vec_, p_new_listener);
  }

  // THERE IS NO NEED TO REPLAY MESSAGE BACK NOW, THIS INVOLVES A LOT OF
  // OVERHEAD ON THE ORS END, AFFECTS THE THROUGHPUT
  // OF ORS REQUEST PROCESSING THREAD
  // WE DON'T EXPECT TO MISS ANY PACKETS SINCE WE ARE USING COMBINED WRITER
  // SETUP, THIS SHOULD IMPROVE LATENCY ON THE
  // SINGLE THREADED QUERY AS WELL
  // CAUTION : NEVER RESTART/STOP COMBINED WRITER UNTIL QUERIES ARE FLAT //ravi
  inline void ReplayProbablyMissedFromORS() {}

  void LogFullStatus();
  void CleanSumSizeMapsBasedOnOrderMaps();

  std::string ShowOrders() const;

  inline int trade_volume() const { return trade_volume_; }

  virtual void ComputeQueueSizes() { queue_sizes_needed_ = true; }

  int SupportingOrderFilledPercent() const {
    unsigned int denom =
        supporting_order_filled_ + best_level_order_filled_ + aggressive_order_filled_ + improve_order_filled_;
    return (denom <= 0u) ? 0 : (int)((supporting_order_filled_ * 100u) / denom);
  }

  int BestLevelOrderFilledPercent() const {
    unsigned int denom =
        supporting_order_filled_ + best_level_order_filled_ + aggressive_order_filled_ + improve_order_filled_;
    return (denom <= 0u) ? 0 : (int)((best_level_order_filled_ * 100u) / denom);
  }

  int AggressiveOrderFilledPercent() const {
    unsigned int denom =
        supporting_order_filled_ + best_level_order_filled_ + aggressive_order_filled_ + improve_order_filled_;
    return (denom <= 0u) ? 0 : (int)((aggressive_order_filled_ * 100u) / denom);
  }

  int ImproveOrderFilledPercent() const {
    unsigned int denom =
        supporting_order_filled_ + best_level_order_filled_ + aggressive_order_filled_ + improve_order_filled_;
    return (denom <= 0u) ? 0 : (int)((improve_order_filled_ * 100u) / denom);
  }

  int AllOrderFilledPercent() const {
    return (total_size_placed_ > 0) ? (trade_volume_ * 100u / total_size_placed_) : 0;
  }

  int SendOrderCount() const { return send_order_count_; }

  int CxlOrderCount() const { return cxl_order_count_; }

  int ModifyOrderCount() const { return modify_order_count_; }

  inline void AddToTradeVolume(int fp_size_executed_) { trade_volume_ += fp_size_executed_; }

 protected:
  /// function to check all three types of maps ( intpx to sum_uncofirmed,
  /// sum_confirmed, order_vec )
  /// and remove keys of the entries which are empty to hopefully make the map
  /// more efficient in operation.
  void ImplCleanMaps();

 public:
  /** \brief called by ORSMessageLiveSource when the messagetype is
   *kORRType_None
   *
   * sort of callback, called by ReplyHandler on receiving ORS which is
   *typically a response to a replay request for an
   * order that the ORS knows nothing about
   * @param _server_assigned_client_id_ assigned by server to the client whose
   *order this message pertains to
   * @param _client_assigned_order_sequence_ assigned by client before sending
   *order to ORS
   * @param _server_assigned_order_sequence_ assigned by ORS before sending to
   *exchange
   * @param _security_id_ the sid of the security, computed by the source of the
   *message .. ORSMessageFileSource or
   * ORSMessageLiveSource or SimMarketMaker
   * @param _buysell_ the side of the order, kTradeTypeBuy if this was a passive
   *bid order or aggressive BUY order
   * @param _int_price_ this is sent by the client when sending the order, this
   *is typically a key to a map where the
   * client has stored info of this order. In particular
   *intpx_2_sum_bid_confirmed_ intpx_2_sum_bid_unconfirmed_
   * intpx_2_bid_order_vec_ all use this key. Sending this and receiving this in
   *the reply saves a computation from
   * price and also protects against situations when the order was filled at a
   *price that is not the limit price sent
   */
  void OrderNotFound(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                     const TradeType_t _buysell_, const int _int_price_, const int32_t server_assigned_message_sequence,
                     const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  void OrderSequenced(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                      const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                      const int _size_executed_, const int _client_position_, const int _global_position_,
                      const int _int_price_, const int32_t server_assigned_message_sequence,
                      const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  void OrderConfirmed(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                      const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                      const int _size_executed_, const int _client_position_, const int _global_position_,
                      const int _int_price_, const int32_t server_assigned_message_sequence,
                      const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  inline void OrderORSConfirmed(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                                const int _size_executed_, const int _int_price_,
                                const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                const ttime_t time_set_by_server) {
    return OrderConfirmed(_server_assigned_client_id_, _client_assigned_order_sequence_,
                          _server_assigned_order_sequence_, _security_id_, _price_, _buysell_, _size_remaining_,
                          _size_executed_, client_position_, global_position_, _int_price_,
                          server_assigned_message_sequence, exchange_order_id, time_set_by_server);
  }

  void OrderConfCxlReplaced(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                            const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                            const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                            const int _size_executed_, const int _client_position_, const int _global_position_,
                            const int _int_price_, const int32_t server_assigned_message_sequence,
                            const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  void OrderConfCxlReplaceRejected(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                   const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                   const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                                   const int _client_position_, const int _global_position_, const int _int_price_,
                                   const int32_t rejection_reason, const int32_t server_assigned_message_sequence,
                                   const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  void OrderCanceled(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const TradeType_t _buysell_, const int _size_remaining_, const int _client_position_,
                     const int _global_position_, const int _int_price_, const int32_t server_assigned_message_sequence,
                     const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  void OrderCancelRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                           const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                           const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                           const int _rejection_reason_, const int t_client_position_, const int t_global_position_,
                           const int r_int_price_, const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  void OrderExecuted(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const TradeType_t _buysell_, const int _size_remaining_, const int _size_executed_,
                     const int _client_position_, const int _global_position_, const int _int_price_,
                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const ttime_t time_set_by_server);

  void PrintCancelSeqdExecTimes();

  inline void OrderInternallyMatched(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                     const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                     const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                                     const int _size_executed_, const int _client_position_,
                                     const int _global_position_, const int _int_price_,
                                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                     const ttime_t time_set_by_server) {
    OrderExecuted(_server_assigned_client_id_, _client_assigned_order_sequence_, _server_assigned_order_sequence_,
                  _security_id_, _price_, _buysell_, _size_remaining_, _size_executed_, _client_position_,
                  _global_position_, _int_price_, server_assigned_message_sequence, exchange_order_id,
                  time_set_by_server);
  }

  void OrderRejected(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const unsigned int _security_id_, const double _price_, const TradeType_t _buysell_,
                     const int _size_remaining_, const int _rejection_reason_, const int _int_price_,
                     const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  void OrderRejectedDueToFunds(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                               const unsigned int _security_id_, const double _price_, const TradeType_t _buysell_,
                               const int _size_remaining_, const int _rejection_reason_, const int _int_price_,
                               const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  void setMargin(double margin_);
  void WakeUpifRejectedDueToFunds();

  virtual double GetMidPrice() { return 0.0; }  ///< stub for SmartOrderManager to override

  void SanitizeMaps();

  void UpdateAutoFreezeSystem(bool const& should_enforce_autofreeze) {
    is_auto_freeze_active_ = should_enforce_autofreeze;

    // Disable further freeze due to rejects, additionally reset existing freeze
    // here checking that rejects based freeze is not 0 is very important
    // otherwise we may potentially affect some other flat condition and end up
    // starting
    // query, this ensures that only if there was freeze due to rejects we are
    // turning it off
    if (0 != reject_based_freeze_timer_ && false == should_enforce_autofreeze) {
      ResetRejectsBasedFreeze();
    }
  }

  void SumBidAskSizesFake() {
    // Loads up some vectors for fake send (CPU cache warming)
    if (sum_total_bid_unconfirmed_.size() > 0 && sum_total_bid_unconfirmed_[0] == -1000) {
      dbglogger_ << "Error: These values shouldn't be present " << sum_total_bid_unconfirmed_[0] << DBGLOG_ENDL_FLUSH;
    }
    if (sum_total_ask_unconfirmed_.size() > 0 && sum_total_ask_unconfirmed_[0] == -1000) {
      dbglogger_ << "Error: These values shouldn't be present " << sum_total_ask_unconfirmed_[0] << DBGLOG_ENDL_FLUSH;
    }
    if (sum_total_bid_confirmed_.size() > 0 && sum_total_bid_confirmed_[0] == -1000) {
      dbglogger_ << "Error: These values shouldn't be present " << sum_total_bid_confirmed_[0] << DBGLOG_ENDL_FLUSH;
    }
    if (sum_total_ask_confirmed_.size() > 0 && sum_total_ask_confirmed_[0] == -1000) {
      dbglogger_ << "Error: These values shouldn't be present " << sum_total_ask_confirmed_[0] << DBGLOG_ENDL_FLUSH;
    }
  }

  void FakeCancel() {
    // Loads up some vectors for fake send (CPU cache warming)
    if (order_vec_top_bid_index_ != -1) {
      std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[order_vec_top_bid_index_];
      if (!_this_base_order_vec_.empty()) {
        if (_this_base_order_vec_[0]->modified_new_int_price_ == -2000) {
          dbglogger_ << "Error: These values shouldn't be present " << _this_base_order_vec_[0]->modified_new_int_price_
                     << DBGLOG_ENDL_FLUSH;
        }
      }
    }
    if (order_vec_top_ask_index_ != -1) {
      std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[order_vec_top_ask_index_];
      if (!_this_base_order_vec_.empty()) {
        if (_this_base_order_vec_[0]->modified_new_int_price_ == -2000) {
          dbglogger_ << "Error: These values shouldn't be present " << _this_base_order_vec_[0]->modified_new_int_price_
                     << DBGLOG_ENDL_FLUSH;
        }
      }
    }

    if (cancel_order_.queue_size_ahead_ == -10001) {
      dbglogger_ << "Error: These values shouldn't be present " << template_order.queue_size_ahead_
                 << DBGLOG_ENDL_FLUSH;
    }

    base_trader_.FakeSendTrade();
  }

  void FakeSendTrade() {
    // Loads up some vectors for fake send (CPU cache warming)
    if (unsequenced_bids_.size() > 0 && unsequenced_bids_[0]->size_requested_ == -10001) {
      dbglogger_ << "Error: These values shouldn't be present " << unsequenced_bids_[0]->size_requested_
                 << DBGLOG_ENDL_FLUSH;
    }
    if (unsequenced_asks_.size() > 0 && unsequenced_asks_[0]->size_requested_ == -10001) {
      dbglogger_ << "Error: These values shouldn't be present " << unsequenced_asks_[0]->size_requested_
                 << DBGLOG_ENDL_FLUSH;
    }

    if (template_order.queue_size_ahead_ == -10001) {
      dbglogger_ << "Error: These values shouldn't be present " << template_order.queue_size_ahead_
                 << DBGLOG_ENDL_FLUSH;
    }

    base_trader_.FakeSendTrade();
  }

 protected:
  /// Simple function to get from double price to int price. Used in
  /// SendTradeDblPx
  inline int GetIntPx(const double& _price_) const { return fast_price_convertor_.GetFastIntPx(_price_); }

  inline void AdjustPosition(const int t_client_position_, const double _trade_price_, const int r_int_price_) {
    // this can be called any number of times ... ti will only do sth if
    // client_position_ != t_client_position_
    if (client_position_ != t_client_position_) {
      int position_diff_ = t_client_position_ - client_position_;

      // updating position here since this gives control to strategy
      client_position_ = t_client_position_;

      NotifyExecAndPosListeners(position_diff_, _trade_price_, r_int_price_);
    }
  }

  bool NotifyExecAndPosListeners(int _position_diff_, const double _trade_price_, const int _int_price_);

  inline void NotifyOrderChangeListeners() {
    if (s_p_order_change_listener_ != NULL) {
      s_p_order_change_listener_->OnOrderChange();
    }
    for (auto i = 0u; i < order_change_listener_vec_.size(); i++) {
      order_change_listener_vec_[i]->OnOrderChange();
    }
  }

  inline void NotifyCancelRejectListeners(const TradeType_t _buysell_, const double _price_, const int _r_int_price_) {
    if (s_p_cxl_reject_listener_ != NULL) {
      s_p_cxl_reject_listener_->OnCancelReject(_buysell_, _price_, _r_int_price_, dep_security_id_);
    }
    for (auto i = 0u; i < cxl_reject_listener_vec_.size(); i++) {
      cxl_reject_listener_vec_[i]->OnCancelReject(_buysell_, _price_, _r_int_price_, dep_security_id_);
    }
  }

  inline void NotifyFokFillListeners(const TradeType_t _buysell_, const double _price_, const int _r_int_price_,
                                     const int _size_exec_) {
    if (s_p_fok_fill_reject_listener_ != NULL) {
      s_p_fok_fill_reject_listener_->OnFokFill(_buysell_, _price_, _r_int_price_, _size_exec_);
    }
    for (auto i = 0u; i < fok_fill_reject_listener_vec_.size(); i++) {
      fok_fill_reject_listener_vec_[i]->OnFokFill(_buysell_, _price_, _r_int_price_, _size_exec_);
    }
  }

  inline void NotifyFokRejectListeners(const TradeType_t _buysell_, const double _price_, const int _r_int_price_,
                                       const int _size_remaining_) {
    if (s_p_fok_fill_reject_listener_ != NULL) {
      s_p_fok_fill_reject_listener_->OnFokReject(_buysell_, _price_, _r_int_price_, _size_remaining_);
    }
    for (auto i = 0u; i < fok_fill_reject_listener_vec_.size(); i++) {
      fok_fill_reject_listener_vec_[i]->OnFokReject(_buysell_, _price_, _r_int_price_, _size_remaining_);
    }
  }

  // Let listeners know about the exchange rejects and the ideal action is to
  // freeze
  inline void NotifyExchangeRejectsListeners(HFSAT::BaseUtils::FreezeEnforcedReason const& freeze_reason) {
    // It's possible that we may end up sending more orders before freeze has
    // taken place
    // i.e as we are awaiting for rejects from exchange/ors which might take
    // time
    // makes sense to only notify once until reset is done
    if (0 == reject_based_freeze_timer_ && true == is_auto_freeze_active_) {
      for (uint32_t listener_counter = 0; listener_counter < exch_rejects_listener_vec_.size(); listener_counter++) {
        exch_rejects_listener_vec_[listener_counter]->OnGetFreezeDueToExchangeRejects(freeze_reason);
      }
    }
  }

  inline void NotifyORSRejectsFreezeListeners() {
    // It's possible that we may end up sending more orders before freeze has
    // taken place
    // i.e as we are awaiting for rejects from exchange/ors which might take
    // time
    // makes sense to only notify once until reset is done
    if (0 == reject_based_freeze_timer_ && true == is_auto_freeze_active_) {
      for (uint32_t listener_counter = 0; listener_counter < ors_rejects_freeze_listener_vec_.size();
           listener_counter++) {
        ors_rejects_freeze_listener_vec_[listener_counter]->OnGetFreezeDueToORSRejects();
      }
    }
  }

  // This will only happen if one gives an external start/getflat message to
  // query
  inline void NotifyResetByManualInterventionOverRejects() {
    // Conditional pass so we don't end up notifying multiple times
    if (0 != reject_based_freeze_timer_) {
      for (uint32_t listener_counter = 0; listener_counter < exch_rejects_listener_vec_.size(); listener_counter++) {
        exch_rejects_listener_vec_[listener_counter]->OnResetByManualInterventionOverRejects(HFSAT::BaseUtils::FreezeEnforcedReason::kAllow);
      }
      // TODO Right Now Not Used
      //      for(uint32_t listener_counter = 0; listener_counter <
      //      ors_rejects_freeze_listener_vec_.size ();
      //      listener_counter ++){
      //        ors_rejects_freeze_listener_vec_[listener_counter]->OnResetByManualInterventionOverRejects
      //        () ;
      //      }
    }
  }

  // fast inlined function to remove an order_ptr from the vector containing it
  inline void RemoveOrderFromVec(std::vector<BaseOrder*>& _this_vec_, const BaseOrder* p_this_order_) {
    for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_vec_.begin();
         _base_order_vec_iter_ != _this_vec_.end(); _base_order_vec_iter_++) {
      if ((*_base_order_vec_iter_) == p_this_order_) {
        _base_order_vec_iter_ = _this_vec_.erase(_base_order_vec_iter_);
        return;
      }
    }
  }

  virtual void InitialQueueBid(BaseOrder* p_newly_confirmed_bid_order_) {}
  virtual void InitialQueueAsk(BaseOrder* p_newly_confirmed_ask_order_) {}

  /// functions to set as supporting orders that may have been placed at top
  /// level but are now not at the best level
  void ResetPlacedLevelIndicatorOnBids(const int t_best_bid_int_price_);
  void ResetPlacedLevelIndicatorOnAsks(const int t_best_ask_int_price_);

  inline void NoteLevelExec(const char placed_at_level_indicator_) {
    switch (placed_at_level_indicator_) {
      case 'S':
        supporting_order_filled_++;
        break;
      case 'B':
        best_level_order_filled_++;
        break;
      case 'A':
        aggressive_order_filled_++;
        break;
      case 'I':
        improve_order_filled_++;
        break;
      default:
        break;
    }
  }

  void PropagateUnconfirmedSizeDifference(int base_index_, int size_requested_, TradeType_t t_buysell_) {
    // It calculates a sum_total_x vector which can be used to directly idenitfy the sum of order sizes
    // between two indices, making that calculation O(1).
    // Example: for sum_bid_confirmed vector as 1 2 3, its total sum vector will be 6 5 3
    // So if we want sum of till index 1 (element 2). we can directly calculate it using total sum vector
    // by 6-5 + 2
    // On each change to the sum_bid vector, the change is propagated incrementally in sum_total vector
    // towards the top index
    if (t_buysell_ == kTradeTypeBuy) {
      int top_index_ = (unconfirmed_top_bid_index_ > base_index_ ? unconfirmed_top_bid_index_ : base_index_);
      for (int index_ = base_index_; index_ <= top_index_; index_++) {
        if (unconfirmed_top_bid_index_ == -1) sum_total_bid_unconfirmed_[index_] = 0;
        sum_total_bid_unconfirmed_[index_] += size_requested_;
        if (sum_total_bid_unconfirmed_[index_] < 0) sum_total_bid_unconfirmed_[index_] = 0;
      }
    } else if (t_buysell_ == kTradeTypeSell) {
      int top_index_ = (unconfirmed_top_ask_index_ > base_index_ ? unconfirmed_top_ask_index_ : base_index_);
      for (int index_ = base_index_; index_ <= top_index_; index_++) {
        if (unconfirmed_top_ask_index_ == -1) sum_total_ask_unconfirmed_[index_] = 0;
        sum_total_ask_unconfirmed_[index_] += size_requested_;
        if (sum_total_ask_unconfirmed_[index_] < 0) sum_total_ask_unconfirmed_[index_] = 0;
      }
    }
  }

  void PropagateConfirmedSizeDifference(int base_index_, int size_requested_, TradeType_t t_buysell_) {
    // It calculates a sum_total_x vector which can be used to directly identify the sum of order sizes
    // between two indices, making that calculation O(1).
    // Example: for sum_bid_confirmed vector as 1 2 3, its total sum vector will be 6 5 3
    // So if we want sum of till index 1 (element 2). we can directly calculate it using total sum vector
    // by 6-5 + 2
    // On each change to the sum_bid vector, the change is propagated incrementally in sum_total vector
    // towards the top index
    if (t_buysell_ == kTradeTypeBuy) {
      int top_index_ = (confirmed_top_bid_index_ > base_index_ ? confirmed_top_bid_index_ : base_index_);
      for (int index_ = base_index_; index_ <= top_index_; index_++) {
        if (confirmed_top_bid_index_ == -1) sum_total_bid_confirmed_[index_] = 0;
        sum_total_bid_confirmed_[index_] += size_requested_;
        if (sum_total_bid_confirmed_[index_] < 0) sum_total_bid_confirmed_[index_] = 0;
      }
    } else if (t_buysell_ == kTradeTypeSell) {
      int top_index_ = (confirmed_top_ask_index_ > base_index_ ? confirmed_top_ask_index_ : base_index_);
      for (int index_ = base_index_; index_ <= top_index_; index_++) {
        if (confirmed_top_ask_index_ == -1) sum_total_ask_confirmed_[index_] = 0;
        sum_total_ask_confirmed_[index_] += size_requested_;
        if (sum_total_ask_confirmed_[index_] < 0) sum_total_ask_confirmed_[index_] = 0;
      }
    }
  }

  void CalculateSumVectorUnconfirmed(TradeType_t t_buysell_, int prev_top_index_) {
    // In case of non incremental changes to sum_x_unconfirmed vectors, this calculates the
    // value from scratch. However, for eg in case of shift of top index, we calculate incrementally
    // from last known top index, saving us some calculation.
    // prev_top_index == -1 signifies that we need to calculate from scratch.
    if (prev_top_index_ != -1) {
      if (t_buysell_ == kTradeTypeBuy) {
        for (int index_ = prev_top_index_ + 1; index_ <= unconfirmed_top_bid_index_; index_++) {
          sum_total_bid_unconfirmed_[index_] =
              sum_total_bid_unconfirmed_[prev_top_index_] + sum_bid_unconfirmed_[index_];
        }
      } else if (t_buysell_ == kTradeTypeSell) {
        for (int index_ = prev_top_index_ + 1; index_ <= unconfirmed_top_ask_index_; index_++) {
          sum_total_ask_unconfirmed_[index_] =
              sum_total_ask_unconfirmed_[prev_top_index_] + sum_ask_unconfirmed_[index_];
        }
      }
    } else {
      int sum = 0;
      if (t_buysell_ == kTradeTypeBuy) {
        for (int index_ = unconfirmed_bottom_bid_index_; index_ <= unconfirmed_top_bid_index_; index_++) {
          sum_total_bid_unconfirmed_[index_] = sum + sum_bid_unconfirmed_[index_];
          sum += sum_bid_unconfirmed_[index_];
        }
      } else if (t_buysell_ == kTradeTypeSell) {
        for (int index_ = unconfirmed_bottom_ask_index_; index_ <= unconfirmed_top_ask_index_; index_++) {
          sum_total_ask_unconfirmed_[index_] = sum + sum_ask_unconfirmed_[index_];
          sum += sum_ask_unconfirmed_[index_];
        }
      }
    }
  }

  void CalculateSumVectorConfirmed(TradeType_t t_buysell_, int prev_top_index_) {
    // In case of non incremental changes to sum_x_confirmed vectors, this calculates the
    // value from scratch. However, for eg in case of shift of top index, we calculate incrementally
    // from last known top index, saving us some calculation.
    // prev_top_index == -1 signifies that we need to calculate from scratch.
    if (prev_top_index_ != -1) {
      if (t_buysell_ == kTradeTypeBuy) {
        for (int index_ = prev_top_index_ + 1; index_ <= confirmed_top_bid_index_; index_++) {
          sum_total_bid_confirmed_[index_] = sum_total_bid_confirmed_[prev_top_index_] + sum_bid_confirmed_[index_];
        }
      } else if (t_buysell_ == kTradeTypeSell) {
        for (int index_ = prev_top_index_ + 1; index_ <= confirmed_top_ask_index_; index_++) {
          sum_total_ask_confirmed_[index_] = sum_total_ask_confirmed_[prev_top_index_] + sum_ask_confirmed_[index_];
        }
      }
    } else {
      int sum = 0;
      if (t_buysell_ == kTradeTypeBuy) {
        for (int index_ = confirmed_bottom_bid_index_; index_ <= confirmed_top_bid_index_; index_++) {
          sum_total_bid_confirmed_[index_] = sum + sum_bid_confirmed_[index_];
          sum += sum_bid_confirmed_[index_];
        }
      } else if (t_buysell_ == kTradeTypeSell) {
        for (int index_ = confirmed_bottom_ask_index_; index_ <= confirmed_top_ask_index_; index_++) {
          sum_total_ask_confirmed_[index_] = sum + sum_ask_confirmed_[index_];
          sum += sum_ask_confirmed_[index_];
        }
      }
    }
  }

 private:
  //@@Function : TimelyDropDetectionAndRecovery
  //
  // This function checks if we have requested some packets via recovery and
  // those have still not
  // recovered after a fixed timeout, we need to re-request the packets again.
  // This is called on time update
  void TimelyDropDetectionAndRecovery();

  //@@Function : GoForPacketRecovery
  //
  // Goes for an immediate recovery of the missing packets, submits the recovery
  // request via shm ( same as send trade )

  //@@Args : GoForPacketRecovery
  //
  //@expected_message_sequence - the first sequneced which we have missed in the
  // given range of missing packets
  //@current_received_sequence - current received sequence number based on which
  // we detected the drops
  void GoForPacketRecovery(const int32_t expected_message_sequence, const int32_t current_received_sequence);
};
}
#endif  // BASE_ORDERROUTING_BASE_ORDER_MANAGER_H
