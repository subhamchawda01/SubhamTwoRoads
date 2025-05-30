/**
   \file OrderRoutingCode/base_order_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include <string.h>

#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"

#include "baseinfra/OrderRouting/base_order_manager.hpp"

namespace HFSAT {

BaseOrderManager::BaseOrderManager(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                   SecurityNameIndexer& t_sec_name_indexer_, BaseTrader& _base_trader_,
                                   const std::string& _dep_shortcode_, const unsigned int _dep_security_id_,
                                   const char* _dep_symbol_, const double _min_price_increment_,
                                   int _first_client_assigned_order_sequence_)
    : dbglogger_(t_dbglogger_),
      watch_(r_watch_),
      sec_name_indexer_(t_sec_name_indexer_),
      base_trader_(_base_trader_),
      dep_shortcode_(_dep_shortcode_),
      dep_security_id_(_dep_security_id_),
      dep_symbol_(_dep_symbol_),
      fast_price_convertor_(_min_price_increment_),
      server_assigned_client_id_(_base_trader_.GetClientId()),
      place_cancel_cooloff_(0),
      client_assigned_order_sequence_(_first_client_assigned_order_sequence_),
      no_new_order_till(0),
      if_reject_set(false),
      baseorder_mempool_(),
      ORDER_MANAGER_INT_PRICE_RANGE(2048),
      initial_adjustment_set_(false),
      bid_int_price_adjustment_(0),
      ask_int_price_adjustment_(0),
      order_vec_top_bid_index_(-1),
      order_vec_bottom_bid_index_(-1),
      confirmed_top_bid_index_(-1),
      confirmed_bottom_bid_index_(-1),
      unconfirmed_top_bid_index_(-1),
      unconfirmed_bottom_bid_index_(-1),
      order_vec_top_ask_index_(-1),
      order_vec_bottom_ask_index_(-1),
      confirmed_top_ask_index_(-1),
      confirmed_bottom_ask_index_(-1),
      unconfirmed_top_ask_index_(-1),
      unconfirmed_bottom_ask_index_(-1),
      unsequenced_bids_(),
      unsequenced_asks_(),
      external_cancel_all_outstanding_orders_(false),
      num_unconfirmed_orders_(0),
      foked_bid_order_size_sum_(0),
      foked_ask_order_size_sum_(0),
      s_p_position_change_listener_(nullptr),
      position_change_listener_vec_(),
      s_p_execution_listener_(nullptr),
      execution_listener_vec_(),
      s_p_order_change_listener_(nullptr),
      order_change_listener_vec_(),
      s_p_cxl_reject_listener_(nullptr),
      cxl_reject_listener_vec_(),
      s_p_reject_funds_listener_(nullptr),
      reject_due_to_funds_listener_vec_(),
      s_p_fok_fill_reject_listener_(nullptr),
      fok_fill_reject_listener_vec_(),
      client_position_(0),
      global_position_(0),
      position_offset_(0),
      map_clean_counter_(0),
      sum_bid_sizes_(0),
      sum_ask_sizes_(0),
      num_self_trades_(0),
      trade_volume_(0),
      last_maps_cleaned_msecs_(0),
      last_top_replay_msecs_(0),
      queue_sizes_needed_(false),
      best_bid_int_price_(0),
      best_ask_int_price_(0),
      supporting_order_filled_(0u),
      best_level_order_filled_(0u),
      aggressive_order_filled_(0u),
      improve_order_filled_(0u),
      total_size_placed_(0),
      send_order_count_(0),
      cxl_order_count_(0),
      modify_order_count_(0),
      is_cancellable_before_confirmation(GetIsCancellableBeforeConfirmation()),
      security_id_to_last_position_(sec_name_indexer_.NumSecurityId(), kInvalidPosition),
      security_position_(0),  // To maintain positions for all contracts of this security.
      p_ticks_to_keep_bid_int_price_(nullptr),
      p_ticks_to_keep_ask_int_price_(nullptr),
      throttle_manager_(nullptr),
      expected_message_sequence_(1),  // We expect packet sequence from 1 onwards
      dropped_sequences_(),
      last_drop_detection_check_time_(0),
      disclosed_size_factor_(1.0),
      set_disclosed_size_(false),
      modify_partial_exec_(true),
      number_of_consecutive_exchange_rejects_(0),
      last_reject_evaluation_time_(0),
      time_based_total_exchange_rejects_(0),
      total_exchange_rejects_(0),
      reject_based_freeze_timer_(0),
      query_trading_auto_freeze_manager_(HFSAT::BaseUtils::QueryTradingAutoFreezeManager::GetUniqueInstance()),
      is_auto_freeze_active_(true),
      last_sent_message_mfm_(0),
      received_after_last_sent_(false),
      last_notify_time_(0),
      cancel_order_seq_time_(),
      cancel_order_exec_time_(),
      top_modifiable_ask_index_(-1),
      bottom_modifiable_ask_index_(-1),
      top_modifiable_bid_index_(-1),
      bottom_modifiable_bid_index_(-1),
      use_modify_logic_(false) {
  bid_order_vec_.resize(ORDER_MANAGER_INT_PRICE_RANGE, std::vector<BaseOrder*>());
  sum_bid_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
  sum_bid_confirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
  sum_bid_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
  sum_bid_unconfirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
  sum_total_bid_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
  sum_total_bid_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);

  ask_order_vec_.resize(ORDER_MANAGER_INT_PRICE_RANGE, std::vector<BaseOrder*>());
  sum_ask_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
  sum_ask_confirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
  sum_ask_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
  sum_ask_unconfirmed_orders_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
  sum_total_ask_confirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);
  sum_total_ask_unconfirmed_.resize(ORDER_MANAGER_INT_PRICE_RANGE, 0);

  InitializeAggVariables();
  DumpAggVariables();
  last_cancel_caos = -1;
  watch_.subscribe_BigTimePeriod(this);
  modify_partial_exec_ = SecurityDefinitions::ModifyAfterPartialExecAllowed(dep_shortcode_, r_watch_.YYYYMMDD());
}

BaseOrder* BaseOrderManager::GetTopBidOrder() {
  BaseOrder* p_top_order_ = nullptr;

  if (!unsequenced_bids_.empty()) {  // first check unseuqenced orders
    p_top_order_ = unsequenced_bids_[0];
    for (unsigned int i = 1; i < unsequenced_bids_.size(); i++) {
      if (p_top_order_->price() < unsequenced_bids_[i]->price()) {
        // new top order .... also could have worked with int_price
        p_top_order_ = unsequenced_bids_[i];
      }
    }

    int _current_top_int_px_ = p_top_order_->int_price();

    if (order_vec_top_bid_index_ != -1) {
      for (int index_ = order_vec_top_bid_index_; index_ >= order_vec_bottom_bid_index_; index_--) {
        if (GetBidIntPrice(index_) < _current_top_int_px_)  // TODO: replace with index_ comparison
        {  // key being lower that _current_top_int_px_ means not best levels
          break;
        }

        const std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];
        if ((!_this_base_order_vec_.empty()) && (_this_base_order_vec_[0] != nullptr)) {
          p_top_order_ = _this_base_order_vec_[0];
          break;
        }
      }
    }
  } else {
    if (order_vec_top_bid_index_ != -1) {
      for (int index_ = order_vec_top_bid_index_; index_ >= order_vec_bottom_bid_index_; index_--) {
        const std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];
        if ((!_this_base_order_vec_.empty()) &&
            (_this_base_order_vec_[0] !=
             nullptr)) {  // any order at this level is a valid answer since the maps is sorted
          p_top_order_ = _this_base_order_vec_[0];
          break;
        }
      }
    }
  }

  return p_top_order_;
}

BaseOrder* BaseOrderManager::GetTopConfirmedBidOrder() {
  BaseOrder* p_top_order_ = nullptr;

  if (order_vec_top_bid_index_ != -1) {
    for (int index_ = order_vec_top_bid_index_; index_ >= order_vec_bottom_bid_index_; index_--) {
      const std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];
      if ((!_this_base_order_vec_.empty()) && (_this_base_order_vec_[0] != nullptr) &&
          _this_base_order_vec_[0]->order_status_ ==
              kORRType_Conf) {  // any order at this level is a valid answer since the maps is sorted
        p_top_order_ = _this_base_order_vec_[0];
        break;
      }
    }
  }

  return p_top_order_;
}

BaseOrder* BaseOrderManager::GetTopAskOrder() {
  BaseOrder* p_top_order_ = nullptr;

  if (!unsequenced_asks_.empty()) {  // first check unconfirmed orders
    p_top_order_ = unsequenced_asks_[0];
    for (unsigned int i = 1; i < unsequenced_asks_.size(); i++) {
      if (p_top_order_->price() >
          unsequenced_asks_[i]->price()) {  // new top order .... also could have worked with int_price
        p_top_order_ = unsequenced_asks_[i];
      }
    }

    int _current_top_int_px_ = p_top_order_->int_price();

    if (order_vec_top_ask_index_ != -1) {
      for (int index_ = order_vec_top_ask_index_; index_ >= order_vec_bottom_ask_index_; index_--) {
        if (GetAskIntPrice(index_) > _current_top_int_px_) {
          break;
        }

        const std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[index_];

        if ((!_this_base_order_vec_.empty()) && (_this_base_order_vec_[0] != nullptr)) {
          p_top_order_ = _this_base_order_vec_[0];
          break;
        }
      }
    }
  } else {
    if (order_vec_top_ask_index_ != -1) {
      for (int index_ = order_vec_top_ask_index_; index_ >= order_vec_bottom_ask_index_; index_--) {
        const std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[index_];
        if ((!_this_base_order_vec_.empty()) && (_this_base_order_vec_[0] != nullptr)) {
          p_top_order_ = _this_base_order_vec_[0];
          break;
        }
      }
    }
  }
  return p_top_order_;
}

BaseOrder* BaseOrderManager::GetTopConfirmedAskOrder() {
  BaseOrder* p_top_order_ = nullptr;

  if (order_vec_top_ask_index_ != -1) {
    for (int index_ = order_vec_top_ask_index_; index_ >= order_vec_bottom_ask_index_; index_--) {
      const std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[index_];
      if ((!_this_base_order_vec_.empty()) && (_this_base_order_vec_[0] != nullptr) &&
          _this_base_order_vec_[0]->order_status_ == kORRType_Conf) {
        p_top_order_ = _this_base_order_vec_[0];
        break;
      }
    }
  }

  return p_top_order_;
}

void BaseOrderManager::GoForPacketRecovery(const int32_t expected_message_sequence,
                                           const int32_t current_received_sequence) {
  dbglogger_ << " Expected Message Sequence : " << expected_message_sequence
             << " Received : " << current_received_sequence << "\n";
  dbglogger_.DumpCurrentBuffer();

  // Well we could have dropped many packets, time to recover
  for (int32_t missing_packets_seq = expected_message_sequence; missing_packets_seq < current_received_sequence;
       missing_packets_seq++) {
    // We'll need to keep track of these requests
    dropped_sequences_.insert(missing_packets_seq);
    base_trader_.SendRecoveryRequest(missing_packets_seq, server_assigned_client_id_, dropped_sequences_);
  }
  // Update last drop detection time, so a timely call for drop detection and recovery doesn't unnecessarily
  // raise a recovery request
  last_drop_detection_check_time_ = watch_.msecs_from_midnight();
}

void BaseOrderManager::TimelyDropDetectionAndRecovery() {
  // Check if we haven't send any recovery request recently and now it's time to resend request for the sequence
  // we still haven't recovered
  if (watch_.msecs_from_midnight() - last_drop_detection_check_time_ >= TIME_TO_CHECK_FOR_DROPS) {
    last_drop_detection_check_time_ = watch_.msecs_from_midnight();

    // We have some sequence yet not recovered,
    // TODO probably we should set alerts if we are just re-requesting things over and again
    // something must be wrong in that scenario
    if (dropped_sequences_.size() > 0) {
      for (std::set<int32_t>::iterator itr = dropped_sequences_.begin(); itr != dropped_sequences_.end(); itr++) {
        GoForPacketRecovery(*itr, *itr + 1);
      }
    }
  }

  // We'll check again on the next time frame now
  last_drop_detection_check_time_ = watch_.msecs_from_midnight();
}

void BaseOrderManager::AddPosition(int _offset_to_add_, const double _price_) {
  /*
   * Add External Position to the order manage
   * Moving it from base_trading, no other exec-logic should implement this separately
   */

  position_offset_ += _offset_to_add_;
  NotifyExecAndPosListeners(_offset_to_add_, _price_, GetIntPx(_price_));
}

bool BaseOrderManager::NotifyExecAndPosListeners(int _position_diff_, const double _trade_price_,
                                                 const int _int_price_) {
  // For notifying a trade to the listeners(BasePnl, ExecLogic etc.)
  // client_position_ & position_offset_ should be updated before calling this

  if (_position_diff_ == 0) {
    return false;
  }
  TradeType_t _implied_buysell_ = (_position_diff_ > 0) ? (kTradeTypeBuy) : (kTradeTypeSell);
  int exec_qty_ = abs(_position_diff_);
  int net_position_ = client_position_ + position_offset_;

  trade_volume_ += exec_qty_;

  // NotifyExecutionListeners
  for (auto i = 0u; i < execution_listener_vec_.size(); i++) {
    execution_listener_vec_[i]->OnExec(net_position_, exec_qty_, _implied_buysell_, _trade_price_, _int_price_,
                                       dep_security_id_);
  }

  if (s_p_execution_listener_ != nullptr) {
    s_p_execution_listener_->OnExec(net_position_, exec_qty_, _implied_buysell_, _trade_price_, _int_price_,
                                    dep_security_id_);
  }

  // NotifyPositionChangeListeners
  if (s_p_position_change_listener_ != nullptr) {
    s_p_position_change_listener_->OnPositionChange(net_position_, exec_qty_, dep_security_id_);
  }

  for (std::vector<PositionChangeListener*>::iterator pciter = position_change_listener_vec_.begin();
       pciter != position_change_listener_vec_.end(); pciter++) {
    (*pciter)->OnPositionChange(net_position_, exec_qty_, dep_security_id_);
  }

  return true;
}

// Don't want to reply on next sending orders etc request or ORS replies,
// rather want to make code robust by adding timely update callbacks
void BaseOrderManager::OnTimePeriodUpdate(const int32_t num_pages_to_add) {
  TimelyDropDetectionAndRecovery();

  // Rejects-Freeze handling
  if (watch_.msecs_from_midnight() - last_reject_evaluation_time_ >
      query_trading_auto_freeze_manager_.GetTimeBasedRejectsDuration()) {
    last_reject_evaluation_time_ = watch_.msecs_from_midnight();
    time_based_total_exchange_rejects_ = 0;
  }

  if (reject_based_freeze_timer_ != 0 && (watch_.msecs_from_midnight() - reject_based_freeze_timer_ >
                                          query_trading_auto_freeze_manager_.GetFreezeExpireTimer())) {
    ResetRejectsBasedFreeze();
    // Timer Has Expired
    NotifyResetByManualInterventionOverRejects();
  }

  // If we have sent atleast one message and
  // a - mfm since last time has exceeded the threshold
  // b - we haven't received any message after last send
  if ((last_sent_message_mfm_ != 0) &&
      // it's been long since we had sent the message
      ((watch_.msecs_from_midnight() - last_sent_message_mfm_) >
       query_trading_auto_freeze_manager_.GetFreezeORSNoReplyDuration()) &&
      // we haven't received any message from ors
      !received_after_last_sent_ &&
      // It's been atleast 1 minute since we sent notification
      ((watch_.msecs_from_midnight() - last_notify_time_) > 10 * 60 * 1000)) {
    NotifyExchangeRejectsListeners(BaseUtils::FreezeEnforcedReason::kFreezeOnNoResponseFromORS);

    // Setting it here to avoid too many mails, it's placeholder for mail sent counter as well.
    last_notify_time_ = watch_.msecs_from_midnight();
  }

  // Go to UnFreeze state in case we have received reply from ors
  // TODO what if the reply is reject
  if (last_notify_time_ != 0 && received_after_last_sent_) {
    ResetNoResponseFromORSFreeze();
    NotifyResetByManualInterventionOverRejects();
    last_notify_time_ = 0;
  }
}

void BaseOrderManager::NotifyWorstPosToOrs(int worst_case_position_) {
  // This function calls the BaseTrader function to notify the ORS about the queries worst_case_positions
  base_trader_.NotifyRiskLimitsToOrs(dep_symbol_, worst_case_position_);
}

BaseOrder* BaseOrderManager::SendTrade(const double _price_, const int _intpx_, int _size_requested_,
                                       TradeType_t t_buysell_, char placed_at_level_indicator_,
                                       OrderType_t order_type) {
#if ENABLE_AGG_TRADING_COOLOFF
#if ENABLE_AGG_TRADING_COOLOFF_ORDER_DISABLE
  if (agg_trading_cooloff_ && disable_orders_due_to_agg_cooloff_) {
    if (watch_.msecs_from_midnight() > agg_cooloff_stop_msecs_) {
      disable_orders_due_to_agg_cooloff_ = false;
      agg_cooloff_stop_msecs_ = 0;

      DBGLOG_TIME_CLASS_FUNC << " agg_trading_cooloff_ deactivated. Enabling order." << DBGLOG_ENDL_FLUSH;
    } else {
      return;
    }
  }
#endif
#endif
/*
  if (if_reject_set) {
    if (watch_.msecs_from_midnight() < no_new_order_till) {
      // no_new_orders = watch_.msecs_from_midnight() + REJECT_COOLOFF_MSECS ; // set in OrderRejected
      DBGLOG_TIME_CLASS_FUNC << " BaseOrderManager::SendTrade :: did not send order due to reject cooloff"
                             << DBGLOG_ENDL_FLUSH;
      if_reject_set = false;
      return nullptr;
    } else {
      if_reject_set = false;
    }
  }
*/

  if (_size_requested_ <= 0) {
    DBGLOG_TIME_CLASS_FUNC << " BaseOrderManager::SendTrade :: did not send order due to non-positive size: "
                           << _size_requested_ << DBGLOG_ENDL_FLUSH;
    return nullptr;
  }

  template_order.security_name_ = dep_symbol_;
  template_order.buysell_ = t_buysell_;
  template_order.price_ = _price_;
  template_order.size_remaining_ = 0;                 // on sending size_remaining_ is 0
  template_order.size_requested_ = _size_requested_;  // and the requested size is shown here

  if (set_disclosed_size_) {
    template_order.size_disclosed_ = ceil(disclosed_size_factor_ * _size_requested_);
  } else {
    template_order.size_disclosed_ = _size_requested_;
  }
  template_order.int_price_ = _intpx_;
  template_order.order_status_ = kORRType_None;  // not even sequenced

  template_order.queue_size_ahead_ = 0;
  template_order.queue_size_behind_ = 0;
  template_order.num_events_seen_ = 0;
  template_order.placed_msecs_ = 0;

  template_order.client_assigned_order_sequence_ = client_assigned_order_sequence_++;
  template_order.server_assigned_order_sequence_ = 0;  // this is intended to be an invalid value and hence all valid
                                                       // server_assigned_order_sequence_ values must be > 0
  // template_order.server_assigned_client_id_ = server_assigned_client_id_ ; // not really needed since the
  // ClientThread
  // at the ORS knows this already

  if (dbglogger_.CheckLoggingLevel(FILL_TIME_INFO)) {
    DBGLOG_TIME_CLASS << "SENDING " << watch_.msecs_from_midnight() << " " << template_order.int_price_ << " "
                      << template_order.buysell_ << " " << template_order.client_assigned_order_sequence_
                      << DBGLOG_ENDL_FLUSH;
  }
  template_order.size_executed_ = 0;
  template_order.canceled_ = false;
  template_order.modified_ = false;
  template_order.replayed_ = false;
  template_order.is_fok_ = order_type == kOrderFOK;
  template_order.is_ioc_ = order_type == kOrderIOC;
  template_order.placed_at_level_indicator_ = placed_at_level_indicator_;

  ///< finally the order was actually sent to the trader ( SImTrader / LiveTrader )
  base_trader_.SendTrade(template_order);

  BaseOrder* p_new_order_ =
      baseorder_mempool_.Clone(&template_order); /* do we need to allocate memory, can't we just use the struct ? */

  if (t_buysell_ == kTradeTypeBuy) {
    int bid_index_ = GetBidIndexAndAdjustIntPrice(p_new_order_->int_price());
    int size_requested_ = p_new_order_->size_requested();
    sum_bid_unconfirmed_[bid_index_] += size_requested_;
    sum_bid_unconfirmed_orders_[bid_index_] += 1;
    PropagateUnconfirmedSizeDifference(bid_index_, size_requested_, t_buysell_);
    AdjustTopBottomUnconfirmedBidIndexes(bid_index_);

    unsequenced_bids_.push_back(p_new_order_);  ///< adding to vector of unsequenced_bids_
    num_unconfirmed_orders_++;  ///< increment number of unconfirmed orders ... could be used to throttle
    // Update Sum Bid size
    sum_bid_sizes_ += p_new_order_->size_requested();
  } else {
    int ask_index_ = GetAskIndexAndAdjustIntPrice(p_new_order_->int_price());
    int size_requested_ = p_new_order_->size_requested();
    sum_ask_unconfirmed_[ask_index_] += size_requested_;
    sum_ask_unconfirmed_orders_[ask_index_] += 1;
    PropagateUnconfirmedSizeDifference(ask_index_, size_requested_, t_buysell_);
    AdjustTopBottomUnconfirmedAskIndexes(ask_index_);

    unsequenced_asks_.push_back(p_new_order_);  ///< adding to vector of unsequenced_asks_
    num_unconfirmed_orders_++;  ///< increment number of unconfirmed orders ... could be used to throttle
    sum_ask_sizes_ += p_new_order_->size_requested();
  }

  if (p_new_order_->is_fok_) {
    if (t_buysell_ == kTradeTypeBuy) {
      foked_bid_order_size_sum_++;
    } else {
      foked_ask_order_size_sum_++;
    }
  }

  // If it is first send message or
  // We have received any message since last send then update the last_sent_message time
  if (last_sent_message_mfm_ == 0 || received_after_last_sent_) {
    last_sent_message_mfm_ = watch_.msecs_from_midnight();
    received_after_last_sent_ = false;
  }

  if (throttle_manager_) {
    throttle_manager_->update_throttle_manager(watch_.msecs_from_midnight());
  }

  if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << " CAOS: " << p_new_order_->client_assigned_order_sequence_
                           << " bs: " << (p_new_order_->buysell_ == kTradeTypeBuy ? "BUY" : "SELL")
                           << " sz: " << p_new_order_->size_requested_ << " px: " << p_new_order_->price_
                           << " intpx: " << p_new_order_->int_price_ << '\n';
    dbglogger_.CheckToFlushBuffer();
    if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO))  // zero logging
    {
      dbglogger_ << "SYM: " << p_new_order_->security_name_ << " Px: " << p_new_order_->price()
                 << " INTPX: " << p_new_order_->int_price() << " BS: " << GetTradeTypeChar(p_new_order_->buysell())
                 << " ST: " << watch_.tv() << " DT: "
                 << watch_.tv() + ttime_t(0, p_new_order_->seqd_msecs_ * 1000 + p_new_order_->seqd_usecs_ % 1000) -
                        ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                 << " ORR: " << ToString(kORRType_Seqd) << " SAOS: " << p_new_order_->server_assigned_order_sequence()
                 << " CAOS: " << p_new_order_->client_assigned_order_sequence() << " CLTPOS: " << client_position_
                 << " SACI: " << server_assigned_client_id_ << " GBLPOS: " << global_position_
                 << " SIZE: " << p_new_order_->size_remaining() << " SE: " << p_new_order_->size_executed()
                 << " MsgSeq : " << 0 << " Seq: " << 0 << DBGLOG_ENDL_FLUSH;
    }
    dbglogger_.CheckToFlushBuffer();
  }

  // For fill-ratio comps.
  total_size_placed_ += p_new_order_->size_requested_;

  // Increment SendOrderCount ( )
  send_order_count_++;

  if (placed_at_level_indicator_ == 'B') {
    switch (p_new_order_->buysell_) {
      case kTradeTypeBuy:
        ResetPlacedLevelIndicatorOnBids(p_new_order_->int_price_);
        break;
      case kTradeTypeSell:
        ResetPlacedLevelIndicatorOnAsks(p_new_order_->int_price_);
        break;
      default:
        break;
    }
  }

