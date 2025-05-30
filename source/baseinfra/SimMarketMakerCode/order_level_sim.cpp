/**
 \file SimMarketMakerCode/order_level_sim.cpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite 217, Level 2, Prestige Omega,
 No 104, EPIP Zone, Whitefield,
 Bangalore - 560066, India
 +91 80 4060 0717
 */

#include "baseinfra/SimMarketMaker/order_level_sim.hpp"

namespace HFSAT {

/**
 * Unique instance
 *
 * @param dbglogger
 * @param watch
 * @param dep_market_view
 * @param market_model_index
 * @param sim_time_series_info
 * @return
 */
OrderLevelSim* OrderLevelSim::GetUniqueInstance(DebugLogger& dbglogger, Watch& watch,
                                                SecurityMarketView& dep_market_view, int market_model_index,
                                                SimTimeSeriesInfo& sim_time_series_info) {
  MarketModel this_market_model;
  MarketModelManager::GetMarketModel(dep_market_view.shortcode(), market_model_index, this_market_model,
                                     watch.YYYYMMDD());

  std::ostringstream temp_oss;
  temp_oss << "OrderLevelSim " << dep_market_view.shortcode() << ' ' << market_model_index;

  std::string olsmm_description(temp_oss.str());

  static std::map<std::string, OrderLevelSim*> SMM_description_map;
  if (SMM_description_map.find(olsmm_description) == SMM_description_map.end()) {
    SMM_description_map[olsmm_description] =
        new OrderLevelSim(dbglogger, watch, dep_market_view, this_market_model, sim_time_series_info);
  }

  return SMM_description_map[olsmm_description];
}

/**
 * Just instance, used in Testing
 * @param dbglogger
 * @param watch
 * @param dep_market_view
 * @param market_model_index
 * @param sim_time_series_info
 * @return
 */

OrderLevelSim* OrderLevelSim::GetInstance(DebugLogger& dbglogger, Watch& watch, SecurityMarketView& dep_market_view,
                                          int market_model_index, SimTimeSeriesInfo& sim_time_series_info) {
  MarketModel this_market_model;
  MarketModelManager::GetMarketModel(dep_market_view.shortcode(), market_model_index, this_market_model,
                                     watch.YYYYMMDD());

  std::ostringstream temp_oss;
  temp_oss << "OrderLevelSim " << dep_market_view.shortcode() << ' ' << market_model_index;

  std::string olsmm_description(temp_oss.str());

  OrderLevelSim* olsmm = new OrderLevelSim(dbglogger, watch, dep_market_view, this_market_model, sim_time_series_info);

  return olsmm;
}

/**
 *  Constructor
 * @param dbglogger
 * @param watch
 * @param dep_market_view
 * @param market_model
 * @param sim_time_series_info
 */
OrderLevelSim::OrderLevelSim(DebugLogger& dbglogger, Watch& watch, SecurityMarketView& dep_market_view,
                             MarketModel market_model, SimTimeSeriesInfo& sim_time_series_info)
    : BaseSimMarketMaker(dbglogger, watch, dep_market_view, market_model, sim_time_series_info),
      mov_(MarketOrdersView::GetUniqueInstance(dbglogger, watch, dep_market_view.security_id())) {
  GetMatchingAlgoForShortcode(dep_market_view.shortcode(), watch.YYYYMMDD());

  mov_->SubscribeQueuePosChange(this);

  // Subscribe to 1 msec time period update callback
  watch_.subscribe_first_SmallTimePeriod(this);
}

// 1 msec time period update callback handler
void OrderLevelSim::OnTimePeriodUpdate(const int num_pages_to_add) {
  if (!all_requests_.empty()) {
    ProcessRequestQueue();
  }
}

void OrderLevelSim::LogUpdate(TradeType_t buysell, std::string update, int int_price, int position, int size) {
  if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << mov_->GetMarketString() << "[" << smv_.bestbid_ordercount() << " " << smv_.bestbid_size()
                           << " " << smv_.bestbid_price() << " * " << smv_.bestask_price() << " " << smv_.bestask_size()
                           << " " << smv_.bestask_ordercount() << "]"
                           << " " << GetTradeTypeChar(buysell) << " " << update << " " << size << " at " << int_price
                           << " pos: " << position << " "
                           << (buysell == kTradeTypeBuy ? GetBidSimOrderStr(int_price) : GetAskSimOrderStr(int_price))
                           << DBGLOG_ENDL_FLUSH;
  }
}

/**
 *
 * @param sim_order
 * @param update
 */
void OrderLevelSim::LogOrderStatus(BaseSimOrder* sim_order, std::string update) {
  if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << update << " " << sim_order->security_name() << ' '
                           << GetTradeTypeChar(sim_order->buysell()) << ' ' << sim_order->price() << ' '
                           << sim_order->size_remaining() << " sE:" << sim_order->size_executed() << ' '
                           << ToString(sim_order->order_status()) << " [ " << sim_order->queue_size_ahead() << "-"
                           << sim_order->queue_size_behind() << '-' << sim_order->num_events_seen() << " ] "
                           << " MOV_SZ: " << mov_->GetSize(sim_order->buysell_, sim_order->int_price())

                           << " CAOS: " << sim_order->client_assigned_order_sequence()
                           << " SAOS: " << sim_order->server_assigned_order_sequence() << " [ " << smv_.bestbid_size()
                           << " " << smv_.bestbid_price() << " * " << smv_.bestask_price() << " " << smv_.bestask_size()
                           << " ] " << DBGLOG_ENDL_FLUSH;
  }
}

int OrderLevelSim::GetSizeAtIntPrice(TradeType_t buysell, int int_price) { return mov_->GetSize(buysell, int_price); }

/**
 * Callback from MarketOrderView ?
 *
 * @param sim_order
 * @param size
 * @param position
 */
void OrderLevelSim::PosAdd(BaseSimOrder* sim_order, int size, int position) {
  int int_price = sim_order->int_price_;
  int mov_sz = mov_->GetSize(sim_order->buysell_, int_price);

  if (sim_order->num_events_seen_ == 0) {
    // Since the order got confirmed right now, it implies that sim order arrived earlier than the present order in mkt
    int last_add_size = 0;
    (void)mov_->GetLastAdd(sim_order->buysell_, int_price, &last_add_size);
    sim_order->queue_size_ahead_ = mov_sz - last_add_size;
    sim_order->queue_size_behind_ = last_add_size;

    // TODO: Approximate this sim order with the closest of the last two orders in market
    /*int last_add_size = 0;
     ttime_t last_add_time = mov_->GetLastAdd(sim_order->buysell_, int_price, &last_add_size);

     ttime_t earlier = sim_order->order_confirmation_time_ - last_add_time;
     ttime_t later = watch_.tv() - sim_order->order_confirmation_time_;

     if (last_add_time != ttime_t(0, 0) && last_add_size > 0 && earlier < later) {
     int deduct_sz = std::min(last_add_size, sim_order->size_remaining());
     sim_order->queue_size_ahead_ -= deduct_sz;
     }*/
  } else if (sim_order->queue_size_ahead_ <= position) {
    sim_order->queue_size_behind_ += size;
  } else {
    sim_order->queue_size_ahead_ += size;
  }

  int total_sz = sim_order->queue_size_ahead_ + sim_order->queue_size_behind_;

  if (total_sz != mov_sz) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR qa/qb does not match."
                           << " " << GetTradeTypeChar(sim_order->buysell_) << " intpx: " << sim_order->int_price_
                           << " qa: " << sim_order->queue_size_ahead_ << " qb: " << sim_order->queue_size_behind_
                           << " total: " << total_sz << " mov: " << mov_sz
                           << " bidsmv: " << smv_.bid_size_at_int_price(int_price)
                           << " asksmv: " << smv_.ask_size_at_int_price(int_price) << DBGLOG_ENDL_FLUSH;
  }
  LogOrderStatus(sim_order, "PosAdd");
}

/**
 *
 * @param sim_order
 * @param size
 * @param position
 */
void OrderLevelSim::PosDel(BaseSimOrder* sim_order, int size, int position) {
  int int_price = sim_order->int_price_;
  int mov_sz = mov_->GetSize(sim_order->buysell_, int_price);

  if (sim_order->num_events_seen_ == 0) {
    sim_order->queue_size_ahead_ = mov_sz;
    sim_order->queue_size_behind_ = 0;
  } else if (sim_order->queue_size_ahead_ <= position) {
    sim_order->queue_size_behind_ -= size;
  } else {
    sim_order->queue_size_ahead_ -= size;
  }

  int total_sz = sim_order->queue_size_ahead_ + sim_order->queue_size_behind_;

  if (total_sz != mov_sz) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR qa/qb does not match."
                           << " qa: " << sim_order->queue_size_ahead_ << " qb: " << sim_order->queue_size_behind_
                           << " total: " << total_sz << " mov: " << mov_sz
                           << " bidsmv: " << smv_.bid_size_at_int_price(int_price)
                           << " asksmv: " << smv_.ask_size_at_int_price(int_price) << DBGLOG_ENDL_FLUSH;
  }

  LogOrderStatus(sim_order, "PosDel");
}

