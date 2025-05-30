/**
   \file IndicatorsCode/expression_indicator.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/expression_indicator.hpp"

namespace HFSAT {

void ExpressionIndicator::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                            std::vector<std::string>& _ors_source_needed_vec_,
                                            const std::vector<const char*>& r_tokens_) {
  // We get complete line as r_tokens_
  // Get the tokens for individual Indicators and call their collectshortcodes

  int tokens_sz = r_tokens_.size();
  int indicator_start_index = 4;
  // end_idx = start_idx + num_args_of_indicator + "2 ( for weight and indicator name )"
  int indicator_end_index = indicator_start_index + atoi(r_tokens_[indicator_start_index]) + 2;

  while (indicator_end_index < tokens_sz) {
    std::vector<const char*> new_ind_tokens_;

    for (int i = indicator_start_index; i <= indicator_end_index; i++) {
      new_ind_tokens_.push_back(r_tokens_[i]);
    }

    // new_ind_tokens_ has the tokens for the indicator, call collect shoprtcode for that
    (CollectShortCodeFunc(new_ind_tokens_[2]))(_shortcodes_affecting_this_indicator_, _ors_source_needed_vec_,
                                               new_ind_tokens_);

    // update the next start and end indexes
    indicator_start_index = indicator_end_index + 1;

    if (indicator_start_index >= tokens_sz || strcmp(r_tokens_[indicator_start_index], "#") == 0 ||
        strcmp(r_tokens_[indicator_start_index], "PARAMS") == 0) {
      break;
    }

    indicator_end_index = indicator_start_index + atoi(r_tokens_[indicator_start_index]) + 2;
  }
}

ExpressionIndicator* ExpressionIndicator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                            const std::vector<const char*>& r_tokens_,
                                                            PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;

  for (unsigned int i = 2; i < r_tokens_.size(); i++) {
    t_temp_oss_ << r_tokens_[i] << ' ';
  }

  t_temp_oss_ << PriceType_t_To_String(_basepx_pxtype_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ExpressionIndicator*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new ExpressionIndicator(t_dbglogger_, r_watch_, concise_indicator_description_, r_tokens_, _basepx_pxtype_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

ExpressionIndicator::ExpressionIndicator(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                         const std::string& concise_indicator_description_,
                                         const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_), initial_regime_(1) {
  InitFunctionPointers();
  // type of expression -> SUM, MULT etc.
  exp_type_ = std::string(r_tokens_[3]);

  // check whether this exp_type is supported or not
  CheckValidExpType();

  aggregate_function = exp_to_func_map[exp_type_];

  // Get every Indicator's tok ens
  int tokens_sz = r_tokens_.size();
  int indicator_start_index = 4;
  // end_idx = start_idx + num_args_of_indicator + "2 ( for weight and indicator name )"
  int indicator_end_index = indicator_start_index + atoi(r_tokens_[indicator_start_index]) + 2;

  while (indicator_end_index < tokens_sz) {
    std::vector<const char*> new_ind_tokens_;

    for (int i = indicator_start_index; i <= indicator_end_index; i++) {
      new_ind_tokens_.push_back(r_tokens_[i]);
    }

    indicator_tokens_vec_.push_back(new_ind_tokens_);
    // update the next start and end indexes
    indicator_start_index = indicator_end_index + 1;

    if (indicator_start_index >= tokens_sz || strcmp(r_tokens_[indicator_start_index], "#") == 0 ||
        strcmp(r_tokens_[indicator_start_index], "PARAMS") == 0) {
      break;
    }

    indicator_end_index = indicator_start_index + atoi(r_tokens_[indicator_start_index]) + 2;
  }

  composite_sig_a_ = 0;
  composite_sig_b_ = 0;
  composite_beta_ = 1;

  // If PARAMS are menetioned, push them to params_array
  if (indicator_start_index < tokens_sz && strcmp(r_tokens_[indicator_start_index], "PARAMS") == 0) {
    int number_of_params = atoi(r_tokens_[indicator_start_index + 1]);
    int start_of_params = indicator_start_index + 2;
    int end_of_params = start_of_params + number_of_params;
    for (int i = start_of_params; i < end_of_params; i++) {
      params_array_.push_back(atof(r_tokens_[i]));
    }
    if (!exp_type_.compare("COMPOSITE") && number_of_params == 6) {
      if (params_array_[3] <= 0 || params_array_[4] <= 0) {
        std::cerr << " Params 4,5 need to be > 0 but got the values: " << params_array_[3] << " " << params_array_[4]
                  << std::endl;
        exit(0);
      }
      composite_sig_a_ = (params_array_[0]) / params_array_[3];
      composite_sig_b_ = params_array_[1] - (params_array_[0] * params_array_[2]) / params_array_[3];
      composite_beta_ = params_array_[5] / params_array_[4];
    } else if (!exp_type_.compare("COMPOSITE") && number_of_params == 3) {
      composite_sig_a_ = params_array_[0];
      composite_sig_b_ = params_array_[1];
      composite_beta_ = params_array_[2];
    } else if(!exp_type_.compare("SIGMOID") && number_of_params==2){
      composite_sig_a_ = params_array_[0];
      composite_sig_b_ = params_array_[1];
    }

  }

  // If the Expression type is TREND_FILTER, we must have exactly 2 Indicators.
  CheckTokensValidity();

  // initialize.
  InitializeValues(indicator_tokens_vec_.size());
  indicator_vec_.resize(indicator_tokens_vec_.size(), NULL);

  for (auto i = 0u; i < indicator_tokens_vec_.size(); i++) {
    std::vector<const char*> tokens = indicator_tokens_vec_[i];
    double ind_weight = atof(tokens[1]);

    CommonIndicatorUniqueInstancePtr indicator_uniq_instance_ptr_ = GetUniqueInstanceFunc(tokens[2]);
    CommonIndicator* _this_indicator_ = (indicator_uniq_instance_ptr_)(t_dbglogger_, r_watch_, tokens, _basepx_pxtype_);

    if (!_this_indicator_) {
      std::cerr << " Could not generate indicator: " << tokens[2] << " " << tokens[3] << " " << tokens[4] << std::endl;
      exit(0);
    }

    indicator_vec_[i] = _this_indicator_;
    is_ready_vec_[i] = indicator_vec_[i]->IsIndicatorReady();
    _this_indicator_->add_indicator_listener(i, this, ind_weight, false);
  }

  is_ready_ = AreAllReady();
}

void ExpressionIndicator::set_start_mfm(int32_t t_start_mfm_) {
  for (auto i = 0u; i < indicator_vec_.size(); i++) {
    if (indicator_vec_[i] != NULL) {
      indicator_vec_[i]->set_start_mfm(t_start_mfm_);
    }
  }
  CommonIndicator::set_start_mfm(t_start_mfm_);
}

void ExpressionIndicator::set_end_mfm(int32_t t_end_mfm_) {
  for (auto i = 0u; i < indicator_vec_.size(); i++) {
    if (indicator_vec_[i] != NULL) {
      indicator_vec_[i]->set_end_mfm(t_end_mfm_);
    }
  }
  CommonIndicator::set_end_mfm(t_end_mfm_);
}

void ExpressionIndicator::CheckValidExpType() {
  // If the given Expression type is not supported, gracefully exit
  if (exp_to_func_map.find(exp_type_) == exp_to_func_map.end()) {
    std::cerr << __FUNCTION__ << " Expression type : " << exp_type_ << " is not supported  \n";
    std::cerr << " Currently Supported : \n";

    for (auto exp_name_func_pair : exp_to_func_map) {
      std::cerr << exp_name_func_pair.first << ", ";
    }
    ExitVerbose(kExitErrorCodeGeneral);
  }
}

void ExpressionIndicator::CheckTokensValidity() {
  if (!exp_type_.compare("COMPOSITE")) {
    if (params_array_.size() != 3 && params_array_.size() != 6) {
      std::cerr << __FUNCTION__ << "Expression type: sigmoid variables(a,b) and normalization factor w required or "
                                   "a,b,mean_cv,sd_cv,sd_1,sd_2 required but num_args "
                << params_array_.size() << "\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
    if (indicator_tokens_vec_.size() != 3) {
      std::cerr << __FUNCTION__ << " Expression type : Composite requires exactly three Indicator arguments but got "
                << indicator_tokens_vec_.size() << "\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }
  if (!exp_type_.compare("DIV")) {
    if (indicator_tokens_vec_.size() != 2) {
      std::cerr << __FUNCTION__ << " Expression type : DIV requires exactly two Indicator arguments but got "
                << indicator_tokens_vec_.size() << "\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }

  if (!exp_type_.compare("TREND_FILTER")) {
    if (indicator_tokens_vec_.size() != 2) {
      std::cerr << __FUNCTION__ << " Expression type : TREND_FILTER requires exactly two Indicator arguments but got "
                << indicator_tokens_vec_.size() << "\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }

  if (!exp_type_.compare("MULTDELTA")) {
    if (indicator_tokens_vec_.size() != 4) {
      std::cerr << __FUNCTION__ << " Expression type : MULTDELTA requires exactly 4 Indicator arguments but got "
                << indicator_tokens_vec_.size() << "\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }

  if (!exp_type_.compare("MAXBYMIN")) {
    if (indicator_tokens_vec_.size() != 2) {
      std::cerr << __FUNCTION__ << " Expression type : MAXBYMIN requires exactly two Indicator arguments but got "
                << indicator_tokens_vec_.size() << "\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }

  if (!exp_type_.compare("ONLINE_COMPUTED")) {
    if (indicator_tokens_vec_.size() != 3) {
      std::cerr << __FUNCTION__
                << " Expression type : ONLINE_COMPUTED requires exactly three Indicator arguments but got "
                << indicator_tokens_vec_.size() << "\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }

  if (!exp_type_.compare("ONLINE_COMPUTED_CUTOFF")) {
    if (indicator_tokens_vec_.size() != 3) {
      std::cerr << __FUNCTION__
                << " Expression type : ONLINE_COMPUTED requires exactly three Indicator arguments but got "
                << indicator_tokens_vec_.size() << "\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }
  if (!exp_type_.compare("STDEV_CUT")) {
    if (indicator_tokens_vec_.size() != 2) {
      std::cerr << __FUNCTION__ << " Expression type : STDEV_CUT requires exactly 2 Indicator arguments but got "
                << indicator_tokens_vec_.size() << "\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }

  if (!exp_type_.compare("STDEV_CUT_NEW")) {
    if (indicator_tokens_vec_.size() != 2) {
      std::cerr << __FUNCTION__ << " Expression type : STDEV_CUT_NEW requires exactly 2 Indicator arguments but got "
                << indicator_tokens_vec_.size() << "\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }

  if (!exp_type_.compare("TREND_DIFF")) {
    if (indicator_tokens_vec_.size() != 2) {
      std::cerr << __FUNCTION__ << " Expression type : TREND_DIFF requires exactly 2 Indicator arguments but got "
                << indicator_tokens_vec_.size() << "\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }

  if (!exp_type_.compare("REGIME_CUTOFF")) {
    if (indicator_tokens_vec_.size() != 2) {
      std::cerr << __FUNCTION__ << " Expression type : REGIME_CUTOFF requires exactly 2 Indicator arguments but got "
                << indicator_tokens_vec_.size() << "\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }

  if (!exp_type_.compare("MOD")) {
    if (indicator_tokens_vec_.size() != 1) {
      std::cerr << __FUNCTION__ << " Expression type : MOD requires exactly 1 Indicator arguments but got "
                << indicator_tokens_vec_.size() << "\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }

  if (!exp_type_.compare("SIGN")) {
    if (indicator_tokens_vec_.size() != 1) {
      std::cerr << __FUNCTION__ << "Expression type : SIGN requires exactly 1 Indicator arguments but got "
                << indicator_tokens_vec_.size() << "\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }

  if (!exp_type_.compare("EMA")) {
    if (indicator_tokens_vec_.size() != 1) {
      std::cerr << __FUNCTION__ << " Expression type : EMA requires exactly 1 Indicator arguments but got "
                << indicator_tokens_vec_.size() << "\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }
  if (!exp_type_.compare("Trend")) {
    if (indicator_tokens_vec_.size() != 1) {
      std::cerr << __FUNCTION__ << " Expression type : Trend requires exactly 1 Indicator arguments but got "
                << indicator_tokens_vec_.size() << "\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }
  if (!exp_type_.compare("STDEV")) {
    if (indicator_tokens_vec_.size() != 1) {
      std::cerr << __FUNCTION__ << " Expression type : Trend requires exactly 1 Indicator arguments but got "
                << indicator_tokens_vec_.size() << "\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
    if (params_array_.size() != 1) {
      std::cerr << __FUNCTION__ << " Expression type : STDEV requires PARAMS with exactly 1 argument \n ";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }

  if (!exp_type_.compare("SPLIT_REGIME")) {
    if (indicator_tokens_vec_.size() != 1 || params_array_.size() <= 0) {
      std::cerr << __FUNCTION__ << " Expression type: StdevRegime requires 1 indicator argument and atleast 1 param\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }
  if (!exp_type_.compare("CUTOFF_HIGH")) {
    if (indicator_tokens_vec_.size() != 1 || params_array_.size() <= 0) {
      std::cerr << __FUNCTION__ << " Expression type: CutoffHigh requires 1 indicator argument and atleast 1 param\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }
  if (!exp_type_.compare("CUTOFF_LOW")) {
    if (indicator_tokens_vec_.size() != 1 || params_array_.size() <= 0) {
      std::cerr << __FUNCTION__ << " Expression type: CutoffLow requires 1 indicator argument and atleast 1 param\n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }
  if (!exp_type_.compare("NORMALIZEDBOUND")) {
    if (indicator_tokens_vec_.size() != 2 || params_array_.size() < 2) {
      std::cerr << __FUNCTION__ << " Expression type: NormalizedBound requires 2 indicator arguments and 2 params but got "
    		  << indicator_tokens_vec_.size() << " indicators and " << params_array_.size() << " params \n";
      ExitVerbose(kExitErrorCodeGeneral);
    }
  }
  if (!exp_type_.compare("NORMALIZEDSTDEVBOUND")) {
      if (indicator_tokens_vec_.size() != 1 || params_array_.size() < 3 ) {
        std::cerr << __FUNCTION__ << " Expression type: NormalizedBound requires 1 indicator arguments and 3 params but got and second param to be min and third param to be max"
      		  << indicator_tokens_vec_.size() << " indicators and " << params_array_.size() << " params \n";
        ExitVerbose(kExitErrorCodeGeneral);
      }else if (params_array_[1] > params_array_[2]){
    	  std::cerr << __FUNCTION__ << " Expression type: NormalizedBound requires second param to be min and third param to be max"
    			  << " but got second param greater than third param";
    	          ExitVerbose(kExitErrorCodeGeneral);
      }
    }
}




void ExpressionIndicator::InitializeValues(unsigned int _num_indicators_) {
  is_ready_vec_.resize(_num_indicators_, false);

  prev_value_vec_.resize(_num_indicators_, 0.0);

  hist_stdev_ = 0.0;
  if (!exp_type_.compare("STDEV_CUT")) {
    std::vector<const char*> tokens_ = indicator_tokens_vec_[1];  // Assuming second indicator is SlowStdev
    std::string t_indicator_ = tokens_[2];
    if (!t_indicator_.compare("SlowStdevCalculator")) {
      if (tokens_.size() >= 5) {
        std::string indep_shorcode_ = tokens_[3];
        hist_stdev_ = SampleDataUtil::GetAvgForPeriod(indep_shorcode_, watch_.YYYYMMDD(), NUM_DAYS_HISTORY_STDEV,
                                                      trading_start_mfm_, trading_end_mfm_, "STDEV");
      }
    }
  }

  if (!exp_type_.compare("STDEV_CUT_NEW")) {
    std::vector<const char*> tokens_ = indicator_tokens_vec_[1];  // Assuming second indicator is SlowStdev
    std::string t_indicator_ = tokens_[2];
    if (!t_indicator_.compare("SlowStdevCalculator")) {
      if (tokens_.size() >= 5) {
        std::string indep_shorcode_ = tokens_[3];
        hist_stdev_ = SampleDataUtil::GetAvgForPeriod(indep_shorcode_, watch_.YYYYMMDD(), NUM_DAYS_HISTORY_STDEV,
                                                      trading_start_mfm_, trading_end_mfm_, "STDEV");
        // Sample Data is 300 seconds stdev
        double online_duration = atof(tokens_[4]);
        hist_stdev_ *= sqrt(online_duration / 300.0);
      }
    }
  }

  if (!exp_type_.compare("STDEV_CUT_COMPLEMENT")) {
    std::vector<const char*> tokens_ = indicator_tokens_vec_[1];  // Assuming second indicator is SlowStdev
    std::string t_indicator_ = tokens_[2];
    if (!t_indicator_.compare("SlowStdevCalculator")) {
      if (tokens_.size() >= 5) {
        std::string indep_shorcode_ = tokens_[3];
        hist_stdev_ = SampleDataUtil::GetAvgForPeriod(indep_shorcode_, watch_.YYYYMMDD(), NUM_DAYS_HISTORY_STDEV,
                                                      trading_start_mfm_, trading_end_mfm_, "STDEV");
        // Sample Data is 300 seconds stdev
        double online_duration = atof(tokens_[4]);
        hist_stdev_ *= sqrt(online_duration / 300.0);
      }
    }
  }

  if (!exp_type_.compare("REGIME_CUTOFF")) {
    std::vector<const char*> tokens_ = indicator_tokens_vec_[0];  // First indicator is Regime
    regime_ = atoi(tokens_[1]);         // Regime to be cutoff is given as weight of the indicator
    indicator_tokens_vec_[0][1] = "1";  // changing the weight value to be one
  }

  if (!exp_type_.compare("SPLIT_REGIME")) {
    initial_regime_ = atoi(indicator_tokens_vec_[0][1]);

    // making sure that initial regime is an accepted value
    // else initializing with 1 as initial regime
    if ( initial_regime_ < 1 || initial_regime_ > params_array_.size() ) {
      initial_regime_ = 1;
    }

    indicator_tokens_vec_[0][1] = "1";
    if (params_array_.size() > 1) {
      std::sort(params_array_.begin(), params_array_.end());
    }

    DBGLOG_TIME_CLASS_FUNC << " Params for SPLIT_REGIME : ";
    for (unsigned i = 0; i < params_array_.size(); i++) {
      dbglogger_ << " " << params_array_[i];
    }
    dbglogger_ << DBGLOG_ENDL_FLUSH;

    indicator_value_ = initial_regime_;
  }
  alpha_composite_ = 0.5;
}

void ExpressionIndicator::OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value) {
  if (!is_ready_) {
    is_ready_vec_[indicator_index] = true;
    is_ready_ = AreAllReady();
  } else {
    // Call the aggregator function
    // Could be Sum , Mult, TrendFilter etc.
    (this->*(HFSAT::ExpressionIndicator::aggregate_function))(indicator_index, new_value);

    // Update prev values and notify
    prev_value_vec_[indicator_index] = new_value;
    NotifyIndicatorListeners(indicator_value_);
  }
}

bool ExpressionIndicator::AreAllReady() {
  for (auto is_ready : is_ready_vec_) {
    if (!is_ready) {
      return false;
    }
  }
  return true;
}

void ExpressionIndicator::set_basepx_pxtype(SecurityMarketView& _dep_market_view_, PriceType_t _basepx_pxtype_) {
  for (auto i = 0u; i < indicator_vec_.size(); i++) {
    indicator_vec_[i]->set_basepx_pxtype(_dep_market_view_, _basepx_pxtype_);
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////

// To add any new expression, just add the "Token Name"  in InitFunctionPointers() funcrion below
// and Add the implementation of the corresponding Function below

/////------------ Initialize Expression Functions Here -----------------------------------------------

void ExpressionIndicator::InitFunctionPointers() {
  exp_to_func_map["SUM"] = &HFSAT::ExpressionIndicator::ExpSum;
  exp_to_func_map["MULT"] = &HFSAT::ExpressionIndicator::ExpMult;
  exp_to_func_map["DIV"] = &HFSAT::ExpressionIndicator::ExpDiv;
  exp_to_func_map["TREND_FILTER"] = &HFSAT::ExpressionIndicator::TrendFilter;
  exp_to_func_map["CONVEX"] = &HFSAT::ExpressionIndicator::Convex;
  exp_to_func_map["MULTDELTA"] = &HFSAT::ExpressionIndicator::MultDelta;
  exp_to_func_map["MAXBYMIN"] = &HFSAT::ExpressionIndicator::MaxByMin;
  exp_to_func_map["ONLINE_COMPUTED"] = &HFSAT::ExpressionIndicator::OnlineComputed;
  exp_to_func_map["ONLINE_COMPUTED_CUTOFF"] = &HFSAT::ExpressionIndicator::OnlineComputedCutOff;
  exp_to_func_map["STDEV_CUT"] = &HFSAT::ExpressionIndicator::StdevCutoff;
  exp_to_func_map["STDEV_CUT_NEW"] = &HFSAT::ExpressionIndicator::StdevCutoffNew;
  exp_to_func_map["STDEV_CUT_COMPLEMENT"] = &HFSAT::ExpressionIndicator::StdevCutoffComplement;
  exp_to_func_map["TREND_DIFF"] = &HFSAT::ExpressionIndicator::TrendDiff;
  exp_to_func_map["REGIME_CUTOFF"] = &HFSAT::ExpressionIndicator::RegimeCutOff;
  exp_to_func_map["COMPOSITE"] = &HFSAT::ExpressionIndicator::Composite;
  exp_to_func_map["MOD"] = &HFSAT::ExpressionIndicator::Mod;
  exp_to_func_map["SIGN"] = &HFSAT::ExpressionIndicator::Sign;
  exp_to_func_map["EMA"] = &HFSAT::ExpressionIndicator::EMA;
  exp_to_func_map["Trend"] = &HFSAT::ExpressionIndicator::Trend;
  exp_to_func_map["STDEV"] = &HFSAT::ExpressionIndicator::Stdev;
  exp_to_func_map["SPLIT_REGIME"] = &HFSAT::ExpressionIndicator::SplitRegime;
  exp_to_func_map["CUTOFF_HIGH"] = &HFSAT::ExpressionIndicator::CutoffHigh;
  exp_to_func_map["CUTOFF_LOW"] = &HFSAT::ExpressionIndicator::CutoffLow;
  exp_to_func_map["WEIGHTED_INDICATOR"] = &HFSAT::ExpressionIndicator::WeightedIndicator;
  exp_to_func_map["SIGMOID"] = &HFSAT::ExpressionIndicator::ExpSig;
  exp_to_func_map["NORMALIZEDBOUND"] = &HFSAT::ExpressionIndicator::NormalizedBound;
  exp_to_func_map["NORMALIZEDSTDEVBOUND"] = &HFSAT::ExpressionIndicator::NormalizedStdevBound;
}

/////------------ Expression Support function ---------------------------------------------------------

double sigmoid(double a, double b, double x) { return 1 / (1 + exp(-1 * (a * x + b))); }

/////------------ Expression Function Definition ------------------------------------------------------

void ExpressionIndicator::ExpSum(const unsigned int& indicator_index, const double& new_value) {
  indicator_value_ += (new_value - prev_value_vec_[indicator_index]);
}

// Simply the modulus value of the indicator provided
void ExpressionIndicator::Mod(const unsigned int& indicator_index, const double& new_value) {
  indicator_value_ = fabs(new_value);
}

// Simply outputs the sign of the indicator provided
void ExpressionIndicator::Sign(const unsigned int& indicator_index, const double& new_value) {
  indicator_value_ = new_value > 0 ? 1 : -1;
}
//Simply outputs the sigmoid of the indicator provided using the parameters
//E.g. Expression SIGMOID 3 1.0 StableScaledTrend VX_0 300 MktSizeWPrice PARAMS 2 1 0 
// Expression SIGMOID <Number of parameters for indicator> <weight> <indicator> <shortcode> <trend> <duration> <price> PARAMS 2 <a> <b>. This is  for the sigmoid function 1/(1+exp(-a*(Indicator) + b))
void ExpressionIndicator::ExpSig(const unsigned int& indicator_index, const double& new_value) {
  if(indicator_index ==0){
     indicator_value_ = sigmoid(composite_sig_a_,composite_sig_b_,new_value);
  }
}
// Eg :Expression COMPOSITE 2 1.0 RecentVolumeMeasure FGBM_0 300 3 1.0 SimpleTrend FGBL_0 0.1 MktSizeWPrice 2 1.0
// TRSumTDiffNSize FGBM_0 30 PARAMS 3.0 -0.006 5.490 0.754
// Expression COMPOSITE <Conditional Variable> <Indicator 1> <Indicator 2> PARAMS 3.0 a b w
void ExpressionIndicator::Composite(const unsigned int& indicator_index, const double& new_value) {
  prev_value_vec_[indicator_index] = new_value;

  if (indicator_index == 0) {
    alpha_composite_ = sigmoid(composite_sig_a_, composite_sig_b_, prev_value_vec_[0]);
  } else {
    indicator_value_ =
        alpha_composite_ * composite_beta_ * prev_value_vec_[1] + (1 - alpha_composite_) * prev_value_vec_[2];
  }
}
void ExpressionIndicator::ExpMult(const unsigned int& indicator_index, const double& new_value) {
  // I don't have good a feeling about multiplying two indicators to get a value

  indicator_value_ = 1.0;
  prev_value_vec_[indicator_index] = new_value;

  for (unsigned int index = 0; index < prev_value_vec_.size(); index++) {
    indicator_value_ *= prev_value_vec_[index];
  }
}
/**
 * indicator value = indicator 2 / indicator 1
 * @param indicator_index
 * @param new_value
 */