#if ENABLE_AGG_TRADING_COOLOFF
  if (agg_trading_cooloff_) {
    if (placed_at_level_indicator_ == 'A') {
      if (p_new_order_->buysell_ == kTradeTypeBuy || p_new_order_->buysell_ == kTradeTypeSell) {
        if (intentional_agg_lasttime_[kTradeTypeBuy] < (watch_.msecs_from_midnight() - agg_time_frame_msecs_)) {
          intentional_agg_sizes_[kTradeTypeBuy] = 0;
        }
        if (intentional_agg_lasttime_[kTradeTypeSell] < (watch_.msecs_from_midnight() - agg_time_frame_msecs_)) {
          intentional_agg_sizes_[kTradeTypeSell] = 0;
        }

        intentional_agg_sizes_[p_new_order_->buysell_]++;
        intentional_agg_lasttime_[p_new_order_->buysell_] = watch_.msecs_from_midnight();

        if (intentional_agg_sizes_[kTradeTypeBuy] > agg_size_threshold_ &&
            intentional_agg_sizes_[kTradeTypeSell] > agg_size_threshold_) {  // Significant amount of bi-directional agg
                                                                             // trading in the last
                                                                             // agg_time_frame_msecs_ milliseconds.

#if ENABLE_AGG_TRADING_COOLOFF_ORDER_DISABLE
          agg_cooloff_stop_msecs_ = watch_.msecs_from_midnight() + (100 * agg_time_frame_msecs_);
          disable_orders_due_to_agg_cooloff_ = true;

          DBGLOG_TIME_CLASS_FUNC << " agg_trading_cooloff_ activated. Disabling orders." << DBGLOG_ENDL_FLUSH;
#endif

          if (!email_sent_) {
            EmailAggVariablesOnDisable();
            email_sent_ = true;
          }

          DumpAggVariables();
        }
      }
    }
  }
#endif

  return p_new_order_;
}

bool BaseOrderManager::ModifyOrderAndLog(BaseOrder* t_this_order, double new_price, int new_int_price, int new_size) {
  if (!modify_partial_exec_ && t_this_order->size_executed() > 0) {
    // If we can't modify an order which has partially executed, cancel it.
    Cancel(*t_this_order);
    return false;
  }

  if (Modify(t_this_order, new_price, new_int_price, new_size)) {
    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " Modified Order: "
                             << " CAOS: " << t_this_order->client_assigned_order_sequence()
                             << " bs: " << ((t_this_order->buysell_ == kTradeTypeBuy) ? "BUY" : "SELL")
                             << " oldsz: " << t_this_order->size_requested_ << " newsz: " << new_size
                             << " oldpx: " << t_this_order->price_ << " newpx: " << new_price
                             << " oldintpx: " << t_this_order->int_price_ << " newintpx: " << new_int_price
                             << DBGLOG_ENDL_FLUSH;
    }

    if (t_this_order->buysell_ == kTradeTypeBuy) {
      int t_bid_index_ = GetBidIndexAndAdjustIntPrice(new_int_price);
      // Increase in unconfirmed size at new price
      sum_bid_unconfirmed_[t_bid_index_] = std::max(0, sum_bid_unconfirmed_[t_bid_index_] + new_size);
      sum_bid_unconfirmed_orders_[t_bid_index_] = std::max(0, sum_bid_unconfirmed_orders_[t_bid_index_] + 1);
      PropagateUnconfirmedSizeDifference(t_bid_index_, new_size, kTradeTypeBuy);
      AdjustTopBottomUnconfirmedBidIndexes(t_bid_index_);
    } else if (t_this_order->buysell_ == kTradeTypeSell) {
      int t_ask_index_ = GetAskIndexAndAdjustIntPrice(new_int_price);
      // Increase in unconfirmed size at new price
      sum_ask_unconfirmed_[t_ask_index_] = std::max(0, sum_ask_unconfirmed_[t_ask_index_] + new_size);
      sum_ask_unconfirmed_orders_[t_ask_index_] = std::max(0, sum_ask_unconfirmed_orders_[t_ask_index_] + 1);
      PropagateUnconfirmedSizeDifference(t_ask_index_, new_size, kTradeTypeSell);
      AdjustTopBottomUnconfirmedAskIndexes(t_ask_index_);
    }
    return true;
  }
  return false;
}

bool BaseOrderManager::Modify(BaseOrder* t_this_order_, double _new_price_, int _new_int_price_, int _new_size_) {
  if (t_this_order_->CanBeModified(watch_.msecs_from_midnight() - MODIFY_WAIT_MSECS)) {
    // If it is first send message or
    // We have received any message since last send then update the last_sent_message time
    if (last_sent_message_mfm_ == 0 || received_after_last_sent_) {
      last_sent_message_mfm_ = watch_.msecs_from_midnight();
      received_after_last_sent_ = false;
    }

    t_this_order_->modified_ = true;

    if (set_disclosed_size_) {
      t_this_order_->size_disclosed_ = ceil(disclosed_size_factor_ * _new_size_);
    } else {
      t_this_order_->size_disclosed_ = _new_size_;
    }

    t_this_order_->modified_new_size_ = _new_size_;
    t_this_order_->modified_new_price_ = _new_price_;
    t_this_order_->modified_new_int_price_ = _new_int_price_;

    base_trader_.Modify(*t_this_order_, _new_price_, _new_int_price_, _new_size_);

    // make sure that the throttle manager is used only for exchanges where we need it
    if (throttle_manager_) {
      throttle_manager_->update_throttle_manager(watch_.msecs_from_midnight());
    }

    modify_order_count_++;
    // t_this_order_->SequenceAtTime( watch_.msecs_from_midnight() , watch_.tv().tv_usec);
    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "CanBeModified - so sending_modify of CAOS: "
                             << t_this_order_->client_assigned_order_sequence_
                             << " bs: " << ((t_this_order_->buysell_ == kTradeTypeBuy) ? "BUY" : "SELL")
                             << " sz: " << t_this_order_->size_requested_ << " px: " << t_this_order_->price_
                             << " intpx: " << t_this_order_->int_price_ << " new_sz: " << _new_size_
                             << " new_px: " << _new_price_ << " new_intpx: " << _new_int_price_ << DBGLOG_ENDL_FLUSH;
    }
    return true;
  }
  return false;
}

void BaseOrderManager::ResetPlacedLevelIndicatorOnBids(const int t_best_bid_int_price_) {
  // Assuming the given int price is the current best bid
  if (t_best_bid_int_price_ != best_bid_int_price_) {
    // if this int price does not match last best_bid_int_price_
    if (t_best_bid_int_price_ > best_bid_int_price_) {
      int bid_index_ = GetBidIndex(best_bid_int_price_);
      if (bid_index_ >= 0 && bid_index_ < ORDER_MANAGER_INT_PRICE_RANGE) {
        const std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[bid_index_];
        // then for all orders at that inpx on the bid side set the placed_at_level_indicator_ to 'S'
        for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
          BaseOrder* p_this_order_ = _this_base_order_vec_[i];
          if (p_this_order_ != nullptr) {
            if (p_this_order_->placed_at_level_indicator_ == 'B') {
              p_this_order_->SetAsSupporting();
            }
          }
        }
      }
    }
    // and set best_bid_int_price_ to this intpx
    best_bid_int_price_ = t_best_bid_int_price_;
  }
}

// If we sent a best bid order and it has now become improve ( because others cancelled ), then we should set it as
// improve
void BaseOrderManager::ResetBidsAsImproveUponCancellation(const int t_best_bid_int_price_,
                                                          const int t_best_ask_int_price_) {
  int best_bid_index_ = GetBidIndex(t_best_bid_int_price_);
  int bid_ask_int_diff_ = t_best_ask_int_price_ - t_best_bid_int_price_;

  for (int bid_index_ = best_bid_index_ + 1; bid_index_ < best_bid_index_ + bid_ask_int_diff_ && bid_index_ >= 0 &&
                                                 bid_index_ < ORDER_MANAGER_INT_PRICE_RANGE;
       bid_index_++) {
    const std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[bid_index_];
    for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
      BaseOrder* p_this_order_ = _this_base_order_vec_[i];
      if (p_this_order_ != nullptr) {
        p_this_order_->SetAsImprove();
      }
    }
  }
}

// If we sent a best ask order and it has now become improve ( because others cancelled ), then we should set it as
// improve
void BaseOrderManager::ResetAsksAsImproveUponCancellation(const int t_best_bid_int_price_,
                                                          const int t_best_ask_int_price_) {
  int best_ask_index_ = GetAskIndex(t_best_ask_int_price_);
  int bid_ask_int_diff_ = t_best_ask_int_price_ - t_best_bid_int_price_;

  for (int ask_index_ = best_ask_index_ + 1; ask_index_ < best_ask_index_ + bid_ask_int_diff_ && ask_index_ >= 0 &&
                                                 ask_index_ < ORDER_MANAGER_INT_PRICE_RANGE;
       ask_index_++) {
    const std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[ask_index_];
    for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
      BaseOrder* p_this_order_ = _this_base_order_vec_[i];
      if (p_this_order_ != nullptr) {
        p_this_order_->SetAsImprove();
      }
    }
  }
}

void BaseOrderManager::ResetPlacedLevelIndicatorOnAsks(const int t_best_ask_int_price_) {
  // Assuming the given int price is the current best ask
  if (t_best_ask_int_price_ != best_ask_int_price_) {
    // if this int price does not match last best_ask_int_price_
    if (t_best_ask_int_price_ < best_ask_int_price_) {
      int ask_index_ = GetAskIndex(best_ask_int_price_);
      if (ask_index_ >= 0 && ask_index_ < ORDER_MANAGER_INT_PRICE_RANGE && order_vec_top_ask_index_ != -1) {
        const std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[ask_index_];
        // then for all orders at that inpx on the ask side set the placed_at_level_indicator_ to 'S'
        for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
          BaseOrder* p_this_order_ = _this_base_order_vec_[i];
          if (p_this_order_ != nullptr) {
            if (p_this_order_->placed_at_level_indicator_ == 'B') {
              p_this_order_->SetAsSupporting();
            }
          }
        }
      }
    }
    // and set best_ask_int_price_ to this intpx
    best_ask_int_price_ = t_best_ask_int_price_;
  }
}

int BaseOrderManager::CancelReplaceBidOrdersEqAboveAndEqBelowIntPrice(
    const int _cxl_upper_int_price_, const int _cxl_lower_int_price_,
    const std::vector<double>& _prices_to_place_at_vec_, const std::vector<int>& _int_prices_to_place_at_vec_,
    const std::vector<int>& _sizes_to_place_vec_, const std::vector<char>& _order_level_indicator_vec_,
    const OrderType_t _order_type_) {
  int top_bid_index_ = GetBidIndex(_cxl_upper_int_price_);     // we need to cancel all orders above this price
  int bottom_bid_index_ = GetBidIndex(_cxl_lower_int_price_);  // we need to cancel all orders below this price

  int top_bid_index_to_cancel_ = top_bid_index_;  // no-min, std::min ( order_vec_top_bid_index_, top_bid_index_ );
  int bottom_bid_index_to_cancel_ = bottom_bid_index_;  // std::max ( bottom_bid_index_, order_vec_bottom_bid_index_ );
  unsigned int num_modified_order_ = 0;

  if (order_vec_top_bid_index_ != -1) {
    for (int index_ = order_vec_top_bid_index_;
         index_ >= std::max(top_bid_index_to_cancel_, order_vec_bottom_bid_index_); index_--) {
      std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];
      if (!_this_base_order_vec_.empty()) {
        for (int i = _this_base_order_vec_.size() - 1; i >= 0; i--) {
          if (num_modified_order_ < _prices_to_place_at_vec_.size()) {
            if (ModifyOrderAndLog(_this_base_order_vec_[i], _prices_to_place_at_vec_[num_modified_order_],
                                  _int_prices_to_place_at_vec_[num_modified_order_],
                                  _sizes_to_place_vec_[num_modified_order_])) {
              // decrease in unconfirmend size at old price
              // Not doing it as not done in cxl
              // sum_bid_unconfirmed_[ index_ ] = std::max ( 0, sum_bid_unconfirmed_[ index_ ] -
              // _this_base_order_vec_[i]->size_remaining() );
              // AdjustTopBottomUnconfirmedBidIndexes ( index_ );

              num_modified_order_++;
            }
          } else {
            Cancel(*_this_base_order_vec_[i]);
          }
        }
      }
    }

    for (int index_ = std::min(bottom_bid_index_to_cancel_, order_vec_top_bid_index_);
         index_ >= order_vec_bottom_bid_index_; index_--) {
      std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];
      if (!_this_base_order_vec_.empty()) {
        for (int i = _this_base_order_vec_.size() - 1; i >= 0; i--) {
          if (num_modified_order_ < _prices_to_place_at_vec_.size()) {
            if (ModifyOrderAndLog(_this_base_order_vec_[i], _prices_to_place_at_vec_[num_modified_order_],
                                  _int_prices_to_place_at_vec_[num_modified_order_],
                                  _sizes_to_place_vec_[num_modified_order_])) {
              num_modified_order_++;
            }
          } else {
            Cancel(*_this_base_order_vec_[i]);
          }
        }
      }
    }
  }

  int num_sent_orders_ = 0;
  if (num_modified_order_ < _int_prices_to_place_at_vec_.size()) {
    while (num_modified_order_ + num_sent_orders_ < _int_prices_to_place_at_vec_.size()) {
      int idx_ = num_modified_order_ + num_sent_orders_;
      SendTrade(_prices_to_place_at_vec_[idx_], _int_prices_to_place_at_vec_[idx_], _sizes_to_place_vec_[idx_],
                kTradeTypeBuy, _order_level_indicator_vec_[idx_], _order_type_);
      if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " SendTrade B of "
                               << " size:  " << _sizes_to_place_vec_[idx_] << " px: " << _prices_to_place_at_vec_[idx_]
                               << " intpx: " << _int_prices_to_place_at_vec_[idx_] << DBGLOG_ENDL_FLUSH;
      }
      num_sent_orders_++;
    }
  }
  return num_modified_order_;
}

int BaseOrderManager::CancelReplaceAskOrdersEqAboveAndEqBelowIntPrice(
    const int _cxl_upper_int_price_, const int _cxl_lower_int_price_,
    const std::vector<double>& _prices_to_place_at_vec_, const std::vector<int>& _int_prices_to_place_at_vec_,
    const std::vector<int>& _sizes_to_place_vec_, const std::vector<char>& _order_level_indicator_vec_,
    const OrderType_t _order_type_) {
  int top_ask_index_ = GetAskIndex(_cxl_upper_int_price_);
  int bottom_ask_index_ = GetAskIndex(_cxl_lower_int_price_);

  int top_ask_index_to_cancel_ = top_ask_index_;        // std::min ( order_vec_top_ask_index_, top_ask_index_ );
  int bottom_ask_index_to_cancel_ = bottom_ask_index_;  // std::max ( bottom_ask_index_, order_vec_bottom_ask_index_ );
  unsigned int num_modified_order_ = 0;
  if (order_vec_top_ask_index_ != -1) {
    for (int index_ = order_vec_top_ask_index_;
         index_ >= std::max(top_ask_index_to_cancel_, order_vec_bottom_ask_index_); index_--) {
      std::vector<BaseOrder*> _this_base_order_vec_ = ask_order_vec_[index_];
      if (!_this_base_order_vec_.empty()) {
        for (int i = _this_base_order_vec_.size() - 1; i >= 0; i--) {
          if (num_modified_order_ < _prices_to_place_at_vec_.size()) {
            if (ModifyOrderAndLog(_this_base_order_vec_[i], _prices_to_place_at_vec_[num_modified_order_],
                                  _int_prices_to_place_at_vec_[num_modified_order_],
                                  _sizes_to_place_vec_[num_modified_order_])) {
              num_modified_order_++;
            }
          } else {
            Cancel(*_this_base_order_vec_[i]);
          }
        }
      }
    }

    for (int index_ = std::min(order_vec_top_ask_index_, bottom_ask_index_to_cancel_);
         index_ >= order_vec_bottom_ask_index_; index_--) {
      std::vector<BaseOrder*> _this_base_order_vec_ = ask_order_vec_[index_];
      if (!_this_base_order_vec_.empty()) {
        for (int i = _this_base_order_vec_.size() - 1; i >= 0; i--) {
          if (num_modified_order_ < _prices_to_place_at_vec_.size()) {
            if (ModifyOrderAndLog(_this_base_order_vec_[i], _prices_to_place_at_vec_[num_modified_order_],
                                  _int_prices_to_place_at_vec_[num_modified_order_],
                                  _sizes_to_place_vec_[num_modified_order_])) {
              num_modified_order_++;
            }
          } else {
            Cancel(*_this_base_order_vec_[i]);
          }
        }
      }
    }
  }

  int num_sent_orders_ = 0;
  if (num_modified_order_ < _int_prices_to_place_at_vec_.size()) {
    while (num_modified_order_ + num_sent_orders_ < _int_prices_to_place_at_vec_.size()) {
      int idx_ = num_modified_order_ + num_sent_orders_;
      SendTrade(_prices_to_place_at_vec_[idx_], _int_prices_to_place_at_vec_[idx_], _sizes_to_place_vec_[idx_],
                kTradeTypeSell, _order_level_indicator_vec_[idx_], _order_type_);

      if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " SendTrade S of "
                               << " size:  " << _sizes_to_place_vec_[idx_] << " px: " << _prices_to_place_at_vec_[idx_]
                               << " intpx: " << _int_prices_to_place_at_vec_[idx_] << DBGLOG_ENDL_FLUSH;
      }
      num_sent_orders_++;
    }
  }
  return num_modified_order_;
}

void BaseOrderManager::CancelAllBidOrders() {
  if (order_vec_top_bid_index_ != -1) {
    for (int index_ = order_vec_top_bid_index_; index_ >= order_vec_bottom_bid_index_; index_--) {
      const std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];
      for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
        BaseOrder* p_this_order_ = _this_base_order_vec_[i];
        if (p_this_order_ != nullptr) Cancel(*p_this_order_);
      }
    }
  }
}

void BaseOrderManager::CancelAllAskOrders() {
  if (order_vec_top_ask_index_ != -1) {
    for (int index_ = order_vec_top_ask_index_; index_ >= order_vec_bottom_ask_index_; index_--) {
      const std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[index_];
      for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
        BaseOrder* p_this_order_ = _this_base_order_vec_[i];
        if (p_this_order_ != nullptr) Cancel(*p_this_order_);
      }
    }
  }
}

void BaseOrderManager::CancelAllOrders() {
  CancelAllBidOrders();
  CancelAllAskOrders();
  external_cancel_all_outstanding_orders_ = true;
}

/**
 *
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param t_server_assigned_order_sequence_
 * @param _security_id_
 * @param t_buysell_
 * @param r_int_price_
 * @param server_assigned_message_sequence
 * @param exchange_order_id
 * @param time_set_by_server
 */
void BaseOrderManager::OrderNotFound(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                     const int t_server_assigned_order_sequence_, const unsigned int _security_id_,
                                     const TradeType_t t_buysell_, const int r_int_price_,
                                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                     const ttime_t time_set_by_server) {
  if (t_server_assigned_client_id_ == server_assigned_client_id_) {
    received_after_last_sent_ = true;

    if (dbglogger_.CheckLoggingLevel(ORS_DATA_INFO)) {
      DBGLOG_CLASS_FUNC_LINE_INFO << "SACI : " << t_server_assigned_client_id_ << ' '
                                  << "CAOS : " << _client_assigned_order_sequence_ << ' '
                                  << "SAOS : " << t_server_assigned_order_sequence_ << ' ' << "BUYSELL : " << t_buysell_
                                  << ' ' << "IntPx : " << r_int_price_ << ' '
                                  << "SAMS : " << server_assigned_message_sequence << DBGLOG_ENDL_FLUSH;
    }

    // Check For Drops
    if (server_assigned_message_sequence > expected_message_sequence_) {
      // let's drop a recovery request now
      GoForPacketRecovery(expected_message_sequence_, server_assigned_message_sequence);
      expected_message_sequence_ = server_assigned_message_sequence + 1;

    } else if (server_assigned_message_sequence == expected_message_sequence_) {  // Move forward

      // It's very important that the expected_message_sequence_ is always correct and in sync
      // potentially incorrect sequence would keep us in loop of recovery
      // Also we don't need to check the dropped_sequences_ set for removing the packet sequence since the recovery
      // packets should have lower sequences
      expected_message_sequence_++;

    } else {
      // We received a lower sequence, it's fine, it should be the dropped message for which ORS has sent reply
      if (dropped_sequences_.end() != dropped_sequences_.find(server_assigned_message_sequence)) {
        dropped_sequences_.erase(server_assigned_message_sequence);
      }
    }

    // remove the order from any vector / map
    // remove the size of the order from the relevant map

    if (t_buysell_ == kTradeTypeBuy) {    // BID order
      BaseOrder* p_this_order_ = nullptr; /* will stay nullptr till order found */
      int bid_index_ = GetBidIndexAndAdjustIntPrice(r_int_price_);

      // if t_server_assigned_order_sequence_ is > 0 then the order is either in intpx_2_bid_order_vec_ or nowhere
      // else ... order could be in intpx_2_bid_order_vec_ ( if it got sequenced later ) or most likely it is in
      // unsequenced_bids_

      /* search in sequenced orders at the price sent by ORS */
      /* fetch order in intpx_2_bid_order_vec_ [ r_int_price_ ] */
      std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[bid_index_];

      if (!_this_base_order_vec_.empty()) {  // and the vector is non empty
        for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
             _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
          if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
              t_server_assigned_order_sequence_) {  // Found the order
            p_this_order_ = (*_base_order_vec_iter_);

            switch (p_this_order_->order_status()) {  // depending on the last status that we had seen of this order,
                                                      // the maps or data structures from which we need to remove /
                                                      // modify this order changes
              case kORRType_Seqd: {  // either missed the confirm or the exec came from the exchange before the confirm
                num_unconfirmed_orders_--;

                sum_bid_unconfirmed_[bid_index_] =
                    std::max(0, sum_bid_unconfirmed_[bid_index_] - p_this_order_->size_requested());
                sum_bid_unconfirmed_orders_[bid_index_] = std::max(0, sum_bid_unconfirmed_orders_[bid_index_] - 1);
                PropagateUnconfirmedSizeDifference(bid_index_, -p_this_order_->size_requested(), kTradeTypeBuy);
                AdjustTopBottomUnconfirmedBidIndexes(bid_index_);

              } break;
              case kORRType_Conf: {
                sum_bid_confirmed_[bid_index_] =
                    std::max(0, sum_bid_confirmed_[bid_index_] - p_this_order_->size_remaining());
                PropagateConfirmedSizeDifference(bid_index_, -p_this_order_->size_remaining(), kTradeTypeBuy);
                AdjustTopBottomConfirmedBidIndexes(bid_index_);
              } break;
              default: {}
            }

            _this_base_order_vec_.erase(_base_order_vec_iter_);  // remove order from vec
            AdjustTopBottomOrderVecBidIndexes(bid_index_);

            baseorder_mempool_.DeAlloc(p_this_order_);
            break;
          }
        }
      }

      if (p_this_order_ == nullptr) {  // If not found among sequenced orders at given price level ...
        // search for order in unsequenced_bids_
        for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = unsequenced_bids_.begin();
             _base_order_vec_iter_ != unsequenced_bids_.end(); _base_order_vec_iter_++) {
          if ((*_base_order_vec_iter_)->client_assigned_order_sequence() ==
              _client_assigned_order_sequence_) { /* found order */
            p_this_order_ = (*_base_order_vec_iter_);

            p_this_order_->SequenceAtTime(watch_.msecs_from_midnight(), watch_.tv().tv_usec);
            p_this_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;

            num_unconfirmed_orders_--;  ///< decrement number of unconfirmed orders

            sum_bid_unconfirmed_[bid_index_] =
                std::max(0, sum_bid_unconfirmed_[bid_index_] - p_this_order_->size_requested());
            sum_bid_unconfirmed_orders_[bid_index_] = std::max(0, sum_bid_unconfirmed_orders_[bid_index_] - 1);
            PropagateUnconfirmedSizeDifference(bid_index_, -p_this_order_->size_requested(), kTradeTypeBuy);
            AdjustTopBottomUnconfirmedBidIndexes(bid_index_);

            unsequenced_bids_.erase(_base_order_vec_iter_);
            break;
          }
        }
      }

      NotifyOrderChangeListeners();

    } else {                              // ASK order
      BaseOrder* p_this_order_ = nullptr; /* will stay nullptr till order found */
      int ask_index_ = GetAskIndexAndAdjustIntPrice(r_int_price_);

      // if t_server_assigned_order_sequence_ is > 0 then the order is either in intpx_2_ask_order_vec_ or nowhere
      // else ... order could be in intpx_2_ask_order_vec_ ( if it got sequenced later ) or most likely it is in
      // unsequenced_asks_

      /* search in sequenced orders at the price sent by ORS */
      /* fetch order in intpx_2_ask_order_vec_ [ r_int_price_ ] */
      std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[ask_index_];
      if (!_this_base_order_vec_.empty()) {  // and the vector is non empty
        for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
             _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
          if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
              t_server_assigned_order_sequence_) {  // Found the order
            p_this_order_ = (*_base_order_vec_iter_);

            switch (p_this_order_->order_status()) {  // depending on the last status that we had seen of this order,
                                                      // the maps or data structures from which we need to remove /
                                                      // modify this order changes
              case kORRType_Seqd: {  // either missed the confirm or the exec came from the exchange before the confirm
                num_unconfirmed_orders_--;

                sum_ask_unconfirmed_[ask_index_] =
                    std::max(0, sum_ask_unconfirmed_[ask_index_] - p_this_order_->size_requested());
                sum_ask_unconfirmed_orders_[ask_index_] = std::max(0, sum_ask_unconfirmed_orders_[ask_index_] - 1);
                PropagateUnconfirmedSizeDifference(ask_index_, -p_this_order_->size_requested(), kTradeTypeSell);
                AdjustTopBottomUnconfirmedAskIndexes(ask_index_);

              } break;
              case kORRType_Conf: {
                sum_ask_confirmed_[ask_index_] =
                    std::max(0, sum_ask_confirmed_[ask_index_] - p_this_order_->size_remaining());
                sum_ask_confirmed_orders_[ask_index_] = std::max(0, sum_ask_confirmed_orders_[ask_index_] - 1);
                PropagateConfirmedSizeDifference(ask_index_, -p_this_order_->size_remaining(), kTradeTypeSell);
                AdjustTopBottomConfirmedAskIndexes(ask_index_);
              } break;
              default: {}
            }

            _this_base_order_vec_.erase(_base_order_vec_iter_);  // remove order from vec
            AdjustTopBottomOrderVecAskIndexes(ask_index_);

            baseorder_mempool_.DeAlloc(p_this_order_);
            break;
          }
        }
      }

      if (p_this_order_ == nullptr) {  // If not found among sequenced orders at given price level ...
        // search for order in unsequenced_asks_
        for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = unsequenced_asks_.begin();
             _base_order_vec_iter_ != unsequenced_asks_.end(); _base_order_vec_iter_++) {
          if ((*_base_order_vec_iter_)->client_assigned_order_sequence() ==
              _client_assigned_order_sequence_) { /* found order */
            p_this_order_ = (*_base_order_vec_iter_);

            p_this_order_->SequenceAtTime(watch_.msecs_from_midnight(), watch_.tv().tv_usec);
            p_this_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;

            num_unconfirmed_orders_--;  ///< decrement number of unconfirmed orders

            sum_ask_unconfirmed_[ask_index_] =
                std::max(0, sum_ask_unconfirmed_[ask_index_] - p_this_order_->size_requested());
            sum_ask_unconfirmed_orders_[ask_index_] = std::max(0, sum_ask_unconfirmed_orders_[ask_index_] - 1);
            PropagateUnconfirmedSizeDifference(ask_index_, -p_this_order_->size_requested(), kTradeTypeSell);
            AdjustTopBottomUnconfirmedAskIndexes(ask_index_);

            unsequenced_asks_.erase(_base_order_vec_iter_);
            break;
          }
        }
      }

      NotifyOrderChangeListeners();
    }

    // log message
    if (dbglogger_.CheckLoggingLevel(OM_INFO))  // zero logging
    {
      DBGLOG_TIME_CLASS_FUNC << " SACI: " << t_server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_
                             << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_
                             << " bs: " << (t_buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << " intpx: " << r_int_price_
                             << DBGLOG_ENDL_FLUSH;
    }

    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " ShowOrders: " << DBGLOG_ENDL_FLUSH;
      LogFullStatus();
    }
  }

  //    CleanMaps ( );
}