/**
 *
 * @param sim_order
 * @param size
 * @param position
 */
void OrderLevelSim::PosExe(BaseSimOrder* sim_order, int size, int position) {
  int int_price = sim_order->int_price_;
  int mov_sz = mov_->GetSize(sim_order->buysell_, int_price);

  if (sim_order->num_events_seen_ == 0) {
    sim_order->queue_size_ahead_ = mov_sz;
    sim_order->queue_size_behind_ = 0;
  }

  // Since this function is specific to FIFO trading algo,
  // ideally the position should always be 0.
  // But, this may not hold where we expect explicit order delete
  // after a bunch of consecutive order execs (OSE,BMF)

  int size_available = 0;

  // Give this sim order fills if the order exec is received
  // for the last order
  int qs = sim_order->queue_size_ahead_ + sim_order->queue_size_behind_;
  if (qs <= position + size) {
    if (dbglogger_.CheckLoggingLevel(OLSMM_INFO) && qs < position + size) {
      DBGLOG_TIME_CLASS_FUNC << "ERROR: qs < pos + size."
                             << " qs: " << qs << " pos: " << position << " sz: " << size << DBGLOG_ENDL_FLUSH;
    }
    size_available = std::min(size, sim_order->size_remaining());
  } else {
    // Compute the size available for exec to this sim order based on its
    // queue position and the position at which the exec occurred
    size_available = std::min(position + size - sim_order->queue_size_ahead_, sim_order->size_remaining());
  }

  // The actual size available should be bound by the total exec size
  // available for each client (saci).
  int saci = sim_order->server_assigned_client_id();

  size_available = std::min(size_available, size - saci_to_executed_size_[saci]);

  int size_executed = 0;
  if (size_available == sim_order->size_remaining()) {
    size_executed = sim_order->ExecuteRemaining();
  } else {
    size_executed = sim_order->MatchPartial(size_available);
  }

  if (size_executed > 0) {
    if (sim_order->buysell_ == kTradeTypeBuy) {
      client_position_map_[saci] += size_executed;
      global_position_to_send_map_[saci] += size_executed;
    } else if (sim_order->buysell_ == kTradeTypeSell) {
      client_position_map_[saci] -= size_executed;
      global_position_to_send_map_[saci] -= size_executed;
    }
    saci_to_executed_size_[saci] += size_executed;

    LogExec(sim_order, size_executed);
    BroadcastExecNotification(sim_order->server_assigned_client_id_, sim_order);
  }

  LogOrderStatus(sim_order, "PosExec");
}

/**
 *
 * @param sim_order
 * @param size
 */
void OrderLevelSim::SplitFIFOProRata(BaseSimOrder* sim_order, int size) {
  int int_price = sim_order->int_price_;

  int top_order_size_ = 0;
  unsigned int total_size_filled_ = 0;
  unsigned int fifo_index_ = 0;

  auto& bid_mkt_vec_ = mov_->GetBidOrders(int_price);
  auto& ask_mkt_vec_ = mov_->GetAskOrders(int_price);

  auto vector_size = 0u;
  // fill the vector of order sizes here.
  auto my_order_index_ = -1;
  auto qa_ = sim_order->queue_size_ahead_;
  auto sum_size_ = 0;
  std::vector<int> mkt_order_sizes_;

  if (sim_order->buysell_ == kTradeTypeBuy) {
    vector_size = bid_mkt_vec_.size();
    top_order_size_ = mov_->GetTopOrderSizeBidLevels(int_price);
    for (unsigned int order_pointer_ = 0; order_pointer_ < vector_size; order_pointer_++) {
      mkt_order_sizes_.push_back(bid_mkt_vec_[order_pointer_]->size_);
      if (sum_size_ == qa_) {
        my_order_index_ = order_pointer_;
      }
      sum_size_ += bid_mkt_vec_[order_pointer_]->size_;
    }
  } else if (sim_order->buysell_ == kTradeTypeSell) {  // Ask what to do on NoInfo
    vector_size = ask_mkt_vec_.size();
    top_order_size_ = mov_->GetTopOrderSizeAskLevels(int_price);
    for (unsigned int order_pointer_ = 0; order_pointer_ < vector_size; order_pointer_++) {
      mkt_order_sizes_.push_back(ask_mkt_vec_[order_pointer_]->size_);
      if (sum_size_ == qa_) {
        my_order_index_ = order_pointer_;
      }
      sum_size_ += ask_mkt_vec_[order_pointer_]->size_;
    }
  }

  // vector for the fills various orders are getting in market
  std::vector<int> order_filled_sizes_;
  order_filled_sizes_.resize(mkt_order_sizes_.size(), 0);

  // Vector for all indices of orders which were filled pro-rata
  std::vector<int> prorata_filled_indexes_;
  prorata_filled_indexes_.resize(mkt_order_sizes_.size(), 0);

  auto my_order_filled_size = 0;
  auto my_order_got_pro_rata_fill = false;

  if (top_order_size_ >= size && top_order_size_ > 0) {
    // all the fills must be given as the top order fills
    order_filled_sizes_[0] = size;

  } else {
    if (size > top_order_size_ && top_order_size_ > 0) {
      // give some of the fills as top order fills and remaining as prorata
      // Step1 : fill the top order completely
      order_filled_sizes_[0] = top_order_size_;
      total_size_filled_ += top_order_size_;
      fifo_index_ = 1;

      if (sim_order->buysell_ == kTradeTypeBuy) {
        mov_->RemoveBidTopOrder(int_price);
      } else if (sim_order->buysell_ == kTradeTypeSell) {  // Ask what to do on NoInfo
        mov_->RemoveAskTopOrder(int_price);
      }
    }

    // Step 2 :40% with FIFO
    int size_remaining_to_fill_ = size - total_size_filled_;

    int size_remaining_to_fill_with_FIFO_ =
        (int)(size_remaining_to_fill_ *
              SecurityDefinitions::GetFIFOPercentageForSimShortcode(dep_shortcode_, watch_.YYYYMMDD()));
    int size_remaining_to_fill_with_prorata_ = size_remaining_to_fill_ - size_remaining_to_fill_with_FIFO_;

    for (unsigned int order_pointer_ = 1; order_pointer_ < vector_size; order_pointer_++) {
      int size_to_fill_ = mkt_order_sizes_[order_pointer_];
      if (size_remaining_to_fill_with_FIFO_ <= 0) {
        break;
      } else if (size_remaining_to_fill_with_FIFO_ >= size_to_fill_) {
        order_filled_sizes_[order_pointer_] += size_to_fill_;
        size_remaining_to_fill_with_FIFO_ -= size_to_fill_;
        fifo_index_ = order_pointer_;
      } else if (size_remaining_to_fill_with_FIFO_ < size_to_fill_) {
        fifo_index_ = order_pointer_;
        order_filled_sizes_[order_pointer_] += size_remaining_to_fill_;
        size_remaining_to_fill_with_FIFO_ = 0;
      }
    }

    // Compute my traded size
    if (size_remaining_to_fill_with_FIFO_ > sim_order->queue_size_ahead_) {
      my_order_filled_size =
          std::min(size_remaining_to_fill_with_FIFO_ - sim_order->queue_size_ahead_, sim_order->size_remaining_);
    }

    // Calculating total size left in the queue to be filled for pro-rata fills
    int total_size_remaining_ = 0;
    for (unsigned int order_pointer_ = fifo_index_; order_pointer_ < vector_size; order_pointer_++) {
      if (order_pointer_ == fifo_index_) {
        total_size_remaining_ += mkt_order_sizes_[order_pointer_] - order_filled_sizes_[order_pointer_];
      } else {
        total_size_remaining_ += mkt_order_sizes_[order_pointer_];
      }
    }

    int total_size_filled_prorata_ = 0;
    // Step 3 : Allocating pro rata fills
    for (unsigned int order_pointer_ = fifo_index_; order_pointer_ < vector_size; order_pointer_++) {
      int size_to_fill_;
      if (order_pointer_ == fifo_index_) {
        size_to_fill_ = (int)((size_remaining_to_fill_with_prorata_ / ((float)total_size_remaining_)) *
                              (mkt_order_sizes_[order_pointer_] - order_filled_sizes_[order_pointer_]));
      } else {
        size_to_fill_ = (int)((size_remaining_to_fill_with_prorata_ / ((float)total_size_remaining_)) *
                              mkt_order_sizes_[order_pointer_]);
      }
      if (size_to_fill_ >= 1) {
        order_filled_sizes_[order_pointer_] += size_to_fill_;
        total_size_filled_prorata_ += size_to_fill_;
        size_remaining_to_fill_with_prorata_ -= size_to_fill_;
        prorata_filled_indexes_[order_pointer_] = 1;
      }
    }

    if (my_order_filled_size < sim_order->size_remaining()) {
      // If my order was not already filled in FIFO iteration
      auto pro_rata_filled_size =
          (double)((sim_order->size_remaining() - my_order_filled_size) * size_remaining_to_fill_with_prorata_) /
          (double)total_size_remaining_;

      if (pro_rata_filled_size >= 1) {
        my_order_filled_size += pro_rata_filled_size;
        my_order_got_pro_rata_fill = true;
      }
    }

    // Step 4 : Pro-Rata levelling i.e providing one lot fill to orders that didnt get fills in previous round
    int size_left_for_prorata_levelling_ = size_remaining_to_fill_with_prorata_ - total_size_filled_prorata_;
    if (size_left_for_prorata_levelling_ > 0) {
      for (unsigned int order_pointer_ = fifo_index_; order_pointer_ < vector_size; order_pointer_++) {
        if (prorata_filled_indexes_[order_pointer_] == 0 && size_left_for_prorata_levelling_ > 0) {
          // If this order didn't get any fills in pro-rata macthing
          order_filled_sizes_[order_pointer_] += 1;
          size_left_for_prorata_levelling_ -= 1;
        }

        if ((int)order_pointer_ == my_order_index_ && my_order_filled_size < sim_order->size_remaining() &&
            !my_order_got_pro_rata_fill) {
          // If we didn't get any fills in pro-rata
          my_order_filled_size += 1;
        }
      }
    }

    // Step 5 : FIFO for remaining orders if any
    if (size_left_for_prorata_levelling_ > 0) {
      for (unsigned int order_pointer_ = 1; order_pointer_ < vector_size; order_pointer_++) {
        int size_to_fill_ = mkt_order_sizes_[order_pointer_] - order_filled_sizes_[order_pointer_];

        if (size_left_for_prorata_levelling_ <= 0) {
          break;
        } else if (size_left_for_prorata_levelling_ >= size_to_fill_) {
          order_filled_sizes_[order_pointer_] += size_to_fill_;
          size_left_for_prorata_levelling_ -= size_to_fill_;
        } else if (size_left_for_prorata_levelling_ < size_to_fill_) {
          order_filled_sizes_[order_pointer_] += size_left_for_prorata_levelling_;
          size_left_for_prorata_levelling_ = 0;
        }

        // For our orders
        if ((int)order_pointer_ == my_order_index_) {
          // My order is at same index as this order, the size filled should be same-residual
          int my_size_to_fill = sim_order->size_remaining() - my_order_filled_size;
          // Compute our size to be filled
          if (size_remaining_to_fill_ >= my_size_to_fill) {
            my_order_filled_size += my_size_to_fill;
          } else {
            my_order_filled_size += size_remaining_to_fill_;
          }
          // No need to compute anything further as we are only concerned about our own order fills
          break;
        }
      }
    }
  }

  if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " Printing Fills: \n";
    for (auto id = 0u; id < mkt_order_sizes_.size(); id++) {
      if (order_filled_sizes_[id] > 0) {
        // Print only for orders which have filled size > 0
        dbglogger_ << " [ " << id << " " << mkt_order_sizes_[id] << " " << order_filled_sizes_[id] << " ] ";
      }
    }
    dbglogger_ << "\n[MY_ORDER " << my_order_index_ << " " << my_order_filled_size << " " << sim_order->ToString()
               << " ]\n";
    dbglogger_ << DBGLOG_ENDL_FLUSH;
  }

  // execute the size that has to be executed
  int saci = sim_order->server_assigned_client_id();
  int my_order_size_to_execute_ = order_filled_sizes_[my_order_index_];

  int size_executed = 0;
  if (my_order_size_to_execute_ == sim_order->size_remaining()) {
    size_executed = sim_order->ExecuteRemaining();
  } else {
    size_executed = sim_order->MatchPartial(my_order_size_to_execute_);
  }

  if (size_executed > 0) {
    if (sim_order->buysell_ == kTradeTypeBuy) {
      client_position_map_[saci] += size_executed;
      global_position_to_send_map_[saci] += size_executed;
    } else if (sim_order->buysell_ == kTradeTypeSell) {
      client_position_map_[saci] -= size_executed;
      global_position_to_send_map_[saci] -= size_executed;
    }
    saci_to_executed_size_[saci] += size_executed;
    LogExec(sim_order, size_executed);
    BroadcastExecNotification(sim_order->server_assigned_client_id_, sim_order);
  }

  LogOrderStatus(sim_order, "SplitFIFOProRata");
}

