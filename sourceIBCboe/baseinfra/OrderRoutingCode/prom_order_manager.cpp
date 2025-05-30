/**
   \file OrderRoutingCode/prom_order_manager.cpp

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

#include "baseinfra/OrderRouting/prom_order_manager.hpp"

namespace HFSAT {

std::map<std::string, PromOrderManager*> PromOrderManager::shortcode_instance_map_;
long double PromOrderManager::total_traded_value = 0;

PromOrderManager* PromOrderManager::GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                      SecurityNameIndexer& _sec_name_indexer_,
                                                      const std::string& _dep_shortcode_,
                                                      const unsigned int _security_id_, const char* _exchange_symbol_) {
  if (shortcode_instance_map_.find(_dep_shortcode_) == shortcode_instance_map_.end()) {
    shortcode_instance_map_[_dep_shortcode_] = new PromOrderManager(_dbglogger_, _watch_, _sec_name_indexer_,
                                                                    _dep_shortcode_, _security_id_, _exchange_symbol_);
  }
  return shortcode_instance_map_[_dep_shortcode_];
}

void PromOrderManager::RemoveUniqueInstance(std::string shortcode) {
  if (shortcode_instance_map_.find(shortcode) != shortcode_instance_map_.end()) {
    delete shortcode_instance_map_[shortcode];
    shortcode_instance_map_.erase(shortcode);
  }
}

PromOrderManager::PromOrderManager(DebugLogger& _dbglogger_, const Watch& _watch_,
                                   SecurityNameIndexer& _sec_name_indexer_, const std::string& _dep_shortcode_,
                                   const unsigned int _security_id_, const char* _exchange_symbol_)
    : dbglogger_(_dbglogger_),
      watch_(_watch_),
      sec_name_indexer_(_sec_name_indexer_),
      dep_shortcode_(_dep_shortcode_),
      dep_security_id_(_security_id_),
      dep_symbol_(_exchange_symbol_),
      enabled_(true),
      intpx_2_sum_bid_confirmed_(),
      intpx_2_sum_bid_orders_confirmed_(),
      intpx_2_sum_bid_unconfirmed_(),
      intpx_2_bid_order_vec_(),
      intpx_2_sum_ask_confirmed_(),
      intpx_2_sum_ask_orders_confirmed_(),
      intpx_2_sum_ask_unconfirmed_(),
      intpx_2_ask_order_vec_(),
      num_unconfirmed_orders_(0),
      global_position_change_listener_vec_(),
      global_order_change_listener_vec_(),
      global_order_exec_listener_vec_(),
      global_position_(0),
      min_price_increment_(SecurityDefinitions::GetContractMinPriceIncrement(_dep_shortcode_, watch_.YYYYMMDD())),
      num_ors_messages_(0),
      total_traded_(0),
      total_traded_size_(0),
      total_rejects_(0),
      total_exch_rejects_(0),
      total_exch_cxl_rejects_(0),
      total_cxl_rejects_(0),
      total_exchange_replace_rejects(0),
      total_ors_replace_rejects(0),
      last_order_rejection_reason_(0),
      last_reject_encountered_time_(0),
      last_exec_bid_price_(kInvalidPrice),
      last_exec_bid_intpx_(kInvalidIntPrice),
      last_exec_ask_price_(kInvalidPrice),
      last_exec_ask_intpx_(kInvalidIntPrice),
      ors_best_bid_ask_(),
      mkt_affirmed_bid_ask_(),
      mfm_ordered_bid_ask_updates_(),
      use_smart_non_self_(false),  // This should remain false in non-trading execs.
      manage_orders_also_(false) {}

void PromOrderManager::OrderSequenced(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                      const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                                      const int _size_executed_, const int _client_position_,
                                      const int _global_position_, const int _int_price_,
                                      const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                      const ttime_t time_set_by_server) {
  if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
    PrintOpenOrders();
    DBGLOG_TIME_CLASS_FUNC_LINE << " bbsz: " << best_bid_size() << " bbpx: " << best_bid_price()
                                << " bapx: " << best_ask_price() << " basz: " << best_ask_size() << DBGLOG_ENDL_FLUSH;
  }
  if (!enabled_) {
    return;
  }

#ifdef MAINTAIN_TOTAL_REJECTS
  if (!total_rejects_ && (watch_.msecs_from_midnight() - last_reject_encountered_time_ > REJECT_REFRESH_TIME)) {
    total_rejects_ = 0;
    last_order_rejection_reason_ = HFSAT::kORSOrderAllowed;
  }
#endif  // MAINTAIN_TOTAL_REJECTS

  if (manage_orders_also_) {
    // If the client had set the int_price_ to 0 explicity but price_ is valid
    int t_int_price_ = (_int_price_ == 0) ? ((int)round(_price_ / min_price_increment_)) : _int_price_;

    if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " PromOrderOrderSeqdStart: "
                             << " SAOS: " << _server_assigned_order_sequence_
                             << " SID: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " PX: " << _price_
                             << " BS: " << GetTradeTypeChar(_buysell_) << " SZR: " << _size_remaining_
                             << " SZE: " << _size_executed_ << " GPOS: " << _global_position_
                             << " CPOS: " << _client_position_ << " SACI: " << _server_assigned_client_id_
                             << DBGLOG_ENDL_FLUSH;
    }

    switch (_buysell_) {
      case kTradeTypeBuy: {
        if (_size_remaining_ > 0) {  // find the order, if found this is a repeat message, as in the message was sent
                                     // earlier but probably the order was replayed
          // if this a repeat then ignore
          BaseOrder* p_this_order_ = FetchBidOrder(t_int_price_, _server_assigned_order_sequence_);
          if (p_this_order_ == NULL) {  // did not find order, hence new order .. not a reply to a replay request

            // create and add order to vec
            BaseOrder* p_new_order_ =
                baseorder_mempool_.Alloc(); /* do we need to allocate memory, can't we just use the struct ? */
            // p_new_order_->security_name_ = dep_symbol_ ; // ignoring since not used anywhere
            p_new_order_->buysell_ = _buysell_;
            p_new_order_->price_ = _price_;
            p_new_order_->size_remaining_ = _size_remaining_;
            // p_new_order_->size_requested_ = _size_remaining_ ; // ignoring since not used anywhere
            p_new_order_->int_price_ = t_int_price_;
            p_new_order_->order_status_ = kORRType_Seqd;  // Sequenced

            // p_new_order_->queue_size_ahead_ = 0; // ignoring since not used anywhere
            // p_new_order_->queue_size_behind_ = 0; // ignoring since not used anywhere
            p_new_order_->num_events_seen_ = 0;
            p_new_order_->placed_msecs_ = 0;

            // p_new_order_->client_assigned_order_sequence_ = _client_assigned_order_sequence_ ; // ignoring since not
            // used anywhere
            p_new_order_->server_assigned_order_sequence_ = _server_assigned_order_sequence_;

            p_new_order_->size_executed_ = _size_executed_;
            // p_new_order_->canceled_ = false ; // ignoring since not used anywhere
            // p_new_order_->to_be_canceled_ = false ; // ignoring since not used anywhere
            // p_new_order_->replayed_ = false ; // ignoring since not used anywhere

            intpx_2_bid_order_vec_[t_int_price_].push_back(p_new_order_);
            num_unconfirmed_orders_++;
            intpx_2_sum_bid_unconfirmed_[t_int_price_] += _size_remaining_;

            NotifyGlobalOrderChangeListeners(_buysell_, t_int_price_);

          } else {
            // do nothing if order found
          }
        }

      } break;
      case kTradeTypeSell: {
        if (_size_remaining_ > 0) {  // find the order, if found this is a repeat message, as in the message was sent
                                     // earlier but probably the order was replayed
          // if this a repeat then ignore
          BaseOrder* p_this_order_ = FetchAskOrder(t_int_price_, _server_assigned_order_sequence_);
          if (p_this_order_ == NULL) {  // did not find order, hence new order .. not a reply to a replay request

            // create and add order to vec
            BaseOrder* p_new_order_ =
                baseorder_mempool_.Alloc(); /* do we need to allocate memory, can't we just use the struct ? */
            // p_new_order_->security_name_ = dep_symbol_ ;
            p_new_order_->buysell_ = _buysell_;
            p_new_order_->price_ = _price_;
            p_new_order_->size_remaining_ = _size_remaining_;
            // p_new_order_->size_requested_ = _size_remaining_ ; // ignoring since not used anywhere
            p_new_order_->int_price_ = t_int_price_;
            p_new_order_->order_status_ = kORRType_Seqd;  // Sequenced

            // p_new_order_->queue_size_ahead_ = 0; // ignoring since not used anywhere
            // p_new_order_->queue_size_behind_ = 0; // ignoring since not used anywhere
            p_new_order_->num_events_seen_ = 0;
            p_new_order_->placed_msecs_ = 0;

            // p_new_order_->client_assigned_order_sequence_ = _client_assigned_order_sequence_ ; // ignoring since not
            // used anywhere
            p_new_order_->server_assigned_order_sequence_ = _server_assigned_order_sequence_;

            p_new_order_->size_executed_ = _size_executed_;
            // p_new_order_->canceled_ = false ; // ignoring since not used anywhere
            // p_new_order_->to_be_canceled_ = false ; // ignoring since not used anywhere
            // p_new_order_->replayed_ = false ; // ignoring since not used anywhere

            intpx_2_ask_order_vec_[t_int_price_].push_back(p_new_order_);
            num_unconfirmed_orders_++;
            intpx_2_sum_ask_unconfirmed_[t_int_price_] += _size_remaining_;

            NotifyGlobalOrderChangeListeners(_buysell_, t_int_price_);

          } else {
            // do nothing if order found
          }
        }

      } break;
      default: {}
    }

    num_ors_messages_++;

    if (last_exec_ask_intpx_ == kInvalidIntPrice || last_exec_bid_intpx_ == kInvalidIntPrice) {  // just initialized
      last_exec_bid_price_ = last_exec_ask_price_ = _price_;
      last_exec_bid_intpx_ = last_exec_ask_intpx_ = _int_price_;
    }

    if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " PromOrderOrderSeqdEnd: "
                             << " SAOS: " << _server_assigned_order_sequence_
                             << " SID: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " PX: " << _price_
                             << " BS: " << GetTradeTypeChar(_buysell_) << " SZR: " << _size_remaining_
                             << " SZE: " << _size_executed_ << " GPOS: " << _global_position_
                             << " CPOS: " << _client_position_ << " SACI: " << _server_assigned_client_id_
                             << DBGLOG_ENDL_FLUSH;
    }

    AdjustGlobalPosition(_global_position_, last_exec_bid_price_, last_exec_ask_price_);
  } else {
    last_exec_bid_price_ = last_exec_ask_price_ = _price_;
    AdjustGlobalPosition(_global_position_, last_exec_bid_price_, last_exec_ask_price_);
  }
}

void PromOrderManager::OrderConfirmed(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                      const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                                      const int _size_executed_, const int _client_position_,
                                      const int _global_position_, const int _int_price_,
                                      const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                      const ttime_t time_set_by_server) {
  if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
    PrintOpenOrders();
    DBGLOG_TIME_CLASS_FUNC_LINE << " bbsz: " << best_bid_size() << " bbpx: " << best_bid_price()
                                << " bapx: " << best_ask_price() << " basz: " << best_ask_size() << DBGLOG_ENDL_FLUSH;
  }
  if (!enabled_) {
    return;
  }

#ifdef MAINTAIN_TOTAL_REJECTS
  if (!total_rejects_ && (watch_.msecs_from_midnight() - last_reject_encountered_time_ > REJECT_REFRESH_TIME)) {
    total_rejects_ = 0;
    last_order_rejection_reason_ = HFSAT::kORSOrderAllowed;
  }