/**
 *
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param t_server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param t_buysell_
 * @param _size_remaining_
 * @param _size_executed_
 * @param t_client_position_
 * @param t_global_position_
 * @param r_int_price_
 * @param server_assigned_message_sequence
 */
void BaseOrderManager::OrderSequenced(const int t_server_assigned_client_id_,
                                      const int _client_assigned_order_sequence_,
                                      const int t_server_assigned_order_sequence_, const unsigned int _security_id_,
                                      const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                                      const int _size_executed_, const int t_client_position_,
                                      const int t_global_position_, const int r_int_price_,
                                      const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                      const ttime_t time_set_by_server) {
  global_position_ = t_global_position_;

  if (t_server_assigned_client_id_ == server_assigned_client_id_) {
    received_after_last_sent_ = true;
    if (dbglogger_.CheckLoggingLevel(ORS_DATA_INFO)) {
      DBGLOG_CLASS_FUNC_LINE_INFO << "SACI : " << t_server_assigned_client_id_ << ' '
                                  << "CAOS : " << _client_assigned_order_sequence_ << ' '
                                  << "SAOS : " << t_server_assigned_order_sequence_ << ' '
                                  << "SECID : " << _security_id_ << ' ' << "Px : " << _price_ << ' '
                                  << "BUYSELL : " << t_buysell_ << ' ' << "SR : " << _size_remaining_ << ' '
                                  << "SE : " << _size_executed_ << ' ' << "CP : " << t_client_position_ << ' '
                                  << "GP : " << t_global_position_ << ' ' << "IntPx : " << r_int_price_ << ' '
                                  << "SAMS : " << server_assigned_message_sequence << DBGLOG_ENDL_FLUSH;
    }

    // Check For Drops
    if (server_assigned_message_sequence > expected_message_sequence_) {
      // let's drop a recovery request now
      GoForPacketRecovery(expected_message_sequence_, server_assigned_message_sequence);
      expected_message_sequence_ = server_assigned_message_sequence + 1;

    } else if (server_assigned_message_sequence == expected_message_sequence_) {  // Move forward

      // It's very important that the expected_message_sequence_ is always correct and in sync
      // potentially incorrect sequence would keep us in loop of recovery
      expected_message_sequence_++;

    } else {
      // We received a lower sequence, it's fine could be over recovered sequence
      if (dropped_sequences_.end() != dropped_sequences_.find(server_assigned_message_sequence)) {
        dropped_sequences_.erase(server_assigned_message_sequence);
      }
    }

    // our order

    /* move unconfirmed order from vector 'unsequenced_bids_' to map 'intpx_2_bid_order_vec_'
           if it needs to be canceled send out the cancel */
    if (t_buysell_ == kTradeTypeBuy) {
      // search for order in unsequenced_bids_
      for (std::vector<BaseOrder*>::iterator base_order_vec_iter_ = unsequenced_bids_.begin();
           base_order_vec_iter_ != unsequenced_bids_.end(); base_order_vec_iter_++) {
        if ((*base_order_vec_iter_)->client_assigned_order_sequence_ == _client_assigned_order_sequence_) {
          /* this is the order */
          BaseOrder* p_this_order_ = (*base_order_vec_iter_);

          p_this_order_->SequenceAtTime(watch_.msecs_from_midnight(), watch_.tv().tv_usec);
          p_this_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;

          int bid_index_ = GetBidIndexAndAdjustIntPrice(r_int_price_);
          bid_order_vec_[bid_index_].push_back(p_this_order_);
          AdjustTopBottomOrderVecBidIndexes(bid_index_);

          // erase this order since it is not unsequenced any more
          // no need to search any more
          base_order_vec_iter_ = unsequenced_bids_.erase(base_order_vec_iter_);

          break;
        }
      }

    } else {
      for (std::vector<BaseOrder*>::iterator base_order_vec_iter_ = unsequenced_asks_.begin();
           base_order_vec_iter_ != unsequenced_asks_.end(); base_order_vec_iter_++) {
        if ((*base_order_vec_iter_)->client_assigned_order_sequence_ == _client_assigned_order_sequence_) {
          /* this is the order */
          BaseOrder* p_this_order_ = (*base_order_vec_iter_);

          p_this_order_->SequenceAtTime(watch_.msecs_from_midnight(), watch_.tv().tv_usec);
          p_this_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;

          int ask_index_ = GetAskIndexAndAdjustIntPrice(r_int_price_);
          ask_order_vec_[ask_index_].push_back(p_this_order_);
          AdjustTopBottomOrderVecAskIndexes(ask_index_);

          base_order_vec_iter_ = unsequenced_asks_.erase(base_order_vec_iter_);
          break;
        }
      }
    }

    /* Sequencing should not have changed position, but due to some reason the ORS has this client's position as
     * different
     * than what the client thinks it is. Typically this happens due to a EXEC message lost by client
     * Update position */
    if (client_position_ != t_client_position_) {
      AdjustPosition(t_client_position_, GetMidPrice(), GetIntPx(GetMidPrice()));
    }
    NotifyOrderChangeListeners();

    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " SACI: " << t_server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_
                             << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_
                             << " px: " << _price_ << " bs: " << ((t_buysell_ == kTradeTypeBuy) ? "BUY" : "SELL")
                             << " sizeE " << _size_executed_ << " cpos: " << t_client_position_
                             << " gpos: " << t_global_position_ << " intpx: " << r_int_price_ << DBGLOG_ENDL_FLUSH;
    }

    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " ShowOrders: " << DBGLOG_ENDL_FLUSH;
      LogFullStatus();
      // DBGLOG_TIME_CLASS_FUNC
      // 	       << " ShowOrders: " << ShowOrders ( ) << DBGLOG_ENDL_FLUSH;
    }
  }
}

/**
 *
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param t_server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param t_buysell_
 * @param _size_remaining_
 * @param _size_executed_
 * @param t_client_position_
 * @param t_global_position_
 * @param r_int_price_
 * @param server_assigned_message_sequence
 * @param exchange_order_id
 * @param time_set_by_server
 */
void BaseOrderManager::OrderConfirmed(const int t_server_assigned_client_id_,
                                      const int _client_assigned_order_sequence_,
                                      const int t_server_assigned_order_sequence_, const unsigned int _security_id_,
                                      const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                                      const int _size_executed_, const int t_client_position_,
                                      const int t_global_position_, const int r_int_price_,
                                      const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                      const ttime_t time_set_by_server) {
  // no dependancy on this so update this first regardless of t_server_assigned_client_id_
  global_position_ = t_global_position_;

  if (t_server_assigned_client_id_ == server_assigned_client_id_) {  // our order

    received_after_last_sent_ = true;

    if (dbglogger_.CheckLoggingLevel(ORS_DATA_INFO)) {
      DBGLOG_CLASS_FUNC_LINE_INFO << "SACI : " << t_server_assigned_client_id_ << ' '
                                  << "CAOS : " << _client_assigned_order_sequence_ << ' '
                                  << "SAOS : " << t_server_assigned_order_sequence_ << ' '
                                  << "SECID : " << _security_id_ << ' ' << "Px : " << _price_ << ' '
                                  << "BUYSELL : " << t_buysell_ << ' ' << "SR : " << _size_remaining_ << ' '
                                  << "SE : " << _size_executed_ << ' ' << "CP : " << t_client_position_ << ' '
                                  << "GP : " << t_global_position_ << ' ' << "IntPx : " << r_int_price_ << ' '
                                  << "SAMS : " << server_assigned_message_sequence << DBGLOG_ENDL_FLUSH;
    }

    // Reset the counter for exchange rejects tracker
    number_of_consecutive_exchange_rejects_ = 0;

    // Check For Drops
    if (server_assigned_message_sequence > expected_message_sequence_) {
      // let's drop a recovery request now
      GoForPacketRecovery(expected_message_sequence_, server_assigned_message_sequence);
      expected_message_sequence_ = server_assigned_message_sequence + 1;

    } else if (server_assigned_message_sequence == expected_message_sequence_) {  // Move forward

      // It's very important that the expected_message_sequence_ is always correct and in sync
      // potentially incorrect sequence would keep us in loop of recovery
      expected_message_sequence_++;

    } else {
      // We received a lower sequence, it's fine could be over recovered sequence
      if (dropped_sequences_.end() != dropped_sequences_.find(server_assigned_message_sequence)) {
        dropped_sequences_.erase(server_assigned_message_sequence);
      }
    }

    BaseOrder* p_this_order_ = nullptr;
    if (t_buysell_ == kTradeTypeBuy) {  // BID order
      int bid_index_ = GetBidIndexAndAdjustIntPrice(r_int_price_);

      /* On Confirm order size is moved from intpx_2_sum_bid_unconfirmed_ to intpx_2_sum_bid_confirmed_ */
      /* fetch order in intpx_2_bid_order_vec_ [ r_int_price_ ] */

      /* search in sequenced orders at the price sent by ORS */
      std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[bid_index_];

      if (!_this_base_order_vec_.empty()) {
        for (std::vector<BaseOrder*>::reverse_iterator _base_order_vec_rev_iter_ = _this_base_order_vec_.rbegin();
             _base_order_vec_rev_iter_ != _this_base_order_vec_.rend(); _base_order_vec_rev_iter_++) {
          /* reverse search should be more optimal since newer orders are towards the end */
          if ((*_base_order_vec_rev_iter_)->server_assigned_order_sequence() ==
              t_server_assigned_order_sequence_) {  // Found the order
            p_this_order_ = *_base_order_vec_rev_iter_;
            if (p_this_order_->order_status() != kORRType_Seqd) {
              /// found order and it was already confirmed .. should only happen for Replay callbacks
              /// can be made more robust to handle partial execution misses - TODO
              return;
            }

            break;
          }
        }
      }

      if (p_this_order_ == nullptr) {  // If not found among sequenced orders at given price level ...
        // search for order among unsequenced orders ( in case we missed the sequence )
        for (std::vector<BaseOrder*>::iterator base_order_vec_iter_ = unsequenced_bids_.begin();
             base_order_vec_iter_ != unsequenced_bids_.end(); base_order_vec_iter_++) {
          if ((*base_order_vec_iter_)->client_assigned_order_sequence() == _client_assigned_order_sequence_) {
            /* found order */
            if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << " Unsequneced order..still SACI: " << t_server_assigned_client_id_
                                     << " CAOS: " << _client_assigned_order_sequence_
                                     << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_
                                     << " px: " << _price_
                                     << " bs: " << ((t_buysell_ == kTradeTypeBuy) ? "BUY" : "SELL") << " sizeR "
                                     << _size_remaining_ << " sizeE " << _size_executed_
                                     << " cpos: " << t_client_position_ << " gpos: " << t_global_position_
                                     << " intpx: " << r_int_price_ << '\n';
              dbglogger_.CheckToFlushBuffer();
            }

            p_this_order_ = (*base_order_vec_iter_);

            p_this_order_->SequenceAtTime(watch_.msecs_from_midnight(), watch_.tv().tv_usec);
            p_this_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;

            if (_size_remaining_ > 0) {  // if order still exists and add it to map // should have been added when SEQ
                                         // was sent but we missed the SEQ

              bid_order_vec_[bid_index_].push_back(*base_order_vec_iter_);
              AdjustTopBottomOrderVecBidIndexes(bid_index_);
            }
            unsequenced_bids_.erase(base_order_vec_iter_);  // in either case the order is sequenced and hence remove it
                                                            // from unsequenced_bids_
            break;
          }
        }
      }

      if (p_this_order_ == nullptr) {
        // If not found among sequenced orders at the given int_price level
        // and not found among unsequenced orders
        // error
        if (dbglogger_.CheckLoggingLevel(OM_ERROR) && !dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
          dbglogger_ << watch_.tv() << " ERROR BaseOrderManager::OrderConfirmed got CAOS "
                     << _client_assigned_order_sequence_ << " SAOS " << t_server_assigned_order_sequence_
                     << " int_price " << r_int_price_ << " buysell " << t_buysell_ << '\n';
          dbglogger_.CheckToFlushBuffer();
        }

        if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {  // Create an order and place it in the maps
          BaseOrder* p_new_order_ = baseorder_mempool_.Alloc();

          p_new_order_->security_name_ = dep_symbol_;
          p_new_order_->buysell_ = t_buysell_;
          p_new_order_->price_ = _price_;
          p_new_order_->size_remaining_ = _size_remaining_;
          p_new_order_->size_requested_ = _size_remaining_;
          p_new_order_->int_price_ = r_int_price_;
          p_new_order_->order_status_ = kORRType_Conf;

          p_new_order_->queue_size_ahead_ = 0;
          p_new_order_->queue_size_behind_ = 0;
          p_new_order_->num_events_seen_ = 0;
          p_new_order_->placed_msecs_ = 0;

          p_new_order_->client_assigned_order_sequence_ = _client_assigned_order_sequence_;
          p_new_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;

          p_new_order_->size_executed_ = 0;
          p_new_order_->canceled_ = false;
          p_new_order_->replayed_ = false;
          // p_new_order_->placed_at_level_indicator_ = placed_at_level_indicator_;

          bid_order_vec_[bid_index_].push_back(p_new_order_);
          AdjustTopBottomOrderVecBidIndexes(bid_index_);

          p_this_order_ = p_new_order_;
        } else {
          return;
        }
      }

      // At this point ( p_this_order_ != nullptr )
      // @ ramkris: reducing the unconfirmed orders
      num_unconfirmed_orders_--;

      sum_bid_unconfirmed_[bid_index_] =
          std::max(0, sum_bid_unconfirmed_[bid_index_] - p_this_order_->size_requested());
      sum_bid_unconfirmed_orders_[bid_index_] = std::max(0, sum_bid_unconfirmed_orders_[bid_index_] - 1);
      PropagateUnconfirmedSizeDifference(bid_index_, -p_this_order_->size_requested(), kTradeTypeBuy);
      AdjustTopBottomUnconfirmedBidIndexes(bid_index_);

      if (_size_remaining_ > 0) {  // order is stil live so confirm it
        p_this_order_->ConfirmAtTime(watch_.msecs_from_midnight());
        p_this_order_->ConfirmNewSize(_size_remaining_);  // set both size_requested_ and size_remaining_ to this value

        sum_bid_confirmed_[bid_index_] += _size_remaining_;
        sum_bid_confirmed_orders_[bid_index_] += 1;
        PropagateConfirmedSizeDifference(bid_index_, _size_remaining_, kTradeTypeBuy);
        AdjustTopBottomConfirmedBidIndexes(bid_index_);
      } else {
        RemoveOrderFromVec(bid_order_vec_[bid_index_], p_this_order_);
        AdjustTopBottomOrderVecBidIndexes(bid_index_);

        baseorder_mempool_.DeAlloc(p_this_order_);
      }

      if (client_position_ != t_client_position_) {
        double _trade_price_ = p_this_order_->price();  ///< crude approximation ... this value is only needed to build
        /// the trades file in case we missed an execution. And hence
        /// this won't really change anything if it is approximate
        AdjustPosition(t_client_position_, _trade_price_, p_this_order_->int_price());
      }

      if (p_this_order_) {
        InitialQueueBid(p_this_order_);
      }

      NotifyOrderChangeListeners();
      if (use_modify_logic_) AdjustModifiableBidIndices('P', bid_index_);
    } else {  // ASK order
      int ask_index_ = GetAskIndexAndAdjustIntPrice(r_int_price_);

      /* On Confirm order size is moved from intpx_2_sum_ask_unconfirmed_ to intpx_2_sum_ask_confirmed_ */
      /* fetch order in intpx_2_ask_order_vec_ [ r_int_price_ ] */

      /* search in sequenced orders at the price sent by ORS */
      std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[ask_index_];
      if (!_this_base_order_vec_.empty()) {
        for (std::vector<BaseOrder*>::reverse_iterator _base_order_vec_rev_iter_ = _this_base_order_vec_.rbegin();
             _base_order_vec_rev_iter_ != _this_base_order_vec_.rend();
             _base_order_vec_rev_iter_++) { /* perhaps reverse search should be more optimal since newer orders are
                                               towards the end */
          if ((*_base_order_vec_rev_iter_)->server_assigned_order_sequence() ==
              t_server_assigned_order_sequence_) {  // Found the order
            p_this_order_ = *_base_order_vec_rev_iter_;
            if (p_this_order_->order_status() != kORRType_Seqd) {
              /// found order and it was already confirmed .. should only happen for Replay callbacks
              /// can be made more robust to handle partial execution misses - TODO
              return;
            }
            break;
          }
        }
      }

      if (p_this_order_ == nullptr) {  // If not found among sequenced orders at given price level ...
        // search for order among unsequenced orders ( in case we missed the sequence )
        for (std::vector<BaseOrder*>::iterator base_order_vec_iter_ = unsequenced_asks_.begin();
             base_order_vec_iter_ != unsequenced_asks_.end(); base_order_vec_iter_++) {
          if ((*base_order_vec_iter_)->client_assigned_order_sequence() ==
              _client_assigned_order_sequence_) { /* found order */
            p_this_order_ = (*base_order_vec_iter_);

            p_this_order_->SequenceAtTime(watch_.msecs_from_midnight(), watch_.tv().tv_usec);
            p_this_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;

            if (_size_remaining_ > 0) {  // if order still exists and add it to map // should have been added when SEQ
                                         // was sent but we missed the SEQ
              ask_order_vec_[ask_index_].push_back(*base_order_vec_iter_);
              AdjustTopBottomOrderVecAskIndexes(ask_index_);
            }
            unsequenced_asks_.erase(base_order_vec_iter_);
            break;
          }
        }
      }

      if (p_this_order_ == nullptr) {  // If not found among sequenced orders at the given int_price level
        // and not found among unsequenced orders
        // error
        if (dbglogger_.CheckLoggingLevel(OM_ERROR) && !dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
          dbglogger_ << watch_.tv() << " ERROR BaseOrderManager::OrderConfirmed got CAOS "
                     << _client_assigned_order_sequence_ << " SAOS " << t_server_assigned_order_sequence_
                     << " int_price " << r_int_price_ << " buysell " << t_buysell_ << '\n';
          dbglogger_.CheckToFlushBuffer();
        }

        if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {  // Create an order and place it in the maps
          BaseOrder* p_new_order_ = baseorder_mempool_.Alloc();

          p_new_order_->security_name_ = dep_symbol_;
          p_new_order_->buysell_ = t_buysell_;
          p_new_order_->price_ = _price_;
          p_new_order_->size_remaining_ = _size_remaining_;
          p_new_order_->size_requested_ = _size_remaining_;
          p_new_order_->int_price_ = r_int_price_;
          p_new_order_->order_status_ = kORRType_Conf;

          p_new_order_->queue_size_ahead_ = 0;
          p_new_order_->queue_size_behind_ = 0;
          p_new_order_->num_events_seen_ = 0;
          p_new_order_->placed_msecs_ = 0;

          p_new_order_->client_assigned_order_sequence_ = _client_assigned_order_sequence_;
          p_new_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;

          p_new_order_->size_executed_ = 0;
          p_new_order_->canceled_ = false;
          p_new_order_->replayed_ = false;
          // p_new_order_->placed_at_level_indicator_ = placed_at_level_indicator_;

          ask_order_vec_[ask_index_].push_back(p_new_order_);
          AdjustTopBottomOrderVecAskIndexes(ask_index_);

          p_this_order_ = p_new_order_;
        } else {
          return;
        }
      }
      // @ ramkris: reducing the unconfirmed orders
      num_unconfirmed_orders_--;

      sum_ask_unconfirmed_[ask_index_] =
          std::max(0, sum_ask_unconfirmed_[ask_index_] - p_this_order_->size_requested());
      sum_ask_unconfirmed_orders_[ask_index_] = std::max(0, sum_ask_unconfirmed_orders_[ask_index_] - 1);
      PropagateUnconfirmedSizeDifference(ask_index_, -p_this_order_->size_requested(), kTradeTypeSell);
      AdjustTopBottomUnconfirmedAskIndexes(ask_index_);

      if (_size_remaining_ > 0) {
        p_this_order_->ConfirmAtTime(watch_.msecs_from_midnight());
        p_this_order_->ConfirmNewSize(_size_remaining_);

        // add to sum_ask_confirmed_ vector
        sum_ask_confirmed_[ask_index_] += _size_remaining_;
        sum_ask_confirmed_orders_[ask_index_] += 1;
        PropagateConfirmedSizeDifference(ask_index_, _size_remaining_, kTradeTypeSell);
        AdjustTopBottomConfirmedAskIndexes(ask_index_);
      } else {
        RemoveOrderFromVec(ask_order_vec_[ask_index_], p_this_order_);
        AdjustTopBottomOrderVecAskIndexes(ask_index_);

        baseorder_mempool_.DeAlloc(p_this_order_);
      }

      if (client_position_ != t_client_position_) {
        double _trade_price_ = p_this_order_->price();
        AdjustPosition(t_client_position_, _trade_price_, p_this_order_->int_price());
      }

      if (p_this_order_) {
        InitialQueueAsk(p_this_order_);
      }

      NotifyOrderChangeListeners();
      if (use_modify_logic_) AdjustModifiableAskIndices('P', ask_index_);
    }

    if (dbglogger_.CheckLoggingLevel(OM_INFO))  // zero logging
    {
      DBGLOG_TIME_CLASS_FUNC << " SACI: " << t_server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_
                             << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_
                             << " px: " << _price_ << " bs: " << ((t_buysell_ == kTradeTypeBuy) ? "BUY" : "SELL")
                             << " sizeR " << _size_remaining_ << " sizeE " << _size_executed_
                             << " cpos: " << t_client_position_ << " gpos: " << t_global_position_
                             << " intpx: " << r_int_price_ << DBGLOG_ENDL_FLUSH;
      if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO) && p_this_order_ != nullptr)  // zero logging
      {
        dbglogger_ << "SYM: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " Px: " << _price_
                   << " INTPX: " << p_this_order_->int_price() << " BS: " << GetTradeTypeChar(t_buysell_)
                   << " ST: " << watch_.tv() << " DT: "
                   << watch_.tv() + ttime_t(0, p_this_order_->seqd_msecs_ * 1000 + p_this_order_->seqd_usecs_ % 1000) -
                          ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                   << " ORR: " << ToString(kORRType_Conf) << " SAOS: " << t_server_assigned_order_sequence_
                   << " CAOS: " << _client_assigned_order_sequence_ << " CLTPOS: " << t_client_position_
                   << " SACI: " << server_assigned_client_id_ << " GBLPOS: " << t_global_position_
                   << " SIZE: " << _size_remaining_ << " SE: " << 0 << " MsgSeq : " << 0 << " Seq: " << 0
                   << DBGLOG_ENDL_FLUSH;
      }
    }

    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " ShowOrders: " << DBGLOG_ENDL_FLUSH;
      LogFullStatus();
      // DBGLOG_TIME_CLASS_FUNC << " ShowOrders: " << ShowOrders ( ) << DBGLOG_ENDL_FLUSH;
    }
  }

  //    CleanMaps ( );
}