void ExpressionIndicator::ExpDiv(const unsigned int& indicator_index, const double& new_value) {
  prev_value_vec_[indicator_index] = new_value;
  if (fabs(prev_value_vec_[0]) > 0.000001) {
    indicator_value_ = prev_value_vec_[1] / prev_value_vec_[0];
  }
}

void ExpressionIndicator::TrendFilter(const unsigned int& indicator_index, const double& new_value) {
  // Assumption: only two indicators : I_1 args_1 I_2 args_2 and we are applying TrendFilter on I_1
  // Usually I_1 will be a relative indicator and I_2 will be dep_trend used to filter I_1

  prev_value_vec_[indicator_index] = new_value;
  if (prev_value_vec_[0] * prev_value_vec_[1] >= 0) {
    indicator_value_ = prev_value_vec_[0];
  } else {
    indicator_value_ = 0;  // Filter case
  }
  if (data_interrupted_) {
    indicator_value_ = 0;
  }
}

void ExpressionIndicator::Convex(const unsigned int& indicator_index, const double& new_value) {
  prev_value_vec_[indicator_index] = new_value;
  if (prev_value_vec_[0] * prev_value_vec_[1] < 0) {
    indicator_value_ = prev_value_vec_[0] + prev_value_vec_[1];  // trends in opposite directions
  } else {
    indicator_value_ = prev_value_vec_[1];  // in same direction
  }
}