int OrderLevelSim::ICEOrderSizeMatch(int const size_total_resting_order_, int const size_preceding_order_,
                                     int const size_current_order_, int const market_order_size_to_fill_,
                                     int base_power) {
  float prorata_factor_ = (pow((size_total_resting_order_ - size_preceding_order_), base_power) -
                           pow((size_total_resting_order_ - size_preceding_order_ - size_current_order_), base_power)) /
                          (pow(size_total_resting_order_, base_power));
  int fill_to_give_ = fmin(size_current_order_, std::ceil(prorata_factor_ * market_order_size_to_fill_));
  return fill_to_give_;
}

void OrderLevelSim::ICEProRata(BaseSimOrder* sim_order, int size_to_execute_, int base_power) {
  int int_price_ = sim_order->int_price_;
  std::vector<ExchMarketOrder*> bid_mkt_vec_ = mov_->GetBidOrders(int_price_);
  std::vector<ExchMarketOrder*> ask_mkt_vec_ = mov_->GetAskOrders(int_price_);
  std::vector<int> order_filled_size_;
  order_filled_size_.resize(bid_mkt_vec_.size(), 0);
  // int vector_size = 0;
  int num_order_ahead_ = sim_order->queue_size_ahead_;
  int my_order_index_ = -1;
  int total_resting_order_bid_side_ = 0;
  int total_resting_order_ask_side_ = 0;
  int total_size_filled_ = 0;
  int total_size_left_ = size_to_execute_;
  int sum_order_size_ahead = 0;
  int size_to_fill_ = 0;
  int sum_size_ = 0;
  int my_order_fifo_fill_ = 0;
  int my_order_prorata_fill_ = 0;
  std::vector<int> mkt_order_sizes_filled_;

  // get the total number of resting order at bid level
  for (auto order_index = 0u; order_index < bid_mkt_vec_.size(); order_index++) {
    total_resting_order_bid_side_ += bid_mkt_vec_[order_index]->size_;
  }

  // get the total number of resting order at ask level
  for (auto order_index = 0u; order_index < ask_mkt_vec_.size(); order_index++) {
    total_resting_order_ask_side_ += ask_mkt_vec_[order_index]->size_;
  }

  // get my oder index
  if (sim_order->buysell_ == kTradeTypeBuy) {
    for (auto order_index = 0u; order_index < bid_mkt_vec_.size(); order_index++) {
      if (sum_order_size_ahead == num_order_ahead_) {
        my_order_index_ = order_index;
      }
      sum_order_size_ahead += bid_mkt_vec_[order_index]->size_;
    }
  } else {
    for (auto order_index = 0u; order_index < ask_mkt_vec_.size(); order_index++) {
      if (sum_order_size_ahead == num_order_ahead_) {
        my_order_index_ = order_index;
      }
      sum_order_size_ahead += ask_mkt_vec_[order_index]->size_;
    }
  }

  // STEP 1 : Do a forward pass to give ProRata fills
  // STEP 2 : For the order left in the first pass give FIFO fills

  // STEP 1

  // get the order index of my order and the fills to give at each level and give pro rata fills to the other orders
  if (sim_order->buysell_ == kTradeTypeBuy) {
    // By default the fill to give for each order is 0, hence initializing the array with 0
    mkt_order_sizes_filled_.resize(bid_mkt_vec_.size(), 0);

    // give pro rata fills to all orders in the bid mkt vec

    for (auto order_index = 0u; order_index < bid_mkt_vec_.size(); order_index++) {
      // mkt_order_sizes_filled.push_back(bid_mkt_vec_[order_index]->size_);

      // Get out of the loop if all the order to be executed is done with
      if (total_size_left_ <= 0) {
        break;
      }

      if ((int)order_index == my_order_index_) {
        if (my_order_index_ == 0) {
          my_order_prorata_fill_ = ICEOrderSizeMatch(total_resting_order_bid_side_, 0, sim_order->size_remaining_,
                                                     size_to_execute_, base_power);
        } else {
          my_order_prorata_fill_ = ICEOrderSizeMatch(total_resting_order_bid_side_, sim_order->queue_size_ahead_,
                                                     sim_order->size_remaining_, size_to_execute_, base_power);
        }
      }

      // If the order is first in the queue then give different execution
      if (order_index == 0) {
        size_to_fill_ = ICEOrderSizeMatch(total_resting_order_bid_side_, 0, bid_mkt_vec_[order_index]->size_,
                                          size_to_execute_, base_power);
      } else {
        size_to_fill_ = ICEOrderSizeMatch(total_resting_order_bid_side_, sum_size_, bid_mkt_vec_[order_index]->size_,
                                          size_to_execute_, base_power);
      }

      // maintaining the counter
      sum_size_ += bid_mkt_vec_[order_index]->size_;
      total_size_filled_ += size_to_fill_;
      total_size_left_ = total_size_left_ - size_to_fill_;
      mkt_order_sizes_filled_[order_index] = size_to_fill_;

      // check if my order got prorata fill or not
      //      if (my_order_index_ == 0) {
      //        my_order_prorata_fill_ = ICEOrderSizeMatch(total_resting_order_bid_side_, 0, sim_order->size_remaining_,
      //                                                   size_to_execute_, base_power);
      //      } else if (my_order_index_ != -1) {
      //        my_order_prorata_fill_ = ICEOrderSizeMatch(total_resting_order_bid_side_, sim_order->queue_size_ahead_,
      //                                                   sim_order->size_remaining_, size_to_execute_, base_power);
      //      }
    }

  } else if (sim_order->buysell_ == kTradeTypeSell) {
    // By default the fill to give for each order is 0, hence initializing the array with 0
    mkt_order_sizes_filled_.resize(ask_mkt_vec_.size(), 0);

    for (auto order_index = 0u; order_index < ask_mkt_vec_.size(); order_index++) {
      // mkt_order_sizes_filled_.push_back(bid_mkt_vec_[order_index]->size_);

      // Get out of the loop if all the roder to be executed is done with

      if (total_size_left_ <= 0) {
        break;
      }

      if ((int)order_index == my_order_index_) {
        if (my_order_index_ == 0) {
          my_order_prorata_fill_ = ICEOrderSizeMatch(total_resting_order_ask_side_, 0, sim_order->size_remaining_,
                                                     size_to_execute_, base_power);
        } else {
          my_order_prorata_fill_ = ICEOrderSizeMatch(total_resting_order_ask_side_, sim_order->queue_size_ahead_,
                                                     sim_order->size_remaining_, size_to_execute_, base_power);
        }
      }

      if (order_index == 0) {
        size_to_fill_ = ICEOrderSizeMatch(total_resting_order_ask_side_, 0, ask_mkt_vec_[order_index]->size_,
                                          size_to_execute_, base_power);
      } else {
        size_to_fill_ = ICEOrderSizeMatch(total_resting_order_ask_side_, sum_size_, ask_mkt_vec_[order_index]->size_,
                                          size_to_execute_, base_power);
      }

      sum_size_ += ask_mkt_vec_[order_index]->size_;
      mkt_order_sizes_filled_[order_index] = size_to_fill_;
      total_size_filled_ += size_to_fill_;
      total_size_left_ = total_size_left_ - size_to_fill_;

      // check if the orders are left to be executed if yes , then give FIFO fills

      // maintaining the counter
      // update the mkt_order_sizes_filled_ vec with fill to give for particular order in the vec
      //      mkt_order_sizes_filled_[order_index] = size_to_fill_;
      //      if (my_order_index_ == 0) {
      //        my_order_prorata_fill_ = ICEOrderSizeMatch(total_resting_order_ask_side_, 0, sim_order->size_remaining_,
      //                                                   size_to_execute_, base_power);
      //      } else if (my_order_index_ != -1) {
      //        my_order_prorata_fill_ = ICEOrderSizeMatch(total_resting_order_ask_side_, sim_order->queue_size_ahead_,
      //                                                   sim_order->size_remaining_, size_to_execute_, base_power);
      //      }
    }  // end of for loop
  }    // end of if condition

  // STEP 2
  // FIFO FILLS

  // check if the orders are left to be executed if yes , then give FIFO fills. total_size_left>0 hence more order to be
  // executed hence give FIFO fills
  if (total_size_left_ > 0) {
    if (sim_order->buysell_ == kTradeTypeBuy) {
      for (auto order_index = 0u; order_index < bid_mkt_vec_.size(); order_index++) {
        // check if there is a potential to fill the order at this order index
        if (total_size_left_ <= 0) {
          break;
        }
        // check if there is potential to give the order more fill apart from the prorata fill. If yes then proceed with
        // giving FIFO fill
        if ((bid_mkt_vec_[order_index]->size_ - mkt_order_sizes_filled_[order_index] > 0) && total_size_left_ > 0) {
          // check if the present order consumes all the fifo fill to be allocated
          if (total_size_left_ < (bid_mkt_vec_[order_index]->size_ - mkt_order_sizes_filled_[order_index])) {
            // Check if my order got fifo fill or not if yes then assign it the fill
            if ((int)order_index == my_order_index_) {
              my_order_fifo_fill_ = fmin(total_size_left_, sim_order->size_remaining_ - my_order_prorata_fill_);
              total_size_left_ = total_size_left_ - my_order_fifo_fill_;
            } else {
              mkt_order_sizes_filled_[order_index] += total_size_left_;
              total_size_left_ = 0;
            }
          }

          // check if the present order cannot consume all the fifo fill to be allocated
          else if (total_size_left_ >= (bid_mkt_vec_[order_index]->size_ - mkt_order_sizes_filled_[order_index])) {
            if ((int)order_index == my_order_index_) {
              my_order_fifo_fill_ = fmin(total_size_left_, sim_order->size_remaining_ - my_order_prorata_fill_);
              total_size_left_ = total_size_left_ - my_order_fifo_fill_;
            } else {
              mkt_order_sizes_filled_[order_index] = bid_mkt_vec_[order_index]->size_;
              total_size_left_ =
                  total_size_left_ - (bid_mkt_vec_[order_index]->size_ - mkt_order_sizes_filled_[order_index]);
            }
          }
        }
      }
    } else if (sim_order->buysell_ == kTradeTypeSell) {
      for (auto order_index = 0u; order_index < ask_mkt_vec_.size(); order_index++) {
        // check if there is a potential to fill the order at this order index
        if (total_size_left_ <= 0) {
          break;
        }
        // check if there is potential to give the order more fill apart from the prorata fill. If yes then proceed with
        // giving FIFO fill
        if ((ask_mkt_vec_[order_index]->size_ - mkt_order_sizes_filled_[order_index] > 0) && total_size_left_ > 0) {
          // check if the present order consumes all the fifo fill to be allocated
          if (total_size_left_ < (ask_mkt_vec_[order_index]->size_ - mkt_order_sizes_filled_[order_index])) {
            // Check if my order got fifo fill or not if yes then assign it the fill
            if ((int)order_index == my_order_index_) {
              my_order_fifo_fill_ = fmin(total_size_left_, sim_order->size_remaining_ - my_order_prorata_fill_);
              total_size_left_ = total_size_left_ - my_order_fifo_fill_;
            } else {
              mkt_order_sizes_filled_[order_index] += total_size_left_;
              total_size_left_ = 0;
            }
          }

          // check if the present order cannot consume all the fifo fill to be allocated
          else if (total_size_left_ >= (ask_mkt_vec_[order_index]->size_ - mkt_order_sizes_filled_[order_index])) {
            if ((int)order_index == my_order_index_) {
              my_order_fifo_fill_ = fmin(total_size_left_, sim_order->size_remaining_ - my_order_prorata_fill_);
              total_size_left_ = total_size_left_ - my_order_fifo_fill_;
            } else {
              mkt_order_sizes_filled_[order_index] = ask_mkt_vec_[order_index]->size_;
              total_size_left_ =
                  total_size_left_ - (ask_mkt_vec_[order_index]->size_ - mkt_order_sizes_filled_[order_index]);
            }
          }
        }
      }
    }
  }
  // execute the order that has to be executed on my end
  int saci = sim_order->server_assigned_client_id();
  int my_size_executed_ = 0;
  int my_order_size_to_execute_ = my_order_prorata_fill_ + my_order_fifo_fill_;

  // executing my oder in SIM
  if (my_order_size_to_execute_ > 0) {
    if (my_order_size_to_execute_ == sim_order->size_remaining()) {
      my_size_executed_ = sim_order->ExecuteRemaining();
      std::cout << "\n";
      std::cout << "Full order size executed of size: " << my_size_executed_ << "\n";
      std::cout << "My pro rata fill is : " << my_order_prorata_fill_ << "\n";
      std::cout << "My fifo fill is : " << my_order_fifo_fill_ << "\n";
      std::cout << "Printing all the relevant information here: "
                << "\n";
      std::cout << "Queue size ahead of me: " << num_order_ahead_ << "\n";
      std::cout << "My queue position: " << my_order_index_ << "\n";
      std::cout << "The order size that I placed: " << sim_order->size_remaining_ << "\n";
      std::cout << "Aggressive trade size: " << size_to_execute_ << "\n";
      std::cout << "The time at the execution: " << watch_.tv() << "\n";
      if (sim_order->buysell_ == kTradeTypeBuy) {
        std::cout << "Total resting order on the bid side: " << total_resting_order_bid_side_;
      } else {
        std::cout << "Total resting order on the ask side: " << total_resting_order_ask_side_;
      }
      std::cout << "\n";
    } else {
      my_size_executed_ = sim_order->MatchPartial(my_order_size_to_execute_);
      std::cout << "\n";
      std::cout << "Partial order size executed of size: " << my_size_executed_ << "\n";
      std::cout << "My pro rata fill is : " << my_order_prorata_fill_ << "\n";
      std::cout << "My fifo fill is : " << my_order_fifo_fill_ << "\n";
      std::cout << "Printing all the relevant information here "
                << "\n";
      std::cout << "Queue size ahead of me: " << num_order_ahead_ << "\n";
      std::cout << "My queue position " << my_order_index_ << "\n";
      std::cout << "The order size that I placed " << sim_order->size_remaining_ << "\n";
      std::cout << "Aggressive trade size " << size_to_execute_ << "\n";
      std::cout << "The time at the execution " << watch_.tv() << "\n";
      if (sim_order->buysell_ == kTradeTypeBuy) {
        std::cout << "Total resting order on the bid side: " << total_resting_order_bid_side_;
      } else {
        std::cout << "Total resting order on the ask side: " << total_resting_order_ask_side_;
      }
      std::cout << "\n";
    }
  }
  // updating the position map with my new position
  if (my_size_executed_ > 0) {
    if (sim_order->buysell_ == kTradeTypeBuy) {
      client_position_map_[saci] += my_size_executed_;
      global_position_to_send_map_[saci] += my_size_executed_;
    } else if (sim_order->buysell_ == kTradeTypeSell) {
      client_position_map_[saci] -= my_size_executed_;
      global_position_to_send_map_[saci] -= my_size_executed_;
    }
    saci_to_executed_size_[saci] += my_size_executed_;
    LogExec(sim_order, my_size_executed_);
    BroadcastExecNotification(sim_order->server_assigned_client_id_, sim_order);
  }
  LogOrderStatus(sim_order, "ICEProrata");
}