/**
 *
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param t_server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param t_buysell_
 * @param _size_remaining_
 * @param _size_executed_
 * @param t_client_position_
 * @param t_global_position_
 * @param r_int_price_
 * @param server_assigned_message_sequence
 */
void BaseOrderManager::OrderConfCxlReplaced(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int t_server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t t_buysell_, const int _size_remaining_, const int _size_executed_, const int t_client_position_,
    const int t_global_position_, const int r_int_price_, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {  //

  // no dependancy on this so update this first regardless of t_server_assigned_client_id_
  global_position_ = t_global_position_;

  if (t_server_assigned_client_id_ == server_assigned_client_id_) {  //

    received_after_last_sent_ = true;

    if (dbglogger_.CheckLoggingLevel(ORS_DATA_INFO)) {
      DBGLOG_CLASS_FUNC_LINE_INFO << "SACI : " << t_server_assigned_client_id_ << ' '
                                  << "CAOS : " << _client_assigned_order_sequence_ << ' '
                                  << "SAOS : " << t_server_assigned_order_sequence_ << ' '
                                  << "SECID : " << _security_id_ << ' ' << "Px : " << _price_ << ' '
                                  << "BUYSELL : " << t_buysell_ << ' ' << "SR : " << _size_remaining_ << ' '
                                  << "SE : " << _size_executed_ << ' ' << "CP : " << t_client_position_ << ' '
                                  << "GP : " << t_global_position_ << ' ' << "IntPx : " << r_int_price_ << ' '
                                  << "SAMS : " << server_assigned_message_sequence << DBGLOG_ENDL_FLUSH;
    }

    // Reset the counter for exchange rejects tracker
    number_of_consecutive_exchange_rejects_ = 0;

    // Check For Drops
    if (server_assigned_message_sequence > expected_message_sequence_) {
      // let's drop a recovery request now
      GoForPacketRecovery(expected_message_sequence_, server_assigned_message_sequence);
      expected_message_sequence_ = server_assigned_message_sequence + 1;

    } else if (server_assigned_message_sequence == expected_message_sequence_) {  // Move forward

      // It's very important that the expected_message_sequence_ is always correct and in sync
      // potentially incorrect sequence would keep us in loop of recovery
      expected_message_sequence_++;

    } else {
      // We received a lower sequence, it's fine could be over recovered sequence
      if (dropped_sequences_.end() != dropped_sequences_.find(server_assigned_message_sequence)) {
        dropped_sequences_.erase(server_assigned_message_sequence);
      }
    }

    BaseOrder* p_this_order_ = nullptr; /* will stay nullptr till order found */
    if (t_buysell_ == kTradeTypeBuy) {  // BID order

      int bid_index_ = GetBidIndexAndAdjustIntPrice(r_int_price_);

      /* search in sequenced orders at the price sent by ORS */
      /* fetch order in intpx_2_bid_order_vec_ [ r_int_price_ ] */
      if (order_vec_top_bid_index_ != -1) {
        for (int index_ = order_vec_top_bid_index_; index_ >= order_vec_bottom_bid_index_; index_--) {
          std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];

          if (!_this_base_order_vec_.empty()) {
            auto _base_order_vec_iter_ = _this_base_order_vec_.begin();
            while (_base_order_vec_iter_ != _this_base_order_vec_.end()) {
              if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
                  t_server_assigned_order_sequence_) {  // Found the order
                p_this_order_ = (*_base_order_vec_iter_);

                // assign the new saos
                p_this_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;

                switch (p_this_order_->order_status()) {  // depending on the last status that we had seen of this
                                                          // order, the maps or data structures from which we need to
                                                          // remove / modify this order changes
                  case kORRType_Seqd: {
                    // would have missed the first confirm ( before modify called )
                    // Logic outside this switch would take care of this case
                    _base_order_vec_iter_++;
                  } break;
                  case kORRType_Conf: {
                    // most likely case

                    // remove earlier size and update order's new size
                    sum_bid_confirmed_[index_] =
                        std::max(0, sum_bid_confirmed_[index_] - p_this_order_->size_remaining());
                    sum_bid_confirmed_orders_[index_] = std::max(0, sum_bid_confirmed_orders_[index_] - 1);
                    PropagateConfirmedSizeDifference(index_, -p_this_order_->size_remaining(), kTradeTypeBuy);
                    sum_bid_sizes_ -= p_this_order_->size_remaining();
                    AdjustTopBottomConfirmedBidIndexes(index_);
                    // update order size
                    p_this_order_->size_remaining_ = _size_remaining_;

                    // change bid confirmed as per new size
                    sum_bid_confirmed_[bid_index_] =
                        std::max(0, sum_bid_confirmed_[bid_index_] + p_this_order_->size_remaining());
                    sum_bid_confirmed_orders_[bid_index_] = std::max(0, sum_bid_confirmed_orders_[bid_index_] + 1);
                    PropagateConfirmedSizeDifference(bid_index_, p_this_order_->size_remaining(), kTradeTypeBuy);
                    sum_bid_sizes_ += p_this_order_->size_remaining();
                    AdjustTopBottomConfirmedBidIndexes(bid_index_);

                    // Decrease in unconfirmed size at new price
                    sum_bid_unconfirmed_[bid_index_] =
                        std::max(0, sum_bid_unconfirmed_[bid_index_] - p_this_order_->size_remaining());
                    sum_bid_unconfirmed_orders_[bid_index_] = std::max(0, sum_bid_unconfirmed_orders_[bid_index_] - 1);
                    PropagateUnconfirmedSizeDifference(bid_index_, -p_this_order_->size_remaining(), kTradeTypeBuy);
                    AdjustTopBottomUnconfirmedBidIndexes(bid_index_);

                    p_this_order_->modified_ = false;
                    p_this_order_->ConfirmAtTime(watch_.msecs_from_midnight());

                    // Setting it invalid, showing  order-modified has been confirmed
                    p_this_order_->modified_new_size_ = -1;

                    if (p_this_order_->int_price() != r_int_price_) {
                      // Price too was modified
                      p_this_order_->price_ = _price_;
                      p_this_order_->int_price_ = r_int_price_;

                      _this_base_order_vec_.erase(_base_order_vec_iter_);
                      AdjustTopBottomOrderVecBidIndexes(index_);
                      bid_order_vec_[bid_index_].push_back(p_this_order_);
                      AdjustTopBottomOrderVecBidIndexes(bid_index_);
                      if (use_modify_logic_) AdjustModifiableBidIndices('C', bid_index_);
                    } else {
                      _base_order_vec_iter_++;
                    }

                    return;  // We are done modifying the order

                  } break;
                  default: { _base_order_vec_iter_++; }
                }
                break;
              } else {
                _base_order_vec_iter_++;
              }
            }
          }
        }
      }
      if (p_this_order_ == nullptr) {  // If not found among sequenced orders at given price level ...
        // search for order in unsequenced_bids_
        for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = unsequenced_bids_.begin();
             _base_order_vec_iter_ != unsequenced_bids_.end(); _base_order_vec_iter_++) {
          if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
              t_server_assigned_order_sequence_) { /* found order */
            p_this_order_ = (*_base_order_vec_iter_);

            p_this_order_->SequenceAtTime(watch_.msecs_from_midnight(), watch_.tv().tv_usec);
            p_this_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;

            unsequenced_bids_.erase(_base_order_vec_iter_);

            break;
          }
        }
      }

      // will only reach here if order was not confirmed
      num_unconfirmed_orders_--;  ///< decrement number of unconfirmed orders

      sum_bid_unconfirmed_[bid_index_] =
          std::max(0, sum_bid_unconfirmed_[bid_index_] - p_this_order_->size_requested());
      sum_bid_unconfirmed_orders_[bid_index_] = std::max(0, sum_bid_unconfirmed_orders_[bid_index_] - 1);
      PropagateUnconfirmedSizeDifference(bid_index_, -p_this_order_->size_requested(), kTradeTypeBuy);
      AdjustTopBottomUnconfirmedBidIndexes(bid_index_);

      // remove older size
      sum_bid_sizes_ -= p_this_order_->size_requested();

      if (_size_remaining_ > 0) {  // order is stil live so confirm it
        p_this_order_->ConfirmAtTime(watch_.msecs_from_midnight());
        p_this_order_->ConfirmNewSize(_size_remaining_);  // set both size_requested_ and size_remaining_ to this value

        sum_bid_confirmed_[bid_index_] += _size_remaining_;
        PropagateConfirmedSizeDifference(bid_index_, _size_remaining_, kTradeTypeBuy);

        // Add new size
        sum_bid_sizes_ += p_this_order_->size_remaining();

        AdjustTopBottomConfirmedBidIndexes(bid_index_);
      } else {
        RemoveOrderFromVec(bid_order_vec_[bid_index_], p_this_order_);
        AdjustTopBottomOrderVecBidIndexes(bid_index_);

        baseorder_mempool_.DeAlloc(p_this_order_);
      }

      if (client_position_ != t_client_position_) {
        double _trade_price_ = p_this_order_->price();  ///< crude approximation ... this value is only needed to build
        /// the trades file in case we missed an execution. And hence
        /// this won't really change anything if it is approximate
        AdjustPosition(t_client_position_, _trade_price_, p_this_order_->int_price());
      }

      if (p_this_order_) {
        InitialQueueBid(p_this_order_);
      }
    }       // trade_type buy
    else {  // ASK order
      int ask_index_ = GetAskIndexAndAdjustIntPrice(r_int_price_);

      /* search in sequenced orders at the price sent by ORS */
      /* fetch order in intpx_2_bid_order_vec_ [ r_int_price_ ] */
      if (order_vec_top_ask_index_ != -1) {
        for (int index_ = order_vec_top_ask_index_; index_ >= order_vec_bottom_ask_index_; index_--) {
          std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[index_];

          if (!_this_base_order_vec_.empty()) {
            auto _base_order_vec_iter_ = _this_base_order_vec_.begin();
            while (_base_order_vec_iter_ != _this_base_order_vec_.end()) {
              if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
                  t_server_assigned_order_sequence_) {  // Found the order
                p_this_order_ = (*_base_order_vec_iter_);

                // assign the new saos
                p_this_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;

                switch (p_this_order_->order_status()) {  // depending on the last status that we had seen of this
                                                          // order, the maps or data structures from which we need to
                                                          // remove / modify this order changes
                  case kORRType_Seqd: {
                    // would have missed the first confirm ( before modify called )
                    // Logic outside this switch would take care of this case
                    _base_order_vec_iter_++;
                  } break;
                  case kORRType_Conf: {
                    // remove earlier size and update order's new size
                    sum_ask_confirmed_[index_] =
                        std::max(0, sum_ask_confirmed_[index_] - p_this_order_->size_remaining());
                    sum_ask_confirmed_orders_[index_] = std::max(0, sum_ask_confirmed_orders_[index_] - 1);
                    PropagateConfirmedSizeDifference(index_, -p_this_order_->size_remaining(), kTradeTypeSell);

                    sum_ask_sizes_ -= p_this_order_->size_remaining();
                    AdjustTopBottomConfirmedAskIndexes(index_);
                    p_this_order_->size_remaining_ = _size_remaining_;

                    // change ask confirmed as per new size
                    sum_ask_confirmed_[ask_index_] =
                        std::max(0, sum_ask_confirmed_[ask_index_] + p_this_order_->size_remaining());
                    sum_ask_confirmed_[ask_index_] = std::max(0, sum_ask_confirmed_[ask_index_] + 1);
                    PropagateConfirmedSizeDifference(ask_index_, p_this_order_->size_remaining(), kTradeTypeSell);
                    sum_ask_sizes_ += p_this_order_->size_remaining();
                    AdjustTopBottomConfirmedAskIndexes(ask_index_);

                    // Decrease in unconfirmed size at new price
                    sum_ask_unconfirmed_[ask_index_] =
                        std::max(0, sum_ask_unconfirmed_[ask_index_] - p_this_order_->size_remaining());
                    sum_ask_unconfirmed_[ask_index_] = std::max(0, sum_ask_unconfirmed_[ask_index_] - 1);
                    PropagateUnconfirmedSizeDifference(ask_index_, -p_this_order_->size_remaining(), kTradeTypeSell);

                    AdjustTopBottomUnconfirmedAskIndexes(ask_index_);

                    p_this_order_->modified_ = false;
                    p_this_order_->ConfirmAtTime(watch_.msecs_from_midnight());
                    // Setting it invalid, showing  order-modified has been confirmed
                    p_this_order_->modified_new_size_ = -1;

                    if (p_this_order_->int_price() != r_int_price_) {
                      p_this_order_->price_ = _price_;
                      p_this_order_->int_price_ = r_int_price_;

                      _this_base_order_vec_.erase(_base_order_vec_iter_);
                      AdjustTopBottomOrderVecAskIndexes(index_);
                      ask_order_vec_[ask_index_].push_back(p_this_order_);
                      AdjustTopBottomOrderVecAskIndexes(ask_index_);
                      if (use_modify_logic_) AdjustModifiableAskIndices('C', ask_index_);
                    } else {
                      _base_order_vec_iter_++;
                    }

                    return;  // We are done modifying it as it was already confirmed

                  } break;
                  default: { _base_order_vec_iter_++; }
                }
                break;
              } else {
                _base_order_vec_iter_++;
              }
            }
          }
        }
      }
      if (p_this_order_ == nullptr) {  // If not found among sequenced orders at given price level ...
        // search for order in unsequenced_bids_
        for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = unsequenced_asks_.begin();
             _base_order_vec_iter_ != unsequenced_asks_.end(); _base_order_vec_iter_++) {
          if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
              t_server_assigned_order_sequence_) { /* found order */
            p_this_order_ = (*_base_order_vec_iter_);

            p_this_order_->SequenceAtTime(watch_.msecs_from_midnight(), watch_.tv().tv_usec);
            p_this_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;

            unsequenced_asks_.erase(_base_order_vec_iter_);

            break;
          }
        }
      }

      // Probably need better for sum_sizes as they would be incorrect here
      // We will only reach here if the order was not confirmed

      num_unconfirmed_orders_--;  ///< decrement number of unconfirmed orders

      sum_ask_unconfirmed_[ask_index_] =
          std::max(0, sum_ask_unconfirmed_[ask_index_] - p_this_order_->size_requested());
      sum_ask_unconfirmed_[ask_index_] = std::max(0, sum_ask_unconfirmed_[ask_index_] - 1);
      PropagateUnconfirmedSizeDifference(ask_index_, -p_this_order_->size_requested(), kTradeTypeSell);
      AdjustTopBottomUnconfirmedAskIndexes(ask_index_);

      // remove older size
      sum_ask_sizes_ -= p_this_order_->size_requested();

      if (_size_remaining_ > 0) {  // order is stil live so confirm it
        p_this_order_->ConfirmAtTime(watch_.msecs_from_midnight());
        p_this_order_->ConfirmNewSize(_size_remaining_);  // set both size_requested_ and size_remaining_ to this value

        sum_ask_confirmed_[ask_index_] += _size_remaining_;
        sum_ask_confirmed_orders_[ask_index_] += 1;
        PropagateConfirmedSizeDifference(ask_index_, _size_remaining_, kTradeTypeSell);
        p_this_order_->price_ = _price_;
        p_this_order_->int_price_ = r_int_price_;

        // Add new size
        sum_ask_sizes_ += p_this_order_->size_remaining();

        AdjustTopBottomConfirmedAskIndexes(ask_index_);
      } else {
        RemoveOrderFromVec(ask_order_vec_[ask_index_], p_this_order_);
        AdjustTopBottomOrderVecAskIndexes(ask_index_);

        baseorder_mempool_.DeAlloc(p_this_order_);
      }

      if (client_position_ != t_client_position_) {
        double _trade_price_ = p_this_order_->price();  ///< crude approximation ... this value is only needed to build
        /// the trades file in case we missed an execution. And hence
        /// this won't really change anything if it is approximate
        AdjustPosition(t_client_position_, _trade_price_, p_this_order_->int_price());
      }

      if (p_this_order_) {
        InitialQueueAsk(p_this_order_);
      }
    }

    NotifyOrderChangeListeners();

    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " SACI: " << t_server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_
                             << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_
                             << " px: " << _price_ << " bs: " << ((t_buysell_ == kTradeTypeBuy) ? "BUY" : "SELL")
                             << " sizeR " << _size_remaining_ << " sizeE " << _size_executed_
                             << " cpos: " << t_client_position_ << " gpos: " << t_global_position_
                             << " intpx: " << r_int_price_ << DBGLOG_ENDL_FLUSH;
    }

    if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO) && p_this_order_ != nullptr)  // zero logging
    {
      dbglogger_ << "SYM: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " Px: " << _price_
                 << " INTPX: " << p_this_order_->int_price() << " BS: " << GetTradeTypeChar(t_buysell_)
                 << " ST: " << watch_.tv() << " DT: "
                 << watch_.tv() + ttime_t(0, p_this_order_->seqd_msecs_ * 1000 + p_this_order_->seqd_usecs_ % 1000) -
                        ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                 << " ORR: " << ToString(kORRType_CxRe) << " SAOS: " << t_server_assigned_order_sequence_
                 << " CAOS: " << _client_assigned_order_sequence_ << " CLTPOS: " << t_client_position_
                 << " SACI: " << server_assigned_client_id_ << " GBLPOS: " << t_global_position_
                 << " SIZE: " << _size_remaining_ << " SE: " << p_this_order_->size_executed_ << " MsgSeq : " << 0
                 << " Seq: " << 0 << DBGLOG_ENDL_FLUSH;
    }

    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " ShowOrders: " << DBGLOG_ENDL_FLUSH;
      LogFullStatus();
      // DBGLOG_TIME_CLASS_FUNC
      // 	   << " ShowOrders: " << ShowOrders ( ) << DBGLOG_ENDL_FLUSH;
    }
  }

  //    CleanMaps ( );
}

/**
 *
 * @param _server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param _server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param _buysell_
 * @param _size_remaining_
 * @param _client_position_
 * @param _global_position_
 * @param _int_price_
 * @param rejection_reason
 * @param server_assigned_message_sequence
 */
void BaseOrderManager::OrderConfCxlReplaceRejected(
    const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t _buysell_, const int _size_remaining_, const int _client_position_, const int _global_position_,
    const int _int_price_, const int32_t rejection_reason, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  if (_server_assigned_client_id_ == server_assigned_client_id_) {  //

    received_after_last_sent_ = true;

    if (dbglogger_.CheckLoggingLevel(ORS_DATA_INFO)) {
      DBGLOG_CLASS_FUNC_LINE_INFO << "SACI : " << _server_assigned_client_id_ << ' '
                                  << "CAOS : " << _client_assigned_order_sequence_ << ' '
                                  << "SAOS : " << _server_assigned_order_sequence_ << ' ' << "SECID : " << _security_id_
                                  << ' ' << "Px : " << _price_ << ' ' << "BUYSELL : " << _buysell_ << ' '
                                  << "SR : " << _size_remaining_ << ' ' << "CP : " << _client_position_ << ' '
                                  << "GP : " << _global_position_ << ' ' << "IntPx : " << _int_price_ << ' '
                                  << "Rej : " << HFSAT::CancelReplaceRejectReasonStr(
                                                     HFSAT::CxlReplaceRejectReason_t(rejection_reason)) << ' '
                                  << "SAMS : " << server_assigned_message_sequence << DBGLOG_ENDL_FLUSH;
    }

    // Start/Increment Counter
    if (HFSAT::kExchCancelReplaceReject == (HFSAT::CxlReplaceRejectReason_t)rejection_reason) {
      number_of_consecutive_exchange_rejects_++;
      time_based_total_exchange_rejects_++;
      total_exchange_rejects_++;
    }

    // Special handling considered for auto freeze
    if (HFSAT::kORSCxReRejectSecurityNotFound == (HFSAT::CxlReplaceRejectReason_t)rejection_reason) {
      NotifyExchangeRejectsListeners(HFSAT::BaseUtils::FreezeEnforcedReason::kFreezeOnORSSecNotFound);
      // If we've not reset the rejects and we've breached threshold
      // our freeze timeout starts now
      if (0 == reject_based_freeze_timer_) {
        reject_based_freeze_timer_ = watch_.msecs_from_midnight();
      }
    }

    HFSAT::BaseUtils::FreezeEnforcedReason freeze_enforcement = query_trading_auto_freeze_manager_.ShouldAllowOrders(
        number_of_consecutive_exchange_rejects_, time_based_total_exchange_rejects_, total_exchange_rejects_);

    if (HFSAT::BaseUtils::FreezeEnforcedReason::kAllow != freeze_enforcement) {
      NotifyExchangeRejectsListeners(freeze_enforcement);

      // If we've not reset the rejects and we've breached threshold
      // our freeze timeout starts now
      if (0 == reject_based_freeze_timer_) {
        reject_based_freeze_timer_ = watch_.msecs_from_midnight();
      }
    }

    BaseOrder* p_this_order_ = nullptr; /* will stay nullptr till order found */
    /// Here we assume that the price received is the old price not the new one
    if (_buysell_ == kTradeTypeBuy) {  // BID order
      // get index corresponding to int price
      int bid_index = GetBidIndexAndAdjustIntPrice(_int_price_);
      if (order_vec_top_bid_index_ != -1) {
        for (auto order : bid_order_vec_[bid_index]) {
          if (order->client_assigned_order_sequence() == _client_assigned_order_sequence_) {
            p_this_order_ = order;
            // Set the modified state to false and revert the unconfirmed sizes
            order->modified_ = false;
            int new_bid_index = GetBidIndexAndAdjustIntPrice(order->modified_new_int_price_);
            sum_bid_unconfirmed_[GetBidIndexAndAdjustIntPrice(order->modified_new_int_price_)] =
                std::max(0, sum_bid_unconfirmed_[GetBidIndexAndAdjustIntPrice(order->modified_new_int_price_)] -
                                order->modified_new_size_);
            // here we are reducing the number of unconfirmed orders ;
            sum_bid_unconfirmed_orders_[new_bid_index] = std::max(0, sum_bid_unconfirmed_orders_[new_bid_index] - 1);
            PropagateUnconfirmedSizeDifference(new_bid_index, -order->modified_new_size_, kTradeTypeBuy);
            AdjustTopBottomUnconfirmedBidIndexes(new_bid_index);
            order->modified_new_size_ = -1;
            break;
          }
        }
      }

    }       // trade_type buy
    else {  // ASK order

      // Assuming the price passed is older one, we should get the order at this price
      int ask_index = GetAskIndexAndAdjustIntPrice(_int_price_);  // Even not adjusting should be fine here
      if (order_vec_top_ask_index_ != -1) {
        for (auto order : ask_order_vec_[ask_index]) {
          if (order->client_assigned_order_sequence() == _client_assigned_order_sequence_) {
            p_this_order_ = order;
            // Set modified state false and revert the unconfirmed sizes
            order->modified_ = false;
            sum_ask_unconfirmed_[GetAskIndexAndAdjustIntPrice(order->modified_new_int_price_)] =
                std::max(0, sum_ask_unconfirmed_[GetAskIndexAndAdjustIntPrice(order->modified_new_int_price_)] -
                                order->modified_new_size_);
            int new_ask_index = GetAskIndexAndAdjustIntPrice(order->modified_new_int_price_);
            sum_ask_unconfirmed_orders_[new_ask_index] = std::max(0, sum_ask_unconfirmed_orders_[new_ask_index] - 1);
            PropagateUnconfirmedSizeDifference(new_ask_index, -order->modified_new_size_, kTradeTypeSell);
            AdjustTopBottomUnconfirmedAskIndexes(new_ask_index);
            order->modified_new_size_ = -1;
          }
        }
      }
    }

    NotifyOrderChangeListeners();

    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " SACI: " << _server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_
                             << " SAOS: " << _server_assigned_order_sequence_ << " sid: " << _security_id_
                             << " px: " << _price_ << " bs: " << ((_buysell_ == kTradeTypeBuy) ? "BUY" : "SELL")
                             << " sizeR " << _size_remaining_ << " cpos: " << _client_position_
                             << " gpos: " << _global_position_ << " intpx: " << _int_price_ << DBGLOG_ENDL_FLUSH;
    }
    if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO) && p_this_order_ != nullptr)  // zero logging
    {
      dbglogger_ << "SYM: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " Px: " << _price_
                 << " INTPX: " << _int_price_ << " BS: " << GetTradeTypeChar(_buysell_) << " ST: " << watch_.tv()
                 << " DT: "
                 << watch_.tv() + ttime_t(0, p_this_order_->seqd_msecs_ * 1000 + p_this_order_->seqd_usecs_ % 1000) -
                        ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                 << " ORR: " << ToString(kORRType_CxReRejc) << " SAOS: " << _server_assigned_order_sequence_
                 << " CAOS: " << _client_assigned_order_sequence_ << " CLTPOS: " << _client_position_
                 << " SACI: " << server_assigned_client_id_ << " GBLPOS: " << _global_position_
                 << " SIZE: " << _size_remaining_ << " SE: " << p_this_order_->size_executed_ << " MsgSeq : " << 0
                 << " Seq: " << 0 << DBGLOG_ENDL_FLUSH;
    }
    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " ShowOrders: " << DBGLOG_ENDL_FLUSH;
      LogFullStatus();
    }
  }
}

