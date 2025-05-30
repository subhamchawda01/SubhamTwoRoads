/**
   \file dvccode/ORSMessages/ors_message_listener.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   India
   +91 80 4190 3551
*/
#ifndef BASE_ORSMESSAGES_ORS_MESSAGE_LISTENER_H
#define BASE_ORSMESSAGES_ORS_MESSAGE_LISTENER_H

#include "dvccode/CDef/ors_defines.hpp"
#include "dvccode/CDef/ors_messages.hpp"

namespace HFSAT {

class OrderNotFoundListener {
 public:
  virtual ~OrderNotFoundListener(){};

  /** \brief called by ORSMessageLiveSource or ORSMessageFileSource or SimMarketMaker when the messagetype is
   * kORRType_None
   *
   * sort of callback, called by ReplyHandler on receiving ORS saying that the client requested a replay of an order but
   * the order was not found
   * @param t_server_assigned_client_id_ assigned by server to the client whose order this message pertains to
   * @param _client_assigned_order_sequence_ assigned by client before sending order to ORS
   * @param _server_assigned_order_sequence_ assigned by ORS before sending to exchange
   * @param _security_id_ the sid of the security, computed by the source of the message .. ORSMessageFileSource or
   * ORSMessageLiveSource or SimMarketMaker
   * @param r_buysell_ the side of the order, kTradeTypeBuy if this was a passive bid order or aggressive BUY order
   * @param r_int_price_ this is sent by the client when sending the order, this is typically a key to a map where the
   * client has stored info of this order. In particular intpx_2_sum_bid_confirmed_ intpx_2_sum_bid_unconfirmed_
   * intpx_2_bid_order_vec_ all use this key. Sending this and receiving this in the reply saves a computation from
   * price and also protects against situations when the order was filled at a price that is not the limit price sent
   */
  virtual void OrderNotFound(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                             const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                             const TradeType_t r_buysell_, const int r_int_price_,
                             const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                             const ttime_t time_set_by_server) = 0;
};

class OrderSequencedListener {
 public:
  virtual ~OrderSequencedListener(){};

  /** \brief called by ORSMessageLiveSource when the messagetype is kORRType_Seqd
   *
   * sort of callback, called by ReplyHandler on receiving ORS confirming submission of an order
   * @param t_server_assigned_client_id_ assigned by server to the client whose order this message pertains to
   * @param _client_assigned_order_sequence_ assigned by client before sending order to ORS
   * @param _server_assigned_order_sequence_ assigned by ORS before sending to exchange
   * @param _security_id_ the sid of the security, computed by the source of the message .. ORSMessageFileSource or
   * ORSMessageLiveSource or SimMarketMaker
   * @param _price_ the limit price of the order
   * @param _buysell_ the side of the order, kTradeTypeBuy if this was a passive bid order or aggressive BUY order
   * @param _size_remaining_ the size still active in the market
   * @param _size_executed_ the size that has been executed by this order
   * @param _client_position_ only the position of this client
   * @param _global_position_ position of all clients trading this security at the source OrderRoutingServer
   * @param _int_price_ this is sent by the client when sending the order, this is typically a key to a map where the
   * client has stored info of this order. In particular intpx_2_sum_bid_confirmed_ intpx_2_sum_bid_unconfirmed_
   * intpx_2_bid_order_vec_ all use this key. Sending this and receiving this in the reply saves a computation from
   * price and also protects against situations when the order was filled at a price that is not the limit price sent
   */
  virtual void OrderSequenced(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                              const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                              const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                              const int _size_executed_, const int _client_position_, const int _global_position_,
                              const int r_int_price_, const int32_t server_assigned_message_sequence,
                              const uint64_t exchange_order_id, const ttime_t time_set_by_server) = 0;

  virtual void OrderSequencedAtTime(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                    const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                    const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                    const int _size_executed_, const int _client_position_, const int _global_position_,
                                    const int r_int_price_, const uint64_t exchange_order_id,
                                    const ttime_t _time_set_by_server_) {
    return;
  }
};

class OrderConfirmedListener {
 public:
  virtual ~OrderConfirmedListener(){};