void ExpressionIndicator::MultDelta(const unsigned int& indicator_index, const double& new_value) {
  // 4 indicators required; 1st would be price and the rest three would be SimpleReturns of the 3 securities
  // eg: MULTDELTA 2 1.0 SimplePriceType Si_0 OfflineMixMMS 3 1.0 SimpleReturns EUR_RUB__TOM 100.0 OfflineMixMMS 3 -1.0
  // SimpleReturns 6E_0 100.0 OfflineMixMMS 3 -1.0 SimpleReturns Si_0 100.0 OfflineMixMMS
  prev_value_vec_[indicator_index] = new_value;
  indicator_value_ = prev_value_vec_[0] * (prev_value_vec_[1] + prev_value_vec_[2] +
                                           prev_value_vec_[3]);  // pricetype * ( ret1 + ret2 + ret3 ) ; sign already
                                                                 // taken into consideration in the expression
}

void ExpressionIndicator::MaxByMin(const unsigned int& indicator_index, const double& new_value) {
  prev_value_vec_[indicator_index] = new_value;
  if (prev_value_vec_[0] >= prev_value_vec_[1])
    indicator_value_ = prev_value_vec_[0] / prev_value_vec_[1];
  else
    indicator_value_ = prev_value_vec_[1] / prev_value_vec_[0];
}