void BaseOrderManager::FakeAllOrsReply(){
    // Assuming only one order will be there to cancel,, as only one order is alive
    // will always go in unseq to check order exist 
    ttime_t tt;
    int server_assigned_order_sequence_ = 0;
    if (last_cancel_caos != -1){
      // for seq order
        OrderCanceled(
                server_assigned_client_id_, last_cancel_caos,
                server_assigned_order_sequence_, dep_security_id_, -1, last_order_buysell,
                 0, client_position_, global_position_,
                -1, expected_message_sequence_, 0, tt);
            DBGLOG_CLASS_FUNC_LINE_INFO << "BaseOrderManager Sending LAST SEQ " << last_cancel_caos << DBGLOG_ENDL_FLUSH;
    }

    for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = unsequenced_bids_.begin();
             _base_order_vec_iter_ != unsequenced_bids_.end(); _base_order_vec_iter_++) {

      DBGLOG_CLASS_FUNC_LINE_INFO << "BaseOrderManager Sending bids Fake reply "<< (*_base_order_vec_iter_)->client_assigned_order_sequence()  << DBGLOG_ENDL_FLUSH;
      OrderCanceled(
                server_assigned_client_id_, (*_base_order_vec_iter_)->client_assigned_order_sequence(),
                server_assigned_order_sequence_, dep_security_id_, /* price */ (*_base_order_vec_iter_)->price(), kTradeTypeBuy,
                 0, client_position_, global_position_,
                (*_base_order_vec_iter_)->int_price(), expected_message_sequence_, 0, tt);
      DBGLOG_CLASS_FUNC_LINE_INFO << "Done Bid Cancel BaseOrderManager  "<< (*_base_order_vec_iter_)->client_assigned_order_sequence()  << DBGLOG_ENDL_FLUSH;
      break; // only one order should exist
    }
    // send for both buy and sell 
    for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = unsequenced_asks_.begin();
             _base_order_vec_iter_ != unsequenced_asks_.end(); _base_order_vec_iter_++) {
      DBGLOG_CLASS_FUNC_LINE_INFO << "BaseOrderManager Sending asks Fake reply "<< (*_base_order_vec_iter_)->client_assigned_order_sequence()  << DBGLOG_ENDL_FLUSH;
      OrderCanceled(
                server_assigned_client_id_, (*_base_order_vec_iter_)->client_assigned_order_sequence(),
                server_assigned_order_sequence_, dep_security_id_, /* price */ (*_base_order_vec_iter_)->price(), kTradeTypeSell,
                /* size remain */ 0, client_position_, global_position_,
                /* int_price */ (*_base_order_vec_iter_)->int_price(), expected_message_sequence_, /* exchange id */ 0, tt);
      DBGLOG_CLASS_FUNC_LINE_INFO << " Done Ask Cancel BaseOrderManager " << (*_base_order_vec_iter_)->client_assigned_order_sequence()  << DBGLOG_ENDL_FLUSH;
      break; // only one order should exist
    }
}

/**
 *
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param t_server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param t_buysell_
 * @param _size_remaining_
 * @param t_client_position_
 * @param t_global_position_
 * @param r_int_price_
 * @param server_assigned_message_sequence
 * @param exchange_order_id
 * @param time_set_by_server
 */

void BaseOrderManager::OrderCanceled(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                     const int t_server_assigned_order_sequence_, const unsigned int _security_id_,
                                     const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                                     const int t_client_position_, const int t_global_position_, const int r_int_price_,
                                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                     const ttime_t time_set_by_server) {
  global_position_ = t_global_position_;

  if (t_server_assigned_client_id_ == server_assigned_client_id_) {  //

    received_after_last_sent_ = true;

    if (dbglogger_.CheckLoggingLevel(ORS_DATA_INFO)) {
      DBGLOG_CLASS_FUNC_LINE_INFO << "SACI : " << t_server_assigned_client_id_ << ' '
                                  << "CAOS : " << _client_assigned_order_sequence_ << ' '
                                  << "SAOS : " << t_server_assigned_order_sequence_ << ' '
                                  << "SECID : " << _security_id_ << ' ' << "Px : " << _price_ << ' '
                                  << "BUYSELL : " << t_buysell_ << ' ' << "SR : " << _size_remaining_ << ' '
                                  << "CP : " << t_client_position_ << ' ' << "GP : " << t_global_position_ << ' '
                                  << "IntPx : " << r_int_price_ << ' ' << "SAMS : " << server_assigned_message_sequence
                                  << DBGLOG_ENDL_FLUSH;
    }

    // Reset the counter for exchange rejects tracker
    number_of_consecutive_exchange_rejects_ = 0;

    // Check For Drops
    if (server_assigned_message_sequence > expected_message_sequence_) {
      // let's drop a recovery request now
      GoForPacketRecovery(expected_message_sequence_, server_assigned_message_sequence);
      expected_message_sequence_ = server_assigned_message_sequence + 1;

    } else if (server_assigned_message_sequence == expected_message_sequence_) {  // Move forward

      // It's very important that the expected_message_sequence_ is always correct and in sync
      // potentially incorrect sequence would keep us in loop of recovery
      expected_message_sequence_++;

    } else {
      // We received a lower sequence, it's fine could be over recovered sequence
      if (dropped_sequences_.end() != dropped_sequences_.find(server_assigned_message_sequence)) {
        dropped_sequences_.erase(server_assigned_message_sequence);
      }
    }

    // find order
    //   sequenced orders at r_int_price_
    //   unsequenced orders ( matching _client_assigned_order_sequence_ )
    // depending on where we find it remove the effects from the maps
    // check if position needs to be adjusted

    BaseOrder* p_this_order_ = nullptr; /* will stay nullptr till order found */

    if (t_buysell_ == kTradeTypeBuy) {  // BID order
      p_this_order_ = nullptr;          /* will stay nullptr till order found */
      int bid_index_ = GetBidIndexAndAdjustIntPrice(r_int_price_);

      /* search in sequenced orders at the price sent by ORS */
      /* fetch order in intpx_2_bid_order_vec_ [ r_int_price_ ] */
      std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[bid_index_];

      if (!_this_base_order_vec_.empty()) {
        for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
             _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
          if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
              t_server_assigned_order_sequence_) {  // Found the order
            p_this_order_ = (*_base_order_vec_iter_);

            switch (p_this_order_->order_status()) {  // depending on the last status that we had seen of this order,
                                                      // the maps or data structures from which we need to remove /
                                                      // modify this order changes
              case kORRType_Seqd: {  // either missed the confirm or the exec came from the exchange before
                                     // the
                                     // confirm
                num_unconfirmed_orders_--;

                sum_bid_unconfirmed_[bid_index_] =
                    std::max(0, sum_bid_unconfirmed_[bid_index_] - p_this_order_->size_requested());
                sum_bid_unconfirmed_orders_[bid_index_] = std::max(0, sum_bid_unconfirmed_orders_[bid_index_] - 1);
                PropagateUnconfirmedSizeDifference(bid_index_, -p_this_order_->size_requested(), kTradeTypeBuy);
                AdjustTopBottomUnconfirmedBidIndexes(bid_index_);

                sum_bid_sizes_ -= p_this_order_->size_requested();

              } break;
              case kORRType_Conf: {
                sum_bid_confirmed_[bid_index_] =
                    std::max(0, sum_bid_confirmed_[bid_index_] - p_this_order_->size_remaining());
                sum_bid_confirmed_orders_[bid_index_] = std::max(0, sum_bid_confirmed_orders_[bid_index_] - 1);
                PropagateConfirmedSizeDifference(bid_index_, -p_this_order_->size_remaining(), kTradeTypeBuy);
                AdjustTopBottomConfirmedBidIndexes(bid_index_);

                sum_bid_sizes_ -= p_this_order_->size_remaining();
              } break;
              default: {}
            }

            _this_base_order_vec_.erase(_base_order_vec_iter_);  // remove order from vec
            AdjustTopBottomOrderVecBidIndexes(bid_index_);
            if (use_modify_logic_) AdjustModifiableBidIndices('C', bid_index_);

            baseorder_mempool_.DeAlloc(p_this_order_);
           DBGLOG_CLASS_FUNC_LINE_INFO << "CLEARING BID IN SEQ \n"  ;
            break;
          }
        }
      }

      if (p_this_order_ == nullptr) {  // If not found among sequenced orders at given price level ...
        // search for order in unsequenced_bids_
        for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = unsequenced_bids_.begin();
             _base_order_vec_iter_ != unsequenced_bids_.end(); _base_order_vec_iter_++) {
          if ((*_base_order_vec_iter_)->client_assigned_order_sequence() ==
              _client_assigned_order_sequence_) { /* found order */
            p_this_order_ = (*_base_order_vec_iter_);

            p_this_order_->SequenceAtTime(watch_.msecs_from_midnight(), watch_.tv().tv_usec);
            p_this_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;

            num_unconfirmed_orders_--;  ///< decrement number of unconfirmed orders

            sum_bid_unconfirmed_[bid_index_] =
                std::max(0, sum_bid_unconfirmed_[bid_index_] - p_this_order_->size_requested());
            sum_bid_unconfirmed_orders_[bid_index_] = std::max(0, sum_bid_unconfirmed_orders_[bid_index_] - 1);
            PropagateUnconfirmedSizeDifference(bid_index_, -p_this_order_->size_requested(), kTradeTypeBuy);
            AdjustTopBottomUnconfirmedBidIndexes(bid_index_);

            sum_bid_sizes_ -= p_this_order_->size_requested();  

            unsequenced_bids_.erase(_base_order_vec_iter_);
            DBGLOG_CLASS_FUNC_LINE_INFO << "CLEARING BID IN UNSEQ \n"  ;
            break;
          }
        }
      }
    } else {                   // ASK order
      p_this_order_ = nullptr; /* will stay nullptr till order found */
      int ask_index_ = GetAskIndexAndAdjustIntPrice(r_int_price_);

      /* search in sequenced orders at the price sent by ORS */
      /* fetch order in intpx_2_ask_order_vec_ [ r_int_price_ ] */
      std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[ask_index_];
      if (!_this_base_order_vec_.empty()) {
        for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
             _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
          if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
              t_server_assigned_order_sequence_) {  // Found the order
            p_this_order_ = (*_base_order_vec_iter_);

            switch (p_this_order_->order_status()) {  // depending on the last status that we had seen of this order,
                                                      // the maps or data structures from which we need to remove /
                                                      // modify this order changes
              case kORRType_Seqd: {  // either missed the confirm or the exec came from the exchange before
                                     // the
                                     // confirm
                num_unconfirmed_orders_--;

                sum_ask_unconfirmed_[ask_index_] =
                    std::max(0, sum_ask_unconfirmed_[ask_index_] - p_this_order_->size_requested());
                sum_ask_unconfirmed_orders_[ask_index_] = std::max(0, sum_ask_unconfirmed_orders_[ask_index_] - 1);
                PropagateUnconfirmedSizeDifference(ask_index_, -p_this_order_->size_requested(), kTradeTypeSell);
                AdjustTopBottomUnconfirmedAskIndexes(ask_index_);

                sum_ask_sizes_ -= p_this_order_->size_requested();

              } break;
              case kORRType_Conf: {
                sum_ask_confirmed_[ask_index_] =
                    std::max(0, sum_ask_confirmed_[ask_index_] - p_this_order_->size_remaining());
                sum_ask_confirmed_orders_[ask_index_] = std::max(0, sum_ask_confirmed_orders_[ask_index_] - 1);
                PropagateConfirmedSizeDifference(ask_index_, -p_this_order_->size_remaining(), kTradeTypeSell);
                AdjustTopBottomConfirmedAskIndexes(ask_index_);
                sum_ask_sizes_ -= p_this_order_->size_remaining();

              } break;
              default: {}
            }

            _this_base_order_vec_.erase(_base_order_vec_iter_);  // remove order from vec
            AdjustTopBottomOrderVecAskIndexes(ask_index_);
            if (use_modify_logic_) AdjustModifiableAskIndices('C', ask_index_);

            baseorder_mempool_.DeAlloc(p_this_order_);
            DBGLOG_CLASS_FUNC_LINE_INFO << "CLEARING ASK IN SEQ \n"  ;
            break;
          }
        }
      }

      if (p_this_order_ == nullptr) {  // If not found among sequenced orders at given price level ...
        // search for order in unsequenced_asks_
        for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = unsequenced_asks_.begin();
             _base_order_vec_iter_ != unsequenced_asks_.end(); _base_order_vec_iter_++) {
          if ((*_base_order_vec_iter_)->client_assigned_order_sequence() ==
              _client_assigned_order_sequence_) { /* found order */
            p_this_order_ = (*_base_order_vec_iter_);

            p_this_order_->SequenceAtTime(watch_.msecs_from_midnight(), watch_.tv().tv_usec);
            p_this_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;

            num_unconfirmed_orders_--;  ///< decrement number of unconfirmed orders

            sum_ask_unconfirmed_[ask_index_] =
                std::max(0, sum_ask_unconfirmed_[ask_index_] - p_this_order_->size_requested());
            sum_ask_unconfirmed_orders_[ask_index_] = std::max(0, sum_ask_unconfirmed_orders_[ask_index_] - 1);
            PropagateUnconfirmedSizeDifference(ask_index_, -p_this_order_->size_requested(), kTradeTypeSell);
            AdjustTopBottomUnconfirmedAskIndexes(ask_index_);

            sum_ask_sizes_ -= p_this_order_->size_remaining();

            unsequenced_asks_.erase(_base_order_vec_iter_);
            DBGLOG_CLASS_FUNC_LINE_INFO << "CLEARING ASK IN UNSEQ \n"  ;
            break;
          }
        }
      }
    }
    if (client_position_ != t_client_position_) {
      AdjustPosition(t_client_position_, GetMidPrice(), GetIntPx(GetMidPrice()));
    }
    NotifyOrderChangeListeners();

    if (dbglogger_.CheckLoggingLevel(OM_INFO))  // zero logging
    {
      DBGLOG_TIME_CLASS_FUNC << " SACI: " << t_server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_
                             << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_
                             << " px: " << _price_ << " bs: " << ((t_buysell_ == kTradeTypeBuy) ? "BUY" : "SELL")
                             << " sizeR " << _size_remaining_ << " cpos: " << t_client_position_
                             << " gpos: " << t_global_position_ << " intpx: " << r_int_price_ << DBGLOG_ENDL_FLUSH;
      if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO) && p_this_order_ != nullptr)  // zero logging
      {
        dbglogger_ << "SYM: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " Px: " << _price_
                   << " INTPX: " << p_this_order_->int_price() << " BS: " << GetTradeTypeChar(t_buysell_)
                   << " ST: " << watch_.tv() << " DT: "
                   << watch_.tv() + ttime_t(0, p_this_order_->seqd_msecs_ * 1000 + p_this_order_->seqd_usecs_ % 1000) -
                          ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                   << " ORR: " << ToString(kORRType_Cxld) << " SAOS: " << t_server_assigned_order_sequence_
                   << " CAOS: " << _client_assigned_order_sequence_ << " CLTPOS: " << t_client_position_
                   << " SACI: " << server_assigned_client_id_ << " GBLPOS: " << t_global_position_
                   << " SIZE: " << _size_remaining_ << " SE: " << 0 << " MsgSeq : " << 0 << " Seq: " << 0
                   << DBGLOG_ENDL_FLUSH;
      }
    }
    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " ShowOrders: " << DBGLOG_ENDL_FLUSH;
      LogFullStatus();
      // DBGLOG_TIME_CLASS_FUNC
      // 	       << " ShowOrders: " << ShowOrders ( ) << DBGLOG_ENDL_FLUSH;
    }
  }

  //    CleanMaps ( );
}

/**
 *  handle the executions
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param t_server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param t_buysell_
 * @param _size_remaining_
 * @param _size_executed_
 * @param t_client_position_
 * @param t_global_position_
 * @param r_int_price_
 * @param server_assigned_message_sequence
 * @param exchange_order_id
 * @param time_set_by_server
 */
