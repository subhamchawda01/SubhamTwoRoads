/**
   \file SimMarketMaker/base_sim_market_maker.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_ORDERROUTING_BASE_SIM_MARKET_MAKER_H
#define BASE_ORDERROUTING_BASE_SIM_MARKET_MAKER_H

#include <algorithm>
#include <deque>
#include <stdexcept>
#include <unordered_map>
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/ors_defines.hpp"
#include "dvccode/CDef/random_channel.hpp"
#include "dvccode/CDef/retail_data_defines.hpp"
#include "dvccode/CommonDataStructures/simple_mempool.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

#include "dvccode/ORSMessages/ors_message_listener.hpp"

#include "baseinfra/OrderRouting/base_order.hpp"
#include "baseinfra/OrderRouting/base_sim_order.hpp"
#include "baseinfra/OrderRouting/market_model.hpp"

#include "dvccode/LiveSources/retail_trade_listener.hpp"
#include "dvccode/LiveSources/retail_trading_listener.hpp"

#include "baseinfra/SimMarketMaker/ors_message_stats.hpp"
#include "baseinfra/SimMarketMaker/ors_trade_message.hpp"
#include "baseinfra/SimMarketMaker/security_delay_stats.hpp"
#include "baseinfra/SimMarketMaker/sim_config.hpp"

#include "baseinfra/LoggedSources/retail_logged_message_filesource.hpp"

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "dvccode/ExternalData/external_data_listener_listener.hpp"

#define DEFAULT_DELAY_SECONDS 0
#define DEFAULT_DELAY_MSEC 100000

namespace HFSAT {

struct SimMarketOrder {
  int64_t order_id_;
  int int_price_;
  double price_;
  int size_;
  ttime_t mkt_time_;
};

struct OrderInfoStruct {
  TradeType_t buysell_;
  int size_;
  int int_price_;
  int server_assigned_order_sequence_;
  ttime_t sequenced_time_;
};

/// BaseSimMarketMaker listens to order-routing messages from ORSMessageLiveSource in live trading to improve fills, and
/// also to maintain global_position_ for the security
class BaseSimMarketMaker : public OrderNotFoundListener,
                           public OrderSequencedListener,
                           public OrderConfirmedListener,
                           public OrderConfCxlReplacedListener,
                           public OrderCanceledListener,
                           public OrderExecutedListener,
                           public OrderRejectedListener,
                           public RetailTradingListener,
                           public OrderCxlSeqdListener,
                           public OrderInternallyMatchedListener,
                           public ExternalDataListenerListener,
                           public SecurityMarketViewStatusListener {
 protected:
  /** \brief package of a send order request, made to include the time after which the request is likely to reach the
   * exchange matching engine */
  struct SimSendOrderRequest {
    BaseSimOrder* p_new_order_;
  };

  struct SimCancelOrderRequest {
    int server_assigned_order_sequence_;
    TradeType_t buysell_;
    int int_price_;
  };

  struct SimCxlReplaceOrderRequest {
    int server_assigned_order_sequence_;
    int old_int_price_;
    int old_size_;
    int new_int_price_;
    int new_size_;
    double new_price_;
    double old_price_;
    TradeType_t buysell_;
  };

  struct SimReplayOrderRequest {
    int client_assigned_order_sequence_;
    TradeType_t buysell_;
    int int_price_;
    int server_assigned_order_sequence_;
  };

  /// Struct holding request from client... only place this is used is in std::deque < SimGenRequest > all_requests_
  struct SimGenRequest {
    ttime_t wakeup_time_;
    ORQType_t orq_request_type_;
    union {
      SimSendOrderRequest ssor_;
      SimCancelOrderRequest scor_;
      SimCxlReplaceOrderRequest sxor_;
      SimReplayOrderRequest sror_;
    } sreq_;
    int server_assigned_client_id_;
    bool postponed_once_;
    int current_secid_;

    bool operator<(const SimGenRequest& r2) const { return (wakeup_time_ < r2.wakeup_time_); }
  };

  enum MatchingAlgo { kFIFO, kSimpleProRata, kTimeProRata, kNewTimeProRata, kSplitFIFOProRata };

 protected:
  DebugLogger& dbglogger_;
  Watch& watch_;
  SecurityMarketView& smv_;
  const std::string dep_shortcode_;
  const unsigned int dep_security_id_;
  const char* dep_symbol_;
  MarketModel market_model_;
  bool process_mkt_updates_; 
  bool is_cancellable_before_confirmation;
  TradingLocation_t dep_trading_location_;

  HFSAT::SimTimeSeriesInfo& sim_time_series_info_;

  SimpleMempool<BaseSimOrder> basesimorder_mempool_;
  std::deque<SimGenRequest> all_requests_;      ///< queue of requests sent by clients
  std::deque<SimGenRequest> pending_requests_;  ///< queue of requests sent by clients when all_requests_busy_
  /// lock on all_requests_. Since while we process all-requests
  /// we could send a rejection or confirmation immediately the
  /// control reaches the client and hence it could come back
  /// and send another request. To avoid data structure and logic problems
  /// all those are added to pending_requests_
  /// and added later
  bool all_requests_busy_;

  int server_assigned_order_sequence_;

  int lower_bound_usecs_;
  int upper_bound_usecs_;
  int normal_usecs_;

  ttime_t median_conf_delay_;
  ttime_t median_mkt_delay_;

  ///< vector of listeners to ORS messages generated by SMM of type kORRType_None
  std::vector<OrderNotFoundListener*> order_not_found_listener_vec_;
  ///< vector of listeners to ORS messages generated by SMM of type kORRType_Seqd
  std::vector<OrderSequencedListener*> order_sequenced_listener_vec_;
  ///< vector of listeners to ORS messages generated by SMM of type kORRType_Conf
  std::vector<OrderConfirmedListener*> order_confirmed_listener_vec_;
  ///< vector of listener to ORS messages generated by SMM of type kORRType_CxRe
  std::vector<OrderConfCxlReplaceRejectListener*> order_conf_cxlreplace_rejected_listener_vec_;
  ///< vector of listeners to ORS messages generated by SMM of type kORRType_CxRe
  std::vector<OrderConfCxlReplacedListener*> order_conf_cxlreplaced_listener_vec_;
  ///< vector of listeners to ORS messages generated by SMM of type kORRType_Cxld
  std::vector<OrderCanceledListener*> order_canceled_listener_vec_;
  ///< vector of listeners to ORS messages generated by SMM of type kORRType_Exec
  std::vector<OrderExecutedListener*> order_executed_listener_vec_;
  ///< vector of listeners to ORS messages generated by SMM of type kORRType_Rejc
  std::vector<OrderRejectedListener*> order_rejected_listener_vec_;
  ///< vector of listeners to ORS messages generated by SMM of type kORRType_IntExec
  // std::vector < OrderInternallyMatchedListener * > order_internally_matched_listener_vec_ ;

  ///< map from unique_client_id_ to position
  std::vector<int> client_position_map_;
  ///< cumulative position of all clients trading this security, currently only read from ORSFileSource messages
  int global_position_;
  ///< map from unique_client_id_ to global_position_ to send to this client
  std::vector<int> global_position_to_send_map_;

  std::vector<std::map<ttime_t, ttime_t>>& sid_to_time_to_seqd_to_conf_times_;
  std::vector<std::map<HFSAT::ttime_t, HFSAT::ttime_t>>& sid_to_time_to_conf_to_market_delay_times_;
  std::vector<std::map<HFSAT::ttime_t, HFSAT::ttime_t>>& sid_to_time_to_cxl_seqd_to_conf_times_;

  std::vector<SimConfigStruct>& sid_to_sim_config_;

  // SACI is key for these maps
  std::map<int, HFSAT::CDef::RetailOffer> retail_offer_map_;

  std::vector<FPOrderExecutedListener*> fporder_executed_listener_vec_;

  std::map<int, std::vector<int>> saos_queue_size_map_;
  MatchingAlgo current_matching_algo_;
  double fifo_matching_fraction_;

  SecurityDelayStats* security_delay_stats_;
  BidPriceSimOrderMap intpx_bid_order_map_;
  AskPriceSimOrderMap intpx_ask_order_map_;

  std::vector<std::map<int, std::pair<int, int>, std::greater<int>>> intpx_agg_bid_size_map_;
  std::vector<std::map<int, std::pair<int, int>, std::less<int>>> intpx_agg_ask_size_map_;

  SimConfigStruct config_;
  std::vector<int> real_saci_map_;  // for sim-real purposes only

  // Used to limit the execs per saci
  std::vector<int> saci_to_executed_size_;

  // ORS Exec
  std::vector<OrderInfoStruct> ors_bid_orders_;
  std::vector<OrderInfoStruct> ors_ask_orders_;

  std::unordered_map<int, int> saos_to_size_exec_real_;
  std::vector<std::vector<int>> saci_to_secid_;
  int current_secid_;
  std::unordered_map<int, ttime_t> saos_to_wakeup_map_;

  BaseSimOrder* FetchOrder(const TradeType_t buysell_, const int int_price_, const int saos_);

  // Log sim order exec event
  void LogExec(BaseSimOrder* sim_order, int size_executed);

  int ProcessSendRequest(SimGenRequest& t_sim_genrequest_);
  int ProcessCancelRequest(SimGenRequest& t_sim_genrequest_);
  int ProcessCxlReplaceRequest(SimGenRequest& t_sim_genrequest_);
  int ProcessReplayRequest(SimGenRequest& t_sim_genrequest_);

 public:
  static std::string GetSameGatewayShortcodeIfRequired(const std::string& dep_shortcode, int tradingdate);

  BaseSimMarketMaker(DebugLogger& t_dbglogger_, Watch& t_watch_, SecurityMarketView& t_dep_market_view_,
                     MarketModel t_market_model_, HFSAT::SimTimeSeriesInfo& t_sim_time_series_info_);

  virtual ~BaseSimMarketMaker() {}

  void OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_);

  virtual void SimulateRetailTrade(const unsigned int t_security_id_, const double t_trade_price_,
                                   const int t_trade_size_, const TradeType_t t_buysell_);

  virtual void OnRetailOfferUpdate(unsigned int _security_id_, const std::string& _shortcode_,
                                   const std::string& _secname_, const int _server_assigned_client_id_,
                                   const HFSAT::CDef::RetailOffer& _retail_offer_);

  virtual int GetSizeAtIntPrice(TradeType_t buysell, int int_price);

  inline void AssignRealSACI(int _sim_saci_, int _real_saci_) {
    if (int(real_saci_map_.size()) > _sim_saci_) {
      real_saci_map_[_sim_saci_] = _real_saci_;
    }
  }

  inline void SetSecId(int sec_id_) { current_secid_ = sec_id_; }

  inline void AddSecIdToSACI(int _saci_, int _sec_id_) {
    if (int(saci_to_secid_.size()) > _saci_) {
      saci_to_secid_[_saci_].push_back(_sec_id_);
    } else {
      for (int i = saci_to_secid_.size(); i < _saci_; i++) saci_to_secid_.push_back(std::vector<int>());
      saci_to_secid_.push_back(std::vector<int>(1, _sec_id_));
    }
  }

  inline void OnRetailOfferUpdate(const std::string& _secname_, const CDef::RetailOffer& _retail_offer_) {
    // this will be used by only for showing spread quotes in real, not yet implemented in sim
  }

  const ttime_t GetComTimeAtTime(const ttime_t t_time_);
  const ttime_t GetConfMarketDelayAtTime(const ttime_t t_time_);

  const ttime_t GetCxlComTimeAtTime(const ttime_t t_time_);

  /// called by BaseSimTrader
  /// returns unique server_assigned_client_id
  virtual int Connect();

  void addRequest(SimGenRequest& genrequest);

  virtual HFSAT::ttime_t GetLastOrderActivityTime() { return HFSAT::ttime_t(0, 0); }

  virtual void SendOrderExch(const int saci, const char* security_name, const TradeType_t buysell, const double price,
                             const int size_requested, const int int_price, const int caos, bool is_fok, bool is_ioc);

  virtual void CancelOrderExch(const int saci, const int saos, const TradeType_t buysell, const int int_price);

  void CancelReplaceOrderExch(const int saci, const int saos, const TradeType_t buysell, const double _old_price_,
                              const int _old_int_price_, const int _old_size_, const double _new_price_,
                              const int _new_int_price_, const int _new_size_);

  void ReplayOrderExch(const int saci, const int caos, const TradeType_t buysell, const int int_price, const int saos);

  // START DELAY FUNCTIONS
  // should implement different logics based on different market model index
  inline ttime_t GetSendOrderDelay(ttime_t current_time) {
    ttime_t mkt_delay = ttime_t(0, market_model_.com_usecs_);
    DelayOutput mkt_delay_output_;

    if (sid_to_sim_config_[dep_security_id_].use_accurate_seqd_to_conf_) {
      if (security_delay_stats_->GetSendMktDelay(current_time, mkt_delay_output_,
                                                 sid_to_sim_config_[dep_security_id_].seq2conf_multiplier_,
                                                 sid_to_sim_config_[dep_security_id_].seq2conf_addend_) ||
          security_delay_stats_->GetSendConfDelay(current_time, mkt_delay_output_,
                                                  sid_to_sim_config_[dep_security_id_].seq2conf_multiplier_,
                                                  sid_to_sim_config_[dep_security_id_].seq2conf_addend_)) {
        // We are first looking for market delay and then if it is not available into conf delay.
        // Only in HK and OSE we use mkt conf in live as it is faster, so ideally we should use
        // conf delay in others.
        // TODO : Find out if using conf delay is better suited to other exchanges.
        mkt_delay = mkt_delay_output_.delay;
      } else {
        mkt_delay = median_mkt_delay_ * sid_to_sim_config_[dep_security_id_].seq2conf_multiplier_;
        mkt_delay.addusecs(sid_to_sim_config_[dep_security_id_].seq2conf_addend_);
      }
    }

    mkt_delay = (mkt_delay < ttime_t(0, lower_bound_usecs_)) ? ttime_t(0, lower_bound_usecs_) : mkt_delay;
    mkt_delay = (mkt_delay > ttime_t(0, upper_bound_usecs_)) ? ttime_t(0, upper_bound_usecs_) : mkt_delay;

    return mkt_delay;
  }

  inline bool GetIsCancellableBeforeConfirmation () {
    HFSAT::SecurityDefinitions  &security_definition_ = HFSAT::SecurityDefinitions::GetUniqueInstance();
    return security_definition_.cancel_before_conf(dep_symbol_);
  }  
   
  inline ttime_t GetCancelOrderDelay(ttime_t current_time) {
    ttime_t mkt_delay = ttime_t(0, market_model_.cxl_usecs_);  // assuming no mkt-ors delay
    DelayOutput mkt_delay_output_;

    if (sid_to_sim_config_[dep_security_id_].use_accurate_seqd_to_conf_) {
      if (security_delay_stats_->GetCancelMktDelay(current_time, mkt_delay_output_,
                                                   sid_to_sim_config_[dep_security_id_].seq2conf_multiplier_,
                                                   sid_to_sim_config_[dep_security_id_].seq2conf_addend_) ||
          security_delay_stats_->GetCancelConfDelay(current_time, mkt_delay_output_,
                                                    sid_to_sim_config_[dep_security_id_].seq2conf_multiplier_,
                                                    sid_to_sim_config_[dep_security_id_].seq2conf_addend_)) {
        mkt_delay = mkt_delay_output_.delay;

      } else {
        mkt_delay = ttime_t(0, 2 * normal_usecs_) * sid_to_sim_config_[dep_security_id_].seq2conf_multiplier_;
        mkt_delay.addusecs(sid_to_sim_config_[dep_security_id_].seq2conf_addend_);
      }
    }

    mkt_delay = (mkt_delay < ttime_t(0, lower_bound_usecs_)) ? ttime_t(0, lower_bound_usecs_) : mkt_delay;
    mkt_delay = (mkt_delay > ttime_t(0, upper_bound_usecs_)) ? ttime_t(0, upper_bound_usecs_) : mkt_delay;

    return mkt_delay;
  }

  inline ttime_t GetCancelReplaceOrderDelay(ttime_t current_time) {
    ttime_t mkt_delay = ttime_t(0, market_model_.cxl_usecs_);  // assuming no mkt-ors delay.
    DelayOutput mkt_delay_output_;

    if (sid_to_sim_config_[dep_security_id_].use_accurate_seqd_to_conf_) {
      if (security_delay_stats_->GetSendMktDelay(current_time, mkt_delay_output_,
                                                 sid_to_sim_config_[dep_security_id_].seq2conf_multiplier_,
                                                 sid_to_sim_config_[dep_security_id_].seq2conf_addend_) ||
          security_delay_stats_->GetSendConfDelay(current_time, mkt_delay_output_,
                                                  sid_to_sim_config_[dep_security_id_].seq2conf_multiplier_,
                                                  sid_to_sim_config_[dep_security_id_].seq2conf_addend_)) {
        mkt_delay = mkt_delay_output_.delay;

      } else {
        mkt_delay = median_mkt_delay_ * sid_to_sim_config_[dep_security_id_].seq2conf_multiplier_;
        mkt_delay.addusecs(sid_to_sim_config_[dep_security_id_].seq2conf_addend_);
      }
    }

    mkt_delay = (mkt_delay < ttime_t(0, lower_bound_usecs_)) ? ttime_t(0, lower_bound_usecs_) : mkt_delay;
    mkt_delay = (mkt_delay > ttime_t(0, upper_bound_usecs_)) ? ttime_t(0, upper_bound_usecs_) : mkt_delay;

    return mkt_delay;
  }

  inline ttime_t GetReplayOrderDelay(ttime_t current_time) {
    return (ttime_t(0, market_model_.conf_usecs_) + GetComTimeAtTime(watch_.tv()));
  }
  // END OF DELAY FUNCTIONS

  virtual int ProcessBidSendRequest(BaseSimOrder* sim_order, bool re_enqueue);
  virtual int ProcessAskSendRequest(BaseSimOrder* sim_order, bool re_enqueue);

  virtual void ProcessRequestQueue();

  inline void AddOrderNotFoundListener(OrderNotFoundListener* new_listener) {
    VectorUtils::UniqueVectorAdd(order_not_found_listener_vec_, new_listener);
  }
  inline void RemoveOrderNotFoundListener(OrderNotFoundListener* new_listener) {
    VectorUtils::UniqueVectorRemove(order_not_found_listener_vec_, new_listener);
  }

  inline void AddOrderSequencedListener(OrderSequencedListener* new_listener) {
    VectorUtils::UniqueVectorAdd(order_sequenced_listener_vec_, new_listener);
  }
  inline void RemOrderSequencedListener(OrderSequencedListener* new_listener) {
    VectorUtils::UniqueVectorRemove(order_sequenced_listener_vec_, new_listener);
  }

  inline void AddOrderConfirmedListener(OrderConfirmedListener* new_listener) {
    VectorUtils::UniqueVectorAdd(order_confirmed_listener_vec_, new_listener);
  }
  inline void RemoveOrderConfirmedListener(OrderConfirmedListener* new_listener) {
    VectorUtils::UniqueVectorRemove(order_confirmed_listener_vec_, new_listener);
  }

  inline void AddOrderConfCxlReplaceRejectedListener(OrderConfCxlReplaceRejectListener* new_listener) {
    VectorUtils::UniqueVectorAdd(order_conf_cxlreplace_rejected_listener_vec_, new_listener);
  }

  inline void RemoveOrderConfCxlReplaceRejectedListener(OrderConfCxlReplaceRejectListener* new_listener) {
    VectorUtils::UniqueVectorAdd(order_conf_cxlreplace_rejected_listener_vec_, new_listener);
  }

  inline void AddOrderConfCxlReplacedListener(OrderConfCxlReplacedListener* new_listener) {
    VectorUtils::UniqueVectorAdd(order_conf_cxlreplaced_listener_vec_, new_listener);
  }
  inline void RemoveOrderConfCxlReplacedListener(OrderConfCxlReplacedListener* new_listener) {
    VectorUtils::UniqueVectorRemove(order_conf_cxlreplaced_listener_vec_, new_listener);
  }

  inline void AddOrderCanceledListener(OrderCanceledListener* new_listener) {
    VectorUtils::UniqueVectorAdd(order_canceled_listener_vec_, new_listener);
  }
  inline void RemoveOrderCanceledListener(OrderCanceledListener* new_listener) {
    VectorUtils::UniqueVectorRemove(order_canceled_listener_vec_, new_listener);
  }

  inline void AddOrderExecutedListener(OrderExecutedListener* new_listener) {
    VectorUtils::UniqueVectorAdd(order_executed_listener_vec_, new_listener);
  }
  inline void RemoveOrderExecutedListener(OrderExecutedListener* new_listener) {
    VectorUtils::UniqueVectorRemove(order_executed_listener_vec_, new_listener);
  }

  inline void AddOrderRejectedListener(OrderRejectedListener* new_listener) {
    VectorUtils::UniqueVectorAdd(order_rejected_listener_vec_, new_listener);
  }
  inline void RemoveOrderRejectedListener(OrderRejectedListener* new_listener) {
    VectorUtils::UniqueVectorRemove(order_rejected_listener_vec_, new_listener);
  }

  inline void AddFPOrderExecutedListener(FPOrderExecutedListener* new_listener) {
    VectorUtils::UniqueVectorAdd(fporder_executed_listener_vec_, new_listener);
  }

  virtual void BackupQueueSizes(BaseSimOrder* sim_order);
  virtual int RestoreQueueSizes(BaseSimOrder* sim_order, const int posttrade_asksize_at_trade_price,
                                const int trd_size);
  void GetMatchingAlgoForShortcode(std::string shortcode, int tradingdate);

  /// Called by ORSMessageLiveSource when the messagetype is kORRType_Conf
  virtual void OrderConfirmed(const int saci, const int caos, const int saos, const unsigned int security_id,
                              const double price, const TradeType_t buysell, const int size_remaining,
                              const int size_executed, const int client_position, const int global_position,
                              const int int_price, const int32_t server_assigned_message_sequence,
                              const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}

  /// Called by ORSMessageLiveSource when the messagetype is kORRType_Confirmed
  virtual void OrderConfirmedAtTime(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                    const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                    const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                                    const int _size_executed_, const int _client_position_, const int _global_position_,
                                    const int _int_price_, const ttime_t _time_set_by_server_,
                                    const uint64_t t_exch_assigned_sequence_);

  /// Called by ORSMessageLiveSource when the messagetype is kORRType_CxlRejc
  virtual void OrderCancelRejected(const int saci, const int caos, const int saos, const unsigned int security_id,
                                   const double price, const TradeType_t buysell, const int size_remaining,
                                   const int rejection_reason, const int client_position, const int global_position,
                                   const int int_price, const uint64_t exchange_order_id,
                                   const ttime_t time_set_by_server);

  /// Called by ORSMessageLiveSource when the messagetype is kORRType_Seqd
  virtual void OrderSequenced(const int saci, const int caos, const int saos, const unsigned int security_id,
                              const double price, const TradeType_t buysell, const int size_remaining,
                              const int size_executed, const int client_position, const int global_position,
                              const int int_price, const int32_t server_assigned_message_sequence,
                              const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}

  /// Called by ORSMessageLiveSource when the messagetype is kORRType_Exec
  virtual void OrderExecuted(const int saci, const int caos, const int saos, const unsigned int security_id,
                             const double price, const TradeType_t buysell, const int size_remaining,
                             const int size_executed, const int client_position, const int global_position,
                             const int int_price, const int32_t server_assigned_message_sequence,
                             const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  /// Called by ORSMessageLiveSource when the messagetype is kORRType_IntExec
  virtual void OrderInternallyMatched(const int saci, const int caos, const int saos, const unsigned int security_id,
                                      const double price, const TradeType_t buysell, const int size_remaining,
                                      const int size_executed, const int client_position, const int global_position,
                                      const int int_price, const int32_t server_assigned_message_sequence,
                                      const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  /// Called by ORSMessageLiveSource when the messagetype is kORRType_ORSConf
  virtual void OrderORSConfirmed(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                 const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                 const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                                 const int _size_executed_, const int _int_price_,
                                 const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                 const ttime_t time_set_by_server) {}

  /// Called by ORSMessageLiveSource when the messagetype is kORRType_CxRe
  virtual void OrderConfCxlReplaced(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                    const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                    const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                                    const int _size_executed_, const int _client_position_, const int _global_position_,
                                    const int _int_price_, const int32_t server_assigned_message_sequence,
                                    const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  /// Called by ORSMessageLiveSource when the messagetype is kORRType_Rejc
  virtual void OrderRejected(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                             const unsigned int _security_id_, const double _price_, const TradeType_t _buysell_,
                             const int _size_remaining_, const int _rejection_reason_, const int _int_price_,
                             const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}

  /// Called by ORSMessageLiveSource when the messagetype is kORRType_None
  virtual void OrderNotFound(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                             const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                             const TradeType_t _buysell_, const int _int_price_,
                             const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                             const ttime_t time_set_by_server) {}

  /// Called by ORSMessageLiveSource when the messagetype is kORRType_Cxld
  virtual void OrderCanceled(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                             const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                             const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                             const int _client_position_, const int _global_position_, const int _int_price_,
                             const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                             const ttime_t time_set_by_server) {}

  /// Called by ORSMessageLiveSource when the messagetype is kORRType_cxld
  virtual void OrderCanceledAtTime(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                   const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                   const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                                   const int _client_position_, const int _global_position_, const int _int_price_,
                                   const ttime_t _time_set_by_server_, const uint64_t t_exch_assigned_sequence_) {}

  /// Called by ORSMesageLiveSource when messagetype is kORRType_Seqd
  virtual void OrderCxlSequenced(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                 const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                 const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                 const int _client_position_, const int _global_position_, const int r_int_price_,
                                 const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                 const ttime_t time_set_by_server) {}

  /// Called by ORSMessageLiveSource when messagetype is kORRType_CxlSeqd
  void OrderCxlSequencedAtTime(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                               const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                               const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                               const int _client_position_, const int _global_position_, const int r_int_price_,
                               const ttime_t _time_set_by_server_, const uint64_t t_exch_assigned_sequence_) {}

  void BidORSExec(int real_saci, int real_caos, int real_saos, int security_id, double price, int size_remaining,
                  int size_executed, int client_position, int global_position, int int_price);

  void AskORSExec(int real_saci, int real_caos, int real_saos, int security_id, double price, int size_remaining,
                  int size_executed, int client_position, int global_position, int int_price);

  int MatchBidORSExec(BaseSimOrder* sim_order, int size_executed, int size_remaining);
  int MatchAskORSExec(BaseSimOrder* sim_order, int size_executed, int size_remaining);

  void BidORSIntExec(int real_saci, int real_caos, int real_saos, int security_id, double price, int size_remaining,
                     int size_executed, int client_position, int global_position, int int_price);

  void AskORSIntExec(int real_saci, int real_caos, int real_saos, int security_id, double price, int size_remaining,
                     int size_executed, int client_position, int global_position, int int_price);

  // Callback from ors_message_livesource
  void OrderSequencedAtTime(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                            const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                            const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                            const int _size_executed_, const int _client_position_, const int _global_position_,
                            const int _int_price_, const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  virtual void BroadcastRejection(const int saci, const BaseSimOrder* sim_order, ORSRejectionReason_t reject_reason);

  virtual void BroadcastSequenced(const int saci, const BaseSimOrder* sim_order);

  virtual void BroadcastConfirm(const int t_server_assigned_client_id_, const BaseSimOrder* p_sim_order_);

  virtual void BroadcastCancelNotification(const int t_server_assigned_client_id_, const BaseSimOrder* p_sim_order_);

  virtual void BroadcastExecNotification(const int t_server_assigned_client_id_, const BaseSimOrder* p_sim_order_);

  virtual void BroadcastOrderNone(const int server_assigned_client_id_, const int client_assigned_order_sequence_,
                                  const int server_assigned_order_sequence_, const char* exchange_symbol_,
                                  const double price_, const int int_price_, const TradeType_t buysell_,
                                  const int size_remaining_, const int size_executed_);

  virtual void BroadcastOrderModifyRejectNotification(
      const int server_assigned_client_id, const int client_assigned_order_sequence,
      const int server_assigned_order_sequence, const unsigned int security_id, const double price,
      const TradeType_t buysell, const int size_remaining, const int client_position, const int global_position,
      const int intprice, const int32_t server_assigned_message_sequence);

  virtual void BroadcastOrderModifyNotification(const int _server_assigned_client_id_, const BaseSimOrder* p_sim_order_,
                                                const double _new_price_, const int _new_int_prie_,
                                                const int _new_size_);

  std::string GetBidSimOrderStr(int int_price);
  std::string GetAskSimOrderStr(int int_price);

  void UpdateAggSizes();

  SecurityMarketView& GetSMV() { return smv_; }

  // for debugging
  void PrintDelayStats(ttime_t start_time, ttime_t end_time);
};
}

#endif  // BASE_ORDERROUTING_BASE_SIM_MARKET_MAKER_H
