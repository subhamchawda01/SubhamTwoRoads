/**
    \file ExecLogic/options_exec_interface.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_EXECLOGIC_OPTIONS_EXEC_INTERFACE_H
#define BASE_EXECLOGIC_OPTIONS_EXEC_INTERFACE_H

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/TradeUtils/market_update_manager_listener.hpp"
#include "dvccode/ORSMessages/control_message_listener.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"

#include "dvctrade/InitCommon/paramset.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"
#include "dvctrade/ExecLogic/ExecLogicHelper/multiple_indicator_helper.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

#include "dvctrade/ExecLogic/options_exec_vars.hpp"

namespace HFSAT {
class OptionsExecInterface : public PositionChangeListener,
                             public OrderChangeListener,
                             public ExecutionListener,
                             public CancelRejectListener,
                             public ControlMessageListener,
                             public SecurityMarketViewChangeListener,
                             public MarketDataInterruptedListener,
                             public SecurityMarketViewStatusListener,
                             public ModelMathListener {
 public:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  const SecurityMarketView& underlying_market_view_;
  SmartOrderManager& order_manager_;
  ThrottleManager* throttle_manager_;
  std::vector<SecurityMarketView*> options_market_view_vec_;
  std::vector<OptionsExecVars*> options_data_vec_;
  std::vector<SmartOrderManager*> order_manager_vec_;
  OptionsParamSet common_paramset_;
  std::vector<OptionsParamSet> param_set_vec_;
  std::vector<int> sec_id_to_index_;

  MultipleIndicatorHelper* exec_logic_indicators_helper_;
  std::vector<BaseModelMath*> model_math_vec_;
  int map_pos_increment_;
  PositionTradeVarSetMap prod_position_tradevarset_map_;
  const unsigned int P2TV_zero_idx_;
  TradeVars_t closeout_zeropos_tradevarset_;
  TradeVars_t closeout_long_tradevarset_;
  TradeVars_t closeout_short_tradevarset_;
  TradeVars_t current_global_tradevarset_C_;
  TradeVars_t current_global_tradevarset_P_;

  const bool livetrading_;

  std::vector<std::string> products_being_traded_;
  int total_products_trading_;

  OptionsExecInterface(DebugLogger& _dbglogger_, const Watch& _watch_,
                       const SecurityMarketView& _underlying_market_view_, SmartOrderManager& _order_manager_,
                       const std::string& _paramfilename_, const bool _livetrading_)
      : dbglogger_(_dbglogger_),
        watch_(_watch_),
        underlying_market_view_(_underlying_market_view_),
        order_manager_(_order_manager_),
        options_market_view_vec_(),
        options_data_vec_(),
        order_manager_vec_(),
        common_paramset_(_paramfilename_, _watch_.YYYYMMDD(), _underlying_market_view_.shortcode()),
        param_set_vec_(),
        sec_id_to_index_(),
        model_math_vec_(),
        map_pos_increment_(1),
        prod_position_tradevarset_map_(2 * MAX_POS_MAP_SIZE + 1),
        P2TV_zero_idx_(MAX_POS_MAP_SIZE),
        closeout_zeropos_tradevarset_(),
        closeout_long_tradevarset_(),
        closeout_short_tradevarset_(),
        current_global_tradevarset_C_(),
        current_global_tradevarset_P_(),
        livetrading_(_livetrading_),
        products_being_traded_(),
        total_products_trading_(0) {
    LoadParamSetVec(_paramfilename_);
    BuildTradeVarSets();
    current_global_tradevarset_C_ = closeout_zeropos_tradevarset_;
    current_global_tradevarset_P_ = closeout_zeropos_tradevarset_;
  }

  virtual ~OptionsExecInterface() {}

  void LoadParamSetVec(std::string paramfilename_);
  virtual int my_position() const = 0;
  virtual void get_positions(std::vector<std::string>& _instrument_vec_, std::vector<int>& _positon_vector_) = 0;

  void AddProductToTrade(SecurityMarketView* _options_market_view_, SmartOrderManager* _order_manager_,
                         BaseModelMath* _model_math_, std::string paramfilename_, int _trading_start_mfm_,
                         int _trading_end_mfm_);

  void BuildTradeVarSets();
  void BuildConstantTradeVarSets();
  void BuildPositionTradeVarSetMap(OptionsParamSet* param, const SecurityMarketView* smv,
                                   PositionTradeVarSetMap& position_tradevarset_map, int& map_pos_increment,
                                   const int& P2TV_zero_idx, bool livetrading);
  void BuildPosToThreshMap();  // Can be implemented for high pos thresh

  virtual void ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool conservative_close_ = false) = 0;
};
}
#endif
