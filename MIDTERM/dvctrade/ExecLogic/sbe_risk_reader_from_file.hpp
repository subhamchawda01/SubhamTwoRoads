/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

#pragma once
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvctrade/ExecLogic/signal_based_trading.hpp"
#include "dvctrade/ExecLogic/sbe_risk_reader.hpp"

// we like to accomdate two formats 
// risk sent by strategy
// position sent by strategy

typedef enum { kOrderRisk, kPositionRisk } RiskType_t;

struct RiskInfo_t {
  std::string instrument_;
  int strat_id_;
  int lot_size_;
  double ref_px_;
  std::string risk_id_;
  RiskType_t risk_type_;
};

typedef std::multimap<uint64_t, RiskInfo_t>::iterator risk_info_itr;
typedef std::pair<risk_info_itr, risk_info_itr> Range_itr;

namespace HFSAT {
class SBERiskReaderFromFile : public SBERiskReader {
  std::multimap<uint64_t, RiskInfo_t> time_to_risk_info_map_;  // sim mode
  std::string risk_file_;

 public:
  SBERiskReaderFromFile(HFSAT::Watch& watch_t, HFSAT::DebugLogger& dbglogger_t,
			std::string risk_file_t);

  void LoadRiskPath();
  void OnTimePeriodUpdate(const int num_pages_to_add_);
  void LookupRiskState();
};
}
