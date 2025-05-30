/**
    \file IndicatorsCode/valuew_bp_momentum.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/valuew_bp_momentum.hpp"
#include "dvctrade/Indicators/index_utils.hpp"

namespace HFSAT {

void ValueWBPMomentum::CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                         std::vector<std::string> &_ors_source_needed_vec_,
                                         const std::vector<const char *> &r_tokens_) {
  HFSAT::VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, std::string(r_tokens_[3]));
}

ValueWBPMomentum *ValueWBPMomentum::GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                                      const std::vector<const char *> &r_tokens_,
                                                      PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _num_levels_ _level_decay_factor_
  // _fractional_seconds_
  if (r_tokens_.size() < 7) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "ValueWBPMomentum incorrect syntax. Should be INDICATOR _this_weight_ _indicator_string_ "
                "_indep_port_market_view_ _num_levels_ _level_decay_factor_");
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           (ShortcodeSecurityMarketViewMap::GetUniqueInstance()).GetSecurityMarketView(r_tokens_[3]),
                           std::max(0, atoi(r_tokens_[4])), atof(r_tokens_[5]), atof(r_tokens_[6]));
}

ValueWBPMomentum *ValueWBPMomentum::GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                                      SecurityMarketView *_indep_security_market_view_,
                                                      unsigned int _num_levels_, double _level_decay_factor_,
                                                      double _fractional_seconds_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_security_market_view_->secname() << ' ' << _num_levels_ << ' '
              << _level_decay_factor_ << ' ' << _fractional_seconds_;

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ValueWBPMomentum *> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new ValueWBPMomentum(t_dbglogger_, r_watch_, _indep_security_market_view_, concise_indicator_description_,
                             _num_levels_, _level_decay_factor_, _fractional_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

ValueWBPMomentum::ValueWBPMomentum(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                   SecurityMarketView *_indep_security_market_view_,
                                   std::string _concise_indicator_description_, unsigned int _num_levels_,
                                   double _level_decay_factor_, double _fractional_seconds_)
    : CommonIndicator(t_dbglogger_, r_watch_, _concise_indicator_description_),
      indep_market_view_(_indep_security_market_view_),
      num_levels_(_num_levels_),
      level_decay_factor_(_level_decay_factor_),
      book_change_history_halflife_msecs_(std::max(
          WATCH_DEFAULT_PAGE_WIDTH_MSECS,
          MathUtils::GetCeilMultipleOf((int)round(1000 * _fractional_seconds_), WATCH_DEFAULT_PAGE_WIDTH_MSECS))),
      last_mkt_status_(kMktTradingStatusOpen) {
  shortcode_ = indep_market_view_->shortcode();

  indep_market_view_->subscribe_MktStatus(this);

  bidpressure_ = 0;
  askpressure_ = 0;
  synthetic_volume_ = 0.0;
  percentage_ = 1.0;
//( ShortcodeSecurityMarketViewMap::GetUniqueInstance ( ) ).GetSecurityMarketViewVec ( shortcode_vec_,
// indep_market_view_vec_ )  ;

#define MAX_LEVEL_DECAY_VECTOR_SIZE 12u
  for (auto i = 0u; i < MAX_LEVEL_DECAY_VECTOR_SIZE; i++) {
    level_decay_vector_[i] = pow(level_decay_factor_, (int)i);
  }
#undef MAX_LEVEL_DECAY_VECTOR_SIZE

  {
    int number_fadeoffs_ = std::max(1, (int)ceil(book_change_history_halflife_msecs_ / WATCH_DEFAULT_PAGE_WIDTH_MSECS));
    double decay_page_factor_ = MathUtils::CalcDecayFactor(number_fadeoffs_);

#define MAX_TIME_DECAY_VECTOR_SIZE 64
    for (auto i = 0u; i < MAX_TIME_DECAY_VECTOR_SIZE; i++) {
      time_decay_vector_[i] = pow(decay_page_factor_, (int)i);
    }
#undef MAX_TIME_DECAY_VECTOR_SIZE
  }

  watch_.subscribe_TimePeriod(this);  // for OnTimePeriodUpdate
  // subscribe to all updates
  indep_market_view_->subscribePlEvents(this);
  indep_market_view_->subscribe_L2(this);
  indep_market_view_->ComputeIntPriceLevels();

  std::ifstream shortcode_list_file;
  std::string filename_ = GetBovespaIndexConstituentFileList(watch_.YYYYMMDD());
  DBGLOG_TIME_CLASS_FUNC_LINE << " Constituent list file picked: " << filename_ << DBGLOG_ENDL_FLUSH;
  shortcode_list_file.open(filename_.c_str(), std::ifstream::in);
  if (shortcode_list_file.is_open()) {
    while (shortcode_list_file.good()) {
      // # Name _theoritical_quantity_ percentage_contribution_in_index_
      // _theoretical_quantity_is and reductor both are divided by factor of 10^11
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);
      shortcode_list_file.getline(readline_buffer_, kL1AvgBufferLen);
      if (readline_buffer_[0] == '#') {
        continue;
      }

      // We expect to read lines like:
      // "shortcode_ theoretical_volume_"
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      auto tokens_ = st_.GetTokens(); // Switched to auto from const std::vector<const char *> &
      if (tokens_.size() < 2) {
        continue;
      }
      std::string t_shc_ = tokens_[0];
      if (tokens_.size() > 4 && std::string(tokens_[1]).compare("Reductor") == 0) {
        continue;
      }
      if (t_shc_.compare(_indep_security_market_view_->shortcode()) == 0) {
        synthetic_volume_ = atof(tokens_[1]) / 100000000000;
        percentage_ = atof(tokens_[2]) / 1000000;  // just a factor since the values may go beyond 10^9
        break;
      }
      bzero(readline_buffer_, kL1AvgBufferLen);
    }
  }
}

// use l1_size adjusted values
void ValueWBPMomentum::OnPLNew(const unsigned int t_security_id_, const MarketUpdateInfo &r_market_update_info_,
                               const TradeType_t t_buysell_, const int t_level_added_, const int t_old_size_,
                               const int t_new_size_, const int t_old_ordercount_, const int t_new_ordercount_,
                               const int t_int_price_, const int t_int_price_level_,
                               const bool t_is_intermediate_message_) {
  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (t_level_added_ < num_levels_) {
        // get_doublepx call can be optimized by just callung it once while returning hte indicator value
        bidpressure_ += t_int_price_ * t_new_size_ * level_decay_vector_[std::max(0, t_level_added_)];
        if (dbglogger_.CheckLoggingLevel(MUM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << indep_market_view_->shortcode() << " " << t_level_added_ << " " << t_new_size_
                                 << " bp " << bidpressure_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } break;
    case kTradeTypeSell: {
      if (t_level_added_ < num_levels_) {
        askpressure_ += t_int_price_ * t_new_size_ * level_decay_vector_[std::max(0, t_level_added_)];
        if (dbglogger_.CheckLoggingLevel(MUM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << indep_market_view_->shortcode() << " " << t_level_added_ << " " << t_new_size_
                                 << " ap " << askpressure_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } break;
    default: {}
  }
}

void ValueWBPMomentum::OnPLDelete(const unsigned int t_security_id_, const MarketUpdateInfo &r_market_update_info_,
                                  const TradeType_t t_buysell_, const int t_level_removed_, const int t_old_size_,
                                  const int t_old_ordercount_, const int t_int_price_, const int t_int_price_level_,
                                  const bool t_is_intermediate_message_) {
  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (t_level_removed_ < num_levels_) {
        bidpressure_ -= t_int_price_ * t_old_size_ * level_decay_vector_[std::max(0, t_level_removed_)];
        if (dbglogger_.CheckLoggingLevel(MUM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << indep_market_view_->shortcode() << " " << t_level_removed_ << " " << t_old_size_
                                 << " bp " << bidpressure_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } break;
    case kTradeTypeSell: {
      if (t_level_removed_ < num_levels_) {
        askpressure_ -= t_int_price_ * t_old_size_ * level_decay_vector_[std::max(0, t_level_removed_)];
        if (dbglogger_.CheckLoggingLevel(MUM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << indep_market_view_->shortcode() << " " << t_level_removed_ << " " << t_old_size_
                                 << " ap " << askpressure_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } break;
    default: {}
  }
}

void ValueWBPMomentum::OnPLChange(const unsigned int t_security_id_, const MarketUpdateInfo &r_market_update_info_,
                                  const TradeType_t t_buysell_, const int t_level_changed_, const int t_int_price_,
                                  const int t_int_price_level_, const int t_old_size_, const int t_new_size_,
                                  const int t_old_ordercount_, const int t_new_ordercount_,
                                  const bool t_is_intermediate_message_) {
  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (t_level_changed_ < num_levels_) {
        bidpressure_ += (t_new_size_ - t_old_size_) * t_int_price_ * level_decay_vector_[std::max(0, t_level_changed_)];
        if (dbglogger_.CheckLoggingLevel(MUM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << indep_market_view_->shortcode() << " " << t_level_changed_ << " " << t_old_size_
                                 << " " << t_new_size_ << " bp " << bidpressure_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } break;
    case kTradeTypeSell: {
      if (t_level_changed_ < num_levels_) {
        // Converted to price while notifying
        askpressure_ += (t_new_size_ - t_old_size_) * t_int_price_ * level_decay_vector_[std::max(0, t_level_changed_)];
        if (dbglogger_.CheckLoggingLevel(MUM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << indep_market_view_->shortcode() << " " << t_level_changed_ << " " << t_old_size_
                                 << " " << t_new_size_ << " ap " << askpressure_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } break;
    default: {}
  }
}

void ValueWBPMomentum::OnMarketUpdate(const unsigned int _security_id_,
                                      const MarketUpdateInfo &cr_market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_ != NULL && indep_market_view_->is_ready_complex(2)) {
      is_ready_ = true;
      indicator_value_ = 0;
      NotifyIndicatorListeners(indicator_value_);
    }

  } else {
    if (indep_market_view_->mkt_size_weighted_price() == 0.0) {
      indicator_value_ = 0;
    } else {
      // indicator_value_ =  indep_market_view_->GetDoublePx(( bidpressure_ - askpressure_ ) ) / ( avg_l1_size_  *
      // indep_market_view_->mkt_size_weighted_price() ) ;
      indicator_value_ = indep_market_view_->GetDoublePx((bidpressure_ - askpressure_)) * percentage_;
      // percentage is doing better than
      // GetDblPx function here is just for optimization, it is actually applied to int price while calculating the
      // pressure
    }
    // DBGLOG_TIME_CLASS_FUNC_LINE << " indicator: " << indep_market_view_->shortcode()<< " " << indicator_value_ <<
    // "bp-ap: " << bidpressure_-askpressure_  << " perc: "<< percentage_ << DBGLOG_ENDL_FLUSH;
    if (data_interrupted_) indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

// market_interrupt_listener interface
void ValueWBPMomentum::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  // If this is not an auction then we dont need to do any thing
  // If this is an auction then we need to reinitialize values

  if (indep_market_view_ != NULL && indep_market_view_->security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else {
    return;
  }
}

void ValueWBPMomentum::OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_) {
  if (_new_market_status_ != last_mkt_status_) {
    if (_new_market_status_ == kMktTradingStatusReserved) {
      indicator_value_ = 0;
    }
    last_mkt_status_ = _new_market_status_;
  }
}

void ValueWBPMomentum::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_ != NULL && indep_market_view_->security_id() == _security_id_) {
    data_interrupted_ = false;
  } else
    return;
}
}
