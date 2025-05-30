/**
    \file ModelMath/md_model_aggregator.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include <assert.h>
#include "dvctrade/ModelMath/base_model_math.hpp"

// we should be able to write delta * price_model + vega * vol__model + gamma*quadratic variance + theta * dt
// in which case we need listen extra indicators to update greeks multipliers
// finally we need to identify subsets implied model / price model and so on so forth through respective modelmath
// let underlying model handle updating the delta model sum_vars, however onsmvready will be handled here, so we can
// convert
// sum_vars into respective price change units ( by multiplying with greek values )
// we have a global_sum_vars = sum ( greek * sum_var_ ) & target_price_ = base_price + global_sum_vars_
namespace HFSAT {
class MDModelAggregator : public BaseModelMath {
 protected:
  /*
  BaseModelMath* delta_model_;
  BaseModelMath* vega_model_;

  CommonIndicator* delta_;
  CommonIndicator* vega_;

  double delta_;
  double vega_;

  double underlying_price_change_;
  double implied_vol_change_;

  double sum_vars_;
  double last_propagated_target_price_;
  */

  // generalized
  std::vector<BaseModelMath*> greek_models_;
  std::vector<CommonIndicator*> greek_indicators_;
  std::vector<double> greek_values_;

  SecurityMarketView& dep_market_view_;

 public:
  MDModelAggregator(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& _model_filename_,
                    SecurityMarketView& _dep_market_view_, PriceType_t _dep_baseprice_type_);

  void SetModelWeightIndicator(CommonIndicator* p_greek_indicator_, bool is_siglr_ = false);
  void SetSubModelMath(BaseModelMath* p_greek_modelmath_);

  // update greeks value
  void OnIndicatorUpdate(const unsigned int& this_indicator_index_, const double& _this_indicator_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  void set_basepx_pxtype();

  // greek indicators ready & model maths ready ?
  bool AreAllReady();

  // reaching signals
  inline void ForceIndicatorReady();
  void OnControlUpdate();

  // modelmath listener to propagate price
  void AddListener();

  void SubscribeMarketInterrupts(MarketUpdateManager& market_update_manager_);

  // after all indicator values are updated, in here we need to compute target_value or target_bias_
  // in our case we need to multiply target/bias of every modelmath with its respective greek
  // if we can add this as listener to modelmath then calc and propagate will be here (target, bias)
  // then we need to call this class calc and propagate to exec logic
  // so exec logic is the listener to this and this is a listener to basemodelmath, so convoluted !
  void SMVOnReady();

  // debug
  void ShowIndicatorValues();
  void DumpIndicatorValues();

  //
  inline void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) {}
  virtual void InterpretModelParameters(const std::vector<const char*> _tokens_) {}
  virtual void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol_, const int trader_id) {}

  virtual ~MDModelAggregator() {}
};
}