void ExpressionIndicator::OnlineComputed(const unsigned int& indicator_index, const double& new_value) {
  if (prev_value_vec_[0] == 0) {
    indicator_value_ = 0;
  } else {
    prev_value_vec_[indicator_index] = new_value;
    indicator_value_ = prev_value_vec_[0] * prev_value_vec_[1] - prev_value_vec_[2];
    if (data_interrupted_) {
      indicator_value_ = 0;
    }
  }
}

void ExpressionIndicator::OnlineComputedCutOff(const unsigned int& indicator_index, const double& new_value) {
  prev_value_vec_[indicator_index] = new_value;
  if (fabs(prev_value_vec_[0] * prev_value_vec_[1]) <= fabs(prev_value_vec_[2])) {
    indicator_value_ = 0;
  } else if (data_interrupted_) {
    indicator_value_ = 0;
  } else {
    indicator_value_ = prev_value_vec_[0] * prev_value_vec_[1] - prev_value_vec_[2];
  }
}

void ExpressionIndicator::StdevCutoff(const unsigned int& indicator_index, const double& new_value) {
  prev_value_vec_[indicator_index] = new_value;
  if (data_interrupted_) {
    indicator_value_ = 0;
  } else {
    if (prev_value_vec_[1] > hist_stdev_) {
      indicator_value_ = prev_value_vec_[0];
    } else {
      indicator_value_ = 0;
    }
  }
}

