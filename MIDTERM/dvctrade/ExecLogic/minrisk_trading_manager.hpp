/*
 * spread_trading_manager.hpp
 *
 *  Created on: 12-May-2014
 *      Author: archit
 */

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "dvctrade/ExecLogic/trading_manager.hpp"
#include "baseinfra/MarketAdapter/shortcode_spread_market_view_map.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"
#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"
#include "dvccode/CDef/math_utils.hpp"
#pragma once

namespace HFSAT {
class MinRiskTradingManagerListener {
 public:
  virtual ~MinRiskTradingManagerListener() {}
  virtual void OnPositionUpdate(int beta_pos, int min_risk_pos) = 0;
  virtual int GetSecId() = 0;
  virtual std::string GetShortCode() = 0;
  virtual const SmartOrderManager& order_manager() = 0;
  virtual void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol, const int trader_id) = 0;
};

class MinRiskTradingManager : public TradingManager, public MultBasePNLListener, public ControlMessageListener {
 protected:
  bool is_trading_spreads = false;
  std::vector<std::string> shortcode_vec_;
  std::vector<int> idx_to_pos_map_;
  std::vector<int> outright_pos;
  std::vector<int> min_risk_pos;
  std::vector<double> beta_pos;
  //  OfflineReturnsLRDB& lrdb_;
  std::vector<OfflineReturnsLRDB> lrdb_vec_;
  MultBasePNL* mult_base_pnl_;
  std::vector<std::string> all_instruments;
  std::vector<MinRiskTradingManagerListener*> stm_listeners_;
  double offilnebeta[20][20];
  std::vector<int> secid_to_idx_map_;
  double outright_risk = 0;
  std::vector<double> stdevratio;
  bool getting_flat_ = true;
  double total_pnl_ = 0;
  bool livetrading_;
  double max_loss_ = 0;
  double max_opentrade_loss_ = 0;
  int break_msecs_on_max_opentrade_loss_ = 300 * 1000u;
  int open_unrealized_pnl_ = 0;
  int realized_pnl_ = 0;
  std::string common_paramfilename_ = "";
  std::string structure_file_ = "";
  bool has_read_structure_file_ = false;
  bool read_break_msecs_on_max_opentrade_loss_ = false;
  int last_max_opentrade_loss_hit_msecs_ = 0;
  bool read_max_loss_ = false;
  bool read_max_opentrade_loss_ = false;
  bool forward_visibility_ = false;

 public:
  MinRiskTradingManager(DebugLogger& _dbglogger_, const Watch& _watch_, MultBasePNL* _mult_base_pnl_,
                        std::string _common_paramfilename, bool livetrading_, std::string base_stir_);

  virtual ~MinRiskTradingManager() {}
  int spread_def_ = 1;
  bool AddStmListener(MinRiskTradingManagerListener* _listener_);
  void AttributePositionsInSpreads();
  void UpdateOutrightPosition(int sec_idx, int _new_position_);
  void OnMarketUpdate();
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& cr_market_update_info_);
  void UpdatePNL(int _total_pnl_);
  void LoadCommonParams();
  void UpdateOpenUnrealizedPNL(double last_pc1_risk_, double current_pc1_risk_);
  void DumpPositions() {}
  void ReportResults(HFSAT::BulkFileWriter& trades_writer_);
  void InitialiseBetaValues();
  void UpdateBetaPosition(const unsigned int sec_id, int _new_position_);
  int num_outrights_ = 0;
  std::string base_stir_;
  std::vector<std::vector<int>> spread_outright_map_;
  std::map<std::string, int> shortcode_secidx_map_;
  std::map<int, int> outright_spread_map_;
  void InitialiseStructure();
  virtual std::vector<int> GetOutrightsFromSpread(int sec_idx);
  std::vector<std::string> GetAllInstruments();
  int GetSecurityIndex(std::string shrt_code);
  int GetSpreadFromOutrights(int outright0, int outright1);
  void GetFlat();
  void UpdateOutrightRisk(const unsigned int sec_idx, int pos_change);
  void ResumeAfterGetFlat();
  void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol, const int trader_id);
  void GetFlatTradingLogic();
  bool ShouldBeGettingFlat();
  double total_pnl();
  bool IsHittingMaxLoss();
  bool IsHittingOpentradeLoss();
  inline std::vector<int> getOutrightPos() { return outright_pos; }
  inline std::string getInstrumentShortcode(int i) { return all_instruments[i]; }
};
}

/* namespace HFSAT */
