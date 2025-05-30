#ifndef _L1_DATA_LOGGER_
#define _L1_DATA_LOGGER_

#include <string>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

namespace HFSAT {

class L1DataLogger : public SecurityMarketViewChangeListener {
 public:
  L1DataLogger(const std::string log_file_path, const Watch& watch);
  ~L1DataLogger();
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);

 private:
  const Watch& watch_;
  BulkFileWriter bulk_file_writer_;
};
}

#endif
