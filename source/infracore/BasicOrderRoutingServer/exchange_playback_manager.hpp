#ifndef _EXCHANGE_PLAYBACK_MANAGER_
#define _EXCHANGE_PLAYBACK_MANAGER_

#include "dvccode/CDef/defines.hpp"
#include "infracore/BMFEP/bmf_playback_manager.hpp"
#include "infracore/BMFEP/bmf_playback_defines.hpp"

#include <map>
#include <string>
#include <vector>

namespace HFSAT {
namespace ORS {

template <typename T, typename S>
class ExchangePlaybackManager {
 public:
  ExchangePlaybackManager() : playback_manager(nullptr) { playback_manager = new T(); }
  ~ExchangePlaybackManager() { delete playback_manager; }

  void OrderNew(S &data) {
    if (playback_manager) {
      playback_manager->OrderNew(data);
    }
  }
  void OrderCancel(S &data) {
    if (playback_manager) {
      playback_manager->OrderCancel(data);
    }
  }
  void OrderCancelReplace(S &data) {
    if (playback_manager) {
      playback_manager->OrderCancelReplace(data);
    }
  }
  void OrderExec(S &data) {
    if (playback_manager) {
      playback_manager->OrderExec(data);
    }
  }

  const int GetLivePositions() const {
    if (playback_manager) {
      return playback_manager->GetLivePositions();
    }
    return 0;
  }

  const std::vector<S> GetLiveOrders() const {
    if (playback_manager) {
      return playback_manager->GetLiveOrders();
    }
    std::vector<S> live_orders;
    return live_orders;
  }

 private:
  ExchangePlaybackManager(const ExchangePlaybackManager &) : playback_manager(nullptr) {}

  T *playback_manager;
};

template <typename T, typename S>
class ExchangePlaybackManagerSet {
 public:
  ExchangePlaybackManagerSet() {}
  ~ExchangePlaybackManagerSet() { ResetAllInstances(); }

  void OrderNew(int security_id, S &data) { GetInstance(security_id).OrderNew(data); }
  void OrderCancel(int security_id, S &data) { GetInstance(security_id).OrderCancel(data); }
  void OrderCancelReplace(int security_id, S &data) { GetInstance(security_id).OrderCancelReplace(data); }
  void OrderExec(int security_id, S &data) { GetInstance(security_id).OrderExec(data); }
  const int GetLivePositions(int security_id) { return GetInstance(security_id).GetLivePositions(); }
  const std::vector<S> GetLiveOrders(int security_id) { return GetInstance(security_id).GetLiveOrders(); }

  const std::map<int, std::vector<S> > GetAllLiveOrders() {
    std::map<int, std::vector<S> > live_orders;
    for (auto iter = sec_id_to_exch_playback_mgr.begin(); iter != sec_id_to_exch_playback_mgr.end(); iter++) {
      live_orders[iter->first] = (*(sec_id_to_exch_playback_mgr[iter->first])).GetLiveOrders();
    }
    return live_orders;
  }

  const std::map<int, int> GetAllLivePositions() {
    std::map<int, int> live_positions;
    for (auto iter = sec_id_to_exch_playback_mgr.begin(); iter != sec_id_to_exch_playback_mgr.end(); iter++) {
      live_positions[iter->first] = (*(sec_id_to_exch_playback_mgr[iter->first])).GetLivePositions();
    }
    return live_positions;
  }

  void ResetInstance(const int security_id) {
    if (sec_id_to_exch_playback_mgr.find(security_id) != sec_id_to_exch_playback_mgr.end()) {
      if (sec_id_to_exch_playback_mgr[security_id]) {
        delete sec_id_to_exch_playback_mgr[security_id];
      }
      sec_id_to_exch_playback_mgr.erase(security_id);
    }
  }

  void ResetAllInstances() {
    for (auto iter = sec_id_to_exch_playback_mgr.begin(); iter != sec_id_to_exch_playback_mgr.end(); iter++) {
      ResetInstance(iter->first);
    }
  }

 private:
  ExchangePlaybackManager<T, S> &GetInstance(const int security_id) {
    if (sec_id_to_exch_playback_mgr.find(security_id) == sec_id_to_exch_playback_mgr.end()) {
      sec_id_to_exch_playback_mgr[security_id] = new ExchangePlaybackManager<T, S>;
    }
    return *(sec_id_to_exch_playback_mgr[security_id]);
  }

  std::map<int, ExchangePlaybackManager<T, S> *> sec_id_to_exch_playback_mgr;
};
}
}

#endif