  /** \brief called by ORSMessageLiveSource when the messagetype is kORRType_Conf
   *
   * sort of callback, called by ReplyHandler on receiving ORS confirming submission of an order
   * @param t_server_assigned_client_id_ assigned by server to the client whose order this message pertains to
   * @param _client_assigned_order_sequence_ assigned by client before sending order to ORS
   * @param _server_assigned_order_sequence_ assigned by ORS before sending to exchange
   * @param _security_id_ the sid of the security, computed by the source of the message .. ORSMessageFileSource or
   * ORSMessageLiveSource or SimMarketMaker
   * @param _price_ the limit price of the order
   * @param _buysell_ the side of the order, kTradeTypeBuy if this was a passive bid order or aggressive BUY order
   * @param _size_remaining_ the size still active in the market
   * @param _size_executed_ the size that has been executed by this order
   * @param _client_position_ only the position of this client
   * @param _global_position_ position of all clients trading this security at the source OrderRoutingServer
   * @param _int_price_ this is sent by the client when sending the order, this is typically a key to a map where the
   * client has stored info of this order. In particular intpx_2_sum_bid_confirmed_ intpx_2_sum_bid_unconfirmed_
   * intpx_2_bid_order_vec_ all use this key. Sending this and receiving this in the reply saves a computation from
   * price and also protects against situations when the order was filled at a price that is not the limit price sent
   */
  virtual void OrderConfirmed(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                              const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                              const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                              const int _size_executed_, const int _client_position_, const int _global_position_,
                              const int r_int_price_, const int32_t server_assigned_message_sequence,
                              const uint64_t exchange_order_id, const ttime_t time_set_by_server) = 0;

  /// This is same as the function above, except that it is called by the LiveSource when the ClientThread sends a
  /// message of orr_type kORRType_ORSConf
  virtual void OrderORSConfirmed(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                 const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                 const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                 const int _size_executed_, const int r_int_price_,
                                 const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                 const ttime_t time_set_by_server) = 0;

  virtual void OrderConfirmedAtTime(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                    const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                    const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                    const int _size_executed_, const int _client_position_, const int _global_position_,
                                    const int r_int_price_, const uint64_t exchange_order_id,
                                    const ttime_t _time_set_by_server_) {
    return;
  }
};

class OrderConfCxlReplaceRejectListener {
 public:
  virtual ~OrderConfCxlReplaceRejectListener(){};

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
  virtual void OrderConfCxlReplaceRejected(const int server_assigned_client_id,
                                           const int client_assigned_order_sequence,
                                           const int server_assigned_order_sequence, const unsigned int security_id,
                                           const double price, const TradeType_t buysell, const int size_remaining,
                                           const int client_position, const int global_position, const int intprice,
                                           const int32_t rejection_reason,
                                           const int32_t server_assigned_message_sequence,
                                           const uint64_t exchange_order_id, const ttime_t time_set_by_server) = 0;
};
class OrderConfCxlReplacedListener {
 public:
  virtual ~OrderConfCxlReplacedListener(){};