void ExpressionIndicator::StdevCutoffNew(const unsigned int& indicator_index, const double& new_value) {
  prev_value_vec_[indicator_index] = new_value;
  if (data_interrupted_) {
    indicator_value_ = 0;
  } else {
    if (prev_value_vec_[1] > hist_stdev_) {
      indicator_value_ = prev_value_vec_[0];
    } else {
      indicator_value_ = 0;
    }
  }
}

void ExpressionIndicator::StdevCutoffComplement(const unsigned int& indicator_index, const double& new_value) {
  prev_value_vec_[indicator_index] = new_value;
  if (data_interrupted_) {
    indicator_value_ = 0;
  } else {
    if (prev_value_vec_[1] <= hist_stdev_) {
      indicator_value_ = prev_value_vec_[0];
    } else {
      indicator_value_ = 0;
    }
  }
}

void ExpressionIndicator::TrendDiff(const unsigned int& indicator_index, const double& new_value) {
  // SimpleTrend(X) - SimpleTrend(Y)
  prev_value_vec_[indicator_index] = new_value;
  if (data_interrupted_) {
    indicator_value_ = 0;
  } else {
    indicator_value_ = prev_value_vec_[0] - prev_value_vec_[1];
  }
}

void ExpressionIndicator::RegimeCutOff(const unsigned int& indicator_index, const double& new_value) {
  // Assumption: only two indicators : I_1 args_1 I_2 args_2
  // We are making Indicator 2 zero valued only in case regime of Indicator 1 is equal to its weight
  prev_value_vec_[indicator_index] = new_value;
  if (data_interrupted_) {
    indicator_value_ = 0;
  } else {
    if (prev_value_vec_[0] != regime_)
      indicator_value_ = prev_value_vec_[1];
    else
      indicator_value_ = 0;
  }
}