#endif  // MAINTAIN_TOTAL_REJECTS

  if (manage_orders_also_) {
    // If the client had set the int_price_ to 0 explicity but price_ is valid
    int t_int_price_ = (_int_price_ == 0) ? ((int)round(_price_ / min_price_increment_)) : _int_price_;

    if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " PromOrderOrderConfirmStart: "
                             << " SAOS: " << _server_assigned_order_sequence_
                             << " SID: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " PX: " << _price_
                             << " BS: " << GetTradeTypeChar(_buysell_) << " SZR: " << _size_remaining_
                             << " SZE: " << _size_executed_ << " GPOS: " << _global_position_
                             << " CPOS: " << _client_position_ << " SACI: " << _server_assigned_client_id_
                             << DBGLOG_ENDL_FLUSH;
    }
    switch (_buysell_) {
      case kTradeTypeBuy: {
        if (_size_remaining_ > 0) {  // find the order
          BaseOrder* p_this_order_ = FetchBidOrder(t_int_price_, _server_assigned_order_sequence_);
          if (p_this_order_ == NULL) {  // did not find order .. missed Sequenced message
            // create and add order to vec

            BaseOrder* p_new_order_ =
                baseorder_mempool_.Alloc(); /* do we need to allocate memory, can't we just use a struct ? */
            // p_new_order_->security_name_ = dep_symbol_ ; // ignoring since not used anywhere
            p_new_order_->buysell_ = _buysell_;
            p_new_order_->price_ = _price_;
            p_new_order_->size_remaining_ = _size_remaining_;
            // p_new_order_->size_requested_ = _size_remaining_ ; // ignoring since not used anywhere
            p_new_order_->int_price_ = t_int_price_;
            p_new_order_->order_status_ = kORRType_Conf;  // Confirmed

            // p_new_order_->queue_size_ahead_ = 0; // ignoring since not used anywhere
            // p_new_order_->queue_size_behind_ = 0; // ignoring since not used anywhere
            p_new_order_->num_events_seen_ = 0;
            p_new_order_->placed_msecs_ = 0;

            // p_new_order_->client_assigned_order_sequence_ = _client_assigned_order_sequence_ ; // ignoring since not
            // used anywhere
            p_new_order_->server_assigned_order_sequence_ = _server_assigned_order_sequence_;

            p_new_order_->size_executed_ = _size_executed_;
            // p_new_order_->canceled_ = false ; // ignoring since not used anywhere
            // p_new_order_->to_be_canceled_ = false ; // ignoring since not used anywhere
            // p_new_order_->replayed_ = false ; // ignoring since not used anywhere

            intpx_2_bid_order_vec_[t_int_price_].push_back(p_new_order_);
            intpx_2_sum_bid_confirmed_[t_int_price_] += _size_remaining_;
            intpx_2_sum_bid_orders_confirmed_[t_int_price_]++;

            // TODO_OPT : Is the following part needed only under remove_self_orders_from_book_ or use_smart_non_self_
            // If so then we should not do it all the time !
            // Update best bid info.
            if (ors_best_bid_ask_.best_bid_int_price_ == 0 || ors_best_bid_ask_.best_bid_int_price_ <= t_int_price_) {
              BestBidAskInfo t_ors_best_bid_ask_(ors_best_bid_ask_);

              ors_best_bid_ask_.best_bid_int_price_ = t_int_price_;
              ors_best_bid_ask_.best_bid_size_ = intpx_2_sum_bid_confirmed_[t_int_price_];
              ors_best_bid_ask_.best_bid_orders_ = intpx_2_sum_bid_orders_confirmed_[t_int_price_];

              if (use_smart_non_self_) {
                // Find the incremental update to expect in mkt. data.
                BestBidAskInfo t_best_bid_info_ =
                    BestBidAskInfo(ors_best_bid_ask_.best_bid_int_price_, 0,
                                   (ors_best_bid_ask_.best_bid_int_price_ == t_ors_best_bid_ask_.best_bid_int_price_)
                                       ? (ors_best_bid_ask_.best_bid_size_ - t_ors_best_bid_ask_.best_bid_size_)
                                       : ors_best_bid_ask_.best_bid_size_,
                                   0, (ors_best_bid_ask_.best_bid_int_price_ == t_ors_best_bid_ask_.best_bid_int_price_)
                                          ? (ors_best_bid_ask_.best_bid_orders_ - t_ors_best_bid_ask_.best_bid_orders_)
                                          : ors_best_bid_ask_.best_bid_orders_,
                                   0, watch_.msecs_from_midnight());

                mfm_ordered_bid_ask_updates_.push_back(t_best_bid_info_);

                if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
                  DBGLOG_TIME_CLASS_FUNC << " Added to mfm_ordered_bid_ask_updates_ : " << t_best_bid_info_.ToString()
                                         << DBGLOG_ENDL_FLUSH;
                }

                if (mfm_ordered_bid_ask_updates_.size() > 500) {  // 500+ unrecognized updates in 5 msecs !
                  if (dbglogger_.CheckLoggingLevel(SMVSELF_ERROR)) {
                    DBGLOG_TIME_CLASS_FUNC << "Warning : Clearing mfm_ordered_bid_ask_updates_." << DBGLOG_ENDL_FLUSH;
                  }
                  mfm_ordered_bid_ask_updates_.clear();
                }
              }
            }
          } else {
            // If this is a repsonse to replay messaage then
            // if already found in the map as KORRType_Conf then just reuturn
            if (p_this_order_->order_status_ == kORRType_Conf) {
              // We dont decrease unconfirmed order count and increment the
              // intpx_2_sum_bid_confirmed_ map size corresponding to that price
              // Dont notify to the GlobalOrderChange Listener
              return;
            }

            num_unconfirmed_orders_--;
            intpx_2_sum_bid_unconfirmed_[p_this_order_->int_price()] -= p_this_order_->size_remaining();

            p_this_order_->order_status_ = kORRType_Conf;  // Confirmed
            intpx_2_sum_bid_confirmed_[p_this_order_->int_price()] += _size_remaining_;
            intpx_2_sum_bid_orders_confirmed_[p_this_order_->int_price()]++;
            p_this_order_->size_remaining_ = _size_remaining_;
            // p_this_order_->size_requested_ = _size_remaining_ ; // ignoring since not used anywhere

            // TODO_OPT : Is the following part needed only under remove_self_orders_from_book_ or use_smart_non_self_
            // If so then we should not do it all the time !
            // Update best bid info.
            if (ors_best_bid_ask_.best_bid_int_price_ == 0 || ors_best_bid_ask_.best_bid_int_price_ <= t_int_price_) {
              if (use_smart_non_self_) {
                BestBidAskInfo t_ors_best_bid_ask_(ors_best_bid_ask_);

                ors_best_bid_ask_.best_bid_int_price_ = t_int_price_;
                ors_best_bid_ask_.best_bid_size_ = intpx_2_sum_bid_confirmed_[t_int_price_];
                ors_best_bid_ask_.best_bid_orders_ = intpx_2_sum_bid_orders_confirmed_[t_int_price_];

                // Find the incremental update to expect in mkt. data.
                BestBidAskInfo t_best_bid_info_ =
                    BestBidAskInfo(ors_best_bid_ask_.best_bid_int_price_, 0,
                                   (ors_best_bid_ask_.best_bid_int_price_ == t_ors_best_bid_ask_.best_bid_int_price_)
                                       ? (ors_best_bid_ask_.best_bid_size_ - t_ors_best_bid_ask_.best_bid_size_)
                                       : ors_best_bid_ask_.best_bid_size_,
                                   0, (ors_best_bid_ask_.best_bid_int_price_ == t_ors_best_bid_ask_.best_bid_int_price_)
                                          ? (ors_best_bid_ask_.best_bid_orders_ - t_ors_best_bid_ask_.best_bid_orders_)
                                          : ors_best_bid_ask_.best_bid_orders_,
                                   0, watch_.msecs_from_midnight());

                mfm_ordered_bid_ask_updates_.push_back(t_best_bid_info_);

                if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
                  DBGLOG_TIME_CLASS_FUNC << " Added to mfm_ordered_bid_ask_updates_ : " << t_best_bid_info_.ToString()
                                         << DBGLOG_ENDL_FLUSH;
                }

                if (mfm_ordered_bid_ask_updates_.size() > 500) {  // 500+ unrecognized updates in 5 msecs !
                  if (dbglogger_.CheckLoggingLevel(SMVSELF_ERROR)) {
                    DBGLOG_TIME_CLASS_FUNC << "Warning : Clearing mfm_ordered_bid_ask_updates_." << DBGLOG_ENDL_FLUSH;
                  }
                  mfm_ordered_bid_ask_updates_.clear();
                }
              } else {
                ors_best_bid_ask_.best_bid_int_price_ = t_int_price_;
                ors_best_bid_ask_.best_bid_size_ = intpx_2_sum_bid_confirmed_[t_int_price_];
                ors_best_bid_ask_.best_bid_orders_ = intpx_2_sum_bid_orders_confirmed_[t_int_price_];
              }
            }
          }

          NotifyGlobalOrderChangeListeners(_buysell_, t_int_price_);
        }

      } break;
      case kTradeTypeSell: {
        if (_size_remaining_ > 0) {  // find the order
          BaseOrder* p_this_order_ = FetchAskOrder(t_int_price_, _server_assigned_order_sequence_);
          if (p_this_order_ == NULL) {  // did not find order
            // create and add order to vec

            BaseOrder* p_new_order_ =
                baseorder_mempool_.Alloc(); /* do we need to allocate memory, can't we just use a struct ? */
            // p_new_order_->security_name_ = dep_symbol_ ; // ignoring since not used anywhere
            p_new_order_->buysell_ = _buysell_;
            p_new_order_->price_ = _price_;
            p_new_order_->size_remaining_ = _size_remaining_;
            // p_new_order_->size_requested_ = _size_remaining_ ; // ignoring since not used anywhere
            p_new_order_->int_price_ = t_int_price_;
            p_new_order_->order_status_ = kORRType_Conf;  // Confirmed

            // p_new_order_->queue_size_ahead_ = 0; // ignoring since not used anywhere
            // p_new_order_->queue_size_behind_ = 0; // ignoring since not used anywhere
            p_new_order_->num_events_seen_ = 0;
            p_new_order_->placed_msecs_ = 0;

            // p_new_order_->client_assigned_order_sequence_ = _client_assigned_order_sequence_ ; // ignoring since not
            // used anywhere
            p_new_order_->server_assigned_order_sequence_ = _server_assigned_order_sequence_;

            p_new_order_->size_executed_ = _size_executed_;
            // p_new_order_->canceled_ = false ; // ignoring since not used anywhere
            // p_new_order_->to_be_canceled_ = false ; // ignoring since not used anywhere
            // p_new_order_->replayed_ = false ; // ignoring since not used anywhere

            intpx_2_ask_order_vec_[t_int_price_].push_back(p_new_order_);
            intpx_2_sum_ask_confirmed_[t_int_price_] += _size_remaining_;
            intpx_2_sum_ask_orders_confirmed_[t_int_price_]++;

            // TODO_OPT : Is the following part needed only under remove_self_orders_from_book_ or use_smart_non_self_
            // If so then we should not do it all the time !
            // Update best ask info.
            if (ors_best_bid_ask_.best_ask_int_price_ == 0 || ors_best_bid_ask_.best_ask_int_price_ >= t_int_price_) {
              if (use_smart_non_self_) {
                BestBidAskInfo t_ors_best_bid_ask_(ors_best_bid_ask_);

                ors_best_bid_ask_.best_ask_int_price_ = t_int_price_;
                ors_best_bid_ask_.best_ask_size_ = intpx_2_sum_ask_confirmed_[t_int_price_];
                ors_best_bid_ask_.best_ask_orders_ = intpx_2_sum_ask_orders_confirmed_[t_int_price_];

                BestBidAskInfo t_best_ask_info_ =
                    BestBidAskInfo(0, ors_best_bid_ask_.best_ask_int_price_, 0,
                                   (ors_best_bid_ask_.best_ask_int_price_ == t_ors_best_bid_ask_.best_ask_int_price_)
                                       ? (ors_best_bid_ask_.best_ask_size_ - t_ors_best_bid_ask_.best_ask_size_)
                                       : ors_best_bid_ask_.best_ask_size_,
                                   0, (ors_best_bid_ask_.best_ask_int_price_ == t_ors_best_bid_ask_.best_ask_int_price_)
                                          ? (ors_best_bid_ask_.best_ask_orders_ - t_ors_best_bid_ask_.best_ask_orders_)
                                          : ors_best_bid_ask_.best_ask_orders_,
                                   watch_.msecs_from_midnight());

                mfm_ordered_bid_ask_updates_.push_back(t_best_ask_info_);

                if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
                  DBGLOG_TIME_CLASS_FUNC << " Added to mfm_ordered_bid_ask_updates_ : " << t_best_ask_info_.ToString()
                                         << DBGLOG_ENDL_FLUSH;
                }

                if (mfm_ordered_bid_ask_updates_.size() > 500) {  // 500+ unrecognized updates in 5 msecs !
                  if (dbglogger_.CheckLoggingLevel(SMVSELF_ERROR)) {
                    DBGLOG_TIME_CLASS_FUNC << "Warning : Clearing mfm_ordered_bid_ask_updates_." << DBGLOG_ENDL_FLUSH;
                  }
                  mfm_ordered_bid_ask_updates_.clear();
                }
              } else {
                ors_best_bid_ask_.best_ask_int_price_ = t_int_price_;
                ors_best_bid_ask_.best_ask_size_ = intpx_2_sum_ask_confirmed_[t_int_price_];
                ors_best_bid_ask_.best_ask_orders_ = intpx_2_sum_ask_orders_confirmed_[t_int_price_];
              }
            }
          } else {
            //@ramkris : If this is a repsonse to replay messaage then
            // if already found in the map as KORRType_Conf then just reuturn
            if (p_this_order_->order_status_ == kORRType_Conf) {
              // We dont decrease unconfirmed order count and increment the
              // intpx_2_sum_bid_confirmed_ map size corresponding to that price
              // Dont notify to the GlobalOrderChange Listener
              return;
            }

            // if order is sequenced but not confirmed
            num_unconfirmed_orders_--;
            intpx_2_sum_ask_unconfirmed_[p_this_order_->int_price()] -= p_this_order_->size_remaining();

            p_this_order_->order_status_ = kORRType_Conf;  // Confirmed
            intpx_2_sum_ask_confirmed_[p_this_order_->int_price()] += _size_remaining_;
            intpx_2_sum_ask_orders_confirmed_[p_this_order_->int_price()]++;
            p_this_order_->size_remaining_ = _size_remaining_;
            // p_this_order_->size_requested_ = _size_remaining_ ; // ignoring since not used anywhere

            // TODO_OPT : Is the following part needed only under remove_self_orders_from_book_ or use_smart_non_self_
            // If so then we should not do it all the time !
            // Update best ask info.
            if (ors_best_bid_ask_.best_ask_int_price_ == 0 || ors_best_bid_ask_.best_ask_int_price_ >= t_int_price_) {
              if (use_smart_non_self_) {
                BestBidAskInfo t_ors_best_bid_ask_(ors_best_bid_ask_);

                ors_best_bid_ask_.best_ask_int_price_ = t_int_price_;
                ors_best_bid_ask_.best_ask_size_ = intpx_2_sum_ask_confirmed_[t_int_price_];
                ors_best_bid_ask_.best_ask_orders_ = intpx_2_sum_ask_orders_confirmed_[t_int_price_];

                BestBidAskInfo t_best_ask_info_ =
                    BestBidAskInfo(0, ors_best_bid_ask_.best_ask_int_price_, 0,
                                   (ors_best_bid_ask_.best_ask_int_price_ == t_ors_best_bid_ask_.best_ask_int_price_)
                                       ? (ors_best_bid_ask_.best_ask_size_ - t_ors_best_bid_ask_.best_ask_size_)
                                       : ors_best_bid_ask_.best_ask_size_,
                                   0, (ors_best_bid_ask_.best_ask_int_price_ == t_ors_best_bid_ask_.best_ask_int_price_)
                                          ? (ors_best_bid_ask_.best_ask_orders_ - t_ors_best_bid_ask_.best_ask_orders_)
                                          : ors_best_bid_ask_.best_ask_orders_,
                                   watch_.msecs_from_midnight());

                mfm_ordered_bid_ask_updates_.push_back(t_best_ask_info_);

                if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
                  DBGLOG_TIME_CLASS_FUNC << " Added to mfm_ordered_bid_ask_updates_ : " << t_best_ask_info_.ToString()
                                         << DBGLOG_ENDL_FLUSH;
                }

                if (mfm_ordered_bid_ask_updates_.size() > 500) {  // 500+ unrecognized updates in 5 msecs !
                  if (dbglogger_.CheckLoggingLevel(SMVSELF_ERROR)) {
                    DBGLOG_TIME_CLASS_FUNC << "Warning : Clearing mfm_ordered_bid_ask_updates_." << DBGLOG_ENDL_FLUSH;
                  }
                  mfm_ordered_bid_ask_updates_.clear();
                }
              } else {
                ors_best_bid_ask_.best_ask_int_price_ = t_int_price_;
                ors_best_bid_ask_.best_ask_size_ = intpx_2_sum_ask_confirmed_[t_int_price_];
                ors_best_bid_ask_.best_ask_orders_ = intpx_2_sum_ask_orders_confirmed_[t_int_price_];
              }
            }
          }

          NotifyGlobalOrderChangeListeners(_buysell_, t_int_price_);
        }

      } break;
      default: {}
    }

    if (last_exec_ask_intpx_ == kInvalidIntPrice || last_exec_bid_intpx_ == kInvalidIntPrice) {  // just initialized
      last_exec_bid_price_ = last_exec_ask_price_ = _price_;
      last_exec_bid_intpx_ = last_exec_ask_intpx_ = _int_price_;
    }

    if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " PromOrderOrderConfirmEnd: "
                             << " SAOS: " << _server_assigned_order_sequence_
                             << " SID: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " PX: " << _price_
                             << " BS: " << GetTradeTypeChar(_buysell_) << " SZR: " << _size_remaining_
                             << " SZE: " << _size_executed_ << " GPOS: " << _global_position_
                             << " CPOS: " << _client_position_ << " SACI: " << _server_assigned_client_id_
                             << DBGLOG_ENDL_FLUSH;
    }

    num_ors_messages_++;

    /// not set in ORS for Confirm messages .. don't check for glob positions here
    //    AdjustGlobalPosition ( _global_position_ );
  } else {
    /// not set in ORS for Confirm messages .. don't check for glob positions here
    //    AdjustGlobalPosition ( _global_position_ );
  }
}