void BaseOrderManager::OrderExecuted(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                     const int t_server_assigned_order_sequence_, const unsigned int _security_id_,
                                     const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                                     const int _size_executed_, const int t_client_position_,
                                     const int t_global_position_, const int r_int_price_,
                                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                     const ttime_t time_set_by_server) {
  bool is_fok_ = false;
  global_position_ = t_global_position_;
  if (dbglogger_.CheckLoggingLevel(FILL_TIME_INFO)) {
    DBGLOG_TIME_CLASS << "EXECUTION " << watch_.msecs_from_midnight() << " " << r_int_price_ << " " << t_buysell_ << " "
                      << _client_assigned_order_sequence_ << DBGLOG_ENDL_FLUSH;
    // std::cout<<"Executed " << watch_.msecs_from_midnight() << " " << _client_assigned_order_sequence_ << std::endl;
  }
  if (t_server_assigned_client_id_ == server_assigned_client_id_) {  //

    received_after_last_sent_ = true;

    if (dbglogger_.CheckLoggingLevel(ORS_DATA_INFO)) {
      DBGLOG_CLASS_FUNC_LINE_INFO << "SACI : " << t_server_assigned_client_id_ << ' '
                                  << "CAOS : " << _client_assigned_order_sequence_ << ' '
                                  << "SAOS : " << t_server_assigned_order_sequence_ << ' '
                                  << "SECID : " << _security_id_ << ' ' << "Px : " << _price_ << ' '
                                  << "BUYSELL : " << t_buysell_ << ' ' << "SR : " << _size_remaining_ << ' '
                                  << "SE : " << _size_executed_ << ' ' << "CP : " << t_client_position_ << ' '
                                  << "GP : " << t_global_position_ << ' ' << "IntPx : " << r_int_price_ << ' '
                                  << "SAMS : " << server_assigned_message_sequence << DBGLOG_ENDL_FLUSH;
    }

    // Reset the counter for exchange rejects tracker
    number_of_consecutive_exchange_rejects_ = 0;

    // Check For Drops
    if (server_assigned_message_sequence > expected_message_sequence_) {
      // let's drop a recovery request now
      GoForPacketRecovery(expected_message_sequence_, server_assigned_message_sequence);
      expected_message_sequence_ = server_assigned_message_sequence + 1;

    } else if (server_assigned_message_sequence == expected_message_sequence_) {  // Move forward

      // It's very important that the expected_message_sequence_ is always correct and in sync
      // potentially incorrect sequence would keep us in loop of recovery
      expected_message_sequence_++;

    } else {
      // We received a lower sequence, it's fine could be over recovered sequence
      if (dropped_sequences_.end() != dropped_sequences_.find(server_assigned_message_sequence)) {
        dropped_sequences_.erase(server_assigned_message_sequence);
      }
    }

    num_self_trades_++;

    if (t_buysell_ == kTradeTypeBuy) {
      BaseOrder* p_this_order_ = nullptr; /* will stay nullptr till order found */
      int bid_index_ = GetBidIndexAndAdjustIntPrice(r_int_price_);

      /* search in sequenced orders at the price sent by ORS */
      /* fetch order in intpx_2_bid_order_vec_ [ r_int_price_ ] */
      std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[bid_index_];

      if (!_this_base_order_vec_.empty()) {
        for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
             _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
          if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
              t_server_assigned_order_sequence_) {  // Found the order
            p_this_order_ = (*_base_order_vec_iter_);

            switch (p_this_order_->order_status()) {  // depending on the last status that we had seen of this order,
                                                      // the maps or data structures from which we need to remove /
                                                      // modify this order changes
              case kORRType_Seqd: {  // either missed the confirm or the exec came from the exchange before
                                     // the
                                     // confirm
                num_unconfirmed_orders_--;

                sum_bid_unconfirmed_[bid_index_] =
                    std::max(0, sum_bid_unconfirmed_[bid_index_] - p_this_order_->size_requested());
                sum_bid_unconfirmed_orders_[bid_index_] = std::max(0, sum_bid_unconfirmed_orders_[bid_index_] - 1);
                PropagateUnconfirmedSizeDifference(bid_index_, -p_this_order_->size_requested(), kTradeTypeBuy);
                AdjustTopBottomUnconfirmedBidIndexes(bid_index_);

                p_this_order_->ConfirmAtTime(watch_.msecs_from_midnight());
                // set both size_requested_ and size_remaining_ to this value
                p_this_order_->ConfirmNewSize(_size_remaining_);
                p_this_order_->Execute(_size_executed_);

                if (_size_remaining_ > 0) {
                  sum_bid_confirmed_[bid_index_] += _size_remaining_;
                  sum_bid_confirmed_orders_[bid_index_] += 1;
                  PropagateConfirmedSizeDifference(bid_index_, _size_remaining_, kTradeTypeBuy);
                  AdjustTopBottomConfirmedBidIndexes(bid_index_);
                } else {  // the order will be deleted and not more maps have to be changed
                  if (p_this_order_->modified_) {
                    // This case should not happen as we sould never send modify for unconfirmed order
                    int new_bid_index = GetBidIndexAndAdjustIntPrice(p_this_order_->modified_new_int_price_);
                    sum_bid_unconfirmed_[new_bid_index] =
                        std::max(0, sum_bid_unconfirmed_[new_bid_index] - p_this_order_->modified_new_size_);
                    sum_bid_unconfirmed_orders_[new_bid_index] =
                        std::max(0, sum_bid_unconfirmed_orders_[new_bid_index] - 1);
                    PropagateUnconfirmedSizeDifference(bid_index_, -p_this_order_->modified_new_size_, kTradeTypeBuy);
                    AdjustTopBottomUnconfirmedBidIndexes(new_bid_index);
                    DBGLOG_TIME_CLASS_FUNC_LINE << " ERROR: Received Bid exec for sequenced and modified order : "
                                                << " exec_px: " << _price_ << " size: " << _size_executed_
                                                << " mod_new_px: " << p_this_order_->modified_new_price_
                                                << " mod_new_sz: " << p_this_order_->modified_new_size_
                                                << DBGLOG_ENDL_FLUSH;
                  }
                }

              } break;
              case kORRType_Conf: {
                sum_bid_confirmed_[bid_index_] =
                    std::max(0, sum_bid_confirmed_[bid_index_] + _size_remaining_ - p_this_order_->size_remaining());
                if (p_this_order_->size_remaining() == 0)
                  sum_bid_confirmed_orders_[bid_index_] = std::max(0, sum_bid_confirmed_orders_[bid_index_] - 1);
                PropagateConfirmedSizeDifference(bid_index_, _size_remaining_ - p_this_order_->size_remaining(),
                                                 kTradeTypeBuy);
                AdjustTopBottomConfirmedBidIndexes(bid_index_);

                // set both size_requested_ and size_remaining_ to this value
                p_this_order_->ConfirmNewSize(_size_remaining_);
                p_this_order_->Execute(_size_executed_);

                if (p_this_order_->modified_) {
                  // We don't modify values for confirmed order unless we get Cancel Replace confitmation. Here we got
                  // exec before confirmation,
                  // hence we need to modify the value of confirmed orders.
                  int new_bid_index = GetBidIndexAndAdjustIntPrice(p_this_order_->modified_new_int_price_);
                  sum_bid_unconfirmed_[new_bid_index] =
                      std::max(0, sum_bid_unconfirmed_[new_bid_index] - p_this_order_->modified_new_size_);
                  sum_bid_unconfirmed_orders_[new_bid_index] =
                      std::max(0, sum_bid_unconfirmed_orders_[new_bid_index] - 1);
                  PropagateUnconfirmedSizeDifference(new_bid_index, -p_this_order_->modified_new_size_, kTradeTypeBuy);
                  AdjustTopBottomUnconfirmedBidIndexes(new_bid_index);
                  DBGLOG_TIME_CLASS_FUNC_LINE << " RARE: Received Bid exec before ModifyConfirm : "
                                              << " exec_px: " << _price_ << " size: " << _size_executed_
                                              << " mod_new_px: " << p_this_order_->modified_new_price_
                                              << " mod_new_sz: " << p_this_order_->modified_new_size_
                                              << DBGLOG_ENDL_FLUSH;
                }

                if (use_modify_logic_) AdjustModifiableBidIndices('C', bid_index_);

              } break;
              default: {}
            }

            if (p_this_order_ != nullptr) {
              if (dbglogger_.CheckLoggingLevel(OM_INFO))  // zero logging
              {
                DBGLOG_TIME_CLASS_FUNC << GetTradeTypeChar(p_this_order_->buysell()) << " " << p_this_order_->price()
                                       << " int(" << p_this_order_->int_price() << ")"
                                       << " " << p_this_order_->size_remaining() << " "
                                       << ToString(p_this_order_->order_status())
                                       << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                                       << " SAOS: " << p_this_order_->server_assigned_order_sequence()
                                       << " sA: " << p_this_order_->queue_size_ahead()
                                       << " sB: " << p_this_order_->queue_size_behind() << " msecs_since_seqd: "
                                       << ((p_this_order_->seqd_msecs_ >= 0)
                                               ? (watch_.msecs_from_midnight() - p_this_order_->seqd_msecs_)
                                               : (0)) << " msecs_since_conf: "
                                       << ((p_this_order_->placed_msecs_ >= 0)
                                               ? (watch_.msecs_from_midnight() - p_this_order_->placed_msecs_)
                                               : (0)) << DBGLOG_ENDL_FLUSH;
                if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO) && p_this_order_ != nullptr)  // zero logging
                {
                  if (p_this_order_ == nullptr) {
                    BaseOrder temp;
                    p_this_order_ = &temp;
                  }
                  dbglogger_ << "SYM: " << p_this_order_->security_name_ << " Px: " << p_this_order_->price()
                             << " INTPX: " << p_this_order_->int_price()
                             << " BS: " << GetTradeTypeChar(p_this_order_->buysell()) << " ST: " << watch_.tv()
                             << " DT: "
                             << watch_.tv() +
                                    ttime_t(0, p_this_order_->seqd_msecs_ * 1000 + p_this_order_->seqd_usecs_ % 1000) -
                                    ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                             << " ORR: " << ToString(kORRType_Exec)
                             << " SAOS: " << p_this_order_->server_assigned_order_sequence()
                             << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                             << " CLTPOS: " << t_client_position_ << " SACI: " << server_assigned_client_id_
                             << " GBLPOS: " << t_global_position_ << " SIZE: " << p_this_order_->size_remaining()
                             << " SE: " << p_this_order_->size_executed() << " MsgSeq : " << 0 << " Seq: " << 0
                             << DBGLOG_ENDL_FLUSH;
                }
              }
              NoteLevelExec(p_this_order_->placed_at_level_indicator_);
              is_fok_ = p_this_order_->is_fok_;
            }

            // Update active bid
            sum_bid_sizes_ -= _size_executed_;
            if (_size_remaining_ <= 0) {
              _this_base_order_vec_.erase(_base_order_vec_iter_);  // remove order from vec
              AdjustTopBottomOrderVecBidIndexes(bid_index_);

              baseorder_mempool_.DeAlloc(p_this_order_);
            }
            break;
          }
        }
      }

      if (p_this_order_ == nullptr) {  // If not found among sequenced orders at given price level ...
        // search for order in unsequenced_bids_
        for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = unsequenced_bids_.begin();
             _base_order_vec_iter_ != unsequenced_bids_.end(); _base_order_vec_iter_++) {
          if ((*_base_order_vec_iter_)->client_assigned_order_sequence() ==
              _client_assigned_order_sequence_) { /* found order */
            p_this_order_ = (*_base_order_vec_iter_);

            p_this_order_->SequenceAtTime(watch_.msecs_from_midnight(), watch_.tv().tv_usec);
            p_this_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;

            num_unconfirmed_orders_--;  ///< decrement number of unconfirmed orders

            sum_bid_unconfirmed_[bid_index_] =
                std::max(0, sum_bid_unconfirmed_[bid_index_] - p_this_order_->size_requested());
            sum_bid_unconfirmed_orders_[bid_index_] = std::max(0, sum_bid_unconfirmed_orders_[bid_index_] - 1);
            PropagateUnconfirmedSizeDifference(bid_index_, -p_this_order_->size_requested(), kTradeTypeBuy);
            AdjustTopBottomUnconfirmedBidIndexes(bid_index_);

            if (_size_remaining_ > 0) {  // order is stil live so confirm it
              p_this_order_->ConfirmAtTime(watch_.msecs_from_midnight());
              // set both size_requested_ and size_remaining_ to this value
              p_this_order_->ConfirmNewSize(_size_remaining_);
              p_this_order_->Execute(_size_executed_);

              int bid_index_ = GetBidIndexAndAdjustIntPrice(r_int_price_);
              sum_bid_confirmed_[bid_index_] += _size_remaining_;
              sum_bid_confirmed_orders_[bid_index_] += 1;
              PropagateConfirmedSizeDifference(bid_index_, _size_remaining_, kTradeTypeBuy);
              AdjustTopBottomConfirmedBidIndexes(bid_index_);

              bid_order_vec_[bid_index_].push_back(p_this_order_);
              AdjustTopBottomOrderVecBidIndexes(bid_index_);
            }

            if (p_this_order_ != nullptr) {
              if (dbglogger_.CheckLoggingLevel(OM_INFO))  // zero logging
              {
                DBGLOG_TIME_CLASS_FUNC << GetTradeTypeChar(p_this_order_->buysell()) << " " << p_this_order_->price()
                                       << " int(" << p_this_order_->int_price() << ")"
                                       << " " << p_this_order_->size_remaining() << " "
                                       << ToString(p_this_order_->order_status())
                                       << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                                       << " SAOS: " << p_this_order_->server_assigned_order_sequence()
                                       << " sA: " << p_this_order_->queue_size_ahead()
                                       << " sB: " << p_this_order_->queue_size_behind() << " msecs_since_seqd: "
                                       << ((p_this_order_->seqd_msecs_ >= 0)
                                               ? (watch_.msecs_from_midnight() - p_this_order_->seqd_msecs_)
                                               : (0)) << " msecs_since_conf: "
                                       << ((p_this_order_->placed_msecs_ >= 0)
                                               ? (watch_.msecs_from_midnight() - p_this_order_->placed_msecs_)
                                               : (0)) << DBGLOG_ENDL_FLUSH;
                if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO) && p_this_order_ != nullptr)  // zero logging
                {
                  if (p_this_order_ == nullptr) {
                    BaseOrder temp;
                    p_this_order_ = &temp;
                  }
                  dbglogger_ << "SYM: " << p_this_order_->security_name_ << " Px: " << p_this_order_->price()
                             << " INTPX: " << p_this_order_->int_price()
                             << " BS: " << GetTradeTypeChar(p_this_order_->buysell()) << " ST: " << watch_.tv()
                             << " DT: "
                             << watch_.tv() +
                                    ttime_t(0, p_this_order_->seqd_msecs_ * 1000 + p_this_order_->seqd_usecs_ % 1000) -
                                    ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                             << " ORR: " << ToString(kORRType_Exec)
                             << " SAOS: " << p_this_order_->server_assigned_order_sequence()
                             << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                             << " CLTPOS: " << t_client_position_ << " SACI: " << server_assigned_client_id_
                             << " GBLPOS: " << t_global_position_ << " SIZE: " << p_this_order_->size_remaining()
                             << " SE: " << p_this_order_->size_executed() << " MsgSeq : " << 0 << " Seq: " << 0
                             << DBGLOG_ENDL_FLUSH;
                }
              }
              NoteLevelExec(p_this_order_->placed_at_level_indicator_);
              is_fok_ = p_this_order_->is_fok_;
            }

            // SHOULDN"T WE DEALLOC ?
            unsequenced_bids_.erase(_base_order_vec_iter_);
            sum_bid_sizes_ -= _size_executed_;
            break;
          }
        }
      }
      if (p_this_order_ != nullptr && p_this_order_->canceled_) {
        cancel_order_seq_time_.push_back(p_this_order_->cancel_seqd_time_);
        cancel_order_exec_time_.push_back(watch_.tv());
      }

    } else {  // ASK order

      BaseOrder* p_this_order_ = nullptr; /* will stay nullptr till order found */
      int ask_index_ = GetAskIndexAndAdjustIntPrice(r_int_price_);

      /* search in sequenced orders at the price sent by ORS */
      /* fetch order in intpx_2_ask_order_vec_ [ r_int_price_ ] */
      std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[ask_index_];
      if (!_this_base_order_vec_.empty()) {
        for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
             _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
          if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
              t_server_assigned_order_sequence_) {  // Found the order
            p_this_order_ = (*_base_order_vec_iter_);

            switch (p_this_order_->order_status()) {  // depending on the last status that we had seen of this order,
                                                      // the maps or data structures from which we need to remove /
                                                      // modify this order changes
              case kORRType_Seqd: {  // either missed the confirm or the exec came from the exchange before
                                     // the
                                     // confirm
                num_unconfirmed_orders_--;

                sum_ask_unconfirmed_[ask_index_] =
                    std::max(0, sum_ask_unconfirmed_[ask_index_] - p_this_order_->size_requested());
                sum_ask_unconfirmed_orders_[ask_index_] = std::max(0, sum_ask_unconfirmed_orders_[ask_index_] - 1);
                PropagateUnconfirmedSizeDifference(ask_index_, -p_this_order_->size_requested(), kTradeTypeSell);
                AdjustTopBottomUnconfirmedAskIndexes(ask_index_);

                p_this_order_->ConfirmAtTime(watch_.msecs_from_midnight());
                // set both size_requested_ and size_remaining_ to this value
                p_this_order_->ConfirmNewSize(_size_remaining_);
                p_this_order_->Execute(_size_executed_);

                if (_size_remaining_ > 0) {
                  p_this_order_->ConfirmAtTime(watch_.msecs_from_midnight());
                  // set both size_requested_ and size_remaining_ to this value
                  p_this_order_->ConfirmNewSize(_size_remaining_);
                  p_this_order_->Execute(_size_executed_);

                  sum_ask_confirmed_[ask_index_] += _size_remaining_;
                  sum_ask_confirmed_[ask_index_] += 1;
                  PropagateConfirmedSizeDifference(ask_index_, _size_remaining_, kTradeTypeSell);
                  AdjustTopBottomConfirmedAskIndexes(ask_index_);
                } else {
                  // the order will be deleted and not more maps have to be changed
                  if (p_this_order_->modified_) {
                    // This case should not happen as we sould never send modify for unconfirmed order
                    int new_ask_index = GetAskIndexAndAdjustIntPrice(p_this_order_->modified_new_int_price_);
                    sum_ask_unconfirmed_[new_ask_index] =
                        std::max(0, sum_ask_unconfirmed_[new_ask_index] - p_this_order_->modified_new_size_);
                    sum_ask_unconfirmed_orders_[new_ask_index] =
                        std::max(0, sum_ask_unconfirmed_orders_[new_ask_index] - 1);
                    PropagateUnconfirmedSizeDifference(new_ask_index, -p_this_order_->modified_new_size_,
                                                       kTradeTypeSell);
                    AdjustTopBottomUnconfirmedAskIndexes(new_ask_index);
                    DBGLOG_TIME_CLASS_FUNC_LINE << " RARE: Received Ask exec without sequenced-modified order : "
                                                << " exec_px: " << _price_ << " size: " << _size_executed_
                                                << " mod_new_px: " << p_this_order_->modified_new_price_
                                                << " mod_new_sz: " << p_this_order_->modified_new_size_
                                                << DBGLOG_ENDL_FLUSH;
                  }
                }

              } break;
              case kORRType_Conf: {
                sum_ask_confirmed_[ask_index_] =
                    std::max(0, sum_ask_confirmed_[ask_index_] + _size_remaining_ - p_this_order_->size_remaining());
                if (p_this_order_->size_remaining() == 0)
                  sum_ask_confirmed_orders_[ask_index_] = std::max(0, sum_ask_confirmed_orders_[ask_index_] - 1);
                PropagateConfirmedSizeDifference(ask_index_, _size_remaining_ - p_this_order_->size_remaining(),
                                                 kTradeTypeSell);
                AdjustTopBottomConfirmedAskIndexes(ask_index_);

                // set both size_requested_ and size_remaining_ to this value
                p_this_order_->ConfirmNewSize(_size_remaining_);
                p_this_order_->Execute(_size_executed_);
                if (p_this_order_->modified_) {
                  // We don't modify values for confirmed order unless we get Cancel Replace confitmation. Here we got
                  // exec before confirmation,
                  // hence we need to modify the value of confirmed orders.
                  int new_ask_index = GetAskIndexAndAdjustIntPrice(p_this_order_->modified_new_int_price_);
                  sum_ask_unconfirmed_[new_ask_index] =
                      std::max(0, sum_ask_unconfirmed_[new_ask_index] - p_this_order_->modified_new_size_);
                  sum_ask_unconfirmed_orders_[ask_index_] = std::max(0, sum_ask_unconfirmed_orders_[ask_index_] - 1);
                  PropagateUnconfirmedSizeDifference(new_ask_index, -p_this_order_->modified_new_size_, kTradeTypeSell);
                  AdjustTopBottomUnconfirmedAskIndexes(new_ask_index);
                  DBGLOG_TIME_CLASS_FUNC_LINE << " RARE: Received Ask exec before ModifyConfirm : "
                                              << " exec_px: " << _price_ << " size: " << _size_executed_
                                              << " mod_new_px: " << p_this_order_->modified_new_price_
                                              << " mod_new_sz: " << p_this_order_->modified_new_size_
                                              << DBGLOG_ENDL_FLUSH;
                }

                if (use_modify_logic_) AdjustModifiableAskIndices('C', ask_index_);
              } break;
              default: {}
            }
            sum_ask_sizes_ -= _size_executed_;

            if (p_this_order_ != nullptr) {
              if (dbglogger_.CheckLoggingLevel(OM_INFO))  // zero logging
              {
                DBGLOG_TIME_CLASS_FUNC << GetTradeTypeChar(p_this_order_->buysell()) << " " << p_this_order_->price()
                                       << " int(" << p_this_order_->int_price() << ")"
                                       << " " << p_this_order_->size_remaining() << " "
                                       << ToString(p_this_order_->order_status())
                                       << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                                       << " SAOS: " << p_this_order_->server_assigned_order_sequence()
                                       << " sA: " << p_this_order_->queue_size_ahead()
                                       << " sB: " << p_this_order_->queue_size_behind() << " msecs_since_seqd: "
                                       << ((p_this_order_->seqd_msecs_ >= 0)
                                               ? (watch_.msecs_from_midnight() - p_this_order_->seqd_msecs_)
                                               : (0)) << " msecs_since_conf: "
                                       << ((p_this_order_->placed_msecs_ >= 0)
                                               ? (watch_.msecs_from_midnight() - p_this_order_->placed_msecs_)
                                               : (0)) << DBGLOG_ENDL_FLUSH;

                if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO) && p_this_order_ != nullptr)  // zero logging
                {
                  if (p_this_order_ == nullptr) {
                    BaseOrder temp;
                    p_this_order_ = &temp;
                  }
                  dbglogger_ << "SYM: " << p_this_order_->security_name_ << " Px: " << p_this_order_->price()
                             << " INTPX: " << p_this_order_->int_price()
                             << " BS: " << GetTradeTypeChar(p_this_order_->buysell()) << " ST: " << watch_.tv()
                             << " DT: "
                             << watch_.tv() +
                                    ttime_t(0, p_this_order_->seqd_msecs_ * 1000 + p_this_order_->seqd_usecs_ % 1000) -
                                    ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                             << " ORR: " << ToString(kORRType_Exec)
                             << " SAOS: " << p_this_order_->server_assigned_order_sequence()
                             << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                             << " CLTPOS: " << t_client_position_ << " SACI: " << server_assigned_client_id_
                             << " GBLPOS: " << t_global_position_ << " SIZE: " << p_this_order_->size_remaining()
                             << " SE: " << p_this_order_->size_executed() << " MsgSeq : " << 0 << " Seq: " << 0
                             << DBGLOG_ENDL_FLUSH;
                }
              }
              NoteLevelExec(p_this_order_->placed_at_level_indicator_);
              is_fok_ = p_this_order_->is_fok_;
            }

            if (_size_remaining_ <= 0) {
              _this_base_order_vec_.erase(_base_order_vec_iter_);  // remove order from vec
              AdjustTopBottomOrderVecAskIndexes(ask_index_);

              baseorder_mempool_.DeAlloc(p_this_order_);  // make sure not to use p_this_order_ after this line,
                                                          // although don't set it to nullptr
            }
            break;
          }
        }
      }

      if (p_this_order_ == nullptr) {  // If not found among sequenced orders at given price level ...
        // search for order in unsequenced_asks_
        for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = unsequenced_asks_.begin();
             _base_order_vec_iter_ != unsequenced_asks_.end(); _base_order_vec_iter_++) {
          if ((*_base_order_vec_iter_)->client_assigned_order_sequence() ==
              _client_assigned_order_sequence_) { /* found order */
            p_this_order_ = (*_base_order_vec_iter_);

            p_this_order_->SequenceAtTime(watch_.msecs_from_midnight(), watch_.tv().tv_usec);
            p_this_order_->server_assigned_order_sequence_ = t_server_assigned_order_sequence_;

            num_unconfirmed_orders_--;  ///< decrement number of unconfirmed orders

            sum_ask_unconfirmed_[ask_index_] =
                std::max(0, sum_ask_unconfirmed_[ask_index_] - p_this_order_->size_requested());
            sum_ask_unconfirmed_orders_[ask_index_] = std::max(0, sum_ask_unconfirmed_orders_[ask_index_] - 1);
            PropagateUnconfirmedSizeDifference(ask_index_, -p_this_order_->size_requested(), kTradeTypeSell);
            AdjustTopBottomUnconfirmedAskIndexes(ask_index_);

            if (_size_remaining_ > 0) {  // order is stil live so confirm it
              p_this_order_->ConfirmAtTime(watch_.msecs_from_midnight());
              // set both size_requested_ and size_remaining_ to this value
              p_this_order_->ConfirmNewSize(_size_remaining_);
              p_this_order_->Execute(_size_executed_);

              sum_ask_confirmed_[ask_index_] += _size_remaining_;
              sum_ask_confirmed_orders_[ask_index_] += 1;
              PropagateConfirmedSizeDifference(ask_index_, _size_remaining_, kTradeTypeSell);
              AdjustTopBottomConfirmedAskIndexes(ask_index_);

              ask_order_vec_[ask_index_].push_back(p_this_order_);
              AdjustTopBottomOrderVecAskIndexes(ask_index_);
            }
            if (p_this_order_ != nullptr) {
              if (dbglogger_.CheckLoggingLevel(OM_INFO))  // zero logging
              {
                DBGLOG_TIME_CLASS_FUNC << GetTradeTypeChar(p_this_order_->buysell()) << " " << p_this_order_->price()
                                       << " int(" << p_this_order_->int_price() << ")"
                                       << " " << p_this_order_->size_remaining() << " "
                                       << ToString(p_this_order_->order_status())
                                       << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                                       << " SAOS: " << p_this_order_->server_assigned_order_sequence()
                                       << " sA: " << p_this_order_->queue_size_ahead()
                                       << " sB: " << p_this_order_->queue_size_behind() << " msecs_since_seqd: "
                                       << ((p_this_order_->seqd_msecs_ >= 0)
                                               ? (watch_.msecs_from_midnight() - p_this_order_->seqd_msecs_)
                                               : (0)) << " msecs_since_conf: "
                                       << ((p_this_order_->placed_msecs_ >= 0)
                                               ? (watch_.msecs_from_midnight() - p_this_order_->placed_msecs_)
                                               : (0)) << DBGLOG_ENDL_FLUSH;

                if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO) && p_this_order_ != nullptr)  // zero logging
                {
                  if (p_this_order_ == nullptr) {
                    BaseOrder temp;
                    p_this_order_ = &temp;
                  }
                  dbglogger_ << "SYM: " << p_this_order_->security_name_ << " Px: " << p_this_order_->price()
                             << " INTPX: " << p_this_order_->int_price()
                             << " BS: " << GetTradeTypeChar(p_this_order_->buysell()) << " ST: " << watch_.tv()
                             << " DT: "
                             << watch_.tv() +
                                    ttime_t(0, p_this_order_->seqd_msecs_ * 1000 + p_this_order_->seqd_usecs_ % 1000) -
                                    ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                             << " ORR: " << ToString(kORRType_Exec)
                             << " SAOS: " << p_this_order_->server_assigned_order_sequence()
                             << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                             << " CLTPOS: " << t_client_position_ << " SACI: " << server_assigned_client_id_
                             << " GBLPOS: " << t_global_position_ << " SIZE: " << p_this_order_->size_remaining()
                             << " SE: " << p_this_order_->size_executed() << " MsgSeq : " << 0 << " Seq: " << 0
                             << DBGLOG_ENDL_FLUSH;
                }
              }
              NoteLevelExec(p_this_order_->placed_at_level_indicator_);
              is_fok_ = p_this_order_->is_fok_;
            }

            unsequenced_asks_.erase(_base_order_vec_iter_);
            sum_ask_sizes_ -= _size_executed_;

            break;
          }
        }
      }
      if (p_this_order_ != nullptr && p_this_order_->canceled_) {
        cancel_order_seq_time_.push_back(p_this_order_->cancel_seqd_time_);
        cancel_order_exec_time_.push_back(watch_.tv());
      }
    }

    if (is_fok_) {
      if (_size_remaining_ == 0) {
        if (t_buysell_ == kTradeTypeBuy) {
          foked_bid_order_size_sum_--;
        } else {
          foked_ask_order_size_sum_--;
        }
      }
      NotifyFokFillListeners(t_buysell_, _price_, r_int_price_, _size_executed_);
    }

    AdjustPosition(t_client_position_, _price_, r_int_price_);  // instead of GetMidPrice() use _price_ sent
    NotifyOrderChangeListeners();

    if (dbglogger_.CheckLoggingLevel(OM_INFO))  // zero logging
    {
      DBGLOG_TIME_CLASS_FUNC << " SACI: " << t_server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_
                             << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_
                             << " px: " << _price_ << " bs: " << GetTradeTypeChar(t_buysell_) << " sizeR "
                             << _size_remaining_ << " sizeE " << _size_executed_ << " cpos: " << t_client_position_
                             << " gpos: " << t_global_position_ << " intpx: " << r_int_price_ << DBGLOG_ENDL_FLUSH;
    }

    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " ShowOrders: " << DBGLOG_ENDL_FLUSH;
      LogFullStatus();
      // DBGLOG_TIME_CLASS_FUNC
      //   << " ShowOrders: " << ShowOrders ( ) << DBGLOG_ENDL_FLUSH;
    }
  }

  // If asked to keep a track of positions across all contracts for this security ,
  // update the security_position_ variable.
  if (_security_id_ < security_id_to_last_position_.size() &&
      security_id_to_last_position_[_security_id_] !=
          kInvalidPosition) {  // This sid belongs to the family we are interested in.
    // Update security position.
    DBGLOG_TIME_CLASS_FUNC << " Updated security_id_=" << _security_id_ << " global_position_=" << global_position_
                           << " security_position_=" << security_position_ << DBGLOG_ENDL_FLUSH;

    for (auto i = 0u; i < security_id_to_last_position_.size(); ++i) {
      dbglogger_ << " security_id_=" << i << " shortcode_=" << sec_name_indexer_.GetShortcodeFromId(i)
                 << " last_position_=" << security_id_to_last_position_[i] << "\n";
    }

    security_position_ -= security_id_to_last_position_[_security_id_];
    security_position_ += global_position_;

    security_id_to_last_position_[_security_id_] = global_position_;
  }

