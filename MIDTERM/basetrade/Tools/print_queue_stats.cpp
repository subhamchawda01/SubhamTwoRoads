/**
   \file basetrade/Tools/print_queue_stats.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   India
   +91 80 4190 3551
*/

#include "basetrade/Tools/print_queue_stats.hpp"

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22141225

class TestBook : public HFSAT::Thread,
                 public HFSAT::SecurityMarketViewChangeListener,
                 public HFSAT::OrderSequencedListener,
                 public HFSAT::OrderConfirmedListener,
                 public HFSAT::OrderCxlSeqdListener,
                 public HFSAT::OrderCanceledListener,
                 public HFSAT::OrderExecutedListener,
                 public HFSAT::OrderConfCxlReplacedListener,
                 public HFSAT::OrderConfCxlReplaceRejectListener,
                 public HFSAT::MarketBook {
  //
  HFSAT::MarketOrdersView* mov_;
  HFSAT::SecurityMarketView* smv_;
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Watch& watch_;
  bool day_over_;
  bool order_count_;
  bool logging_;
  /// map storing the values from saos to ors/mkt details
  std::map<int, ORSMktStruct*> saos_to_details_;
  std::map<int64_t, std::pair<int, bool> > exch_id_to_saos_and_found_in_mkt_data_map_;
  std::map<int64_t, bool> added_in_market_data_;

 public:
  TestBook(HFSAT::MarketOrdersView* this_mov_, HFSAT::SecurityMarketView* this_smv_, HFSAT::DebugLogger& _dbglogger_,
           HFSAT::Watch& _watch_, bool order_count)
      : MarketBook(_dbglogger_, _watch_),
        mov_(this_mov_),
        smv_(this_smv_),
        dbglogger_(_dbglogger_),
        watch_(_watch_),
        day_over_(false),
        order_count_(order_count),
        logging_(true) {}

  ~TestBook() {
    bool first_printed = false;
    for (auto it = saos_to_details_.begin(); it != saos_to_details_.end(); it++) {
      if (false && !first_printed) {
        std::cout << it->second->Header() << std::endl;
        first_printed = true;
      }
      std::cout << it->second->ToString() << std::endl;
    }
  }

  int GetCurrentQueueSize(HFSAT::TradeType_t buysell, int int_price, bool order_count) {
    if (buysell == HFSAT::kTradeTypeBuy) {
      if (order_count) {
        return smv_->bid_order_at_int_price(int_price);
      } else {
        return smv_->bid_size_at_int_price(int_price);
      }
    } else if (buysell == HFSAT::kTradeTypeSell) {
      if (order_count) {
        return smv_->ask_order_at_int_price(int_price);
      } else {
        return smv_->ask_size_at_int_price(int_price);
      }
    }

    return -1;
  }

  void OrderSequenced(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                      const double _price_, const HFSAT::TradeType_t r_buysell_, const int _size_remaining_,
                      const int _size_executed_, const int _client_position_, const int _global_position_,
                      const int r_int_price_, const int32_t server_assigned_message_sequence,
                      const uint64_t exchange_order_id, const HFSAT::ttime_t time_set_by_server) {
    auto iter = saos_to_details_.find(_server_assigned_order_sequence_);
    if (iter != saos_to_details_.end()) {
      // we found the order, should not be the case
      std::cerr << " Multiple Sequenced SAOS: " << _server_assigned_order_sequence_
                << " details: " << iter->second->ToString() << std::endl;
    } else {
      auto this_struct = new ORSMktStruct();
      this_struct->ors_struct.price_ = _price_;
      this_struct->ors_struct.time_set_by_server_ = time_set_by_server;
      this_struct->ors_struct.client_request_time_ = time_set_by_server;
      this_struct->ors_struct.orr_type_ = HFSAT::kORRType_Seqd;
      this_struct->ors_struct.server_assigned_message_sequence_ = server_assigned_message_sequence;
      this_struct->ors_struct.server_assigned_client_id_ = t_server_assigned_client_id_;
      this_struct->ors_struct.size_remaining_ = _size_remaining_;
      this_struct->ors_struct.buysell_ = r_buysell_;
      this_struct->ors_struct.server_assigned_order_sequence_ = _server_assigned_order_sequence_;
      this_struct->ors_struct.client_assigned_order_sequence_ = _client_assigned_order_sequence_;
      this_struct->ors_struct.size_executed_ = _size_executed_;
      this_struct->ors_struct.client_position_ = _client_position_;
      this_struct->ors_struct.global_position_ = _global_position_;
      this_struct->ors_struct.int_price_ = r_int_price_;
      this_struct->ors_struct.exch_assigned_sequence_ = exchange_order_id;

      this_struct->send_time_vec.push_back(watch_.tv());
      this_struct->data_time_vec.push_back(time_set_by_server);
      this_struct->orr_type_vec.push_back(HFSAT::kORRType_Seqd);
      this_struct->is_live = true;
      this_struct->queue_at_send = GetCurrentQueueSize(r_buysell_, r_int_price_, order_count_);

      saos_to_details_[_server_assigned_order_sequence_] = this_struct;
    }
  }

  void OrderConfirmed(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                      const double _price_, const HFSAT::TradeType_t r_buysell_, const int _size_remaining_,
                      const int _size_executed_, const int _client_position_, const int _global_position_,
                      const int r_int_price_, const int32_t server_assigned_message_sequence,
                      const uint64_t exchange_order_id, const HFSAT::ttime_t time_set_by_server) {
    auto iter = saos_to_details_.find(_server_assigned_order_sequence_);
    if (iter != saos_to_details_.end()) {
      // Update the conf related parameters

      int position = GetPositionForOrderId(r_int_price_, r_buysell_, exchange_order_id);
      if (position > -1) {
        iter->second->queue_at_conf = position;
        exch_id_to_saos_and_found_in_mkt_data_map_[exchange_order_id] =
            std::make_pair(_server_assigned_order_sequence_, true);
      } else {
        // In case we couldn't find the queue pos in orderfeed, assign the smv value
        iter->second->queue_at_conf = GetCurrentQueueSize(r_buysell_, r_int_price_, order_count_);
        exch_id_to_saos_and_found_in_mkt_data_map_[exchange_order_id] =
            std::make_pair(_server_assigned_order_sequence_, false);
      }

      iter->second->ors_struct.exch_assigned_sequence_ = exchange_order_id;

      iter->second->send_time_vec.push_back(watch_.tv());
      iter->second->data_time_vec.push_back(time_set_by_server);
      iter->second->orr_type_vec.push_back(HFSAT::kORRType_Conf);

    } else {
      std::cerr << " No details found for confirmed order with saos: " << _server_assigned_order_sequence_ << std::endl;
    }
  }

  void OrderORSConfirmed(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                         const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                         const double _price_, const HFSAT::TradeType_t r_buysell_, const int _size_remaining_,
                         const int _size_executed_, const int r_int_price_,
                         const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                         const HFSAT::ttime_t time_set_by_server) {}

  void OrderConfCxlReplaceRejected(const int server_assigned_client_id, const int client_assigned_order_sequence,
                                   const int server_assigned_order_sequence, const unsigned int security_id,
                                   const double price, const HFSAT::TradeType_t buysell, const int size_remaining,
                                   const int client_position, const int global_position, const int intprice,
                                   const int32_t rejection_reason, const int32_t server_assigned_message_sequence,
                                   const uint64_t exchange_order_id, const HFSAT::ttime_t time_set_by_server) {
    auto iter = saos_to_details_.find(server_assigned_order_sequence);
    if (iter != saos_to_details_.end()) {
      // Update the conf related parameters
      iter->second->send_time_vec.push_back(watch_.tv());
      iter->second->data_time_vec.push_back(time_set_by_server);
      iter->second->orr_type_vec.push_back(HFSAT::kORRType_CxReRejc);

      // iter->second->queue_at_= GetCurrentQueueSize(r_buysell_, r_int_price_, order_count_);

    } else {
      std::cerr << " No details found for CxlReplaceRej order with saos: " << server_assigned_order_sequence
                << std::endl;
    }
  }

  void OrderConfCxlReplaced(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                            const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                            const double _price_, const HFSAT::TradeType_t r_buysell_, const int _size_remaining_,
                            const int _size_executed_, const int _client_position_, const int _global_position_,
                            const int r_int_price_, const int32_t server_assigned_message_sequence,
                            const uint64_t exchange_order_id, const HFSAT::ttime_t time_set_by_server) {
    auto iter = saos_to_details_.find(_server_assigned_order_sequence_);
    if (iter != saos_to_details_.end()) {
      // Update the Modify related parameters
      iter->second->send_time_vec.push_back(watch_.tv());
      iter->second->data_time_vec.push_back(time_set_by_server);
      iter->second->orr_type_vec.push_back(HFSAT::kORRType_CxRe);

      iter->second->queue_at_modify = GetCurrentQueueSize(r_buysell_, r_int_price_, order_count_);

    } else {
      std::cerr << " No details found for Modified order with saos: " << _server_assigned_order_sequence_ << std::endl;
    }
  }

  void OrderCxlSequenced(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                         const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                         const double _price_, const HFSAT::TradeType_t r_buysell_, const int _size_remaining_,
                         const int _client_position_, const int _global_position_, const int r_int_price_,
                         const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                         const HFSAT::ttime_t time_set_by_server) {
    auto iter = saos_to_details_.find(_server_assigned_order_sequence_);
    if (iter != saos_to_details_.end()) {
      // Update the cancelSeqd related parameters
      iter->second->send_time_vec.push_back(watch_.tv());
      iter->second->data_time_vec.push_back(time_set_by_server);
      iter->second->orr_type_vec.push_back(HFSAT::kORRType_CxlSeqd);

      auto position = GetPositionForOrderId(iter->second->ors_struct.int_price_, r_buysell_, exchange_order_id);

      auto&& bid_order_vec = GetBidOrders(iter->second->ors_struct.int_price_);
      auto&& ask_order_vec = GetAskOrders(iter->second->ors_struct.int_price_);
      if (r_buysell_ == HFSAT::kTradeTypeBuy && bid_order_vec.size() > 0) {
        // there's order at this level
        iter->second->queue_at_cxl_seq = bid_order_vec.size();
        iter->second->position_at_cxl_seq = position;
      } else if (r_buysell_ == HFSAT::kTradeTypeSell && ask_order_vec.size() > 0) {
        iter->second->queue_at_cxl_seq = ask_order_vec.size();
        iter->second->position_at_cxl_seq = position;
      } else {
        // use the price-feed data to get the queue
        iter->second->queue_at_cxl_seq = GetCurrentQueueSize(r_buysell_, r_int_price_, order_count_);
      }

    } else {
      std::cerr << " No details found for CxlSeqd order with saos: " << _server_assigned_order_sequence_ << std::endl;
    }
  }

  void OrderCanceled(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const HFSAT::TradeType_t r_buysell_, const int _size_remaining_, const int _client_position_,
                     const int _global_position_, const int r_int_price_,
                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const HFSAT::ttime_t time_set_by_server) {
    auto iter = saos_to_details_.find(_server_assigned_order_sequence_);
    if (iter != saos_to_details_.end()) {
      // Update the canceled related parameters
      iter->second->send_time_vec.push_back(watch_.tv());
      iter->second->data_time_vec.push_back(time_set_by_server);
      iter->second->orr_type_vec.push_back(HFSAT::kORRType_Cxld);
      iter->second->is_live = false;

      iter->second->queue_at_cxld = GetCurrentQueueSize(r_buysell_, r_int_price_, order_count_);

    } else {
      std::cerr << " No details found for canceled order with saos: " << _server_assigned_order_sequence_ << std::endl;
    }
  }

  void OrderCancelRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                           const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                           const double _price_, const HFSAT::TradeType_t t_buysell_, const int _size_remaining_,
                           const int _rejection_reason_, const int t_client_position_, const int t_global_position_,
                           const int r_int_price_, const uint64_t exchange_order_id,
                           const HFSAT::ttime_t time_set_by_server) {
    auto iter = saos_to_details_.find(_server_assigned_order_sequence_);
    if (iter != saos_to_details_.end()) {
      // Update the conf related parameters
      iter->second->send_time_vec.push_back(watch_.tv());
      iter->second->data_time_vec.push_back(time_set_by_server);
      iter->second->orr_type_vec.push_back(HFSAT::kORRType_CxlRejc);

      iter->second->queue_at_conf = GetCurrentQueueSize(t_buysell_, r_int_price_, order_count_);

    } else {
      std::cerr << " No details found for CancelRejected order with saos: " << _server_assigned_order_sequence_
                << std::endl;
    }
  }

  void OrderExecuted(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const HFSAT::TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_,
                     const int _client_position_, const int _global_position_, const int r_int_price_,
                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const HFSAT::ttime_t time_set_by_server) {
    auto iter = saos_to_details_.find(_server_assigned_order_sequence_);
    if (iter != saos_to_details_.end()) {
      // Update the executed related parameters
      iter->second->send_time_vec.push_back(watch_.tv());
      iter->second->data_time_vec.push_back(time_set_by_server);
      iter->second->orr_type_vec.push_back(HFSAT::kORRType_Exec);

      iter->second->is_live = false;

      if (iter->second->queue_at_exec == -1) {
        iter->second->queue_at_exec = GetCurrentQueueSize(r_buysell_, r_int_price_, order_count_);
      }

    } else {
      std::cerr << " No details found for executed order with saos: " << _server_assigned_order_sequence_ << std::endl;
    }
  }

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {
    /*
     * if (false && mov_->IsReady()) {
      std::cout << watch_.tv() << " OnMarketUpdate " << mov_->GetMarketString() << " "
                << smv_->MarketUpdateInfoToString() << std::endl;
    }
    */
  }

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                           const HFSAT::MarketUpdateInfo& _market_update_info_) {
    /*if (false && mov_->IsReady()) {
      std::cout << watch_.tv() << " OnTradePrint " << mov_->GetMarketString() << " " << _trade_print_info_.ToString()
                << " " << smv_->MarketUpdateInfoToString() << std::endl;
    }*/
  }

  /**
   * Add the bid side orer in market book
   * currently we are just using it to get the current queue stats
   * and then call MarketBook function
   * @param t_order_
   * @param queue_pos
   * @param position_based
   * @param l1_update
   * @return
   */
  bool AddBid(HFSAT::ExchMarketOrder* t_order_, HFSAT::QueuePositionUpdate* queue_pos, bool position_based,
              bool& l1_update) override {
    auto&& iter = exch_id_to_saos_and_found_in_mkt_data_map_.find(t_order_->order_id_);
    if (iter != exch_id_to_saos_and_found_in_mkt_data_map_.end()) {
      auto& saos_details = saos_to_details_[iter->second.first];
      auto&& order_vec = GetBidOrders(t_order_->int_price_);
      // auto index = find(order_vec.begin(), order_vec.end(), t_order_) - order_vec.begin();

      // override by default
      saos_details->queue_at_conf = order_vec.size();
      // if (saos_details->position_at_cxld == -1) saos_details->position_at_cxld = index;
    }

    return MarketBook::AddBid(t_order_, queue_pos, position_based, l1_update);
  }

  /**
   * Add the ask side order in market book
   * curently we are just using to get the current queue stats
   * and then call MarketBook function
   * @param order
   * @param queue_pos
   * @param position_based
   * @param l1_update
   * @return
   */
  bool AddAsk(HFSAT::ExchMarketOrder* order, HFSAT::QueuePositionUpdate* queue_pos, bool position_based,
              bool& l1_update) override {
    auto&& iter = exch_id_to_saos_and_found_in_mkt_data_map_.find(order->order_id_);
    if (iter != exch_id_to_saos_and_found_in_mkt_data_map_.end()) {
      auto& saos_details = saos_to_details_[iter->second.first];
      auto&& order_vec = GetAskOrders(order->int_price_);
      // auto index = find(order_vec.begin(), order_vec.end(), order) - order_vec.begin();

      // override by default
      saos_details->queue_at_conf = order_vec.size();
      // if (saos_details->position_at_cxld == -1) saos_details->position_at_cxld = index;
    }
    return MarketBook::AddAsk(order, queue_pos, position_based, l1_update);
  }

  /**
   * Remove given order from market book.
   * We are currently using this to get the queue pos stats
   * @param t_order_
   * @param queue_pos_
   * @return
   */
  bool RemoveMarketBid(HFSAT::ExchMarketOrder* t_order_, HFSAT::QueuePositionUpdate* queue_pos_) override {
    auto&& iter = exch_id_to_saos_and_found_in_mkt_data_map_.find(t_order_->order_id_);
    if (iter != exch_id_to_saos_and_found_in_mkt_data_map_.end()) {
      auto& saos_details = saos_to_details_[iter->second.first];
      auto&& order_vec = GetBidOrders(t_order_->int_price_);
      auto index = find(order_vec.begin(), order_vec.end(), t_order_) - order_vec.begin();

      if (saos_details->queue_at_cxld == -1) saos_details->queue_at_cxld = order_vec.size();
      if (saos_details->position_at_cxld == -1) saos_details->position_at_cxld = index;
    }

    return MarketBook::RemoveMarketBid(t_order_, queue_pos_);
  }

  /**
   * Remove the ask side order from book
   * Currently we are using this function to collect the queue stats
   * After that we call MarketBook function
   * @param t_order_
   * @param queue_pos_
   * @return
   */
  bool RemoveMarketAsk(HFSAT::ExchMarketOrder* t_order_, HFSAT::QueuePositionUpdate* queue_pos_) override {
    auto&& iter = exch_id_to_saos_and_found_in_mkt_data_map_.find(t_order_->order_id_);
    if (iter != exch_id_to_saos_and_found_in_mkt_data_map_.end()) {
      auto&& saos_details = saos_to_details_[iter->second.first];
      auto&& order_vec = GetAskOrders(t_order_->int_price_);
      auto index = find(order_vec.begin(), order_vec.end(), t_order_) - order_vec.begin();

      if (saos_details->queue_at_cxld == -1) saos_details->queue_at_cxld = order_vec.size();
      if (saos_details->position_at_cxld == -1) saos_details->position_at_cxld = index;
    }

    return MarketBook::RemoveMarketAsk(t_order_, queue_pos_);
  }

  bool IsOrderActive(ORSMktStruct* order) {
    if (order->orr_type_vec.size() > 0) {
      for (auto orr : order->orr_type_vec) {
        if (orr == HFSAT::kORRType_Exec || orr == HFSAT::kORRType_Cxld) return false;
      }
    } else {
      return false;
    }
    return true;
  }

  /**
   *
   * @param t_order
   * @param queue_pos
   */
  void ExecMarketBid(HFSAT::ExchMarketOrder* t_order, HFSAT::QueuePositionUpdate* queue_pos) override {
    if (t_order == nullptr) return;

    auto&& iter = exch_id_to_saos_and_found_in_mkt_data_map_.find(t_order->order_id_);
    if (iter != exch_id_to_saos_and_found_in_mkt_data_map_.end()) {
      auto&& saos_details = saos_to_details_[iter->second.first];
      auto&& order_vec = GetBidOrders(t_order->int_price_);
      //      auto index = find(order_vec.begin(), order_vec.end(), t_order) - order_vec.begin();
      if (saos_details->queue_at_exec == -1) saos_details->queue_at_exec = order_vec.size();
      saos_details->is_live = false;

    } else {
      // This part can increase the run time a lot, can enable it conditionally
      for (auto t_iter = saos_to_details_.begin(); t_iter != saos_to_details_.end(); t_iter++) {
        // find all orders at this price
        if (t_iter->second->ors_struct.buysell_ == HFSAT::kTradeTypeBuy &&
            t_iter->second->ors_struct.int_price_ == t_order->int_price_ && t_iter->second->is_live) {
          // add the size executed to the order
          t_iter->second->size_executed_before += queue_pos->size_;
          if (t_order->size_ == queue_pos->size_) {
            t_iter->second->orders_executed_before++;
          }
          /*
                    std::cout << watch_.tv().ToString() << " Bid " << t_iter->second->size_executed_before << " "
                              << t_order->size_ << " " << queue_pos->size_ << " " <<
             t_iter->second->orders_executed_before << " "
                              << t_order->order_id_ << " " << t_order->int_price_
                              << t_iter->second->ors_struct.exch_assigned_sequence_ << " " << std::endl;*/
        }
      }
    }

    MarketBook::ExecMarketBid(t_order, queue_pos);
  }

  /**
   *
   * @param t_order
   * @param queue_pos
   */
  void ExecMarketAsk(HFSAT::ExchMarketOrder* t_order, HFSAT::QueuePositionUpdate* queue_pos) override {
    if (t_order == nullptr) return;

    auto&& iter = exch_id_to_saos_and_found_in_mkt_data_map_.find(t_order->order_id_);
    if (iter != exch_id_to_saos_and_found_in_mkt_data_map_.end()) {
      auto&& saos_details = saos_to_details_[iter->second.first];
      auto&& order_vec = GetAskOrders(t_order->int_price_);
      //    auto index = find(order_vec.begin(), order_vec.end(), t_order) - order_vec.begin();
      if (saos_details->queue_at_exec == -1) saos_details->queue_at_cxld = order_vec.size();
      saos_details->is_live = false;

    } else {
      // This part can increase the run time a lot, can enable it conditionally
      for (auto t_iter = saos_to_details_.begin(); t_iter != saos_to_details_.end(); t_iter++) {
        // find all orders at this price
        if (t_iter->second->ors_struct.buysell_ == HFSAT::kTradeTypeSell &&
            t_iter->second->ors_struct.int_price_ == t_order->int_price_ && t_iter->second->is_live) {
          // add the size executed to the order
          t_iter->second->size_executed_before += queue_pos->size_;
          if (t_order->size_ == queue_pos->size_) {
            t_iter->second->orders_executed_before++;
          }
          /*
                    std::cout << watch_.tv().ToString() << " Ask " << t_iter->second->size_executed_before << " "
                              << t_order->size_ << " " << queue_pos->size_ << " " <<
             t_iter->second->orders_executed_before << " "
                              << t_order->order_id_ << " " << t_order->int_price_
                              << t_iter->second->ors_struct.exch_assigned_sequence_ << " " << std::endl;
                              */
        }
      }
    }
    MarketBook::ExecMarketAsk(t_order, queue_pos);
  }

  /**
   * returns the position of an order at given price level
   * @param int_price
   * @param buysell
   * @param order_id
   * @return
   */
  int GetPositionForOrderId(int int_price, HFSAT::TradeType_t buysell, int64_t order_id) {
    /*std::cout << mov_->GetBidOrderCount(int_price) << " " << mov_->GetBidSize(int_price) << " | "
              << mov_->GetAskSize(int_price) << " " << mov_->GetAskOrderCount(int_price) << " ]\n";
              */
    if (buysell == HFSAT::kTradeTypeBuy) {
      // Buy side
      auto& ord_vec = mov_->GetBidOrders(int_price);
      for (auto i = 0; i < (int)ord_vec.size(); i++) {
        if (ord_vec[i]->order_id_ == order_id) {
          return i;
        }
      }
    } else if (buysell == HFSAT::kTradeTypeSell) {
      // Sell side
      auto& ord_vec = mov_->GetAskOrders(int_price);
      for (auto i = 0; i < (int)ord_vec.size(); i++) {
        if (ord_vec[i]->order_id_ == order_id) {
          return i;
        }
      }
    }
    return -1;
  }

  void thread_main() {
    while (!day_over_) {
      sleep(1);
    }
  }

  void DayOver() { day_over_ = true; }
};

