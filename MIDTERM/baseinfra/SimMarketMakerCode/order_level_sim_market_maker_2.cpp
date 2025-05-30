/**
   \file SimMarketMakerCode/order_level_sim_market_maker_2.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */

#include "baseinfra/SimMarketMaker/order_level_sim_market_maker_2.hpp"
#include "baseinfra/OrderRouting/market_model_manager.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"

#define POSTPONE_MSECS 100

namespace HFSAT {

OrderLevelSimMarketMaker2* OrderLevelSimMarketMaker2::GetUniqueInstance(
    DebugLogger& dbglogger, Watch& watch, SecurityMarketView& dep_market_view, int _market_model_index_,
    HFSAT::SimTimeSeriesInfo& t_sim_time_series_info_) {
  MarketModel _this_market_model_;
  MarketModelManager::GetMarketModel(dep_market_view.shortcode(), _market_model_index_, _this_market_model_,
                                     watch.YYYYMMDD());

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "OrderLevelSimMarketMaker2 " << dep_market_view.shortcode() << ' ' << _market_model_index_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OrderLevelSimMarketMaker2*> SMM_description_map_;
  if (SMM_description_map_.find(concise_indicator_description_) == SMM_description_map_.end()) {
    SMM_description_map_[concise_indicator_description_] =
        new OrderLevelSimMarketMaker2(dbglogger, watch, _this_market_model_, dep_market_view, t_sim_time_series_info_);
  }

  return SMM_description_map_[concise_indicator_description_];
}

OrderLevelSimMarketMaker2* OrderLevelSimMarketMaker2::GetInstance(DebugLogger& dbglogger, Watch& watch,
                                                                  SecurityMarketView& dep_market_view,
                                                                  int _market_model_index_,
                                                                  HFSAT::SimTimeSeriesInfo& t_sim_time_series_info_) {
  MarketModel _this_market_model_;
  MarketModelManager::GetMarketModel(dep_market_view.shortcode(), _market_model_index_, _this_market_model_,
                                     watch.YYYYMMDD());

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "OrderLevelSimMarketMaker2 " << dep_market_view.shortcode() << ' ' << _market_model_index_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  OrderLevelSimMarketMaker2* olsmm2_ =
      new OrderLevelSimMarketMaker2(dbglogger, watch, _this_market_model_, dep_market_view, t_sim_time_series_info_);

  return olsmm2_;
}

OrderLevelSimMarketMaker2::OrderLevelSimMarketMaker2(DebugLogger& dbglogger, Watch& watch, MarketModel market_model,
                                                     SecurityMarketView& dep_market_view,
                                                     HFSAT::SimTimeSeriesInfo& sim_time_series_info)
    : BaseSimMarketMaker(dbglogger, watch, dep_market_view, market_model, sim_time_series_info),
      mkt_order_mempool_(),
      buy_side_trade_level_(0),
      sell_side_trade_level_(0),
      ose_trade_time_manager_(
          OseTradeTimeManager::GetUniqueInstance(SecurityNameIndexer::GetUniqueInstance(), watch.YYYYMMDD())) {
  watch.subscribe_first_SmallTimePeriod(this);  ///< to get TimePeriod caled every msec
}

OrderLevelSimMarketMaker2::~OrderLevelSimMarketMaker2() {}

void OrderLevelSimMarketMaker2::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (!all_requests_.empty()) {
    ProcessRequestQueue();
  }
}

void OrderLevelSimMarketMaker2::SanitizeMarketOrderMaps() {
  // Sanitize bid map
  for (auto it_ = intpx_2_bid_mkt_order_vec_.begin(); it_ != intpx_2_bid_mkt_order_vec_.end(); it_++) {
    if (it_->second.size() == 0) {
      intpx_2_bid_mkt_order_vec_.erase(it_->first);
    }
  }

  // Sanitize ask map
  for (auto it_ = intpx_2_ask_mkt_order_vec_.begin(); it_ != intpx_2_ask_mkt_order_vec_.end(); it_++) {
    if (it_->second.size() == 0) {
      intpx_2_ask_mkt_order_vec_.erase(it_->first);
    }
  }

  for (auto it_ = intpx_bid_order_map_.begin(); it_ != intpx_bid_order_map_.end(); it_++) {
    if (it_->second.size() == 0) {
      intpx_bid_order_map_.erase(it_->first);
    }
  }

  for (auto it_ = intpx_ask_order_map_.begin(); it_ != intpx_ask_order_map_.end(); it_++) {
    if (it_->second.size() == 0) {
      intpx_ask_order_map_.erase(it_->first);
    }
  }
}

inline void OrderLevelSimMarketMaker2::LogBidSimOrder(int t_type_, int t_size_, int t_int_price_) {
  DBGLOG_TIME_CLASS_FUNC;

  int bid_int_price_ = intpx_2_bid_mkt_order_vec_.begin()->first;
  int bid_ord_size_ = intpx_2_bid_mkt_order_vec_.begin()->second.size();
  int ask_int_price_ = intpx_2_ask_mkt_order_vec_.begin()->first;
  int ask_ord_size_ = intpx_2_ask_mkt_order_vec_.begin()->second.size();

  dbglogger_ << smv_.secname();
  dbglogger_ << "[ " << bid_ord_size_ << " " << bid_int_price_ << " * " << ask_int_price_ << " " << ask_ord_size_
             << "]";
  dbglogger_ << "[ " << smv_.market_update_info_.bestbid_size_ << " " << smv_.market_update_info_.bestbid_ordercount_
             << " " << smv_.market_update_info_.bestbid_int_price_ << " * "
             << smv_.market_update_info_.bestask_int_price_ << " " << smv_.market_update_info_.bestask_ordercount_
             << " " << smv_.market_update_info_.bestask_size_ << "]";

  if (intpx_bid_order_map_.size() > 0) {
    auto it_ = intpx_bid_order_map_.begin();

    for (; it_ != intpx_bid_order_map_.end() && it_->second.size() == 0; it_++)
      ;

    if (it_ != intpx_bid_order_map_.end()) {
      int int_price_ = it_->first;

      std::vector<BaseSimOrder*>& sim_order_vec_ = it_->second;

      dbglogger_ << " orders at " << int_price_ << ".";

      for (size_t i = 0; i < sim_order_vec_.size(); i++) {
        int ahead_ordercount_ = 0;
        int behind_ordercount_ = 0;
        int ahead_size_ = 0;
        int behind_size_ = 0;

        if (intpx_2_bid_mkt_order_vec_.find(int_price_) != intpx_2_bid_mkt_order_vec_.end()) {
          std::vector<SimMarketOrder*>& market_order_vec_ = intpx_2_bid_mkt_order_vec_[int_price_];

          size_t j = 0;
          for (j = 0; j < market_order_vec_.size(); j++) {
            if (market_order_vec_[j]->order_id_ != sim_order_vec_[i]->order_id_) {
              ahead_ordercount_++;
              ahead_size_ += market_order_vec_[j]->size_;
            } else {
              break;
            }
          }

          j++;
          for (; j < market_order_vec_.size(); j++) {
            behind_ordercount_++;
            behind_size_ += market_order_vec_[j]->size_;
          }
        }

        dbglogger_ << "[ " << ahead_size_ << " " << ahead_ordercount_;
        dbglogger_ << " " << sim_order_vec_[i]->size_remaining_;
        dbglogger_ << " " << behind_ordercount_ << " " << behind_size_;
        dbglogger_ << "] ";
      }
    }
  }

  std::string type_ = " ";
  if (t_type_ == 1)
    type_ = "ADD   ";
  else if (t_type_ == 2)
    type_ = "REMOVE";
  else if (t_type_ == 3)
    type_ = "EXEC  ";
  else if (t_type_ == 4)
    type_ = "TRADE ";
  else if (t_type_ == 5)
    type_ = "CHANGE";
  dbglogger_ << " " << type_ << " BUY  " << t_size_ << " @ " << t_int_price_ << DBGLOG_ENDL_FLUSH;
}

inline void OrderLevelSimMarketMaker2::LogAskSimOrder(int t_type_, int t_size_, int t_int_price_) {
  DBGLOG_TIME_CLASS_FUNC;

  int bid_int_price_ = intpx_2_bid_mkt_order_vec_.begin()->first;
  int bid_ord_size_ = intpx_2_bid_mkt_order_vec_.begin()->second.size();
  int ask_int_price_ = intpx_2_ask_mkt_order_vec_.begin()->first;
  int ask_ord_size_ = intpx_2_ask_mkt_order_vec_.begin()->second.size();

  dbglogger_ << smv_.secname();
  dbglogger_ << "[ " << bid_ord_size_ << " " << bid_int_price_ << " * " << ask_int_price_ << " " << ask_ord_size_
             << "]";
  dbglogger_ << "[ " << smv_.market_update_info_.bestbid_size_ << " " << smv_.market_update_info_.bestbid_ordercount_
             << " " << smv_.market_update_info_.bestbid_int_price_ << " * "
             << smv_.market_update_info_.bestask_int_price_ << " " << smv_.market_update_info_.bestask_ordercount_
             << " " << smv_.market_update_info_.bestask_size_ << "]";

  if (intpx_ask_order_map_.size() > 0) {
    auto it_ = intpx_ask_order_map_.begin();

    for (; it_ != intpx_ask_order_map_.end() && it_->second.size() == 0; it_++)
      ;

    if (it_ != intpx_ask_order_map_.end()) {
      int int_price_ = it_->first;

      std::vector<BaseSimOrder*>& sim_order_vec_ = it_->second;

      dbglogger_ << " orders at " << int_price_ << ".";

      for (size_t i = 0; i < sim_order_vec_.size(); i++) {
        int ahead_ordercount_ = 0;
        int behind_ordercount_ = 0;
        int ahead_size_ = 0;
        int behind_size_ = 0;

        if (intpx_2_ask_mkt_order_vec_.find(int_price_) != intpx_2_ask_mkt_order_vec_.end()) {
          std::vector<SimMarketOrder*>& market_order_vec_ = intpx_2_ask_mkt_order_vec_[int_price_];

          size_t j = 0;
          for (j = 0; j < market_order_vec_.size(); j++) {
            if (market_order_vec_[j]->order_id_ != sim_order_vec_[i]->order_id_) {
              ahead_ordercount_++;
              ahead_size_ += market_order_vec_[j]->size_;
            } else {
              break;
            }
          }

          j++;
          for (; j < market_order_vec_.size(); j++) {
            behind_ordercount_++;
            behind_size_ += market_order_vec_[j]->size_;
          }
        }

        dbglogger_ << "[ " << ahead_size_ << " " << ahead_ordercount_;
        dbglogger_ << " " << sim_order_vec_[i]->size_remaining_;
        dbglogger_ << " " << behind_ordercount_ << " " << behind_size_;
        dbglogger_ << "] ";
      }
    }
  }

  std::string type_ = " ";
  if (t_type_ == 1)
    type_ = "ADD   ";
  else if (t_type_ == 2)
    type_ = "REMOVE";
  else if (t_type_ == 3)
    type_ = "EXEC  ";
  else if (t_type_ == 4)
    type_ = "TRADE ";
  else if (t_type_ == 5)
    type_ = "CHANGE";
  dbglogger_ << " " << type_ << " SELL " << t_size_ << " @ " << t_int_price_ << DBGLOG_ENDL_FLUSH;
}

/*
 * Objective: Compute the queue position of the given BaseOrder.
 * 1. We first find the BaseSimOrder corresponding to the given BaseOrder using saos
 * 2. Then, we need to find SimMarketOrder using the order_id of BaseSimOrder.
 * 3. We can compute qa/qb during this phase.
 */
