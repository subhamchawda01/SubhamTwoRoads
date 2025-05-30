/**
   \file IndicatorsCode/size_based_regime.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 351, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include <fstream>
#include <strings.h>
#include <stdlib.h>
#include <sstream>

#include "dvccode/CDef/math_utils.hpp"

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"

#include "dvctrade/Indicators/size_based_regime.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"

namespace HFSAT {

void SizeBasedRegime::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                        std::vector<std::string>& _ors_source_needed_vec_,
                                        const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

SizeBasedRegime* SizeBasedRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    const std::vector<const char*>& r_tokens_,
                                                    PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_ _fractional_seconds_  _num_levels_  _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  double t_stdev_duration_ = 0;
  if (r_tokens_.size() < 7) {
    std::cerr << "SizeBasedRegime syntax incorrect! given text:";
    for (auto i = 0u; i < r_tokens_.size(); i++) {
      std::cerr << " " << r_tokens_[i];
    }
    std::cerr << std::endl;
    std::cerr << "Expected syntax: INDICATOR  _this_weight_ SizeBasedRegime _dep_market_view_ _fractional_seconds_  "
                 "_num_levels_  _price_type_"
              << std::endl;

    exit(1);
  }

  PriceType_t t_price_type_ = _basepx_pxtype_;
  IndicatorUtil::GetLastTwoArgsFromIndicatorTokens(6, r_tokens_, t_stdev_duration_, t_price_type_);

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), atoi(r_tokens_[5]), t_price_type_, t_stdev_duration_);
}

SizeBasedRegime* SizeBasedRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    SecurityMarketView& _dep_market_view_, double _fractional_seconds_,
                                                    int t_num_levels_, PriceType_t _price_type_,
                                                    double _stdev_duration_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _fractional_seconds_ << ' ' << t_num_levels_
              << " " << _stdev_duration_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SizeBasedRegime*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new SizeBasedRegime(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                            _fractional_seconds_, t_num_levels_, _price_type_, _stdev_duration_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SizeBasedRegime::SizeBasedRegime(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                 const std::string& concise_indicator_description_,
                                 SecurityMarketView& t_dep_market_view_, double _fractional_seconds_, int t_num_levels_,
                                 PriceType_t _price_type_, double _stdev_duration_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      current_projection_multiplier_(0.0),
      current_projected_trend_(0),
      pred_mode_(1u),
      moving_average_bid_size_(0),
      moving_average_ask_size_(0),
      cumulative_bid_size_(0),
      cumulative_ask_size_(0),
      num_event_count_(0),
      stdev_duration_(_stdev_duration_),
      fractional_seconds_(_fractional_seconds_),
      last_new_page_msecs_(0),
      page_width_msecs_(_fractional_seconds_),
      last_recorded_ask_size_(0),
      last_recorded_bid_size_(0),
      first_time_(true),
      alpha_(0.1),
      book_info_manager_(*(BookInfoManager::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_))) {
  watch_.subscribe_FifteenSecondPeriod(this);

  // dep_market_view_.subscribe_L2 ( this );
  dep_market_view_.subscribe_L2(this);

  book_info_manager_.ComputeSumSize(t_num_levels_, 1, stdev_duration_);

  book_info_struct_ = book_info_manager_.GetBookInfoStruct(t_num_levels_, 1, stdev_duration_);
  if (book_info_struct_ == NULL) {
    std::cerr << "Error getting book_info_struct for SizeBasedRegime: " << t_num_levels_ << " 1" << std::endl;
    exit(1);
  }

  InitializeValues();
  alpha_ = 1.00 / fractional_seconds_;
  avg_ask_level_size_ = fractional_seconds_;
  avg_bid_level_size_ = fractional_seconds_;
  DBGLOG_TIME_CLASS_FUNC << " Seconds : " << _fractional_seconds_ << " num_lev: " << t_num_levels_ << " " << alpha_
                         << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;
}

void SizeBasedRegime::OnTimePeriodUpdate(const int num_pages_to_add_) {
  // need to take last _fractional_seconds_average_

  /*
  int num_pages_ = ( int ) floor ( ( watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_ );

  if ( first_time_ )
    {
      moving_average_ask_size_ = (unsigned int)(moving_average_ask_size_ + ( book_info_struct_ -> sum_ask_size_ ) ) ;
      last_recorded_ask_size_ = book_info_struct_ -> sum_ask_size_   ;

      if (watch_.msecs_from_midnight ( ) - last_new_page_msecs_ > page_width_msecs_ )
        {
          first_time_ = false ;
          last_new_page_msecs_ += ( num_pages_ * page_width_msecs_ ) ;
        }

      //    moving_average_bid_size_ = ( unsigned int ) ( moving_average_bid_size_ + ( book_info_struct_ ->
  sum_bid_size_ ) );
      last_recorded_bid_size_ = book_info_struct_ -> sum_bid_size_ ;
    }
  else
    {
      moving_average_ask_size_ = ( unsigned int ) ( ( 1 - alpha_ ) * moving_average_ask_size_ + (1-alpha_)*alpha_*(
  book_info_struct_ -> sum_ask_size_ ) );
      //   moving_average_bid_size_ = ( unsigned int ) ( ( 1 - alpha_ ) * moving_average_bid_size_ + (1-alpha_)*alpha_*(
  book_info_struct_ -> sum_bid_size_ ) );

      last_recorded_ask_size_ = book_info_struct_ -> sum_ask_size_ ;
      last_recorded_bid_size_ = book_info_struct_ -> sum_bid_size_ ;
    }

*/

  if (num_event_count_ != 0) {
    moving_average_ask_size_ = double(cumulative_ask_size_) / double(num_event_count_);
    moving_average_bid_size_ = double(cumulative_bid_size_) / double(num_event_count_);
  }

  cumulative_ask_size_ = 0;
  cumulative_bid_size_ = 0;
  num_event_count_ = 0;

  if (moving_average_ask_size_ >= avg_ask_level_size_ && moving_average_bid_size_ >= avg_bid_level_size_) {
    pred_mode_ = 0u;
  } else {
    pred_mode_ = 1u;
  }

  //    DBGLOG_TIME_CLASS_FUNC_LINE << "SIZES: " << book_info_struct_ -> sum_ask_size_ << " " << avg_ask_level_size_ <<
  //    " " << moving_average_ask_size_
  //      << " "<< book_info_struct_ -> sum_bid_size_ << " " << avg_bid_level_size_ <<" "  << moving_average_bid_size_
  //      << " "<< pred_mode_ <<  DBGLOG_ENDL_FLUSH ;
}

