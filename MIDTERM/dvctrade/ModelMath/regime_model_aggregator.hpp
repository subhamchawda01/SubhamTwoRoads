// =====================================================================================
//
//       Filename:  regime_model_aggregator.hpp
//
//    Description:  switch between diferent model aggregator
//
//        Version:  1.0
//        Created:  11/04/2014 03:30:57 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include "dvctrade/ModelMath/siglr_model_aggregator.hpp"
#include "dvctrade/ModelMath/linear_model_aggregator.hpp"

namespace HFSAT {

class RegimeModelAggregator : public BaseModelMath {
 protected:
  DebugLogger& dbglogger_;
  std::vector<BaseModelMath*> model_math_vec_;
  unsigned int this_regime_index_;

 public:
  RegimeModelAggregator(DebugLogger& _dbglogger_, const Watch& _watch_, std::string _model_filename_)
      : BaseModelMath(_dbglogger_, _watch_, _model_filename_),
        dbglogger_(_dbglogger_),
        model_math_vec_(),
        this_regime_index_(0) {}

  void SetRegimeIndicator(CommonIndicator* regime_based_indictor_, bool siglr_ = false) {
    p_dep_indep_based_regime_ = regime_based_indictor_;
    p_dep_indep_based_regime_->add_unweighted_indicator_listener(1, this);
  }

  void SetRegimeModelMath(BaseModelMath* _p_base_model_math_, unsigned int _index_) {
    if (model_math_vec_.size() + 1 < _index_) {
      model_math_vec_.resize(_index_);
    } else if (model_math_vec_.size() == _index_) {
      model_math_vec_.push_back(_p_base_model_math_);
    } else {
      std::cerr << " ModelMathVec size " << model_math_vec_.size() << " is more than this indicator_index_" << _index_
                << std::endl;
    }
  }

  void set_basepx_pxtype() {
    for (unsigned i = 0; i < model_math_vec_.size(); i++) {
      model_math_vec_[i]->set_basepx_pxtype();
    }
  }

  void OnIndicatorUpdate(const unsigned int& this_indicator_index_, const double& _this_indicator_value_) {
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_INFO)) {
      DBGLOG_CLASS_FUNC_LINE << " RegimeIndicator Value Update: " << _this_indicator_value_ << " "
                             << this_indicator_index_ << DBGLOG_ENDL_FLUSH;
    }
    if (_this_indicator_value_ > 0) {
      this_regime_index_ = _this_indicator_value_ - 1;
    }
  }

  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  bool AreAllReady() {
    bool ready_ = true;
    for (unsigned i = 0; i < model_math_vec_.size(); i++) {
      ready_ = ready_ && model_math_vec_[i]->is_ready();
    }
    ready_ = ready_ && p_dep_indep_based_regime_->IsIndicatorReady();

    return ready_;
  }

  void InterpretModelParameters(const std::vector<const char*> _tokenxs_) {}

  void SubscribeMarketInterrupts(MarketUpdateManager& market_update_manager_) {
    for (unsigned i = 0; i < model_math_vec_.size(); i++) {
      model_math_vec_[this_regime_index_]->SubscribeMarketInterrupts(market_update_manager_);
    }
  }
  void SMVOnReady() {
    if (!is_ready_) {
      is_ready_ = AreAllReady();
    }

    if (is_ready_) {
      if (this_regime_index_ < model_math_vec_.size())

      {
        model_math_vec_[this_regime_index_]->SMVOnReady();
      }
    }
  }

  void OnGlobalPositionChange(const unsigned int t_security_id_, int _new_global_position_) {
    if (!is_ready_) {
      is_ready_ = AreAllReady();
    }

    if (is_ready_) {
      if (this_regime_index_ < model_math_vec_.size())

      {
        model_math_vec_[this_regime_index_]->SMVOnReady();
      }
    }
  }

  void ShowIndicatorValues() {
    bool ready_ = true;
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_INFO)) {
      DBGLOG_CLASS_FUNC_LINE << "Regime Indicator Value : " << p_dep_indep_based_regime_->indicator_value(ready_)
                             << DBGLOG_ENDL_FLUSH;
      for (unsigned i = 0; i < model_math_vec_.size(); i++) {
        DBGLOG_CLASS_FUNC_LINE << " Indicator Values for Regime Model: " << i + 1 << DBGLOG_ENDL_FLUSH;
        model_math_vec_[i]->ShowIndicatorValues();
      }
    }
  }

  void DumpIndicatorValues() {
    bool ready_ = true;
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_INFO)) {
      DBGLOG_CLASS_FUNC_LINE << "Regime Indicator Value : " << p_dep_indep_based_regime_->indicator_value(ready_)
                             << DBGLOG_ENDL_FLUSH;

      for (unsigned i = 0; i < model_math_vec_.size(); i++) {
        DBGLOG_CLASS_FUNC_LINE << " Indicator Values for Regime Model: " << i + 1 << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        model_math_vec_[i]->ShowIndicatorValues();
      }
    }
  }

  void AddListener(ModelMathListener* new_model_math_listener, int modelmath_index = 0) {
    for (unsigned i = 0; i < model_math_vec_.size(); i++) {
      model_math_vec_[i]->AddListener(new_model_math_listener);
    }
  }

  void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol_, const int trader_id) {
    if (this_regime_index_ < model_math_vec_.size()) {
      model_math_vec_[this_regime_index_]->OnControlUpdate(_control_message_, symbol_,
                                                           trader_id);  // Will need to update for all mdoel maths?
    }
  }

  inline void ForceIndicatorReady(const unsigned int _indicator_index_) {
    unsigned int indicators_so_far_ = 0;
    for (unsigned i = 0; i < model_math_vec_.size(); i++) {
      indicators_so_far_ += model_math_vec_[i]->NumIndicatorsInModel();
      if (indicators_so_far_ >= _indicator_index_) {
        model_math_vec_[i]->ForceIndicatorReady(indicators_so_far_ - _indicator_index_ -
                                                model_math_vec_[i]->NumIndicatorsInModel());
      }
    }
  }
};
}
