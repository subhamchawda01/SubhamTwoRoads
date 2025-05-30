/**
   \file ModelMath/sumvars_logger.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include "dvctrade/ModelMath/sumvars_logger.hpp"

namespace HFSAT {
SumVarsLogger::SumVarsLogger(DebugLogger& _dbglogger_, const Watch& _watch_, BulkFileWriter& _bulk_file_writer_,
                             EconomicEventsManager& r_economic_events_manager_, const std::string& _model_filename_,
                             SecurityMarketView* _p_dep_market_view_, PriceType_t _dep_baseprice_type_,
                             PriceType_t _dep_pred_price_type_, const std::string& _output_filename_,
                             const unsigned int t_msecs_to_wait_to_print_again_,
                             const unsigned long long t_l1events_timeout_,
                             const unsigned int t_num_trades_to_wait_print_again_,
                             const unsigned int t_to_print_on_economic_times_,
                             const unsigned int t_sample_on_core_shortcodes_,
                             const std::vector<SecurityMarketView*>& t_p_sampling_shc_smv_vec_,
                             const std::vector<double> t_c3_required_cutoffs_, unsigned int t_regime_mode_to_print_)
    : IndicatorLogger(_dbglogger_, _watch_, _bulk_file_writer_, r_economic_events_manager_, _model_filename_,
                      _p_dep_market_view_, _dep_pred_price_type_, _dep_pred_price_type_, _output_filename_,
                      t_msecs_to_wait_to_print_again_, t_l1events_timeout_, t_num_trades_to_wait_print_again_,
                      t_to_print_on_economic_times_, t_sample_on_core_shortcodes_, t_p_sampling_shc_smv_vec_,
                      t_c3_required_cutoffs_, t_regime_mode_to_print_) {
  model_type_ = 1;
  sum_vars_ = 0.0;
}
}
