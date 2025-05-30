/*
  \file dvctrade/ExecLogic/curve_trading_manager.hpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
  Address:
  Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
 */

#pragma once

#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"

#include "dvctrade/ExecLogic/curve_trading_manager_listener.hpp"
#include "dvctrade/ExecLogic/trading_manager.hpp"

#define FAT_FINGER_FACTOR 5

namespace HFSAT {

class CurveTradingManager : public ControlMessageListener {
 public:
  virtual ~CurveTradingManager() {}

  virtual void ResetCancelFlag() {}
  virtual void UpdateRiskPosition() {}
  virtual void ReportResults(HFSAT::BulkFileWriter &_trades_writer_) = 0;

  virtual bool MaxLossReached() = 0;

  virtual int MaxGlobalRisk() = 0;

  virtual double CombinedRisk() = 0;

  virtual bool MaxOpentradeLossReached() = 0;

  virtual double RecomputeSignal(double current_sumvars, int t_security_id) = 0;

  virtual int total_pnl() = 0;
  virtual int exposed_pnl() = 0;
  virtual std::string SavePositionsAndCheck() = 0;

  virtual int ComputeGetFlatPositions(int security_id, bool force_compute) = 0;

  virtual void AddListener(unsigned _security_id_, CurveTradingManagerListener *p_listener_) = 0;

  virtual void OnPositionUpdate(int _new_position_, int _position_diff_, unsigned int _security_id_) = 0;
  virtual void UpdateOutrightRisk(unsigned _security_id_, int new_position_) = 0;
  virtual void OnControlUpdate(const ControlMessage &_control_message_, const char *symbol, const int trader_id) = 0;
};
}