bool OrderLevelSimMarketMaker2::UpdateBidQaQb(int t_saos_, BaseOrder* t_query_order_) {
  // Assuming the int price is okay.
  if (intpx_bid_order_map_.find(t_query_order_->int_price_) != intpx_bid_order_map_.end()) {
    std::vector<BaseSimOrder*>& sim_order_vec_ = intpx_bid_order_map_[t_query_order_->int_price_];
    BaseSimOrder* this_sim_order_ = NULL;

    for (size_t i = 0; i < sim_order_vec_.size(); i++) {
      if (sim_order_vec_[i]->server_assigned_order_sequence_ == t_saos_) {
        this_sim_order_ = sim_order_vec_[i];
        break;
      }
    }

    if (this_sim_order_ == NULL) {
      return false;
    }

    t_query_order_->queue_orders_ahead_ = 0;
    t_query_order_->queue_orders_behind_ = 0;
    t_query_order_->queue_size_ahead_ = 0;
    t_query_order_->queue_size_behind_ = 0;

    // If there is no market order present, qa/qb will be 0.
    // Otherwise, compute the qa/qb
    if (intpx_2_bid_mkt_order_vec_.find(t_query_order_->int_price_) != intpx_2_bid_mkt_order_vec_.end()) {
      std::vector<SimMarketOrder*>& market_order_vec_ = intpx_2_bid_mkt_order_vec_[t_query_order_->int_price_];
      size_t i = 0;
      for (i = 0; i < market_order_vec_.size(); i++) {
        if (market_order_vec_[i]->order_id_ != this_sim_order_->order_id_) {
          t_query_order_->queue_orders_ahead_++;
          t_query_order_->queue_size_ahead_ += market_order_vec_[i]->size_;
        } else {
          break;
        }
      }

      for (; i < market_order_vec_.size(); i++) {
        t_query_order_->queue_orders_behind_++;
        t_query_order_->queue_size_behind_ += market_order_vec_[i]->size_;
      }
    }

    return true;
  }

  return false;
}

/*
 * Objective: Compute the queue position of the given BaseOrder.
 * 1. We first find the BaseSimOrder corresponding to the given BaseOrder using saos
 * 2. Then, we need to find SimMarketOrder using the order_id of BaseSimOrder.
 * 3. We can compute qa/qb during this phase.
 */
bool OrderLevelSimMarketMaker2::UpdateAskQaQb(int t_saos_, BaseOrder* t_query_order_) {
  // Assuming the int price is okay.
  if (intpx_ask_order_map_.find(t_query_order_->int_price_) != intpx_ask_order_map_.end()) {
    std::vector<BaseSimOrder*>& sim_order_vec_ = intpx_ask_order_map_[t_query_order_->int_price_];
    BaseSimOrder* this_sim_order_ = NULL;

    for (size_t i = 0; i < sim_order_vec_.size(); i++) {
      if (sim_order_vec_[i]->server_assigned_order_sequence_ == t_saos_) {
        this_sim_order_ = sim_order_vec_[i];
        break;
      }
    }

    if (this_sim_order_ == NULL) {
      return false;
    }

    t_query_order_->queue_orders_ahead_ = 0;
    t_query_order_->queue_orders_behind_ = 0;
    t_query_order_->queue_size_ahead_ = 0;
    t_query_order_->queue_size_behind_ = 0;

    // If there is no market order present, qa/qb will be 0.
    // Otherwise, compute the qa/qb
    if (intpx_2_ask_mkt_order_vec_.find(t_query_order_->int_price_) != intpx_2_ask_mkt_order_vec_.end()) {
      std::vector<SimMarketOrder*>& market_order_vec_ = intpx_2_ask_mkt_order_vec_[t_query_order_->int_price_];
      size_t i = 0;
      for (i = 0; i < market_order_vec_.size(); i++) {
        if (market_order_vec_[i]->order_id_ != this_sim_order_->order_id_) {
          t_query_order_->queue_orders_ahead_++;
          t_query_order_->queue_size_ahead_ += market_order_vec_[i]->size_;
        } else {
          break;
        }
      }

      for (; i < market_order_vec_.size(); i++) {
        t_query_order_->queue_orders_behind_++;
        t_query_order_->queue_size_behind_ += market_order_vec_[i]->size_;
      }
    }

    return true;
  }

  return false;
}

bool OrderLevelSimMarketMaker2::UpdateQaQb(int t_saos_, BaseOrder* t_query_order_) {
  switch (t_query_order_->buysell_) {
    case kTradeTypeBuy: {
      return UpdateBidQaQb(t_saos_, t_query_order_);
    } break;

    case kTradeTypeSell: {
      return UpdateAskQaQb(t_saos_, t_query_order_);
    } break;

    default: { return false; } break;
  }

  return false;
}

std::pair<int, int> OrderLevelSimMarketMaker2::GetBidQaQb(int t_int_price_) {
  std::pair<int, int> qa_qb_(-1, -1);

  auto it_ = intpx_bid_order_map_.find(t_int_price_);

  if (it_ != intpx_bid_order_map_.end() && !it_->second.empty()) {
    std::vector<BaseSimOrder*>& sim_order_vec_ = it_->second;

    auto it_mkt_order_ = intpx_2_bid_mkt_order_vec_.find(t_int_price_);

    if (it_mkt_order_ == intpx_2_bid_mkt_order_vec_.end()) {
      qa_qb_.first = 0;
      qa_qb_.second = sim_order_vec_.empty() ? 0 : sim_order_vec_[0]->size_remaining_;

      if (qa_qb_.second <= 0) {
        qa_qb_.first = -1;
        qa_qb_.second = -1;
      }
    } else {
      std::vector<SimMarketOrder*>& market_order_vec_ = it_mkt_order_->second;

      int ahead_size_ = 0;
      int behind_size_ = 0;
      size_t i = 0;
      for (i = 0; i < market_order_vec_.size(); i++) {
        if (market_order_vec_[i]->order_id_ != sim_order_vec_[0]->order_id_) {
          ahead_size_ += market_order_vec_[i]->size_;
        } else {
          break;
        }
      }

      i++;
      for (; i < market_order_vec_.size(); i++) {
        behind_size_ += market_order_vec_[i]->size_;
      }

      qa_qb_.first = ahead_size_;
      qa_qb_.second = behind_size_;
    }
  }

  return qa_qb_;
}

std::pair<int, int> OrderLevelSimMarketMaker2::GetAskQaQb(int t_int_price_) {
  std::pair<int, int> qa_qb_(-1, -1);

  auto it_ = intpx_ask_order_map_.find(t_int_price_);

  if (it_ != intpx_ask_order_map_.end() && !it_->second.empty()) {
    std::vector<BaseSimOrder*>& sim_order_vec_ = it_->second;

    auto it_mkt_order_ = intpx_2_ask_mkt_order_vec_.find(t_int_price_);

    if (it_mkt_order_ == intpx_2_ask_mkt_order_vec_.end()) {
      qa_qb_.first = 0;
      qa_qb_.second = sim_order_vec_.empty() ? 0 : sim_order_vec_[0]->size_remaining_;

      if (qa_qb_.second <= 0) {
        qa_qb_.first = -1;
        qa_qb_.second = -1;
      }
    } else {
      std::vector<SimMarketOrder*>& market_order_vec_ = it_mkt_order_->second;

      int ahead_size_ = 0;
      int behind_size_ = 0;
      size_t i = 0;
      for (i = 0; i < market_order_vec_.size(); i++) {
        if (market_order_vec_[i]->order_id_ != sim_order_vec_[0]->order_id_) {
          ahead_size_ += market_order_vec_[i]->size_;
        } else {
          break;
        }
      }

      i++;
      for (; i < market_order_vec_.size(); i++) {
        behind_size_ += market_order_vec_[i]->size_;
      }

      qa_qb_.first = ahead_size_;
      qa_qb_.second = behind_size_;
    }
  }

  return qa_qb_;
}

void OrderLevelSimMarketMaker2::AddMarketBid(int int_price, SimMarketOrder* market_order) {
  if (intpx_2_bid_mkt_order_vec_.find(int_price) == intpx_2_bid_mkt_order_vec_.end()) {
    intpx_2_bid_mkt_order_vec_[int_price] = std::vector<SimMarketOrder*>();
  }

  intpx_2_bid_mkt_order_vec_[int_price].push_back(market_order);
}

void OrderLevelSimMarketMaker2::AddMarketAsk(int int_price, SimMarketOrder* market_order) {
  if (intpx_2_ask_mkt_order_vec_.find(int_price) == intpx_2_ask_mkt_order_vec_.end()) {
    intpx_2_ask_mkt_order_vec_[int_price] = std::vector<SimMarketOrder*>();
  }

  intpx_2_ask_mkt_order_vec_[int_price].push_back(market_order);
}

void OrderLevelSimMarketMaker2::ExecuteBidsAboveIntPrice(int int_price) {
  while (!intpx_bid_order_map_.empty() && intpx_bid_order_map_.begin()->first > int_price) {
    auto it_ = intpx_bid_order_map_.begin();
    std::vector<BaseSimOrder*>& sim_order_vec = it_->second;

    for (size_t i = 0; i < sim_order_vec.size(); i++) {
      int this_size_executed_ = sim_order_vec[i]->ExecuteRemaining();

      client_position_map_[sim_order_vec[i]->server_assigned_client_id_] += this_size_executed_;
      global_position_to_send_map_[sim_order_vec[i]->server_assigned_client_id_] += this_size_executed_;
      BroadcastExecNotification(sim_order_vec[i]->server_assigned_client_id_, sim_order_vec[i]);

      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "SIM order executed: " << smv_.secname() << " BUY " << this_size_executed_ << " @ "
                               << intpx_bid_order_map_.begin()->first
                               << " CAOS: " << sim_order_vec[i]->client_assigned_order_sequence_
                               << " SAOS: " << sim_order_vec[i]->server_assigned_order_sequence_
                               << " SACI: " << sim_order_vec[i]->server_assigned_client_id_ << DBGLOG_ENDL_FLUSH;
      }

      basesimorder_mempool_.DeAlloc(sim_order_vec[i]);
    }

    it_->second.clear();

    intpx_bid_order_map_.erase(it_);
  }
}

void OrderLevelSimMarketMaker2::ExecuteAsksAboveIntPrice(int int_price) {
  while (!intpx_ask_order_map_.empty() && intpx_ask_order_map_.begin()->first < int_price) {
    auto it_ = intpx_ask_order_map_.begin();
    std::vector<BaseSimOrder*>& sim_order_vec = it_->second;

    for (size_t i = 0; i < sim_order_vec.size(); i++) {
      int this_size_executed_ = sim_order_vec[i]->ExecuteRemaining();

      client_position_map_[sim_order_vec[i]->server_assigned_client_id_] -= this_size_executed_;
      global_position_to_send_map_[sim_order_vec[i]->server_assigned_client_id_] -= this_size_executed_;
      BroadcastExecNotification(sim_order_vec[i]->server_assigned_client_id_, sim_order_vec[i]);

      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "SIM order executed: " << smv_.secname() << " SELL " << this_size_executed_ << " @ "
                               << intpx_ask_order_map_.begin()->first
                               << " CAOS: " << sim_order_vec[i]->client_assigned_order_sequence_
                               << " SAOS: " << sim_order_vec[i]->server_assigned_order_sequence_
                               << " SACI: " << sim_order_vec[i]->server_assigned_client_id_ << DBGLOG_ENDL_FLUSH;
      }

      basesimorder_mempool_.DeAlloc(sim_order_vec[i]);
    }

    it_->second.clear();

    intpx_ask_order_map_.erase(it_);
  }
}

void OrderLevelSimMarketMaker2::ExecuteBidsEqAboveIntPrice(int int_price) { ExecuteBidsAboveIntPrice(int_price - 1); }

void OrderLevelSimMarketMaker2::ExecuteAsksEqAboveIntPrice(int int_price) { ExecuteAsksAboveIntPrice(int_price + 1); }

