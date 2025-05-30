#include "baseinfra/MinuteBar/Signal/simple_moving_average.hpp"

namespace HFSAT {

namespace SignalHelper {
typedef BaseMinuteBarSignal *(*BaseSignalUniqInstanceFuncPtr)(DebugLogger &, const Watch &,
                                                              const std::vector<const char *> &, MinuteBarPriceType);

extern std::map<std::string, BaseSignalUniqInstanceFuncPtr> SignalTokenToUniqInstMap;

void SetSignalListMap();

BaseSignalUniqInstanceFuncPtr GetUniqueInstanceFunc(std::string varname_);

template <class T>
BaseMinuteBarSignal *GetUniqueInstanceWrapper(DebugLogger &dbglogger, const Watch &watch,
                                              const std::vector<const char *> &str, MinuteBarPriceType price_type) {
  return T::GetUniqueInstance(dbglogger, watch, str, price_type);
}

std::vector<std::string> GetSignalOrder();
}
}
