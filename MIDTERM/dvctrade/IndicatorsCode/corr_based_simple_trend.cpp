/**
   \file IndicatorsCode/simple_trend.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/corr_based_simple_trend.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

namespace HFSAT {

void CorrBasedSimpleTrend::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                             std::vector<std::string>& _ors_source_needed_vec_,
                                             const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

CorrBasedSimpleTrend* CorrBasedSimpleTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                              const std::vector<const char*>& r_tokens_,
                                                              PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

CorrBasedSimpleTrend* CorrBasedSimpleTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                              const SecurityMarketView& _indep_market_view_,
                                                              const SecurityMarketView& _dep_market_view_,
                                                              double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << " " << _dep_market_view_.secname() << ' '
              << _fractional_seconds_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, CorrBasedSimpleTrend*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new CorrBasedSimpleTrend(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                                 _dep_market_view_, _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

CorrBasedSimpleTrend::CorrBasedSimpleTrend(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                           const std::string& concise_indicator_description_,
                                           const SecurityMarketView& _indep_market_view_,
                                           const SecurityMarketView& _dep_market_view_, double _fractional_seconds_,
                                           PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      dep_market_view_(_dep_market_view_),
      price_type_(_price_type_),
      moving_avg_price_(0),
      max_corr_(0),
      use_corrs_(true),
      index_(0),
      mean_(0),
      stdev_(0),
      date_today_(r_watch_.YYYYMMDD()),
      last_price_recorded_(0),
      current_indep_price_(0),
      num_intervals_(0),
      daycount_(0),
      decay_factor_(0.5) {
  trend_history_msecs_ = std::max(20, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  if (!indep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  std::string corr_file_name_ = CORR_INFO_FILE_PREFIX;
  corr_file_name_.append(dep_market_view_.shortcode());
  corr_file_name_.append("_");
  corr_file_name_.append(indep_market_view_.shortcode());
  // std::cout << corr_file_name_ <<std::endl;
  std::fstream file(corr_file_name_, std::ofstream::in);
  if (!file || !file.is_open()) {
    std::cerr << "Could not open file " << corr_file_name_ << std::endl;
    use_corrs_ = false;
  } else {
    char line[1024];
    memset(line, 0, sizeof(line));
    int yymmdd_ = 20110101;
    int count = 0;
    while (file.getline(line, sizeof(line))) {
      HFSAT::PerishableStringTokenizer st_(line, 1024);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      int date_ = atoi(tokens_[0]);
      if (date_ >= date_today_) continue;
      // std::cout<<date_<<" "<<yymmdd_<<std::endl;
      if (date_ < yymmdd_) {
        if (count == 0) daycount_++;
        if (daycount_ >= MAX_NUM_DAYS) break;
        int temp_start_time_ = atoi(tokens_[1]);
        // int temp_end_time_ = atoi ( tokens_ [ 2 ] );
        if (start_times_[count] == temp_start_time_) {
          corrs_[count] += atof(tokens_[3]);
          count = (count + 1) % num_intervals_;
        }
      } else {
        yymmdd_ = date_;
        start_times_.push_back(atoi(tokens_[1]));
        end_times_.push_back(atoi(tokens_[2]));
        double corr_ = atof(tokens_[3]);
        if (corr_ < 0) corr_ *= -1;
        corrs_.push_back(corr_);
        num_intervals_++;
        if (corr_ > max_corr_) max_corr_ = corr_;
        // std::cout<<"start time = " << start_times_ [ start_times_.size( ) -1 ]<<"end time = "<<end_times_ [
        // start_times_.size( ) - 1 ]<<" current_corr_ "<<corrs_ [ start_times_.size( ) -1 ]<<" max_corr_
        // "<<max_corr_<<std::endl;
      }
    }
    for (auto i = 0u; i < num_intervals_; i++) {
      corrs_[i] /= daycount_;
    }
  }
  // VectorUtils::CalcMeanStdevNormalizeData( corrs_, mean_, stdev_ ); //Will end up assigning -ve corrs, so not
  // desirable
  SmoothenCorrs();
  // std::cout<<max_corr_<< " " <<use_corrs_<< " " <<mean_<<" "<<stdev_<<std::endl;
}

void CorrBasedSimpleTrend::SmoothenCorrs() {
  for (auto i = 0u; i < corrs_.size(); i++) {
    if (i > 0) corrs_[i] = decay_factor_ * corrs_[i - 1] + (1 - decay_factor_) * corrs_[i];
  }
}

void CorrBasedSimpleTrend::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void CorrBasedSimpleTrend::OnMarketUpdate(const unsigned int _security_id_,
                                          const MarketUpdateInfo& _market_update_info_) {
  current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);

  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_price_ += inv_decay_sum_ * (current_indep_price_ - last_price_recorded_);
    } else {
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_price_ = (current_indep_price_ * inv_decay_sum_) + (moving_avg_price_ * decay_vector_[1]);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          moving_avg_price_ = (current_indep_price_ * inv_decay_sum_) +
                              (last_price_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                              (moving_avg_price_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }

    last_price_recorded_ = current_indep_price_;
    indicator_value_ = (current_indep_price_ - moving_avg_price_);
    if (use_corrs_) indicator_value_ *= GetCorratTime(watch_);

    // if ( data_interrupted_ )
    //  { // not sure we will need this ... since by definition data is already iterrupted
    //    indicator_value_ = 0;
    //  }
    // std::cout<<" Indicator Value = "<<indicator_value_<<std::endl;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void CorrBasedSimpleTrend::InitializeValues() {
  moving_avg_price_ = current_indep_price_;
  last_price_recorded_ = current_indep_price_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

// market_interrupt_listener interface
void CorrBasedSimpleTrend::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                   const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void CorrBasedSimpleTrend::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

double CorrBasedSimpleTrend::GetCorratTime(const Watch& _watch_) {
  int msecs_frm_midnight = _watch_.msecs_from_midnight();
  int minutes_frm_midnight = msecs_frm_midnight / (1000 * 60);
  int hhmm = (int)(minutes_frm_midnight / 60) * 100 + minutes_frm_midnight % 60;
  // std::cout<<index_<<" "<<end_times_.size ( )<<std::endl;
  for (unsigned int i = index_; i < end_times_.size(); i++) {
    // std::cout<<"HHMM = "<<hhmm<<" start time = "<<start_times_ [ i ]<<" end time = "<<end_times_ [ i ]<<std::endl;
    if (start_times_[i] <= hhmm && hhmm < end_times_[i]) {
      index_ = i;
      // std::cout<<"HHMM = "<<hhmm<<" corr found = "<<corrs_ [ i ]<<std::endl;
      return corrs_[i];
    }
  }
  return 1;
}
}