inline void OrderLevelSimMarketMaker2::ExecuteBidsAtIntPrice(int t_int_price_, int t_size_) {
  if (intpx_bid_order_map_.find(t_int_price_) == intpx_bid_order_map_.end()) {
    return;
  }

  std::vector<BaseSimOrder*>& sim_order_vec_ = intpx_bid_order_map_[t_int_price_];

  for (size_t i = 0; i < sim_order_vec_.size(); i++) {
    if (saci_to_executed_size_[sim_order_vec_[i]->server_assigned_client_id_] < t_size_) {
      int size_to_be_executed_ = t_size_ - saci_to_executed_size_[sim_order_vec_[i]->server_assigned_client_id_];
      int this_size_executed_ = 0;

      if (size_to_be_executed_ >= sim_order_vec_[i]->size_remaining()) {
        this_size_executed_ = sim_order_vec_[i]->ExecuteRemaining();
      } else {
        this_size_executed_ = sim_order_vec_[i]->MatchPartial(size_to_be_executed_);
      }

      client_position_map_[sim_order_vec_[i]->server_assigned_client_id_] += this_size_executed_;
      global_position_to_send_map_[sim_order_vec_[i]->server_assigned_client_id_] += this_size_executed_;
      saci_to_executed_size_[sim_order_vec_[i]->server_assigned_client_id_] += this_size_executed_;
      BroadcastExecNotification(sim_order_vec_[i]->server_assigned_client_id_, sim_order_vec_[i]);

      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "SIM order executed: " << smv_.secname() << " BUY " << this_size_executed_ << " @ "
                               << t_int_price_ << " CAOS: " << sim_order_vec_[i]->client_assigned_order_sequence_
                               << " SAOS: " << sim_order_vec_[i]->server_assigned_order_sequence_
                               << " SACI: " << sim_order_vec_[i]->server_assigned_client_id_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  for (size_t i = 0; i < sim_order_vec_.size(); i++) {
    if (sim_order_vec_[i]->size_remaining_ == 0) {
      basesimorder_mempool_.DeAlloc(sim_order_vec_[i]);
      VectorUtils::UniqueVectorRemove(sim_order_vec_, sim_order_vec_[i]);
    }
  }

  for (size_t i = 0; i < saci_to_executed_size_.size(); i++) {
    saci_to_executed_size_[i] = 0;
  }
}

inline void OrderLevelSimMarketMaker2::ExecuteAsksAtIntPrice(int t_int_price_, int t_size_) {
  if (intpx_ask_order_map_.find(t_int_price_) == intpx_ask_order_map_.end()) {
    return;
  }

  std::vector<BaseSimOrder*>& sim_order_vec_ = intpx_ask_order_map_[t_int_price_];

  for (size_t i = 0; i < sim_order_vec_.size(); i++) {
    if (saci_to_executed_size_[sim_order_vec_[i]->server_assigned_client_id_] < t_size_) {
      int size_to_be_executed_ = t_size_ - saci_to_executed_size_[sim_order_vec_[i]->server_assigned_client_id_];
      int this_size_executed_ = 0;

      if (size_to_be_executed_ >= sim_order_vec_[i]->size_remaining()) {
        this_size_executed_ = sim_order_vec_[i]->ExecuteRemaining();
      } else {
        this_size_executed_ = sim_order_vec_[i]->MatchPartial(size_to_be_executed_);
      }

      client_position_map_[sim_order_vec_[i]->server_assigned_client_id_] -= this_size_executed_;
      global_position_to_send_map_[sim_order_vec_[i]->server_assigned_client_id_] -= this_size_executed_;
      saci_to_executed_size_[sim_order_vec_[i]->server_assigned_client_id_] += this_size_executed_;
      BroadcastExecNotification(sim_order_vec_[i]->server_assigned_client_id_, sim_order_vec_[i]);

      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "SIM order executed: " << smv_.secname() << " SELL " << this_size_executed_ << " @ "
                               << t_int_price_ << " CAOS: " << sim_order_vec_[i]->client_assigned_order_sequence_
                               << " SAOS: " << sim_order_vec_[i]->server_assigned_order_sequence_
                               << " SACI: " << sim_order_vec_[i]->server_assigned_client_id_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  for (size_t i = 0; i < sim_order_vec_.size(); i++) {
    if (sim_order_vec_[i]->size_remaining_ == 0) {
      basesimorder_mempool_.DeAlloc(sim_order_vec_[i]);
      VectorUtils::UniqueVectorRemove(sim_order_vec_, sim_order_vec_[i]);
    }
  }

  for (size_t i = 0; i < saci_to_executed_size_.size(); i++) {
    saci_to_executed_size_[i] = 0;
  }
}

SimMarketOrder* OrderLevelSimMarketMaker2::GetNewMarketOrder(int t_int_price_, int t_size_, double t_price_,
                                                             int64_t t_order_id_) {
  SimMarketOrder* p_new_market_order_ = mkt_order_mempool_.Alloc();
  p_new_market_order_->int_price_ = t_int_price_;
  p_new_market_order_->size_ = t_size_;
  p_new_market_order_->price_ = t_price_;
  p_new_market_order_->order_id_ = t_order_id_;
  p_new_market_order_->mkt_time_ = watch_.tv();

  return p_new_market_order_;
}

// Tag all the untagged sim bids present at the given price level with the given order id
void OrderLevelSimMarketMaker2::TagSimBids(int t_int_price_, int64_t t_order_id_) {
  auto it_ = intpx_2_bid_mkt_order_vec_.find(t_int_price_);
  SimMarketOrder* last_mkt_order_at_price_ = NULL;
  int64_t last_order_id_ = 0;
  ttime_t last_order_time_(0, 0);

  if (it_ != intpx_2_bid_mkt_order_vec_.end()) {
    std::vector<SimMarketOrder*>& market_order_vec_ = it_->second;

    if (market_order_vec_.size() > 0) {
      last_mkt_order_at_price_ = market_order_vec_[market_order_vec_.size() - 1];
      last_order_id_ = last_mkt_order_at_price_->order_id_;
      last_order_time_ = last_mkt_order_at_price_->mkt_time_;
    }
  }

  if (intpx_bid_order_map_.find(t_int_price_) != intpx_bid_order_map_.end()) {
    std::vector<BaseSimOrder*>& sim_order_vec_ = intpx_bid_order_map_[t_int_price_];

    for (auto i = 0u; i < sim_order_vec_.size(); i++) {
      if (sim_order_vec_[i]->order_id_ == 0) {
        if (last_mkt_order_at_price_ != NULL &&
            (sim_order_vec_[i]->order_confirmation_time_ - last_order_time_) <
                (watch_.tv() - sim_order_vec_[i]->order_confirmation_time_)) {
          sim_order_vec_[i]->order_id_ = last_order_id_;
        } else {
          sim_order_vec_[i]->order_id_ = t_order_id_;
        }
      }
    }
  }
}

// Tag all the untagged sim asks present at the given price level with the given order id
void OrderLevelSimMarketMaker2::TagSimAsks(int int_price, int64_t order_id) {
  auto it_ = intpx_2_ask_mkt_order_vec_.find(int_price);
  SimMarketOrder* last_mkt_order_at_price_ = NULL;
  int64_t last_order_id_ = 0;
  ttime_t last_order_time_(0, 0);

  if (it_ != intpx_2_ask_mkt_order_vec_.end()) {
    std::vector<SimMarketOrder*>& market_order_vec_ = it_->second;

    if (market_order_vec_.size() > 0) {
      last_mkt_order_at_price_ = market_order_vec_[market_order_vec_.size() - 1];
      last_order_id_ = last_mkt_order_at_price_->order_id_;
      last_order_time_ = last_mkt_order_at_price_->mkt_time_;
    }
  }

  if (intpx_ask_order_map_.find(int_price) != intpx_ask_order_map_.end()) {
    std::vector<BaseSimOrder*>& sim_order_vec_ = intpx_ask_order_map_[int_price];

    for (auto i = 0u; i < sim_order_vec_.size(); i++) {
      if (sim_order_vec_[i]->order_id_ == 0) {
        if (last_mkt_order_at_price_ != NULL &&
            sim_order_vec_[i]->order_confirmation_time_ - last_order_time_ <
                watch_.tv() - sim_order_vec_[i]->order_confirmation_time_) {
          sim_order_vec_[i]->order_id_ = last_order_id_;
        } else {
          sim_order_vec_[i]->order_id_ = order_id;
        }
      }
    }
  }
}

void OrderLevelSimMarketMaker2::ExecuteSimBid(int t_int_price_, int64_t t_order_id_, int t_size_executed_,
                                              bool t_assign_next_order_) {
  std::vector<BaseSimOrder*>& sim_order_vec_ = intpx_bid_order_map_[t_int_price_];

  int64_t next_order_id_ = 0;
  if (t_assign_next_order_) {
    next_order_id_ = GetNextMarketBid(t_int_price_, t_order_id_);
  }

  for (size_t i = 0; i < sim_order_vec_.size(); i++) {
    // Execute a sim order, if
    // 1. tagged market order gets executed, or
    // 2. the sim order isn't tagged and there only one market order. (This situation arises when a market order arrives
    //    and then a sim order is placed and there isn't any more addition of orders at that level before
    //    this lone market order at this level gets executed. In this case, the sim order will remain untagged.)
    // Also, for each client_id_, bound the total execution size with t_size_executed_

    bool last_sim_order_ = false;

    if (sim_order_vec_[i]->order_id_ == 0) {
      if (intpx_2_bid_mkt_order_vec_.find(t_int_price_) != intpx_2_bid_mkt_order_vec_.end()) {
        std::vector<SimMarketOrder*>& market_order_vec_ = intpx_2_bid_mkt_order_vec_[t_int_price_];

        size_t mkt_order_sz_ = market_order_vec_.size();

        if (mkt_order_sz_ > 0 && market_order_vec_[mkt_order_sz_ - 1]->order_id_ == t_order_id_) {
          last_sim_order_ = true;
        }
      }
    }

    if ((sim_order_vec_[i]->order_id_ == t_order_id_ || last_sim_order_) &&
        saci_to_executed_size_[sim_order_vec_[i]->server_assigned_client_id_] < t_size_executed_) {
      int size_to_be_executed_ =
          t_size_executed_ - saci_to_executed_size_[sim_order_vec_[i]->server_assigned_client_id_];
      int this_size_executed_ = 0;

      if (size_to_be_executed_ >= sim_order_vec_[i]->size_remaining_) {
        // Full EXEC
        this_size_executed_ = sim_order_vec_[i]->ExecuteRemaining();
      } else {
        // Partial EXEC
        this_size_executed_ = sim_order_vec_[i]->MatchPartial(size_to_be_executed_);

        // This market order is completely executed, assign the next order id here.
        if (t_assign_next_order_) {
          sim_order_vec_[i]->order_id_ = next_order_id_;
        }
      }

      client_position_map_[sim_order_vec_[i]->server_assigned_client_id_] += this_size_executed_;
      global_position_to_send_map_[sim_order_vec_[i]->server_assigned_client_id_] += this_size_executed_;
      saci_to_executed_size_[sim_order_vec_[i]->server_assigned_client_id_] += this_size_executed_;
      BroadcastExecNotification(sim_order_vec_[i]->server_assigned_client_id_, sim_order_vec_[i]);

      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "SIM order executed: " << smv_.secname() << " BUY " << this_size_executed_ << " @ "
                               << t_int_price_ << " CAOS: " << sim_order_vec_[i]->client_assigned_order_sequence_
                               << " SAOS: " << sim_order_vec_[i]->server_assigned_order_sequence_
                               << " SACI: " << sim_order_vec_[i]->server_assigned_client_id_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  for (size_t i = 0; i < saci_to_executed_size_.size(); i++) {
    saci_to_executed_size_[i] = 0;
  }

  for (size_t i = 0; i < sim_order_vec_.size(); i++) {
    if (sim_order_vec_[i]->size_remaining_ == 0) {
      basesimorder_mempool_.DeAlloc(sim_order_vec_[i]);
      VectorUtils::UniqueVectorRemove(sim_order_vec_, sim_order_vec_[i]);
    }
  }
}

void OrderLevelSimMarketMaker2::ExecuteSimAsk(int t_int_price_, int64_t t_order_id_, int t_size_executed_,
                                              bool t_assign_next_order_) {
  std::vector<BaseSimOrder*>& sim_order_vec_ = intpx_ask_order_map_[t_int_price_];

  int64_t next_order_id_ = 0;
  if (t_assign_next_order_) {
    next_order_id_ = GetNextMarketAsk(t_int_price_, t_order_id_);
  }

  for (size_t i = 0; i < sim_order_vec_.size(); i++) {
    // Execute a sim order, if
    // 1. tagged market order gets executed, or
    // 2. the sim order isn't tagged and there only one market order. (This situation arises when a market order arrives
    //    and then a sim order is placed and there isn't any more addition of orders at that level before
    //    this lone market order at this level gets executed. In this case, the sim order will remain untagged.)
    // Also, for each client_id_, bound the total execution size with t_size_executed_

    bool last_sim_order_ = false;

    if (sim_order_vec_[i]->order_id_ == 0) {
      if (intpx_2_ask_mkt_order_vec_.find(t_int_price_) != intpx_2_ask_mkt_order_vec_.end()) {
        std::vector<SimMarketOrder*>& market_order_vec_ = intpx_2_ask_mkt_order_vec_[t_int_price_];

        size_t mkt_order_sz_ = market_order_vec_.size();

        if (mkt_order_sz_ > 0 && market_order_vec_[mkt_order_sz_ - 1]->order_id_ == t_order_id_) {
          last_sim_order_ = true;
        }
      }
    }

    if ((sim_order_vec_[i]->order_id_ == t_order_id_ || last_sim_order_) &&
        saci_to_executed_size_[sim_order_vec_[i]->server_assigned_client_id_] < t_size_executed_) {
      int size_to_be_executed_ =
          t_size_executed_ - saci_to_executed_size_[sim_order_vec_[i]->server_assigned_client_id_];
      int this_size_executed_ = 0;

      if (size_to_be_executed_ >= sim_order_vec_[i]->size_remaining_) {
        // Full EXEC
        this_size_executed_ = sim_order_vec_[i]->ExecuteRemaining();
      } else {
        // Partial EXEC
        this_size_executed_ = sim_order_vec_[i]->MatchPartial(size_to_be_executed_);

        // This market order is completely executed, assign the next order id here.
        if (t_assign_next_order_) {
          sim_order_vec_[i]->order_id_ = next_order_id_;
        }
      }

      client_position_map_[sim_order_vec_[i]->server_assigned_client_id_] -= this_size_executed_;
      global_position_to_send_map_[sim_order_vec_[i]->server_assigned_client_id_] -= this_size_executed_;
      saci_to_executed_size_[sim_order_vec_[i]->server_assigned_client_id_] += this_size_executed_;
      BroadcastExecNotification(sim_order_vec_[i]->server_assigned_client_id_, sim_order_vec_[i]);

      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "SIM order executed: " << smv_.secname() << " SELL " << this_size_executed_ << " @ "
                               << t_int_price_ << " CAOS: " << sim_order_vec_[i]->client_assigned_order_sequence_
                               << " SAOS: " << sim_order_vec_[i]->server_assigned_order_sequence_
                               << " SACI: " << sim_order_vec_[i]->server_assigned_client_id_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  for (size_t i = 0; i < saci_to_executed_size_.size(); i++) {
    saci_to_executed_size_[i] = 0;
  }

  for (size_t i = 0; i < sim_order_vec_.size(); i++) {
    if (sim_order_vec_[i]->size_remaining_ == 0) {
      basesimorder_mempool_.DeAlloc(sim_order_vec_[i]);
      VectorUtils::UniqueVectorRemove(sim_order_vec_, sim_order_vec_[i]);
    }
  }
}

inline int64_t OrderLevelSimMarketMaker2::GetNextMarketBid(int int_price, int64_t order_id) {
  auto& market_order_vec = intpx_2_bid_mkt_order_vec_[int_price];
  int64_t next_order_id = 0;

  for (size_t i = 0; i < market_order_vec.size(); i++) {
    if (market_order_vec[i]->order_id_ == order_id) {
      if ((i + 1) < market_order_vec.size()) {
        next_order_id = market_order_vec[i + 1]->order_id_;
      }
    }
  }

  return next_order_id;
}

inline int64_t OrderLevelSimMarketMaker2::GetNextMarketAsk(int t_int_price_, int64_t t_order_id_) {
  std::vector<SimMarketOrder*>& market_order_vec_ = intpx_2_ask_mkt_order_vec_[t_int_price_];
  int64_t next_order_id_ = 0;

  for (size_t i = 0; i < market_order_vec_.size(); i++) {
    if (market_order_vec_[i]->order_id_ == t_order_id_) {
      if ((i + 1) < market_order_vec_.size()) {
        next_order_id_ = market_order_vec_[i + 1]->order_id_;
      }
    }
  }

  return next_order_id_;
}

inline void OrderLevelSimMarketMaker2::DeleteMarketBid(int t_int_price_, int64_t t_order_id_) {
  std::vector<SimMarketOrder*>& market_order_vec_ = intpx_2_bid_mkt_order_vec_[t_int_price_];
  int64_t next_order_id_ = 0;

  for (size_t i = 0; i < market_order_vec_.size(); i++) {
    if (market_order_vec_[i]->order_id_ == t_order_id_) {
      mkt_order_mempool_.DeAlloc(market_order_vec_[i]);
      VectorUtils::UniqueVectorRemove(market_order_vec_, market_order_vec_[i]);

      if ((i + 1) < market_order_vec_.size()) {
        next_order_id_ = market_order_vec_[i + 1]->order_id_;
      }
    }
  }

  if (market_order_vec_.size() == 0) {
    intpx_2_bid_mkt_order_vec_.erase(t_int_price_);
  }

  // If there exists sim order with order_id_ same as this order, assign its order_id_ to next_order_id_.
  if (intpx_bid_order_map_.find(t_int_price_) != intpx_bid_order_map_.end()) {
    std::vector<BaseSimOrder*>& sim_order_vec_ = intpx_bid_order_map_[t_int_price_];

    for (size_t i = 0; i < sim_order_vec_.size(); i++) {
      if (sim_order_vec_[i]->order_id_ == t_order_id_) {
        sim_order_vec_[i]->order_id_ = next_order_id_;
      }
    }
  }
}

inline void OrderLevelSimMarketMaker2::DeleteMarketAsk(int t_int_price_, int64_t t_order_id_) {
  std::vector<SimMarketOrder*>& market_order_vec_ = intpx_2_ask_mkt_order_vec_[t_int_price_];
  int64_t next_order_id_ = 0;

  for (size_t i = 0; i < market_order_vec_.size(); i++) {
    if (market_order_vec_[i]->order_id_ == t_order_id_) {
      mkt_order_mempool_.DeAlloc(market_order_vec_[i]);
      VectorUtils::UniqueVectorRemove(market_order_vec_, market_order_vec_[i]);

      if ((i + 1) < market_order_vec_.size()) {
        next_order_id_ = market_order_vec_[i + 1]->order_id_;
      }
    }
  }

  if (market_order_vec_.size() == 0) {
    intpx_2_ask_mkt_order_vec_.erase(t_int_price_);
  }

  // 3. If there exists sim order with order_id_ same as this order, assign its order_id_ to next_order_id_.
  if (intpx_ask_order_map_.find(t_int_price_) != intpx_ask_order_map_.end()) {
    std::vector<BaseSimOrder*>& sim_order_vec_ = intpx_ask_order_map_[t_int_price_];

    for (size_t i = 0; i < sim_order_vec_.size(); i++) {
      if (sim_order_vec_[i]->order_id_ == t_order_id_) {
        sim_order_vec_[i]->order_id_ = next_order_id_;
      }
    }
  }
}

void OrderLevelSimMarketMaker2::ChangeMarketBidSize(int t_int_price_, int64_t t_order_id_, int t_size_change_) {
  std::vector<SimMarketOrder*>& market_order_vec_ = intpx_2_bid_mkt_order_vec_[t_int_price_];
  std::vector<BaseSimOrder*>& sim_order_vec_ = intpx_bid_order_map_[t_int_price_];

  size_t i = 0;
  for (i = 0; i < market_order_vec_.size(); i++) {
    if (market_order_vec_[i]->order_id_ == t_order_id_) {
      break;
    }
  }

  if (i > 0) {
    // Error
    if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " ERROR: non-first order in bid order_change. i: " << i
                             << " order_id: " << (unsigned long long)t_order_id_ << " int_price: " << t_int_price_
                             << DBGLOG_ENDL_FLUSH;
    }
  }

  if (i == market_order_vec_.size()) {
    if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " ERROR: could not find order in bid order_change. order_id: "
                             << (unsigned long long)t_order_id_ << " int_price: " << t_int_price_ << DBGLOG_ENDL_FLUSH;
    }

    return;
  }

  int64_t next_order_id_ = 0;

  if (t_size_change_ + market_order_vec_[i]->size_ == 0) {
    // Full execution
    if ((i + 1) < market_order_vec_.size()) {
      next_order_id_ = market_order_vec_[i + 1]->order_id_;
    }

    for (size_t sim_order_index_ = 0; sim_order_index_ < sim_order_vec_.size(); sim_order_index_++) {
      if (sim_order_vec_[sim_order_index_]->order_id_ == t_order_id_) {
        sim_order_vec_[sim_order_index_]->order_id_ = next_order_id_;
      }
    }

    mkt_order_mempool_.DeAlloc(market_order_vec_[i]);
    VectorUtils::UniqueVectorRemove(market_order_vec_, market_order_vec_[i]);
  } else {
    if (t_size_change_ + market_order_vec_[i]->size_ < 0) {
      // Error
      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " ERROR: order with size_changed > size in bid order_change. order_id: "
                               << (unsigned long long)t_order_id_ << " int_price: " << t_int_price_
                               << DBGLOG_ENDL_FLUSH;
      }
    }

    // Partial execution
    market_order_vec_[i]->size_ += t_size_change_;
  }

  if (market_order_vec_.size() == 0) {
    intpx_2_bid_mkt_order_vec_.erase(t_int_price_);
  }
}

void OrderLevelSimMarketMaker2::ChangeMarketAskSize(int t_int_price_, int64_t t_order_id_, int t_size_change_) {
  std::vector<SimMarketOrder*>& market_order_vec_ = intpx_2_ask_mkt_order_vec_[t_int_price_];
  std::vector<BaseSimOrder*>& sim_order_vec_ = intpx_ask_order_map_[t_int_price_];

  size_t i = 0;
  for (i = 0; i < market_order_vec_.size(); i++) {
    if (market_order_vec_[i]->order_id_ == t_order_id_) {
      break;
    }
  }

  if (i > 0) {
    // Error
    if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " ERROR: non-first order in ask order_change. i: " << i
                             << " order_id: " << (unsigned long long)t_order_id_ << " int_price: " << t_int_price_
                             << DBGLOG_ENDL_FLUSH;
    }
  }

  if (i == market_order_vec_.size()) {
    if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " ERROR: could not find order in ask order_change. order_id: "
                             << (unsigned long long)t_order_id_ << " int_price: " << t_int_price_ << DBGLOG_ENDL_FLUSH;
    }

    return;
  }

  int64_t next_order_id_ = 0;

  if (t_size_change_ + market_order_vec_[i]->size_ == 0) {
    // Full execution
    if ((i + 1) < market_order_vec_.size()) {
      next_order_id_ = market_order_vec_[i + 1]->order_id_;
    }

    for (size_t sim_order_index_ = 0; sim_order_index_ < sim_order_vec_.size(); sim_order_index_++) {
      if (sim_order_vec_[sim_order_index_]->order_id_ == t_order_id_) {
        sim_order_vec_[sim_order_index_]->order_id_ = next_order_id_;
      }
    }

    mkt_order_mempool_.DeAlloc(market_order_vec_[i]);
    VectorUtils::UniqueVectorRemove(market_order_vec_, market_order_vec_[i]);
  } else {
    if (t_size_change_ + market_order_vec_[i]->size_ < 0) {
      // Error
      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " ERROR: order with size_changed > size in ask order_change. order_id: "
                               << (unsigned long long)t_order_id_ << " int_price: " << t_int_price_
                               << DBGLOG_ENDL_FLUSH;
      }
    }

    // Partial execution
    market_order_vec_[i]->size_ += t_size_change_;
  }

  if (market_order_vec_.size() == 0) {
    intpx_2_ask_mkt_order_vec_.erase(t_int_price_);
  }
}

void OrderLevelSimMarketMaker2::ResetBook(const unsigned int t_security_id_) {
  intpx_2_bid_mkt_order_vec_.clear();
  intpx_2_ask_mkt_order_vec_.clear();

  // TODO: de-alloc all the orders
}

void OrderLevelSimMarketMaker2::UpdateNumEvents() {
  auto it_bid_ = intpx_bid_order_map_.begin();

  while (it_bid_ != intpx_bid_order_map_.end() && it_bid_->first > intpx_2_bid_mkt_order_vec_.begin()->first) {
    std::vector<BaseSimOrder*>& sim_order_vec_ = intpx_bid_order_map_[it_bid_->first];

    for (size_t i = 0; i < sim_order_vec_.size(); i++) {
      if (sim_order_vec_[i] != NULL) {
        sim_order_vec_[i]->num_events_seen_++;
      }
    }

    it_bid_++;
  }

  auto it_ask_ = intpx_ask_order_map_.begin();

  while (it_ask_ != intpx_ask_order_map_.end() && it_ask_->first > intpx_2_ask_mkt_order_vec_.begin()->first) {
    std::vector<BaseSimOrder*>& sim_order_vec_ = intpx_ask_order_map_[it_ask_->first];

    for (size_t i = 0; i < sim_order_vec_.size(); i++) {
      if (sim_order_vec_[i] != NULL) {
        sim_order_vec_[i]->num_events_seen_++;
      }
    }

    it_ask_++;
  }
}

void OrderLevelSimMarketMaker2::OnOrderAddEobi(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                               const int t_level_, const int64_t t_order_id_, const double t_price_,
                                               const int t_size_) {
  int int_price_ = smv_.GetIntPx(t_price_);

  SimMarketOrder* p_new_market_order_ = GetNewMarketOrder(int_price_, t_size_, t_price_, t_order_id_);

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      // Error/Sanitization check
      if (int_price_ >= intpx_2_ask_mkt_order_vec_.begin()->first) {
        // Sanitize
        for (auto it_ = intpx_2_ask_mkt_order_vec_.begin();
             it_ != intpx_2_ask_mkt_order_vec_.end() && int_price_ >= it_->first; it_++) {
          if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << " ask side sanitization at int_price: " << it_->first << DBGLOG_ENDL_FLUSH;
          }

          intpx_2_ask_mkt_order_vec_.erase(it_);
        }
      }

      // Tag the sim orders (orders sent by the query) with the order id of this order, if possible
      TagSimBids(int_price_, t_order_id_);

      // Add the new order to bid market_order map.
      AddMarketBid(int_price_, p_new_market_order_);

      // Check if any of our sim orders (orders sent by the query) match with this order. If yes, execute the matching
      // sim orders.
      ExecuteAsksEqAboveIntPrice(int_price_);

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogBidSimOrder(1, t_size_, int_price_);
      }
    } break;

    case kTradeTypeSell: {
      // Error/Sanitization check
      if (int_price_ <= intpx_2_bid_mkt_order_vec_.begin()->first) {
        // Sanitize
        for (auto it_ = intpx_2_bid_mkt_order_vec_.begin();
             it_ != intpx_2_bid_mkt_order_vec_.end() && int_price_ <= it_->first; it_++) {
          if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << " bid side sanitization at int_price: " << it_->first << DBGLOG_ENDL_FLUSH;
          }

          intpx_2_bid_mkt_order_vec_.erase(it_);
        }
      }

      // Tag the sim orders (orders sent by the query) with the order id of this order, if possible
      TagSimAsks(int_price_, t_order_id_);

      // Add the new order to bid market_order map.
      AddMarketAsk(int_price_, p_new_market_order_);

      // Check if any of our sim orders (orders sent by the query) match with this order. If yes, execute the matching
      // sim orders.
      ExecuteBidsEqAboveIntPrice(int_price_);

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogAskSimOrder(1, t_size_, int_price_);
      }
    } break;

    default:
      break;
  }
}

