/**
   \file ModelMath/indicator_stats.hpp

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

class IndicatorStats : public IndicatorLogger {
 public:
  std::vector<double> indicator_weights_;
  IndicatorStats(DebugLogger& _dbglogger_, const Watch& _watch_, BulkFileWriter& _bulk_file_writer_,
                 EconomicEventsManager& r_economic_events_manager_, const std::string& _model_filename_,
                 SecurityMarketView* _p_dep_market_view_, PriceType_t _dep_baseprice_type_,
                 PriceType_t _dep_pred_price_type_, const std::string& _output_filename_,
                 const unsigned int t_msecs_to_wait_to_print_again_, const unsigned long long t_l1events_timeout_,
                 const unsigned int t_num_trades_to_wait_print_again_, const unsigned int t_to_print_on_economic_times_,
                 const unsigned int t_sample_on_core_shortcodes_,
                 const std::vector<SecurityMarketView*>& t_p_sampling_shc_smv_vec_,
                 const std::vector<double> t_c3_required_cutoffs_, unsigned int t_regime_mode_to_print_ = 0,
                 HFSAT::IndicatorLogger::DatagenStats_t _samples_print_ = HFSAT::IndicatorLogger::kIndStats,
                 int _sample_duration_secs_ = 900, bool _write_to_file_ = false, bool t_live_trading = false,
                 std::map<int, std::pair<std::string, std::string> > t_stat_samples_map_live =
                     std::map<int, std::pair<std::string, std::string> >());
  ~IndicatorStats() {
    if (!write_to_file_) {
      remove(output_filename_.c_str());
    }
    if (samples_print_ == HFSAT::IndicatorLogger::kIndStats) {
      PrintStats();
    } else if (samples_print_ == HFSAT::IndicatorLogger::kSampleStats) {
      PrintStatsSamples(next_sample_mfm_);
    } else if (samples_print_ == HFSAT::IndicatorLogger::kPnlBasedStats) {
      PrintPnlBasedStats();
    }
    delete ind_data_;
  }

 protected:
  /**
   * It Overloads the base function. It adds the new row of indicator values in InMemData instead print the row in
   * bulk_file_writer_
   */
  inline void PrintVals() {  // necassary to be inline ?
    if (!next_sample_mfm_updated) UpdateNextSampleMfm();
    if (samples_print_ == HFSAT::IndicatorLogger::kSampleStats && watch_.msecs_from_midnight() > next_sample_mfm_) {
      PrintStatsSamples(next_sample_mfm_);
      delete ind_data_;
      ind_data_ = new InMemData();
      next_sample_mfm_ = next_sample_mfm_ + sample_duration_msecs_;
    }
    if ((is_ready_) && (ReachedTimePeriodToPrint() || ReachedEventPeriodToPrint() || ReachedNumTradesToPrint()) &&
        (watch_.msecs_from_midnight() > min_msec_toprint_) && (watch_.msecs_from_midnight() < max_msec_toprint_) &&
        (economic_events_allows_print_) &&
        (!only_print_on_economic_events_ || (only_print_on_economic_events_ && currently_tradable_)) &&
        (regime_mode_to_print_ > 0 ? prev_value_vec_[0] == regime_mode_to_print_ : true)) {
      if (sample_on_core_shortcodes_ == 1u) {
        last_print_l1events_ = CoreShortCodesAndDepl1events();
      } else if (sample_on_core_shortcodes_ == 2u) {
        for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
          last_print_trigger_values_[i] = p_sampling_shc_smv_vec_[i]->l1events();
        }
      } else if (sample_on_core_shortcodes_ == 3u) {
        for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
          last_print_trigger_values_[i] = p_sampling_shc_smv_vec_[i]->mkt_size_weighted_price();
        }
      } else {
        last_print_l1events_ = l1events();
      }

      for (unsigned int i = (regime_mode_to_print_ > 0 ? 1 : 0); i < indicator_vec_.size(); i++) {
        if (indicator_name_vec_[i].find("SlowCorrCalculator") != std::string::npos ||
            samples_print_ == HFSAT::IndicatorLogger::kPnlBasedStats) {
          ind_data_->AddWord(prev_value_vec_[i]);
        } else {
          ind_data_->AddWord(fabs(prev_value_vec_[i]));
        }
      }

      ind_data_->FinishLine();
      last_print_msecs_from_midnight_ = watch_.msecs_from_midnight();
      last_print_num_trades_ = num_trades();
    }
  }

  /// Update NextSampleMfm to be in whole number multiples of SampleDurationMsecs
  /// This is needed when datagen is run in live mode, when the UTC_0000 can't define the start time of NextSampleMfm
  inline void UpdateNextSampleMfm() {
    int nextsamplemfmdatagen = GetTimeAfterDurationMsecs(min_msec_toprint_);
    int nextsamplemfmcurrent = GetTimeAfterDurationMsecs(watch_.msecs_from_midnight());
    if (nextsamplemfmdatagen < nextsamplemfmcurrent) {
      next_sample_mfm_ = nextsamplemfmcurrent;
      next_sample_mfm_updated = true;
    }
  }

  inline int GetTimeAfterDurationMsecs(int time_in_msecs_) {
    return (time_in_msecs_ - (time_in_msecs_ % sample_duration_msecs_) + sample_duration_msecs_);
  }

  /* ind_data has each row in the format <msecs_from_midnight> <indicator vector for this timestamp> */
  InMemData* ind_data_;
  /* to print as samples (/NAS1/SampleData) or as a summary (/spare/local/Features/) */
  HFSAT::IndicatorLogger::DatagenStats_t samples_print_;
  /* if sample_print_ is true, the sample duration(default:15mins) */
  int sample_duration_msecs_;
  /* if sample_print_ is true, it keeps the end_time of the current sample */
  int next_sample_mfm_;
  /*  if write_to_file_ is false, then write to STDOUT, else write to STATS/STATS_SAMPLES */
  bool write_to_file_;

  const std::string& output_filename_;
  // is the next sample mfm updated to the next sample end time after the watch was started
  bool next_sample_mfm_updated;
  bool live_trading;
  std::map<int, std::pair<std::string, std::string> > stat_samples_map_live;

 private:
  // stat functions
  void PrintToFilesInLive(int indx, int hhmm, double val);
  void PrintStats();
  void PrintStatsSamples(int _next_sample_mfm_);
  void PrintPnlBasedStats();
};
}
