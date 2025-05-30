/**
   \file IndicatorsCode/mult_mkt_complex_price_combo.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/combo_indicator.hpp"

namespace HFSAT {

void ComboIndicator::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                       std::vector<std::string>& _ors_source_needed_vec_,
                                       const std::vector<const char*>& r_tokens_) {
  if (isRelativeCombo(r_tokens_[2])) {
    // std::cerr << "Relative Combo Indicator: " << r_tokens_[2] << " " << r_tokens_[3] << " " << r_tokens_[4] <<
    // std::endl; -- debug print
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
    IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[4], _shortcodes_affecting_this_indicator_);
  } else {
    IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[3], _shortcodes_affecting_this_indicator_);
  }
}

ComboIndicator* ComboIndicator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  const std::vector<const char*>& r_tokens_,
                                                  PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  for (unsigned int i = 2; i < r_tokens_.size() && r_tokens_[i][0] != '#'; i++) t_temp_oss_ << r_tokens_[i] << ' ';
  t_temp_oss_ << PriceType_t_To_String(_basepx_pxtype_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ComboIndicator*> concise_indicator_description_map_;
  if (isRelativeCombo(r_tokens_[2])) ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new ComboIndicator(t_dbglogger_, r_watch_, concise_indicator_description_, r_tokens_, _basepx_pxtype_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

ComboIndicator::ComboIndicator(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                               const std::string& concise_indicator_description_,
                               const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_short_code_(""),
      lrdb_(nullptr),
      is_relative_(false),
      is_offline_(false),
      is_trend_(false) {
  if (!IndicatorUtil::IsPortfolioShortcode(r_tokens_[3])) {
    lrdb_ = &OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, r_tokens_[3]);
  }
  // extract the underlying indicator's name OnlineComputedCutoffPairCombo -> OnlineComputedCutoffPair
  std::string ind_var_name_(r_tokens_[2]);
  size_t t_index_ = ind_var_name_.npos;
  if ((t_index_ = ind_var_name_.find("Combo")) != ind_var_name_.npos) {
    ind_var_name_.replace(t_index_, 5, "");
  } else {  // Should not reach here at all. definitely ERROR
    std::cerr << __FUNCTION__ << " " << r_tokens_[2] << " does not look like a combo indicator.\n";
    exit(0);
  }
  is_relative_ = isRelativeCombo(ind_var_name_);
  is_offline_ = isOfflineCombo(ind_var_name_);
  is_trend_ = isTrendCombo(ind_var_name_);
  // std::cerr << "So you are looking for "<< ind_var_name_ << " this indicator combo.\n"; //-- debug print

  // get the new Indicator desc vec with modified indicator name
  std::vector<const char*> new_ind_tokens_ = r_tokens_;  // should do deep copy -- check
  new_ind_tokens_[2] = ind_var_name_.c_str();

  // get the portfolio SMVs
  unsigned int pf_index_in_ind_desc_ = (is_relative_ ? 4 : 3);
  if (is_relative_) dep_short_code_ = new_ind_tokens_[pf_index_in_ind_desc_ - 1];
  std::string _portfolio_descriptor_shortcode_ = r_tokens_[pf_index_in_ind_desc_];  /// TO CHECK is it correct for all
  std::vector<SecurityMarketView*> indep_market_view_vec_;
  IndicatorUtil::GetPortfolioSMVVec(_portfolio_descriptor_shortcode_, indep_market_view_vec_);

  CommonIndicatorUniqueInstancePtr indicator_uniq_instance_ptr_ = GetUniqueInstanceFunc(ind_var_name_);
  std::vector<double> this_indicator_weight_vec_(indep_market_view_vec_.size(), 1.00);  // default weight is 1.00
  portfolio_constituents_.resize(indep_market_view_vec_.size(), "");

  // Special handling for Trend indicators
  if (is_trend_) GetIndicatorWeight(_portfolio_descriptor_shortcode_, this_indicator_weight_vec_);

  // initialize.
  InitializeValues(indep_market_view_vec_.size());
  indicator_vec_.resize(indep_market_view_vec_.size(), NULL);

  for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
    portfolio_constituents_[i] = (indep_market_view_vec_[i]->shortcode()).c_str();
    new_ind_tokens_[pf_index_in_ind_desc_] = (indep_market_view_vec_[i]->shortcode()).c_str();

    // FIXING_BASEPX_BUG
    // original context
    // Rahul : In every combo indicator we set the basepx_type_ for only indicators with dep_market_view_.
    // So all other indicators of this combo has default basepx_type_ ( kMidPrice ).
    //
    // Now, In the new scenario(single combo indicator) I unknowingly fixed this bug and the indicators that uses
    // basepx_type_ was giving different result. So to be consistent with the current master(or devtrade/devsim) version
    // I forcibly introduced this bug and tried to check the sim_result with devtrade in basepx_type_devtrade branch.
    //
    // There is no easy way to fix this bug. as if we change the indicator, all our current models will get invalidated.
    // Or, to be backward compatible we have to double the number of mult_combo indicators ( these use basepx_type ).
    // On the other branch, basepx_type_bugeffect, I tried to build some models and test which indicator ( with or
    // without the bug ) is more powerful.
    // What I found that it is completely random, I mean, fixing the bug will just create
    // a new set of indicators, thats all. Neither in correlation space nor in pnl space this bug has any perceivable
    // bias.
    // I could not find any elegant way to make the system backward compatible after fixing this bug,

    // PriceType_t px_type_ = kPriceTypeMidprice ;
    // if ( dep_short_code_ == portfolio_constituents_[i] ) px_type_ = _basepx_pxtype_ ; // redundant check.. pseudo
    // bug,..:P
    // CommonIndicator * _this_indicator_ = (indicator_uniq_instance_ptr_) ( t_dbglogger_, r_watch_, new_ind_tokens_,
    // px_type_ ) ;
    CommonIndicator* _this_indicator_ =
        (indicator_uniq_instance_ptr_)(t_dbglogger_, r_watch_, new_ind_tokens_, _basepx_pxtype_);
    if (!_this_indicator_) {
      std::cerr << " Could not generate indicator: " << new_ind_tokens_[2] << " " << new_ind_tokens_[3] << " "
                << new_ind_tokens_[4] << std::endl;
      exit(0);
    }
    indicator_vec_[i] = _this_indicator_;
    is_ready_vec_[i] = indicator_vec_[i]->IsIndicatorReady();
    _this_indicator_->add_indicator_listener(i, this, this_indicator_weight_vec_[i]);
  }
  is_ready_ = IsAnyoneReady();
  if (is_ready_ && is_offline_) InitializeWeights();
  // InitializeValues ( indep_market_view_vec_.size ( ) ) ;
}

void ComboIndicator::GetIndicatorWeight(std::string _portfolio_descriptor_shortcode_,
                                        std::vector<double>& this_indicator_weight_vec_) {
  if ((concise_indicator_description_.find("ScaledTrend") != std::string::npos) &&
      (concise_indicator_description_.find("Stable") == std::string::npos)
      // Now one more for rule for StableScaledTrend
      )
    return;  // ScaledTrendCombo - no need to modify weights

  const EigenConstituentsVec& eigen_constituent_vec =
      (PcaWeightsManager::GetUniqueInstance()).GetEigenConstituentVec(_portfolio_descriptor_shortcode_);
  /// <stdevs>
  const std::vector<double>& stdev_constituent_vec =
      (PcaWeightsManager::GetUniqueInstance()).GetPortfolioStdevs(_portfolio_descriptor_shortcode_);

  /// Safety checks
  if (eigen_constituent_vec.empty()) {
    std::cerr << "::EigenConstituentsVec " << _portfolio_descriptor_shortcode_
              << "does not have even single eigenvectors/values computed " << std::endl;
    ExitVerbose(kPCAWeightManagerMissingPortFromMap, _portfolio_descriptor_shortcode_.c_str());
  }
  if (stdev_constituent_vec.size() != this_indicator_weight_vec_.size()) {
    std::cerr << "SimpleTrendMktEventsCombo::Stdevs computed for " << stdev_constituent_vec.size()
              << " but portfolio size is " << this_indicator_weight_vec_.size() << std::endl;
    ExitVerbose(kPCAWeightManagerMissingPortFromMap, _portfolio_descriptor_shortcode_.c_str());
  }
  /// Using the first PC, ignoring others for now
  if (!eigen_constituent_vec.empty()) {
    if (eigen_constituent_vec[0].eigenvec_components_.size() != this_indicator_weight_vec_.size()) {
      std::cerr << "SimpleTrendMktEventsCombo:: PCA EIgenvector components size"
                << eigen_constituent_vec[0].eigenvec_components_.size() << " but portfolio size is "
                << this_indicator_weight_vec_.size() << std::endl;
      ExitVerbose(kPCAWeightManagerMissingPortFromMap, _portfolio_descriptor_shortcode_.c_str());
    }
  }
  double sum_of_eigen_compo_by_stdev_ = 0;
  /// After all checks, data if available must be correct
  /// Sum_(e_i/sigma_i) should be the normalizing factor
  for (unsigned int ii = 0; ii < this_indicator_weight_vec_.size(); ii++) {
    if (stdev_constituent_vec[ii] > 0) {
      sum_of_eigen_compo_by_stdev_ += (eigen_constituent_vec[0].eigenvec_components_[ii] / stdev_constituent_vec[ii]);
    } else {
      ExitVerbose(kExitErrorCodeGeneral, "stdev <= 0 in PCAPortPrice");
    }
  }

  sum_of_eigen_compo_by_stdev_ = fabs(sum_of_eigen_compo_by_stdev_);

  for (auto i = 0u; i < this_indicator_weight_vec_.size(); i++) {
    this_indicator_weight_vec_[i] =
        (eigen_constituent_vec[0].eigenvec_components_[i] / stdev_constituent_vec[i]) / sum_of_eigen_compo_by_stdev_;
  }
}

void ComboIndicator::InitializeValues(unsigned int _num_indicators_) {
  is_ready_vec_.resize(_num_indicators_, false);
  prev_value_vec_.resize(_num_indicators_, 0.0);
}

void ComboIndicator::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    is_ready_vec_[_indicator_index_] = true;
    is_ready_ = IsAnyoneReady();
    if (is_ready_ & is_offline_) InitializeWeights();
  } else {
    indicator_value_ += (_new_value_ - prev_value_vec_[_indicator_index_]);
    prev_value_vec_[_indicator_index_] = _new_value_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void ComboIndicator::InitializeWeights() {
  std::vector<double> t_temp_weights_(indicator_vec_.size());  ///< store of the weights based on correlation
  double sum_weights_ = 0.0;

  for (auto i = 0u; i < indicator_vec_.size(); i++) {
    if (lrdb_) {
      t_temp_weights_[i] =
          std::max(0.05, fabs((lrdb_->GetLRCoeff(dep_short_code_, portfolio_constituents_[i])).lr_correlation_));
    } else {
      t_temp_weights_[i] = 0;
    }

    sum_weights_ += t_temp_weights_[i];
  }

  for (auto i = 0u; i < indicator_vec_.size(); i++) {
    indicator_vec_[i]->UpdateIndicatorListenerWeight(this, (t_temp_weights_[i] / sum_weights_));
  }
}

bool ComboIndicator::IsAnyoneReady() { return VectorUtils::LinearSearchValue(is_ready_vec_, true); }

void ComboIndicator::set_basepx_pxtype(SecurityMarketView& _dep_market_view_, PriceType_t _basepx_pxtype_) {
  for (auto i = 0u; i < indicator_vec_.size(); i++) {
    indicator_vec_[i]->set_basepx_pxtype(_dep_market_view_, _basepx_pxtype_);
  }
}
}