void SizeBasedRegime::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    } else {
      is_ready_ = true;
      InitializeValues();
    }
  }
}

void SizeBasedRegime::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
    }
  }

  if (!data_interrupted_) {
    cumulative_ask_size_ += book_info_struct_->sum_ask_size_;
    cumulative_bid_size_ += book_info_struct_->sum_bid_size_;
    num_event_count_++;
  }

  if (is_ready_ && !data_interrupted_) {
    indicator_value_ = pred_mode_ + 1;
    NotifyIndicatorListeners(indicator_value_);
  }

  // Need to check/make-sure it is called after market upadate of book_info_manager
  /*
   *

  if ( moving_average_ask_size_ >= avg_ask_level_size_ && moving_average_bid_size_ >= avg_bid_level_size_ )
    {
      pred_mode_ = 0u; // vol strats
    }
  else
    {
      pred_mode_ = 1u ;
    }
   */

  //    DBGLOG_TIME_CLASS_FUNC_LINE << "SIZES: " << book_info_struct_ -> sum_ask_size_ << " " << avg_ask_level_size_ <<
  //    " " << moving_average_ask_size_
  ///      << " "<< book_info_struct_ -> sum_bid_size_ << " " << avg_bid_level_size_ <<" "  << moving_average_bid_size_
  ///      << " "<< pred_mode_ <<  DBGLOG_ENDL_FLUSH ;

  //    indicator_value_ = pred_mode_ + 1;
  //   NotifyIndicatorListeners( indicator_value_ );
}

// market_interrupt_listener interface
void SizeBasedRegime::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void SizeBasedRegime::OnMarketDataResumed(const unsigned int _security_id_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

void SizeBasedRegime::InitializeValues() {
  indicator_value_ = 0;
  last_new_page_msecs_ =
      watch_.msecs_from_midnight() - (watch_.msecs_from_midnight()) % (int)(fractional_seconds_ * 1000);
  last_recorded_ask_size_ = book_info_struct_->sum_ask_size_;
  last_recorded_bid_size_ = book_info_struct_->sum_bid_size_;
}
}
