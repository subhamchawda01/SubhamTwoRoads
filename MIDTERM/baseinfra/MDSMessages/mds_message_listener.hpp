/**
    \file MDSMessages/mds_message_listener.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#pragma once

#include <sys/time.h>
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/generic_l1_data_struct.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/nse_mds_defines.hpp"
#include "baseinfra/MarketAdapter/basic_market_view_structs.hpp"
#include "baseinfra/MDSMessages/order_message_listeners.hpp"

namespace HFSAT {
#define FULL_BOOK_DEPTH 5
#define QUINCY_DEPTH_3 3

/// \brief interface for all internal PriceLevel events, typically called by CMELoggedMessageFileSource /
/// EUREXLoggedMessageFileSource / MDSFileSource etc and listened by EUREXPriceLevelMarketViewManager
class PriceLevelGlobalListener {
 public:
  virtual ~PriceLevelGlobalListener() {}

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_buysell_ the side this is level is added to
   * @param t_level_added_ the level number ( post change ) that was added
   * @param t_price_ price of the level
   * @param t_new_size_ the new size after change
   * @param t_new_ordercount_ the new number of orders after change
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                               const int t_level_added_, const double t_price_, const int t_new_size_,
                               const int t_new_ordercount_, const bool t_is_intermediate_message_) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_buysell_ the side this is level is deleted from
   * @param t_level_removed_ the level removed
   * @param t_price_ price of the level
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnPriceLevelDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                  const int t_level_removed_, const double t_price_,
                                  const bool t_is_intermediate_message_) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_buysell_ the side this is level is deleted from
   * @param t_level_changed_ the level number lchanged
   * @param t_price_ price of the level
   * @param t_new_size_ the new size after change
   * @param t_new_ordercount_ the new number of orders after change
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnPriceLevelChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                  const int t_level_changed_, const double t_price_, const int t_new_size_,
                                  const int t_new_ordercount_, const bool t_is_intermediate_message_) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_buysell_ the side this is level is deleted from
   * @param t_min_level_deleted_ starting at this level all higher ( away from market ) levels are to be removed
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnPriceLevelDeleteFrom(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                      const int t_min_level_deleted_, const bool t_is_intermediate_message_) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_buysell_ the side this is level is deleted from
   * @param t_max_level_deleted_ starting at top (0) level all levels upto this level are to be removed
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnPriceLevelDeleteThrough(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                         const int t_max_level_deleted_, const bool t_is_intermediate_message_) = 0;

  /**

       * @param t_security_id_ security_id_ of the exchange symbol this update was about
       * @param t_buysell_ the side this is level is deleted from
       * @param t_price_ price
       * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
       * should not alerted
       */

  virtual void OnPriceLevelDeleteSynthetic(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                           const double t_price_, const bool t_is_intermediate_message_) {}

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_buysell_ the side this is level is deleted from
   * @param t_level_overlayed_ the level number changed
   * @param t_price_ price of the level
   * @param t_new_size_ the new size after change
   * @param t_new_ordercount_ the new number of orders after change
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnPriceLevelOverlay(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                   const int t_level_overlayed_, const double t_price_, const int t_new_size_,
                                   const int t_new_ordercount_, const bool t_is_intermediate_message_) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_trade_price_ price at which trade was matched
   * @param t_trade_size_ size matched in this trade
   * @param t_buysell_ aggressive part side
   */
  virtual void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
                       const TradeType_t t_buysell_) = 0;
  virtual void OnMarketStatusUpdate(const unsigned int t_security_id_, const MktStatus_t t_this_mkt_status_) {}
  virtual void OnOffMarketTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
                                const TradeType_t t_buysell_) {}

  virtual void OnResetBegin(const unsigned int t_security_id_) {}
  virtual void OnResetEnd(const unsigned int t_security_id_) {}

  virtual void OnPriceLevelUpdate(void* ptr_to_price_level_update, int length_of_bytes,
                                  HFSAT::MDS_MSG::MDSMessageExchType exch_type) {}
};

// book building modification purpose
class OrderFeedGlobalListener {
 public:
  virtual ~OrderFeedGlobalListener() {}

