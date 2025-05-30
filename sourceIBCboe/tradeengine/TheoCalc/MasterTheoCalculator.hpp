#pragma once

#include "tradeengine/Executioner/BaseExecutioner.hpp"
#include "tradeengine/TheoCalc/BaseTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MidTermTheoCalculator.hpp"
#include "tradeengine/TheoCalc/SquareOffTheoCalculator.hpp"
#include "tradeengine/Utils/BarGenerator.hpp"

class MidTermDetails;
/*! \brief Header file for MasterTheo
*/
class MasterTheoCalculator : public BaseTheoCalculator, public HFSAT::BasePNLListener {
 protected:
  double filter_ptile_;
  double mom_filter_ptile_;
  bool use_obv_filter_;
  bool mr_filter_;
  bool gaprevert_filter_;
  std::vector<double> long_term_volatility_vec_;
  std::vector<double> long_term_obv_vec_;
  MidTermDetails* midterm_detail;
  void LoadParams();
  virtual void UpdateTheoPrices(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);
  void ComputeAndUpdateTheoListeners();
  void UpdateTheoListeners();
  void OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_);

 public:
  MasterTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                       HFSAT::DebugLogger& dbglogger_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_,
                       int _aggressive_get_flat_mfm);

  virtual ~MasterTheoCalculator() {}

  void ConfigureMidTermDetails(std::map<std::string, BaseTheoCalculator*>& theo_map_);
  void SetupPNLHooks();
  virtual int total_pnl() { return total_pnl_; }
  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {}

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                           const HFSAT::MarketUpdateInfo& _market_update_info_) {}

  void OnExec(const int _new_position_, const int _exec_quantity_, const HFSAT::TradeType_t _buysell_,
              const double _price_, const int r_int_price_, const int _security_id_, const int _caos_);

  virtual void OnPNLUpdate(int index_, int t_pnl_, int& t_mult_pnl_, double& t_mult_risk_, int& t_port_pnl_,
                           double& t_port_risk_) {}
  virtual void PNLStats(HFSAT::BulkFileWriter* trades_writer_ = nullptr, bool dump_to_cout = true);

  void SquareOff(bool set_aggressive_ = false) {}

  void NoSquareOff() {}
};

class MidTermDetails {
 public:
  std::vector<MidTermTheoCalculator*> midterm_theo_vec_;
};
