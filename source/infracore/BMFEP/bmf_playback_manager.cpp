#include "infracore/BMFEP/bmf_playback_manager.hpp"

namespace HFSAT {
namespace ORS {

const std::vector<BMFPlaybackStruct> BMFPlaybackManager::GetLiveOrders() const {
  std::vector<BMFPlaybackStruct> live_orders;
  for (auto iter = order_details_.begin(); iter != order_details_.end(); iter++) {
    live_orders.push_back(*(static_cast<BMFPlaybackStruct *>(iter->second)));
  }
  return live_orders;
}

void BMFPlaybackManager::AddOrderDetails(BasePlaybackStruct &base_data) {
  order_details_[base_data.origClOrdID] = new BMFPlaybackStruct(static_cast<BMFPlaybackStruct &>(base_data));
}

void BMFPlaybackManager::DelOrderDetails(unsigned long long id) {
  delete (static_cast<BMFPlaybackStruct *>(order_details_[id]));
  order_details_.erase(id);
}
}
}
