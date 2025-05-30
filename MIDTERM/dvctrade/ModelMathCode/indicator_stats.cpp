/**
   \file ModelMath/indicator_stats.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include "dvctrade/ModelMath/indicator_stats.hpp"

namespace HFSAT {
IndicatorStats::IndicatorStats(
    DebugLogger& _dbglogger_, const Watch& _watch_, BulkFileWriter& _bulk_file_writer_,
    EconomicEventsManager& r_economic_events_manager_, const std::string& _model_filename_,
    SecurityMarketView* _p_dep_market_view_, PriceType_t _dep_baseprice_type_, PriceType_t _dep_pred_price_type_,
    const std::string& _output_filename_, const unsigned int t_msecs_to_wait_to_print_again_,
    const unsigned long long t_l1events_timeout_, const unsigned int t_num_trades_to_wait_print_again_,
    const unsigned int t_to_print_on_economic_times_, const unsigned int t_sample_on_core_shortcodes_,
    const std::vector<SecurityMarketView*>& t_p_sampling_shc_smv_vec_, const std::vector<double> t_c3_required_cutoffs_,
    unsigned int t_regime_mode_to_print_, HFSAT::IndicatorLogger::DatagenStats_t _samples_print_,
    int _sample_duration_secs_, bool _write_to_file_, bool t_live_trading,
    std::map<int, std::pair<std::string, std::string> > t_stat_samples_map_live)
    : IndicatorLogger(_dbglogger_, _watch_, _bulk_file_writer_, r_economic_events_manager_, _model_filename_,
                      _p_dep_market_view_, _dep_pred_price_type_, _dep_pred_price_type_, _output_filename_,
                      t_msecs_to_wait_to_print_again_, t_l1events_timeout_, t_num_trades_to_wait_print_again_,
                      t_to_print_on_economic_times_, t_sample_on_core_shortcodes_, t_p_sampling_shc_smv_vec_,
                      t_c3_required_cutoffs_, t_regime_mode_to_print_),
      indicator_weights_(),
      ind_data_(new InMemData()),
      samples_print_(_samples_print_),
      sample_duration_msecs_(1000 * _sample_duration_secs_),
      next_sample_mfm_(min_msec_toprint_ + sample_duration_msecs_),
      write_to_file_(_write_to_file_),
      output_filename_(_output_filename_),
      next_sample_mfm_updated(false),
      live_trading(t_live_trading) {
  stat_samples_map_live = t_stat_samples_map_live;
}

void IndicatorStats::PrintStats() {
  std::vector<double> ind_vals_(ind_data_->NumLines(), 0);
  for (int i = 0; i < ind_data_->NumWords(); i++) {
    ind_data_->GetColumn(i, ind_vals_);

    if (write_to_file_) {
      bulk_file_writer_ << p_dep_market_view_->shortcode() << " " << min_msec_toprint_ << " " << max_msec_toprint_
                        << " " << VectorUtils::GetMean(ind_vals_) << " " << VectorUtils::GetMedian(ind_vals_) << " "
                        << VectorUtils::GetStdev(ind_vals_) << " " << VectorUtils::GetMin(ind_vals_) << " "
                        << VectorUtils::GetMax(ind_vals_) << " " << VectorUtils::GetMeanHighestQuartile(ind_vals_)
                        << "\n";
      bulk_file_writer_.CheckToFlushBuffer();
    } else {
      std::cout << p_dep_market_view_->shortcode() << " " << min_msec_toprint_ << " " << max_msec_toprint_ << " "
                << VectorUtils::GetMean(ind_vals_) << " " << VectorUtils::GetMedian(ind_vals_) << " "
                << VectorUtils::GetStdev(ind_vals_) << " "
                // << VectorUtils::GetMin(ind_vals_) << " " << VectorUtils::GetMax(ind_vals_) << " "
                << VectorUtils::GetMeanHighestQuartile(ind_vals_) << " "
                << VectorUtils::GetMeanLowestQuartile(ind_vals_) << std::endl;
    }
  }
}

void IndicatorStats::PrintToFilesInLive(int indx, int hhmm, double val) {
  std::string feature_key = stat_samples_map_live[indx].first;
  std::string fname_ = "/spare/local/logs/datalogs/" + p_dep_market_view_->shortcode() + "/" +
                       std::to_string(watch_.YYYYMMDD()) + "/" + feature_key + ".txt";
  HFSAT::FileUtils::MkdirEnclosing(fname_);
  std::ofstream fhandle_;
  fhandle_.open(fname_, std::ios_base::app);
  fhandle_ << std::to_string(hhmm) << " " << val << "\n";
  fhandle_.close();
}

void IndicatorStats::PrintStatsSamples(int _next_sample_mfm_) {
  if (ind_data_->NumLines() > 0) {
    std::vector<double> ind_vals_(ind_data_->NumLines(), 0);
    for (int i = 0; i < ind_data_->NumWords(); i++) {
      ind_data_->GetColumn(i, ind_vals_);
      int hhmm_ = GetHHMMSSFromMsecsFromMidnight(_next_sample_mfm_, 1, 0) / 100;

      if (write_to_file_) {
        if (indicator_weights_[i] == -1.0)
          bulk_file_writer_ << "INDICATOR_" << i << " " << hhmm_ << " " << VectorUtils::GetAbsMax(ind_vals_) << "\n";
        else
          bulk_file_writer_ << "INDICATOR_" << i << " " << hhmm_ << " " << VectorUtils::GetMean(ind_vals_) << "\n";
        bulk_file_writer_.CheckToFlushBuffer();
      } else if (live_trading) {
        if (indicator_weights_[i] == -1.0)
          PrintToFilesInLive(i, hhmm_, VectorUtils::GetAbsMax(ind_vals_));
        else
          PrintToFilesInLive(i, hhmm_, VectorUtils::GetMean(ind_vals_));
      } else {
        if (indicator_weights_[i] == -1.0)
          std::cout << "INDICATOR_" << i << " " << hhmm_ << " " << VectorUtils::GetAbsMax(ind_vals_) << std::endl;
        else
          std::cout << "INDICATOR_" << i << " " << hhmm_ << " " << VectorUtils::GetMean(ind_vals_) << std::endl;
      }
    }
  }
}

/// Prints concise output for use in pnl based modelling.
/// We need to compute stdev of each indicator across days, so we're dumping mean and mean_sqaured and num_instances.
void IndicatorStats::PrintPnlBasedStats() {
  std::vector<double> ind_vals_(ind_data_->NumLines(), 0);
  for (int i = 0; i < ind_data_->NumWords(); i++) {
    ind_data_->GetColumn(i, ind_vals_);

    std::vector<double> ind_products(ind_data_->NumWords(), 0);
    for (int j = 0; j < ind_data_->NumWords(); j++) {
      std::vector<double> col2_data_(ind_data_->NumLines(), 0);
      ind_data_->GetColumn(j, col2_data_);
      std::vector<double> ind1_ind2_product(ind_data_->NumLines(), 0);
      for (auto k = 0u; k < std::min(ind_vals_.size(), col2_data_.size()); k++) {
        ind1_ind2_product[k] = ind_vals_[k] * col2_data_[k];
      }
      ind_products[j] = VectorUtils::GetMean(ind1_ind2_product);
    }

    std::cout << "PNL_BASED_STATS " << ind_data_->NumLines() << " " << VectorUtils::GetMean(ind_vals_) << " "
              << VectorUtils::GetMeanSquared(ind_vals_);

    for (int j = 0; j < ind_data_->NumWords(); j++) {
      std::cout << " " << ind_products[j];
    }
    std::cout << "\n";

    bulk_file_writer_.CheckToFlushBuffer();
  }
}
}