void PrintUsage() { std::cout << "Usage: EXEC SHORTCODE YYYYMMDD" << std::endl; }

int main(int argc, char** argv) {
  if (argc < 3) {
    PrintUsage();
    exit(-1);
  }

  std::string shortcode_ = argv[1];
  int tradingdate_ = atoi(argv[2]);
  bool order_count = true;

  if (argc > 3) {
    order_count = (atoi(argv[3]) != 0);
  }

  std::vector<std::string> source_shc_vec = {shortcode_};
  auto common_smv_source = new CommonSMVSource(source_shc_vec, tradingdate_);

  common_smv_source->SetSourcesNeedingOrs(source_shc_vec);

  common_smv_source->Initialize(true);

  auto sid = common_smv_source->getSMV()->security_id();

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

  auto&& watch_ = common_smv_source->getWatch();

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "/spare/local/logs/alllogs/print_queue_pos." << shortcode_ << "." << tradingdate_;
  std::string logfilename_ = t_temp_oss_.str();
  auto&& dbglogger_ = common_smv_source->getLogger();
  dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);

  std::vector<HFSAT::MarketOrdersView*> sid_to_mov_ptr_map_ = HFSAT::sid_to_market_orders_view_map();
  //   HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ = HFSAT::sid_to_security_market_view_map();

  TestBook test_book_(common_smv_source->getMOVMap()[sid], common_smv_source->getSMV(), dbglogger_, watch_,
                      order_count);

  HFSAT::ShortcodeORSMessageFilesourceMap& shortcode_ors_data_filesource_map_ =
      HFSAT::ShortcodeORSMessageFilesourceMap::GetUniqueInstance();

  auto ors_message_filesource = shortcode_ors_data_filesource_map_.GetORSMessageFileSource(shortcode_);

  ors_message_filesource->AddOrderSequencedListener(&test_book_);
  ors_message_filesource->AddOrderConfirmedListener(&test_book_);
  ors_message_filesource->AddOrderCxlSeqdListener(&test_book_);
  ors_message_filesource->AddOrderCanceledListener(&test_book_);
  ors_message_filesource->AddOrderExecutedListener(&test_book_);
  ors_message_filesource->AddOrderConfCxlReplacedListener(&test_book_);
  ors_message_filesource->AddOrderConfCxlReplaceRejectListener(&test_book_);

  test_book_.run();

  common_smv_source->getSMV()->subscribe_L2(&test_book_);

  // common_smv_source->getMOVMap()[sid]->SubscribeQueuePosChange(&test_book_);
  common_smv_source->getMOVMap()[sid]->SetMarketBook(&test_book_);
  common_smv_source->Run();
}
