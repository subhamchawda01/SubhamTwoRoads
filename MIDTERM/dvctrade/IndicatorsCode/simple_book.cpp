#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/simple_book.hpp"

namespace HFSAT {
void SimpleBook::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                   std::vector<std::string>& _ors_source_needed_vec_,
                                   const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

SimpleBook* SimpleBook::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                          const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _num_levels_ _decay_factor_
  double t_stdev_duration_ = 0;
  if (r_tokens_.size() < 6) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "SimpleBook incorrect syntax. Should be INDICATOR _this_weight_ _indicator_string_ "
                "_indep_market_view_ _num_levels_ _decay_factor_ ");
  } else if (r_tokens_.size() >= 7) {
    if (std::string(r_tokens_[6]).compare("#") != 0) {
      t_stdev_duration_ = atof(r_tokens_[6]);
    }
  }

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           std::max(0, atoi(r_tokens_[4])), atof(r_tokens_[5]), _basepx_pxtype_, t_stdev_duration_);
}

SimpleBook* SimpleBook::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                          SecurityMarketView& _indep_market_view_, unsigned int _num_levels_,
                                          double _decay_factor_, PriceType_t _basepx_pxtype_, double _stdev_duration_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _num_levels_ << ' ' << _decay_factor_
              << " " << _stdev_duration_ << " " << PriceType_t_To_String(_basepx_pxtype_);

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SimpleBook*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new SimpleBook(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _num_levels_,
                       _decay_factor_, _stdev_duration_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SimpleBook::SimpleBook(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                       const std::string& concise_indicator_description_, SecurityMarketView& _indep_market_view_,
                       unsigned int _num_levels_, double _decay_factor_, double _stdev_duration_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      num_levels_(std::max((unsigned int)1, _num_levels_)),
      decay_factor_(_decay_factor_),
      stdev_duration_(_stdev_duration_),
      book_info_manager_(*(BookInfoManager::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view_))) {
  book_info_manager_.ComputeSumSize(num_levels_, decay_factor_, stdev_duration_);
  book_info_manager_.ComputeSumFactorSize(num_levels_, decay_factor_, stdev_duration_);

  book_info_struct_ = book_info_manager_.GetBookInfoStruct(num_levels_, decay_factor_, stdev_duration_);

  if (book_info_struct_ == NULL) {
    std::cerr << "Error getting book_info_struct for SimpleBook: " << _num_levels_ << " " << _decay_factor_
              << std::endl;
  }

  int num_exponents_ = std::max(10, (int)(2 * _num_levels_));
  decay_vector_.resize(num_exponents_);

  for (auto i = 0u; i < decay_vector_.size(); i++) {
    decay_vector_[i] = pow(sqrt(decay_factor_), (int)i);
  }

  indep_market_view_.subscribe_L2(this);
  indep_market_view_.ComputeIntPriceLevels();
}

void SimpleBook::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& cr_market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready() &&
        (indep_market_view_.spread_increments() < (2 * indep_market_view_.normal_spread_increments()))) {
      is_ready_ = true;
      indicator_value_ = 0;
      NotifyIndicatorListeners(indicator_value_);
    }
  } else {
    if (cr_market_update_info_.bestbid_int_price_ != indep_market_view_.bid_int_price(0) ||
        cr_market_update_info_.bestask_int_price_ != indep_market_view_.ask_int_price(0)) {
      indicator_value_ = 0;
      return;
    }

    if ((book_info_struct_->sum_bid_size_ <= 0) || (book_info_struct_->sum_ask_size_ <= 0)) {
      indicator_value_ = 0;
    } else {
      indicator_value_ = (book_info_struct_->sum_bid_factor_size_ - book_info_struct_->sum_ask_factor_size_) /
                         (book_info_struct_->sum_bid_size_ + book_info_struct_->sum_ask_size_);
    }

    if (std::isnan(indicator_value_)) {
      std::cerr << __PRETTY_FUNCTION__ << " nan in " << concise_indicator_description() << std::endl;
      indicator_value_ = 0;
    }
    if (data_interrupted_) indicator_value_ = 0;

    NotifyIndicatorListeners(indicator_value_);
  }
}
void SimpleBook::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void SimpleBook::OnMarketDataResumed(const unsigned int _security_id_) { data_interrupted_ = false; }
}
