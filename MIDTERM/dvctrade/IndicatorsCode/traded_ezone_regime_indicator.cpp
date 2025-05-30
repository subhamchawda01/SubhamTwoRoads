#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvctrade/Indicators/traded_ezone_regime_indicator.hpp"

namespace HFSAT {

void TradedEzoneRegimeIndicator::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                    std::vector<std::string>& _ors_source_needed_vec_,
                                    const std::vector<const char*>& r_tokens_) {
}

TradedEzoneRegimeIndicator* TradedEzoneRegimeIndicator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                              const std::vector<const char*>& r_tokens_, PriceType_t _price_type_) {
  // INDICATOR _this_weight_ _indicator_string_ traded_ezone_ pricetype
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           std::string(r_tokens_[3]), _price_type_);
}


TradedEzoneRegimeIndicator* TradedEzoneRegimeIndicator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const std::string& traded_ezone_,
                                            PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << traded_ezone_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TradedEzoneRegimeIndicator*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new TradedEzoneRegimeIndicator(t_dbglogger_, r_watch_, concise_indicator_description_, traded_ezone_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}



TradedEzoneRegimeIndicator::TradedEzoneRegimeIndicator(DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_, const std::string& traded_ezone_, PriceType_t _price_type_)
 : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
   eco_event_manager_ (t_dbglogger_, r_watch_, traded_ezone_)
{
  eco_event_manager_.GetTradedEventsForToday();
  watch_.subscribe_TimePeriod(this);
  indicator_value_ = 0;

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "TradedEventsForToday for " << concise_indicator_description_ << ":\n";
  for (auto& eventline_ : eco_event_manager_.traded_events_of_the_day()) {
    t_temp_oss_ << HFSAT::GetStrFromEconomicZone(eventline_.ez_) << ' ' << eventline_.severity_ << ' '
    << eventline_.event_text_ << ' ' << eventline_.start_mfm_ << ' ' << eventline_.end_mfm_ << "\n";
  }
  DBGLOG_TIME_CLASS << t_temp_oss_.str() << DBGLOG_ENDL_FLUSH;
}

void TradedEzoneRegimeIndicator::OnTimePeriodUpdate(const int num_pages_to_add_) {
  //iterating over the traded_ezone_vec  
  int mfm_ = watch_.msecs_from_midnight() ;

  bool t_indicator_value_ = false;
  for (auto& eventline_ : eco_event_manager_.traded_events_of_the_day()) {
    // if the event time lies in between the start time and end time then 1 else 0		  
    if (mfm_ >= eventline_.start_mfm_ && mfm_ <= eventline_.end_mfm_ ){
      t_indicator_value_ = true;
      break;
    }
  }
  indicator_value_ = (double)t_indicator_value_;
  NotifyIndicatorListeners( indicator_value_ );
}

}