  // Main Functions
  virtual void OnOrderAdd(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_, double t_price_,
                          int t_size_, uint32_t t_priority_, bool t_intermediate_) = 0;
  virtual void OnOrderDelete(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_,
                             bool t_intermediate_, bool t_is_set_order_info_map_iter_ = false) = 0;
  virtual void OnOrderModify(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_, int t_new_size_,
                             uint64_t t_new_order_id_, bool t_intermediate_,
                             bool t_is_set_order_info_map_iter_ = false) = 0;
  virtual void OnOrderReplace(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_,
                              double t_new_price_, int t_new_size_, uint64_t t_new_order_id_, bool t_intermediate_) = 0;
  virtual void OnOrderExec(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_, double t_price_,
                           int t_size_exec_, bool t_intermediate_) = 0;
  virtual void OnOrderResetBegin(const unsigned int t_security_id_) = 0;
  virtual void OnOrderResetEnd(const unsigned int t_security_id_) = 0;

  virtual void OnOrderExecWithTradeInfo(const uint32_t security_id, const uint64_t order_id, const uint8_t t_side_,
                                        const double exec_price, const uint32_t size_exec, bool intermediate) {}
  virtual void OnTradingStatus(const uint32_t security_id, std::string status) {}
};

class NTPPriceLevelGlobalListener {
 public:
  virtual ~NTPPriceLevelGlobalListener() {}

  virtual void OnPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                               const int t_level_added_, const double t_price_, const int t_new_size_,
                               const int t_new_ordercount_, const bool t_is_intermediate_message_) = 0;
  virtual void OnPriceLevelDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                  const int t_level_removed_, const double t_price_,
                                  const bool t_is_intermediate_message_) = 0;
  virtual void OnPriceLevelChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                  const int t_level_changed_, const double t_price_, const int t_new_size_,
                                  const int t_new_ordercount_, const bool t_is_intermediate_message_) = 0;
  virtual void OnPriceLevelDeleteThru(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                      const bool t_is_intermediate_message_) = 0;
  virtual void OnPriceLevelDeleteFrom(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                      const int t_max_level_removed_, const double t_price_,
                                      const bool t_is_intermediate_message_) = 0;
  virtual void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_) = 0;
  virtual void OnOTCTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_) = 0;
  virtual void OnMarketStatusUpdate(const unsigned int t_security_id_, const MktStatus_t t_this_mkt_status_) = 0;
  virtual void OnPriceLevelUpdate(void* ptr_to_price_level_update, int length_of_bytes,
                                  HFSAT::MDS_MSG::MDSMessageExchType exch_type) {}
};

class KRXPriceLevelGlobalListener {
 public:
  virtual ~KRXPriceLevelGlobalListener() {}

  virtual void OnPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                               const int t_level_added_, const double t_price_, const int t_new_size_,
                               const int t_new_ordercount_, const bool t_is_intermediate_message_) = 0;
  virtual void OnPriceLevelDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                  const int t_level_removed_, const double t_price_,
                                  const bool t_is_intermediate_message_) = 0;
  virtual void OnPriceLevelChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                  const int t_level_changed_, const double t_price_, const int t_new_size_,
                                  const int t_new_ordercount_, const bool t_is_intermediate_message_) = 0;
  virtual void OnPriceLevelDeleteThru(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                      const bool t_is_intermediate_message_) = 0;
  virtual void OnPriceLevelDeleteFrom(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                      const int t_max_level_removed_, const double t_price_,
                                      const bool t_is_intermediate_message_) = 0;
  virtual void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_) = 0;
  virtual void OnOTCTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_) = 0;
  virtual void OnMarketStatusUpdate(const unsigned int t_security_id_, const MktStatus_t t_this_mkt_status_) = 0;
};

class CFEPriceLevelGlobalListener {
 public:
  virtual ~CFEPriceLevelGlobalListener() {}

