/**
   \file ExecLogic/structured_trading_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */
#pragma once

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"
#include "dvctrade/ExecLogic/trading_manager.hpp"
#include "dvctrade/ExecLogic/base_trading.hpp"

typedef unsigned int uint32_t;

#define FAT_FINGER_FACTOR 5

namespace HFSAT {

class StructuredTradingManagerListener {
 public:
  virtual ~StructuredTradingManagerListener(){};
  virtual void UpdateRisk(int _risk_, int margin_risk_) = 0;
  virtual void UpdateMaxGlobalRisk(int _max_global_risk_) = 0;
  virtual SmartOrderManager& order_manager() = 0;
  virtual void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol, const int trader_id) = 0;
  virtual bool SetPositionOffset(int t_position_offset_) = 0;
};

class StructuredTradingManager : public TradingManager,
                                 public SecurityMarketViewChangeListener,
                                 public TimePeriodListener,
                                 public MultBasePNLListener,
                                 public ControlMessageListener {
 protected:
  bool livetrading_;
  std::vector<int> pos_vec_;
  std::vector<double> risk_vec_;
  std::vector<double> dv01_vec_;
  std::vector<double> pc1_risk_vec_;
  std::vector<double> pc2_risk_vec_;
  std::vector<double> pc_risk_vec_;
  std::vector<int> indices_;
  std::vector<std::string> shortcode_vec_;
  std::vector<unsigned int> term_vec_;
  std::vector<double> p_value_vec_;
  std::vector<double> price_vec_;
  std::vector<double> overnight_price_vec_;
  std::vector<double> overnight_position_vec_;
  std::vector<double> pc1_eigen_vec_;
  std::vector<double> pc2_eigen_vec_;
  std::vector<double> stdev_vec_;
  std::vector<unsigned int> first_vertex_index_vec_;
  std::vector<double> first_vertex_weight_vec_;
  std::vector<double> optimal_margin_risk_;
  std::vector<double> optimal_margin_;
  std::vector<std::vector<double> > margin_scenario_matrix_;
  std::vector<std::vector<double> > margin_scenario_pvalue_matrix_;
  std::vector<double> vertex_pvalue_vec_;
  std::vector<double> vertex_prices_vec_;
  std::vector<int> max_global_risk_vec_;
  double pc1_eigen_value_;
  double pc2_eigen_value_;
  unsigned int num_scenario_;
  std::vector<SecurityMarketView*> p_dep_market_view_vec_;
  double margin_;
  std::string trading_structure_;
  std::vector<StructuredTradingManagerListener*> stm_listeners_;
  std::vector<bool> is_liquid_vec_;
  double total_pc1_risk_;
  double pc1_stdev_;
  double total_pc2_risk_;
  double pc2_stdev_;
  double total_pc_risk_;
  bool getting_flat_;

  MultBasePNL* mult_base_pnl_;
  std::string common_paramfilename_;

  int total_pnl_;
  int realized_pnl_;
  int open_unrealized_pnl_;

  // common params
  int max_opentrade_loss_;
  int max_loss_;
  double max_global_risk_;
  int break_msecs_on_max_opentrade_loss_;
  int last_max_opentrade_loss_hit_msecs_;
  int risk_metric_;  // 0 for pc_risk, 1 for margin_risk, 2 for pc1_risk
  bool read_max_opentrade_loss_;
  bool read_max_loss_;
  bool read_break_msecs_on_max_opentrade_loss_;
  bool read_risk_metric_;
  bool read_max_global_risk_;
  double risk_norm_factor_;

  bool adjusted_overnight_pnl_;
  bool start_given_;

 public:
  StructuredTradingManager(DebugLogger& t_dbglogger_, const Watch& _watch_, std::string _structure_,
                           MultBasePNL* _mult_base_pnl_, const bool _livetrading_, std::string _common_paramfilename_)
      : TradingManager(t_dbglogger_, _watch_),
        livetrading_(_livetrading_),
        pos_vec_(),
        risk_vec_(),
        dv01_vec_(),
        pc1_risk_vec_(),
        pc2_risk_vec_(),
        pc_risk_vec_(),
        indices_(),
        shortcode_vec_(),
        price_vec_(),
        overnight_price_vec_(),
        overnight_position_vec_(),
        pc1_eigen_vec_(),
        pc2_eigen_vec_(),
        stdev_vec_(),
        margin_scenario_matrix_(),
        margin_scenario_pvalue_matrix_(),
        vertex_pvalue_vec_(),
        vertex_prices_vec_(),
        max_global_risk_vec_(),
        pc1_eigen_value_(0.0),
        pc2_eigen_value_(0.0),
        num_scenario_(0),
        margin_(0.0),
        trading_structure_(_structure_),
        stm_listeners_(),
        is_liquid_vec_(),
        total_pc1_risk_(0.0),
        pc1_stdev_(0.0),
        total_pc2_risk_(0.0),
        pc2_stdev_(0.0),
        total_pc_risk_(0.0),
        getting_flat_(false),
        mult_base_pnl_(_mult_base_pnl_),
        common_paramfilename_(_common_paramfilename_),
        total_pnl_(0),
        realized_pnl_(0),
        open_unrealized_pnl_(0),
        max_opentrade_loss_(-100000),
        max_loss_(-100000),
        max_global_risk_(0),
        break_msecs_on_max_opentrade_loss_(900000),
        last_max_opentrade_loss_hit_msecs_(0),
        risk_metric_(0),
        read_max_opentrade_loss_(false),
        read_max_loss_(false),
        read_break_msecs_on_max_opentrade_loss_(false),
        read_risk_metric_(false),
        read_max_global_risk_(false),
        risk_norm_factor_(0.0),
        adjusted_overnight_pnl_(false),
        start_given_(!livetrading_) {
    watch_.subscribe_FifteenSecondPeriod(this);
    LoadPC();
    LoadMarginScenario();
    SetVertices();
    ComputeMarginForScenario();
    GetOutrightRisk();
    LoadCommonParams();
    ComputeMaxGlobalRisk();
    mult_base_pnl_->AddListener(this);
  }

  inline void GetFlat() { getting_flat_ = true; }

  inline void ResumeAfterGetFlat() { getting_flat_ = false; }

  inline void AddListener(unsigned int _security_id_, SecurityMarketView* p_dep_market_view_, bool is_liquid_,
                          StructuredTradingManagerListener* p_this_listener_) {
    std::string shortcode_ = p_dep_market_view_->shortcode();
    for (auto i = 0u; i < shortcode_vec_.size(); i++) {
      if (shortcode_.compare(shortcode_vec_[i]) == 0) {
        indices_.push_back(i);
      }
    }
    p_dep_market_view_->ComputeMidPrice();
    p_dep_market_view_->subscribe_price_type(this, kPriceTypeMktSizeWPrice);
    p_dep_market_view_vec_.push_back(p_dep_market_view_);
    stm_listeners_.push_back(p_this_listener_);
    is_liquid_vec_.push_back(is_liquid_);
    p_this_listener_->UpdateMaxGlobalRisk(max_global_risk_vec_[_security_id_]);
  }

  bool AreAllReady() const {
    for (auto i = 0u; i < p_dep_market_view_vec_.size(); i++) {
      if (!p_dep_market_view_vec_[i]->is_ready()) {
        return false;
      }
    }
    return true;
  }

  inline int total_pnl() { return total_pnl_; }

  inline void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol, const int trader_id) {
    switch (_control_message_.message_code_) {
      case kControlMessageCodeGetflat: {
        if (!getting_flat_) {
          if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
            DBGLOG_TIME << "getflat_due_to_external_getflat_ " << trader_id << DBGLOG_ENDL_FLUSH;
            if (livetrading_) {
              DBGLOG_DUMP;
            }
          }
          GetFlat();
          for (auto i = 0u; i < stm_listeners_.size(); i++) {
            stm_listeners_[i]->OnControlUpdate(_control_message_, symbol, trader_id);
          }
        }
      } break;
      case kControlMessageCodeStartTrading: {
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "StartTrading Called " << trader_id << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
        ResumeAfterGetFlat();
        start_given_ = true;

        for (auto i = 0u; i < stm_listeners_.size(); i++) {
          stm_listeners_[i]->OnControlUpdate(_control_message_, symbol, trader_id);
        }

      } break;

      case kControlMessageCodeDumpPositions: {
        DBGLOG_TIME_CLASS << "DumpPositions" << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
        DumpPositions();
      } break;

      case kControlMessageCodeFreezeTrading:
      case kControlMessageCodeUnFreezeTrading:
      case kControlMessageCodeCancelAllFreezeTrading:
      case kControlMessageCodeSetTradeSizes:
      case kControlMessageCodeSetUnitTradeSize:
      case kControlMessageCodeSetMaxUnitRatio:
      case kControlMessageCodeSetMaxPosition:
      case kControlMessageCodeSetWorstCaseUnitRatio:
      case kControlMessageCodeAddPosition:
      case kControlMessageCodeSetWorstCasePosition:
      case kControlMessageCodeDisableImprove:
      case kControlMessageCodeEnableImprove:
      case kControlMessageCodeDisableAggressive:
      case kControlMessageCodeEnableAggressive:
      case kControlMessageCodeShowParams:
      case kControlMessageCodeCleanSumSizeMaps:
      case kControlMessageCodeSetEcoSeverity:
      case kControlMessageCodeForceIndicatorReady:
      case kControlMessageCodeForceAllIndicatorReady:
      case kControlMessageDisableSelfOrderCheck:
      case kControlMessageEnableSelfOrderCheck:
      case kControlMessageDumpNonSelfSMV:
      case kControlMessageCodeEnableAggCooloff:
      case kControlMessageCodeDisableAggCooloff:
      case kControlMessageCodeEnableNonStandardCheck:
      case kControlMessageCodeDisableNonStandardCheck:
      case kControlMessageCodeSetMaxIntSpreadToPlace:
      case kControlMessageCodeSetMaxIntLevelDiffToPlace:
      case kControlMessageCodeSetExplicitMaxLongPosition:
      case kControlMessageCodeSetExplicitWorstLongPosition: {
        for (auto i = 0u; i < stm_listeners_.size(); i++) {
          if (strcmp(_control_message_.strval_1_, shortcode_vec_[indices_[i]].c_str()) == 0) {
            DBGLOG_TIME_CLASS << "User Message for shortcode " << shortcode_vec_[indices_[i]] << DBGLOG_ENDL_FLUSH;
            if (livetrading_) {
              DBGLOG_DUMP;
            }
            stm_listeners_[i]->OnControlUpdate(_control_message_, symbol, trader_id);
          }
        }
      } break;
      case kControlMessageCodeShowIndicators:
      case kControlMessageDisableMarketManager:
      case kControlMessageEnableMarketManager:
      case kControlMessageCodeEnableLogging:
      case kControlMessageCodeDisableLogging:
      case kControlMessageCodeShowOrders:
      case kControlMessageCodeEnableZeroLoggingMode:
      case kControlMessageCodeDisableZeroLoggingMode:
      case kControlMessageCodeSetStartTime:
      case kControlMessageCodeSetEndTime: {
        for (auto i = 0u; i < stm_listeners_.size(); i++) {
          stm_listeners_[i]->OnControlUpdate(_control_message_, symbol, trader_id);
        }
      } break;

      case kControlMessageCodeSetMaxGlobalRisk: {
        if (_control_message_.intval_1_ > max_global_risk_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < max_global_risk_ * FAT_FINGER_FACTOR) {
          max_global_risk_ = _control_message_.intval_1_;
          ComputeMaxGlobalRisk();
          for (auto i = 0u; stm_listeners_.size(); i++) {
            stm_listeners_[i]->UpdateMaxGlobalRisk(max_global_risk_vec_[i]);
          }
        }
      } break;

      case kControlMessageCodeSetMaxLoss: {
        if (_control_message_.intval_1_ > max_loss_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < max_loss_ * FAT_FINGER_FACTOR) {
          max_loss_ = _control_message_.intval_1_;
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxLoss " << trader_id
                            << " called for abs_max_loss_ = " << _control_message_.intval_1_ << " and MaxLoss set to "
                            << max_loss_ << DBGLOG_ENDL_FLUSH;

          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      } break;

      case kControlMessageCodeSetOpenTradeLoss: {
        if (_control_message_.intval_1_ > max_opentrade_loss_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < max_opentrade_loss_ * FAT_FINGER_FACTOR) {
          max_opentrade_loss_ = _control_message_.intval_1_;
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetOpenTradeLoss " << trader_id
                            << " called for abs_opentrade_loss_ = " << _control_message_.intval_1_
                            << " and OpenTradeLoss set to " << max_opentrade_loss_ << DBGLOG_ENDL_FLUSH;

          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      } break;

      case kControlMessageCodeSetBreakMsecsOpenTradeLoss: {
        if (_control_message_.intval_1_ > 0) {
          break_msecs_on_max_opentrade_loss_ = _control_message_.intval_1_;
          break_msecs_on_max_opentrade_loss_ = std::max(1 * 60 * 1000, break_msecs_on_max_opentrade_loss_);
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetBreakMsecsOpenTradeLoss " << trader_id
                            << " called for break_msecs_on_max_opentrade_loss_ = " << _control_message_.intval_1_
                            << " and BreakMsecsOpenTradeLoss set to " << break_msecs_on_max_opentrade_loss_
                            << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      } break;

      default:
        break;
    }
  }

  inline void UpdatePNL(int _total_pnl_) {
    total_pnl_ = _total_pnl_;
    open_unrealized_pnl_ = total_pnl_ - realized_pnl_;
  }

  inline void LoadCommonParams() {
    std::ifstream paramfile_;
    paramfile_.open(common_paramfilename_.c_str(), std::ifstream::in);
    if (paramfile_.is_open()) {
      const int kParamFileLineBufferLen = 1024;
      char readline_buffer_[kParamFileLineBufferLen];
      bzero(readline_buffer_, kParamFileLineBufferLen);

      while (paramfile_.good()) {
        bzero(readline_buffer_, kParamFileLineBufferLen);
        paramfile_.getline(readline_buffer_, kParamFileLineBufferLen);
        PerishableStringTokenizer st_(readline_buffer_, kParamFileLineBufferLen);
        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if (tokens_.size() < 1) continue;

        // look at the second token and depending on string fill in the appropriate variable from the third token
        // example :
        // PARAMVALUE WORST_CASE_POSITION 60 # comments ...
        // PARAMVALUE MAX_POSITION 30 # comments ...
        // PARAMVALUE UNIT_TRADE_SIZE 5 # comments ...
        if ((strcmp(tokens_[0], "PARAMVALUE") == 0) && (tokens_.size() >= 3)) {
          if (strcmp(tokens_[1], "MAX_OPENTRADE_LOSS") == 0) {
            max_opentrade_loss_ = atoi(tokens_[2]);
            read_max_opentrade_loss_ = true;
          } else if (strcmp(tokens_[1], "MAX_LOSS") == 0) {
            max_loss_ = atoi(tokens_[2]);
            read_max_loss_ = true;
          } else if (strcmp(tokens_[1], "BREAK_MSECS_ON_OPENTRADE_LOSS") == 0) {
            break_msecs_on_max_opentrade_loss_ = (int)(std::max(60 * 1000.0, atof(tokens_[2])));
            read_break_msecs_on_max_opentrade_loss_ = true;
          } else if (strcmp(tokens_[1], "RISK_METRIC") == 0) {
            risk_metric_ = atoi(tokens_[2]);
            read_risk_metric_ = true;
          } else if (strcmp(tokens_[1], "MAX_GLOBAL_RISK") == 0) {
            max_global_risk_ = atoi(tokens_[2]);
            read_max_global_risk_ = true;
          } else if (strcmp(tokens_[1], "RISK_NORM_FACTOR") == 0) {
            risk_norm_factor_ = atof(tokens_[2]);
          }
        }
      }
      paramfile_.close();
    } else {
      char tmp_str_[1024] = {0};
      sprintf(tmp_str_, "StructuredTradingManager could not open common paramfile %s", common_paramfilename_.c_str());
      ExitVerbose(kStirExitError, tmp_str_);
    }

    if (!read_max_loss_ || !read_max_opentrade_loss_ || !read_risk_metric_ || !read_max_global_risk_) {
      ExitVerbose(kStirExitError, "common paramfile incomplete");
    }
  }

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& cr_market_update_info_) {
    price_vec_[_security_id_] = cr_market_update_info_.mid_price_;
    if (!adjusted_overnight_pnl_ && start_given_ && AreAllReady()) {
      AdjustAndReportOvernightPNL();
    }
  }

  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& cr_market_update_info_) {}

  inline void UpdatePC1Risk() {
    double t_total_pc1_risk_ = 0.0;
    for (auto i = 0u; i < pos_vec_.size(); i++) {
      t_total_pc1_risk_ += pos_vec_[i] * risk_vec_[i] * pc1_eigen_vec_[i];
    }
    for (auto i = 0u; i < pos_vec_.size(); i++) {
      if (risk_vec_[i] > 0 && pc1_eigen_vec_[i] > 0) {
        pc1_risk_vec_[i] = t_total_pc1_risk_ / (pc1_eigen_vec_[i] * risk_vec_[i]);
      } else {
        pc1_risk_vec_[i] = 0;
      }
      //                DBGLOG_TIME_CLASS_FUNC_LINE << pos_vec_[i] << " " << risk_vec_[i] << " " << pc1_eigen_vec_[i] <<
      //                " " << pc1_risk_vec_[i] << " " << t_total_pc1_risk_ <<  DBGLOG_ENDL_FLUSH;
    }

    for (auto i = 0u; i < pos_vec_.size(); i++) {
      t_total_pc1_risk_ += pos_vec_[i] * risk_vec_[i] * pc1_stdev_ / stdev_vec_[i];
    }

    UpdateOpenUnrealizedPNL(total_pc1_risk_, t_total_pc1_risk_);
    total_pc1_risk_ = t_total_pc1_risk_;
    mult_base_pnl_->UpdateTotalRisk(total_pc1_risk_);
    if (livetrading_) {
      DBGLOG_DUMP;
    }
  }

  inline void UpdateOpenUnrealizedPNL(double last_pc1_risk_, double current_pc1_risk_) {
    if (current_pc1_risk_ * last_pc1_risk_ < 0) {
      realized_pnl_ += open_unrealized_pnl_;
      open_unrealized_pnl_ = 0.0;
    }
  }

  inline void UpdatePC2Risk() {
    total_pc2_risk_ = 0.0;
    for (auto i = 0u; i < pos_vec_.size(); i++) {
      total_pc2_risk_ += pos_vec_[i] * risk_vec_[i] * pc2_eigen_vec_[i];
    }
    for (auto i = 0u; i < pos_vec_.size(); i++) {
      if (risk_vec_[i] > 0 && pc2_eigen_vec_[i] > 0) {
        pc2_risk_vec_[i] = total_pc2_risk_ / (pc2_eigen_vec_[i] * risk_vec_[i]);
      } else {
        pc2_risk_vec_[i] = 0;
      }
    }

    total_pc2_risk_ = 0.0;

    for (auto i = 0u; i < pos_vec_.size(); i++) {
      total_pc2_risk_ += pos_vec_[i] * risk_vec_[i] * pc2_stdev_ / stdev_vec_[i];
    }
  }

  inline void UpdatePCRisk() {
    UpdatePC1Risk();
    UpdatePC2Risk();

    for (auto i = 0u; i < pos_vec_.size(); i++) {
      if (fabs(pc1_eigen_value_ * (total_pc1_risk_ - pc1_eigen_vec_[i] * risk_vec_[i] * pc2_risk_vec_[i])) >
          fabs(pc2_eigen_value_ * (total_pc2_risk_ - pc2_eigen_vec_[i] * risk_vec_[i] * pc1_risk_vec_[i]))) {
        pc_risk_vec_[i] = pc1_risk_vec_[i];
      } else {
        pc_risk_vec_[i] = pc2_risk_vec_[i];
      }
      //		DBGLOG_TIME_CLASS_FUNC_LINE << shortcode_vec_[i] << " " << pos_vec_[i] << " " <<
      // pc1_risk_vec_[i] << " " << pc2_risk_vec_[i] << " " << pc_risk_vec_[i] << " " << risk_vec_[i] <<
      // DBGLOG_ENDL_FLUSH;
    }
    if (livetrading_) {
      DBGLOG_DUMP;
    }
    total_pc_risk_ = (fabs(total_pc1_risk_ * pc1_eigen_value_) + fabs(total_pc2_risk_ * pc2_eigen_value_)) /
                     (pc1_eigen_value_ + pc2_eigen_value_);
  }

  inline double CalculateAbsoluteRisk(std::vector<int> t_pos_vec_) {
    double t_total_abs_risk_ = 0;
    for (auto i = 0u; i < t_pos_vec_.size(); i++) {
      t_total_abs_risk_ += abs(t_pos_vec_[i]) * risk_vec_[i];
    }
    t_total_abs_risk_ *= risk_norm_factor_;
    return t_total_abs_risk_;
  }

  inline void CalculateMarginRisk2() {
    for (unsigned int sid_ = 0; sid_ < pos_vec_.size(); sid_++) {
      std::vector<double> margin_coeff_vec_;
      margin_coeff_vec_.clear();
      std::vector<size_t> mcsi_;

      double absolute_risk_ = CalculateAbsoluteRisk(pos_vec_);

      std::vector<double> scenario_pnl_vec_;
      for (auto i = 0u; i < num_scenario_; i++) {
        double t_scenario_pnl_ = 0.0;
        for (unsigned int j = 0; j < pos_vec_.size(); j++) {
          t_scenario_pnl_ += pos_vec_[j] * first_vertex_weight_vec_[j] *
                             (margin_scenario_pvalue_matrix_[first_vertex_index_vec_[j]][i] -
                              vertex_pvalue_vec_[first_vertex_index_vec_[j]]);
          t_scenario_pnl_ += pos_vec_[j] * (1 - first_vertex_weight_vec_[j]) *
                             (margin_scenario_pvalue_matrix_[first_vertex_index_vec_[j] + 1][i] -
                              vertex_pvalue_vec_[first_vertex_index_vec_[j] + 1]);
        }
        scenario_pnl_vec_.push_back(t_scenario_pnl_ - absolute_risk_);
        scenario_pnl_vec_.push_back(t_scenario_pnl_ - absolute_risk_ +
                                    2 * risk_norm_factor_ * risk_vec_[sid_] * abs(pos_vec_[sid_]));
      }

      for (auto i = 0u; i < num_scenario_; i++) {
        double t_margin_coeff_ = 0.0;
        t_margin_coeff_ +=
            first_vertex_weight_vec_[sid_] * (margin_scenario_pvalue_matrix_[first_vertex_index_vec_[sid_]][i] -
                                              vertex_pvalue_vec_[first_vertex_index_vec_[sid_]]);
        t_margin_coeff_ += (1 - first_vertex_weight_vec_[sid_]) *
                           (margin_scenario_pvalue_matrix_[first_vertex_index_vec_[sid_] + 1][i] -
                            vertex_pvalue_vec_[first_vertex_index_vec_[sid_] + 1]);

        int sign_ = pos_vec_[sid_] > 0 ? 1 : -1;
        margin_coeff_vec_.push_back(t_margin_coeff_ - risk_norm_factor_ * risk_vec_[sid_] * sign_);
        margin_coeff_vec_.push_back(t_margin_coeff_ + risk_norm_factor_ * risk_vec_[sid_] * sign_);
      }

      HFSAT::VectorUtils::signed_index_sort(margin_coeff_vec_, mcsi_);

      for (auto i = 0u; i < margin_coeff_vec_.size(); i++) {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << "Margin coeff for " << shortcode_vec_[sid_] << " for scenario "
                                      << margin_coeff_vec_[i] << " with scenario_pnl " << scenario_pnl_vec_[i]
                                      << " and index " << mcsi_[i] << DBGLOG_ENDL_FLUSH;
        }
      }

      if (livetrading_) {
        DBGLOG_DUMP;
      }

      unsigned int num_vertices_ = 0;

      double min_scenario_pnl_ = scenario_pnl_vec_[mcsi_[0]];
      int last_min_index_ = mcsi_[0];

      for (auto i = 0u; i < margin_coeff_vec_.size(); i++) {
        if (margin_coeff_vec_[i] < 1.001 * margin_coeff_vec_[mcsi_[0]] &&
            margin_coeff_vec_[i] > 0.9999 * margin_coeff_vec_[mcsi_[0]]) {
          if (scenario_pnl_vec_[i] < min_scenario_pnl_) {
            min_scenario_pnl_ = scenario_pnl_vec_[i];
            last_min_index_ = i;
          }
        }
      }

      double last_vertex_position_ = -100000;

      while (num_vertices_ < margin_coeff_vec_.size()) {
        int current_min_index_ = last_min_index_;
        double t_min_position_ = 1000000;
        for (unsigned int j = 0; j < margin_coeff_vec_.size(); j++) {
          if (margin_coeff_vec_[last_min_index_] < 1.0001 * margin_coeff_vec_[j] &&
              margin_coeff_vec_[last_min_index_] > 0.9999 * margin_coeff_vec_[j]) {
            continue;
          } else {
            double t_risk_ = (scenario_pnl_vec_[last_min_index_] - scenario_pnl_vec_[j]) /
                             (margin_coeff_vec_[j] - margin_coeff_vec_[last_min_index_]);
            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              DBGLOG_TIME_CLASS_FUNC_LINE << t_risk_ << " " << t_min_position_ << " " << last_vertex_position_ << " "
                                          << current_min_index_ << DBGLOG_ENDL_FLUSH;
            }
            if (t_risk_ < t_min_position_ && t_risk_ > last_vertex_position_) {
              t_min_position_ = t_risk_;
              current_min_index_ = j;
            }
          }
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << shortcode_vec_[sid_] << " first vertex found " << last_min_index_ << " "
                                      << current_min_index_ << " " << last_vertex_position_ << DBGLOG_ENDL_FLUSH;
        }
        last_min_index_ = current_min_index_;
        last_vertex_position_ = t_min_position_;
        num_vertices_++;

        if (margin_coeff_vec_[last_min_index_] < 0) {
          optimal_margin_risk_[sid_] = -last_vertex_position_;
          std::vector<int> t_pos_vec_ = pos_vec_;
          t_pos_vec_[sid_] = optimal_margin_risk_[sid_];
          optimal_margin_[sid_] = CalculateMarginGivenPos(t_pos_vec_);
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC_LINE << shortcode_vec_[sid_] << " optimal margin position "
                                        << optimal_margin_risk_[sid_] << " with margin " << optimal_margin_[sid_]
                                        << DBGLOG_ENDL_FLUSH;
          }
          break;
        }
      }

      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
  }

  inline void CalculateMarginRisk() {
    // find the max ( min ( scenario_pnls ) ) with  respect to position
    // in case of linear combinations only points of intersection are potential solutions
    // the solution is the point where the slope of the min contour changes from +ve to -ve.

    std::vector<double> scenario_pnl_vec_;
    for (auto i = 0u; i < num_scenario_; i++) {
      double t_scenario_pnl_ = 0.0;
      for (unsigned int j = 0; j < pos_vec_.size(); j++) {
        t_scenario_pnl_ +=
            pos_vec_[j] * first_vertex_weight_vec_[j] * (margin_scenario_pvalue_matrix_[first_vertex_index_vec_[j]][i] -
                                                         vertex_pvalue_vec_[first_vertex_index_vec_[j]]);
        t_scenario_pnl_ += pos_vec_[j] * (1 - first_vertex_weight_vec_[j]) *
                           (margin_scenario_pvalue_matrix_[first_vertex_index_vec_[j] + 1][i] -
                            vertex_pvalue_vec_[first_vertex_index_vec_[j] + 1]);
      }
      scenario_pnl_vec_.push_back(t_scenario_pnl_);
    }

    for (unsigned int sid_ = 0; sid_ < pos_vec_.size(); sid_++) {
      std::vector<double> margin_coeff_vec_;
      margin_coeff_vec_.clear();
      std::vector<size_t> mcsi_;
      for (auto i = 0u; i < num_scenario_; i++) {
        double t_margin_coeff_ = 0.0;
        t_margin_coeff_ +=
            first_vertex_weight_vec_[sid_] * (margin_scenario_pvalue_matrix_[first_vertex_index_vec_[sid_]][i] -
                                              vertex_pvalue_vec_[first_vertex_index_vec_[sid_]]);
        t_margin_coeff_ += (1 - first_vertex_weight_vec_[sid_]) *
                           (margin_scenario_pvalue_matrix_[first_vertex_index_vec_[sid_] + 1][i] -
                            vertex_pvalue_vec_[first_vertex_index_vec_[sid_] + 1]);
        margin_coeff_vec_.push_back(t_margin_coeff_);
      }
      HFSAT::VectorUtils::signed_index_sort(margin_coeff_vec_, mcsi_);

      for (auto i = 0u; i < num_scenario_; i++) {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << "Margin coeff for " << shortcode_vec_[sid_] << " for scenario "
                                      << margin_coeff_vec_[i] << " with scenario_pnl " << scenario_pnl_vec_[i]
                                      << " and index " << mcsi_[i] << DBGLOG_ENDL_FLUSH;
        }
      }

      if (livetrading_) {
        DBGLOG_DUMP;
      }

      unsigned int num_vertices_ = 0;

      double min_scenario_pnl_ = scenario_pnl_vec_[mcsi_[0]];
      int last_min_index_ = mcsi_[0];

      for (auto i = 0u; i < num_scenario_; i++) {
        if (margin_coeff_vec_[i] < 1.001 * margin_coeff_vec_[mcsi_[0]] &&
            margin_coeff_vec_[i] > 0.9999 * margin_coeff_vec_[mcsi_[0]]) {
          if (scenario_pnl_vec_[i] < min_scenario_pnl_) {
            min_scenario_pnl_ = scenario_pnl_vec_[i];
            last_min_index_ = i;
          }
        }
      }

      double last_vertex_position_ = -100000;

      while (num_vertices_ < num_scenario_) {
        int current_min_index_ = last_min_index_;
        double t_min_position_ = 1000000;
        double t_min_slope_ = 1000000;
        for (unsigned int j = 0; j < num_scenario_; j++) {
          if (margin_coeff_vec_[last_min_index_] < 1.0001 * margin_coeff_vec_[j] &&
              margin_coeff_vec_[last_min_index_] > 0.9999 * margin_coeff_vec_[j]) {
            continue;
          } else {
            double t_risk_ = (scenario_pnl_vec_[last_min_index_] - scenario_pnl_vec_[j]) /
                             (margin_coeff_vec_[j] - margin_coeff_vec_[last_min_index_]);
            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              DBGLOG_TIME_CLASS_FUNC_LINE << t_risk_ << " " << t_min_position_ << " " << last_vertex_position_ << " "
                                          << current_min_index_ << DBGLOG_ENDL_FLUSH;
            }
            if ((t_risk_ < (t_min_position_ + 0.01)) && (t_risk_ > last_vertex_position_)) {
              if (t_risk_ < (t_min_position_ - 0.01)) {
                t_min_slope_ = 100000;
              }
              if (margin_coeff_vec_[j] < t_min_slope_) {
                t_min_slope_ = margin_coeff_vec_[j];
                t_min_position_ = t_risk_;
                current_min_index_ = j;
              }
            }
          }
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << shortcode_vec_[sid_] << " first vertex found " << last_min_index_ << " "
                                      << current_min_index_ << " " << last_vertex_position_ << DBGLOG_ENDL_FLUSH;
        }
        last_min_index_ = current_min_index_;
        last_vertex_position_ = t_min_position_;
        num_vertices_++;

        if (margin_coeff_vec_[last_min_index_] < 0) {
          optimal_margin_risk_[sid_] = -last_vertex_position_;
          optimal_margin_[sid_] =
              1.20 * (scenario_pnl_vec_[last_min_index_] + margin_coeff_vec_[last_min_index_] * last_vertex_position_);
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC_LINE << shortcode_vec_[sid_] << " optimal margin position "
                                        << optimal_margin_risk_[sid_] << " with margin " << optimal_margin_[sid_]
                                        << DBGLOG_ENDL_FLUSH;
          }
          break;
        }
      }

      if (livetrading_) {
        DBGLOG_DUMP;
      }
    }
  }

  inline void GetOutrightRisk() {
    for (auto i = 0u; i < pos_vec_.size(); i++) {
      p_value_vec_[i] = CurveUtils::_get_pvalue_(term_vec_[i], price_vec_[i]);
      dv01_vec_[i] = 0.01 * 0.01 * term_vec_[i] * p_value_vec_[i] / (252 * (1 + 0.01 * price_vec_[i]));
      risk_vec_[i] = dv01_vec_[i] * 100 * stdev_vec_[i];
    }
  }

  inline void OnTimePeriodUpdate(const int num_pages_to_add_) { GetOutrightRisk(); }

  inline bool IsHittingMaxLoss() {
    if (total_pnl_ < -max_loss_) {
      return true;
    } else {
      return false;
    }
  }

  inline bool IsHittingOpentradeLoss() {
    if (open_unrealized_pnl_ < -max_opentrade_loss_) {
      last_max_opentrade_loss_hit_msecs_ = watch_.msecs_from_midnight();
      return true;
    } else if (watch_.msecs_from_midnight() - last_max_opentrade_loss_hit_msecs_ < break_msecs_on_max_opentrade_loss_) {
      return true;
    } else {
      return false;
    }
  }

  inline void ComputeMaxGlobalRisk() {
    for (auto i = 0u; i < stdev_vec_.size(); i++) {
      SecurityMarketView* t_dep_market_view_ =
          (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(shortcode_vec_[i]));
      max_global_risk_vec_[i] = t_dep_market_view_->min_order_size() *
                                ((int)(max_global_risk_ / (dv01_vec_[i] * t_dep_market_view_->min_order_size())));
    }
  }

  inline void UpdateMaxGlobalRisk(const unsigned int _security_id_, int _new_max_global_risk_) {
    max_global_risk_vec_[_security_id_] = _new_max_global_risk_;
  }

  inline void OnPositionUpdate(const unsigned int _security_id_, const int _new_position_) {
    pos_vec_[_security_id_] = _new_position_;
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << shortcode_vec_[_security_id_] << " " << pos_vec_[_security_id_]
                                  << DBGLOG_ENDL_FLUSH;
    }
    UpdatePCRisk();
    //    	if ( getting_flat_ )
    { CalculateMarginRisk2(); }
    CalculateMargin();
    NotifyAllListeners();
  }

  inline void CalculateMargin() {
    double t_margin_ = 0.0;
    for (auto i = 0u; i < num_scenario_; i++) {
      double t_scenario_pnl_ = 0.0;
      for (unsigned int j = 0; j < pos_vec_.size(); j++) {
        t_scenario_pnl_ +=
            pos_vec_[j] * first_vertex_weight_vec_[j] * (margin_scenario_pvalue_matrix_[first_vertex_index_vec_[j]][i] -
                                                         vertex_pvalue_vec_[first_vertex_index_vec_[j]]);
        t_scenario_pnl_ += pos_vec_[j] * (1 - first_vertex_weight_vec_[j]) *
                           (margin_scenario_pvalue_matrix_[first_vertex_index_vec_[j] + 1][i] -
                            vertex_pvalue_vec_[first_vertex_index_vec_[j] + 1]);
      }
      if (t_scenario_pnl_ < t_margin_) {
        t_margin_ = t_scenario_pnl_;
      }
    }
    margin_ = 1.20 * t_margin_;  // 1.20 is the empirical constant we found was used

    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << " Margin : " << margin_ << " getting_flat " << getting_flat_ << DBGLOG_ENDL_FLUSH;
    }
    if (livetrading_) {
      DBGLOG_DUMP;
    }
  }

  inline double CalculateMarginGivenPos(std::vector<int> t_pos_vec_) {
    double t_margin_ = 0.0;
    for (auto i = 0u; i < num_scenario_; i++) {
      double t_scenario_pnl_ = 0.0;
      for (unsigned int j = 0; j < t_pos_vec_.size(); j++) {
        t_scenario_pnl_ += t_pos_vec_[j] * first_vertex_weight_vec_[j] *
                           (margin_scenario_pvalue_matrix_[first_vertex_index_vec_[j]][i] -
                            vertex_pvalue_vec_[first_vertex_index_vec_[j]]);
        t_scenario_pnl_ += t_pos_vec_[j] * (1 - first_vertex_weight_vec_[j]) *
                           (margin_scenario_pvalue_matrix_[first_vertex_index_vec_[j] + 1][i] -
                            vertex_pvalue_vec_[first_vertex_index_vec_[j] + 1]);
      }
      if (t_scenario_pnl_ < t_margin_) {
        t_margin_ = t_scenario_pnl_;
      }
    }
    return 1.20 * t_margin_;  // 1.20 is the empirical constant we found was used
  }

  inline void LoadMarginScenario() {
    std::ifstream t_margin_scenario_infile_;
    std::string t_margin_scenario_infilename_;

    int this_YYYYMMDD_ = DateTime::CalcPrevDay(watch_.YYYYMMDD());  // Start yesterday.

    for (unsigned int ii = 0; ii < 40; ii++) {
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << "/spare/local/tradeinfo/StructureInfo/scenarios/scenario." << this_YYYYMMDD_;
      t_margin_scenario_infilename_ = t_temp_oss_.str();

      int t_scenario_file_size_ = 0;

      if (FileUtils::ExistsWithSize(t_margin_scenario_infilename_, t_scenario_file_size_)) {
        if (t_scenario_file_size_ > 0) break;
      } else {
        // Try previous day
        this_YYYYMMDD_ = DateTime::CalcPrevDay(this_YYYYMMDD_);
      }
    }

    int t_scenario_file_size_ = 0;

    if (!FileUtils::ExistsWithSize(t_margin_scenario_infilename_, t_scenario_file_size_)) {  // All attempts failed
      char tmp_str_[1024] = {0};
      sprintf(tmp_str_, "StructuredTradingManager missing margin scenario file %s",
              t_margin_scenario_infilename_.c_str());
      ExitVerbose(kStirExitError, tmp_str_);
    }

    t_margin_scenario_infile_.open(t_margin_scenario_infilename_.c_str());

    if (t_margin_scenario_infile_.is_open()) {
      const int kL1AvgBufferLen = 4096;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);

      num_scenario_ = 0;
      bool first_line_ = true;

      while (t_margin_scenario_infile_.good()) {
        bzero(readline_buffer_, kL1AvgBufferLen);
        t_margin_scenario_infile_.getline(readline_buffer_, kL1AvgBufferLen);
        PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if ((first_line_ || (tokens_.size() >= (num_scenario_ + 2))) && tokens_.size() >= 3) {
          std::vector<double> t_vertex_row_;
          for (unsigned int i = 2; i < tokens_.size(); i++) {
            t_vertex_row_.push_back(atof(tokens_[i]));
            if (first_line_) {
              num_scenario_++;
            }
          }

          margin_scenario_matrix_.push_back(t_vertex_row_);
          margin_scenario_pvalue_matrix_.push_back(std::vector<double>(num_scenario_, 0.0));
          vertex_pvalue_vec_.push_back(0.0);
          vertex_prices_vec_.push_back(atof(tokens_[1]));
        } else {
        }
        first_line_ = false;
      }
      if (num_scenario_ < 2) {
        ExitVerbose(kStirExitError, "not enough scenario...probably empy scenario file");
      }
    } else {
      ExitVerbose(kStirExitError, "could not open margin scenario file");
    }
  }

  inline void SetVertices() {
    for (auto i = 0u; i < shortcode_vec_.size(); i++) {
      unsigned int term_ =
          CurveUtils::_get_term_(watch_.YYYYMMDD(), HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_vec_[i]));
      term_vec_.push_back(term_);
      double t_first_vertex_weight_ = (((double)term_) / 21.0) - (term_ / 21);
      first_vertex_index_vec_.push_back(term_ / 21);
      first_vertex_weight_vec_.push_back(t_first_vertex_weight_);
    }
  }

  inline void ComputeMarginForScenario() {
    for (auto i = 0u; i < margin_scenario_matrix_.size(); i++) {
      for (unsigned int j = 0; j < margin_scenario_matrix_[i].size(); j++) {
        margin_scenario_pvalue_matrix_[i][j] = CurveUtils::_get_pvalue_(21 * (i), margin_scenario_matrix_[i][j]);
      }
      vertex_pvalue_vec_[i] = CurveUtils::_get_pvalue_(21 * (i), vertex_prices_vec_[i]);
    }
  }

  inline void LoadPC() {
    std::ifstream t_structured_trading_risk_matrix_infile_;
    t_structured_trading_risk_matrix_infile_.open("/spare/local/tradeinfo/StructureInfo/structured_trading.txt",
                                                  std::ifstream::in);
    bool matrix_reading_mode_ = false;
    if (t_structured_trading_risk_matrix_infile_.is_open()) {
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);

      while (t_structured_trading_risk_matrix_infile_.good()) {
        bzero(readline_buffer_, kL1AvgBufferLen);
        t_structured_trading_risk_matrix_infile_.getline(readline_buffer_, kL1AvgBufferLen);
        PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if (!matrix_reading_mode_ && tokens_.size() >= 2 && strcmp(tokens_[0], trading_structure_.c_str()) == 0) {
          for (unsigned int i = 1; i < tokens_.size(); i++) {
            shortcode_vec_.push_back(std::string(tokens_[i]));
            pos_vec_.push_back(0);
            risk_vec_.push_back(0);
            dv01_vec_.push_back(0);
            pc1_risk_vec_.push_back(0);
            pc2_risk_vec_.push_back(0);
            pc_risk_vec_.push_back(0);
            price_vec_.push_back(CurveUtils::GetLastDayClosingPrice(watch_.YYYYMMDD(), std::string(tokens_[i])));
            overnight_price_vec_.push_back(0.0);
            overnight_position_vec_.push_back(0);
            optimal_margin_risk_.push_back(0.0);
            optimal_margin_.push_back(0.0);
            p_value_vec_.push_back(0);
            max_global_risk_vec_.push_back(0);
          }
          matrix_reading_mode_ = true;
        }
        if (matrix_reading_mode_ && tokens_.size() > 4 && strcmp(tokens_[0], trading_structure_.c_str()) == 0 &&
            strcmp(tokens_[1], "PC1") == 0) {
          if (tokens_.size() < shortcode_vec_.size() + 4) {
            ExitVerbose(kStirExitError, "not enough tokens");
          }
          pc1_eigen_value_ = atof(tokens_[2]);
          pc1_stdev_ = atof(tokens_[3]);
          for (unsigned int i = 4; i < tokens_.size(); i++) {
            pc1_eigen_vec_.push_back(atof(tokens_[i]));
          }
        }

        if (matrix_reading_mode_ && tokens_.size() > 4 && strcmp(tokens_[0], trading_structure_.c_str()) == 0 &&
            strcmp(tokens_[1], "PC2") == 0) {
          if (tokens_.size() < shortcode_vec_.size() + 4) {
            ExitVerbose(kStirExitError, "not enough tokens");
          }
          pc2_eigen_value_ = atof(tokens_[2]);
          pc2_stdev_ = atof(tokens_[3]);
          for (unsigned int i = 4; i < tokens_.size(); i++) {
            pc2_eigen_vec_.push_back(atof(tokens_[i]));
          }
        }
        if (matrix_reading_mode_ && tokens_.size() > 3 && strcmp(tokens_[0], trading_structure_.c_str()) == 0 &&
            strcmp(tokens_[1], "STDEV") == 0) {
          if (tokens_.size() < shortcode_vec_.size() + 2) {
            ExitVerbose(kStirExitError, "not enough tokens");
          }
          for (unsigned int i = 2; i < tokens_.size(); i++) {
            stdev_vec_.push_back(atof(tokens_[i]));
          }
        }
      }
    }
  }

  inline void NotifyAllListeners() {
    for (auto i = 0u; i < stm_listeners_.size(); i++) {
      int t_margin_risk_ = p_dep_market_view_vec_[i]->min_order_size() *
                           (round(optimal_margin_risk_[indices_[i]] / p_dep_market_view_vec_[i]->min_order_size()));
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME << "NotifyingListeners " << shortcode_vec_[indices_[i]] << " " << optimal_margin_risk_[indices_[i]]
                    << " " << round(optimal_margin_risk_[indices_[i]]) << " " << t_margin_risk_ << DBGLOG_ENDL_FLUSH;
      }
      if (livetrading_) {
        DBGLOG_DUMP;
      }

      if (!p_dep_market_view_vec_[i]->is_ready()) {
        stm_listeners_[i]->UpdateRisk(0, 0);
      } else if (is_liquid_vec_[i]) {
        stm_listeners_[i]->UpdateRisk(
            p_dep_market_view_vec_[i]->min_order_size() *
                (round(((double)pc_risk_vec_[indices_[i]]) / p_dep_market_view_vec_[i]->min_order_size())),
            t_margin_risk_);
      } else {
        stm_listeners_[i]->UpdateRisk(pos_vec_[indices_[i]], t_margin_risk_);
      }
    }
  }

  inline bool IsFlat() {
    for (auto i = 0u; i < pos_vec_.size(); i++) {
      if (pos_vec_[i] != 0) {
        return false;
      }
    }
    return true;
  }

  inline std::string ExitText() {
    std::ostringstream t_oss_;
    for (auto i = 0u; i < pos_vec_.size(); i++) {
      //		if ( pos_vec_[i] != 0 )
      { t_oss_ << "\nwith position " << pos_vec_[i] << " in " << shortcode_vec_[i] << "\n"; }
    }
    return t_oss_.str();
  }

  inline void LoadOvernightPositions() {
    std::ifstream t_overnight_position_infile_;
    std::string t_overnight_position_infilename_;

    int this_YYYYMMDD_ = watch_.YYYYMMDD();

    for (unsigned int ii = 0; ii < 40; ii++) {
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << "/spare/local/tradeinfo/StructureInfo/logs/position." << trading_structure_ << "."
                  << this_YYYYMMDD_;
      t_overnight_position_infilename_ = t_temp_oss_.str();

      if (FileUtils::exists(t_overnight_position_infilename_)) {
        break;
      } else {
        // Try previous day
        this_YYYYMMDD_ = DateTime::CalcPrevDay(this_YYYYMMDD_);
      }
    }

    if (!FileUtils::exists(t_overnight_position_infilename_)) {  // All attempts failed.
      return;
    }

    t_overnight_position_infile_.open(t_overnight_position_infilename_.c_str(), std::ifstream::in);

    if (t_overnight_position_infile_.is_open()) {
      DBGLOG_TIME_CLASS_FUNC_LINE << "Using overnight position file " << t_overnight_position_infilename_
                                  << DBGLOG_ENDL_FLUSH;

      const int kOvernightPositionLineBufferLen = 1024;
      char readline_buffer_[kOvernightPositionLineBufferLen];
      bzero(readline_buffer_, kOvernightPositionLineBufferLen);

      while (t_overnight_position_infile_.good()) {
        bzero(readline_buffer_, kOvernightPositionLineBufferLen);
        t_overnight_position_infile_.getline(readline_buffer_, kOvernightPositionLineBufferLen);
        PerishableStringTokenizer st_(readline_buffer_, kOvernightPositionLineBufferLen);
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if (tokens_.size() >= 3) {
          for (auto i = 0u; i < stm_listeners_.size(); i++) {
            std::string t_shortcode_ = tokens_[0];
            if (t_shortcode_.compare(shortcode_vec_[indices_[i]]) == 0) {
              overnight_price_vec_[indices_[i]] = atof(tokens_[2]);
              overnight_position_vec_[indices_[i]] = atof(tokens_[1]);
            }
          }
        }
      }
      t_overnight_position_infile_.close();
    }
    DBGLOG_DUMP;
  }

  inline void DumpPositions() {
    for (auto i = 0u; i < pos_vec_.size(); i++) {
      //	    if ( pos_vec_[i]!=0 )
      { DBGLOG_TIME_CLASS_FUNC << shortcode_vec_[i] << " " << pos_vec_[i] << "\n"; }
    }
    if (livetrading_) {
      DBGLOG_DUMP;
    }
  }

  inline void SaveEODPositions() {
    std::ostringstream t_oss_;
    t_oss_ << "/spare/local/tradeinfo/StructureInfo/logs/position." << trading_structure_ << "." << watch_.YYYYMMDD();

    std::ofstream position_output_file_(t_oss_.str().c_str(), std::ofstream::out);

    DBGLOG_TIME_CLASS_FUNC_LINE << "Saving EOD positions in file " << t_oss_.str() << DBGLOG_ENDL_FLUSH;

    for (auto i = 0u; i < pos_vec_.size(); i++) {
      position_output_file_ << shortcode_vec_[i] << " " << pos_vec_[i] << " " << price_vec_[i] << "\n";
    }

    position_output_file_.close();
    if (livetrading_) {
      DBGLOG_DUMP;
    }
  }

  inline void PerformExitTasks() {
    SaveEODPositions();
    CalculateMargin();
  }

  inline double TotalMargin() {
    CalculateMargin();
    return margin_;
  }

  inline void AdjustAndReportOvernightPNL() {
    // first add the overnight positions to the query
    adjusted_overnight_pnl_ = true;

    double t_overnight_pnl_ = 0.0;
    bool report_pnl_ = false;
    for (auto i = 0u; i < stm_listeners_.size(); i++) {
      int t_idx_ = indices_[i];
      if (overnight_position_vec_[t_idx_] != 0) {
        double t_n2ds_ = SecurityDefinitions::GetContractNumbersToDollars(shortcode_vec_[t_idx_], watch_.YYYYMMDD());
        t_overnight_pnl_ +=
            overnight_position_vec_[t_idx_] * t_n2ds_ * (price_vec_[t_idx_] - overnight_price_vec_[t_idx_]);
        stm_listeners_[i]->SetPositionOffset(overnight_position_vec_[t_idx_]);
        report_pnl_ = true;
      }
    }

    // now report the overnight pnl as calculated above if needed
    if (report_pnl_) {
      std::string overnight_pnl_email_subject_string_ = "";
      std::string overnight_pnl_email_body_string_ = "";
      {
        std::ostringstream t_oss_;
        t_oss_ << "StructuredStrategy Overnight PNL : " << t_overnight_pnl_ << std::endl;
        overnight_pnl_email_subject_string_ = t_oss_.str();
        t_oss_.clear();
        for (auto i = 0u; i < overnight_position_vec_.size(); i++) {
          if (overnight_position_vec_[i] != 0) {
            t_oss_ << shortcode_vec_[i] << " Position : " << overnight_position_vec_[i]
                   << " Last Px : " << overnight_price_vec_[i] << " Current Px : " << price_vec_[i] << "\n";
          }
        }
        overnight_pnl_email_body_string_ = t_oss_.str();
      }
      if (livetrading_) {
        HFSAT::Email email_;
        email_.setSubject(overnight_pnl_email_subject_string_);
        email_.addRecepient("nseall@tworoads.co.in");
        email_.addSender("nseall@tworoads.co.in");
        email_.content_stream << overnight_pnl_email_body_string_ << "\n";
        email_.sendMail();
      } else {
        DBGLOG_TIME_CLASS_FUNC << overnight_pnl_email_body_string_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  inline void ReportResults(HFSAT::BulkFileWriter& trades_writer_) {
    int t_total_volume_ = 0;
    int t_supporting_orders_filled_ = 0;
    int t_bestlevel_orders_filled_ = 0;
    int t_aggressive_orders_filled_ = 0;
    int t_improve_orders_filled_ = 0;

    for (auto i = 0u; i < stm_listeners_.size(); i++) {
      SmartOrderManager& t_order_manager_ = stm_listeners_[i]->order_manager();
      t_total_volume_ += t_order_manager_.trade_volume();
      t_supporting_orders_filled_ += t_order_manager_.SupportingOrderFilledPercent() * t_order_manager_.trade_volume();
      t_bestlevel_orders_filled_ += t_order_manager_.BestLevelOrderFilledPercent() * t_order_manager_.trade_volume();
      t_improve_orders_filled_ += t_order_manager_.ImproveOrderFilledPercent() * t_order_manager_.trade_volume();
      t_aggressive_orders_filled_ += t_order_manager_.AggressiveOrderFilledPercent() * t_order_manager_.trade_volume();
    }

    if (t_total_volume_ != 0) {
      t_supporting_orders_filled_ /= t_total_volume_;
      t_bestlevel_orders_filled_ /= t_total_volume_;
      t_improve_orders_filled_ /= t_total_volume_;
      t_aggressive_orders_filled_ /= t_total_volume_;
    }

    printf("SIMRESULT %d %d %d %d %d %d\n", total_pnl_, (t_total_volume_), t_supporting_orders_filled_,
           t_bestlevel_orders_filled_, t_aggressive_orders_filled_, t_improve_orders_filled_);
  }
};
}
