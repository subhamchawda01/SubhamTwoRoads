#ifndef _L1_DATA_GENERATOR_
#define _L1_DATA_GENERATOR_

#include <string>
#include <vector>

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/Tools/common_smv_source.hpp"

namespace HFSAT {

class L1DataGenerator {
 public:
  L1DataGenerator(int trading_date, const std::vector<std::string>& shortcodes);
  ~L1DataGenerator();
  void Run();
  void AddListener(SecurityMarketViewChangeListener* listener);
  const Watch& GetWatch();

 private:
  CommonSMVSource* common_smv_source_;
};
}

#endif