  virtual void OnPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                               const int t_level_added_, const double t_price_, const int t_new_size_,
                               const int t_new_ordercount_, const bool t_is_intermediate_message_) = 0;
  virtual void OnPriceLevelDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                  const int t_level_removed_, const double t_price_,
                                  const bool t_is_intermediate_message_) = 0;
  virtual void OnPriceLevelChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                  const int t_level_changed_, const double t_price_, const int t_new_size_,
                                  const int t_new_ordercount_, const bool t_is_intermediate_message_) = 0;
  virtual void OnPriceLevelOverlay(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                   const int t_level_overlayed_, const double t_price_, const int t_new_size_,
                                   const int t_new_ordercount_, const bool t_is_intermediate_message_) = 0;
  virtual void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_) = 0;
  virtual void OnSpreadTrade(const unsigned int t_security_id_, const double t_trade_price_,
                             const int t_trade_size_) = 0;
  virtual void OnPriceLevelUpdate(void* ptr_to_price_level_update, int length_of_bytes,
                                  HFSAT::MDS_MSG::MDSMessageExchType exch_type) {}
};

class BMFFPGAFullBookGlobalListener {
 public:
  virtual ~BMFFPGAFullBookGlobalListener() {}

  virtual void OnFullBookChange(const unsigned int security_id, FPGA_MDS::BMFFPGABookDeltaStruct* full_book,
                                bool is_intermediate, bool is_mkt_closed) = 0;
  virtual void OnTrade(const unsigned int security_id, const double trade_price, const int trade_size,
                       TradeType_t buy_sell, bool is_mkt_closed) = 0;
  virtual void OnMarketStatusUpdate(const unsigned int security_id, const MktStatus_t mkt_status) = 0;
  virtual void OnPriceLevelUpdate(void* ptr_to_price_level_update, int length_of_bytes,
                                  HFSAT::MDS_MSG::MDSMessageExchType exch_type) {}
};

struct FPGAHalfBook {
  double prices_[5];
  int sizes_[5];
  int16_t num_orders_[5];
};

class FPGAHalfBookGlobalListener {
 public:
  virtual ~FPGAHalfBookGlobalListener() {}

  virtual void OnHalfBookChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                FPGAHalfBook* t_full_book_, bool is_intermediate_) = 0;
  virtual void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
                       const TradeType_t t_buysell_) = 0;
};

struct FullBook {
  double bid_price_[FULL_BOOK_DEPTH];
  int bid_size_[FULL_BOOK_DEPTH];
  int bid_ordercount_[FULL_BOOK_DEPTH];
  double ask_price_[FULL_BOOK_DEPTH];
  int ask_size_[FULL_BOOK_DEPTH];
  int ask_ordercount_[FULL_BOOK_DEPTH];

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    for (auto i = 0u; i < 5; i++) {
      t_temp_oss_ << "( " << std::setw(4) << std::setfill(' ') << bid_ordercount_[i] << " " << std::setw(4)
                  << std::setfill(' ') << bid_size_[i] << " " << std::setw(8) << std::setfill(' ')
                  << std::setprecision(2) << std::fixed << bid_price_[i] << "    X    " << std::setprecision(2)
                  << std::fixed << ask_price_[i] << " " << std::setw(6) << std::setfill(' ') << ask_size_[i] << " "
                  << std::setw(6) << std::setfill(' ') << ask_ordercount_[i] << std::setw(4) << std::setfill(' ')
                  << " )\n";
    }
    return t_temp_oss_.str();
  }
};

class FullBookGlobalListener {
 public:
  virtual ~FullBookGlobalListener() {}

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_full_book_ reference to the structure sent bythe exch
   */
  virtual void OnFullBookChange(const unsigned int t_security_id_, const FullBook* t_full_book_) = 0;

  virtual void OnL1Change(const unsigned int t_security_id_, double bid_price, int bid_size, int num_bid_orders,
                          double ask_price, int ask_size, int num_ask_orders) = 0;
  virtual void OnL1Change(const unsigned int t_security_id_, const TradeType_t t_buysell_, const double t_price_,
                          const int t_new_size_, const int t_new_ordercount_, const bool t_is_intermediate_message_) {}

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_trade_price_ price at which trade was matched
   * @param t_trade_size_ size matched in this trade
   * @param t_buysell_ aggressive part side
   */
  virtual void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
                       const TradeType_t t_buysell_) = 0;
};

class HKHalfBookGlobalListener {
 public:
  virtual ~HKHalfBookGlobalListener() {}

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_full_book_ reference to the structure sent bythe exch
   */
  virtual void OnHKHalfBookChange(const unsigned int t_security_id_,
                                  const HKEX_MDS::HKEXBookStruct& t_hkex_book_struct_) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_trade_price_ price at which trade was matched
   * @param t_trade_size_ size matched in this trade
   */
  virtual void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_) = 0;
};

