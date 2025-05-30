#ifndef _BMF_PLAYBACK_MANAGER_
#define _BMF_PLAYBACK_MANAGER_

#include "infracore/BasicOrderRoutingServer/base_playback_manager.hpp"
#include "infracore/BMFEP/bmf_playback_defines.hpp"

#include <map>
#include <vector>

namespace HFSAT {
namespace ORS {

class BMFPlaybackManager : protected BasePlaybackManager {
 public:
  ~BMFPlaybackManager() { CleanUp(); }
  void OrderNew(BMFPlaybackStruct &bmf_data) { BasePlaybackManager::OrderNew(bmf_data); }
  void OrderCancel(BMFPlaybackStruct &bmf_data) { BasePlaybackManager::OrderCancel(bmf_data); }
  void OrderCancelReplace(BMFPlaybackStruct &bmf_data) { BasePlaybackManager::OrderCancelReplace(bmf_data); }
  void OrderExec(BMFPlaybackStruct &bmf_data) { BasePlaybackManager::OrderExec(bmf_data); }

  const int GetLivePositions() const { return BasePlaybackManager::GetLivePositions(); }
  const std::vector<BMFPlaybackStruct> GetLiveOrders() const;

 protected:
  virtual void AddOrderDetails(BasePlaybackStruct &base_data);
  virtual void DelOrderDetails(unsigned long long id);
};
}
}
#endif
