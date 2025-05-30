#include "infracore/BasicOrderRoutingServer/base_playback_manager.hpp"

#include <iostream>

namespace HFSAT {
namespace ORS {

void BasePlaybackManager::OrderNew(BasePlaybackStruct &base_data) {
  if (order_details_.find(base_data.origClOrdID) == order_details_.end()) {
    AddOrderDetails(base_data);
  } else {
    std::cerr << "New Order Already present with the same origClOrdID. Ignoring this new order. Details : \n";
    std::cerr << "\nSide" << base_data.side << "\nSize : " << base_data.size << "\n";
  }
}
void BasePlaybackManager::OrderCancel(BasePlaybackStruct &base_data) {
  if (order_details_.find(base_data.origClOrdID) != order_details_.end()) {
    DelOrderDetails(base_data.origClOrdID);
  } else {
    std::cerr << "Order to Cancel not placed. Ignoring this cancel order. Details : \n";
    std::cerr << "\nSide" << base_data.side << "\nSize : " << base_data.size << "\n";
  }
}
void BasePlaybackManager::OrderCancelReplace(BasePlaybackStruct &base_data) {
  if (order_details_.find(base_data.origClOrdID) != order_details_.end()) {
    DelOrderDetails(base_data.origClOrdID);
    AddOrderDetails(base_data);
  } else {
    std::cerr << "No Order to cancel. Ignoring Cancel for this CancelReplace. Details : \n";
    std::cerr << "\nSide" << base_data.side << "\nSize : " << base_data.size << "\n";
  }
}
void BasePlaybackManager::OrderExec(BasePlaybackStruct &base_data) {
  if (order_details_.find(base_data.origClOrdID) != order_details_.end()) {
    if (order_details_[base_data.origClOrdID]->size <= base_data.size) {
      // Full Execution
      if (order_details_[base_data.origClOrdID]->size < base_data.size) {
        std::cerr << "Execution Size != Remaining size.\n";
        std::cerr << "\nSide" << base_data.side << "\nExec Size : " << base_data.size
                  << "\nRemaining Size : " << order_details_[base_data.origClOrdID]->size << "\n";
      }
      DelOrderDetails(base_data.origClOrdID);
    } else {
      // Partial Execution
      order_details_[base_data.origClOrdID]->size -= base_data.size;
    }

    if (base_data.side == 1) {  // Buy
      positions_ += base_data.size;
    } else if (base_data.side == 2) {  // Sell
      positions_ -= base_data.size;
    } else {
      std::cerr << "Unknown Side : " << base_data.side << "\n";
    }
  } else {
    std::cerr << "No Order to Execute. Ignoring. Details : \n";
    std::cerr << "\nSide" << base_data.side << "\nSize : " << base_data.size << "\n";
  }
}

std::vector<BasePlaybackStruct> BasePlaybackManager::GetLiveOrders() const {
  std::vector<BasePlaybackStruct> live_orders;
  for (auto iter = order_details_.begin(); iter != order_details_.end(); iter++) {
    live_orders.push_back(*(iter->second));
  }
  return live_orders;
}

void BasePlaybackManager::CleanUp() {
  for (auto iter = order_details_.begin(); iter != order_details_.end(); iter++) {
    if (iter->second) {
      DelOrderDetails(iter->first);
    }
  }
}
}
}