#if ENABLE_AGG_TRADING_COOLOFF
  if (agg_trading_cooloff_ && !disable_orders_due_to_agg_cooloff_) {
    if (t_buysell_ == kTradeTypeBuy || t_buysell_ == kTradeTypeSell) {
      if (last_exec_time_[kTradeTypeBuy] < (watch_.msecs_from_midnight() - agg_time_frame_msecs_)) {
        last_exec_int_price_[kTradeTypeBuy] = 0;
      }
      if (last_exec_time_[kTradeTypeSell] < (watch_.msecs_from_midnight() - agg_time_frame_msecs_)) {
        last_exec_int_price_[kTradeTypeSell] = 0;
      }

      last_exec_int_price_[t_buysell_] = r_int_price_;
      last_exec_time_[t_buysell_] = watch_.msecs_from_midnight();

      if (last_exec_int_price_[kTradeTypeBuy] != 0 && last_exec_int_price_[kTradeTypeSell] != 0) {
        if (last_exec_int_price_[kTradeTypeBuy] > last_exec_int_price_[kTradeTypeSell]) {
          ++unintentional_agg_sizes_;
        }
      } else {
        unintentional_agg_sizes_ = 0;
      }
    }

    if (unintentional_agg_sizes_ > agg_size_threshold_) {
#if ENABLE_AGG_TRADING_COOLOFF_ORDER_DISABLE
      agg_cooloff_stop_msecs_ = watch_.msecs_from_midnight() + (100 * agg_time_frame_msecs_);
      disable_orders_due_to_agg_cooloff_ = true;

      DBGLOG_TIME_CLASS_FUNC << " agg_trading_cooloff_ activated. Disabling orders." << DBGLOG_ENDL_FLUSH;
#endif

      if (!email_sent_) {
        EmailAggVariablesOnDisable();
        email_sent_ = true;
      }

      DumpAggVariables();
    }
  }
#endif

  //    CleanMaps ( );
}

void BaseOrderManager::PrintCancelSeqdExecTimes() {
  for (auto i = 0u; i < cancel_order_seq_time_.size(); i++) {
    DBGLOG_CLASS_FUNC << cancel_order_seq_time_[i] << " " << cancel_order_exec_time_[i] << DBGLOG_ENDL_FLUSH;
  }
}

/**
 * Handle exchange rejections based on funds
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param t_buysell_
 * @param _size_remaining_
 * @param _rejection_reason_
 * @param r_int_price_
 * @param exchange_order_id
 * @param time_set_by_server
 */
void BaseOrderManager::OrderRejectedDueToFunds(const int t_server_assigned_client_id_,
                                               const int _client_assigned_order_sequence_,
                                               const unsigned int _security_id_, const double _price_,
                                               const TradeType_t t_buysell_, const int _size_remaining_,
                                               const int _rejection_reason_, const int r_int_price_,
                                               const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  DBGLOG_TIME_CLASS_FUNC << "OrderRejectedDueToFunds in base order manager " << DBGLOG_ENDL_FLUSH;

  // Don't need to check for packet drops here, refer to the OrderRejected comments
  OrderRejected(t_server_assigned_client_id_, _client_assigned_order_sequence_, _security_id_, _price_, t_buysell_,
                _size_remaining_, _rejection_reason_, r_int_price_, exchange_order_id, time_set_by_server);
  for (int i = 0; i < (int)reject_due_to_funds_listener_vec_.size(); i++) {
    reject_due_to_funds_listener_vec_[i]->OnRejectDueToFunds(t_buysell_);
  }
}

void BaseOrderManager::WakeUpifRejectedDueToFunds() {
  // Not interested in recovering or detection drops here
  for (int i = 0; i < (int)reject_due_to_funds_listener_vec_.size(); i++) {
    reject_due_to_funds_listener_vec_[i]->OnWakeUpifRejectDueToFunds();
  }
}

/**
 *
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param t_buysell_
 * @param _size_remaining_
 * @param _rejection_reason_
 * @param r_int_price_
 * @param exchange_order_id
 * @param time_set_by_server
 */
void BaseOrderManager::OrderRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                     const unsigned int _security_id_, const double _price_,
                                     const TradeType_t t_buysell_, const int _size_remaining_,
                                     const int _rejection_reason_, const int r_int_price_,
                                     const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  if (t_server_assigned_client_id_ == server_assigned_client_id_) {  // the rejected order was sent by me :(

    received_after_last_sent_ = true;

    if (dbglogger_.CheckLoggingLevel(ORS_DATA_INFO)) {
      DBGLOG_CLASS_FUNC_LINE_INFO << "SACI : " << t_server_assigned_client_id_ << ' '
                                  << "CAOS : " << _client_assigned_order_sequence_ << ' ' << "SECID : " << _security_id_
                                  << ' ' << "Px : " << _price_ << ' ' << "BUYSELL : " << t_buysell_ << ' '
                                  << "SR : " << _size_remaining_ << ' ' << "IntPx : " << r_int_price_ << ' '
                                  << "RejRes : "
                                  << HFSAT::ORSRejectionReasonStr(HFSAT::ORSRejectionReason_t(_rejection_reason_))
                                  << DBGLOG_ENDL_FLUSH;
    }

    // Start/Increment Counter
    if (HFSAT::kExchOrderReject == (HFSAT::ORSRejectionReason_t)_rejection_reason_) {
      number_of_consecutive_exchange_rejects_++;
      time_based_total_exchange_rejects_++;
      total_exchange_rejects_++;
    }

    // Special handling considered for auto freeze
    if (HFSAT::kORSRejectSecurityNotFound == (HFSAT::ORSRejectionReason_t)_rejection_reason_) {
      NotifyExchangeRejectsListeners(HFSAT::BaseUtils::FreezeEnforcedReason::kFreezeOnORSSecNotFound);
      // If we've not reset the rejects and we've breached threshold
      // our freeze timeout starts now
      if (0 == reject_based_freeze_timer_) {
        reject_based_freeze_timer_ = watch_.msecs_from_midnight();
      }
    }

    HFSAT::BaseUtils::FreezeEnforcedReason freeze_enforcement = query_trading_auto_freeze_manager_.ShouldAllowOrders(
        number_of_consecutive_exchange_rejects_, time_based_total_exchange_rejects_, total_exchange_rejects_);

    if (HFSAT::BaseUtils::FreezeEnforcedReason::kAllow != freeze_enforcement) {
      NotifyExchangeRejectsListeners(freeze_enforcement);
      // If we've not reset the rejects and we've breached threshold
      // our freeze timeout starts now
      if (0 == reject_based_freeze_timer_) {
        reject_based_freeze_timer_ = watch_.msecs_from_midnight();
      }
    }

    // Let's not store history of rejections on ORS end and not include it in drop detection
    // or recovery, some ORS rejects can potentially fill up the storage eroding messsage we actually
    // care about recovering - @ravi

    //    //Check For Drops
    //    if ( server_assigned_message_sequence > expected_message_sequence_ ) {
    //
    //      //let's drop a recovery request now
    //      GoForPacketRecovery ( expected_message_sequence_, server_assigned_message_sequence ) ;
    //
    //    }else if ( server_assigned_message_sequence == expected_message_sequence_ ) { //Move forward
    //
    //      //It's very important that the expected_message_sequence_ is always correct and in sync
    //      //potentially incorrect sequence would keep us in loop of recovery
    //      expected_message_sequence_ ++ ;
    //
    //    }else {
    //
    //      //We received a lower sequence, it's fine could be over recovered sequence
    //      if ( dropped_sequences_.end () != dropped_sequences_.find ( server_assigned_message_sequence ) )
    //      {
    //        dropped_sequences_.erase ( server_assigned_message_sequence ) ;
    //      }
    //
    //    }

    if (_rejection_reason_ == kORSRejectETIAlgoTaggingFailure) {
      // Send a mail and exit.
      // Not removing pid file intentinally as this is an error case.
      DBGLOG_TIME_CLASS_FUNC << "ETIAlgoTaggingFailure...Killing Myself" << DBGLOG_ENDL_FLUSH;
      dbglogger_.DumpCurrentBuffer();
      exit(0);
    }

    if (_rejection_reason_ != kORSRejectSelfTradeCheck) {
      no_new_order_till = watch_.msecs_from_midnight() + REJECT_COOLOFF_MSECS;
      if_reject_set = true;
    }

    bool is_fok_ = false;

    if (dbglogger_.CheckLoggingLevel(OM_INFO))  // zero logging
    {
      DBGLOG_TIME_CLASS_FUNC << " SACI: " << t_server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_ << " sid: " << _security_id_
                             << " px: " << _price_ << " bs: " << GetTradeTypeChar(t_buysell_) << " sizeR "
                             << _size_remaining_ << " rejR "
                             << ORSRejectionReasonStr((ORSRejectionReason_t)_rejection_reason_)
                             << " intpx: " << r_int_price_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;  // to make sure we see rejects
    }

    bool t_order_found_ = false;
    if (t_buysell_ == kTradeTypeBuy) {
      int bid_index_ = GetBidIndexAndAdjustIntPrice(r_int_price_);

      for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = unsequenced_bids_.begin();
           _base_order_vec_iter_ != unsequenced_bids_.end(); _base_order_vec_iter_++) {
        if ((*_base_order_vec_iter_)->client_assigned_order_sequence() ==
            _client_assigned_order_sequence_) { /* found order */
          BaseOrder* p_this_order_ = (*_base_order_vec_iter_);

          if (p_this_order_->order_status() == kORRType_Conf) {  // Got a reject for a confirmed order. Ignore.
            break;
          }

          sum_bid_unconfirmed_[bid_index_] =
              std::max(0, sum_bid_unconfirmed_[bid_index_] - p_this_order_->size_requested());
          sum_bid_unconfirmed_orders_[bid_index_] = std::max(0, sum_bid_unconfirmed_orders_[bid_index_] - 1);
          PropagateUnconfirmedSizeDifference(bid_index_, -p_this_order_->size_requested(), kTradeTypeBuy);
          AdjustTopBottomUnconfirmedBidIndexes(bid_index_);

          unsequenced_bids_.erase(_base_order_vec_iter_);
          num_unconfirmed_orders_--;  ///< decrement number of unconfirmed orders

          sum_bid_sizes_ = sum_bid_sizes_ - p_this_order_->size_requested();

          is_fok_ = p_this_order_->is_fok_;
          baseorder_mempool_.DeAlloc(p_this_order_);
          t_order_found_ = true;

          break;
        }
      }
      /// it has been sequenced
      if (!t_order_found_) {
        std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[bid_index_];
        if (!_this_base_order_vec_.empty()) {
          for (std::vector<BaseOrder*>::reverse_iterator _base_order_vec_rev_iter_ = _this_base_order_vec_.rbegin();
               _base_order_vec_rev_iter_ != _this_base_order_vec_.rend();
               _base_order_vec_rev_iter_++) { /* reverse search should be more optimal since newer orders are
                                                 towards
                                                 the end */
            if ((*_base_order_vec_rev_iter_)->client_assigned_order_sequence() ==
                _client_assigned_order_sequence_) {  // Found the order
              BaseOrder* p_this_order_ = *_base_order_vec_rev_iter_;

              if (p_this_order_->order_status() == kORRType_Seqd) {
                num_unconfirmed_orders_--;  ///< decrement number of unconfirmed orders

                sum_bid_unconfirmed_[bid_index_] =
                    std::max(0, sum_bid_unconfirmed_[bid_index_] - p_this_order_->size_requested());
                sum_bid_unconfirmed_orders_[bid_index_] = std::max(0, sum_bid_unconfirmed_orders_[bid_index_] - 1);
                PropagateUnconfirmedSizeDifference(bid_index_, -p_this_order_->size_requested(), kTradeTypeBuy);
                AdjustTopBottomUnconfirmedBidIndexes(bid_index_);

              } else if (p_this_order_->order_status() == kORRType_Conf) {
                sum_bid_confirmed_[bid_index_] =
                    std::max(0, sum_bid_confirmed_[bid_index_] - p_this_order_->size_remaining());
                sum_bid_confirmed_[bid_index_] = std::max(0, sum_bid_confirmed_[bid_index_] - 1);
                PropagateConfirmedSizeDifference(bid_index_, -p_this_order_->size_remaining(), kTradeTypeBuy);
                AdjustTopBottomConfirmedBidIndexes(bid_index_);
              }

              RemoveOrderFromVec(bid_order_vec_[bid_index_], p_this_order_);
              AdjustTopBottomOrderVecBidIndexes(bid_index_);

              sum_bid_sizes_ -= p_this_order_->size_requested();

              is_fok_ = p_this_order_->is_fok_;
              baseorder_mempool_.DeAlloc(p_this_order_);

              break;
            }
          }
        }
      }
    }
    /// ask order rejected
    else {
      int ask_index_ = GetAskIndexAndAdjustIntPrice(r_int_price_);

      for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = unsequenced_asks_.begin();
           _base_order_vec_iter_ != unsequenced_asks_.end(); _base_order_vec_iter_++) {
        if ((*_base_order_vec_iter_)->client_assigned_order_sequence() ==
            _client_assigned_order_sequence_) { /* found order */
          BaseOrder* p_this_order_ = (*_base_order_vec_iter_);

          if (p_this_order_->order_status() == kORRType_Conf) {  // Got a reject for a confirmed order. Ignore.
            break;
          }

          sum_ask_unconfirmed_[ask_index_] =
              std::max(0, sum_ask_unconfirmed_[ask_index_] - p_this_order_->size_requested());
          sum_ask_unconfirmed_orders_[ask_index_] = std::max(0, sum_ask_unconfirmed_orders_[ask_index_] - 1);
          PropagateUnconfirmedSizeDifference(ask_index_, -p_this_order_->size_requested(), kTradeTypeSell);
          AdjustTopBottomUnconfirmedAskIndexes(ask_index_);

          unsequenced_asks_.erase(_base_order_vec_iter_);
          num_unconfirmed_orders_--;  ///< decrement number of unconfirmed orders

          sum_ask_sizes_ -= p_this_order_->size_requested();

          is_fok_ = p_this_order_->is_fok_;
          baseorder_mempool_.DeAlloc(p_this_order_);
          t_order_found_ = true;

          break;
        }
      }
      /// it has been sequenced
      if (!t_order_found_) {
        std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[ask_index_];
        if (!_this_base_order_vec_.empty()) {
          for (std::vector<BaseOrder*>::reverse_iterator _base_order_vec_rev_iter_ = _this_base_order_vec_.rbegin();
               _base_order_vec_rev_iter_ != _this_base_order_vec_.rend();
               _base_order_vec_rev_iter_++) { /* perhaps reverse search should be more optimal since newer
                                                 orders are
                                                 towards the end */
            if ((*_base_order_vec_rev_iter_)->client_assigned_order_sequence() ==
                _client_assigned_order_sequence_) {  // Found the order
              BaseOrder* p_this_order_ = *_base_order_vec_rev_iter_;

              if (p_this_order_->order_status() == kORRType_Seqd) {
                num_unconfirmed_orders_--;  ///< decrement number of unconfirmed orders

                sum_ask_unconfirmed_[ask_index_] =
                    std::max(0, sum_ask_unconfirmed_[ask_index_] - p_this_order_->size_requested());
                sum_ask_unconfirmed_orders_[ask_index_] = std::max(0, sum_ask_unconfirmed_orders_[ask_index_] - 1);
                PropagateUnconfirmedSizeDifference(ask_index_, -p_this_order_->size_requested(), kTradeTypeSell);
                AdjustTopBottomUnconfirmedAskIndexes(ask_index_);

              } else if (p_this_order_->order_status() == kORRType_Conf) {
                sum_ask_confirmed_[ask_index_] =
                    std::max(0, sum_ask_confirmed_[ask_index_] - p_this_order_->size_remaining());
                sum_ask_confirmed_[ask_index_] = std::max(0, sum_ask_confirmed_[ask_index_] - 1);
                PropagateConfirmedSizeDifference(ask_index_, -p_this_order_->size_remaining(), kTradeTypeSell);
                AdjustTopBottomConfirmedAskIndexes(ask_index_);
              }
              RemoveOrderFromVec(ask_order_vec_[ask_index_], p_this_order_);
              AdjustTopBottomOrderVecAskIndexes(ask_index_);

              sum_ask_sizes_ -= p_this_order_->size_requested();

              is_fok_ = p_this_order_->is_fok_;
              baseorder_mempool_.DeAlloc(p_this_order_);

              break;
            }
          }
        }
      }
    }

    NotifyOrderChangeListeners();
    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " ShowOrders: " << DBGLOG_ENDL_FLUSH;
      LogFullStatus();
      // DBGLOG_TIME_CLASS_FUNC
      // 	       << " ShowOrders: " << ShowOrders ( ) << DBGLOG_ENDL_FLUSH;
    }

    if (is_fok_) {
      if (t_buysell_ == kTradeTypeBuy) {
        foked_bid_order_size_sum_--;
      } else {
        foked_ask_order_size_sum_--;
      }
      NotifyFokRejectListeners(t_buysell_, _price_, r_int_price_, _size_remaining_);
    }
  }
  //    CleanMaps ( );
}

// @ramkris: Implementation of OrderCancelReject in the Base Order Manager
// depending on different reasons of rejection
// 0 - Too late to reject
// 1 - Unknown Order
// 2-  Rejected from the ORS , so just set the cancel= false; again so that it can be sent to the ORS again
// 3 - Order already in the pending cancel or pending replace
// 6 - Duplicate ClOrdID received
void BaseOrderManager::OrderCancelRejected(const int t_server_assigned_client_id_,
                                           const int _client_assigned_order_sequence_,
                                           const int t_server_assigned_order_sequence_,
                                           const unsigned int _security_id_, const double _price_,
                                           const TradeType_t t_buysell_, const int _size_remaining_,
                                           const int _rejection_reason_, const int t_client_position_,
                                           const int t_global_position_, const int r_int_price_,
                                           const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  global_position_ = t_global_position_;

  if (t_server_assigned_client_id_ == server_assigned_client_id_) {  //

    received_after_last_sent_ = true;

    if (dbglogger_.CheckLoggingLevel(ORS_DATA_INFO)) {
      DBGLOG_CLASS_FUNC_LINE_INFO << "SACI : " << t_server_assigned_client_id_ << ' '
                                  << "CAOS : " << _client_assigned_order_sequence_ << ' '
                                  << "SAOS : " << t_server_assigned_order_sequence_ << ' '
                                  << "SECID : " << _security_id_ << ' ' << "Px : " << _price_ << ' '
                                  << "BUYSELL : " << t_buysell_ << ' ' << "SR : " << _size_remaining_ << ' '
                                  << "CP : " << t_client_position_ << ' ' << "GP : " << t_global_position_ << ' '
                                  << "IntPx : " << r_int_price_ << ' ' << "RejRes : "
                                  << HFSAT::CancelRejectReasonStr(HFSAT::CxlRejectReason_t(_rejection_reason_))
                                  << DBGLOG_ENDL_FLUSH;
    }

    // Start/Increment Counter
    if (HFSAT::kExchCancelReject == (HFSAT::CxlRejectReason_t)_rejection_reason_) {
      number_of_consecutive_exchange_rejects_++;
      time_based_total_exchange_rejects_++;
      total_exchange_rejects_++;
    }

    HFSAT::BaseUtils::FreezeEnforcedReason freeze_enforcement = query_trading_auto_freeze_manager_.ShouldAllowOrders(
        number_of_consecutive_exchange_rejects_, time_based_total_exchange_rejects_, total_exchange_rejects_);
    if (HFSAT::BaseUtils::FreezeEnforcedReason::kAllow != freeze_enforcement) {
      NotifyExchangeRejectsListeners(freeze_enforcement);
      // If we've not reset the rejects and we've breached threshold
      // our freeze timeout starts now
      if (0 == reject_based_freeze_timer_) {
        reject_based_freeze_timer_ = watch_.msecs_from_midnight();
      }
    }

    // Let's not store history of rejections on ORS end and not include it in drop detection
    // or recovery, some ORS rejects can potentially fill up the storage eroding messsage we actually
    // care about recovering - @ravi

    //    //Check For Drops
    //    if ( server_assigned_message_sequence > expected_message_sequence_ ) {
    //
    //      //let's drop a recovery request now
    //      GoForPacketRecovery ( expected_message_sequence_, server_assigned_message_sequence ) ;
    //
    //    }else if ( server_assigned_message_sequence == expected_message_sequence_ ) { //Move forward
    //
    //      //It's very important that the expected_message_sequence_ is always correct and in sync
    //      //potentially incorrect sequence would keep us in loop of recovery
    //      expected_message_sequence_ ++ ;
    //
    //    }else {
    //
    //      //We received a lower sequence, it's fine could be over recovered sequence
    //      if ( dropped_sequences_.end () != dropped_sequences_.find ( server_assigned_message_sequence ) )
    //      {
    //        dropped_sequences_.erase ( server_assigned_message_sequence ) ;
    //      }
    //
    //    }

    // the rejected cancel was sent by me :(

    BaseOrder* p_this_order_ = nullptr;
    switch (_rejection_reason_) {
      case kCxlRejectReasonTooLate:
      // Too late to reject ( which means that either the order has been
      // executed (full / parital(?) ) or is in process of execution, if
      // it is executed fully/partially, OnorderExecuted will be called
      // and handle the case
      case kCxlRejectReasonUnknownOrder:
        // Unknown Order : The order doesnot exist with the exchange anymore
        // Find the order and throw it out

        // Since we are able to send it to Exchange a cancel order, the order must
        // have been confirmed ( our current logic ) and must be found in this vector
        // no need to look into unsequenced Orders
        if (t_buysell_ == kTradeTypeBuy) {
          int bid_index_ = GetBidIndexAndAdjustIntPrice(r_int_price_);
          std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[bid_index_];

          if (!_this_base_order_vec_.empty()) {
            p_this_order_ = nullptr;  // will stay nullptr till order found
            for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
                 _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
              if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
                  t_server_assigned_order_sequence_) {  // Found the order
                p_this_order_ = (*_base_order_vec_iter_);
                // Remove it
                _this_base_order_vec_.erase(_base_order_vec_iter_);  // remove order from vec
                AdjustTopBottomOrderVecBidIndexes(bid_index_);

                sum_bid_confirmed_[bid_index_] =
                    std::max(0, sum_bid_confirmed_[bid_index_] - p_this_order_->size_remaining());
                sum_bid_confirmed_orders_[bid_index_] = std::max(0, sum_bid_confirmed_[bid_index_] - 1);
                PropagateConfirmedSizeDifference(bid_index_, -p_this_order_->size_remaining(), kTradeTypeBuy);
                AdjustTopBottomConfirmedBidIndexes(bid_index_);

                baseorder_mempool_.DeAlloc(p_this_order_);

                break;
              }
            }
          }
        } else {
          int ask_index_ = GetAskIndexAndAdjustIntPrice(r_int_price_);
          std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[ask_index_];

          if (!_this_base_order_vec_.empty()) {
            p_this_order_ = nullptr;  // will stay nullptr till order found
            for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
                 _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
              if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
                  t_server_assigned_order_sequence_) {  // Found the order
                p_this_order_ = (*_base_order_vec_iter_);
                // Remove it
                _this_base_order_vec_.erase(_base_order_vec_iter_);  // remove order from vec
                AdjustTopBottomOrderVecAskIndexes(ask_index_);

                // Reduce the confirmed asks
                sum_ask_confirmed_[ask_index_] =
                    std::max(0, sum_ask_confirmed_[ask_index_] - p_this_order_->size_remaining());
                sum_ask_confirmed_[ask_index_] = std::max(0, sum_ask_confirmed_[ask_index_] - 1);
                PropagateConfirmedSizeDifference(ask_index_, -p_this_order_->size_remaining(), kTradeTypeSell);

                AdjustTopBottomConfirmedAskIndexes(ask_index_);

                baseorder_mempool_.DeAlloc(p_this_order_);
                break;
              }
            }
          }
        }

        break;
      case kCxlRejectReasonOrderNotConfirmed:
      case kCxlRejectReasonOrderAlreadyInPendingQueue:
      // Order in Pending Cancel or Pending Replace
      // Do nothing since the cancel order exists with the exchange
      case kCxlRejectReasonDuplicateClOrdID:
      // Duplicate ClOrdid Recieved, there is a bug somewhere else
      // in the SAOS assignment
      case kCxlRejectReasonThrottle:
      case kCxlRejectReasonTAPThrottle:
      case kCxlRejectReasonMarketClosed:
      // in case of throttle just resetting canceled flag
      // we want to set the cancelled_ field as false for now
      case kCxlRejectReasonOther:
        // We might want to assume this is a throttle

        if (t_buysell_ == kTradeTypeBuy) {
          int bid_index_ = GetBidIndex(r_int_price_);
          std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[bid_index_];

          if (!_this_base_order_vec_.empty()) {
            p_this_order_ = nullptr;  // will stay nullptr till order found
            for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
                 _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
              if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
                  t_server_assigned_order_sequence_) {  // Found the order
                p_this_order_ = (*_base_order_vec_iter_);
                p_this_order_->canceled_ = false;
                break;
              }
            }
          }
        } else {
          int ask_index_ = GetAskIndex(r_int_price_);
          std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[ask_index_];

          if (!_this_base_order_vec_.empty()) {
            p_this_order_ = nullptr;  // will stay nullptr till order found
            for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
                 _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
              if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
                  t_server_assigned_order_sequence_) {  // Found the order
                p_this_order_ = (*_base_order_vec_iter_);
                p_this_order_->canceled_ = false;
                break;
              }
            }
          }
        }

        break;
      default:
        break;

    }  // End Switch

    if (client_position_ != t_client_position_) {
      AdjustPosition(t_client_position_, GetMidPrice(), GetIntPx(GetMidPrice()));
    }

    NotifyCancelRejectListeners(t_buysell_, _price_, r_int_price_);
    NotifyOrderChangeListeners();  ///

    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " SACI: " << t_server_assigned_client_id_
                             << " CAOS: " << _client_assigned_order_sequence_
                             << " SAOS: " << t_server_assigned_order_sequence_ << " sid: " << _security_id_
                             << " px: " << _price_ << " bs: " << GetTradeTypeChar(t_buysell_) << " sizeR "
                             << _size_remaining_ << " CrejR "
                             << CancelRejectReasonStr((CxlRejectReason_t)_rejection_reason_)
                             << " cpos: " << t_client_position_ << " gpos: " << t_global_position_
                             << " intpx: " << r_int_price_ << DBGLOG_ENDL_FLUSH;
      if (dbglogger_.CheckLoggingLevel(SIM_ORDER_INFO) && p_this_order_ != nullptr)  // zero logging
      {
        dbglogger_ << "SYM: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " Px: " << _price_
                   << " INTPX: " << p_this_order_->int_price() << " BS: " << GetTradeTypeChar(t_buysell_)
                   << " ST: " << watch_.tv() << " DT: "
                   << watch_.tv() + ttime_t(0, p_this_order_->seqd_msecs_ * 1000 + p_this_order_->seqd_usecs_ % 1000) -
                          ttime_t(0, watch_.msecs_from_midnight() * 1000 + watch_.tv().tv_usec % 1000)
                   << " ORR: " << ToString(kORRType_CxlRejc) << " SAOS: " << t_server_assigned_order_sequence_
                   << " CAOS: " << _client_assigned_order_sequence_ << " CLTPOS: " << t_client_position_
                   << " SACI: " << server_assigned_client_id_ << " GBLPOS: " << t_global_position_
                   << " SIZE: " << _size_remaining_ << " SE: " << 0 << " MsgSeq : " << 0 << " Seq: " << 0
                   << DBGLOG_ENDL_FLUSH;
      }
    }

    if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " ShowOrders: " << DBGLOG_ENDL_FLUSH;
      LogFullStatus();
      // DBGLOG_TIME_CLASS_FUNC << " ShowOrders: " << ShowOrders ( ) << DBGLOG_ENDL_FLUSH;
    }
  }

  //    CleanMaps ( );
}

