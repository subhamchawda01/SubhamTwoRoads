#ifndef _LIVE_TRADE_PRICE_MANAGER_HPP_
#define _LIVE_TRADE_PRICE_MANAGER_HPP_

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/mds_shm_interface_defines.hpp"
#include "dvccode/Utils/thread.hpp"

namespace HFSAT {

namespace Utils {

struct LastTradePriceInfo {
  LastTradePriceInfo()
      : last_trade_price(std::numeric_limits<double>::max()),
        min_price(std::numeric_limits<double>::max()),
        max_price(std::numeric_limits<double>::max()),
        is_trade_received_(false) {}
  double last_trade_price;
  double min_price;
  double max_price;
  bool is_trade_received_;
};

class LiveTradePriceManager : public HFSAT::Thread, public SimpleSecuritySymbolIndexerListener {
 public:
  LiveTradePriceManager(HFSAT::ExchSource_t exchange, std::vector<LastTradePriceInfo>& sec_id_to_last_traded_price,
                        DebugLogger& dbglogger);

  ~LiveTradePriceManager() {}

  void thread_main();

  void ShowLastTradedPrices();

  void SetAllowedPriceTicks(int num_of_ticks);

 protected:
  void OnAddString(unsigned int t_num_security_id_);

 private:
  key_t generic_mds_shm_key_;
  int32_t generic_mds_shmid_;

  volatile HFSAT::MDS_MSG::GenericMDSMessage* generic_mds_shm_struct_;
  struct shmid_ds generic_shmid_ds_;
  int32_t index_;

  HFSAT::ExchSource_t exchange_;
  SimpleSecuritySymbolIndexer& simple_security_symbol_indexer_;
  std::vector<LastTradePriceInfo>& sec_id_to_last_traded_price_;
  std::vector<std::pair<double, double>> sec_id_to_last_bid_ask_price_;
  std::vector<double> sec_id_to_allowed_price_range_;
  unsigned int allowed_num_of_ticks_;

  DebugLogger& dbglogger_;

  int num_trades_;
};
}
}

#endif
