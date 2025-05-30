#ifndef _LIVE_PRODUCTS_MANAGER_HPP_
#define _LIVE_PRODUCTS_MANAGER_HPP_

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "dvccode/CombinedControlUtils/combined_control_message_listener.hpp"
#include "dvccode/Listeners/live_products_manager_listener.hpp"

namespace HFSAT {

class LiveProductsManager : public CombinedControlMessageListener {
 public:
  static LiveProductsManager& GetUniqueInstance();
  static void ResetUniqueInstance();

  void AddListener(LiveProductsManagerListener* listener);
  void RemoveListener(LiveProductsManagerListener* listener);

 private:
  LiveProductsManager();
  ~LiveProductsManager();

  void NotifyListeners(LiveProductsChangeAction action, std::string exchange_symbol, std::string shortcode,
                       ExchSource_t exchange);
  void OnCombinedControlMessageReceived(CombinedControlMessage combined_control_request_);

  void AddProduct(int query_id, const std::vector<std::string>& shortcodes);
  void RemoveProduct(int query_id, const std::vector<std::string> shortcodes);

  const char* GetExchangeSymbol(const std::string& shortcode);

  static LiveProductsManager* uniqueinstance_;
  std::map<std::string, std::set<int> > shortcode_to_query_ids_;
  std::map<int, std::set<std::string> > query_id_to_shortcodes_;
  std::vector<LiveProductsManagerListener*> listeners_;
};
}

#endif
