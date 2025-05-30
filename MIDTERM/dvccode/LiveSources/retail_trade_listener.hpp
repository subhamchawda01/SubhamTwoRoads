/*
 * retail_trade_listener.hpp
 *
 *  Created on: Jun 17, 2015
 *      Author: archit
 */

#ifndef MDSMESSAGES_RETAIL_TRADE_LISTENER_HPP_
#define MDSMESSAGES_RETAIL_TRADE_LISTENER_HPP_

namespace HFSAT {

/// Interface of listeners to All OrderReuests from GUI,
/// Implemented by Indicators/Execlogics to react on large GUI trade requests
class RetailTradeListener {
 public:
  virtual ~RetailTradeListener();
  virtual void OnRetailTradeRequest(const char* _secname_, double _price_, TradeType_t r_buysell_,
                                    unsigned int _size_executed_, unsigned int _quoted_size_,
                                    unsigned int _requested_size_) = 0;
};

/// Interface of listeners to OrdersExecuted from GUI
/// RetailTrading implements this interface
class FPOrderExecutedListener {
 public:
  virtual ~FPOrderExecutedListener(){};
  virtual void FPOrderExecuted(const char* _secname_, double _price_, TradeType_t r_buysell_, int _size_executed_) = 0;
};

} /* namespace HFSAT */

#endif /* MDSMESSAGES_RETAIL_TRADE_LISTENER_HPP_ */
