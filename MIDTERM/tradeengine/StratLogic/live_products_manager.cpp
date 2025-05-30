#include <unistd.h>

#include "infracore/Tools/live_products_manager.hpp"

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CombinedControlUtils/combined_control_messages.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

namespace HFSAT {

#define DEBUG_LOGGING 0

LiveProductsManager* LiveProductsManager::uniqueinstance_ = nullptr;

LiveProductsManager::LiveProductsManager() {}

LiveProductsManager::~LiveProductsManager() {}

LiveProductsManager& LiveProductsManager::GetUniqueInstance() {
  if (uniqueinstance_ == nullptr) {
    uniqueinstance_ = new LiveProductsManager();
  }
  return *(uniqueinstance_);
}

void LiveProductsManager::ResetUniqueInstance() {
  if (uniqueinstance_ != nullptr) {
    delete uniqueinstance_;
    uniqueinstance_ = nullptr;
  }
}

void LiveProductsManager::OnCombinedControlMessageReceived(CombinedControlMessage combined_control_request_) {
  char hostname[64];
  hostname[63] = '\0';
  gethostname(hostname, 63);

  if (strcmp(hostname, combined_control_request_.location_) != 0) {
    return;
  }

  if (combined_control_request_.message_code_ == HFSAT::kCmdControlMessageCodeAddRemoveShortcode) {
#if DEBUG_LOGGING
    std::cout << combined_control_request_.generic_combined_control_msg_.add_rm_shortcode_.ToString() << std::endl;
#endif
    if (combined_control_request_.generic_combined_control_msg_.add_rm_shortcode_.signal_ ==
        HFSAT::SignalType::kSignalAdd) {
      const int query_id = combined_control_request_.generic_combined_control_msg_.add_rm_shortcode_.query_id_;
      std::vector<std::string> shortcodes =
          combined_control_request_.generic_combined_control_msg_.add_rm_shortcode_.GetShortcodes();
      AddProduct(query_id, shortcodes);
    } else if (combined_control_request_.generic_combined_control_msg_.add_rm_shortcode_.signal_ ==
               HFSAT::SignalType::kSignalRemove) {
      const int query_id = combined_control_request_.generic_combined_control_msg_.add_rm_shortcode_.query_id_;
      std::vector<std::string> shortcodes =
          combined_control_request_.generic_combined_control_msg_.add_rm_shortcode_.GetShortcodes();
      RemoveProduct(query_id, shortcodes);
    } else if (combined_control_request_.generic_combined_control_msg_.add_rm_shortcode_.signal_ ==
               HFSAT::SignalType::kSignalShow) {
      std::cout << "ReqBasedCombinedWriter State (Shortcode and List of Query Ids): " << std::endl;
      for (auto iter = shortcode_to_query_ids_.begin(); iter != shortcode_to_query_ids_.end(); iter++) {
        const std::set<int>& query_ids = iter->second;
        if (query_ids.empty()) {
          continue;
        }
        std::cout << (iter->first) << ": ";
        for (auto ids_iter = query_ids.begin(); ids_iter != query_ids.end(); ids_iter++) {
          std::cout << *ids_iter << " ";
        }
        std::cout << std::endl;
      }
      std::cout << "ReqBasedCombinedWriter State Complete" << std::endl;
    }
  }
}

void LiveProductsManager::AddProduct(int query_id, const std::vector<std::string>& shortcodes) {
  for (const std::string& shortcode : shortcodes) {
    if (!HFSAT::SecurityDefinitions::GetUniqueInstance().IsValidContract(shortcode)) {
      std::cout << "ReqCW: LiveProductsManager Invalid Shortcode Received : " << shortcode << ". Ignoring."
                << std::endl;
      continue;
    }
    if (query_id_to_shortcodes_.find(query_id) == query_id_to_shortcodes_.end() ||
        query_id_to_shortcodes_[query_id].find(shortcode) == query_id_to_shortcodes_[query_id].end()) {
      query_id_to_shortcodes_[query_id].insert(shortcode);

      if (shortcode_to_query_ids_.find(shortcode) == shortcode_to_query_ids_.end() ||
          shortcode_to_query_ids_[shortcode].empty()) {
        ExchSource_t exchange = HFSAT::SecurityDefinitions::GetUniqueInstance().GetContractExchSource(shortcode);
        const char* exchange_symbol = GetExchangeSymbol(shortcode);
        if (exchange_symbol != nullptr) {
          NotifyListeners(LiveProductsChangeAction::kAdd, exchange_symbol, shortcode, exchange);
        }
      }
#if DEBUG_LOGGING
      else {
        std::cout << "ReqCW: LiveProductsManager: Shortcode " << shortcode << " already requested." << std::endl;
      }
#endif
      shortcode_to_query_ids_[shortcode].insert(query_id);
    }
#if DEBUG_LOGGING
    else {
      std::cout << "ReqCW: LiveProductsManager: Shortcode already added for QueryId: " << query_id << std::endl;
    }
#endif
  }
}

void LiveProductsManager::RemoveProduct(int query_id, const std::vector<std::string> shortcodes) {
  for (const std::string& shortcode : shortcodes) {
    if (!HFSAT::SecurityDefinitions::GetUniqueInstance().IsValidContract(shortcode)) {
      std::cout << "ReqCW: LiveProductsManager Invalid Shortcode Received : " << shortcode << ". Ignoring."
                << std::endl;
      continue;
    }
    if (shortcode_to_query_ids_.find(shortcode) == shortcode_to_query_ids_.end() ||
        shortcode_to_query_ids_[shortcode].find(query_id) == shortcode_to_query_ids_[shortcode].end()) {
#if DEBUG_LOGGING
      std::cout << "ReqCW: LiveProductsManager: Shortcode " << shortcode << " not in global requested list or in Query "
                << query_id << " Requested List." << std::endl;
#endif
      continue;
    }

    if (query_id_to_shortcodes_.find(query_id) != query_id_to_shortcodes_.end() &&
        query_id_to_shortcodes_[query_id].find(shortcode) != query_id_to_shortcodes_[query_id].end()) {
      query_id_to_shortcodes_[query_id].erase(shortcode);
    }
#if DEBUG_LOGGING
    else {
      std::cout << "ReqCW: LiveProductsManager: Magically shortcode " << shortcode
                << " not present in internal maps for Query: " << query_id << std::endl;
    }
#endif

    shortcode_to_query_ids_[shortcode].erase(query_id);

    if (shortcode_to_query_ids_.find(shortcode) == shortcode_to_query_ids_.end() ||
        shortcode_to_query_ids_[shortcode].empty()) {
      ExchSource_t exchange = HFSAT::SecurityDefinitions::GetUniqueInstance().GetContractExchSource(shortcode);
      const char* exchange_symbol = GetExchangeSymbol(shortcode);
      if (exchange_symbol != nullptr) {
        NotifyListeners(LiveProductsChangeAction::kRemove, exchange_symbol, shortcode, exchange);
      }
    }
  }
}

const char* LiveProductsManager::GetExchangeSymbol(const std::string& shortcode) {
  const char* exchange_symbol = nullptr;
  try {
    exchange_symbol = ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(shortcode);

  } catch (...) {
    std::cerr << " ReqCW: LiveProductsManager: Error in ExchangeSymbol Conversion for Shortcode : " << shortcode
              << std::endl;
  }

  return exchange_symbol;
}

void LiveProductsManager::AddListener(LiveProductsManagerListener* listener) {
  HFSAT::VectorUtils::UniqueVectorAdd(listeners_, listener);
}

void LiveProductsManager::RemoveListener(LiveProductsManagerListener* listener) {
  HFSAT::VectorUtils::UniqueVectorRemove(listeners_, listener);
}

void LiveProductsManager::NotifyListeners(LiveProductsChangeAction action, std::string exchange_symbol,
                                          std::string shortcode, ExchSource_t exchange) {
  for (auto listener : listeners_) {
    listener->OnLiveProductChange(action, exchange_symbol, shortcode, exchange);
  }
}
}  // namespace HFSAT
