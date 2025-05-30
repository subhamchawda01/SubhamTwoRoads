/**
   \file OrderRouting/prom_order_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_ORDERROUTING_PROM_ORDER_MANAGER_H
#define BASE_ORDERROUTING_PROM_ORDER_MANAGER_H

#include <map>
#include <vector>
#include <math.h>

#include "dvccode/CDef/debug_logger.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/lockfree_simple_mempool.hpp"

#include "dvccode/ORSMessages/ors_message_listener.hpp"

#include "dvccode/CommonTradeUtils/watch.hpp"

#include "baseinfra/OrderRouting/size_maps.hpp"
#include "baseinfra/OrderRouting/base_order.hpp"
#include "baseinfra/OrderRouting/base_trader.hpp"

#include "baseinfra/OrderRouting/order_manager_listeners.hpp"

#include "baseinfra/MarketAdapter/basic_market_view_structs.hpp"

// #define MAINTAIN_TOTAL_REJECTS // only in infracore since this is needed only in oebu
#define REJECT_REFRESH_TIME 300000

#define CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember))

namespace HFSAT {

/** @brief Maintains a global ordermanager for the security. Similar to ( but does not extend ) BaseOrderManager.
 *
 * Ignoring clientid maintains orders for all clients.
 * Does not need to support order sending operations.
 * To be used by
 *    (i) SSMV ( SmartSecurityMarketView ), in case using_non_self_market_view_ is set to true.
 *         top level sizes are then computed as original values minus values obtained from here
 *    (ii) Strategy classes .. ExecInterface to know stuff
 *    (iii) indicators, listening to this through SSMV
 *
 * Modes : keep_orders_ = false. If false then this only maintains global_position_
 *
 * TODO : In future BaseOrderManager could use this to see when orders should be placed less on one side, to comply with
 * worst case position limits
 */
