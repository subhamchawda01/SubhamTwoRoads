/*
 * regime_stats_diff.cpp
 *
 *  Created on: 19-Nov-2015
 *      Author: raghuram
 */
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/regime_stats_diff.hpp"

namespace HFSAT {

void RegimeStatsDiff::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                        std::vector<std::string>& _ors_source_needed_vec_,
                                        const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

RegimeStatsDiff* RegimeStatsDiff::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    const std::vector<const char*>& r_tokens_,
                                                    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atoi(r_tokens_[4]), atof(r_tokens_[5]), atof(r_tokens_[6]));
}

RegimeStatsDiff* RegimeStatsDiff::GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                    const SecurityMarketView& _indep_market_view_, int column_index_,
                                                    double threshold_, double tolerance_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << column_index_ << ' ' << threshold_
              << tolerance_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, RegimeStatsDiff*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new RegimeStatsDiff(_dbglogger_, _watch_, concise_indicator_description_, _indep_market_view_, column_index_,
                            threshold_, tolerance_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

RegimeStatsDiff::RegimeStatsDiff(DebugLogger& _dbglogger_, const Watch& _watch_,
                                 const std::string& _concise_indicator_description_,
                                 const SecurityMarketView& _indep_market_view_, int _column_index_, double _threshold_,
                                 double _tolerance_)
    : CommonIndicator(_dbglogger_, _watch_, _concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      column_index_(_column_index_),
      threshold_(_threshold_),
      tolerance_(_tolerance_),
      watch_(_watch_) {
  if (!indep_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
}

void RegimeStatsDiff::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void RegimeStatsDiff::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    time_t unix_time = watch_.tv().tv_sec;

    struct tm* tm = localtime(&unix_time);
    char temp_date[20];
    strftime(temp_date, sizeof(date_), "%Y%m%d", tm);
    date_ = atoi(temp_date);
    is_ready_ = true;

    std::ifstream infile(data_file_);

    std::string line_0;
    std::string line_1;

    while (std::getline(infile, line_0)) {
      line_1 = line_0;
      std::istringstream iss(line_0);

      int temp_date;
      iss >> temp_date;

      if (temp_date == date_) {
        int temp_ind = column_index_;
        std::istringstream temp_iss_0(line_0);
        std::istringstream temp_iss_1(line_1);

        while (temp_ind > 0) {
          temp_iss_0 >> value_0_;
          temp_iss_1 >> value_1_;
          temp_ind--;
        }
      }
    }
    if (std::abs(value_0_ - value_1_) > threshold_) {
      indicator_value_ = 1;
    } else {
      indicator_value_ = 2;
    }

  } else {
    NotifyIndicatorListeners(indicator_value_);
  }
}

// void RegimeStatsDiff::InitializeValues() {}

// market_interrupt_listener interface
void RegimeStatsDiff::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void RegimeStatsDiff::OnMarketDataResumed(const unsigned int _security_id_) {
  /*
        if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }*/
}
}