struct QuincyFullBook  // Change this as per the strct, curently using int prices with the struct
{
  int bid_price_[QUINCY_DEPTH_3];
  int bid_size_[QUINCY_DEPTH_3];
  int bid_ordercount_[QUINCY_DEPTH_3];
  int ask_price_[QUINCY_DEPTH_3];
  int ask_size_[QUINCY_DEPTH_3];
  int ask_ordercount_[QUINCY_DEPTH_3];
};

class QuincyFullBookGlobalListener {
 public:
  virtual ~QuincyFullBookGlobalListener() {}

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_full_book_ reference to the structure sent bythe exch
   */
  virtual void OnQuincyFullBookChange(const unsigned int t_security_id_, const QuincyFullBook* t_full_book_,
                                      double price_factor_ = 1.00) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_trade_price_ price at which trade was matched
   * @param t_trade_size_ size matched in this trade
   * @param t_buysell_ aggressive part side
   */
  virtual void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
                       const TradeType_t t_buysell_) = 0;
};

// Interface added to accomodate order depth books - BMF
class OrderLevelGlobalListener {
 public:
  virtual void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {}

  virtual ~OrderLevelGlobalListener() {}

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param order_id_ the order id of the market refresh if available otherwise -1.
   * @param t_buysell_ the side this is level is added to
   * @param t_level_added_ the level number ( post change ) that was added
   * @param t_price_ price of the level
   * @param t_new_size_ the new size after change
   * @param t_new_ordercount_ the new number of orders after change
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnOrderLevelNew(const unsigned int t_security_id_, uint64_t t_order_id_, const TradeType_t t_buysell_,
                               const int t_level_added_, const double t_price_, const int t_new_size_,
                               const int t_new_ordercount_, const bool t_is_intermediate_message_) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_buysell_ the side this is level is deleted from
   * @param t_level_removed_ the level removed
   * @param t_price_ price of the level
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnOrderLevelDelete(const unsigned int t_security_id_, uint64_t t_order_id_, const TradeType_t t_buysell_,
                                  const int t_level_removed_, const double t_price_,
                                  const bool t_is_intermediate_message_, const bool t_is_delete_on_change_ = false) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_buysell_ the side this is level is deleted from
   * @param t_level_changed_ the level number lchanged
   * @param t_price_ price of the level
   * @param t_new_size_ the new size after change
   * @param t_new_ordercount_ the new number of orders after change
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnOrderLevelChange(const unsigned int t_security_id_, uint64_t t_order_id_, const TradeType_t t_buysell_,
                                  const int t_level_changed_, const double t_price_, const int t_new_size_,
                                  const int t_new_ordercount_, const bool t_is_intermediate_message_) = 0;

  virtual void OnOrderLevelSnapNew(const unsigned int t_security_id_, uint64_t t_order_id_,
                                   const TradeType_t t_buysell_, const int t_level_added_, const double t_price_,
                                   const int t_new_size_, const int t_new_ordercount_,
                                   const bool t_is_intermediate_message_) = 0;
  virtual void OnOrderLevelSnapDelete(const unsigned int t_security_id_, uint64_t t_order_id_,
                                      const TradeType_t t_buysell_, const int t_level_removed_, const double t_price_,
                                      const bool t_is_intermediate_message_) = 0;
  virtual void OnOrderLevelSnapChange(const unsigned int t_security_id_, uint64_t t_order_id_,
                                      const TradeType_t t_buysell_, const int t_level_changed_, const double t_price_,
                                      const int t_new_size_, const int t_new_ordercount_,
                                      const bool t_is_intermediate_message_) = 0;