void OrderLevelSim::PosExeProRataResidualFIFO(BaseSimOrder* sim_order, int size) {
  int int_price = sim_order->int_price_;

  int top_order_size_ = 0;
  std::vector<ExchMarketOrder*>& bid_mkt_vec_ = mov_->GetBidOrders(int_price);
  std::vector<ExchMarketOrder*>& ask_mkt_vec_ = mov_->GetAskOrders(int_price);
  unsigned int vector_size = 0;
  // fill the vector of order sizes here.
  int my_order_index_ = -1;
  int qa_ = sim_order->queue_size_ahead_;
  int sum_size_ = 0;
  std::vector<int> mkt_order_sizes_;

  if (sim_order->buysell_ == kTradeTypeBuy) {
    vector_size = bid_mkt_vec_.size();
    top_order_size_ = mov_->GetTopOrderSizeBidLevels(int_price);
    for (unsigned int order_pointer_ = 0; order_pointer_ < vector_size; order_pointer_++) {
      mkt_order_sizes_.push_back(bid_mkt_vec_[order_pointer_]->size_);
      if (sum_size_ == qa_) {
        my_order_index_ = order_pointer_;
      }
      sum_size_ += bid_mkt_vec_[order_pointer_]->size_;
    }
  } else if (sim_order->buysell_ == kTradeTypeSell) {  // Ask what to do on NoInfo
    vector_size = ask_mkt_vec_.size();
    top_order_size_ = mov_->GetTopOrderSizeAskLevels(int_price);
    for (unsigned int order_pointer_ = 0; order_pointer_ < vector_size; order_pointer_++) {
      mkt_order_sizes_.push_back(ask_mkt_vec_[order_pointer_]->size_);
      if (sum_size_ == qa_) {
        my_order_index_ = order_pointer_;
      }
      sum_size_ += ask_mkt_vec_[order_pointer_]->size_;
    }
  }

  // vector for the fills various orders are getting in market
  std::vector<int> order_filled_sizes_;
  order_filled_sizes_.resize(mkt_order_sizes_.size(), 0);

  int my_order_filled_size = 0;

  int total_size_filled_ = 0;
  int start_index_ = 0;

  if (top_order_size_ >= size && top_order_size_ > 0) {
    // all the fills must be given as the top order fills
    order_filled_sizes_[0] = size;

  } else {
    if (size > top_order_size_ && top_order_size_ > 0) {
      // give some of the fills as top order fills and remaining as prorata
      // Step1 : fill the top order completely
      order_filled_sizes_[0] = top_order_size_;
      total_size_filled_ += top_order_size_;
      start_index_ = 1;

      if (sim_order->buysell_ == kTradeTypeBuy) {
        mov_->RemoveBidTopOrder(int_price);
      } else if (sim_order->buysell_ == kTradeTypeSell) {  // Ask what to do on NoInfo
        mov_->RemoveAskTopOrder(int_price);
      }
    }

    // Step2 : Fill prorata such that prorata size fills >= 2
    int size_remaining_to_fill_ = size - total_size_filled_;

    int total_size_remaining_ = sum_size_ - top_order_size_;
    for (unsigned int order_pointer_ = start_index_; order_pointer_ < vector_size; order_pointer_++) {
      // Compute the pro-rated executed size
      double size_to_fill_ =
          (double)((size_remaining_to_fill_ / ((float)total_size_remaining_)) * mkt_order_sizes_[order_pointer_]);
      if (size_to_fill_ >= 2) {
        order_filled_sizes_[order_pointer_] += (int)size_to_fill_;
        total_size_filled_ += size_to_fill_;
      }
    }

    // Get the pro-rated fill size for my order
    int pro_rata_fill_size = ((size_remaining_to_fill_ * sim_order->size_remaining()) / (double)total_size_remaining_);
    if (pro_rata_fill_size >= 2) {
      my_order_filled_size += pro_rata_fill_size;
    }

    // step3 : Fill fifo in the end for remaining size
    size_remaining_to_fill_ = size - total_size_filled_;

    for (unsigned int order_pointer_ = 1; order_pointer_ < vector_size; order_pointer_++) {
      /*
       * if (order_filled_sizes_[order_pointer_] > 0) {
       * // If we had given fill to this order in earlier iterations then exclude this from fills in further iterations
       * continue;
       * }
       * */

      int size_to_fill_ = mkt_order_sizes_[order_pointer_] - order_filled_sizes_[order_pointer_];

      if (size_remaining_to_fill_ <= 0) {
        break;
      } else if (size_remaining_to_fill_ >= size_to_fill_) {
        order_filled_sizes_[order_pointer_] += size_to_fill_;
        size_remaining_to_fill_ -= size_to_fill_;
      } else if (size_remaining_to_fill_ < size_to_fill_) {
        order_filled_sizes_[order_pointer_] += size_remaining_to_fill_;
        size_remaining_to_fill_ = 0;
      }

      // For now assuming that my order would be right behind this order
      // passive fill assumption

      if ((int)order_pointer_ == my_order_index_) {
        // My order is at same index as this order, the size filled should be same-residual
        int my_size_to_fill = sim_order->size_remaining() - my_order_filled_size;
        // Compute our size to be filled
        if (size_remaining_to_fill_ >= my_size_to_fill) {
          my_order_filled_size += my_size_to_fill;
        } else {
          my_order_filled_size += size_remaining_to_fill_;
        }
        // No need to compute anything further as we are only concerned about our own order fills
        break;
      }
    }
  }

  if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " Printing Fills: \n";
    for (auto id = 0u; id < mkt_order_sizes_.size(); id++) {
      if (order_filled_sizes_[id] > 0) {
        // Print only for orders which have filled size > 0
        dbglogger_ << " [ " << id << " " << mkt_order_sizes_[id] << " " << order_filled_sizes_[id] << " ] ";
      }
    }
    dbglogger_ << "\n[MY_ORDER " << my_order_index_ << " " << my_order_filled_size << " " << sim_order->ToString()
               << " ]\n";
    dbglogger_ << DBGLOG_ENDL_FLUSH;
  }

  // execute the size that has to be executed
  int saci = sim_order->server_assigned_client_id();
  int my_order_size_to_execute_ = my_order_filled_size;

  int size_executed = 0;
  if (my_order_size_to_execute_ == sim_order->size_remaining()) {
    size_executed = sim_order->ExecuteRemaining();
  } else {
    size_executed = sim_order->MatchPartial(my_order_size_to_execute_);
  }

  if (size_executed > 0) {
    if (sim_order->buysell_ == kTradeTypeBuy) {
      client_position_map_[saci] += size_executed;
      global_position_to_send_map_[saci] += size_executed;
    } else if (sim_order->buysell_ == kTradeTypeSell) {
      client_position_map_[saci] -= size_executed;
      global_position_to_send_map_[saci] -= size_executed;
    }
    saci_to_executed_size_[saci] += size_executed;
    LogExec(sim_order, size_executed);
    BroadcastExecNotification(sim_order->server_assigned_client_id_, sim_order);
  }

  LogOrderStatus(sim_order, "PosExeProRataResidualFIFO");
}

