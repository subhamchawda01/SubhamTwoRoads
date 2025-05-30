/**
    \file IndicatorsCode/simple_hawkes_price_process_mkt_events.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/simple_hawkes_price_process_mkt_events.hpp"

namespace HFSAT {

void SimpleHawkesPriceProcessMktEvents::CollectShortCodes(
    std::vector<std::string>& _shortcodes_affecting_this_indicator_, std::vector<std::string>& _ors_source_needed_vec_,
    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

SimpleHawkesPriceProcessMktEvents* SimpleHawkesPriceProcessMktEvents::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), _basepx_pxtype_);
}

SimpleHawkesPriceProcessMktEvents* SimpleHawkesPriceProcessMktEvents::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const SecurityMarketView& _indep_market_view_, double _beta_,
    PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _beta_ << ' ' << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SimpleHawkesPriceProcessMktEvents*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new SimpleHawkesPriceProcessMktEvents(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _beta_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SimpleHawkesPriceProcessMktEvents::SimpleHawkesPriceProcessMktEvents(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                     const std::string& concise_indicator_description_,
                                                                     const SecurityMarketView& _indep_market_view_,
                                                                     double _beta_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      num_moves_(10),  // hardcoded as it is not causing much difference
      beta_(_beta_),
      up_move_index_(0),
      down_move_index_(0),
      up_move_prob_(0),
      down_move_prob_(0),
      current_indep_int_price_(0),
      last_indep_int_price_(0) {
  if (!indep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice)) {
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << kPriceTypeMktSizeWPrice << std::endl;
  }
  for (auto i = 0u; i < num_moves_; i++) {
    up_moves_event_vec_.push_back(0);
    down_moves_event_vec_.push_back(0);
  }
}

void SimpleHawkesPriceProcessMktEvents::OnMarketUpdate(const unsigned int _security_id_,
                                                       const MarketUpdateInfo& cr_market_update_info_) {
  current_indep_int_price_ = cr_market_update_info_.bestbid_int_price_ + cr_market_update_info_.bestask_int_price_;

  if (!is_ready_) {
    if (indep_market_view_.is_ready() &&
        (indep_market_view_.spread_increments() < (2 * indep_market_view_.normal_spread_increments()))) {
      is_ready_ = true;

      indicator_value_ = 0;
    }
  } else if (!data_interrupted_) {
    if (last_indep_int_price_ > current_indep_int_price_) {
      last_indep_int_price_ = current_indep_int_price_;
      down_moves_event_vec_[down_move_index_] = cr_market_update_info_.l1events_;
      if (down_move_index_ == (int)(down_moves_event_vec_.size() - 1)) {
        down_move_index_ = 0;
      } else {
        down_move_index_ = down_move_index_ + 1;
      }
    } else if (last_indep_int_price_ < current_indep_int_price_) {
      last_indep_int_price_ = current_indep_int_price_;
      up_moves_event_vec_[up_move_index_] = cr_market_update_info_.l1events_;
      if (up_move_index_ == (int)(down_moves_event_vec_.size() - 1)) {
        up_move_index_ = 0;
      } else {
        up_move_index_ = up_move_index_ + 1;
      }
    }

    long long t_l1events_ = cr_market_update_info_.l1events_;

    down_move_prob_ = 0.0;
    up_move_prob_ = 0.0;

    for (auto i = 0u; i < down_moves_event_vec_.size(); i++) {
      down_move_prob_ += exp(-beta_ * (t_l1events_ - down_moves_event_vec_[i]));
      up_move_prob_ += exp(-beta_ * (t_l1events_ - up_moves_event_vec_[i]));
    }

    indicator_value_ = up_move_prob_ - down_move_prob_;
  }

  NotifyIndicatorListeners(indicator_value_);
}

void SimpleHawkesPriceProcessMktEvents::InitializeValues() { indicator_value_ = 0; }
// market_interrupt_listener interface
void SimpleHawkesPriceProcessMktEvents::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                                const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void SimpleHawkesPriceProcessMktEvents::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
