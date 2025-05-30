/**
    \file OrderRouting/base_order.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_ORDERROUTING_BASE_ORDER_H
#define BASE_ORDERROUTING_BASE_ORDER_H

#include <iostream>
#include <string.h>
#include <vector>
#include <map>
#include <tr1/unordered_map>
#include <unordered_set>
#include "dvccode/CDef/defines.hpp"
#include "baseinfra/OrderRouting/defines.hpp"
#include "baseinfra/OrderRouting/size_maps.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
namespace HFSAT {

/// \brief struct used for storing all information of an order
struct BaseOrder {
  const char* security_name_;  ///< exchange symbol
  TradeType_t buysell_;
  double price_;
  double modified_new_price_;

  int size_remaining_;
  int size_requested_;
  int size_disclosed_;

  bool is_mirror_order_;
  int mirror_factor_;

  int int_price_;  ///< only to save computation

  int modified_new_size_;
  int modified_new_int_price_;
  int num_open_modified_orders_;

  ORRType_t order_status_;  ///< what state the order is in

  int queue_size_ahead_;
  int queue_size_behind_;
  int queue_orders_ahead_;
  int queue_orders_behind_;

  /// number of times Enqueue has been called. i.e. this order has been at L1 and there has been an L1 update
  int num_events_seen_;

  /// msecs_from_midnight when the order was sequenced
  int seqd_msecs_;
  int seqd_usecs_;
  int mod_msecs_;  // for message reduction purposes
  ttime_t cancel_seqd_time_;
  /// msecs_from_midnight when the order was confirmed
  int placed_msecs_;

  int client_assigned_order_sequence_;
  int server_assigned_order_sequence_;

  int num_outstanding_mirror_orders_;

  int size_executed_;  ///< the size of the order already executed. primarily used by PriceLevelSimMarketMaker

  bool canceled_;
  bool replayed_;  ///< flag used by BaseOrderManager
  bool modified_;
  char placed_at_level_indicator_;  ///< S for supporting, B for best, A for aggressive / above-best

  bool is_fok_;
  bool is_ioc_;
  // mutable bool is_cancellable_before_confirmation;

  uint64_t exchange_order_id_;  // orderid of this order set by exchange - currently only updated on Confirm

  int executioner_id_;
  uint64_t this_ptr_;
  uint64_t ors_ptr_;

  bool is_reserved_type_;  // We have special reseve messsage limit if this is true

  /// @brief constructor initializes everything with invalid values
  BaseOrder()
      : security_name_(NULL),
        buysell_(kTradeTypeNoInfo),
        price_(0),
        modified_new_price_(0),
        size_remaining_(0),
        size_requested_(0),
        size_disclosed_(0),
        is_mirror_order_(false),
        mirror_factor_(1),
        int_price_(0),
        modified_new_size_(0),
        modified_new_int_price_(0),
        num_open_modified_orders_(0),
        order_status_(kORRType_None),
        queue_size_ahead_(0),
        queue_size_behind_(0),
        queue_orders_ahead_(0),
        queue_orders_behind_(0),
        num_events_seen_(0),
        seqd_msecs_(-1),
        seqd_usecs_(-1),
        placed_msecs_(-1),
        client_assigned_order_sequence_(-1),
        server_assigned_order_sequence_(-1),
        size_executed_(0),
        canceled_(false),
        replayed_(false),
        modified_(false),
        placed_at_level_indicator_('S'),
        is_fok_(false),
        is_ioc_(false),
        exchange_order_id_(0),
        executioner_id_(-1),
        is_reserved_type_(false) {
    cancel_seqd_time_.tv_sec = 0;
    cancel_seqd_time_.tv_usec = 0;
  }

  inline void ConfirmNewSize(int _size_remaining_) { size_remaining_ = size_requested_ = _size_remaining_; }

  inline void SequenceAtTime(const int _seqd_msecs_, const int _seqd_usecs_ = 0) {
    order_status_ = kORRType_Seqd;
    size_remaining_ = size_requested_;
    seqd_msecs_ = _seqd_msecs_;
    seqd_usecs_ = _seqd_usecs_;
  }

  inline void ConfirmAtTime(const int _placed_msecs_) {
    order_status_ = kORRType_Conf;
    placed_msecs_ = _placed_msecs_;
    num_events_seen_ = 0;
  }

  // /// Called by PriceLevelSimMarketMaker to update the size executed of this order
  inline void Execute(int this_exec_size) { size_executed_ = this_exec_size; }

  /// @brief called by SmartOrderManager to update placeinline
  inline void Enqueue(const int t_queue_size_, const int t_queue_orders_) {
    if (num_events_seen_ == 0) {
      queue_size_behind_ = 0;
      queue_orders_behind_ = 0;
      queue_size_ahead_ = t_queue_size_;
      queue_orders_ahead_ = t_queue_orders_;
      num_events_seen_ = 1;
    } else {
      if (queue_size_ahead_ > t_queue_size_) {
        queue_size_ahead_ = t_queue_size_;
      }
      queue_size_behind_ = t_queue_size_ - queue_size_ahead_;

      if (queue_orders_ahead_ > t_queue_orders_) {
        queue_orders_ahead_ = t_queue_orders_;
      }
      queue_orders_behind_ = t_queue_orders_ - queue_orders_ahead_;

      num_events_seen_++;
    }
  }

  inline void SetAloneAtLevel() { queue_size_ahead_ = queue_size_behind_ = 0; }
  inline void SendToTop() {
    queue_size_behind_ += queue_size_ahead_;
    queue_size_ahead_ = 0;
  }
  inline void ResetQueue() { queue_size_ahead_ = queue_size_ahead_ + queue_size_behind_; }
  inline void ModifySize(const int _new_size_, const int _placed_msecs_) {
    size_remaining_ = _new_size_;
    placed_msecs_ = _placed_msecs_;
    num_events_seen_ = 0;
  }

  inline const char* security_name() const { return security_name_; }
  inline TradeType_t buysell() const { return buysell_; }
  inline double price() const { return price_; }
  inline int size_remaining() const { return size_remaining_; }
  inline int size_requested() const { return size_requested_; }
  inline int size_disclosed() const { return size_disclosed_; }
  inline int mirror_factor() const { return mirror_factor_; }
  inline int int_price() const { return int_price_; }
  inline ORRType_t order_status() const { return order_status_; }
  inline int queue_size_ahead() const { return queue_size_ahead_; }
  inline int queue_size_behind() const { return queue_size_behind_; }
  inline int num_events_seen() const { return num_events_seen_; }
  inline int placed_msecs() const { return placed_msecs_; }
  inline int client_assigned_order_sequence() const { return client_assigned_order_sequence_; }
  inline int server_assigned_order_sequence() const { return server_assigned_order_sequence_; }
  inline int size_executed() const { return size_executed_; }
  inline bool canceled() const { return canceled_; }
  inline bool replayed() const { return replayed_; }

  // Function check whether an order can be cancelled or not
  inline bool CanBeCanceled(const int watch_minus_cxl_wait_msecs_, bool is_cancellable_before_confirmation) {
    return (
        !(canceled_) && !is_ioc_ &&
        (order_status_ == kORRType_Conf ||
         (order_status_ == kORRType_Seqd &&
          (is_cancellable_before_confirmation ||
           watch_minus_cxl_wait_msecs_ >= seqd_msecs_)))); /* we are checking for CXL_WAIT if not confirmed already */
  }

  inline bool CanBeModified(const int watch_minus_modify_wait_msecs_) const {
    return (!(modified_ || canceled_) && size_remaining_ > 0 &&  // Is this check necessary ? What happens to modify
                                                                 // messages which exchange receives after order is
                                                                 // executed
            !is_ioc_ && (order_status_ == kORRType_Conf ||
                         (order_status_ == kORRType_Seqd && watch_minus_modify_wait_msecs_ > seqd_msecs_)));
  }

  inline void SetAsSupporting() { placed_at_level_indicator_ = 'S'; }
  inline void SetAsImprove() { placed_at_level_indicator_ = 'I'; }
};

typedef std::map<int, std::vector<BaseOrder*>, std::greater<int> > BidPriceOrderMap;
typedef std::map<int, std::vector<BaseOrder*>, std::greater<int> >::iterator BidPriceOrderMapIter_t;
typedef std::map<int, std::vector<BaseOrder*>, std::greater<int> >::const_iterator BidPriceOrderMapConstIter_t;
typedef std::map<int, std::vector<BaseOrder*>, std::greater<int> >::reverse_iterator BidPriceOrderMapRevIter_t;
typedef std::map<int, std::vector<BaseOrder*>, std::less<int> > AskPriceOrderMap;
typedef std::map<int, std::vector<BaseOrder*>, std::less<int> >::iterator AskPriceOrderMapIter_t;
typedef std::map<int, std::vector<BaseOrder*>, std::less<int> >::const_iterator AskPriceOrderMapConstIter_t;
typedef std::map<int, std::vector<BaseOrder*>, std::less<int> >::reverse_iterator AskPriceOrderMapRevIter_t;
}

#endif  // BASE_ORDERROUTING_BASE_ORDER_H