void OrderLevelSimMarketMaker2::OnOrderModifyEobi(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                  const int t_level_, const int64_t t_old_order_id_,
                                                  const double t_old_price_, const int t_old_size_,
                                                  const int64_t t_new_order_id_, const double t_new_price_,
                                                  const int t_new_size_) {
  // If the order id remains same, find the order and change the size
  if (t_old_order_id_ == t_new_order_id_) {
    int int_price_ = smv_.GetIntPx(t_old_price_);

    switch (t_buysell_) {
      case kTradeTypeBuy: {
        if (intpx_2_bid_mkt_order_vec_.find(int_price_) == intpx_2_bid_mkt_order_vec_.end()) {
          if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << " ERROR: bid order not found at int_price: " << int_price_ << " order_id_ "
                                   << (unsigned long long)t_old_order_id_ << DBGLOG_ENDL_FLUSH;
          }

          return;
        }

        std::vector<SimMarketOrder*>& market_order_vec_ = intpx_2_bid_mkt_order_vec_[int_price_];

        for (size_t i = 0; i < market_order_vec_.size(); i++) {
          if (market_order_vec_[i]->order_id_ == t_old_order_id_) {
            if (market_order_vec_[i]->size_ != t_old_size_) {
              if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
                DBGLOG_TIME_CLASS_FUNC << " ERROR: bid order old size does not match. old_size: " << t_old_size_
                                       << " new_size: " << t_new_size_ << DBGLOG_ENDL_FLUSH;
              }
            }

            market_order_vec_[i]->size_ = t_new_size_;
            return;
          }
        }
      } break;

      case kTradeTypeSell: {
        if (intpx_2_ask_mkt_order_vec_.find(int_price_) == intpx_2_ask_mkt_order_vec_.end()) {
          if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << " ERROR: ask order not found at int_price: " << int_price_ << " order_id_ "
                                   << (unsigned long long)t_old_order_id_ << DBGLOG_ENDL_FLUSH;
          }
        }

        std::vector<SimMarketOrder*>& market_order_vec_ = intpx_2_ask_mkt_order_vec_[int_price_];

        for (size_t i = 0; i < market_order_vec_.size(); i++) {
          if (market_order_vec_[i]->order_id_ == t_old_order_id_) {
            if (market_order_vec_[i]->size_ != t_old_size_) {
              if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
                DBGLOG_TIME_CLASS_FUNC << " ERROR: ask order old size does not match. old_size: " << t_old_size_
                                       << " new_size: " << t_new_size_ << DBGLOG_ENDL_FLUSH;
              }
            }

            market_order_vec_[i]->size_ = t_new_size_;
            return;
          }
        }
      } break;

      default:
        break;
    }

    if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " ERROR: did not find the old order in order modify. "
                             << (t_buysell_ == kTradeTypeBuy ? "BUY" : "SELL")
                             << " old_order_id: " << (unsigned long long)t_old_order_id_
                             << " new_order_id: " << (unsigned long long)t_new_order_id_ << " int_price: " << int_price_
                             << DBGLOG_ENDL_FLUSH;
    }
  }

  else {
    OnOrderDelete(t_security_id_, t_buysell_, t_level_, t_old_order_id_, t_old_price_, t_old_size_);
    OnOrderAdd(t_security_id_, t_buysell_, t_level_, t_new_order_id_, t_new_price_, t_new_size_);
  }
}