void PromOrderManager::OrderConfCxlReplaced(
    const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t _buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int _int_price_, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
    PrintOpenOrders();
    DBGLOG_TIME_CLASS_FUNC_LINE << " bbsz: " << best_bid_size() << " bbpx: " << best_bid_price()
                                << " bapx: " << best_ask_price() << " basz: " << best_ask_size() << DBGLOG_ENDL_FLUSH;
  }
  if (!enabled_) {
    return;
  }

#ifdef MAINTAIN_TOTAL_REJECTS
  if (!total_rejects_ && (watch_.msecs_from_midnight() - last_reject_encountered_time_ > REJECT_REFRESH_TIME)) {
    total_rejects_ = 0;
    last_order_rejection_reason_ = HFSAT::kORSOrderAllowed;
  }
#endif  // MAINTAIN_TOTAL_REJECTS

  if (manage_orders_also_) {
    // If the client had set the int_price_ to 0 explicity but price_ is valid
    int t_int_price_ = (_int_price_ == 0) ? ((int)round(_price_ / min_price_increment_)) : _int_price_;

    if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " PromOrderOrderCxlReplaceStart: "
                             << " SAOS: " << _server_assigned_order_sequence_
                             << " SID: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " PX: " << _price_
                             << " BS: " << GetTradeTypeChar(_buysell_) << " SZR: " << _size_remaining_
                             << " SZE: " << _size_executed_ << " GPOS: " << _global_position_
                             << " CPOS: " << _client_position_ << " SACI: " << _server_assigned_client_id_
                             << DBGLOG_ENDL_FLUSH;
    }

    // find the order by CAOS at the level t_int_price_
    switch (_buysell_) {
      case kTradeTypeBuy: {
        // find the order
        BaseOrder* p_this_order_ = FetchBidOrder(t_int_price_, _server_assigned_order_sequence_);
        if (p_this_order_ == NULL) {  // did not find order .. missed Sequenced message
          // create and add order to vec

          BaseOrder* p_new_order_ =
              baseorder_mempool_.Alloc(); /* do we need to allocate memory, can't we just use a struct ? */
          // p_new_order_->security_name_ = dep_symbol_ ; // ignoring since not used anywhere
          p_new_order_->buysell_ = _buysell_;
          p_new_order_->price_ = _price_;
          p_new_order_->size_remaining_ = _size_remaining_;
          // p_new_order_->size_requested_ = _size_remaining_ ; // ignoring since not used anywhere
          p_new_order_->int_price_ = t_int_price_;
          p_new_order_->order_status_ = kORRType_Conf;  // Confirmed

          // p_new_order_->queue_size_ahead_ = 0; // ignoring since not used anywhere
          // p_new_order_->queue_size_behind_ = 0; // ignoring since not used anywhere
          p_new_order_->num_events_seen_ = 0;
          p_new_order_->placed_msecs_ = 0;

          // p_new_order_->client_assigned_order_sequence_ = _client_assigned_order_sequence_ ; // ignoring since not
          // used anywhere
          p_new_order_->server_assigned_order_sequence_ = _server_assigned_order_sequence_;

          p_new_order_->size_executed_ = _size_executed_;
          // p_new_order_->canceled_ = false ; // ignoring since not used anywhere
          // p_new_order_->to_be_canceled_ = false ; // ignoring since not used anywhere
          // p_new_order_->replayed_ = false ; // ignoring since not used anywhere

          intpx_2_bid_order_vec_[t_int_price_].push_back(p_new_order_);
          intpx_2_sum_bid_confirmed_[t_int_price_] += _size_remaining_;
          intpx_2_sum_bid_orders_confirmed_[t_int_price_]++;

          // TODO_OPT : Is the following part needed only under remove_self_orders_from_book_ or use_smart_non_self_
          // If so then we should not do it all the time !
          // Update best bid info.
          if (ors_best_bid_ask_.best_bid_int_price_ == 0 || ors_best_bid_ask_.best_bid_int_price_ <= t_int_price_) {
            if (use_smart_non_self_) {
              BestBidAskInfo t_ors_best_bid_ask_(ors_best_bid_ask_);

              ors_best_bid_ask_.best_bid_int_price_ = t_int_price_;
              ors_best_bid_ask_.best_bid_size_ = intpx_2_sum_bid_confirmed_[t_int_price_];
              ors_best_bid_ask_.best_bid_orders_ = intpx_2_sum_bid_orders_confirmed_[t_int_price_];

              // Find the incremental update to expect in mkt. data.
              BestBidAskInfo t_best_bid_info_ =
                  BestBidAskInfo(ors_best_bid_ask_.best_bid_int_price_, 0,
                                 (ors_best_bid_ask_.best_bid_int_price_ == t_ors_best_bid_ask_.best_bid_int_price_)
                                     ? (ors_best_bid_ask_.best_bid_size_ - t_ors_best_bid_ask_.best_bid_size_)
                                     : ors_best_bid_ask_.best_bid_size_,
                                 0, (ors_best_bid_ask_.best_bid_int_price_ == t_ors_best_bid_ask_.best_bid_int_price_)
                                        ? (ors_best_bid_ask_.best_bid_orders_ - t_ors_best_bid_ask_.best_bid_orders_)
                                        : ors_best_bid_ask_.best_bid_orders_,
                                 0, watch_.msecs_from_midnight());

              mfm_ordered_bid_ask_updates_.push_back(t_best_bid_info_);

              if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
                DBGLOG_TIME_CLASS_FUNC << " Added to mfm_ordered_bid_ask_updates_ : " << t_best_bid_info_.ToString()
                                       << DBGLOG_ENDL_FLUSH;
              }

              if (mfm_ordered_bid_ask_updates_.size() > 500) {  // 500+ unrecognized updates in 5 msecs !
                if (dbglogger_.CheckLoggingLevel(SMVSELF_ERROR)) {
                  DBGLOG_TIME_CLASS_FUNC << "Warning : Clearing mfm_ordered_bid_ask_updates_." << DBGLOG_ENDL_FLUSH;
                }
                mfm_ordered_bid_ask_updates_.clear();
              }
            } else {
              ors_best_bid_ask_.best_bid_int_price_ = t_int_price_;
              ors_best_bid_ask_.best_bid_size_ = intpx_2_sum_bid_confirmed_[t_int_price_];
              ors_best_bid_ask_.best_bid_orders_ = intpx_2_sum_bid_orders_confirmed_[t_int_price_];
            }
          }
        } else {
          p_this_order_->server_assigned_order_sequence_ =
              _server_assigned_order_sequence_;  // assign because this is a CxRe and we are assigning new SAOS

          p_this_order_->order_status_ = kORRType_Conf;  // Confirmed

          if (p_this_order_->int_price() != _int_price_) {
            intpx_2_sum_bid_confirmed_[p_this_order_->int_price()] -=
                std::min(intpx_2_sum_bid_confirmed_[p_this_order_->int_price()], p_this_order_->size_remaining());
            intpx_2_sum_bid_confirmed_[_int_price_] += _size_remaining_;

            auto iter = intpx_2_bid_order_vec_.find(p_this_order_->int_price());
            if (iter != intpx_2_bid_order_vec_.end()) {
              for (unsigned i = 0; i < (iter->second).size(); i++) {
                if ((iter->second)[i] == p_this_order_) {
                  (iter->second).erase((iter->second).begin() + i);
                }
              }
            }
            intpx_2_bid_order_vec_[_int_price_].push_back(p_this_order_);
            p_this_order_->price_ = _price_;
            p_this_order_->int_price_ = _int_price_;
          } else {
            intpx_2_sum_bid_confirmed_[p_this_order_->int_price()] += _size_remaining_ - p_this_order_->size_remaining_;
          }

          // intpx_2_sum_bid_orders_confirmed_ [ t_int_price_ ] remains unchanged.
          p_this_order_->size_remaining_ = _size_remaining_;
          // p_this_order_->size_requested_ = _size_remaining_ ; // ignoring since not used anywhere

          // TODO_OPT : Is the following part needed only under remove_self_orders_from_book_ or use_smart_non_self_
          // If so then we should not do it all the time !
          // Update best bid info.
          if (ors_best_bid_ask_.best_bid_int_price_ == 0 || ors_best_bid_ask_.best_bid_int_price_ <= t_int_price_) {
            if (use_smart_non_self_) {
              BestBidAskInfo t_ors_best_bid_ask_(ors_best_bid_ask_);

              ors_best_bid_ask_.best_bid_int_price_ = t_int_price_;
              ors_best_bid_ask_.best_bid_size_ = intpx_2_sum_bid_confirmed_[t_int_price_];
              ors_best_bid_ask_.best_bid_orders_ = intpx_2_sum_bid_orders_confirmed_[t_int_price_];

              // Find the incremental update to expect in mkt. data.
              BestBidAskInfo t_best_bid_info_ =
                  BestBidAskInfo(ors_best_bid_ask_.best_bid_int_price_, 0,
                                 (ors_best_bid_ask_.best_bid_int_price_ == t_ors_best_bid_ask_.best_bid_int_price_)
                                     ? (ors_best_bid_ask_.best_bid_size_ - t_ors_best_bid_ask_.best_bid_size_)
                                     : ors_best_bid_ask_.best_bid_size_,
                                 0, (ors_best_bid_ask_.best_bid_int_price_ == t_ors_best_bid_ask_.best_bid_int_price_)
                                        ? (ors_best_bid_ask_.best_bid_orders_ - t_ors_best_bid_ask_.best_bid_orders_)
                                        : ors_best_bid_ask_.best_bid_orders_,
                                 0, watch_.msecs_from_midnight());

              mfm_ordered_bid_ask_updates_.push_back(t_best_bid_info_);

              if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
                DBGLOG_TIME_CLASS_FUNC << " Added to mfm_ordered_bid_ask_updates_ : " << t_best_bid_info_.ToString()
                                       << DBGLOG_ENDL_FLUSH;
              }

              if (mfm_ordered_bid_ask_updates_.size() > 500) {  // 500+ unrecognized updates in 5 msecs !
                if (dbglogger_.CheckLoggingLevel(SMVSELF_ERROR)) {
                  DBGLOG_TIME_CLASS_FUNC << "Warning : Clearing mfm_ordered_bid_ask_updates_." << DBGLOG_ENDL_FLUSH;
                }
                mfm_ordered_bid_ask_updates_.clear();
              }
            } else {
              ors_best_bid_ask_.best_bid_int_price_ = t_int_price_;
              ors_best_bid_ask_.best_bid_size_ = intpx_2_sum_bid_confirmed_[t_int_price_];
              ors_best_bid_ask_.best_bid_orders_ = intpx_2_sum_bid_orders_confirmed_[t_int_price_];
            }
          }
        }

        NotifyGlobalOrderChangeListeners(_buysell_, t_int_price_);
      } break;
      case kTradeTypeSell: {
        // find the order
        BaseOrder* p_this_order_ = FetchAskOrder(t_int_price_, _server_assigned_order_sequence_);
        if (p_this_order_ == NULL) {  // did not find order .. missed Sequenced message
          // create and add order to vec

          BaseOrder* p_new_order_ =
              baseorder_mempool_.Alloc(); /* do we need to allocate memory, can't we just use a struct ? */
          // p_new_order_->security_name_ = dep_symbol_ ; // ignoring since not used anywhere
          p_new_order_->buysell_ = _buysell_;
          p_new_order_->price_ = _price_;
          p_new_order_->size_remaining_ = _size_remaining_;
          // p_new_order_->size_requested_ = _size_remaining_ ; // ignoring since not used anywhere
          p_new_order_->int_price_ = t_int_price_;
          p_new_order_->order_status_ = kORRType_Conf;  // Confirmed

          // p_new_order_->queue_size_ahead_ = 0; // ignoring since not used anywhere
          // p_new_order_->queue_size_behind_ = 0; // ignoring since not used anywhere
          p_new_order_->num_events_seen_ = 0;
          p_new_order_->placed_msecs_ = 0;

          // p_new_order_->client_assigned_order_sequence_ = _client_assigned_order_sequence_ ; // ignoring since not
          // used anywhere
          p_new_order_->server_assigned_order_sequence_ = _server_assigned_order_sequence_;

          p_new_order_->size_executed_ = _size_executed_;
          // p_new_order_->canceled_ = false ; // ignoring since not used anywhere
          // p_new_order_->to_be_canceled_ = false ; // ignoring since not used anywhere
          // p_new_order_->replayed_ = false ; // ignoring since not used anywhere

          intpx_2_ask_order_vec_[t_int_price_].push_back(p_new_order_);
          intpx_2_sum_ask_confirmed_[t_int_price_] += _size_remaining_;
          intpx_2_sum_ask_orders_confirmed_[t_int_price_]++;

          // TODO_OPT : Is the following part needed only under remove_self_orders_from_book_ or use_smart_non_self_
          // If so then we should not do it all the time !
          // Update best ask info.
          if (ors_best_bid_ask_.best_ask_int_price_ == 0 || ors_best_bid_ask_.best_ask_int_price_ >= t_int_price_) {
            if (use_smart_non_self_) {
              BestBidAskInfo t_ors_best_bid_ask_(ors_best_bid_ask_);

              ors_best_bid_ask_.best_ask_int_price_ = t_int_price_;
              ors_best_bid_ask_.best_ask_size_ = intpx_2_sum_ask_confirmed_[t_int_price_];
              ors_best_bid_ask_.best_ask_orders_ = intpx_2_sum_ask_orders_confirmed_[t_int_price_];

              BestBidAskInfo t_best_ask_info_ =
                  BestBidAskInfo(0, ors_best_bid_ask_.best_ask_int_price_, 0,
                                 (ors_best_bid_ask_.best_ask_int_price_ == t_ors_best_bid_ask_.best_ask_int_price_)
                                     ? (ors_best_bid_ask_.best_ask_size_ - t_ors_best_bid_ask_.best_ask_size_)
                                     : ors_best_bid_ask_.best_ask_size_,
                                 0, (ors_best_bid_ask_.best_ask_int_price_ == t_ors_best_bid_ask_.best_ask_int_price_)
                                        ? (ors_best_bid_ask_.best_ask_orders_ - t_ors_best_bid_ask_.best_ask_orders_)
                                        : ors_best_bid_ask_.best_ask_orders_,
                                 watch_.msecs_from_midnight());

              mfm_ordered_bid_ask_updates_.push_back(t_best_ask_info_);

              if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
                DBGLOG_TIME_CLASS_FUNC << " Added to mfm_ordered_bid_ask_updates_ : " << t_best_ask_info_.ToString()
                                       << DBGLOG_ENDL_FLUSH;
              }

              if (mfm_ordered_bid_ask_updates_.size() > 500) {  // 500+ unrecognized updates in 5 msecs !
                if (dbglogger_.CheckLoggingLevel(SMVSELF_ERROR)) {
                  DBGLOG_TIME_CLASS_FUNC << "Warning : Clearing mfm_ordered_bid_ask_updates_." << DBGLOG_ENDL_FLUSH;
                }
                mfm_ordered_bid_ask_updates_.clear();
              }
            } else {
              ors_best_bid_ask_.best_ask_int_price_ = t_int_price_;
              ors_best_bid_ask_.best_ask_size_ = intpx_2_sum_ask_confirmed_[t_int_price_];
              ors_best_bid_ask_.best_ask_orders_ = intpx_2_sum_ask_orders_confirmed_[t_int_price_];
            }
          }
        } else {
          p_this_order_->server_assigned_order_sequence_ =
              _server_assigned_order_sequence_;  // assign because this is a CxRe and we are assigning new SAOS

          p_this_order_->order_status_ = kORRType_Conf;  // Confirmed
          if (p_this_order_->int_price() != _int_price_) {
            intpx_2_sum_ask_confirmed_[p_this_order_->int_price()] -=
                std::min(intpx_2_sum_ask_confirmed_[p_this_order_->int_price()], p_this_order_->size_remaining());
            intpx_2_sum_ask_confirmed_[_int_price_] += _size_remaining_;

            auto iter = intpx_2_ask_order_vec_.find(p_this_order_->int_price());
            if (iter != intpx_2_ask_order_vec_.end()) {
              for (unsigned i = 0; i < (iter->second).size(); i++) {
                if ((iter->second)[i] == p_this_order_) {
                  (iter->second).erase((iter->second).begin() + i);
                }
              }
            }
            intpx_2_ask_order_vec_[_int_price_].push_back(p_this_order_);

            p_this_order_->int_price_ = _int_price_;
            p_this_order_->price_ = _price_;
          } else {
            intpx_2_sum_ask_confirmed_[p_this_order_->int_price()] += _size_remaining_ - p_this_order_->size_remaining_;
          }

          // intpx_2_sum_ask_orders_confirmed_ [ t_int_price_ ] remains unchanged.
          p_this_order_->size_remaining_ = _size_remaining_;
          // p_this_order_->size_requested_ = _size_remaining_ ; // ignoring since not used anywhere

          // TODO_OPT : Is the following part needed only under remove_self_orders_from_book_ or use_smart_non_self_
          // If so then we should not do it all the time !
          // Update best ask info.
          if (ors_best_bid_ask_.best_ask_int_price_ == 0 || ors_best_bid_ask_.best_ask_int_price_ >= t_int_price_) {
            if (use_smart_non_self_) {
              BestBidAskInfo t_ors_best_bid_ask_(ors_best_bid_ask_);

              ors_best_bid_ask_.best_ask_int_price_ = t_int_price_;
              ors_best_bid_ask_.best_ask_size_ = intpx_2_sum_ask_confirmed_[t_int_price_];
              ors_best_bid_ask_.best_ask_orders_ = intpx_2_sum_ask_orders_confirmed_[t_int_price_];

              BestBidAskInfo t_best_ask_info_ =
                  BestBidAskInfo(0, ors_best_bid_ask_.best_ask_int_price_, 0,
                                 (ors_best_bid_ask_.best_ask_int_price_ == t_ors_best_bid_ask_.best_ask_int_price_)
                                     ? (ors_best_bid_ask_.best_ask_size_ - t_ors_best_bid_ask_.best_ask_size_)
                                     : ors_best_bid_ask_.best_ask_size_,
                                 0, (ors_best_bid_ask_.best_ask_int_price_ == t_ors_best_bid_ask_.best_ask_int_price_)
                                        ? (ors_best_bid_ask_.best_ask_orders_ - t_ors_best_bid_ask_.best_ask_orders_)
                                        : ors_best_bid_ask_.best_ask_orders_,
                                 watch_.msecs_from_midnight());

              mfm_ordered_bid_ask_updates_.push_back(t_best_ask_info_);

              if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
                DBGLOG_TIME_CLASS_FUNC << " Added to mfm_ordered_bid_ask_updates_ : " << t_best_ask_info_.ToString()
                                       << DBGLOG_ENDL_FLUSH;
              }

              if (mfm_ordered_bid_ask_updates_.size() > 500) {  // 500+ unrecognized updates in 5 msecs !
                if (dbglogger_.CheckLoggingLevel(SMVSELF_ERROR)) {
                  DBGLOG_TIME_CLASS_FUNC << "Warning : Clearing mfm_ordered_bid_ask_updates_." << DBGLOG_ENDL_FLUSH;
                }
                mfm_ordered_bid_ask_updates_.clear();
              }
            } else {
              ors_best_bid_ask_.best_ask_int_price_ = t_int_price_;
              ors_best_bid_ask_.best_ask_size_ = intpx_2_sum_ask_confirmed_[t_int_price_];
              ors_best_bid_ask_.best_ask_orders_ = intpx_2_sum_ask_orders_confirmed_[t_int_price_];
            }
          }
        }

        NotifyGlobalOrderChangeListeners(_buysell_, t_int_price_);
      } break;
      default: {}
    }

    num_ors_messages_++;

    if (last_exec_ask_intpx_ == kInvalidIntPrice || last_exec_bid_intpx_ == kInvalidIntPrice) {  // just initialized
      last_exec_bid_price_ = last_exec_ask_price_ = _price_;
      last_exec_bid_intpx_ = last_exec_ask_intpx_ = _int_price_;
    }

    AdjustGlobalPosition(_global_position_, last_exec_bid_price_, last_exec_ask_price_);

    // if ( dbglogger_.CheckLoggingLevel ( PROM_OM_INFO ) )
    //   {
    // 	DBGLOG_TIME_CLASS_FUNC << " PromOrderOrderCXlRepEnd: "
    // 		   << " SAOS: " << _server_assigned_order_sequence_
    // 		   << " SID: " << sec_name_indexer_.GetSecurityNameFromId ( _security_id_ )
    // 		   << " PX: " << _price_
    // 		   << " BS: " << GetTradeTypeChar ( _buysell_ )
    // 		   << " SZR: " << _size_remaining_
    // 		   << " SZE: " << _size_executed_
    // 		   << " GPOS: " << _global_position_
    // 		   << " CPOS: " << _client_position_
    // 		   << " SACI: " << _server_assigned_client_id_
    // 		   << DBGLOG_ENDL_FLUSH ;
    //   }
  } else {
    AdjustGlobalPosition(_global_position_, last_exec_bid_price_, last_exec_ask_price_);
  }
}