  /** \brief called by ORSMessageLiveSource when the messagetype is kORRType_CxRe
   *
   * sort of callback, called by ReplyHandler on receiving ORS confirming submission of an order
   * @param t_server_assigned_client_id_ assigned by server to the client whose order this message pertains to
   * @param _client_assigned_order_sequence_ assigned by client before sending order to ORS
   * @param _server_assigned_order_sequence_ assigned by ORS before sending to exchange
   * @param _security_id_ the sid of the security, computed by the source of the message .. ORSMessageFileSource or
   * ORSMessageLiveSource or SimMarketMaker
   * @param _price_ the limit price of the order
   * @param _buysell_ the side of the order, kTradeTypeBuy if this was a passive bid order or aggressive BUY order
   * @param _size_remaining_ the size still active in the market
   * @param _size_executed_ the size that has been executed by this order
   * @param _client_position_ only the position of this client
   * @param _global_position_ position of all clients trading this security at the source OrderRoutingServer
   * @param _int_price_ this is sent by the client when sending the order, this is typically a key to a map where the
   * client has stored info of this order. In particular intpx_2_sum_bid_confirmed_ intpx_2_sum_bid_unconfirmed_
   * intpx_2_bid_order_vec_ all use this key. Sending this and receiving this in the reply saves a computation from
   * price and also protects against situations when the order was filled at a price that is not the limit price sent
   */
  virtual void OrderConfCxlReplaced(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                    const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                    const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                    const int _size_executed_, const int _client_position_, const int _global_position_,
                                    const int r_int_price_, const int32_t server_assigned_message_sequence,
                                    const uint64_t exchange_order_id, const ttime_t time_set_by_server) = 0;

  /** \brief
  *@param margin_ used to conveymargin from orsreply
  */
  virtual void setMargin(double margin_) = 0; 
};

class OrderCxlSeqdListener {
 public:
  virtual ~OrderCxlSeqdListener(){};

  virtual void OrderCxlSequenced(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                 const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                 const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                 const int _client_position_, const int _global_position_, const int r_int_price_,
                                 const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                 const ttime_t time_set_by_server) = 0;

  virtual void OrderCxlSequencedAtTime(const int t_server_assigned_client_id_,
                                       const int _client_assigned_order_sequence_,
                                       const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                       const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                       const int _client_position_, const int _global_position_, const int r_int_price_,
                                       const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
    return;
  }
};

class OrderCanceledListener {
 public:
  virtual ~OrderCanceledListener(){};

  /** \brief called by ORSMessageLiveSource when the messagetype is kORRType_Cxld
   *
   * sort of callback, called by ReplyHandler on receiving ORS confirming successful cancelation of an order
   * @param t_server_assigned_client_id_ assigned by server to the client whose order this message pertains to
   * @param _client_assigned_order_sequence_ assigned by client before sending order to ORS
   * @param _server_assigned_order_sequence_ assigned by ORS before sending to exchange
   * @param _security_id_ the sid of the security, computed by the source of the message .. ORSMessageFileSource or
   * ORSMessageLiveSource or SimMarketMaker
   * @param _price_ the limit price of the order
   * @param _buysell_ the side of the order, kTradeTypeBuy if this was a passive bid order or aggressive BUY order
   * @param _size_remaining_ the size still active in the market
   * @param _client_position_ only the position of this client
   * @param _global_position_ position of all clients trading this security at the source OrderRoutingServer
   * @param _int_price_ this is sent by the client when sending the order, this is typically a key to a map where the
   * client has stored info of this order. In particular intpx_2_sum_bid_confirmed_ intpx_2_sum_bid_unconfirmed_
   * intpx_2_bid_order_vec_ all use this key. Sending this and receiving this in the reply saves a computation from
   * price and also protects against situations when the order was filled at a price that is not the limit price sent
   */
  virtual void OrderCanceled(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                             const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                             const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                             const int _client_position_, const int _global_position_, const int r_int_price_,
                             const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                             const ttime_t time_set_by_server) = 0;

  virtual void OrderCanceledAtTime(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                   const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                   const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                   const int _client_position_, const int _global_position_, const int r_int_price_,
                                   const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
    return;
  }