void OrderLevelSimMarketMaker2::OnOrderDeleteEobi(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                  const int t_level_, const int64_t t_order_id_, const double t_price_,
                                                  const int t_size_) {
  int int_price_ = smv_.GetIntPx(t_price_);

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (intpx_2_bid_mkt_order_vec_.find(int_price_) == intpx_2_bid_mkt_order_vec_.end()) {
        // Error
        if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " int_price: " << int_price_
                                 << " mkt_order_top: " << intpx_2_bid_mkt_order_vec_.begin()->first
                                 << " sim_order_top: " << intpx_bid_order_map_.begin()->first << " "
                                 << (t_buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << " " << smv_.secname()
                                 << DBGLOG_ENDL_FLUSH;
        }

        return;
      }

      // Find and remove the order using order_id_. Also extract the next order_id_ in that queue
      DeleteMarketBid(int_price_, t_order_id_);

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogBidSimOrder(2, t_size_, int_price_);
      }
    } break;

    case kTradeTypeSell: {
      if (intpx_2_ask_mkt_order_vec_.find(int_price_) == intpx_2_ask_mkt_order_vec_.end()) {
        // Error
        if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " int_price: " << int_price_
                                 << " mkt_order_top: " << intpx_2_ask_mkt_order_vec_.begin()->first
                                 << " sim_order_top: " << intpx_ask_order_map_.begin()->first << " "
                                 << (t_buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << " " << smv_.secname()
                                 << DBGLOG_ENDL_FLUSH;
        }

        return;
      }

      // Find and remove the order using order_id_. Also extract the next order_id_ in that queue
      DeleteMarketAsk(int_price_, t_order_id_);

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogAskSimOrder(2, t_size_, int_price_);
      }
    } break;

    default:
      break;
  }
}
void OrderLevelSimMarketMaker2::OnOrderExecEobi(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                const int t_level_, const int64_t t_order_id_, const double t_price_,
                                                const int t_size_executed_, const int t_size_remaining_) {
  int int_price_ = smv_.GetIntPx(t_price_);

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (intpx_2_bid_mkt_order_vec_.find(int_price_) == intpx_2_bid_mkt_order_vec_.end()) {
        // Error
        if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "ERROR: could not find the order in order_exec. order_id: "
                                 << (unsigned long long)t_order_id_ << " int_price: " << int_price_ << " "
                                 << (t_buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << " " << smv_.secname()
                                 << DBGLOG_ENDL_FLUSH;
        }

        return;
      }

      // 1. Execute sim orders more aggressive than the current order
      ExecuteBidsAboveIntPrice(int_price_);

      // 2. Executed sim orders whose order_id_ match with this order_id_
      ExecuteSimBid(int_price_, t_order_id_, t_size_executed_);

      // 3. Change the order size.
      ChangeMarketBidSize(int_price_, t_order_id_, -t_size_executed_);

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogBidSimOrder(3, t_size_executed_, int_price_);
      }
    } break;

    case kTradeTypeSell: {
      if (intpx_2_ask_mkt_order_vec_.find(int_price_) == intpx_2_ask_mkt_order_vec_.end()) {
        // Error
        if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "ERROR: could not find the order in order_exec. order_id: "
                                 << (unsigned long long)t_order_id_ << " int_price: " << int_price_ << " "
                                 << (t_buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << " " << smv_.secname()
                                 << DBGLOG_ENDL_FLUSH;
        }

        return;
      }

      // 1. Execute sim orders more aggressive than the current order
      ExecuteAsksAboveIntPrice(int_price_);

      // 2. Executed sim orders whose order_id_ match with this order_id_
      ExecuteSimAsk(int_price_, t_order_id_, t_size_executed_);

      // 3. Change the order size.
      ChangeMarketAskSize(int_price_, t_order_id_, -t_size_executed_);

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogAskSimOrder(3, t_size_executed_, int_price_);
      }
    } break;

    default:
      break;
  }
}
void OrderLevelSimMarketMaker2::OnTradeEobi(const unsigned int t_security_id_, const double t_trade_price_,
                                            const int t_trade_size_, const TradeType_t t_buysell_) {}

void OrderLevelSimMarketMaker2::OnOrderAddOse(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                              const int t_level_, const int64_t t_order_id_, const double price,
                                              const int t_size_) {
  int int_price_ = 0;

  if (price == -2147483648) {
    // In OSE, price_ == -2147483648 corresponds to something like market order in pre-open sessions.
    // In such cases, assign a very high number as the int_price
    if (t_buysell_ == kTradeTypeBuy) {
      int_price_ = 1000000;
    } else {
      int_price_ = 0;
    }
  } else {
    int_price_ = smv_.GetIntPx(price);
  }

  SimMarketOrder* new_mkt_order = GetNewMarketOrder(int_price_, t_size_, price, t_order_id_);

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      // Error/Sanitization check, if this is a valid time for the ose security
      if (ose_trade_time_manager_.isValidTimeToTrade(smv_.security_id(), watch_.tv().tv_sec % 86400) &&
          int_price_ >= intpx_2_ask_mkt_order_vec_.begin()->first) {
        // Sanitize
        for (auto it_ = intpx_2_ask_mkt_order_vec_.begin();
             it_ != intpx_2_ask_mkt_order_vec_.end() && int_price_ >= it_->first; it_++) {
          if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << " ask side sanitization at int_price: " << it_->first << DBGLOG_ENDL_FLUSH;
          }

          intpx_2_ask_mkt_order_vec_.erase(it_);
        }
      }

      // Tag the sim orders (orders sent by the query) with the order id of this order, if possible
      TagSimBids(int_price_, t_order_id_);

      // Add the new order to bid market_order map.
      AddMarketBid(int_price_, new_mkt_order);

      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        int level_ = 0;

        for (auto it_ = intpx_2_bid_mkt_order_vec_.begin(); it_ != intpx_2_bid_mkt_order_vec_.end(); it_++) {
          std::vector<SimMarketOrder*> market_order_vec_ = it_->second;

          bool break_from_loop_ = false;
          for (size_t i = 0; i < market_order_vec_.size(); i++) {
            level_++;
            if (market_order_vec_[i]->order_id_ == t_order_id_) {
              break_from_loop_ = true;
              break;
            }
          }

          if (break_from_loop_) {
            break;
          }
        }

        if (level_ != t_level_) {
          if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << " bid level does not match: " << int_price_ << " " << t_level_ << " " << level_
                                   << DBGLOG_ENDL_FLUSH;
          }
        }
      }

      // Check if any of our sim orders (orders sent by the query) match with this order. If yes, execute the matching
      // sim orders.
      ExecuteAsksEqAboveIntPrice(int_price_);

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogBidSimOrder(1, t_size_, int_price_);
      }
      break;
    }

    case kTradeTypeSell: {
      // Error/Sanitization check, if this is a valid time for the ose security
      if (ose_trade_time_manager_.isValidTimeToTrade(smv_.security_id(), watch_.tv().tv_sec % 86400) &&
          int_price_ <= intpx_2_bid_mkt_order_vec_.begin()->first) {
        // Sanitize
        for (auto it_ = intpx_2_bid_mkt_order_vec_.begin();
             it_ != intpx_2_bid_mkt_order_vec_.end() && int_price_ <= it_->first; it_++) {
          if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << " bid side sanitization at int_price: " << it_->first << DBGLOG_ENDL_FLUSH;
          }

          intpx_2_bid_mkt_order_vec_.erase(it_);
        }
      }

      // Tag the sim orders (orders sent by the query) with the order id of this order, if possible
      TagSimAsks(int_price_, t_order_id_);

      // Add the new order to bid market_order map.
      AddMarketAsk(int_price_, new_mkt_order);

      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        int level_ = 0;

        for (auto it_ = intpx_2_ask_mkt_order_vec_.begin(); it_ != intpx_2_ask_mkt_order_vec_.end(); it_++) {
          std::vector<SimMarketOrder*> market_order_vec_ = it_->second;

          bool break_from_loop_ = false;
          for (size_t i = 0; i < market_order_vec_.size(); i++) {
            level_++;
            if (market_order_vec_[i]->order_id_ == t_order_id_) {
              break_from_loop_ = true;
              break;
            }
          }

          if (break_from_loop_) {
            break;
          }
        }

        if (level_ != t_level_) {
          if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << " ask level does not match: " << int_price_ << " " << t_level_ << " " << level_
                                   << DBGLOG_ENDL_FLUSH;
          }
        }
      }

      // Check if any of our sim orders (orders sent by the query) match with this order. If yes, execute the matching
      // sim orders.
      ExecuteBidsEqAboveIntPrice(int_price_);

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogAskSimOrder(1, t_size_, int_price_);
      }
    } break;

    default:
      break;
  }
}

