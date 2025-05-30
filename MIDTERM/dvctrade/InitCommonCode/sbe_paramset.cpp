/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

#include "dvctrade/InitCommon/sbe_paramset.hpp"
#include <math.h> // fabs

namespace HFSAT {
SBEParamSet::SBEParamSet(const std::string& _paramfilename_)  // set default param values
    : avg_sprd_(1),
      agg_sprd_factor_(1),
      instrument_(""),
      trade_cooloff_interval_(1000),     // 1 secs
      market_participation_factor_(0.1),  // percentage of traded volume
      market_participation_factor_tolerance_(0.05),
      exec_algo_(kPassAndAgg),
      l1size_limit_per_trade_(0.2),
      max_lots_(-1),
      max_position_(-1),
      use_nonbest_support_(false),
      start_time_msecs_(0),
      end_time_msecs_(0),
      yyyymmdd_(20151010) {
  ParseParamFile(_paramfilename_);
}

void SBEParamSet::ParseParamFile(const std::string& _paramfilename_) {
  std::ifstream paramfile_;
  paramfile_.open(_paramfilename_.c_str(), std::ifstream::in);
  if (paramfile_.is_open()) {
    const int kParamFileLineBufferLen = 1024;
    char readline_buffer_[kParamFileLineBufferLen];
    bzero(readline_buffer_, kParamFileLineBufferLen);

    while (paramfile_.good()) {
      bzero(readline_buffer_, kParamFileLineBufferLen);
      paramfile_.getline(readline_buffer_, kParamFileLineBufferLen);
      char* param_name_ = strtok(readline_buffer_, " \t\n");
      if (param_name_ == NULL) continue;
      char* param_value_ = strtok(NULL, " \t\n");
      if (strcmp(param_name_, "AVG_SPREAD") == 0) {
        avg_sprd_ = atof(param_value_);
      } else if (strcmp(param_name_, "AGG_SPREAD_FACTOR") == 0) {
        agg_sprd_factor_ = atof(param_value_);
      } else if (strcmp(param_name_, "INSTRUMENT") == 0) {
        instrument_ = param_value_;
      } else if (strcmp(param_name_, "COOLOFF_MSECS") == 0) {
        trade_cooloff_interval_ = atoi(param_value_);
      } else if (strcmp(param_name_, "PARTICIPATION_FACTOR") == 0) {
        market_participation_factor_ = atof(param_value_);
      } else if (strcmp(param_name_, "PARTICIPATION_FACTOR_TOLERANCE") == 0) {
        market_participation_factor_tolerance_ = atof(param_value_);
      } else if (strcmp(param_name_, "EXEC_ALGO") == 0) {
        if (strcmp(param_value_, "AGGONLY") == 0) {
          exec_algo_ = kAggOnly;
        } else if (strcmp(param_value_, "PASSANDAGG") == 0) {
          exec_algo_ = kPassAndAgg;
        } else if (strcmp(param_value_, "PASSONLY") == 0) {
          exec_algo_ = kPassOnly;
        } else {
          std::cerr << "Wrong Exec Algo provided \n";
        }
      } else if (strcmp(param_name_, "MAX_LOTS") == 0) {
        max_lots_ = atoi(param_value_);
      } else if (strcmp(param_name_, "USE_NONBEST_SUPPORT") == 0) {
	use_nonbest_support_ = (atoi(param_value_) > 0 ? true : false);
      } else if (strcmp(param_name_, "L1PERC_PER_TRADE") == 0) {
        l1size_limit_per_trade_ = atof(param_value_);
      } else if (strcmp(param_name_, "START_TIME_MSECS") == 0) {
        start_time_msecs_ = atoi(param_value_);
      } else if (strcmp(param_name_, "END_TIME_MSECS") == 0) {
        end_time_msecs_ = atoi(param_value_);
      } else if (strcmp(param_name_, "DATE") == 0) {
        yyyymmdd_ = atoi(param_value_);
      }
    }
  }
}

void SBEParamSet::SetSizeFactors(int _min_order_size_) {
  max_position_ = max_lots_ * _min_order_size_;
}

std::string SBEParamSet::ToString() {
  std::ostringstream t_retval_;
  t_retval_ << "AVG_SPREAD: " << avg_sprd_ << '\n';
  t_retval_ << "AGG_SPRD_FACTOR: " << agg_sprd_factor_ << '\n';
  t_retval_ << "INSTRUMENT: " << instrument_ << '\n';
  t_retval_ << "COOLOFF_INTERVAL: " << trade_cooloff_interval_ / 1000 << " secs\n";
  t_retval_ << "PARTICIPATION: " << market_participation_factor_ * 100.0 << " %\n";
  t_retval_ << "START_MSECS: " << start_time_msecs_ << '\n';
  t_retval_ << "END_MSECS: " << end_time_msecs_ << '\n';
  t_retval_ << "DATE: " << yyyymmdd_ << '\n';
  t_retval_ << "MAX_POSITION: " << max_position_ << '\n';
  t_retval_ << "L1PERC_PER_TRADE: "  << l1size_limit_per_trade_ << '\n';
  return t_retval_.str();
}
}
