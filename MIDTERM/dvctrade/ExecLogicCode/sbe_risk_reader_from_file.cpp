/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

#include "dvctrade/ExecLogic/sbe_risk_reader_from_file.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {
SBERiskReaderFromFile::SBERiskReaderFromFile(HFSAT::Watch& watch_t, HFSAT::DebugLogger& dbglogger_t,
					     std::string risk_file_t)
  : SBERiskReader(watch_t, dbglogger_t),
    time_to_risk_info_map_(),
    risk_file_(risk_file_t) {
  dbglogger_ << "LoadingAllRiskPath " << watch_t.tv() << "\n";
  LoadRiskPath();
}

void SBERiskReaderFromFile::LoadRiskPath() {
  // open file and see if there are any new risk present
  std::ifstream riskfile_;
  riskfile_.open(risk_file_.c_str(), std::ifstream::in);
  if (riskfile_.is_open()) {
    const int kRiskFileLineBufferLen = 1024;
    char readline_buffer_[kRiskFileLineBufferLen];
    bzero(readline_buffer_, kRiskFileLineBufferLen);

    while (riskfile_.good()) {
      bzero(readline_buffer_, kRiskFileLineBufferLen);
      riskfile_.getline(readline_buffer_, kRiskFileLineBufferLen);
      HFSAT::PerishableStringTokenizer st_(readline_buffer_, kRiskFileLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() < 6)  // assuming format: timestamp Instrument uts strat_id Ref_Px Entry/Exit X Y Z
        continue;
      else if (tokens_[0][0] == '#')
        continue;
      else if ((strcmp(tokens_[5], "Order") != 0) &&
	       (strcmp(tokens_[5], "Position") != 0)) {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "Incomplete order / position entry : " << readline_buffer_ << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      } else {
        uint64_t risk_request_time = strtoul(tokens_[0], NULL, 0);
        risk_request_time = risk_request_time / (1000000000);
        int strat_id = atoi(tokens_[3]);
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << tokens_[0] << "_" << strat_id;  // timestamp_stratID
        RiskInfo_t risk_info;
        risk_info.instrument_ = std::string(tokens_[1]);
        risk_info.strat_id_ = strat_id;
        risk_info.lot_size_ = atoi(tokens_[2]);
        risk_info.ref_px_ = atof(tokens_[4]);
        risk_info.risk_id_ = t_temp_oss_.str();
	if (strcmp(tokens_[5], "Order") == 0) {
	  risk_info.risk_type_ = RiskType_t::kOrderRisk;
	} else if (strcmp(tokens_[5], "Position") == 0) {
	  risk_info.risk_type_ = RiskType_t::kPositionRisk;
	}

        time_to_risk_info_map_.insert(std::make_pair(risk_request_time, risk_info));
        dbglogger_ << "this risk entry " << strtoul(tokens_[0], NULL, 0) << " " << risk_request_time << " "
                   << risk_info.instrument_ << '\n';
      }
    }
    dbglogger_ << "Loaded all risk entries. Total Size: " << time_to_risk_info_map_.size() << '\n';
    dbglogger_.DumpCurrentBuffer();
  }
}

void SBERiskReaderFromFile::OnTimePeriodUpdate(const int num_pages_to_add_) {
  LookupRiskState();
}

void SBERiskReaderFromFile::LookupRiskState() {
  Range_itr current_risk_itr;
  uint64_t current_time = (uint64_t)watch_.tv().tv_sec;
  current_risk_itr = time_to_risk_info_map_.equal_range(current_time);
  if (current_risk_itr.first == current_risk_itr.second) {
    //dbglogger_ << "Risk Reader: No Risk at this time " << watch_.tv() << "\n";
    return;  // no risk at this time
  } else {
    for (risk_info_itr o_itr = current_risk_itr.first; o_itr != current_risk_itr.second;) {
      DBGLOG_TIME_CLASS_FUNC << "Passing risk to execlogic at " << watch_.tv() << " "
                 << o_itr->second.instrument_ << " " << o_itr->second.risk_id_ << DBGLOG_ENDL_FLUSH;
      if (o_itr->second.risk_type_ == RiskType_t::kOrderRisk) {
	NotifyOrderListeners(o_itr->second.instrument_, o_itr->second.risk_id_, o_itr->second.lot_size_,
			     o_itr->second.ref_px_);
      } else if (o_itr->second.risk_type_ == RiskType_t::kPositionRisk) {
	NotifyPositionListeners(o_itr->second.instrument_, o_itr->second.risk_id_, o_itr->second.lot_size_,
				o_itr->second.ref_px_);
      }
      time_to_risk_info_map_.erase(o_itr++);
    }
  }
}

}