/**
 *
 * @param int_price
 * @param position
 * @param size
 */
void OrderLevelSim::UpdateBidsOrderAdd(int int_price, int position, int size) {
  LogUpdate(kTradeTypeBuy, "Add", int_price, position, size);

  // Give fills to the agg ask orders
  FillAggAsks(int_price, size);

  BidPriceSimOrderMapIter_t i2bov_iter = intpx_bid_order_map_.find(int_price);

  // If there isn't any order at this int price, no need to
  // adjust the queue position
  if (i2bov_iter == intpx_bid_order_map_.end() || i2bov_iter->second.empty()) {
    return;
  }

  for (auto sim_order : i2bov_iter->second) {
    PosAdd(sim_order, size, position);
  }
}

/**
 *
 * @param int_price
 * @param position
 * @param size
 */
void OrderLevelSim::UpdateAsksOrderAdd(int int_price, int position, int size) {
  LogUpdate(kTradeTypeSell, "Add", int_price, position, size);

  // Give fills to the agg bid orders
  FillAggBids(int_price, size);

  AskPriceSimOrderMapIter_t i2aov_iter = intpx_ask_order_map_.find(int_price);

  // If there isn't any order at this int price, no need to
  // adjust the queue position
  if (i2aov_iter == intpx_ask_order_map_.end() || i2aov_iter->second.empty()) {
    return;
  }

  for (auto sim_order : i2aov_iter->second) {
    PosAdd(sim_order, size, position);
  }
}