void OrderLevelSimMarketMaker2::OnOrderModifyOse(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                 const int t_level_, const int64_t t_old_order_id_,
                                                 const double t_old_price_, const int t_old_size_,
                                                 const int64_t t_new_order_id_, const double t_new_price_,
                                                 const int t_new_size_) {
  int int_price_ = 0;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (t_old_price_ == -2147483648) {
        int_price_ = 1000000;
      } else {
        int_price_ = smv_.GetIntPx(t_old_price_);
      }

      if (intpx_2_bid_mkt_order_vec_.find(int_price_) != intpx_2_bid_mkt_order_vec_.end()) {
        std::vector<SimMarketOrder*>& market_order_vec_ = intpx_2_bid_mkt_order_vec_[int_price_];

        size_t i;
        for (i = 0; i < market_order_vec_.size(); i++) {
          if (market_order_vec_[i]->order_id_ == t_old_order_id_) {
            // In OSE, t_old_size represents the size difference
            market_order_vec_[i]->size_ += t_old_size_;
            break;
          }
        }

        if (i == market_order_vec_.size() && dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "ERROR: could not find the order to modify" << DBGLOG_ENDL_FLUSH;
        }
      } else if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR: could not find price_level for the order to modify" << DBGLOG_ENDL_FLUSH;
      }

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogBidSimOrder(5, t_old_size_, int_price_);
      }
    } break;

    case kTradeTypeSell: {
      if (t_old_price_ == -2147483648) {
        int_price_ = 0;
      } else {
        int_price_ = smv_.GetIntPx(t_old_price_);
      }

      if (intpx_2_ask_mkt_order_vec_.find(int_price_) != intpx_2_ask_mkt_order_vec_.end()) {
        std::vector<SimMarketOrder*>& market_order_vec_ = intpx_2_ask_mkt_order_vec_[int_price_];

        size_t i;
        for (i = 0; i < market_order_vec_.size(); i++) {
          if (market_order_vec_[i]->order_id_ == t_old_order_id_) {
            // In OSE, t_old_size represents the size difference
            market_order_vec_[i]->size_ += t_old_size_;
            break;
          }
        }

        if (i == market_order_vec_.size() && dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "ERROR: could not find the order to modify" << DBGLOG_ENDL_FLUSH;
        }
      } else if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR: could not find price_level for the order to modify" << DBGLOG_ENDL_FLUSH;
      }

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogAskSimOrder(5, t_old_size_, int_price_);
      }
    } break;

    default:
      break;
  }
}

void OrderLevelSimMarketMaker2::OnOrderDeleteOse(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                 const int t_level_, const int64_t t_order_id_, const double t_price_,
                                                 const int t_size_) {
  int int_price_ = 0;

  // OSE order feed: size is actually the size changed.
  // int price: for -2147483648, set int_price to 0.
  if (t_price_ == -2147483648) {
    switch (t_buysell_) {
      case kTradeTypeBuy: {
        int_price_ = 1000000;
      } break;
      case kTradeTypeSell: {
        int_price_ = 0;
      } break;
      default:
        break;
    }
  } else {
    int_price_ = smv_.GetIntPx(t_price_);
  }

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (intpx_2_bid_mkt_order_vec_.find(int_price_) == intpx_2_bid_mkt_order_vec_.end()) {
        // Error
        if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " int_price: " << int_price_
                                 << " mkt_order_top: " << intpx_2_bid_mkt_order_vec_.begin()->first
                                 << " sim_order_top: " << intpx_bid_order_map_.begin()->first << " "
                                 << (t_buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << " " << smv_.secname()
                                 << DBGLOG_ENDL_FLUSH;
        }

        return;
      }

      // Find and remove the order using order_id_. Also extract the next order_id_ in that queue
      DeleteMarketBid(int_price_, t_order_id_);

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogBidSimOrder(2, t_size_, int_price_);
      }
    } break;

    case kTradeTypeSell: {
      if (intpx_2_ask_mkt_order_vec_.find(int_price_) == intpx_2_ask_mkt_order_vec_.end()) {
        // Error
        if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " int_price: " << int_price_
                                 << " mkt_order_top: " << intpx_2_ask_mkt_order_vec_.begin()->first
                                 << " sim_order_top: " << intpx_ask_order_map_.begin()->first << " "
                                 << (t_buysell_ == kTradeTypeBuy ? "BUY" : "SELL") << " " << smv_.secname()
                                 << DBGLOG_ENDL_FLUSH;
        }

        return;
      }

      // Find and remove the order using order_id_. Also extract the next order_id_ in that queue
      DeleteMarketAsk(int_price_, t_order_id_);

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogAskSimOrder(2, t_size_, int_price_);
      }
    } break;

    default:
      break;
  }
}

void OrderLevelSimMarketMaker2::OnOrderExecOse(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                               const int t_level_, const int64_t t_order_id_, const double t_price_,
                                               const int t_size_executed_, const int t_size_remaining_) {
  int int_price_ = smv_.GetIntPx(t_price_);

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (intpx_2_bid_mkt_order_vec_.find(int_price_) == intpx_2_bid_mkt_order_vec_.end()) {
        return;
      }

      if (ose_trade_time_manager_.isValidTimeToTrade(smv_.security_id(), watch_.tv().tv_sec % 86400) &&
          intpx_2_bid_mkt_order_vec_.begin()->first > int_price_) {
        return;
      }

      ExecuteBidsAboveIntPrice(int_price_);

      ExecuteSimBid(int_price_, t_order_id_, t_size_executed_);

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogBidSimOrder(3, t_size_executed_, int_price_);
      }
    } break;
    case kTradeTypeSell: {
      if (intpx_2_ask_mkt_order_vec_.find(int_price_) == intpx_2_ask_mkt_order_vec_.end()) {
        return;
      }

      if (ose_trade_time_manager_.isValidTimeToTrade(smv_.security_id(), watch_.tv().tv_sec % 86400) &&
          intpx_2_ask_mkt_order_vec_.begin()->first < int_price_) {
        return;
      }

      ExecuteAsksAboveIntPrice(int_price_);

      ExecuteSimAsk(int_price_, t_order_id_, t_size_executed_);

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogAskSimOrder(3, t_size_executed_, int_price_);
      }
    } break;
    default:
      break;
  }
}

void OrderLevelSimMarketMaker2::OnTradeOse(const unsigned int t_security_id_, const double t_trade_price_,
                                           const int t_trade_size_, const TradeType_t t_buysell_) {
  TradeType_t buysell_ = t_buysell_;
  int int_price_ = smv_.GetIntPx(t_trade_price_);

  if (buysell_ == kTradeTypeNoInfo) {
    if (int_price_ <= intpx_2_bid_mkt_order_vec_.begin()->first) {
      buysell_ = kTradeTypeSell;
    } else if (int_price_ >= intpx_2_ask_mkt_order_vec_.begin()->first) {
      buysell_ = kTradeTypeBuy;
    }
  }

  switch (buysell_) {
    case kTradeTypeSell: {
      if (intpx_2_bid_mkt_order_vec_.begin()->first - int_price_ >= 2) {
        // Handle weird trades in OSE
        return;
      }

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogBidSimOrder(4, t_trade_size_, int_price_);
      }
    } break;

    case kTradeTypeBuy: {
      if (int_price_ - intpx_2_ask_mkt_order_vec_.begin()->first >= 2) {
        // Handle weird trades in OSE
        return;
      }

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogAskSimOrder(4, t_trade_size_, int_price_);
      }
    } break;

    default:
      break;
  }
}

void OrderLevelSimMarketMaker2::OnOrderAddBmf(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                              const int t_level_, const int64_t t_order_id_, const double t_price_,
                                              const int t_size_) {
  int int_price_ = smv_.GetIntPx(t_price_);

  SimMarketOrder* p_new_market_order_ = GetNewMarketOrder(int_price_, t_size_, t_price_, t_order_id_);

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (intpx_2_bid_mkt_order_vec_.size() > 0) {
        auto it_ = intpx_2_bid_mkt_order_vec_.begin();
        std::vector<SimMarketOrder*>& market_order_vec_ = it_->second;

        if (it_->first == int_price_ && (int)market_order_vec_.size() != t_level_ - 1) {
          if ((int)market_order_vec_.size() + 1 > t_level_) {
            if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "Sanitizing bids at top level. Need to remove orders. Expected: "
                                     << market_order_vec_.size() + 1 << " Actual: " << t_level_ << DBGLOG_ENDL_FLUSH;
            }

            // Remove orders from the back
            while ((int)market_order_vec_.size() > t_level_ - 1) {
              DeleteMarketBid(int_price_, market_order_vec_.back()->order_id_);
            }
          } else if ((int)market_order_vec_.size() + 1 < t_level_) {
            if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "Sanitizing bids at top level. Need to add orders. Expected: "
                                     << market_order_vec_.size() + 1 << " Actual: " << t_level_ << DBGLOG_ENDL_FLUSH;
            }

            // Add orders
            while ((int)market_order_vec_.size() < t_level_ - 1) {
              //                          SimMarketOrder * missing_market_order_ = GetNewMarketOrder (int_price_,
              //                          smv_.min_order_size(), t_price_, 78889);
              AddMarketBid(int_price_, p_new_market_order_);
            }
          }
        }
      }

      // BMF specific sanitization
      if (t_level_ == 1)  // TODO: can we do something better here?
      {
        while (intpx_2_bid_mkt_order_vec_.size() > 0 && intpx_2_bid_mkt_order_vec_.begin()->first > int_price_) {
          auto it_ = intpx_2_bid_mkt_order_vec_.begin();
          int this_int_price_ = it_->first;

          if (intpx_bid_order_map_.find(this_int_price_) != intpx_bid_order_map_.end()) {
            std::vector<BaseSimOrder*>& sim_order_vec_ = intpx_bid_order_map_[int_price_];

            for (size_t i = 0; i < sim_order_vec_.size(); i++) {
              sim_order_vec_[i]->order_id_ = 0;
            }
          }

          intpx_2_bid_mkt_order_vec_.erase(it_);
        }
      }

      // Tag the sim orders (orders sent by the query) with the order id of this order, if possible
      TagSimBids(int_price_, t_order_id_);

      // Add the new order to bid market_order map.
      AddMarketBid(int_price_, p_new_market_order_);

      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        int level_ = 0;

        for (auto it_ = intpx_2_bid_mkt_order_vec_.begin(); it_ != intpx_2_bid_mkt_order_vec_.end(); it_++) {
          std::vector<SimMarketOrder*> market_order_vec_ = it_->second;

          bool break_from_loop_ = false;
          for (size_t i = 0; i < market_order_vec_.size(); i++) {
            level_++;
            if (market_order_vec_[i]->order_id_ == t_order_id_) {
              break_from_loop_ = true;
              break;
            }
          }

          if (break_from_loop_) {
            break;
          }
        }

        if (level_ != t_level_) {
          if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << " bid level does not match at int_px: " << int_price_
                                   << ". Expected: " << t_level_ << " Actual: " << level_
                                   << " order_id: " << (unsigned long long)t_order_id_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }

      // Check if any of our sim orders (orders sent by the query) match with this order. If yes, execute the matching
      // sim orders.
      ExecuteAsksAboveIntPrice(int_price_);
      ExecuteAsksAtIntPrice(int_price_, t_size_);

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogBidSimOrder(1, t_size_, int_price_);
      }
    } break;

    case kTradeTypeSell: {
      if (intpx_2_ask_mkt_order_vec_.size() > 0) {
        auto it_ = intpx_2_ask_mkt_order_vec_.begin();
        std::vector<SimMarketOrder*>& market_order_vec_ = it_->second;

        if (it_->first == int_price_ && (int)market_order_vec_.size() != t_level_ - 1) {
          if ((int)market_order_vec_.size() + 1 > t_level_) {
            if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "Sanitizing asks at top level. Need to remove orders. Expected: "
                                     << market_order_vec_.size() + 1 << " Actual: " << t_level_ << DBGLOG_ENDL_FLUSH;
            }

            // Remove orders from the back
            while ((int)market_order_vec_.size() > t_level_ - 1) {
              DeleteMarketAsk(int_price_, market_order_vec_.back()->order_id_);
            }
          } else if ((int)market_order_vec_.size() + 1 < t_level_) {
            if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "Sanitizing asks at top level. Need to add orders. Expected: "
                                     << market_order_vec_.size() + 1 << " Actual: " << t_level_ << DBGLOG_ENDL_FLUSH;
            }

            // Add orders
            while ((int)market_order_vec_.size() < t_level_ - 1) {
              //                          SimMarketOrder * missing_market_order_ = GetNewMarketOrder (int_price_,
              //                          smv_.min_order_size(), t_price_, 78889);
              AddMarketAsk(int_price_, p_new_market_order_);
            }
          }
        }
      }

      // BMF specific sanitization
      if (t_level_ == 1)  // TODO: do something better for sanitization
      {
        while (intpx_2_ask_mkt_order_vec_.size() > 0 && intpx_2_ask_mkt_order_vec_.begin()->first < int_price_) {
          auto it_ = intpx_2_ask_mkt_order_vec_.begin();
          int this_int_price_ = it_->first;

          if (intpx_ask_order_map_.find(this_int_price_) != intpx_ask_order_map_.end()) {
            std::vector<BaseSimOrder*>& sim_order_vec_ = intpx_ask_order_map_[int_price_];

            for (size_t i = 0; i < sim_order_vec_.size(); i++) {
              sim_order_vec_[i]->order_id_ = 0;
            }
          }

          intpx_2_ask_mkt_order_vec_.erase(it_);
        }
      }

      // Tag the sim orders (orders sent by the query) with the order id of this order, if possible
      TagSimAsks(int_price_, t_order_id_);

      // Add the new order to bid market_order map.
      AddMarketAsk(int_price_, p_new_market_order_);

      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        int level_ = 0;

        for (auto it_ = intpx_2_ask_mkt_order_vec_.begin(); it_ != intpx_2_ask_mkt_order_vec_.end(); it_++) {
          std::vector<SimMarketOrder*> market_order_vec_ = it_->second;

          bool break_from_loop_ = false;
          for (size_t i = 0; i < market_order_vec_.size(); i++) {
            level_++;
            if (market_order_vec_[i]->order_id_ == t_order_id_) {
              break_from_loop_ = true;
              break;
            }
          }

          if (break_from_loop_) {
            break;
          }
        }

        if (level_ != t_level_) {
          if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << " ask level does not match at int_px: " << int_price_
                                   << ". Expected: " << t_level_ << " Actual: " << level_
                                   << " order_id: " << (unsigned long long)t_order_id_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }

      // Check if any of our sim orders (orders sent by the query) match with this order. If yes, execute the matching
      // sim orders.
      ExecuteBidsAboveIntPrice(int_price_);
      ExecuteBidsAtIntPrice(int_price_, t_size_);

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogAskSimOrder(1, t_size_, int_price_);
      }
    } break;

    default:
      break;
  }
}