typedef std::map<unsigned int ,std::pair<bool,BaseOrder*>> SidBaseOrderMap;
class PromOrderManager : public OrderSequencedListener,
                         public OrderConfirmedListener,
                         public OrderConfCxlReplacedListener,
                         public OrderConfCxlReplaceRejectListener,
                         public OrderCanceledListener,
                         public OrderExecutedListener,
                         public OrderRejectedListener,
                         public OrderInternallyMatchedListener,
                         public OrderRejectedDueToFundsListener {
 private:
  /// Added copy constructor to disable it
  PromOrderManager(const PromOrderManager&);

  typedef void (PromOrderManager::*AdjustGlobalPositionForModes)(const int _global_position_, const double _trade_bid_price_,
                                   const double _trade_ask_price_, const unsigned int _security_id_, int _saci_, int _client_position_);
  AdjustGlobalPositionForModes adjust_global_position_for_modes_;
  SidBaseOrderMap secid_baseorder_map_;

 protected:
  static std::map<std::string, PromOrderManager*> shortcode_instance_map_;

  DebugLogger& dbglogger_;
  const Watch& watch_;
  SecurityNameIndexer& sec_name_indexer_;

  const std::string dep_shortcode_;
  const unsigned int dep_security_id_;
  const char* const dep_symbol_;
  // const FastPriceConvertor fast_price_convertor_;

  // flag set to disable all promom work
  bool enabled_;

  LockFreeSimpleMempool<BaseOrder> baseorder_mempool_;

  BidPriceSizeMap intpx_2_sum_bid_confirmed_;        /**< confirmed size at price */
  BidPriceSizeMap intpx_2_sum_bid_orders_confirmed_; /**< confirmed orders at price */
  BidPriceSizeMap intpx_2_sum_bid_unconfirmed_;      /**< sequenced but unconfirmed */
  BidPriceOrderMap intpx_2_bid_order_vec_;           /**< all sequenced orders, even unconfirmed ones */

  AskPriceSizeMap intpx_2_sum_ask_confirmed_;
  AskPriceSizeMap intpx_2_sum_ask_orders_confirmed_;
  AskPriceSizeMap intpx_2_sum_ask_unconfirmed_; /**< sequenced but unconfirmed */
  AskPriceOrderMap intpx_2_ask_order_vec_;      /**< all sequenced orders, even unconfirmed ones */

  int num_unconfirmed_orders_; /**< number of sequenced but unconfirmed orders ( does not know about unsequenced orders
                                  ) */

  std::vector<GlobalPositionChangeListener*> global_position_change_listener_vec_;
  std::vector<GlobalOrderChangeListener*> global_order_change_listener_vec_;
  std::vector<GlobalOrderExecListener*> global_order_exec_listener_vec_;

  std::map<int32_t, int> saci_to_cp_map_;

  int global_position_;
  double min_price_increment_;
  int num_ors_messages_;
  int total_traded_;
  int total_traded_size_;
  int total_rejects_;
  int total_exch_rejects_;
  int total_exch_cxl_rejects_;
  int total_cxl_rejects_;
  int total_exchange_replace_rejects;
  int total_ors_replace_rejects;
  int last_order_rejection_reason_;
  int last_reject_encountered_time_;
  double last_exec_bid_price_;
  int last_exec_bid_intpx_;
  double last_exec_ask_price_;
  int last_exec_ask_intpx_;
  static long double total_traded_value;

  // Based purely on information available
  // to the ORS.
  BestBidAskInfo ors_best_bid_ask_;

  // These hold the sizes/prices which have been
  // "recognized" in the market data updates.
  // A very correct non-self implementation will use these.
  mutable BestBidAskInfo mkt_affirmed_bid_ask_;

  // Need to maintain a time-series of our best_bid_ask updates.
  // This is needed for us to detect these in mkt updates.
  // Upon recognition , these will be moved to the mkt_affirmed_bid_ask_.
  mutable std::deque<BestBidAskInfo> mfm_ordered_bid_ask_updates_;

  // This should only be true in trading execs.
  mutable bool use_smart_non_self_;

  mutable bool manage_orders_also_;  ///< by default PromOrderManager only manages position. If this is set to TRUE then
  /// itmanages orders as well.

  //    PromOrderManager ( DebugLogger & _dbglogger_, const Watch & _watch_, SecurityNameIndexer & _sec_name_indexer_,
  //    const std::string & _dep_shortcode_, const unsigned int _security_id_, const char * _exchange_symbol_, const
  //    double _min_price_increment_ );
  PromOrderManager(DebugLogger& _dbglogger_, const Watch& _watch_, SecurityNameIndexer& _sec_name_indexer_,
                   const std::string& _dep_shortcode_, const unsigned int _security_id_, const char* _exchange_symbol_);

 public:
  static PromOrderManager* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             SecurityNameIndexer& _sec_name_indexer_,
                                             const std::string& _dep_shortcode_, const unsigned int _security_id_,
                                             const char* _exchange_symbol_);
  static void RemoveUniqueInstance(std::string shortcode);

  static PromOrderManager* GetCreatedInstance(const std::string& shortcode_) {
    std::map<std::string, PromOrderManager*>::const_iterator _iter_ = shortcode_instance_map_.find(shortcode_);
    if (_iter_ != shortcode_instance_map_.end()) {
      return _iter_->second;
    }
    return NULL;
  }

  int global_position() const { return global_position_; }
  int getOpenOrders();
  void PrintOpenOrders();
  int getOpenConfirmedOrders();
  int get_num_ors_messages() const { return num_ors_messages_; }
  int get_executed() const { return total_traded_; }
  int get_executed_totalsize() const { return total_traded_size_; }
  int get_rejected() const { return total_rejects_; }
  int get_exch_rejected() const { return total_exch_rejects_; }
  int get_exch_cxl_rejected() const { return total_exch_cxl_rejects_; }
  int get_exch_replace_rejects() const { return total_exchange_replace_rejects; }
  int get_ors_replace_rejects() const { return total_ors_replace_rejects; }
  int get_cxl_rejected() const { return total_cxl_rejects_; }
  int get_last_order_rejection_reason() const { return last_order_rejection_reason_; }
  inline const char* secname() const { return dep_symbol_; }  // returns the exchange symbol for this instrument
  SidBaseOrderMap& get_secid_baseorder_map() { return secid_baseorder_map_; }
  std::string get_shortcode(){ return dep_shortcode_; }

  inline void EnableSaciFiltering(std::vector<int32_t>& saci_vec) {
    for (auto & iter : saci_vec) {
      saci_to_cp_map_[iter] = 0;
    }
    adjust_global_position_for_modes_ = &PromOrderManager::AdjustFilteredSACIGlobalPosition;
  }

  inline void AddGlobalPositionChangeListener(GlobalPositionChangeListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(global_position_change_listener_vec_, _new_listener_);
    enabled_ = true;
  }
  inline void RemoveGlobalPositionChangeListener(GlobalPositionChangeListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(global_position_change_listener_vec_, _new_listener_);
  }

  inline void AddGlobalOrderChangeListener(GlobalOrderChangeListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(global_order_change_listener_vec_, _new_listener_);
    enabled_ = true;
  }
  inline void RemoveGlobalOrderChangeListener(GlobalOrderChangeListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(global_order_change_listener_vec_, _new_listener_);
  }

  inline void AddGlobalOrderExecListener(GlobalOrderExecListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(global_order_exec_listener_vec_, _new_listener_);
    enabled_ = true;
  }
  inline void RemoveGlobalOrderExecListener(GlobalOrderExecListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(global_order_exec_listener_vec_, _new_listener_);
  }

  inline void EnabledIfListenedTo() {
    if ((global_position_change_listener_vec_.empty()) && (global_order_change_listener_vec_.empty()) &&
        (global_order_exec_listener_vec_.empty())) {
      enabled_ = false;
    }
  }

  inline const BidPriceSizeMap& intpx_2_sum_bid_confirmed() const { return intpx_2_sum_bid_confirmed_; }
  inline const AskPriceSizeMap& intpx_2_sum_ask_confirmed() const { return intpx_2_sum_ask_confirmed_; }

  inline const BidPriceSizeMap& intpx_2_sum_bid_orders_confirmed() const { return intpx_2_sum_bid_orders_confirmed_; }
  inline const AskPriceSizeMap& intpx_2_sum_ask_orders_confirmed() const { return intpx_2_sum_ask_orders_confirmed_; }

  // Best bid/ask information.
  inline int best_bid_price() const { return ors_best_bid_ask_.best_bid_int_price_; }
  inline int best_bid_size() const { return ors_best_bid_ask_.best_bid_size_; }
  inline int best_bid_orders() const { return ors_best_bid_ask_.best_bid_orders_; }
  inline int best_ask_price() const { return ors_best_bid_ask_.best_ask_int_price_; }
  inline int best_ask_size() const { return ors_best_bid_ask_.best_ask_size_; }
  inline int best_ask_orders() const { return ors_best_bid_ask_.best_ask_orders_; }

  inline std::deque<BestBidAskInfo>& mfm_ordered_bid_ask_updates() const { return mfm_ordered_bid_ask_updates_; }
  inline BestBidAskInfo& mkt_affirmed_bid_ask() const { return mkt_affirmed_bid_ask_; }

  // Called from SMV ( "Simple or Dumb" non-self market calculation )
  inline const BestBidAskInfo& ors_best_bid_ask() const { return ors_best_bid_ask_; }

  inline void SetSmartNonSelf(const bool t_use_smart_non_self_) const {
    use_smart_non_self_ = t_use_smart_non_self_;
    DBGLOG_TIME_CLASS_FUNC << " use_smart_non_self_ = " << (use_smart_non_self_ ? "true" : "false")
                           << DBGLOG_ENDL_FLUSH;
  }
  inline void ManageOrdersAlso() const { manage_orders_also_ = true; }

  inline void RecomputeBestBids();
  inline void RecomputeBestAsks();

  int GetBidSizePlacedAboveEqIntPx(int _bid_int_px_, int _max_int_px_);
  int GetAskSizePlacedAboveEqIntPx(int _ask_int_px_, int _max_int_px_);

 public:
  /** \brief called by ORSLiveSource or ORSFileSource when orr_type_ is kORRType_Seqd
   *
   * sort of callback, called by ReplyHandler on receiving ORS confirming submission of an order
   * @param _server_assigned_client_id_ assigned by server to the client whose order this message pertains to
   * @param _client_assigned_order_sequence_ assigned by client before sending order to ORS
   * @param _server_assigned_order_sequence_ assigned by ORS before sending to exchange
   * @param _security_id_ the sid of the security, computed by the source of the message .. ORSFS or ORSLS or
   * SimMarketMaker
   * @param _price_ the limit price of the order
   * @param _buysell_ the side of the order, kTradeTypeBuy if this was a passive bid order or aggressive BUY order
   * @param _size_remaining_ the size still active in the market
   * @param _size_executed_ the size that has been executed by this order
   * @param _client_position_ only the position of this client
   * @param _global_position_ position of all clients trading this security at source ORS
   * @param _int_price_ this is sent by the client when sending the order, this is typically a key to a map where the
   * client has stored info of this order. In particular intpx_2_sum_bid_confirmed_ intpx_2_sum_bid_unconfirmed_
   * intpx_2_bid_order_vec_ all use this key. Sending this and receiving this in the reply saves a computation from
   * price and also protects against situations when the order was filled at a price that is not the limit price sent
   */
  void OrderSequenced(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                      const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                      const int _size_executed_, const int _client_position_, const int _global_position_,
                      const int _int_price_, const int32_t server_assigned_message_sequence,
                      const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  /** \brief called by ORSLiveSource or ORSFileSource when orr_type_ is kORRType_Conf
   *
   * sort of callback, called by ReplyHandler on receiving ORS confirming submission of an order
   * @param _server_assigned_client_id_ assigned by server to the client whose order this message pertains to
   * @param _client_assigned_order_sequence_ assigned by client before sending order to ORS
   * @param _server_assigned_order_sequence_ assigned by ORS before sending to exchange
   * @param _security_id_ the sid of the security, computed by the source of the message .. ORSFS or ORSLS or
   * SimMarketMaker
   * @param _price_ the limit price of the order
   * @param _buysell_ the side of the order, kTradeTypeBuy if this was a passive bid order or aggressive BUY order
   * @param _size_remaining_ the size still active in the market
   * @param _size_executed_ the size that has been executed by this order
   * @param _client_position_ only the position of this client
   * @param _global_position_ position of all clients trading this security at source ORS
   * @param _int_price_ this is sent by the client when sending the order, this is typically a key to a map where the
   * client has stored info of this order. In particular intpx_2_sum_bid_confirmed_ intpx_2_sum_bid_unconfirmed_
   * intpx_2_bid_order_vec_ all use this key. Sending this and receiving this in the reply saves a computation from
   * price and also protects against situations when the order was filled at a price that is not the limit price sent
   */
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
    // Subtracting here so that we donot count the ORS replay confirms as real ors confirm message
    num_ors_messages_--;
    if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " SID: " << _security_id_ << " SAOS: " << _server_assigned_order_sequence_
                             << " SZE: " << _size_executed_ << " SZR: " << _size_remaining_ << " BS: " << _buysell_
                             << DBGLOG_ENDL_FLUSH;
    }
    return OrderConfirmed(_server_assigned_client_id_, _client_assigned_order_sequence_,
                          _server_assigned_order_sequence_, _security_id_, _price_, _buysell_, _size_remaining_,
                          _size_executed_, 0 /*ignoring client_position_ */, global_position_, _int_price_,
                          server_assigned_message_sequence, exchange_order_id, time_set_by_server);
  }

  /** \brief called by ORSLiveSource or ORSFileSource when orr_type_ is kORRType_CxRe
   *
   * sort of callback, called by ReplyHandler on receiving ORS confirming submission of an order
   * @param _server_assigned_client_id_ assigned by server to the client whose order this message pertains to
   * @param _client_assigned_order_sequence_ assigned by client before sending order to ORS
   * @param _server_assigned_order_sequence_ assigned by ORS before sending to exchange
   * @param _security_id_ the sid of the security, computed by the source of the message .. ORSFS or ORSLS or
   * SimMarketMaker
   * @param _price_ the limit price of the order
   * @param _buysell_ the side of the order, kTradeTypeBuy if this was a passive bid order or aggressive BUY order
   * @param _size_remaining_ the size still active in the market
   * @param _size_executed_ the size that has been executed by this order
   * @param _client_position_ only the position of this client
   * @param _global_position_ position of all clients trading this security at source ORS
   * @param _int_price_ this is sent by the client when sending the order, this is typically a key to a map where the
   * client has stored info of this order. In particular intpx_2_sum_bid_confirmed_ intpx_2_sum_bid_unconfirmed_
   * intpx_2_bid_order_vec_ all use this key. Sending this and receiving this in the reply saves a computation from
   * price and also protects against situations when the order was filled at a price that is not the limit price sent
   */
  void OrderConfCxlReplaced(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                            const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                            const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                            const int _size_executed_, const int _client_position_, const int _global_position_,
                            const int _int_price_, const int32_t server_assigned_message_sequence,
                            const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  /**
   *
   * @param server_assigned_client_id
   * @param client_assigned_order_sequence
   * @param server_assigned_order_sequence
   * @param security_id
   * @param price
   * @param buysell
   * @param size_remaining
   * @param client_position
   * @param global_position
   * @param intprice
   * @param server_assigned_message_sequence
   */
  void OrderConfCxlReplaceRejected(const int server_assigned_client_id, const int client_assigned_order_sequence,
                                   const int server_assigned_order_sequence, const unsigned int security_id,
                                   const double price, const TradeType_t buysell, const int size_remaining,
                                   const int client_position, const int global_position, const int intprice,
                                   const int32_t rejection_reason, const int32_t server_assigned_message_sequence,
                                   const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  void OrderCancelRejected(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                           const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                           const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                           const int _rejection_reason_, const int _client_position_, const int _global_position_,
                           const int _int_price_, const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  /** \brief called by ORSLiveSource or ORSFileSource when orr_type_ is kORRType_Cxld
   *
   * sort of callback, called by ReplyHandler on receiving ORS confirming submission of an order
   * @param _server_assigned_client_id_ assigned by server to the client whose order this message pertains to
   * @param _client_assigned_order_sequence_ assigned by client before sending order to ORS
   * @param _server_assigned_order_sequence_ assigned by ORS before sending to exchange
   * @param _security_id_ the sid of the security, computed by the source of the message .. ORSFS or ORSLS or
   * SimMarketMaker
   * @param _price_ the limit price of the order
   * @param _buysell_ the side of the order, kTradeTypeBuy if this was a passive bid order or aggressive BUY order
   * @param _size_remaining_ the size still active in the market
   * @param _client_position_ only the position of this client
   * @param _global_position_ position of all clients trading this security at source ORS
   * @param _int_price_ this is sent by the client when sending the order, this is typically a key to a map where the
   * client has stored info of this order. In particular intpx_2_sum_bid_confirmed_ intpx_2_sum_bid_unconfirmed_
   * intpx_2_bid_order_vec_ all use this key. Sending this and receiving this in the reply saves a computation from
   * price and also protects against situations when the order was filled at a price that is not the limit price sent
   */
  void OrderCanceled(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const TradeType_t _buysell_, const int _size_remaining_, const int _client_position_,
                     const int _global_position_, const int _int_price_, const int32_t server_assigned_message_sequence,
                     const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  /** \brief called by ORSLiveSource or ORSFileSource when orr_type_ is kORRType_Exec
   *
   * sort of callback, called by ReplyHandler on receiving ORS confirming submission of an order
   * @param _server_assigned_client_id_ assigned by server to the client whose order this message pertains to
   * @param _client_assigned_order_sequence_ assigned by client before sending order to ORS
   * @param _server_assigned_order_sequence_ assigned by ORS before sending to exchange
   * @param _security_id_ the sid of the security, computed by the source of the message .. ORSFS or ORSLS or
   * SimMarketMaker
   * @param _price_ the execution price of the order !
   * @param _buysell_ the side of the order, kTradeTypeBuy if this was a passive bid order or aggressive BUY order
   * @param _size_remaining_ the size still active in the market
   * @param _size_executed_ the size that has been executed by this order
   * @param _client_position_ only the position of this client
   * @param _global_position_ position of all clients trading this security at source ORS
   * @param _int_price_ this is sent by the client when sending the order, this is typically a key to a map where the
   * client has stored info of this order. In particular intpx_2_sum_bid_confirmed_ intpx_2_sum_bid_unconfirmed_
   * intpx_2_bid_order_vec_ all use this key. Sending this and receiving this in the reply saves a computation from
   * price and also protects against situations when the order was filled at a price that is not the limit price sent
   */
  void OrderExecuted(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const TradeType_t _buysell_, const int _size_remaining_, const int _size_executed_,
                     const int _client_position_, const int _global_position_, const int _int_price_,
                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const ttime_t time_set_by_server);

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

  /** \brief called by ORSMessageLiveSource when the messagetype is kORRType_Rejc
   *
   * sort of callback, called by ORSMessageLiveSource on receiving ORS which is typically a response to a sendtrade that
   * is rejected
   * @param t_server_assigned_client_id_ assigned by server to the client whose order this message pertains to
   * @param _client_assigned_order_sequence_ assigned by client before sending order to ORS
   * @param _security_id_ the sid of the security, computed by the source of the message .. ORSMessageFileSource or
   * ORSMessageLiveSource or SimMarketMaker
   * @param _price_ the limit price of the order
   * @param _buysell_ the side of the order e.g. kTradeTypeBuy if this was a passive bid order or aggressive BUY order
   * @param _size_remaining_ the size still active in the market
   * @param _rejection_reason_ ORSRejectionReason_t from ORS
   * @param _int_price_ this is sent by the client when sending the order, this is typically a key to a map where the
   * client has stored info of this order. In particular intpx_2_sum_bid_confirmed_ intpx_2_sum_bid_unconfirmed_
   * intpx_2_bid_order_vec_ all use this key. Sending this and receiving this in the reply saves a computation from
   * price and also protects against situations when the order was filled at a price that is not the limit price sent
   */
  void OrderRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const unsigned int _security_id_, const double _price_, const TradeType_t r_buysell_,
                     const int _size_remaining_, const int _rejection_reason_, const int r_int_price_,
                     const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
    last_order_rejection_reason_ = _rejection_reason_;

    last_reject_encountered_time_ = watch_.msecs_from_midnight();

    // match for exch rejection reasons
    if (_rejection_reason_ == HFSAT::kExchOrderReject) {
      total_exch_rejects_++;
    } else if ( _rejection_reason_ == HFSAT::kExchDataEntryOrderReject){
      // Freeze Strat Here... 
      total_exch_rejects_++;
    } else {
      total_rejects_++;
    }
  }

  void setMargin (double margin_);

  void OrderRejectedDueToFunds(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                               const unsigned int _security_id_, const double _price_, const TradeType_t r_buysell_,
                               const int _size_remaining_, const int _rejection_reason_, const int r_int_price_,
                               const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
    total_exch_rejects_++;
  }

  void WakeUpifRejectedDueToFunds() {}

  long double get_total_traded_value() { return total_traded_value; }

 protected:
  // inline int GetIntPx ( const double & _price_ ) const { return fast_price_convertor_.GetFastIntPx ( _price_ ) ; }

  inline BaseOrder* FetchBidOrder(const int _int_price_, const int _server_assigned_order_sequence_) {
    if (intpx_2_bid_order_vec_.find(_int_price_) != intpx_2_bid_order_vec_.end()) {
      std::vector<BaseOrder*>& _this_base_order_vec_ = intpx_2_bid_order_vec_[_int_price_];
      for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
           _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
        if ((*_base_order_vec_iter_)->server_assigned_order_sequence() == _server_assigned_order_sequence_) {
          return *_base_order_vec_iter_;
        }
      }
    }
    for (auto iter = intpx_2_bid_order_vec_.begin(); iter != intpx_2_bid_order_vec_.end(); iter++) {
      std::vector<BaseOrder*>& _this_base_order_vec_ = iter->second;
      for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
           _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
        if ((*_base_order_vec_iter_)->server_assigned_order_sequence() == _server_assigned_order_sequence_) {
          return *_base_order_vec_iter_;
        }
      }
    }
    return NULL;
  }

  inline BaseOrder* FetchBidOrderByCAOS(const int _int_price_, const int _client_assigned_order_sequence_) {
    if (intpx_2_bid_order_vec_.find(_int_price_) != intpx_2_bid_order_vec_.end()) {
      std::vector<BaseOrder*>& _this_base_order_vec_ = intpx_2_bid_order_vec_[_int_price_];
      for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
           _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
        if ((*_base_order_vec_iter_)->client_assigned_order_sequence() == _client_assigned_order_sequence_) {
          return *_base_order_vec_iter_;
        }
      }
    }

    return NULL;
  }

  inline BaseOrder* FetchAskOrder(const int _int_price_, const int _server_assigned_order_sequence_) {
    if (intpx_2_ask_order_vec_.find(_int_price_) != intpx_2_ask_order_vec_.end()) {
      std::vector<BaseOrder*>& _this_base_order_vec_ = intpx_2_ask_order_vec_[_int_price_];
      for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
           _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
        if ((*_base_order_vec_iter_)->server_assigned_order_sequence() == _server_assigned_order_sequence_) {
          return *_base_order_vec_iter_;
        }
      }
    }
    for (auto iter = intpx_2_ask_order_vec_.begin(); iter != intpx_2_ask_order_vec_.end(); iter++) {
      std::vector<BaseOrder*>& _this_base_order_vec_ = iter->second;
      for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
           _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
        if ((*_base_order_vec_iter_)->server_assigned_order_sequence() == _server_assigned_order_sequence_) {
          return *_base_order_vec_iter_;
        }
      }
    }
    return NULL;
  }

  inline BaseOrder* FetchAskOrderByCAOS(const int _int_price_, const int _client_assigned_order_sequence_) {
    if (intpx_2_ask_order_vec_.find(_int_price_) != intpx_2_ask_order_vec_.end()) {
      std::vector<BaseOrder*>& _this_base_order_vec_ = intpx_2_ask_order_vec_[_int_price_];
      for (std::vector<BaseOrder*>::iterator _base_order_vec_iter_ = _this_base_order_vec_.begin();
           _base_order_vec_iter_ != _this_base_order_vec_.end(); _base_order_vec_iter_++) {
        if ((*_base_order_vec_iter_)->client_assigned_order_sequence() == _client_assigned_order_sequence_) {
          return *_base_order_vec_iter_;
        }
      }
    }

    return NULL;
  }

  inline void AdjustFilteredSACIGlobalPosition(const int _global_position_, const double _trade_bid_price_,
                                   const double _trade_ask_price_, const unsigned int _security_id_, int _saci_, int _client_position_) {
    int saci_filtered_global_position_ = _global_position_;
    if(saci_to_cp_map_.find(_saci_) != saci_to_cp_map_.end()){
      saci_to_cp_map_[_saci_] = _client_position_;
    }
    for(auto & itr : saci_to_cp_map_)
      saci_filtered_global_position_ += itr.second;


    AdjustGlobalPosition(saci_filtered_global_position_, _trade_bid_price_, _trade_ask_price_, _security_id_, _saci_, _client_position_);
  }

  inline void AdjustGlobalPosition(const int _global_position_, const double _trade_bid_price_,
                                   const double _trade_ask_price_, const unsigned int _security_id_, int _saci_, int _client_position_) {
    if (global_position_ != _global_position_) {
      if (!global_order_exec_listener_vec_
               .empty()) {  // Adjust position also by sending Execution for the possibly missed ones

        int size_missed = _global_position_ - global_position_;
        if (size_missed > 0) {  // missed a buy
          for (std::vector<GlobalOrderExecListener*>::iterator pciter = global_order_exec_listener_vec_.begin();
               pciter != global_order_exec_listener_vec_.end(); pciter++) {
            (*pciter)->OnGlobalOrderExec(dep_security_id_, kTradeTypeBuy, size_missed, _trade_bid_price_);
          }
          //dbglogger_  << "GLOBAL POSITION MISMATCH: SYM: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) 
	  //            << " SIZE: " << size_missed << " B "
          //            << " PX: " << _trade_bid_price_ << DBGLOG_ENDL_FLUSH;
         // if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
         //   DBGLOG_TIME_CLASS_FUNC << " SIZE: " << size_missed << " B "
         //                          << " PX: " << _trade_bid_price_ << DBGLOG_ENDL_FLUSH;
         // }
        }

        if (size_missed < 0) {
          for (std::vector<GlobalOrderExecListener*>::iterator pciter = global_order_exec_listener_vec_.begin();
               pciter != global_order_exec_listener_vec_.end(); pciter++) {
            (*pciter)->OnGlobalOrderExec(dep_security_id_, kTradeTypeSell, (-1) * size_missed, _trade_ask_price_);
          }
          //dbglogger_  << "GLOBAL POSITION MISMATCH: SYM: " << sec_name_indexer_.GetSecurityNameFromId(_security_id_) 
          //	      << " SIZE: " << -size_missed << " S "
          //            << " PX: " << _trade_bid_price_ << DBGLOG_ENDL_FLUSH;
         // if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
         //   DBGLOG_TIME_CLASS_FUNC << " SIZE: " << -size_missed << " S "
         //                          << " PX: " << _trade_bid_price_ << DBGLOG_ENDL_FLUSH;
         // }
        }
      }

      global_position_ = _global_position_;
      for (std::vector<GlobalPositionChangeListener*>::iterator pciter = global_position_change_listener_vec_.begin();
           pciter != global_position_change_listener_vec_.end(); pciter++) {
        (*pciter)->OnGlobalPositionChange(dep_security_id_, _global_position_);
      }
    }
  }

  inline void AdjustGlobalPosition(const int _global_position_, const double _trade_bid_price_,
                                   const double _trade_ask_price_) {
    if (global_position_ != _global_position_) {
      if (!global_order_exec_listener_vec_
               .empty()) {  // Adjust position also by sending Execution for the possibly missed ones

        int size_missed = _global_position_ - global_position_;
        if (size_missed > 0) {  // missed a buy
          for (std::vector<GlobalOrderExecListener*>::iterator pciter = global_order_exec_listener_vec_.begin();
               pciter != global_order_exec_listener_vec_.end(); pciter++) {
            (*pciter)->OnGlobalOrderExec(dep_security_id_, kTradeTypeBuy, size_missed, _trade_bid_price_);
          }
          if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << " SIZE: " << size_missed << " B "
                                   << " PX: " << _trade_bid_price_ << DBGLOG_ENDL_FLUSH;
          }
        }

        if (size_missed < 0) {
          for (std::vector<GlobalOrderExecListener*>::iterator pciter = global_order_exec_listener_vec_.begin();
               pciter != global_order_exec_listener_vec_.end(); pciter++) {
            (*pciter)->OnGlobalOrderExec(dep_security_id_, kTradeTypeSell, (-1) * size_missed, _trade_ask_price_);
          }
          if (dbglogger_.CheckLoggingLevel(PROM_OM_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << " SIZE: " << -size_missed << " S "
                                   << " PX: " << _trade_bid_price_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }

      global_position_ = _global_position_;
      for (std::vector<GlobalPositionChangeListener*>::iterator pciter = global_position_change_listener_vec_.begin();
           pciter != global_position_change_listener_vec_.end(); pciter++) {
        (*pciter)->OnGlobalPositionChange(dep_security_id_, _global_position_);
      }
    }
  }

  /// @param _buysell_ the side on which the order has changed kTradeTypeBuy means on the bid side
  /// @param _int_price_ the int_price at which some client's orders have changed
  inline void NotifyGlobalOrderChangeListeners(const TradeType_t _buysell_, const int _int_price_) {
    for (std::vector<GlobalOrderChangeListener*>::iterator pciter = global_order_change_listener_vec_.begin();
         pciter != global_order_change_listener_vec_.end(); pciter++) {
      (*pciter)->OnGlobalOrderChange(dep_security_id_, _buysell_, _int_price_);
    }
  }

  inline void NotifyGlobalOrderExecListeners(const TradeType_t _buysell_, const int _size_, const double _price_) {
    /// adjust global position to avoid multiple counting
    if (_buysell_ == kTradeTypeBuy) {
      global_position_ += _size_;
    } else {
      global_position_ -= _size_;
    }

    for (std::vector<GlobalOrderExecListener*>::iterator pciter = global_order_exec_listener_vec_.begin();
         pciter != global_order_exec_listener_vec_.end(); pciter++) {
      (*pciter)->OnGlobalOrderExec(dep_security_id_, _buysell_, _size_, _price_);
    }
  }

  // void DumpBidAskMap ( )
  // {
  //   // Best Bid, Assuming sorted decreasing
  //   HFSAT::BidPriceSizeMapConstIter_t intpx_2_sum_bid_confirmed_iter = intpx_2_sum_bid_confirmed_.begin ( ) ;
  //   // Best Ask, Assuming sorted increasing
  //   HFSAT::AskPriceSizeMapConstIter_t intpx_2_sum_ask_confirmed_iter = intpx_2_sum_ask_confirmed_.begin ( ) ;

  //   dbglogger_ << "------- PROM BID MAP: " <<"\n";

  //   for (intpx_2_sum_bid_confirmed_iter = intpx_2_sum_bid_confirmed_.begin ( );
  // 	   intpx_2_sum_bid_confirmed_iter != intpx_2_sum_bid_confirmed_.end ( );
  // 	   intpx_2_sum_bid_confirmed_iter ++)
  // 	{
  // 	  dbglogger_ << "SIZE : "<< intpx_2_sum_bid_confirmed_iter -> second << " PRICE: "<<
  // intpx_2_sum_bid_confirmed_iter -> first
  // 		     << DBGLOG_ENDL_FLUSH;
  // 	}

  //   dbglogger_ << "-------- PROM ASK MAP: " <<"\n";

  //   for (intpx_2_sum_ask_confirmed_iter = intpx_2_sum_ask_confirmed_.begin ( );
  // 	   intpx_2_sum_ask_confirmed_iter != intpx_2_sum_ask_confirmed_.end ( );
  // 	   intpx_2_sum_ask_confirmed_iter ++)
  // 	{
  // 	  dbglogger_ << "SIZE : "<< intpx_2_sum_ask_confirmed_iter -> second << " PRICE: "<<
  // intpx_2_sum_ask_confirmed_iter -> first
  // 		     << DBGLOG_ENDL_FLUSH;
  // 	}
  // }

 public:
  inline void CleanPromBAConfirmedMap(int t_int_best_bid_price, int t_int_best_ask_price) {
    // HFSAT::BidPriceSizeMap & intpx_2_sum_bid_confirmed_ = p_prom_order_manager_->intpx_2_sum_bid_confirmed ();
    // HFSAT::AskPriceSizeMap & intpx_2_sum_ask_confirmed_ = p_prom_order_manager_->intpx_2_sum_ask_confirmed ();

    // // Best Bid, Assuming sorted decreasing
    // HFSAT::BidPriceSizeMapConstIter_t intpx_2_sum_bid_confirmed_iter = intpx_2_sum_bid_confirmed_.begin ( ) ;
    // // Best Ask, Assuming sorted increasing
    // HFSAT::AskPriceSizeMapConstIter_t intpx_2_sum_ask_confirmed_iter = intpx_2_sum_ask_confirmed_.begin ( ) ;

    // Best Bid, Assuming sorted decreasing
    HFSAT::BidPriceSizeMapConstIter_t intpx_2_sum_bid_confirmed_iter = intpx_2_sum_bid_confirmed_.begin();
    // Best Ask, Assuming sorted increasing
    HFSAT::AskPriceSizeMapConstIter_t intpx_2_sum_ask_confirmed_iter = intpx_2_sum_ask_confirmed_.begin();

    for (intpx_2_sum_bid_confirmed_iter = intpx_2_sum_bid_confirmed_.begin();
         intpx_2_sum_bid_confirmed_iter != intpx_2_sum_bid_confirmed_.end(); intpx_2_sum_bid_confirmed_iter++) {
      // No bid map should have entries  higher than mkt price
      if (intpx_2_sum_bid_confirmed_iter->first > t_int_best_bid_price && intpx_2_sum_bid_confirmed_iter->second > 0) {
        //	  intpx_2_sum_bid_confirmed_iter -> second = 0;
        int t_int_price_ = intpx_2_sum_bid_confirmed_iter->first;
        intpx_2_sum_bid_confirmed_[t_int_price_] = 0;
      }
    }

    for (intpx_2_sum_ask_confirmed_iter = intpx_2_sum_ask_confirmed_.begin();
         intpx_2_sum_ask_confirmed_iter != intpx_2_sum_ask_confirmed_.end(); intpx_2_sum_ask_confirmed_iter++) {
      if (intpx_2_sum_ask_confirmed_iter->second > 0 && intpx_2_sum_ask_confirmed_iter->first < t_int_best_ask_price) {
        int t_int_price_ = intpx_2_sum_ask_confirmed_iter->first;
        intpx_2_sum_ask_confirmed_[t_int_price_] = 0;
      }
    }
  }
};

typedef std::vector<PromOrderManager*> PromOrderManagerPtrVec;

///< stored here as a static sid to smv* map so that indicators can use this directly
static inline PromOrderManagerPtrVec& sid_to_prom_order_manager_map() {
  static PromOrderManagerPtrVec sid_to_prom_order_manager_map_;
  return sid_to_prom_order_manager_map_;
}
}
#endif  // BASE_ORDERROUTING_PROM_ORDER_MANAGER_H