void PromOrderManager::setMargin( double margin_) {}


void PromOrderManager::OrderConfCxlReplaceRejected(
    const int server_assigned_client_id, const int client_assigned_order_sequence,
    const int server_assigned_order_sequence, const unsigned int security_id, const double price,
    const TradeType_t buysell, const int size_remaining, const int client_position, const int global_position,
    const int intprice, const int32_t rejection_reason, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
    PrintOpenOrders();
    DBGLOG_TIME_CLASS_FUNC_LINE << " bbsz: " << best_bid_size() << " bbpx: " << best_bid_price()
                                << " bapx: " << best_ask_price() << " basz: " << best_ask_size() << DBGLOG_ENDL_FLUSH;
  }
  if (!enabled_) {
    return;
  }

  if (rejection_reason == HFSAT::kExchCancelReplaceReject) {
    total_exchange_replace_rejects++;
    total_exch_rejects_++;
  } else {
    total_ors_replace_rejects++;
    total_rejects_++;
  }

  // TODO rejects count it not being tracked down
  last_reject_encountered_time_ = watch_.msecs_from_midnight();

  num_ors_messages_++;
  // Not adjusting positions as global_position is not set by ORS for these messages
  // AdjustGlobalPosition(global_position, last_exec_bid_price_, last_exec_ask_price_);
}