  virtual void resetBook(int t_security_id_) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_buysell_ the side this is level is deleted from
   * @param t_min_level_deleted_ starting at this level all higher ( away from market ) levels are to be removed
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnOrderLevelDeleteFrom(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                      const int t_min_level_deleted_, const bool t_is_intermediate_message_) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_buysell_ the side this is level is deleted from
   * @param t_max_level_deleted_ starting at top (0) level all levels upto this level are to be removed
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnOrderLevelDeleteThrough(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                         const int t_max_level_deleted_, const bool t_is_intermediate_message_) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_level_overlayed_ starting at top (0) level at which the change is being reported
   * @param bid_price_ new price on bid side at this level
   * @param bid_size_ new size on bid side at this level
   * @param bid_ordercount_ new number of orders on bid side at this level
   * @param ask_price_ new price on ask side at this level
   * @param ask_size_ new size on ask side at this level
   * @param ask_ordercount_ new number of orders on ask side at this level
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnOrderLevelOverlay(const unsigned int t_security_id_, const int t_level_overlayed_,
                                   const double bid_price_, const int bid_size_, const int bid_ordercount_,
                                   const double ask_price_, const int ask_size_, const int ask_ordercount_,
                                   const bool t_is_intermediate_message_) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_trade_price_ price at which trade was matched
   * @param t_trade_size_ size matched in this trade
   * @param t_buysell_ aggressive part side
   */
  virtual void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
                       const TradeType_t t_buysell_) = 0;
};

class PriceLevelOrderBookGlobalListener {
 public:
  virtual ~PriceLevelOrderBookGlobalListener() {}

  virtual void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {}

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param order_id_ the order id of the market refresh if available otherwise -1.
   * @param t_buysell_ the side this is level is added to
   * @param t_level_added_ the level number ( post change ) that was added
   * @param t_price_ price of the level
   * @param t_new_size_ the new size after change
   * @param t_new_ordercount_ the new number of orders after change
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnOrderLevelNew(const unsigned int t_security_id_, uint64_t t_order_id, const TradeType_t t_buysell_,
                               const int t_level_added_, const double t_price_, const int t_new_size_,
                               const int t_new_ordercount_, const bool t_is_intermediate_message_,
                               const bool is_change_on_trade_ = false) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_buysell_ the side this is level is deleted from
   * @param t_level_removed_ the level removed
   * @param t_price_ price of the level
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnOrderLevelDelete(const unsigned int t_security_id_, uint64_t t_order_id, const TradeType_t t_buysell_,
                                  const int t_level_removed_, const double t_price_,
                                  const bool t_is_intermediate_message_, const bool is_change_on_trade_ = false) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_buysell_ the side this is level is deleted from
   * @param t_level_changed_ the level number lchanged
   * @param t_price_ price of the level
   * @param t_new_size_ the new size after change
   * @param t_new_ordercount_ the new number of orders after change
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnOrderLevelChange(const unsigned int t_security_id_, uint64_t t_order_id, const TradeType_t t_buysell_,
                                  const int t_level_changed_, const double t_price_, const int t_new_size_,
                                  const int t_new_ordercount_, const bool t_is_intermediate_message_) = 0;

  virtual void OnOrderLevelSnapNew(const unsigned int t_security_id_, uint64_t t_order_id, const TradeType_t t_buysell_,
                                   const int t_level_added_, const double t_price_, const int t_new_size_,
                                   const int t_new_ordercount_, const bool t_is_intermediate_message_) = 0;
  virtual void OnOrderLevelSnapDelete(const unsigned int t_security_id_, uint64_t t_order_id,
                                      const TradeType_t t_buysell_, const int t_level_removed_, const double t_price_,
                                      const bool t_is_intermediate_message_) = 0;
  virtual void OnOrderLevelSnapChange(const unsigned int t_security_id_, uint64_t t_order_id,
                                      const TradeType_t t_buysell_, const int t_level_changed_, const double t_price_,
                                      const int t_new_size_, const int t_new_ordercount_,
                                      const bool t_is_intermediate_message_) = 0;

