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
#include "dvctrade/Indicators/etf_synthetic_price_diff.hpp"
#include "dvctrade/Indicators/synthetic_index.hpp"
#define INDEX_RATIO 0.001;

namespace HFSAT {

void ETFSyntheticPriceDiff::CollectShortCodes(std::vector<std::string>& shortcodes_affecting_this_indicator,
                                              std::vector<std::string>& ors_source_needed_vec,
                                              const std::vector<const char*>& tokens) {
  VectorUtils::UniqueVectorAdd(shortcodes_affecting_this_indicator, (std::string)tokens[3]);
  VectorUtils::UniqueVectorAdd(shortcodes_affecting_this_indicator, (std::string)tokens[4]);
  SyntheticIndex::CollectShortCodes(shortcodes_affecting_this_indicator, ors_source_needed_vec, tokens);
}

ETFSyntheticPriceDiff* ETFSyntheticPriceDiff::GetUniqueInstance(DebugLogger& dbglogger, const Watch& watch,
                                                                const std::vector<const char*>& tokens,
                                                                PriceType_t basepx_pxtype) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(tokens[3]);
  return GetUniqueInstance(dbglogger, watch, (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(tokens[3])),
                           (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(tokens[4])), atof(tokens[5]),
                           StringToPriceType_t(tokens[6]));
}

ETFSyntheticPriceDiff* ETFSyntheticPriceDiff::GetUniqueInstance(DebugLogger& dbglogger, const Watch& watch,
                                                                SecurityMarketView* dep_market_view,
                                                                SecurityMarketView* indep_market_view,
                                                                double fractional_seconds, PriceType_t price_type) {
  std::ostringstream temp_oss;
  temp_oss << VarName() << ' ' << dep_market_view->secname() << ' ' << indep_market_view->secname() << ' '
           << fractional_seconds << ' ' << PriceType_t_To_String(price_type);
  std::string indicator_description(temp_oss.str());

  static std::map<std::string, ETFSyntheticPriceDiff*> indicator_description_map;

  if (indicator_description_map.find(indicator_description) == indicator_description_map.end()) {
    indicator_description_map[indicator_description] = new ETFSyntheticPriceDiff(
        dbglogger, watch, indicator_description, dep_market_view, indep_market_view, fractional_seconds, price_type);
  }
  return indicator_description_map[indicator_description];
}

ETFSyntheticPriceDiff::ETFSyntheticPriceDiff(DebugLogger& dbglogger, const Watch& watch,
                                             const std::string& indicator_description,
                                             SecurityMarketView* dep_market_view, SecurityMarketView* indep_market_view,
                                             double fractional_seconds, PriceType_t price_type)
    : CommonIndicator(dbglogger, watch, indicator_description),
      dep_market_view_(dep_market_view),
      indep_market_view_(indep_market_view),
      price_type_(price_type),
      current_indep_dep_price_diff_(0),
      moving_avg_indep_dep_price_diff_(0),
      last_indep_dep_price_diff_recorded_(0),
      current_indep_synthetic_price_(0),
      dep_ready_(false) {
  trend_history_msecs_ = std::max(20, (int)round(1000 * fractional_seconds));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();

  if (!dep_market_view_->subscribe_price_type(this, price_type)) {
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << indicator_description
              << " passed " << price_type << std::endl;
  }

  synthetic_index_ = SyntheticIndex::GetUniqueInstance(dbglogger, watch, indep_market_view_, price_type);

  if (synthetic_index_ == NULL) {
    ExitVerbose(kExitErrorCodeGeneral, " Could not create the sythetic index indicator\n");
  }

  synthetic_index_->add_unweighted_indicator_listener(1u, this);
}

void ETFSyntheticPriceDiff::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_->is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_->secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void ETFSyntheticPriceDiff::OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value) {
  if (indicator_index == 1) {
    current_indep_synthetic_price_ = new_value * INDEX_RATIO;

    if (!is_ready_) {
      if (dep_ready_ || dep_market_view_->is_ready_complex(2)) {
        dep_ready_ = true;

        if (dep_ready_ && synthetic_index_->IsIndicatorReady()) {
          is_ready_ = true;
          InitializeValues();
        }
      }
    }

    if (is_ready_) {
      current_indep_dep_price_diff_ = current_indep_synthetic_price_ - currrent_dep_price_;
      ComputeIndicatorValue();
      indicator_value_ = (current_indep_dep_price_diff_ - moving_avg_indep_dep_price_diff_);
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void ETFSyntheticPriceDiff::OnMarketUpdate(const unsigned int sec_id, const MarketUpdateInfo& market_update_info) {
  currrent_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, market_update_info);

  if (!is_ready_) {
    if (dep_ready_ || dep_market_view_->is_ready_complex(2)) {
      dep_ready_ = true;
      if (dep_ready_ && synthetic_index_->IsIndicatorReady()) {
        is_ready_ = true;
        InitializeValues();
      }
    }
  }

  if (is_ready_) {
    current_indep_dep_price_diff_ = current_indep_synthetic_price_ - currrent_dep_price_;
    ComputeIndicatorValue();
    indicator_value_ = (current_indep_dep_price_diff_ - moving_avg_indep_dep_price_diff_);
    NotifyIndicatorListeners(indicator_value_);
  }
}

void ETFSyntheticPriceDiff::InitializeValues() {
  bool is_ready = true;
  current_indep_synthetic_price_ = synthetic_index_->indicator_value(is_ready) * INDEX_RATIO;
  currrent_dep_price_ = dep_market_view_->price_from_type(price_type_);
  current_indep_dep_price_diff_ = current_indep_synthetic_price_ - currrent_dep_price_;
  moving_avg_indep_dep_price_diff_ = current_indep_dep_price_diff_;

  last_indep_dep_price_diff_recorded_ = current_indep_dep_price_diff_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

void ETFSyntheticPriceDiff::ComputeIndicatorValue() {
  if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
    moving_avg_indep_dep_price_diff_ +=
        inv_decay_sum_ * (current_indep_dep_price_diff_ - last_indep_dep_price_diff_recorded_);

  } else {
    int num_pages_to_add = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);

    if (num_pages_to_add >= (int)decay_vector_.size()) {
      InitializeValues();

    } else {
      if (num_pages_to_add == 1) {
        moving_avg_indep_dep_price_diff_ =
            (current_indep_dep_price_diff_ * inv_decay_sum_) + (moving_avg_indep_dep_price_diff_ * decay_vector_[1]);

      } else {  // num_pages_to_add_ >= 2 < decay_vector_.size()

        moving_avg_indep_dep_price_diff_ =
            (current_indep_dep_price_diff_ * inv_decay_sum_) +
            (last_indep_dep_price_diff_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add - 1)]) +
            (moving_avg_indep_dep_price_diff_ * decay_vector_[num_pages_to_add]);
      }

      last_new_page_msecs_ += (num_pages_to_add * page_width_msecs_);
    }
  }
  last_indep_dep_price_diff_recorded_ = current_indep_dep_price_diff_;
}

// market_interrupt_listener interface
void ETFSyntheticPriceDiff::OnMarketDataInterrupted(const unsigned int sec_id, const int msecs_since_last_receive) {
  if (dep_market_view_->security_id() == sec_id) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void ETFSyntheticPriceDiff::OnMarketDataResumed(const unsigned int sec_id) {
  if (dep_market_view_->security_id() == sec_id) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
