/**
    \file ModelMathCode/base_model_math.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/ModelMath/base_model_math.hpp"

namespace HFSAT {

void BaseModelMath::set_start_end_mfm() {
  for (auto i = 0u; i < indicator_vec_.size(); i++) {
    indicator_vec_[i]->set_start_mfm(t_trading_start_utc_mfm_);
    indicator_vec_[i]->set_end_mfm(t_trading_end_utc_mfm_);
  }
}

void BaseModelMath::AddIndicator(CommonIndicator* _this_indicator_, const double& _this_weight_,
                                 bool _readiness_required_) {
  if (_this_indicator_ != nullptr) {
    _this_indicator_->add_indicator_listener(indicator_vec_.size(), this, _this_weight_);
    indicator_vec_.push_back(_this_indicator_);

    const bool t_is_this_indicator_ready_ = _this_indicator_->IsIndicatorReady();
    if (t_is_this_indicator_ready_ == false) {
      is_ready_vec_.push_back(false);
      readiness_required_vec_.push_back(_readiness_required_);
      is_ready_ = false;  // since is_ready_vec_ is now sure not to be false;
    } else {
      is_ready_vec_.push_back(true);
      readiness_required_vec_.push_back(false);
    }
  } else {
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "nullptr t_indicator_" << DBGLOG_ENDL_FLUSH;
    }
    ExitVerbose(kExitErrorCodeGeneral, dbglogger_);
  }
}

void BaseModelMath::AddIndicator(CommonIndicator* _this_indicator_, const double& _this_weight_decrease_,
                                 const double& _this_weight_nochange_, const double& _this_weight_increase_,
                                 bool _readiness_required_) {
  if (_this_indicator_ != nullptr) {
    _this_indicator_->add_indicator_listener(indicator_vec_.size(), this, _this_weight_decrease_,
                                             _this_weight_nochange_, _this_weight_increase_);
    indicator_vec_.push_back(_this_indicator_);

    const bool t_is_this_indicator_ready_ = _this_indicator_->IsIndicatorReady();
    if (t_is_this_indicator_ready_ == false) {
      is_ready_vec_.push_back(false);
      readiness_required_vec_.push_back(_readiness_required_);
      is_ready_ = false;  // since is_ready_vec_ is now sure not to be false;
    } else {
      is_ready_vec_.push_back(true);
      readiness_required_vec_.push_back(false);
    }
  } else {
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "nullptr t_indicator_" << DBGLOG_ENDL_FLUSH;
    }
    ExitVerbose(kExitErrorCodeGeneral, dbglogger_);
  }
}

void BaseModelMath::AddListener(ModelMathListener* p_new_model_math_listener_, int modelmath_index_) {
  if (single_model_math_listener__ == nullptr) {
    if (model_math_listener_vec_.empty()) {
      single_model_math_listener__ = p_new_model_math_listener_;
    } else {
      VectorUtils::UniqueVectorAdd(model_math_listener_vec_, p_new_model_math_listener_);
    }
  } else {  // probably never expect to be here
    VectorUtils::UniqueVectorAdd(model_math_listener_vec_, single_model_math_listener__);
    VectorUtils::UniqueVectorAdd(model_math_listener_vec_, p_new_model_math_listener_);
    single_model_math_listener__ = nullptr;
  }
  SetModelMathIndex(modelmath_index_);
}

void BaseModelMath::AddCancellationListener(CancelModelListener* p_new_cancel_model_listener_) {
  if (single_cancel_model_listener__ == nullptr) {
    if (cancel_model_listener_vec_.empty()) {
      single_cancel_model_listener__ = p_new_cancel_model_listener_;
    } else {
      VectorUtils::UniqueVectorAdd(cancel_model_listener_vec_, p_new_cancel_model_listener_);
    }
  } else {
    VectorUtils::UniqueVectorAdd(cancel_model_listener_vec_, single_cancel_model_listener__);
    VectorUtils::UniqueVectorAdd(cancel_model_listener_vec_, p_new_cancel_model_listener_);
    single_cancel_model_listener__ = nullptr;
  }
}

void BaseModelMath::SetStrategyType(std::string _strategy_string_) {
  if (_strategy_string_.compare("DirectionalAggressiveTrading") == 0) {
    this_strategy_type_ = kDirectionalAggressiveTrading;
  } else if (_strategy_string_.compare("PriceBasedTrading") == 0) {
    this_strategy_type_ = kPriceBasedTrading;
  } else if (_strategy_string_.compare("PriceBasedAggressiveTrading") == 0) {
    this_strategy_type_ = kPriceBasedAggressiveTrading;
  } else if (_strategy_string_.compare("PriceBasedSecurityAggressiveTrading") == 0) {
    this_strategy_type_ = kPriceBasedSecurityAggressiveTrading;
  } else if (_strategy_string_.compare("PriceBasedSecurityAggressiveTrading") == 0) {
    this_strategy_type_ = kPriceBasedSecurityAggressiveTrading;
  } else if (_strategy_string_.compare("PricePairBasedAggressiveTrading") == 0) {
    this_strategy_type_ = kPricePairBasedAggressiveTrading;
  } else if (_strategy_string_.compare("TradeBasedAggressiveTrading") == 0) {
    this_strategy_type_ = kTradeBasedAggressiveTrading;
  } else if (_strategy_string_.compare("PriceBasedScalper") == 0) {
    this_strategy_type_ = kPriceBasedScalper;
  } else if (_strategy_string_.compare("PriceBasedAggressiveScalper") == 0) {
    this_strategy_type_ = kPriceBasedAggressiveScalper;
  } else if (_strategy_string_.compare("PriceBasedVolTrading") == 0) {
    this_strategy_type_ = kPriceBasedVolTrading;
  } else if (_strategy_string_.compare("DirectionalInterventionAggressiveTrading") == 0) {
    this_strategy_type_ = kDirectionalInterventionAggressiveTrading;
  } else if (_strategy_string_.compare("DirectionalInterventionLogisticTrading") == 0) {
    this_strategy_type_ = kDirectionalInterventionLogisticTrading;
  } else if (_strategy_string_.compare("DirectionalLogisticTrading") == 0) {
    this_strategy_type_ = kDirectionalLogisticTrading;
  } else if (_strategy_string_.compare("DirectionalPairAggressiveTrading") == 0) {
    this_strategy_type_ = kDirectionalPairAggressiveTrading;
  } else if (_strategy_string_.compare("DesiredPostionTrading") == 0) {
    this_strategy_type_ = kDesiredPositionTrading;
  } else if (_strategy_string_.compare("ReturnsBasedAggressiveTrading") == 0) {
    this_strategy_type_ = kReturnsBasedAggressiveTrading;
  } else {
    this_strategy_type_ = kPriceBasedAggressiveTrading;
  }
}
}
