/**
   \file OrderRouting/base_sim_order.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_ORDERROUTING_BASE_SIM_ORDER_H
#define BASE_ORDERROUTING_BASE_SIM_ORDER_H

#include <iostream>
#include <string.h>
#include <vector>
#include <map>
#include <tr1/unordered_map>

#include "dvccode/CDef/defines.hpp"
#include "baseinfra/OrderRouting/defines.hpp"
#include "baseinfra/OrderRouting/size_maps.hpp"

#define MAX_MSECS_TO_CANCEL 100

namespace HFSAT {

/// \brief struct used for storing all information of an order for SimMarketMaker
struct BaseSimOrder {
  const char* security_name_;  ///< exchange symbol
  TradeType_t buysell_;
  double price_;
  int size_remaining_;

  int int_price_;           ///< stored only to save computation
  ORRType_t order_status_;  ///< what state the order is in

  int queue_size_ahead_;
  int queue_size_behind_;
  int queue_orders_ahead_;
  int queue_orders_behind_;
  int num_events_seen_;  ///< number of times Enqueue has been called. i.e. the number of times that this order has been
  /// at L1 and there has been an L1 size update
  int64_t order_id_;

  int client_assigned_order_sequence_;
  int server_assigned_order_sequence_;
  int server_assigned_client_id_;  ///< the unique client_id of the client who sent this order. primarily used by
  /// PriceLevelSimMarketMaker to know what to send as
  /// GenericORSReplyStruct::server_assigned_client_id_

  int min_order_size_;         ///< the minimum sized fill for this product
  int size_partial_executed_;  ///< the size of the order that would have been executed ignoring min_order_size
  int size_executed_;          ///< the size of the order already executed. primarily used by PriceLevelSimMarketMaker
  double size_fractional_executed_;  ///< the size of the order that should have been executed. primarily used by
  /// TimeProRataSimMarketMaker

  bool alone_above_best_market_;  ///< flag used by PriceLevelSimMarketMaker

  //========================================== OrderLevel ========================================//

  ttime_t order_sequenced_time_;  // this gives us an idea of our position in the order depth book
  ttime_t order_confirmation_time_;
  ttime_t order_entry_time_;
  bool correct_update_time_;
  bool priority_order_;

  //===============================================================================================//

  bool is_executed_;  // Needed when orders are marked as executed before OnTradePrint // currently only for
                      // quote-before-trade.
  int ors_exec_size_;

  bool is_fok_;
  bool is_ioc_;

  /// @brief constructor initializes everything with invalid values
  BaseSimOrder()
      : security_name_(NULL),
        buysell_(kTradeTypeNoInfo),
        price_(0),
        size_remaining_(0),
        int_price_(0),
        order_status_(kORRType_None),
        queue_size_ahead_(0),
        queue_size_behind_(0),
        queue_orders_ahead_(0),
        queue_orders_behind_(0),
        num_events_seen_(0),
        order_id_(0),
        client_assigned_order_sequence_(-1),
        server_assigned_order_sequence_(-1),
        server_assigned_client_id_(0),
        min_order_size_(1),
        size_partial_executed_(0),
        size_executed_(0),
        size_fractional_executed_(0),
        alone_above_best_market_(false),
        order_sequenced_time_(ttime_t(0, 0)),
        order_confirmation_time_(ttime_t(0, 0)),
        order_entry_time_(ttime_t(0, 0)),
        correct_update_time_(false),
        priority_order_(false),
        is_executed_(false),
        ors_exec_size_(0),
        is_fok_(false),
        is_ioc_(false) {}

  // inline void SetMinOrderSize ( int _min_order_size_ ) { min_order_size_= _min_order_size_; }
  inline void ConfirmNewSize(int _size_remaining_) { size_remaining_ = _size_remaining_; }

  inline void Confirm() {
    order_status_ = kORRType_Conf;
    num_events_seen_ = 0;
  }
  inline bool IsConfirmed() const { return (order_status_ == kORRType_Conf); }

  inline void SetExecuted(bool t_is_executed_) { is_executed_ = t_is_executed_; }
  inline bool IsExecuted() const { return is_executed_; }

  inline int ExecuteRemaining() {
    int this_size_executed = size_remaining_;
    size_remaining_ = 0;
    size_executed_ += this_size_executed;
    size_partial_executed_ = size_executed_;
    return this_size_executed;
  }

  inline int MatchPartial(int _further_match_) {
    int r_unsent_match_ = 0;
    if (_further_match_ > 0) {
      size_partial_executed_ += _further_match_;
      if (min_order_size_ > 1) {
        r_unsent_match_ =
            std::min(size_remaining_,
                     std::max(0, ((size_partial_executed_ - size_executed_) / min_order_size_) * min_order_size_));
      } else {
        r_unsent_match_ = std::min(size_remaining_, std::max(0, size_partial_executed_ - size_executed_));
      }
      if (r_unsent_match_ > 0) {
        size_executed_ += r_unsent_match_;
        size_remaining_ -= r_unsent_match_;
      }
    }
    return r_unsent_match_;
  }

  inline int SimulateMatchPartial(int _further_match_) const {
    int r_unsent_match_ = 0;
    if (_further_match_ > 0) {
      int t_size_partial_executed_ = size_partial_executed_ + _further_match_;
      r_unsent_match_ =
          std::min(size_remaining_,
                   std::max(0, ((t_size_partial_executed_ - size_executed_) / min_order_size_) * min_order_size_));

      // // Don't actually execute this order ,
      // // just get a size that would be executed.

      // if ( r_unsent_match_ > 0 )
      //   {
      //     size_executed_ += r_unsent_match_ ;
      //     size_remaining_ -= r_unsent_match_ ;
      //   }
    }

    return r_unsent_match_;
  }

  /// @brief called by SmartOrderManager to update placeinline
  /// and by HandleCrossingTrade which is called by PriceLevelSimMarketMaker
  inline void Enqueue(const int t_total_queue_size_) {
    if (num_events_seen_ == 0) {
      queue_size_behind_ = 0;
      queue_size_ahead_ = t_total_queue_size_;
      num_events_seen_ = 1;
    } else {
      if (queue_size_ahead_ > t_total_queue_size_) {
        queue_size_ahead_ = t_total_queue_size_;
      }
      queue_size_behind_ = t_total_queue_size_ - queue_size_ahead_;
      num_events_seen_++;
    }
  }

  inline void EnqueueLIFFETimeProRata(const int t_total_queue_size_) {  // called OnMarketUpdate()
    if (num_events_seen_ == 0) {
      queue_size_behind_ = 0;
      queue_size_ahead_ = t_total_queue_size_;
      num_events_seen_ = 1;
    } else {
      int t_trade_size_ = (queue_size_ahead_ + queue_size_behind_) - t_total_queue_size_;
      if (false && t_total_queue_size_ < (queue_size_ahead_ + queue_size_behind_)) {
        int t_trade_size_ = (queue_size_ahead_ + queue_size_behind_) - t_total_queue_size_;

        // double size_reduced_qA_ = std::min ( (double)queue_size_ahead_ , ( ((double)t_trade_size_ )  * ( double ) ( (
        // queue_size_ahead_ + queue_size_behind_ ) * ( queue_size_ahead_ + queue_size_behind_) - ( std::max ( 0,
        // queue_size_behind_ - queue_size_ahead_) * ( std::max ( 0, queue_size_behind_ - queue_size_ahead_)) ) ) /
        // (double)( (queue_size_ahead_ + queue_size_behind_) * (queue_size_ahead_ + queue_size_behind_) ) ) ) ;
        // double size_reduced_qB_ = std::min ( (double)queue_size_behind_ , ( ((double)t_trade_size_ )  * ( double ) (
        // queue_size_behind_ * queue_size_behind_  ) / (double)( (queue_size_ahead_ + queue_size_behind_) *
        // (queue_size_ahead_ + queue_size_behind_) ) ) ) ;

        // int removal_size_ahead_ = (int)( ((double)t_trade_size_) * ( ( double ) queue_size_ahead_
        // )*((double)queue_size_ahead_) * ( - ((double)queue_size_ahead_) + ( 2.00 * (double) queue_size_behind_ ) ) /
        // (double)( (queue_size_ahead_ + queue_size_behind_) * (queue_size_ahead_ + queue_size_behind_) ) ) ;
        int removal_size_ahead_ = (int)(((double)(queue_size_ahead_) * (float)(t_trade_size_)) /
                                        (double)(queue_size_ahead_ + queue_size_behind_));
        // int removal_size_ahead_ = ( int ) ( 0.3 * (float)(t_trade_size_) );
        queue_size_ahead_ = std::min(queue_size_ahead_, std::max(0, queue_size_ahead_ - int(removal_size_ahead_)));
        queue_size_ahead_ = std::min(queue_size_ahead_, t_total_queue_size_);
      }

      queue_size_behind_ = std::max(0, queue_size_behind_ - t_trade_size_);
      queue_size_ahead_ = std::max(0, t_total_queue_size_ - queue_size_behind_);
      num_events_seen_++;
    }
  }

  // New TimeProRata Algo for LFL and EuroSwiss
  inline void EnqueueLIFFENewTimeProRata(const int t_total_queue_size_,
                                         bool cancel_from_behind_ = false) {  // called OnMarketUpdate()
    if (num_events_seen_ == 0) {
      queue_size_behind_ = 0;
      queue_size_ahead_ = t_total_queue_size_;
      num_events_seen_ = 1;
    } else {
      if (t_total_queue_size_ < (queue_size_ahead_ + queue_size_behind_)) {
        if (cancel_from_behind_) {
          int t_trade_size_ = queue_size_ahead_ + queue_size_behind_ - t_total_queue_size_;
          if (t_trade_size_ > queue_size_behind_) {
            queue_size_behind_ = 0;
          }
          queue_size_behind_ -= (queue_size_ahead_ + queue_size_behind_ - t_total_queue_size_);
          queue_size_ahead_ = t_total_queue_size_ - queue_size_behind_;
        } else {
          int t_trade_size_ = (queue_size_ahead_ + queue_size_behind_) - t_total_queue_size_;

          int removal_size_ahead_ =
              (int)(((double)(t_trade_size_)) * ((double)(queue_size_ahead_ * queue_size_ahead_)) /
                    (double)((queue_size_ahead_ + queue_size_behind_) * (queue_size_ahead_ + queue_size_behind_)));
          // removal_size_ahead_ = (int) ( 0.3 * (float)(t_trade_size_) ) ;
          queue_size_ahead_ = std::min(queue_size_ahead_, std::max(0, queue_size_ahead_ - removal_size_ahead_));
          queue_size_ahead_ = std::min(queue_size_ahead_, t_total_queue_size_);
        }
      }
      queue_size_behind_ = t_total_queue_size_ - queue_size_ahead_;
      num_events_seen_++;
    }
  }

  /**
   * Change the size of orders based on simple pro-rata logic
   * @param t_total_queue_size_
   * @param priority_order_size_
   * @param cancel_from_behind_
   * @param priority_order_exist_
   */
  inline void EnqueueSimpleProRata(const int t_total_queue_size_, int& priority_order_size_,
                                   bool cancel_from_behind_ = false,
                                   bool priority_order_exist_ = false) {  // called OnMarketUpdate()

    if (num_events_seen_ == 0) {
      if (!t_total_queue_size_) {
        priority_order_ = true;
      }
      queue_size_behind_ = 0;
      queue_size_ahead_ = t_total_queue_size_;
      num_events_seen_ = 1;
    } else {
      if (t_total_queue_size_ < (queue_size_ahead_ + queue_size_behind_)) {
        // If size has decreased since last
        int t_trade_size_ = (queue_size_ahead_ + queue_size_behind_) - t_total_queue_size_;

        // If cancel from behind is enabled
        if (cancel_from_behind_) {
          if (queue_size_behind_ < t_trade_size_) {
            queue_size_ahead_ -= (t_trade_size_ - queue_size_behind_);
            queue_size_behind_ = 0;
          } else {
            queue_size_behind_ -= t_trade_size_;
            queue_size_ahead_ = t_total_queue_size_ - queue_size_behind_;
          }
        } else {
          // In CME trade message comes before quote
          // queue_sz < qsa + qsb => orders being removed and not trades
          // use heuristics to adjust qsa, qsb

          // pro-rated size reduction
          int removal_size_ahead_ =
              (int)std::floor(((double(queue_size_ahead_) / double(queue_size_ahead_ + queue_size_behind_)) *
                               (float)(t_trade_size_)));  // parametrize this
          // making it passive
          removal_size_ahead_ = 0;

          queue_size_ahead_ = std::min(queue_size_ahead_, std::max(0, queue_size_ahead_ - removal_size_ahead_));
          queue_size_ahead_ = std::min(queue_size_ahead_, t_total_queue_size_);
          queue_size_behind_ = t_total_queue_size_ - queue_size_ahead_;
        }
      } else {
        queue_size_behind_ = t_total_queue_size_ - queue_size_ahead_;
      }
      num_events_seen_++;
    }
  }

  inline void EnqueueSimpleProRata(const int t_trade_size_, const int t_total_queue_size_, const int my_fill_size_) {
    // called OnTradePrint()
    // ideally this should use pro-rata on all orders in qsa/qsb and then fifo for the remaining trade size
    if (num_events_seen_ == 0) {
      if (!t_total_queue_size_) {
        priority_order_ = true;
      }
      queue_size_behind_ = 0;
      queue_size_ahead_ = t_total_queue_size_;
      num_events_seen_ = 1;
    } else {
      double fill_percentage_ahead =
          ((double)queue_size_ahead_) / (double)((queue_size_ahead_ + queue_size_behind_ + size_remaining_));
      int removal_size_ahead_ = t_trade_size_ * fill_percentage_ahead;
      if (removal_size_ahead_ < 2) removal_size_ahead_ = 0;

      double fill_percentage_behind =
          ((double)queue_size_behind_) / (double)((queue_size_ahead_ + queue_size_behind_ + size_remaining_));
      int removal_size_behind_ = t_trade_size_ * fill_percentage_behind;
      if (removal_size_behind_ < 2) removal_size_behind_ = 0;

      removal_size_ahead_ = std::max(removal_size_ahead_, t_trade_size_ - (removal_size_behind_ + my_fill_size_));

      removal_size_ahead_ = 0;

      queue_size_ahead_ = std::min(queue_size_ahead_, std::max(0, queue_size_ahead_ - removal_size_ahead_));
      queue_size_ahead_ = std::min(queue_size_ahead_, t_total_queue_size_);
      queue_size_behind_ = t_total_queue_size_ - queue_size_ahead_;
      num_events_seen_++;
    }
  }

  inline void EnqueueLIFFETimeProRata(const int t_trade_size_, const int t_total_queue_size_) {
    // called OnTradePrint()
    if (num_events_seen_ == 0) {
      queue_size_behind_ = 0;
      queue_size_ahead_ = t_total_queue_size_;
      num_events_seen_ = 1;
    } else {
      //	  if ( t_total_queue_size_ < ( queue_size_ahead_ + queue_size_behind_ ) )
      //	  this if condition not required  due to 'mkt_update before trade' => total_que_size has already been
      // reduced => QSB was reduced => QSA was reduced only if QSB became 0
      //	  just reduce the QSA => QSB increases by same amount
      //	  this doesnt take care of rounding off (down) which gets allocated at top
      //	    {
      int removal_size_ahead_ =
          (int)(((double)t_trade_size_) * ((double)queue_size_ahead_) *
                (((double)queue_size_ahead_) + (2.00 * (double)queue_size_behind_)) /
                (double)((queue_size_ahead_ + queue_size_behind_) * (queue_size_ahead_ + queue_size_behind_)));
      queue_size_ahead_ = std::min(queue_size_ahead_, std::max(0, queue_size_ahead_ - removal_size_ahead_));
      queue_size_ahead_ = std::min(queue_size_ahead_, t_total_queue_size_);
      //	    }

      queue_size_behind_ = t_total_queue_size_ - queue_size_ahead_;
      num_events_seen_++;
    }
  }

  // New TimeProRata Algo for LFL and EuroSwiss
  inline void EnqueueLIFFENewTimeProRata(const int t_trade_size_, const int t_total_queue_size_) {
    // called OnTradePrint()
    if (num_events_seen_ == 0) {
      queue_size_behind_ = 0;
      queue_size_ahead_ = t_total_queue_size_;
      num_events_seen_ = 1;
    } else {
      //        if ( t_total_queue_size_ < ( queue_size_ahead_ + queue_size_behind_ ) )
      //        this if condition not required  due to 'mkt_update before trade' => total_que_size has already been
      //        reduced => QSB was reduced => QSA was reduced only if QSB became 0
      //        just reduce the QSA => QSB increases by same amount
      //        this doesnt take care of rounding off (down) which gets allocated at top
      //          {
      int removal_size_ahead_ = 0;
      if (queue_size_ahead_ + queue_size_behind_ > 0) {
        removal_size_ahead_ = (int)(((double)t_trade_size_) *
                                    (pow(queue_size_ahead_ + queue_size_behind_, 4) - pow(queue_size_behind_, 4)) /
                                    pow(queue_size_ahead_ + queue_size_behind_, 4));
      } else {
        removal_size_ahead_ = std::min(t_trade_size_, size_remaining_);
      }
      queue_size_ahead_ = std::min(queue_size_ahead_, std::max(0, queue_size_ahead_ - removal_size_ahead_));
      queue_size_ahead_ = std::min(queue_size_ahead_, t_total_queue_size_);
      //          }

      queue_size_behind_ = t_total_queue_size_ - queue_size_ahead_;
      num_events_seen_++;
    }
  }

  inline void SetAloneAtLevel() { queue_size_ahead_ = queue_size_behind_ = 0; }
  inline void SendToTop() {
    queue_size_behind_ += queue_size_ahead_;
    queue_size_ahead_ = 0;
  }
  inline void ResetQueue() { queue_size_ahead_ = queue_size_ahead_ + queue_size_behind_; }

  inline void Modify(const double _price_, const int _int_price_, const int _new_size_) {
    size_remaining_ = _new_size_;
    num_events_seen_ = 0;
    price_ = _price_;
    int_price_ = _int_price_;
  }

  /// changes the queue sizes and returns the size executed in this trade.
  /// @param t_trade_size_ the size of the trade in the market
  /// @param t_posttrade_size_at_price_ an estimate of the total_market_non_self_size at this level after this trade, we
  /// can use it to adjust queue_size_ahead_ and queue_size_behind_
  inline int HandleCrossingTrade(const int t_trade_size_, const int t_posttrade_size_at_price_) {
    // queue_size_ahead_ isn't ready if num_events_seen_ == 0
    if (num_events_seen_ < 1) return 0;
    int trade_size = t_trade_size_;
    if (ors_exec_size_ > 0) {
      // this trade update corresponds to a previously executed Real Order => ignore this update
      trade_size = 0;
    }
    ors_exec_size_ = std::max(0, ors_exec_size_ - t_trade_size_);

    if (trade_size > queue_size_ahead_) {
      // trade clears all orders ahead of this order
      int _further_match_ =
          (trade_size - queue_size_ahead_);  ///< remaining size after the ones above have been cleared
      int t_size_executed_ = MatchPartial(_further_match_);
      return t_size_executed_;  // returns the size executed in this trade
    } else {                    // trade does not clear all the orders ahead of this one.
      queue_size_ahead_ -= trade_size;
      Enqueue(t_posttrade_size_at_price_);  // since we have an estimate of the total_market_non_self_size at this level
                                            // after this trade, we use it to adjust queue_size_ahead_ and
                                            // queue_size_behind_
      return 0;
    }
    return 0;
  }

  /// This corresponds to LIFFE TimeProrataAlgorithm
  /// changes the queue sizes and returns the size executed in this trade.
  /// @param t_trade_size_ the size of the trade in the market
  /// @param t_posttrade_size_at_price_ an estimate of the total_market_non_self_size at this level after this trade, we
  /// can use it to adjust queue_size_ahead_ and queue_size_behind_
  inline int HandleCrossingTradeTimeProrataLIFFE(const int t_trade_size_, const int t_posttrade_size_at_price_,
                                                 int& priority_order_size_, bool priority_order_exists_ = false) {
    // queue_size_ahead_ isn't ready if num_events_seen_ == 0
    if (num_events_seen_ < 1) return 0;

    // Since this is LIFFE with quote before trade, we can expect that total_size_ has already taken the t_trade_size_
    // out

    double t_this_fractional_size_executed_ = 0;
    if (queue_size_behind_ + queue_size_ahead_ > 0) {
      t_this_fractional_size_executed_ =
          std::min((double)size_remaining_,
                   (((double)t_trade_size_) * ((double)size_remaining_) *
                    (-((double)size_remaining_) + (2.00 * (double)queue_size_behind_)) /
                    (double)((queue_size_ahead_ + queue_size_behind_) * (queue_size_ahead_ + queue_size_behind_))));
    } else {
      t_this_fractional_size_executed_ = std::min(t_trade_size_, size_remaining_);
    }

    double size_reduced_qA_ =
        std::min((double)queue_size_ahead_,
                 (((double)t_trade_size_) *
                  (double)((queue_size_ahead_ + queue_size_behind_) * (queue_size_ahead_ + queue_size_behind_) -
                           (std::max(0, queue_size_behind_ - queue_size_ahead_) *
                            (std::max(0, queue_size_behind_ - queue_size_ahead_)))) /
                  (double)((queue_size_ahead_ + queue_size_behind_) * (queue_size_ahead_ + queue_size_behind_))));
    double size_reduced_qB_ =
        std::min((double)queue_size_behind_,
                 (((double)t_trade_size_ * queue_size_behind_ * queue_size_behind_) /
                  (double)((queue_size_ahead_ + queue_size_behind_) * (queue_size_ahead_ + queue_size_behind_))));

    if ((double(queue_size_ahead_ * queue_size_ahead_) /
         double((queue_size_ahead_ + queue_size_behind_) * (queue_size_ahead_ + queue_size_behind_))) >= 0.60) {
      size_reduced_qB_ = 0;
    }
    // t_this_fractional_size_executed_ = std::min ( ( double ) size_remaining_ , ( ( double ) t_trade_size_ ) * ( 2 *
    // (size_remaining_ ) ) / double ( queue_size_ahead_ + queue_size_behind_ ) ) ;
    // t_this_fractional_size_executed_ = std::max ( t_this_fractional_size_executed_, ( t_trade_size_ -
    // size_reduced_qA_ - size_reduced_qB_) );

    size_fractional_executed_ = std::min(
        (double)size_remaining_,
        std::max(
            (double)std::min((double(t_trade_size_) - std::ceil(size_reduced_qA_) - std::ceil(size_reduced_qB_)), 1.00),
            t_this_fractional_size_executed_));
    // size_fractional_executed_ = t_this_fractional_size_executed_ ;
    int t_this_size_executed_ = 0;
#define MIN_FRACTIONAL_SIZE_TO_EXECUTE 0.99
    if (size_fractional_executed_ > MIN_FRACTIONAL_SIZE_TO_EXECUTE) {
      int _further_match_ =
          std::min(size_remaining_, ((size_fractional_executed_ <= 1.00) ? 1 : (int)(size_fractional_executed_)));
      t_this_size_executed_ = MatchPartial(_further_match_);
      size_fractional_executed_ = std::max(0.00, size_fractional_executed_ - t_this_size_executed_);
    }
#undef MIN_FRACTIONAL_SIZE_TO_EXECUTE
    EnqueueLIFFETimeProRata(t_trade_size_, t_posttrade_size_at_price_);
    return t_this_size_executed_;
  }

  // New ProRata Algo for LFL and EuroSwiss
  inline int HandleCrossingTradeNewTimeProrataLIFFE(const int t_trade_size_, const int t_posttrade_size_at_price_,
                                                    int& priority_order_size_, bool priority_order_exists_ = false) {
    // queue_size_ahead_ isn't ready if num_events_seen_ == 0
    if (num_events_seen_ < 1) return 0;

    // Since this is LIFFE with quote before trade, we can expect that total_size_ has already taken the t_trade_size_
    // out

    if (priority_order_exists_) {
      if (priority_order_size_ > t_trade_size_) {
        priority_order_size_ -= t_trade_size_;
        queue_size_ahead_ -= t_trade_size_;
        if (queue_size_ahead_ < 0) {
          queue_size_ahead_ = 0;
        }
      } else {
        priority_order_size_ = 0;
        queue_size_ahead_ -= t_trade_size_;
        if (queue_size_ahead_ < 0) {
          queue_size_ahead_ = 0;
        }
      }
    }

    double t_this_fractional_size_executed_ = 0;
    if (queue_size_behind_ + queue_size_ahead_ > 0) {
      t_this_fractional_size_executed_ =
          std::min((double)size_remaining_,
                   (((double)t_trade_size_) *
                    (pow(queue_size_behind_, 4) - pow(std::max(0, queue_size_behind_ - size_remaining_), 4)) /
                    pow(queue_size_ahead_ + queue_size_behind_, 4)));
    } else {
      t_this_fractional_size_executed_ = std::min(t_trade_size_, size_remaining_);
    }

    size_fractional_executed_ =
        std::min((double)size_remaining_,
                 t_this_fractional_size_executed_);  // redundant always will be t_this_fractional_size_executed_
    // double size_reduced_qA_ = std::min (  ( double ) queue_size_ahead_ ,
    //                                    t_trade_size_ * ( pow ( queue_size_behind_ , 4 ) - pow ( queue_size_behind_ -
    //                                    queue_size_ahead_, 4 ) )/ ( pow ( queue_size_behind_, 4 ) ) )  ;
    // double size_reduced_qB_ = std::min ( ( double ) queue_size_behind_ ,
    //                                    t_trade_size_ * ( ( double ) pow ( queue_size_behind_ , 4 )/ ( ( double ) pow
    //                                    ( queue_size_ahead_ + queue_size_behind_, 4 ) ) ) ) ;

    // size_fractional_executed_ = std::min ( ( double ) size_remaining_, std::max ( ( double ) std::min ( ( double (
    // t_trade_size_) - std::ceil ( size_reduced_qA_) - std::ceil ( size_reduced_qB_ ) ), 1.00 ),
    // t_this_fractional_size_executed_ ));

    int t_this_size_executed_ = 0;
#define MIN_FRACTIONAL_SIZE_TO_EXECUTE 0.99
    if (size_fractional_executed_ > MIN_FRACTIONAL_SIZE_TO_EXECUTE) {
      int _further_match_ =
          std::min(size_remaining_, ((size_fractional_executed_ <= 1.00) ? 1 : (int)(size_fractional_executed_)));
      t_this_size_executed_ = MatchPartial(_further_match_);
      size_fractional_executed_ = std::max(0.00, size_fractional_executed_ - t_this_size_executed_);
    }
#undef MIN_FRACTIONAL_SIZE_TO_EXECUTE
    EnqueueLIFFENewTimeProRata(t_trade_size_, t_posttrade_size_at_price_);
    return t_this_size_executed_;
  }

  inline int HandleCrossingTradeSimpleProrata(const int t_trade_size_, const int t_posttrade_size_at_price_,
                                              int& priority_order_size_, bool priority_order_exists_ = false) {
    // queue_size_ahead_ isn't ready if num_events_seen_ == 0
    if (num_events_seen_ < 1) return 0;

    if (priority_order_exists_) {
      if (priority_order_size_ > t_trade_size_) {
        priority_order_size_ -= t_trade_size_;
        queue_size_ahead_ -= t_trade_size_;
        if (queue_size_ahead_ < 0) {
          queue_size_ahead_ = 0;
        }
      } else {
        priority_order_size_ = 0;
        queue_size_ahead_ -= t_trade_size_;
        if (queue_size_ahead_ < 0) {
          queue_size_ahead_ = 0;
        }
      }
    }
    int my_fill_size = 0;
    // fill priority order, use pro-rata with min 2 fill, remaining FIFO
    if (priority_order_) {
      my_fill_size = std::min(t_trade_size_, size_remaining_);
    } else {
      double my_fill_percentage =
          (double)(size_remaining_) / (double)(queue_size_ahead_ + queue_size_behind_ + size_remaining_);
      my_fill_size = my_fill_percentage * t_trade_size_;  // had to be rounded-down to int anyways
      if (my_fill_size < 2) my_fill_size = 0;
      my_fill_size = std::min(my_fill_size, size_remaining_);
    }

    // std::cerr << " my fill: " << my_fill_size <<  " total : " << t_trade_size_ << " totalsize: " <<
    // t_posttrade_size_at_price_ << " ";

    double qA_fill_percentage_ = (double)(queue_size_ahead_) / (double)(queue_size_ahead_ + queue_size_behind_);
    double qB_fill_percentage_ =
        (double)(queue_size_behind_) / (double)(queue_size_ahead_ + queue_size_behind_ + size_remaining_);
    int qA_fill_ = qA_fill_percentage_ * t_trade_size_;
    if (qA_fill_ < 2) {
      qA_fill_ = 0;
    }
    if (qA_fill_ > queue_size_ahead_) {
      qA_fill_ = queue_size_ahead_;
    }
    int qB_fill_ = qB_fill_percentage_ * t_trade_size_;
    if (qB_fill_ < 2) {
      qB_fill_ = 0;
    }
    if (qB_fill_ > queue_size_behind_) {
      qB_fill_ = queue_size_behind_;
    }

    // std::cerr << " qa_fill: " << qA_fill_ << " qb_fill: " << qB_fill_ << " my_fill_size : " << my_fill_size << " [ "
    // << queue_size_ahead_ << " " << queue_size_behind_ << " ] \n" ;
    int remaining_size_ = t_trade_size_ - qA_fill_ - qB_fill_;
    if (remaining_size_ > 0 && my_fill_size == 0) {
      int fifo_fill_ = (remaining_size_ - (queue_size_ahead_ - qA_fill_));
      if (fifo_fill_ > 0) {
        my_fill_size += (fifo_fill_);
      }
    }
    if (my_fill_size > size_remaining_) {
      my_fill_size = size_remaining_;
    }
    size_executed_ += my_fill_size;
    size_remaining_ -= my_fill_size;

    EnqueueSimpleProRata(t_trade_size_, t_posttrade_size_at_price_, my_fill_size);
    return my_fill_size;
  }

  /// See if a subsequent call to HandleCrossing trade changes the queue sizes and
  /// returns the size which would be executed in that trade.
  /// @param t_trade_size_ the size of the trade in the market
  /// @param t_posttrade_size_at_price_ an estimate of the total_market_non_self_size at this level after this trade, we
  /// can use it to adjust queue_size_ahead_ and queue_size_behind_
  inline int SimulateHandleCrossingTrade(const int t_trade_size_, const int t_posttrade_size_at_price_) const {
    // queue_size_ahead_ isn't ready if num_events_seen_ == 0
    if (num_events_seen_ < 1) return 0;

    if (t_trade_size_ > queue_size_ahead_) {  // trade clears all orders ahead of this order
      int _further_match_ =
          (t_trade_size_ - queue_size_ahead_);  ///< remaining size after the ones above have been cleared
      int t_size_executed_ = SimulateMatchPartial(_further_match_);
      return t_size_executed_;  // returns the size which would be executed in this trade
    }

    // trade does not clear all the orders ahead of this one.
    return 0;
  }

  inline int HandleCrossingTradeSplitFIFOProRata(const int t_trade_size_, const int t_posttrade_size_at_price_,
                                                 const double fifo_matching_fraction_) {
    if (num_events_seen_ < 1) {
      return 0;
    }
    double avg_order_size_ = 0;
    if (queue_orders_ahead_ + queue_orders_behind_ > 0) {
      avg_order_size_ =
          double(queue_size_ahead_ + queue_size_behind_) / double(queue_orders_ahead_ + queue_orders_behind_);
    }

    int fifo_executed_size_ = std::ceil(((double)t_trade_size_ * fifo_matching_fraction_));
    int pro_rata_executed_size_ = t_trade_size_ - fifo_executed_size_;
    int _further_match_ = 0;
    // fifo execution
    // std::cerr << " pro_rata_ " << pro_rata_executed_size_ << " [ " << queue_orders_ahead_ <<" "<< queue_size_ahead_
    // << " " << queue_size_behind_ <<" "<< queue_orders_behind_ <<"  ] ";
    if (queue_size_ahead_ < fifo_executed_size_) {
      _further_match_ = (fifo_executed_size_ - queue_size_ahead_);
      if (size_remaining_ > _further_match_) {
        _further_match_ = MatchPartial(_further_match_);
        queue_size_behind_ -= std::max(0, fifo_executed_size_ - queue_size_ahead_);
      } else {
        _further_match_ = ExecuteRemaining();
      }
      queue_size_ahead_ = 0;
    } else {
      queue_size_ahead_ -= fifo_executed_size_;
    }

    bool already_filled_ = false;
    queue_orders_ahead_ = int((double)queue_size_ahead_ / avg_order_size_);
    // pro rata execution
    int my_size_executed_ =
        int(double(pro_rata_executed_size_ * size_remaining_) / (double)(queue_size_ahead_ + queue_size_behind_));

    // std::cerr << " our_size_ " << my_size_executed_ << " [ " << queue_orders_ahead_ << " " << queue_size_ahead_ << "
    // " << queue_size_behind_ << " " << queue_size_behind_ << "  ] ";

    if (my_size_executed_ >= size_remaining_) {
      _further_match_ += ExecuteRemaining();
    } else {
      _further_match_ += MatchPartial(my_size_executed_);
    }
    if (my_size_executed_ > 0) {
      already_filled_ = true;
    }
    int reduction_per_order_ =
        int(double(pro_rata_executed_size_ * avg_order_size_) / (double)(queue_size_ahead_ + queue_size_behind_));
    int reduction_queue_size_ahead_ =
        int(double(pro_rata_executed_size_ * queue_size_ahead_) / (double)(queue_size_ahead_ + queue_size_behind_));
    // std::cerr << " rpo: " << reduction_per_order_ << " " << reduction_queue_size_ahead_ << " " << queue_orders_ahead_
    // << " our exec: " << _further_match_ << std::endl;
    if ((reduction_per_order_ * queue_orders_ahead_ + queue_orders_ahead_ < reduction_queue_size_ahead_) &&
        !already_filled_) {
      _further_match_ += MatchPartial(1);
    }

    int reduction_queue_size_behind_ =
        int(double(pro_rata_executed_size_ * queue_size_behind_) / (double)(queue_size_ahead_ + queue_size_behind_));
    if (reduction_queue_size_ahead_ + reduction_queue_size_behind_ < t_trade_size_) {
      reduction_queue_size_ahead_ +=
          (pro_rata_executed_size_ - reduction_queue_size_behind_ - reduction_queue_size_ahead_);
    }
    // std::cerr << " qA red: " << reduction_queue_size_ahead_ << " [ " << queue_orders_ahead_ << " " <<
    // queue_size_ahead_ << " " << queue_size_behind_ << " "<< queue_orders_behind_<< "  ] " << reduction_per_order_ <<
    // " \n";

    queue_size_ahead_ = std::min(queue_size_ahead_, std::max(0, queue_size_ahead_ - reduction_queue_size_ahead_));
    queue_size_behind_ = t_posttrade_size_at_price_ - queue_size_ahead_;
    num_events_seen_++;
    return _further_match_;
  }

  void EnqueueSplitFIFOProRata(const int t_new_size_, const int t_new_ordercount_, const double fifo_matching_fraction_,
                               bool cancel_from_behind_ = true) {
    // std::cerr << "book [ " << queue_orders_ahead_ << " " << queue_size_ahead_ << " " << queue_size_behind_ << " "<<
    // queue_orders_behind_  << " ] " << t_new_ordercount_ ;
    int prev_size_ = queue_size_ahead_ + queue_size_behind_;
    // int t_queue_size_ahead_ = queue_size_ahead_;
    int size_reduction_queue_size_ahead_ = 0;

    if (prev_size_ > t_new_size_) {
      int t_del_size_ = prev_size_ - t_new_size_;
      // just using trade logic, change later based on how sim-real is
      // int fifo_executed_size_ = std::ceil ( ( ( double ) t_del_size_ ) * fifo_matching_fraction_ ) ;
      // queue_size_ahead_ -= fifo_executed_size_ ;
      // if ( queue_size_ahead_ < 0 ) { queue_size_ahead_ = 0; queue_size_behind_ -= ( fifo_executed_size_ -
      // t_queue_size_ahead_ ) ;}
      // fifo_executed_size_ = 0;
      // int pro_rata_executed_size_ = t_del_size_ - fifo_executed_size_ ;
      size_reduction_queue_size_ahead_ =
          int(double(t_del_size_ * queue_size_ahead_) / (double)(queue_size_behind_ + queue_size_ahead_));
      if (cancel_from_behind_) {
        queue_size_behind_ -= (prev_size_ - t_new_size_);
        if (queue_size_behind_ < 0) {
          queue_size_behind_ = 0;
        }
        queue_size_ahead_ = std::max(0, t_new_size_ - queue_size_behind_);
      } else {
        if (t_new_ordercount_ - queue_orders_ahead_ - queue_orders_behind_ == 1) {
          queue_size_ahead_ -= (prev_size_ - t_new_size_);
        } else {
          queue_size_ahead_ -= (size_reduction_queue_size_ahead_);
        }

        if (queue_size_ahead_ < 0) {
          queue_size_ahead_ = 0;
        }
        queue_size_behind_ = std::max(0, t_new_size_ - queue_size_ahead_);
      }
      if (queue_size_behind_ == 0) {
        queue_orders_behind_ = 0;
        queue_orders_ahead_ = t_new_ordercount_;
      } else {
        if (size_reduction_queue_size_ahead_ != 0) {
          queue_orders_ahead_ -= std::ceil((double)(size_reduction_queue_size_ahead_ * t_new_ordercount_) /
                                           ((double)(queue_size_behind_ + queue_size_ahead_)));
        }

        queue_orders_behind_ =
            std::max(queue_orders_behind_, int(std::ceil((double)(queue_size_behind_ * t_new_ordercount_) /
                                                         ((double)(queue_size_behind_ + queue_size_ahead_)))));
        queue_orders_behind_ = t_new_ordercount_ - queue_orders_ahead_;
      }

      // queue_size_ahead_ = std::min ( queue_size_ahead_, std::max ( 0, queue_size_ahead_ -
      // size_reduction_queue_size_ahead_ ) ) ;
      num_events_seen_++;
    } else {
      if (queue_size_behind_ == 0) {
        queue_size_behind_ = 1;  // hack , some how in some orders this functions is not called to set seizes needs fix
      } else {
        queue_size_behind_ = t_new_size_ - queue_size_ahead_;
      }
      queue_orders_ahead_ = t_new_ordercount_ - queue_orders_behind_;
      num_events_seen_++;
    }
    // std::cerr << " book [ " << queue_orders_ahead_ << " " << queue_size_ahead_ << " " << queue_size_behind_ << " "<<
    // queue_orders_behind_  << " ] " << t_new_ordercount_ << "\n";
  }

  inline const char* security_name() const { return security_name_; }
  inline TradeType_t buysell() const { return buysell_; }
  inline double price() const { return price_; }
  inline int size_remaining() const { return size_remaining_; }
  inline int int_price() const { return int_price_; }
  inline ORRType_t order_status() const { return order_status_; }
  inline int queue_size_ahead() const { return queue_size_ahead_; }
  inline int queue_size_behind() const { return queue_size_behind_; }
  inline int num_events_seen() const { return num_events_seen_; }
  inline int client_assigned_order_sequence() const { return client_assigned_order_sequence_; }
  inline int server_assigned_order_sequence() const { return server_assigned_order_sequence_; }
  inline int server_assigned_client_id() const { return server_assigned_client_id_; }
  inline int size_executed() const { return size_executed_; }
  inline bool alone_above_best_market() const { return alone_above_best_market_; }
  inline std::string ToString() {
    std::stringstream st;
    st << " SimOrder: " << security_name_ << " " << GetTradeTypeChar(buysell_) << " " << price_ << " " << int_price_
       << " " << size_remaining_ << " " << order_status_ << " [ " << queue_size_ahead_ << " " << queue_size_behind_
       << " " << queue_orders_ahead_ << " " << queue_orders_behind_ << " " << num_events_seen_ << " oid: " << order_id_
       << " CAOS: " << client_assigned_order_sequence_ << " SAOS: " << server_assigned_order_sequence_
       << " SACI:" << server_assigned_client_id_ << " " << min_order_size_ << " " << size_partial_executed_ << " "
       << size_executed_ << " " << size_fractional_executed_ << " " << alone_above_best_market_ << " "
       << order_sequenced_time_.ToString() << " " << order_confirmation_time_.ToString() << " "
       << order_entry_time_.ToString() << " " << correct_update_time_ << " " << priority_order_;
    return st.str();
  }
};

typedef std::vector<BaseSimOrder*> BaseSimOrderPtrVec;
typedef std::map<int, std::vector<BaseSimOrder*>, std::greater<int> > BidPriceSimOrderMap;
typedef std::map<int, std::vector<BaseSimOrder*>, std::greater<int> >::iterator BidPriceSimOrderMapIter_t;
typedef std::map<int, std::vector<BaseSimOrder*>, std::greater<int> >::const_iterator BidPriceSimOrderMapConstIter_t;
typedef std::map<int, std::vector<BaseSimOrder*>, std::greater<int> >::reverse_iterator BidPriceSimOrderMapRevIter_t;
typedef std::map<int, std::vector<BaseSimOrder*>, std::less<int> > AskPriceSimOrderMap;
typedef std::map<int, std::vector<BaseSimOrder*>, std::less<int> >::iterator AskPriceSimOrderMapIter_t;
typedef std::map<int, std::vector<BaseSimOrder*>, std::less<int> >::const_iterator AskPriceSimOrderMapConstIter_t;
typedef std::map<int, std::vector<BaseSimOrder*>, std::less<int> >::reverse_iterator AskPriceSimOrderMapRevIter_t;
}

#endif  // BASE_ORDERROUTING_BASE_SIM_ORDER_H
