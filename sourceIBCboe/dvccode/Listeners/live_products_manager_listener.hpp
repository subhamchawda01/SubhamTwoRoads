#ifndef _LIVE_PRODUCTS_MANAGER_LISTENER_HPP_
#define _LIVE_PRODUCTS_MANAGER_LISTENER_HPP_

#include <string>

namespace HFSAT {

enum LiveProductsChangeAction { kInvalid = 0, kAdd = 1, kRemove = 2 };

class LiveProductsManagerListener {
 public:
  virtual void OnLiveProductChange(LiveProductsChangeAction action, std::string exchange_symbol, std::string shortcode,
                                   ExchSource_t exchange) = 0;
  virtual ~LiveProductsManagerListener() {}
};
}

#endif
