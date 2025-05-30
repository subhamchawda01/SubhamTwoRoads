#include <map>
#include <vector>
#include <string>
#include "baseinfra/MarketAdapter/nse_bar_data_generator.hpp"

namespace HFSAT {

NSEBarDataGenerator::NSEBarDataGenerator(HFSAT::SecurityNameIndexer &_sec_symbol_indexer_, HFSAT::Watch &_watch_, const int granularity,
		const int start_unix_time_, const int end_unix_time_, int num_sec_id)
  : granularity_(granularity),
  multiplying_factor(60), // default 1 min
  start_unix_time_(start_unix_time_),
  end_unix_time_(end_unix_time_),
  watch_(_watch_),
  sec_symbol_indexer(_sec_symbol_indexer_),
  bar_metrics_mempool(),
  secid_to_bar_metrics(num_sec_id) {
  for (int ii = 0; ii < num_sec_id; ++ii) {
    secid_to_bar_metrics[ii] = bar_metrics_mempool.Alloc();
    secid_to_bar_metrics[ii]->expiry = NSESecurityDefinitions::GetExpiryFromShortCode(sec_symbol_indexer.GetShortcodeFromId(ii));
  }
}

void NSEBarDataGenerator::OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo &_market_update_info_) {
   // implementation for market updates goes here
}

void NSEBarDataGenerator::OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo &_trade_print_info_,
                           const HFSAT::MarketUpdateInfo &_market_update_info_) {
  int64_t current_time_ = watch_.tv().tv_sec;	
  int msecs_from_midnight = watch_.msecs_from_midnight();
  if ( msecs_from_midnight >= start_unix_time_ && msecs_from_midnight <= end_unix_time_) {
    double trade_px_ = _trade_print_info_.trade_price_;
    int size = _trade_print_info_.size_traded_;
    int64_t current_minute_start_time = current_time_ - (current_time_ % (granularity_ * multiplying_factor));
    if ( secid_to_bar_metrics[_security_id_]->bar_time !=  current_minute_start_time) {
      if (secid_to_bar_metrics[_security_id_]->bar_time != 0 ) {
        NotifyBarUpdate(_security_id_);
      }
      secid_to_bar_metrics[_security_id_]->bar_time = current_minute_start_time;
      secid_to_bar_metrics[_security_id_]->start_time = current_time_;
      secid_to_bar_metrics[_security_id_]->open = trade_px_;
      secid_to_bar_metrics[_security_id_]->close = trade_px_;
      secid_to_bar_metrics[_security_id_]->high = trade_px_;
      secid_to_bar_metrics[_security_id_]->low = trade_px_;
      secid_to_bar_metrics[_security_id_]->volume = size;
      secid_to_bar_metrics[_security_id_]->num_trades = 1;
      secid_to_bar_metrics[_security_id_]->ask_price = ( _market_update_info_.bestask_price_ != -1000.00)? _market_update_info_.bestask_price_:trade_px_;
      
      secid_to_bar_metrics[_security_id_]->bid_price = ( _market_update_info_.bestbid_price_ != -1000.00)? _market_update_info_.bestbid_price_:trade_px_;
    }
    else {
      if ( trade_px_ > secid_to_bar_metrics[_security_id_]->high ) {
         secid_to_bar_metrics[_security_id_]->high = trade_px_;
       }
       else if ( trade_px_ < secid_to_bar_metrics[_security_id_]->low) {
         secid_to_bar_metrics[_security_id_]->low = trade_px_;
       } 
       secid_to_bar_metrics[_security_id_]->close = trade_px_;
       secid_to_bar_metrics[_security_id_]->volume += size;
       secid_to_bar_metrics[_security_id_]->num_trades += 1;
    }       
    secid_to_bar_metrics[_security_id_]->end_time = current_time_;
    secid_to_bar_metrics[_security_id_]->ask_price = ( _market_update_info_.bestask_price_ != -1000.00)? _market_update_info_.bestask_price_:trade_px_;
    secid_to_bar_metrics[_security_id_]->bid_price = ( _market_update_info_.bestbid_price_ != -1000.00)? _market_update_info_.bestbid_price_:trade_px_;

  }			
}

//
void NSEBarDataGenerator::OnIndexUpdate(const unsigned int _security_id_, double const t_spot_price_){
  //std::cout << "NSEBarDataGenerator::OnIndexUpdate " << watch_.tv() 
  //	    << " sec_id: " << _security_id_ << " Px: " << t_spot_price_ << std::endl;
  int64_t current_time_ = watch_.tv().tv_sec;	
  int msecs_from_midnight = watch_.msecs_from_midnight();
  if ( msecs_from_midnight >= start_unix_time_ && msecs_from_midnight < end_unix_time_) {
    double trade_px_ = t_spot_price_;
    int size = 0;
    int64_t current_minute_start_time = current_time_ - (current_time_ % (granularity_ * multiplying_factor));
    if ( secid_to_bar_metrics[_security_id_]->bar_time !=  current_minute_start_time) {
      if (secid_to_bar_metrics[_security_id_]->bar_time != 0 ) {
        NotifyBarUpdate(_security_id_);
      }
      secid_to_bar_metrics[_security_id_]->bar_time = current_minute_start_time;
      secid_to_bar_metrics[_security_id_]->start_time = current_time_;
      secid_to_bar_metrics[_security_id_]->open = trade_px_;
      secid_to_bar_metrics[_security_id_]->close = trade_px_;
      secid_to_bar_metrics[_security_id_]->high = trade_px_;
      secid_to_bar_metrics[_security_id_]->low = trade_px_;
      secid_to_bar_metrics[_security_id_]->volume = size;
      secid_to_bar_metrics[_security_id_]->num_trades = 1;
      secid_to_bar_metrics[_security_id_]->ask_price = -1000.00;
      
      secid_to_bar_metrics[_security_id_]->bid_price = -1000.00; 
    }
    else {
      if ( trade_px_ > secid_to_bar_metrics[_security_id_]->high ) {
         secid_to_bar_metrics[_security_id_]->high = trade_px_;
       }
       else if ( trade_px_ < secid_to_bar_metrics[_security_id_]->low) {
         secid_to_bar_metrics[_security_id_]->low = trade_px_;
       } 
       secid_to_bar_metrics[_security_id_]->close = trade_px_;
       secid_to_bar_metrics[_security_id_]->volume += size;
       secid_to_bar_metrics[_security_id_]->num_trades += 1;
    }       
    secid_to_bar_metrics[_security_id_]->end_time = current_time_;
    secid_to_bar_metrics[_security_id_]->ask_price = -1000.00;
    secid_to_bar_metrics[_security_id_]->bid_price = -1000.00;

  }			
}
//

void NSEBarDataGenerator::DayOver() {
  int num_sec_id = secid_to_bar_metrics.size();
  for (int ii = 0; ii < num_sec_id; ii++) {
    if ( secid_to_bar_metrics[ii]->bar_time == 0 ) continue;
    for (auto listener : bar_update_listener_vec) {
      listener->OnBarUpdate(ii, *secid_to_bar_metrics[ii]);
    }
  }  
}

NSEBarDataGenerator::~NSEBarDataGenerator() {
}

void NSEBarDataGenerator::NotifyBarUpdate(int _security_id_) {
  for (auto listener : bar_update_listener_vec ) {
    listener->OnBarUpdate(_security_id_, *secid_to_bar_metrics[_security_id_]);
  }
}
}