/**
 * Check for valid sizes in the unconfirmed/confirmed maps
 * Also if the sizes do not match with the orders existing in the maps, then change them
 */
void BaseOrderManager::SanitizeMaps() {
  /* Currently we are doing very light check
  Most of the errors should come in this section,
  If we choose to do complete sanitization, it would not serve the purpose of maintaining the sum_szs variables */
  /// Buy side
  if (order_vec_top_bid_index_ == -1 && order_vec_bottom_bid_index_ == -1) {
    if (unconfirmed_top_bid_index_ != -1 || unconfirmed_bottom_bid_index_ != -1) {
      // Even if we dont have any order, our sizes maps are filled with non-zero values
      for (int i = unconfirmed_bottom_bid_index_; i <= unconfirmed_top_bid_index_; i++) {
        // Currently  having a light check
        if (unsequenced_bids_.size() <= 0) {
          DBGLOG_TIME_CLASS_FUNC << " Found non-zero unconfirmed bid-size " << sum_bid_unconfirmed_[i] << " @ "
                                 << GetBidIntPrice(i) << " even though there are no orders " << DBGLOG_ENDL_FLUSH;
          sum_bid_unconfirmed_[i] = 0;
          sum_bid_unconfirmed_orders_[i] = 0;
          sum_total_bid_unconfirmed_[i] = 0;
        }
      }
    }
    if (confirmed_top_bid_index_ != -1 || confirmed_bottom_bid_index_) {
      for (int i = confirmed_bottom_bid_index_; i <= confirmed_top_bid_index_; i++) {
        DBGLOG_TIME_CLASS_FUNC << " Found non-zero confirmed bid-size " << sum_bid_confirmed_[i] << " @ "
                               << GetBidIntPrice(i) << " even though there are no orders " << DBGLOG_ENDL_FLUSH;
        sum_bid_confirmed_[i] = 0;
        sum_bid_confirmed_orders_[i] = 0;
        sum_total_bid_confirmed_[i] = 0;
      }
    }
  }

  /// Sell side
  if (order_vec_top_ask_index_ == -1 && order_vec_bottom_ask_index_ == -1) {
    if (unconfirmed_top_ask_index_ != -1 || unconfirmed_bottom_ask_index_ != -1) {
      for (int i = unconfirmed_bottom_ask_index_; i <= unconfirmed_top_ask_index_; i++) {
        if (unsequenced_asks_.size() <= 0) {
          DBGLOG_TIME_CLASS_FUNC << " Found non-zero unconfirmed ask-size " << sum_ask_unconfirmed_[i] << " @ "
                                 << GetAskIntPrice(i) << " even though there are no orders " << DBGLOG_ENDL_FLUSH;
          sum_ask_unconfirmed_[i] = 0;
          sum_ask_unconfirmed_orders_[i] = 0;
          sum_total_ask_unconfirmed_[i] = 0;
        }
      }
    }

    if (confirmed_top_ask_index_ != -1 || confirmed_bottom_ask_index_ != -1) {
      for (int i = confirmed_bottom_ask_index_; i <= confirmed_top_ask_index_; i++) {
        DBGLOG_TIME_CLASS_FUNC << " Found non-zero unconfirmed bid-size " << sum_ask_confirmed_[i] << " @ "
                               << GetAskIntPrice(i) << " even though there are no orders " << DBGLOG_ENDL_FLUSH;
        sum_ask_confirmed_[i] = 0;
        sum_ask_confirmed_orders_[i] = 0;
        sum_total_ask_confirmed_[i] = 0;
      }
    }
  }
}

std::string BaseOrderManager::ShowOrders() const {
  std::ostringstream t_temp_oss_;

  t_temp_oss_ << "Unsequenced Bids" << std::endl;
  for (std::vector<BaseOrder*>::const_iterator base_order_vec_citer_ = unsequenced_bids_.begin();
       base_order_vec_citer_ != unsequenced_bids_.end(); base_order_vec_citer_++) {
    const BaseOrder* p_this_order_ = (*base_order_vec_citer_);
    if (p_this_order_ != nullptr) {
      t_temp_oss_ << GetTradeTypeChar(p_this_order_->buysell()) << " " << p_this_order_->price() << " "
                  << p_this_order_->size_remaining() << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                  << std::endl;
    }
  }
  t_temp_oss_ << "Unsequenced Asks" << std::endl;
  for (std::vector<BaseOrder*>::const_iterator base_order_vec_citer_ = unsequenced_asks_.begin();
       base_order_vec_citer_ != unsequenced_asks_.end(); base_order_vec_citer_++) {
    const BaseOrder* p_this_order_ = (*base_order_vec_citer_);
    if (p_this_order_ != nullptr) {
      t_temp_oss_ << GetTradeTypeChar(p_this_order_->buysell()) << " " << p_this_order_->price() << " "
                  << p_this_order_->size_remaining() << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                  << std::endl;
    }
  }

  t_temp_oss_ << "Sequenced Bids" << std::endl;
  if (order_vec_top_bid_index_ != -1) {
    for (int bid_index_ = order_vec_top_bid_index_; bid_index_ >= order_vec_bottom_bid_index_; bid_index_--) {
      const std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[bid_index_];
      for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
        BaseOrder* p_this_order_ = _this_base_order_vec_[i];
        if (p_this_order_ != nullptr) {
          t_temp_oss_ << GetTradeTypeChar(p_this_order_->buysell()) << " " << p_this_order_->price() << " "
                      << p_this_order_->size_remaining() << " " << ToString(p_this_order_->order_status())
                      << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                      << " SAOS: " << p_this_order_->server_assigned_order_sequence()
                      << " sA: " << p_this_order_->queue_size_ahead() << " sB: " << p_this_order_->queue_size_behind()
                      << std::endl;
        }
      }
    }
  }

  t_temp_oss_ << "Sequenced Asks" << std::endl;
  if (order_vec_top_ask_index_ != -1) {
    for (int ask_index_ = order_vec_top_ask_index_; ask_index_ >= order_vec_bottom_ask_index_; ask_index_--) {
      const std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[ask_index_];
      for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
        BaseOrder* p_this_order_ = _this_base_order_vec_[i];
        if (p_this_order_ != nullptr) {
          t_temp_oss_ << GetTradeTypeChar(p_this_order_->buysell()) << " " << p_this_order_->price() << " "
                      << p_this_order_->size_remaining() << " " << ToString(p_this_order_->order_status())
                      << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                      << " SAOS: " << p_this_order_->server_assigned_order_sequence()
                      << " sA: " << p_this_order_->queue_size_ahead() << " sB: " << p_this_order_->queue_size_behind()
                      << std::endl;
        }
      }
    }
  }

  return t_temp_oss_.str();
}

void BaseOrderManager::LogFullStatus() {
  DBGLOG_TIME << "Unsequenced Bids" << DBGLOG_ENDL_FLUSH;
  for (std::vector<BaseOrder*>::const_iterator base_order_vec_citer_ = unsequenced_bids_.begin();
       base_order_vec_citer_ != unsequenced_bids_.end(); base_order_vec_citer_++) {
    const BaseOrder* p_this_order_ = (*base_order_vec_citer_);
    if (p_this_order_ != nullptr) {
      DBGLOG_TIME << GetTradeTypeChar(p_this_order_->buysell()) << " " << p_this_order_->price() << " int("
                  << p_this_order_->int_price() << ")"
                  << " " << p_this_order_->size_remaining()
                  << " CAOS: " << p_this_order_->client_assigned_order_sequence() << DBGLOG_ENDL_FLUSH;
    }
  }

  DBGLOG_TIME << "UnconfBidMap" << DBGLOG_ENDL_FLUSH;
  if (unconfirmed_bottom_bid_index_ != -1) {
    for (int bid_index_ = unconfirmed_top_bid_index_; bid_index_ >= unconfirmed_bottom_bid_index_; bid_index_--) {
      if (sum_bid_unconfirmed_[bid_index_] > 0) {
        DBGLOG_TIME << "B " << GetBidIntPrice(bid_index_) << " sz " << sum_bid_unconfirmed_[bid_index_]
                    << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  DBGLOG_TIME << "Unsequenced Asks" << DBGLOG_ENDL_FLUSH;
  for (std::vector<BaseOrder*>::const_iterator base_order_vec_citer_ = unsequenced_asks_.begin();
       base_order_vec_citer_ != unsequenced_asks_.end(); base_order_vec_citer_++) {
    const BaseOrder* p_this_order_ = (*base_order_vec_citer_);
    if (p_this_order_ != nullptr) {
      DBGLOG_TIME << GetTradeTypeChar(p_this_order_->buysell()) << " " << p_this_order_->price() << " int("
                  << p_this_order_->int_price() << ")"
                  << " " << p_this_order_->size_remaining()
                  << " CAOS: " << p_this_order_->client_assigned_order_sequence() << DBGLOG_ENDL_FLUSH;
    }
  }

  DBGLOG_TIME << "UnconfAskMap" << DBGLOG_ENDL_FLUSH;
  if (unconfirmed_top_ask_index_ != -1) {
    for (int ask_index_ = unconfirmed_top_ask_index_; ask_index_ >= unconfirmed_bottom_ask_index_; ask_index_--) {
      if (sum_ask_unconfirmed_[ask_index_] > 0) {
        DBGLOG_TIME << "A " << GetAskIntPrice(ask_index_) << " sz " << sum_ask_unconfirmed_[ask_index_]
                    << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  DBGLOG_TIME << "Sequenced Bids" << DBGLOG_ENDL_FLUSH;
  if (order_vec_top_bid_index_ != -1) {
    for (int bid_index_ = order_vec_top_bid_index_; bid_index_ >= order_vec_bottom_bid_index_; bid_index_--) {
      const std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[bid_index_];
      for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
        BaseOrder* p_this_order_ = _this_base_order_vec_[i];
        if (p_this_order_ != nullptr) {
          DBGLOG_TIME << GetTradeTypeChar(p_this_order_->buysell()) << " " << p_this_order_->price() << " int("
                      << p_this_order_->int_price() << ")"
                      << " " << p_this_order_->size_remaining() << " " << ToString(p_this_order_->order_status())
                      << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                      << " SAOS: " << p_this_order_->server_assigned_order_sequence()
                      << " sA: " << p_this_order_->queue_size_ahead() << " sB: " << p_this_order_->queue_size_behind()
                      << " msecs_since_seqd: " << ((p_this_order_->seqd_msecs_ >= 0)
                                                       ? (watch_.msecs_from_midnight() - p_this_order_->seqd_msecs_)
                                                       : (0))
                      << " msecs_since_conf: " << ((p_this_order_->placed_msecs_ >= 0)
                                                       ? (watch_.msecs_from_midnight() - p_this_order_->placed_msecs_)
                                                       : (0)) << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  DBGLOG_TIME << "ConfBidMap" << DBGLOG_ENDL_FLUSH;
  if (confirmed_bottom_bid_index_ != -1) {
    for (int bid_index_ = confirmed_top_bid_index_; bid_index_ >= confirmed_bottom_bid_index_; bid_index_--) {
      if (sum_bid_confirmed_[bid_index_] > 0) {
        DBGLOG_TIME << "B " << GetBidIntPrice(bid_index_) << " sz " << sum_bid_confirmed_[bid_index_]
                    << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  DBGLOG_TIME << "Sequenced Asks" << DBGLOG_ENDL_FLUSH;
  if (order_vec_top_ask_index_ != -1) {
    for (int ask_index_ = order_vec_top_ask_index_; ask_index_ >= order_vec_bottom_ask_index_; ask_index_--) {
      const std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[ask_index_];
      for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
        BaseOrder* p_this_order_ = _this_base_order_vec_[i];
        if (p_this_order_ != nullptr) {
          DBGLOG_TIME << GetTradeTypeChar(p_this_order_->buysell()) << " " << p_this_order_->price() << " int("
                      << p_this_order_->int_price() << ")"
                      << " " << p_this_order_->size_remaining() << " " << ToString(p_this_order_->order_status())
                      << " CAOS: " << p_this_order_->client_assigned_order_sequence()
                      << " SAOS: " << p_this_order_->server_assigned_order_sequence()
                      << " sA: " << p_this_order_->queue_size_ahead() << " sB: " << p_this_order_->queue_size_behind()
                      << " msecs_since_seqd: " << ((p_this_order_->seqd_msecs_ >= 0)
                                                       ? (watch_.msecs_from_midnight() - p_this_order_->seqd_msecs_)
                                                       : (0))
                      << " msecs_since_conf: " << ((p_this_order_->placed_msecs_ >= 0)
                                                       ? (watch_.msecs_from_midnight() - p_this_order_->placed_msecs_)
                                                       : (0)) << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  DBGLOG_TIME << "ConfAskMap" << DBGLOG_ENDL_FLUSH;
  if (confirmed_top_ask_index_ != -1) {
    for (int ask_index_ = confirmed_top_ask_index_; ask_index_ >= confirmed_bottom_ask_index_; ask_index_--) {
      if (sum_ask_confirmed_[ask_index_] > 0) {
        DBGLOG_TIME << "A " << GetAskIntPrice(ask_index_) << " sz " << sum_ask_confirmed_[ask_index_]
                    << DBGLOG_ENDL_FLUSH;
      }
    }
  }
}

std::string BaseOrderManager::OMToString() {
  std::ostringstream t_oss_;

  std::map<int, int> bid_int_px_to_size_;
  std::map<int, int> ask_int_px_to_size_;

  if (confirmed_bottom_bid_index_ != -1) {
    for (int bid_index_ = confirmed_top_bid_index_; bid_index_ >= confirmed_bottom_bid_index_; bid_index_--) {
      if (sum_bid_confirmed_[bid_index_] > 0) {
        bid_int_px_to_size_[GetBidIntPrice(bid_index_)] = sum_bid_confirmed_[bid_index_];
      }
    }
  }

  if (confirmed_top_ask_index_ != -1) {
    for (int ask_index_ = confirmed_top_ask_index_; ask_index_ >= confirmed_bottom_ask_index_; ask_index_--) {
      if (sum_ask_confirmed_[ask_index_] > 0) {
        ask_int_px_to_size_[GetAskIntPrice(ask_index_)] = sum_ask_confirmed_[ask_index_];
      }
    }
  }

  std::map<int, int>::reverse_iterator itr_bid_int_px_to_size_ = bid_int_px_to_size_.rbegin();
  std::map<int, int>::iterator itr_ask_int_px_to_size_ = ask_int_px_to_size_.begin();

  for (int num_lines_ = 0; num_lines_ < 5; ++num_lines_) {
    if (itr_bid_int_px_to_size_ != bid_int_px_to_size_.rend()) {
      t_oss_ << "                      " << std::setw(3) << std::setfill(' ') << itr_bid_int_px_to_size_->second << " "
             << std::setw(8) << std::setfill(' ') << itr_bid_int_px_to_size_->first;
      ++itr_bid_int_px_to_size_;
    } else {
      t_oss_ << "                      " << std::setw(3) << std::setfill(' ') << "--"
             << " " << std::setw(8) << std::setfill(' ') << "-----";
    }

    t_oss_ << " X";

    if (itr_ask_int_px_to_size_ != ask_int_px_to_size_.end()) {
      t_oss_ << std::setw(8) << std::setfill(' ') << itr_ask_int_px_to_size_->first << " " << std::setw(3)
             << std::setfill(' ') << itr_ask_int_px_to_size_->second;
      ++itr_ask_int_px_to_size_;
    } else {
      t_oss_ << std::setw(8) << std::setfill(' ') << "-----"
             << " " << std::setw(3) << std::setfill(' ') << "--";
    }

    t_oss_ << "                     \n";
  }

  return t_oss_.str();
}

void BaseOrderManager::CleanSumSizeMapsBasedOnOrderMaps() {
  if (confirmed_bottom_bid_index_ != -1) {
    for (int bid_index_ = confirmed_top_bid_index_; bid_index_ >= confirmed_bottom_bid_index_; bid_index_--) {
      if (sum_bid_confirmed_[bid_index_] > 0) {
        int this_size_ = 0;
        int this_order_count_ = 0;
        std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[bid_index_];

        for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
             _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
          BaseOrder* p_this_order_ = (*_base_order_vec_iter_);
          if (p_this_order_ != nullptr) {
            this_size_ += p_this_order_->size_remaining();
            this_order_count_ += 1;
          }
        }

        if (this_size_ < sum_bid_confirmed_[bid_index_]) {
          sum_bid_confirmed_[bid_index_] = this_size_;
          sum_bid_confirmed_orders_[bid_index_] = this_order_count_;
          CalculateSumVectorConfirmed(kTradeTypeBuy, -1);
          AdjustTopBottomConfirmedBidIndexes(bid_index_);
        }
      }
    }
  }

  if (confirmed_top_ask_index_ != -1) {
    for (int ask_index_ = confirmed_top_ask_index_; ask_index_ >= confirmed_bottom_ask_index_; ask_index_--) {
      if (sum_ask_confirmed_[ask_index_] > 0) {
        int this_size_ = 0;
        int this_order_count_ = 0;
        std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[ask_index_];

        for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
             _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
          BaseOrder* p_this_order_ = (*_base_order_vec_iter_);
          if (p_this_order_ != nullptr) {
            this_size_ += p_this_order_->size_remaining();
            this_order_count_ += 1;
          }
        }

        if (this_size_ < sum_ask_confirmed_[ask_index_]) {
          sum_ask_confirmed_[ask_index_] = this_size_;
          sum_bid_confirmed_orders_[ask_index_] = this_order_count_;
          CalculateSumVectorConfirmed(kTradeTypeSell, -1);
          AdjustTopBottomConfirmedAskIndexes(ask_index_);
        }
      }
    }
  }
}

bool BaseOrderManager::IOCOrderExists() {
  if (order_vec_top_bid_index_ != -1) {
    for (int bid_index = order_vec_top_bid_index_; bid_index >= order_vec_bottom_bid_index_; bid_index--) {
      for (auto order : bid_order_vec_[bid_index]) {
        if (order->is_ioc_) {
          return true;
        }
      }
    }
  }

  if (order_vec_top_ask_index_ != -1) {
    for (int ask_index = order_vec_top_ask_index_; ask_index >= order_vec_bottom_ask_index_; ask_index--) {
      for (auto order : ask_order_vec_[ask_index]) {
        if (order->is_ioc_) {
          return true;
        }
      }
    }
  }

  return false;
}

int BaseOrderManager::KeepBidSizeInPriceRange(int _keep_size_, int _bottom_px_, int _top_px_) {
  int retval = 0;

  if (order_vec_top_bid_index_ != -1) {
    int t_bottom_idx_ = _bottom_px_ == kInvalidIntPrice
                            ? order_vec_bottom_bid_index_
                            : std::max(order_vec_bottom_bid_index_, GetBidIndex(_bottom_px_));
    int t_top_idx_ = _top_px_ == kInvalidIntPrice ? order_vec_top_bid_index_
                                                  : std::min(order_vec_top_bid_index_, GetBidIndex(_top_px_));

    for (int index_ = t_top_idx_; index_ >= t_bottom_idx_; index_--) {
      std::vector<BaseOrder*>& _this_base_order_vec_ = bid_order_vec_[index_];
      if (!_this_base_order_vec_.empty()) {
        for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
          BaseOrder* _p_this_order_ = _this_base_order_vec_[i];
          if (_p_this_order_ == nullptr) {
            continue;
          }

          if (_keep_size_ <= 0) {
            if (Cancel(*_p_this_order_)) {
              retval += _p_this_order_->size_remaining();
            }
          } else if (!_this_base_order_vec_[i]->canceled_) {
            _keep_size_ -= _this_base_order_vec_[i]->size_remaining();
          }
        }
      }
    }
  }
  return retval;
}

int BaseOrderManager::KeepAskSizeInPriceRange(int _keep_size_, int _bottom_px_, int _top_px_) {
  int retval = 0;

  if (order_vec_top_ask_index_ != -1) {
    int t_bottom_idx_ = _bottom_px_ == kInvalidIntPrice
                            ? order_vec_bottom_ask_index_
                            : std::max(order_vec_bottom_ask_index_, GetAskIndex(_bottom_px_));
    int t_top_idx_ = _top_px_ == kInvalidIntPrice ? order_vec_top_ask_index_
                                                  : std::min(order_vec_top_ask_index_, GetAskIndex(_top_px_));

    for (int index_ = t_top_idx_; index_ >= t_bottom_idx_; index_--) {
      std::vector<BaseOrder*>& _this_base_order_vec_ = ask_order_vec_[index_];
      if (!_this_base_order_vec_.empty()) {
        for (auto i = 0u; i < _this_base_order_vec_.size(); i++) {
          BaseOrder* _p_this_order_ = _this_base_order_vec_[i];
          if (_p_this_order_ == nullptr) {
            continue;
          }

          if (_keep_size_ <= 0) {
            if (Cancel(*_p_this_order_)) {
              retval += _p_this_order_->size_remaining();
            }
          } else if (!_this_base_order_vec_[i]->canceled_) {
            _keep_size_ -= _this_base_order_vec_[i]->size_remaining();
          }
        }
      }
    }
  }
  return retval;
}
}