void OrderLevelSimMarketMaker2::OnOrderModifyBmf(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                 const int t_level_, const int64_t t_old_order_id_,
                                                 const double t_old_price_, const int t_old_size_,
                                                 const int64_t t_new_order_id_, const double t_new_price_,
                                                 const int t_new_size_) {
  int int_price_ = smv_.GetIntPx(t_old_price_);

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (intpx_2_bid_mkt_order_vec_.find(int_price_) == intpx_2_bid_mkt_order_vec_.end()) {
        return;
      }

      buy_side_trade_level_ = std::max(0, buy_side_trade_level_ - 1);

      std::vector<SimMarketOrder*>& market_order_vec_ = intpx_2_bid_mkt_order_vec_[int_price_];
      bool order_found_ = false;

      for (size_t i = 0; i < market_order_vec_.size(); i++) {
        if (market_order_vec_[i]->order_id_ == t_old_order_id_) {
          order_found_ = true;
          market_order_vec_[i]->size_ = t_old_size_;
          return;
        }
      }

      if (!order_found_) {
        int level_ = 0;
        for (auto it_ = intpx_2_bid_mkt_order_vec_.begin(); it_ != intpx_2_bid_mkt_order_vec_.end(); it_++) {
          std::vector<SimMarketOrder*>& market_order_vec_ = it_->second;

          bool break_from_loop_ = false;
          for (size_t i = 0; i < market_order_vec_.size(); i++) {
            level_++;
            if (level_ == t_level_) {
              market_order_vec_[i]->order_id_ = t_old_order_id_;
              market_order_vec_[i]->size_ = t_old_size_;

              break_from_loop_ = true;
              break;
            }
          }

          if (break_from_loop_) {
            break;
          }
        }
      }

      if (!order_found_) {
        //                  OnOrderAdd (t_security_id_, t_buysell_, t_level_, t_old_order_id_, t_old_price_,
        //                  t_old_size_);
      }
    } break;

    case kTradeTypeSell: {
      if (intpx_2_ask_mkt_order_vec_.find(int_price_) == intpx_2_ask_mkt_order_vec_.end()) {
        return;
      }

      sell_side_trade_level_ = std::max(0, sell_side_trade_level_ - 1);

      std::vector<SimMarketOrder*>& market_order_vec_ = intpx_2_ask_mkt_order_vec_[int_price_];
      bool order_found_ = false;

      for (size_t i = 0; i < market_order_vec_.size(); i++) {
        if (market_order_vec_[i]->order_id_ == t_old_order_id_) {
          order_found_ = true;
          market_order_vec_[i]->size_ = t_old_size_;
          return;
        }
      }

      if (!order_found_) {
        int level_ = 0;
        for (auto it_ = intpx_2_ask_mkt_order_vec_.begin(); it_ != intpx_2_ask_mkt_order_vec_.end(); it_++) {
          std::vector<SimMarketOrder*>& market_order_vec_ = it_->second;

          bool break_from_loop_ = false;
          for (size_t i = 0; i < market_order_vec_.size(); i++) {
            level_++;
            if (level_ == t_level_) {
              market_order_vec_[i]->order_id_ = t_old_order_id_;
              market_order_vec_[i]->size_ = t_old_size_;

              break_from_loop_ = true;
              break;
            }
          }

          if (break_from_loop_) {
            break;
          }
        }
      }

      if (!order_found_) {
        //                  OnOrderAdd (t_security_id_, t_buysell_, t_level_, t_old_order_id_, t_old_price_,
        //                  t_old_size_);
      }
    } break;

    default:
      break;
  }
}

SimMarketOrder* OrderLevelSimMarketMaker2::GetBuyOrderFromLevel(int level) {
  int temp_level = 1;

  for (auto& pair : intpx_2_bid_mkt_order_vec_) {
    for (auto sim_order : pair.second) {
      if (temp_level == level) {
        return sim_order;
      }
      temp_level++;
    }
  }

  return NULL;
}

SimMarketOrder* OrderLevelSimMarketMaker2::GetSellOrderFromLevel(int level) {
  int temp_level = 1;

  for (auto& pair : intpx_2_ask_mkt_order_vec_) {
    for (auto sim_order : pair.second) {
      if (temp_level == level) {
        return sim_order;
      }
      temp_level++;
    }
  }

  return NULL;
}

void OrderLevelSimMarketMaker2::OnOrderDeleteBmf(const unsigned int security_id, const TradeType_t buysell,
                                                 const int level, const int64_t order_id, const double price,
                                                 const int size) {
  switch (buysell) {
    case kTradeTypeBuy: {
      buy_side_trade_level_ = std::max(0, buy_side_trade_level_ - 1);

      SimMarketOrder* delete_order = GetBuyOrderFromLevel(level);

      if (delete_order != NULL) {
        DeleteMarketBid(delete_order->int_price_, order_id);
      } else if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR: buy order could not be found for order_level" << DBGLOG_ENDL_FLUSH;
      }

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogBidSimOrder(2, size, delete_order == NULL ? 0 : delete_order->int_price_);
      }
      break;
    }
    case kTradeTypeSell: {
      sell_side_trade_level_ = std::max(0, sell_side_trade_level_ - 1);

      SimMarketOrder* delete_order = GetSellOrderFromLevel(level);

      if (delete_order != NULL) {
        DeleteMarketAsk(delete_order->int_price_, order_id);
      } else if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR: sell order could not be found for order_level" << DBGLOG_ENDL_FLUSH;
      }

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogAskSimOrder(2, size, delete_order == NULL ? 0 : delete_order->int_price_);
      }
      break;
    }
    default:
      break;
  }
}

void OrderLevelSimMarketMaker2::OnOrderExecBmf(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                               const int t_level_, const int64_t t_order_id_, const double t_price_,
                                               const int t_size_executed_, const int t_size_remaining_) {
  // We don't get execs for individual orders in NTP
}

void OrderLevelSimMarketMaker2::OnTradeBmf(const unsigned int t_security_id_, const double t_trade_price_,
                                           const int t_trade_size_, const TradeType_t t_buysell_) {
  TradeType_t buysell_ = t_buysell_;
  int int_price = smv_.GetIntPx(t_trade_price_);

  if (buysell_ == kTradeTypeNoInfo) {
    if (int_price <= intpx_2_bid_mkt_order_vec_.begin()->first) {
      buysell_ = kTradeTypeSell;
    } else if (int_price >= intpx_2_ask_mkt_order_vec_.begin()->first) {
      buysell_ = kTradeTypeBuy;
    }
  }

  switch (buysell_) {
    case kTradeTypeSell: {
      // 1. Execute order above this level
      ExecuteBidsAboveIntPrice(int_price);

      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        if (intpx_2_bid_mkt_order_vec_.begin()->first > int_price) {
          DBGLOG_TIME_CLASS_FUNC << "WARNING: trade at non-best level" << DBGLOG_ENDL_FLUSH;
        }
      }

      buy_side_trade_level_++;
      SimMarketOrder* trade_order_ = GetBuyOrderFromLevel(buy_side_trade_level_);

      if (trade_order_ == NULL) {
        if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "ERROR: trade order not found. level: " << buy_side_trade_level_
                                 << DBGLOG_ENDL_FLUSH;
        }
      } else {
        int this_size_executed_ = std::min(t_trade_size_, trade_order_->size_);
        ExecuteSimBid(trade_order_->int_price_, trade_order_->order_id_, this_size_executed_,
                      t_trade_size_ >= trade_order_->size_);
      }

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogBidSimOrder(4, t_trade_size_, int_price);
      }
    } break;

    case kTradeTypeBuy: {
      // 1. Execute order above this level
      ExecuteAsksAboveIntPrice(int_price);

      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        if (intpx_2_ask_mkt_order_vec_.begin()->first < int_price) {
          DBGLOG_TIME_CLASS_FUNC << "WARNING: trade at non-best level" << DBGLOG_ENDL_FLUSH;
        }
      }

      sell_side_trade_level_++;
      SimMarketOrder* trade_order_ = GetSellOrderFromLevel(sell_side_trade_level_);

      if (trade_order_ == NULL) {
        if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "ERROR: trade order not found. level: " << sell_side_trade_level_
                                 << DBGLOG_ENDL_FLUSH;
        }
      } else {
        int this_size_executed_ = std::min(t_trade_size_, trade_order_->size_);
        ExecuteSimAsk(trade_order_->int_price_, trade_order_->order_id_, this_size_executed_,
                      t_trade_size_ >= trade_order_->size_);
      }

      if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
        LogAskSimOrder(4, t_trade_size_, int_price);
      }
    } break;

    default:
      break;
  }
}