void PromOrderManager::OrderCancelRejected(const int _server_assigned_client_id_,
                                           const int _client_assigned_order_sequence_,
                                           const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                           const double _price_, const TradeType_t _buysell_,
                                           const int _size_remaining_, const int _rejection_reason_,
                                           const int _client_position_, const int _global_position_,
                                           const int _int_price_, const uint64_t exchange_order_id,
                                           const ttime_t time_set_by_server) {
  if (!enabled_) {
    return;
  }

#ifdef MAINTAIN_TOTAL_REJECTS
  if (!total_rejects_ && (watch_.msecs_from_midnight() - last_reject_encountered_time_ > REJECT_REFRESH_TIME)) {
    total_rejects_ = 0;
    last_order_rejection_reason_ = HFSAT::kORSOrderAllowed;
  }
#endif  // MAINTAIN_TOTAL_REJECTS

  if (manage_orders_also_) {
    if (_rejection_reason_ == HFSAT::kExchCancelReject)
      total_exch_cxl_rejects_++;
    else
      total_cxl_rejects_++;
    // If the client had set the int_price_ to 0 explicity but price_ is valid
    int t_int_price_ = (_int_price_ == 0) ? ((int)round(_price_ / min_price_increment_)) : _int_price_;
    switch (_buysell_) {
      case kTradeTypeBuy: {
        // find the order, if found remove that size from maps and dealloc order
        if (intpx_2_bid_order_vec_.find(t_int_price_) != intpx_2_bid_order_vec_.end()) {
          std::vector<BaseOrder*>& _this_base_order_vec_ = intpx_2_bid_order_vec_[t_int_price_];
          for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
               _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
            if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
                _server_assigned_order_sequence_) {  // found the order
              // BaseOrder * p_this_order_ = *_base_order_vec_iter_ ;
              // p_this_order_->canceled_ = false ; // ignoring since not used anywhere
              // TODO ... see if we need to rremove order ... then copy the part from OrderCanceled below
              break;
            }
          }
        }

      } break;
      case kTradeTypeSell: {
        // find the order, if found remove that size from maps and dealloc order
        if (intpx_2_ask_order_vec_.find(t_int_price_) != intpx_2_ask_order_vec_.end()) {
          std::vector<BaseOrder*>& _this_base_order_vec_ = intpx_2_ask_order_vec_[t_int_price_];
          for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
               _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
            if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
                _server_assigned_order_sequence_) {  // found the order
              // BaseOrder * p_this_order_ = *_base_order_vec_iter_ ;
              // p_this_order_->canceled_ = false ; // ignoring since not used anywhere
              // TODO ... see if we need to rremove order ... then copy the part from OrderCanceled below
              break;
            }
          }
        }

      } break;
      default: {}
    }

    num_ors_messages_++;

    if (last_exec_ask_intpx_ == kInvalidIntPrice || last_exec_bid_intpx_ == kInvalidIntPrice) {  // just initialized
      last_exec_bid_price_ = last_exec_ask_price_ = _price_;
      last_exec_bid_intpx_ = last_exec_ask_intpx_ = _int_price_;
    }

    AdjustGlobalPosition(_global_position_, last_exec_bid_price_, last_exec_ask_price_);
  } else {
    AdjustGlobalPosition(_global_position_, last_exec_bid_price_, last_exec_ask_price_);
  }
}

void PromOrderManager::OrderCanceled(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                     const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                     const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                                     const int _client_position_, const int _global_position_, const int _int_price_,
                                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                     const ttime_t time_set_by_server) {
  if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
    PrintOpenOrders();
    DBGLOG_TIME_CLASS_FUNC_LINE << " bbsz: " << best_bid_size() << " bbpx: " << best_bid_price()
                                << " bapx: " << best_ask_price() << " basz: " << best_ask_size() << DBGLOG_ENDL_FLUSH;
  }
  if (!enabled_) {
    return;
  }

#ifdef MAINTAIN_TOTAL_REJECTS
  if (!total_rejects_ && (watch_.msecs_from_midnight() - last_reject_encountered_time_ > REJECT_REFRESH_TIME)) {
    total_rejects_ = 0;
    last_order_rejection_reason_ = HFSAT::kORSOrderAllowed;
  }
#endif  // MAINTAIN_TOTAL_REJECTS

  if (manage_orders_also_) {
    // If the client had set the int_price_ to 0 explicity but price_ is valid
    int t_int_price_ = (_int_price_ == 0) ? ((int)round(_price_ / min_price_increment_)) : _int_price_;

    if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " PromOrderOrderCxledStart: "
                             << " SAOS: " << _server_assigned_order_sequence_
                             << " SID: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " PX: " << _price_
                             << " BS: " << GetTradeTypeChar(_buysell_) << " SZR: " << _size_remaining_
                             << " GPOS: " << _global_position_ << " CPOS: " << _client_position_
                             << " SACI: " << _server_assigned_client_id_ << DBGLOG_ENDL_FLUSH;
    }

    switch (_buysell_) {
      case kTradeTypeBuy: {
        // find the order, if found remove that size from maps and dealloc order
        if (intpx_2_bid_order_vec_.find(t_int_price_) != intpx_2_bid_order_vec_.end()) {
          std::vector<BaseOrder*>& _this_base_order_vec_ = intpx_2_bid_order_vec_[t_int_price_];
          for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
               _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
            if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
                _server_assigned_order_sequence_) {  // found the order
              BaseOrder* p_this_order_ = *_base_order_vec_iter_;
              switch (p_this_order_->order_status()) {  // depending on status remove from maps
                case kORRType_Seqd: {
                  num_unconfirmed_orders_--;
                  intpx_2_sum_bid_unconfirmed_[p_this_order_->int_price()] -= p_this_order_->size_remaining();
                } break;
                case kORRType_Conf: {
                  intpx_2_sum_bid_confirmed_[p_this_order_->int_price()] -= p_this_order_->size_remaining();
                  intpx_2_sum_bid_orders_confirmed_[t_int_price_]--;

                  // TODO_OPT : Is the following part needed only under remove_self_orders_from_book_ or
                  // use_smart_non_self_
                  // If so then we should not do it all the time !
                  // Update best bid info.
                  if (ors_best_bid_ask_.best_bid_int_price_ == 0 ||
                      ors_best_bid_ask_.best_bid_int_price_ <= t_int_price_) {
                    BestBidAskInfo t_ors_best_bid_ask_(ors_best_bid_ask_);

                    ors_best_bid_ask_.best_bid_int_price_ = t_int_price_;
                    ors_best_bid_ask_.best_bid_size_ = intpx_2_sum_bid_confirmed_[t_int_price_];
                    ors_best_bid_ask_.best_bid_orders_ = intpx_2_sum_bid_orders_confirmed_[t_int_price_];

                    if (ors_best_bid_ask_.best_bid_size_ == 0) {  // Cleared orders from previous best level.
                      RecomputeBestBids();

                      if (use_smart_non_self_) {
                        // Remove all "unrecognized" bids.
                        for (std::deque<BestBidAskInfo>::iterator _itr_ = mfm_ordered_bid_ask_updates_.begin();
                             _itr_ != mfm_ordered_bid_ask_updates_.end();) {
                          if (_itr_->best_bid_size_) {
                            mfm_ordered_bid_ask_updates_.erase(_itr_);
                            _itr_ = mfm_ordered_bid_ask_updates_.begin();
                            continue;
                          }

                          ++_itr_;
                        }

                        mkt_affirmed_bid_ask_.best_bid_int_price_ = ors_best_bid_ask_.best_bid_int_price_;
                        mkt_affirmed_bid_ask_.best_bid_size_ = ors_best_bid_ask_.best_bid_size_;
                        mkt_affirmed_bid_ask_.best_bid_orders_ = ors_best_bid_ask_.best_bid_orders_;
                      }
                    } else if (use_smart_non_self_) {
                      // Find the incremental update to expect in mkt. data.
                      BestBidAskInfo t_best_bid_info_ = BestBidAskInfo(
                          ors_best_bid_ask_.best_bid_int_price_, 0,
                          (ors_best_bid_ask_.best_bid_int_price_ == t_ors_best_bid_ask_.best_bid_int_price_)
                              ? (ors_best_bid_ask_.best_bid_size_ - t_ors_best_bid_ask_.best_bid_size_)
                              : ors_best_bid_ask_.best_bid_size_,
                          0, (ors_best_bid_ask_.best_bid_int_price_ == t_ors_best_bid_ask_.best_bid_int_price_)
                                 ? (ors_best_bid_ask_.best_bid_orders_ - t_ors_best_bid_ask_.best_bid_orders_)
                                 : ors_best_bid_ask_.best_bid_orders_,
                          0, watch_.msecs_from_midnight());

                      mfm_ordered_bid_ask_updates_.push_back(t_best_bid_info_);

                      if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
                        DBGLOG_TIME_CLASS_FUNC
                            << " Added to mfm_ordered_bid_ask_updates_ : " << t_best_bid_info_.ToString()
                            << DBGLOG_ENDL_FLUSH;
                      }

                      if (mfm_ordered_bid_ask_updates_.size() > 500) {  // 500+ unrecognized updates in 5 msecs !
                        if (dbglogger_.CheckLoggingLevel(SMVSELF_ERROR)) {
                          DBGLOG_TIME_CLASS_FUNC << "Warning : Clearing mfm_ordered_bid_ask_updates_."
                                                 << DBGLOG_ENDL_FLUSH;
                        }
                        mfm_ordered_bid_ask_updates_.clear();
                      }
                    }
                  }
                } break;
                default: {}
              }

              _this_base_order_vec_.erase(_base_order_vec_iter_);  // remove order from vec
              baseorder_mempool_.DeAlloc(p_this_order_);

              NotifyGlobalOrderChangeListeners(_buysell_, t_int_price_);

              break;
            }
          }
        }

      } break;
      case kTradeTypeSell: {
        // find the order, if found remove that size from maps and dealloc order
        if (intpx_2_ask_order_vec_.find(t_int_price_) != intpx_2_ask_order_vec_.end()) {
          std::vector<BaseOrder*>& _this_base_order_vec_ = intpx_2_ask_order_vec_[t_int_price_];
          for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
               _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
            if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
                _server_assigned_order_sequence_) {  // found the order
              BaseOrder* p_this_order_ = *_base_order_vec_iter_;
              switch (p_this_order_->order_status()) {  // depending on status remove from maps
                case kORRType_Seqd: {
                  num_unconfirmed_orders_--;
                  intpx_2_sum_ask_unconfirmed_[p_this_order_->int_price()] -= p_this_order_->size_remaining();
                } break;
                case kORRType_Conf: {
                  intpx_2_sum_ask_confirmed_[p_this_order_->int_price()] -= p_this_order_->size_remaining();
                  intpx_2_sum_ask_orders_confirmed_[t_int_price_]--;

                  // Update best ask info.
                  if (ors_best_bid_ask_.best_ask_int_price_ == 0 ||
                      ors_best_bid_ask_.best_ask_int_price_ >= t_int_price_) {
                    BestBidAskInfo t_ors_best_bid_ask_(ors_best_bid_ask_);

                    ors_best_bid_ask_.best_ask_int_price_ = t_int_price_;
                    ors_best_bid_ask_.best_ask_size_ = intpx_2_sum_ask_confirmed_[t_int_price_];
                    ors_best_bid_ask_.best_ask_orders_ = intpx_2_sum_ask_orders_confirmed_[t_int_price_];

                    if (ors_best_bid_ask_.best_ask_size_ == 0) {  // Cleared orders from previous best level.
                      RecomputeBestAsks();

                      if (use_smart_non_self_) {
                        // Remove all "unrecognized" asks.
                        for (std::deque<BestBidAskInfo>::iterator _itr_ = mfm_ordered_bid_ask_updates_.begin();
                             _itr_ != mfm_ordered_bid_ask_updates_.end();) {
                          if (_itr_->best_ask_size_) {
                            mfm_ordered_bid_ask_updates_.erase(_itr_);
                            _itr_ = mfm_ordered_bid_ask_updates_.begin();
                            continue;
                          }

                          ++_itr_;
                        }

                        mkt_affirmed_bid_ask_.best_ask_int_price_ = ors_best_bid_ask_.best_ask_int_price_;
                        mkt_affirmed_bid_ask_.best_ask_size_ = ors_best_bid_ask_.best_ask_size_;
                        mkt_affirmed_bid_ask_.best_ask_orders_ = ors_best_bid_ask_.best_ask_orders_;
                      }
                    } else if (use_smart_non_self_) {
                      BestBidAskInfo t_best_ask_info_ = BestBidAskInfo(
                          0, ors_best_bid_ask_.best_ask_int_price_, 0,
                          (ors_best_bid_ask_.best_ask_int_price_ == t_ors_best_bid_ask_.best_ask_int_price_)
                              ? (ors_best_bid_ask_.best_ask_size_ - t_ors_best_bid_ask_.best_ask_size_)
                              : ors_best_bid_ask_.best_ask_size_,
                          0, (ors_best_bid_ask_.best_ask_int_price_ == t_ors_best_bid_ask_.best_ask_int_price_)
                                 ? (ors_best_bid_ask_.best_ask_orders_ - t_ors_best_bid_ask_.best_ask_orders_)
                                 : ors_best_bid_ask_.best_ask_orders_,
                          watch_.msecs_from_midnight());

                      mfm_ordered_bid_ask_updates_.push_back(t_best_ask_info_);

                      if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
                        DBGLOG_TIME_CLASS_FUNC
                            << " Added to mfm_ordered_bid_ask_updates_ : " << t_best_ask_info_.ToString()
                            << DBGLOG_ENDL_FLUSH;
                      }

                      if (mfm_ordered_bid_ask_updates_.size() > 500) {  // 500+ unrecognized updates in 5 msecs !
                        if (dbglogger_.CheckLoggingLevel(SMVSELF_ERROR)) {
                          DBGLOG_TIME_CLASS_FUNC << "Warning : Clearing mfm_ordered_bid_ask_updates_."
                                                 << DBGLOG_ENDL_FLUSH;
                        }
                        mfm_ordered_bid_ask_updates_.clear();
                      }
                    }
                  }
                } break;
                default: {}
              }

              _this_base_order_vec_.erase(_base_order_vec_iter_);  // remove order from vec
              baseorder_mempool_.DeAlloc(p_this_order_);

              NotifyGlobalOrderChangeListeners(_buysell_, t_int_price_);

              break;
            }
          }
        }
      } break;
      default: {}
    }

    num_ors_messages_++;

    if (last_exec_ask_intpx_ == kInvalidIntPrice || last_exec_bid_intpx_ == kInvalidIntPrice) {  // just initialized
      last_exec_bid_price_ = last_exec_ask_price_ = _price_;
      last_exec_bid_intpx_ = last_exec_ask_intpx_ = _int_price_;
    }

    AdjustGlobalPosition(_global_position_, last_exec_bid_price_, last_exec_ask_price_);
  } else {
    AdjustGlobalPosition(_global_position_, last_exec_bid_price_, last_exec_ask_price_);
  }

  // if ( dbglogger_.CheckLoggingLevel ( PROM_OM_INFO ) )
  //   {
  // 	DBGLOG_TIME_CLASS_FUNC << " PromOrderOrderCxledEnd: "
  // 		   << " SAOS: " << _server_assigned_order_sequence_
  // 		   << " SID: " << sec_name_indexer_.GetSecurityNameFromId ( _security_id_ )
  // 		   << " PX: " << _price_
  // 		   << " BS: " << GetTradeTypeChar ( _buysell_ )
  // 		   << " SZR: " << _size_remaining_
  // 		   << " GPOS: " << _global_position_
  // 		   << " CPOS: " << _client_position_
  // 		   << " SACI: " << _server_assigned_client_id_
  // 		   << DBGLOG_ENDL_FLUSH ;
  //   }
}

