#pragma once

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvccode/CommonTradeUtils/trade_time_manager.hpp"
#include "baseinfra/MarketAdapter/order_history_cache.hpp"
#include "baseinfra/TradeUtils/big_trades_listener.hpp"
// listens to ORS messages for supoprting non-self book
#include "dvccode/ORSMessages/ors_message_listener.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"
#include "dvccode/Utils/unqiue_instance_for_storing_details_across.hpp"

#include <EventApi.hpp>
#include <EventConfig.hpp>

namespace HFSAT {

struct LevelInfo {
  HwBookEntryMBP bid;  //  uint32_t price, uint32_t quantity;
  HwBookEntryMBP ask;  //  uint32_t price, uint32_t quantity;
};

#define MAXMBOFPGALEVEL 3
#define Configtoml "/home/pengine/prod/live_configs/configNSE.toml"

class FpgaNseMarketViewManager : public BaseMarketViewManager {
 private:
  int security_id;
  LevelChangeType_t event_type_;
  struct timeval source_time;
  SecurityMarketView* market_view_ptr_;
  ExternalTimeListener* p_time_keeper_;
  MBOCHIP::SiliconMD::EApi::EventApi<MBOCHIP::SiliconMD::EApi::MessageApi<MBOCHIP::SiliconMD::EApi::NSEFPGAReceiver>>*
      api;
  TradeTimeManager& trade_time_manager_;
  HFSAT::Utils::NSEDailyTokenSymbolHandler& nse_daily_token_symbol_handler_;
  HFSAT::Utils::NSERefDataLoader& nse_refdata_loader_;
  LevelInfo l1, l2;
  std::vector<std::string> source_shortcode_vec_;
  std::map<char, std::unordered_map<int32_t, int32_t>> segment_to_token_secid_map_;

 public:
  FpgaNseMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                           const SecurityNameIndexer& t_sec_name_indexer_,
                           const std::vector<SecurityMarketView*>& t_security_market_view_map_,
                           std::vector<std::string>& source_shortcode_list_);

  void Run();
  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { p_time_keeper_ = t_new_listener_; }

 private:
  void DummyFunc();

  void ProcessHWBookMessage(const MBOCHIP::SiliconMD::EApi::Event* event);

  void ProcessTradeMessage(const MBOCHIP::SiliconMD::EApi::Event* event);

  void NotifyListenersOnLevelChange();

  void NotifyListenersOnTrade(const uint32_t t_security_id_, const int t_trade_int_price_, const int t_trade_size_);

  void SetSMVBestVars(const MBOCHIP::SiliconMD::EApi::Event *event);

  bool CheckValidTime(int sec_id);
};
}  // namespace HFSAT