void ExpressionIndicator::NormalizedBound(const unsigned int& indicator_index, const double& new_value) {
  prev_value_vec_[indicator_index] = new_value;
  if (data_interrupted_) {
    indicator_value_ = 0;
  } else {
	     indicator_value_ = prev_value_vec_[0] / std::min((std::max(prev_value_vec_[1],params_array_[0])), params_array_[1]);
	   }
}

void ExpressionIndicator::NormalizedStdevBound(const unsigned int& indicator_index, const double& new_value) {

  prev_value_vec_[indicator_index] = new_value;
  if (data_interrupted_) {
    indicator_value_ = 0;
  } else {


  if (!is_initialized_) {
      is_initialized_ = true;
      current_val_ = new_value;
      SetTimeDecayWeights_EMA((double)params_array_[0]);
      InitializeValues();
    }
    current_val_ = new_value;

    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_ += inv_decay_sum_ * (current_val_ - last_val_);
      moving_square_avg_ += inv_decay_sum_ * (current_val_ - last_val_) * (current_val_ - last_val_);
    } else {
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_ = (current_val_ * inv_decay_sum_) + (moving_avg_ * decay_vector_[1]);
          moving_square_avg_ = (current_val_ * current_val_ * inv_decay_sum_) + (moving_square_avg_ * decay_vector_[1]);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          moving_avg_ = (current_val_ * inv_decay_sum_) +
                        (last_val_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                        (moving_avg_ * decay_vector_[num_pages_to_add_]);
          moving_square_avg_ = (current_val_ * current_val_ * inv_decay_sum_) +
                               (last_val_ * last_val_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                               (moving_square_avg_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }

      last_val_ = current_val_;
      if (moving_square_avg_ > moving_avg_ * moving_avg_) {
        indicator_value_ =  prev_value_vec_[0] / (std::min(std::max(sqrt(moving_square_avg_ - moving_avg_ * moving_avg_),params_array_[1]), params_array_[2]));
      }
    }
  }
}

void ExpressionIndicator::SetTimeDecayWeights_EMA(double _fractional_seconds_) {
  trend_history_msecs_ = (std::max(20, (int)round(1000 * _fractional_seconds_)));

  ///< here number of samples are not required to be very high and hence the decaylength target is just 20
  const unsigned int kDecayLength = 20;
  const unsigned int kMinPageWidth = 10;
  ///< keeping kMaxPageWidth low makes the number_fadeoffs_ pretty high and
  const unsigned int kMaxPageWidth = 200;
  /// hence keeps lots of sample points
  page_width_msecs_ = std::min(kMaxPageWidth, std::max(kMinPageWidth, (trend_history_msecs_ / kDecayLength)));

  int number_fadeoffs_ = std::max(1, (int)ceil(trend_history_msecs_ / page_width_msecs_));

  decay_page_factor_ = MathUtils::CalcDecayFactor(number_fadeoffs_);

  decay_vector_.resize(2 * number_fadeoffs_);
  decay_vector_sums_.resize(2 * number_fadeoffs_);

  for (auto i = 0u; i < decay_vector_.size(); i++) {
    decay_vector_[i] = pow(decay_page_factor_, (int)i);
  }

  decay_vector_sums_[0] = 0;
  for (unsigned int i = 1; i < decay_vector_sums_.size(); i++) {
    decay_vector_sums_[i] = decay_vector_sums_[i - 1] + decay_vector_[i];
  }

  inv_decay_sum_ = (1 - decay_page_factor_);
}

void ExpressionIndicator::InitializeValues() {
  moving_avg_ = current_val_;
  moving_square_avg_ = current_val_ * current_val_;
  last_val_ = current_val_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
}

void ExpressionIndicator::EMA(const unsigned int& indicator_index, const double& new_value) {
  if (!is_initialized_) {
    is_initialized_ = true;
    current_val_ = new_value;
    SetTimeDecayWeights_EMA((double)params_array_[0]);
    InitializeValues();
  }
  current_val_ = new_value;

  if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
    moving_avg_ += inv_decay_sum_ * (current_val_ - last_val_);
  } else {
    int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
    if (num_pages_to_add_ >= (int)decay_vector_.size()) {
      InitializeValues();
    } else {
      if (num_pages_to_add_ == 1) {
        moving_avg_ = (current_val_ * inv_decay_sum_) + (moving_avg_ * decay_vector_[1]);
      } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
        moving_avg_ = (current_val_ * inv_decay_sum_) +
                      (last_val_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                      (moving_avg_ * decay_vector_[num_pages_to_add_]);
      }
      last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
    }
  }
  last_val_ = current_val_;
  indicator_value_ = moving_avg_;
}

