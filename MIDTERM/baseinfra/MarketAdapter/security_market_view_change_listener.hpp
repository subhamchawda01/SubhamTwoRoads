/**
   \file MarketAdapter/security_market_view_change_listener.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_MARKETADAPTER_SECURITY_MARKET_VIEW_CHANGE_LISTENER_H
#define BASE_MARKETADAPTER_SECURITY_MARKET_VIEW_CHANGE_LISTENER_H

#include "baseinfra/MarketAdapter/basic_market_view_structs.hpp"

namespace HFSAT {

// Currently it's used to notify last_traded_price update from MSR feed in MICEX
class FastTradeListener {
 public:
  virtual ~FastTradeListener() {}
  virtual void OnFastTradeUpdate(const unsigned int t_security_id_, TradeType_t t_aggressive_side,
                                 double t_last_trade_price_) {}
};

typedef std::vector<FastTradeListener*> FastTradeListenerVec;

/// For L1 change listeners. We should not be interested to use levels > 0 for any computations since the mkt_book
/// may not be in sanitized state
class SecurityMarketViewChangeListener {
 public:
  virtual ~SecurityMarketViewChangeListener() {}
  virtual void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) = 0;
  virtual void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                            const MarketUpdateInfo& _market_update_info_) = 0;
};

typedef std::vector<SecurityMarketViewChangeListener*> SMVChangeListenerVec;

/// Exclusive for L2 only change listeners.
class SecurityMarketViewL2ChangeListener {
 public:
  virtual ~SecurityMarketViewL2ChangeListener() {}
  virtual void OnMarketUpdateL2(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) = 0;
};

/// For Raw TradePrints
class SecurityMarketViewRawTradesListener {
 public:
  virtual ~SecurityMarketViewRawTradesListener() {}
  virtual void OnRawTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                               const MarketUpdateInfo& _market_update_info_) = 0;
};
typedef std::vector<SecurityMarketViewRawTradesListener*> SMVRawTradeListenerVec;

typedef std::vector<SecurityMarketViewL2ChangeListener*> SMVL2ChangeListenerVec;

class SecurityMarketViewOnReadyListener {
 public:
  virtual ~SecurityMarketViewOnReadyListener() {}
  virtual void SMVOnReady() = 0;
};

typedef std::vector<SecurityMarketViewOnReadyListener*> SMVOnReadyListenerVec;

class SecurityMarketViewStatusListener {
 public:
  virtual ~SecurityMarketViewStatusListener() {}
  virtual void OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_) = 0;
};

typedef std::vector<SecurityMarketViewStatusListener*> SMVStatusListenerVec;

class SMVOffMarketTradeListener {
 public:
  virtual ~SMVOffMarketTradeListener() {}
  virtual void OnOffMarketTrade(const unsigned int _security_id_, const double _price_, const int _int_price_,
                                const int _trade_size_, const TradeType_t _buysell_, int _lvl_) {}
};

typedef std::vector<SMVOffMarketTradeListener*> SMVOffMarketTradeListenerVec;

/// Only difference from GlobalOrderChangeListener is that this is meant to be
/// an indicator which is passed on this info from SMV
class SMVPLChangeListener {
 public:
  virtual ~SMVPLChangeListener() {}
  virtual void OnPLNew(const unsigned int t_security_id_, const MarketUpdateInfo& r_market_update_info_,
                       const TradeType_t t_buysell_, const int t_level_added_, const int t_old_size_,
                       const int t_new_size_, const int t_old_ordercount_, const int t_new_ordercount_,
                       const int t_int_price_, const int t_int_price_level_, const bool t_is_intermediate_message_) = 0;

  virtual void OnPLDelete(const unsigned int t_security_id_, const MarketUpdateInfo& r_market_update_info_,
                          const TradeType_t t_buysell_, const int t_level_removed_, const int t_old_size_,
                          const int t_old_ordercount_, const int t_int_price_, const int t_int_price_level_,
                          const bool t_is_intermediate_message_) = 0;

  virtual void OnPLChange(const unsigned int t_security_id_, const MarketUpdateInfo& r_market_update_info_,
                          const TradeType_t t_buysell_, const int t_level_changed_, const int t_int_price_,
                          const int t_int_price_level_, const int t_old_size_, const int t_new_size_,
                          const int t_old_ordercount_, const int t_new_ordercount_,
                          const bool t_is_intermediate_message_) = 0;
};

typedef std::vector<SMVPLChangeListener*> SMVPLChangeListenerVec;

class SMVPreTradeListener {
 public:
  virtual ~SMVPreTradeListener(){};
  virtual void SMVPreTrade() = 0;
  virtual void OnSMVPreTrade(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                             const MarketUpdateInfo& _market_update_info_) {}
};

typedef std::vector<SMVPreTradeListener*> SMVPreTradeListenerVec;
}

#endif  // BASE_MARKETADAPTER_SECURITY_MARKET_VIEW_CHANGE_LISTENER_H
