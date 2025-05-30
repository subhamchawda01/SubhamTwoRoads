/**
   \file ModelMath/sumvars_logger.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#pragma once
#include "dvccode/Utils/in_memory_data.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/ModelMath/indicator_logger.hpp"

namespace HFSAT {

class SumVarsLogger : public IndicatorLogger {
 public:
  SumVarsLogger(DebugLogger& _dbglogger_, const Watch& _watch_, BulkFileWriter& _bulk_file_writer_,
                EconomicEventsManager& r_economic_events_manager_, const std::string& _model_filename_,
                SecurityMarketView* _p_dep_market_view_, PriceType_t _dep_baseprice_type_,
                PriceType_t _dep_pred_price_type_, const std::string& _output_filename_,
                const unsigned int t_msecs_to_wait_to_print_again_, const unsigned long long t_l1events_timeout_,
                const unsigned int t_num_trades_to_wait_print_again_, const unsigned int t_to_print_on_economic_times_,
                const unsigned int t_sample_on_core_shortcodes_,
                const std::vector<SecurityMarketView*>& t_p_sampling_shc_smv_vec_,
                const std::vector<double> t_c3_required_cutoffs_, unsigned int t_regime_mode_to_print_ = 0);

  ~SumVarsLogger() {}

 protected:
  /**
   * It Overloads the base function. It adds the new row of indicator values in InMemData instead print the row in
   * bulk_file_writer_
   */
  inline void PrintVals() {
    if ((is_ready_) && (ReachedTimePeriodToPrint() || ReachedEventPeriodToPrint() || ReachedNumTradesToPrint()) &&
        (watch_.msecs_from_midnight() > min_msec_toprint_) && (watch_.msecs_from_midnight() < max_msec_toprint_) &&
        (economic_events_allows_print_) &&
        (!only_print_on_economic_events_ || (only_print_on_economic_events_ && currently_tradable_)) &&
        (regime_mode_to_print_ > 0 ? prev_value_vec_[0] == regime_mode_to_print_ : true)) {
      if (sample_on_core_shortcodes_ == 1u) {
        last_print_l1events_ = CoreShortCodesAndDepl1events();
      } else if (sample_on_core_shortcodes_ == 2u) {
        for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
          { last_print_trigger_values_[i] = p_sampling_shc_smv_vec_[i]->l1events(); }
        }
      } else if (sample_on_core_shortcodes_ == 3u) {
        for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
          { last_print_trigger_values_[i] = p_sampling_shc_smv_vec_[i]->mkt_size_weighted_price(); }
        }
      } else {
        last_print_l1events_ = l1events();
      }

      bulk_file_writer_ << watch_.msecs_from_midnight() << ' ' << l1events();

      if (use_implied_price_ && p_implied_price_indicator_ && p_implied_price_indicator_->IsIndicatorReady()) {
        bulk_file_writer_ << ' ' << p_implied_price_indicator_->GetBaseImpliedPrice(dep_pred_price_type_) << ' '
                          << p_implied_price_indicator_->GetBaseImpliedPrice(dep_baseprice_type_);
      } else if (p_dep_market_view_ != NULL) {
        bulk_file_writer_ << ' ' << p_dep_market_view_->price_from_type(dep_pred_price_type_) << ' '
                          << p_dep_market_view_->price_from_type(dep_baseprice_type_);
      }

      sum_vars_ = 0.0;
      for (unsigned int i = (regime_mode_to_print_ > 0 ? 1 : 0); i < indicator_vec_.size(); i++) {
        // linear
        if (model_type_ == 1) {
          sum_vars_ += prev_value_vec_[i];
        }
      }
      bulk_file_writer_ << ' ' << sum_vars_;
      bulk_file_writer_ << '\n';
      bulk_file_writer_.CheckToFlushBuffer();

      last_print_msecs_from_midnight_ = watch_.msecs_from_midnight();
      last_print_num_trades_ = num_trades();
    }
  }

  double sum_vars_;
  int model_type_;
};
}