  /** \brief called by ORSMessageLiveSource when the messagetype is kORRType_CxlRejc
   *
   * sort of callback, called by ReplyHandler on receiving ORS message tat a cancel was rejected by the exchange
   * @param t_server_assigned_client_id_ assigned by server to the client whose order this message pertains to
   * @param _client_assigned_order_sequence_ assigned by client before sending order to ORS
   * @param _server_assigned_order_sequence_ assigned by ORS before sending to exchange
   * @param _security_id_ the sid of the security, computed by the source of the message .. ORSMessageFileSource or
   * ORSMessageLiveSource or SimMarketMaker
   * @param _price_ the limit price of the order
   * @param _buysell_ the side of the order, kTradeTypeBuy if this was a passive bid order or aggressive BUY order
   * @param _size_remaining_ the size still active in the market
   * @param _rejection_reason_ the reason why the cancel did not go through
   * @param _client_position_ only the position of this client
   * @param _global_position_ position of all clients trading this security at the source OrderRoutingServer
   * @param _int_price_ this is sent by the client when sending the order, this is typically a key to a map where the
   * client has stored info of this order. In particular intpx_2_sum_bid_confirmed_ intpx_2_sum_bid_unconfirmed_
   * intpx_2_bid_order_vec_ all use this key. Sending this and receiving this in the reply saves a computation from
   * price and also protects against situations when the order was filled at a price that is not the limit price sent
   */
  virtual void OrderCancelRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                   const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                   const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                                   const int _rejection_reason_, const int t_client_position_,
                                   const int t_global_position_, const int r_int_price_,
                                   const uint64_t exchange_order_id, const ttime_t time_set_by_server) = 0;
};

/// Interface of listeners to ORS messages conveying execution of an order sent to the exchang/ or in future even if
/// internally matched
/// fields expected : _global_position_, t_server_assigned_client_id_,
class OrderExecutedListener {
 public:
  virtual ~OrderExecutedListener(){};

  /** \brief called by ORSMessageLiveSource or ORSMessageFileSource when the messagetype is kORRType_Exec
   *
   * sort of callback, called by ReplyHandler on receiving ORS confirming execution of an order ( which must have
   * received it from the exchange unless matched internally )
   * @param t_server_assigned_client_id_ assigned by server to the client whose order this message pertains to
   * @param _client_assigned_order_sequence_ assigned by client before sending order to ORS
   * @param _server_assigned_order_sequence_ assigned by ORS before sending to exchange
   * @param _security_id_ the sid of the security, computed by the source of the message .. ORSMessageFileSource or
   * ORSMessageLiveSource or SimMarketMaker
   * @param _price_ the execution price of the order !
   * @param r_buysell_ the side of the order, kTradeTypeBuy if this was a passive bid order or aggressive BUY order
   * @param _size_remaining_ the size still active in the market
   * @param _size_executed_ the size that has been executed by this order
   * @param _client_position_ only the position of this client
   * @param _global_position_ position of all clients trading this security at the source OrderRoutingServer
   * @param r_int_price_ this is sent by the client when sending the order, this is typically a key to a map where the
   * client has stored info of this order. In particular intpx_2_sum_bid_confirmed_ intpx_2_sum_bid_unconfirmed_
   * intpx_2_bid_order_vec_ all use this key. Sending this and receiving this in the reply saves a computation from
   * price and also protects against situations when the order was filled at a price that is not the limit price sent
   */
  virtual void OrderExecuted(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                             const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                             const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                             const int _size_executed_, const int _client_position_, const int _global_position_,
                             const int r_int_price_, const int32_t server_assigned_message_sequence,
                             const uint64_t exchange_order_id, const ttime_t time_set_by_server) = 0;
};

/// Interface of listeners to ORS messages conveying execution of an order sent to the exchang/ or in future even if
/// internally matched
/// fields expected : _global_position_, t_server_assigned_client_id_,
class OrderInternallyMatchedListener {
 public:
  virtual ~OrderInternallyMatchedListener(){};

