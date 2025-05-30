#ifndef _BASE_PLAYBACK_MANAGER_
#define _BASE_PLAYBACK_MANAGER_

#include "infracore/BasicOrderRoutingServer/playback_defines.hpp"

#include <map>
#include <vector>

namespace HFSAT {
namespace ORS {

class BasePlaybackManager {
 public:
  BasePlaybackManager() : positions_(0) {}
  virtual ~BasePlaybackManager() { CleanUp(); }
  void OrderNew(BasePlaybackStruct &base_data);
  void OrderCancel(BasePlaybackStruct &base_data);
  void OrderCancelReplace(BasePlaybackStruct &base_data);
  void OrderExec(BasePlaybackStruct &base_data);

  const int GetLivePositions() const { return positions_; }
  std::vector<BasePlaybackStruct> GetLiveOrders() const;

 protected:
  virtual void AddOrderDetails(BasePlaybackStruct &base_data) {
    order_details_[base_data.origClOrdID] = new BasePlaybackStruct(base_data);
  }
  virtual void DelOrderDetails(unsigned long long id) {
    delete order_details_[id];
    order_details_.erase(id);
  }
  void CleanUp();

 protected:
  std::map<unsigned long long, BasePlaybackStruct *> order_details_;

 private:
  int positions_;
};
}
}
#endif
