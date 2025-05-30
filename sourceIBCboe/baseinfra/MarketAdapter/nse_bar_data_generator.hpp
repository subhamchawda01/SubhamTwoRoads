#include <map>
#include <vector>
#include <string>
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/simple_mempool.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"


namespace HFSAT {

typedef struct {
  int64_t bar_time;
  int64_t start_time;
  int64_t end_time;
  int64_t expiry;
  double open;
  double close;
  double high;
  double low;
  int64_t volume;
  int64_t num_trades;
  double bid_price;
  double ask_price;
} BarMetrics;

class BarUpdateListener {
 public:
  virtual void OnBarUpdate(int security_id, BarMetrics &metrics) = 0;
};

class NSEBarDataGenerator : public HFSAT::SecurityMarketViewChangeListener {
 private:
  int granularity_;
  int multiplying_factor;
  int start_unix_time_;
  int end_unix_time_;
  HFSAT::Watch &watch_;
  HFSAT::SecurityNameIndexer &sec_symbol_indexer;
  SimpleMempool<BarMetrics> bar_metrics_mempool;
  std::vector<BarMetrics*>  secid_to_bar_metrics;
  std::vector<BarUpdateListener*> bar_update_listener_vec; 
  NSEBarDataGenerator(HFSAT::SecurityNameIndexer &_sec_symbol_indexer_, HFSAT::Watch &_watch_, const int granularity,
		  const int start_unix_time_, const int end_unix_time_, int num_sec_id);
 public:
  ~NSEBarDataGenerator(); 
  static HFSAT::NSEBarDataGenerator&  GetUniqueInstance(HFSAT::SecurityNameIndexer &_sec_symbol_indexer_, HFSAT::Watch &_watch_, const int granularity,
		   const int start_unix_time_, const int end_unix_time_, int num_sec_id) {
     static HFSAT::NSEBarDataGenerator unique_instance(_sec_symbol_indexer_, _watch_, granularity, start_unix_time_, 
		     end_unix_time_, num_sec_id);
     return unique_instance;
   }

   virtual void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo &_market_update_info_);
   
   virtual void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo &_trade_print_info_,
                           const HFSAT::MarketUpdateInfo &_market_update_info_);

   void UpdateMultiplyingFactor(int factor){ multiplying_factor = factor; }

   virtual void OnIndexUpdate(const unsigned int t_security_id_, double const t_spot_price_);
   //function to be called , to get the last bar update
   void DayOver();
  
   void NotifyBarUpdate(int _security_id_);

   inline void AddBarUpdateListener(HFSAT::BarUpdateListener *bar_update_listener) {
     VectorUtils::UniqueVectorAdd(bar_update_listener_vec, bar_update_listener);
   }
};
}