void ExpressionIndicator::Trend(const unsigned int& indicator_index, const double& new_value) {
  if (!is_initialized_) {
    is_initialized_ = true;
    current_val_ = new_value;
    SetTimeDecayWeights_EMA((double)params_array_[0]);
    InitializeValues();
  }
  current_val_ = new_value;

  if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
    moving_avg_ += inv_decay_sum_ * (current_val_ - last_val_);
  } else {
    int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
    if (num_pages_to_add_ >= (int)decay_vector_.size()) {
      InitializeValues();
    } else {
      if (num_pages_to_add_ == 1) {
        moving_avg_ = (current_val_ * inv_decay_sum_) + (moving_avg_ * decay_vector_[1]);
      } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
        moving_avg_ = (current_val_ * inv_decay_sum_) +
                      (last_val_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                      (moving_avg_ * decay_vector_[num_pages_to_add_]);
      }
      last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
    }

    last_val_ = current_val_;
    indicator_value_ = current_val_ - moving_avg_;
  }
}

void ExpressionIndicator::Stdev(const unsigned int& indicator_index, const double& new_value) {
  if (!is_initialized_) {
    is_initialized_ = true;
    current_val_ = new_value;
    SetTimeDecayWeights_EMA((double)params_array_[0]);
    InitializeValues();
  }
  current_val_ = new_value;

  if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
    moving_avg_ += inv_decay_sum_ * (current_val_ - last_val_);
    moving_square_avg_ += inv_decay_sum_ * (current_val_ - last_val_) * (current_val_ - last_val_);
  } else {
    int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
    if (num_pages_to_add_ >= (int)decay_vector_.size()) {
      InitializeValues();
    } else {
      if (num_pages_to_add_ == 1) {
        moving_avg_ = (current_val_ * inv_decay_sum_) + (moving_avg_ * decay_vector_[1]);
        moving_square_avg_ = (current_val_ * current_val_ * inv_decay_sum_) + (moving_square_avg_ * decay_vector_[1]);
      } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
        moving_avg_ = (current_val_ * inv_decay_sum_) +
                      (last_val_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                      (moving_avg_ * decay_vector_[num_pages_to_add_]);
        moving_square_avg_ = (current_val_ * current_val_ * inv_decay_sum_) +
                             (last_val_ * last_val_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                             (moving_square_avg_ * decay_vector_[num_pages_to_add_]);
      }
      last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
    }

    last_val_ = current_val_;
    if (moving_square_avg_ > moving_avg_ * moving_avg_) {
      indicator_value_ = sqrt(moving_square_avg_ - moving_avg_ * moving_avg_);
    }
  }
}