  /** \brief called by ORSMessageLiveSource or ORSMessageFileSource when the messagetype is kORRType_IntExec
   *
   * sort of callback, called by ReplyHandler on receiving ORS confirming execution of an order ( which must have
   * received it from the exchange unless matched internally )
   * @param t_server_assigned_client_id_ assigned by server to the client whose order this message pertains to
   * @param _client_assigned_order_sequence_ assigned by client before sending order to ORS
   * @param _server_assigned_order_sequence_ assigned by ORS before sending to exchange
   * @param _security_id_ the sid of the security, computed by the source of the message .. ORSMessageFileSource or
   * ORSMessageLiveSource or SimMarketMaker
   * @param _price_ the execution price of the order !
   * @param r_buysell_ the side of the order, kTradeTypeBuy if this was a passive bid order or aggressive BUY order
   * @param _size_remaining_ the size still active in the market
   * @param _size_executed_ the size that has been executed by this order
   * @param _client_position_ only the position of this client
   * @param _global_position_ position of all clients trading this security at the source OrderRoutingServer
   * @param r_int_price_ this is sent by the client when sending the order, this is typically a key to a map where the
   * client has stored info of this order. In particular intpx_2_sum_bid_confirmed_ intpx_2_sum_bid_unconfirmed_
   * intpx_2_bid_order_vec_ all use this key. Sending this and receiving this in the reply saves a computation from
   * price and also protects against situations when the order was filled at a price that is not the limit price sent
   */
  virtual void OrderInternallyMatched(const int t_server_assigned_client_id_,
                                      const int _client_assigned_order_sequence_,
                                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                      const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                      const int _size_executed_, const int _client_position_,
                                      const int _global_position_, const int r_int_price_,
                                      const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                      const ttime_t time_set_by_server) = 0;
};

/// Interface of listeners to ORS messages conveying rejection of an order sent to it

class OrderRejectedListener {
 public:
  virtual ~OrderRejectedListener(){};

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
  virtual void OrderRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                             const unsigned int _security_id_, const double _price_, const TradeType_t r_buysell_,
                             const int _size_remaining_, const int _rejection_reason_, const int r_int_price_,
                             const uint64_t exchange_order_id, const ttime_t time_set_by_server) = 0;
};

class OrderRejectedDueToFundsListener {
 public:
  virtual ~OrderRejectedDueToFundsListener(){};
  virtual void OrderRejectedDueToFunds(const int t_server_assigned_client_id_,
                                       const int _client_assigned_order_sequence_, const unsigned int _security_id_,
                                       const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                       const int _rejection_reason_, const int r_int_price_,
                                       const uint64_t exchange_order_id, const ttime_t time_set_by_server) = 0;
  virtual void WakeUpifRejectedDueToFunds() = 0;
};