void PromOrderManager::OrderExecuted(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                     const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                     const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                                     const int _size_executed_, const int _client_position_,
                                     const int _global_position_, const int _int_price_,
                                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                     const ttime_t time_set_by_server) {
  if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
    PrintOpenOrders();
    DBGLOG_TIME_CLASS_FUNC_LINE << " bbsz: " << best_bid_size() << " bbpx: " << best_bid_price()
                                << " bapx: " << best_ask_price() << " basz: " << best_ask_size() << DBGLOG_ENDL_FLUSH;
  }
  if (!enabled_) {
    return;
  }

#ifdef MAINTAIN_TOTAL_REJECTS
  if (!total_rejects_ && (watch_.msecs_from_midnight() - last_reject_encountered_time_ > REJECT_REFRESH_TIME)) {
    total_rejects_ = 0;
    last_order_rejection_reason_ = HFSAT::kORSOrderAllowed;
  }
#endif  // MAINTAIN_TOTAL_REJECTS

  if (manage_orders_also_) {
    // If the client had set the int_price_ to 0 explicity but price_ is valid
    int t_int_price_ = (_int_price_ == 0) ? ((int)round(_price_ / min_price_increment_)) : _int_price_;

    if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " PromOrderOrderExecutedStart: "
                             << " SAOS: " << _server_assigned_order_sequence_
                             << " SID: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " PX: " << _price_
                             << " BS: " << GetTradeTypeChar(_buysell_) << " SZR: " << _size_remaining_
                             << " SZE: " << _size_executed_ << " GPOS: " << _global_position_
                             << " CPOS: " << _client_position_ << " SACI: " << _server_assigned_client_id_
                             << DBGLOG_ENDL_FLUSH;
    }
    total_traded_value += _price_ * _size_executed_;

    switch (_buysell_) {
      case kTradeTypeBuy: {
        bool _found_this_order_ = false;

        // find the order, if found remove that size from maps and dealloc order
        if (intpx_2_bid_order_vec_.find(t_int_price_) != intpx_2_bid_order_vec_.end()) {
          std::vector<BaseOrder*>& _this_base_order_vec_ = intpx_2_bid_order_vec_[t_int_price_];
          for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
               _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
            if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
                _server_assigned_order_sequence_) {  // found the order
              _found_this_order_ = true;
              BaseOrder* p_this_order_ = *_base_order_vec_iter_;
              switch (p_this_order_->order_status()) {  // depending on status remove from maps
                case kORRType_Seqd: {
                  num_unconfirmed_orders_--;
                  intpx_2_sum_bid_unconfirmed_[p_this_order_->int_price()] -= p_this_order_->size_remaining();
                } break;
                case kORRType_Conf: {
                  intpx_2_sum_bid_confirmed_[p_this_order_->int_price()] -= p_this_order_->size_remaining();
                  intpx_2_sum_bid_orders_confirmed_[t_int_price_]--;

                  // Update best bid info.
                  if (ors_best_bid_ask_.best_bid_int_price_ == 0 ||
                      ors_best_bid_ask_.best_bid_int_price_ <= t_int_price_) {
                    BestBidAskInfo t_ors_best_bid_ask_(ors_best_bid_ask_);

                    ors_best_bid_ask_.best_bid_int_price_ = t_int_price_;
                    ors_best_bid_ask_.best_bid_size_ = intpx_2_sum_bid_confirmed_[t_int_price_];
                    ors_best_bid_ask_.best_bid_orders_ = intpx_2_sum_bid_orders_confirmed_[t_int_price_];

                    if (ors_best_bid_ask_.best_bid_size_ == 0) {  // Cleared orders from previous best level.
                      RecomputeBestBids();

                      if (use_smart_non_self_) {
                        // Remove all "unrecognized" bids.
                        for (std::deque<BestBidAskInfo>::iterator _itr_ = mfm_ordered_bid_ask_updates_.begin();
                             _itr_ != mfm_ordered_bid_ask_updates_.end();) {
                          if (_itr_->best_bid_size_) {
                            mfm_ordered_bid_ask_updates_.erase(_itr_);
                            _itr_ = mfm_ordered_bid_ask_updates_.begin();
                            continue;
                          }

                          ++_itr_;
                        }

                        mkt_affirmed_bid_ask_.best_bid_int_price_ = ors_best_bid_ask_.best_bid_int_price_;
                        mkt_affirmed_bid_ask_.best_bid_size_ = ors_best_bid_ask_.best_bid_size_;
                        mkt_affirmed_bid_ask_.best_bid_orders_ = ors_best_bid_ask_.best_bid_orders_;
                      }
                    } else if (use_smart_non_self_) {
                      // Find the incremental update to expect in mkt. data.
                      BestBidAskInfo t_best_bid_info_ = BestBidAskInfo(
                          ors_best_bid_ask_.best_bid_int_price_, 0,
                          (ors_best_bid_ask_.best_bid_int_price_ == t_ors_best_bid_ask_.best_bid_int_price_)
                              ? (ors_best_bid_ask_.best_bid_size_ - t_ors_best_bid_ask_.best_bid_size_)
                              : ors_best_bid_ask_.best_bid_size_,
                          0, (ors_best_bid_ask_.best_bid_int_price_ == t_ors_best_bid_ask_.best_bid_int_price_)
                                 ? (ors_best_bid_ask_.best_bid_orders_ - t_ors_best_bid_ask_.best_bid_orders_)
                                 : ors_best_bid_ask_.best_bid_orders_,
                          0, watch_.msecs_from_midnight());

                      mfm_ordered_bid_ask_updates_.push_back(t_best_bid_info_);

                      if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
                        DBGLOG_TIME_CLASS_FUNC
                            << " Added to mfm_ordered_bid_ask_updates_ : " << t_best_bid_info_.ToString()
                            << DBGLOG_ENDL_FLUSH;
                      }

                      if (mfm_ordered_bid_ask_updates_.size() > 500) {  // 500+ unrecognized updates in 5 msecs !
                        if (dbglogger_.CheckLoggingLevel(SMVSELF_ERROR)) {
                          DBGLOG_TIME_CLASS_FUNC << "Warning : Clearing mfm_ordered_bid_ask_updates_."
                                                 << DBGLOG_ENDL_FLUSH;
                        }
                        mfm_ordered_bid_ask_updates_.clear();
                      }
                    }
                  }
                } break;
                default: {}
              }

              _this_base_order_vec_.erase(_base_order_vec_iter_);  // remove order from vec
              baseorder_mempool_.DeAlloc(p_this_order_);

              if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
                DBGLOG_TIME_CLASS_FUNC << " SID: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_)
                                       << " BS: " << GetTradeTypeChar(_buysell_)
                                       << " SAOS: " << _server_assigned_order_sequence_ << " PX: " << _price_
                                       << " SZE: " << _size_executed_ << " GPOS: " << _global_position_
                                       << " CPOS: " << _client_position_ << DBGLOG_ENDL_FLUSH;
              }

              // Adjust global position at the end will compensate the diff
              //		      NotifyGlobalOrderExecListeners( _buysell_, _size_executed_, _price_ );
              NotifyGlobalOrderChangeListeners(_buysell_, t_int_price_);

              break;
            }
          }
        }

        // TODO_OPT can this ever happen due to order being in a map indexed by not int_price ?
        // I don't think so ... hence this part of the code is not needed
        if (!_found_this_order_) {  // not found at given price
          for (BidPriceOrderMapIter_t _intpx_2_bid_order_vec_iter_ = intpx_2_bid_order_vec_.begin();
               (!_found_this_order_) && (_intpx_2_bid_order_vec_iter_ != intpx_2_bid_order_vec_.end());
               _intpx_2_bid_order_vec_iter_++) {
            std::vector<BaseOrder*>& _this_base_order_vec_ = _intpx_2_bid_order_vec_iter_->second;
            for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
                 _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
              if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
                  _server_assigned_order_sequence_) {  // Found the order
                _found_this_order_ = true;
                BaseOrder* p_this_order_ = *_base_order_vec_iter_;
                switch (p_this_order_->order_status()) {  // depending on status remove from maps
                  case kORRType_Seqd: {
                    num_unconfirmed_orders_--;
                    intpx_2_sum_bid_unconfirmed_[p_this_order_->int_price()] -= p_this_order_->size_remaining();
                  } break;
                  case kORRType_Conf: {
                    intpx_2_sum_bid_confirmed_[p_this_order_->int_price()] -= p_this_order_->size_remaining();
                    intpx_2_sum_bid_orders_confirmed_[t_int_price_]--;

                    // Update best bid info.
                    if (ors_best_bid_ask_.best_bid_int_price_ == 0 ||
                        ors_best_bid_ask_.best_bid_int_price_ <= t_int_price_) {
                      BestBidAskInfo t_ors_best_bid_ask_(ors_best_bid_ask_);

                      ors_best_bid_ask_.best_bid_int_price_ = t_int_price_;
                      ors_best_bid_ask_.best_bid_size_ = intpx_2_sum_bid_confirmed_[t_int_price_];
                      ors_best_bid_ask_.best_bid_orders_ = intpx_2_sum_bid_orders_confirmed_[t_int_price_];

                      if (ors_best_bid_ask_.best_bid_size_ == 0) {  // Cleared orders from previous best level.
                        RecomputeBestBids();

                        if (use_smart_non_self_) {
                          // Remove all "unrecognized" bids.
                          for (std::deque<BestBidAskInfo>::iterator _itr_ = mfm_ordered_bid_ask_updates_.begin();
                               _itr_ != mfm_ordered_bid_ask_updates_.end();) {
                            if (_itr_->best_bid_size_) {
                              mfm_ordered_bid_ask_updates_.erase(_itr_);
                              _itr_ = mfm_ordered_bid_ask_updates_.begin();
                              continue;
                            }

                            ++_itr_;
                          }

                          mkt_affirmed_bid_ask_.best_bid_int_price_ = ors_best_bid_ask_.best_bid_int_price_;
                          mkt_affirmed_bid_ask_.best_bid_size_ = ors_best_bid_ask_.best_bid_size_;
                          mkt_affirmed_bid_ask_.best_bid_orders_ = ors_best_bid_ask_.best_bid_orders_;
                        }
                      } else if (use_smart_non_self_) {
                        // Find the incremental update to expect in mkt. data.
                        BestBidAskInfo t_best_bid_info_ = BestBidAskInfo(
                            ors_best_bid_ask_.best_bid_int_price_, 0,
                            (ors_best_bid_ask_.best_bid_int_price_ == t_ors_best_bid_ask_.best_bid_int_price_)
                                ? (ors_best_bid_ask_.best_bid_size_ - t_ors_best_bid_ask_.best_bid_size_)
                                : ors_best_bid_ask_.best_bid_size_,
                            0, (ors_best_bid_ask_.best_bid_int_price_ == t_ors_best_bid_ask_.best_bid_int_price_)
                                   ? (ors_best_bid_ask_.best_bid_orders_ - t_ors_best_bid_ask_.best_bid_orders_)
                                   : ors_best_bid_ask_.best_bid_orders_,
                            0, watch_.msecs_from_midnight());

                        mfm_ordered_bid_ask_updates_.push_back(t_best_bid_info_);

                        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
                          DBGLOG_TIME_CLASS_FUNC
                              << " Added to mfm_ordered_bid_ask_updates_ : " << t_best_bid_info_.ToString()
                              << DBGLOG_ENDL_FLUSH;
                        }

                        if (mfm_ordered_bid_ask_updates_.size() > 500) {  // 500+ unrecognized updates in 5 msecs !
                          if (dbglogger_.CheckLoggingLevel(SMVSELF_ERROR)) {
                            DBGLOG_TIME_CLASS_FUNC << "Warning : Clearing mfm_ordered_bid_ask_updates_."
                                                   << DBGLOG_ENDL_FLUSH;
                          }
                          mfm_ordered_bid_ask_updates_.clear();
                        }
                      }
                    }
                  } break;
                  default: {}
                }

                _this_base_order_vec_.erase(_base_order_vec_iter_);  // remove order from vec
                baseorder_mempool_.DeAlloc(p_this_order_);

                if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
                  DBGLOG_TIME_CLASS_FUNC << " SID: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_)
                                         << " BS: " << GetTradeTypeChar(_buysell_)
                                         << " SAOS: " << _server_assigned_order_sequence_ << " PX: " << _price_
                                         << " SZE: " << _size_executed_ << " GPOS: " << _global_position_
                                         << " CPOS: " << _client_position_ << DBGLOG_ENDL_FLUSH;
                }

                // Adjust global position at the end will compensate the diff
                //			  NotifyGlobalOrderExecListeners( _buysell_, _size_executed_, _price_ );
                NotifyGlobalOrderChangeListeners(_buysell_, t_int_price_);

                break;
              }
            }
          }
        }
      }
        last_exec_bid_intpx_ = t_int_price_;
        last_exec_bid_price_ = _price_;

        if (last_exec_ask_intpx_ == kInvalidIntPrice) {
          last_exec_ask_intpx_ = last_exec_bid_intpx_;
          last_exec_ask_price_ = last_exec_bid_price_;
        }
        break;
      case kTradeTypeSell: {
        bool _found_this_order_ = false;
        // find the order, if found remove that size from maps and dealloc order
        if (intpx_2_ask_order_vec_.find(t_int_price_) != intpx_2_ask_order_vec_.end()) {
          std::vector<BaseOrder*>& _this_base_order_vec_ = intpx_2_ask_order_vec_[t_int_price_];
          for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
               _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
            if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
                _server_assigned_order_sequence_) {  // found the order
              _found_this_order_ = true;
              BaseOrder* p_this_order_ = *_base_order_vec_iter_;
              switch (p_this_order_->order_status()) {  // depending on status remove from maps
                case kORRType_Seqd: {
                  num_unconfirmed_orders_--;
                  intpx_2_sum_ask_unconfirmed_[p_this_order_->int_price()] -= p_this_order_->size_remaining();
                } break;
                case kORRType_Conf: {
                  intpx_2_sum_ask_confirmed_[p_this_order_->int_price()] -= p_this_order_->size_remaining();
                  intpx_2_sum_ask_orders_confirmed_[t_int_price_]--;

                  // Update best ask info.
                  if (ors_best_bid_ask_.best_ask_int_price_ == 0 ||
                      ors_best_bid_ask_.best_ask_int_price_ >= t_int_price_) {
                    BestBidAskInfo t_ors_best_bid_ask_(ors_best_bid_ask_);

                    ors_best_bid_ask_.best_ask_int_price_ = t_int_price_;
                    ors_best_bid_ask_.best_ask_size_ = intpx_2_sum_ask_confirmed_[t_int_price_];
                    ors_best_bid_ask_.best_ask_orders_ = intpx_2_sum_ask_orders_confirmed_[t_int_price_];

                    if (ors_best_bid_ask_.best_ask_size_ == 0) {  // Cleared orders from previous best level.
                      RecomputeBestAsks();

                      if (use_smart_non_self_) {
                        // Remove all "unrecognized" asks.
                        for (std::deque<BestBidAskInfo>::iterator _itr_ = mfm_ordered_bid_ask_updates_.begin();
                             _itr_ != mfm_ordered_bid_ask_updates_.end();) {
                          if (_itr_->best_ask_size_) {
                            mfm_ordered_bid_ask_updates_.erase(_itr_);
                            _itr_ = mfm_ordered_bid_ask_updates_.begin();
                            continue;
                          }

                          ++_itr_;
                        }

                        mkt_affirmed_bid_ask_.best_ask_int_price_ = ors_best_bid_ask_.best_ask_int_price_;
                        mkt_affirmed_bid_ask_.best_ask_size_ = ors_best_bid_ask_.best_ask_size_;
                        mkt_affirmed_bid_ask_.best_ask_orders_ = ors_best_bid_ask_.best_ask_orders_;
                      }
                    } else if (use_smart_non_self_) {
                      BestBidAskInfo t_best_ask_info_ = BestBidAskInfo(
                          0, ors_best_bid_ask_.best_ask_int_price_, 0,
                          (ors_best_bid_ask_.best_ask_int_price_ == t_ors_best_bid_ask_.best_ask_int_price_)
                              ? (ors_best_bid_ask_.best_ask_size_ - t_ors_best_bid_ask_.best_ask_size_)
                              : ors_best_bid_ask_.best_ask_size_,
                          0, (ors_best_bid_ask_.best_ask_int_price_ == t_ors_best_bid_ask_.best_ask_int_price_)
                                 ? (ors_best_bid_ask_.best_ask_orders_ - t_ors_best_bid_ask_.best_ask_orders_)
                                 : ors_best_bid_ask_.best_ask_orders_,
                          watch_.msecs_from_midnight());

                      mfm_ordered_bid_ask_updates_.push_back(t_best_ask_info_);

                      if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
                        DBGLOG_TIME_CLASS_FUNC
                            << " Added to mfm_ordered_bid_ask_updates_ : " << t_best_ask_info_.ToString()
                            << DBGLOG_ENDL_FLUSH;
                      }

                      if (mfm_ordered_bid_ask_updates_.size() > 500) {  // 500+ unrecognized updates in 5 msecs !
                        if (dbglogger_.CheckLoggingLevel(SMVSELF_ERROR)) {
                          DBGLOG_TIME_CLASS_FUNC << "Warning : Clearing mfm_ordered_bid_ask_updates_."
                                                 << DBGLOG_ENDL_FLUSH;
                        }
                        mfm_ordered_bid_ask_updates_.clear();
                      }
                    }
                  }
                } break;
                default: {}
              }

              _this_base_order_vec_.erase(_base_order_vec_iter_);  // remove order from vec
              baseorder_mempool_.DeAlloc(p_this_order_);

              if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
                DBGLOG_TIME_CLASS_FUNC << " SID: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_)
                                       << " BS: " << GetTradeTypeChar(_buysell_)
                                       << " SAOS: " << _server_assigned_order_sequence_ << " PX: " << _price_
                                       << " SZE: " << _size_executed_ << " GPOS: " << _global_position_
                                       << " CPOS: " << _client_position_ << DBGLOG_ENDL_FLUSH;
              }

              // Adjust global position at the end will compensate the diff
              //		      NotifyGlobalOrderExecListeners( _buysell_, _size_executed_, _price_ );
              NotifyGlobalOrderChangeListeners(_buysell_, t_int_price_);

              break;
            }
          }
        }

        if (!_found_this_order_) {  // not found at given price
          for (AskPriceOrderMapIter_t _intpx_2_ask_order_vec_iter_ = intpx_2_ask_order_vec_.begin();
               (!_found_this_order_) && (_intpx_2_ask_order_vec_iter_ != intpx_2_ask_order_vec_.end());
               _intpx_2_ask_order_vec_iter_++) {
            std::vector<BaseOrder*>& _this_base_order_vec_ = _intpx_2_ask_order_vec_iter_->second;
            for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
                 _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
              if ((*_base_order_vec_iter_)->server_assigned_order_sequence() ==
                  _server_assigned_order_sequence_) {  // Found the order
                _found_this_order_ = true;
                BaseOrder* p_this_order_ = *_base_order_vec_iter_;
                switch (p_this_order_->order_status()) {  // depending on status remove from maps
                  case kORRType_Seqd: {
                    num_unconfirmed_orders_--;
                    intpx_2_sum_ask_unconfirmed_[p_this_order_->int_price()] -= p_this_order_->size_remaining();
                  } break;
                  case kORRType_Conf: {
                    intpx_2_sum_ask_confirmed_[p_this_order_->int_price()] -= p_this_order_->size_remaining();
                    intpx_2_sum_ask_orders_confirmed_[t_int_price_]--;

                    // Update best ask info.
                    if (ors_best_bid_ask_.best_ask_int_price_ == 0 ||
                        ors_best_bid_ask_.best_ask_int_price_ >= t_int_price_) {
                      BestBidAskInfo t_ors_best_bid_ask_(ors_best_bid_ask_);

                      ors_best_bid_ask_.best_ask_int_price_ = t_int_price_;
                      ors_best_bid_ask_.best_ask_size_ = intpx_2_sum_ask_confirmed_[t_int_price_];
                      ors_best_bid_ask_.best_ask_orders_ = intpx_2_sum_ask_orders_confirmed_[t_int_price_];

                      if (ors_best_bid_ask_.best_ask_size_ == 0) {  // Cleared orders from previous best level.
                        RecomputeBestAsks();

                        if (use_smart_non_self_) {
                          // Remove all "unrecognized" asks.
                          for (std::deque<BestBidAskInfo>::iterator _itr_ = mfm_ordered_bid_ask_updates_.begin();
                               _itr_ != mfm_ordered_bid_ask_updates_.end();) {
                            if (_itr_->best_ask_size_) {
                              mfm_ordered_bid_ask_updates_.erase(_itr_);
                              _itr_ = mfm_ordered_bid_ask_updates_.begin();
                              continue;
                            }

                            ++_itr_;
                          }

                          mkt_affirmed_bid_ask_.best_ask_int_price_ = ors_best_bid_ask_.best_ask_int_price_;
                          mkt_affirmed_bid_ask_.best_ask_size_ = ors_best_bid_ask_.best_ask_size_;
                          mkt_affirmed_bid_ask_.best_ask_orders_ = ors_best_bid_ask_.best_ask_orders_;
                        }
                      } else if (use_smart_non_self_) {
                        BestBidAskInfo t_best_ask_info_ = BestBidAskInfo(
                            0, ors_best_bid_ask_.best_ask_int_price_, 0,
                            (ors_best_bid_ask_.best_ask_int_price_ == t_ors_best_bid_ask_.best_ask_int_price_)
                                ? (ors_best_bid_ask_.best_ask_size_ - t_ors_best_bid_ask_.best_ask_size_)
                                : ors_best_bid_ask_.best_ask_size_,
                            0, (ors_best_bid_ask_.best_ask_int_price_ == t_ors_best_bid_ask_.best_ask_int_price_)
                                   ? (ors_best_bid_ask_.best_ask_orders_ - t_ors_best_bid_ask_.best_ask_orders_)
                                   : ors_best_bid_ask_.best_ask_orders_,
                            watch_.msecs_from_midnight());

                        mfm_ordered_bid_ask_updates_.push_back(t_best_ask_info_);

                        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
                          DBGLOG_TIME_CLASS_FUNC
                              << " Added to mfm_ordered_bid_ask_updates_ : " << t_best_ask_info_.ToString()
                              << DBGLOG_ENDL_FLUSH;
                        }

                        if (mfm_ordered_bid_ask_updates_.size() > 500) {  // 500+ unrecognized updates in 5 msecs !
                          if (dbglogger_.CheckLoggingLevel(SMVSELF_ERROR)) {
                            DBGLOG_TIME_CLASS_FUNC << "Warning : Clearing mfm_ordered_bid_ask_updates_."
                                                   << DBGLOG_ENDL_FLUSH;
                          }
                          mfm_ordered_bid_ask_updates_.clear();
                        }
                      }
                    }
                  } break;
                  default: {}
                }

                _this_base_order_vec_.erase(_base_order_vec_iter_);  // remove order from vec
                baseorder_mempool_.DeAlloc(p_this_order_);

                if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
                  DBGLOG_TIME_CLASS_FUNC << " SID: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_)
                                         << " BS: " << GetTradeTypeChar(_buysell_)
                                         << " SAOS: " << _server_assigned_order_sequence_ << " PX: " << _price_
                                         << " SZE: " << _size_executed_ << " GPOS: " << _global_position_
                                         << " CPOS: " << _client_position_ << DBGLOG_ENDL_FLUSH;
                }

                // Adjust global position at the end will compensate the diff
                // NotifyGlobalOrderExecListeners( _buysell_, _size_executed_, _price_ );
                NotifyGlobalOrderChangeListeners(_buysell_, t_int_price_);

                break;
              }
            }
          }
        }
      }
        last_exec_ask_intpx_ = t_int_price_;
        last_exec_ask_price_ = _price_;

        if (last_exec_bid_intpx_ == kInvalidIntPrice) {
          last_exec_bid_intpx_ = last_exec_ask_intpx_;
          last_exec_bid_price_ = last_exec_ask_price_;
        }
        break;
      default: {}
    }

    num_ors_messages_++;
    total_traded_++;
    total_traded_size_ += abs(_global_position_ - global_position_);

    AdjustGlobalPosition(_global_position_, last_exec_bid_price_, last_exec_ask_price_);
  } else {
    last_exec_bid_price_ = last_exec_ask_price_ = _price_;
    AdjustGlobalPosition(_global_position_, last_exec_bid_price_, last_exec_ask_price_);
  }

  if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << " PromOrderOrderExecutedEnd: "
                           << " SAOS: " << _server_assigned_order_sequence_
                           << " SID: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) << " PX: " << _price_
                           << " BS: " << GetTradeTypeChar(_buysell_) << " SZR: " << _size_remaining_
                           << " SZE: " << _size_executed_ << " GPOS: " << _global_position_
                           << " CPOS: " << _client_position_ << " SACI: " << _server_assigned_client_id_
                           << DBGLOG_ENDL_FLUSH;
  }
}

