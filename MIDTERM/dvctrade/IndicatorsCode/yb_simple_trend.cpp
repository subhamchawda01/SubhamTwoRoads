/**
    \file IndicatorsCode/yb_simple_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include <cctype>

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/yb_simple_trend.hpp"

namespace HFSAT {

void YBSimpleTrend::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                      std::vector<std::string>& _ors_source_needed_vec_,
                                      const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

YBSimpleTrend* YBSimpleTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                const std::vector<const char*>& r_tokens_,
                                                PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
}

YBSimpleTrend* YBSimpleTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                const SecurityMarketView& _indep_market_view_,
                                                double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_seconds_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, YBSimpleTrend*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new YBSimpleTrend(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                          _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

YBSimpleTrend::YBSimpleTrend(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                             const std::string& concise_indicator_description_,
                             const SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                             PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      price_type_(_price_type_),
      moving_avg_yield_(0),
      last_yield_recorded_(0),
      current_indep_yield_(0),
      lower_int_price_level_(9999999),
      higher_int_price_level_(-1) {
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

  min_price_inc_ =
      SecurityDefinitions::GetContractMinPriceIncrement(_indep_market_view_.shortcode(), r_watch_.YYYYMMDD());
  // FV_Price FV_Yield
  // 56.984375 20.001701
  // 56.992188 19.998065
  // 57.000000 19.994429
  // 57.007812 19.990794
  // 57.015625 19.987160

  if ((indep_market_view_.shortcode().find("ZT_0") != std::string::npos) ||
      (indep_market_view_.shortcode().find("ZF_0") != std::string::npos) ||
      (indep_market_view_.shortcode().find("ZN_0") != std::string::npos) ||
      (indep_market_view_.shortcode().find("ZB_0") != std::string::npos) ||
      (indep_market_view_.shortcode().find("UB_0") != std::string::npos) ||
      (indep_market_view_.shortcode().find("FGBS_0") != std::string::npos) ||
      (indep_market_view_.shortcode().find("FGBM_0") != std::string::npos) ||
      (indep_market_view_.shortcode().find("FGBL_0") != std::string::npos) ||
      (indep_market_view_.shortcode().find("FGBX_0") != std::string::npos) ||
      (indep_market_view_.shortcode().find("FOAT_0") != std::string::npos) ||
      (indep_market_view_.shortcode().find("FBTP_0") != std::string::npos) ||
      (indep_market_view_.shortcode().find("JGBL_0") != std::string::npos)) {
    ReadP2YFile(indep_market_view_.shortcode());
  } else {
    std::cerr << indep_market_view_.shortcode() << " product code not handled " << std::endl;
  }
}

void YBSimpleTrend::ReadP2YFile(std::string _this_shortcode_) {
  std::string base_shc_ = _this_shortcode_.substr(0, _this_shortcode_.find("_"));

  std::stringstream ss;
  ss << BASEP2YINFODIR << base_shc_ << "_p2y";
  std::string _p2y_filename_ = ss.str();

  std::ifstream p2y_infile_;
  p2y_infile_.open(_p2y_filename_.c_str(), std::ifstream::in);
  if (p2y_infile_.is_open()) {
    const int kP2YLineBufferLen = 1024;
    char readline_buffer_[kP2YLineBufferLen];
    bzero(readline_buffer_, kP2YLineBufferLen);

    double price_ = -1.0;
    double yld_ = -1.0;
    int int_price_ = -1;

    while (p2y_infile_.good()) {
      bzero(readline_buffer_, kP2YLineBufferLen);
      p2y_infile_.getline(readline_buffer_, kP2YLineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kP2YLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() == 2) {
        // PRICE YIELD
        if (isdigit(tokens_[0][0])) {
          price_ = atof(tokens_[0]);
          yld_ = atof(tokens_[1]);
          int_price_ = indep_market_view_.GetIntPx(price_);

          if (int_price_ < lower_int_price_level_) {
            lower_int_price_level_ = int_price_;
          }
          if (int_price_ > higher_int_price_level_) {
            higher_int_price_level_ = int_price_;
          }
          yield_.push_back(yld_);
        }
      }
    }
    p2y_infile_.close();
  }
  std::sort(yield_.begin(), yield_.end(), std::greater<double>());  // just in case
}

void YBSimpleTrend::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void YBSimpleTrend::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    double current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
    double slope_ = (current_indep_price_ - _market_update_info_.bestbid_price_) / min_price_inc_;
    double bpyi_ = _market_update_info_.bestbid_int_price_ - lower_int_price_level_ + 1;
    double apyi_ = _market_update_info_.bestask_int_price_ - lower_int_price_level_ + 1;

    if (bpyi_ < yield_.size() && apyi_ < yield_.size()) {
      current_indep_yield_ = yield_[bpyi_] + (slope_) * (yield_[apyi_] - yield_[bpyi_]);
      //  yield ( bid_int_price ) + ( price - bid_price ) / ( min_px_increment ) * ( yield ( ask_int_price ) - yield (
      //  bid_int_price ) )
      if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
        moving_avg_yield_ += inv_decay_sum_ * (current_indep_yield_ - last_yield_recorded_);
      } else {
        int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
        if (num_pages_to_add_ >= (int)decay_vector_.size()) {
          InitializeValues();
        } else {
          if (num_pages_to_add_ == 1) {
            moving_avg_yield_ = (current_indep_yield_ * inv_decay_sum_) + (moving_avg_yield_ * decay_vector_[1]);
          } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
            moving_avg_yield_ = (current_indep_yield_ * inv_decay_sum_) +
                                (last_yield_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                (moving_avg_yield_ * decay_vector_[num_pages_to_add_]);
          }
          last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
        }
      }
      last_yield_recorded_ = current_indep_yield_;
      indicator_value_ = (current_indep_yield_ - moving_avg_yield_);
    } else {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " prices out of range " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      indicator_value_ = 0;
    }
    NotifyIndicatorListeners(indicator_value_);
  }
  // if ( data_interrupted_ )
  //  { // not sure we will need this ... since by definition data is already iterrupted
  //    indicator_value_ = 0;
  //  }
}

void YBSimpleTrend::InitializeValues() {
  moving_avg_yield_ = current_indep_yield_;
  last_yield_recorded_ = current_indep_yield_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

// market_interrupt_listener interface
void YBSimpleTrend::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void YBSimpleTrend::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
