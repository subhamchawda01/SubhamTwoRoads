/**
    \file ModelMathCode/base_multiple_model_math.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/ModelMath/base_multiple_model_math.hpp"

namespace HFSAT {

void BaseMultipleModelMath::set_start_end_mfm() {
  for (auto i = 0u; i < num_shortcodes_; i++) {
    for (unsigned int j = 0; j < individual_indicator_vec_[i].size(); j++) {
      individual_indicator_vec_[i][j]->set_start_mfm(t_trading_start_utc_mfm_);
      individual_indicator_vec_[i][j]->set_end_mfm(t_trading_end_utc_mfm_);
    }
  }
  for (auto i = 0u; i < global_indicator_vec_.size(); i++) {
    global_indicator_vec_[i]->set_start_mfm(t_trading_start_utc_mfm_);
    global_indicator_vec_[i]->set_end_mfm(t_trading_end_utc_mfm_);
  }
}

void BaseMultipleModelMath::AddShortCode(SecurityMarketView* _smv_) {
  shortcodes_list_.push_back(_smv_->shortcode());
  const_smv_list_.push_back(_smv_);
  last_individual_indicators_debug_print_.push_back(0.0);
  individual_indicator_vec_.push_back(std::vector<CommonIndicator*>());
  prev_individual_indicator_value_vec_.push_back(std::vector<double>());
  is_ready_product_.push_back(false);
  last_is_ready_product_.push_back(false);
  value_vec_.push_back(0);
  last_propagated_value_vec_.push_back(0);
  is_ready_vec_product_.push_back(std::vector<bool>());
  num_shortcodes_++;
}

void BaseMultipleModelMath::AddIndividualIndicator(CommonIndicator* _this_indicator_, const double& _this_weight_,
                                                   bool _readiness_required_, int _product_index_) {
  if (_this_indicator_ != NULL) {
    _this_indicator_->add_indicator_listener(current_indicator_index_, this, _this_weight_);
    individual_indicator_vec_[_product_index_].push_back(_this_indicator_);

    const bool t_is_this_indicator_ready_ = _this_indicator_->IsIndicatorReady();
    if (t_is_this_indicator_ready_ == false) {
      is_ready_vec_product_[_product_index_].push_back(false);
      is_ready_product_[_product_index_] = false;  // since is_ready_vec_ is now sure not to be false;
    } else {
      is_ready_vec_product_[_product_index_].push_back(true);
    }
    prev_individual_indicator_value_vec_[_product_index_].push_back(0.0);
  } else {
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "NULL t_indicator_" << DBGLOG_ENDL_FLUSH;
    }
    ExitVerbose(kExitErrorCodeGeneral, dbglogger_);
  }
  current_indicator_index_++;
}

// Global Indicators can only be added after all individual indicators are added
  void BaseMultipleModelMath::AddGlobalIndicator(CommonIndicator* _this_indicator_, const std::vector<double>& this_wts_vec_,
						 bool _readiness_required_) {
  if (_this_indicator_ != NULL) {
    _this_indicator_->add_unweighted_indicator_listener(current_indicator_index_, this);
    global_indicator_vec_.push_back(_this_indicator_);
    global_indicators_weights_[current_indicator_index_] = this_wts_vec_;

    const bool t_is_this_indicator_ready_ = _this_indicator_->IsIndicatorReady();
    if (t_is_this_indicator_ready_ == false) {
      is_ready_vec_.push_back(false);
      is_ready_ = false;  // since is_ready_vec_ is now sure not to be false;
    } else {
      is_ready_vec_.push_back(true);
    }
    prev_global_indicator_value_vec_.push_back(0.0);
  } else {
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "NULL t_indicator_" << DBGLOG_ENDL_FLUSH;
    }
    ExitVerbose(kExitErrorCodeGeneral, dbglogger_);
  }
  current_indicator_index_++;
}

// Global Indicators can only be added after all individual indicators are added, this one takes weights vector ( one for each option for that underlying )
void BaseMultipleModelMath::AddGlobalIndicator(CommonIndicator* _this_indicator_, const double& _this_weight_,
                                               bool _readiness_required_) {
  if (_this_indicator_ != NULL) {
    _this_indicator_->add_indicator_listener(current_indicator_index_, this, _this_weight_);
    global_indicator_vec_.push_back(_this_indicator_);

    const bool t_is_this_indicator_ready_ = _this_indicator_->IsIndicatorReady();
    if (t_is_this_indicator_ready_ == false) {
      is_ready_vec_.push_back(false);
      is_ready_ = false;  // since is_ready_vec_ is now sure not to be false;
    } else {
      is_ready_vec_.push_back(true);
    }
    prev_global_indicator_value_vec_.push_back(0.0);
  } else {
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "NULL t_indicator_" << DBGLOG_ENDL_FLUSH;
    }
    ExitVerbose(kExitErrorCodeGeneral, dbglogger_);
  }
  current_indicator_index_++;
}


// Assumption : Only One Object will subscribe to the Model (ExecLogic or IVAdapterType)
void BaseMultipleModelMath::AddListener(MultipleModelMathListener* p_new_model_math_listener_, int modelmath_index_) {
  model_math_listener__ = p_new_model_math_listener_;
  SetModelMathIndex(modelmath_index_);
}
}