/**
 *
 * @param int_price
 * @param position
 * @param size
 */
void OrderLevelSim::UpdateBidsOrderRemove(int int_price, int position, int size) {
  LogUpdate(kTradeTypeBuy, "Del", int_price, position, size);

  BidPriceSimOrderMapIter_t i2bov_iter = intpx_bid_order_map_.find(int_price);

  // If there isn't any order at this int price, no need to
  // adjust the queue position
  if (i2bov_iter == intpx_bid_order_map_.end() || i2bov_iter->second.empty()) {
    return;
  }

  for (auto sim_order : i2bov_iter->second) {
    PosDel(sim_order, size, position);
  }
}

/**
 *
 * @param int_price
 * @param position
 * @param size
 */
void OrderLevelSim::UpdateAsksOrderRemove(int int_price, int position, int size) {
  LogUpdate(kTradeTypeSell, "Del", int_price, position, size);

  AskPriceSimOrderMapIter_t i2aov_iter = intpx_ask_order_map_.find(int_price);

  // If there isn't any order at this int price, no need to
  // adjust the queue position
  if (i2aov_iter == intpx_ask_order_map_.end() || i2aov_iter->second.empty()) {
    return;
  }

  for (auto sim_order : i2aov_iter->second) {
    PosDel(sim_order, size, position);
  }
}

/**
 *
 * @param int_price_
 * @param size_
 */
void OrderLevelSim::FillAggAsks(int int_price_, int size_) {
  AskPriceSimOrderMapIter_t i2aov_iter_ = intpx_ask_order_map_.begin();

  // Iterate through all the price levels below the given bid int price
  while (i2aov_iter_ != intpx_ask_order_map_.end() && int_price_ >= i2aov_iter_->first) {
    BaseSimOrderPtrVec::iterator order_vec_iter_ = i2aov_iter_->second.begin();

    // Iterate over all sim orders at that price
    while (order_vec_iter_ != i2aov_iter_->second.end()) {
      BaseSimOrder* sim_order_ = *order_vec_iter_;

      int size_available_ = sim_order_->size_remaining();
      int saci_ = sim_order_->server_assigned_client_id();

      // If sim ask int price is less than the given bid int price,
      // then execute the entire order.
      // Otherwise, execute only upto the incoming size.
      if (int_price_ == sim_order_->int_price_) {
        size_available_ = std::max(size_available_, size_ - saci_to_executed_size_[saci_]);
      }

      int size_executed_ = 0;

      if (size_available_ == sim_order_->size_remaining()) {
        size_executed_ = sim_order_->ExecuteRemaining();
      } else {
        size_executed_ = sim_order_->MatchPartial(size_available_);
      }

      // Update client position and global position
      client_position_map_[saci_] -= size_executed_;
      global_position_to_send_map_[saci_] -= size_executed_;

      // Put the executed_size restriction only if the int prices match
      if (int_price_ == sim_order_->int_price_) {
        saci_to_executed_size_[saci_] += size_executed_;
      }

      BroadcastExecNotification(saci_, sim_order_);
      LogExec(sim_order_, size_executed_);

      if (sim_order_->size_remaining() == 0) {
        // Dealloc order and erase from the vector
        basesimorder_mempool_.DeAlloc(sim_order_);
        order_vec_iter_ = i2aov_iter_->second.erase(order_vec_iter_);
      } else {
        order_vec_iter_++;
      }
    }
    i2aov_iter_++;
  }
  VectorUtils::FillInValue(saci_to_executed_size_, 0);
}

/**
 *
 * @param int_price
 * @param position
 * @param size
 * @param agg_order_update whether this udpate is for aggressive order
 */
void OrderLevelSim::UpdateBidsOrderExec(int int_price, int position, int size, bool agg_order_update) {
  LogUpdate(kTradeTypeBuy, "Exe", int_price, position, size);

  if (position != 0) {
    DBGLOG_TIME_CLASS_FUNC << "bid exec at non zero position: " << position << DBGLOG_ENDL_FLUSH;
    // in CME the order can get modified to aggressive price and we can directly receive the execution without modify
    // The trade would get done in next mkt-update ( passive order)
    // if (smv_.exch_source() == kExchSourceCME || smv_.exch_source() == kExchSourceCMEMDP) {
    // return;
    //}
  }

  // Give agg ask fills
  FillAggAsks(int_price, size);

  BidPriceSimOrderMapIter_t i2bov_iter = intpx_bid_order_map_.find(int_price);

  // If there isn't any order at this int price, return
  if (i2bov_iter == intpx_bid_order_map_.end() || i2bov_iter->second.empty()) {
    //	std::cout<<"Int px: "<<int_price<<"\n";
    //	std::cout<<"Cant find the order"<<"\n";
    //	std::cout<<"The order vec at the current int price "<< intpx_bid_order_map_[int_price].size()<<"\n";
    return;
  }

  VectorUtils::FillInValue(saci_to_executed_size_, 0);

  for (auto ord_it = i2bov_iter->second.begin(); ord_it != i2bov_iter->second.end();) {
    BaseSimOrder* sim_order = *ord_it;

    if (current_matching_algo_ == kSimpleProRata) {
      if (agg_order_update) PosExeProRataResidualFIFO(sim_order, size);
    } else if (current_matching_algo_ == kSplitFIFOProRata) {
      if (agg_order_update) SplitFIFOProRata(sim_order, size);
    } else if (current_matching_algo_ == kTimeProRata) {
      if (agg_order_update) ICEProRata(sim_order, size, 2);
    } else if (current_matching_algo_ == kNewTimeProRata) {
      if (agg_order_update) ICEProRata(sim_order, size, 4);
    } else {
      if (!agg_order_update) PosExe(sim_order, size, position);
    }

    if (sim_order->size_remaining() == 0) {
      // Dealloc order and erase from the vector
      basesimorder_mempool_.DeAlloc(sim_order);
      ord_it = i2bov_iter->second.erase(ord_it);
    } else {
      ord_it++;
    }
  }
  VectorUtils::FillInValue(saci_to_executed_size_, 0);
}