  virtual void resetBook(int t_security_id_) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_buysell_ the side this is level is deleted from
   * @param t_min_level_deleted_ starting at this level all higher ( away from market ) levels are to be removed
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnOrderLevelDeleteFrom(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                      const int t_min_level_deleted_, const bool t_is_intermediate_message_) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_buysell_ the side this is level is deleted from
   * @param t_max_level_deleted_ starting at top (0) level all levels upto this level are to be removed
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnOrderLevelDeleteThrough(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                         const int t_max_level_deleted_, const bool t_is_intermediate_message_) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_level_overlayed_ starting at top (0) level at which the change is being reported
   * @param bid_price_ new price on bid side at this level
   * @param bid_size_ new size on bid side at this level
   * @param bid_ordercount_ new number of orders on bid side at this level
   * @param ask_price_ new price on ask side at this level
   * @param ask_size_ new size on ask side at this level
   * @param ask_ordercount_ new number of orders on ask side at this level
   * @param t_is_intermediate_message_ indicates if this callback is an intermediate callback, and hence listeners
   * should not alerted
   */
  virtual void OnOrderLevelOverlay(const unsigned int t_security_id_, const int t_level_overlayed_,
                                   const double bid_price_, const int bid_size_, const int bid_ordercount_,
                                   const double ask_price_, const int ask_size_, const int ask_ordercount_,
                                   const bool t_is_intermediate_message_) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_trade_price_ price at which trade was matched
   * @param t_trade_size_ size matched in this trade
   * @param t_buysell_ aggressive part side
   */
  virtual void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
                       const TradeType_t t_buysell_) = 0;
};
class OrderLevelGlobalListenerOSE {
 public:
  virtual void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {}
  virtual ~OrderLevelGlobalListenerOSE() {}
  virtual void OnOrderLevelNew(const unsigned int t_security_id_, uint64_t t_order_id_, const TradeType_t t_buysell_,
                               const int t_level_added_, const int t_price_, const int t_new_size_,
                               const bool t_is_intermediate_message_) = 0;
  virtual void OnOrderLevelDelete(const unsigned int t_security_id_, uint64_t t_order_id_, const TradeType_t t_buysell_,
                                  const int t_level_removed_, const int t_price_,
                                  const bool t_is_intermediate_message_) = 0;
  virtual void OnOrderLevelChange(const unsigned int t_security_id_, uint64_t t_order_id_, const TradeType_t t_buysell_,
                                  const int t_level_changed_, const int t_new_price_, const int t_new_size_,
                                  const bool t_is_intermediate_message_) = 0;
  virtual void OnOrderLevelSnapNew(const unsigned int t_security_id_, uint64_t t_order_id_,
                                   const TradeType_t t_buysell_, const int t_level_added_, const int t_price_,
                                   const int t_new_size_, const bool t_is_intermediate_message_) = 0;
  virtual void resetBook(int t_security_id_) = 0;
  virtual void OnTrade(const unsigned int t_security_id_, const int t_trade_price_, const int t_trade_size_,
                       const TradeType_t t_buysell_) = 0;
};

class OrderGlobalListenerNSE {
 public:
  virtual ~OrderGlobalListenerNSE() {}

  virtual void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {}

  virtual void OnTradeExecRange(const unsigned int t_security_id_, double const t_low_exec_band_, 
                                double const t_high_exec_band_) = 0;
  virtual void OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t order_id_,
                          const double t_price_, const uint32_t t_size_, const bool t_is_intermediate_) = 0;
  virtual void OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t order_id_,
                             const double t_price_, const uint32_t t_size_) = 0;
  virtual void OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t order_id_,
                             const double t_price_, const bool t_delete_order_, const bool t_is_intermediate_) = 0;
  virtual void OnTrade(const unsigned int t_security_id_, double const t_trade_price_, const int t_trade_size_,
                       uint64_t const buy_order_num, uint64_t const sell_order_num) = 0;
  virtual void OnHiddenTrade(const unsigned int t_security_id_, double const t_trade_price_, const int t_trade_size_,
                             uint64_t const buy_order_num, uint64_t const sell_order_num) = 0;
  virtual void OnPriceLevelUpdate(void* ptr_to_price_level_update, int length_of_bytes,
                                  HFSAT::MDS_MSG::MDSMessageExchType exch_type) {}
};

template <typename T>
class OrderGlobalListener {
 public:
  virtual ~OrderGlobalListener() {}

  virtual void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {}

  virtual void Process(const unsigned int security_id, T* next_event_) {}
};

class NSEFullBookGlobalListener {
 public:
  virtual ~NSEFullBookGlobalListener() {}

  virtual void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {}

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_full_book_ reference to the structure sent bythe exch
   */
  virtual void OnNSEFullBookChange(const unsigned int t_security_id_,
                                   const NSE_UDP_MDS::NSEBookStruct* t_full_book_) = 0;