// On new order from market data:
// 1. Add the new order to bid market_order map.
// 2. Check if any of our sim orders (orders sent by the query) match with this order. If yes, execute the matching sim
// orders.
// 3. Tag the sim orders (orders sent by the query) with the order id of this order, if possible
// 4. update best_bid_int_price
void OrderLevelSimMarketMaker2::OnOrderAdd(const unsigned int security_id, const TradeType_t buysell, const int level,
                                           const int64_t order_id, const double price, const int size) {
  ProcessRequestQueue();
  UpdateNumEvents();
  UpdateAggSizes();

  if (!process_mkt_updates_) return;

  switch (smv_.exch_source()) {
    case kExchSourceEUREX:
    case kExchSourceEOBI: {
      OnOrderAddEobi(security_id, buysell, level, order_id, price, size);
      break;
    }

    case kExchSourceJPY: {
      if (!ose_trade_time_manager_.isExchClosed(security_id, watch_.tv().tv_sec % 86400)) {
        OnOrderAddOse(security_id, buysell, level, order_id, price, size);
      }
      break;
    }

    case kExchSourceBMF: {
      OnOrderAddBmf(security_id, buysell, level, order_id, price, size);
      break;
    }

    default: { std::cerr << "Error in OrderLevelSimMarketMaker2::OnOrderAdd\n"; }
  }
}

void OrderLevelSimMarketMaker2::OnOrderModify(const unsigned int security_id, const TradeType_t buysell,
                                              const int level, const int64_t old_order_id, const double old_price,
                                              const int old_size, const int64_t new_order_id, const double new_price,
                                              const int new_size) {
  ProcessRequestQueue();
  UpdateNumEvents();
  UpdateAggSizes();

  if (!process_mkt_updates_) return;

  switch (smv_.exch_source()) {
    case kExchSourceEUREX:
    case kExchSourceEOBI: {
      OnOrderModifyEobi(security_id, buysell, level, old_order_id, old_price, old_size, new_order_id, new_price,
                        new_size);
      break;
    }

    case kExchSourceJPY: {
      if (!ose_trade_time_manager_.isExchClosed(security_id, watch_.tv().tv_sec % 86400)) {
        OnOrderModifyOse(security_id, buysell, level, old_order_id, old_price, old_size, new_order_id, new_price,
                         new_size);
      }
      break;
    }

    case kExchSourceBMF: {
      OnOrderModifyBmf(security_id, buysell, level, old_order_id, old_price, old_size, new_order_id, new_price,
                       new_size);
      break;
    }

    default: { std::cerr << "Error in OrderLevelSimMarketMaker2::OnOrderModify\n"; }
  }
}

// On order delete:
// 1. Find and remove the order using order_id_. Also extract the next order_id_ in that queue
// 2. Adjust best int price
// 3. If there exists sim order with order_id_ same as this order, assign their order_id_ to next_order_id_.
void OrderLevelSimMarketMaker2::OnOrderDelete(const unsigned int security_id, const TradeType_t t_buysell_,
                                              const int level, const int64_t order_id, const double price,
                                              const int size) {
  ProcessRequestQueue();
  UpdateNumEvents();
  UpdateAggSizes();

  if (!process_mkt_updates_) return;

  switch (smv_.exch_source()) {
    case kExchSourceEUREX:
    case kExchSourceEOBI: {
      OnOrderDeleteEobi(security_id, t_buysell_, level, order_id, price, size);
    } break;

    case kExchSourceJPY: {
      if (!ose_trade_time_manager_.isExchClosed(security_id, watch_.tv().tv_sec % 86400)) {
        OnOrderDeleteOse(security_id, t_buysell_, level, order_id, price, size);
      }
      break;
    }

    case kExchSourceBMF: {
      OnOrderDeleteBmf(security_id, t_buysell_, level, order_id, price, size);
    } break;

    default: { std::cerr << "Error in OrderLevelSimMarketMaker2::OnOrderDelete\n"; }
  }
}

// On order exec:
// 1. Execute sim orders more aggresive than the current order
// 2. Executed sim orders whose order_id_ match with this order_id_
// 3. Delete this order if the size_remaining is 0.
void OrderLevelSimMarketMaker2::OnOrderExec(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                            const int t_level_, const int64_t t_order_id_, const double t_price_,
                                            const int t_size_executed_, const int t_size_remaining_) {
  ProcessRequestQueue();
  UpdateNumEvents();
  UpdateAggSizes();

  switch (smv_.exch_source()) {
    case kExchSourceEUREX:
    case kExchSourceEOBI: {
      OnOrderExecEobi(t_security_id_, t_buysell_, t_level_, t_order_id_, t_price_, t_size_executed_, t_size_remaining_);
    } break;

    case kExchSourceJPY: {
      if (!ose_trade_time_manager_.isExchClosed(t_security_id_, watch_.tv().tv_sec % 86400)) {
        OnOrderExecOse(t_security_id_, t_buysell_, t_level_, t_order_id_, t_price_, t_size_executed_,
                       t_size_remaining_);
      }
      break;
    }

    case kExchSourceBMF: {
      OnOrderExecBmf(t_security_id_, t_buysell_, t_level_, t_order_id_, t_price_, t_size_executed_, t_size_remaining_);
    } break;

    default: { std::cerr << "Error in OrderLevelSimMarketMaker2::OnOrderExec\n"; }
  }
}

void OrderLevelSimMarketMaker2::OnTrade(const unsigned int t_security_id_, const double t_trade_price_,
                                        const int t_trade_size_, const TradeType_t t_buysell_) {
  ProcessRequestQueue();
  UpdateNumEvents();

  switch (smv_.exch_source()) {
    case kExchSourceEUREX:
    case kExchSourceEOBI: {
      OnTradeEobi(t_security_id_, t_trade_price_, t_trade_size_, t_buysell_);
    } break;

    case kExchSourceJPY: {
      if (!ose_trade_time_manager_.isExchClosed(t_security_id_, watch_.tv().tv_sec % 86400)) {
        OnTradeOse(t_security_id_, t_trade_price_, t_trade_size_, t_buysell_);
      }
    } break;

    case kExchSourceBMF: {
      OnTradeBmf(t_security_id_, t_trade_price_, t_trade_size_, t_buysell_);
    } break;

    default: { std::cerr << "Error in OrderLevelSimMarketMaker2::OnTrade\n"; }
  }
}

void OrderLevelSimMarketMaker2::SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {}

int OrderLevelSimMarketMaker2::GetQueueSize(std::vector<SimMarketOrder*>& market_order_vec) {
  int total_size = 0;
  for (auto order : market_order_vec) {
    total_size += order->size_;
  }

  return total_size;
}

int OrderLevelSimMarketMaker2::ProcessBidSendRequest(BaseSimOrder* sim_order) {
  int saci = sim_order->server_assigned_client_id();
  double prev_conf_price = sim_order->price();
  sim_order->queue_size_behind_ = 0;
  sim_order->queue_size_ahead_ = smv_.bid_size_at_int_price(sim_order->int_price_);
  sim_order->order_id_ = 0;

  // Check if this is an aggressive buy order
  if (sim_order->int_price_ >= intpx_2_ask_mkt_order_vec_.begin()->first) {
    if (intpx_2_ask_mkt_order_vec_.begin()->first < smv_.market_update_info_.bestask_int_price_ &&
        smv_.exch_source() == kExchSourceBMF) {
      return 1;
    }

    for (int int_price = intpx_2_ask_mkt_order_vec_.begin()->first; int_price <= sim_order->int_price_; int_price++) {
      if (intpx_2_ask_mkt_order_vec_.find(int_price) == intpx_2_ask_mkt_order_vec_.end() ||
          (smv_.exch_source() == kExchSourceJPY && int_price == 0) ||
          intpx_2_bid_mkt_order_vec_.begin()->first >= intpx_2_ask_mkt_order_vec_.begin()->first) {
        continue;
      }

      sim_order->price_ = smv_.GetDoublePx(int_price);

      // Execute the order, update position, send EXEC
      auto& market_order_vec = intpx_2_ask_mkt_order_vec_[int_price];
      int ask_size = GetQueueSize(market_order_vec);

      if (intpx_agg_bid_size_map_[saci].find(int_price) != intpx_agg_bid_size_map_[saci].end()) {
        ask_size = ask_size - intpx_agg_bid_size_map_[saci][int_price].second;
      }

      // If we have already used this price level for some other aggressive order, skip this level
      if (ask_size <= 0) {
        continue;
      }

      int size_executed = std::min(sim_order->size_remaining_, ask_size);

      if (size_executed == sim_order->size_remaining_) {
        // Full exec
        size_executed = sim_order->ExecuteRemaining();
      } else {
        // Partial match and append sim order to the order_vec_
        size_executed = sim_order->MatchPartial(size_executed);
      }

      if (size_executed > 0) {
        LogExec(sim_order, size_executed);
        client_position_map_[saci] += size_executed;
        global_position_to_send_map_[saci] += size_executed;
        intpx_agg_bid_size_map_[saci][int_price].first = GetQueueSize(market_order_vec);
        intpx_agg_bid_size_map_[saci][int_price].second += size_executed;
        BroadcastExecNotification(saci, sim_order);
      }
    }
  }

  if (sim_order->size_remaining() <= 0) {
    return 0;
  }

  if (sim_order->is_ioc_) {
    BroadcastCancelNotification(saci, sim_order);
    return 0;
  }

  // Restore the order price for the remaining size
  sim_order->price_ = prev_conf_price;

  // Push to the order vector
  intpx_bid_order_map_[sim_order->int_price_].push_back(sim_order);

  if (sim_order->int_price_ > smv_.bestbid_int_price()) {
    sim_order->alone_above_best_market_ = true;
  }

  return 0;
}

int OrderLevelSimMarketMaker2::ProcessAskSendRequest(BaseSimOrder* sim_order) {
  int saci = sim_order->server_assigned_client_id();
  double prev_conf_price = sim_order->price();
  sim_order->queue_size_behind_ = 0;
  sim_order->queue_size_ahead_ = smv_.ask_size_at_int_price(sim_order->int_price_);
  sim_order->order_id_ = 0;

  // Check if this is an aggressive sell order
  if (sim_order->int_price_ <= intpx_2_bid_mkt_order_vec_.begin()->first) {
    if (intpx_2_bid_mkt_order_vec_.begin()->first > smv_.market_update_info_.bestbid_int_price_ &&
        smv_.exch_source() == kExchSourceBMF) {
      return 1;
    }

    for (int this_int_price_ = intpx_2_bid_mkt_order_vec_.begin()->first; this_int_price_ >= sim_order->int_price_;
         this_int_price_--) {
      if (intpx_2_bid_mkt_order_vec_.find(this_int_price_) == intpx_2_bid_mkt_order_vec_.end() ||
          (smv_.exch_source() == kExchSourceJPY && this_int_price_ == 1000000) ||
          intpx_2_bid_mkt_order_vec_.begin()->first >= intpx_2_ask_mkt_order_vec_.begin()->first) {
        continue;
      }

      sim_order->price_ = smv_.GetDoublePx(intpx_2_bid_mkt_order_vec_.begin()->first);

      // Execute the order, update position, send EXEC
      auto& market_order_vec = intpx_2_bid_mkt_order_vec_[intpx_2_bid_mkt_order_vec_.begin()->first];
      int bid_size = GetQueueSize(market_order_vec);

      if (intpx_agg_ask_size_map_[saci].find(this_int_price_) != intpx_agg_ask_size_map_[saci].end()) {
        bid_size = bid_size - intpx_agg_ask_size_map_[saci][this_int_price_].second;
      }

      // If we have already used this price level for some other aggressive order, skip this level
      if (bid_size <= 0) {
        continue;
      }

      int size_executed = std::min(sim_order->size_remaining_, bid_size);

      if (size_executed == sim_order->size_remaining_) {
        size_executed = sim_order->ExecuteRemaining();
      } else {
        size_executed = sim_order->MatchPartial(size_executed);
      }

      if (size_executed > 0) {
        LogExec(sim_order, size_executed);
        client_position_map_[saci] -= size_executed;
        global_position_to_send_map_[saci] -= size_executed;
        intpx_agg_ask_size_map_[saci][this_int_price_].first = GetQueueSize(market_order_vec);
        intpx_agg_ask_size_map_[saci][this_int_price_].second += size_executed;
        BroadcastExecNotification(saci, sim_order);
      }
    }
  }

  if (sim_order->size_remaining() <= 0) {
    return 0;
  }

  if (sim_order->is_ioc_) {
    BroadcastCancelNotification(saci, sim_order);
    return 0;
  }

  // Restore the order price for the remaining size
  sim_order->price_ = prev_conf_price;

  // Push to the order vector
  intpx_ask_order_map_[sim_order->int_price_].push_back(sim_order);

  if (sim_order->int_price_ < smv_.bestask_int_price()) {
    sim_order->alone_above_best_market_ = true;
  }
  return 0;
}
}