void PromOrderManager::PrintOpenOrders() {
  int total_bid_orders = 0;
  int total_ask_orders = 0;
  for (BidPriceOrderMapConstIter_t itr_ = intpx_2_bid_order_vec_.begin(); itr_ != intpx_2_bid_order_vec_.end();
       ++itr_) {
    for (auto order : itr_->second) {
      total_bid_orders += order->size_remaining();
    }
    dbglogger_ << " px: " << itr_->first << " sz: " << total_bid_orders << DBGLOG_ENDL_FLUSH;
    total_bid_orders = 0;
  }

  for (AskPriceOrderMapConstIter_t itr_ = intpx_2_ask_order_vec_.begin(); itr_ != intpx_2_ask_order_vec_.end();
       ++itr_) {
    for (auto order : itr_->second) {
      total_ask_orders += order->size_remaining();
    }
    dbglogger_ << " px: " << itr_->first << " sz: " << total_ask_orders << DBGLOG_ENDL_FLUSH;
    total_ask_orders = 0;
  }
}
// To send out the information in the margin server updates.
int PromOrderManager::getOpenOrders() {
  int total_open_orders_ = 0;

  for (BidPriceOrderMapConstIter_t itr_ = intpx_2_bid_order_vec_.begin(); itr_ != intpx_2_bid_order_vec_.end();
       ++itr_) {
    total_open_orders_ += (itr_->second).size();
  }

  for (AskPriceOrderMapConstIter_t itr_ = intpx_2_ask_order_vec_.begin(); itr_ != intpx_2_ask_order_vec_.end();
       ++itr_) {
    total_open_orders_ += (itr_->second).size();
  }

  total_open_orders_ -= num_unconfirmed_orders_;

  return total_open_orders_;
}

