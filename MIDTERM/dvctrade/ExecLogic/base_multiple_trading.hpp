/**
   \file ExecLogic/base_multiple_trading.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"
#include <sstream>
#include <string>

#include "dvccode/CDef/math_utils.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "dvctrade/OptionsHelper/option_vars.hpp"
#include "dvctrade/RiskManagement/options_risk_manager.hpp"
#include "dvctrade/ModelMath/implied_vol_adapter_listener.hpp"
#include "dvctrade/ModelMath/implied_vol_adapter.hpp"
#include "dvctrade/OptionsHelper/base_risk_premium_logic.hpp"

#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/ORSMessages/shortcode_ors_message_livesource_map.hpp"
#include "baseinfra/VolatileTradingInfo/shortcode_ezone_vec.hpp"
#include "dvctrade/Indicators/core_shortcodes.hpp"
#include "dvctrade/ExecLogic/strategy_options.hpp"

#include "dvccode/CommonTradeUtils/throttle_manager.hpp"

namespace HFSAT {

class BaseMultipleTrading : public ImpliedVolAdapterListener {
 public:
  // not required ?
  static void CollectORSShortCodes(DebugLogger& _dbglogger_, const std::string& _strategy_name_,
                                   const std::vector<std::string>& r__shortcode_,
                                   std::vector<std::string>& source_shortcode_vec_,
                                   std::vector<std::string>& ors_source_needed_vec_);
  // not required ?
  static void CollectShortCodes(DebugLogger& _dbglogger_, const std::string& r_dep_shortcode_,
                                std::vector<std::string>& source_shortcode_vec_);

  BaseMultipleTrading(DebugLogger& _dbglogger_, const Watch& _watch_,
                      std::vector<SecurityMarketView*> _underlying_market_view_vec_,
                      std::map<std::string, std::vector<SmartOrderManager*> > _shc_const_som_map_,
                      OptionsRiskManager* _options_risk_manager_, BaseOptionRiskPremium* _options_risk_premium_,
                      int _trading_start_mfm_, int _trading_end_mfm_, const bool _livetrading_, const int t_runtime_id_,
                      SecurityNameIndexer& _sec_name_indexer_);

  virtual ~BaseMultipleTrading() {}

  static int class_var_counter_;

  static std::string StrategyName() { return "BaseMultipleTrading"; }

 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  int tradingdate_;
  SecurityNameIndexer& sec_name_indexer_;
  int trading_start_utc_mfm_;
  int trading_end_utc_mfm_;
  const int runtime_id_;

  int num_underlying_;

  OptionsRiskManager* options_risk_manager_;     // to access delta's
  BaseOptionRiskPremium* options_risk_premium_;  // to retrieve premium given delta

  std::map<unsigned int, std::pair<int, int> > security_id_prod_idx_map_;

  std::vector<std::vector<OptionVars*> >
      options_data_matrix_;  // nonselfl1data + underlying_smv_ + som + self_smv_ + prod_id + underlying_id
  std::vector<OptionVars*> underlying_data_vec_;
  std::vector<std::vector<OptionRisk> > underlying_to_risk_matrix_;

  std::vector<std::vector<double> > target_vec_;
  std::vector<std::vector<double> > targetbias_numbers_vec_;

  TradeVars_t current_tradevarset_;

  bool cpu_allocated_;

  bool livetrading_;
  bool start_not_given_;           // just to check if start was given once or not in live
  bool is_affined_;                // to avoid affining more than once
  int last_affine_attempt_msecs_;  // to avoid too frequent affining attempts

  std::string pid_file_;
  bool exit_bool_set_;
  bool first_obj_;

  bool print_on_trade_;
  int last_full_logging_msecs_;

  bool zero_logging_mode_;
  bool override_zero_logging_mode_once_for_external_cmd_;

  std::vector<ThrottleManager*> throttle_manager_pvec_;
  bool is_alert_raised_; 

  int improve_keep_cancels_;

 public:
  virtual void SetStartTrading(bool _set_) {
    for (int i = 0; i < num_underlying_; i++) {
      underlying_data_vec_[i]->external_getflat_ = !_set_;
      for (unsigned int j = 0; j < options_data_matrix_[i].size(); j++) {
        options_data_matrix_[i][j]->external_getflat_ = !_set_;
        options_data_matrix_[i][j]->getflat_due_to_external_getflat_ = !_set_;
      }
    }
  }

  bool UpdateTarget(double _target_price_, double _targetbias_numbers_, int _modelmath_index_, int _option_index_);
  void TargetNotReady(int _modelmath_index_, int _product_index_);

  void CallPlaceCancelNonBestLevels(int _product_index_, int _option_index_);
  void PlaceCancelNonBestLevels(int _product_index_, int _option_index_);

  //  void PBATTradingLogic(int _product_index_,int _option_index_) ;
  void TradingLogic(int _product_index_, int _option_index_);

  int GetImproveCancels();
  void PrintFullStatus(int _product_index_, int _option_index_);
  void AllocateCPU() {
    if (is_affined_ || watch_.msecs_from_midnight() - last_affine_attempt_msecs_ < 20000) {
      return;
    }

    std::vector<std::string> affinity_process_list_vec;
    process_type_map process_and_type = AffinityAllocator::parseProcessListFile(affinity_process_list_vec);

    int32_t core_assigned = CPUManager::allocateFirstBestAvailableCore(process_and_type, affinity_process_list_vec,
                                                                       getpid(), "tradeinit", false);
    is_affined_ = (core_assigned >= 0);

    // If we don't get a core even after 5 mins, raise an alert over slack
    // TODO - A lot of the code here has been duplicated across functions and classes,
    // Ideally we should have a parent class for all affinity management
    if (!is_affined_ && (watch_.msecs_from_midnight() - trading_start_utc_mfm_ > 300000)) {
      if (!is_alert_raised_) {
        std::ostringstream t_temp_oss;
        t_temp_oss << "ALERT : UNABLE TO ALLOCATE A CORE TO THE TRADEINIT WITH PID : " << getpid()
                   << " QID : " << runtime_id_;
        DBGLOG_CLASS_FUNC_LINE_ERROR << t_temp_oss.str() << DBGLOG_ENDL_FLUSH;
        is_alert_raised_ = true;
      }
    }

    last_affine_attempt_msecs_ = watch_.msecs_from_midnight();
    DBGLOG_CLASS_FUNC_LINE_INFO << " AFFINED TO : " << core_assigned << " PID : " << getpid() << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }
};
}