class ORSMessagesListener : public OrderNotFoundListener,
                            public OrderSequencedListener,
                            public OrderConfirmedListener,
                            public OrderConfCxlReplaceRejectListener,
                            public OrderConfCxlReplacedListener,
                            public OrderCxlSeqdListener,
                            public OrderCanceledListener,
                            public OrderExecutedListener,
                            public OrderInternallyMatchedListener,
                            public OrderRejectedListener,
                            public OrderRejectedDueToFundsListener {
 public:
  virtual void ORSMessageBegin(const unsigned int _security_id_, const GenericORSReplyStruct& ors_reply) {}

  virtual void ORSMessageEnd(const unsigned int _security_id_, const GenericORSReplyStruct& ors_reply) {}

  virtual void OrderNotFound(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                             const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                             const TradeType_t r_buysell_, const int r_int_price_,
                             const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                             const ttime_t time_set_by_server) override {}

  virtual void OrderSequenced(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                              const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                              const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                              const int _size_executed_, const int _client_position_, const int _global_position_,
                              const int r_int_price_, const int32_t server_assigned_message_sequence,
                              const uint64_t exchange_order_id, const ttime_t time_set_by_server) override final {}

  virtual void OrderSequencedAtTime(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                    const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                    const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                    const int _size_executed_, const int _client_position_, const int _global_position_,
                                    const int r_int_price_, const uint64_t exchange_order_id,
                                    const ttime_t time_set_by_server) override {}

  // This API is obsolete. Use OrderConfirmedAtTime() instead
  virtual void OrderConfirmed(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                              const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                              const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                              const int _size_executed_, const int _client_position_, const int _global_position_,
                              const int r_int_price_, const int32_t server_assigned_message_sequence,
                              const uint64_t exchange_order_id, const ttime_t time_set_by_server) override final {}

  virtual void OrderORSConfirmed(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                 const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                 const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                 const int _size_executed_, const int r_int_price_,
                                 const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                 const ttime_t time_set_by_server) override {}

  virtual void OrderConfirmedAtTime(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                    const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                    const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                    const int _size_executed_, const int _client_position_, const int _global_position_,
                                    const int r_int_price_, const uint64_t exchange_order_id,
                                    const ttime_t time_set_by_server) override {}

  virtual void OrderConfCxlReplaceRejected(
      const int server_assigned_client_id, const int client_assigned_order_sequence,
      const int server_assigned_order_sequence, const unsigned int security_id, const double price,
      const TradeType_t buysell, const int size_remaining, const int client_position, const int global_position,
      const int intprice, const int32_t rejection_reason, const int32_t server_assigned_message_sequence,
      const uint64_t exchange_order_id, const ttime_t time_set_by_server) override {}

  virtual void OrderConfCxlReplaced(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                    const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                    const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                    const int _size_executed_, const int _client_position_, const int _global_position_,
                                    const int r_int_price_, const int32_t server_assigned_message_sequence,
                                    const uint64_t exchange_order_id, const ttime_t time_set_by_server) override {}

  void setMargin( double margin_) override {}                                    

  // This API is obsolete. Use OrderCxlSequencedAtTime() instead
  virtual void OrderCxlSequenced(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                 const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                 const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                 const int _client_position_, const int _global_position_, const int r_int_price_,
                                 const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                 const ttime_t time_set_by_server) override final {}

  virtual void OrderCxlSequencedAtTime(const int t_server_assigned_client_id_,
                                       const int _client_assigned_order_sequence_,
                                       const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                       const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                       const int _client_position_, const int _global_position_, const int r_int_price_,
                                       const uint64_t exchange_order_id, const ttime_t time_set_by_server) override {}

  // This API is obsolete. Use OrderCanceledAtTime() instead
  virtual void OrderCanceled(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                             const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                             const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                             const int _client_position_, const int _global_position_, const int r_int_price_,
                             const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                             const ttime_t time_set_by_server) override final {}

  virtual void OrderCanceledAtTime(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                   const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                   const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                   const int _client_position_, const int _global_position_, const int r_int_price_,
                                   const uint64_t exchange_order_id, const ttime_t time_set_by_server) override {}

  virtual void OrderCancelRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                   const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                   const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                                   const int _rejection_reason_, const int t_client_position_,
                                   const int t_global_position_, const int r_int_price_,
                                   const uint64_t exchange_order_id, const ttime_t time_set_by_server) override {}

  virtual void OrderExecuted(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                             const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                             const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                             const int _size_executed_, const int _client_position_, const int _global_position_,
                             const int r_int_price_, const int32_t server_assigned_message_sequence,
                             const uint64_t exchange_order_id, const ttime_t time_set_by_server) override {}

  virtual void OrderInternallyMatched(const int t_server_assigned_client_id_,
                                      const int _client_assigned_order_sequence_,
                                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                      const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                      const int _size_executed_, const int _client_position_,
                                      const int _global_position_, const int r_int_price_,
                                      const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                      const ttime_t time_set_by_server) override {}

  virtual void OrderRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                             const unsigned int _security_id_, const double _price_, const TradeType_t r_buysell_,
                             const int _size_remaining_, const int _rejection_reason_, const int r_int_price_,
                             const uint64_t exchange_order_id, const ttime_t time_set_by_server) override {}

  virtual void OrderRejectedDueToFunds(const int t_server_assigned_client_id_,
                                       const int _client_assigned_order_sequence_, const unsigned int _security_id_,
                                       const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                       const int _rejection_reason_, const int r_int_price_,
                                       const uint64_t exchange_order_id, const ttime_t time_set_by_server) override {}
  virtual void WakeUpifRejectedDueToFunds() {}
};
}
#endif  // BASE_ORSMESSAGES_ORS_MESSAGE_LISTENER_H
