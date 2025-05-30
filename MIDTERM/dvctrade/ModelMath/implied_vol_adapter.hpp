/**
        \file ModelMath/implied_vol_adapter.hpp

        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
 */
#ifndef IMPLIED_VOL_ADAPTER_H
#define IMPLIED_VOL_ADAPTER_H

#include <vector>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/ModelMath/base_multiple_model_math.hpp"
#include "dvctrade/ModelMath/implied_vol_adapter_listener.hpp"
#include "dvctrade/ModelMath/mult_model_creator.hpp"
#include "baseinfra/OptionsUtils/option_object.hpp"

#include "dvccode/Profiler/cpucycle_profiler.hpp"

namespace HFSAT {

typedef enum {
  kMethodBlackScholes,
  kMethodRandom,
} AdapterType_t;

class ImpliedVolAdapter : public MultipleModelMathListener,
                          public SecurityMarketViewChangeListener,
                          public SecurityMarketViewOnReadyListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;

  std::vector<std::vector<double> > prev_value_vec_;
  std::vector<std::vector<double> > prev_iv_vec_;

  ImpliedVolAdapterListener* implied_vol_adapter_listener__;  // ExecLogic listener

  std::vector<SecurityMarketView*> underlying_smv_vec_;
  std::vector<PriceType_t> underlying_pricetye_vec_;
  std::map<std::string, std::vector<SecurityMarketView*> > shc_const_smv_map_;
  std::map<unsigned int, std::pair<int, int> > sec_id_index_map_;
  AdapterType_t conversion_method_;
  int current_sec_id_;

  // Variables need to calculate price from IV

  std::vector<std::vector<OptionObject*> > option_matrix_;

  double last_msecs_time_to_maturity_update_;
  double last_msecs_iv_matrix_propogation_;
  double time_to_deduct_;

 public:
  ImpliedVolAdapter(DebugLogger& _dbglogger_, const Watch& _watch_,
                    std::vector<SecurityMarketView*> _underlying_smv_vec_,
                    std::vector<PriceType_t> _underlying_pricetype_vec_,
                    std::map<std::string, std::vector<SecurityMarketView*> > _shc_const_smv_map_)
      : dbglogger_(_dbglogger_),
        watch_(_watch_),
        prev_value_vec_(),
        prev_iv_vec_(),
        implied_vol_adapter_listener__(),
        underlying_smv_vec_(_underlying_smv_vec_),
        underlying_pricetye_vec_(_underlying_pricetype_vec_),
        shc_const_smv_map_(_shc_const_smv_map_),
        conversion_method_(kMethodBlackScholes),
        current_sec_id_(-1),
        last_msecs_time_to_maturity_update_(0),
        last_msecs_iv_matrix_propogation_(0),
        time_to_deduct_(0) {
    prev_value_vec_.resize(underlying_smv_vec_.size(), std::vector<double>());
    prev_iv_vec_.resize(underlying_smv_vec_.size(), std::vector<double>());

    sec_id_index_map_ = HFSAT::MultModelCreator::GetSecIdMap();

    for (auto i = 0u; i < underlying_smv_vec_.size(); i++) {
      if (!underlying_smv_vec_[i]->subscribe_price_type(this, underlying_pricetye_vec_[i])) {
        PriceType_t t_error_price_type_ = underlying_pricetye_vec_[i];
        std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                  << t_error_price_type_ << std::endl;
      }
      underlying_smv_vec_[i]->subscribe_OnReady(this);
      option_matrix_.push_back(std::vector<OptionObject*>());
      std::vector<SecurityMarketView*> const_smv_list_ = shc_const_smv_map_[(underlying_smv_vec_[i]->shortcode())];
      for (unsigned int j = 0; j < const_smv_list_.size(); j++) {
        prev_value_vec_[i].push_back(0.0);
        prev_iv_vec_[i].push_back(0.0);
        option_matrix_[i].push_back(
            HFSAT::OptionObject::GetUniqueInstance(dbglogger_, watch_, const_smv_list_[j]->shortcode()));
      }
    }
    // We have already subscribed to dep price type of all underlying future on model math
  }

  virtual ~ImpliedVolAdapter() {}

  virtual void AddListener(ImpliedVolAdapterListener* _new_model_math_listener_);
  bool UpdateTarget(double _new_target_, double _new_sum_vars_, int _modelmath_index_, int _product_index_);

  void SetTimeToDeduct(int trading_start_utc_mfm_) {
    time_to_deduct_ = trading_start_utc_mfm_ / (1000 * 3600 * 24);
    time_to_deduct_ = time_to_deduct_ / 365;
    last_msecs_time_to_maturity_update_ = trading_start_utc_mfm_;
  }

  void TargetNotReady(int _modelmath_index_, int _product_index_) {
    PropagateNotReady(_modelmath_index_, _product_index_);
  }
  void SMVOnReady();

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_) {}

 protected:
  inline void PropagateNewTargetPrice(const double& _new_target_price_, const double& _new_sum_vars_,
                                      int _modelmath_index_, int _product_index_) {
#if CCPROFILING_TRADEINIT
    HFSAT::CpucycleProfiler::GetUniqueInstance().End(25);
    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(26);
    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(27);
#endif
    implied_vol_adapter_listener__->UpdateTarget(_new_target_price_, _new_sum_vars_, _modelmath_index_,
                                                 _product_index_);
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      dbglogger_ << "Model Price of Product " << _product_index_ << "of "
                 << underlying_smv_vec_[_modelmath_index_]->shortcode() << " is " << _new_target_price_
                 << DBGLOG_ENDL_FLUSH;
    }
  }

  inline void PropagateNotReady(int _modelmath_index_, int _product_index_) {
    implied_vol_adapter_listener__->TargetNotReady(_modelmath_index_, _product_index_);
  }
};
}
#endif  // IMPLIED_VOL_ADAPTER_H