// To send out the information in the margin server updates.
int PromOrderManager::getOpenConfirmedOrders() {
  int total_open_orders_ = 0;

  for (AskPriceSizeMapConstIter_t itr_ = intpx_2_sum_ask_orders_confirmed_.begin();
       itr_ != intpx_2_sum_ask_confirmed_.end(); ++itr_) {
    total_open_orders_ += (itr_->second);
  }

  for (BidPriceSizeMapConstIter_t itr_ = intpx_2_sum_bid_orders_confirmed_.begin();
       itr_ != intpx_2_sum_bid_confirmed_.end(); ++itr_) {
    total_open_orders_ += (itr_->second);
  }

  return total_open_orders_;
}

void PromOrderManager::RecomputeBestBids() {
  ors_best_bid_ask_.best_bid_int_price_ = ors_best_bid_ask_.best_bid_size_ = ors_best_bid_ask_.best_bid_orders_ = 0;

  for (HFSAT::BidPriceSizeMapConstIter_t intpx_2_sum_bid_confirmed_iter = intpx_2_sum_bid_confirmed_.begin();
       intpx_2_sum_bid_confirmed_iter != intpx_2_sum_bid_confirmed_.end(); ++intpx_2_sum_bid_confirmed_iter) {
    if (intpx_2_sum_bid_confirmed_iter->second > 0) {
      ors_best_bid_ask_.best_bid_size_ = intpx_2_sum_bid_confirmed_iter->second;
      ors_best_bid_ask_.best_bid_int_price_ = intpx_2_sum_bid_confirmed_iter->first;
      break;
    }
  }
  for (HFSAT::BidPriceSizeMapConstIter_t intpx_2_sum_bid_orders_confirmed_iter =
           intpx_2_sum_bid_orders_confirmed_.begin();
       intpx_2_sum_bid_orders_confirmed_iter != intpx_2_sum_bid_orders_confirmed_.end();
       ++intpx_2_sum_bid_orders_confirmed_iter) {
    if (intpx_2_sum_bid_orders_confirmed_iter->second > 0) {
      ors_best_bid_ask_.best_bid_orders_ = intpx_2_sum_bid_orders_confirmed_iter->second;
      if (ors_best_bid_ask_.best_bid_int_price_ != intpx_2_sum_bid_orders_confirmed_iter->first) {
        if (dbglogger_.CheckLoggingLevel(SMVSELF_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC << " BestBidSizeMap price = " << ors_best_bid_ask_.best_bid_int_price_
                                 << " | BestBidOrdersSizeMap price = " << (intpx_2_sum_bid_orders_confirmed_iter->first)
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
      break;
    }
  }
}

void PromOrderManager::RecomputeBestAsks() {
  ors_best_bid_ask_.best_ask_int_price_ = ors_best_bid_ask_.best_ask_size_ = ors_best_bid_ask_.best_ask_orders_ = 0;

  for (HFSAT::AskPriceSizeMapConstIter_t intpx_2_sum_ask_confirmed_iter = intpx_2_sum_ask_confirmed_.begin();
       intpx_2_sum_ask_confirmed_iter != intpx_2_sum_ask_confirmed_.end(); ++intpx_2_sum_ask_confirmed_iter) {
    if (intpx_2_sum_ask_confirmed_iter->second > 0) {
      ors_best_bid_ask_.best_ask_size_ = intpx_2_sum_ask_confirmed_iter->second;
      ors_best_bid_ask_.best_ask_int_price_ = intpx_2_sum_ask_confirmed_iter->first;
      break;
    }
  }

  for (HFSAT::AskPriceSizeMapConstIter_t intpx_2_sum_ask_orders_confirmed_iter =
           intpx_2_sum_ask_orders_confirmed_.begin();
       intpx_2_sum_ask_orders_confirmed_iter != intpx_2_sum_ask_orders_confirmed_.end();
       ++intpx_2_sum_ask_orders_confirmed_iter) {
    if (intpx_2_sum_ask_orders_confirmed_iter->second > 0) {
      ors_best_bid_ask_.best_ask_orders_ = intpx_2_sum_ask_orders_confirmed_iter->second;
      if (ors_best_bid_ask_.best_ask_int_price_ != intpx_2_sum_ask_orders_confirmed_iter->first) {
        if (dbglogger_.CheckLoggingLevel(SMVSELF_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC << " BestAskSizeMap price = " << ors_best_bid_ask_.best_ask_int_price_
                                 << " | BestAskOrdersSizeMap price = " << (intpx_2_sum_ask_orders_confirmed_iter->first)
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
      break;
    }
  }
}

int PromOrderManager::GetBidSizePlacedAboveEqIntPx(int _bid_int_px_, int _max_int_px_) {
  // returns bidsize placed b/w _bid_int_px_ & int _max_int_px_
  int total_bid_size_ = 0;

  for (BidPriceSizeMapIter_t itr_ = intpx_2_sum_bid_confirmed_.begin(); itr_ != intpx_2_sum_bid_confirmed_.end();
       ++itr_) {
    if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
      DBGLOG_CLASS_FUNC_LINE << "price: " << itr_->first << " size: " << itr_->second << DBGLOG_ENDL_FLUSH;
    }
    if (itr_->first < _bid_int_px_) {
      return total_bid_size_;
    }

    if (itr_->first < _max_int_px_) {
      total_bid_size_ += itr_->second;
    }
  }

  return total_bid_size_;
}

int PromOrderManager::GetAskSizePlacedAboveEqIntPx(int _ask_int_px_, int _max_int_px_) {
  // returns asksize placed b/w _ask_int_px_ & int _max_int_px_
  int total_ask_size_ = 0;

  for (AskPriceSizeMapIter_t itr_ = intpx_2_sum_ask_confirmed_.begin(); itr_ != intpx_2_sum_ask_confirmed_.end();
       ++itr_) {
    if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
      DBGLOG_CLASS_FUNC_LINE << "price: " << itr_->first << " size: " << itr_->second << DBGLOG_ENDL_FLUSH;
    }

    if (itr_->first > _ask_int_px_) {
      return total_ask_size_;
    }

    if (itr_->first > _max_int_px_) {
      total_ask_size_ += itr_->second;
    }
  }
  return total_ask_size_;
}
}