void ExpressionIndicator::SplitRegime(const unsigned int& indicator_index, const double& new_value) {
  // Given an indicator I, and set of values x1,x2,x3.., get the regimes for all split of indicator values
  // num regimes would be num(x) = params_array_.size()
  // Eg: SPLIT_REGIME 3 1.00 SlowStdevCalculator BR_DOL_0 300 MktSizeWPrice PARAMS 2.0 3.5 6.5
  // initial regime is 1 ( as the weight of the indicator )
  // num regimes is 2 ( we have 2 cutoffs 3.5 and 6.5 )
  // if current regime is 1, then it would switch to regime 2 if the ind val > 6.5 else stay in 1
  // if current regime is 2, then it would switch to regime 1 if the ind val < 3.5 else stay in 2
  // basically the gap 3.5 and 6.5 is kind of tolerance
  //
  // You can extend the same thing for multiple regimes 
  
  prev_value_vec_[indicator_index] = (new_value);

  // indicator_value will always be +ve integer
  if (data_interrupted_) {
    indicator_value_ = initial_regime_;  // default regime
  } else {
    int current_regime_ = indicator_value_;
    // computing the upper and lower indexes to compare 
    // for regime switch based on current regime
    int upper_index = std::min(current_regime_, (int)params_array_.size() - 1);
    int lower_index = std::max(current_regime_ - 2, 0);

    // currently increasing the regime gradually i.e it would take atleast 4 mkt-updates to take from regime 1 to 5
    if (prev_value_vec_[indicator_index] >= params_array_[upper_index]) {
      indicator_value_ = upper_index + 1;
    } else if (prev_value_vec_[indicator_index] <= params_array_[lower_index]) {
      indicator_value_ = lower_index + 1;
    } else {
      indicator_value_ = current_regime_;
    }

    //  DBGLOG_TIME_CLASS_FUNC << "Current regime : " << current_regime_ <<
    //  " lower thresh: " << params_array_[lower_index] << " upper thresh: " << params_array_[upper_index] <<
    //  " current val: " << prev_value_vec_[indicator_index] << " new regime: " << indicator_value_
    //  << DBGLOG_ENDL_FLUSH;
  }
}
void ExpressionIndicator::CutoffHigh(const unsigned int& indicator_index, const double& new_value) {
  if (new_value > params_array_[0])
    indicator_value_ = new_value;
  else
    indicator_value_ = 0;
}

void ExpressionIndicator::CutoffLow(const unsigned int& indicator_index, const double& new_value) {
  if (new_value < params_array_[0])
    indicator_value_ = new_value;
  else
    indicator_value_ = 0;
}

void ExpressionIndicator::WeightedIndicator(const unsigned int& indicator_index, const double& new_value) {
  indicator_value_ = new_value;
}

void ExpressionIndicator::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                  const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  // Calling OnIndicatorUpdate with dummy values so that it may notify in case any diff behavior is expected for the
  // data_interrupt
  OnIndicatorUpdate(0, prev_value_vec_[0]);
}

void ExpressionIndicator::OnMarketDataResumed(const unsigned int _security_id_) {
  data_interrupted_ = false;
  InitializeValues();
}
}
