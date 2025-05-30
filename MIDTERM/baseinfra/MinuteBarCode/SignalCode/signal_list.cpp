#include "baseinfra/MinuteBar/Signal/signal_list.hpp"

namespace HFSAT {

namespace SignalHelper {
std::map<std::string, BaseSignalUniqInstanceFuncPtr> SignalTokenToUniqInstMap;
void SetSignalListMap() {
  if (SignalTokenToUniqInstMap.empty()) {
    SignalTokenToUniqInstMap[SimpleMovingAverage::SignalName()] = GetUniqueInstanceWrapper<SimpleMovingAverage>;
  }
}

BaseSignalUniqInstanceFuncPtr GetUniqueInstanceFunc(std::string signal) {
  if (SignalTokenToUniqInstMap.find(signal) != SignalTokenToUniqInstMap.end()) {
    return SignalTokenToUniqInstMap[signal];
  } else {
    std::cerr << __FUNCTION__ << " Could not find Signal : " << signal << std::endl;
    return NULL;
  }
}

std::vector<std::string> GetSignalOrder() {
  std::vector<std::string> signal_vec;
  signal_vec.push_back(SimpleMovingAverage::SignalName());
  return signal_vec;
}
}
}