void OrderLevelSim::FillAggBids(int int_price_, int size_) {
  BidPriceSimOrderMapIter_t i2bov_iter_ = intpx_bid_order_map_.begin();

  // Iterate through all the price levels above the given ask int price
  while (i2bov_iter_ != intpx_bid_order_map_.end() && int_price_ <= i2bov_iter_->first) {
    BaseSimOrderPtrVec::iterator order_vec_iter_ = i2bov_iter_->second.begin();

    // Iterate over all sim orders at that price
    while (order_vec_iter_ != i2bov_iter_->second.end()) {
      BaseSimOrder* sim_order_ = *order_vec_iter_;

      int size_available_ = sim_order_->size_remaining();
      int saci_ = sim_order_->server_assigned_client_id();

      // If sim bid int price is greater than the given ask int price,
      // then execute the entire order.
      // Otherwise, execute only upto the incoming size.
      if (int_price_ == sim_order_->int_price_) {
        size_available_ = std::max(size_available_, size_ - saci_to_executed_size_[saci_]);
      }

      int size_executed_ = 0;

      if (size_available_ == sim_order_->size_remaining()) {
        size_executed_ = sim_order_->ExecuteRemaining();
      } else {
        size_executed_ = sim_order_->MatchPartial(size_available_);
      }

      // Update client position and global position
      client_position_map_[saci_] += size_executed_;
      global_position_to_send_map_[saci_] += size_executed_;

      // Put the executed_size restriction only if the int prices match
      if (int_price_ == sim_order_->int_price_) {
        saci_to_executed_size_[saci_] += size_executed_;
      }

      BroadcastExecNotification(saci_, sim_order_);
      LogExec(sim_order_, size_executed_);

      if (sim_order_->size_remaining() == 0) {
        // Dealloc order and erase from the vector
        basesimorder_mempool_.DeAlloc(sim_order_);
        order_vec_iter_ = i2bov_iter_->second.erase(order_vec_iter_);
      } else {
        order_vec_iter_++;
      }
    }
    i2bov_iter_++;
  }
  VectorUtils::FillInValue(saci_to_executed_size_, 0);
}

/**
 *
 * @param int_price_
 * @param position_
 * @param size_
 * @param agg_order_update
 */
void OrderLevelSim::UpdateAsksOrderExec(int int_price_, int position_, int size_, bool agg_order_update) {
  LogUpdate(kTradeTypeSell, "Exe", int_price_, position_, size_);

  if (position_ != 0) {
    DBGLOG_TIME_CLASS_FUNC << "ask exec at non zero position: " << position_ << DBGLOG_ENDL_FLUSH;
    // Returning here,
    // in CME the order can get modified to aggressive price and we can directly receive the execution without modify
    // The trade would get done in next mkt-update ( passive order)
    // if (smv_.exch_source() == kExchSourceCME || smv_.exch_source() == kExchSourceCMEMDP) {
    // return;
    //}
  }

  // Give agg. fills if required
  FillAggBids(int_price_, size_);

  AskPriceSimOrderMapIter_t i2aov_iter_ = intpx_ask_order_map_.find(int_price_);

  // If there isn't any order at this int price, return
  if (i2aov_iter_ == intpx_ask_order_map_.end() || i2aov_iter_->second.empty()) {
    //  std::cout<<"Int px: "<<int_price_<<"\n";
    //	std::cout<<"Cant find the order"<<"\n";
    //	std::cout<<"The order vec at the current int price "<< intpx_ask_order_map_[int_price_].size()<<"\n";
    return;
  }

  VectorUtils::FillInValue(saci_to_executed_size_, 0);

  for (auto ord_it_ = i2aov_iter_->second.begin(); ord_it_ != i2aov_iter_->second.end();) {
    BaseSimOrder* sim_order_ = *ord_it_;
    if (current_matching_algo_ == kSimpleProRata) {
      if (agg_order_update) PosExeProRataResidualFIFO(sim_order_, size_);
    } else if (current_matching_algo_ == kSplitFIFOProRata) {
      if (agg_order_update) SplitFIFOProRata(sim_order_, size_);
    } else if (current_matching_algo_ == kTimeProRata) {
      if (agg_order_update) ICEProRata(sim_order_, size_, 2);
    } else if (current_matching_algo_ == kNewTimeProRata) {
      if (agg_order_update) ICEProRata(sim_order_, size_, 4);
    } else {
      if (!agg_order_update) PosExe(sim_order_, size_, position_);
    }

    if (sim_order_->size_remaining() == 0) {
      // Dealloc order and erase from the vector
      basesimorder_mempool_.DeAlloc(sim_order_);
      ord_it_ = i2aov_iter_->second.erase(ord_it_);
    } else {
      ord_it_++;
    }
  }
  VectorUtils::FillInValue(saci_to_executed_size_, 0);
}

void OrderLevelSim::AggBuyTrade(int int_price_, int size_) {
  LogUpdate(kTradeTypeSell, "Trd", int_price_, 0, size_);

  // Give agg. fills if required
  FillAggAsks(int_price_, size_);

  if (current_matching_algo_ == kTimeProRata || current_matching_algo_ == kNewTimeProRata) {
    // create a new sim_order object
    ProRataAskExec(int_price_, size_);
  }
}

void OrderLevelSim::AggSellTrade(int int_price_, int size_) {
  LogUpdate(kTradeTypeBuy, "Trd", int_price_, 0, size_);

  // Give agg. fills if required
  FillAggBids(int_price_, size_);

  if (current_matching_algo_ == kTimeProRata || current_matching_algo_ == kNewTimeProRata) {
    ProRataBidExec(int_price_, size_);
  }
}

void OrderLevelSim::ProRataBidExec(int int_price_, int size_) {
  if (mov_->GetBidSize(int_price_) <= 0) {
    return;
  }

  std::vector<ExchMarketOrder*>& mkt_vec_ = mov_->GetBidOrders(int_price_);

  ProRataExec(int_price_, size_, mkt_vec_);
}

void OrderLevelSim::ProRataAskExec(int int_price_, int size_) {
  if (mov_->GetAskSize(int_price_) <= 0) {
    return;
  }

  std::vector<ExchMarketOrder*>& mkt_vec_ = mov_->GetAskOrders(int_price_);

  ProRataExec(int_price_, size_, mkt_vec_);
}

void OrderLevelSim::ProRataExec(int int_price_, int size_, std::vector<ExchMarketOrder*>& mkt_vec_) {
  std::vector<int> ord_sz_(mkt_vec_.size(), 0);
  std::vector<int> sz_sum_(mkt_vec_.size(), 0);
  std::vector<int> fill_(mkt_vec_.size(), 0);
  std::vector<std::pair<double, int> > fill_ratio_(mkt_vec_.size());

  int power_ = 2;
  if (current_matching_algo_ == kNewTimeProRata) {
    power_ = 4;
  }

  int total_size_ = 0;

  for (size_t i = 0; i < mkt_vec_.size(); i++) {
    ord_sz_[i] = mkt_vec_[i]->size_;
    total_size_ += ord_sz_[i];
    sz_sum_[i] = total_size_;
  }

  int size_remaining_ = size_;
  // First pass

  for (size_t i = 0; i < mkt_vec_.size(); i++) {
    int first_term_ = total_size_ - sz_sum_[i] + ord_sz_[i];
    int second_term_ = total_size_ - sz_sum_[i];

    double time_prorata_diff_ = pow(first_term_, power_) - pow(second_term_, power_);

    double denom_ = pow(total_size_, power_);

    fill_ratio_[i] = std::make_pair(size_remaining_ * time_prorata_diff_ / denom_, i);

    fill_[i] = std::min(ord_sz_[i], (int)fill_ratio_[i].first);

    DBGLOG_TIME_CLASS_FUNC << "OrderId: " << mkt_vec_[i]->order_id_ << " ratio: " << fill_ratio_[i].first
                           << " ord_sz: " << ord_sz_[i] << " fill: " << fill_[i]
                           << " priority: " << mkt_vec_[i]->priority_ << DBGLOG_ENDL_FLUSH;
  }

  // Update the size remaining
  for (size_t i = 0; i < mkt_vec_.size(); i++) {
    size_remaining_ -= fill_[i];
  }

  // Subsequent passes
  while (size_remaining_ > 0) {
    DBGLOG_TIME_CLASS_FUNC << "SizeRemaining: " << size_remaining_ << DBGLOG_ENDL_FLUSH;

    fill_ratio_.resize(mkt_vec_.size());
    total_size_ = 0;
    for (size_t i = 0; i < mkt_vec_.size(); i++) {
      ord_sz_[i] = mkt_vec_[i]->size_ - fill_[i];
      total_size_ += ord_sz_[i];
      sz_sum_[i] = total_size_;
    }

    for (size_t i = 0; i < mkt_vec_.size(); i++) {
      if (ord_sz_[i] == 0) {
        continue;
      }

      int first_term_ = total_size_ - sz_sum_[i] + ord_sz_[i];
      int second_term_ = total_size_ - sz_sum_[i];

      double time_prorata_diff_ = pow(first_term_, power_) - pow(second_term_, power_);

      double denom_ = pow(total_size_, power_);

      fill_ratio_[i] = std::make_pair(size_remaining_ * time_prorata_diff_ / denom_, i);

      DBGLOG_TIME_CLASS_FUNC << "OrderId: " << mkt_vec_[i]->order_id_ << " ratio: " << fill_ratio_[i].first
                             << " ord_sz: " << ord_sz_[i] << " priority: " << mkt_vec_[i]->priority_
                             << DBGLOG_ENDL_FLUSH;
    }

    std::make_heap(fill_ratio_.begin(), fill_ratio_.end());

    while (size_remaining_ > 0 && !fill_ratio_.empty()) {
      std::pair<double, int> ratio_pair_ = *fill_ratio_.begin();

      if (ratio_pair_.first >= 1) {
        fill_[ratio_pair_.second] += (int)ratio_pair_.first;
        size_remaining_ -= (int)ratio_pair_.first;
      } else {
        fill_[ratio_pair_.second]++;
        size_remaining_--;
      }

      std::pop_heap(fill_ratio_.begin(), fill_ratio_.end());
      fill_ratio_.pop_back();

      DBGLOG_TIME_CLASS_FUNC << "OrderId: " << mkt_vec_[ratio_pair_.second]->order_id_
                             << " ratio: " << ratio_pair_.first << " ord_sz: " << ord_sz_[ratio_pair_.second]
                             << " fill: " << fill_[ratio_pair_.second] << DBGLOG_ENDL_FLUSH;
    }
  }
}