  /**
   * @param t_security_id_ security_id_ of the exchange symbol this update was about
   * @param t_trade_price_ price at which trade was matched
   * @param t_trade_size_ size matched in this trade
   * @param t_buysell_ aggressive part side
   */
  virtual void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
                       const TradeType_t t_buysell_) = 0;
};

class OrderGlobalListenerEOBI {
 public:
  virtual ~OrderGlobalListenerEOBI() {}

  virtual void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {}

  virtual void OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_, const double t_price_,
                          const uint32_t t_size_, const bool t_is_intermediate_) = 0;

  virtual void OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_, const double t_price_,
                             const uint32_t t_size_, const double t_prev_price_, const uint32_t t_prev_size_) = 0;

  virtual void OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_, const double t_price_,
                             const int t_size_, const bool t_delete_order_, const bool t_is_intermediate_) = 0;

  virtual void OnOrderMassDelete(const uint32_t t_security_id_) = 0;

  virtual void OnPartialOrderExecution(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                       const double t_traded_price_, const uint32_t t_traded_size_) = 0;

  virtual void OnFullOrderExecution(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                    const double t_traded_price_, const uint32_t t_traded_size_) = 0;

  virtual void OnExecutionSummary(const uint32_t t_security_id_, TradeType_t t_aggressor_side_, const double t_price_,
                                  const uint32_t t_size_) = 0;
};

class OrderLevelGlobalListenerHKOMD {
 public:
  virtual ~OrderLevelGlobalListenerHKOMD() {}

  virtual void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {}

  virtual void OnOrderAdd(const uint32_t t_security_id_, const uint64_t t_order_id_, const double t_price_,
                          const uint32_t t_quantity_, const TradeType_t t_buy_sell_, bool t_intermediate_) = 0;

  virtual void OnOrderModify(const uint32_t t_security_id_, const uint64_t t_order_id_, const double t_price_,
                             const uint32_t t_quantity_, const TradeType_t t_buy_sell_, bool t_intermediate_) = 0;

  virtual void OnOrderDelete(const uint32_t t_security_id_, const uint64_t t_order_id_, const TradeType_t t_buy_sell_,
                             bool t_is_delete_order_, bool t_intermediate_) = 0;

  virtual void OnTrade(const uint32_t t_security_id_, const uint64_t t_order_id_, const double t_price_,
                       const uint32_t t_quantity_, const TradeType_t t_buy_sell_) = 0;
};

class OrderLevelListenerSim {
 public:
  virtual ~OrderLevelListenerSim() {}
  virtual void SetTimeToSkipUntilFirstEvent(const ttime_t t_start_time_) {}
  virtual void OnOrderAdd(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                          const int64_t t_order_id_, const double t_price_, const int t_size_) = 0;
  virtual void OnOrderModify(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                             const int64_t t_old_order_id_, const double t_old_price_, const int t_old_size_,
                             const int64_t t_new_order_id_, const double t_new_price_, const int t_new_size_) = 0;
  virtual void OnOrderDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                             const int64_t t_order_id_, const double t_price_, const int t_size_) = 0;
  virtual void OnOrderExec(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                           const int64_t t_order_id_, const double t_price_, const int t_size_executed_,
                           const int t_size_remaining_) = 0;

  virtual void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
                       const TradeType_t t_buysell_) = 0;
  virtual void ResetBook(const unsigned int t_security_id_) = 0;
  virtual void OnOrderLevelDeleteFrom(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                      const int t_min_level_deleted_, const bool t_is_intermediate_message_) {}
  virtual void OnStatusChange(MktStatus_t _this_market_status_) {}
};

class TradeGlobalListener {
 public:
  virtual ~TradeGlobalListener() {}

  virtual void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
                       const TradeType_t t_buysell_) = 0;
};

class L1DataListener {
 public:
  virtual void OnL1New(const unsigned int t_, const GenericL1DataStruct& l1_data) = 0;
  virtual void OnTrade(const unsigned int security_, const GenericL1DataStruct& l1_data) = 0;
  virtual void SetTimeToSkipUntilFirstEvent(const ttime_t start_time) = 0;

  virtual ~L1DataListener(){};
};
}
