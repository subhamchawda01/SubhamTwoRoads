/**
    \file ExecLogic/exec_interface.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_EXECLOGIC_EXEC_INTERFACE_H
#define BASE_EXECLOGIC_EXEC_INTERFACE_H

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/new_midnight_listener.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "baseinfra/TradeUtils/market_update_manager_listener.hpp"

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"

#include "dvctrade/ModelMath/model_math_listener.hpp"
#include "dvctrade/ModelMath/base_model_math.hpp"

#include "dvccode/ORSMessages/control_message_listener.hpp"
#include "dvctrade/InitCommon/paramset.hpp"

#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"
#include "baseinfra/SmartOrderRouting/pnl_listeners.hpp"

#include "dvctrade/RiskManagement/risk_manager.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

namespace HFSAT {

class ExecInterface : public NewMidnightListener,
                      public PositionChangeListener,
                      public OrderChangeListener,
                      public ExecutionListener,
                      public GlobalPositionChangeListener,
                      public RiskManagerListener,
                      public GlobalPNLChangeListener,
                      public CancelRejectListener,
                      public RejectDueToFundsListener,
                      public FokFillRejectListener,
                      public ControlMessageListener,
                      public SecurityMarketViewChangeListener,
                      public ModelMathListener,
                      public CancelModelListener,
                      public MarketDataInterruptedListener,
                      public SecurityMarketViewStatusListener,
                      public ExchangeRejectsListener,
                      public ORSRejectsFreezeListener {
 public:
  // base variables needed by all strategy implementations, and in implementing the interface methods
  DebugLogger& dbglogger_;
  const Watch& watch_;
  const SecurityMarketView& dep_market_view_;
  SmartOrderManager& order_manager_;
  ParamSet param_set_;
  std::vector<ParamSet> param_set_vec_;
  std::string regime_indicator_string_;
  const bool livetrading_;
  BaseModelMath* p_base_model_math_;
  BaseModelMath* p_base_cancel_model_;

  ExecInterface(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
                SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_)
      : dbglogger_(_dbglogger_),
        watch_(_watch_),
        dep_market_view_(_dep_market_view_),
        order_manager_(_order_manager_),
        param_set_(_paramfilename_, _watch_.YYYYMMDD(), _dep_market_view_.shortcode()),
        param_set_vec_(),
        livetrading_(_livetrading_),
        p_base_model_math_(NULL),
        p_base_cancel_model_(NULL) {
    order_manager_.AddPositionChangeListener(this);
    order_manager_.AddExecutionListener(this);
    order_manager_.AddCancelRejectListener(this);
    order_manager_.AddRejectDueToFundsListener(this);
    order_manager_.AddFokFillRejectListener(this);
    order_manager_.AddExchangeRejectsListeners(this);
    order_manager_.AddORSRejectsFreezeListener(this);
    LoadParamSetVec(_paramfilename_);
  }

  virtual ~ExecInterface() {}

  virtual void ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool conservative_close_ = false) = 0;

  virtual bool UsingCancellationModel() { return false; }
  virtual int my_position() const = 0;
  virtual void get_positions(std::vector<std::string>& _instrument_vec_, std::vector<int>& _positon_vector_) = 0;

  virtual void SetOrderPlacingLogic(const StrategyType _strategy_type_) {}
  virtual void SetModelMathComponent(BaseModelMath* t_base_model_math_) { p_base_model_math_ = t_base_model_math_; }
  virtual void SetCancellationModelComponent(BaseModelMath* t_base_cancel_model_) {
    p_base_cancel_model_ = t_base_cancel_model_;
  }

  virtual void SetStartTrading(bool _set_) {}
  virtual void AddProductToTrade(SecurityMarketView* _dep_market_view_, SmartOrderManager* _order_manager_,
                                 BaseModelMath* _model_math_, std::string paramfilename_, int _trading_start_mfm_,
                                 int _trading_end_mfm_) {}

  void LoadParamSetVec(std::string paramfilename_) {
    std::ifstream paramlistfile_;
    paramlistfile_.open(paramfilename_);
    bool paramset_file_list_read_ = false;
    std::ofstream temp_param_file_;
    int param_indx_ = 0;
    int max_worst_case_pos = 0;  // This is a local variable for finding the max of worst_case position for all
                                 // the regimes, to be notified to ORS, Initialising it with 0.
    srand(time(NULL));
    int rand_id = rand() % 10000;
    // creating temporary param for regime params
    std::string temp_param_filename_ = "/tmp/param_" + std::to_string(rand_id);

    if (paramlistfile_.is_open()) {
      const int kParamfileListBufflerLen = 1024;
      char readlinebuffer_[kParamfileListBufflerLen];
      bzero(readlinebuffer_, kParamfileListBufflerLen);
      while (paramlistfile_.good()) {
        bzero(readlinebuffer_, kParamfileListBufflerLen);
        paramlistfile_.getline(readlinebuffer_, kParamfileListBufflerLen);
        std::string this_line_ = std::string(readlinebuffer_);
        PerishableStringTokenizer st_(readlinebuffer_, kParamfileListBufflerLen);
        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if (tokens_.size() < 1) {
          continue;
        }
        // PARAMFILELIST acts as delimiter, signifies the starting of new params, create a temporary param
        // and start writing the contents to it.
        if (strcmp(tokens_[0], "PARAMFILELIST") == 0) {
          paramset_file_list_read_ = true;
          if (temp_param_file_.is_open()) {
            temp_param_file_.close();
            ParamSet this_paramset_(temp_param_filename_, watch_.YYYYMMDD(), dep_market_view_.shortcode());
            param_set_vec_.push_back(this_paramset_);
            max_worst_case_pos = std::max(max_worst_case_pos, this_paramset_.worst_case_position_);
            remove(temp_param_filename_.c_str());
            param_indx_++;
          }
          temp_param_file_.open(temp_param_filename_);
          paramset_file_list_read_ = true;
        }
        // if regime params , then write to temporary param file , else normal param file
        else if ((strcmp(tokens_[0], "PARAMVALUE") == 0) && paramset_file_list_read_) {
          temp_param_file_ << this_line_;
          temp_param_file_ << "\n";
        } else if ((strcmp(tokens_[0], "PARAMVALUE") == 0) && !paramset_file_list_read_) {
          // This is a single paramfile read it normally
          paramlistfile_.close();
          param_set_ = ParamSet(paramfilename_, watch_.YYYYMMDD(), dep_market_view_.shortcode());
          param_set_vec_.push_back(param_set_);
          max_worst_case_pos = std::max(max_worst_case_pos, param_set_.worst_case_position_);
          break;
        }
        // regime indicator, store the so far created temporary param file
        else if ((strcmp(tokens_[0], "INDICATOR") == 0) && paramset_file_list_read_) {
          if (temp_param_file_.is_open()) {
            temp_param_file_.close();
            ParamSet this_paramset_(temp_param_filename_, watch_.YYYYMMDD(), dep_market_view_.shortcode());
            remove(temp_param_filename_.c_str());
            param_set_vec_.push_back(this_paramset_);
            max_worst_case_pos = std::max(max_worst_case_pos, this_paramset_.worst_case_position_);
          }
          regime_indicator_string_ = this_line_;
        }
      }
      param_set_ = param_set_vec_[param_set_vec_.size() -
                                  1];  // using the last param by default because doing same in basetrading
      if (paramlistfile_.is_open()) {
        // may have been closed in case of normal param
        paramlistfile_.close();
      }
      if (temp_param_file_.is_open()) {
        temp_param_file_.close();
        ParamSet this_paramset_(temp_param_filename_, watch_.YYYYMMDD(), dep_market_view_.shortcode());
        remove(temp_param_filename_.c_str());
        param_set_vec_.push_back(this_paramset_);
        max_worst_case_pos = std::max(max_worst_case_pos, this_paramset_.worst_case_position_);
      }
    } else {
      std::cerr << "ExecInterface::LoadParamSetVec: can't open paramlistfile_: " << paramfilename_ << " for reading\n";
      exit(1);
    }
    order_manager_.NotifyWorstPosToOrs(max_worst_case_pos);  // Notify the ORS worst_case_positions for the strategy
  }
};
}
#endif  // BASE_EXECLOGIC_EXEC_INTERFACE_H