void OrderLevelSim::BidQueuePos(QueuePositionUpdate pos_update) {
  switch (pos_update.action_) {
    case QueuePositionUpdate::OrderAdd: {
      UpdateBidsOrderAdd(pos_update.int_price_, pos_update.position_, pos_update.size_);
      break;
    }

    case QueuePositionUpdate::OrderRemove: {
      UpdateBidsOrderRemove(pos_update.int_price_, pos_update.position_, pos_update.size_);
      break;
    }

    case QueuePositionUpdate::OrderExec: {
      // Do not proceed with resting order execs
      // if the matching algo isn't FIFO

      UpdateBidsOrderExec(pos_update.int_price_, pos_update.position_, pos_update.size_, pos_update.agg_order_);
      break;
    }
    default:
      break;
  }
}

void OrderLevelSim::AskQueuePos(QueuePositionUpdate pos_update) {
  switch (pos_update.action_) {
    case QueuePositionUpdate::OrderAdd: {
      UpdateAsksOrderAdd(pos_update.int_price_, pos_update.position_, pos_update.size_);
      break;
    }

    case QueuePositionUpdate::OrderRemove: {
      UpdateAsksOrderRemove(pos_update.int_price_, pos_update.position_, pos_update.size_);
      break;
    }

    case QueuePositionUpdate::OrderExec: {
      // Do not proceed with resting order execs
      // if the matching algo isn't FIFO

      UpdateAsksOrderExec(pos_update.int_price_, pos_update.position_, pos_update.size_, pos_update.agg_order_);
      break;
    }
    default:
      break;
  }
}

void OrderLevelSim::QueuePosChange(QueuePositionUpdate position_update) {
  UpdateNumEvents();
  ProcessRequestQueue();
  UpdateAggSizes();

  if (!process_mkt_updates_) return;

  switch (position_update.buysell_) {
    case kTradeTypeBuy: {
      BidQueuePos(position_update);
      break;
    }

    case kTradeTypeSell: {
      AskQueuePos(position_update);
      break;
    }

    default: { break; }
  }
}

void OrderLevelSim::OnTrade(const unsigned int t_security_id_, const double trade_price, const int trade_size,
                            const TradeType_t t_buysell_) {
  UpdateNumEvents();
  ProcessRequestQueue();

  if (!smv_.is_ready()) {
    return;
  }

  TradeType_t buysell_ = t_buysell_;

  int int_price_ = smv_.GetIntPx(trade_price);

  if (buysell_ == kTradeTypeNoInfo) {
    if (int_price_ <= smv_.bid_int_price(0)) {
      buysell_ = kTradeTypeSell;
    } else if (int_price_ >= smv_.ask_int_price(0)) {
      buysell_ = kTradeTypeBuy;
    }
  }

  switch (buysell_) {
    case kTradeTypeBuy: {
      AggBuyTrade(smv_.GetIntPx(trade_price), trade_size);
      break;
    }

    case kTradeTypeSell: {
      AggSellTrade(smv_.GetIntPx(trade_price), trade_size);
      break;
    }

    default: { break; }
  }
}

void OrderLevelSim::UpdateNumEvents() {
  for (auto& order_vec : intpx_bid_order_map_) {
    for (auto& order : order_vec.second) {
      // Do not update if the event corresponds to this order's wakeup
      if (order->order_confirmation_time_ != watch_.tv()) {
        order->num_events_seen_++;
      }
    }
  }

  for (auto& order_vec : intpx_ask_order_map_) {
    for (auto& order : order_vec.second) {
      // Do not update if the event corresponds to this order's wakeup
      if (order->order_confirmation_time_ != watch_.tv()) {
        order->num_events_seen_++;
      }
    }
  }
}

int OrderLevelSim::ProcessBidSendRequest(BaseSimOrder* sim_order) {
  int saci = sim_order->server_assigned_client_id();
  double prev_conf_price = sim_order->price();
  const int int_price = sim_order->int_price_;
  sim_order->queue_size_behind_ = 0;
  sim_order->queue_size_ahead_ = mov_->GetBidSize(int_price);
  sim_order->order_id_ = 0;

  // Check if this is an aggressive buy order
  if (int_price >= mov_->GetBestAskIntPrice()) {
    for (int this_int_price = mov_->GetBestAskIntPrice(); this_int_price <= int_price; this_int_price++) {
      sim_order->price_ = smv_.GetDoublePx(this_int_price);

      // Execute the order, update position, send EXEC
      int ask_size = mov_->GetAskSize(this_int_price);

      if (intpx_agg_bid_size_map_[saci].find(this_int_price) != intpx_agg_bid_size_map_[saci].end()) {
        ask_size = ask_size - intpx_agg_bid_size_map_[saci][this_int_price].second;
      }

      // If we have already used this price level for some other aggressive order, skip this level
      if (ask_size <= 0) {
        continue;
      }

      int size_executed = std::min(sim_order->size_remaining_, ask_size);

      if (size_executed == sim_order->size_remaining_) {
        // Full exec
        sim_order->ExecuteRemaining();
      } else {
        // Partial match and append sim order to the order_vec_
        size_executed = sim_order->MatchPartial(size_executed);
      }

      if (size_executed > 0) {
        LogExec(sim_order, size_executed);
        client_position_map_[saci] += size_executed;
        global_position_to_send_map_[saci] += size_executed;
        intpx_agg_bid_size_map_[saci][this_int_price].first = mov_->GetAskSize(this_int_price);
        intpx_agg_bid_size_map_[saci][this_int_price].second += size_executed;
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
  intpx_bid_order_map_[int_price].push_back(sim_order);

  if (int_price > mov_->GetBestBidIntPrice()) {
    sim_order->alone_above_best_market_ = true;
  }

  return 0;
}

int OrderLevelSim::ProcessAskSendRequest(BaseSimOrder* sim_order) {
  int saci = sim_order->server_assigned_client_id();
  double prev_conf_price = sim_order->price();
  const int int_price = sim_order->int_price_;
  sim_order->queue_size_behind_ = 0;
  sim_order->queue_size_ahead_ = mov_->GetAskSize(int_price);
  sim_order->order_id_ = 0;

  // Check if this is an aggressive sell order
  if (int_price <= mov_->GetBestBidIntPrice()) {
    for (int this_int_price = mov_->GetBestBidIntPrice(); this_int_price >= int_price; this_int_price--) {
      sim_order->price_ = smv_.GetDoublePx(this_int_price);

      // Execute the order, update position, send EXEC
      int bid_size = mov_->GetBidSize(this_int_price);

      if (intpx_agg_ask_size_map_[saci].find(this_int_price) != intpx_agg_ask_size_map_[saci].end()) {
        bid_size = bid_size - intpx_agg_ask_size_map_[saci][this_int_price].second;
      }

      // If we have already used this price level for some other aggressive order, skip this level
      if (bid_size <= 0) {
        continue;
      }

      int size_executed = std::min(sim_order->size_remaining_, bid_size);

      if (size_executed == sim_order->size_remaining_) {
        sim_order->ExecuteRemaining();
      } else {
        size_executed = sim_order->MatchPartial(size_executed);
      }

      if (size_executed > 0) {
        LogExec(sim_order, size_executed);
        client_position_map_[saci] -= size_executed;
        global_position_to_send_map_[saci] -= size_executed;
        intpx_agg_ask_size_map_[saci][this_int_price].first = mov_->GetBidSize(this_int_price);
        intpx_agg_ask_size_map_[saci][this_int_price].second += size_executed;
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
  intpx_ask_order_map_[int_price].push_back(sim_order);

  if (int_price < mov_->GetBestAskIntPrice()) {
    sim_order->alone_above_best_market_ = true;
  }
  return 0;
}

}  // HFSAT
